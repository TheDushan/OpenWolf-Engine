////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 - 2022 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   serverSnapshot.hpp
// Created:     12/25/2018
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SERVERSNAPSHOT_HPP__
#define __SERVERSNAPSHOT_HPP__

#define MAX_SNAPSHOT_ENTITIES 1024

// include our header, IP header, and some overhead
#define HEADER_RATE_BYTES 48

typedef struct {
    sint numSnapshotEntities;
    sint snapshotEntities[MAX_SNAPSHOT_ENTITIES];
} snapshotEntityNumbers_t;

//
// idServerSnapshotSystemLocal
//
class idServerSnapshotSystemLocal : public idServerSnapshotSystem {
public:
    virtual void SendMessageToClient(msg_t *msg, client_t *client);
    virtual void SendClientIdle(client_t *client);
    virtual void SendClientSnapshot(client_t *client);
    virtual void SendClientMessages(void);
    virtual void CheckClientUserinfoTimer(void);
    virtual void UpdateServerCommandsToClient(client_t *client, msg_t *msg);

public:
    idServerSnapshotSystemLocal();
    ~idServerSnapshotSystemLocal();

    static void EmitPacketEntities(clientSnapshot_t *from,
                                   clientSnapshot_t *to, msg_t *msg);
    static void WriteSnapshotToClient(client_t *client, msg_t *msg);
    static sint QsortEntityNumbers(const void *a, const void *b);
    static void AddEntToSnapshot(sharedEntity_t *clientEnt, svEntity_t *svEnt,
                                 sharedEntity_t *gEnt, snapshotEntityNumbers_t *eNums);
    static void AddEntitiesVisibleFromPoint(vec3_t origin,
                                            clientSnapshot_t *frame, snapshotEntityNumbers_t *eNums, bool portal);
    static void BuildClientSnapshot(client_t *client);
    static sint RateMsec(client_t *client, sint messageSize);
    static bool UpdateServerCommandsToClients(client_t *client, msg_t *msg,
            bool allowPartial);
};

extern idServerSnapshotSystemLocal serverSnapshotSystemLocal;

#endif //!__SERVERSNAPSHOT_HPP__