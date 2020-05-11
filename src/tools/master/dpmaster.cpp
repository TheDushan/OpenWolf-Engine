
/*
	dpmaster.c

	An open master server

	Copyright (C) 2002-2011  Mathieu Olivier

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

#include "clients.h"
#include "games.h"
#include "messages.h"
#include "servers.h"


// ---------- Constants ---------- //

// Version of master
#define VERSION "2.2"


// ---------- Private variables ---------- //

// Cross-platform command line options
static const cmdlineopt_t cmdline_options [] =
{
    {
        "allow-loopback",
        NULL,
        "Accept servers on loopback interfaces.\n"
        "   FOR DEBUGGING PURPOSES ONLY!",
        { 0, 0 },
        '\0',
        0,
        0
    },
    {
        "cl-hash-size",
        "<hash_size>",
        "Hash size used for clients, in bits, up to %d (default: %d)",
        { MAX_HASH_SIZE, DEFAULT_CL_HASH_SIZE },
        '\0',
        1,
        1
    },
    {
        "flood-protection",
        NULL,
        "Enable the flood protection against abusive client requests",
        { 0, 0 },
        'f',
        0,
        0
    },
    {
        "fp-decay-time",
        "<decay_time>",
        "Set the decay time of the flood protection, in seconds (default: %d)",
        { DEFAULT_FP_DECAY_TIME, 0 },
        '\0',
        1,
        1
    },
    {
        "fp-throttle",
        "<throttle_limit>",
        "Set the throttle limit of the flood protection (default: %d)",
        { DEFAULT_FP_THROTTLE, 0 },
        '\0',
        1,
        1
    },
    {
        "game-properties",
        "[game_name <property> ...]",
        "Without parameter, print the list of all known games and their properties.\n"
        "   Otherwise, update the properties of a game according to the other parameters.",
        { 0, 0 },
        'g',
        0,
        UINT_MAX
    },
    {
        "game-policy",
        "<accept|reject> <game_name> ...",
        "Accept or reject the listed games. Can be specified more than once only\n"
        "   if all instances set the same policy (\"accept\" or \"reject\").\n"
        "   All non-listed games will implicitely get the opposite policy.",
        { 0, 0 },
        '\0',
        2,
        UINT_MAX
    },
    {
        "help",
        NULL,
        "This help text",
        { 0, 0 },
        'h',
        0,
        0
    },
    {
        "hash-ports",
        NULL,
        "Use both an host's address and port number when computing its hash value.\n"
        "   The check for a maximum number of servers per address won't work correctly.\n"
        "   FOR DEBUGGING PURPOSES ONLY!",
        { 0, 0 },
        '\0',
        0,
        0
    },
    {
        "hash-size",
        "<hash_size>",
        "Hash size used for servers in bits, up to %d (default: %d)",
        { MAX_HASH_SIZE, DEFAULT_SV_HASH_SIZE },
        'H',
        1,
        1
    },
    {
        "listen",
        "<address>",
        "Listen on local address <address>\n"
        "   You can listen on up to %d addresses",
        { MAX_LISTEN_SOCKETS, 0 },
        'l',
        1,
        1
    },
    {
        "log",
        NULL,
        "Enable the logging to disk",
        { 0, 0 },
        'L',
        0,
        0
    },
    {
        "log-file",
        "<file_path>",
        "Use <file_path> as the log file (default: " DEFAULT_LOG_FILE ")",
        { 0, 0 },
        '\0',
        1,
        1
    },
    {
        "map",
        "<a1>=<a2>",
        "Map IPv4 address <a1> to IPv4 address <a2> when sending it to clients\n"
        "   Addresses can contain a port number (ex: myaddr.net:1234)",
        { 0, 0 },
        'm',
        1,
        1
    },
    {
        "max-clients",
        "<max_clients>",
        "Maximum number of clients recorded (default: %d)",
        { DEFAULT_MAX_NB_CLIENTS, 0 },
        '\0',
        1,
        1
    },
    {
        "max-servers",
        "<max_servers>",
        "Maximum number of servers recorded (default: %d)",
        { DEFAULT_MAX_NB_SERVERS, 0 },
        'n',
        1,
        1
    },
    {
        "max-servers-per-addr",
        "<max_per_addr>",
        "Maximum number of servers per IPv4 address or IPv6 subnet (default: %d)\n"
        "   0 means there's no limit",
        { DEFAULT_MAX_NB_SERVERS_PER_ADDRESS, 0 },
        'N',
        1,
        1
    },
    {
        "port",
        "<port_num>",
        "Default network port (default value: %d)",
        { DEFAULT_MASTER_PORT, 0 },
        'p',
        1,
        1
    },
    {
        "verbose",
        "[verbose_lvl]",
        "Verbose level, up to %d (default: %d; no value means max)",
        { MSG_DEBUG, MSG_NORMAL },
        'v',
        0,
        1
    },
    {
        NULL,
        NULL,
        NULL,
        { 0, 0 },
        '\0',
        0,
        0
    }
};


// ---------- Private functions ---------- //

/*
====================
PrintPacket

Print the contents of a packet on stdout
====================
*/
static void PrintPacket( const qbyte* packet, size_t length )
{
    size_t i;
    
    // Exceptionally, we use MSG_NOPRINT here because if the function is
    // called, the user probably wants this text to be displayed
    // whatever the maximum message level is.
    Com_Printf( MSG_NOPRINT, "\"" );
    
    for( i = 0; i < length; i++ )
    {
        qbyte c = packet[i];
        if( c == '\\' )
            Com_Printf( MSG_NOPRINT, "\\\\" );
        else if( c == '\"' )
            Com_Printf( MSG_NOPRINT, "\"" );
        else if( c >= 32 && c <= 127 )
            Com_Printf( MSG_NOPRINT, "%c", c );
        else
            Com_Printf( MSG_NOPRINT, "\\x%02X", c );
    }
    
    Com_Printf( MSG_NOPRINT, "\" (%u bytes)\n", length );
}


