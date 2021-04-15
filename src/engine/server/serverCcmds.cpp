////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   serverCcmds.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.hpp>
#elif DEDICATED
#include <null/null_serverprecompiled.hpp>
#else
#include <framework/precompiled.hpp>
#endif

idServerCcmdsSystemLocal serverCcmdsLocal;

/*
===============
idServerCcmdsSystemLocal::idServerCcmdsSystemLocal
===============
*/
idServerCcmdsSystemLocal::idServerCcmdsSystemLocal(void) {
}

/*
===============
idServerCcmdsSystemLocal::~idServerCcmdsSystemLocal
===============
*/
idServerCcmdsSystemLocal::~idServerCcmdsSystemLocal(void) {
}

/*
===============================================================================
OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/

/*
==================
idServerCcmdsSystemLocal::GetPlayerByHandle

Returns the player with player id or name from cmdSystem->Argv(1)
==================
*/
client_t *idServerCcmdsSystemLocal::GetPlayerByHandle(void) {
    sint i;
    client_t *cl;
    valueType *s, cleanName[64];

    // make sure server is running
    if(!com_sv_running->integer) {
        return nullptr;
    }

    if(cmdSystem->Argc() < 2) {
        Com_Printf("No player specified.\n");
        return nullptr;
    }

    s = cmdSystem->Argv(1);

    // Check whether this is a numeric player handle
    for(i = 0; s[i] >= '0' && s[i] <= '9'; i++);

    if(!s[i]) {
        int plid = atoi(s);

        // Check for numeric playerid match
        if(plid >= 0 && plid < sv_maxclients->integer) {
            cl = &svs.clients[plid];

            if(cl->state) {
                return cl;
            }
        }
    }

    // check for a name match
    for(i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
        if(!cl->state) {
            continue;
        }

        if(!Q_stricmp(cl->name, s)) {
            return cl;
        }

        Q_strncpyz(cleanName, cl->name, sizeof(cleanName));
        Q_CleanStr(cleanName);

        if(!Q_stricmp(cleanName, s)) {
            return cl;
        }
    }

    Com_Printf("Player %s is not on the server\n", s);

    return nullptr;
}

/*
==================
idServerCcmdsSystemLocal::GetPlayerByName

Returns the player with name from cmdSystem->Argv(1)
==================
*/
client_t *idServerCcmdsSystemLocal::GetPlayerByName(void) {
    sint i;
    valueType *s, cleanName[64];
    client_t *cl;

    // make sure server is running
    if(!com_sv_running->integer) {
        return nullptr;
    }

    if(cmdSystem->Argc() < 2) {
        Com_Printf("idServerCcmdsSystemLocal::GetPlayerByName - No player specified.\n");
        return nullptr;
    }

    s = cmdSystem->Argv(1);

    // check for a name match
    for(i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
        if(cl->state <= CS_ZOMBIE) {
            continue;
        }

        if(!Q_stricmp(cl->name, s)) {
            return cl;
        }

        Q_strncpyz(cleanName, cl->name, sizeof(cleanName));
        Q_CleanStr(cleanName);

        if(!Q_stricmp(cleanName, s)) {
            return cl;
        }
    }

    Com_Printf("idServerCcmdsSystemLocal::GetPlayerByName - Player %s is not on the server\n",
               s);

    return nullptr;
}

/*
==================
idServerCcmdsSystemLocal::Map_f

Restart the server on a different map
==================
*/
void idServerCcmdsSystemLocal::Map_f(void) {
    sint savegameTime = -1;
    valueType *cmd, *map, smapname[MAX_QPATH], mapname[MAX_QPATH],
              expanded[MAX_QPATH], *cl_profileStr =
                  cvarSystem->VariableString("cl_profile");
    bool killBots, cheat, buildScript;

    map = cmdSystem->Argv(1);

    if(!map) {
        return;
    }

    if(::strchr(map, '\\')) {
        Com_Printf("Can't have mapnames with a \\\n");
        return;
    }

    if(!com_gameInfo.spEnabled) {
        if(!Q_stricmp(cmdSystem->Argv(0), "spdevmap") ||
                !Q_stricmp(cmdSystem->Argv(0), "spmap")) {
            Com_Printf("Single Player is not enabled.\n");
            return;
        }
    }

    buildScript = static_cast< bool >
                  (cvarSystem->VariableIntegerValue("com_buildScript"));

    if(serverGameSystem->GameIsSinglePlayer()) {
        if(!buildScript && sv_reloading->integer &&
                sv_reloading->integer !=
                RELOAD_NEXTMAP) { // game is in 'reload' mode, don't allow starting new maps yet.
            return;
        }

        // Trap a savegame load
        if(strstr(map, ".sav")) {
            // open the savegame, read the mapname, and copy it to the map string
            valueType savemap[MAX_QPATH], savedir[MAX_QPATH];
            uchar8 *buffer;
            sint size, csize;

            if(com_gameInfo.usesProfiles && cl_profileStr[0]) {
                Q_vsprintf_s(savedir, sizeof(savedir), sizeof(savedir),
                             "profiles/%s/save/", cl_profileStr);
            } else {
                Q_strncpyz(savedir, "save/", sizeof(savedir));
            }

            if(!(strstr(map, savedir) == map)) {
                Q_vsprintf_s(savemap, sizeof(savemap), sizeof(savemap), "%s%s", savedir,
                             map);
            } else {
                Q_strcpy_s(savemap, map);
            }

            size = fileSystem->ReadFile(savemap, nullptr);

            if(size < 0) {
                Com_Printf("Can't find savegame %s\n", savemap);
                return;
            }

            //buffer = Hunk_AllocateTempMemory(size);
            fileSystem->ReadFile(savemap, (void **)&buffer);

            if(Q_stricmp(savemap, va("%scurrent.sav", savedir)) != 0) {
                // copy it to the current savegame file
                fileSystem->WriteFile(va("%scurrent.sav", savedir), buffer, size);
                // make sure it is the correct size
                csize = fileSystem->ReadFile(va("%scurrent.sav", savedir), nullptr);

                if(csize != size) {
                    Hunk_FreeTempMemory(buffer);
                    fileSystem->Delete(va("%scurrent.sav", savedir));
                    // TTimo
#ifdef __linux__
                    Com_Error(ERR_DROP,
                              "Unable to save game.\n\nPlease check that you have at least 5mb free of disk space in your home directory.");
#else
                    Com_Error(ERR_DROP,
                              "Insufficient free disk space.\n\nPlease free at least 5mb of free space on game drive.");
#endif
                    return;
                }
            }

            // set the cvar, so the game knows it needs to load the savegame once the clients have connected
            cvarSystem->Set("savegame_loading", "1");

            // set the filename
            cvarSystem->Set("savegame_filename", savemap);

            // the mapname is at the very start of the savegame file
            Q_strncpyz(savemap, reinterpret_cast< valueType *>(buffer) + sizeof(sint),
                       sizeof(savemap));    // skip the version
            Q_strncpyz(smapname, savemap, sizeof(smapname));
            map = smapname;

            savegameTime = *reinterpret_cast<sint *>(buffer + sizeof(
                               sint) + MAX_QPATH);

            if(savegameTime >= 0) {
                svs.time = savegameTime;
            }

            Hunk_FreeTempMemory(buffer);
        } else {
            cvarSystem->Set("savegame_loading", "0");    // make sure it's turned off

            // set the filename
            cvarSystem->Set("savegame_filename", "");
        }
    } else {
        cvarSystem->Set("savegame_loading", "0");    // make sure it's turned off

        // set the filename
        cvarSystem->Set("savegame_filename", "");
    }

    // make sure the level exists before trying to change, so that
    // a typo at the server console won't end the game
    Q_vsprintf_s(expanded, sizeof(expanded), sizeof(expanded), "maps/%s.bsp",
                 map);

    if(fileSystem->ReadFile(expanded, nullptr) == -1) {
        Com_Printf("Can't find map %s\n", expanded);
        return;
    }

    cvarSystem->Set("gamestate", va("%i",
                                    GS_INITIALIZE));     // NERVE - SMF - reset gamestate on map/devmap

    cvarSystem->Set("g_currentRound",
                    "0");   // NERVE - SMF - reset the current round
    cvarSystem->Set("g_nextTimeLimit",
                    "0");   // NERVE - SMF - reset the next time limit

    // START Mad Doctor I changes, 8/14/2002.  Need a way to force load a single player map as single player
    if(!Q_stricmp(cmdSystem->Argv(0), "spdevmap") ||
            !Q_stricmp(cmdSystem->Argv(0), "spmap")) {
        // This is explicitly asking for a single player load of this map
        cvarSystem->Set("g_gametype", va("%i", com_gameInfo.defaultSPGameType));

        // force latched values to get set
        cvarSystem->Get("g_gametype", va("%i", com_gameInfo.defaultSPGameType),
                        CVAR_SERVERINFO | CVAR_USERINFO | CVAR_LATCH,
                        "Sets the type of game being played, 2=objective, 3=stopwatch, 4=campaign, 5=LMS");

        // enable bot support for AI
        cvarSystem->Set("bot_enable", "1");
    }

    cmd = cmdSystem->Argv(0);

    if(!Q_stricmp(cmd, "devmap")) {
        cheat = true;
        killBots = true;
    } else if(!Q_stricmp(cmdSystem->Argv(0), "spdevmap")) {
        cheat = true;
        killBots = true;
    } else {
        cheat = false;
        killBots = false;
    }

    // save the map name here cause on a map restart we reload the q3config.cfg
    // and thus nuke the arguments of the map command
    Q_strncpyz(mapname, map, sizeof(mapname));

    // start up the map
    serverInitSystem->SpawnServer(mapname, killBots);

    // set the cheat value
    // if the level was started with "map <levelname>", then
    // cheats will not be allowed.  If started with "devmap <levelname>"
    // then cheats will be allowed
    if(cheat) {
        cvarSystem->Set("sv_cheats", "1");
    } else {
        cvarSystem->Set("sv_cheats", "0");
    }
}

