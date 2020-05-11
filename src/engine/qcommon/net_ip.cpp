////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2019 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of the OpenWolf GPL Source Code.
// OpenWolf Source Code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenWolf Source Code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenWolf Source Code.  If not, see <http://www.gnu.org/licenses/>.
//
// In addition, the OpenWolf Source Code is also subject to certain additional terms.
// You should have received a copy of these additional terms immediately following the
// terms and conditions of the GNU General Public License which accompanied the
// OpenWolf Source Code. If not, please request a copy in writing from id Software
// at the address below.
//
// If you have questions concerning this license or the applicable additional terms,
// you may contact in writing id Software LLC, c/o ZeniMax Media Inc.,
// Suite 120, Rockville, Maryland 20850 USA.
//
// -------------------------------------------------------------------------------------
// File name:   net_ip.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.h>
#elif DEDICATED
#include <null/null_serverprecompiled.h>
#else
#include <framework/precompiled.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#if WINVER < 0x501
#ifdef __MINGW32__
// wspiapi.h isn't available on MinGW, so if it's
// present it's because the end user has added it
// and we should look for it in our tree
#include "wspiapi.h"
#else
#include <wspiapi.h>
#endif
#else
#include <ws2spi.h>
#endif

typedef S32 socklen_t;
#ifdef ADDRESS_FAMILY
#define sa_family_t	ADDRESS_FAMILY
#else
typedef U16 sa_family_t;
#endif

#define EAGAIN					WSAEWOULDBLOCK
#define EADDRNOTAVAIL	WSAEADDRNOTAVAIL
#define EAFNOSUPPORT		WSAEAFNOSUPPORT
#define ECONNRESET			WSAECONNRESET
#define socketError		WSAGetLastError( )

static WSADATA	winsockdata;
static bool	winsockInitialized = false;

#else

#if MAC_OS_X_VERSION_MIN_REQUIRED == 1020
// needed for socklen_t on OSX 10.2
#define _BSD_SOCKLEN_T_
#endif

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#if !defined(__sun) && !defined(__sgi)
#include <ifaddrs.h>
#endif

#ifdef __sun
#include <sys/filio.h>
#endif

typedef S32 SOCKET;
#define INVALID_SOCKET		-1
#define SOCKET_ERROR			-1
#define closesocket			close
#define ioctlsocket			ioctl
#define socketError			errno

#endif

static bool usingSocks = false;
static bool networkingEnabled = false;

static convar_t*	net_enabled;

static convar_t*	net_socksEnabled;
static convar_t*	net_socksServer;
static convar_t*	net_socksPort;
static convar_t*	net_socksUsername;
static convar_t*	net_socksPassword;

static convar_t*	net_ip;
static convar_t*	net_ip6;
static convar_t*	net_port;
static convar_t*	net_port6;
static convar_t*	net_mcast6addr;
static convar_t*	net_mcast6iface;

static struct sockaddr	socksRelayAddr;

static SOCKET	ip_socket = INVALID_SOCKET;
static SOCKET	ip6_socket = INVALID_SOCKET;
static SOCKET	socks_socket = INVALID_SOCKET;
static SOCKET	multicast6_socket = INVALID_SOCKET;

// Keep track of currently joined multicast group.
static struct ipv6_mreq curgroup;
// And the currently bound address.
static struct sockaddr_in6 boundto;

#ifndef IF_NAMESIZE
#define IF_NAMESIZE 16
#endif

// use an admin local address per default so that network admins can decide on how to handle quake3 traffic.
#define NET_MULTICAST_IP6 "ff04::696f:7175:616b:6533"

#define	MAX_IPS		32

typedef struct
{
    UTF8 ifname[IF_NAMESIZE];
    
    netadrtype_t type;
    sa_family_t family;
    struct sockaddr_storage addr;
    struct sockaddr_storage netmask;
} nip_localaddr_t;

static nip_localaddr_t localIP[MAX_IPS];
static S32 numIP;


//=============================================================================


/*
====================
NET_ErrorString
====================
*/
UTF8* NET_ErrorString( void )
{
#ifdef _WIN32
    //FIXME: replace with FormatMessage?
    switch( socketError )
    {
        case WSAEINTR:
            return "WSAEINTR";
        case WSAEBADF:
            return "WSAEBADF";
        case WSAEACCES:
            return "WSAEACCES";
        case WSAEDISCON:
            return "WSAEDISCON";
        case WSAEFAULT:
            return "WSAEFAULT";
        case WSAEINVAL:
            return "WSAEINVAL";
        case WSAEMFILE:
            return "WSAEMFILE";
        case WSAEWOULDBLOCK:
            return "WSAEWOULDBLOCK";
        case WSAEINPROGRESS:
            return "WSAEINPROGRESS";
        case WSAEALREADY:
            return "WSAEALREADY";
        case WSAENOTSOCK:
            return "WSAENOTSOCK";
        case WSAEDESTADDRREQ:
            return "WSAEDESTADDRREQ";
        case WSAEMSGSIZE:
            return "WSAEMSGSIZE";
        case WSAEPROTOTYPE:
            return "WSAEPROTOTYPE";
        case WSAENOPROTOOPT:
            return "WSAENOPROTOOPT";
        case WSAEPROTONOSUPPORT:
            return "WSAEPROTONOSUPPORT";
        case WSAESOCKTNOSUPPORT:
            return "WSAESOCKTNOSUPPORT";
        case WSAEOPNOTSUPP:
            return "WSAEOPNOTSUPP";
        case WSAEPFNOSUPPORT:
            return "WSAEPFNOSUPPORT";
        case WSAEAFNOSUPPORT:
            return "WSAEAFNOSUPPORT";
        case WSAEADDRINUSE:
            return "WSAEADDRINUSE";
        case WSAEADDRNOTAVAIL:
            return "WSAEADDRNOTAVAIL";
        case WSAENETDOWN:
            return "WSAENETDOWN";
        case WSAENETUNREACH:
            return "WSAENETUNREACH";
        case WSAENETRESET:
            return "WSAENETRESET";
        case WSAECONNABORTED:
            return "WSWSAECONNABORTEDAEINTR";
        case WSAECONNRESET:
            return "WSAECONNRESET";
        case WSAENOBUFS:
            return "WSAENOBUFS";
        case WSAEISCONN:
            return "WSAEISCONN";
        case WSAENOTCONN:
            return "WSAENOTCONN";
        case WSAESHUTDOWN:
            return "WSAESHUTDOWN";
        case WSAETOOMANYREFS:
            return "WSAETOOMANYREFS";
        case WSAETIMEDOUT:
            return "WSAETIMEDOUT";
        case WSAECONNREFUSED:
            return "WSAECONNREFUSED";
        case WSAELOOP:
            return "WSAELOOP";
        case WSAENAMETOOLONG:
            return "WSAENAMETOOLONG";
        case WSAEHOSTDOWN:
            return "WSAEHOSTDOWN";
        case WSASYSNOTREADY:
            return "WSASYSNOTREADY";
        case WSAVERNOTSUPPORTED:
            return "WSAVERNOTSUPPORTED";
        case WSANOTINITIALISED:
            return "WSANOTINITIALISED";
        case WSAHOST_NOT_FOUND:
            return "WSAHOST_NOT_FOUND";
        case WSATRY_AGAIN:
            return "WSATRY_AGAIN";
        case WSANO_RECOVERY:
            return "WSANO_RECOVERY";
        case WSANO_DATA:
            return "WSANO_DATA";
        default:
            return "NO ERROR";
    }
#else
    return strerror( errno );
#endif
}

