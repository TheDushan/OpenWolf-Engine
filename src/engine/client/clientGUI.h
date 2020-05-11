////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2020 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   cl_ui.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTGUI_LOCAL_H__
#define __CLIENTGUI_LOCAL_H__

//
// idClientGUISystemLocal
//
class idClientGUISystemLocal : public idClientGUISystem
{
public:
    idClientGUISystemLocal();
    ~idClientGUISystemLocal();
    
    virtual void GetClientState( uiClientState_t* state );
    virtual void GetGlconfig( vidconfig_t* config );
    virtual void GUIGetClipboardData( UTF8* buf, S32 buflen );
    virtual S32 GetConfigString( S32 index, UTF8* buf, S32 size );
    virtual bool GetNews( bool begin );
    virtual void KeynumToStringBuf( S32 keynum, UTF8* buf, S32 buflen );
    virtual void GetBindingBuf( S32 keynum, UTF8* buf, S32 buflen );
    virtual S32 GetCatcher( void );
    virtual void SetCatcher( S32 catcher );
    virtual bool checkKeyExec( S32 key );
    virtual bool GameCommand( void );
    virtual void InitGUI( void );
    virtual void ShutdownGUI( void );
    
    static void CreateExportTable( void );
};

#endif // !__CLIENTGUI_LOCAL_H__