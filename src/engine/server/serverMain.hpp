////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2023 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   serverMain.hpp
// Created:     12/26/2018
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SERVERMAIN_HPP__
#define __SERVERMAIN_HPP__

#define HEARTBEAT_MSEC  50 * 1000

//Dushan - I will soon move this in the appConfig.hpp
#define HEARTBEAT_GAME  "StellarPrey-1"
#define HEARTBEAT_DEAD  "StellarPreyFlatline-1"

static sint lastTimeResolve[MAX_MASTER_SERVERS];

//
// idServerGameSystemLocal
//
class idServerMainSystemLocal : public idServerMainSystem {
public:
    virtual void AddServerCommand(client_t *client, pointer cmd);
    virtual void SendServerCommand(client_t *cl, pointer fmt, ...);
    virtual void MasterShutdown(void);
    virtual void MasterGameCompleteStatus(void);
    virtual void MasterGameStat(pointer data);
    virtual void PacketEvent(netadr_t from, msg_t *msg);
    virtual void Frame(sint msec);
    virtual sint LoadTag(pointer mod_name);
    virtual sint RateMsec(client_t *client);
    virtual sint SendQueuedPackets(void);

public:

    idServerMainSystemLocal();
    ~idServerMainSystemLocal();

    static void IntegerOverflowShutDown(valueType *msg);
    static valueType *ExpandNewlines(valueType *in);
    static void MasterHeartbeat(pointer hbname);
    static bool VerifyChallenge(valueType *challenge);
    static void Status(netadr_t from);
    static void GameCompleteStatus(netadr_t from);
    static void Info(netadr_t from);
    static bool CheckDRDoS(netadr_t from);
    static void RemoteCommand(netadr_t from, msg_t *msg);
    static void ConnectionlessPacket(netadr_t from, msg_t *msg);
    static void CalcPings(void);
    static void CheckTimeouts(void);
    static bool CheckPaused(void);
    static void GetUpdateInfo(netadr_t from);
    static void CheckCvars(void);

    static void FlushRedirect(valueType *outputbuf);

private:
    static const sint IPUDP_HEADER_SIZE = 28;
    static const sint IP6UDP_HEADER_SIZE = 48;
};

extern idServerMainSystemLocal serverMainSystemLocal;

#endif //!__SERVERMAIN_HPP__
