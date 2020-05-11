////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2019 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   clientGame.h
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: client system interaction with client game
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTGAME_LOCAL_H__
#define __CLIENTGAME_LOCAL_H__

#define RESET_TIME 500

//
// idClientGameSystemLocal
//
class idClientGameSystemLocal : public idClientGameSystem
{
public:
    idClientGameSystemLocal();
    ~idClientGameSystemLocal();
    
    virtual void GetGameState( gameState_t* gs );
    virtual void GetGlconfig( vidconfig_t* glconfig );
    virtual S32 CompleteCallback( StringEntry complete );
    virtual bool GetUserCmd( S32 cmdNumber, usercmd_t* ucmd );
    virtual S32 GetCurrentCmdNumber( void );
    virtual void GetCurrentSnapshotNumber( S32* snapshotNumber, S32* serverTime );
    virtual bool GetSnapshot( S32 snapshotNumber, snapshot_t* snapshot );
    virtual void SetUserCmdValue( S32 userCmdValue, S32 flags, F32 sensitivityScale, S32 mpIdentClient );
    virtual void SetClientLerpOrigin( F32 x, F32 y, F32 z );
    virtual void CgameCompletion( void( *callback )( StringEntry s ), S32 argNum );
    virtual void AddCgameCommand( StringEntry cmdName, StringEntry cmdDesc );
    virtual void CgameError( StringEntry string );
    virtual bool CGameCheckKeyExec( S32 key );
    virtual void UIPopup( StringEntry uiname );
    virtual bool GetServerCommand( S32 serverCommandNumber );
    virtual void LoadMap( StringEntry mapname );
    virtual void ShutdownCGame( void );
    virtual void UIClosePopup( StringEntry uiname );
    virtual void KeySetCatcher( S32 catcher );
    virtual void InitCGame( void );
    virtual bool GameCommand( void );
    virtual void GameConsoleText( void );
    virtual void CGameRendering( stereoFrame_t stereo );
    virtual void SetCGameTime( void );
    virtual bool GetTag( S32 clientNum, UTF8* tagname, orientation_t* _or );
    
    static void UpdateLevelHunkUsage( void );
    static void CompleteCgameCommand( UTF8* args, S32 argNum );
    static void ConfigstringModified( void );
    static void SetExpectedHunkUsage( StringEntry mapname );
    static void CreateExportTable( void );
    static void AdjustTimeDelta( void );
    static void FirstSnapshot( void );
};

extern idClientGameSystemLocal clientGameLocal;

#endif // !__CLIENTGAME_LOCAL_H__

