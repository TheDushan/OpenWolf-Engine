////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2008 - 2011  Mathieu Olivier
// Copyright(C) 2011 - 2021 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of OpenWolf.
//
// OpenWolf is free software; you can redistribute it
// and / or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3 of the License,
// or (at your option) any later version.
//
// OpenWolf is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenWolf; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110 - 1301  USA
//
// -------------------------------------------------------------------------------------
// File name:   servers.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: Server list and address mapping management for owauthserver
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include "common.hpp"
#include "system.hpp"
#include "servers.hpp"

// ---------- Constants ---------- //

// Timeout for a newly added server (in seconds)
#define TIMEOUT_HEARTBEAT   2


// ---------- Private variables ---------- //

// All server structures are allocated in one block in the "servers" array.
// Each used slot is also part of a linked list in "hash_table". A simple
// hash of the address of a server gives its index in the table.
static server_t *servers = NULL;
static unsigned int max_nb_servers = DEFAULT_MAX_NB_SERVERS;
static unsigned int nb_servers = 0;
static user_hash_table_t hash_table;
static size_t sv_hash_size = DEFAULT_SV_HASH_SIZE;

static unsigned int max_per_address = DEFAULT_MAX_NB_SERVERS_PER_ADDRESS;

// Used to speed up the server allocation / deallocation process
static int last_used_slot = -1;  // -1 = no used slot
static int first_free_slot = 0;  // -1 = no more room

// Variables for Sv_GetFirst, Sv_GetNext and Sv_Remove
static int crt_server_ind = -1;
static int last_server_ind = -1;

// List of address mappings. They are sorted by "from" field (IP, then port)
static addrmap_t *addrmaps = NULL;


// ---------- Public variables ---------- //

// Are servers talking from a loopback interface allowed?
bool allow_loopback = false;


// ---------- Private functions ---------- //

/*
====================
Sv_Remove

Remove a server from the lists
====================
*/
static void Sv_Remove(server_t *sv) {
    int sv_ind;

    Com_UserHashTable_Remove(&sv->user);

    // Mark this structure as "free"
    sv->state = sv_state_unused_slot;

    // Update first_free_slot if necessary
    sv_ind = (int)(sv - servers);
    assert(sv_ind >= 0);
    assert(sv_ind <= last_used_slot);

    if(first_free_slot == -1 || sv_ind < first_free_slot) {
        first_free_slot = sv_ind;
    }

    // If it was the last used slot, look for the previous one
    if(last_used_slot == sv_ind)
        do {
            last_used_slot--;
        } while(last_used_slot >= 0 &&
                servers[last_used_slot].state == sv_state_unused_slot);

    // If we have removed the end of the server iteration, set it to the new end of the list
    if(last_server_ind > last_used_slot) {
        last_server_ind = last_used_slot;
    }

    // Same thing for the current iteration value
    if(crt_server_ind > last_used_slot) {
        crt_server_ind = last_used_slot;
    }

    nb_servers--;
    Com_Printf(MSG_NORMAL,
               "> %s timed out; %u server(s) currently registered\n",
               Sys_SockaddrToString(&sv->user.address, sv->user.addrlen), nb_servers);

    assert(last_used_slot >= (int)nb_servers - 1);
}


/*
====================
Sv_IsActive

Return true if a server is active.
Test if the server has timed out and remove it if it's the case.
====================
*/
static bool Sv_IsActive(unsigned int sv_ind) {
    server_t *sv = &servers[sv_ind];

    assert(sv_ind < max_nb_servers);

    // If the entry isn't even used
    if(sv->state == sv_state_unused_slot) {
        return false;
    }

    assert(sv->gamename[0] != '\0' || sv->state == sv_state_uninitialized);

    // If the server has timed out
    if(sv->timeout < crt_time) {
        Sv_Remove(sv);
        return false;
    }

    return true;
}


