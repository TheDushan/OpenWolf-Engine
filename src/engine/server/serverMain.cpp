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
// File name:   serverMain.cpp
// Created:     12/26/2018
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <server/serverAutoPrecompiled.hpp>
#elif DEDICATED
#include <server/serverDedPrecompiled.hpp>
#else
#include <framework/precompiled.hpp>
#endif

serverStatic_t svs;         // persistant server info
server_t sv;                // local server
void  *gvm = nullptr;       // game

#if defined (UPDATE_SERVER)
versionMapping_t versionMap[MAX_UPDATE_VERSIONS];
sint numVersions = 0;
#endif

serverRconPassword_t rconWhitelist[MAXIMUM_RCON_WHITELIST];
sint rconWhitelistCount = 0;

#define LL( x ) x = LittleLong( x )

/*
=============================================================================
EVENT MESSAGES
=============================================================================
*/

idServerMainSystemLocal serverMainSystemLocal;
idServerMainSystem *serverMainSystem = &serverMainSystemLocal;


/*
===============
idServerMainSystemLocal::idServerMainSystemLocal
===============
*/
idServerMainSystemLocal::idServerMainSystemLocal(void) {
}

/*
===============
idServerMainSystemLocal::~idServerMainSystemLocal
===============
*/
idServerMainSystemLocal::~idServerMainSystemLocal(void) {
}

/*
===============
idServerMainSystemLocal::ExpandNewlines

Converts newlines to "\n" so a line prints nicer
===============
*/
valueType *idServerMainSystemLocal::ExpandNewlines(valueType *in) {
    sint l;
    static valueType string[1024];

    l = 0;

    while(*in && l < sizeof(string) - 3) {
        if(*in == '\n') {
            string[l++] = '\\';
            string[l++] = 'n';
        } else {
            // NERVE - SMF - HACK - strip out localization tokens before string command is displayed in syscon window
            if(!Q_strncmp(in, "[lon]", 5) || !Q_strncmp(in, "[lof]", 5)) {
                in += 5;
                continue;
            }

            string[l++] = *in;
        }

        in++;
    }

    string[l] = 0;

    return string;
}

/*
======================
idServerMainSystemLocal::AddServerCommand

The given command will be transmitted to the client, and is guaranteed to
not have future snapshot_t executed before it is executed
======================
*/
void idServerMainSystemLocal::AddServerCommand(client_t *client,
        pointer cmd) {
    sint index, i;

    if(!client) {
        return;
    }

    // do not send commands until the gamestate has been sent
    if(client->state < CS_PRIMED) {
        return;
    }

    client->reliableSequence++;

    // if we would be losing an old command that hasn't been acknowledged,
    // we must drop the connection
    // we check == instead of >= so a broadcast print added by SV_DropClient()
    // doesn't cause a recursive drop client
    if(client->reliableSequence - client->reliableAcknowledge ==
            MAX_RELIABLE_COMMANDS + 1) {
        common->Printf("===== pending server commands =====\n");

        for(i = client->reliableAcknowledge + 1; i <= client->reliableSequence;
                i++) {
            common->Printf("cmd %5d: %s\n", i,
                           client->reliableCommands[i & (MAX_RELIABLE_COMMANDS - 1)]);
        }

        common->Printf("cmd %5d: %s\n", i, cmd);

        serverClientSystem->DropClient(client, "Server command overflow");
        return;
    }

    index = client->reliableSequence & (MAX_RELIABLE_COMMANDS - 1);
    Q_strncpyz(client->reliableCommands[index], cmd,
               sizeof(client->reliableCommands[index]));
}

/*
=================
idServerMainSystemLocal::SendServerCommand

Sends a reliable command string to be interpreted by
the client game module: "cp", "print", "chat", etc
A nullptr client will broadcast to all clients
=================
*/
void idServerMainSystemLocal::SendServerCommand(client_t *cl, pointer fmt,
        ...) {
    sint j;
    uchar8 message[MAX_MSGLEN];
    va_list argptr;
    client_t *client;

    va_start(argptr, fmt);
    Q_vsprintf_s(reinterpret_cast<valueType *>(message), sizeof(message), fmt,
                 argptr);
    va_end(argptr);

    // do not forward server command messages that would be too big to clients
    // ( q3infoboom / q3msgboom stuff )
    if(::strlen(reinterpret_cast<valueType *>(message)) > 1022) {
        return;
    }

    if(cl != nullptr) {

        if(idServerCommunityServer::ProcessServerCmd(reinterpret_cast<valueType *>
                (message))) {
            return;
        }

        AddServerCommand(cl, reinterpret_cast<valueType *>(message));
        return;
    }

    if(::strstr(reinterpret_cast<valueType *>(message), "limit hit") ||
            strstr(reinterpret_cast<valueType *>(message), "hit the")) {
        community_stats.mapstatus = 2;
    }

    // hack to echo broadcast prints to console
    if(dedicated->integer &&
            !strncmp(reinterpret_cast<valueType *>(message), "print", 5)) {
        common->Printf("broadcast: %s\n",
                       ExpandNewlines(reinterpret_cast<valueType *>(message)));
    }

    // send the data to all relevent clients
    for(j = 0, client = svs.clients; j < sv_maxclients->integer;
            j++, client++) {
        if(client->state < CS_PRIMED) {
            continue;
        }

        // Ridah, don't need to send messages to AI
        if(client->gentity && client->gentity->r.svFlags & SVF_BOT) {
            continue;
        }

        // done.

        AddServerCommand(client, reinterpret_cast<valueType *>(message));
    }
}

/*
==============================================================================
MASTER SERVER FUNCTIONS
==============================================================================
*/

/*
===============
idServerMainSystemLocal::MasterHeartbeat

Send a message to the masters every few minutes to
let it know we are alive, and log information.
We will also have a heartbeat sent when a server
changes from empty to non-empty, and full to non-full,
but not on every player enter or exit.
===============
*/
void idServerMainSystemLocal::MasterHeartbeat(pointer hbname) {
    static netadr_t
    adr[MAX_MASTER_SERVERS][2]; // [2] for v4 and v6 address for the same address string.
    sint             i;
    sint             res;
    sint             netenabled;
    valueType *master;

    // Update Server doesn't send heartbeat
#if defined (UPDATE_SERVER)
    return;
#endif

    if(serverGameSystem->GameIsSinglePlayer()) {
        // no heartbeats for SP
        return;
    }

    netenabled = net_enabled->integer;;

    // "dedicated 1" is for lan play, "dedicated 2" is for inet public play
    if(!dedicated || dedicated->integer != 2 ||
            !(netenabled & (NET_ENABLEV6 | NET_ENABLEV4))) {
        return;     // only dedicated servers send heartbeats
    }

    // if not time yet, don't send anything
    if(svs.time < svs.nextHeartbeatTime) {
        return;
    }

    svs.nextHeartbeatTime = svs.time + HEARTBEAT_MSEC;

    // send to group masters
    for(i = 0; i < MAX_MASTER_SERVERS; i++) {
        master = cvarSystem->VariableString(va("sv_master%i", i + 1));

        if(master[0] == '\0') {
            continue;
        }

        // see if we haven't already resolved the name
        if(netenabled & NET_ENABLEV4) {
            if(adr[i][0].type == NA_BAD) {
                common->Printf("Resolving %s (IPv4)\n", master);
                res = networkChainSystem->StringToAdr(master, &adr[i][0], NA_IP);

                if(res == 2) {
                    // if no port was specified, use the default master port
                    adr[i][0].port = BigShort(PORT_MASTER);
                }

                if(res) {
                    common->Printf("%s resolved to %s\n", master,
                                   networkSystem->AdrToString(adr[i][0]));
                } else {
                    common->Printf("%s has no IPv4 address.\n", master);
                }
            }
        }

        if(netenabled & NET_ENABLEV6) {
            if(adr[i][1].type == NA_BAD) {
                common->Printf("Resolving %s (IPv6)\n", master);
                res = networkChainSystem->StringToAdr(master, &adr[i][1], NA_IP6);

                if(res == 2) {
                    // if no port was specified, use the default master port
                    adr[i][1].port = BigShort(PORT_MASTER);
                }

                if(res) {
                    common->Printf("%s resolved to %s\n", master,
                                   networkSystem->AdrToString(adr[i][1]));
                } else {
                    common->Printf("%s has no IPv6 address.\n", master);
                }
            }
        }

        if((((netenabled & NET_ENABLEV4) && adr[i][0].type == NA_BAD) ||
                !(netenabled & NET_ENABLEV4))
                && (((netenabled & NET_ENABLEV6) && adr[i][1].type == NA_BAD) ||
                    !(netenabled & NET_ENABLEV6))) {
            // if the address failed to resolve, clear it
            // so we don't take repeated dns hits
            common->Printf("Couldn't resolve address: %s\n", master);
            cvarSystem->Set(va("sv_master%i", i + 1), "");
            continue;
        }

        common->Printf("Sending heartbeat to %s\n", master);

        // this command should be changed if the server info / status format
        // ever incompatably changes

        if((netenabled & NET_ENABLEV4) && adr[i][0].type != NA_BAD) {
            networkChainSystem->OutOfBandPrint(NS_SERVER, adr[i][0], "heartbeat %s\n",
                                               hbname);
        }

        if(netenabled & NET_ENABLEV6 && adr[i][1].type != NA_BAD) {
            networkChainSystem->OutOfBandPrint(NS_SERVER, adr[i][1], "heartbeat %s\n",
                                               hbname);
        }

    }
}

