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
// File name:   serverInit.hpp
// Created:     12/27/2018
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
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
    virtual void SetConfigstringNoUpdate( sint index, pointer val );
    virtual void SetConfigstring( sint index, pointer val );
    virtual void GetConfigstring( sint index, valueType* buffer, uint64 bufferSize );
    virtual void SetConfigstringRestrictions( sint index, const clientList_t* clientList );
    virtual void SetUserinfo( sint index, pointer val );
    virtual void GetUserinfo( sint index, valueType* buffer, uint64 bufferSize );
    virtual void SpawnServer( valueType* server, bool killBots );
    virtual void Init( void );
    virtual void Shutdown( valueType* finalmsg );
    
public:
    idServerInitSystemLocal();
    ~idServerInitSystemLocal();
    
    void SendConfigstring( client_t* client, sint index );
    void CreateBaseline( void );
    void BoundMaxClients( sint minimum );
    void Startup( void );
    void ChangeMaxClients( void );
    void SetExpectedHunkUsage( valueType* mapname );
    void ClearServer( void );
    void TouchCGameDLL( void );
    void FinalCommand( valueType* cmd, bool disconnect );
    void ParseVersionMapping( void );
};

extern idServerInitSystemLocal serverInitSystemLocal;

#endif //!__SERVERINIT_H__
