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

#ifndef __CLIENTBROWSER_LOCAL_H__
#define __CLIENTBROWSER_LOCAL_H__

typedef struct serverStatus_s
{
    S32 time, startTime;
    UTF8 string[BIG_INFO_STRING];
    bool pending;
    bool print;
    bool retrieved;
    netadr_t address;
} serverStatus_t;

#define MAX_SERVERSPERPACKET 256

extern convar_t* cl_serverStatusResendTime;
extern convar_t* cl_gamename;

//
// idClientBrowserSystemLocal
//
class idClientBrowserSystemLocal
{
public:
    idClientBrowserSystemLocal();
    ~idClientBrowserSystemLocal();
    
    static void InitServerInfo( serverInfo_t* server, netadr_t* address );
    static void ServersResponsePacket( const netadr_t* from, msg_t* msg, bool extended );
    static void SetServerInfo( serverInfo_t* server, StringEntry info, S32 ping );
    static void SetServerInfoByAddress( netadr_t from, StringEntry info, S32 ping );
    static void ServerInfoPacket( netadr_t from, msg_t* msg );
    static serverStatus_t* GetServerStatus( netadr_t from );
    static S32 ServerStatus( UTF8* serverAddress, UTF8* serverStatusString, S32 maxLen );
    static void ServerStatusResponse( netadr_t from, msg_t* msg );
    static void LocalServers( void );
    static void GlobalServers( void );
    static void GetPing( S32 n, UTF8* buf, S32 buflen, S32* pingtime );
    static void GetPingInfo( S32 n, UTF8* buf, S32 buflen );
    static void ClearPing( S32 n );
    static S32 GetPingQueueCount( void );
    static ping_t* GetFreePing( void );
    static void Ping( void );
    static bool UpdateVisiblePings( S32 source );
    static void ServerStatus( void );
    static void ShowIP( void );
};

extern idClientBrowserSystemLocal clientBrowserLocal;

#endif // !__CLIENTBROWSER_LOCAL_H__