/*
====================
UnsecureInit

System independent initializations, called BEFORE the security initializations.
We need this intermediate step because DNS requests may not be able to resolve
after the security initializations, due to chroot.
====================
*/
static qboolean UnsecureInit( void )
{
    // Resolve the address mapping list
    if( ! Sv_ResolveAddressMappings() )
        return qfalse;
        
    // Resolve the listening socket addresses
    if( ! Sys_ResolveListenAddresses() )
        return qfalse;
        
    return qtrue;
}


/*
====================
PrintBanner

Print the
====================
*/
static void PrintBanner( void )
{
    static qboolean banner_printed = qfalse;
    
    if( ! banner_printed )
    {
        Com_Printf( MSG_NORMAL, "OWMaster (version: " VERSION ", compiled: " __DATE__ " at " __TIME__ ")\n" );
        
        banner_printed = qtrue;
    }
}


/*
====================
Cmdline_Option

Parse a system-independent command line option
====================
*/
static cmdline_status_t Cmdline_Option( const cmdlineopt_t* opt, const char** params, unsigned int nb_params )
{
    const char* opt_name;
    
    opt_name = opt->long_name;
    
    // Are servers on loopback interfaces allowed?
    if( strcmp( opt_name, "allow-loopback" ) == 0 )
        allow_loopback = qtrue;
        
    // Flood protection
    else if( strcmp( opt_name, "flood-protection" ) == 0 )
        flood_protection = qtrue;
        
    // Flood protection decay time
    else if( strcmp( opt_name, "fp-decay-time" ) == 0 )
    {
        const char* start_ptr;
        char* end_ptr;
        unsigned int decay_time;
        
        start_ptr = params[0];
        decay_time = ( unsigned int )strtol( start_ptr, &end_ptr, 0 );
        if( end_ptr == start_ptr || *end_ptr != '\0' )
            return CMDLINE_STATUS_INVALID_OPT_PARAMS;
            
        if( ! Cl_SetFPDecayTime( decay_time ) )
            return CMDLINE_STATUS_INVALID_OPT_PARAMS;
    }
    
    // Flood protection throttle limit
    else if( strcmp( opt_name, "fp-throttle" ) == 0 )
    {
        const char* start_ptr;
        char* end_ptr;
        unsigned int throttle;
        
        start_ptr = params[0];
        throttle = ( unsigned int )strtol( start_ptr, &end_ptr, 0 );
        if( end_ptr == start_ptr || *end_ptr != '\0' )
            return CMDLINE_STATUS_INVALID_OPT_PARAMS;
            
        if( ! Cl_SetFPThrottle( throttle ) )
            return CMDLINE_STATUS_INVALID_OPT_PARAMS;
    }
    
    // Client hash size
    else if( strcmp( opt_name, "cl-hash-size" ) == 0 )
    {
        const char* start_ptr;
        char* end_ptr;
        unsigned int hash_size;
        
        start_ptr = params[0];
        hash_size = ( unsigned int )strtol( start_ptr, &end_ptr, 0 );
        if( end_ptr == start_ptr || *end_ptr != '\0' )
            return CMDLINE_STATUS_INVALID_OPT_PARAMS;
            
        if( ! Cl_SetHashSize( hash_size ) )
            return CMDLINE_STATUS_INVALID_OPT_PARAMS;
    }
    
    // Game properties
    else if( strcmp( opt_name, "game-properties" ) == 0 )
    {
        if( nb_params == 0 )
            return CMDLINE_STATUS_SHOW_GAME_PROPERTIES;
        else if( nb_params == 1 )
            return CMDLINE_STATUS_NOT_ENOUGH_OPT_PARAMS;
        else
            return Game_UpdateProperties( params[0], &params[1], nb_params - 1 );
    }
    
    // Game policy
    else if( strcmp( opt_name, "game-policy" ) == 0 )
        return Game_DeclarePolicy( params[0], &params[1], nb_params - 1 );
        
    // Help
    else if( strcmp( opt_name, "help" ) == 0 )
        return CMDLINE_STATUS_SHOW_HELP;
        
    // Hash ports
    else if( strcmp( opt_name, "hash-ports" ) == 0 )
        hash_ports = qtrue;
        
    // Server hash size
    else if( strcmp( opt_name, "hash-size" ) == 0 )
    {
        const char* start_ptr;
        char* end_ptr;
        unsigned int hash_size;
        
        start_ptr = params[0];
        hash_size = ( unsigned int )strtol( start_ptr, &end_ptr, 0 );
        if( end_ptr == start_ptr || *end_ptr != '\0' )
            return CMDLINE_STATUS_INVALID_OPT_PARAMS;
            
        if( ! Sv_SetHashSize( hash_size ) )
            return CMDLINE_STATUS_INVALID_OPT_PARAMS;
    }
    
    // Listen address
    else if( strcmp( opt_name, "listen" ) == 0 )
    {
        const char* param;
        
        param = params[0];
        if( param[0] == '\0' )
            return CMDLINE_STATUS_INVALID_OPT_PARAMS;
            
        if( ! Sys_DeclareListenAddress( param ) )
            return CMDLINE_STATUS_INVALID_OPT_PARAMS;
    }
    
    // Log
    else if( strcmp( opt_name, "log" ) == 0 )
        Com_EnableLog();
        
    // Log file
    else if( strcmp( opt_name, "log-file" ) == 0 )
    {
        if( ! Com_SetLogFilePath( params[0] ) )
            return CMDLINE_STATUS_INVALID_OPT_PARAMS;
    }
    
    // Address mapping
    else if( strcmp( opt_name, "map" ) == 0 )
    {
        if( ! Sv_AddAddressMapping( params[0] ) )
            return CMDLINE_STATUS_INVALID_OPT_PARAMS;
    }
    
    // Maximum number of clients
    else if( strcmp( opt_name, "max-clients" ) == 0 )
    {
        const char* start_ptr;
        char* end_ptr;
        unsigned int max_nb_clients;
        
        start_ptr = params[0];
        max_nb_clients = ( unsigned int )strtol( start_ptr, &end_ptr, 0 );
        if( end_ptr == start_ptr || *end_ptr != '\0' )
            return CMDLINE_STATUS_INVALID_OPT_PARAMS;
            
        if( ! Cl_SetMaxNbClients( max_nb_clients ) )
            return CMDLINE_STATUS_INVALID_OPT_PARAMS;
    }
    
    // Maximum number of servers
    else if( strcmp( opt_name, "max-servers" ) == 0 )
    {
        const char* start_ptr;
        char* end_ptr;
        unsigned int max_nb_servers;
        
        start_ptr = params[0];
        max_nb_servers = ( unsigned int )strtol( start_ptr, &end_ptr, 0 );
        if( end_ptr == start_ptr || *end_ptr != '\0' )
            return CMDLINE_STATUS_INVALID_OPT_PARAMS;
            
        if( ! Sv_SetMaxNbServers( max_nb_servers ) )
            return CMDLINE_STATUS_INVALID_OPT_PARAMS;
    }
    
    // Maximum number of servers per address
    else if( strcmp( opt_name, "max-servers-per-addr" ) == 0 )
    {
        const char* start_ptr;
        char* end_ptr;
        unsigned int max_per_address;
        
        start_ptr = params[0];
        max_per_address = ( unsigned int )strtol( start_ptr, &end_ptr, 0 );
        if( end_ptr == start_ptr || *end_ptr != '\0' )
            return CMDLINE_STATUS_INVALID_OPT_PARAMS;
            
        if( ! Sv_SetMaxNbServersPerAddress( max_per_address ) )
            return CMDLINE_STATUS_INVALID_OPT_PARAMS;
    }
    
    // Port number
    else if( strcmp( opt_name, "port" ) == 0 )
    {
        const char* start_ptr;
        char* end_ptr;
        unsigned short port_num;
        
        start_ptr = params[0];
        port_num = ( unsigned short )strtol( start_ptr, &end_ptr, 0 );
        if( end_ptr == start_ptr || *end_ptr != '\0' || port_num == 0 )
            return CMDLINE_STATUS_INVALID_OPT_PARAMS;
            
        master_port = port_num;
    }
    
    // Verbose level
    else if( strcmp( opt_name, "verbose" ) == 0 )
    {
        // If a verbose level has been specified
        if( nb_params > 0 )
        {
            const char* start_ptr;
            char* end_ptr;
            unsigned int vlevel;
            
            start_ptr = params[0];
            vlevel = ( unsigned int )strtol( start_ptr, &end_ptr, 0 );
            if( end_ptr == start_ptr || *end_ptr != '\0' ||
                    vlevel > MSG_DEBUG )
                return CMDLINE_STATUS_INVALID_OPT_PARAMS;
            max_msg_level = ( msg_level_t )vlevel;
        }
        else
            max_msg_level = MSG_DEBUG;
    }
    
    else
    {
        // If we end there, the array "cmdline_options" must be incorrect
        assert( qfalse );
        return CMDLINE_STATUS_INVALID_OPT;
    }
    
    return CMDLINE_STATUS_OK;
}


