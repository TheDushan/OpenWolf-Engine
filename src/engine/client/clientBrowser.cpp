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
// File name:   clientBrowser.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

sint serverStatusCount;
ping_t cl_pinglist[MAX_PINGREQUESTS];
serverStatus_t cl_serverStatusList[MAX_SERVERSTATUSREQUESTS];

idClientBrowserSystemLocal clientBrowserLocal;

/*
===============
idClientBrowserSystemLocal::idClientBrowserSystemLocal
===============
*/
idClientBrowserSystemLocal::idClientBrowserSystemLocal(void) {
}

/*
===============
idClientBrowserSystemLocal::~idClientBrowserSystemLocal
===============
*/
idClientBrowserSystemLocal::~idClientBrowserSystemLocal(void) {
}

/*
===================
idClientBrowserSystemLocal::InitServerInfo
===================
*/
void idClientBrowserSystemLocal::InitServerInfo(serverInfo_t *server,
        netadr_t *address) {
    server->adr = *address;
    server->clients = 0;
    server->hostName[0] = '\0';
    server->mapName[0] = '\0';
    server->maxClients = 0;
    server->maxPing = 0;
    server->minPing = 0;
    server->ping = -1;
    server->game[0] = '\0';
    server->gameType = 0;
    server->netType = 0;
    server->allowAnonymous = 0;
}

/*
===================
idClientBrowserSystemLocal::ServersResponsePacket
===================
*/
void idClientBrowserSystemLocal::ServersResponsePacket(
    const netadr_t *from, msg_t *msg, bool extended) {
    sint i, j, count, total;
    netadr_t addresses[MAX_SERVERSPERPACKET];
    sint numservers;
    uchar8 *buffptr;
    uchar8 *buffend;

    Com_Printf("idClientBrowserSystemLocal::ServersResponsePacket from %s\n",
               networkSystem->AdrToString(*from));

    if(cls.numglobalservers == -1) {
        // state to detect lack of servers or lack of response
        cls.numglobalservers = 0;
        cls.numGlobalServerAddresses = 0;
    }

    // parse through server response string
    numservers = 0;
    buffptr = msg->data;
    buffend = buffptr + msg->cursize;

    // advance to initial token
    do {
        if(*buffptr == '\\' || (extended && *buffptr == '/')) {
            break;
        }

        buffptr++;
    } while(buffptr < buffend);

    while(buffptr + 1 < buffend) {
        // IPv4 address
        if(*buffptr == '\\') {
            buffptr++;

            if(buffend - buffptr < sizeof(addresses[numservers].ip) + sizeof(
                        addresses[numservers].port) + 1) {
                break;
            }

            for(i = 0; i < sizeof(addresses[numservers].ip); i++) {
                addresses[numservers].ip[i] = *buffptr++;
            }

            addresses[numservers].type = NA_IP;
        }
        // IPv6 address, if it's an extended response
        else if(extended && *buffptr == '/') {
            buffptr++;

            if(buffend - buffptr < sizeof(addresses[numservers].ip6) + sizeof(
                        addresses[numservers].port) + 1) {
                break;
            }

            for(i = 0; i < sizeof(addresses[numservers].ip6); i++) {
                addresses[numservers].ip6[i] = *buffptr++;
            }

            addresses[numservers].type = NA_IP6;
            addresses[numservers].scope_id = from->scope_id;
        } else {
            // syntax error!
            break;
        }

        // parse out port
        addresses[numservers].port = (*buffptr++) << 8;
        addresses[numservers].port += *buffptr++;
        addresses[numservers].port = BigShort(addresses[numservers].port);

        // syntax check
        if(*buffptr != '\\' && *buffptr != '/') {
            break;
        }

        numservers++;

        if(numservers >= MAX_SERVERSPERPACKET) {
            break;
        }
    }

    count = cls.numglobalservers;

    for(i = 0; i < numservers && count < MAX_GLOBAL_SERVERS; i++) {
        // build net address
        serverInfo_t *server = &cls.globalServers[count];

        for(j = 0; j < count; j++) {
            if(networkSystem->CompareAdr(cls.globalServers[j].adr, addresses[i])) {
                break;
            }
        }

        if(j < count) {
            continue;
        }

        InitServerInfo(server, &addresses[i]);

        // advance to next slot
        count++;
    }

    // if getting the global list
    if(count >= MAX_GLOBAL_SERVERS &&
            cls.numGlobalServerAddresses < MAX_GLOBAL_SERVERS) {
        // if we couldn't store the servers in the main list anymore
        for(; i < numservers &&
                cls.numGlobalServerAddresses < MAX_GLOBAL_SERVERS; i++) {
            // just store the addresses in an additional list
            cls.globalServerAddresses[cls.numGlobalServerAddresses++] = addresses[i];
        }
    }

    cls.numglobalservers = count;
    total = count + cls.numGlobalServerAddresses;

    Com_Printf("getserversResponse:%3d servers parsed (total %d)\n",
               numservers, total);
}

