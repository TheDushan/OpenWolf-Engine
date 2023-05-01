////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2019 - 2023 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   NetworkChain_api.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __NETWORKCHAIN_API_HPP__
#define __NETWORKCHAIN_API_HPP__

//
// idNetworkChainSystem
//
class idNetworkChainSystem {
public:
    virtual void Init(sint port) = 0;
    virtual void Setup(netsrc_t sock, netchan_t *chan, netadr_t adr,
                       sint qport) = 0;
    virtual void TransmitNextFragment(netchan_t *chan) = 0;
    virtual void Transmit(netchan_t *chan, sint length,
                          const uchar8 *data) = 0;
    virtual bool Process(netchan_t *chan, msg_t *msg) = 0;
    virtual bool GetLoopPacket(netsrc_t sock, netadr_t *net_from,
                               msg_t *net_message) = 0;
    virtual void FlushPacketQueue(void) = 0;
    virtual void SendPacket(netsrc_t sock, sint length, const void *data,
                            netadr_t to) = 0;
    virtual void OutOfBandPrint(netsrc_t sock, netadr_t adr, pointer format,
                                ...) = 0;
    virtual void OutOfBandData(netsrc_t sock, netadr_t adr, uchar8 *format,
                               sint len) = 0;
    virtual sint StringToAdr(pointer s, netadr_t *a, netadrtype_t family) = 0;
};

extern idNetworkChainSystem *networkChainSystem;

#endif //!__THREADS_API_HPP__
