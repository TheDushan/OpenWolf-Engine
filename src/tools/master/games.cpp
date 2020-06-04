/*
	games.c

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


#include "common.h"
#include "system.h"
#include "games.h"


// ---------- Private variables ---------- //

static const char** game_names = NULL;
static unsigned int nb_game_names = 0;
static qboolean reject_when_known = qtrue;


// ---------- Private functions ---------- //

/*
====================
Game_Find

Find a game name in the list of game names
After the call, *index_ptr will contain the index where the game is stored in game_names (or should be stored, if it is not present)
====================
*/
static qboolean Game_Find( const char* game_name, unsigned int* index_ptr )
{
    int left = 0;
    
    if( game_names != NULL )
    {
        int right = nb_game_names - 1;
        
        while( left <= right )
        {
            int middle, diff;
            
            middle = ( left + right ) / 2;
            diff = strcmp( game_names[middle], game_name );
            
            if( diff == 0 )
            {
                if( index_ptr != NULL )
                    *index_ptr = middle;
                return qtrue;
            }
            
            if( diff > 0 )
                right = middle - 1;
            else
                left = middle + 1;
        }
    }
    
    if( index_ptr != NULL )
        *index_ptr = left;
    return qfalse;
}


// ---------- Public functions (game policy) ---------- //

/*
====================
Game_DeclarePolicy

Declare the server policy regarding which games are allowed on this master
====================
*/
cmdline_status_t Game_DeclarePolicy( const char* policy, const char** games, unsigned int nb_games )
{
    qboolean new_reject_when_known;
    unsigned int i;
    
    if( strcmp( policy, "accept" ) == 0 )
        new_reject_when_known = qfalse;
    else if( strcmp( policy, "reject" ) == 0 )
        new_reject_when_known = qtrue;
    else
        return CMDLINE_STATUS_INVALID_OPT_PARAMS;
        
    // If this is the first game policy option we parse, assign the default game policy
    if( game_names == NULL )
        reject_when_known = new_reject_when_known;
        
    // Else, this list must be compatible with the previous one(s)
    else if( new_reject_when_known != reject_when_known )
        return CMDLINE_STATUS_INVALID_OPT_PARAMS;
        
    for( i = 0; i < nb_games; i++ )
    {
        unsigned int index;
        const char* game = games[i];
        
        // If we don't already have this game in the list, add it
        if( ! Game_Find( game, &index ) )
        {
            const char** new_game_names;
            
            new_game_names = ( const char** )realloc( ( void* )game_names, ( nb_game_names + 1 ) * sizeof( game_names[0] ) );
            if( new_game_names == NULL )
                return CMDLINE_STATUS_NOT_ENOUGH_MEMORY;
                
            memmove( ( void* )&new_game_names[index + 1], &new_game_names[index], ( nb_game_names - index ) * sizeof( new_game_names[0] ) );
            new_game_names[index] = game;
            
            game_names = new_game_names;
            nb_game_names++;
        }
    }
    
    return CMDLINE_STATUS_OK;
}


/*
====================
Game_IsAccepted

Return qtrue if the game is allowed on this master
====================
*/
qboolean Game_IsAccepted( const char* game_name )
{
    return qboolean( Game_Find( game_name, NULL ) ^ reject_when_known );
}


// ---------- Private constants ---------- //

// Gamenames
#define GAMENAME_Q3A	"Quake3Arena"	// Quake 3 Arena
#define GAMENAME_RTCW	"wolfmp"		// Return to Castle Wolfenstein
#define GAMENAME_WOET	"et"			// Wolfenstein: Enemy Territory
#define GAMENAME_ST     "StellarPrey"   // Stellar Prey

// ---------- Private types (game properties) ---------- //

typedef struct
{
    game_options_t	value;
    const char*		string;
} game_option_string_t;

typedef struct
{
    int					protocol;
    game_properties_t*	game;
} game_protocol_assoc_t;


// ---------- Private variables (game properties) ---------- //

static const game_option_string_t option_strings [] =
{
    { GAME_OPTION_SEND_EMPTY_SERVERS,	"send-empty-servers"	},
    { GAME_OPTION_SEND_FULL_SERVERS,	"send-full-servers"		},
    
    { GAME_OPTION_NONE, NULL },		// Marks the end of the list
};

static game_properties_t* game_properties_list = NULL;