/*
===================
idClientBrowserSystemLocal::SetServerInfo
===================
*/
void idClientBrowserSystemLocal::SetServerInfo(serverInfo_t *server,
        pointer info, sint ping) {
    if(server) {
        if(info) {
            sint32 vL;
            valueType *p;

#define setserverinfo(field, sfield)    \
    vL = strtol(Info_ValueForKey(info, field), &p, 10); \
    if (vL < 0 || vL > INT_MAX || *p != 0) \
        server->sfield = 0; \
    else \
        server->sfield = (sint)vL

            Q_strncpyz(server->hostName, Info_ValueForKey(info, "hostname"),
                       sizeof(server->hostName));
            Q_strncpyz(server->mapName, Info_ValueForKey(info, "mapname"),
                       sizeof(server->mapName));
            Q_strncpyz(server->game, Info_ValueForKey(info, "game"),
                       sizeof(server->game));
            Q_strncpyz(server->gameName, Info_ValueForKey(info, "gamename"),
                       sizeof(server->gameName));

            setserverinfo("clients", clients);
            setserverinfo("serverload", load);
            setserverinfo("sv_maxclients", maxClients);
            setserverinfo("gametype", gameType);
            setserverinfo("nettype", netType);
            setserverinfo("minping", minPing);
            setserverinfo("maxping", maxPing);
            setserverinfo("sv_allowAnonymous", allowAnonymous);
            setserverinfo("friendlyFire", friendlyFire);
            setserverinfo("maxlives", maxlives);
            setserverinfo("needpass", needpass);
            setserverinfo("g_antilag", antilag);
            setserverinfo("weaprestrict", weaprestrict);
            setserverinfo("balancedteams", balancedteams);
        }

        server->ping = ping;
    }
}

/*
===================
idClientBrowserSystemLocal::SetServerInfoByAddress
===================
*/
void idClientBrowserSystemLocal::SetServerInfoByAddress(netadr_t from,
        pointer info, sint ping) {
    sint i;

    for(i = 0; i < MAX_OTHER_SERVERS; i++) {
        if(networkSystem->CompareAdr(from, cls.localServers[i].adr)) {
            SetServerInfo(&cls.localServers[i], info, ping);
        }
    }

    for(i = 0; i < MAX_GLOBAL_SERVERS; i++) {
        if(networkSystem->CompareAdr(from, cls.globalServers[i].adr)) {
            SetServerInfo(&cls.globalServers[i], info, ping);
        }
    }

    for(i = 0; i < MAX_OTHER_SERVERS; i++) {
        if(networkSystem->CompareAdr(from, cls.favoriteServers[i].adr)) {
            SetServerInfo(&cls.favoriteServers[i], info, ping);
        }
    }

}