/*
================
idServerCcmdsSystemLocal::CheckTransitionGameState

NERVE - SMF
================
*/
bool idServerCcmdsSystemLocal::CheckTransitionGameState(gamestate_t new_gs,
        gamestate_t old_gs) {
    if(old_gs == new_gs && new_gs != GS_PLAYING) {
        return false;
    }

    //  if ( old_gs == GS_WARMUP && new_gs != GS_WARMUP_COUNTDOWN )
    //      return false;

    //  if ( old_gs == GS_WARMUP_COUNTDOWN && new_gs != GS_PLAYING )
    //      return false;

    if(old_gs == GS_WAITING_FOR_PLAYERS && new_gs != GS_WARMUP) {
        return false;
    }

    if(old_gs == GS_INTERMISSION && new_gs != GS_WARMUP) {
        return false;
    }

    if(old_gs == GS_RESET && (new_gs != GS_WAITING_FOR_PLAYERS &&
                              new_gs != GS_WARMUP)) {
        return false;
    }

    return true;
}

/*
================
idServerCcmdsSystemLocal::TransitionGameState

NERVE - SMF
================
*/
bool idServerCcmdsSystemLocal::TransitionGameState(gamestate_t new_gs,
        gamestate_t old_gs, sint delay) {
    if(!serverGameSystem->GameIsSinglePlayer() &&
            !serverGameSystem->GameIsCoop()) {
        // we always do a warmup before starting match
        if(old_gs == GS_INTERMISSION && new_gs == GS_PLAYING) {
            new_gs = GS_WARMUP;
        }
    }

    // check if its a valid state transition
    if(!CheckTransitionGameState(new_gs, old_gs)) {
        return false;
    }

    if(new_gs == GS_RESET) {
        new_gs = GS_WARMUP;
    }

    cvarSystem->Set("gamestate", va("%i", new_gs));

    return true;
}

void MSG_PrioritiseEntitystateFields(void);
void MSG_PrioritisePlayerStateFields(void);

/*
================
idServerCcmdsSystemLocal::FieldInfo_f
================
*/
void idServerCcmdsSystemLocal::FieldInfo_f(void) {
    MSG_PrioritiseEntitystateFields();
    MSG_PrioritisePlayerStateFields();
}

