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
// File name:   null_client.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: all other sound mixing is portable
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.h>
#elif DEDICATED
#include <null/null_serverprecompiled.h>
#endif

convar_t*         cl_shownet;

// TTimo: win32 dedicated
convar_t*         cl_language;

void CL_Shutdown( void )
{
}

void CL_Init( void )
{
    cl_shownet = cvarSystem->Get( "cl_shownet", "0", CVAR_TEMP, "description" );
    // TTimo: localisation, prolly not any use in dedicated / null client
    cl_language = cvarSystem->Get( "cl_language", "0", CVAR_ARCHIVE, "description" );
}

void CL_MouseEvent( S32 dx, S32 dy, S32 time )
{
}

void Key_WriteBindings( fileHandle_t f )
{
}

void CL_Frame( S32 msec )
{
}

void CL_PacketEvent( netadr_t from, msg_t* msg )
{
}

void CL_CharEvent( S32 key )
{
}

void CL_Disconnect( bool showMainMenu )
{
}

void CL_MapLoading( void )
{
}

void CL_KeyEvent( S32 key, S32 down, S32 time )
{
}

void CL_ForwardCommandToServer( StringEntry string )
{
}

void CL_ConsolePrint( UTF8* txt )
{
}

void CL_JoystickEvent( S32 axis, S32 value, S32 time )
{
}

void CL_InitKeyCommands( void )
{
}

void CL_CDDialog( void )
{
}

void CL_FlushMemory( void )
{
}

void CL_StartHunkUsers( void )
{
}

// bk001119 - added new dummy for sv_init.c
void CL_ShutdownAll( void )
{
};

// bk001208 - added new dummy (RC4)
bool CL_CDKeyValidate( StringEntry key, StringEntry checksum )
{
    return true;
}

// TTimo added for win32 dedicated
void Key_ClearStates( void )
{
}

// Dushan
StringEntry Con_GetText( S32 console )
{
    return NULL;
}

/*
===============
idSystemLocal::Init
===============
*/
void idSystemLocal::Init( void* windowData )
{
}

/*
===============
idSystemLocal::Shutdown
===============
*/
void idSystemLocal::Shutdown( void )
{
}

/*
===============
idSystemLocal::Restart
===============
*/
void idSystemLocal::Restart( void )
{
}

/*
===============
idSystemLocal::Frame
===============
*/
void idSystemLocal::Frame( void )
{
}