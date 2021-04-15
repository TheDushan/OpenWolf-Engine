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
// File name:   clientLAN_api.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTLAN_API_H__
#define __CLIENTLAN_API_H__

//
// idClientLANSystem
//
class idClientLANSystem {
public:
    virtual void LoadCachedServers(void) = 0;
    virtual void SaveServersToCache(void) = 0;
    virtual void ResetPings(sint source) = 0;
    virtual sint AddServer(sint source, pointer name, pointer address) = 0;
    virtual void RemoveServer(sint source, pointer addr) = 0;
    virtual sint GetServerCount(sint source) = 0;
    virtual void GetServerAddressString(sint source, sint n, valueType *buf,
                                        uint64 buflen) = 0;
    virtual void GetServerInfo(sint source, sint n, valueType *buf,
                               uint64 buflen) = 0;
    virtual sint GetServerPing(sint source, sint n) = 0;
    virtual sint CompareServers(sint source, sint sortKey, sint sortDir,
                                sint s1, sint s2) = 0;
    virtual sint GetPingQueueCount(void) = 0;
    virtual void ClearPing(sint n) = 0;
    virtual void GetPing(sint n, valueType *buf, uint64 buflen,
                         sint *pingtime) = 0;
    virtual void GetPingInfo(sint n, valueType *buf, uint64 buflen) = 0;
    virtual void MarkServerVisible(sint source, sint n, bool visible) = 0;
    virtual sint ServerIsVisible(sint source, sint n) = 0;
    virtual bool UpdateVisiblePings(sint source) = 0;
    virtual sint GetServerStatus(valueType *serverAddress,
                                 valueType *serverStatus, uint64 maxLen) = 0;
    virtual bool ServerIsInFavoriteList(sint source, sint n) = 0;
};

extern idClientLANSystem *clientLANSystem;

#endif // !__CLIENTLAN_API_H__