/*
================
idServerCcmdsSystemLocal::MapRestart_f

Completely restarts a level, but doesn't send a new gamestate to the clients.
This allows fair starts with variable load times.
================
*/
void idServerCcmdsSystemLocal::MapRestart_f(void) {
    sint i, delay = 0;
    client_t *client;
    valueType *denied;
    bool isBot;
    gamestate_t new_gs, old_gs; // NERVE - SMF

    // make sure we aren't restarting twice in the same frame
    if(com_frameTime == sv.serverId) {
        return;
    }

    // make sure server is running
    if(!com_sv_running->integer) {
        Com_Printf("Server is not running.\n");
        return;
    }

    if(cmdSystem->Argc() > 1) {
        delay = atoi(cmdSystem->Argv(1));
    }

    if(delay) {
        sv.restartTime = sv.time + delay * 1000;
        serverInitSystem->SetConfigstring(CS_WARMUP, va("%i", sv.restartTime));
        return;
    }

    // NERVE - SMF - read in gamestate or just default to GS_PLAYING
    old_gs = (gamestate_t)atoi(cvarSystem->VariableString("gamestate"));

    if(serverGameSystem->GameIsSinglePlayer() ||
            serverGameSystem->GameIsCoop()) {
        new_gs = GS_PLAYING;
    } else {
        if(cmdSystem->Argc() > 2) {
            new_gs = (gamestate_t)atoi(cmdSystem->Argv(2));
        } else {
            new_gs = GS_PLAYING;
        }
    }

    if(!TransitionGameState(new_gs, old_gs, delay)) {
        return;
    }

    // check for changes in variables that can't just be restarted
    // check for maxclients change
    if(sv_maxclients->modified) {
        valueType mapname[MAX_QPATH];

        Com_Printf("sv_maxclients variable change -- restarting.\n");
        // restart the map the slow way
        Q_strncpyz(mapname, cvarSystem->VariableString("mapname"),
                   sizeof(mapname));

        serverInitSystem->SpawnServer(mapname, false);
        return;
    }

    // Check for loading a saved game
    if(cvarSystem->VariableIntegerValue("savegame_loading")) {
        // open the current savegame, and find out what the time is, everything else we can ignore
        valueType savemap[MAX_QPATH],
                  *cl_profileStr = cvarSystem->VariableString("cl_profile");
        uchar8 *buffer;
        sint size, savegameTime;

        if(com_gameInfo.usesProfiles) {
            Q_vsprintf_s(savemap, sizeof(savemap), sizeof(savemap),
                         "profiles/%s/save/current.sav", cl_profileStr);
        } else {
            Q_strncpyz(savemap, "save/current.sav", sizeof(savemap));
        }

        size = fileSystem->ReadFile(savemap, nullptr);

        if(size < 0) {
            Com_Printf("Can't find savegame %s\n", savemap);
            return;
        }

        //buffer = Hunk_AllocateTempMemory(size);
        fileSystem->ReadFile(savemap, (void **)&buffer);

        // the mapname is at the very start of the savegame file
        savegameTime = *reinterpret_cast<sint *>(buffer + sizeof(
                           sint) + MAX_QPATH);

        if(savegameTime >= 0) {
            svs.time = savegameTime;
        }

        Hunk_FreeTempMemory(buffer);
    }

    // done.

    StopAutoRecordDemos();

    // toggle the server bit so clients can detect that a
    // map_restart has happened
    svs.snapFlagServerBit ^= SNAPFLAG_SERVERCOUNT;

    // generate a new serverid
    sv.restartedServerId = sv.serverId;
    sv.serverId = com_frameTime;
    cvarSystem->Set("sv_serverid", va("%i", sv.serverId));

    time(&sv.realMapTimeStarted);

    // if a map_restart occurs while a client is changing maps, we need
    // to give them the correct time so that when they finish loading
    // they don't violate the backwards time check in cl_cgame.cpp
    for(i = 0; i < sv_maxclients->integer; i++) {
        if(svs.clients[i].state == CS_PRIMED) {
            svs.clients[i].oldServerTime = sv.restartTime;
        }
    }

    // reset all the vm data in place without changing memory allocation
    // note that we do NOT set sv.state = SS_LOADING, so configstrings that
    // had been changed from their default values will generate broadcast updates
    sv.state = SS_LOADING;
    sv.restarting = true;

    cvarSystem->Set("sv_serverRestarting", "1");

    serverGameSystem->RestartGameProgs();

    // run a few frames to allow everything to settle
    for(i = 0; i < GAME_INIT_FRAMES; i++) {
        sgame->RunFrame(sv.time);
        sv.time += 100;
        svs.time += FRAMETIME;
    }

    // create a baseline for more efficient communications
    // Gordon: meh, this wont work here as the client doesn't know it has happened
    // CreateBaseline ();

    sv.state = SS_GAME;
    sv.restarting = false;

    // connect and begin all the clients
    for(i = 0; i < sv_maxclients->integer; i++) {
        client = &svs.clients[i];

        // send the new gamestate to all connected clients
        if(client->state < CS_CONNECTED) {
            continue;
        }

        if(client->netchan.remoteAddress.type == NA_BOT) {
            if(serverGameSystem->GameIsSinglePlayer() ||
                    serverGameSystem->GameIsCoop()) {
                continue;       // dont carry across bots in single player
            }

            isBot = true;
        } else {
            isBot = false;
        }

        // add the map_restart command
        serverMainSystem->AddServerCommand(client, "map_restart\n");

        // connect the client again, without the firstTime flag
        denied = static_cast<valueType *>(sgame->ClientConnect(i, false));

        if(denied) {
            // this generally shouldn't happen, because the client
            // was connected before the level change
            serverClientSystem->DropClient(client, denied);

            if((!serverGameSystem->GameIsSinglePlayer()) || (!isBot)) {
                Com_Printf("idServerBotSystemLocal::MapRestart_f(%d): dropped client %i - denied!\n",
                           delay, i);    // bk010125
            }

            continue;
        }

        if(client->state == CS_ACTIVE) {
            serverClientSystem->ClientEnterWorld(client, &client->lastUsercmd);
        } else {
            // If we don't reset client->lastUsercmd and are restarting during map load,
            // the client will hang because we'll use the last Usercmd from the previous map,
            // which is wrong obviously.
            serverClientSystem->ClientEnterWorld(client, nullptr);
        }
    }

    // run another frame to allow things to look at all the players
    sgame->RunFrame(sv.time);
    sv.time += 100;
    svs.time += FRAMETIME;

    cvarSystem->Set("sv_serverRestarting", "0");

    BeginAutoRecordDemos();
}

/*
=================
idServerCcmdsSystemLocal::LoadGame_f
=================
*/
void idServerCcmdsSystemLocal::LoadGame_f(void) {
    sint size;
    valueType filename[MAX_QPATH], mapname[MAX_QPATH], savedir[MAX_QPATH],
              *cl_profileStr = cvarSystem->VariableString("cl_profile");
    uchar8 *buffer;

    // dont allow command if another loadgame is pending
    if(cvarSystem->VariableIntegerValue("savegame_loading")) {
        return;
    }

    if(sv_reloading->integer) {
        // (SA) disabling
        // if(sv_reloading->integer && sv_reloading->integer != RELOAD_FAILED )    // game is in 'reload' mode, don't allow starting new maps yet.
        return;
    }

    Q_strncpyz(filename, cmdSystem->Argv(1), sizeof(filename));

    if(!filename[0]) {
        Com_Printf("You must specify a savegame to load\n");
        return;
    }

    if(com_gameInfo.usesProfiles && cl_profileStr[0]) {
        Q_vsprintf_s(savedir, sizeof(savedir), sizeof(savedir),
                     "profiles/%s/save/", cl_profileStr);
    } else {
        Q_strncpyz(savedir, "save/", sizeof(savedir));
    }

    //if ( Q_strncmp( filename, "save/", 5 ) && Q_strncmp( filename, "save\\", 5 ) )
    //{
    //  Q_strncpyz( filename, va("save/%s", filename), sizeof( filename ) );
    //}

    // go through a va to avoid vsnprintf call with same source and target
    Q_strncpyz(filename, va("%s%s", savedir, filename), sizeof(filename));

    // enforce .sav extension
    if(!strstr(filename, ".") ||
            Q_strncmp(strstr(filename, ".") + 1, "sav", 3)) {
        Q_strcat(filename, sizeof(filename), ".sav");
    }

    // use '/' instead of '\\' for directories
    while(strstr(filename, "\\")) {
        *static_cast<valueType *>(::strstr(filename, "\\")) = '/';
    }

    size = fileSystem->ReadFile(filename, nullptr);

    if(size < 0) {
        Com_Printf("Can't find savegame %s\n", filename);
        return;
    }

    //buffer = Hunk_AllocateTempMemory(size);
    fileSystem->ReadFile(filename, (void **)&buffer);

    // read the mapname, if it is the same as the current map, then do a fast load
    Q_strncpyz(mapname, reinterpret_cast<pointer>(buffer) + sizeof(sint),
               sizeof(mapname));

    if(com_sv_running->integer && (com_frameTime != sv.serverId)) {
        // check mapname
        if(!Q_stricmp(mapname, sv_mapname->string)) {   // same
            if(Q_stricmp(filename, va("%scurrent.sav", savedir)) != 0) {
                // copy it to the current savegame file
                fileSystem->WriteFile(va("%scurrent.sav", savedir), buffer, size);
            }

            Hunk_FreeTempMemory(buffer);

            cvarSystem->Set("savegame_loading",
                            "2");    // 2 means it's a restart, so stop rendering until we are loaded

            // set the filename
            cvarSystem->Set("savegame_filename", filename);

            // quick-restart the server
            MapRestart_f(); // savegame will be loaded after restart

            return;
        }
    }

    Hunk_FreeTempMemory(buffer);

    // otherwise, do a slow load
    if(cvarSystem->VariableIntegerValue("sv_cheats")) {
        cmdBufferSystem->ExecuteText(EXEC_APPEND, va("spdevmap %s", filename));
    } else { // no cheats
        cmdBufferSystem->ExecuteText(EXEC_APPEND, va("spmap %s", filename));
    }
}

