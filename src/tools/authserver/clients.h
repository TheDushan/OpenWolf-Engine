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

#define DEFAULT_FP_DECAY_TIME	30
#define DEFAULT_FP_THROTTLE		20

// ---------- Public variables ---------- //

// Enable/disabled the flood protection mechanism against abusive client requests
extern bool flood_protection;


// ---------- Public functions ---------- //

// Will simply return "false" if called after Sv_Init
bool Cl_SetHashSize( unsigned int size );
bool Cl_SetMaxNbClients( unsigned int nb );
bool Cl_SetFPDecayTime( time_t decay );
bool Cl_SetFPThrottle( unsigned int throttle );

// Initialize the client list and hash tables
bool Cl_Init( void );

// Return "true" if a client should be temporary ignored because he has sent too many requests recently
bool Cl_BlockedByThrottle( const struct sockaddr_storage* addr, socklen_t addrlen, int counter );
bool Cl_Counter( const struct sockaddr_storage* addr, socklen_t addrlen, int counter );

#endif  // #ifndef _CLIENTS_H_
