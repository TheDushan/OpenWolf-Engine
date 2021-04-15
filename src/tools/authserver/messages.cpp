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
// File name:   messages.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: Message management for owauthserver
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include "common.hpp"
#include "system.hpp"

#include "clients.hpp"
#include "games.hpp"
#include "messages.hpp"
#include "servers.hpp"

// ---------- Constants ---------- //

// Timeout after a valid infoResponse (in secondes)
#define TIMEOUT_INFORESPONSE (15 * 60)

// Period of validity for a challenge string (in secondes)
#define TIMEOUT_CHALLENGE 2

// Maximum size of a reponse packet
#define MAX_PACKET_SIZE_OUT 1400


// Types of messages (with samples):

// Q3: "heartbeat QuakeArena-1\x0A"
// DP: "heartbeat DarkPlaces\x0A"
#define S2M_HEARTBEAT "heartbeat "

// Q3 & DP & QFusion: "getinfo A_Challenge"
#define M2S_GETINFO "getinfo"

// Q3 & DP & QFusion: "infoResponse\x0A\\pure\\1\\..."
#define S2M_INFORESPONSE "infoResponse\x0A"

// Q3: "getservers 67 ffa empty full"
// DP: "getservers DarkPlaces-Quake 3 empty full"
// DP: "getservers Transfusion 3 empty full"
// QFusion: "getservers qfusion 39 empty full"
#define C2M_GETSERVERS "getservers "

// DP: "getserversExt DarkPlaces-Quake 3 empty full ipv4 ipv6"
// IOQuake3: "getserversExt 68 empty ipv6"
#define C2M_GETSERVERSEXT "getserversExt "

// Q3 & DP & QFusion:
// "getserversResponse\\...(6 bytes)...\\...(6 bytes)...\\EOT\0\0\0"
#define M2C_GETSERVERSREPONSE "getserversResponse"

// DP & IOQuake3:
// "getserversExtResponse\\...(6 bytes)...//...(18 bytes)...\\EOT\0\0\0"
#define M2C_GETSERVERSEXTREPONSE "getserversExtResponse"

#define PACKET_CLIENT "getKeyAuthorize"
#define PACKET_SERVER "getIpAuthorize"

// ---------- Private functions ---------- //

/*
====================
SearchInfostring

Search an infostring for the value of a key
====================
*/
static const char *SearchInfostring(const char *infostring,
                                    const char *key) {
    static char str_buffer[256];
    size_t buffer_ind;
    char c;

    if(*infostring++ != '\\') {
        return NULL;
    }

    for(;;) {
        buffer_ind = 0;

        // Get the key name
        for(;;) {
            c = *infostring++;

            if(c == '\\') {
                str_buffer[buffer_ind] = '\0';
                break;
            }

            // If it's the end of the infostring
            if(c == '\0') {
                return NULL;
            }

            // If the key name is too long, skip this key/value pair
            if(buffer_ind == sizeof(str_buffer) - 1) {
                // Skip the rest of the key name
                for(;;) {
                    c = *infostring++;

                    if(c == '\0') {
                        return NULL;
                    }

                    if(c == '\\') {
                        break;
                    }
                }

                str_buffer[0] = '\0';
                break;
            }

            str_buffer[buffer_ind++] = c;
        }

        // If it's the key we are looking for, save its value in "str_buffer"
        if(!strcmp(str_buffer, key)) {
            buffer_ind = 0;

            for(;;) {
                c = *infostring++;

                if(c == '\0' || c == '\\') {
                    str_buffer[buffer_ind] = '\0';
                    return str_buffer;
                }

                // If the value name is too long, ignore it
                if(buffer_ind == sizeof(str_buffer) - 1) {
                    return NULL;
                }

                str_buffer[buffer_ind++] = c;
            }
        }

        // Else, skip the value
        for(;;) {
            c = *infostring++;

            if(c == '\0') {
                return NULL;
            }

            if(c == '\\') {
                break;
            }
        }
    }
}


