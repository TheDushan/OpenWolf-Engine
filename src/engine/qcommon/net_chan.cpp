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
// File name:   net_chan.cpp
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

/*

packet header
-------------
4	outgoing sequence.  high bit will be set if this is a fragmented message
[2	qport (only for client to server)]
[2	fragment start U8]
[2	fragment length. if < FRAGMENT_SIZE, this is the last fragment]

if the sequence number is -1, the packet should be handled as an out-of-band
message instead of as part of a netcon.

All fragments will have the same sequence numbers.

The qport field is a workaround for bad address translating routers that
sometimes remap the client's source port on a packet during gameplay.

If the base part of the net address matches and the qport matches, then the
channel matches even if the IP port differs.  The IP port should be updated
to the new value before sending out any replies.

*/


#define	MAX_PACKETLEN			1400		// max size of a network packet

#define	FRAGMENT_SIZE			(MAX_PACKETLEN - 100)
#define	PACKET_HEADER			10			// two ints and a short

#define	FRAGMENT_BIT	(1U<<31)

convar_t*		showpackets;
convar_t*		showdrop;
convar_t*		qport;

static UTF8* netsrcString[2] =
{
    "client",
    "server"
};

/*
===============
Netchan_Init

===============
*/
void Netchan_Init( S32 port )
{
    port &= 0xffff;
    showpackets = cvarSystem->Get( "showpackets", "0", CVAR_TEMP, "Toggles the running display of all packets sent and received. 0=disables;1=enables. " );
    showdrop = cvarSystem->Get( "showdrop", "0", CVAR_TEMP, "When enabled, reports dropped packets should they occur. 0=disables;1=enables. " );
    qport = cvarSystem->Get( "net_qport", va( "%i", port ), CVAR_INIT, "Define the port to connect with, useful for bypassing annoying routers & firewalls" );
}

/*
==============
Netchan_Setup

called to open a channel to a remote system
==============
*/
void Netchan_Setup( netsrc_t sock, netchan_t* chan, netadr_t adr, S32 qport )
{
    ::memset( chan, 0, sizeof( *chan ) );
    
    chan->sock = sock;
    chan->remoteAddress = adr;
    chan->qport = qport;
    chan->incomingSequence = 0;
    chan->outgoingSequence = 1;
}

// TTimo: unused, commenting out to make gcc happy
#if 0
/*
==============
Netchan_ScramblePacket

A probably futile attempt to make proxy hacking somewhat
more difficult.
==============
*/
#define	SCRAMBLE_START	6
static void Netchan_ScramblePacket( msg_t* buf )
{
    U32	seed;
    S32			i, j, c, mask, temp;
    S32			seq[MAX_PACKETLEN];
    
    seed = ( LittleLong( *( U32* )buf->data ) * 3 ) ^ ( buf->cursize * 123 );
    c = buf->cursize;
    if( c <= SCRAMBLE_START )
    {
        return;
    }
    if( c > MAX_PACKETLEN )
    {
        Com_Error( ERR_DROP, "MAX_PACKETLEN" );
    }
    
    // generate a sequence of "random" numbers
    for( i = 0 ; i < c ; i++ )
    {
        seed = ( 119 * seed + 1 );
        seq[i] = seed;
    }
    
    // transpose each character
    for( mask = 1 ; mask < c - SCRAMBLE_START ; mask = ( mask << 1 ) + 1 )
    {
    }
    mask >>= 1;
    for( i = SCRAMBLE_START ; i < c ; i++ )
    {
        j = SCRAMBLE_START + ( seq[i] & mask );
        temp = buf->data[j];
        buf->data[j] = buf->data[i];
        buf->data[i] = temp;
    }
    
    // U8 xor the data after the header
    for( i = SCRAMBLE_START ; i < c ; i++ )
    {
        buf->data[i] ^= seq[i];
    }
}