/*
=================
idServerMainSystemLocal::MasterGameCompleteStatus

NERVE - SMF - Sends gameCompleteStatus messages to all master servers
=================
*/
void idServerMainSystemLocal::MasterGameCompleteStatus(void) {
    sint i;
    static netadr_t adr[MAX_MASTER_SERVERS];

    // "dedicated 1" is for lan play, "dedicated 2" is for inet public play
    if(!dedicated || dedicated->integer != 2) {
        // only dedicated servers send master game status
        return;
    }

    // send to group masters
    for(i = 0 ; i < MAX_MASTER_SERVERS ; i++) {
        if(!sv_master[i]->string[0]) {
            continue;
        }

        // see if we haven't already resolved the name
        // resolving usually causes hitches on win95, so only
        // do it when needed
        if(sv_master[i]->modified) {
            sv_master[i]->modified = false;

            common->Printf("Resolving %s\n", sv_master[i]->string);

            if(!networkChainSystem->StringToAdr(sv_master[i]->string, &adr[i],
                                                NA_IP)) {
                // if the address failed to resolve, clear it
                // so we don't take repeated dns hits
                common->Printf("Couldn't resolve address: %s\n", sv_master[i]->string);
                cvarSystem->Set(sv_master[i]->name, "");
                sv_master[i]->modified = false;
                continue;
            }

            if(!::strchr(sv_master[i]->string, ':')) {
                adr[i].port = BigShort(PORT_MASTER);
            }

            common->Printf("%s resolved to %i.%i.%i.%i:%i\n", sv_master[i]->string,
                           adr[i].ip[0], adr[i].ip[1], adr[i].ip[2], adr[i].ip[3],
                           BigShort(adr[i].port));
        }

        common->Printf("Sending gameCompleteStatus to %s\n", sv_master[i]->string);

        // this command should be changed if the server info / status format
        // ever incompatably changes
        GameCompleteStatus(adr[i]);
    }
}

/*
=================
idServerMainSystemLocal::MasterShutdown

Informs all masters that this server is going down
=================
*/
void idServerMainSystemLocal::MasterShutdown(void) {
    // send a hearbeat right now
    svs.nextHeartbeatTime = -9999;
    MasterHeartbeat(HEARTBEAT_DEAD);     // NERVE - SMF - changed to flatline

    // send it again to minimize chance of drops
    //svs.nextHeartbeatTime = -9999;
    //MasterHeartbeat( HEARTBEAT_DEAD );

    // when the master tries to poll the server, it won't respond, so
    // it will be removed from the list
}


/*
=================
idServerMainSystemLocal::MasterGameStat
=================
*/
void idServerMainSystemLocal::MasterGameStat(pointer data) {
    netadr_t adr;

    if(!dedicated || dedicated->integer != 2) {
        return; // only dedicated servers send stats
    }

    common->Printf("Resolving %s\n", MASTER_SERVER_NAME);

    switch(networkChainSystem->StringToAdr(MASTER_SERVER_NAME, &adr,
                                           NA_UNSPEC)) {
        case 0:
            common->Printf("Couldn't resolve master address: %s\n",
                           MASTER_SERVER_NAME);
            return;

        case 2:
            adr.port = BigShort(PORT_MASTER);

        default:
            break;
    }

    common->Printf("%s resolved to %s\n", MASTER_SERVER_NAME,
                   networkSystem->AdrToStringwPort(adr));

    common->Printf("Sending gamestat to %s\n", MASTER_SERVER_NAME);
    networkChainSystem->OutOfBandPrint(NS_SERVER, adr, "gamestat %s", data);
}

/*
==============================================================================

CONNECTIONLESS COMMANDS

==============================================================================
*/

//bani - bugtraq 12534
//returns true if valid challenge
//returns false if m4d h4x0rz
/*
===============
idServerMainSystemLocal::VerifyChallenge
===============
*/
bool idServerMainSystemLocal::VerifyChallenge(valueType *challenge) {
    sint i, j;

    if(!challenge) {
        return false;
    }

    j = strlen(challenge);

    if(j > 64) {
        return false;
    }

    for(i = 0; i < j; i++) {
        if(challenge[i] == '\\' || challenge[i] == '/' || challenge[i] == '%' ||
                challenge[i] == ';' || challenge[i] == '"' || challenge[i] < 32 ||
                /*// non-ascii */ challenge[i] > 126) { // non-ascii
            return false;
        }
    }

    return true;
}

/*
================
idServerMainSystemLocal::Status

Responds with all the info that qplug or qspy can see about the server
and all connected players.  Used for getting detailed information after
the simple info query.
================
*/
void idServerMainSystemLocal::Status(netadr_t from) {
    sint i;
    uint64 statusLength, playerLength;
    valueType player[1024], status[MAX_MSGLEN], infostring[MAX_INFO_STRING];
    client_t *cl;
    playerState_t *ps;

    // ignore if we are in single player
    if(serverGameSystem->GameIsSinglePlayer()) {
        return;
    }

    //bani - bugtraq 12534
    if(!VerifyChallenge(cmdSystem->Argv(1))) {
        return;
    }

#if defined (UPDATE_SERVER)
    return;
#endif

    Q_strcpy_s(infostring, cvarSystem->InfoString(CVAR_SERVERINFO |
               CVAR_SERVERINFO_NOUPDATE));

    // echo back the parameter to status. so master servers can use it as a challenge
    // to prevent timed spoofed reply packets that add ghost servers
    Info_SetValueForKey(infostring, "challenge", cmdSystem->Argv(1));

    // add "demo" to the sv_keywords if restricted
    if(cvarSystem->VariableValue("fs_restrict")) {
        valueType keywords[MAX_INFO_STRING];

        Q_vsprintf_s(keywords, sizeof(keywords), sizeof(keywords), "ettest %s",
                     Info_ValueForKey(infostring, "sv_keywords"));
        Info_SetValueForKey(infostring, "sv_keywords", keywords);
    } else {
        // echo back the parameter to status. so master servers can use it as a challenge
        // to prevent timed spoofed reply packets that add ghost servers
        Info_SetValueForKey(infostring, "challenge", cmdSystem->Argv(1));
    }

    status[0] = 0;
    statusLength = 0;

    for(i = 0; i < sv_maxclients->integer; i++) {
        cl = &svs.clients[i];

        if(cl->state >= CS_CONNECTED) {
            ps = serverGameSystem->GameClientNum(i);
            Q_vsprintf_s(player, sizeof(player), sizeof(player), "%i %i \"%s\"\n",
                         ps->persistant[PERS_SCORE], cl->ping, cl->name);
            playerLength = strlen(player);

            if(statusLength + playerLength >= sizeof(status)) {
                // can't hold any more
                break;
            }

            ::strcpy(status + statusLength, player);

            statusLength += playerLength;
        }
    }

    networkChainSystem->OutOfBandPrint(NS_SERVER, from,
                                       "statusResponse\n%s\n%s", infostring, status);
}