/*
==================
idServerCcmdsSystemLocal::TempBanNetAddress
==================
*/
void idServerCcmdsSystemLocal::TempBanNetAddress(netadr_t address,
        sint length) {
    sint i, oldesttime = 0, oldest = -1;

    for(i = 0; i < MAX_TEMPBAN_ADDRESSES; i++) {
        if(!svs.tempBanAddresses[i].endtime ||
                svs.tempBanAddresses[i].endtime < svs.time) {
            // found a free slot
            svs.tempBanAddresses[i].adr = address;
            svs.tempBanAddresses[i].endtime = svs.time + (length * 1000);
            return;
        } else {
            if(oldest == -1 || oldesttime > svs.tempBanAddresses[i].endtime) {
                oldesttime = svs.tempBanAddresses[i].endtime;
                oldest = i;
            }
        }
    }

    svs.tempBanAddresses[oldest].adr = address;
    svs.tempBanAddresses[oldest].endtime = svs.time + length;
}

/*
==================
idServerCcmdsSystemLocal::TempBanIsBanned
==================
*/
bool idServerCcmdsSystemLocal::TempBanIsBanned(netadr_t address) {
    sint i;

    for(i = 0; i < MAX_TEMPBAN_ADDRESSES; i++) {
        if(svs.tempBanAddresses[i].endtime &&
                svs.tempBanAddresses[i].endtime > svs.time) {
            if(networkSystem->CompareAdr(address, svs.tempBanAddresses[i].adr)) {
                return true;
            }
        }
    }

    return false;
}

/*
================
idServerCcmdsSystemLocal::Status_f
================
*/
void idServerCcmdsSystemLocal::Status_f(void) {
    sint i, j, ping;
    uint64 l;
    client_t *cl;
    playerState_t *ps;
    pointer s;
    uchar8 cpu, avg; //Dushan

    // make sure server is running
    if(!com_sv_running->integer) {
        Com_Printf("Server is not running.\n");
        return;
    }

    // Dushan
    cpu = static_cast<uchar8>(svs.stats.latched_active +
                              svs.stats.latched_idle);

    if(cpu) {
        cpu = static_cast<uchar8>(100 * svs.stats.latched_active / cpu);
    }

    avg = static_cast<uchar8>(1000 * svs.stats.latched_active / STATFRAMES);

    Com_Printf("cpu utilization  : %3i%%\n", static_cast<sint>(cpu));
    Com_Printf("avg response time: %i ms\n", static_cast<sint>(avg));

    Com_Printf("map: %s\n", sv_mapname->string);
    Com_Printf("Game ID: %s\n", community_stats.game_id);
    Com_Printf("num score ping name            lastmsg address               qport rate\n");
    Com_Printf("--- ----- ---- --------------- ------- --------------------- ----- -----\n");

    for(i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
        if(!cl->state) {
            continue;
        }

        Com_Printf("%3i ", i);

#ifndef UPDATE_SERVER
        ps = serverGameSystem->GameClientNum(i);

        Com_Printf("%5i ", ps->persistant[PERS_SCORE]);
#endif

        if(cl->state == CS_CONNECTED) {
            Com_Printf("CNCT ");
        } else if(cl->state == CS_ZOMBIE) {
            Com_Printf("ZMBI ");
        } else {
            ping = cl->ping < 9999 ? cl->ping : 9999;
            Com_Printf("%4i ", ping);
        }

        Com_Printf("%s", cl->name);
        l = 16 - static_cast<sint>(::strlen(cl->name));

        for(j = 0; j < l; j++) {
            Com_Printf(" ");
        }

        Com_Printf("%7i ", svs.time - cl->lastPacketTime);

        s = networkSystem->AdrToString(cl->netchan.remoteAddress);
        Com_Printf("%s", s);
        l = 22 - static_cast<sint>(::strlen(s));

        for(j = 0; j < l; j++) {
            Com_Printf(" ");
        }

        Com_Printf("%5i", cl->netchan.qport);

        Com_Printf(" %5i", cl->rate);

        if(cl->cs_user != nullptr) {
            Com_Printf(" %s", cl->cs_user->name);
        } else {
            Com_Printf(" N/A");
        }

        Com_Printf("\n");
    }

    Com_Printf("\n");
}

/*
==================
idServerCcmdsSystemLocal::ConSay_f
==================
*/
void idServerCcmdsSystemLocal::ConSay_f(void) {
    valueType *p;
    valueType text[1024];

    if(!com_dedicated->integer) {
        Com_Printf("Server is not dedicated.\n");
        return;
    }

    // make sure server is running
    if(!com_sv_running->integer) {
        Com_Printf("Server is not running.\n");
        return;
    }

    if(cmdSystem->Argc() < 2) {
        return;
    }

    Q_strcpy_s(text, "Server: ");
    p = cmdSystemLocal.Args();

    if(*p == '"') {
        Q_strcat(text, sizeof(text), p + 1);
        text[strlen(text) - 1] = '\0';
    } else {
        Q_strcat(text, sizeof(text), p);
    }

    strcat(text, p);

    serverMainSystem->SendServerCommand(nullptr, "chat \"%s\"", text);
}


/*
==================
idServerCcmdsSystemLocal::Heartbeat_f

Also called by idServerClientSystemLocal::DropClient, idServerClientSystemLocal::DirectConnect, and idServerInitSystemLocal::SpawnServer
==================
*/
void idServerCcmdsSystemLocal::Heartbeat_f(void) {
    svs.nextHeartbeatTime = -9999999;
}

/*
===========
idServerCcmdsSystemLocal::Serverinfo_f

Examine the serverinfo string
===========
*/
void idServerCcmdsSystemLocal::Serverinfo_f(void) {
    if(!com_sv_running->integer) {
        Com_Printf("Server is not running.\n");
    }

    Com_Printf("Server info settings:\n");

    Info_Print(cvarSystem->InfoString(CVAR_SERVERINFO |
                                      CVAR_SERVERINFO_NOUPDATE));
}

/*
===========
idServerCcmdsSystemLocal::Systeminfo_f

Examine or change the serverinfo string
===========
*/
void idServerCcmdsSystemLocal::Systeminfo_f(void) {
    // make sure server is running
    if(!com_sv_running->integer) {
        Com_Printf("Server is not running.\n");
        return;
    }

    Com_Printf("System info settings:\n");

    Info_Print(cvarSystem->InfoString_Big(CVAR_SERVERINFO |
                                          CVAR_SERVERINFO_NOUPDATE));
}

/*
===========
idServerCcmdsSystemLocal::DumpUser_f

Examine all a users info strings FIXME: move to game
===========
*/
void idServerCcmdsSystemLocal::DumpUser_f(void) {
    client_t *cl;

    // make sure server is running
    if(!com_sv_running->integer) {
        Com_Printf("Server is not running.\n");
        return;
    }

    if(cmdSystem->Argc() != 2) {
        Com_Printf("Usage: info <userid>\n");
        return;
    }

    cl = GetPlayerByName();

    if(!cl) {
        return;
    }

    Com_Printf("userinfo\n");
    Com_Printf("--------\n");
    Info_Print(cl->userinfo);
}

/*
====================
idServerCcmdsSystemLocal::UserInfo_f
====================
*/
void idServerCcmdsSystemLocal::UserInfo_f(void) {
    // make sure server is running
    if(!com_sv_running->integer) {
        Com_Printf("Server is not running.\n");
        return;
    }

    if(cmdSystem->Argc() != 2) {
        Com_Printf("Usage: userinfo <username>\n");
        return;
    }

    idServerCommunityServer::UserInfo(cmdSystem->Argv(1));
}