/*
===================
idClientBrowserSystemLocal::ServerInfoPacket
===================
*/
void idClientBrowserSystemLocal::ServerInfoPacket(netadr_t from,
        msg_t *msg) {
    sint i, type;
    valueType info[MAX_INFO_STRING];
    valueType *str;
    valueType *infoString;
    sint prot;
    valueType *gameName;

    infoString = MSG_ReadString(msg);

    // if this isn't the correct protocol version, ignore it
    prot = atoi(Info_ValueForKey(infoString, "protocol"));

    if(prot != com_protocol->integer) {
        if(developer->integer) {
            Com_Printf("Different protocol info packet: %s\n", infoString);
        }

        return;
    }

    // Arnout: if this isn't the correct game, ignore it
    gameName = Info_ValueForKey(infoString, "gamename");

    if(!gameName[0] || Q_stricmp(gameName, GAMENAME_STRING)) {
        if(developer->integer) {
            Com_Printf("Different game info packet: %s\n", infoString);
        }

        return;
    }

    // iterate servers waiting for ping response
    for(i = 0; i < MAX_PINGREQUESTS; i++) {
        if(cl_pinglist[i].adr.port && !cl_pinglist[i].time &&
                networkSystem->CompareAdr(from, cl_pinglist[i].adr)) {
            // calc ping time
            cl_pinglist[i].time = cls.realtime - cl_pinglist[i].start + 1;

            if(developer->integer) {
                Com_Printf("ping time %dms from %s\n", cl_pinglist[i].time,
                           networkSystem->AdrToString(from));
            }

            // save of info
            Q_strncpyz(cl_pinglist[i].info, infoString, sizeof(cl_pinglist[i].info));

            // tack on the net type
            // NOTE: make sure these types are in sync with the netnames strings in the UI
            switch(from.type) {
                case NA_BROADCAST:
                case NA_IP:
                    str = "udp";
                    type = 1;
                    break;

                case NA_IP6:
                    type = 2;
                    break;

                default:
                    str = "???";
                    type = 0;
                    break;
            }

            Info_SetValueForKey(cl_pinglist[i].info, "nettype", va("%d", type));
            SetServerInfoByAddress(from, infoString, cl_pinglist[i].time);

            return;
        }
    }

    // if not just sent a local broadcast or pinging local servers
    if(cls.pingUpdateSource != AS_LOCAL) {
        return;
    }

    for(i = 0; i < MAX_OTHER_SERVERS; i++) {
        // empty slot
        if(cls.localServers[i].adr.port == 0) {
            break;
        }

        // avoid duplicate
        if(networkSystem->CompareAdr(from, cls.localServers[i].adr)) {
            return;
        }
    }

    if(i == MAX_OTHER_SERVERS) {
        if(developer->integer) {
            Com_Printf("MAX_OTHER_SERVERS hit, dropping infoResponse\n");
        }

        return;
    }

    // add this to the list
    cls.numlocalservers = i + 1;
    cls.localServers[i].adr = from;
    cls.localServers[i].clients = 0;
    cls.localServers[i].hostName[0] = '\0';
    cls.localServers[i].load = -1;
    cls.localServers[i].mapName[0] = '\0';
    cls.localServers[i].maxClients = 0;
    cls.localServers[i].maxPing = 0;
    cls.localServers[i].minPing = 0;
    cls.localServers[i].ping = -1;
    cls.localServers[i].game[0] = '\0';
    cls.localServers[i].gameType = 0;
    cls.localServers[i].netType = from.type;
    cls.localServers[i].allowAnonymous = 0;
    cls.localServers[i].friendlyFire = 0;
    cls.localServers[i].maxlives = 0;
    cls.localServers[i].needpass = 0;
    cls.localServers[i].antilag = 0;
    cls.localServers[i].weaprestrict = 0;
    cls.localServers[i].balancedteams = 0;
    cls.localServers[i].gameName[0] = '\0';

    Q_strncpyz(info, MSG_ReadString(msg), MAX_INFO_STRING);

    if(::strlen(info)) {
        if(info[::strlen(info) - 1] != '\n') {
            Q_strcat(info, sizeof(info), "\n");
        }

        Com_Printf("%s: %s", networkSystem->AdrToStringwPort(from), info);
    }
}