/*
=================
idServerMainSystemLocal::GameCompleteStatus

NERVE - SMF - Send serverinfo cvars, etc to master servers when
game complete. Useful for tracking global player stats.
=================
*/
void idServerMainSystemLocal::GameCompleteStatus(netadr_t from) {
    sint i, statusLength, playerLength;
    valueType player[1024], status[MAX_MSGLEN], infostring[MAX_INFO_STRING];
    client_t *cl;
    playerState_t *ps;

    // ignore if we are in single player
    if(serverGameSystem->GameIsSinglePlayer()) {
        return;
    }

    //bani - bugtraq 12534
    if(!VerifyChallenge(cmdSystem->Argv(1))) {
        return;
    }

    Q_strcpy_s(infostring, cvarSystem->InfoString(CVAR_SERVERINFO |
               CVAR_SERVERINFO_NOUPDATE));

    // echo back the parameter to status. so master servers can use it as a challenge
    // to prevent timed spoofed reply packets that add ghost servers
    Info_SetValueForKey(infostring, "challenge", cmdSystem->Argv(1));

    // add "demo" to the sv_keywords if restricted
    if(cvarSystem->VariableValue("fs_restrict")) {
        valueType keywords[MAX_INFO_STRING];

        Q_vsprintf_s(keywords, sizeof(keywords), sizeof(keywords), "ettest %s",
                     Info_ValueForKey(infostring, "sv_keywords"));
        Info_SetValueForKey(infostring, "sv_keywords", keywords);
    }

    status[0] = 0;
    statusLength = 0;

    for(i = 0; i < sv_maxclients->integer; i++) {
        cl = &svs.clients[i];

        if(cl->state >= CS_CONNECTED) {
            ps = serverGameSystem->GameClientNum(i);

            Q_vsprintf_s(player, sizeof(player), sizeof(player), "%i %i \"%s\"\n",
                         ps->persistant[PERS_SCORE], cl->ping, cl->name);

            playerLength = strlen(player);

            if(statusLength + playerLength >= sizeof(status)) {
                // can't hold any more
                break;
            }

            ::strcpy(status + statusLength, player);
            statusLength += playerLength;
        }
    }

    networkChainSystem->OutOfBandPrint(NS_SERVER, from,
                                       "gameCompleteStatus\n%s\n%s", infostring, status);
}

/*
================
idServerMainSystemLocal::Info

Responds with a short info message that should be enough to determine
if a user is interested in a server to do a full status
================
*/
void idServerMainSystemLocal::Info(netadr_t from) {
    sint i, count;
    valueType *gamedir, infostring[MAX_INFO_STRING], *antilag, *weaprestrict,
              *balancedteams;

    // ignore if we are in single player
    if(serverGameSystem->GameIsSinglePlayer()) {
        return;
    }

    //bani - bugtraq 12534
    if(!VerifyChallenge(cmdSystem->Argv(1))) {
        return;
    }

    /*
     * Check whether cmdSystem->Argv(1) has a sane length. This was not done in the original Quake3 version which led
     * to the Infostring bug discovered by Luigi Auriemma. See http://aluigi.altervista.org/ for the advisory.
    */
    // A maximum challenge length of 128 should be more than plenty.
    if(::strlen(cmdSystem->Argv(1)) > 128) {
        return;
    }

#if defined (UPDATE_SERVER)
    return;
#endif

    // don't count privateclients
    count = 0;

    for(i = 0; i < sv_maxclients->integer; i++) {
        if(svs.clients[i].state >= CS_CONNECTED) {
            count++;
        }
    }

    infostring[0] = 0;

    // echo back the parameter to status. so servers can use it as a challenge
    // to prevent timed spoofed reply packets that add ghost servers
    Info_SetValueForKey(infostring, "challenge", cmdSystem->Argv(1));
    Info_SetValueForKey(infostring, "protocol", va("%i",
                        com_protocol->integer));
    Info_SetValueForKey(infostring, "hostname", sv_hostname->string);
    Info_SetValueForKey(infostring, "serverload", va("%i", svs.serverLoad));
    Info_SetValueForKey(infostring, "mapname", sv_mapname->string);
    Info_SetValueForKey(infostring, "clients", va("%i", count));
    Info_SetValueForKey(infostring, "sv_maxclients", va("%i",
                        sv_maxclients->integer - sv_privateClients->integer));
    //Info_SetValueForKey( infostring, "gametype", va("%i", sv_gametype->integer ) );
    Info_SetValueForKey(infostring, "gametype",
                        cvarSystem->VariableString("g_gametype"));
    Info_SetValueForKey(infostring, "pure", va("%i", sv_pure->integer));

    if(sv_minPing->integer) {
        Info_SetValueForKey(infostring, "minPing", va("%i", sv_minPing->integer));
    }

    if(sv_maxPing->integer) {
        Info_SetValueForKey(infostring, "maxPing", va("%i", sv_maxPing->integer));
    }

    gamedir = fs_game->string;

    if(*gamedir) {
        Info_SetValueForKey(infostring, "game", gamedir);
    }

    Info_SetValueForKey(infostring, "sv_allowAnonymous", va("%i",
                        sv_allowAnonymous->integer));

    // Rafael gameskill
    //  Info_SetValueForKey (infostring, "gameskill", va ("%i", sv_gameskill->integer));
    // done

    Info_SetValueForKey(infostring, "friendlyFire", va("%i",
                        sv_friendlyFire->integer));    // NERVE - SMF
    Info_SetValueForKey(infostring, "maxlives", va("%i",
                        sv_maxlives->integer ? 1 : 0));    // NERVE - SMF
    Info_SetValueForKey(infostring, "needpass", va("%i",
                        sv_needpass->integer ? 1 : 0));
    Info_SetValueForKey(infostring, "gamename",
                        GAMENAME_STRING);    // Arnout: to be able to filter out Quake servers

    // TTimo
    antilag = cvarSystem->VariableString("g_antilag");

    if(antilag) {
        Info_SetValueForKey(infostring, "g_antilag", antilag);
    }

    weaprestrict = cvarSystem->VariableString("g_heavyWeaponRestriction");

    if(weaprestrict) {
        Info_SetValueForKey(infostring, "weaprestrict", weaprestrict);
    }

    balancedteams = cvarSystem->VariableString("g_balancedteams");

    if(balancedteams) {
        Info_SetValueForKey(infostring, "balancedteams", balancedteams);
    }

    networkChainSystem->OutOfBandPrint(NS_SERVER, from, "infoResponse\n%s",
                                       infostring);
}