/*
====================
idServerCcmdsSystemLocal::StartMatch_f
====================
*/
void idServerCcmdsSystemLocal::StartMatch_f(void) {
    // make sure server is running
    if(!com_sv_running->integer) {
        Com_Printf("Server is not running.\n");
        return;
    }

    idServerCommunityServer::startMatch();
}

/*
====================
idServerCcmdsSystemLocal::StopMatch_f
====================
*/
void idServerCcmdsSystemLocal::StopMatch_f(void) {
    int i;
    client_t *cl;

    // make sure server is running
    if(!com_sv_running->integer) {
        Com_Printf("Server is not running.\n");
        return;
    }

    idServerCommunityServer::stopMatch();

    // Say bye to all players and referees and restart server
    for(i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
        if(!cl->state) {
            continue;
        }

        if(cl->netchan.remoteAddress.type == NA_LOOPBACK) {
            continue;
        }

        serverClientSystem->DropClient(cl, "Thanks for playing! Bye!");
        cl->lastPacketTime = svs.time;  // in case there is a funny zombie
    }

    cmdSystem->ExecuteString("exec server.cfg");

}

/*
====================
idServerCcmdsSystemLocal::AddClanMatch_f
====================
*/
void idServerCcmdsSystemLocal::AddClanMatch_f(void) {
    // make sure server is running
    if(!com_sv_running->integer) {
        Com_Printf("Server is not running.\n");
        return;
    }

    if(cmdSystem->Argc() != 2) {
        Com_Printf("Usage: addclanmatch <clanname>\n");
        return;
    }

    idServerCommunityServer::addMatchClan(cmdSystem->Argv(1));
}

/*
====================
idServerCcmdsSystemLocal::AddUserMatch_f
====================
*/
void idServerCcmdsSystemLocal::AddUserMatch_f(void) {
    // make sure server is running
    if(!com_sv_running->integer) {
        Com_Printf("Server is not running.\n");
        return;
    }

    if(cmdSystem->Argc() != 2) {
        Com_Printf("Usage: addusermatch <username>\n");
        return;
    }

    idServerCommunityServer::addMatchUser(cmdSystem->Argv(1));
}

/*
====================
idServerCcmdsSystemLocal::AddRefereeMatch_f
====================
*/
void idServerCcmdsSystemLocal::AddRefereeMatch_f(void) {
    // make sure server is running
    if(!com_sv_running->integer) {
        Com_Printf("Server is not running.\n");
        return;
    }

    if(cmdSystem->Argc() != 2) {
        Com_Printf("Usage: addrefereematch <username>\n");
        return;
    }

    idServerCommunityServer::addMatchReferee(cmdSystem->Argv(1));
}

/*
====================
idServerCcmdsSystemLocal::MatchInfo_f
====================
*/
void idServerCcmdsSystemLocal::MatchInfo_f(void) {
    // make sure server is running
    if(!com_sv_running->integer) {
        Com_Printf("Server is not running.\n");
        return;
    }

    idServerCommunityServer::matchInfo();
}

/*
====================
idServerCcmdsSystemLocal::AddIP_f
====================
*/
void idServerCcmdsSystemLocal::AddIP_f(void) {
    client_t *cl;

    // make sure server is running
    if(!com_sv_running->integer) {
        Com_Printf("Server is not running.\n");
        return;
    }

    if(cmdSystem->Argc() != 2) {
        Com_Printf("Usage: addip <userid>\n");
        return;
    }

    cl = GetPlayerByHandle();

    if(!cl) {
        return;
    }

    idServerCommunityServer::BanUser(cl);

}

/*
====================
idServerCcmdsSystemLocal::BanList_f
====================
*/
void idServerCcmdsSystemLocal::BanList_f(void) {

    // make sure server is running
    if(!com_sv_running->integer) {
        Com_Printf("Server is not running.\n");
        return;
    }

    idServerCommunityServer::showBanUsers();
}

/*
====================
idServerCcmdsSystemLocal::UnBan_f
====================
*/
void idServerCcmdsSystemLocal::UnBan_f(void) {
    // make sure server is running
    if(!com_sv_running->integer) {
        Com_Printf("Server is not running.\n");
        return;
    }

    if(cmdSystem->Argc() != 2) {
        Com_Printf("Usage: unban <banlist num>\n");
        return;
    }

    idServerCommunityServer::unbanUser(cmdSystem->Argv(1));
}

/*
====================
idServerCcmdsSystemLocal::StatsPlayers_f
====================
*/
void idServerCcmdsSystemLocal::StatsPlayers_f(void) {
    sint i;

    valueType bigbuffer[MAX_INFO_STRING * 2];

    // make sure server is running
    if(!com_sv_running->integer) {
        Com_Printf("Server is not running.\n");
        return;
    }

    for(i = 0; i < sv_maxclients->integer; i++) {
        if(svs.clients[i].state >= CS_ACTIVE) {
            ::sprintf(bigbuffer, "stats %i MELEE", i);
            cmdSystem->TokenizeString(bigbuffer);
            sgame->ClientCommand(i);

            ::sprintf(bigbuffer, "stats %i BLASTER", i);
            cmdSystem->TokenizeString(bigbuffer);
            sgame->ClientCommand(i);

            ::sprintf(bigbuffer, "stats %i MACHINEGUNE", i);
            cmdSystem->TokenizeString(bigbuffer);
            sgame->ClientCommand(i);

            ::sprintf(bigbuffer, "stats %i PAIN_SAW", i);
            cmdSystem->TokenizeString(bigbuffer);
            sgame->ClientCommand(i);

            ::sprintf(bigbuffer, "stats %i SHOTGUN", i);
            cmdSystem->TokenizeString(bigbuffer);
            sgame->ClientCommand(i);

            ::sprintf(bigbuffer, "stats %i LAS_GUN", i);
            cmdSystem->TokenizeString(bigbuffer);
            sgame->ClientCommand(i);

            ::sprintf(bigbuffer, "stats %i MASS_DRIVER", i);
            cmdSystem->TokenizeString(bigbuffer);
            sgame->ClientCommand(i);

            ::sprintf(bigbuffer, "stats %i CHAINGUN", i);
            cmdSystem->TokenizeString(bigbuffer);
            sgame->ClientCommand(i);

            ::sprintf(bigbuffer, "stats %i PULSE_RIFLE", i);
            cmdSystem->TokenizeString(bigbuffer);
            sgame->ClientCommand(i);

            ::sprintf(bigbuffer, "stats %i FLAMER", i);
            cmdSystem->TokenizeString(bigbuffer);
            sgame->ClientCommand(i);

            ::sprintf(bigbuffer, "stats %i LUCIFER_CANNON", i);
            cmdSystem->TokenizeString(bigbuffer);
            sgame->ClientCommand(i);

            ::sprintf(bigbuffer, "stats %i GRENADE", i);
            cmdSystem->TokenizeString(bigbuffer);
            sgame->ClientCommand(i);

            ::sprintf(bigbuffer, "stats %i LOCKBLOB_LAUNCHER", i);
            cmdSystem->TokenizeString(bigbuffer);
            sgame->ClientCommand(i);

            ::sprintf(bigbuffer, "stats %i MISC", i);
            cmdSystem->TokenizeString(bigbuffer);
            sgame->ClientCommand(i);

            ::sprintf(bigbuffer, "stats %i EXPLOSIONS", i);
            cmdSystem->TokenizeString(bigbuffer);
            sgame->ClientCommand(i);
        }
    }
}