/*
====================
Sv_GetByAddr_Internal

Search for a particular server in the list
====================
*/
static server_t *Sv_GetByAddr_Internal(const struct sockaddr_storage
                                       *address, unsigned int *same_address_found) {
    unsigned int hash = Com_AddressHash(address, sv_hash_size);
    server_t *sv;
    bool (*IsSameAddress)(const struct sockaddr_storage * addr1,
                          const struct sockaddr_storage * addr2, bool * same_public_address);

    if(address->ss_family == AF_INET6) {
        IsSameAddress = &Com_SameIPv6Addr;
    } else {
        assert(address->ss_family == AF_INET);
        IsSameAddress = &Com_SameIPv4Addr;
    }

    sv = (server_t *)hash_table.entries[hash];

    *same_address_found = 0;

    while(sv != NULL) {
        server_t *next_sv = (server_t *)sv->user.next;
        unsigned int sv_ind = (unsigned int)(sv - servers);

        if(Sv_IsActive(sv_ind)) {
            const struct sockaddr_storage *sv_address = &sv->user.address;

            if(address->ss_family == sv_address->ss_family) {
                // Same address?
                bool same_public_address;
                bool same_address;

                same_public_address = false;
                same_address = IsSameAddress(sv_address, address, &same_public_address);

                if(same_public_address) {
                    *same_address_found += 1;
                }

                if(same_address) {
                    // Move it on top of the list (it's useful because heartbeats
                    // are almost always followed by infoResponses)
                    Com_UserHashTable_Remove(&sv->user);
                    Com_UserHashTable_Add(&hash_table, &sv->user, hash);

                    return sv;
                }
            }
        }

        sv = next_sv;
    }

    return NULL;
}


/*
====================
Sv_CheckTimeouts

Browse the server list and remove all the servers that have timed out
====================
*/
static void Sv_CheckTimeouts(void) {
    int ind;

    for(ind = 0; ind <= last_used_slot; ind++) {
        Sv_IsActive(ind);
    }
}


/*
====================
Sv_ResolveIPv4Addr

Resolve an internet address
name may include a port number, after a ':'
====================
*/
static bool Sv_ResolveIPv4Addr(const char *name,
                               struct sockaddr_in *addr) {
    char *namecpy, *port;
    struct hostent *host;

    // Create a work copy
    namecpy = strdup(name);

    if(namecpy == NULL) {
        Com_Printf(MSG_ERROR,
                   "> ERROR: can't allocate enough memory to resolve %s\n",
                   name);
        return false;
    }

    // Find the port in the address
    port = strchr(namecpy, ':');

    if(port != NULL) {
        *port++ = '\0';
    }

    // Resolve the address
    host = gethostbyname(namecpy);

    if(host == NULL) {
        Com_Printf(MSG_ERROR, "> ERROR: can't resolve %s\n", namecpy);
        free(namecpy);
        return false;
    }

    if(host->h_addrtype != AF_INET) {
        Com_Printf(MSG_ERROR, "> ERROR: %s is not an IPv4 address\n",
                   namecpy);
        free(namecpy);
        return false;
    }

    // Build the structure
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    memcpy(&addr->sin_addr.s_addr, host->h_addr,
           sizeof(addr->sin_addr.s_addr));

    if(port != NULL) {
        char *end_ptr;
        unsigned short port_num;

        port_num = (unsigned short)strtol(port, &end_ptr, 0);

        if(end_ptr == port || *end_ptr != '\0' || port_num == 0) {
            Com_Printf(MSG_ERROR, "> ERROR: %s is not a valid port number\n",
                       port);
            free(namecpy);
            return false;
        }

        addr->sin_port = htons(port_num);
    }

    Com_Printf(MSG_DEBUG, "> \"%s\" resolved to %s:%hu\n",
               name, inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));

    free(namecpy);
    return true;
}


/*
====================
Sv_InsertAddrmapIntoList

Insert an addrmap structure to the addrmaps list
====================
*/
static bool Sv_InsertAddrmapIntoList(addrmap_t *new_map) {
    addrmap_t *addrmap = addrmaps;
    addrmap_t **prev = &addrmaps;
    char from_addr [16];

    // Stop at the end of the list, or if the addresses become too high
    while(addrmap != NULL &&
            addrmap->from.sin_addr.s_addr <= new_map->from.sin_addr.s_addr) {
        // If we found the right place
        if(addrmap->from.sin_addr.s_addr == new_map->from.sin_addr.s_addr &&
                addrmap->from.sin_port >= new_map->from.sin_port) {
            // If a mapping is already recorded for this address
            if(addrmap->from.sin_port == new_map->from.sin_port) {
                Com_Printf(MSG_ERROR,
                           "> ERROR: several mappings are declared for address %s:%hu\n",
                           inet_ntoa(new_map->from.sin_addr),
                           ntohs(new_map->from.sin_port));
                return false;
            }

            break;
        }

        prev = &addrmap->next;
        addrmap = addrmap->next;
    }

    // Insert it
    new_map->next = *prev;
    *prev = new_map;

    strncpy(from_addr, inet_ntoa(new_map->from.sin_addr),
            sizeof(from_addr) - 1);
    from_addr[sizeof(from_addr) - 1] = '\0';
    Com_Printf(MSG_NORMAL,
               "> Address \"%s\" (%s:%hu) mapped to \"%s\" (%s:%hu)\n",
               new_map->from_string,
               from_addr, ntohs(new_map->from.sin_port),
               new_map->to_string,
               inet_ntoa(new_map->to.sin_addr), ntohs(new_map->to.sin_port));

    return true;
}