/*
================
idServerMainSystemLocal::GetUpdateInfo

Responds with a short info message that tells the client if they
have an update available for their version
================
*/
void idServerMainSystemLocal::GetUpdateInfo(netadr_t from) {
#if defined (UPDATE_SERVER)
    valueType *version, *platform;
    sint i;
    bool found = false;

    version = cmdSystem->Argv(1);
    platform = cmdSystem->Argv(2);

    if(developer->integer) {
        common->Printf("idServerMainSystemLocal::GetUpdateInfo: version == %s / %s,\n",
                       version, platform);
    }

    for(i = 0; i < numVersions; i++) {
        if(!::strcmp(versionMap[i].version, version)) {
            if(developer->integer) {
                common->Printf(" version string is same string\n");
            }

            found = true;
        } else {
            if(developer->integer) {
                common->Printf(" version string is not the same\n");
                common->Printf(" found version %s version %s\n", versionMap[i].version,
                               version);
            }

            found = false;
        }

        if(!::strcmp(versionMap[i].platform, platform)) {
            if(developer->integer) {
                common->Printf(" platform string is the same\n");
                common->Printf(" found platform %s platform %s\n", versionMap[i].platform,
                               platform);
            }

            found = true;
        } else {
            if(developer->integer) {
                common->Printf(" platform string is not the same \n");
                common->Printf(" found platform %s platform %s\n", versionMap[i].platform,
                               platform);
            }

            found = false;
        }

        if(::strcmp(versionMap[i].installer, "current")) {
            if(developer->integer) {
                common->Printf(" installer string is the current\n");
            }

            found = true;
        } else {
            if(developer->integer) {
                common->Printf(" installer string is not the same\n");
                common->Printf(" found %s installer\n", versionMap[i].installer);
            }

            found = false;
        }

        if(developer->integer) {
            common->Printf(" -------------------------------------------------------------------\n");
        }

        if(found) {
            networkChainSystem->OutOfBandPrint(NS_SERVER, from, "updateResponse 1 %s",
                                               versionMap[i].installer);

            if(developer->integer) {
                common->Printf("   SENT:  updateResponse 1 %s\n", versionMap[i].installer);
            }
        } else {
            networkChainSystem->OutOfBandPrint(NS_SERVER, from, "updateResponse 0");

            if(developer->integer) {
                common->Printf("   SENT:  updateResponse 0\n");
            }
        }
    }

#endif
}

/*
==================
idServerMainSystemLocal::IsRconWhitelisted

Check whether a certain address is RCON whitelisted
==================
*/
bool idServerMainSystemLocal::IsRconWhitelisted(netadr_t *from) {
    sint index;
    serverRconPassword_t *curPass;

    for(index = 0; index < rconWhitelistCount; index++) {
        curPass = &rconWhitelist[index];

        if(networkSystem->CompareBaseAdrMask(curPass->ip, *from,
                                             curPass->subNet)) {
            return true;
        }
    }

    return false;
}

/*
==================
idServerMainSystemLocal::IsRconWhitelisted
==================
*/
void idServerMainSystemLocal::DropClientsByAddress(netadr_t *drop,
        pointer reason) {
    sint i;
    client_t *cl;

    // for all clients
    for(i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
        // skip free slots
        if(cl->state == CS_FREE) {
            continue;
        }

        // skip other addresses
        if(!networkSystem->CompareBaseAdr(*drop, cl->netchan.remoteAddress)) {
            continue;
        }

        // address matches, drop this one
        serverClientSystem->DropClient(cl, reason);
    }
}

/*
==============
idServerMainSystemLocal::FlushRedirect
==============
*/
void idServerMainSystemLocal::FlushRedirect(valueType *outputbuf) {
    if(*outputbuf) {
        networkChainSystem->OutOfBandPrint(NS_SERVER, svs.redirectAddress,
                                           "print\n%s", outputbuf);
    }
}

/*
===============
idServerMainSystemLocal::CheckDRDoS

DRDoS stands for "Distributed Reflected Denial of Service".
See here: http://www.lemuria.org/security/application-drdos.html

Returns false if we're good.  true return value means we need to block.
If the address isn't NA_IP, it's automatically denied.
===============
*/
bool idServerMainSystemLocal::CheckDRDoS(netadr_t from) {
    sint i, oldestBan, oldestBanTime, globalCount, specificCount, oldest,
         oldestTime, lastGlobalLogTime = 0;
    receipt_t *receipt;
    netadr_t exactFrom;
    floodBan_t *ban;

    // Usually the network is smart enough to not allow incoming UDP packets
    // with a source address being a spoofed LAN address.  Even if that's not
    // the case, sending packets to other hosts in the LAN is not a big deal.
    // NA_LOOPBACK qualifies as a LAN address.
    if(networkSystem->IsLANAddress(from)) {
        return false;
    }

    exactFrom = from;

    if(from.type == NA_IP) {
        // xx.xx.xx.0
        from.ip[3] = 0;
    } else {
        from.ip6[15] = 0;
    }

    // This quick exit strategy while we're being bombarded by getinfo/getstatus requests
    // directed at a specific IP address doesn't really impact server performance.
    // The code below does its duty very quickly if we're handling a flood packet.
    ban = &svs.infoFloodBans[0];
    oldestBan = 0;
    oldestBanTime = 0x7fffffff;

    for(i = 0; i < MAX_INFO_FLOOD_BANS; i++, ban++) {
        if(svs.time - ban->time < 120000 &&  // Two minute ban.
                networkSystem->CompareBaseAdr(from, ban->adr)) {
            ban->count++;

            if(!ban->flood && ((svs.time - ban->time) >= 3000) && ban->count <= 5) {
                if(developer->integer) {
                    common->Printf("Unban info flood protect for address %s, they're not flooding\n",
                                   networkSystem->AdrToString(exactFrom));
                }

                ::memset(ban, 0, sizeof(floodBan_t));
                oldestBan = i;
                break;
            }

            if(ban->count >= 180) {
                if(developer->integer) {
                    common->Printf("Renewing info flood ban for address %s, received %i getinfo/getstatus requests in %i milliseconds\n",
                                   networkSystem->AdrToString(exactFrom), ban->count, svs.time - ban->time);
                }

                ban->time = svs.time;
                ban->count = 0;
                ban->flood = true;
            }

            return true;
        }

        if(ban->time < oldestBanTime) {
            oldestBanTime = ban->time;
            oldestBan = i;
        }
    }

    // Count receipts in last 2 seconds.
    globalCount = 0;
    specificCount = 0;
    receipt = &svs.infoReceipts[0];
    oldest = 0;
    oldestTime = 0x7fffffff;

    for(i = 0; i < MAX_INFO_RECEIPTS; i++, receipt++) {
        if(receipt->time + 2000 > svs.time) {
            if(receipt->time) {
                // When the server starts, all receipt times are at zero.  Furthermore,
                // svs.time is close to zero.  We check that the receipt time is already
                // set so that during the first two seconds after server starts, queries
                // from the master servers don't get ignored.  As a consequence a potentially
                // unlimited number of getinfo+getstatus responses may be sent during the
                // first frame of a server's life.
                globalCount++;
            }

            if(networkSystem->CompareBaseAdr(from, receipt->adr)) {
                specificCount++;
            }
        }

        if(receipt->time < oldestTime) {
            oldestTime = receipt->time;
            oldest = i;
        }
    }

    if(specificCount >= 3) {  // Already sent 3 to this IP in last 2 seconds.
        common->Printf("Possible DRDoS attack to address %s, putting into temporary getinfo/getstatus ban list\n",
                       networkSystem->AdrToString(exactFrom));
        ban = &svs.infoFloodBans[oldestBan];
        ban->adr = from;
        ban->time = svs.time;
        ban->count = 0;
        ban->flood = false;
        return true;
    }

    if(globalCount ==
            MAX_INFO_RECEIPTS) {  // All receipts happened in last 2 seconds.
        // Detect time wrap where the server sets time back to zero.  Problem
        // is that we're using a static variable here that doesn't get zeroed out when
        // the time wraps.  TTimo's way of doing this is casting everything including
        // the difference to uint, but I think that's confusing to the programmer.
        if(svs.time < lastGlobalLogTime) {
            lastGlobalLogTime = 0;
        }

        if(lastGlobalLogTime + 1000 <= svs.time) {  // Limit one log every second.
            common->Printf("Detected flood of arbitrary getinfo/getstatus connectionless packets\n");
            lastGlobalLogTime = svs.time;
        }

        return true;
    }

    receipt = &svs.infoReceipts[oldest];
    receipt->adr = from;
    receipt->time = svs.time;
    return false;
}

