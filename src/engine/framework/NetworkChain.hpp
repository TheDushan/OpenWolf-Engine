////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   NetworkChain.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __NETWORKCHAIN_H__
#define __NETWORKCHAIN_H__

/*
packet header
-------------
4   outgoing sequence.  high bit will be set if this is a fragmented message
[2  qport (only for client to server)]
[2  fragment start uchar8]
[2  fragment length. if < FRAGMENT_SIZE, this is the last fragment]

if the sequence number is -1, the packet should be handled as an out-of-band
message instead of as part of a netcon.

All fragments will have the same sequence numbers.

The qport field is a workaround for bad address translating routers that
sometimes remap the client's source port on a packet during gameplay.

If the base part of the net address matches and the qport matches, then the
channel matches even if the IP port differs.  The IP port should be updated
to the new value before sending out any replies.
*/


#define MAX_PACKETLEN           1400        // max size of a network packet

#define FRAGMENT_SIZE           (MAX_PACKETLEN - 100)
#define PACKET_HEADER           10          // two ints and a short

#define FRAGMENT_BIT    (1U<<31)

// there needs to be enough loopback messages to hold a complete
// gamestate of maximum size
#define MAX_LOOPBACK    16

typedef struct {
    uchar8  data[MAX_PACKETLEN];
    sint        datalen;
} loopmsg_t;

typedef struct {
    loopmsg_t   msgs[MAX_LOOPBACK];
    sint            get, send;
} loopback_t;

typedef struct packetQueue_s {
    struct packetQueue_s *next;
    sint length;
    uchar8 *data;
    netadr_t to;
    sint release;
} packetQueue_t;

static convar_t *showpackets;
static convar_t *showdrop;
static convar_t *qport;

//
// idNetworChainSystemLocal
//
class idNetworkChainSystemLocal : public idNetworkChainSystem {
public:
    idNetworkChainSystemLocal();
    ~idNetworkChainSystemLocal();

    virtual void Init(sint port);
    virtual void Setup(netsrc_t sock, netchan_t *chan, netadr_t adr,
                       sint qport);
    virtual void TransmitNextFragment(netchan_t *chan);
    virtual void Transmit(netchan_t *chan, sint length, const uchar8 *data);
    virtual bool Process(netchan_t *chan, msg_t *msg);
    virtual bool GetLoopPacket(netsrc_t sock, netadr_t *net_from,
                               msg_t *net_message);
    virtual void FlushPacketQueue(void);
    virtual void SendPacket(netsrc_t sock, sint length, const void *data,
                            netadr_t to);
    virtual void OutOfBandPrint(netsrc_t sock, netadr_t adr, pointer format,
                                ...);
    virtual void OutOfBandData(netsrc_t sock, netadr_t adr, uchar8 *format,
                               sint len);
    virtual sint StringToAdr(pointer s, netadr_t *a, netadrtype_t family);

    static void SendLoopPacket(netsrc_t sock, sint length, const void *data,
                               netadr_t to);
    static void QueuePacket(sint length, const void *data, netadr_t to,
                            sint offset);
    static void ScramblePacket(msg_t *buf);
    static void UnScramblePacket(msg_t *buf);

    static const sint SCRAMBLE_START = 6;
};

extern idNetworkChainSystemLocal networkChainSystemLocal;

#endif //!__NETWORK_H__