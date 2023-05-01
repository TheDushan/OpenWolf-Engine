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
// File name:   clientParse.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description: parse a message received from the server
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTPARSE_HPP__
#define __CLIENTPARSE_HPP__

static sint entLastVisible[MAX_CLIENTS];

//
// idClientParseSystemLocal
//
class idClientParseSystemLocal {
public:
    idClientParseSystemLocal();
    ~idClientParseSystemLocal();

    static void ParseServerMessage(msg_t *msg);
    static void ShowNet(const msg_t *msg, pointer s);
    static bool isEntVisible(entityState_t *ent);
    static void DeltaEntity(msg_t *msg, clSnapshot_t *frame, sint newnum,
                            entityState_t *old, bool unchanged);
    static void ParsePacketEntities(msg_t *msg, clSnapshot_t *oldframe,
                                    clSnapshot_t *newframe);
    static void ParseSnapshot(msg_t *msg);
    static void SystemInfoChanged(void);
    static void ParseGamestate(msg_t *msg);
    static void ParseDownload(msg_t *msg);
    static void ParseCommandString(msg_t *msg);
};

extern idClientParseSystemLocal clientParseSystemLocal;

#endif //!__CLIENTPARSE_HPP__