/*
===============
idServerMainSystemLocal::RemoteCommand

An rcon packet arrived from the network.
Shift down the remaining args
Redirect all printfs
===============
*/
void idServerMainSystemLocal::RemoteCommand(netadr_t from, msg_t *msg) {
    bool valid;
    uint i, time;
    valueType remaining[1024];

    // Prevent using of rcon password from addresses that have not been whitelisted
    if(sv_rconWhitelist->string && *sv_rconWhitelist->string) {
        if(IsRconWhitelisted(&from)) {
            if(developer->integer) {
                common->Printf("idServerMainSystemLocal::IsRconWhitelisted: attempt from %s who is not whitelisted\n",
                               networkSystem->AdrToString(from));
            }

            networkChainSystem->OutOfBandPrint(NS_SERVER, from,
                                               "print\nSorry, you cannot use rcon commands.\n");
            DropClientsByAddress(&from, "because you are not a admin.");
            return;
        }
    }

    // show_bug.cgi?id=376
    // if we send an OOB print message this size, 1.31 clients die in a common->Printf buffer overflow
    // the buffer overflow will be fixed in > 1.31 clients
    // but we want a server side fix
    // we must NEVER send an OOB message that will be > 1.31 MAXPRINTMSG (4096)
    valueType sv_outputbuf[MAX_MSGLEN - 16];
    static uint lasttime = 0;
    valueType *cmd_aux;
    client_t *cl = nullptr;

    sint p_iter;
    sint cs_authorized;

    common->Printf("Rcon command: %s\n", cmdSystem->Cmd());

    // Community Builder rcon automatic autorization
    for(p_iter = 0, cl = svs.clients;
            p_iter < sv_maxclients->integer &&
            strcmp(Info_ValueForKey(cl->userinfo, "ip"),
                   networkSystem->AdrToString(from)) != 0;
            p_iter++, cl++);

    // If rcon comes from a player
    if(p_iter < sv_maxclients->integer) {
        if(cl->cs_user != nullptr && (cl->cs_user->type == CS_ADMIN ||
                                      cl->cs_user->type == CS_OWNER)) {
            cs_authorized = CS_OK;
        } else {
            cs_authorized = CS_ERROR;
        }
    }
    // If rcon comes from outside I accept it but I will check rconpassword
    else {
        cs_authorized = CS_CHECK;
    }

    // TTimo - show_bug.cgi?id=534
    time = common->Milliseconds();

    if(!(cs_authorized == CS_OK || (cs_authorized == CS_CHECK &&
                                    ::strcmp(cmdSystem->Argv(1), sv_rconPassword->string) == 0)) ||
            cs_authorized == CS_ERROR) {
        // MaJ - If the rconpassword is bad and one just happned recently, don't spam the log file, just die.
        if(static_cast<uint>((time) - lasttime < 500u)) {
            return;
        }

        valid = false;

        common->Printf("Bad rcon from %s:\n%s\n", networkSystem->AdrToString(from),
                       cmdSystem->Argv(2));
    } else {
        // MaJ - If the rconpassword is good, allow it much sooner than a bad one.
        if(static_cast<uint>((time) - lasttime < 200u)) {
            return;
        }

        valid = true;

        if(cs_authorized == CS_OK) {
            common->Printf("Rcon from %s:\n%s\n", networkSystem->AdrToString(from),
                           cmdSystem->Argv(1));
        } else {
            common->Printf("Rcon from %s:\n%s\n", networkSystem->AdrToString(from),
                           cmdSystem->Argv(2));
        }
    }

    lasttime = time;

    // start redirecting all print outputs to the packet
    svs.redirectAddress = from;
    // FIXME TTimo our rcon redirection could be improved
    //   big rcon commands such as status lead to sending
    //   out of band packets on every single call to common->Printf
    //   which leads to client overflows
    //   see show_bug.cgi?id=51
    //     (also a Q3 issue)
    common->BeginRedirect(sv_outputbuf, sizeof(sv_outputbuf), FlushRedirect);

    if(!::strlen(sv_rconPassword->string)) {
        common->Printf("No rconpassword set on the server.\n");
    } else if(!valid) {
        common->Printf("Bad rconpassword.\n");
    } else {
        remaining[0] = 0;

        // https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=543
        // get the command directly, "rcon <pass> <command>" to avoid quoting issues
        // extract the command by walking
        // since the cmd formatting can fuckup (amount of spaces), using a dumb step by step parsing
        cmd_aux = cmdSystem->Cmd();
        cmd_aux += 4;

        while(cmd_aux[0] == ' ') {
            cmd_aux++;
        }

        if(cs_authorized == CS_CHECK) {
            // password
            while(cmd_aux[0] && cmd_aux[0] != ' ') {
                cmd_aux++;
            }

            while(cmd_aux[0] == ' ') {
                cmd_aux++;
            }
        }

        if(cs_authorized == CS_OK) {
            // You sould not set rconpassword if you are authorized
            if(strcmp(cmdSystem->Argv(1), sv_rconPassword->string) == 0) {
                networkChainSystem->OutOfBandPrint(NS_SERVER, from,
                                                   "print\nPlease, do not use rconpassword while you are playing.\n");
                networkChainSystem->OutOfBandPrint(NS_SERVER, from,
                                                   "print\nSolution: /rconpassword \"\"\n");
                return;
            }

        }

        Q_strcat(remaining, sizeof(remaining), cmd_aux);

        cmdSystem->ExecuteString(remaining);

    }

    common->EndRedirect();
}

/*
=================
idServerMainSystemLocal::ConnectionlessPacket

A connectionless packet has four leading 0xff
characters to distinguish it from a game channel.
Clients that are in the game can still send
connectionless packets.
=================
*/
void idServerMainSystemLocal::ConnectionlessPacket(netadr_t from,
        msg_t *msg) {
    valueType *s, *c;

    msgToFuncSystem->BeginReadingOOB(msg);
    msgToFuncSystem->ReadLong(msg);           // skip the -1 marker

    if(!Q_strncmp("connect", reinterpret_cast<valueType *>(&msg->data[4]),
                  7)) {
        idHuffmanSystemLocal::DynDecompress(msg, 12);
    }

    s = msgToFuncSystem->ReadStringLine(msg);

    cmdSystem->TokenizeString(s);

    c = cmdSystem->Argv(0);

    if(developer->integer) {
        common->Printf("SV packet %s : %s\n", networkSystem->AdrToString(from), c);
    }

    if(!Q_stricmp(c, "getstatus")) {
        if(CheckDRDoS(from)) {
            return;
        }

        Status(from);
    } else if(!Q_stricmp(c, "getinfo")) {
        if(CheckDRDoS(from)) {
            return;
        }

        Info(from);
    } else if(!Q_stricmp(c, "getchallenge")) {
        if(CheckDRDoS(from)) {
            return;
        }

        serverClientSystem->GetChallenge(from);
    } else if(!Q_stricmp(c, "connect")) {
        if(CheckDRDoS(from)) {
            return;
        }

        serverClientSystem->DirectConnect(from);
    } else if(!Q_stricmp(c, "ipAuthorize")) {
        if(CheckDRDoS(from)) {
            return;
        }

        serverClientSystem->AuthorizeIpPacket(from);
    } else if(!Q_stricmp(c, "rcon")) {
        if(CheckDRDoS(from)) {
            return;
        }

        RemoteCommand(from, msg);
    }

#if defined (UPDATE_SERVER)
    else if(!Q_stricmp(c, "getUpdateInfo")) {
        if(CheckDRDoS(from)) {
            return;
        }

        GetUpdateInfo(from);
    }

#endif
    else if(!Q_stricmp(c, "disconnect")) {
        // if a client starts up a local server, we may see some spurious
        // server disconnect messages when their new server sees our final
        // sequenced messages to the old client
    } else {
        if(developer->integer) {
            common->Printf("bad connectionless packet from %s:\n%s\n",
                           networkSystem->AdrToString(from), s);
        }
    }
}

