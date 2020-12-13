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
// File name:   clients.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: Client list and flood protection for owmaster
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include "common.hpp"
#include "system.hpp"
#include "clients.hpp"

// ---------- Private types ---------- //

typedef struct client_s
{
    user_t user;		// WARNING: MUST be the 1st member, for compatibility with the user hash tables
    int count;
    time_t last_time;
} client_t;


// ---------- Private variables ---------- //

static client_t* clients = NULL;

static unsigned int max_nb_clients = DEFAULT_MAX_NB_CLIENTS;
static user_hash_table_t hash_clients;
static size_t cl_hash_size = DEFAULT_CL_HASH_SIZE;

// rolling window for allocation
static int last_used_slot = -1;

// Allow "throttle - 1" queries in a row, then force a throttle to one every "decay time" seconds
static time_t fp_decay_time = DEFAULT_FP_DECAY_TIME;
static int fp_throttle = DEFAULT_FP_THROTTLE;


// ---------- Public variables ---------- //

// Enable/disabled the flood protection mechanism against abusive client requests
qboolean flood_protection = qfalse;


// ---------- Private functions ---------- //

/*
====================
Cl_QueryThrottleDecay

Compute the current throttle value of a client
====================
*/
static int Cl_QueryThrottleDecay( client_t* client )
{
    int count = client->count - ( int )( ( crt_time - client->last_time ) / fp_decay_time );
    if( count < 0 )
        count = 0;
        
    return count;
}


/*
====================
Cl_AddClient

Add a client to an hash table
====================
*/
static qboolean Cl_AddClient( const struct sockaddr_storage* address, socklen_t addrlen )
{
    int first_slot = ( last_used_slot + 1 ) % max_nb_clients;
    int free_slot = first_slot;
    client_t* free_client = NULL;
    
    // look for the next free slot
    do
    {
        int count;
        client_t* client = &clients[ free_slot ];
        
        if( client->user.prev_ptr == NULL )
        {
            // this slot is not in use
            free_client = client;
            break;
        }
        
        // the rolling window lets us retire inactive queries
        assert( client->count != 0 );
        count = Cl_QueryThrottleDecay( client );
        if( count == 0 )
        {
            // this entry is expired, remove from the hash
            Com_UserHashTable_Remove( &client->user );
            free_client = client;
            Com_Printf( MSG_DEBUG, "> Reusing expired client entry %d\n", ( int )( client - clients ) );
            break;
        }
        
        free_slot = ( free_slot + 1 ) % max_nb_clients;
    }
    while( free_slot != first_slot );
    
    if( free_client != NULL )
    {
        int hash;
        
        last_used_slot = free_slot;
        
        memcpy( &free_client->user.address, address, sizeof( free_client->user.address ) );
        free_client->user.addrlen = addrlen;
        free_client->count = 1;
        free_client->last_time = crt_time;
        
        hash = Com_AddressHash( address, cl_hash_size );
        Com_UserHashTable_Add( &hash_clients, &free_client->user, hash );
        
        Com_Printf( MSG_DEBUG,
                    "> New client added: %s\n"
                    "  - index: %u\n"
                    "  - hash: 0x%04X\n",
                    peer_address, free_slot, hash );
        return qtrue;
    }
    else
    {
        Com_Printf( MSG_WARNING,
                    "> WARNING: can't add client %s (client list is full)\n",
                    peer_address );
        return qfalse;
    }
}


// ---------- Public functions ---------- //

/*
====================
Cl_SetHashSize

Set a new hash size value
====================
*/
qboolean Cl_SetHashSize( unsigned int size )
{
    // Too late? Or too big?
    if( clients != NULL || size > MAX_HASH_SIZE )
        return qfalse;
        
    cl_hash_size = size;
    return qtrue;
}


/*
====================
Cl_SetMaxNbClients

Set a new maximum number of clients
====================
*/
qboolean Cl_SetMaxNbClients( unsigned int nb )
{
    // Too late? Or too small?
    if( clients != NULL || nb <= 0 )
        return qfalse;
        
    max_nb_clients = nb;
    return qtrue;
}


/*
====================
Cl_SetFPDecayTime

Set a new decay time for the flood protection
====================
*/
qboolean Cl_SetFPDecayTime( time_t decay )
{
    // Too small?
    if( decay <= 0 )
        return qfalse;
        
    fp_decay_time = decay;
    return qtrue;
}


/*
====================
Cl_SetFPThrottle

Set a new throttle limit for the flood protection
====================
*/
qboolean Cl_SetFPThrottle( unsigned int throttle )
{
    // Too small?
    if( throttle <= 1 )
        return qfalse;
        
    fp_throttle = throttle;
    return qtrue;
}


/*
====================
Cl_Init

Initialize the client list and hash tables
====================
*/
qboolean Cl_Init( void )
{
    // If the flood protection is enabled
    if( flood_protection )
    {
        size_t array_size;
        
        last_used_slot = -1;
        
        // data
        array_size = max_nb_clients * sizeof( clients[0] );
        clients = ( client_t* )malloc( array_size );
        if( !clients )
        {
            Com_Printf( MSG_ERROR,
                        "> ERROR: can't allocate the clients array (%s)\n",
                        strerror( errno ) );
            return qfalse;
        }
        memset( clients, 0, array_size );
        
        Com_Printf( MSG_NORMAL, "> %u client records allocated\n", max_nb_clients );
        
        if( ! Com_UserHashTable_Init( &hash_clients, cl_hash_size, "client" ) )
            return qfalse;
    }
    
    return qtrue;
}


/*
====================
Cl_BlockedByThrottle

Return "true" if a client should be temporary ignored because he has sent too many requests recently
====================
*/
qboolean Cl_BlockedByThrottle( const struct sockaddr_storage* addr, socklen_t addrlen )
{
    unsigned int hash;
    client_t* client;
    qboolean( *IsSameAddress )( const struct sockaddr_storage * addr1, const struct sockaddr_storage * addr2, qboolean * same_public_address );
    
    // If the flood protection is disabled
    if( !flood_protection )
        return qfalse;
        
    if( addr->ss_family == AF_INET6 )
        IsSameAddress = &Com_SameIPv6Addr;
    else
    {
        assert( addr->ss_family == AF_INET );
        IsSameAddress = &Com_SameIPv4Addr;
    }
    
    // look for activity information about this client
    hash = Com_AddressHash( addr, cl_hash_size );
    client = ( client_t* )hash_clients.entries[ hash ];
    while( client != NULL )
    {
        if( addr->ss_family == client->user.address.ss_family )
        {
            qboolean same_public_address = qfalse;
            
            IsSameAddress( addr, &client->user.address, &same_public_address );
            
            // found entry
            if( same_public_address )
            {
                msg_level_t msg_level;
                const char* msg_result;
                
                int new_count = Cl_QueryThrottleDecay( client ) + 1;
                qboolean is_blocked = qboolean( new_count >= fp_throttle );
                if( ! is_blocked )
                {
                    client->count = new_count;
                    client->last_time = crt_time;
                    msg_level = MSG_DEBUG;
                    msg_result = "not throttled";
                    
                }
                else
                {
                    msg_level = MSG_NORMAL;
                    msg_result = "throttled";
                }
                
                Com_Printf( msg_level, "> Client %s: %s (new count == %d)\n", peer_address, msg_result, new_count );
                return is_blocked;
            }
        }
        
        client = ( client_t* )client->user.next;
    }
    
    assert( client == NULL );
    return qboolean( ! Cl_AddClient( addr, addrlen ) );
}