static game_protocol_assoc_t* game_protocols = NULL;	// TODO: sort it to allow binary searches
static unsigned int nb_game_protocols = 0;


// ---------- Private functions (game properties) ---------- //

/*
====================
Game_GetAnonymous

Returns the properties of an anonymous game
====================
*/
static game_properties_t* Game_GetAnonymous( const char* game, qboolean creation_allowed )
{
    game_properties_t* props = game_properties_list;
    
    while( props != NULL )
    {
        if( strcmp( game, props->name ) == 0 )
            return props;
            
        props = props->next;
    }
    
    if( creation_allowed )
    {
        // Empty game names, or game names containing spaces aren't allowed
        if( game[0] != '\0' && strchr( game, ' ' ) == NULL )
        {
            props = ( game_properties_t* )malloc( sizeof( *props ) );
            if( props != NULL )
            {
                memset( props, 0, sizeof( *props ) );
                props->name = game;
                
                props->next = game_properties_list;
                game_properties_list = props;
                
                return props;
            }
        }
    }
    
    return NULL;
}


/*
====================
Game_FindAnonymous

Find game properties in the list of game protocols
After the call, *index_ptr will contain the index where the game is stored in game_names (or should be stored, if it is not present)
====================
*/
static qboolean Game_FindAnonymous( int protocol, unsigned int* index_ptr )
{
    int left = 0;
    
    if( game_protocols != NULL )
    {
        int right = nb_game_protocols - 1;
        
        while( left <= right )
        {
            int middle;
            
            middle = ( left + right ) / 2;
            
            if( game_protocols[middle].protocol == protocol )
            {
                if( index_ptr != NULL )
                    *index_ptr = middle;
                return qtrue;
            }
            
            if( game_protocols[middle].protocol > protocol )
                right = middle - 1;
            else
                left = middle + 1;
        }
    }
    
    if( index_ptr != NULL )
        *index_ptr = left;
    return qfalse;
}


/*
====================
Game_RemoveAllProtocols

Remove all the protocol numbers associated to a game
====================
*/
static void Game_RemoveAllProtocols( game_properties_t* game_props )
{
    unsigned int proto_ind, proto_copy_ind;
    
    proto_copy_ind = 0;
    for( proto_ind = 0; proto_ind < nb_game_protocols; proto_ind++ )
    {
        if( game_protocols[proto_ind].game != game_props )
        {
            if( proto_ind != proto_copy_ind )
                memcpy( &game_protocols[proto_copy_ind], &game_protocols[proto_ind], sizeof( game_protocols[proto_copy_ind] ) );
            proto_copy_ind++;
        }
    }
    
    nb_game_protocols = proto_copy_ind;
}


/*
====================
Game_AddProtocol

Add a protocol number to the properties of a game
====================
*/
static cmdline_status_t Game_AddProtocol( game_properties_t* game_props, int protocol )
{
    unsigned int index;
    
    if( ! Game_FindAnonymous( protocol, &index ) )
    {
        game_protocol_assoc_t* new_array;
        
        new_array = ( game_protocol_assoc_t* )realloc( game_protocols, ( nb_game_protocols + 1 ) * sizeof( game_protocols[0] ) );
        if( new_array == NULL )
            return CMDLINE_STATUS_NOT_ENOUGH_MEMORY;
        game_protocols = new_array;
        
        memmove( &game_protocols[index + 1], &game_protocols[index], ( nb_game_protocols - index ) * sizeof( game_protocols[0] ) );
        game_protocols[index].protocol = protocol;
        nb_game_protocols++;
    }
    
    game_protocols[index].game = game_props;
    return CMDLINE_STATUS_OK;
}


/*
====================
Game_RemoveProtocol

Remove a protocol number from the properties of a game
====================
*/
static cmdline_status_t Game_RemoveProtocol( game_properties_t* game_props, int protocol )
{
    unsigned int index;
    
    // FIXME? shouldn't we abort if the protocol number isn't used?
    if( Game_FindAnonymous( protocol, &index ) )
    {
        if( game_protocols[index].game != game_props )
            return CMDLINE_STATUS_INVALID_OPT_PARAMS;
            
        memmove( &game_protocols[index], &game_protocols[index + 1], ( nb_game_protocols - index - 1 ) * sizeof( game_protocols[0] ) );
        nb_game_protocols--;
    }
    
    return CMDLINE_STATUS_OK;
}