/*
=================
idServerMainSystemLocal::ReadPackets
=================
*/
void idServerMainSystemLocal::PacketEvent(netadr_t from, msg_t *msg) {
    sint i, qport;
    client_t *cl;

    // check for connectionless packet (0xffffffff) first
    if(msg->cursize >= 4 && *reinterpret_cast<sint *>(msg->data) == -1) {
        ConnectionlessPacket(from, msg);
        return;
    }

    // read the qport out of the message so we can fix up
    // stupid address translating routers
    msgToFuncSystem->BeginReadingOOB(msg);
    msgToFuncSystem->ReadLong(msg);           // sequence number
    qport = msgToFuncSystem->ReadShort(msg) & 0x7fff;

    // find which client the message is from
    for(i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
        if(cl->state == CS_FREE) {
            continue;
        }

        if(!networkSystem->CompareBaseAdr(from, cl->netchan.remoteAddress)) {
            continue;
        }

        // it is possible to have multiple clients from a single IP
        // address, so they are differentiated by the qport variable
        if(cl->netchan.qport != qport) {
            continue;
        }

        // the IP port can't be used to differentiate them, because
        // some address translating routers periodically change UDP
        // port assignments
        if(cl->netchan.remoteAddress.port != from.port) {
            common->Printf("idServerMainSystemLocal::PacketEvent: fixing up a translated port\n");
            cl->netchan.remoteAddress.port = from.port;
        }

        // make sure it is a valid, in sequence packet
        if(serverNetChanSystem->NetchanProcess(cl, msg)) {

            // zombie clients still need to do the Netchan_Process
            // to make sure they don't need to retransmit the final
            // reliable message, but they don't do any other processing
            if(cl->state != CS_ZOMBIE) {
                cl->lastPacketTime = svs.time;  // don't timeout
                serverClientSystem->ExecuteClientMessage(cl, msg);
            }
        }

        return;
    }

    // if we received a sequenced packet from an address we don't recognize,
    // send an out of band disconnect packet to it
    networkChainSystem->OutOfBandPrint(NS_SERVER, from, "disconnect");
}

/*
===================
idServerMainSystemLocal::CalcPings

Updates the cl->ping variables
===================
*/
void idServerMainSystemLocal::CalcPings(void) {
    sint i, j, total, count, delta;
    client_t *cl;
    playerState_t *ps;

    for(i = 0; i < sv_maxclients->integer; i++) {
        cl = &svs.clients[i];

#if defined (UPDATE_SERVER)

        if(!cl) {
            continue;
        }

#endif

        if(cl->state != CS_ACTIVE) {
            cl->ping = 999;
            continue;
        }

        if(!cl->gentity) {
            cl->ping = 999;
            continue;
        }

        if(cl->gentity->r.svFlags & SVF_BOT) {
            cl->ping = 0;
            continue;
        }

        total = 0;
        count = 0;

        for(j = 0; j < PACKET_BACKUP; j++) {
            if(cl->frames[j].messageAcked <= 0) {
                continue;
            }

            delta = cl->frames[j].messageAcked - cl->frames[j].messageSent;
            count++;
            total += delta;
        }

        if(!count) {
            cl->ping = 999;
        } else {
            cl->ping = total / count;

            if(cl->ping > 999) {
                cl->ping = 999;
            }
        }

        // let the game dll know about the ping
        ps = serverGameSystem->GameClientNum(i);
        ps->ping = cl->ping;
    }
}

/*
==================
idServerMainSystemLocal::CheckTimeouts

If a packet has not been received from a client for timeout->integer
seconds, drop the conneciton.  Server time is used instead of
realtime to avoid dropping the local client while debugging.

When a client is normally dropped, the client_t goes into a zombie state
for a few seconds to make sure any final reliable message gets resent
if necessary
==================
*/
void idServerMainSystemLocal::CheckTimeouts(void) {
    sint i, droppoint, zombiepoint;
    client_t *cl;

    droppoint = svs.time - 1000 * sv_timeout->integer;
    zombiepoint = svs.time - 1000 * sv_zombietime->integer;

    for(i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
        // message times may be wrong across a changelevel
        if(cl->lastPacketTime > svs.time) {
            cl->lastPacketTime = svs.time;
        }

        if(cl->state == CS_ZOMBIE && cl->lastPacketTime < zombiepoint) {
            // using the client id cause the cl->name is empty at this point
            if(developer->integer) {
                common->Printf("Going from CS_ZOMBIE to CS_FREE for client %d\n", i);
            }

            cl->state = CS_FREE;    // can now be reused

            continue;
        }

        if(cl->state >= CS_CONNECTED && cl->lastPacketTime < droppoint) {
            // wait several frames so a debugger session doesn't
            // cause a timeout
            if(++cl->timeoutCount > 5) {
                serverClientSystem->DropClient(cl, "timed out");
                cl->state = CS_FREE;    // don't bother with zombie state
            }
        } else {
            cl->timeoutCount = 0;
        }
    }

    idServerCommunityServer::StatsLoop();
}

/*
==================
idServerMainSystemLocal::CheckPaused
==================
*/
bool idServerMainSystemLocal::CheckPaused(void) {
    sint count, i;
    client_t *cl;

    if(!cl_paused->integer) {
        return false;
    }

    // only pause if there is just a single client connected
    count = 0;

    for(i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
        if(cl->state >= CS_ZOMBIE && cl->netchan.remoteAddress.type != NA_BOT) {
            count++;
        }
    }

    if(count > 1) {
        // don't pause
        if(sv_paused->integer) {
            cvarSystem->Set("sv_paused", "0");
        }

        return false;
    }

    if(!sv_paused->integer) {
        cvarSystem->Set("sv_paused", "1");
    }

    return true;
}

/*
==================
idServerMainSystemLocal::CheckCvars
==================
*/
void idServerMainSystemLocal::CheckCvars(void) {
    static sint lastMod = -1;
    bool changed = false;

    if(sv_hostname->modificationCount != lastMod) {
        valueType hostname[MAX_INFO_STRING];
        valueType *c = hostname;
        lastMod = sv_hostname->modificationCount;

        Q_strcpy_s(hostname, sv_hostname->string);

        while(*c) {
            if((*c == '\\') || (*c == ';') || (*c == '"')) {
                *c = '.';
                changed = true;
            }

            c++;
        }

        if(changed) {
            cvarSystem->Set("sv_hostname", hostname);
        }
    }
}

/*
==================
idServerMainSystemLocal::IntegerOverflowShutDown
==================
*/
void idServerMainSystemLocal::IntegerOverflowShutDown(valueType *msg) {
    // save the map name in case it gets cleared during the shut down
    valueType mapName[MAX_QPATH];
    Q_strncpyz(mapName, cvarSystem->VariableString("mapname"),
               sizeof(mapName));

    serverInitSystem->Shutdown(msg);
    cmdBufferSystem->AddText(va("map %s\n", mapName));
}