/*
===================
idClientBrowserSystemLocal::GetServerStatus
===================
*/
serverStatus_t *idClientBrowserSystemLocal::GetServerStatus(
    netadr_t from) {
    sint i, oldest, oldestTime;
    serverStatus_t *serverStatus;

    serverStatus = nullptr;

    for(i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
        if(networkSystem->CompareAdr(from, cl_serverStatusList[i].address)) {
            return &cl_serverStatusList[i];
        }
    }

    for(i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
        if(cl_serverStatusList[i].retrieved) {
            return &cl_serverStatusList[i];
        }
    }

    oldest = -1;
    oldestTime = 0;

    for(i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
        if(oldest == -1 || cl_serverStatusList[i].startTime < oldestTime) {
            oldest = i;
            oldestTime = cl_serverStatusList[i].startTime;
        }
    }

    if(oldest != -1) {
        return &cl_serverStatusList[oldest];
    }

    serverStatusCount++;

    return &cl_serverStatusList[serverStatusCount & (MAX_SERVERSTATUSREQUESTS -
                                                  1)];
}

/*
===================
idClientBrowserSystemLocal::ServerStatus
===================
*/
sint idClientBrowserSystemLocal::ServerStatus(valueType *serverAddress,
        valueType *serverStatusString, sint maxLen) {
    sint i;
    netadr_t to;
    serverStatus_t *serverStatus;

    // if no server address then reset all server status requests
    if(!serverAddress) {
        for(i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
            cl_serverStatusList[i].address.port = 0;
            cl_serverStatusList[i].retrieved = true;
        }

        return false;
    }

    // get the address
    if(!networkChainSystem->StringToAdr(serverAddress, &to, NA_UNSPEC)) {
        return false;
    }

    serverStatus = GetServerStatus(to);

    // if no server status string then reset the server status request for this address
    if(!serverStatusString) {
        serverStatus->retrieved = true;
        return false;
    }

    // if this server status request has the same address
    if(networkSystem->CompareAdr(to, serverStatus->address)) {
        // if we recieved an response for this server status request
        if(!serverStatus->pending) {
            Q_strncpyz(serverStatusString, serverStatus->string, maxLen);
            serverStatus->retrieved = true;
            serverStatus->startTime = 0;

            return true;
        }
        // resend the request regularly
        else if(serverStatus->startTime < idsystem->Milliseconds() -
                cl_serverStatusResendTime->integer) {
            serverStatus->print = false;
            serverStatus->pending = true;
            serverStatus->retrieved = false;
            serverStatus->time = 0;
            serverStatus->startTime = idsystem->Milliseconds();

            networkChainSystem->OutOfBandPrint(NS_CLIENT, to, "getstatus");
            return false;
        }
    }
    // if retrieved
    else if(serverStatus->retrieved) {
        serverStatus->address = to;
        serverStatus->print = false;
        serverStatus->pending = true;
        serverStatus->retrieved = false;
        serverStatus->startTime = idsystem->Milliseconds();
        serverStatus->time = 0;

        networkChainSystem->OutOfBandPrint(NS_CLIENT, to, "getstatus");
        return false;
    }

    return false;
}

