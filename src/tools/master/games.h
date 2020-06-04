/*
	games.h

	Games management for dpmaster

	Copyright (C) 2009-2010  Mathieu Olivier

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef _GAMES_H_
#define _GAMES_H_


// ---------- Public functions (game policy) ---------- //

// Declare the server policy regarding which games are allowed on this master
cmdline_status_t Game_DeclarePolicy( const char* policy, const char** games, unsigned int nb_games );

// Return true if the game is allowed on this master
qboolean Game_IsAccepted( const char* game_name );


// ---------- Public constants (game properties) ---------- //

// Heartbeat tag for the DarkPlaces protocol
#define HEARTBEAT_DARKPLACES	"StellarPrey-1"


// ---------- Public types (game properties) ---------- //

typedef enum
{
    GAME_OPTION_NONE				= 0,
    
    // Send empty servers even when the "getservers" requests don't ask for them
    GAME_OPTION_SEND_EMPTY_SERVERS	= ( 1 << 0 ),
    
    // Send full servers even when the "getservers" requests don't ask for them
    GAME_OPTION_SEND_FULL_SERVERS	= ( 1 << 1 ),
} game_options_t;

typedef enum
{
    HEARTBEAT_TYPE_ALIVE,
    HEARTBEAT_TYPE_DEAD,
    
    NB_HEARTBEAT_TYPES,
} heartbeat_type_t;

typedef struct game_properties_s
{
    const char*					name;
    game_options_t				options;
    char*						heartbeats [NB_HEARTBEAT_TYPES];	// Heartbeat tags
    struct game_properties_s*	next;
} game_properties_t;


// ---------- Public functions (game properties) ---------- //

// Initialize the game properties using a built-in list
void Game_InitProperties( void );

// Print the list of known game properties
void Game_PrintProperties( void );

// Update the properties of a game according to the given list of properties
cmdline_status_t Game_UpdateProperties( const char* game, const char** props, size_t nb_props );

// Set the name that is returned when an anonymous game uses an unknown protocol number
cmdline_status_t Game_SetDefaultAnonymous( const char* game );

// Returns the name of a game based on its protocol number
const char* Game_GetNameByProtocol( int protocol, game_options_t* options );

// Returns the properties of the game which uses this heartbeat tag.
// "flatline_heartbeat" will be set to "true" if it's a flatline tag
const game_properties_t* Game_GetPropertiesByHeartbeat( const char* heartbeat_tag, qboolean* flatline_heartbeat );

// Returns the options of a game
game_options_t Game_GetOptions( const char* game );


#endif  // #ifndef _GAMES_H_