/*
====================
PrintCmdlineOptionHelp

Print the help text for a command line option
====================
*/
static void PrintCmdlineOptionHelp( const cmdlineopt_t* opt )
{
    qboolean has_short_name = qboolean( opt->short_name != '\0' );
    
    // Short name, if any
    if( has_short_name )
    {
        Com_Printf( MSG_ERROR, " * -%c", opt->short_name );
        if( opt->help_syntax != NULL )
            Com_Printf( MSG_ERROR, " %s", opt->help_syntax );
        Com_Printf( MSG_ERROR, "\n" );
    }
    
    // Long name
    Com_Printf( MSG_ERROR, " %c --%s",
                has_short_name ? ' ' : '*', opt->long_name );
    if( opt->help_syntax != NULL )
        Com_Printf( MSG_ERROR, " %s", opt->help_syntax );
    Com_Printf( MSG_ERROR, "\n" );
    
    // Description
    Com_Printf( MSG_ERROR, "   " );
    Com_Printf( MSG_ERROR, opt->help_desc,
                opt->help_param[0], opt->help_param[1] );
    Com_Printf( MSG_ERROR, "\n" );
}


/*
====================
ParseCommandLine

Parse the options passed by the command line
====================
*/
static cmdline_status_t ParseCommandLine( int argc, const char* argv [] )
{
    cmdline_status_t cmdline_status = CMDLINE_STATUS_OK;
    const cmdlineopt_t* cmdline_opt = NULL;
    int ind = 1;
    
    while( ind < argc && cmdline_status == CMDLINE_STATUS_OK )
    {
        const char* crt_arg = argv[ind];
        
        cmdline_status = CMDLINE_STATUS_INVALID_OPT;
        cmdline_opt = NULL;
        
        // If it doesn't even look like an option, why bother?
        if( crt_arg[0] == '-' && crt_arg[1] != '\0' )
        {
            const char* first_param = NULL;
            qboolean sys_option = qfalse;
            
            // If it's a long option
            if( crt_arg[1] == '-' )
            {
                const char* equal_char;
                char option_name [64];
                unsigned int cmd_ind;
                
                // Extract the option, and its attached parameter if any
                equal_char = strchr( &crt_arg[2], '=' );
                if( equal_char != NULL )
                {
                    size_t opt_size = equal_char - &crt_arg[2];
                    
                    // If it's an invalid option
                    if( opt_size <= 0 || opt_size >= sizeof( option_name ) )
                        break;
                        
                    memcpy( option_name, &crt_arg[2], opt_size );
                    option_name[opt_size] = '\0';
                    
                    first_param = equal_char + 1;
                }
                else
                {
                    strncpy( option_name, &crt_arg[2], sizeof( option_name ) - 1 );
                    option_name[sizeof( option_name ) - 1] = '\0';
                }
                
                // Cross-platform options
                for( cmd_ind = 0; cmdline_options[cmd_ind].long_name != NULL; cmd_ind++ )
                    if( strcmp( cmdline_options[cmd_ind].long_name, option_name ) == 0 )
                    {
                        cmdline_opt = &cmdline_options[cmd_ind];
                        sys_option = qfalse;
                        break;
                    }
                    
                if( cmdline_opt == NULL )
                {
                    // System-dependent options
                    for( cmd_ind = 0; sys_cmdline_options[cmd_ind].long_name != NULL; cmd_ind++ )
                        if( strcmp( sys_cmdline_options[cmd_ind].long_name, option_name ) == 0 )
                        {
                            cmdline_opt = &sys_cmdline_options[cmd_ind];
                            sys_option = qtrue;
                            break;
                        }
                }
            }
            
            // If it's a short option
            else
            {
                const char short_cmd = crt_arg[1];
                unsigned int cmd_ind;
                
                // Extract the attached parameter if any
                assert( crt_arg[1] != '\0' );
                if( crt_arg[2] != '\0' )
                    first_param = &crt_arg[2];
                    
                // Cross-platform options
                for( cmd_ind = 0; cmdline_options[cmd_ind].long_name != NULL; cmd_ind++ )
                    if( cmdline_options[cmd_ind].short_name == short_cmd )
                    {
                        cmdline_opt = &cmdline_options[cmd_ind];
                        sys_option = qfalse;
                        break;
                    }
                    
                if( cmdline_opt == NULL )
                {
                    // System-dependent options
                    for( cmd_ind = 0; sys_cmdline_options[cmd_ind].long_name != NULL; cmd_ind++ )
                        if( sys_cmdline_options[cmd_ind].short_name == short_cmd )
                        {
                            cmdline_opt = &sys_cmdline_options[cmd_ind];
                            sys_option = qtrue;
                            break;
                        }
                }
                
            }
            
            if( cmdline_opt != NULL )
            {
                unsigned int nb_params;
                int param_ind;
                
                nb_params = ( first_param != NULL ? 1 : 0 );
                param_ind = ind + 1 - nb_params + cmdline_opt->min_params;
                
                // Do we have enough arguments to provide this option with the parameters it needs?
                if( param_ind <= argc )
                {
                    // If we already have a first parameter, start looking at the second one
                    if( nb_params == 1 && cmdline_opt->min_params == 0 )
                        param_ind++;
                    else
                        nb_params = cmdline_opt->min_params;
                        
                    // Gather as many parameters as possible
                    while( param_ind < argc && argv[param_ind][0] != '-' )
                    {
                        param_ind++;
                        nb_params++;
                    }
                    
                    // Don't we have too many parameters for this option?
                    if( nb_params <= cmdline_opt->max_params )
                    {
                        const char** opt_params;
                        qboolean free_opt_params = qfalse;
                        
                        if( first_param != NULL )
                        {
                            if( nb_params > 1 )
                            {
                                // For the most complex cases, we have to allocate a temporary array
                                opt_params = ( const char** )( malloc( nb_params * sizeof( const char* ) ) );
                                if( opt_params != NULL )
                                {
                                    free_opt_params = qtrue;
                                    opt_params[0] = first_param;
                                    memcpy( ( void* )&opt_params[1], argv[ind + 1], ( nb_params - 1 ) * sizeof( const char* ) );
                                }
                                else
                                    cmdline_status = CMDLINE_STATUS_NOT_ENOUGH_MEMORY;
                            }
                            else
                                // Use "first_param" directly if it's the only parameter
                                opt_params = &first_param;
                        }
                        // Use "argv" directly when it's possible
                        else
                            opt_params = &argv[ind + 1];
                            
                        if( sys_option )
                            cmdline_status = Sys_Cmdline_Option( cmdline_opt, opt_params, nb_params );
                        else
                            cmdline_status = Cmdline_Option( cmdline_opt, opt_params, nb_params );
                            
                        if( cmdline_status == CMDLINE_STATUS_OK )
                            ind = param_ind;
                            
                        if( free_opt_params )
                            free( ( void* )opt_params );
                    }
                    else
                        cmdline_status = CMDLINE_STATUS_TOO_MUCH_OPT_PARAMS;
                }
                else
                    cmdline_status = CMDLINE_STATUS_NOT_ENOUGH_OPT_PARAMS;
            }
        }
    }
    
    // If there's a problem with the command line arguments
    if( cmdline_status != CMDLINE_STATUS_OK )
    {
        const char* errormsg_part1, *errormsg_part2, *errormsg_arg;
        
        // Reset the verbose level to make sure the help text will be printed
        max_msg_level = MSG_NORMAL;
        
        // Build the error message
        errormsg_arg = NULL;
        switch( cmdline_status )
        {
            case CMDLINE_STATUS_INVALID_OPT:
                errormsg_part1 = "the option \"";
                errormsg_part2 = "\" is unknown";
                break;
                
            case CMDLINE_STATUS_NOT_ENOUGH_OPT_PARAMS:
                errormsg_part1 = "the option \"--";
                errormsg_part2 = "\" needs more parameters";
                break;
                
            case CMDLINE_STATUS_TOO_MUCH_OPT_PARAMS:
                errormsg_part1 = "the option \"--";
                errormsg_part2 = "\" doesn't take so many parameters";
                break;
                
            case CMDLINE_STATUS_INVALID_OPT_PARAMS:
                errormsg_part1 = "the parameter(s) of the option \"--";
                errormsg_part2 = "\" is/are invalid";
                break;
                
            case CMDLINE_STATUS_NOT_ENOUGH_MEMORY:
                errormsg_part1 = "not enough memory for parsing the option \"--";
                errormsg_part2 = "\"";
                break;
                
            default:
                assert( cmdline_status == CMDLINE_STATUS_SHOW_HELP ||
                        cmdline_status == CMDLINE_STATUS_SHOW_GAME_PROPERTIES );
                errormsg_part1 = NULL;
                errormsg_part2 = NULL;
                break;
        }
        
        if( errormsg_part1 != NULL )
        {
            // Print the banner before the error text
            PrintBanner();
            
            // If we have not been able to identify the faulty option,
            // use the last parsed argument
            if( cmdline_opt != NULL )
                errormsg_arg = cmdline_opt->long_name;
            else
                errormsg_arg = argv[ind];
                
            Com_Printf( MSG_ERROR, "\nERROR: %s%s%s\n",
                        errormsg_part1, errormsg_arg, errormsg_part2 );
                        
            // Print the help text of the faulty option if we have it
            if( cmdline_opt != NULL )
            {
                Com_Printf( MSG_ERROR, "\nSyntax:\n" );
                PrintCmdlineOptionHelp( cmdline_opt );
            }
            
            Com_Printf( MSG_ERROR,
                        "\nUse the \"-h\" or \"--help\" option to display the complete help text\n\n" );
        }
    }
    
    return cmdline_status;
}