/*
===================
idClientBrowserSystemLocal::ServerStatusResponse
===================
*/
void idClientBrowserSystemLocal::ServerStatusResponse(netadr_t from,
        msg_t *msg) {
    sint i, l, score, ping;
    uint64 len;
    valueType *s, info[MAX_INFO_STRING];
    serverStatus_t *serverStatus;

    serverStatus = nullptr;

    for(i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
        if(networkSystem->CompareAdr(from, cl_serverStatusList[i].address)) {
            serverStatus = &cl_serverStatusList[i];
            break;
        }
    }

    // if we didn't request this server status
    if(!serverStatus) {
        return;
    }

    s = MSG_ReadStringLine(msg);

    len = 0;
    Q_vsprintf_s(&serverStatus->string[len],
                 sizeof(serverStatus->string) - len, sizeof(serverStatus->string) - len,
                 "%s", s);

    if(serverStatus->print) {
        Com_Printf("Server settings:\n");

        // print cvars
        while(*s) {
            for(i = 0; i < 2 && *s; i++) {
                if(*s == '\\') {
                    s++;
                }

                l = 0;

                while(*s) {
                    info[l++] = *s;

                    if(l >= MAX_INFO_STRING - 1) {
                        break;
                    }

                    s++;

                    if(*s == '\\') {
                        break;
                    }
                }

                info[l] = '\0';

                if(i) {
                    Com_Printf("%s\n", info);
                } else {
                    Com_Printf("%-24s", info);
                }
            }
        }
    }

    len = ::strlen(serverStatus->string);
    Q_vsprintf_s(&serverStatus->string[len],
                 sizeof(serverStatus->string) - len, sizeof(serverStatus->string) - len,
                 "\\");

    if(serverStatus->print) {
        Com_Printf("\nPlayers:\n");
        Com_Printf("num: score: ping: name:\n");
    }

    for(i = 0, s = MSG_ReadStringLine(msg); *s;
            s = MSG_ReadStringLine(msg), i++) {

        len = ::strlen(serverStatus->string);
        Q_vsprintf_s(&serverStatus->string[len],
                     sizeof(serverStatus->string) - len, sizeof(serverStatus->string) - len,
                     "\\%s", s);

        if(serverStatus->print) {
            score = ping = 0;

            ::sscanf(s, "%d %d", &score, &ping);

            s = ::strchr(s, ' ');

            if(s) {
                s = ::strchr(s + 1, ' ');
            }

            if(s) {
                s++;
            } else {
                s = "unknown";
            }

            Com_Printf("%-2d   %-3d    %-3d   %s\n", i, score, ping, s);
        }
    }

    len = strlen(serverStatus->string);
    Q_vsprintf_s(&serverStatus->string[len],
                 sizeof(serverStatus->string) - len, sizeof(serverStatus->string) - len,
                 "\\");

    serverStatus->time = idsystem->Milliseconds();
    serverStatus->address = from;
    serverStatus->pending = false;

    if(serverStatus->print) {
        serverStatus->retrieved = true;
    }
}

/*
==================
idClientBrowserSystemLocal::LocalServers_f
==================
*/
void idClientBrowserSystemLocal::LocalServers(void) {
    valueType *message;
    sint i, j;
    netadr_t to;

    Com_Printf("Scanning for servers on the local network...\n");

    // reset the list, waiting for response
    cls.numlocalservers = 0;
    cls.pingUpdateSource = AS_LOCAL;

    for(i = 0; i < MAX_OTHER_SERVERS; i++) {
        bool b = cls.localServers[i].visible;
        ::memset(&cls.localServers[i], 0, sizeof(cls.localServers[i]));
        cls.localServers[i].visible = b;
    }

    ::memset(&to, 0, sizeof(to));

    // The 'xxx' in the message is a challenge that will be echoed back
    // by the server.  We don't care about that here, but master servers
    // can use that to prevent spoofed server responses from invalid ip
    message = "\377\377\377\377getinfo xxx";

    // send each message twice in case one is dropped
    for(i = 0; i < 2; i++) {
        // send a broadcast packet on each server port
        // we support multiple server ports so a single machine
        // can nicely run multiple servers
        for(j = 0; j < NUM_SERVER_PORTS; j++) {
            to.port = BigShort((schar16)(PORT_SERVER + j));

            to.type = NA_BROADCAST;
            networkChainSystem->SendPacket(NS_CLIENT, strlen(message), message, to);

            to.type = NA_MULTICAST6;
            networkChainSystem->SendPacket(NS_CLIENT, strlen(message), message, to);
        }
    }
}

