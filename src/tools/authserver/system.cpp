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
// File name:   system.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
//              AppleClang 9.0.0.9000039
// Description: System specific code for owauthserver
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include "common.hpp"
#include "system.hpp"

// ---------- Constants ---------- //

#ifndef WIN32

// Default path we use for chroot
# define DEFAULT_JAIL_PATH "/var/empty/"

// User we use by default for dropping super-user privileges
# define DEFAULT_LOW_PRIV_USER "nobody"

#endif


// ---------- Private variables ---------- //

#ifndef WIN32

// Path we use for chroot
static const char *jail_path = DEFAULT_JAIL_PATH;

// Low privileges user
static const char *low_priv_user = DEFAULT_LOW_PRIV_USER;

// File descriptor to /dev/null, used by the daemonization process
static int null_device = -1;

#endif


// ---------- Public variables ---------- //

// The master sockets
unsigned int nb_sockets = 0;
listen_socket_t listen_sockets [MAX_LISTEN_SOCKETS];

// The port we use by default
unsigned short master_port = DEFAULT_MASTER_PORT;

// System specific command line options
const cmdlineopt_t sys_cmdline_options [] = {
#ifndef WIN32
    {
        "daemon",
        NULL,
        "Run as a daemon",
        { 0, 0 },
        'D',
        0,
        0
    },
    {
        "jail-path",
        "<jail_path>",
        "Use <jail_path> as chroot path (default: " DEFAULT_JAIL_PATH ")\n"
        "   Only available when running with super-user privileges",
        { 0, 0 },
        'j',
        1,
        1
    },
    {
        "user",
        "<user>",
        "Use <user> privileges (default: " DEFAULT_LOW_PRIV_USER ")\n"
        "   Only available when running with super-user privileges",
        { 0, 0 },
        'u',
        1,
        1
    },
#endif
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

// Daemon state
daemon_state_t daemon_state = DAEMON_STATE_NO;


// ---------- Private functions ---------- //

/*
====================
Sys_CloseSocket

Close a network socket
====================
*/
static void Sys_CloseSocket(socket_t sock) {
#ifdef WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}


/*
====================
Sys_CloseAllSockets

Close all network sockets
====================
*/
static void Sys_CloseAllSockets(void) {
    size_t sock_ind;

    for(sock_ind = 0; sock_ind < nb_sockets; sock_ind++) {
        listen_socket_t *sock = &listen_sockets[sock_ind];

        if(sock->socket != -1) {
            Sys_CloseSocket(sock->socket);
        }
    }

    nb_sockets = 0;
}


/*
====================
Sys_BuildSockaddr

Build a sockaddr
====================
*/
static bool Sys_BuildSockaddr(const char *addr_name, const char *port_name,
                              int addr_family_hint,
                              struct sockaddr_storage *sock_address,
                              socklen_t *sock_address_len) {
    char port_buff [8];
    struct addrinfo hints;
    struct addrinfo *addrinf = NULL;
    int err;

    // If there is no port, use the default one
    if(port_name == NULL) {
        snprintf(port_buff, sizeof(port_buff), "%hu", master_port);
        port_buff[sizeof(port_buff) - 1] = '\0';
        port_name = port_buff;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = addr_family_hint;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    err = getaddrinfo(addr_name, port_name, &hints, &addrinf);

    if(err != 0 || addrinf == NULL) {
        Com_Printf(MSG_ERROR, "> ERROR: can't resolve %s:%s (%s)\n",
                   addr_name, port_name, gai_strerror(err));

        if(addrinf != NULL) {
            freeaddrinfo(addrinf);
        }

        return false;
    }

    assert(addrinf->ai_addrlen <= sizeof(*sock_address));
    *sock_address_len = (socklen_t)addrinf->ai_addrlen;
    memcpy(sock_address, addrinf->ai_addr, addrinf->ai_addrlen);

    freeaddrinfo(addrinf);
    return true;
}


/*
====================
Sys_StringToSockaddr

Resolve an address
====================
*/
static bool Sys_StringToSockaddr(const char *address,
                                 struct sockaddr_storage *sock_address,
                                 socklen_t *sock_address_len) {
    const char *addr_start;
    const char *addr_end = NULL;
    const char *port_name = NULL;
    int addr_family = AF_UNSPEC;
    size_t addr_length;
    char addr_buff [128];

    // If it's a bracketed IPv6 address
    if(address[0] == '[') {
        const char *end_bracket = strchr(address, ']');

        if(end_bracket == NULL) {
            Com_Printf(MSG_ERROR,
                       "> ERROR: IPv6 address has no closing bracket (%s)\n",
                       address);
            return false;
        }

        if(end_bracket[1] != ':' && end_bracket[1] != '\0') {
            Com_Printf(MSG_ERROR,
                       "> ERROR: invalid end of bracketed IPv6 address (%s)\n",
                       address);
            return false;
        }

        if(end_bracket[1] == ':') {
            port_name = end_bracket + 2;
        }

        addr_family = AF_INET6;
        addr_start = &address[1];
        addr_end = end_bracket;
    } else {
        const char *first_colon;

        addr_start = address;

        // If it's a numeric non-bracket IPv6 address (-> no port),
        // or it's a numeric IPv4 address, or a name, with a port
        first_colon = strchr(address, ':');

        if(first_colon != NULL) {
            const char *last_colon = strrchr(first_colon + 1, ':');

            // If it's an numeric IPv4 address, or a name, with a port
            if(last_colon == NULL) {
                addr_end = first_colon;
                port_name = first_colon + 1;
            } else {
                addr_family = AF_INET6;
            }
        }
    }

    if(addr_end != NULL) {
        assert(addr_end >= addr_start);
        addr_length = addr_end - addr_start;
    } else {
        addr_length = strlen(addr_start);
    }

    // Check the address length
    if(addr_length >= sizeof(addr_buff)) {
        Com_Printf(MSG_ERROR,
                   "> ERROR: address too long to be resolved (%s)\n",
                   address);
        return false;
    }

    memcpy(addr_buff, addr_start, addr_length);
    addr_buff[addr_length] = '\0';

    return Sys_BuildSockaddr(addr_buff, port_name, addr_family, sock_address,
                             sock_address_len);
}


// ---------- Public functions (listening sockets) ---------- //

/*
====================
Sys_DeclareListenAddress

Step 1 - Add a listen socket to the listening socket list
====================
*/
bool Sys_DeclareListenAddress(const char *local_addr_name) {
    if(nb_sockets < MAX_LISTEN_SOCKETS) {
        listen_socket_t *listen_sock = &listen_sockets[nb_sockets];

        memset(listen_sock, 0, sizeof(*listen_sock));
        listen_sock->socket = INVALID_SOCKET;
        listen_sock->local_addr_name = local_addr_name;

        nb_sockets++;
        return true;
    } else
        Com_Printf(MSG_ERROR,
                   "> ERROR: too many listening addresses (max: %d)\n",
                   MAX_LISTEN_SOCKETS);

    return false;
}


/*
====================
Sys_ResolveListenAddresses

Step 2 - Resolve the address names of all the listening sockets
====================
*/
bool Sys_ResolveListenAddresses(void) {
    // If nothing to resolve, add the local IPv4 & IPv6 addresses
    if(nb_sockets == 0) {
        const sa_family_t addr_families [] = { AF_INET, AF_INET6 };
        const unsigned int nb_addrs = sizeof(addr_families) / sizeof(
                                          addr_families[0]);
        unsigned int addr_ind;

        memset(listen_sockets, 0, sizeof(listen_sockets[0]) * nb_addrs);

        for(addr_ind = 0; addr_ind < nb_addrs; addr_ind++) {
            if(! Sys_BuildSockaddr(NULL, NULL, addr_families[addr_ind],
                                   &listen_sockets[addr_ind].local_addr,
                                   &listen_sockets[addr_ind].local_addr_len)) {
                return false;
            }

            listen_sockets[addr_ind].optional = true;
            nb_sockets++;
        }
    } else {
        unsigned int sock_ind;

        for(sock_ind = 0; sock_ind < nb_sockets; sock_ind++) {
            listen_socket_t *listen_sock = &listen_sockets[sock_ind];

            if(! Sys_StringToSockaddr(listen_sock->local_addr_name,
                                      &listen_sock->local_addr,
                                      &listen_sock->local_addr_len)) {
                return false;
            }
        }
    }

    return true;
}


/*
====================
Sys_CreateListenSockets

Step 3 - Create the listening sockets
====================
*/
bool Sys_CreateListenSockets(void) {
    unsigned int sock_ind;

    for(sock_ind = 0; sock_ind < nb_sockets; sock_ind++) {
        listen_socket_t *listen_sock = &listen_sockets[sock_ind];
        socket_t crt_sock;
        int addr_family;

        addr_family = listen_sock->local_addr.ss_family;
        crt_sock = socket(addr_family, SOCK_DGRAM, IPPROTO_UDP);

        if(crt_sock == INVALID_SOCKET) {
            // If the address family isn't supported but the socket is optional, don't fail!
            if(Sys_GetLastNetError() == NETERR_AFNOSUPPORT &&
                    listen_sock->optional) {
                Com_Printf(MSG_WARNING, "> WARNING: protocol %s isn't supported\n",
                           (addr_family == AF_INET) ? "IPv4" :
                           ((addr_family == AF_INET6) ? "IPv6" : "UNKNOWN"));

                if(sock_ind + 1 < nb_sockets)
                    memmove(&listen_sockets[sock_ind], &listen_sockets[sock_ind + 1],
                            (nb_sockets - sock_ind - 2) * sizeof(listen_sockets[0]));

                sock_ind--;
                nb_sockets--;
                continue;
            }

            Com_Printf(MSG_ERROR, "> ERROR: socket creation failed (%s)\n",
                       Sys_GetLastNetErrorString());
            Sys_CloseAllSockets();
            return false;
        }

        if(addr_family == AF_INET6) {
            // Win32's API only supports it since Windows Vista, but fortunately
            // the default value is what we want on Win32 anyway (IPV6_V6ONLY = true)
#ifdef IPV6_V6ONLY
            int ipv6_only = 1;

            if(setsockopt(crt_sock, IPPROTO_IPV6, IPV6_V6ONLY,
                          (const char *)&ipv6_only, sizeof(ipv6_only)) != 0) {
#ifdef WIN32

                // This flag isn't supported before Windows Vista
                if(Sys_GetLastNetError() != NETERR_NOPROTOOPT)
#endif
                {
                    Com_Printf(MSG_ERROR, "> ERROR: setsockopt(IPV6_V6ONLY) failed (%s)\n",
                               Sys_GetLastNetErrorString());

                    Sys_CloseAllSockets();
                    return false;
                }
            }

#endif
        }

        if(listen_sock->local_addr_name != NULL) {
            const char *addr_str;

            addr_str = Sys_SockaddrToString(&listen_sock->local_addr,
                                            listen_sock->local_addr_len);
            Com_Printf(MSG_NORMAL, "> Listening on address %s (%s)\n",
                       listen_sock->local_addr_name,
                       addr_str);
        } else
            Com_Printf(MSG_NORMAL, "> Listening on all %s addresses\n",
                       addr_family == AF_INET6 ? "IPv6" : "IPv4");

        if(bind(crt_sock, (struct sockaddr *)&listen_sock->local_addr,
                listen_sock->local_addr_len) != 0) {
            Com_Printf(MSG_ERROR, "> ERROR: socket binding failed (%s)\n",
                       Sys_GetLastNetErrorString());

            Sys_CloseAllSockets();
            return false;
        }

        listen_sock->socket = crt_sock;
    }

    return true;
}


// ---------- Public functions (the rest) ---------- //

/*
====================
Sys_Cmdline_Option

Parse a system-dependent command line option
====================
*/
cmdline_status_t Sys_Cmdline_Option(const cmdlineopt_t *opt,
                                    const char **params, unsigned int nb_params) {
#ifndef WIN32

    const char *opt_name;

    opt_name = opt->long_name;

    // Daemon mode
    if(strcmp(opt_name, "daemon") == 0) {
        daemon_state = DAEMON_STATE_REQUEST;
    }

    // Jail path
    else if(strcmp(opt_name, "jail-path") == 0) {
        jail_path = params[0];
    }

    // Low privileges user
    else if(strcmp(opt_name, "user") == 0) {
        low_priv_user = params[0];
    }

    return CMDLINE_STATUS_OK;

#else

    assert(false);   // We should never be here
    return CMDLINE_STATUS_INVALID_OPT;

#endif
}


/*
====================
Sys_UnsecureInit

System dependent initializations (called BEFORE security initializations)
====================
*/
bool Sys_UnsecureInit(void) {
#ifdef WIN32
    WSADATA winsockdata;

    if(WSAStartup(MAKEWORD(1, 1), &winsockdata)) {
        Com_Printf(MSG_ERROR, "> ERROR: can't initialize winsocks\n");
        return false;
    }

#endif

    return true;
}


/*
====================
Sys_SecurityInit

System dependent security initializations
====================
*/
bool Sys_SecurityInit(void) {
#ifndef WIN32

    // If we will run as a daemon, we need to open /dev/null before chrooting
    if(daemon_state == DAEMON_STATE_REQUEST) {
        null_device = open("/dev/null", O_RDWR, 0);

        if(null_device == -1) {
            Com_Printf(MSG_ERROR, "> ERROR: can't open /dev/null\n");
            return false;
        }
    }

    // UNIX allows us to be completely paranoid, so let's go for it
    if(geteuid() == 0) {
        struct passwd *pw;

        Com_Printf(MSG_WARNING,
                   "> WARNING: running with super-user privileges\n");

        // We must get the account infos before the calls to chroot and chdir
        pw = getpwnam(low_priv_user);

        if(pw == NULL) {
            Com_Printf(MSG_ERROR, "> ERROR: can't get user \"%s\" properties\n",
                       low_priv_user);
            return false;
        }

        // Chroot ourself
        if(chroot(jail_path) || chdir("/")) {
            Com_Printf(MSG_ERROR,
                       "  - ERROR: can't chroot myself to %s (%s)\n",
                       jail_path, strerror(errno));
            return false;
        }

        Com_Printf(MSG_NORMAL, "  - Chrooted myself to %s\n", jail_path);

        // Switch to lower privileges
        if(setgid(pw->pw_gid) || setuid(pw->pw_uid)) {
            Com_Printf(MSG_ERROR,
                       "  - ERROR: can't switch to user \"%s\" privileges (%s)\n",
                       low_priv_user, strerror(errno));
            return false;
        }

        Com_Printf(MSG_NORMAL,
                   "  - Switched to user \"%s\" privileges (UID: %d, GID: %d)\n",
                   low_priv_user, (int)pw->pw_uid, (int)pw->pw_gid);

        Com_Printf(MSG_NORMAL, "\n");
    }

#endif

    return true;
}


/*
====================
Sys_SecureInit

System dependent initializations (called AFTER security initializations)
====================
*/
bool Sys_SecureInit(void) {
#ifndef WIN32

    // Should we run as a daemon?
    if(daemon_state == DAEMON_STATE_REQUEST) {
        if(daemon(0, 1) != 0) {
            Com_Printf(MSG_ERROR, "> ERROR: daemonization failed (%s)\n",
                       strerror(errno));

            daemon_state = DAEMON_STATE_NO;
            return false;
        }

        // Replace the standard input and outputs by /dev/null
        assert(null_device != -1);
        dup2(null_device, STDIN_FILENO);
        dup2(null_device, STDOUT_FILENO);
        dup2(null_device, STDERR_FILENO);

        // We no longer need to keep this file descriptor open
        close(null_device);
        null_device = -1;

        daemon_state = DAEMON_STATE_EFFECTIVE;
    }

#endif

    return true;
}


/*
====================
Sys_SockaddrToString

Returns a pointer to its static character buffer (do NOT free it!)
====================
*/
const char *Sys_SockaddrToString(const struct sockaddr_storage *address,
                                 socklen_t socklen) {
    static char result [NI_MAXHOST + NI_MAXSERV];
    char port_str [NI_MAXSERV];
    int err;
    size_t res_len = 0;

    if(address->ss_family == AF_INET6) {
        result[res_len] = '[';
        res_len += 1;
    }

    err = getnameinfo((struct sockaddr *)address, socklen,
                      result + res_len, sizeof(result) - res_len,
                      port_str, sizeof(port_str),
                      NI_NUMERICHOST | NI_NUMERICSERV);

    if(err == 0) {
        const char *suffix = (address->ss_family == AF_INET6 ? "]" : "");

        res_len = strlen(result);
        snprintf(result + res_len, sizeof(result) - res_len, "%s:%s", suffix,
                 port_str);
    } else {
        Com_Printf(MSG_WARNING,
                   "> WARNING: can't convert address to a printable form: %s\n",
                   gai_strerror(err));
        strncpy(result, "NON-PRINTABLE ADDRESS", sizeof(result) - 1);
    }

    result[sizeof(result) - 1] = '\0';

    return result;
}


/*
====================
Sys_GetSockaddrPort

Get the network port from a sockaddr
====================
*/
unsigned short Sys_GetSockaddrPort(const struct sockaddr_storage
                                   *address) {
    if(address->ss_family == AF_INET6) {
        const struct sockaddr_in6 *addr_v6;

        addr_v6 = (const struct sockaddr_in6 *)address;
        return ntohs(addr_v6->sin6_port);
    } else {
        const struct sockaddr_in *addr_v4;

        assert(address->ss_family == AF_INET);
        addr_v4 = (const struct sockaddr_in *)address;
        return ntohs(addr_v4->sin_port);
    }
}


/*
====================
Sys_GetLastNetError

Get the last network error code
====================
*/
int Sys_GetLastNetError(void) {
#ifdef WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}


/*
====================
Sys_GetLastNetErrorString

Get the last network error string
====================
*/
const char *Sys_GetLastNetErrorString(void) {
    int last_error = Sys_GetLastNetError();

#ifndef WIN32
    return strerror(last_error);
#else

    switch(last_error) {
        case NETERR_AFNOSUPPORT:
            return "Address family not supported by protocol family";

        case NETERR_NOPROTOOPT:
            return "Bad protocol option";

        case NETERR_INTR:
            return "Blocking operation interrupted";

        default: {
            static char last_error_string [32];

            snprintf(last_error_string, sizeof(last_error_string),
                     "Unknown error (%d)", last_error);
            last_error_string[sizeof(last_error_string) - 1] = '\0';

            return last_error_string;
        }
    }

#endif
}