/*
====================
PrintCmdlineOptionsHelp

Print the help text for a pool of command line options
====================
*/
static void PrintCmdlineOptionsHelp( const char* pool_name, const cmdlineopt_t* opts )
{
    // If the option list isn't empty
    if( opts[0].long_name != NULL )
    {
        unsigned int cmd_ind;
        
        Com_Printf( MSG_ERROR, "Available %s options are:\n", pool_name );
        
        for( cmd_ind = 0; opts[cmd_ind].long_name != NULL; cmd_ind++ )
        {
            PrintCmdlineOptionHelp( &opts[cmd_ind] );
            Com_Printf( MSG_ERROR, "\n" );
        }
    }
}


/*
====================
PrintHelp

Print the command line syntax and the available options
====================
*/
static void PrintHelp( void )
{
    Com_Printf( MSG_ERROR, "\nSyntax: OWMaster [options]\n\n" );
    
    PrintCmdlineOptionsHelp( "cross-platform", cmdline_options );
    PrintCmdlineOptionsHelp( "platform-specific", sys_cmdline_options );
}


/*
====================
SecureInit

System independent initializations, called AFTER the security initializations
====================
*/
static qboolean SecureInit( void )
{
    // Init the time and the random seed
    crt_time = time( NULL );
    srand( ( unsigned int )crt_time );
    
#ifdef SIGUSR1
    if( signal( SIGUSR1, Com_SignalHandler ) == SIG_ERR )
    {
        Com_Printf( MSG_ERROR, "> ERROR: can't capture the SIGUSR1 signal\n" );
        return qfalse;
    }
#endif
#ifdef SIGUSR2
    if( signal( SIGUSR2, Com_SignalHandler ) == SIG_ERR )
    {
        Com_Printf( MSG_ERROR, "> ERROR: can't capture the SIGUSR2 signal\n" );
        return qfalse;
    }
#endif
    
    if( ! Sys_CreateListenSockets() )
        return qfalse;
        
    // If there no socket to listen to for whatever reason, there's simply nothing to do
    if( nb_sockets <= 0 )
    {
        Com_Printf( MSG_ERROR, "> ERROR: there's no listening socket. There's nothing to do\n" );
        return qfalse;
    }
    
    // Initialize the server list and hash table
    if( ! Sv_Init() )
        return qfalse;
        
    // Initialize the client list and hash table (query rate throttling)
    if( ! Cl_Init() )
        return qfalse;
        
    return qtrue;
}


