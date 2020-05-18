////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2019 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   clientGame_api.h
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
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
    S32 snapFlags;	// SNAPFLAG_RATE_DELAYED, etc
    S32 ping;
    S32 serverTime;	// server time the message is valid for (in msec)
    S32 numEntities;	// all of the entities that need to be presented
    S32 numServerCommands;	// text based server commands to execute when this
    S32 serverCommandSequence;	// snapshot becomes current
    U8 areamask[MAX_MAP_AREA_BYTES];	// portalarea visibility bits
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
    virtual S32 CompleteCallback( StringEntry complete ) = 0;
    virtual bool GetUserCmd( S32 cmdNumber, usercmd_t* ucmd ) = 0;
    virtual S32 GetCurrentCmdNumber( void ) = 0;
    virtual void GetCurrentSnapshotNumber( S32* snapshotNumber, S32* serverTime ) = 0;
    virtual bool GetSnapshot( S32 snapshotNumber, snapshot_t* snapshot ) = 0;
    virtual void SetUserCmdValue( S32 userCmdValue, S32 flags, F32 sensitivityScale, S32 mpIdentClient ) = 0;
    virtual void SetClientLerpOrigin( F32 x, F32 y, F32 z ) = 0;
    virtual void CgameCompletion( void( *callback )( StringEntry s ), S32 argNum ) = 0;
    virtual void AddCgameCommand( StringEntry cmdName, StringEntry cmdDesc ) = 0;
    virtual void CgameError( StringEntry string ) = 0;
    virtual bool CGameCheckKeyExec( S32 key ) = 0;
    virtual void UIPopup( StringEntry uiname ) = 0;
    virtual bool GetServerCommand( S32 serverCommandNumber ) = 0;
    virtual void LoadMap( StringEntry mapname ) = 0;
    virtual void ShutdownCGame( void ) = 0;
    virtual void UIClosePopup( StringEntry uiname ) = 0;
    virtual void KeySetCatcher( S32 catcher ) = 0;
    virtual void InitCGame( void ) = 0;
    virtual bool GameCommand( void ) = 0;
    virtual void GameConsoleText( void ) = 0;
    virtual void CGameRendering( stereoFrame_t stereo ) = 0;
    virtual void SetCGameTime( void ) = 0;
    virtual bool GetTag( S32 clientNum, UTF8* tagname, orientation_t* _or ) = 0;
};

extern idClientGameSystem* clientGameSystem;

#endif // !__CLIENTGAME_API_H__