static void NetadrToSockadr( netadr_t* a, struct sockaddr* s )
{
    if( a->type == NA_BROADCAST )
    {
        ( ( struct sockaddr_in* )s )->sin_family = AF_INET;
        ( ( struct sockaddr_in* )s )->sin_port = a->port;
        ( ( struct sockaddr_in* )s )->sin_addr.s_addr = INADDR_BROADCAST;
    }
    else if( a->type == NA_IP )
    {
        ( ( struct sockaddr_in* )s )->sin_family = AF_INET;
        ( ( struct sockaddr_in* )s )->sin_addr.s_addr = *( S32* )&a->ip;
        ( ( struct sockaddr_in* )s )->sin_port = a->port;
    }
    else if( a->type == NA_IP6 )
    {
        ( ( struct sockaddr_in6* )s )->sin6_family = AF_INET6;
        ( ( struct sockaddr_in6* )s )->sin6_addr = * ( ( struct in6_addr* ) &a->ip6 );
        ( ( struct sockaddr_in6* )s )->sin6_port = a->port;
        ( ( struct sockaddr_in6* )s )->sin6_scope_id = a->scope_id;
    }
    else if( a->type == NA_MULTICAST6 )
    {
        ( ( struct sockaddr_in6* )s )->sin6_family = AF_INET6;
        ( ( struct sockaddr_in6* )s )->sin6_addr = curgroup.ipv6mr_multiaddr;
        ( ( struct sockaddr_in6* )s )->sin6_port = a->port;
    }
}


static void SockadrToNetadr( struct sockaddr* s, netadr_t* a )
{
    if( s->sa_family == AF_INET )
    {
        a->type = NA_IP;
        *( S32* )&a->ip = ( ( struct sockaddr_in* )s )->sin_addr.s_addr;
        a->port = ( ( struct sockaddr_in* )s )->sin_port;
    }
    else if( s->sa_family == AF_INET6 )
    {
        a->type = NA_IP6;
        memcpy( a->ip6, &( ( struct sockaddr_in6* )s )->sin6_addr, sizeof( a->ip6 ) );
        a->port = ( ( struct sockaddr_in6* )s )->sin6_port;
        a->scope_id = ( ( struct sockaddr_in6* )s )->sin6_scope_id;
    }
}


static struct addrinfo* SearchAddrInfo( struct addrinfo* hints, sa_family_t family )
{
    while( hints )
    {
        if( hints->ai_family == family )
            return hints;
            
        hints = hints->ai_next;
    }
    
    return NULL;
}

/*
=============
Sys_StringToSockaddr
=============
*/
static bool Sys_StringToSockaddr( StringEntry s, struct sockaddr* sadr, S32 sadr_len, sa_family_t family )
{
    struct addrinfo hints;
    struct addrinfo* res = NULL;
    struct addrinfo* search = NULL;
    struct addrinfo* hintsp;
    S32 retval;
    
    memset( sadr, '\0', sizeof( *sadr ) );
    memset( &hints, '\0', sizeof( hints ) );
    
    hintsp = &hints;
    hintsp->ai_family = family;
    hintsp->ai_socktype = SOCK_DGRAM;
    
    retval = getaddrinfo( s, NULL, hintsp, &res );
    
    if( !retval )
    {
        if( family == AF_UNSPEC )
        {
            // Decide here and now which protocol family to use
            if( net_enabled->integer & NET_PRIOV6 )
            {
                if( net_enabled->integer & NET_ENABLEV6 )
                    search = SearchAddrInfo( res, AF_INET6 );
                    
                if( !search && ( net_enabled->integer & NET_ENABLEV4 ) )
                    search = SearchAddrInfo( res, AF_INET );
            }
            else
            {
                if( net_enabled->integer & NET_ENABLEV4 )
                    search = SearchAddrInfo( res, AF_INET );
                    
                if( !search && ( net_enabled->integer & NET_ENABLEV6 ) )
                    search = SearchAddrInfo( res, AF_INET6 );
            }
        }
        else
            search = SearchAddrInfo( res, family );
            
        if( search )
        {
            if( res->ai_addrlen > sadr_len )
                res->ai_addrlen = sadr_len;
                
            memcpy( sadr, res->ai_addr, res->ai_addrlen );
            freeaddrinfo( res );
            
            return true;
        }
        else
            Com_Printf( "Sys_StringToSockaddr: Error resolving %s: No address of required type found.\n", s );
    }
    else
        Com_Printf( "Sys_StringToSockaddr: Error resolving %s: %s\n", s, gai_strerror( retval ) );
        
    if( res )
        freeaddrinfo( res );
        
    return false;
}

/*
=============
Sys_SockaddrToString
=============
*/
static void Sys_SockaddrToString( UTF8* dest, S32 destlen, struct sockaddr* input )
{
    socklen_t inputlen;
    
    if( input->sa_family == AF_INET6 )
        inputlen = sizeof( struct sockaddr_in6 );
    else
        inputlen = sizeof( struct sockaddr_in );
        
    if( getnameinfo( input, inputlen, dest, destlen, NULL, 0, NI_NUMERICHOST ) && destlen > 0 )
        *dest = '\0';
}

/*
=============
Net_StringToAdr
=============
*/
bool Net_StringToAdr( StringEntry s, netadr_t* a, netadrtype_t family )
{
    struct sockaddr_storage sadr;
    sa_family_t fam;
    
    switch( family )
    {
        case NA_IP:
            fam = AF_INET;
            break;
        case NA_IP6:
            fam = AF_INET6;
            break;
        default:
            fam = AF_UNSPEC;
            break;
    }
    if( !Sys_StringToSockaddr( s, ( struct sockaddr* ) &sadr, sizeof( sadr ), fam ) )
    {
        return false;
    }
    
    SockadrToNetadr( ( struct sockaddr* ) &sadr, a );
    return true;
}