/*
====================
main

Main function
====================
*/
int main( int argc, const char* argv [] )
{
    cmdline_status_t valid_options;
    
    // Game properties must be initialized first, since the user
    // may modify them using the command line's arguments
    Game_InitProperties();
    
    // Get the options from the command line
    valid_options = ParseCommandLine( argc, argv );
    
    PrintBanner();
    
    // If something goes wrong with the command line, exit
    if( valid_options != CMDLINE_STATUS_OK )
    {
        switch( valid_options )
        {
            case CMDLINE_STATUS_SHOW_HELP:
                PrintHelp();
                break;
                
            case CMDLINE_STATUS_SHOW_GAME_PROPERTIES:
                Game_PrintProperties();
                break;
                
            default:
                // Nothing
                break;
        }
        
        return EXIT_FAILURE;
    }
    
    // Start the log if necessary
    if( ! Com_UpdateLogStatus( qtrue ) )
        return EXIT_FAILURE;
        
    crt_time = time( NULL );
    print_date = qtrue;
    
    // Initializations
    if( ! Sys_UnsecureInit() || ! UnsecureInit() ||
            ! Sys_SecurityInit() ||
            ! Sys_SecureInit() || ! SecureInit() )
        return EXIT_FAILURE;
        
    // Until the end of times...
    for( ;; )
    {
        fd_set sock_set;
        socket_t max_sock;
        size_t sock_ind;
        int nb_sock_ready;
        
        FD_ZERO( &sock_set );
        max_sock = INVALID_SOCKET;
        for( sock_ind = 0; sock_ind < nb_sockets; sock_ind++ )
        {
            socket_t crt_sock = listen_sockets[sock_ind].socket;
            
            FD_SET( crt_sock, &sock_set );
            if( max_sock == INVALID_SOCKET || max_sock < crt_sock )
                max_sock = crt_sock;
        }
        
        // Flush the console and log file
        if( Com_IsLogEnabled() )
            Com_FlushLog();
        if( daemon_state < DAEMON_STATE_EFFECTIVE )
            fflush( stdout );
            
        nb_sock_ready = select( ( int )( max_sock + 1 ), &sock_set, NULL, NULL, NULL );
        
        // Update the current time
        crt_time = time( NULL );
        
        print_date = qfalse;
        Com_UpdateLogStatus( qfalse );
        
        // Print the date once per select()
        print_date = qtrue;
        
        if( nb_sock_ready <= 0 )
        {
            if( Sys_GetLastNetError() != NETERR_INTR )
                Com_Printf( MSG_WARNING,
                            "> WARNING: \"select\" returned %d\n",
                            nb_sock_ready );
            continue;
        }
        
        for( sock_ind = 0;
                sock_ind < nb_sockets && nb_sock_ready > 0;
                sock_ind++ )
        {
            struct sockaddr_storage address;
            socklen_t addrlen;
            int nb_bytes;
            char packet [MAX_PACKET_SIZE_IN + 1];  // "+ 1" because we append a '\0'
            socket_t crt_sock = listen_sockets[sock_ind].socket;
            
            if( ! FD_ISSET( crt_sock, &sock_set ) )
                continue;
            nb_sock_ready--;
            
            // Get the next valid message
            addrlen = sizeof( address );
            nb_bytes = recvfrom( crt_sock, packet, sizeof( packet ) - 1, 0,
                                 ( struct sockaddr* )&address, &addrlen );
                                 
            if( nb_bytes <= 0 )
            {
                Com_Printf( MSG_WARNING,
                            "> WARNING: \"recvfrom\" returned %d\n", nb_bytes );
                continue;
            }
            
            // If we may print something, rebuild the peer address string
            if( max_msg_level > MSG_NOPRINT &&
                    ( Com_IsLogEnabled() || daemon_state < DAEMON_STATE_EFFECTIVE ) )
            {
                strncpy( peer_address, Sys_SockaddrToString( &address, addrlen ),
                         sizeof( peer_address ) );
                peer_address[sizeof( peer_address ) - 1] = '\0';
            }
            
            // We print the packet contents if necessary
            if( max_msg_level >= MSG_DEBUG )
            {
                Com_Printf( MSG_DEBUG, "> New packet received from %s: ",
                            peer_address );
                PrintPacket( ( qbyte* )packet, nb_bytes );
            }
            
            // A few sanity checks
            if( address.ss_family != AF_INET && address.ss_family != AF_INET6 )
            {
                Com_Printf( MSG_WARNING,
                            "> WARNING: rejected packet from %s (invalid address family: %hd)\n",
                            peer_address, address.ss_family );
                continue;
            }
            if( Sys_GetSockaddrPort( &address ) == 0 )
            {
                Com_Printf( MSG_WARNING,
                            "> WARNING: rejected packet from %s (source port = 0)\n",
                            peer_address );
                continue;
            }
            if( nb_bytes < MIN_PACKET_SIZE_IN )
            {
                Com_Printf( MSG_WARNING,
                            "> WARNING: rejected packet from %s (size = %d bytes)\n",
                            peer_address, nb_bytes );
                continue;
            }
            if( packet[0] != '\xFF' || packet[1] != '\xFF' || packet[2] != '\xFF' || packet[3] != '\xFF' )
            {
                Com_Printf( MSG_WARNING,
                            "> WARNING: rejected packet from %s (invalid header)\n",
                            peer_address );
                continue;
            }
            
            // Append a '\0' to make the parsing easier
            packet[nb_bytes] = '\0';
            
            // Call HandleMessage with the remaining contents
            HandleMessage( packet + 4, nb_bytes - 4, &address, addrlen, crt_sock );
        }
    }
}