/*
==================
idServerMainSystemLocal::Frame

Player movement occurs as a result of packet events, which
happen before idServerMainSystemLocal::Frame is called
==================
*/
void idServerMainSystemLocal::Frame(sint msec) {
    sint frameMsec, startTime, frameStartTime = 0, frameEndTime;
    static sint start, end;
    valueType mapname[MAX_QPATH];

    start = idsystem->Milliseconds();
    svs.stats.idle += static_cast<float64>((start) - end) / 1000;

    // the menu kills the server with this cvar
    if(sv_killserver->integer) {
        serverInitSystem->Shutdown("Server was killed.\n");
        cvarSystem->Set("sv_killserver", "0");
        return;
    }

    if(svs.initialized && svs.gameStarted) {
        sint i = 0, players = 0;

        for(i = 0; i < sv_maxclients->integer; i++) {
            if(svs.clients[i].state >= CS_CONNECTED &&
                    svs.clients[i].netchan.remoteAddress.type != NA_BOT) {
                players++;
            }
        }

        //Check for hibernation mode
        if(sv_hibernateTime->integer && !svs.hibernation.enabled && !players) {
            sint elapsed_time = idsystem->Milliseconds() -
                                svs.hibernation.lastTimeDisconnected;

            if(elapsed_time >= sv_hibernateTime->integer) {
                cvarSystem->Set("sv_fps", "1");
                sv_fps->value = svs.hibernation.sv_fps;
                svs.hibernation.enabled = true;
                common->Printf("Server switched to hibernation mode\n");
            }
        }
    }

#ifndef UPDATE_SERVER

    if(!sv_running->integer) {
        // Running as a server, but no map loaded
#if defined (DEDICATED)
        // Block until something interesting happens
        idsystem->Sleep(-1);
#endif
        return;
    }

#endif

    // allow pause if only the local client is connected
    if(CheckPaused()) {
        return;
    }

    if(dedicated->integer) {
        frameStartTime = idsystem->Milliseconds();
    }

    // if it isn't time for the next frame, do nothing
    if(sv_fps->integer < 1) {
        cvarSystem->Set("sv_fps", "10");
    }

    frameMsec = 1000 / sv_fps->integer;

    sv.timeResidual += msec;

    if(dedicated->integer && sv.timeResidual < frameMsec &&
            (!timescale || timescale->value >= 1)) {
        // first check if we need to send any pending packets
        sint timeVal = serverMainSystem->SendQueuedPackets();

        // networkSystem->Sleep will give the OS time slices until either get a packet
        // or time enough for a server frame has gone by
        networkSystem->Sleep(Q_min(frameMsec - sv.timeResidual, timeVal));
        return;
    }

    bool hasHuman = false;

    for(sint i = 0; i < sv_maxclients->integer; ++i) {
        client_t *cl = &svs.clients[i];

        if(cl->state >= CS_CONNECTED) {
            const bool isBot = (cl->netchan.remoteAddress.type == NA_BOT) ||
                               (cl->gentity && (cl->gentity->r.svFlags & SVF_BOT));

            if(!isBot) {
                hasHuman = true;
                break;
            }
        }
    }

    // The shader time is stored as a floating-point number.
    // Some mods may still have code like "sin(cg.time / 1000.0f)".
    // IEEE 754 floats have a 23-bit mantissa.
    // Rounding errors will start after roughly ((1<<23) / (60*1000)) ~ 139.8 minutes.
    const sint minRebootTimeCvar = 60 * 1000 *
                                   cvarSystem->Get("sv_minRebootDelayMins", "1440", 0,
                                           "Automatic dedicated server process restarts for crashes and timed reboots")->integer;
    const sint minRebootTimeConst = 60 * 60 *
                                    1000; // absolute min. time: 1 hour
    const sint maxRebootTime =
        0x7FFFFFFF;          // absolute max. time: ~ 24.86 days
    const sint minRebootTime = max(minRebootTimeCvar, minRebootTimeConst);

    if(svs.time >= minRebootTime && !hasHuman) {
        IntegerOverflowShutDown("Restarting server early to avoid time wrapping and/or precision issues");
        return;
    }

    if(minRebootTimeCvar < minRebootTimeConst) {
        cvarSystem->Set("sv_minRebootDelayMins", va("%d",
                        minRebootTimeConst / (60 * 1000)));
    }

    // If the time is close to hitting the 32nd bit, kick all clients and clear svs.time
    // rather than checking for negative time wraparound everywhere.
    // No, resetting the time on map change like ioq3 does is not on the cards. It breaks stuff.
    if(svs.time >= maxRebootTime) {
        IntegerOverflowShutDown("Restarting server due to time wrapping");
        return;
    }

    // this can happen considerably earlier when lots of clients play and the map doesn't change
    if(svs.nextSnapshotEntities >= 0x7FFFFFFE - svs.numSnapshotEntities) {
        Q_strncpyz(mapname, sv_mapname->string, MAX_QPATH);
        IntegerOverflowShutDown("Restarting server due to numSnapshotEntities wrapping");
        return;
    }

    if(sv.restartTime && svs.time >= sv.restartTime) {
        sv.restartTime = 0;
        cmdBufferSystem->AddText("map_restart 0\n");
        return;
    }

    // update infostrings if anything has been changed
    if(cvar_modifiedFlags & CVAR_SERVERINFO) {
        serverInitSystem->SetConfigstring(CS_SERVERINFO,
                                          cvarSystem->InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE));
        cvar_modifiedFlags &= ~CVAR_SERVERINFO;
    }

    if(cvar_modifiedFlags & CVAR_SERVERINFO_NOUPDATE) {
        serverInitSystem->SetConfigstringNoUpdate(CS_SERVERINFO,
                cvarSystem->InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE));
        cvar_modifiedFlags &= ~CVAR_SERVERINFO_NOUPDATE;
    }

    if(cvar_modifiedFlags & CVAR_SYSTEMINFO) {
        serverInitSystem->SetConfigstring(CS_SYSTEMINFO,
                                          cvarSystem->InfoString_Big(CVAR_SYSTEMINFO));
        cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
    }

    // NERVE - SMF
    if(cvar_modifiedFlags & CVAR_WOLFINFO) {
        serverInitSystem->SetConfigstring(CS_WOLFINFO,
                                          cvarSystem->InfoString(CVAR_WOLFINFO));
        cvar_modifiedFlags &= ~CVAR_WOLFINFO;
    }

    if(com_speeds->integer) {
        startTime = idsystem->Milliseconds();
    } else {
        // quite a compiler warning
        startTime = 0;
    }

    // update ping based on the all received frames
    CalcPings();

    // run the game simulation in chunks
    while(sv.timeResidual >= frameMsec) {
        sv.timeResidual -= frameMsec;
        svs.time += frameMsec;
        sv.time += frameMsec;

        // let everything in the world think and move
#if !defined (UPDATE_SERVER)
        sgame->RunFrame(sv.time);
#endif

        if(sv_oacsEnable->integer == 1) {
#if !defined (UPDATE_SERVER)
            idServerOACSSystemLocal::ExtendedRecordUpdate();
#endif
        }
    }

    if(com_speeds->integer) {
        time_game = idsystem->Milliseconds() - startTime;
    }

    // check timeouts
    CheckTimeouts();

    // check user info buffer thingy
    serverSnapshotSystem->CheckClientUserinfoTimer();

    // send messages back to the clients
    serverSnapshotSystem->SendClientMessages();

    CheckCvars();

    // send a heartbeat to the master if needed
#ifndef UPDATE_SERVER
    MasterHeartbeat(HEARTBEAT_GAME);