/*
===================
NET_CompareBaseAdrMask

Compare without port, and up to the bit number given in netmask.
===================
*/
bool NET_CompareBaseAdrMask( netadr_t a, netadr_t b, S32 netmask )
{
    bool differed;
    U8 cmpmask, *addra, *addrb;
    S32 curbyte;
    
    if( a.type != b.type )
        return false;
        
    if( a.type == NA_LOOPBACK )
        return true;
        
    if( a.type == NA_IP )
    {
        addra = ( U8* ) &a.ip;
        addrb = ( U8* ) &b.ip;
        
        if( netmask < 0 || netmask > 32 )
            netmask = 32;
    }
    else if( a.type == NA_IP6 )
    {
        addra = ( U8* ) &a.ip6;
        addrb = ( U8* ) &b.ip6;
        
        if( netmask < 0 || netmask > 128 )
            netmask = 128;
    }
    else
    {
        Com_Printf( "NET_CompareBaseAdr: bad address type\n" );
        return false;
    }
    
    differed = false;
    curbyte = 0;
    
    while( netmask > 7 )
    {
        if( addra[curbyte] != addrb[curbyte] )
        {
            differed = true;
            break;
        }
        
        curbyte++;
        netmask -= 8;
    }
    
    if( differed )
        return false;
        
    if( netmask )
    {
        cmpmask = ( 1 << netmask ) - 1;
        cmpmask <<= 8 - netmask;
        
        if( ( addra[curbyte] & cmpmask ) == ( addrb[curbyte] & cmpmask ) )
            return true;
    }
    else
        return true;
        
    return false;
}


/*
===================
NET_CompareBaseAdr

Compares without the port
===================
*/
bool NET_CompareBaseAdr( netadr_t a, netadr_t b )
{
    return NET_CompareBaseAdrMask( a, b, -1 );
}

StringEntry	NET_AdrToString( netadr_t a )
{
    static	UTF8	s[NET_ADDRSTRMAXLEN];
    
    if( a.type == NA_LOOPBACK )
        Com_sprintf( s, sizeof( s ), "loopback" );
    else if( a.type == NA_BOT )
        Com_sprintf( s, sizeof( s ), "bot" );
    else if( a.type == NA_IP || a.type == NA_IP6 )
    {
        struct sockaddr_storage sadr;
        
        memset( &sadr, 0, sizeof( sadr ) );
        NetadrToSockadr( &a, ( struct sockaddr* ) &sadr );
        Sys_SockaddrToString( s, sizeof( s ), ( struct sockaddr* ) &sadr );
    }
    
    return s;
}

StringEntry	NET_AdrToStringwPort( netadr_t a )
{
    static	UTF8	s[NET_ADDRSTRMAXLEN];
    
    if( a.type == NA_LOOPBACK )
        Com_sprintf( s, sizeof( s ), "loopback" );
    else if( a.type == NA_BOT )
        Com_sprintf( s, sizeof( s ), "bot" );
    else if( a.type == NA_IP )
        Com_sprintf( s, sizeof( s ), "%s:%d", NET_AdrToString( a ), ( U64 )ntohs( a.port ) );
    else if( a.type == NA_IP6 )
        Com_sprintf( s, sizeof( s ), "[%s]:%d", NET_AdrToString( a ), ( U64 )ntohs( a.port ) );
    return s;
}


bool	NET_CompareAdr( netadr_t a, netadr_t b )
{
    if( !NET_CompareBaseAdr( a, b ) )
        return false;
        
    if( a.type == NA_IP || a.type == NA_IP6 )
    {
        if( a.port == b.port )
            return true;
    }
    else
    {
        return true;
    }
    
    return false;
}


bool	NET_IsLocalAddress( netadr_t adr )
{
    return ( bool )( adr.type == NA_LOOPBACK );
}

//=============================================================================

/*
==================
Net_GetPacket

Never called by the game logic, just the system event queing
==================
*/
#ifdef _DEBUG
S32	recvfromCount;
#endif

bool Net_GetPacket( netadr_t* net_from, msg_t* net_message )
{
    S32 	ret;
    struct sockaddr_storage from;
    socklen_t	fromlen;
    S32		err;
    
#ifdef _DEBUG
    recvfromCount++;		// performance check
#endif
    
    if( ip_socket != INVALID_SOCKET )
    {
        fromlen = sizeof( from );
        ret = recvfrom( ip_socket, ( UTF8* )net_message->data, net_message->maxsize, 0, ( struct sockaddr* ) &from, &fromlen );
        
        if( ret == SOCKET_ERROR )
        {
            err = socketError;
            
            if( err != EAGAIN && err != ECONNRESET )
                Com_Printf( "NET_GetPacket: %s\n", NET_ErrorString() );
        }
        else
        {
        
            memset( ( ( struct sockaddr_in* )&from )->sin_zero, 0, 8 );
            
            if( usingSocks && memcmp( &from, &socksRelayAddr, fromlen ) == 0 )
            {
                if( ret < 10 || net_message->data[0] != 0 || net_message->data[1] != 0 || net_message->data[2] != 0 || net_message->data[3] != 1 )
                {
                    return false;
                }
                net_from->type = NA_IP;
                net_from->ip[0] = net_message->data[4];
                net_from->ip[1] = net_message->data[5];
                net_from->ip[2] = net_message->data[6];
                net_from->ip[3] = net_message->data[7];
                net_from->port = *( S16* )&net_message->data[8];
                net_message->readcount = 10;
            }
            else
            {
                SockadrToNetadr( ( struct sockaddr* ) &from, net_from );
                net_message->readcount = 0;
            }
            
            if( ret == net_message->maxsize )
            {
                Com_Printf( "Oversize packet from %s\n", NET_AdrToString( *net_from ) );
                return false;
            }
            
            net_message->cursize = ret;
            return true;
        }
    }
    
    if( ip6_socket != INVALID_SOCKET )
    {
        fromlen = sizeof( from );
        ret = recvfrom( ip6_socket, ( UTF8* )net_message->data, net_message->maxsize, 0, ( struct sockaddr* ) &from, &fromlen );
        
        if( ret == SOCKET_ERROR )
        {
            err = socketError;
            
            if( err != EAGAIN && err != ECONNRESET )
                Com_Printf( "NET_GetPacket: %s\n", NET_ErrorString() );
        }
        else
        {
            SockadrToNetadr( ( struct sockaddr* ) &from, net_from );
            net_message->readcount = 0;
            
            if( ret == net_message->maxsize )
            {
                Com_Printf( "Oversize packet from %s\n", NET_AdrToString( *net_from ) );
                return false;
            }
            
            net_message->cursize = ret;
            return true;
        }
    }
    
    if( multicast6_socket != INVALID_SOCKET && multicast6_socket != ip6_socket )
    {
        fromlen = sizeof( from );
        ret = recvfrom( multicast6_socket, ( UTF8* )net_message->data, net_message->maxsize, 0, ( struct sockaddr* ) &from, &fromlen );
        
        if( ret == SOCKET_ERROR )
        {
            err = socketError;
            
            if( err != EAGAIN && err != ECONNRESET )
                Com_Printf( "NET_GetPacket: %s\n", NET_ErrorString() );
        }
        else
        {
            SockadrToNetadr( ( struct sockaddr* ) &from, net_from );
            net_message->readcount = 0;
            
            if( ret == net_message->maxsize )
            {
                Com_Printf( "Oversize packet from %s\n", NET_AdrToString( *net_from ) );
                return false;
            }
            
            net_message->cursize = ret;
            return true;
        }
    }
    
    
    return false;
}