/*
==================
idClientBrowserSystemLocal::GlobalServers
==================
*/
void idClientBrowserSystemLocal::GlobalServers(void) {
    sint count, i, masterNum;
    valueType command[1024], * masteraddress;
    netadr_t to;

    if((count = cmdSystem->Argc()) < 3 ||
            (masterNum = atoi(cmdSystem->Argv(1))) < 0 ||
            masterNum > MAX_MASTER_SERVERS) {
        Com_Printf("usage: globalservers <master# 0-%d> <protocol> [keywords]\n",
                   MAX_MASTER_SERVERS);
        return;
    }

    // request from all master servers
    if(masterNum == 0) {
        sint numAddress = 0;

        for(i = 1; i <= MAX_MASTER_SERVERS; i++) {
            Q_vsprintf_s(command, sizeof(command), sizeof(command), "sv_master%d", i);
            masteraddress = cvarSystem->VariableString(command);

            if(!*masteraddress) {
                continue;
            }

            numAddress++;

            Q_vsprintf_s(command, sizeof(command), sizeof(command),
                         "globalservers %d %s %s\n", i, cmdSystem->Argv(2), cmdSystem->ArgsFrom(3));
            cmdBufferSystem->AddText(command);
        }

        if(!numAddress) {
            Com_Printf("idClientBrowserSystemLocal::GlobalServers: Error: No master server addresses.\n");
        }

        return;
    }

    Q_vsprintf_s(command, sizeof(command), sizeof(command), "sv_master%d",
                 masterNum);
    masteraddress = cvarSystem->VariableString(command);

    if(!*masteraddress) {
        Com_Printf("idClientBrowserSystemLocal::GlobalServers: Error: No master server address given for %s.\n",
                   command);
        return;
    }

    // reset the list, waiting for response
    // -1 is used to distinguish a "no response"
    i = networkChainSystem->StringToAdr(masteraddress, &to, NA_UNSPEC);

    if(!i) {
        Com_Printf("idClientBrowserSystemLocal::GlobalServers: Error: could not resolve address of master %s\n",
                   masteraddress);
        return;
    } else if(i == 2) {
        to.port = BigShort(PORT_MASTER);
    }

    Com_Printf("Requesting servers from %s (%s)...\n", masteraddress,
               networkSystem->AdrToStringwPort(to));

    cls.numglobalservers = -1;
    cls.pingUpdateSource = AS_GLOBAL;

    // Use the extended query for IPv6 masters
    if(to.type == NA_IP6 || to.type == NA_MULTICAST6) {
        sint v4enabled = net_enabled->integer & NET_ENABLEV4;

        if(v4enabled) {
            Q_vsprintf_s(command, sizeof(command), sizeof(command),
                         "getserversExt %s %s", cl_gamename->string, cmdSystem->Argv(2));
        } else {
            Q_vsprintf_s(command, sizeof(command), sizeof(command),
                         "getserversExt %s %s ipv6", cl_gamename->string, cmdSystem->Argv(2));
        }
    } else {
        Q_vsprintf_s(command, sizeof(command), sizeof(command), "getservers %s %s",
                     cl_gamename->string, cmdSystem->Argv(2));
    }

    for(i = 3; i < count; i++) {
        Q_strcat(command, sizeof(command), " ");
        Q_strcat(command, sizeof(command), cmdSystem->Argv(i));
    }

    networkChainSystem->OutOfBandPrint(NS_SERVER, to, "%s", command);
}

/*
==================
idClientBrowserSystemLocal::GetPing
==================
*/
void idClientBrowserSystemLocal::GetPing(sint n, valueType *buf,
        uint64 buflen, sint *pingtime) {
    sint time, maxPing;
    pointer str;

    if(n < 0 || n >= MAX_PINGREQUESTS || !cl_pinglist[n].adr.port) {
        // empty slot
        buf[0] = '\0';
        *pingtime = 0;

        return;
    }

    str = networkSystem->AdrToStringwPort(cl_pinglist[n].adr);
    Q_strncpyz(buf, str, buflen);

    time = cl_pinglist[n].time;

    if(!time) {
        // check for timeout
        time = cls.realtime - cl_pinglist[n].start;
        maxPing = cvarSystem->VariableIntegerValue("cl_maxPing");

        if(maxPing < 100) {
            maxPing = 100;
        }

        if(time < maxPing) {
            // not timed out yet
            time = 0;
        }
    }

    SetServerInfoByAddress(cl_pinglist[n].adr, cl_pinglist[n].info,
                           cl_pinglist[n].time);

    *pingtime = time;
}

/*
==================
idClientBrowserSystemLocal::GetPingInfo
==================
*/
void idClientBrowserSystemLocal::GetPingInfo(sint n, valueType *buf,
        uint64 buflen) {
    if(n < 0 || n >= MAX_PINGREQUESTS || !cl_pinglist[n].adr.port) {
        // empty slot
        if(buflen) {
            buf[0] = '\0';
        }

        return;
    }

    Q_strncpyz(buf, cl_pinglist[n].info, buflen);
}

