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
// File name:   clientMain_api.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTMAIN_API_H__
#define __CLIENTMAIN_API_H__

#if defined (GUI) || defined (CGAMEDLL)
typedef struct msg_t msg_t;
#endif

//
// idClientMainSystemAPI
//
class idClientMainSystemAPI {
public:

    virtual void ShutdownAll(bool shutdownRen) = 0;
    virtual void FlushMemory(void) = 0;
    virtual void MapLoading(void) = 0;
    virtual void ClearState(void) = 0;
    virtual void ForwardCommandToServer(pointer string) = 0;
    virtual void SendPureChecksums(void) = 0;
    virtual void ResetPureClientAtServer(void) = 0;
    virtual void PacketEvent(netadr_t from, msg_t *msg) = 0;
    virtual void Frame(sint msec) = 0;
    virtual void StartHunkUsers(bool rendererOnly) = 0;
    virtual void Init(void) = 0;
    virtual void Shutdown(void) = 0;
    virtual void OpenURL(pointer url) = 0;
    virtual void AddToLimboChat(pointer str) = 0;
    virtual void LogPrintf(fileHandle_t fileHandle, pointer fmt, ...) = 0;
};

extern idClientMainSystemAPI *clientMainSystem;

#endif // !__CLIENTMAIN_API_H__