static void Netchan_UnScramblePacket( msg_t* buf )
{
    U32	seed;
    S32			i, j, c, mask, temp;
    S32			seq[MAX_PACKETLEN];
    
    seed = ( LittleLong( *( U32* )buf->data ) * 3 ) ^ ( buf->cursize * 123 );
    c = buf->cursize;
    if( c <= SCRAMBLE_START )
    {
        return;
    }
    if( c > MAX_PACKETLEN )
    {
        Com_Error( ERR_DROP, "MAX_PACKETLEN" );
    }
    
    // generate a sequence of "random" numbers
    for( i = 0 ; i < c ; i++ )
    {
        seed = ( 119 * seed + 1 );
        seq[i] = seed;
    }
    
    // U8 xor the data after the header
    for( i = SCRAMBLE_START ; i < c ; i++ )
    {
        buf->data[i] ^= seq[i];
    }
    
    // transpose each character in reverse order
    for( mask = 1 ; mask < c - SCRAMBLE_START ; mask = ( mask << 1 ) + 1 )
    {
    }
    mask >>= 1;
    for( i = c - 1 ; i >= SCRAMBLE_START ; i-- )
    {
        j = SCRAMBLE_START + ( seq[i] & mask );
        temp = buf->data[j];
        buf->data[j] = buf->data[i];
        buf->data[i] = temp;
    }
}
#endif

/*
=================
Netchan_TransmitNextFragment

Send one fragment of the current message
=================
*/
void Netchan_TransmitNextFragment( netchan_t* chan )
{
    msg_t		send;
    U8		send_buf[MAX_PACKETLEN];
    S32			fragmentLength;
    
    // write the packet header
    MSG_InitOOB( &send, send_buf, sizeof( send_buf ) );				// <-- only do the oob here
    
    MSG_WriteLong( &send, chan->outgoingSequence | FRAGMENT_BIT );
    
    // send the qport if we are a client
    if( chan->sock == NS_CLIENT )
    {
        MSG_WriteShort( &send, qport->integer );
    }
    
    // copy the reliable message to the packet first
    fragmentLength = FRAGMENT_SIZE;
    if( chan->unsentFragmentStart  + fragmentLength > chan->unsentLength )
    {
        fragmentLength = chan->unsentLength - chan->unsentFragmentStart;
    }
    
    MSG_WriteShort( &send, chan->unsentFragmentStart );
    MSG_WriteShort( &send, fragmentLength );
    MSG_WriteData( &send, chan->unsentBuffer + chan->unsentFragmentStart, fragmentLength );
    
    // send the datagram
    NET_SendPacket( chan->sock, send.cursize, send.data, chan->remoteAddress );
    
    // Store send time and size of this packet for rate control
    chan->lastSentTime = idsystem->Milliseconds();
    chan->lastSentSize = send.cursize;
    
    if( showpackets->integer )
    {
        Com_Printf( "%s send %4i : s=%i fragment=%i,%i\n"
                    , netsrcString[ chan->sock ]
                    , send.cursize
                    , chan->outgoingSequence
                    , chan->unsentFragmentStart, fragmentLength );
    }
    
    chan->unsentFragmentStart += fragmentLength;
    
    // this exit condition is a little tricky, because a packet
    // that is exactly the fragment length still needs to send
    // a second packet of zero length so that the other side
    // can tell there aren't more to follow
    if( chan->unsentFragmentStart == chan->unsentLength && fragmentLength != FRAGMENT_SIZE )
    {
        chan->outgoingSequence++;
        chan->unsentFragments = false;
    }
}


/*
===============
Netchan_Transmit

Sends a message to a connection, fragmenting if necessary
A 0 length will still generate a packet.
================
*/
void Netchan_Transmit( netchan_t* chan, S32 length, const U8* data )
{
    msg_t		send;
    U8		send_buf[MAX_PACKETLEN];
    
    if( length > MAX_MSGLEN )
    {
        Com_Error( ERR_DROP, "Netchan_Transmit: length = %i", length );
    }
    chan->unsentFragmentStart = 0;
    
    // fragment large reliable messages
    if( length >= FRAGMENT_SIZE )
    {
        chan->unsentFragments = true;
        chan->unsentLength = length;
        ::memcpy( chan->unsentBuffer, data, length );
        
        // only send the first fragment now
        Netchan_TransmitNextFragment( chan );
        
        return;
    }
    
    // write the packet header
    MSG_InitOOB( &send, send_buf, sizeof( send_buf ) );
    
    MSG_WriteLong( &send, chan->outgoingSequence );
    chan->outgoingSequence++;
    
    // send the qport if we are a client
    if( chan->sock == NS_CLIENT )
    {
        MSG_WriteShort( &send, qport->integer );
    }
    
    MSG_WriteData( &send, data, length );
    
    // send the datagram
    NET_SendPacket( chan->sock, send.cursize, send.data, chan->remoteAddress );
    
    // Store send time and size of this packet for rate control
    chan->lastSentTime = idsystem->Milliseconds();
    chan->lastSentSize = send.cursize;
    
    
    if( showpackets->integer )
    {
        Com_Printf( "%s send %4i : s=%i ack=%i\n"
                    , netsrcString[ chan->sock ]
                    , send.cursize
                    , chan->outgoingSequence - 1
                    , chan->incomingSequence );
    }
}

