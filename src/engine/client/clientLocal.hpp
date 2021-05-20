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
// File name:   clientLocal.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: primary header for client
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTLOCAL_HPP__
#define __CLIENTLOCAL_HPP__

#if !defined ( DEDICATED ) && !defined ( UPDATE_SERVER ) && !defined ( BSPC )
#endif


//=============================================================================

extern void    *cgvm;           // interface to cgame dll or vm
extern void    *uivm;
extern void    *dbvm;
extern idCGame *cgame;
extern idUserInterfaceManager *uiManager;

//=================================================

//
// cl_main
//
void CL_PurgeCache(void);
void CL_Init(void);
void CL_FlushMemory(void);
void CL_ShutdownAll(bool shutdownRen);
void CL_AddReliableCommand(pointer cmd);

void            CL_StartHunkUsers(bool rendererOnly);

#if !defined(UPDATE_SERVER)
void            CL_CheckAutoUpdate(void);
bool            CL_NextUpdateServer(void);
void            CL_GetAutoUpdate(void);
#endif

void            CL_Disconnect_f(void);
void            CL_Vid_Restart_f(void);
void            CL_Snd_Restart_f(void);
void            CL_NextDemo(void);
void            CL_ReadDemoMessage(void);
void            CL_StartDemoLoop(void);

void            CL_InitDownloads(void);
void            CL_NextDownload(void);


void            CL_ShutdownRef(void);
void            CL_InitRef(void);

void            CL_AddToLimboChat(pointer str);      // NERVE - SMF

void            CL_OpenURL(pointer url);     // TTimo
void            CL_Record(pointer name);


void            CL_ClearState(void);

extern sint      cl_connectedToPureServer;

//====================================================================

void            CL_UpdateInfoPacket(netadr_t from);      // DHM - Nerve

//
// cl_main.c
//
void            CL_WriteDemoMessage(msg_t *msg, sint headerBytes);
void            CL_RequestMotd(void);

#endif //!__CLIENTLOCAL_HPP__
