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
// Description: Server list and address mapping management for owmaster
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SERVERS_H_
#define _SERVERS_H_

// ---------- Constants ---------- //

// Maximum number of servers in all lists by default
#define DEFAULT_MAX_NB_SERVERS 4096

// Maximum number of servers for one given IP address by default
#define DEFAULT_MAX_NB_SERVERS_PER_ADDRESS 32

// Address hash size in bits for servers (between 0 and MAX_HASH_SIZE)
#define DEFAULT_SV_HASH_SIZE 10

// Number of characters in a challenge, including the '\0'
#define CHALLENGE_MIN_LENGTH 9
#define CHALLENGE_MAX_LENGTH 12

// Max number of characters for a gamename, including the '\0'
#define GAMENAME_LENGTH 64

// Max number of characters for a gametype, including the '\0'
#define GAMETYPE_LENGTH 32

// Max size of a server info string, including the '\0'
#define SERVERINFO_LENGTH 1024


// ---------- Types ---------- //

// Address mapping
typedef struct addrmap_s {
    struct addrmap_s *next;
    struct sockaddr_in from;
    struct sockaddr_in to;
    char *from_string;
    char *to_string;
} addrmap_t;

// Server state
typedef enum {
    sv_state_unused_slot,
    sv_state_uninitialized,
    sv_state_empty,
    sv_state_occupied,
    sv_state_full,
} server_state_t;

// Server properties
struct game_properties_s;       // Defined in games.h
typedef struct server_s {
    user_t user;                                        // WARNING: MUST be the 1st member, for compatibility with the user hash tables
    const struct addrmap_s *addrmap;
    const struct game_properties_s
        *anon_properties;    // game properties, for an anonymous game
    const struct game_properties_s
        *hb_properties;      // future "anon_properties", not yet validated by an infoResponse
    time_t timeout;
    time_t challenge_timeout;
    int protocol;
    server_state_t state;
    char challenge [CHALLENGE_MAX_LENGTH];
    char gametype [GAMETYPE_LENGTH];
    char gamename [GAMENAME_LENGTH];
    char serverinfo [SERVERINFO_LENGTH];
} server_t;


// ---------- Public variables ---------- //

// Are servers talking from a loopback interface allowed?
extern qboolean allow_loopback;


// ---------- Public functions (servers) ---------- //

// Will simply return "false" if called after Sv_Init
qboolean Sv_SetHashSize(unsigned int size);
qboolean Sv_SetMaxNbServers(unsigned int nb);
qboolean Sv_SetMaxNbServersPerAddress(unsigned int nb);

// Initialize the server list and hash tables
qboolean Sv_Init(void);

// Search for a particular server in the list; add it if necessary
// NOTE: doesn't change the current position for "Sv_GetNext"
server_t *Sv_GetByAddr(const struct sockaddr_storage *address,
                       socklen_t addrlen, qboolean add_it);

// Get the first server in the list
server_t *Sv_GetFirst(void);

// Get the next server in the list
server_t *Sv_GetNext(void);

// Print the list of servers to the output
void Sv_PrintServerList(msg_level_t msg_level);


// ---------- Public functions (address mappings) ---------- //

// NOTE: this is a 2-step process because resolving address mappings directly
// during the parsing of the command line would cause several problems

// Add an unresolved address mapping to the list
// mapping must be of the form "addr1:port1=addr2:port2", ":portX" are optional
qboolean Sv_AddAddressMapping(const char *mapping);

// Resolve the address mapping list
qboolean Sv_ResolveAddressMappings(void);


#endif  // #ifndef _SERVERS_H_