/*
====================
idServerCcmdsSystemLocal::StatsPlayer_f
====================
*/
void idServerCcmdsSystemLocal::StatsPlayer_f(void) {
    static sint player;

    valueType bigbuffer[MAX_INFO_STRING * 2];

    // make sure server is running
    if(!com_sv_running->integer) {
        Com_Printf("Server is not running.\n");
        return;
    }

    player = (player + 1) % sv_maxclients->integer;

    if(svs.clients[player].state >= CS_ACTIVE) {
        ::sprintf(bigbuffer, "stats %i MELEE", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i BLASTER", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i MACHINEGUNE", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i PAIN_SAW", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i SHOTGUN", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i LAS_GUN", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i MASS_DRIVER", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i CHAINGUN", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i PULSE_RIFLE", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i FLAMER", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i LUCIFER_CANNON", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i GRENADE", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i LOCKBLOB_LAUNCHER", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i MISC", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i EXPLOSIONS", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);
    }
}

/*
=================
idServerCcmdsSystemLocal::KillServer
=================
*/
void idServerCcmdsSystemLocal::KillServer_f(void) {
    serverInitSystem->Shutdown("killserver");
}

/*
=================
idServerCcmdsSystemLocal::WriteDemoMessage
=================
*/
void idServerCcmdsSystemLocal::WriteDemoMessage(client_t *cl, msg_t *msg,
        sint headerBytes) {
    sint len, swlen;

    // write the packet sequence
    len = cl->netchan.outgoingSequence;
    swlen = LittleLong(len);
    fileSystem->Write(&swlen, 4, cl->demo.demofile);

    // skip the packet sequencing information
    len = msg->cursize - headerBytes;
    swlen = LittleLong(len);
    fileSystem->Write(&swlen, 4, cl->demo.demofile);
    fileSystem->Write(msg->data + headerBytes, len, cl->demo.demofile);
}

/*
=================
idServerCcmdsSystemLocal::StopRecordDemo
=================
*/
void idServerCcmdsSystemLocal::StopRecordDemo(client_t *cl) {
    sint len;

    if(!cl->demo.demorecording) {
        Com_Printf("Client %d is not recording a demo.\n", cl - svs.clients);
        return;
    }

    // finish up
    len = -1;
    fileSystem->Write(&len, 4, cl->demo.demofile);
    fileSystem->Write(&len, 4, cl->demo.demofile);
    fileSystem->FCloseFile(cl->demo.demofile);
    cl->demo.demofile = 0;
    cl->demo.demorecording = false;
    Com_Printf("Stopped demo for client %d.\n", cl - svs.clients);
}

/*
=================
idServerCcmdsSystemLocal::StopAutoRecordDemos

stops all recording demos
=================
*/
void idServerCcmdsSystemLocal::StopAutoRecordDemos(void) {
    if(svs.clients && sv_autoRecDemo->integer) {
        for(client_t *client = svs.clients;
                client - svs.clients < sv_maxclients->integer; client++) {
            if(client->demo.demorecording) {
                StopRecordDemo(client);
            }
        }
    }
}

/*
====================
idServerCcmdsSystemLocal::StopRecording_f

stop recording a demo
====================
*/
void idServerCcmdsSystemLocal::StopRecord_f(void) {
    sint i;

    client_t *cl = nullptr;

    if(cmdSystem->Argc() == 2) {
        sint clIndex = atoi(cmdSystem->Argv(1));

        if(clIndex < 0 || clIndex >= sv_maxclients->integer) {
            Com_Printf("Unknown client number %d.\n", clIndex);
            return;
        }

        cl = &svs.clients[clIndex];
    } else {
        for(i = 0; i < sv_maxclients->integer; i++) {
            if(svs.clients[i].demo.demorecording) {
                cl = &svs.clients[i];
                break;
            }
        }

        if(cl == nullptr) {
            Com_Printf("No demo being recorded.\n");
            return;
        }
    }

    if(!cl->demo.demorecording) {
        Com_Printf("Client %d is not recording a demo.\n", cl - svs.clients);
        return;
    }

    StopRecordDemo(cl);
}

/*
==================
idServerCcmdsSystemLocal::DemoFilename
==================
*/
void idServerCcmdsSystemLocal::DemoFilename(valueType *buf, sint bufSize) {
    time_t rawtime;
    valueType timeStr[32] = { 0 }; // should really only reach ~19 chars

    time(&rawtime);
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d_%H-%M-%S",
             localtime(&rawtime));  // or gmtime

    Q_vsprintf_s(buf, bufSize, bufSize, "demo%s", timeStr);
}

/*
==================
idServerCcmdsSystemLocal::RecordDemo
==================
*/
void idServerCcmdsSystemLocal::RecordDemo(client_t *cl,
        valueType *demoName) {
    valueType       name[MAX_OSPATH];
    uchar8      bufData[MAX_MSGLEN];
    msg_t       msg;
    sint            len;

    if(cl->demo.demorecording) {
        Com_Printf("Already recording.\n");
        return;
    }

    if(cl->state != CS_ACTIVE) {
        Com_Printf("Client is not active.\n");
        return;
    }

    // open the demo file
    Q_strncpyz(cl->demo.demoName, demoName, sizeof(cl->demo.demoName));
    Q_vsprintf_s(name, sizeof(name), sizeof(name), "demos/%s.dm_%d",
                 cl->demo.demoName, PROTOCOL_VERSION);

    Com_Printf("recording to %s.\n", name);

    cl->demo.demofile = fileSystem->FOpenFileWrite(name);

    if(!cl->demo.demofile) {
        Com_Printf("ERROR: couldn't open.\n");
        return;
    }

    cl->demo.demorecording = true;

    // don't start saving messages until a non-delta compressed message is received
    cl->demo.demowaiting = true;

    cl->demo.isBot = (cl->netchan.remoteAddress.type == NA_BOT) ? true : false;
    cl->demo.botReliableAcknowledge = cl->reliableSent;

    // write out the gamestate message
    MSG_Init(&msg, bufData, sizeof(bufData));

    // NOTE, MRE: all server->client messages now acknowledge
    sint tmp = cl->reliableSent;
    idServerClientSystemLocal::CreateClientGameStateMessage(cl, &msg);
    cl->reliableSent = tmp;

    // finished writing the client packet
    MSG_WriteByte(&msg, svc_EOF);

    // write it to the demo file
    len = LittleLong(cl->netchan.outgoingSequence - 1);
    fileSystem->Write(&len, 4, cl->demo.demofile);

    len = LittleLong(msg.cursize);
    fileSystem->Write(&len, 4, cl->demo.demofile);
    fileSystem->Write(msg.data, msg.cursize, cl->demo.demofile);

    // the rest of the demo file will be copied from net messages
}

