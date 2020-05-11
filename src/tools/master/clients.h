/*
	clients.h

	Client list and flood protection for dpmaster

	Copyright (C) 2010  Timothee Besset
	Copyright (C) 2010  Mathieu Olivier

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


#ifndef _CLIENTS_H_
#define _CLIENTS_H_


// ---------- Constants ---------- //

// Maximum number of clients in all lists by default
#define DEFAULT_MAX_NB_CLIENTS 512

// Address hash size in bits for clients (between 0 and MAX_HASH_SIZE)
#define DEFAULT_CL_HASH_SIZE 7

// Allow "throttle - 1" queries in a row, then force a throttle to one every "decay time" seconds
#define DEFAULT_FP_DECAY_TIME	3
#define DEFAULT_FP_THROTTLE		5


// ---------- Public variables ---------- //

// Enable/disabled the flood protection mechanism against abusive client requests
extern qboolean flood_protection;


// ---------- Public functions ---------- //

// Will simply return "false" if called after Sv_Init
qboolean Cl_SetHashSize( unsigned int size );
qboolean Cl_SetMaxNbClients( unsigned int nb );
qboolean Cl_SetFPDecayTime( time_t decay );
qboolean Cl_SetFPThrottle( unsigned int throttle );

// Initialize the client list and hash tables
qboolean Cl_Init( void );

// Return "true" if a client should be temporary ignored because he has sent too many requests recently
qboolean Cl_BlockedByThrottle( const struct sockaddr_storage* addr, socklen_t addrlen );


#endif  // #ifndef _CLIENTS_H_
