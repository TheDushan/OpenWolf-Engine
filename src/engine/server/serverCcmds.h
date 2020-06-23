////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2018 - 2019 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   serverCcmds.h
// Version:     v1.02
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SERVERCCMDS_H__
#define __SERVERCCMDS_H__

//
// idServerCcmdsSystemLocal
//
class idServerCcmdsSystemLocal
{
public:
    idServerCcmdsSystemLocal();
    ~idServerCcmdsSystemLocal();
    
    static void TempBanNetAddress( netadr_t address, sint length );
    static void AddOperatorCommands( void );
    static void Heartbeat_f( void );
    static bool TempBanIsBanned( netadr_t address );
    
    static client_t* GetPlayerByName( void );
    static void Map_f( void );
    static bool CheckTransitionGameState( gamestate_t new_gs, gamestate_t old_gs );
    static bool TransitionGameState( gamestate_t new_gs, gamestate_t old_gs, sint delay );
    static void FieldInfo_f( void );
    static void MapRestart_f( void );
    static void LoadGame_f( void );
    static void Status_f( void );
    static void ConSay_f( void );
    static void Serverinfo_f( void );
    static void Systeminfo_f( void );
    static void DumpUser_f( void );
    static void KillServer_f( void );
    static void GameCompleteStatus_f( void );
    static void CompleteMapName( valueType* args, sint argNum );
    static void ClientRedirect( valueType* outputbuf );
    static void StartRedirect_f( void );
    static void Demo_Record_f( void );
    static void Demo_Play_f( void );
    static void Demo_Stop_f( void );
    static void CompleteDemoName( valueType* args, sint argNum );
};

extern idServerCcmdsSystemLocal serverCcmdsLocal;

#endif //!__SERVERCCMDS_H__