//=============================================================================

static UTF8 socksBuf[4096];

/*
==================
Net_SendPacket
==================
*/
void Net_SendPacket( S32 length, const void* data, netadr_t to )
{
    S32				ret = SOCKET_ERROR;
    struct			sockaddr_storage addr;
    
    if( to.type != NA_BROADCAST && to.type != NA_IP && to.type != NA_IP6 && to.type != NA_MULTICAST6 )
    {
        Com_Error( ERR_FATAL, "Sys_SendPacket: bad address type" );
        return;
    }
    
    if( ( ip_socket == INVALID_SOCKET && to.type == NA_IP ) ||
            ( ip6_socket == INVALID_SOCKET && to.type == NA_IP6 ) ||
            ( ip6_socket == INVALID_SOCKET && to.type == NA_MULTICAST6 ) )
        return;
        
    if( to.type == NA_MULTICAST6 && ( net_enabled->integer & NET_DISABLEMCAST ) )
        return;
        
    memset( &addr, 0, sizeof( addr ) );
    NetadrToSockadr( &to, ( struct sockaddr* ) &addr );
    
    if( usingSocks && to.type == NA_IP )
    {
        socksBuf[0] = 0;	// reserved
        socksBuf[1] = 0;
        socksBuf[2] = 0;	// fragment (not fragmented)
        socksBuf[3] = 1;	// address type: IPV4
        *( S32* )&socksBuf[4] = ( ( struct sockaddr_in* )&addr )->sin_addr.s_addr;
        *( S16* )&socksBuf[8] = ( ( struct sockaddr_in* )&addr )->sin_port;
        memcpy( &socksBuf[10], data, length );
        ret = sendto( ip_socket, socksBuf, length + 10, 0, &socksRelayAddr, sizeof( socksRelayAddr ) );
    }
    else
    {
        if( addr.ss_family == AF_INET )
            ret = sendto( ip_socket, ( StringEntry )data, length, 0, ( struct sockaddr* ) &addr, sizeof( struct sockaddr_in ) );
        else if( addr.ss_family == AF_INET6 )
            ret = sendto( ip6_socket, ( StringEntry )data, length, 0, ( struct sockaddr* ) &addr, sizeof( struct sockaddr_in6 ) );
    }
    if( ret == SOCKET_ERROR )
    {
        S32 err = socketError;
        
        // wouldblock is silent
        if( err == EAGAIN )
        {
            return;
        }
        
        // some PPP links do not allow broadcasts and return an error
        if( ( err == EADDRNOTAVAIL ) && ( ( to.type == NA_BROADCAST ) ) )
        {
            return;
        }
        
        Com_Printf( "NET_SendPacket: %s\n", NET_ErrorString() );
    }
}


//=============================================================================

/*
==================
Net_IsLANAddress

LAN clients will have their rate var ignored
==================
*/
bool Net_IsLANAddress( netadr_t adr )
{
    S32		index, run, addrsize;
    bool differed;
    U8* compareadr, *comparemask, *compareip;
    
    if( adr.type == NA_LOOPBACK )
    {
        return true;
    }
    
    if( adr.type == NA_IP )
    {
        // RFC1918:
        // 10.0.0.0        -   10.255.255.255  (10/8 prefix)
        // 172.16.0.0      -   172.31.255.255  (172.16/12 prefix)
        // 192.168.0.0     -   192.168.255.255 (192.168/16 prefix)
        if( adr.ip[0] == 10 )
            return true;
        if( adr.ip[0] == 172 && ( adr.ip[1] & 0xf0 ) == 16 )
            return true;
        if( adr.ip[0] == 192 && adr.ip[1] == 168 )
            return true;
            
        if( adr.ip[0] == 127 )
            return true;
    }
    else if( adr.type == NA_IP6 )
    {
        if( adr.ip6[0] == 0xfe && ( adr.ip6[1] & 0xc0 ) == 0x80 )
            return true;
        if( ( adr.ip6[0] & 0xfe ) == 0xfc )
            return true;
    }
    
    // Now compare against the networks this computer is member of.
    for( index = 0; index < numIP; index++ )
    {
        if( localIP[index].type == adr.type )
        {
            if( adr.type == NA_IP )
            {
                compareip = ( U8* ) & ( ( struct sockaddr_in* ) &localIP[index].addr )->sin_addr.s_addr;
                comparemask = ( U8* ) & ( ( struct sockaddr_in* ) &localIP[index].netmask )->sin_addr.s_addr;
                compareadr = adr.ip;
                
                addrsize = sizeof( adr.ip );
            }
            else
            {
                // TODO? should we check the scope_id here?
                
                compareip = ( U8* ) & ( ( struct sockaddr_in6* ) &localIP[index].addr )->sin6_addr;
                comparemask = ( U8* ) & ( ( struct sockaddr_in6* ) &localIP[index].netmask )->sin6_addr;
                compareadr = adr.ip6;
                
                addrsize = sizeof( adr.ip6 );
            }
            
            differed = false;
            for( run = 0; run < addrsize; run++ )
            {
                if( ( compareip[run] & comparemask[run] ) != ( compareadr[run] & comparemask[run] ) )
                {
                    differed = true;
                    break;
                }
            }
            
            if( !differed )
                return true;
                
        }
    }
    
    return false;
}

/*
==================
Net_ShowIP
==================
*/
void Net_ShowIP( void )
{
    S32 i;
    UTF8 addrbuf[NET_ADDRSTRMAXLEN];
    
    for( i = 0; i < numIP; i++ )
    {
        Sys_SockaddrToString( addrbuf, sizeof( addrbuf ), ( struct sockaddr* ) &localIP[i].addr );
        
        if( localIP[i].type == NA_IP )
            Com_Printf( "IP: %s\n", addrbuf );
        else if( localIP[i].type == NA_IP6 )
            Com_Printf( "IP6: %s\n", addrbuf );
    }
}


//=============================================================================