/*
==================
idClientBrowserSystemLocal::ClearPing
==================
*/
void idClientBrowserSystemLocal::ClearPing(sint n) {
    if(n < 0 || n >= MAX_PINGREQUESTS) {
        return;
    }

    cl_pinglist[n].adr.port = 0;
}

/*
==================
idClientBrowserSystemLocal::GetPingQueueCount
==================
*/
sint idClientBrowserSystemLocal::GetPingQueueCount(void) {
    sint i, count;
    ping_t *pingptr;

    count = 0;
    pingptr = cl_pinglist;

    for(i = 0; i < MAX_PINGREQUESTS; i++, pingptr++) {
        if(pingptr->adr.port) {
            count++;
        }
    }

    return (count);
}

/*
==================
idClientBrowserSystemLocal::GetFreePing
==================
*/
ping_t *idClientBrowserSystemLocal::GetFreePing(void) {
    sint i, oldest, time;
    ping_t *pingptr, *best;

    pingptr = cl_pinglist;

    for(i = 0; i < MAX_PINGREQUESTS; i++, pingptr++) {
        // find free ping slot
        if(pingptr->adr.port) {
            if(!pingptr->time) {
                if(cls.realtime - pingptr->start < 500) {
                    // still waiting for response
                    continue;
                }
            } else if(pingptr->time < 500) {
                // results have not been queried
                continue;
            }
        }

        // clear it
        pingptr->adr.port = 0;
        return (pingptr);
    }

    // use oldest entry
    pingptr = cl_pinglist;
    best = cl_pinglist;
    oldest = INT_MIN;

    for(i = 0; i < MAX_PINGREQUESTS; i++, pingptr++) {
        // scan for oldest
        time = cls.realtime - pingptr->start;

        if(time > oldest) {
            oldest = time;
            best = pingptr;
        }
    }

    return (best);
}

/*
==================
idClientBrowserSystemLocal::Ping
==================
*/
void idClientBrowserSystemLocal::Ping(void) {
    sint argc;
    valueType *server;
    netadr_t to;
    ping_t *pingptr;
    netadrtype_t family = NA_UNSPEC;

    argc = cmdSystem->Argc();

    if(argc != 2 && argc != 3) {
        Com_Printf("usage: ping [-4|-6] server\n");
        return;
    }

    if(argc == 2) {
        server = cmdSystem->Argv(1);
    } else {
        if(!strcmp(cmdSystem->Argv(1), "-4")) {
            family = NA_IP;
        } else if(!strcmp(cmdSystem->Argv(1), "-6")) {
            family = NA_IP6;
        } else {
            Com_Printf("warning: only -4 or -6 as address type understood.\n");
        }

        server = cmdSystem->Argv(2);
    }

    ::memset(&to, 0, sizeof(netadr_t));

    if(!networkChainSystem->StringToAdr(server, &to, family)) {
        return;
    }

    pingptr = GetFreePing();

    ::memcpy(&pingptr->adr, &to, sizeof(netadr_t));
    pingptr->start = cls.realtime;
    pingptr->time = 0;

    SetServerInfoByAddress(pingptr->adr, nullptr, 0);

    networkChainSystem->OutOfBandPrint(NS_CLIENT, to, "getinfo xxx");
}