/*
=================
Netchan_Process

Returns false if the message should not be processed due to being
out of order or a fragment.

Msg must be large enough to hold MAX_MSGLEN, because if this is the
final fragment of a multi-part message, the entire thing will be
copied out.
=================
*/
bool Netchan_Process( netchan_t* chan, msg_t* msg )
{
    S32			sequence;
    S32			qport;
    S32			fragmentStart, fragmentLength;
    bool	fragmented;
    
    // XOR unscramble all data in the packet after the header
//	Netchan_UnScramblePacket( msg );

    // get sequence numbers
    MSG_BeginReadingOOB( msg );
    sequence = MSG_ReadLong( msg );
    
    // check for fragment information
    if( sequence & FRAGMENT_BIT )
    {
        sequence &= ~FRAGMENT_BIT;
        fragmented = true;
    }
    else
    {
        fragmented = false;
    }
    
    // read the qport if we are a server
    if( chan->sock == NS_SERVER )
    {
        qport = MSG_ReadShort( msg );
    }
    
    // read the fragment information
    if( fragmented )
    {
        fragmentStart = MSG_ReadShort( msg );
        fragmentLength = MSG_ReadShort( msg );
    }
    else
    {
        fragmentStart = 0;		// stop warning message
        fragmentLength = 0;
    }
    
    if( showpackets->integer )
    {
        if( fragmented )
        {
            Com_Printf( "%s recv %4i : s=%i fragment=%i,%i\n"
                        , netsrcString[ chan->sock ]
                        , msg->cursize
                        , sequence
                        , fragmentStart, fragmentLength );
        }
        else
        {
            Com_Printf( "%s recv %4i : s=%i\n"
                        , netsrcString[ chan->sock ]
                        , msg->cursize
                        , sequence );
        }
    }
    
    //
    // discard out of order or duplicated packets
    //
    if( sequence <= chan->incomingSequence )
    {
        if( showdrop->integer || showpackets->integer )
        {
            Com_Printf( "%s:Out of order packet %i at %i\n"
                        , NET_AdrToString( chan->remoteAddress )
                        ,  sequence
                        , chan->incomingSequence );
        }
        return false;
    }
    
    //
    // dropped packets don't keep the message from being used
    //
    chan->dropped = sequence - ( chan->incomingSequence + 1 );
    if( chan->dropped > 0 )
    {
        if( showdrop->integer || showpackets->integer )
        {
            Com_Printf( "%s:Dropped %i packets at %i\n"
                        , NET_AdrToString( chan->remoteAddress )
                        , chan->dropped
                        , sequence );
        }
    }
    
    
    //
    // if this is the final framgent of a reliable message,
    // bump incoming_reliable_sequence
    //
    if( fragmented )
    {
        // TTimo
        // make sure we add the fragments in correct order
        // either a packet was dropped, or we received this one too soon
        // we don't reconstruct the fragments. we will wait till this fragment gets to us again
        // (NOTE: we could probably try to rebuild by out of order chunks if needed)
        if( sequence != chan->fragmentSequence )
        {
            chan->fragmentSequence = sequence;
            chan->fragmentLength = 0;
        }
        
        // if we missed a fragment, dump the message
        if( fragmentStart != chan->fragmentLength )
        {
            if( showdrop->integer || showpackets->integer )
            {
                Com_Printf( "%s:Dropped a message fragment\n"
                            , NET_AdrToString( chan->remoteAddress ) );
            }
            // we can still keep the part that we have so far,
            // so we don't need to clear chan->fragmentLength
            return false;
        }
        
        // copy the fragment to the fragment buffer
        if( fragmentLength < 0 || msg->readcount + fragmentLength > msg->cursize ||
                chan->fragmentLength + fragmentLength > sizeof( chan->fragmentBuffer ) )
        {
            if( showdrop->integer || showpackets->integer )
            {
                Com_Printf( "%s:illegal fragment length\n"
                            , NET_AdrToString( chan->remoteAddress ) );
            }
            return false;
        }
        
        ::memcpy( chan->fragmentBuffer + chan->fragmentLength,
                  msg->data + msg->readcount, fragmentLength );
                  
        chan->fragmentLength += fragmentLength;
        
        // if this wasn't the last fragment, don't process anything
        if( fragmentLength == FRAGMENT_SIZE )
        {
            return false;
        }
        
        if( chan->fragmentLength > msg->maxsize )
        {
            Com_Printf( "%s:fragmentLength %i > msg->maxsize\n"
                        , NET_AdrToString( chan->remoteAddress ),
                        chan->fragmentLength );
            return false;
        }
        
        // copy the full message over the partial fragment
        
        // make sure the sequence number is still there
        *( S32* )msg->data = LittleLong( sequence );
        
        ::memcpy( msg->data + 4, chan->fragmentBuffer, chan->fragmentLength );
        msg->cursize = chan->fragmentLength + 4;
        chan->fragmentLength = 0;
        msg->readcount = 4;	// past the sequence number
        msg->bit = 32;	// past the sequence number
        
        // TTimo
        // clients were not acking fragmented messages
        chan->incomingSequence = sequence;
        
        return true;
    }
    
    //
    // the message can now be read from the current message pointer
    //
    chan->incomingSequence = sequence;
    
    return true;
}