/*
====================
Game_RemoveHeartbeat

Remove an heartbeat tag from the properties of a game
====================
*/
static void Game_RemoveHeartbeat( game_properties_t* game_props, heartbeat_type_t hb_type )
{
    char* tag = game_props->heartbeats[hb_type];
    
    if( tag != NULL )
    {
        free( tag );
        game_props->heartbeats[hb_type] = NULL;
    }
}


/*
====================
Game_UpdateHeartbeat

Add or remove an heartbeat to the properties of a game
====================
*/
static cmdline_status_t Game_UpdateHeartbeat( game_properties_t* game_props, heartbeat_type_t hb_type, const char* value, qboolean remove )
{
    if( remove )
    {
        if( strcmp( game_props->heartbeats[hb_type], value ) == 0 )
            Game_RemoveHeartbeat( game_props, hb_type );
    }
    else
    {
        // The tag used by the DarkPlaces protocol is reserved
        if( strcmp( value, HEARTBEAT_DARKPLACES ) == 0 )
            return CMDLINE_STATUS_INVALID_OPT_PARAMS;
            
        // You can't have more than one heartbeat tag for each type
        if( game_props->heartbeats[hb_type] != NULL )
            return CMDLINE_STATUS_INVALID_OPT_PARAMS;
            
        game_props->heartbeats[hb_type] = strdup( value );
        if( game_props->heartbeats[hb_type] == NULL )
            return CMDLINE_STATUS_NOT_ENOUGH_MEMORY;
    }
    
    return CMDLINE_STATUS_OK;
}


/*
====================
Game_UpdateOption

Add or remove an option to the properties of a game
====================
*/
static cmdline_status_t Game_UpdateOption( game_properties_t* game_props, const char* option_name, qboolean remove )
{
    size_t option_ind;
    
    for( option_ind = 0; option_strings[option_ind].value != GAME_OPTION_NONE; option_ind++ )
    {
        const game_option_string_t* option = &option_strings[option_ind];
        
        if( strcmp( option->string, option_name ) == 0 )
        {
            if( remove )
                game_props->options = ( game_options_t )( game_props->options & ~( option->value ) );
            else
                game_props->options = ( game_options_t )( game_props->options | ( option->value ) );
            return CMDLINE_STATUS_OK;
        }
    }
    
    return CMDLINE_STATUS_INVALID_OPT_PARAMS;
}


/*
====================
Game_UpdateProperty

Update a game property
====================
*/
static cmdline_status_t Game_UpdateProperty( game_properties_t* game_props, const char* prop_name, char* prop_value,
        qboolean reset_property, qboolean remove_values )
{
    char* next_value;
    
    // No property accepts values with a blank space in it so far
    if( strchr( prop_value, ' ' ) != NULL )
        return CMDLINE_STATUS_INVALID_OPT_PARAMS;
        
    if( reset_property )
    {
        if( strcmp( prop_name, "protocols" ) == 0 )
        {
            Game_RemoveAllProtocols( game_props );
        }
        else if( strcmp( prop_name, "options" ) == 0 )
        {
            game_props->options = GAME_OPTION_NONE;
        }
        else if( strcmp( prop_name, "heartbeat" ) == 0 )
        {
            Game_RemoveHeartbeat( game_props, HEARTBEAT_TYPE_ALIVE );
        }
        else if( strcmp( prop_name, "flatline" ) == 0 )
        {
            Game_RemoveHeartbeat( game_props, HEARTBEAT_TYPE_DEAD );
        }
        else
            return CMDLINE_STATUS_INVALID_OPT_PARAMS;
    }
    
    next_value = strtok( prop_value, "," );
    while( next_value != NULL )
    {
        cmdline_status_t result;
        
        if( strcmp( prop_name, "protocols" ) == 0 )
        {
            char* end_ptr;
            
            int protocol = ( int )strtol( next_value, &end_ptr, 0 );
            if( *end_ptr != '\0' )
                return CMDLINE_STATUS_INVALID_OPT_PARAMS;
                
            if( remove_values )
                result = Game_RemoveProtocol( game_props, protocol );
            else
                result = Game_AddProtocol( game_props, protocol );
            if( result != CMDLINE_STATUS_OK )
                return result;
        }
        else if( strcmp( prop_name, "options" ) == 0 )
        {
            result = Game_UpdateOption( game_props, next_value, remove_values );
            if( result != CMDLINE_STATUS_OK )
                return result;
        }
        else if( strcmp( prop_name, "heartbeat" ) == 0 )
        {
            result = Game_UpdateHeartbeat( game_props, HEARTBEAT_TYPE_ALIVE, next_value, remove_values );
            if( result != CMDLINE_STATUS_OK )
                return result;
        }
        else if( strcmp( prop_name, "flatline" ) == 0 )
        {
            result = Game_UpdateHeartbeat( game_props, HEARTBEAT_TYPE_DEAD, next_value, remove_values );
            if( result != CMDLINE_STATUS_OK )
                return result;
        }
        else
            return CMDLINE_STATUS_INVALID_OPT_PARAMS;
            
        next_value = strtok( NULL, "," );
    }
    
    return CMDLINE_STATUS_OK;
}


