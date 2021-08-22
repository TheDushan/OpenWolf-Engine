////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2019 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   clientGame.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: client system interaction with client game
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTGAME_LOCAL_H__
#define __CLIENTGAME_LOCAL_H__

#define RESET_TIME 500

//
// idClientGameSystemLocal
//
class idClientGameSystemLocal : public idClientGameSystem {
public:
    idClientGameSystemLocal();
    ~idClientGameSystemLocal();

    virtual void GetGameState(gameState_t *gs);
    virtual void GetGlconfig(vidconfig_t *glconfig);
    virtual sint CompleteCallback(pointer complete);
    virtual bool GetUserCmd(sint cmdNumber, usercmd_t *ucmd);
    virtual sint GetCurrentCmdNumber(void);
    virtual void GetCurrentSnapshotNumber(sint *snapshotNumber,
                                          sint *serverTime);
    virtual bool GetSnapshot(sint snapshotNumber, snapshot_t *snapshot);
    virtual void SetUserCmdValue(sint userCmdValue, sint flags,
                                 float32 sensitivityScale, sint mpIdentClient);
    virtual void SetClientLerpOrigin(float32 x, float32 y, float32 z);
    virtual void CgameCompletion(void(*callback)(pointer s), sint argNum);
    virtual void AddCgameCommand(pointer cmdName, pointer cmdDesc);
    virtual void CgameError(pointer string);
    virtual bool CGameCheckKeyExec(sint key);
    virtual void UIPopup(pointer uiname);
    virtual bool GetServerCommand(sint serverCommandNumber);
    virtual void LoadMap(pointer mapname);
    virtual void ShutdownCGame(void);
    virtual void UIClosePopup(pointer uiname);
    virtual void KeySetCatcher(sint catcher);
    virtual void InitCGame(void);
    virtual bool GameCommand(void);
    virtual void GameConsoleText(void);
    virtual void CGameRendering(stereoFrame_t stereo);
    virtual void SetCGameTime(void);
    virtual bool GetTag(sint clientNum, valueType *tagname,
                        orientation_t *_or);

    static void UpdateLevelHunkUsage(void);
    static void CompleteCgameCommand(valueType *args, sint argNum);
    static void ConfigstringModified(void);
    static void SetExpectedHunkUsage(pointer mapname);
    static void CreateExportTable(void);
    static void AdjustTimeDelta(void);
    static void FirstSnapshot(void);
    static void OpenLog(pointer filename, fileHandle_t *f, bool sync);
    static void CloseLog(fileHandle_t *f);
};

extern idClientGameSystemLocal clientGameLocal;

#endif // !__CLIENTGAME_LOCAL_H__