//==============================================================================


/*
=============================================================================

LOOPBACK BUFFERS FOR LOCAL PLAYER

=============================================================================
*/

// there needs to be enough loopback messages to hold a complete
// gamestate of maximum size
#define	MAX_LOOPBACK	16

typedef struct
{
    U8	data[MAX_PACKETLEN];
    S32		datalen;
} loopmsg_t;

typedef struct
{
    loopmsg_t	msgs[MAX_LOOPBACK];
    S32			get, send;
} loopback_t;

loopback_t	loopbacks[2];


bool	NET_GetLoopPacket( netsrc_t sock, netadr_t* net_from, msg_t* net_message )
{
    S32		i;
    loopback_t*	loop;
    
    loop = &loopbacks[sock];
    
    if( loop->send - loop->get > MAX_LOOPBACK )
        loop->get = loop->send - MAX_LOOPBACK;
        
    if( loop->get >= loop->send )
        return false;
        
    i = loop->get & ( MAX_LOOPBACK - 1 );
    loop->get++;
    
    ::memcpy( net_message->data, loop->msgs[i].data, loop->msgs[i].datalen );
    net_message->cursize = loop->msgs[i].datalen;
    ::memset( net_from, 0, sizeof( *net_from ) );
    net_from->type = NA_LOOPBACK;
    return true;
    
}


void NET_SendLoopPacket( netsrc_t sock, S32 length, const void* data, netadr_t to )
{
    S32		i;
    loopback_t*	loop;
    
    loop = &loopbacks[sock ^ 1];
    
    i = loop->send & ( MAX_LOOPBACK - 1 );
    loop->send++;
    
    ::memcpy( loop->msgs[i].data, data, length );
    loop->msgs[i].datalen = length;
}

//=============================================================================

typedef struct packetQueue_s
{
    struct packetQueue_s* next;
    S32 length;
    U8* data;
    netadr_t to;
    S32 release;
} packetQueue_t;

packetQueue_t* packetQueue = nullptr;

static void NET_QueuePacket( S32 length, const void* data, netadr_t to,
                             S32 offset )
{
    packetQueue_t* _new, *next = packetQueue;
    
    if( offset > 999 )
        offset = 999;
        
    _new = ( packetQueue_t* )S_Malloc( sizeof( packetQueue_t ) );
    _new->data = ( U8* )S_Malloc( length );
    ::memcpy( _new->data, data, length );
    _new->length = length;
    _new->to = to;
    _new->release = idsystem->Milliseconds() + ( S32 )( ( F32 )offset / com_timescale->value );
    _new->next = nullptr;
    
    if( !packetQueue )
    {
        packetQueue = _new;
        return;
    }
    while( next )
    {
        if( !next->next )
        {
            next->next = _new;
            return;
        }
        next = next->next;
    }
}