/*
==================
idServerCcmdsSystemLocal::AutoRecordDemo
==================
*/
void idServerCcmdsSystemLocal::AutoRecordDemo(client_t *cl) {
    valueType demoName[MAX_OSPATH];
    valueType demoFolderName[MAX_OSPATH];
    valueType demoFileName[MAX_OSPATH];
    valueType *demoNames[] = { demoFolderName, demoFileName };
    valueType date[MAX_OSPATH];
    valueType folderDate[MAX_OSPATH];
    valueType folderTreeDate[MAX_OSPATH];
    valueType demoPlayerName[MAX_NAME_LENGTH];
    time_t rawtime;
    struct tm *timeinfo;

    ::time(&rawtime);
    timeinfo = ::localtime(&rawtime);
    ::strftime(date, sizeof(date), "%Y-%m-%d_%H-%M-%S", timeinfo);

    timeinfo = ::localtime(&sv.realMapTimeStarted);

    ::strftime(folderDate, sizeof(folderDate), "%Y-%m-%d_%H-%M-%S", timeinfo);
    ::strftime(folderTreeDate, sizeof(folderTreeDate), "%Y/%m/%d", timeinfo);

    Q_strncpyz(demoPlayerName, cl->name, sizeof(demoPlayerName));

    Q_CleanStr(demoPlayerName);

    Q_vsprintf_s(demoFileName, sizeof(demoFileName), sizeof(demoFileName),
                 "%s %s", cvarSystem->VariableString("mapname"), date);
    Q_vsprintf_s(demoFolderName, sizeof(demoFolderName),
                 sizeof(demoFolderName), "%s %s", cvarSystem->VariableString("mapname"),
                 folderDate);

    // sanitize filename
    for(valueType **start = demoNames;
            start - demoNames < static_cast<sint32>(ARRAY_LEN(demoNames)); start++) {
        Q_strstrip(*start, "\n\r;:.?*<>|\\/\"", nullptr);
    }

    Q_vsprintf_s(demoName, sizeof(demoName), sizeof(demoName),
                 "autorecord/%s/%s/%s", folderTreeDate, demoFolderName, demoFileName);

    RecordDemo(cl, demoName);
}

/*
==================
idServerCcmdsSystemLocal::ExtractTimeFromDemoFolder
==================
*/
time_t idServerCcmdsSystemLocal::ExtractTimeFromDemoFolder(
    valueType *folder) {
    sint timeLen = strlen("0000-00-00_00-00-00");

    if(strlen(folder) < timeLen) {
        return 0;
    }

    struct tm timeinfo;

    timeinfo.tm_isdst = 0;

    sint numMatched = sscanf(folder + (strlen(folder) - timeLen),
                             "%4d-%2d-%2d_%2d-%2d-%2d",
                             &timeinfo.tm_year, &timeinfo.tm_mon, &timeinfo.tm_mday, &timeinfo.tm_hour,
                             &timeinfo.tm_min, &timeinfo.tm_sec);

    if(numMatched < 6) {
        // parsing failed
        return 0;
    }

    timeinfo.tm_year -= 1900;
    return mktime(&timeinfo);
}

/*
==================
idServerCcmdsSystemLocal::DemoFolderTimeComparator
==================
*/
sint QDECL idServerCcmdsSystemLocal::DemoFolderTimeComparator(
    const void *arg1, const void *arg2) {
    valueType *folder1 = (valueType *)arg1;
    valueType *folder2 = (valueType *)arg2;
    return ExtractTimeFromDemoFolder((valueType *)arg2) -
           ExtractTimeFromDemoFolder((valueType *)arg1);
}

/*
==================
idServerCcmdsSystemLocal::BeginAutoRecordDemos

starts demo recording on all active clients
==================
*/
void idServerCcmdsSystemLocal::BeginAutoRecordDemos(void) {
    if(sv_autoRecDemo->integer) {
        for(client_t *client = svs.clients;
                client - svs.clients < sv_maxclients->integer; client++) {
            if(client->state == CS_ACTIVE && !client->demo.demorecording) {
                if(client->netchan.remoteAddress.type != NA_BOT ||
                        sv_autoRecDemoBots->integer) {
                    AutoRecordDemo(client);
                }
            }
        }

        if(sv_autoRecDemoMaxMaps->integer > 0) {
            valueType fileList[MAX_QPATH * 500];
            valueType autorecordDirList[500][MAX_QPATH];
            sint autorecordDirListCount = 0;
            valueType *fileName;
            sint i;
            sint len;
            sint numFiles = fileSystem->GetFileList("demos", "/", fileList,
                                                    sizeof(fileList));

            fileName = fileList;

            for(i = 0; i < numFiles; i++) {
                if(Q_stricmp(fileName, ".") && Q_stricmp(fileName, "..")) {
                    Q_strncpyz(autorecordDirList[autorecordDirListCount++], fileName,
                               MAX_QPATH);
                }

                fileName += strlen(fileName) + 1;
            }

            qsort(autorecordDirList, autorecordDirListCount,
                  sizeof(autorecordDirList[0]), DemoFolderTimeComparator);

            for(i = sv_autoRecDemoMaxMaps->integer; i < autorecordDirListCount; i++) {
                fileSystem->HomeRmdir(va("demos/autorecord/%s", autorecordDirList[i]),
                                      true);
            }
        }
    }
}

/*
==================
idServerCcmdsSystemLocal::Record_f

Code is a merge of the cl_main.cpp function of the same name
and idServerCcmdsSystemLocal::SendClientGameState in serverClient.cpp
==================
*/
void idServerCcmdsSystemLocal::Record_f(void) {
    valueType demoName[MAX_OSPATH], name[MAX_OSPATH];
    sint i, start, len;
    valueType *s;

    if(svs.clients == nullptr) {
        Com_Printf("cannot record server demo - null svs.clients\n");
        return;
    }

    if(cmdSystem->Argc() > 3) {
        Com_Printf("record <demoname> <clientnum>\n");
        return;
    }

    client_t *cl;

    if(cmdSystem->Argc() == 3) {
        sint clIndex = atoi(cmdSystem->Argv(1));

        if(clIndex < 0 || clIndex >= sv_maxclients->integer) {
            Com_Printf("Unknown client number %d.\n", clIndex);
            return;
        }

        cl = &svs.clients[clIndex];
    } else {
        for(i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
            if(!cl->state) {
                continue;
            }

            if(cl->demo.demorecording) {
                continue;
            }

            if(cl->state == CS_ACTIVE) {
                break;
            }
        }
    }

    if(cl - svs.clients >= sv_maxclients->integer) {
        Com_Printf("No active client could be found.\n");
        return;
    }

    if(cl->demo.demorecording) {
        Com_Printf("Already recording.\n");
        return;
    }

    if(cl->state != CS_ACTIVE) {
        Com_Printf("Client is not active.\n");
        return;
    }

    if(cmdSystem->Argc() >= 2) {
        s = cmdSystem->Argv(1);
        Q_strncpyz(demoName, s, sizeof(demoName));
        Q_vsprintf_s(name, sizeof(name), sizeof(name), "demos/%s.dm_%d", demoName,
                     PROTOCOL_VERSION);
    } else {
        DemoFilename(demoName, sizeof(demoName));
        Q_vsprintf_s(name, sizeof(name), sizeof(name), "demos/%s.dm_%d", demoName,
                     PROTOCOL_VERSION);

        if(fileSystem->FileExists(name)) {
            Com_Printf("Record: Couldn't create a file\n");
            return;
        }
    }

    RecordDemo(cl, demoName);
}

/*
=================
idServerCcmdsSystemLocal::GameCompleteStatus_f

NERVE - SMF
=================
*/
void idServerCcmdsSystemLocal::GameCompleteStatus_f(void) {
    serverMainSystem->MasterGameCompleteStatus();
}

//===========================================================

/*
==================
idServerCcmdsSystemLocal::CompleteMapName
==================
*/
void idServerCcmdsSystemLocal::CompleteMapName(valueType *args,
        sint argNum) {
    if(argNum == 2) {
        cmdCompletionSystem->CompleteFilename("maps", "bsp", true);
    }
}

/*
=================
idServerCcmdsSystemLocal::ClientRedirect

Redirect console output to a client
=================
*/
static client_t *redirect_client = nullptr;

void idServerCcmdsSystemLocal::ClientRedirect(valueType *outputbuf) {
    serverMainSystem->SendServerCommand(redirect_client, "%s", outputbuf);
}

