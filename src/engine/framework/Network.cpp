////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2020 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   network.cpp
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

idNetworkSystemLocal networkSystemLocal;
idNetworkSystem* networkSystem = &networkSystemLocal;

/*
===============
idNetworkSystemLocal::idNetworkSystemLocal
===============
*/
idNetworkSystemLocal::idNetworkSystemLocal( void )
{
}

/*
===============
idNetworkSystemLocal::~idNetworkSystemLocal
===============
*/
idNetworkSystemLocal::~idNetworkSystemLocal( void )
{
}

/*
====================
idNetworkSystemLocal::ErrorString
====================
*/
valueType* idNetworkSystemLocal::ErrorString( void )
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

/*
=============
idNetworkSystemLocal::NetadrToSockadr
=============
*/
void idNetworkSystemLocal::NetadrToSockadr( netadr_t* a, struct sockaddr* s )
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
        ( ( struct sockaddr_in* )s )->sin_addr.s_addr = *( sint* )&a->ip;
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

/*
=============
idNetworkSystemLocal::SockadrToNetadr
=============
*/
void idNetworkSystemLocal::SockadrToNetadr( struct sockaddr* s, netadr_t* a )
{
    if( s->sa_family == AF_INET )
    {
        a->type = NA_IP;
        *( sint* )&a->ip = ( ( struct sockaddr_in* )s )->sin_addr.s_addr;
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

/*
=============
idNetworkSystemLocal::SearchAddrInfo
=============
*/
struct addrinfo* idNetworkSystemLocal::SearchAddrInfo( struct addrinfo* hints, sa_family_t family )
{
    while( hints )
    {
        if( hints->ai_family == family )
            return hints;
            
        hints = hints->ai_next;
    }
    
    return nullptr;
}

/*
=============
idNetworkSystemLocal::StringToSockaddr
=============
*/
bool idNetworkSystemLocal::StringToSockaddr( pointer s, struct sockaddr* sadr, sint sadr_len, sa_family_t family )
{
    struct addrinfo hints;
    struct addrinfo* res = nullptr;
    struct addrinfo* search = nullptr;
    struct addrinfo* hintsp;
    sint retval;
    
    memset( sadr, '\0', sizeof( *sadr ) );
    memset( &hints, '\0', sizeof( hints ) );
    
    hintsp = &hints;
    hintsp->ai_family = family;
    hintsp->ai_socktype = SOCK_DGRAM;
    
    retval = getaddrinfo( s, nullptr, hintsp, &res );
    
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
            Com_Printf( "idNetworkSystemLocal::StringToSockaddr: Error resolving %s: No address of required type found.\n", s );
    }
    else
        Com_Printf( "idNetworkSystemLocal::StringToSockaddr: Error resolving %s: %s\n", s, gai_strerror( retval ) );
        
    if( res )
        freeaddrinfo( res );
        
    return false;
}

/*
=============
idNetworkSystemLocal::SockaddrToString
=============
*/
void idNetworkSystemLocal::SockaddrToString( valueType* dest, sint destlen, struct sockaddr* input )
{
    socklen_t inputlen;
    
    if( input->sa_family == AF_INET6 )
        inputlen = sizeof( struct sockaddr_in6 );
    else
        inputlen = sizeof( struct sockaddr_in );
        
    if( getnameinfo( input, inputlen, dest, destlen, nullptr, 0, NI_NUMERICHOST ) && destlen > 0 )
        *dest = '\0';
}

/*
=============
idNetworkSystemLocal::StringToAdr
=============
*/
bool idNetworkSystemLocal::StringToAdr( pointer s, netadr_t* a, netadrtype_t family )
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
    
    if( !StringToSockaddr( s, ( struct sockaddr* ) &sadr, sizeof( sadr ), fam ) )
    {
        return false;
    }
    
    SockadrToNetadr( ( struct sockaddr* ) &sadr, a );
    return true;
}