// ---------- Public functions (game properties) ---------- //

/*
====================
Game_InitProperties

Initialize the game properties using a built-in list
====================
*/
void Game_InitProperties( void )
{
    typedef struct
    {
        const char*		gamename;
        size_t			nb_props;
        const char*		props [4];
    } builtin_props_t;
    
    builtin_props_t builtin_props_array [] =
    {
        // Quake 3 Arena
        {
            GAMENAME_Q3A,
            2,
            {
                "protocols=66,67,68",
                "heartbeat=QuakeArena-1",
            },
        },
        
        // Return to Castle Wolfenstein
        {
            GAMENAME_RTCW,
            3,
            {
                "protocols=50,59,60",
                "heartbeat=Wolfenstein-1",
                "flatline=WolfFlatline-1",
            },
        },
        
        // Wolfenstein: Enemy Territory
        {
            GAMENAME_WOET,
            4,
            {
                "protocols=72,80,83,84",
                "options=send-empty-servers,send-full-servers",
                "heartbeat=EnemyTerritory-1",
                "flatline=ETFlatline-1",
            },
        },
        
        // Stellar Prey
        {
            GAMENAME_ST,
            4,
            {
                "protocols=1,2",
                "options=send-empty-servers,send-full-servers",
                "heartbeat=StellarPray-1",
                "flatline=StellarPrayFlatline-1",
            },
        },
        
    };
    
    size_t game_count = sizeof( builtin_props_array ) / sizeof( builtin_props_array[0] );
    size_t game_ind;
    
    for( game_ind = 0; game_ind < game_count; game_ind++ )
    {
        builtin_props_t* builtin_props = &builtin_props_array[game_ind];
        Game_UpdateProperties( builtin_props->gamename, builtin_props->props, builtin_props->nb_props );
    }
}


/*
====================
Game_PrintProperties

Print the list of known game properties
====================
*/
void Game_PrintProperties( void )
{
    const game_properties_t* props;
    const char* hb_types [] = { "alive", "dead" };
    
    Com_Printf( MSG_ERROR, "\nGame properties:\n" );
    
    props = game_properties_list;
    while( props != NULL )
    {
        unsigned int count, ind;
        
        // Name
        Com_Printf( MSG_ERROR, "* %s:\n", props->name );
        
        // Protocols
        Com_Printf( MSG_ERROR, "   - protocols:" );
        count = 0;
        for( ind = 0; ind < nb_game_protocols; ind++ )
        {
            const game_protocol_assoc_t* assoc = &game_protocols[ind];
            
            if( assoc->game == props )
            {
                Com_Printf( MSG_ERROR, "%s %d", count > 0 ? "," : "",
                            assoc->protocol );
                count++;
            }
        }
        if( count == 0 )
            Com_Printf( MSG_ERROR, " none" );
        Com_Printf( MSG_ERROR, "\n" );
        
        // Options
        Com_Printf( MSG_ERROR, "   - options:" );
        count = 0;
        if( props->options != GAME_OPTION_NONE )
        {
            for( ind = 0; option_strings[ind].value != GAME_OPTION_NONE; ind++ )
            {
                const game_option_string_t* option = &option_strings[ind];
                
                if( ( props->options & option->value ) != 0 )
                {
                    Com_Printf( MSG_ERROR, "%s %s", count > 0 ? "," : "",
                                option->string );
                    count++;
                }
            }
        }
        else
            Com_Printf( MSG_ERROR, " none" );
        Com_Printf( MSG_ERROR, "\n" );
        
        // Heartbeats
        Com_Printf( MSG_ERROR, "   - heartbeats:" );
        count = 0;
        for( ind = 0; ind < sizeof( props->heartbeats ) / sizeof( props->heartbeats[0] ); ind++ )
        {
            const char* tag = props->heartbeats[ind];
            
            if( tag != NULL )
            {
                Com_Printf( MSG_ERROR, "%s %s (%s)", count > 0 ? "," : "",
                            tag, hb_types[ind] );
                count++;
            }
        }
        if( count == 0 )
            Com_Printf( MSG_ERROR, " none" );
        Com_Printf( MSG_ERROR, "\n" );
        
        Com_Printf( MSG_ERROR, "\n" );
        props = props->next;
    }
}