/*
====================
Sv_GetAddrmap

Look for an address mapping corresponding to addr
====================
*/
static const addrmap_t *Sv_GetAddrmap(const struct sockaddr_in *addr) {
    const addrmap_t *addrmap = addrmaps;
    const addrmap_t *found = NULL;

    // Stop at the end of the list, or if the addresses become too high
    while(addrmap != NULL &&
            addrmap->from.sin_addr.s_addr <= addr->sin_addr.s_addr) {
        // If it's the right address
        if(addrmap->from.sin_addr.s_addr == addr->sin_addr.s_addr) {
            // If the exact mapping isn't there
            if(addrmap->from.sin_port > addr->sin_port) {
                return found;
            }

            // If we found the exact address
            if(addrmap->from.sin_port == addr->sin_port) {
                return addrmap;
            }

            // General mapping
            // Store it in case we don't find the exact address mapping
            if(addrmap->from.sin_port == 0) {
                found = addrmap;
            }
        }

        addrmap = addrmap->next;
    }

    return found;
}


/*
====================
Sv_ResolveAddrmap

Resolve an addrmap structure and check the parameters validity
====================
*/
static bool Sv_ResolveAddrmap(addrmap_t *addrmap) {
    // Resolve the addresses
    if(!Sv_ResolveIPv4Addr(addrmap->from_string, &addrmap->from) ||
            !Sv_ResolveIPv4Addr(addrmap->to_string, &addrmap->to)) {
        return false;
    }

    // 0.0.0.0 addresses are forbidden
    if(addrmap->from.sin_addr.s_addr == 0 ||
            addrmap->to.sin_addr.s_addr == 0) {
        Com_Printf(MSG_ERROR,
                   "> ERROR: Mapping from or to 0.0.0.0 is forbidden\n");
        return false;
    }

    // Do NOT allow mapping to loopback addresses
    if((ntohl(addrmap->to.sin_addr.s_addr) >> 24) == 127) {
        Com_Printf(MSG_ERROR,
                   "> ERROR: Mapping to a loopback address is forbidden\n");
        return false;
    }

    return true;
}


// ---------- Public functions (servers) ---------- //

/*
====================
Sv_SetHashSize

Set a new hash size value
====================
*/
bool Sv_SetHashSize(unsigned int size) {
    // Too late? Or too big?
    if(servers != NULL || size > MAX_HASH_SIZE) {
        return false;
    }

    sv_hash_size = size;
    return true;
}


/*
====================
Sv_SetMaxNbServers

Set a new maximum number of servers
====================
*/
bool Sv_SetMaxNbServers(unsigned int nb) {
    // Too late? Or too small?
    if(servers != NULL || nb <= 0) {
        return false;
    }

    max_nb_servers = nb;
    return true;
}


/*
====================
Sv_SetMaxNbServersPerAddress

Set a new maximum number of servers for one given IP address
====================
*/
bool Sv_SetMaxNbServersPerAddress(unsigned int nb) {
    // Too late?
    if(servers != NULL) {
        return false;
    }

    max_per_address = nb;
    return true;
}


/*
====================
Sv_Init

Initialize the server list and hash tables
====================
*/
bool Sv_Init(void) {
    size_t array_size;

    // Allocate "servers" and clean it
    array_size = max_nb_servers * sizeof(servers[0]);
    servers = (server_t *)malloc(array_size);

    if(!servers) {
        Com_Printf(MSG_ERROR,
                   "> ERROR: can't allocate the servers array (%s)\n",
                   strerror(errno));
        return false;
    }

    memset(servers, 0, array_size);
    Com_Printf(MSG_NORMAL,
               "> %u server records allocated (maximum number per address: ",
               max_nb_servers);

    if(max_per_address == 0) {
        Com_Printf(MSG_NORMAL, "unlimited)\n");
    } else {
        Com_Printf(MSG_NORMAL, "%u)\n", max_per_address);
    }

    if(! Com_UserHashTable_Init(&hash_table, sv_hash_size, "server")) {
        return false;
    }

    return true;
}