/*
==================
idClientBrowserSystemLocal::UpdateVisiblePings
==================
*/
bool idClientBrowserSystemLocal::UpdateVisiblePings(sint source) {
    sint slots, i, pingTime, max;
    valueType buff[MAX_STRING_CHARS];
    bool status = false;

    if(source < 0 || source > AS_FAVORITES) {
        return false;
    }

    cls.pingUpdateSource = source;

    slots = GetPingQueueCount();

    if(slots < MAX_PINGREQUESTS) {
        serverInfo_t *server = nullptr;

        max = (source == AS_GLOBAL) ? MAX_GLOBAL_SERVERS : MAX_OTHER_SERVERS;

        switch(source) {
            case AS_LOCAL:
                server = &cls.localServers[0];
                max = cls.numlocalservers;
                break;

            case AS_GLOBAL:
                server = &cls.globalServers[0];
                max = cls.numglobalservers;
                break;

            case AS_FAVORITES:
                server = &cls.favoriteServers[0];
                max = cls.numfavoriteservers;
                break;
        }

        for(i = 0; i < max; i++) {
            if(server[i].visible) {
                if(server[i].ping == -1) {
                    sint j;

                    if(slots >= MAX_PINGREQUESTS) {
                        break;
                    }

                    for(j = 0; j < MAX_PINGREQUESTS; j++) {
                        if(!cl_pinglist[j].adr.port) {
                            continue;
                        }

                        if(networkSystem->CompareAdr(cl_pinglist[j].adr, server[i].adr)) {
                            // already on the list
                            break;
                        }
                    }

                    if(j >= MAX_PINGREQUESTS) {
                        status = true;

                        for(j = 0; j < MAX_PINGREQUESTS; j++) {
                            if(!cl_pinglist[j].adr.port) {
                                break;
                            }
                        }

                        if(j < MAX_PINGREQUESTS) {

                            ::memcpy(&cl_pinglist[j].adr, &server[i].adr, sizeof(netadr_t));
                            cl_pinglist[j].start = cls.realtime;
                            cl_pinglist[j].time = 0;
                            networkChainSystem->OutOfBandPrint(NS_CLIENT, cl_pinglist[j].adr,
                                                               "getinfo xxx");
                            slots++;
                        }
                    }
                }
                // if the server has a ping higher than cl_maxPing or
                // the ping packet got lost
                else if(server[i].ping == 0) {
                    // if we are updating global servers
                    if(source == AS_GLOBAL) {
                        //
                        if(cls.numGlobalServerAddresses > 0) {
                            // overwrite this server with one from the additional global servers
                            cls.numGlobalServerAddresses--;
                            InitServerInfo(&server[i],
                                           &cls.globalServerAddresses[cls.numGlobalServerAddresses]);
                            // NOTE: the server[i].visible flag stays untouched
                        }
                    }
                }
            }
        }
    }

    if(slots) {
        status = true;
    }

    for(i = 0; i < MAX_PINGREQUESTS; i++) {
        if(!cl_pinglist[i].adr.port) {
            continue;
        }

        GetPing(i, buff, MAX_STRING_CHARS, &pingTime);

        if(pingTime != 0) {
            ClearPing(i);
            status = true;
        }
    }

    return status;
}

/*
==================
idClientBrowserSystemLocal::ServerStatus
==================
*/
void idClientBrowserSystemLocal::ServerStatus(void) {
    sint argc;
    valueType *server;
    netadr_t to, * toptr = nullptr;
    serverStatus_t *serverStatus;
    netadrtype_t family = NA_UNSPEC;

    argc = cmdSystem->Argc();

    if(argc != 2 && argc != 3) {
        if(cls.state != CA_ACTIVE || clc.demoplaying) {
            Com_Printf("Not connected to a server.\n");
            Com_Printf("usage: serverstatus [-4|-6] server\n");

            return;
        }

        toptr = &clc.serverAddress;
    }

    if(!toptr) {
        ::memset(&to, 0, sizeof(netadr_t));

        if(argc == 2) {
            server = cmdSystem->Argv(1);
        } else {
            if(!strcmp(cmdSystem->Argv(1), "-4")) {
                family = NA_IP;
            } else if(!strcmp(cmdSystem->Argv(1), "-6")) {
                family = NA_IP6;
            } else {
                Com_Printf("warning: only -4 or -6 as address type understood.\n");
            }

            server = cmdSystem->Argv(2);
        }

        toptr = &to;

        if(!networkChainSystem->StringToAdr(server, toptr, family)) {
            return;
        }
    }

    networkChainSystem->OutOfBandPrint(NS_CLIENT, *toptr, "getstatus");

    serverStatus = GetServerStatus(*toptr);
    serverStatus->address = *toptr;
    serverStatus->print = true;
    serverStatus->pending = true;
}

/*
==================
idClientBrowserSystemLocal::ShowIP
==================
*/
void idClientBrowserSystemLocal::ShowIP(void) {
    networkSystem->ShowIP();
}
