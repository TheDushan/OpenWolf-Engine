/*
	common.h

	Common header file for dpmaster

	Copyright (C) 2004-2011  Mathieu Olivier

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


#ifndef _COMMON_H_
#define _COMMON_H_


#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>


#ifdef WIN32
#	include <winsock2.h>
#   include <Winsock.h>
#	include <ws2tcpip.h>
#else
#	include <pwd.h>
#	include <unistd.h>
#	include <netinet/in.h>
#	include <arpa/inet.h>
#	include <netdb.h>
#	include <sys/socket.h>
#endif


// ---------- Constants ---------- //

// Maximum and minimum sizes for a valid incoming packet
#define MAX_PACKET_SIZE_IN 2048
#define MIN_PACKET_SIZE_IN 5

// Maximum address hash size in bits
#define MAX_HASH_SIZE 16


// ---------- Types ---------- //

// A few basic types
typedef enum {qfalse, qtrue} qboolean;
typedef unsigned char qbyte;

// The various messages levels
typedef enum
{
    MSG_NOPRINT,	// used by "max_msg_level" (= no printings)
    MSG_ERROR,		// errors
    MSG_WARNING,	// warnings
    MSG_NORMAL,		// standard messages
    MSG_DEBUG		// for debugging purpose
} msg_level_t;

// Command line option
typedef struct
{
    const char* long_name;		// if NULL, this is the end of the list
    const char* help_syntax;	// help string printed by PrintHelp (syntax)
    const char* help_desc;		// help string printed by PrintHelp (description)
    int	help_param [2];			// optional parameters for the "help_desc" string
    char short_name;			// may be '\0' if it has no short name
    unsigned int min_params;	// minimum number of parameters for this option
    unsigned int max_params;	// maximum number of parameters for this option
}  cmdlineopt_t;

// Command line status
typedef enum
{
    CMDLINE_STATUS_OK,
    CMDLINE_STATUS_SHOW_HELP,
    CMDLINE_STATUS_SHOW_GAME_PROPERTIES,
    
    // Errors
    CMDLINE_STATUS_INVALID_OPT,
    CMDLINE_STATUS_NOT_ENOUGH_OPT_PARAMS,
    CMDLINE_STATUS_TOO_MUCH_OPT_PARAMS,
    CMDLINE_STATUS_INVALID_OPT_PARAMS,
    CMDLINE_STATUS_NOT_ENOUGH_MEMORY,
} cmdline_status_t;

// User (client or server)
typedef struct user_s
{
    struct sockaddr_storage address;
    socklen_t addrlen;
    struct user_s* next;
    struct user_s** prev_ptr;
} user_t;

// Hash table for users
typedef struct user_hash_table_s
{
    user_t** entries;
} user_hash_table_t;


// ---------- Public variables ---------- //

// The current time (updated every time we receive a packet)
extern time_t crt_time;

// Maximum level for a message to be printed
extern msg_level_t max_msg_level;

// Peer address. We rebuild it every time we receive a new packet
extern char peer_address [128];

// Should we print the date before any new console message?
extern qboolean print_date;

// Are port numbers used when computing address hashes?
extern qboolean hash_ports;


// ---------- Public functions (user hash table) ---------- //

// Initialize a user hash table
qboolean Com_UserHashTable_Init( user_hash_table_t* table,
                                 size_t hash_size,
                                 const char* table_name );

// Add a user to the hash table
void Com_UserHashTable_Add( user_hash_table_t* table, user_t* user, unsigned int hash );

// Remove a user from its hash table
void Com_UserHashTable_Remove( user_t* user );


// ---------- Public functions (logging) ---------- //

// Enable the logging
void Com_EnableLog( void );

// Flush the buffer of the log file
void Com_FlushLog( void );

// Test if the logging is enabled
qboolean Com_IsLogEnabled( void );

// Set the log file path (taken into account the next time it's opened)
qboolean Com_SetLogFilePath( const char* filepath );

// Update the logging status, opening or closing the log file when necessary
qboolean Com_UpdateLogStatus( qboolean init );


// ---------- Public functions (misc) ---------- //

// Print a text to the screen and/or to the log file
void Com_Printf( msg_level_t msg_level, const char* format, ... );

// Handling of the signals sent to this process
void Com_SignalHandler( int Signal );

// Compute the hash of a server address
unsigned int Com_AddressHash( const struct sockaddr_storage* address, size_t hash_size );

// Compare 2 IPv4 addresses and return "true" if they're equal
qboolean Com_SameIPv4Addr( const struct sockaddr_storage* addr1, const struct sockaddr_storage* addr2, qboolean* same_public_address );

// Compare 2 IPv6 addresses and return "true" if they're equal
qboolean Com_SameIPv6Addr( const struct sockaddr_storage* addr1, const struct sockaddr_storage* addr2, qboolean* same_public_address );


#endif  // #ifndef _COMMON_H_