/*
====================
idServerCcmdsSystemLocal::StartRedirect_f
====================
*/
void idServerCcmdsSystemLocal::StartRedirect_f(void) {
#define SV_OUTPUTBUF_LENGTH (1024 - 16)
    sint clientNum;
    static valueType sv_outputbuf[SV_OUTPUTBUF_LENGTH];

    clientNum = atoi(cmdSystem->Argv(1));

    if(clientNum < 0 || clientNum >= sv_maxclients->integer) {
        return;
    }

    redirect_client = svs.clients + clientNum;

    Com_EndRedirect();
    Com_BeginRedirect(sv_outputbuf, SV_OUTPUTBUF_LENGTH, ClientRedirect);
}

/*
====================
idServerCcmdsSystemLocal::CompleteDemoName
====================
*/
void idServerCcmdsSystemLocal::CompleteDemoName(valueType *args,
        sint argNum) {
    if(argNum == 2) {
        valueType demoExt[16];

        Q_vsprintf_s(demoExt, sizeof(demoExt), sizeof(demoExt), ".svdm_%d",
                     PROTOCOL_VERSION);
        cmdCompletionSystem->CompleteFilename("svdemos", demoExt, true);
    }
}


/*
==================
idServerCcmdsSystemLocal::AddOperatorCommands
==================
*/
void idServerCcmdsSystemLocal::AddOperatorCommands(void) {
    static bool initialized;

    if(initialized) {
        return;
    }

    initialized = true;

    cmdSystem->AddCommand("heartbeat", &idServerCcmdsSystemLocal::Heartbeat_f,
                          "Sends an update from the server to the master server with the result of updating server info.");
    cmdSystem->AddCommand("status", &idServerCcmdsSystemLocal::Status_f,
                          "Reports map loaded, and information on all connected players.");
    cmdSystem->AddCommand("serverinfo",
                          &idServerCcmdsSystemLocal::Serverinfo_f,
                          "Shows server cvars on the local machine, including user created variables set with the sets command.");
    cmdSystem->AddCommand("systeminfo",
                          &idServerCcmdsSystemLocal::Systeminfo_f,
                          "eports settings for: g_syncronousclients sv_serverid");
    cmdSystem->AddCommand("dumpuser", &idServerCcmdsSystemLocal::DumpUser_f,
                          "Reports info on the specified user. Info includes: name, handicap, color, snd, model, snaps, rate");
    cmdSystem->AddCommand("map_restart",
                          &idServerCcmdsSystemLocal::MapRestart_f,
                          "Restarts the game on the current map. Replaces restart.");
    cmdSystem->AddCommand("fieldinfo", &idServerCcmdsSystemLocal::FieldInfo_f,
                          "Lists entitystate fields and playerstate fields and other data in the console, useful to developers.");
    cmdSystem->AddCommand("sectorlist",
                          &idServerWorldSystemLocal::SectorList_f,
                          "Lists sectors and number of entities in each on the currently loaded map. ");
    cmdSystem->AddCommand("map", &idServerCcmdsSystemLocal::Map_f,
                          "oads a map file, specifying cheats disabled. The .BSP file extension is not required. See also devmap.");
    cmdSystem->SetCommandCompletionFunc("map",
                                        &idServerCcmdsSystemLocal::CompleteMapName);
    cmdSystem->AddCommand("gameCompleteStatus",
                          &idServerCcmdsSystemLocal::GameCompleteStatus_f,
                          "Sends game complete status to master server.");  // NERVE - SMF
#ifndef PRE_RELEASE_DEMO_NODEVMAP
    cmdSystem->AddCommand("devmap", &idServerCcmdsSystemLocal::Map_f,
                          "Loads a map file specifying cheats 1. The .BSP file extension is not required. See also map");
    cmdSystem->SetCommandCompletionFunc("devmap",
                                        &idServerCcmdsSystemLocal::CompleteMapName);
    cmdSystem->AddCommand("spmap", &idServerCcmdsSystemLocal::Map_f,
                          "Loads a single player map file");
    cmdSystem->SetCommandCompletionFunc("devmap",
                                        &idServerCcmdsSystemLocal::CompleteMapName);
    cmdSystem->AddCommand("spdevmap", &idServerCcmdsSystemLocal::Map_f,
                          "Loads a single player map file specifying cheats 1. The .BSP file extension is not required. See also map");
    cmdSystem->SetCommandCompletionFunc("devmap",
                                        &idServerCcmdsSystemLocal::CompleteMapName);
#endif
    cmdSystem->AddCommand("loadgame", &idServerCcmdsSystemLocal::LoadGame_f,
                          "Loading a saved game file");
    cmdSystem->AddCommand("killserver",
                          &idServerCcmdsSystemLocal::KillServer_f,
                          "Command for terminating the server, but leaving application running.");
    cmdSystem->AddCommand("startRedirect",
                          &idServerCcmdsSystemLocal::StartRedirect_f, "Redirecting clients");
    cmdSystem->AddCommand("endRedirect", Com_EndRedirect,
                          "End of the cient redirection");

    cmdSystem->AddCommand("cheater",
                          &idServerOACSSystemLocal::ExtendedRecordSetCheater_f,
                          "Server-side command to set a client's cheater label cheater <client> <label> where label is 0 for honest players, and >= 1 for cheaters");

    cmdSystem->AddCommand("userinfo", &idServerCcmdsSystemLocal::UserInfo_f,
                          "List user information");
    cmdSystem->AddCommand("startmatch",
                          &idServerCcmdsSystemLocal::StartMatch_f, "Starts a the match.");
    cmdSystem->AddCommand("stopmatch", &idServerCcmdsSystemLocal::StopMatch_f,
                          "Stops the match.");
    cmdSystem->AddCommand("addclanmatch",
                          &idServerCcmdsSystemLocal::AddClanMatch_f, "Adding a clan match.");
    cmdSystem->AddCommand("addusermatch",
                          &idServerCcmdsSystemLocal::AddUserMatch_f, "Adding a user in the match.");
    cmdSystem->AddCommand("addrefereematch",
                          &idServerCcmdsSystemLocal::AddRefereeMatch_f,
                          "Adding a referee in the match.");
    cmdSystem->AddCommand("matchinfo", &idServerCcmdsSystemLocal::MatchInfo_f,
                          "Match information");
    cmdSystem->AddCommand("addip", &idServerCcmdsSystemLocal::AddIP_f,
                          "Banning a user IP and user GUID.");
    cmdSystem->AddCommand("banlist", &idServerCcmdsSystemLocal::BanList_f,
                          "Banning a user.");
    cmdSystem->AddCommand("unban", &idServerCcmdsSystemLocal::UnBan_f,
                          "Unbanning a user");

    cmdSystem->AddCommand("csstats_players",
                          &idServerCcmdsSystemLocal::StatsPlayers_f, "Statistics for the players.");
    cmdSystem->AddCommand("csstats_player",
                          &idServerCcmdsSystemLocal::StatsPlayer_f,
                          "Statistics for the one specific player.");

    cmdSystem->AddCommand("svrecord", &idServerCcmdsSystemLocal::Record_f, "");
    cmdSystem->AddCommand("svstoprecord",
                          &idServerCcmdsSystemLocal::StopRecord_f, "");


    if(com_dedicated->integer) {
        cmdSystem->AddCommand("say", &idServerCcmdsSystemLocal::ConSay_f,
                              "Used by the server. The text in the string is sent to all players as a message.");
    }
}
