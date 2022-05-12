////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 - 2022 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of OpenWolf.
//
// OpenWolf is free software; you can redistribute it
// and / or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the License,
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
// File name:   serverSnapshot_api.hpp
// Created:     12/25/2018
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SERVERSNAPSHOT_API_HPP__
#define __SERVERSNAPSHOT_API_HPP__

//
// idServerSnapshotSystem
//
class idServerSnapshotSystem {
public:
    virtual void SendMessageToClient(msg_t *msg, client_t *client) = 0;
    virtual void SendClientIdle(client_t *client) = 0;
    virtual void SendClientSnapshot(client_t *client) = 0;
    virtual void SendClientMessages(void) = 0;
    virtual void CheckClientUserinfoTimer(void) = 0;
    virtual void UpdateServerCommandsToClient(client_t *client,
            msg_t *msg) = 0;
};

extern idServerSnapshotSystem *serverSnapshotSystem;

#endif //!__SERVERSNAPSHOT_API_HPP__