/*
====================
NET_IPSocket
====================
*/
SOCKET NET_IPSocket( UTF8* net_interface, S32 port, S32* err )
{
    SOCKET				newsocket;
    struct sockaddr_in	address;
    u_long				_true = 1;
    S32					i = 1;
    
    *err = 0;
    
    if( net_interface )
    {
        Com_Printf( "Opening IP socket: %s:%i\n", net_interface, port );
    }
    else
    {
        Com_Printf( "Opening IP socket: 0.0.0.0:%i\n", port );
    }
    
    if( ( newsocket = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) == INVALID_SOCKET )
    {
        *err = socketError;
        Com_Printf( "WARNING: NET_IPSocket: socket: %s\n", NET_ErrorString() );
        return newsocket;
    }
    // make it non-blocking
    if( ioctlsocket( newsocket, FIONBIO, &_true ) == SOCKET_ERROR )
    {
        Com_Printf( "WARNING: NET_IPSocket: ioctl FIONBIO: %s\n", NET_ErrorString() );
        *err = socketError;
        closesocket( newsocket );
        return INVALID_SOCKET;
    }
    
    // make it broadcast capable
    if( setsockopt( newsocket, SOL_SOCKET, SO_BROADCAST, ( UTF8* ) &i, sizeof( i ) ) == SOCKET_ERROR )
    {
        Com_Printf( "WARNING: NET_IPSocket: setsockopt SO_BROADCAST: %s\n", NET_ErrorString() );
    }
    
    if( !net_interface || !net_interface[0] )
    {
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
        if( !Sys_StringToSockaddr( net_interface, ( struct sockaddr* )&address, sizeof( address ), AF_INET ) )
        {
            closesocket( newsocket );
            return INVALID_SOCKET;
        }
    }
    
    if( port == PORT_ANY )
    {
        address.sin_port = 0;
    }
    else
    {
        address.sin_port = htons( ( S16 )port );
    }
    
    if( bind( newsocket, ( struct sockaddr* )&address, sizeof( address ) ) == SOCKET_ERROR )
    {
        Com_Printf( "WARNING: NET_IPSocket: bind: %s\n", NET_ErrorString() );
        *err = socketError;
        closesocket( newsocket );
        return INVALID_SOCKET;
    }
    
    return newsocket;
}

/*
====================
NET_IP6Socket
====================
*/
SOCKET NET_IP6Socket( UTF8* net_interface, S32 port, struct sockaddr_in6* bindto, S32* err )
{
    SOCKET				newsocket;
    struct sockaddr_in6	address;
    u_long				_true = 1;
    
    *err = 0;
    
    if( net_interface )
    {
        // Print the name in brackets if there is a colon:
        if( Q_CountChar( net_interface, ':' ) )
            Com_Printf( "Opening IP6 socket: [%s]:%i\n", net_interface, port );
        else
            Com_Printf( "Opening IP6 socket: %s:%i\n", net_interface, port );
    }
    else
        Com_Printf( "Opening IP6 socket: [::]:%i\n", port );
        
    if( ( newsocket = socket( PF_INET6, SOCK_DGRAM, IPPROTO_UDP ) ) == INVALID_SOCKET )
    {
        *err = socketError;
        Com_Printf( "WARNING: NET_IP6Socket: socket: %s\n", NET_ErrorString() );
        return newsocket;
    }
    
    // make it non-blocking
    if( ioctlsocket( newsocket, FIONBIO, &_true ) == SOCKET_ERROR )
    {
        Com_Printf( "WARNING: NET_IP6Socket: ioctl FIONBIO: %s\n", NET_ErrorString() );
        *err = socketError;
        closesocket( newsocket );
        return INVALID_SOCKET;
    }
    
#ifdef IPV6_V6ONLY
    {
        S32 i = 1;
        
        // ipv4 addresses should not be allowed to connect via this socket.
        if( setsockopt( newsocket, IPPROTO_IPV6, IPV6_V6ONLY, ( UTF8* ) &i, sizeof( i ) ) == SOCKET_ERROR )
        {
            // win32 systems don't seem to support this anyways.
            Com_DPrintf( "WARNING: NET_IP6Socket: setsockopt IPV6_V6ONLY: %s\n", NET_ErrorString() );
        }
    }
#endif
    
    if( !net_interface || !net_interface[0] )
    {
        address.sin6_family = AF_INET6;
        address.sin6_addr = in6addr_any;
    }
    else
    {
        if( !Sys_StringToSockaddr( net_interface, ( struct sockaddr* )&address, sizeof( address ), AF_INET6 ) )
        {
            closesocket( newsocket );
            return INVALID_SOCKET;
        }
    }
    
    if( port == PORT_ANY )
    {
        address.sin6_port = 0;
    }
    else
    {
        address.sin6_port = htons( ( S16 )port );
    }
    
    if( bind( newsocket, ( struct sockaddr* )&address, sizeof( address ) ) == SOCKET_ERROR )
    {
        Com_Printf( "WARNING: NET_IP6Socket: bind: %s\n", NET_ErrorString() );
        *err = socketError;
        closesocket( newsocket );
        return INVALID_SOCKET;
    }
    
    if( bindto )
        *bindto = address;
        
    return newsocket;
}

/*
====================
NET_SetMulticast
Set the current multicast group
====================
*/
void NET_SetMulticast6( void )
{
    struct sockaddr_in6 addr;
    
    if( !*net_mcast6addr->string || !Sys_StringToSockaddr( net_mcast6addr->string, ( struct sockaddr* ) &addr, sizeof( addr ), AF_INET6 ) )
    {
        Com_Printf( "WARNING: NET_JoinMulticast6: Incorrect multicast address given, "
                    "please set cvar %s to a sane value.\n", net_mcast6addr->name );
                    
        cvarSystem->SetValue( net_enabled->name, net_enabled->integer | NET_DISABLEMCAST );
        
        return;
    }
    
    memcpy( &curgroup.ipv6mr_multiaddr, &addr.sin6_addr, sizeof( curgroup.ipv6mr_multiaddr ) );
    
    if( *net_mcast6iface->string )
    {
#ifdef _WIN32
        curgroup.ipv6mr_interface = net_mcast6iface->integer;
#else
        curgroup.ipv6mr_interface = if_nametoindex( net_mcast6iface->string );
#endif
    }
    else
        curgroup.ipv6mr_interface = 0;
}

