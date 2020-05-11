////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 - 2019 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   serverGame_api.h
// Version:     v1.01
// Created:     11/24/2019
// Compilers:   Visual Studio 2017, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SERVERCLIENT_API_H__
#define __SERVERCLIENT_API_H__

//
// idServerClientSystem
//
class idServerClientSystem
{
public:
    virtual void DropClient( client_t* drop, StringEntry reason ) = 0;
    virtual void ExecuteClientCommand( client_t* cl, StringEntry s, bool clientOK, bool premaprestart ) = 0;
    virtual void ClientEnterWorld( client_t* client, usercmd_t* cmd ) = 0;
    virtual void CloseDownload( client_t* cl ) = 0;
    virtual void UserinfoChanged( client_t* cl ) = 0;
    virtual void FreeClient( client_t* client ) = 0;
    virtual void ClientThink( S32 client, usercmd_t* cmd ) = 0;
    virtual void WriteDownloadToClient( client_t* cl, msg_t* msg ) = 0;
    virtual void GetChallenge( netadr_t from ) = 0;
    virtual void DirectConnect( netadr_t from ) = 0;
    virtual void ExecuteClientMessage( client_t* cl, msg_t* msg ) = 0;
};

extern idServerClientSystem* serverClientSystem;

#endif //!__SERVERCLIENT_API_H__
