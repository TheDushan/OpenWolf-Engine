////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2005 - 2006 Tim Angus
// Copyright(C) 2011 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   clientAVI.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTMAIN_LOCAL_HPP__
#define __CLIENTMAIN_LOCAL_HPP__

//
// idClientMainSystemLocal
//
class idClientMainSystemLocal : public idClientMainSystemAPI {
public:
    idClientMainSystemLocal();
    ~idClientMainSystemLocal();

    virtual void ShutdownAll(bool shutdownRen);
    virtual void FlushMemory(void);
    virtual void MapLoading(void);
    virtual void ClearState(void);
    virtual void ForwardCommandToServer(pointer string);
    virtual void SendPureChecksums(void);
    virtual void ResetPureClientAtServer(void);
    virtual void PacketEvent(netadr_t from, msg_t *msg);
    virtual void Frame(sint msec);
    virtual void StartHunkUsers(bool rendererOnly);
    virtual void Init(void);
    virtual void Shutdown(void);
    virtual void OpenURL(pointer url);
    virtual void AddToLimboChat(pointer str);
    virtual void LogPrintf(fileHandle_t fileHandle, pointer fmt, ...);

    static void PurgeCache(void);
    static void DoPurgeCache(void);
    static void ClearMemory(bool shutdownRen);
    static void CheckForResend(void);
    static void DisconnectPacket(netadr_t from);
    static void PrintPacket(netadr_t from, msg_t *msg);
    static void ConnectionlessPacket(netadr_t from, msg_t *msg);
    static void CheckTimeout(void);
    static void CheckUserinfo(void);
    static void UpdateInfoPacket(netadr_t from);
};

extern idClientMainSystemLocal clientMainLocal;

#endif // !__CLIENTMAIN_LOCAL_HPP__