/*
====================
NET_JoinMulticast
Join an ipv6 multicast group
====================
*/
void NET_JoinMulticast6( void )
{
    S32 err;
    
    if( ip6_socket == INVALID_SOCKET || multicast6_socket != INVALID_SOCKET || ( net_enabled->integer & NET_DISABLEMCAST ) )
        return;
        
    if( IN6_IS_ADDR_MULTICAST( &boundto.sin6_addr ) || IN6_IS_ADDR_UNSPECIFIED( &boundto.sin6_addr ) )
    {
        // The way the socket was bound does not prohibit receiving multi-cast packets. So we don't need to open a new one.
        multicast6_socket = ip6_socket;
    }
    else
    {
        if( ( multicast6_socket = NET_IP6Socket( net_mcast6addr->string, ntohs( boundto.sin6_port ), NULL, &err ) ) == INVALID_SOCKET )
        {
            // If the OS does not support binding to multicast addresses, like WinXP, at least try with the normal file descriptor.
            multicast6_socket = ip6_socket;
        }
    }
    
    if( curgroup.ipv6mr_interface )
    {
        if( setsockopt( multicast6_socket, IPPROTO_IPV6, IPV6_MULTICAST_IF,
                        ( UTF8* ) &curgroup.ipv6mr_interface, sizeof( curgroup.ipv6mr_interface ) ) < 0 )
        {
            Com_Printf( "NET_JoinMulticast6: Couldn't set scope on multicast socket: %s\n", NET_ErrorString() );
            
            if( multicast6_socket != ip6_socket )
            {
                closesocket( multicast6_socket );
                multicast6_socket = INVALID_SOCKET;
                return;
            }
        }
    }
    
    if( setsockopt( multicast6_socket, IPPROTO_IPV6, IPV6_JOIN_GROUP, ( UTF8* ) &curgroup, sizeof( curgroup ) ) )
    {
        Com_Printf( "NET_JoinMulticast6: Couldn't join multicast group: %s\n", NET_ErrorString() );
        
        if( multicast6_socket != ip6_socket )
        {
            closesocket( multicast6_socket );
            multicast6_socket = INVALID_SOCKET;
            return;
        }
    }
}

void NET_LeaveMulticast6()
{
    if( multicast6_socket != INVALID_SOCKET )
    {
        if( multicast6_socket != ip6_socket )
            closesocket( multicast6_socket );
        else
            setsockopt( multicast6_socket, IPPROTO_IPV6, IPV6_LEAVE_GROUP, ( UTF8* ) &curgroup, sizeof( curgroup ) );
            
        multicast6_socket = INVALID_SOCKET;
    }
}

/*
====================
NET_OpenSocks
====================
*/
void NET_OpenSocks( S32 port )
{
    struct sockaddr_in	address;
    S32					err;
    struct hostent*		h;
    S32					len;
    bool			rfc1929;
    U8		buf[64];
    
    usingSocks = false;
    
    Com_Printf( "Opening connection to SOCKS server.\n" );
    
    if( ( socks_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP ) ) == INVALID_SOCKET )
    {
        err = socketError;
        Com_Printf( "WARNING: NET_OpenSocks: socket: %s\n", NET_ErrorString() );
        return;
    }
    
    h = gethostbyname( net_socksServer->string );
    if( h == NULL )
    {
        err = socketError;
        Com_Printf( "WARNING: NET_OpenSocks: gethostbyname: %s\n", NET_ErrorString() );
        return;
    }
    if( h->h_addrtype != AF_INET )
    {
        Com_Printf( "WARNING: NET_OpenSocks: gethostbyname: address type was not AF_INET\n" );
        return;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = *( S32* )h->h_addr_list[0];
    address.sin_port = htons( ( S16 )net_socksPort->integer );
    
    if( connect( socks_socket, ( struct sockaddr* )&address, sizeof( address ) ) == SOCKET_ERROR )
    {
        err = socketError;
        Com_Printf( "NET_OpenSocks: connect: %s\n", NET_ErrorString() );
        return;
    }
    
    // send socks authentication handshake
    if( *net_socksUsername->string || *net_socksPassword->string )
    {
        rfc1929 = true;
    }
    else
    {
        rfc1929 = false;
    }
    
    buf[0] = 5;		// SOCKS version
    // method count
    if( rfc1929 )
    {
        buf[1] = 2;
        len = 4;
    }
    else
    {
        buf[1] = 1;
        len = 3;
    }
    buf[2] = 0;		// method #1 - method id #00: no authentication
    if( rfc1929 )
    {
        buf[2] = 2;		// method #2 - method id #02: username/password
    }
    if( send( socks_socket, ( UTF8* )buf, len, 0 ) == SOCKET_ERROR )
    {
        err = socketError;
        Com_Printf( "NET_OpenSocks: send: %s\n", NET_ErrorString() );
        return;
    }
    
    // get the response
    len = recv( socks_socket, ( UTF8* )buf, 64, 0 );
    if( len == SOCKET_ERROR )
    {
        err = socketError;
        Com_Printf( "NET_OpenSocks: recv: %s\n", NET_ErrorString() );
        return;
    }
    if( len != 2 || buf[0] != 5 )
    {
        Com_Printf( "NET_OpenSocks: bad response\n" );
        return;
    }
    switch( buf[1] )
    {
        case 0:	// no authentication
            break;
        case 2: // username/password authentication
            break;
        default:
            Com_Printf( "NET_OpenSocks: request denied\n" );
            return;
    }
    
    // do username/password authentication if needed
    if( buf[1] == 2 )
    {
        S32		ulen;
        S32		plen;
        
        // build the request
        ulen = strlen( net_socksUsername->string );
        plen = strlen( net_socksPassword->string );
        
        buf[0] = 1;		// username/password authentication version
        buf[1] = ulen;
        if( ulen )
        {
            memcpy( &buf[2], net_socksUsername->string, ulen );
        }
        buf[2 + ulen] = plen;
        if( plen )
        {
            memcpy( &buf[3 + ulen], net_socksPassword->string, plen );
        }
        
        // send it
        if( send( socks_socket, ( UTF8* )buf, 3 + ulen + plen, 0 ) == SOCKET_ERROR )
        {
            err = socketError;
            Com_Printf( "NET_OpenSocks: send: %s\n", NET_ErrorString() );
            return;
        }
        
        // get the response
        len = recv( socks_socket, ( UTF8* )buf, 64, 0 );
        if( len == SOCKET_ERROR )
        {
            err = socketError;
            Com_Printf( "NET_OpenSocks: recv: %s\n", NET_ErrorString() );
            return;
        }
        if( len != 2 || buf[0] != 1 )
        {
            Com_Printf( "NET_OpenSocks: bad response\n" );
            return;
        }
        if( buf[1] != 0 )
        {
            Com_Printf( "NET_OpenSocks: authentication failed\n" );
            return;
        }
    }
    
    // send the UDP associate request
    buf[0] = 5;		// SOCKS version
    buf[1] = 3;		// command: UDP associate
    buf[2] = 0;		// reserved
    buf[3] = 1;		// address type: IPV4
    *( S32* )&buf[4] = INADDR_ANY;
    *( S16* )&buf[8] = htons( ( S16 )port );		// port
    if( send( socks_socket, ( UTF8* )buf, 10, 0 ) == SOCKET_ERROR )
    {
        err = socketError;
        Com_Printf( "NET_OpenSocks: send: %s\n", NET_ErrorString() );
        return;
    }
    
    // get the response
    len = recv( socks_socket, ( UTF8* )buf, 64, 0 );
    if( len == SOCKET_ERROR )
    {
        err = socketError;
        Com_Printf( "NET_OpenSocks: recv: %s\n", NET_ErrorString() );
        return;
    }
    if( len < 2 || buf[0] != 5 )
    {
        Com_Printf( "NET_OpenSocks: bad response\n" );
        return;
    }
    // check completion code
    if( buf[1] != 0 )
    {
        Com_Printf( "NET_OpenSocks: request denied: %i\n", buf[1] );
        return;
    }
    if( buf[3] != 1 )
    {
        Com_Printf( "NET_OpenSocks: relay address is not IPV4: %i\n", buf[3] );
        return;
    }
    ( ( struct sockaddr_in* )&socksRelayAddr )->sin_family = AF_INET;
    ( ( struct sockaddr_in* )&socksRelayAddr )->sin_addr.s_addr = *( S32* )&buf[4];
    ( ( struct sockaddr_in* )&socksRelayAddr )->sin_port = *( S16* )&buf[8];
    memset( ( ( struct sockaddr_in* )&socksRelayAddr )->sin_zero, 0, 8 );
    
    usingSocks = true;
}


