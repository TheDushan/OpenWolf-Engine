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
// File name:   serverInit.h
// Version:     v1.00
// Created:     12/27/2018
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SERVERINIT_H__
#define __SERVERINIT_H__

//
// idServerGameSystemLocal
//
class idServerInitSystemLocal : public idServerInitSystem
{
public:
    virtual void UpdateConfigStrings( void );
    virtual void SetConfigstringNoUpdate( S32 index, StringEntry val );
    virtual void SetConfigstring( S32 index, StringEntry val );
    virtual void GetConfigstring( S32 index, UTF8* buffer, S32 bufferSize );
    virtual void SetConfigstringRestrictions( S32 index, const clientList_t* clientList );
    virtual void SetUserinfo( S32 index, StringEntry val );
    virtual void GetUserinfo( S32 index, UTF8* buffer, S32 bufferSize );
    virtual void SpawnServer( UTF8* server, bool killBots );
    virtual void Init( void );
    virtual void Shutdown( UTF8* finalmsg );
    
public:
    idServerInitSystemLocal();
    ~idServerInitSystemLocal();
    
    void ResolveAuthHost( void );
    void SendConfigstring( client_t* client, S32 index );
    void CreateBaseline( void );
    void BoundMaxClients( S32 minimum );
    void Startup( void );
    void ChangeMaxClients( void );
    void SetExpectedHunkUsage( UTF8* mapname );
    void ClearServer( void );
    void TouchCGameDLL( void );
    void FinalCommand( UTF8* cmd, bool disconnect );
    void ParseVersionMapping( void );
};

extern idServerInitSystemLocal serverInitSystemLocal;

#endif //!__SERVERINIT_H__