/*
====================
Sv_GetByAddr

Search for a particular server in the list; add it if necessary
====================
*/
server_t *Sv_GetByAddr(const struct sockaddr_storage *address,
                       socklen_t addrlen, bool add_it) {
    unsigned int nb_same_address = 0;
    server_t *sv;
    const addrmap_t *addrmap = NULL;
    unsigned int hash;
    unsigned int ind;

    sv = Sv_GetByAddr_Internal(address, &nb_same_address);

    if(sv != NULL) {
        assert(addrlen == sv->user.addrlen);
        return sv;
    }

    if(! add_it) {
        return NULL;
    }

    assert(nb_same_address <= max_per_address || max_per_address == 0);

    if(nb_same_address >= max_per_address && max_per_address != 0) {
        Com_Printf(MSG_WARNING,
                   "> WARNING: server %s isn't allowed (max number of servers reached for this address)\n",
                   peer_address);
        return NULL;
    }

    if(! allow_loopback) {
        // IPv4 servers on a loopback address are allowed if a mapping is defined for them
        if(address->ss_family == AF_INET) {
            const struct sockaddr_in *addr_in = (const struct sockaddr_in *)address;
            addrmap = Sv_GetAddrmap(addr_in);

            if((ntohl(addr_in->sin_addr.s_addr) >> 24) == 127 &&
                    addrmap == NULL) {
                Com_Printf(MSG_WARNING,
                           "> WARNING: server %s isn't allowed (loopback address without address mapping)\n",
                           peer_address);
                return NULL;
            }
        } else {
            const struct sockaddr_in6 *addr_in6;

            assert(address->ss_family == AF_INET6);
            addr_in6 = (const struct sockaddr_in6 *)address;

            if(memcmp(&addr_in6->sin6_addr.s6_addr, &in6addr_loopback.s6_addr,
                      sizeof(addr_in6->sin6_addr.s6_addr)) == 0) {
                Com_Printf(MSG_WARNING,
                           "> WARNING: server %s isn't allowed (IPv6 loopback address)\n",
                           peer_address);
                return NULL;
            }
        }
    }


    // If the list is full, check the entries to see if we can free a slot
    if(nb_servers == max_nb_servers) {
        assert(last_used_slot == (int)max_nb_servers - 1);
        assert(first_free_slot == -1);

        Sv_CheckTimeouts();

        if(nb_servers == max_nb_servers) {
            Com_Printf(MSG_WARNING,
                       "> WARNING: can't add server %s (server list is full)\n",
                       peer_address);
            return NULL;
        }
    }

    // Use the first free entry in "servers"
    assert(first_free_slot != -1);
    assert(-1 <= last_used_slot);
    assert(last_used_slot < (int)max_nb_servers);
    sv = &servers[first_free_slot];

    if(last_used_slot < first_free_slot) {
        last_used_slot = first_free_slot;
    }

    // Look for the next free entry in "servers"
    ind = (unsigned int)first_free_slot + 1;
    first_free_slot = -1;

    while(ind < max_nb_servers) {
        if(! Sv_IsActive(ind)) {
            first_free_slot = (int)ind;
            break;
        }

        ind++;
    }

    // Initialize the structure
    memset(sv, 0, sizeof(*sv));
    memcpy(&sv->user.address, address, sizeof(sv->user.address));
    sv->user.addrlen = addrlen;
    sv->addrmap = addrmap;

    // Add it to the list it belongs to
    hash = Com_AddressHash(address, sv_hash_size);
    Com_UserHashTable_Add(&hash_table, &sv->user, hash);

    sv->state = sv_state_uninitialized;
    sv->timeout = crt_time + TIMEOUT_HEARTBEAT;

    nb_servers++;

    Com_Printf(MSG_NORMAL,
               "> New server added: %s. %u server(s) now registered, including %u for this address quota\n",
               peer_address, nb_servers, nb_same_address + 1);
    Com_Printf(MSG_DEBUG,
               "  - index: %u\n"
               "  - hash: 0x%04X\n",
               (unsigned int)(sv - servers), hash);

    return sv;
}


/*
====================
Sv_GetFirst

Get the first server in the list
====================
*/
server_t *Sv_GetFirst(void) {
    if(nb_servers <= 0) {
        return NULL;
    }

    // Pick the start of the iteration at random
    crt_server_ind = rand() % (last_used_slot + 1);

    // Set the end of the iteration
    if(crt_server_ind == 0) {
        last_server_ind = last_used_slot;
    } else {
        last_server_ind = crt_server_ind - 1;
    }

    // If this first server is active, returns it
    if(Sv_IsActive(crt_server_ind)) {
        return &servers[crt_server_ind];
    }

    // Else, go on with the iteration
    return Sv_GetNext();
}