/*
====================
BuildChallenge

Build a challenge string for a "getinfo" message
====================
*/
static const char *BuildChallenge(void) {
    static char challenge[CHALLENGE_MAX_LENGTH];
    size_t ind;
    size_t length = CHALLENGE_MIN_LENGTH - 1;  // We start at the minimum size

    // ... then we add a random number of characters
    length += rand() % (CHALLENGE_MAX_LENGTH - CHALLENGE_MIN_LENGTH + 1);

    for(ind = 0; ind < length; ind++) {
        char c;

        do {
            c = 33 + rand() % (126 - 33 + 1);   // -> c = 33..126
        } while(c == '\\' || c == ';' || c == '"' || c == '%' || c == '/');

        challenge[ind] = c;
    }

    challenge[length] = '\0';
    return challenge;
}


/*
====================
SendGetInfo

Send a "getinfo" message to a server
====================
*/
static void SendGetInfo(server_t *server, socket_t recv_socket,
                        bool force_new_challenge) {
    char msg[64] = "\xFF\xFF\xFF\xFF" M2S_GETINFO " ";
    size_t msglen;

    if(force_new_challenge || !server->challenge_timeout ||
            server->challenge_timeout < crt_time) {
        const char *challenge;

        challenge = BuildChallenge();
        strncpy(server->challenge, challenge, sizeof(server->challenge) - 1);
        server->challenge_timeout = crt_time + TIMEOUT_CHALLENGE;
    }

    msglen = strlen(msg);
    strncpy(msg + msglen, server->challenge, sizeof(msg) - msglen - 1);
    msg[sizeof(msg) - 1] = '\0';

    if(sendto(recv_socket, msg, strlen(msg), 0,
              (const struct sockaddr *)&server->user.address,
              server->user.addrlen) < 0)
        Com_Printf(MSG_WARNING, "> WARNING: can't send getinfo (%s)\n",
                   Sys_GetLastNetErrorString());
    else
        Com_Printf(MSG_NORMAL, "> %s <--- getinfo with challenge \"%s\"\n",
                   peer_address, server->challenge);
}


/*
====================
HandleHeartbeat

Parse heartbeat requests
====================
*/
static void HandleHeartbeat(const char *msg,
                            const struct sockaddr_storage *addr, socklen_t addrlen,
                            socket_t recv_socket) {
    char tag[64];
    const game_properties_t *game_props;
    server_t *server;
    bool flatlineHeartbeat;

    // Extract the tag
    sscanf(msg, "%63s", tag);
    Com_Printf(MSG_NORMAL, "> %s ---> heartbeat (%s)\n",
               peer_address, tag);

    // If it's not a game that uses the DarkPlaces protocol
    if(strcmp(tag, HEARTBEAT_DARKPLACES) != 0) {
        game_props = Game_GetPropertiesByHeartbeat(tag, &flatlineHeartbeat);

        if(game_props == NULL) {
            Com_Printf(MSG_WARNING,
                       "> WARNING: Rejecting heartbeat from %s (heartbeat \"%s\" is unknown)\n",
                       peer_address, tag);
            return;
        }

        Com_Printf(MSG_DEBUG, "  - belongs to game \"%s\"\n",
                   game_props->name);

        // Ignore flatline (shutdown) heartbeats
        if(flatlineHeartbeat) {
            Com_Printf(MSG_NORMAL, "  - flatline heartbeat (ignored)\n");
            return;
        }

        // If the game isn't accepted on this server, ignore the heartbeat
        if(!Game_IsAccepted(game_props->name)) {
            Com_Printf(MSG_WARNING,
                       "> WARNING: Rejecting heartbeat from %s (game \"%s\" is not accepted)\n",
                       peer_address, game_props->name);
            return;
        }
    } else {
        game_props = NULL;
    }

    // Get the server in the list (add it to the list if necessary)
    server = Sv_GetByAddr(addr, addrlen, true);

    if(server == NULL) {
        return;
    }

    assert(server->state != sv_state_unused_slot);

    // Ask for some infos.
    // Force a new challenge if the heartbeat tag has changed
    SendGetInfo(server, recv_socket, server->hb_properties != game_props);

    // Save the game properties for a future use
    server->hb_properties = game_props;
}