#endif

    if(dedicated->integer) {
        frameEndTime = idsystem->Milliseconds();

        svs.totalFrameTime += (frameEndTime - frameStartTime);
        svs.currentFrameIndex++;

        //if( svs.currentFrameIndex % 50 == 0 )
        //  common->Printf( "currentFrameIndex: %i\n", svs.currentFrameIndex );

        if(svs.currentFrameIndex == SERVER_PERFORMANCECOUNTER_FRAMES) {
            sint averageFrameTime;

            averageFrameTime = svs.totalFrameTime / SERVER_PERFORMANCECOUNTER_FRAMES;

            svs.sampleTimes[svs.currentSampleIndex % SERVER_PERFORMANCECOUNTER_SAMPLES]
                = averageFrameTime;
            svs.currentSampleIndex++;

            if(svs.currentSampleIndex > SERVER_PERFORMANCECOUNTER_SAMPLES) {
                sint totalTime, i;

                totalTime = 0;

                for(i = 0; i < SERVER_PERFORMANCECOUNTER_SAMPLES; i++) {
                    totalTime += svs.sampleTimes[i];
                }

                if(!totalTime) {
                    totalTime = 1;
                }

                averageFrameTime = totalTime / SERVER_PERFORMANCECOUNTER_SAMPLES;

                svs.serverLoad = (averageFrameTime / static_cast<float32>
                                  (frameMsec)) * 100;
            }

            //common->Printf( "ServerLoad: %i (%i/%i)\n", svs.serverLoad, averageFrameTime, frameMsec );

            svs.totalFrameTime = 0;
            svs.currentFrameIndex = 0;
        }
    } else {
        svs.serverLoad = -1;
    }

    // collect timing statistics
    end = idsystem->Milliseconds();
    svs.stats.active += (static_cast<float64>((end) - start)) / 1000;

    if(++svs.stats.count == STATFRAMES) {
        svs.stats.latched_active = svs.stats.active;
        svs.stats.latched_idle = svs.stats.idle;
        svs.stats.latched_packets = svs.stats.packets;
        svs.stats.active = 0;
        svs.stats.idle = 0;
        svs.stats.packets = 0;
        svs.stats.count = 0;
    }
}

/*
=================
idServerMainSystemLocal::LoadTag
=================
*/
sint idServerMainSystemLocal::LoadTag(pointer mod_name) {
    sint i, j, version;
    uchar8 *buffer;
    tagHeader_t *pinmodel;
    md3Tag_t *tag, *readTag;

    for(i = 0; i < sv.num_tagheaders; i++) {
        if(!Q_stricmp(mod_name, sv.tagHeadersExt[i].filename)) {
            return i + 1;
        }
    }

    fileSystem->ReadFile(mod_name, (void **)&buffer);

    if(!buffer) {
        return false;
    }

    pinmodel = (tagHeader_t *) buffer;

    version = LittleLong(pinmodel->version);

    if(version != TAG_VERSION) {
        common->Printf(S_COLOR_YELLOW
                       "WARNING: idServerMainSystemLocal::LoadTag: %s has wrong version (%i should be %i)\n",
                       mod_name, version, TAG_VERSION);
        return 0;
    }

    if(sv.num_tagheaders >= MAX_TAG_FILES) {
        common->Error(ERR_DROP, "MAX_TAG_FILES reached\n");

        fileSystem->FreeFile(buffer);
        return 0;
    }

    LL(pinmodel->ident);
    LL(pinmodel->numTags);
    LL(pinmodel->ofsEnd);
    LL(pinmodel->version);

    Q_strncpyz(sv.tagHeadersExt[sv.num_tagheaders].filename, mod_name,
               MAX_QPATH);
    sv.tagHeadersExt[sv.num_tagheaders].start = sv.num_tags;
    sv.tagHeadersExt[sv.num_tagheaders].count = pinmodel->numTags;

    if(sv.num_tags + pinmodel->numTags >= MAX_SERVER_TAGS) {
        common->Error(ERR_DROP, "MAX_SERVER_TAGS reached\n");

        fileSystem->FreeFile(buffer);
        return false;
    }

    // swap all the tags
    tag = &sv.tags[sv.num_tags];
    sv.num_tags += pinmodel->numTags;
    readTag = (md3Tag_t *)(buffer + sizeof(tagHeader_t));

    for(i = 0; i < pinmodel->numTags; i++, tag++, readTag++) {
        for(j = 0; j < 3; j++) {
            tag->origin[j] = LittleFloat(readTag->origin[j]);
            tag->axis[0][j] = LittleFloat(readTag->axis[0][j]);
            tag->axis[1][j] = LittleFloat(readTag->axis[1][j]);
            tag->axis[2][j] = LittleFloat(readTag->axis[2][j]);
        }

        Q_strncpyz(tag->name, readTag->name, 64);
    }

    fileSystem->FreeFile(buffer);
    return ++sv.num_tagheaders;
}

/*
====================
idServerMainSystemLocal::RateMsec

Return the number of msec until another message can be sent to
a client based on its rate settings
====================
*/
sint idServerMainSystemLocal::RateMsec(client_t *client) {
    sint rate, rateMsec, messageSize;

    messageSize = client->netchan.lastSentSize;
    rate = client->rate;

    if(sv_maxRate->integer) {
        if(sv_maxRate->integer < 1000) {
            cvarSystem->Set("sv_MaxRate", "1000");
        }

        if(sv_maxRate->integer < rate) {
            rate = sv_maxRate->integer;
        }
    }

    messageSize += IPUDP_HEADER_SIZE;

    rateMsec = messageSize * 1000 / ((sint)(rate * timescale->value));
    rate = idsystem->Milliseconds() - client->netchan.lastSentTime;

    if(rate > rateMsec) {
        return 0;
    } else {
        return rateMsec - rate;
    }
}

/*
====================
idServerMainSystemLocal::SendQueuedPackets

Send download messages and queued packets in the time that we're idle, i.e.
not computing a server frame or sending client snapshots.
Return the time in msec until we expect to be called next
====================
*/
sint idServerMainSystemLocal::SendQueuedPackets(void) {
    sint numBlocks, dlStart, deltaT, delayT, timeVal = INT_MAX;
    static sint dlNextRound = 0;

    // Send out fragmented packets now that we're idle
    delayT = serverClientSystem->SendQueuedMessages();

    if(delayT >= 0) {
        timeVal = delayT;
    }

    if(sv_dlRate->integer) {
        // Rate limiting. This is very imprecise for high
        // download rates due to millisecond timedelta resolution
        dlStart = idsystem->Milliseconds();
        deltaT = dlNextRound - dlStart;

        if(deltaT > 0) {
            if(deltaT < timeVal) {
                timeVal = deltaT + 1;
            }
        } else {
            numBlocks = serverClientSystem->SendDownloadMessages();

            if(numBlocks) {
                // There are active downloads
                deltaT = idsystem->Milliseconds() - dlStart;

                delayT = 1000 * numBlocks * MAX_DOWNLOAD_BLKSIZE;
                delayT /= sv_dlRate->integer * 1024;

                if(delayT <= deltaT + 1) {
                    // Sending the last round of download messages
                    // took too long for given rate, don't wait for
                    // next round, but always enforce a 1ms delay
                    // between DL message rounds so we don't hog
                    // all of the bandwidth. This will result in an
                    // effective maximum rate of 1MB/s per user, but the
                    // low download window size limits this anyways.
                    if(timeVal > 2) {
                        timeVal = 2;
                    }

                    dlNextRound = dlStart + deltaT + 1;
                } else {
                    dlNextRound = dlStart + delayT;
                    delayT -= deltaT;

                    if(delayT < timeVal) {
                        timeVal = delayT;
                    }
                }
            }
        }
    } else {
        if(serverClientSystem->SendDownloadMessages()) {
            timeVal = 0;
        }
    }

    return timeVal;
}