/*
====================
Game_UpdateProperties

Update the properties of a game according to the given list of properties
====================
*/
cmdline_status_t Game_UpdateProperties( const char* game, const char** props, size_t nb_props )
{
    unsigned int prop_ind;
    game_properties_t* game_props = Game_GetAnonymous( game, qtrue );
    
    if( game_props == NULL )
        return CMDLINE_STATUS_NOT_ENOUGH_MEMORY;
        
    // Parse the properties and apply them
    for( prop_ind = 0; prop_ind < nb_props; prop_ind++ )
    {
        char* work_buff = strdup( props[prop_ind] );
        char* equal_sign;
        
        if( work_buff == NULL )
            return CMDLINE_STATUS_NOT_ENOUGH_MEMORY;
            
        equal_sign = strchr( work_buff, '=' );
        if( equal_sign != NULL && equal_sign != work_buff )
        {
            cmdline_status_t result;
            qboolean reset_property, remove_values;
            
            if( equal_sign[-1] == '+' )
            {
                equal_sign[-1] = '\0';
                reset_property = qfalse;
                remove_values = qfalse;
            }
            else if( equal_sign[-1] == '-' )
            {
                equal_sign[-1] = '\0';
                reset_property = qfalse;
                remove_values = qtrue;
            }
            else
            {
                equal_sign[0] = '\0';
                reset_property = qtrue;
                remove_values = qfalse;
            }
            
            result = Game_UpdateProperty( game_props, work_buff, equal_sign + 1, reset_property, remove_values );
            free( work_buff );
            
            if( result != CMDLINE_STATUS_OK )
                return result;
            else
                continue;
        }
        
        free( work_buff );
        return CMDLINE_STATUS_INVALID_OPT_PARAMS;
    }
    
    return CMDLINE_STATUS_OK;
}


/*
====================
Game_GetNameByProtocol

Returns the name of a game based on its protocol number
====================
*/
const char* Game_GetNameByProtocol( int protocol, game_options_t* options )
{
    const game_properties_t* props;
    unsigned int index;
    
    if( Game_FindAnonymous( protocol, &index ) )
    {
        props = game_protocols[index].game;
        
        if( options != NULL )
            *options = props->options;
        return props->name;
    }
    
    return NULL;
}


/*
====================
Game_GetPropertiesByHeartbeat

Returns the properties of the game which uses this heartbeat tag.
"flatline_heartbeat" will be set to "qtrue" if it's a flatline tag
====================
*/
const game_properties_t* Game_GetPropertiesByHeartbeat( const char* heartbeat_tag, qboolean* flatline_heartbeat )
{
    game_properties_t* props = game_properties_list;
    
    while( props != NULL )
    {
        size_t hb_ind;
        
        for( hb_ind = 0; hb_ind < NB_HEARTBEAT_TYPES; hb_ind++ )
        {
            const char* tag = props->heartbeats[hb_ind];
            
            if( tag != NULL && strcmp( heartbeat_tag, tag ) == 0 )
            {
                *flatline_heartbeat = qboolean( hb_ind == HEARTBEAT_TYPE_DEAD );
                return props;
            }
        }
        
        props = props->next;
    }
    
    *flatline_heartbeat = qfalse;
    return NULL;
}


/*
====================
Game_GetOptions

Returns the options of a game
====================
*/
game_options_t Game_GetOptions( const char* game )
{
    const game_properties_t* props = Game_GetAnonymous( game, qfalse );
    
    if( props != NULL )
        return props->options;
    else
        return GAME_OPTION_NONE;
}