/*
====================
HandleGetServers

Parse getservers requests and send the appropriate response
====================
*/
static void HandleGetServers(const char *msg,
                             const struct sockaddr_storage *addr, socklen_t addrlen,
                             socket_t recv_socket, bool extended_request) {
    const char *packetheader;
    size_t headersize;
    char *end_ptr;
    const char *msg_ptr;
    char gamename[GAMENAME_LENGTH] = "";
    qbyte packet[MAX_PACKET_SIZE_OUT];
    size_t packetind;
    server_t *sv;
    int protocol;
    game_options_t game_options = GAME_OPTION_NONE;
    char gametype[GAMETYPE_LENGTH] = "0";
    bool use_dp_protocol;
    bool opt_empty = false;
    bool opt_full = false;
    bool opt_ipv4 = (!extended_request);
    bool opt_ipv6 = false;
    bool opt_gametype = false;
    char filter_options[MAX_PACKET_SIZE_IN];
    char *option_ptr;
    unsigned int nb_servers;
    const char *request_name;


    if(extended_request) {
        request_name = "getserversExt";
        use_dp_protocol = true;
    } else {
        request_name = "getservers";

        // Check if there's a name before the protocol number
        // In this case, the message comes from a DarkPlaces-compatible client
        protocol = (int)strtol(msg, &end_ptr, 0);
        use_dp_protocol = (end_ptr == msg || (*end_ptr != ' ' &&
                                              *end_ptr != '\0'));
    }

    if(use_dp_protocol) {
        char *space;

        // Skip leading spaces
        msg_ptr = msg;

        while(*msg_ptr == ' ') {
            msg_ptr++;
        }

        if(*msg_ptr == '\0') {
            Com_Printf(MSG_WARNING,
                       "> WARNING: Rejecting %s from %s (missing game name and protocol number)\n",
                       request_name, peer_address);
            return;
        }

        // Read the game name
        strncpy(gamename, msg_ptr, sizeof(gamename) - 1);
        gamename[sizeof(gamename) - 1] = '\0';
        space = strchr(gamename, ' ');

        if(space) {
            *space = '\0';
        }

        msg_ptr = msg_ptr + strlen(gamename);

        game_options = Game_GetOptions(gamename);

        // Read the protocol number
        protocol = (int)strtol(msg_ptr, &end_ptr, 0);

        if(end_ptr == msg_ptr || (*end_ptr != ' ' && *end_ptr != '\0')) {
            Com_Printf(MSG_WARNING,
                       "> WARNING: Rejecting %s from %s (missing or invalid protocol number)\n",
                       request_name, peer_address);
            return;
        }
    }
    // Else, it comes from an anonymous client
    else {
        const char *anon_game = Game_GetNameByProtocol(protocol, &game_options);

        // If we can't determine the game name from the protocol, we will just use
        // the 1st server we found with this protocol to get a game name
        if(anon_game != NULL) {
            strncpy(gamename, anon_game, sizeof(gamename) - 1);
            gamename[sizeof(gamename) - 1] = '\0';
        } else {
            gamename[0] = '\0';
        }

        msg_ptr = end_ptr;
    }

    Com_Printf(MSG_NORMAL, "> %s ---> %s (%s, %i)\n", peer_address,
               request_name,
               gamename[0] != '\0' ? gamename : "unknown game", protocol);

    if(gamename[0] != '\0' && !Game_IsAccepted(gamename)) {
        Com_Printf(MSG_WARNING,
                   "> WARNING: Rejecting %s from %s (game \"%s\" is not accepted)\n",
                   request_name, peer_address, gamename);
        return;
    }

    // Apply the game options
    if((game_options & GAME_OPTION_SEND_EMPTY_SERVERS) != 0) {
        opt_empty = true;
    }

    if((game_options & GAME_OPTION_SEND_FULL_SERVERS) != 0) {
        opt_full = true;
    }

    // Parse the filtering options
    strncpy(filter_options, msg_ptr, sizeof(filter_options) - 1);
    filter_options[sizeof(filter_options) - 1] = '\0';
    option_ptr = strtok(filter_options, " ");

    while(option_ptr != NULL) {
        if(strcmp(option_ptr, "empty") == 0) {
            opt_empty = true;
        } else if(strcmp(option_ptr, "full") == 0) {
            opt_full = true;
        } else if(strcmp(option_ptr, "ffa") == 0) {
            gametype[0] = '0';
            gametype[1] = '\0';
            opt_gametype = true;
        } else if(strcmp(option_ptr, "tourney") == 0) {
            gametype[0] = '1';
            gametype[1] = '\0';
            opt_gametype = true;
        } else if(strcmp(option_ptr, "team") == 0) {
            gametype[0] = '3';
            gametype[1] = '\0';
            opt_gametype = true;
        } else if(strcmp(option_ptr, "ctf") == 0) {
            gametype[0] = '4';
            gametype[1] = '\0';
            opt_gametype = true;
        } else if(strncmp(option_ptr, "gametype=", 9) == 0) {
            const char *gametype_string = option_ptr + 9;

            strncpy(gametype, gametype_string, sizeof(gametype) - 1);
            gametype[sizeof(gametype) - 1] = '\0';
            opt_gametype = true;
        } else if(extended_request) {
            if(strcmp(option_ptr, "ipv4") == 0) {
                opt_ipv4 = true;
            } else if(strcmp(option_ptr, "ipv6") == 0) {
                opt_ipv6 = true;
            }
        }

        option_ptr = strtok(NULL, " ");
    }

    // If no IP version was given for the filtering, accept any version
    if(!opt_ipv4 && !opt_ipv6) {
        opt_ipv4 = true;
        opt_ipv6 = true;
    }

    // Initialize the packet contents with the header
    if(extended_request) {
        packetheader = "\xFF\xFF\xFF\xFF" M2C_GETSERVERSEXTREPONSE;
    } else {
        packetheader = "\xFF\xFF\xFF\xFF" M2C_GETSERVERSREPONSE;
    }

    headersize = strlen(packetheader);
    packetind = headersize;
    memcpy(packet, packetheader, headersize);

    // Add every relevant server
    nb_servers = 0;

    for(sv = Sv_GetFirst(); sv != NULL; sv = Sv_GetNext()) {
        size_t next_sv_size;

        assert(sv->state != sv_state_unused_slot);

        // Extra debugging info
        if(max_msg_level >= MSG_DEBUG) {
            const char *addrstr = Sys_SockaddrToString(&sv->user.address,
                                  sv->user.addrlen);
            Com_Printf(MSG_DEBUG,
                       "  - Comparing server: IP:\"%s\", p:%d, g:\"%s\"\n",
                       addrstr, sv->protocol, sv->gamename);

            if(sv->state <= sv_state_uninitialized)
                Com_Printf(MSG_DEBUG,
                           "    Reject: server is not initialized\n");
            else if(sv->protocol != protocol)
                Com_Printf(MSG_DEBUG,
                           "    Reject: protocol %d != requested %d\n",
                           sv->protocol, protocol);
            else if(!opt_empty && sv->state == sv_state_empty) {
                Com_Printf(MSG_DEBUG, "    Reject: no empty server allowed\n");
            } else if(!opt_full && sv->state == sv_state_full) {
                Com_Printf(MSG_DEBUG, "    Reject: no full server allowed\n");
            } else if(!opt_ipv4 && sv->user.address.ss_family == AF_INET) {
                Com_Printf(MSG_DEBUG, "    Reject: no IPv4 servers allowed\n");
            } else if(!opt_ipv6 && sv->user.address.ss_family == AF_INET6) {
                Com_Printf(MSG_DEBUG, "    Reject: no IPv6 servers allowed\n");
            } else if(opt_gametype && strcmp(gametype, sv->gametype) != 0)
                Com_Printf(MSG_DEBUG,
                           "    Reject: gametype \"%s\" != requested \"%s\"\n",
                           sv->gametype, gametype);
            else {
                if(gamename[0] != '\0') {
                    if(strcmp(gamename, sv->gamename) != 0)
                        Com_Printf(MSG_DEBUG,
                                   "    Reject: gamename \"%s\" != requested \"%s\"\n",
                                   sv->gamename, gamename);
                } else {
                    if(sv->anon_properties == NULL)
                        Com_Printf(MSG_DEBUG,
                                   "    Reject: can't use \"%s\" as an anonymous game name\n",
                                   sv->gamename);
                }
            }
        }

        // Check state and protocol
        if(sv->state <= sv_state_uninitialized ||
                sv->protocol != protocol) {
            // Skip it
            continue;
        }

        // Since the protocols match, if we don't know the game name yet and
        // that this server doesn't use the DarkPlaces protocol, use its game name
        // (if the unknown game was using the DP protocol, the client should have
        // sent a game name with its "getservers" query)
        if(gamename[0] == '\0' && sv->anon_properties != NULL) {
            strncpy(gamename, sv->gamename, sizeof(gamename) - 1);
            gamename[sizeof(gamename) - 1] = '\0';

            Com_Printf(MSG_DEBUG, "  - Using this server's game name\n");

            if(!Game_IsAccepted(gamename)) {
                Com_Printf(MSG_WARNING,
                           "> WARNING: Rejecting %s from %s (game \"%s\" is not accepted)\n",
                           request_name, peer_address, gamename);
                return;
            }
        }

        // Check options, game type and game name
        if((!opt_empty && sv->state == sv_state_empty) ||
                (!opt_full && sv->state == sv_state_full) ||
                (!opt_ipv4 && sv->user.address.ss_family == AF_INET) ||
                (!opt_ipv6 && sv->user.address.ss_family == AF_INET6) ||
                (opt_gametype && strcmp(gametype, sv->gametype) != 0) ||
                strcmp(gamename, sv->gamename) != 0) {
            // Skip it
            continue;
        }

        // If the packet doesn't have enough free space for this server
        next_sv_size = (sv->user.address.ss_family == AF_INET ? 4 : 16) + 3;

        if(packetind + next_sv_size > sizeof(packet)) {
            // Send the packet to the client
            if(sendto(recv_socket, (char *)packet, packetind, 0,
                      (const struct sockaddr *)addr, addrlen) < 0)
                Com_Printf(MSG_WARNING, "> WARNING: can't send %s (%s)\n",
                           request_name, Sys_GetLastNetErrorString());
            else
                Com_Printf(MSG_NORMAL, "> %s <--- %sResponse (%u servers)\n",
                           peer_address, request_name, nb_servers);

            // Reset the packet index (no need to change the header)
            packetind = headersize;
            nb_servers = 0;
        }

        if(sv->user.address.ss_family == AF_INET) {
            const struct sockaddr_in *sv_sockaddr;
            unsigned int sv_addr;
            unsigned short sv_port;

            sv_sockaddr = (const struct sockaddr_in *)&sv->user.address;
            sv_addr = ntohl(sv_sockaddr->sin_addr.s_addr);
            sv_port = ntohs(sv_sockaddr->sin_port);

            // Use the address mapping associated with the server, if any
            if(sv->addrmap != NULL) {
                const addrmap_t *addrmap = sv->addrmap;

                sv_addr = ntohl(addrmap->to.sin_addr.s_addr);

                if(addrmap->to.sin_port != 0) {
                    sv_port = ntohs(addrmap->to.sin_port);
                }

                Com_Printf(MSG_DEBUG,
                           "  - Using mapped address %u.%u.%u.%u:%hu\n",
                           sv_addr >> 24, (sv_addr >> 16) & 0xFF,
                           (sv_addr >> 8) & 0xFF, sv_addr & 0xFF,
                           sv_port);
            }

            // Heading '\'
            packet[packetind] = '\\';

            // IP address
            packet[packetind + 1] = sv_addr >> 24;
            packet[packetind + 2] = (sv_addr >> 16) & 0xFF;
            packet[packetind + 3] = (sv_addr >> 8) & 0xFF;
            packet[packetind + 4] = sv_addr & 0xFF;

            // Port
            packet[packetind + 5] = sv_port >> 8;
            packet[packetind + 6] = sv_port & 0xFF;

            Com_Printf(MSG_DEBUG, "  - Sending server %u.%u.%u.%u:%hu\n",
                       packet[packetind + 1], packet[packetind + 2],
                       packet[packetind + 3], packet[packetind + 4],
                       sv_port);

            packetind += 7;
        } else {
            const struct sockaddr_in6 *sv_sockaddr6;
            unsigned short sv_port;

            sv_sockaddr6 = (const struct sockaddr_in6 *)&sv->user.address;

            // Heading '/'
            packet[packetind] = '/';
            packetind += 1;

            // IP address
            memcpy(&packet[packetind], &sv_sockaddr6->sin6_addr.s6_addr,
                   sizeof(sv_sockaddr6->sin6_addr.s6_addr));
            packetind += sizeof(sv_sockaddr6->sin6_addr.s6_addr);

            // Port
            sv_port = ntohs(sv_sockaddr6->sin6_port);
            packet[packetind] = sv_port >> 8;
            packet[packetind + 1] = sv_port & 0xFF;
            packetind += 2;
        }

        nb_servers++;
    }

    // If the packet doesn't have enough free space for the EOT mark
    if(packetind + 7 > sizeof(packet)) {
        // Send the packet to the client
        if(sendto(recv_socket, (char *)packet, packetind, 0,
                  (const struct sockaddr *)addr, addrlen) < 0)
            Com_Printf(MSG_WARNING, "> WARNING: can't send %s (%s)\n",
                       request_name, Sys_GetLastNetErrorString());
        else
            Com_Printf(MSG_NORMAL, "> %s <--- %sResponse (%u servers)\n",
                       peer_address, request_name, nb_servers);

        // Reset the packet index (no need to change the header)
        packetind = headersize;
        nb_servers = 0;
    }

    // End Of Transmission
    packet[packetind] = '\\';
    packet[packetind + 1] = 'E';
    packet[packetind + 2] = 'O';
    packet[packetind + 3] = 'T';
    packet[packetind + 4] = '\0';
    packet[packetind + 5] = '\0';
    packet[packetind + 6] = '\0';
    packetind += 7;

    // Send the packet to the client
    if(sendto(recv_socket, (char *)packet, packetind, 0,
              (const struct sockaddr *)addr, addrlen) < 0)
        Com_Printf(MSG_WARNING, "> WARNING: can't send %s (%s)\n",
                   request_name, Sys_GetLastNetErrorString());
    else
        Com_Printf(MSG_NORMAL, "> %s <--- %sResponse (%u servers)\n",
                   peer_address, request_name, nb_servers);
}