/*
===================
idNetworkSystemLocal::CompareBaseAdrMask

Compare without port, and up to the bit number given in netmask.
===================
*/
bool idNetworkSystemLocal::CompareBaseAdrMask( netadr_t a, netadr_t b, sint netmask )
{
    bool differed;
    uchar8 cmpmask, *addra, *addrb;
    sint curbyte;
    
    if( a.type != b.type )
        return false;
        
    if( a.type == NA_LOOPBACK )
        return true;
        
    if( a.type == NA_IP )
    {
        addra = ( uchar8* ) &a.ip;
        addrb = ( uchar8* ) &b.ip;
        
        if( netmask < 0 || netmask > 32 )
            netmask = 32;
    }
    else if( a.type == NA_IP6 )
    {
        addra = ( uchar8* ) &a.ip6;
        addrb = ( uchar8* ) &b.ip6;
        
        if( netmask < 0 || netmask > 128 )
            netmask = 128;
    }
    else
    {
        Com_Printf( "idNetworkSystemLocal::CompareBaseAdr: bad address type\n" );
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
idNetworkSystemLocal::CompareBaseAdr

Compares without the port
===================
*/
bool idNetworkSystemLocal::CompareBaseAdr( netadr_t a, netadr_t b )
{
    return CompareBaseAdrMask( a, b, -1 );
}

/*
=============
idNetworkSystemLocal::AdrToString
=============
*/
pointer	idNetworkSystemLocal::AdrToString( netadr_t a )
{
    static valueType s[NET_ADDRSTRMAXLEN];
    
    switch( a.type )
    {
        case NA_LOOPBACK:
            Com_sprintf( s, sizeof( s ), "loopback" );
            break;
        case NA_BOT:
            Com_sprintf( s, sizeof( s ), "bot" );
            break;
        case NA_IP:
            Com_sprintf( s, sizeof( s ), "%i.%i.%i.%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3] );
            break;
        case NA_IP6:
        {
            struct sockaddr_storage sadr;
            
            memset( &sadr, 0, sizeof( sadr ) );
            NetadrToSockadr( &a, ( struct sockaddr* )&sadr );
            SockaddrToString( s, sizeof( s ), ( struct sockaddr* )&sadr );
            break;
        }
        break;
        case NA_BAD:
            Com_sprintf( s, sizeof( s ), "invalid" );
            break;
        default:
            Com_Printf( "idNetworkSystemLocal::AdrToString: Unknown address type: %i\n", a.type );
            Com_sprintf( s, sizeof( s ), "unknown" );
            break;
    }
    
    return s;
}

/*
=============
idNetworkSystemLocal::AdrToStringwPort
=============
*/
pointer	idNetworkSystemLocal::AdrToStringwPort( netadr_t a )
{
    static valueType s[NET_ADDRSTRMAXLEN];
    
    switch( a.type )
    {
        case NA_LOOPBACK:
            Com_sprintf( s, sizeof( s ), "loopback" );
            break;
        case NA_BOT:
            Com_sprintf( s, sizeof( s ), "bot" );
            break;
        case NA_IP:
            Com_sprintf( s, sizeof( s ), "%i.%i.%i.%i:%hu", a.ip[0], a.ip[1], a.ip[2], a.ip[3], BigShort( a.port ) );
            break;
        case NA_IP6:
            Com_sprintf( s, sizeof( s ), "[%s]:%hu", AdrToString( a ), ntohs( a.port ) );
            break;
        case NA_BAD:
            Com_sprintf( s, sizeof( s ), "invalid" );
            break;
        default:
            Com_Printf( "idNetworkSystemLocal::AdrToStringwPort: Unknown address type: %i\n", a.type );
            Com_sprintf( s, sizeof( s ), "unknown" );
            break;
    }
    
    return s;
}

/*
=============
idNetworkSystemLocal::CompareAdr
=============
*/
bool idNetworkSystemLocal::CompareAdr( netadr_t a, netadr_t b )
{
    if( !CompareBaseAdr( a, b ) )
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

/*
=============
idNetworkSystemLocal::IsLocalAddress
=============
*/
bool idNetworkSystemLocal::IsLocalAddress( netadr_t adr )
{
    return ( bool )( adr.type == NA_LOOPBACK );
}

//=============================================================================

/*
==================
idNetworkSystemLocal::GetPacket

Never called by the game logic, just the system event queing
==================
*/
#ifdef _DEBUG
sint	recvfromCount;
#endif

bool idNetworkSystemLocal::GetPacket( netadr_t* net_from, msg_t* net_message )
{
    sint 	ret;
    struct sockaddr_storage from;
    socklen_t	fromlen;
    sint		err;
    
#ifdef _DEBUG
    recvfromCount++;		// performance check
#endif
    
    if( ip_socket != INVALID_SOCKET )
    {
        fromlen = sizeof( from );
        ret = recvfrom( ip_socket, ( valueType* )net_message->data, net_message->maxsize, 0, ( struct sockaddr* ) &from, &fromlen );
        
        if( ret == SOCKET_ERROR )
        {
            err = socketError;
            
            if( err != EAGAIN && err != ECONNRESET )
                Com_Printf( "idNetworkSystemLocal::GetPacket: %s\n", ErrorString() );
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
                net_from->port = *( schar16* )&net_message->data[8];
                net_message->readcount = 10;
            }
            else
            {
                SockadrToNetadr( ( struct sockaddr* ) &from, net_from );
                net_message->readcount = 0;
            }
            
            if( ret == net_message->maxsize )
            {
                Com_Printf( "Oversize packet from %s\n", AdrToString( *net_from ) );
                return false;
            }
            
            net_message->cursize = ret;
            return true;
        }
    }
    
    if( ip6_socket != INVALID_SOCKET )
    {
        fromlen = sizeof( from );
        ret = recvfrom( ip6_socket, ( valueType* )net_message->data, net_message->maxsize, 0, ( struct sockaddr* ) &from, &fromlen );
        
        if( ret == SOCKET_ERROR )
        {
            err = socketError;
            
            if( err != EAGAIN && err != ECONNRESET )
                Com_Printf( "idNetworkSystemLocal::GetPacket: %s\n", ErrorString() );
        }
        else
        {
            SockadrToNetadr( ( struct sockaddr* ) &from, net_from );
            net_message->readcount = 0;
            
            if( ret == net_message->maxsize )
            {
                Com_Printf( "Oversize packet from %s\n", AdrToString( *net_from ) );
                return false;
            }
            
            net_message->cursize = ret;
            return true;
        }
    }
    
    if( multicast6_socket != INVALID_SOCKET && multicast6_socket != ip6_socket )
    {
        fromlen = sizeof( from );
        ret = recvfrom( multicast6_socket, ( valueType* )net_message->data, net_message->maxsize, 0, ( struct sockaddr* ) &from, &fromlen );
        
        if( ret == SOCKET_ERROR )
        {
            err = socketError;
            
            if( err != EAGAIN && err != ECONNRESET )
                Com_Printf( "idNetworkSystemLocal::GetPacket: %s\n", ErrorString() );
        }
        else
        {
            SockadrToNetadr( ( struct sockaddr* ) &from, net_from );
            net_message->readcount = 0;
            
            if( ret == net_message->maxsize )
            {
                Com_Printf( "Oversize packet from %s\n", AdrToString( *net_from ) );
                return false;
            }
            
            net_message->cursize = ret;
            return true;
        }
    }
    
    
    return false;
}

/*
==================
idNetworkSystemLocal::SendPacket
==================
*/
void idNetworkSystemLocal::SendPacket( sint length, const void* data, netadr_t to )
{
    sint				ret = SOCKET_ERROR;
    struct			sockaddr_storage addr;
    
    if( to.type != NA_BROADCAST && to.type != NA_IP && to.type != NA_IP6 && to.type != NA_MULTICAST6 )
    {
        Com_Error( ERR_FATAL, "idNetworkSystemLocal::SendPacket: bad address type" );
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
        *( sint* )&socksBuf[4] = ( ( struct sockaddr_in* )&addr )->sin_addr.s_addr;
        *( schar16* )&socksBuf[8] = ( ( struct sockaddr_in* )&addr )->sin_port;
        memcpy( &socksBuf[10], data, length );
        ret = sendto( ip_socket, socksBuf, length + 10, 0, &socksRelayAddr, sizeof( socksRelayAddr ) );
    }
    else
    {
        if( addr.ss_family == AF_INET )
            ret = sendto( ip_socket, ( pointer )data, length, 0, ( struct sockaddr* ) &addr, sizeof( struct sockaddr_in ) );
        else if( addr.ss_family == AF_INET6 )
            ret = sendto( ip6_socket, ( pointer )data, length, 0, ( struct sockaddr* ) &addr, sizeof( struct sockaddr_in6 ) );
    }
    if( ret == SOCKET_ERROR )
    {
        sint err = socketError;
        
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
        
        Com_Printf( "idNetworkSystemLocal::SendPacket: %s\n", ErrorString() );
    }
}

/*
==================
idNetworkSystemLocal::IsLANAddress

LAN clients will have their rate var ignored
==================
*/
bool idNetworkSystemLocal::IsLANAddress( netadr_t adr )
{
    sint		index, run, addrsize;
    bool differed;
    uchar8* compareadr, *comparemask, *compareip;
    
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
                compareip = ( uchar8* ) & ( ( struct sockaddr_in* ) &localIP[index].addr )->sin_addr.s_addr;
                comparemask = ( uchar8* ) & ( ( struct sockaddr_in* ) &localIP[index].netmask )->sin_addr.s_addr;
                compareadr = adr.ip;
                
                addrsize = sizeof( adr.ip );
            }
            else
            {
                // TODO? should we check the scope_id here?
                
                compareip = ( uchar8* ) & ( ( struct sockaddr_in6* ) &localIP[index].addr )->sin6_addr;
                comparemask = ( uchar8* ) & ( ( struct sockaddr_in6* ) &localIP[index].netmask )->sin6_addr;
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
idNetworkSystemLocal::ShowIP
==================
*/
void idNetworkSystemLocal::ShowIP( void )
{
    sint i;
    valueType addrbuf[NET_ADDRSTRMAXLEN];
    
    for( i = 0; i < numIP; i++ )
    {
        SockaddrToString( addrbuf, sizeof( addrbuf ), ( struct sockaddr* ) &localIP[i].addr );
        
        if( localIP[i].type == NA_IP )
            Com_Printf( "IP: %s\n", addrbuf );
        else if( localIP[i].type == NA_IP6 )
            Com_Printf( "IP6: %s\n", addrbuf );
    }
}

/*
====================
idNetworkSystemLocal::IPSocket
====================
*/
SOCKET idNetworkSystemLocal::IPSocket( valueType* net_interface, sint port, sint* err )
{
    SOCKET				newsocket;
    struct sockaddr_in	address;
    u_long				_true = 1;
    sint					i = 1;
    
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
        Com_Printf( "WARNING: idNetworkSystemLocal::IPSocket: socket: %s\n", ErrorString() );
        return newsocket;
    }
    // make it non-blocking
    if( ioctlsocket( newsocket, FIONBIO, &_true ) == SOCKET_ERROR )
    {
        Com_Printf( "WARNING: idNetworkSystemLocal::IPSocket: ioctl FIONBIO: %s\n", ErrorString() );
        *err = socketError;
        closesocket( newsocket );
        return INVALID_SOCKET;
    }
    
    // make it broadcast capable
    if( setsockopt( newsocket, SOL_SOCKET, SO_BROADCAST, ( valueType* ) &i, sizeof( i ) ) == SOCKET_ERROR )
    {
        Com_Printf( "WARNING: idNetworkSystemLocal::IPSocket: setsockopt SO_BROADCAST: %s\n", ErrorString() );
    }
    
    if( !net_interface || !net_interface[0] )
    {
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
        if( !StringToSockaddr( net_interface, ( struct sockaddr* )&address, sizeof( address ), AF_INET ) )
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
        address.sin_port = htons( ( schar16 )port );
    }
    
    if( bind( newsocket, ( struct sockaddr* )&address, sizeof( address ) ) == SOCKET_ERROR )
    {
        Com_Printf( "WARNING: idNetworkSystemLocal::IPSocket: bind: %s\n", ErrorString() );
        *err = socketError;
        closesocket( newsocket );
        return INVALID_SOCKET;
    }
    
    return newsocket;
}

/*
====================
idNetworkSystemLocal::IP6Socket
====================
*/
SOCKET idNetworkSystemLocal::IP6Socket( valueType* net_interface, sint port, struct sockaddr_in6* bindto, sint* err )
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
        Com_Printf( "WARNING: idNetworkSystemLocal::IP6Socket: socket: %s\n", ErrorString() );
        return newsocket;
    }
    
    // make it non-blocking
    if( ioctlsocket( newsocket, FIONBIO, &_true ) == SOCKET_ERROR )
    {
        Com_Printf( "WARNING: idNetworkSystemLocal::IP6Socket: ioctl FIONBIO: %s\n", ErrorString() );
        *err = socketError;
        closesocket( newsocket );
        return INVALID_SOCKET;
    }
    
#ifdef IPV6_V6ONLY
    {
        sint i = 1;
        
        // ipv4 addresses should not be allowed to connect via this socket.
        if( setsockopt( newsocket, IPPROTO_IPV6, IPV6_V6ONLY, ( valueType* ) &i, sizeof( i ) ) == SOCKET_ERROR )
        {
            // win32 systems don't seem to support this anyways.
            Com_DPrintf( "WARNING: idNetworkSystemLocal::IP6Socket: setsockopt IPV6_V6ONLY: %s\n", ErrorString() );
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
        if( !StringToSockaddr( net_interface, ( struct sockaddr* )&address, sizeof( address ), AF_INET6 ) )
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
        address.sin6_port = htons( ( schar16 )port );
    }
    
    if( bind( newsocket, ( struct sockaddr* )&address, sizeof( address ) ) == SOCKET_ERROR )
    {
        Com_Printf( "WARNING: idNetworkSystemLocal::IP6Socket: bind: %s\n", ErrorString() );
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
idNetworkSystemLocal::SetMulticast

Set the current multicast group
====================
*/
void idNetworkSystemLocal::SetMulticast6( void )
{
    struct sockaddr_in6 addr;
    
    if( !*net_mcast6addr->string || !StringToSockaddr( net_mcast6addr->string, ( struct sockaddr* ) &addr, sizeof( addr ), AF_INET6 ) )
    {
        Com_Printf( "WARNING: idNetworkSystemLocal::JoinMulticast6: Incorrect multicast address given, "
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
    {
        curgroup.ipv6mr_interface = 0;
    }
}

/*
====================
idNetworkSystemLocal::JoinMulticast

Join an ipv6 multicast group
====================
*/
void idNetworkSystemLocal::JoinMulticast6( void )
{
    sint err;
    
    if( ip6_socket == INVALID_SOCKET || multicast6_socket != INVALID_SOCKET || ( net_enabled->integer & NET_DISABLEMCAST ) )
        return;
        
    if( IN6_IS_ADDR_MULTICAST( &boundto.sin6_addr ) || IN6_IS_ADDR_UNSPECIFIED( &boundto.sin6_addr ) )
    {
        // The way the socket was bound does not prohibit receiving multi-cast packets. So we don't need to open a new one.
        multicast6_socket = ip6_socket;
    }
    else
    {
        if( ( multicast6_socket = IP6Socket( net_mcast6addr->string, ntohs( boundto.sin6_port ), nullptr, &err ) ) == INVALID_SOCKET )
        {
            // If the OS does not support binding to multicast addresses, like WinXP, at least try with the normal file descriptor.
            multicast6_socket = ip6_socket;
        }
    }
    
    if( curgroup.ipv6mr_interface )
    {
        if( setsockopt( multicast6_socket, IPPROTO_IPV6, IPV6_MULTICAST_IF,
                        ( valueType* ) &curgroup.ipv6mr_interface, sizeof( curgroup.ipv6mr_interface ) ) < 0 )
        {
            Com_Printf( "idNetworkSystemLocal::JoinMulticast6: Couldn't set scope on multicast socket: %s\n", ErrorString() );
            
            if( multicast6_socket != ip6_socket )
            {
                closesocket( multicast6_socket );
                multicast6_socket = INVALID_SOCKET;
                return;
            }
        }
    }
    
    if( setsockopt( multicast6_socket, IPPROTO_IPV6, IPV6_JOIN_GROUP, ( valueType* ) &curgroup, sizeof( curgroup ) ) )
    {
        Com_Printf( "idNetworkSystemLocal::JoinMulticast6: Couldn't join multicast group: %s\n", ErrorString() );
        
        if( multicast6_socket != ip6_socket )
        {
            closesocket( multicast6_socket );
            multicast6_socket = INVALID_SOCKET;
            return;
        }
    }
}

/*
=============
idNetworkSystemLocal::LeaveMulticast6
=============
*/
void idNetworkSystemLocal::LeaveMulticast6( void )
{
    if( multicast6_socket != INVALID_SOCKET )
    {
        if( multicast6_socket != ip6_socket )
        {
            closesocket( multicast6_socket );
        }
        else
        {
            setsockopt( multicast6_socket, IPPROTO_IPV6, IPV6_LEAVE_GROUP, ( valueType* )&curgroup, sizeof( curgroup ) );
        }
        
        multicast6_socket = INVALID_SOCKET;
    }
}

/*
====================
idNetworkSystemLocal::OpenSocks
====================
*/
void idNetworkSystemLocal::OpenSocks( sint port )
{
    struct sockaddr_in	address;
    sint					err;
    struct hostent*		h;
    sint					len;
    bool			rfc1929;
    uchar8		buf[64];
    
    usingSocks = false;
    
    Com_Printf( "Opening connection to SOCKS server.\n" );
    
    if( ( socks_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP ) ) == INVALID_SOCKET )
    {
        err = socketError;
        Com_Printf( "WARNING: idNetworkSystemLocal::OpenSocks: socket: %s\n", ErrorString() );
        return;
    }
    
    h = gethostbyname( net_socksServer->string );
    if( h == nullptr )
    {
        err = socketError;
        Com_Printf( "WARNING: idNetworkSystemLocal::OpenSocks: gethostbyname: %s\n", ErrorString() );
        return;
    }
    if( h->h_addrtype != AF_INET )
    {
        Com_Printf( "WARNING: idNetworkSystemLocal::OpenSocks: gethostbyname: address type was not AF_INET\n" );
        return;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = *( sint* )h->h_addr_list[0];
    address.sin_port = htons( ( schar16 )net_socksPort->integer );
    
    if( connect( socks_socket, ( struct sockaddr* )&address, sizeof( address ) ) == SOCKET_ERROR )
    {
        err = socketError;
        Com_Printf( "idNetworkSystemLocal::OpenSocks: connect: %s\n", ErrorString() );
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
    if( send( socks_socket, ( valueType* )buf, len, 0 ) == SOCKET_ERROR )
    {
        err = socketError;
        Com_Printf( "idNetworkSystemLocal::OpenSocks: send: %s\n", ErrorString() );
        return;
    }
    
    // get the response
    len = recv( socks_socket, ( valueType* )buf, 64, 0 );
    if( len == SOCKET_ERROR )
    {
        err = socketError;
        Com_Printf( "idNetworkSystemLocal::OpenSocks: recv: %s\n", ErrorString() );
        return;
    }
    if( len != 2 || buf[0] != 5 )
    {
        Com_Printf( "idNetworkSystemLocal::OpenSocks: bad response\n" );
        return;
    }
    switch( buf[1] )
    {
        case 0:	// no authentication
            break;
        case 2: // username/password authentication
            break;
        default:
            Com_Printf( "idNetworkSystemLocal::OpenSocks: request denied\n" );
            return;
    }
    
    // do username/password authentication if needed
    if( buf[1] == 2 )
    {
        sint		ulen;
        sint		plen;
        
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
        if( send( socks_socket, ( valueType* )buf, 3 + ulen + plen, 0 ) == SOCKET_ERROR )
        {
            err = socketError;
            Com_Printf( "idNetworkSystemLocal::OpenSocks: send: %s\n", ErrorString() );
            return;
        }
        
        // get the response
        len = recv( socks_socket, ( valueType* )buf, 64, 0 );
        if( len == SOCKET_ERROR )
        {
            err = socketError;
            Com_Printf( "idNetworkSystemLocal::OpenSocks: recv: %s\n", ErrorString() );
            return;
        }
        if( len != 2 || buf[0] != 1 )
        {
            Com_Printf( "idNetworkSystemLocal::OpenSocks: bad response\n" );
            return;
        }
        if( buf[1] != 0 )
        {
            Com_Printf( "idNetworkSystemLocal::OpenSocks: authentication failed\n" );
            return;
        }
    }
    
    // send the UDP associate request
    buf[0] = 5;		// SOCKS version
    buf[1] = 3;		// command: UDP associate
    buf[2] = 0;		// reserved
    buf[3] = 1;		// address type: IPV4
    *( sint* )&buf[4] = INADDR_ANY;
    *( schar16* )&buf[8] = htons( ( schar16 )port );		// port
    if( send( socks_socket, ( valueType* )buf, 10, 0 ) == SOCKET_ERROR )
    {
        err = socketError;
        Com_Printf( "idNetworkSystemLocal::OpenSocks: send: %s\n", ErrorString() );
        return;
    }
    
    // get the response
    len = recv( socks_socket, ( valueType* )buf, 64, 0 );
    if( len == SOCKET_ERROR )
    {
        err = socketError;
        Com_Printf( "idNetworkSystemLocal::OpenSocks: recv: %s\n", ErrorString() );
        return;
    }
    if( len < 2 || buf[0] != 5 )
    {
        Com_Printf( "idNetworkSystemLocal::OpenSocks: bad response\n" );
        return;
    }
    // check completion code
    if( buf[1] != 0 )
    {
        Com_Printf( "idNetworkSystemLocal::OpenSocks: request denied: %i\n", buf[1] );
        return;
    }
    if( buf[3] != 1 )
    {
        Com_Printf( "idNetworkSystemLocal::OpenSocks: relay address is not IPV4: %i\n", buf[3] );
        return;
    }
    ( ( struct sockaddr_in* )&socksRelayAddr )->sin_family = AF_INET;
    ( ( struct sockaddr_in* )&socksRelayAddr )->sin_addr.s_addr = *( sint* )&buf[4];
    ( ( struct sockaddr_in* )&socksRelayAddr )->sin_port = *( schar16* )&buf[8];
    memset( ( ( struct sockaddr_in* )&socksRelayAddr )->sin_zero, 0, 8 );
    
    usingSocks = true;
}


/*
=====================
idNetworkSystemLocal::AddLocalAddress
=====================
*/
void idNetworkSystemLocal::AddLocalAddress( valueType* ifname, struct sockaddr* addr, struct sockaddr* netmask )
{
    sint addrlen;
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

/*
=============
idNetworkSystemLocal::GetLocalAddress
=============
*/
void idNetworkSystemLocal::GetLocalAddress( void )
{
#if defined(__linux__) || defined(MACOSX) || defined(__BSD__)
    struct ifaddrs* ifap, * search;
#else
    valueType hostname[256];
    struct addrinfo	hint;
    struct addrinfo* res = NULL;
#endif
    
    numIP = 0;
    
#if defined(__linux__) || defined(MACOSX) || defined(__BSD__)
    if( getifaddrs( &ifap ) )
    {
        Com_Printf( "idNetworkSystemLocal::GetLocalAddress: Unable to get list of network interfaces: %s\n", ErrorString() );
    }
    else
    {
        for( search = ifap; search; search = search->ifa_next )
        {
            // Only add interfaces that are up.
            if( ifap->ifa_flags & IFF_UP )
            {
                AddLocalAddress( search->ifa_name, search->ifa_addr, search->ifa_netmask );
            }
        }
        
        freeifaddrs( ifap );
        
        networkSystemLocal.ShowIP();
    }
#else
    if( gethostname( hostname, 256 ) == SOCKET_ERROR )
    {
        return;
    }
    
    Com_Printf( "Hostname: %s\n", hostname );
    
    ::memset( &hint, 0, sizeof( hint ) );
    
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_DGRAM;
    
    if( !getaddrinfo( hostname, NULL, &hint, &res ) )
    {
        struct sockaddr_in mask4;
        struct sockaddr_in6 mask6;
        struct addrinfo* search;
    
        //On operating systems where it's more difficult to find out the configured interfaces, we'll just assume a
        //netmask with all bits set.
    
        ::memset( &mask4, 0, sizeof( mask4 ) );
        ::memset( &mask6, 0, sizeof( mask6 ) );
        mask4.sin_family = AF_INET;
        ::memset( &mask4.sin_addr.s_addr, 0xFF, sizeof( mask4.sin_addr.s_addr ) );
        mask6.sin6_family = AF_INET6;
        ::memset( &mask6.sin6_addr, 0xFF, sizeof( mask6.sin6_addr ) );
    
        // add all IPs from returned list.
        for( search = res; search; search = search->ai_next )
        {
            if( search->ai_family == AF_INET )
            {
                AddLocalAddress( "", search->ai_addr, ( struct sockaddr* )&mask4 );
            }
            else if( search->ai_family == AF_INET6 )
            {
                AddLocalAddress( "", search->ai_addr, ( struct sockaddr* )&mask6 );
            }
        }
    
        networkSystemLocal.ShowIP();
    }
    
    if( res )
    {
        freeaddrinfo( res );
    }
#endif
}

/*
====================
idNetworkSystemLocal::OpenIP
====================
*/
void idNetworkSystemLocal::OpenIP( void )
{
    sint		i;
    sint		err;
    sint		port;
    sint		port6;
    
    port = net_port->integer;
    port6 = net_port6->integer;
    
    GetLocalAddress();
    
    // automatically scan for a valid port, so multiple
    // dedicated servers can be started without requiring
    // a different net_port for each one
    
    if( net_enabled->integer & NET_ENABLEV6 )
    {
        for( i = 0 ; i < 10 ; i++ )
        {
            ip6_socket = IP6Socket( net_ip6->string, port6 + i, &boundto, &err );
            if( ip6_socket != INVALID_SOCKET )
            {
                cvarSystem->SetValue( "net_port6", port6 + i );
                cvarSystem->SetValue( "sv_cs_ServerPort", port6 + i );
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
            ip_socket = IPSocket( net_ip->string, port + i, &err );
            if( ip_socket != INVALID_SOCKET )
            {
                cvarSystem->SetValue( "net_port", port + i );
                cvarSystem->SetValue( "sv_cs_ServerPort", port + i );
                
                if( net_socksEnabled->integer )
                    OpenSocks( port + i );
                    
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

/*
====================
idNetworkSystemLocal::GetCvars
====================
*/
bool idNetworkSystemLocal::GetCvars( void )
{
    sint modified;
    
#ifdef DEDICATED
    // I want server owners to explicitly turn on ipv6 support.
    net_enabled = cvarSystem->Get( "net_enabled", "1", CVAR_LATCH | CVAR_ARCHIVE, "Enable networking, bitmask. Add up number for option to enable it : enable ipv4 networking : 1 enable ipv6 networking : 2 prioritise ipv6 over ipv4 : 4 disable multicast support : 8" );
#else
    /* End users have it enabled so they can connect to ipv6-only hosts, but ipv4 will be
     * used if available due to ping */
    net_enabled = cvarSystem->Get( "net_enabled", "3", CVAR_LATCH | CVAR_ARCHIVE, "Enable networking, bitmask. Add up number for option to enable it : enable ipv4 networking : 1 enable ipv6 networking : 2 prioritise ipv6 over ipv4 : 4 disable multicast support : 8" );
#endif
    modified = net_enabled->modified;
    net_enabled->modified = false;
    
    net_ip = cvarSystem->Get( "net_ip", "0.0.0.0", CVAR_LATCH, "IPv4 address to bind to" );
    modified += net_ip->modified;
    net_ip->modified = false;
    
    net_ip6 = cvarSystem->Get( "net_ip6", "::", CVAR_LATCH, "IPv6 address to bind to" );
    modified += net_ip6->modified;
    net_ip6->modified = false;
    
    net_port = cvarSystem->Get( "net_port", va( "%i", PORT_SERVER ), CVAR_LATCH, "Port to bind to using the ipv4 address" );
    modified += net_port->modified;
    net_port->modified = false;
    
    net_port6 = cvarSystem->Get( "net_port6", va( "%i", PORT_SERVER ), CVAR_LATCH, "Port to bind to using the ipv6 address" );
    modified += net_port6->modified;
    net_port6->modified = false;
    
    // Some cvars for configuring multicast options which facilitates scanning for servers on local subnets.
    net_mcast6addr = cvarSystem->Get( "net_mcast6addr", NET_MULTICAST_IP6, CVAR_LATCH | CVAR_ARCHIVE, "Multicast address to use for scanning for ipv6 servers on the local network" );
    modified += net_mcast6addr->modified;
    net_mcast6addr->modified = false;
    
#ifdef _WIN32
    net_mcast6iface = cvarSystem->Get( "net_mcast6iface", "0", CVAR_LATCH | CVAR_ARCHIVE, "Enables the outgoing interface used for IPv6 multicast scanning on LAN." );
#else
    net_mcast6iface = cvarSystem->Get( "net_mcast6iface", "", CVAR_LATCH | CVAR_ARCHIVE, "Enables the outgoing interface used for IPv6 multicast scanning on LAN." );
#endif
    modified += net_mcast6iface->modified;
    net_mcast6iface->modified = false;
    
    net_socksEnabled = cvarSystem->Get( "net_socksEnabled", "0", CVAR_LATCH | CVAR_ARCHIVE, "Enables socks 5 network protocol." );
    modified += net_socksEnabled->modified;
    net_socksEnabled->modified = false;
    
    net_socksServer = cvarSystem->Get( "net_socksServer", "", CVAR_LATCH | CVAR_ARCHIVE, "Sets the name or IP address of the socks server." );
    modified += net_socksServer->modified;
    net_socksServer->modified = false;
    
    net_socksPort = cvarSystem->Get( "net_socksPort", "1080", CVAR_LATCH | CVAR_ARCHIVE, "Sets proxy and firewall port." );
    modified += net_socksPort->modified;
    net_socksPort->modified = false;
    
    net_socksUsername = cvarSystem->Get( "net_socksUsername", "", CVAR_LATCH | CVAR_ARCHIVE, "Sets the username for socks firewall supports. It does not support GSS-API authentication." );
    modified += net_socksUsername->modified;
    net_socksUsername->modified = false;
    
    net_socksPassword = cvarSystem->Get( "net_socksPassword", "", CVAR_LATCH | CVAR_ARCHIVE, "Sets password for socks network/firewall access." );
    modified += net_socksPassword->modified;
    net_socksPassword->modified = false;
    
    return modified ? true : false;
}


/*
====================
idNetworkSystemLocal::Config
====================
*/
void idNetworkSystemLocal::Config( bool enableNetworking )
{
    bool	modified;
    bool	stop;
    bool	start;
    
    // get any latched changes to cvars
    modified = GetCvars();
    
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
            OpenIP();
            SetMulticast6();
        }
    }
}

/*
===============
idNetworkSystemLocal::NETRestart_f

I had a problem with AddCommand and pointing to the class member so this is one way of doing
pointing to a function that is a class member
===============
*/
void idNetworkSystemLocal::NETRestart_f( void )
{
    static_cast<idNetworkSystemLocal*>( networkSystem )->Restart_f();
}

/*
====================
idNetworkSystemLocal::Init
====================
*/
void idNetworkSystemLocal::Init( void )
{
#ifdef _WIN32
    sint		r;
    
    r = WSAStartup( MAKEWORD( 1, 1 ), &winsockdata );
    if( r )
    {
        Com_Printf( "WARNING: Winsock initialization failed, returned %d\n", r );
        return;
    }
    
    winsockInitialized = true;
    Com_Printf( "Winsock Initialized\n" );
#endif
    
    Config( true );
    
    cmdSystem->AddCommand( "net_restart", NETRestart_f, "If you change any net_ setting in-game or in a config you need to also to a net_restart to make the changes take effect, i have seen very few people with this in their configurations, but i have seen some people specify a lot of net_ settings and they have it in their config." );
}


/*
====================
idNetworkSystemLocal::Shutdown
====================
*/
void idNetworkSystemLocal::Shutdown( void )
{
    if( !networkingEnabled )
    {
        return;
    }
    
    Config( false );
    
#ifdef _WIN32
    WSACleanup();
    winsockInitialized = false;
#endif
}


/*
====================
idNetworkSystemLocal::Sleep

Sleeps msec or until something happens on the network
====================
*/
void idNetworkSystemLocal::Sleep( sint msec )
{
    struct timeval timeout;
    fd_set	fdset;
    sint highestfd = -1;
    
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
    select( highestfd + 1, &fdset, nullptr, nullptr, &timeout );
}

/*
====================
idNetworkSystemLocal::Restart_f
====================
*/
void idNetworkSystemLocal::Restart_f( void )
{
    Config( networkingEnabled );
}

/*
====================
idNetworkSystemLocal::ConnectTCP
====================
*/
sint idNetworkSystemLocal::ConnectTCP( valueType* s_host_port )
{
    sint err, sock;
    valueType buffer[1024], *s_server, *s_port;
    struct sockaddr_in address;
    struct hostent* h;
    
    ::strcpy( buffer, s_host_port );
    
    s_server = strtok( buffer, ":\n\0" );
    if( s_server == nullptr )
    {
        Com_Printf( "Error parsing server string %s does not have port\n", s_host_port );
        return -1;
    }
    
    s_port = ::strtok( nullptr, "\n\0" );
    if( s_port == nullptr )
    {
        Com_Printf( "Error parsing server string %s port problem\n", s_host_port );
    }
    
    if( ( sock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP ) ) == INVALID_SOCKET )
    {
        err = socketError;
        Com_Printf( "WARNING: idNetworkSystemLocal::ConnectTCP: socket: %s\n", ErrorString() );
        return -1;
    }
    
    h = gethostbyname( s_server );
    if( h == nullptr )
    {
        err = socketError;
        Com_Printf( "WARNING: idNetworkSystemLocal::ConnectTCP: gethostbyname: %s\n", ErrorString() );
        close( sock );
        return -1;
    }
    
    if( h->h_addrtype != AF_INET )
    {
        Com_Printf( "WARNING: idNetworkSystemLocal::ConnectTCP: gethostbyname: address type was not AF_INET\n" );
        close( sock );
        return -1;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = *( sint* )h->h_addr_list[0];
    address.sin_port = htons( atoi( s_port ) );
    
    if( connect( sock, ( struct sockaddr* )&address, sizeof( address ) ) == SOCKET_ERROR )
    {
        err = socketError;
        Com_Printf( "NET_OpenSocks: connect: %s\n", ErrorString() );
        close( sock );
        return -1;
    }
    
    return sock;
}