/*
====================
Sv_GetNext

Get the next server in the list
====================
*/
server_t *Sv_GetNext(void) {
    assert(last_used_slot >= -1);
    assert(last_used_slot < (int)max_nb_servers);

    while(crt_server_ind != last_server_ind) {
        crt_server_ind = (crt_server_ind + 1) % (last_used_slot + 1);

        if(Sv_IsActive(crt_server_ind)) {
            return &servers[crt_server_ind];
        }
    }

    return NULL;
}


/*
====================
Sv_PrintServerList

Print the list of servers to the output
====================
*/
void Sv_PrintServerList(msg_level_t msg_level) {
    int ind;

    Com_Printf(msg_level, "\n> %u servers registered (time: %lu):\n",
               nb_servers, (unsigned long)crt_time);

    for(ind = 0; ind <= last_used_slot; ind++)
        if(Sv_IsActive(ind)) {
            const server_t *sv = &servers[ind];
            const char *state_string;

            Com_Printf(msg_level, " * %s",
                       Sys_SockaddrToString(&sv->user.address, sv->user.addrlen));

            if(sv->addrmap != NULL)
                Com_Printf(msg_level, ", mapped to %s",
                           sv->addrmap->to_string);

            assert(sv->state > sv_state_unused_slot);
            assert(sv->state <= sv_state_full);

            switch(sv->state) {
                case sv_state_unused_slot:
                    state_string = "unused";
                    break;

                case sv_state_uninitialized:
                    state_string = "not initialized";
                    break;

                case sv_state_empty:
                    state_string = "empty";
                    break;

                case sv_state_occupied:
                    state_string = "occupied";
                    break;

                case sv_state_full:
                    state_string = "full";
                    break;

                default:
                    state_string = "UNKNOWN";
                    break;
            }

            Com_Printf(msg_level,
                       " (timeout: %lu)\n"
                       "\tgame: \"%s\" (protocol: %d, gametype: %s)\n"
                       "\tstate: %s\n"
                       "\tchallenge: \"%s\" (timeout: %lu)\n",
                       (unsigned long)sv->timeout,
                       sv->gamename, sv->protocol, sv->gametype,
                       state_string,
                       sv->challenge, (unsigned long)sv->challenge_timeout);
        }
}


// ---------- Public functions (address mappings) ---------- //

/*
====================
Sv_AddAddressMapping

Add an unresolved address mapping to the list
mapping must be of the form "addr1:port1=addr2:port2", ":portX" are optional
====================
*/
bool Sv_AddAddressMapping(const char *mapping) {
    char *map_string, *to_ip;
    addrmap_t *addrmap;

    // Get a working copy of the mapping string
    map_string = strdup(mapping);

    if(map_string == NULL) {
        Com_Printf(MSG_ERROR,
                   "> ERROR: can't allocate address mapping string\n");
        return false;
    }

    // Find the '='
    to_ip = strchr(map_string, '=');

    if(to_ip == NULL) {
        Com_Printf(MSG_ERROR,
                   "> ERROR: invalid syntax in address mapping string\n");
        free(map_string);
        return false;
    }

    *to_ip++ = '\0';

    // Allocate the structure
    addrmap = (addrmap_t *)malloc(sizeof(*addrmap));

    if(addrmap == NULL) {
        Com_Printf(MSG_ERROR,
                   "> ERROR: can't allocate address mapping structure\n");
        free(map_string);
        return false;
    }

    memset(addrmap, 0, sizeof(*addrmap));

    addrmap->from_string = strdup(map_string);
    addrmap->to_string = strdup(to_ip);
    free(map_string);

    if(addrmap->from_string == NULL || addrmap->to_string == NULL) {
        Com_Printf(MSG_ERROR,
                   "> ERROR: can't allocate address mapping strings\n");
        free(addrmap->to_string);
        free(addrmap->from_string);
        return false;
    }

    // Add it on top of "addrmaps"
    addrmap->next = addrmaps;
    addrmaps = addrmap;

    return true;
}


/*
====================
Sv_ResolveAddressMappings

Resolve the address mapping list
====================
*/
bool Sv_ResolveAddressMappings(void) {
    addrmap_t *addrmap;

    // Resolve all addresses
    for(addrmap = addrmaps; addrmap != NULL; addrmap = addrmap->next)
        if(!Sv_ResolveAddrmap(addrmap)) {
            return false;
        }

    // Build the sorted list
    addrmap = addrmaps;
    addrmaps = NULL;

    while(addrmap != NULL) {
        addrmap_t *next_addrmap = addrmap->next;

        if(! Sv_InsertAddrmapIntoList(addrmap)) {
            return false;
        }

        addrmap = next_addrmap;
    }

    return true;
}
