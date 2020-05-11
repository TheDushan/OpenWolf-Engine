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

#ifndef __CLIENTGUI_API_H__
#define __CLIENTGUI_API_H__

typedef struct
{
    S32 connectPacketCount;
    S32 clientNum;
    UTF8 servername[MAX_STRING_CHARS];
    UTF8 updateInfoString[MAX_STRING_CHARS];
    UTF8 messageString[MAX_STRING_CHARS];
    connstate_t connState;
} uiClientState_t;

//
// idClientGUISystem
//
class idClientGUISystem
{
public:
    virtual void GetClientState( uiClientState_t* state ) = 0;
    virtual void GetGlconfig( vidconfig_t* config ) = 0;
    virtual void GUIGetClipboardData( UTF8* buf, S32 buflen ) = 0;
    virtual S32 GetConfigString( S32 index, UTF8* buf, S32 size ) = 0;
    virtual bool GetNews( bool begin ) = 0;
    virtual void KeynumToStringBuf( S32 keynum, UTF8* buf, S32 buflen ) = 0;
    virtual void GetBindingBuf( S32 keynum, UTF8* buf, S32 buflen ) = 0;
    virtual S32 GetCatcher( void ) = 0;
    virtual void SetCatcher( S32 catcher ) = 0;
    virtual bool checkKeyExec( S32 key ) = 0;
    virtual bool GameCommand( void ) = 0;
    virtual void InitGUI( void ) = 0;
    virtual void ShutdownGUI( void ) = 0;
};

extern idClientGUISystem* clientGUISystem;

#endif // !__CLIENTGUI_API_H__

