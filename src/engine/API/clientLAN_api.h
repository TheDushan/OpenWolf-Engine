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

#ifndef __CLIENTLAN_API_H__
#define __CLIENTLAN_API_H__

//
// idClientLANSystem
//
class idClientLANSystem
{
public:
    virtual void LoadCachedServers( void ) = 0;
    virtual void SaveServersToCache( void ) = 0;
    virtual void ResetPings( S32 source ) = 0;
    virtual S32 AddServer( S32 source, StringEntry name, StringEntry address ) = 0;
    virtual void RemoveServer( S32 source, StringEntry addr ) = 0;
    virtual S32 GetServerCount( S32 source ) = 0;
    virtual void GetServerAddressString( S32 source, S32 n, UTF8* buf, S32 buflen ) = 0;
    virtual void GetServerInfo( S32 source, S32 n, UTF8* buf, S32 buflen ) = 0;
    virtual S32 GetServerPing( S32 source, S32 n ) = 0;
    virtual S32 CompareServers( S32 source, S32 sortKey, S32 sortDir, S32 s1, S32 s2 ) = 0;
    virtual S32 GetPingQueueCount( void ) = 0;
    virtual void ClearPing( S32 n ) = 0;
    virtual void GetPing( S32 n, UTF8* buf, S32 buflen, S32* pingtime ) = 0;
    virtual void GetPingInfo( S32 n, UTF8* buf, S32 buflen ) = 0;
    virtual void MarkServerVisible( S32 source, S32 n, bool visible ) = 0;
    virtual S32 ServerIsVisible( S32 source, S32 n ) = 0;
    virtual bool UpdateVisiblePings( S32 source ) = 0;
    virtual S32 GetServerStatus( UTF8* serverAddress, UTF8* serverStatus, S32 maxLen ) = 0;
    virtual bool ServerIsInFavoriteList( S32 source, S32 n ) = 0;
};

extern idClientLANSystem* clientLANSystem;

#endif // !__CLIENTLAN_API_H__