void NET_FlushPacketQueue( void )
{
    packetQueue_t* last;
    S32 now;
    
    while( packetQueue )
    {
        now = idsystem->Milliseconds();
        if( packetQueue->release >= now )
            break;
        Net_SendPacket( packetQueue->length, packetQueue->data, packetQueue->to );
        last = packetQueue;
        packetQueue = packetQueue->next;
        Z_Free( last->data );
        Z_Free( last );
    }
}

void NET_SendPacket( netsrc_t sock, S32 length, const void* data, netadr_t to )
{

    // sequenced packets are shown in netchan, so just show oob
    if( showpackets->integer && *( S32* )data == -1 )
    {
        Com_Printf( "send packet %4i\n", length );
    }
    
    if( to.type == NA_LOOPBACK )
    {
        NET_SendLoopPacket( sock, length, data, to );
        return;
    }
    if( to.type == NA_BOT )
    {
        return;
    }
    if( to.type == NA_BAD )
    {
        return;
    }
    
    if( sock == NS_CLIENT && cl_packetdelay->integer > 0 )
    {
        NET_QueuePacket( length, data, to, cl_packetdelay->integer );
    }
    else if( sock == NS_SERVER && sv_packetdelay->integer > 0 )
    {
        NET_QueuePacket( length, data, to, sv_packetdelay->integer );
    }
    else
    {
        Net_SendPacket( length, data, to );
    }
}

/*
===============
NET_OutOfBandPrint

Sends a text message in an out-of-band datagram
================
*/
void NET_OutOfBandPrint( netsrc_t sock, netadr_t adr, StringEntry format, ... )
{
    va_list		argptr;
    UTF8		string[MAX_MSGLEN];
    
    
    // set the header
    string[0] = -1;
    string[1] = -1;
    string[2] = -1;
    string[3] = -1;
    
    va_start( argptr, format );
    Q_vsnprintf( string + 4, sizeof( string ) - 4, format, argptr );
    va_end( argptr );
    
    // send the datagram
    NET_SendPacket( sock, strlen( string ), string, adr );
}

/*
===============
NET_OutOfBandPrint

Sends a data message in an out-of-band datagram (only used for "connect")
================
*/
void NET_OutOfBandData( netsrc_t sock, netadr_t adr, U8* format, S32 len )
{
    U8		string[MAX_MSGLEN * 2];
    S32			i;
    msg_t		mbuf;
    
    MSG_InitOOB( &mbuf, string, sizeof( string ) );
    
    // set the header
    string[0] = 0xff;
    string[1] = 0xff;
    string[2] = 0xff;
    string[3] = 0xff;
    
    for( i = 0; i < len; i++ )
    {
        string[i + 4] = format[i];
    }
    
    mbuf.data = string;
    mbuf.cursize = len + 4;
    idHuffmanSystemLocal::DynCompress( &mbuf, 12 );
    // send the datagram
    NET_SendPacket( sock, mbuf.cursize, mbuf.data, adr );
}

/*
=============
NET_StringToAdr

Traps "localhost" for loopback, passes everything else to system
return 0 on address not found, 1 on address found with port, 2 on address found without port.
=============
*/
S32 NET_StringToAdr( StringEntry s, netadr_t* a, netadrtype_t family )
{
    UTF8	base[MAX_STRING_CHARS], *search;
    UTF8*	port = nullptr;
    
    if( !strcmp( s, "localhost" ) )
    {
        ::memset( a, 0, sizeof( *a ) );
        a->type = NA_LOOPBACK;
// as NA_LOOPBACK doesn't require ports report port was given.
        return 1;
    }
    
    Q_strncpyz( base, s, sizeof( base ) );
    
    if( *base == '[' || Q_CountChar( base, ':' ) > 1 )
    {
        // This is an ipv6 address, handle it specially.
        search = strchr( base, ']' );
        if( search )
        {
            *search = '\0';
            search++;
            
            if( *search == ':' )
                port = search + 1;
        }
        
        if( *base == '[' )
            search = base + 1;
        else
            search = base;
    }
    else
    {
        // look for a port number
        port = strchr( base, ':' );
        
        if( port )
        {
            *port = '\0';
            port++;
        }
        
        search = base;
    }
    
    if( !Net_StringToAdr( search, a, family ) )
    {
        a->type = NA_BAD;
        return 0;
    }
    
    if( port )
    {
        a->port = BigShort( ( S16 ) atoi( port ) );
        return 1;
    }
    else
    {
        a->port = BigShort( PORT_SERVER );
        return 2;
    }
}
