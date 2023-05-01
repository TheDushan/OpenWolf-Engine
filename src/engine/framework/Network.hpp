////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2023 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   Network.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __NETWORK_HPP__
#define __NETWORK_HPP__

#ifdef _WIN32
typedef sint socklen_t;
#undef EAGAIN
#undef EADDRNOTAVAIL
#undef EAFNOSUPPORT
#undef ECONNRESET

#ifdef ADDRESS_FAMILY
#define sa_family_t ADDRESS_FAMILY
#else
typedef uchar16 sa_family_t;
#endif

#define EAGAIN WSAEWOULDBLOCK
#define EADDRNOTAVAIL WSAEADDRNOTAVAIL
#define EAFNOSUPPORT WSAEAFNOSUPPORT
#define ECONNRESET WSAECONNRESET
#define socketError WSAGetLastError( )

static WSADATA  winsockdata;
static bool winsockInitialized = false;

#else
#define INVALID_SOCKET      -1
#define SOCKET_ERROR            -1
#define closesocket         close
#define ioctlsocket         ioctl
#define socketError         errno
typedef sint SOCKET;
#endif

#ifndef IF_NAMESIZE
#define IF_NAMESIZE 16
#endif

#define MAX_IPS     32

static bool usingSocks = false;
static bool networkingEnabled = false;

static struct sockaddr  socksRelayAddr;

static SOCKET ip_socket = INVALID_SOCKET;
static SOCKET ip6_socket = INVALID_SOCKET;
static SOCKET socks_socket = INVALID_SOCKET;
static SOCKET multicast6_socket = INVALID_SOCKET;

// Keep track of currently joined multicast group.
static struct ipv6_mreq curgroup;
// And the currently bound address.
static struct sockaddr_in6 boundto;

#ifndef IF_NAMESIZE
#define IF_NAMESIZE 16
#endif

// use an admin local address per default so that network admins can decide on how to handle quake3 traffic.
#define NET_MULTICAST_IP6 "ff04::696f:7175:616b:6533"

#define MAX_IPS     32

typedef struct {
    valueType ifname[IF_NAMESIZE];

    netadrtype_t type;
    sa_family_t family;
    struct sockaddr_storage addr;
    struct sockaddr_storage netmask;
} nip_localaddr_t;

static nip_localaddr_t localIP[MAX_IPS];
static sint numIP;
static valueType socksBuf[4096];

//
// idNetworkSystemLocal
//
class idNetworkSystemLocal : public idNetworkSystem {
public:
    idNetworkSystemLocal();
    ~idNetworkSystemLocal();

    virtual bool StringToAdr(pointer s, netadr_t *a, netadrtype_t family);
    virtual bool CompareBaseAdrMask(netadr_t a, netadr_t b, sint netmask);
    virtual bool CompareBaseAdr(netadr_t a, netadr_t b);
    virtual pointer AdrToString(netadr_t a);
    virtual pointer AdrToStringwPort(netadr_t a);
    virtual bool CompareAdr(netadr_t a, netadr_t b);
    virtual bool IsLocalAddress(netadr_t adr);
    virtual bool GetPacket(netadr_t *net_from, msg_t *net_message);
    virtual void SendPacket(sint length, const void *data, netadr_t to);
    virtual bool IsLANAddress(netadr_t adr);
    virtual void ShowIP(void);
    virtual void JoinMulticast6(void);
    virtual void LeaveMulticast6(void);
    virtual void Init(void);
    virtual void Shutdown(void);
    virtual void Sleep(sint msec);
    virtual void Restart_f(void);
    virtual sint ConnectTCP(valueType *s_host_port);

    static valueType *ErrorString(void);
    static void SockaddrToString(valueType *dest, uint64 destlen,
                                 struct sockaddr *input);
    static void NetadrToSockadr(netadr_t *a, struct sockaddr *s);
    static void SockadrToNetadr(struct sockaddr *s, netadr_t *a);
    static struct addrinfo *SearchAddrInfo(struct addrinfo *hints,
                                           sa_family_t family);
    static bool StringToSockaddr(pointer s, struct sockaddr *sadr,
                                 sint sadr_len, sa_family_t family);
    static void SetMulticast6(void);
    static SOCKET IPSocket(valueType *net_interface, sint port, sint *err);
    static SOCKET IP6Socket(valueType *net_interface, sint port,
                            struct sockaddr_in6 *bindto, sint *err);
    static void OpenSocks(sint port);
    static void AddLocalAddress(valueType *ifname, struct sockaddr *addr,
                                struct sockaddr *netmask);
    static void GetLocalAddress(void);
    static bool GetCvars(void);
    static void OpenIP(void);
    static void Config(bool enableNetworking);
    static void NETRestart_f(void);
};

extern idNetworkSystemLocal networkSystemLocal;

#endif //!__NETWORK_HPP__