/*
=====================
NET_AddLocalAddress
=====================
*/
static void NET_AddLocalAddress( UTF8* ifname, struct sockaddr* addr, struct sockaddr* netmask )
{
    S32 addrlen;
    sa_family_t family;
    
    // only add addresses that have all required info.
    if( !addr || !netmask || !ifname )
        return;
        
    family = addr->sa_family;
    
    if( numIP < MAX_IPS )
    {
        if( family == AF_INET )
        {
            addrlen = sizeof( struct sockaddr_in );
            localIP[numIP].type = NA_IP;
        }
        else if( family == AF_INET6 )
        {
            addrlen = sizeof( struct sockaddr_in6 );
            localIP[numIP].type = NA_IP6;
        }
        else
            return;
            
        Q_strncpyz( localIP[numIP].ifname, ifname, sizeof( localIP[numIP].ifname ) );
        
        localIP[numIP].family = family;
        
        memcpy( &localIP[numIP].addr, addr, addrlen );
        memcpy( &localIP[numIP].netmask, netmask, addrlen );
        
        numIP++;
    }
}

#if defined(__linux__) || defined(MACOSX) || defined(__BSD__)
static void NET_GetLocalAddress( void )
{
    struct ifaddrs* ifap, *search;
    
    numIP = 0;
    
    if( getifaddrs( &ifap ) )
        Com_Printf( "NET_GetLocalAddress: Unable to get list of network interfaces: %s\n", NET_ErrorString() );
    else
    {
        for( search = ifap; search; search = search->ifa_next )
        {
            // Only add interfaces that are up.
            if( ifap->ifa_flags & IFF_UP )
                NET_AddLocalAddress( search->ifa_name, search->ifa_addr, search->ifa_netmask );
        }
        
        freeifaddrs( ifap );
        
        Net_ShowIP();
    }
}
#else
static void NET_GetLocalAddress( void )
{
    UTF8				hostname[256];
    struct addrinfo	hint;
    struct addrinfo*	res = NULL;

    numIP = 0;

    if( gethostname( hostname, 256 ) == SOCKET_ERROR )
        return;

    Com_Printf( "Hostname: %s\n", hostname );

    memset( &hint, 0, sizeof( hint ) );

    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_DGRAM;

    if( !getaddrinfo( hostname, NULL, &hint, &res ) )
    {
        struct sockaddr_in mask4;
        struct sockaddr_in6 mask6;
        struct addrinfo* search;

        /* On operating systems where it's more difficult to find out the configured interfaces, we'll just assume a
         * netmask with all bits set. */

        memset( &mask4, 0, sizeof( mask4 ) );
        memset( &mask6, 0, sizeof( mask6 ) );
        mask4.sin_family = AF_INET;
        memset( &mask4.sin_addr.s_addr, 0xFF, sizeof( mask4.sin_addr.s_addr ) );
        mask6.sin6_family = AF_INET6;
        memset( &mask6.sin6_addr, 0xFF, sizeof( mask6.sin6_addr ) );

        // add all IPs from returned list.
        for( search = res; search; search = search->ai_next )
        {
            if( search->ai_family == AF_INET )
                NET_AddLocalAddress( "", search->ai_addr, ( struct sockaddr* ) &mask4 );
            else if( search->ai_family == AF_INET6 )
                NET_AddLocalAddress( "", search->ai_addr, ( struct sockaddr* ) &mask6 );
        }

        Net_ShowIP();
    }

    if( res )
        freeaddrinfo( res );
}
#endif

/*
====================
NET_OpenIP
====================
*/
void NET_OpenIP( void )
{
    S32		i;
    S32		err;
    S32		port;
    S32		port6;
    
    port = net_port->integer;
    port6 = net_port6->integer;
    
    NET_GetLocalAddress();
    
    // automatically scan for a valid port, so multiple
    // dedicated servers can be started without requiring
    // a different net_port for each one
    
    if( net_enabled->integer & NET_ENABLEV6 )
    {
        for( i = 0 ; i < 10 ; i++ )
        {
            ip6_socket = NET_IP6Socket( net_ip6->string, port6 + i, &boundto, &err );
            if( ip6_socket != INVALID_SOCKET )
            {
                cvarSystem->SetValue( "net_port6", port6 + i );
                break;
            }
            else
            {
                if( err == EAFNOSUPPORT )
                    break;
            }
        }
        if( ip6_socket == INVALID_SOCKET )
            Com_Printf( "WARNING: Couldn't bind to a v6 ip address.\n" );
    }
    
    if( net_enabled->integer & NET_ENABLEV4 )
    {
        for( i = 0 ; i < 10 ; i++ )
        {
            ip_socket = NET_IPSocket( net_ip->string, port + i, &err );
            if( ip_socket != INVALID_SOCKET )
            {
                cvarSystem->SetValue( "net_port", port + i );
                
                if( net_socksEnabled->integer )
                    NET_OpenSocks( port + i );
                    
                break;
            }
            else
            {
                if( err == EAFNOSUPPORT )
                    break;
            }
        }
        
        if( ip_socket == INVALID_SOCKET )
            Com_Printf( "WARNING: Couldn't bind to a v4 ip address.\n" );
    }
}


//===================================================================