/*
====================
HandleInfoResponse

Parse infoResponse messages
====================
*/
static void HandleInfoResponse(server_t *server, const char *msg) {
    const char *value;
    int new_protocol;
    char new_gametype[GAMETYPE_LENGTH];
    char *end_ptr;
    unsigned int new_maxclients, new_clients;

    // Check the challenge
    if(!server->challenge_timeout || server->challenge_timeout < crt_time) {
        Com_Printf(MSG_WARNING,
                   "> WARNING: infoResponse with obsolete challenge from %s\n",
                   peer_address);
        return;
    }

    value = SearchInfostring(msg, "challenge");

    if(!value || strcmp(value, server->challenge)) {
        Com_Printf(MSG_WARNING, "> WARNING: invalid challenge from %s (%s)\n",
                   peer_address, value);
        return;
    }

    // Check the value of "protocol"
    value = SearchInfostring(msg, "protocol");

    if(value == NULL) {
        Com_Printf(MSG_WARNING,
                   "> WARNING: invalid infoResponse from %s (no protocol value)\n",
                   peer_address);
        return;
    }

    new_protocol = (int)strtol(value, &end_ptr, 0);

    if(end_ptr == value || *end_ptr != '\0') {
        Com_Printf(MSG_WARNING,
                   "> WARNING: invalid infoResponse from %s (invalid protocol value: %s)\n",
                   peer_address, value);
        return;
    }

    // Check the value of "gametype"
    value = SearchInfostring(msg, "gametype");

    if(value != NULL) {
        if(strchr(value, ' ') != NULL) {
            Com_Printf(MSG_WARNING,
                       "> WARNING: invalid infoResponse from %s (game type contains whitespaces)\n",
                       peer_address);
            return;
        }
    }
    // Default to gametype = "0" if the server hasn't sent this information
    else {
        value = "0";
    }

    strncpy(new_gametype, value, sizeof(new_gametype) - 1);
    new_gametype[sizeof(new_gametype) - 1] = '\0';


    // Check the value of "maxclients"
    value = SearchInfostring(msg, "sv_maxclients");
    new_maxclients = ((value != NULL) ? atoi(value) : 0);

    if(new_maxclients == 0) {
        Com_Printf(MSG_WARNING,
                   "> WARNING: invalid infoResponse from %s (sv_maxclients = %d)\n",
                   peer_address, new_maxclients);
        return;
    }

    // Check the presence of "clients"
    value = SearchInfostring(msg, "clients");

    if(value == NULL) {
        Com_Printf(MSG_WARNING,
                   "> WARNING: invalid infoResponse from %s (no \"clients\" value)\n",
                   peer_address);
        return;
    }

    new_clients = ((value != NULL) ? atoi(value) : 0);

    // If the server didn't send a gamename, guess it using the protocol
    value = SearchInfostring(msg, "gamename");

    if(value == NULL) {
        // Games that neither send a known heartbeat nor provide a game name are ignored
        if(server->hb_properties == NULL) {
            Com_Printf(MSG_WARNING,
                       "> WARNING: invalid infoResponse from %s (no game name)\n",
                       peer_address);
            return;
        }

        value = server->hb_properties->name;
    }
    // ... but if it did, it must match the one its heartbeat advertized (if any)
    else {
        if(server->hb_properties != NULL &&
                strcmp(value, server->hb_properties->name) != 0) {
            Com_Printf(MSG_WARNING,
                       "> WARNING: invalid infoResponse from %s (game name is different from the one advertized by the heartbeat)\n",
                       peer_address);
            return;
        }
    }

    if(value[0] == '\0') {
        Com_Printf(MSG_WARNING,
                   "> WARNING: invalid infoResponse from %s (game name is void)\n",
                   peer_address);
        return;
    } else if(strchr(value, ' ') != NULL) {
        Com_Printf(MSG_WARNING,
                   "> WARNING: invalid infoResponse from %s (game name contains whitespaces)\n",
                   peer_address);
        return;
    }

    if(!Game_IsAccepted(value)) {
        Com_Printf(MSG_WARNING,
                   "> WARNING: Rejecting infoResponse from %s (game \"%s\" is not accepted)\n",
                   peer_address, value);
        return;
    }

    // Save some useful informations in the server entry
    strncpy(server->gamename, value, sizeof(server->gamename) - 1);
    server->protocol = new_protocol;
    server->anon_properties = server->hb_properties;
    strncpy(server->gametype, new_gametype, sizeof(server->gametype) - 1);

    if(new_clients == 0) {
        server->state = sv_state_empty;
    } else if(new_clients == new_maxclients) {
        server->state = sv_state_full;
    } else {
        server->state = sv_state_occupied;
    }

    // Set a new timeout
    server->timeout = crt_time + TIMEOUT_INFORESPONSE;
}

