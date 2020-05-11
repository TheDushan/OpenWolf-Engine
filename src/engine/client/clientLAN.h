////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 - 2019 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   clientAVI.h
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTLAN_LOCAL_H__
#define __CLIENTLAN_LOCAL_H__

//
// idClientLANSystemLocal
//
class idClientLANSystemLocal : public idClientLANSystem
{
public:
    idClientLANSystemLocal();
    ~idClientLANSystemLocal();
    
    virtual void LoadCachedServers( void );
    virtual void SaveServersToCache( void );
    virtual void ResetPings( S32 source );
    virtual S32 AddServer( S32 source, StringEntry name, StringEntry address );
    virtual void RemoveServer( S32 source, StringEntry addr );
    virtual S32 GetServerCount( S32 source );
    virtual void GetServerAddressString( S32 source, S32 n, UTF8* buf, S32 buflen );
    virtual void GetServerInfo( S32 source, S32 n, UTF8* buf, S32 buflen );
    virtual S32 GetServerPing( S32 source, S32 n );
    virtual S32 CompareServers( S32 source, S32 sortKey, S32 sortDir, S32 s1, S32 s2 );
    virtual S32 GetPingQueueCount( void );
    virtual void ClearPing( S32 n );
    virtual void GetPing( S32 n, UTF8* buf, S32 buflen, S32* pingtime );
    virtual void GetPingInfo( S32 n, UTF8* buf, S32 buflen );
    virtual void MarkServerVisible( S32 source, S32 n, bool visible );
    virtual S32 ServerIsVisible( S32 source, S32 n );
    virtual bool UpdateVisiblePings( S32 source );
    virtual S32 GetServerStatus( UTF8* serverAddress, UTF8* serverStatus, S32 maxLen );
    virtual bool ServerIsInFavoriteList( S32 source, S32 n );
    
    static serverInfo_t* GetServerPtr( S32 source, S32 n );
};

extern idClientLANSystemLocal clientLANLocal;

#endif // !__CLIENTLAN_LOCAL_H__