/*
====================
NET_GetCvars
====================
*/
static bool NET_GetCvars( void )
{
    S32 modified;
    
#ifdef DEDICATED
    // I want server owners to explicitly turn on ipv6 support.
    net_enabled = cvarSystem->Get( "net_enabled", "1", CVAR_LATCH | CVAR_ARCHIVE, "description" );
#else
    /* End users have it enabled so they can connect to ipv6-only hosts, but ipv4 will be
     * used if available due to ping */
    net_enabled = cvarSystem->Get( "net_enabled", "3", CVAR_LATCH | CVAR_ARCHIVE, "description" );
#endif
    modified = net_enabled->modified;
    net_enabled->modified = false;
    
    net_ip = cvarSystem->Get( "net_ip", "0.0.0.0", CVAR_LATCH, "description" );
    modified += net_ip->modified;
    net_ip->modified = false;
    
    net_ip6 = cvarSystem->Get( "net_ip6", "::", CVAR_LATCH, "description" );
    modified += net_ip6->modified;
    net_ip6->modified = false;
    
    net_port = cvarSystem->Get( "net_port", va( "%i", PORT_SERVER ), CVAR_LATCH, "description" );
    modified += net_port->modified;
    net_port->modified = false;
    
    net_port6 = cvarSystem->Get( "net_port6", va( "%i", PORT_SERVER ), CVAR_LATCH, "description" );
    modified += net_port6->modified;
    net_port6->modified = false;
    
    // Some cvars for configuring multicast options which facilitates scanning for servers on local subnets.
    net_mcast6addr = cvarSystem->Get( "net_mcast6addr", NET_MULTICAST_IP6, CVAR_LATCH | CVAR_ARCHIVE, "description" );
    modified += net_mcast6addr->modified;
    net_mcast6addr->modified = false;
    
#ifdef _WIN32
    net_mcast6iface = cvarSystem->Get( "net_mcast6iface", "0", CVAR_LATCH | CVAR_ARCHIVE, "description" );
#else
    net_mcast6iface = cvarSystem->Get( "net_mcast6iface", "", CVAR_LATCH | CVAR_ARCHIVE, "description" );
#endif
    modified += net_mcast6iface->modified;
    net_mcast6iface->modified = false;
    
    net_socksEnabled = cvarSystem->Get( "net_socksEnabled", "0", CVAR_LATCH | CVAR_ARCHIVE, "description" );
    modified += net_socksEnabled->modified;
    net_socksEnabled->modified = false;
    
    net_socksServer = cvarSystem->Get( "net_socksServer", "", CVAR_LATCH | CVAR_ARCHIVE, "description" );
    modified += net_socksServer->modified;
    net_socksServer->modified = false;
    
    net_socksPort = cvarSystem->Get( "net_socksPort", "1080", CVAR_LATCH | CVAR_ARCHIVE, "description" );
    modified += net_socksPort->modified;
    net_socksPort->modified = false;
    
    net_socksUsername = cvarSystem->Get( "net_socksUsername", "", CVAR_LATCH | CVAR_ARCHIVE, "description" );
    modified += net_socksUsername->modified;
    net_socksUsername->modified = false;
    
    net_socksPassword = cvarSystem->Get( "net_socksPassword", "", CVAR_LATCH | CVAR_ARCHIVE, "description" );
    modified += net_socksPassword->modified;
    net_socksPassword->modified = false;
    
    return modified ? true : false;
}


/*
====================
NET_Config
====================
*/
void NET_Config( bool enableNetworking )
{
    bool	modified;
    bool	stop;
    bool	start;
    
    // get any latched changes to cvars
    modified = NET_GetCvars();
    
    if( !net_enabled->integer )
    {
        enableNetworking = false;
    }
    
    // if enable state is the same and no cvars were modified, we have nothing to do
    if( enableNetworking == networkingEnabled && !modified )
    {
        return;
    }
    
    if( enableNetworking == networkingEnabled )
    {
        if( enableNetworking )
        {
            stop = true;
            start = true;
        }
        else
        {
            stop = false;
            start = false;
        }
    }
    else
    {
        if( enableNetworking )
        {
            stop = false;
            start = true;
        }
        else
        {
            stop = true;
            start = false;
        }
        networkingEnabled = enableNetworking;
    }
    
    if( stop )
    {
        if( ip_socket != INVALID_SOCKET )
        {
            closesocket( ip_socket );
            ip_socket = INVALID_SOCKET;
        }
        
        if( multicast6_socket )
        {
            if( multicast6_socket != ip6_socket )
                closesocket( multicast6_socket );
                
            multicast6_socket = INVALID_SOCKET;
        }
        
        if( ip6_socket != INVALID_SOCKET )
        {
            closesocket( ip6_socket );
            ip6_socket = INVALID_SOCKET;
        }
        
        if( socks_socket != INVALID_SOCKET )
        {
            closesocket( socks_socket );
            socks_socket = INVALID_SOCKET;
        }
        
    }
    
    if( start )
    {
        if( net_enabled->integer )
        {
            NET_OpenIP();
            NET_SetMulticast6();
        }
    }
}


/*
====================
NET_Init
====================
*/
void NET_Init( void )
{
#ifdef _WIN32
    S32		r;
    
    r = WSAStartup( MAKEWORD( 1, 1 ), &winsockdata );
    if( r )
    {
        Com_Printf( "WARNING: Winsock initialization failed, returned %d\n", r );
        return;
    }
    
    winsockInitialized = true;
    Com_Printf( "Winsock Initialized\n" );
#endif
    
    NET_Config( true );
    
    cmdSystem->AddCommand( "net_restart", NET_Restart_f, "description" );
}


/*
====================
NET_Shutdown
====================
*/
void NET_Shutdown( void )
{
    if( !networkingEnabled )
    {
        return;
    }
    
    NET_Config( false );
    
#ifdef _WIN32
    WSACleanup();
    winsockInitialized = false;
#endif
}


/*
====================
NET_Sleep

Sleeps msec or until something happens on the network
====================
*/
void NET_Sleep( S32 msec )
{
    struct timeval timeout;
    fd_set	fdset;
    S32 highestfd = -1;
    
    if( !com_dedicated->integer )
        return; // we're not a server, just run full speed
        
    if( ip_socket == INVALID_SOCKET && ip6_socket == INVALID_SOCKET )
        return;
        
    if( msec < 0 )
        return;
        
    FD_ZERO( &fdset );
    
    if( ip_socket != INVALID_SOCKET )
    {
        FD_SET( ip_socket, &fdset );
        
        highestfd = ip_socket;
    }
    if( ip6_socket != INVALID_SOCKET )
    {
        FD_SET( ip6_socket, &fdset );
        
        if( ip6_socket > highestfd )
            highestfd = ip6_socket;
    }
    
    timeout.tv_sec = msec / 1000;
    timeout.tv_usec = ( msec % 1000 ) * 1000;
    select( highestfd + 1, &fdset, NULL, NULL, &timeout );
}


/*
====================
NET_Restart_f
====================
*/
void NET_Restart_f( void )
{
    NET_Config( networkingEnabled );
}