/*
====================
ValidPacket

Checks if packet is valid
====================
*/
int validPacket(const char *data, int size) {

    if(!data) {
        return 4;
    } else if(strlen(data) != size) {
        return 0;
    }

    return 1;
}

/*
====================
ValidPacketRange

Checks if packet is valid
====================
*/
int validPacketRange(const char *data, unsigned int min,
                     unsigned int max) {

    if(!data) {
        return 0;
    } else if(strlen(data) > max && strlen(data) < min) {
        return 0;
    }

    return 1;
}

/*
====================
is_numeric

Checks if packet is valid
====================
*/
int is_numeric(const char *p) {
    if(*p) {
        char c;

        while((c = *p++)) {
            if(!isdigit(c)) {
                return 0;
            }
        }

        return 1;
    }

    return 0;
}

/*
====================
SendResponse
====================
*/
static void SendResponseToServer(socket_t recvSocket, char challenge[64],
                                 const struct sockaddr_storage *address, socklen_t addressLen) {
    char msg[64] = "ipAuthorize ";
    char info[128];

    sprintf(info, "\xFF\xFF\xFF\xFFipAuthorize %s", challenge);

    if(sendto(recvSocket, info, strlen(info), 0,
              (const struct sockaddr *)address, addressLen) < 0) {
        Com_Printf(MSG_NORMAL, "> WARNING: can't send " PACKET_SERVER " (%s)\n",
                   Sys_GetLastNetErrorString());
    } else {
        Com_Printf(MSG_NORMAL, "> %s <--- %sResponse\n", peer_address,
                   PACKET_SERVER);
    }
}
/*
====================
HandleKeyResponse

Parse keyResponse messages
====================
*/
static void HandleKeyResponse(const char *msg,
                              const struct sockaddr_storage *addr, socklen_t addrlen,
                              socket_t recv_socket) {
    char str[128];
    char *token[64] = { 0 };
    unsigned int index = 0;

    // Flood protection
    if(Cl_BlockedByThrottle(addr, addrlen, DEFAULT_FP_THROTTLE)) {
        return;
    }

    // Break message so packets can be tested
    strncpy(str, msg, sizeof(str));
    token[index] = strtok(str, " ");

    while(token[index] != 0) {
        ++index;
        token[index] = strtok(0, " ");
    }

    // We expect 3 packets.
    if(index != 3) {
        return;
    }

    // Every packet is pre-defined
    if(!validPacket(token[0], 16) || !validPacket(token[1], 5) ||
            !validPacket(token[2], 12)) {
        return;
    }

    return;
}

