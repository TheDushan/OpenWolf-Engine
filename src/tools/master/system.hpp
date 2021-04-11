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
// File name:   system.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: System specific code for dpmaster
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

// ---------- Contants ---------- //

// The default name of the log file
#ifdef WIN32
#	define DEFAULT_LOG_FILE "dpmaster.log"
#else
#	define DEFAULT_LOG_FILE "/var/log/dpmaster.log"
#endif

// The maximum number of listening sockets
#define MAX_LISTEN_SOCKETS 8

// Default master port
#define DEFAULT_MASTER_PORT 12950

// Network errors code
#ifdef WIN32
#	define NETERR_AFNOSUPPORT	WSAEAFNOSUPPORT
#	define NETERR_NOPROTOOPT	WSAENOPROTOOPT
#	define NETERR_INTR			WSAEINTR
#else
#	define NETERR_AFNOSUPPORT	EAFNOSUPPORT
#	define NETERR_NOPROTOOPT	ENOPROTOOPT
#	define NETERR_INTR			EINTR
#endif

// Windows' CRT wants an explicit buffer size for its setvbuf() calls
#ifndef WIN32
#	define SETVBUF_DEFAULT_SIZE 0
#else
#	define SETVBUF_DEFAULT_SIZE 4096
#endif

// Value used to specify an invalid socket
#ifndef WIN32
#	define INVALID_SOCKET (-1)
#endif

#ifndef MAX_PATH
#	define MAX_PATH PATH_MAX
#endif

// ---------- Public types ---------- //

#ifdef WIN32
typedef SOCKET socket_t;
typedef u_short sa_family_t;
#else
typedef int socket_t;
#endif

// Listening socket
typedef struct
{
    socket_t socket;
    socklen_t local_addr_len;
    const char* local_addr_name;
    struct sockaddr_storage local_addr;
    qboolean optional;
} listen_socket_t;

// The steps for running as a daemon (no console output)
typedef enum
{
    DAEMON_STATE_NO,
    DAEMON_STATE_REQUEST,
    DAEMON_STATE_EFFECTIVE,
} daemon_state_t;


// ---------- Public variables ---------- //

// The listening sockets
extern unsigned int nb_sockets;
extern listen_socket_t listen_sockets [MAX_LISTEN_SOCKETS];

// The port we use dy default
extern unsigned short master_port;

// System specific command line options
extern const cmdlineopt_t sys_cmdline_options [];

// Daemon state
extern daemon_state_t daemon_state;


// ---------- Public functions (listening sockets) ---------- //

// Step 1 - Add a listen socket to the listening socket list
qboolean Sys_DeclareListenAddress( const char* local_addr_name );

// Step 2 - Resolve the address names of all the listening sockets
qboolean Sys_ResolveListenAddresses( void );

// Step 3 - Create the listening sockets
qboolean Sys_CreateListenSockets( void );


// ---------- Public functions (the rest) ---------- //

// Win32 uses a different name for some standard functions
#ifdef WIN32
# define snprintf _snprintf
# define strdup _strdup
#endif


// Parse a system-dependent command line option
cmdline_status_t Sys_Cmdline_Option( const cmdlineopt_t* opt, const char** params, unsigned int nb_params );

// System dependent initializations (called BEFORE security initializations)
qboolean Sys_UnsecureInit( void );

// System dependent security initializations
qboolean Sys_SecurityInit( void );

// System dependent initializations (called AFTER security initializations)
qboolean Sys_SecureInit( void );

// Returns a pointer to its static character buffer (do NOT free it!)
const char* Sys_SockaddrToString( const struct sockaddr_storage* address, socklen_t socklen );

// Get the network port from a sockaddr
unsigned short Sys_GetSockaddrPort( const struct sockaddr_storage* address );

// Get the last network error code
int Sys_GetLastNetError( void );

// Get the last network error string
const char* Sys_GetLastNetErrorString( void );


#endif  // #ifndef _SYSTEM_H_
