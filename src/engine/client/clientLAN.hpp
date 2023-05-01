////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 - 2023 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   clientLAN.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTLAN_HPP__
#define __CLIENTLAN_HPP__

//
// idClientLANSystemLocal
//
class idClientLANSystemLocal : public idClientLANSystem {
public:
    idClientLANSystemLocal();
    ~idClientLANSystemLocal();

    virtual void LoadCachedServers(void);
    virtual void SaveServersToCache(void);
    virtual void ResetPings(sint source);
    virtual sint AddServer(sint source, pointer name, pointer address);
    virtual void RemoveServer(sint source, pointer addr);
    virtual sint GetServerCount(sint source);
    virtual void GetServerAddressString(sint source, sint n, valueType *buf,
                                        uint64 buflen);
    virtual void GetServerInfo(sint source, sint n, valueType *buf,
                               uint64 buflen);
    virtual sint GetServerPing(sint source, sint n);
    virtual sint CompareServers(sint source, sint sortKey, sint sortDir,
                                sint s1, sint s2);
    virtual sint GetPingQueueCount(void);
    virtual void ClearPing(sint n);
    virtual void GetPing(sint n, valueType *buf, uint64 buflen,
                         sint *pingtime);
    virtual void GetPingInfo(sint n, valueType *buf, uint64 buflen);
    virtual void MarkServerVisible(sint source, sint n, bool visible);
    virtual sint ServerIsVisible(sint source, sint n);
    virtual bool UpdateVisiblePings(sint source);
    virtual sint GetServerStatus(valueType *serverAddress,
                                 valueType *serverStatus, uint64 maxLen);
    virtual bool ServerIsInFavoriteList(sint source, sint n);

    static serverInfo_t *GetServerPtr(sint source, sint n);
};

extern idClientLANSystemLocal clientLANLocal;

#endif // !__CLIENTLAN_HPP__