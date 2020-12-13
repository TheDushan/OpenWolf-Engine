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
// File name:   communityserver.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <sys/types.h>

#if defined (__LINUX__)
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

#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <communityserver.hpp>

#define MAX_BUFFER 24576

typedef struct
{
    int socket;
    char* buffer;
} connection_t;

//int *slist = NULL;
//char **blist = NULL; // buffer list, the same as slist
connection_t* connections = NULL;
int nlist = 0;


int actual_client_socket = -1;
int sock_server = 0;

void daemon_loop( int dport );
int new_connection( int sock_server );
int find_connection( fd_set* reqfd );
int process_recv( int socket, char* buffer, int bsize );
int open_logs();

FILE* flog = NULL;


#ifdef _WIN32
#include <string.h>

int     opterr = 1,             /* if error message should be printed */
        optind = 1,             /* index into parent argv vector */
        optopt,                 /* character checked for validity */
        optreset;               /* reset getopt */
char* optarg;                /* argument associated with option */

#define BADCH   (int)'?'
#define BADARG  (int)':'
#define EMSG    ""

/*
* getopt --
*      Parse argc/argv argument vector.
*/
int getopt( int nargc, char* const nargv[], const char* ostr )
{
    static char* place = EMSG;              /* option letter processing */
    const char* oli;                        /* option letter list index */
    
    if( optreset || !*place )               /* update scanning pointer */
    {
        optreset = 0;
        if( optind >= nargc || *( place = nargv[optind] ) != '-' )
        {
            place = EMSG;
            return ( -1 );
        }
        if( place[1] && *++place == '-' )       /* found "--" */
        {
            ++optind;
            place = EMSG;
            return ( -1 );
        }
    }                                       /* option letter okay? */
    if( ( optopt = ( int ) * place++ ) == ( int )':' ||
            !( oli = strchr( ostr, optopt ) ) )
    {
        /*
        * if the user didn't specify '-' as an option,
        * assume it means -1.
        */
        if( optopt == ( int )'-' )
            return ( -1 );
        if( !*place )
            ++optind;
        if( opterr && *ostr != ':' )
            ( void )printf( "illegal option -- %c\n", optopt );
        return ( BADCH );
    }
    if( *++oli != ':' )                     /* don't need argument */
    {
        optarg = NULL;
        if( !*place )
            ++optind;
    }
    else                                    /* need an argument */
    {
        if( *place )                    /* no white space */
            optarg = place;
        else if( nargc <= ++optind )    /* no arg */
        {
            place = EMSG;
            if( *ostr == ':' )
                return ( BADARG );
            if( opterr )
                ( void )printf( "option requires an argument -- %c\n", optopt );
            return ( BADCH );
        }
        else                            /* white space */
            optarg = nargv[optind];
        place = EMSG;
        ++optind;
    }
    return ( optopt );                      /* dump back option letter */
}

#endif

int main( int argc, char* argv[] )
{
    char* user = NULL;
    char* password = NULL;
    char* host = NULL;
    char* database = NULL;
    MYSQL* connection = NULL;
    int port = 27999;
    int c;
    
    if( open_logs() == -1 )
    {
        printf( "Error I cannot open %s for log\n", FILE_TO_LOG );
        return -1;
    }
    
    lprintf( "********************************************\n" );
    lprintf( "*             COMMUNITY SERVER             *\n" );
    lprintf( "********************************************\n" );
    
    while( ( c = getopt( argc, argv, "u:p:h:d:t:" ) ) != -1 )
    {
        switch( c )
        {
            case 'u':
                user = optarg;
                break;
            case 'p':
                password = optarg;
                break;
            case 'h':
                host = optarg;
                break;
            case 'd':
                database = optarg;
                break;
            case 't':
                port = atoi( optarg );
                break;
            default:
                printf( "Argument %d not recognized\n", c );
                return -1;
        }
    }
    if( user == NULL || password == NULL || host == NULL || database == NULL )
    {
        printf( "%s %s %s %s\n", user, password, host, database );
        printf( "Arguments:\n-u user\n-p password\n-h host\n-d database\n-t daemon port\n" );
        printf( "All this arguments are needed\n" );
        return -1;
    }
    
    connection = connect_database( host, user, password, database );
    
    if( connection == NULL )
    {
        printf( "Error connection mysql!\n" );
        return -1;
    }
    
    daemon_loop( port );
    
    return 0;
}

