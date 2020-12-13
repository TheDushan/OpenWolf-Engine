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
// File name:   clientGUI_api.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTGUI_API_H__
#define __CLIENTGUI_API_H__

typedef struct
{
    sint connectPacketCount;
    sint clientNum;
    valueType servername[MAX_STRING_CHARS];
    valueType updateInfoString[MAX_STRING_CHARS];
    valueType messageString[MAX_STRING_CHARS];
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
    virtual void GUIGetClipboardData( valueType* buf, sint buflen ) = 0;
    virtual sint GetConfigString( sint index, valueType* buf, sint size ) = 0;
    virtual bool GetNews( bool begin ) = 0;
    virtual void KeynumToStringBuf( sint keynum, valueType* buf, sint buflen ) = 0;
    virtual void GetBindingBuf( sint keynum, valueType* buf, sint buflen ) = 0;
    virtual sint GetCatcher( void ) = 0;
    virtual void SetCatcher( sint catcher ) = 0;
    virtual bool checkKeyExec( sint key ) = 0;
    virtual bool GameCommand( void ) = 0;
    virtual void InitGUI( void ) = 0;
    virtual void ShutdownGUI( void ) = 0;
};

extern idClientGUISystem* clientGUISystem;

#endif // !__CLIENTGUI_API_H__

