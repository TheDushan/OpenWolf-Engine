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
// File name:   serverMain_api.hpp
// Created:     12/26/2018
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SERVERMAIN_API_HPP__
#define __SERVERMAIN_API_HPP__

#ifdef GAMEDLL
typedef struct client_s client_t;
typedef struct msg_t msg_t;
#endif

//
// idServerMainSystem
//
class idServerMainSystem {
public:
    virtual void AddServerCommand(client_t *client, pointer cmd) = 0;
    virtual void SendServerCommand(client_t *cl, pointer fmt, ...) = 0;
    virtual void MasterShutdown(void) = 0;
    virtual void MasterGameCompleteStatus(void) = 0;
    virtual void MasterGameStat(pointer data) = 0;
    virtual void PacketEvent(netadr_t from, msg_t *msg) = 0;
    virtual void Frame(sint msec) = 0;
    virtual sint LoadTag(pointer mod_name) = 0;
    virtual sint RateMsec(client_t *client) = 0;
    virtual sint SendQueuedPackets(void) = 0;
};

extern idServerMainSystem *serverMainSystem;

#endif //!__SERVERMAIN_API_HPP__
