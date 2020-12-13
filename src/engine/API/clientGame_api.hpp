////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2019 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   clientGame_api.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTGAME_API_H__
#define __CLIENTGAME_API_H__

#define MAX_ENTITIES_IN_SNAPSHOT    512

// snapshots are a view of the server at a given time

// Snapshots are generated at regular time intervals by the server,
// but they may not be sent if a client's rate level is exceeded, or
// they may be dropped by the network.
typedef struct
{
    sint snapFlags;	// SNAPFLAG_RATE_DELAYED, etc
    sint ping;
    sint serverTime;	// server time the message is valid for (in msec)
    sint numEntities;	// all of the entities that need to be presented
    sint numServerCommands;	// text based server commands to execute when this
    sint serverCommandSequence;	// snapshot becomes current
    uchar8 areamask[MAX_MAP_AREA_BYTES];	// portalarea visibility bits
    playerState_t ps;			// complete information about the current player at this time
    entityState_t entities[MAX_ENTITIES_IN_SNAPSHOT];	// at the time of this snapshot
} snapshot_t;

//
// idClientGameSystem
//
class idClientGameSystem
{
public:
    virtual void GetGameState( gameState_t* gs ) = 0;
    virtual void GetGlconfig( vidconfig_t* glconfig ) = 0;
    virtual sint CompleteCallback( pointer complete ) = 0;
    virtual bool GetUserCmd( sint cmdNumber, usercmd_t* ucmd ) = 0;
    virtual sint GetCurrentCmdNumber( void ) = 0;
    virtual void GetCurrentSnapshotNumber( sint* snapshotNumber, sint* serverTime ) = 0;
    virtual bool GetSnapshot( sint snapshotNumber, snapshot_t* snapshot ) = 0;
    virtual void SetUserCmdValue( sint userCmdValue, sint flags, float32 sensitivityScale, sint mpIdentClient ) = 0;
    virtual void SetClientLerpOrigin( float32 x, float32 y, float32 z ) = 0;
    virtual void CgameCompletion( void( *callback )( pointer s ), sint argNum ) = 0;
    virtual void AddCgameCommand( pointer cmdName, pointer cmdDesc ) = 0;
    virtual void CgameError( pointer string ) = 0;
    virtual bool CGameCheckKeyExec( sint key ) = 0;
    virtual void UIPopup( pointer uiname ) = 0;
    virtual bool GetServerCommand( sint serverCommandNumber ) = 0;
    virtual void LoadMap( pointer mapname ) = 0;
    virtual void ShutdownCGame( void ) = 0;
    virtual void UIClosePopup( pointer uiname ) = 0;
    virtual void KeySetCatcher( sint catcher ) = 0;
    virtual void InitCGame( void ) = 0;
    virtual bool GameCommand( void ) = 0;
    virtual void GameConsoleText( void ) = 0;
    virtual void CGameRendering( stereoFrame_t stereo ) = 0;
    virtual void SetCGameTime( void ) = 0;
    virtual bool GetTag( sint clientNum, valueType* tagname, orientation_t* _or ) = 0;
};

extern idClientGameSystem* clientGameSystem;

#endif // !__CLIENTGAME_API_H__