/*
====================
HandleIPResponse
====================
*/
static void HandleIPResponse(const char *message,
                             const struct sockaddr_storage *address, socklen_t addressLen,
                             socket_t recvSocket) {
    char str[128];
    char *token[64] = { 0 };
    unsigned int index = 0;
    int data;

    // Flood protection
    if(Cl_BlockedByThrottle(address, addressLen, DEFAULT_FP_THROTTLE)) {
        return;
    }

    // Break message so packets can be tested
    strncpy(str, message, sizeof(str));
    token[index] = strtok(str, " ");

    while(token[index] != 0) {
        ++index;
        token[index] = strtok(0, " ");
    }

    // We expect 6 packets
    if(index != 6) {
        return;
    }

    // Send packet now
    SendResponseToServer(recvSocket, token[0], address, addressLen);
}

/*
====================
HandlePacketMessage
====================
*/
void HandlePacketMessage(const char *message, size_t length,
                         const struct sockaddr_storage *address, socklen_t addressLen,
                         socket_t recvSocket) {
    //Client make a query
    if(!strncmp(PACKET_CLIENT, message, strlen(PACKET_CLIENT))) {
        Com_Printf(MSG_NORMAL, "> %s ---> %s\n", peer_address, PACKET_CLIENT);

        HandleKeyResponse(message + strlen(PACKET_CLIENT), address, addressLen,
                          recvSocket);
    }
    // Server reply
    else if(!strncmp(PACKET_SERVER, message, strlen(PACKET_SERVER))) {
        Com_Printf(MSG_NORMAL, "> %s ---> %s\n", peer_address, PACKET_SERVER);

        HandleIPResponse(message + strlen(PACKET_SERVER), address, addressLen,
                         recvSocket);
    }
}
