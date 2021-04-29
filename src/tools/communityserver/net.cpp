////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2010 Aldo Luis Aguirre
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
// File name:   net.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <sys/types.h>
#include <stdio.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2spi.h>
#else
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
#include <ifaddrs.h>
#endif

#include <communityserver.hpp>

/* socket_t g_sockets={0}; */

int init_socket_server(char *s_server, int port) {
    struct sockaddr_in server;
    int sock;
    struct hostent      *h;
    int yes = 1;

    /* Create socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock < 0) {
        printf("Error while trying to open ip %s port %d\n", s_server, port);
        return -1;
    }

    h = gethostbyname(s_server);

    if(h == NULL) {
        printf("Error in hostname!");
#if defined (_WIN32)
        closesocket(sock);
#else
        close(sock);
#endif
        return -1;
    }

    if(h->h_addrtype != AF_INET) {
        printf("gethostbyname: address type was not AF_INET\n");
#if defined (_WIN32)
        closesocket(sock);
#else
        close(sock);
#endif
        return -1;
    }

    server.sin_family = AF_INET;
    //server.sin_addr.s_addr=inet_network(ip);
    //server.sin_addr.s_addr=inet_addr(ip);
    //server.sin_addr.s_addr=*(int *)h->h_addr_list[0];
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes,
                  sizeof(int)) == -1) {
        perror("setsockopt");
        return -1;
    }

    if(bind(sock, (struct sockaddr *)&server, sizeof(server))) {
        printf("Error while binding %s port %d\n", s_server, port);
        return -1;
    }

    listen(sock, 5);

    printf("Initializing socket ip %s port %d\n", s_server, port);


    return sock;
}

int init_socket_client(char *ip, int port) {
    struct sockaddr_in server;
    int sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock < 0) {
        printf("Error opeing socket %s port %d\n", ip, port);
        return -1;
    }

    server.sin_family = AF_INET;
    //server.sin_addr.s_addr=inet_network(ip);
    //server.sin_addr.s_addr=inet_addr(ip);
    server.sin_port = htons(port);

    if(connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Error while trying to connect to server %s port %d\n", ip, port);
        return -1;
    }

    printf("Conected to socket ip %s port %d\n", ip, port);

    return sock;
}

int accept_socket(int sock) {
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    int new_fd;

    //struct sockaddr_in sa;
    char str[INET_ADDRSTRLEN];

    addr_size = sizeof(their_addr);
    new_fd = accept(sock, (struct sockaddr *)&their_addr, &addr_size);

    if(new_fd == -1) {
        printf("Error while accepting connection!\n");
        return -1;
    }

    inet_ntop(AF_INET, &(((struct sockaddr_in *) & (their_addr))->sin_addr),
              str, INET_ADDRSTRLEN);

    if(db_accept_ip(str) == 0) {
        lprintf("This IP is not allowed to connect here: %s\n", str);
#if defined (_WIN32)
        closesocket(new_fd);
#else
        close(new_fd);
#endif
        return -1;
    }

    lprintf("Connection accepted: %s fd: %d\n", str, new_fd);

    return new_fd;
}

int net_get_actual_client_ip(char *ip) {
    int ret;
    struct sockaddr_storage their_addr;
    socklen_t addr_size;

    if(actual_client_socket == -1) {
        ip[0] = '\0';
        return -1;
    }

    addr_size = sizeof(their_addr);

    ret = getpeername(actual_client_socket, (struct sockaddr *)&their_addr,
                      &addr_size);

    if(ret == -1) {
        lprintf("Error retriving ip!\n");
        return -1;
    }

    inet_ntop(AF_INET, &(((struct sockaddr_in *) & (their_addr))->sin_addr),
              ip, INET_ADDRSTRLEN);

    return 0;
}