void daemon_loop( int dport )
{
    fd_set rfds, rfdsaux;
    int i;
    int retval;
    char buffer[1024 * 64] = {0};
    int actual_socket;
    int max_n;
    
    sock_server = init_socket_server( "localhost", dport );
    if( sock_server == -1 ) exit( -1 );
    
    while( 1 )
    {
        actual_client_socket = -1;
        
        FD_ZERO( &rfds );
        
        /* Proxy input */
        FD_SET( sock_server, &rfds );
        max_n = sock_server;
        
        /* All attached devices */
        for( i = 0; i < nlist; i++ )
        {
            FD_SET( connections[i].socket, &rfds );
            if( max_n < connections[i].socket ) max_n = connections[i].socket;
        }
        
        rfdsaux = rfds;
        /* Wait up */
        //tv.tv_sec = 0;
        //tv.tv_usec = 500;
        
        memset( buffer, 0, sizeof( buffer ) );
        
        lprintf( "Waiting for activity...\n" );
        
        retval = select( max_n + 1, &rfdsaux, NULL, NULL, NULL );
        
        if( retval > 0 )
        {
        
            /* Default proxy socket */
            if( FD_ISSET( sock_server, &rfdsaux ) )
            {
                new_connection( sock_server );
            }
            else
            {
                actual_socket = find_connection( &rfdsaux );
                if( actual_socket != -1 )
                {
                    if( process_recv( actual_socket, buffer, sizeof( buffer ) ) >= 0 )
                    {
                        //parse_message(buffer);
                        lprintf( "RECV: |%s|\n", buffer );
                        parse_messageJSON( buffer );
                    }
                    
                }
                
            }
        }
        
#if defined (_WIN32)
        closesocket( sock_server );
#else
        close( sock_server );
#endif
    }
}

int new_connection( int sock_server )
{
    int new_socket;
    
    lprintf( "Accepting new connection\n" );
    
    new_socket = accept_socket( sock_server );
    
    if( new_socket != -1 )
    {
        nlist++;
        //slist = realloc(slist, nlist * sizeof(int));
        //slist[nlist-1] = new_socket;
        connections = ( connection_t* )realloc( connections, nlist * sizeof( connection_t ) );
        connections[nlist - 1].socket = new_socket;
        connections[nlist - 1].buffer = ( char* )malloc( MAX_BUFFER );
        memset( connections[nlist - 1].buffer, 0, MAX_BUFFER );
    }
    
    return new_socket;
}

int find_connection( fd_set* reqfd )
{
    int i;
    
    lprintf( "Finding data connection...\n" );
    for( i = 0; i < nlist && !FD_ISSET( connections[i].socket, reqfd ); i++ );
    
    if( i == nlist )
    {
        lprintf( "Error! Cannot find connection!\n" );
        return -1;
    }
    
    actual_client_socket = connections[i].socket;
    
    return connections[i].socket;
    
}

int process_recv( int socket, char* buffer, int bsize )
{
    connection_t* caux = NULL;
    int i;
    int j;
    int ret = 0;
    int sindex;
    int offset = 0;
    
    for( sindex = 0; sindex < nlist && connections[sindex].socket != socket; sindex++ );
    if( nlist == sindex )
    {
        lprintf( "ERROR! Cannot find socket %d in socket list!\n", socket );
        return 0;
    }
    
    if( strlen( connections[sindex].buffer ) != 0 )
    {
        strcat( buffer, connections[sindex].buffer );
        offset = strlen( buffer );
    }
    
    lprintf( "Reciving from %d\n", socket );
    //if((ret = recv(socket, buffer, bsize, MSG_WAITALL)) <= 0)
    if( ( ret = recv( socket, &( buffer[offset] ), bsize - 1, 0 ) ) <= 0 )
    {
        lprintf( "Connection closed!\n" );
        caux = ( connection_t* )malloc( ( nlist - 1 ) * sizeof( connection_t ) );
        for( i = 0, j = 0; i < nlist; i++ )
        {
            if( connections[i].socket == socket )
            {
                free( connections[i].buffer );
            }
            else
            {
                caux[j].socket = connections[i].socket;
                caux[j].buffer = connections[i].buffer;
                j++;
            }
        }
        nlist--;
        if( nlist == 0 ) caux = NULL;
        free( connections );
        connections = caux;
#if defined (_WIN32)
        closesocket( socket );
#else
        close( socket );
#endif}
        ret = -1;
        actual_client_socket = -1;
        
        return ret;
    }
    
    ret += offset;
    
    // convert to readable string
    buffer[ret] = '\0';
    
    for( i = 0; i < ret; i++ )( buffer[i] == '\0' ? buffer[i] = '\n' : i );
    
    if( ret > 2 && buffer[ret - 1] != '\n' )
    {
    
        for( i = ret; i >= 0 && buffer[i] != '\n'; i-- );
        if( i < 0 )
        {
            lprintf( "ERROR! Cannot find start of string! Buffering again...\n" );
            strcpy( connections[sindex].buffer, buffer );
            return ret;
        }
        if( i != 0 ) buffer[i] = '\0';
        strcpy( connections[sindex].buffer, &( buffer[i + 1] ) );
    }
    else
    {
        memset( connections[sindex].buffer, 0, MAX_BUFFER );
    }
    
    return ret;
}

int open_logs( void )
{
    flog = fopen( FILE_TO_LOG, "at" );
    if( flog == NULL ) return -1;
    return 0;
}

void lprintf( const char* fmt, ... )
{
    va_list args;
    va_start( args, fmt );
    vprintf( fmt, args );
    va_end( args );
}