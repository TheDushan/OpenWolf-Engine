////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2008 - 2011 Mathieu Olivier
// Copyright(C) 2011 - 2022 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   games.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
//              AppleClang 9.0.0.9000039
// Description: Games management for owauthserver
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef _GAMES_H_
#define _GAMES_H_

// ---------- Public functions (game policy) ---------- //

// Declare the server policy regarding which games are allowed on this master
cmdline_status_t Game_DeclarePolicy(const char *policy, const char **games,
                                    unsigned int nb_games);

// Return true if the game is allowed on this master
bool Game_IsAccepted(const char *game_name);


// ---------- Public constants (game properties) ---------- //

// Heartbeat tag for the DarkPlaces protocol
#define HEARTBEAT_DARKPLACES    "DarkPlaces"


// ---------- Public types (game properties) ---------- //

typedef enum {
    GAME_OPTION_NONE                = 0,

    // Send empty servers even when the "getservers" requests don't ask for them
    GAME_OPTION_SEND_EMPTY_SERVERS  = (1 << 0),

    // Send full servers even when the "getservers" requests don't ask for them
    GAME_OPTION_SEND_FULL_SERVERS   = (1 << 1),
} game_options_t;

typedef enum {
    HEARTBEAT_TYPE_ALIVE,
    HEARTBEAT_TYPE_DEAD,

    NB_HEARTBEAT_TYPES,
} heartbeat_type_t;

typedef struct game_properties_s {
    const char                 *name;
    game_options_t              options;
    char
    *heartbeats [NB_HEARTBEAT_TYPES];    // Heartbeat tags
    struct game_properties_s   *next;
} game_properties_t;

// ---------- Public functions (game properties) ---------- //

// Initialize the game properties using a built-in list
void Game_InitProperties(void);

// Print the list of known game properties
void Game_PrintProperties(void);

// Update the properties of a game according to the given list of properties
cmdline_status_t Game_UpdateProperties(const char *game,
                                       const char **props, size_t nb_props);

// Set the name that is returned when an anonymous game uses an unknown protocol number
cmdline_status_t Game_SetDefaultAnonymous(const char *game);

// Returns the name of a game based on its protocol number
const char *Game_GetNameByProtocol(int protocol, game_options_t *options);

// Returns the properties of the game which uses this heartbeat tag.
// "flatline_heartbeat" will be set to "true" if it's a flatline tag
const game_properties_t *Game_GetPropertiesByHeartbeat(
    const char *heartbeat_tag, bool *flatline_heartbeat);

// Returns the options of a game
game_options_t Game_GetOptions(const char *game);


#endif  // #ifndef _GAMES_H_
