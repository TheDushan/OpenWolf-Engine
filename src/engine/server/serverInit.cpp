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
// File name:   serverInit.cpp
// Created:     12/27/2018
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

idServerInitSystemLocal serverInitSystemLocal;
idServerInitSystem *serverInitSystem = &serverInitSystemLocal;

/*
===============
idServerInitSystemLocal::idServerInitSystemLocal
===============
*/
idServerInitSystemLocal::idServerInitSystemLocal(void) {
}

/*
===============
idServerBotSystemLocal::~idServerBotSystemLocal
===============
*/
idServerInitSystemLocal::~idServerInitSystemLocal(void) {
}

/*
===============
idServerInitSystemLocal::SendConfigstring

Creates and sends the server command necessary to update the CS index for the
given client
===============
*/
void idServerInitSystemLocal::SendConfigstring(client_t *client,
        sint index) {
    sint maxChunkSize = MAX_STRING_CHARS - 24;
    uint64 len;

    if(sv.configstrings[index].restricted &&
            Com_ClientListContains(&sv.configstrings[index].clientList,
                                   client - svs.clients)) {
        // Send a blank config string for this client if it's listed
        serverMainSystem->SendServerCommand(client, "cs %i \"\"\n", index);
        return;
    }

    len = strlen(sv.configstrings[index].s);

    if(len >= maxChunkSize) {
        sint sent = 0;
        uint64 remaining = len;
        valueType *cmd, buf[MAX_STRING_CHARS];

        while(remaining > 0) {
            if(sent == 0) {
                cmd = "bcs0";
            } else if(remaining < maxChunkSize) {
                cmd = "bcs2";
            } else {
                cmd = "bcs1";
            }

            Q_strncpyz(buf, &sv.configstrings[index].s[sent], maxChunkSize);

            serverMainSystem->SendServerCommand(client, "%s %i \"%s\"\n", cmd, index,
                                                buf);

            sent += (maxChunkSize - 1);
            remaining -= (maxChunkSize - 1);
        }
    } else {
        // standard cs, just send it
        serverMainSystem->SendServerCommand(client, "cs %i \"%s\"\n", index,
                                            sv.configstrings[index].s);
    }
}

/*
===============
idServerInitSystemLocal::UpdateConfigStrings
===============
*/
void idServerInitSystemLocal::UpdateConfigStrings(void) {
    sint len, i, index, maxChunkSize = MAX_STRING_CHARS - 24;
    client_t *client;

    for(index = 0; index < MAX_CONFIGSTRINGS; index++) {
        if(!sv.configstringsmodified[index]) {
            continue;
        }

        sv.configstringsmodified[index] = false;

        // send it to all the clients if we aren't
        // spawning a new server
        if(sv.state == SS_GAME || sv.restarting) {
            // send the data to all relevent clients
            for(i = 0, client = svs.clients; i < sv_maxclients->integer;
                    i++, client++) {
                if(client->state < CS_ACTIVE) {
                    if(client->state == CS_PRIMED) {
                        client->csUpdated[index] = true;
                    }

                    continue;
                }

                // do not always send server info to all clients
                if(index == CS_SERVERINFO && client->gentity &&
                        (client->gentity->r.svFlags & SVF_NOSERVERINFO)) {
                    continue;
                }

                // RF, don't send to bot/AI
                // Gordon: Note: might want to re-enable later for bot support
                // RF, re-enabled
                // Arnout: removed hardcoded gametype
                // Arnout: added coop
                if((serverGameSystem->GameIsSinglePlayer() ||
                        serverGameSystem->GameIsCoop()) && client->gentity &&
                        (client->gentity->r.svFlags & SVF_BOT)) {
                    continue;
                }

                len = strlen(sv.configstrings[index].s);

                SendConfigstring(client, index);
            }
        }
    }
}

/*
===============
idServerInitSystemLocal::SetConfigstringNoUpdate
===============
*/
void idServerInitSystemLocal::SetConfigstringNoUpdate(sint index,
        pointer val) {
    if(index < 0 || index >= MAX_CONFIGSTRINGS) {
        Com_Error(ERR_DROP,
                  "idServerInitSystemLocal::SetConfigstring: bad index %i\n", index);
    }

    if(!val) {
        val = "";
    }

    if(::strlen(val) >= BIG_INFO_STRING) {
        Com_Error(ERR_DROP,
                  "idServerInitSystemLocal::SetConfigstring: CS %d is too long\n", index);
    }

    // don't bother broadcasting an update if no change
    if(!strcmp(val, sv.configstrings[index].s)) {
        return;
    }

    // change the string in sv
    Z_Free(sv.configstrings[index].s);
    sv.configstrings[index].s = CopyString(val);
}

/*
===============
idServerInitSystemLocal::SetConfigstring
===============
*/
void idServerInitSystemLocal::SetConfigstring(sint index, pointer val) {
    sint i;
    client_t *client;

    if(index < 0 || index >= MAX_CONFIGSTRINGS) {
        Com_Error(ERR_DROP,
                  "idServerInitSystemLocal::SetConfigstring: bad index %i\n", index);
    }

    if(!val) {
        val = "";
    }

    // don't bother broadcasting an update if no change
    if(!strcmp(val, sv.configstrings[ index ].s)) {
        return;
    }

    // change the string in sv
    Z_Free(sv.configstrings[index].s);
    sv.configstrings[index].s = CopyString(val);

    // send it to all the clients if we aren't
    // spawning a new server
    if(sv.state == SS_GAME || sv.restarting) {
        // send the data to all relevent clients
        for(i = 0, client = svs.clients; i < sv_maxclients->integer ;
                i++, client++) {
            if(client->state < CS_ACTIVE) {
                if(client->state == CS_PRIMED) {
                    client->csUpdated[index] = true;
                }

                continue;
            }

            // do not always send server info to all clients
            if(index == CS_SERVERINFO && client->gentity &&
                    (client->gentity->r.svFlags & SVF_NOSERVERINFO)) {
                continue;
            }

            SendConfigstring(client, index);
        }
    }
}

/*
===============
idServerInitSystemLocal::GetConfigstring
===============
*/
void idServerInitSystemLocal::GetConfigstring(sint index,
        valueType *buffer, uint64 bufferSize) {
    if(bufferSize < 1) {
        Com_Error(ERR_DROP,
                  "idServerInitSystemLocal::GetConfigstring: bufferSize == %i", bufferSize);
    }

    if(index < 0 || index >= MAX_CONFIGSTRINGS) {
        Com_Error(ERR_DROP,
                  "idServerInitSystemLocal::GetConfigstring: bad index %i\n", index);
    }

    if(!sv.configstrings[index].s) {
        buffer[0] = 0;
        return;
    }

    Q_strncpyz(buffer, sv.configstrings[index].s, bufferSize);
}

/*
===============
idServerInitSystemLocal::SetConfigstringRestrictions
===============
*/
void idServerInitSystemLocal::SetConfigstringRestrictions(sint index,
        const clientList_t *clientList) {
    sint    i;
    clientList_t oldClientList = sv.configstrings[index].clientList;

    sv.configstrings[index].clientList = *clientList;
    sv.configstrings[index].restricted = true;

    for(i = 0 ; i < sv_maxclients->integer ; i++) {
        if(svs.clients[i].state >= CS_CONNECTED) {
            if(Com_ClientListContains(&oldClientList,
                                      i) != Com_ClientListContains(clientList, i)) {
                // A client has left or joined the restricted list, so update
                SendConfigstring(&svs.clients[i], index);
            }
        }
    }
}

/*
===============
idServerInitSystemLocal::SetUserinfo
===============
*/
void idServerInitSystemLocal::SetUserinfo(sint index, pointer val) {
    if(index < 0 || index >= sv_maxclients->integer) {
        Com_Error(ERR_DROP, "idServerInitSystemLocal::SetUserinfo: bad index %i\n",
                  index);
    }

    if(!val) {
        val = "";
    }

    Q_strncpyz(svs.clients[index].userinfo, val,
               sizeof(svs.clients[index].userinfo));
    Q_strncpyz(svs.clients[index].name, Info_ValueForKey(val, "name"),
               sizeof(svs.clients[index].name));
}

/*
===============
idServerInitSystemLocal::GetUserinfo
===============
*/
void idServerInitSystemLocal::GetUserinfo(sint index, valueType *buffer,
        uint64 bufferSize) {
    if(bufferSize < 1) {
        Com_Error(ERR_DROP,
                  "idServerInitSystemLocal::GetUserinfo: bufferSize == %i", bufferSize);
    }

    if(index < 0 || index >= sv_maxclients->integer) {
        Com_Error(ERR_DROP, "idServerInitSystemLocal::GetUserinfo: bad index %i\n",
                  index);
    }

    Q_strncpyz(buffer, svs.clients[index].userinfo, bufferSize);
}

/*
================
idServerInitSystemLocal::CreateBaseline

Entity baselines are used to compress non-delta messages
to the clients -- only the fields that differ from the
baseline will be transmitted
================
*/
void idServerInitSystemLocal::CreateBaseline(void) {
    sint entnum;
    sharedEntity_t *svent;

    for(entnum = 0; entnum < sv.num_entities; entnum++) {
        svent = serverGameSystem->GentityNum(entnum);

        if(!svent->r.linked) {
            continue;
        }

        svent->s.number = entnum;

        // take current state as baseline
        sv.svEntities[entnum].baseline = svent->s;
    }
}

/*
===============
idServerInitSystemLocal::BoundMaxClients
===============
*/
void idServerInitSystemLocal::BoundMaxClients(sint minimum) {
    // get the current maxclients value
    cvarSystem->Get("sv_maxclients", "20", 0,
                    "Sets the maximum number of clients hosted on the server. ");

    // START    xkan, 10/03/2002
    // allow many bots in single player. note that this pretty much means all previous
    // settings will be ignored (including the one set through "seta sv_maxclients <num>"
    // in user profile's wolfconfig_mp.cfg). also that if the user subsequently start
    // the server in multiplayer mode, the number of clients will still be the number
    // set here, which may be wrong - we can certainly just set it to a sensible number
    // when it is not in single player mode in the else part of the if statement when
    // necessary
    if(serverGameSystem->GameIsSinglePlayer() ||
            serverGameSystem->GameIsCoop()) {
        cvarSystem->Set("sv_maxclients", "64");
    }

    // END xkan, 10/03/2002

    sv_maxclients->modified = false;

    if(sv_maxclients->integer < minimum) {
        cvarSystem->Set("sv_maxclients", va("%i", minimum));
    } else if(sv_maxclients->integer > MAX_CLIENTS) {
        cvarSystem->Set("sv_maxclients", va("%i", MAX_CLIENTS));
    }
}

/*
===============
idServerInitSystemLocal::Startup

Called when a host starts a map when it wasn't running
one before.  Successive map or map_restart commands will
NOT cause this to be called, unless the game is exited to
the menu system first.
===============
*/
void idServerInitSystemLocal::Startup(void) {
    if(svs.initialized) {
        Com_Error(ERR_FATAL, "idServerInitSystemLocal::Startup: svs.initialized");
    }

    BoundMaxClients(1);

    // RF, avoid trying to allocate large chunk on a fragmented zone
    svs.clients = (client_t *)(calloc(sizeof(client_t) *
                                      sv_maxclients->integer, 1));

    if(!svs.clients) {
        Com_Error(ERR_FATAL,
                  "idServerInitSystemLocal::Startup: unable to allocate svs.clients");
    }

    if(com_dedicated->integer) {
        svs.numSnapshotEntities = sv_maxclients->integer * PACKET_BACKUP *
                                  MAX_SNAPSHOT_ENTITIES;
    } else {
        // we don't need nearly as many when playing locally
        svs.numSnapshotEntities = sv_maxclients->integer * 8 *
                                  MAX_SNAPSHOT_ENTITIES;
    }

    svs.initialized = true;

    // Don't respect sv_killserver unless a server is actually running
    if(sv_killserver->integer) {
        cvarSystem->Set("sv_killserver", "0");
    }

    cvarSystem->Set("sv_running", "1");

    // Join the ipv6 multicast group now that a map is running so clients can scan for us on the local network.
    networkSystem->JoinMulticast6();
}

/*
==================
idServerInitSystemLocal::ChangeMaxClients
==================
*/
void idServerInitSystemLocal::ChangeMaxClients(void) {
    sint oldMaxClients, i, count;
    client_t *oldClients;

    // get the highest client number in use
    count = 0;

    for(i = 0; i < sv_maxclients->integer; i++) {
        if(svs.clients[i].state >= CS_CONNECTED) {
            if(i > count) {
                count = i;
            }
        }
    }

    count++;

    oldMaxClients = sv_maxclients->integer;

    // never go below the highest client number in use
    BoundMaxClients(count);

    // if still the same
    if(sv_maxclients->integer == oldMaxClients) {
        return;
    }

    oldClients = (client_t *)(Hunk_AllocateTempMemory(count * sizeof(
                                  client_t)));

    // copy the clients to hunk memory
    for(i = 0; i < count; i++) {
        if(svs.clients[i].state >= CS_CONNECTED) {
            oldClients[i] = svs.clients[i];
        } else {
            ::memset(&oldClients[i], 0, sizeof(client_t));
        }
    }

    // free old clients arrays
    //Z_Free( svs.clients );
    free(svs.clients);   // RF, avoid trying to allocate large chunk on a fragmented zone

    // allocate new clients
    // RF, avoid trying to allocate large chunk on a fragmented zone
    svs.clients = (client_t *)(calloc(sizeof(client_t) *
                                      sv_maxclients->integer, 1));

    if(!svs.clients) {
        Com_Error(ERR_FATAL,
                  "idServerInitSystemLocal::Startup: unable to allocate svs.clients");
    }

    ::memset(svs.clients, 0, sv_maxclients->integer * sizeof(client_t));

    // copy the clients over
    for(i = 0; i < count; i++) {
        if(oldClients[i].state >= CS_CONNECTED) {
            svs.clients[i] = oldClients[i];
        }
    }

    // free the old clients on the hunk
    Hunk_FreeTempMemory(oldClients);

    // allocate new snapshot entities
    if(com_dedicated->integer) {
        svs.numSnapshotEntities = sv_maxclients->integer * PACKET_BACKUP *
                                  MAX_SNAPSHOT_ENTITIES;
    } else {
        // we don't need nearly as many when playing locally
        svs.numSnapshotEntities = sv_maxclients->integer * 4 *
                                  MAX_SNAPSHOT_ENTITIES;
    }
}

/*
====================
SV_SetExpectedHunkUsage

Sets com_expectedhunkusage, so the client knows how to draw the percentage bar
====================
*/
void idServerInitSystemLocal::SetExpectedHunkUsage(valueType *mapname) {
    sint handle, len;
    valueType *memlistfile = "hunkusage.dat", *buf, *buftrav, *token;

    len = fileSystem->FOpenFileByMode(memlistfile, &handle, FS_READ);

    if(len >= 0) { // the file exists, so read it in, strip out the current entry for this map, and save it out, so we can append the new value
        buf = static_cast< valueType * >(Z_Malloc(len + 1));
        ::memset(buf, 0, len + 1);

        fileSystem->Read(reinterpret_cast<void *>(buf), len, handle);
        fileSystem->FCloseFile(handle);

        // now parse the file, filtering out the current map
        buftrav = buf;

        while((token = COM_Parse(&buftrav)) != nullptr && token[0]) {
            if(!Q_stricmp(token, mapname)) {
                // found a match
                token = COM_Parse(&buftrav);     // read the size

                if(token && token[0]) {
                    // this is the usage
                    com_expectedhunkusage = atoi(token);
                    Z_Free(buf);
                    return;
                }
            }
        }

        Z_Free(buf);
    }

    // just set it to a negative number,so the cgame knows not to draw the percent bar
    com_expectedhunkusage = -1;
}

/*
================
idServerInitSystemLocal::ClearServer
================
*/
void idServerInitSystemLocal::ClearServer(void) {
    sint i;

    for(i = 0; i < MAX_CONFIGSTRINGS; i++) {
        if(sv.configstrings[i].s) {
            Z_Free(sv.configstrings[i].s);
        }
    }

    ::memset(&sv, 0, sizeof(sv));
}

/*
================
idServerInitSystemLocal::TouchCGameDLL

touch the cgame DLL so that a pure client (with DLL sv_pure support) can load do the correct checks
================
*/
void idServerInitSystemLocal::TouchCGameDLL(void) {
    sint ref;
    fileHandle_t f;
    pointer filename;

    filename = idsystem->GetDLLName("cgame");
    ref = fileSystem->FOpenFileRead_Filtered(filename, &f, false,
            FS_EXCLUDE_DIR);

    if(ref <= FS_GENERAL_REF && sv_pure->integer) {
        Com_Error(ERR_DROP,
                  "idServerInitSystemLocal::TouchCGameDLL - Failed to locate cgame DLL for pure server mode");
    }
}

/*
================
idServerInitSystemLocal::SpawnServer

Change the server to a new map, taking all connected
clients along with it.
This is NOT called for map_restart
================
*/
void idServerInitSystemLocal::SpawnServer(valueType *server,
        bool killBots) {
    sint i, checksum;
    pointer p;
    bool isBot;

    idServerCcmdsSystemLocal::StopAutoRecordDemos();

    svs.queryDone = 0;

    // ydnar: broadcast a level change to all connected clients
    if(svs.clients && !com_errorEntered) {
        FinalCommand("spawnserver", false);
    }

    // shut down the existing game if it is running
    serverGameSystem->ShutdownGameProgs();
    svs.gameStarted = false;

    Com_Printf("------ Server Initialization ------\n");
    Com_Printf("Server: %s\n", server);

    // if not running a dedicated server CL_MapLoading will connect the client to the server
    // also print some status stuff
    CL_MapLoading();

    // make sure all the client stuff is unloaded
    CL_ShutdownAll();

    // clear the whole hunk because we're (re)loading the server
    Hunk_Clear();

    // clear collision map data     // (SA) NOTE: TODO: used in missionpack
    collisionModelManager->ClearMap();

    // close all filehandles before fileSystem->Restart
    for(i = 0; i < sv_maxclients->integer; i++) {
        serverClientSystem->CloseDownload(&svs.clients[i]);
    }

    // wipe the entire per-level structure
    ClearServer();

    // MrE: main zone should be pretty much emtpy at this point
    // except for file system data and cached renderer data
#ifdef ZONE_DEBUG
    Z_LogHeap();
#endif

    // allocate empty config strings
    for(i = 0; i < MAX_CONFIGSTRINGS; i++) {
        sv.configstrings[i].s = CopyString("");
        sv.configstringsmodified[i] = false;
    }

    // init client structures and svs.numSnapshotEntities
    if(!cvarSystem->VariableValue("sv_running")) {
        Startup();
    } else {
        // check for maxclients change
        if(sv_maxclients->modified) {
            ChangeMaxClients();
        }
    }

    // clear pak references
    fileSystem->ClearPakReferences(0);

    // allocate the snapshot entities on the hunk
    svs.snapshotEntities = (entityState_t *)Hunk_Alloc(sizeof(
                               entityState_t) * svs.numSnapshotEntities, h_high);
    svs.nextSnapshotEntities = 0;

    // toggle the server bit so clients can detect that a
    // server has changed
    svs.snapFlagServerBit ^= SNAPFLAG_SERVERCOUNT;

    // set nextmap to the same map, but it may be overriden
    // by the game startup or another console command
    cvarSystem->Set("nextmap", "map_restart 0");
    //  cvarSystem->Set( "nextmap", va("map %s", server) );

    for(i = 0; i < sv_maxclients->integer; i++) {
        // save when the server started for each client already connected
        if(svs.clients[i].state >= CS_CONNECTED) {
            svs.clients[i].oldServerTime = sv.time;
        }
    }

    // Ridah
    // DHM - Nerve :: We want to use the completion bar in multiplayer as well
    // Arnout: just always use it
    if(!serverGameSystem->GameIsSinglePlayer()) {
        SetExpectedHunkUsage(va("maps/%s.bsp", server));
    } else {
        // just set it to a negative number,so the cgame knows not to draw the percent bar
        cvarSystem->Set("com_expectedhunkusage", "-1");
    }

    // make sure we are not paused
    cvarSystem->Set("cl_paused", "0");

    // get a new checksum feed and restart the file system
    srand(idsystem->Milliseconds());
    sv.checksumFeed = ((rand() << 16) ^ rand()) ^ idsystem->Milliseconds();

    // DO_LIGHT_DEDICATED
    // only comment out when you need a new pure checksum string and it's associated random feed
    //Com_DPrintf("idServerInitSystemLocal::SpawnServer checksum feed: %p\n", sv.checksumFeed);

    fileSystem->Restart(sv.checksumFeed);

    collisionModelManager->LoadMap(va("maps/%s.bsp", server), false,
                                   &checksum);

    // set serverinfo visible name
    cvarSystem->Set("mapname", server);

    cvarSystem->Set("sv_mapChecksum", va("%i", checksum));

    sv_newGameShlib = cvarSystem->Get("sv_newGameShlib", "", CVAR_TEMP,
                                      "Toggle with a new name to change dynamic module of a sgame without quiting the application");

    // serverid should be different each time
    sv.serverId = com_frameTime;
    sv.restartedServerId = sv.serverId;
    sv.checksumFeedServerId = sv.serverId;
    cvarSystem->Set("sv_serverid", va("%i", sv.serverId));

    time(&sv.realMapTimeStarted);

    // media configstring setting should be done during
    // the loading stage, so connected clients don't have
    // to load during actual gameplay
    sv.state = SS_LOADING;

    cvarSystem->Set("sv_serverRestarting", "1");

    // load and spawn all other entities
    serverGameSystem->InitGameProgs();

    // don't allow a map_restart if game is modified
    // Arnout: there isn't any check done against this, obsolete
    //  sv_gametype->modified = false;

    // run a few frames to allow everything to settle
    for(i = 0; i < GAME_INIT_FRAMES; i++) {
        sgame->RunFrame(sv.time);
        sv.time += 100;
        svs.time += FRAMETIME;
    }

    // create a baseline for more efficient communications
    CreateBaseline();

    for(i = 0; i < sv_maxclients->integer; i++) {
        // send the new gamestate to all connected clients
        if(svs.clients[i].state >= CS_CONNECTED) {
            valueType *denied;

            if(svs.clients[i].netchan.remoteAddress.type == NA_BOT) {
                if(killBots || serverGameSystem->GameIsSinglePlayer() ||
                        serverGameSystem->GameIsCoop()) {
                    serverClientSystem->DropClient(&svs.clients[i], "");
                    continue;
                }

                isBot = true;
            } else {
                isBot = false;
            }

            // connect the client again
            denied = static_cast<valueType *>(sgame->ClientConnect(i,
                                              false));    // firstTime = false

            if(denied) {
                // this generally shouldn't happen, because the client
                // was connected before the level change
                serverClientSystem->DropClient(&svs.clients[i], denied);
            } else {
                if(!isBot) {
                    // when we get the next packet from a connected client,
                    // the new gamestate will be sent
                    svs.clients[i].state = CS_CONNECTED;
                } else {
                    client_t *client;
                    sharedEntity_t *ent;

                    client = &svs.clients[i];
                    client->state = CS_ACTIVE;
                    ent = serverGameSystem->GentityNum(i);
                    ent->s.number = i;
                    client->gentity = ent;

                    client->deltaMessage = -1;
                    client->nextSnapshotTime = svs.time;    // generate a snapshot immediately

                    sgame->ClientBegin(i);
                }
            }
        }
    }

    // run another frame to allow things to look at all the players
    sgame->RunFrame(sv.time);
    sv.time += 100;
    svs.time += FRAMETIME;

    if(sv_pure->integer) {
        // the server sends these to the clients so they will only
        // load pk3s also loaded at the server
        p = fileSystem->LoadedPakChecksums();
        cvarSystem->Set("sv_paks", p);

        if(strlen(p) == 0) {
            Com_Printf("WARNING: sv_pure set but no PK3 files loaded\n");
        }

        p = fileSystem->LoadedPakNames();
        cvarSystem->Set("sv_pakNames", p);
    } else {
        cvarSystem->Set("sv_paks", "");
        cvarSystem->Set("sv_pakNames", "");
    }

    // the server sends these to the clients so they can figure
    // out which pk3s should be auto-downloaded
    // NOTE: we consider the referencedPaks as 'required for operation'

    // we want the server to reference the bin pk3 that the client is expected to load from
    TouchCGameDLL();

    p = fileSystem->ReferencedPakChecksums();
    cvarSystem->Set("sv_referencedPaks", p);
    p = fileSystem->ReferencedPakNames();
    cvarSystem->Set("sv_referencedPakNames", p);

    // save systeminfo and serverinfo strings
    cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
    SetConfigstring(CS_SYSTEMINFO,
                    cvarSystem->InfoString_Big(CVAR_SYSTEMINFO));

    SetConfigstring(CS_SERVERINFO,
                    cvarSystem->InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE));
    cvar_modifiedFlags &= ~CVAR_SERVERINFO;

    // NERVE - SMF
    SetConfigstring(CS_WOLFINFO, cvarSystem->InfoString(CVAR_WOLFINFO));
    cvar_modifiedFlags &= ~CVAR_WOLFINFO;

    // any media configstring setting now should issue a warning
    // and any configstring changes should be reliably transmitted
    // to all clients
    sv.state = SS_GAME;

    // send a heartbeat now so the master will get up to date info
    serverCcmdsLocal.Heartbeat_f();

    Hunk_SetMark();

    idServerCommunityServer::SaveStatistics();
    idServerCommunityServer::InitStatistics();

    UpdateConfigStrings();

    cvarSystem->Set("sv_serverRestarting", "0");

    for(client_t *client = svs.clients;
            client - svs.clients < sv_maxclients->integer; client++) {
        // bots will not request gamestate, so it must be manually sent
        // cannot do this above where it says it will because mapname is not set at that time
        if(client->netchan.remoteAddress.type == NA_BOT &&
                client->demo.demorecording) {
            idServerClientSystemLocal::SendClientGameState(client);
        }
    }

    idServerCcmdsSystemLocal::BeginAutoRecordDemos();

    Com_Printf("-----------------------------------\n");
}

// Update Server
/*
====================
idServerInitSystemLocal::ParseVersionMapping

Reads versionmap.cfg which sets up a mapping of client version to installer to download
====================
*/
void idServerInitSystemLocal::ParseVersionMapping(void) {
#if defined (UPDATE_SERVER)
    sint handle, len;
    valueType *filename = "versionmap.cfg", *buf, *buftrav, *token;

    len = fileSystem->SV_FOpenFileRead(filename, &handle);

    // the file exists
    if(len >= 0) {
        buf = static_cast<valueType *>(Z_Malloc(len + 1));
        memset(buf, 0, len + 1);

        fileSystem->Read(reinterpret_cast<void *>(buf), len, handle);
        fileSystem->FCloseFile(handle);

        // now parse the file, setting the version table info
        buftrav = buf;

        token = COM_Parse(&buftrav);

        if(strcmp(token, "OpenWolf-VersionMap")) {
            Z_Free(buf);
            Com_Error(ERR_FATAL, "invalid versionmap.cfg");
            return;
        }

        Com_Printf("\n------------Update Server-------------\n\nParsing version map...\n");

        while((token = COM_Parse(&buftrav)) && token[0]) {
            // read the version number
            Q_strcpy_s(versionMap[ numVersions ].version, token);

            // read the platform
            token = COM_Parse(&buftrav);

            if(token && token[0]) {
                Q_strcpy_s(versionMap[ numVersions ].platform, token);
            } else {
                Z_Free(buf);
                Com_Error(ERR_FATAL, "error parsing versionmap.cfg, after %s",
                          versionMap[numVersions].platform);
                return;
            }

            // read the installer name
            token = COM_Parse(&buftrav);

            if(token && token[0]) {
                Q_strcpy_s(versionMap[ numVersions ].installer, token);
            } else {
                Z_Free(buf);
                Com_Error(ERR_FATAL, "error parsing versionmap.cfg, after %s",
                          versionMap[numVersions].installer);
                return;
            }

            numVersions++;

            if(numVersions >= MAX_UPDATE_VERSIONS) {
                Z_Free(buf);
                Com_Error(ERR_FATAL, "Exceeded maximum number of mappings(%d)",
                          MAX_UPDATE_VERSIONS);
                return;
            }

        }

        Com_Printf(" found %d mapping%c\n--------------------------------------\n\n",
                   numVersions, numVersions > 1 ? 's' : ' ');

        Z_Free(buf);
    } else {
        Com_Error(ERR_FATAL, "Couldn't open versionmap.cfg");
    }

#endif
}

/*
===============
idServerInitSystemLocal::Init

Only called at main exe startup, not for each game
===============
*/
void idServerInitSystemLocal::Init(void) {
    sint index;

    serverCcmdsLocal.AddOperatorCommands();

    // serverinfo vars
    cvarSystem->Get("dmflags", "0", /*CVAR_SERVERINFO */ 0,
                    "Sets the game options for deathmatch play.");
    cvarSystem->Get("fraglimit", "0", /*CVAR_SERVERINFO */ 0,
                    "Sets the number of frags required for the game to be won if timelimit has been not reached or set. Setting to 0 disables fraglimit. ");
    cvarSystem->Get("timelimit", "0", CVAR_SERVERINFO,
                    "Sets the amount of time before a game will end if fraglimit is not reached or set. Setting to 0 disables timelimit. ");

    cvarSystem->Get("sv_keywords", "", CVAR_SERVERINFO,
                    "Variable holds the search string entered in the internet connection menu");
    cvarSystem->Get("protocol", va("%i", PROTOCOL_VERSION),
                    CVAR_SERVERINFO | CVAR_ARCHIVE,
                    "Display network protocol version. Useful for backward compatibility with servers with otherwise incompatible versions.");
    sv_mapname = cvarSystem->Get("mapname", "nomap",
                                 CVAR_SERVERINFO | CVAR_ROM,
                                 "Display the name of the current map being used");
    sv_privateClients = cvarSystem->Get("sv_privateClients", "0",
                                        CVAR_SERVERINFO,
                                        "The number of spots, out of sv_maxclients, reserved for players with the server password (sv_privatePassword)");
    sv_hostname = cvarSystem->Get("sv_hostname", "OpenWolf Host",
                                  CVAR_SERVERINFO | CVAR_ARCHIVE,
                                  "The name of the server in server browsers.");
    //
#ifdef __MACOS__
    sv_maxclients = cvarSystem->Get("sv_maxclients", "16",
                                    CVAR_SERVERINFO | CVAR_LATCH,
                                    "Number of players allowed to connect wen running a server. 16 is a 'soft' limit—it is the maximum recommended. Many servers run 32 or more players. You could set this lower than 2, but then you only have one player on a server; better to use /devmap if you want to play alone.");   //DAJ HOG
#else
    sv_maxclients = cvarSystem->Get("sv_maxclients", "20",
                                    CVAR_SERVERINFO | CVAR_LATCH,
                                    "Number of players allowed to connect wen running a server. 16 is a 'soft' limit—it is the maximum recommended. Many servers run 32 or more players. You could set this lower than 2, but then you only have one player on a server; better to use /devmap if you want to play alone.");   // NERVE - SMF - changed to 20 from 8
#endif

    sv_maxRate = cvarSystem->Get("sv_maxRate", "0",
                                 CVAR_ARCHIVE | CVAR_SERVERINFO,
                                 "Option to force all clients to play with a max rate. This can be used to limit the advantage of low pings, or to cap bandwidth utilization for a server. Note that rate is ignored for clients that are on the same LAN.");
    sv_minPing = cvarSystem->Get("sv_minPing", "0",
                                 CVAR_ARCHIVE | CVAR_SERVERINFO,
                                 "Set the minimum ping aloud on the server to keep low pings out");
    sv_maxPing = cvarSystem->Get("sv_maxPing", "0",
                                 CVAR_ARCHIVE | CVAR_SERVERINFO,
                                 "Set the maximum ping allowed on the server to keep high pings out");
    sv_dlRate = cvarSystem->Get("sv_dlRate", "100",
                                CVAR_ARCHIVE | CVAR_SERVERINFO,
                                "Bandwidth allotted to PK3 file downloads via UDP, in kbyte / s");
    sv_floodProtect = cvarSystem->Get("sv_floodProtect", "1",
                                      CVAR_ARCHIVE | CVAR_SERVERINFO,
                                      "Whether or not to use flood protection, preventing clients from sending numerous consecutive commands to the server.");
    sv_allowAnonymous = cvarSystem->Get("sv_allowAnonymous", "0",
                                        CVAR_SERVERINFO, "Allow anonymous connections in the server");
    sv_friendlyFire = cvarSystem->Get("g_friendlyFire", "1",
                                      CVAR_SERVERINFO | CVAR_ARCHIVE,
                                      "Toggles wether players can damage their teammates");  // NERVE - SMF
    sv_maxlives = cvarSystem->Get("g_maxlives", "0",
                                  CVAR_ARCHIVE | CVAR_LATCH | CVAR_SERVERINFO,
                                  "Number of lives (respawns) all players have. 0 = unlimited");    // NERVE - SMF
    sv_needpass = cvarSystem->Get("g_needpass", "0",
                                  CVAR_SERVERINFO | CVAR_ROM,
                                  "Toggles requiring a password for players to join");

    // systeminfo
    //bani - added convar_t for sv_cheats so server engine can reference it
    sv_cheats = cvarSystem->Get("sv_cheats", "0", CVAR_SYSTEMINFO | CVAR_ROM,
                                "Enable cheats (serverside only)");
    sv_serverid = cvarSystem->Get("sv_serverid", "0",
                                  CVAR_SYSTEMINFO | CVAR_ROM,
                                  "The identification number of the local server. ");
    sv_pure = cvarSystem->Get("sv_pure", "1", CVAR_SYSTEMINFO,
                              "Toggles check that client's files are the same as the servers (basic anticheat).");

    cvarSystem->Get("sv_paks", "", CVAR_SYSTEMINFO | CVAR_ROM,
                    "Variable holds the checksum of all pk3 files");
    cvarSystem->Get("sv_pakNames", "", CVAR_SYSTEMINFO | CVAR_ROM,
                    "Variable holds a list of all the pk3 files the server found");
    cvarSystem->Get("sv_referencedPaks", "", CVAR_SYSTEMINFO | CVAR_ROM,
                    "Variable holds the checksum of the referenced pk3 files");
    cvarSystem->Get("sv_referencedPakNames", "", CVAR_SYSTEMINFO | CVAR_ROM,
                    "Variable holds a list of all the pk3 files the server loaded data from. these pk3 files will be autodownloaded by a client if the client does not have them.");

    // server vars
    sv_rconPassword = cvarSystem->Get("rconPassword", "", CVAR_TEMP,
                                      "RCon password, for server admins to use Remote Console commands.");
    sv_privatePassword = cvarSystem->Get("sv_privatePassword", "", CVAR_TEMP,
                                         "Set password for private clients to login with");
    sv_fps = cvarSystem->Get("sv_fps", "20", CVAR_TEMP,
                             "Set the max frames per second the server sends the client");
    sv_timeout = cvarSystem->Get("sv_timeout", "240", CVAR_TEMP,
                                 "Sets the amount of time for the server to wait for a client packet before assuming a disconnected state.");
    sv_zombietime = cvarSystem->Get("sv_zombietime", "2", CVAR_TEMP,
                                    "The amount of time in minutes before a frozen character is removed from the map.");
    cvarSystem->Get("nextmap", "", CVAR_TEMP,
                    "Start the next map in the rotation");

    sv_allowDownload = cvarSystem->Get("sv_allowDownload", "1", CVAR_ARCHIVE,
                                       "Whether the server will allow data to be downloaded from it (default 0) .");

    sv_master[0] = cvarSystem->Get("sv_master1", MASTER_SERVER_NAME, 0,
                                   "Set URL or address to master server");

    for(index = 1; index < MAX_MASTER_SERVERS; index++) {
        sv_master[index] = cvarSystem->Get(va("sv_master%d", index + 1), "",
                                           CVAR_ARCHIVE, "Set URL or address to master server");
    }

    sv_reconnectlimit = cvarSystem->Get("sv_reconnectlimit", "3", 0,
                                        "Number of times a disconnected client can come back and reconnect");
    sv_tempbanmessage = cvarSystem->Get("sv_tempbanmessage",
                                        "You have been kicked and are temporarily banned from joining this server.",
                                        0, "What player's get after being kicked - “you have been kicked and are temporarily banned...");
    sv_showloss = cvarSystem->Get("sv_showloss", "0", 0,
                                  "Toggle sever packet loss display");
    sv_padPackets = cvarSystem->Get("sv_padPackets", "0", 0,
                                    "Toggles the padding of network packets on the server PAD - Packet Assembler/Disassembler");
    sv_killserver = cvarSystem->Get("sv_killserver", "0", 0,
                                    "When set to 1, kills the server (shuts down map or demo) without shutting down the application. Value then returns to 0.");
    sv_mapChecksum = cvarSystem->Get("sv_mapChecksum", "", CVAR_ROM,
                                     "Reports the file size of the currently loaded map. Used to prevent cheating by ensuring all clients are not using hacked maps.");

    sv_lanForceRate = cvarSystem->Get("sv_lanForceRate", "1", CVAR_ARCHIVE,
                                      "Toggle for forcing very high rate setting for clients detected (sometimes wrongly) as connecting via LAN.");

    sv_onlyVisibleClients = cvarSystem->Get("sv_onlyVisibleClients", "0", 0,
                                            "Showing only visible clients while generating a new challenge");  // DHM - Nerve

    sv_showAverageBPS = cvarSystem->Get("sv_showAverageBPS", "0", 0,
                                        "BSP Network debugging");  // NERVE - SMF - net debugging

    sv_cs_ServerType = cvarSystem->Get("sv_cs_ServerType", "0", 0,
                                       "Setup server type for the community server. 0: public, 1: public-registered, 2: private.");
    sv_cs_Salt = cvarSystem->Get("sv_cs_Salt", "12345", 0,
                                 "Community server hash password field.");
    sv_cs_BotLog = cvarSystem->Get("sv_cs_BotLog", "0", 0,
                                   "Log bots in the community server.");
    sv_cs_MemberColor = cvarSystem->Get("sv_cs_MemberColor", "0 255 0", 0,
                                        "Color of the registered user in the community server.");
    sv_cs_UnknownColor = cvarSystem->Get("sv_cs_UnknownColor", "255 0 0", 0,
                                         "Color of the unknown player in the community server.");
    sv_cs_PrivateOnlyMSG = cvarSystem->Get("sv_cs_PrivateOnlyMSG",
                                           "This server is for registered users only. Register at " PRODUCT_NAME
                                           ".com", 0, "Send private message only to the registered user.");
    sv_cs_stats = cvarSystem->Get("sv_cs_stats", "", 0,
                                  "Enter the address of the community server. Example: xxx.xxx.xxx.xxx:port");
    sv_cs_ServerPort = cvarSystem->Get("sv_cs_ServerPort", "0", 0,
                                       "Enter the port address of the community server.");

    //Dushan - we need to clean all this
    // NERVE - SMF - create user set cvars
    cvarSystem->Get("g_userTimeLimit", "0", 0,
                    "	Sets the timelimit for the round");
    cvarSystem->Get("g_userAlliedRespawnTime", "0", 0,
                    "Sets the respawn interval for Axis");
    cvarSystem->Get("g_userAxisRespawnTime", "0", 0,
                    "Sets the respawn interval for Allies");
    cvarSystem->Get("g_maxlives", "0", 0,
                    "Number of lives (respawns) all players have. 0 = unlimited");
    cvarSystem->Get("g_altStopwatchMode", "0", CVAR_ARCHIVE,
                    "Toggles ABAB stopwatch format instead of ABBA");
    cvarSystem->Get("g_minGameClients", "8", CVAR_SERVERINFO,
                    "The minimum number of players needed for a round to begin");
    cvarSystem->Get("g_complaintlimit", "6", CVAR_ARCHIVE,
                    "Kick player after this number of complaints");
    cvarSystem->Get("gamestate", "-1", CVAR_WOLFINFO | CVAR_ROM, "Gamestate");
    cvarSystem->Get("g_currentRound", "0", CVAR_WOLFINFO, "Current round");
    cvarSystem->Get("g_nextTimeLimit", "0", CVAR_WOLFINFO, "Next time limit");
    // -NERVE - SMF

    // TTimo - some UI additions
    // NOTE: sucks to have this hardcoded really, I suppose this should be in UI
    cvarSystem->Get("g_axismaxlives", "0", 0,
                    "Sets the maximum number of lives available to the Axis team");
    cvarSystem->Get("g_alliedmaxlives", "0", 0,
                    "Sets the maximum number of lives available to the Allied team");
    cvarSystem->Get("g_fastres", "0", CVAR_ARCHIVE,
                    "Player is instantly active after being revived");
    cvarSystem->Get("g_fastResMsec", "1000", CVAR_ARCHIVE,
                    "Duration of invulnerability if g_fastResMsec is >0?");

    // ATVI Tracker Wolfenstein Misc #273
    cvarSystem->Get("g_voteFlags", "0", CVAR_ROM | CVAR_SERVERINFO,
                    "Sets voting options available to players (note the match_ overrule?)");

    // ATVI Tracker Wolfenstein Misc #263
    cvarSystem->Get("g_antilag", "1", CVAR_ARCHIVE | CVAR_SERVERINFO,
                    "Toggles antilag, better for high ping players.");

    cvarSystem->Get("g_needpass", "0", CVAR_SERVERINFO,
                    "Toggles requiring a password for players to join");

    g_gameType = cvarSystem->Get("g_gametype", va("%i",
                                 com_gameInfo.defaultGameType), CVAR_SERVERINFO | CVAR_LATCH,
                                 "Sets the type of game being played, 2=objective, 3=stopwatch, 4=campaign, 5=LMS");

#if !defined (UPDATE_SERVER)
    // the download netcode tops at 18/20 kb/s, no need to make you think you can go above
    sv_dl_maxRate = cvarSystem->Get("sv_dl_maxRate", "42000", CVAR_ARCHIVE,
                                    "Sets the maximum speed clients can download files from the server");
#else
    // the update server is on steroids, sv_fps 60 and no snapshotMsec limitation, it can go up to 30 kb/s
    sv_dl_maxRate = cvarSystem->Get("sv_dl_maxRate", "60000", CVAR_ARCHIVE,
                                    "Sets the maximum speed clients can download files from the server");
#endif

    sv_minimumAgeGuid = cvarSystem->Get("sv_minimumAgeGuid", "0",
                                        CVAR_ARCHIVE | CVAR_SERVERINFO, "Min guid age to enter a server");
    sv_maximumAgeGuid = cvarSystem->Get("sv_maximumAgeGuid", "0",
                                        CVAR_ARCHIVE | CVAR_SERVERINFO, "Max guid age to enter a server");

    sv_wwwDownload = cvarSystem->Get("sv_wwwDownload", "0", CVAR_ARCHIVE,
                                     "Toggles enabling www download redirect");
    sv_wwwBaseURL = cvarSystem->Get("sv_wwwBaseURL", "", CVAR_ARCHIVE,
                                    "Sets the location of www download redirect");
    sv_wwwDlDisconnected = cvarSystem->Get("sv_wwwDlDisconnected", "0",
                                           CVAR_ARCHIVE,
                                           "Wether to disconnect players from gameserver while they download via www");
    sv_wwwFallbackURL = cvarSystem->Get("sv_wwwFallbackURL", "", CVAR_ARCHIVE,
                                        "Alternative URL to download the files from");

    //bani
    sv_packetloss = cvarSystem->Get("sv_packetloss", "0", CVAR_CHEAT,
                                    "Mimic packet loss.");
    sv_packetdelay = cvarSystem->Get("sv_packetdelay", "0", CVAR_CHEAT,
                                     "Mimic packet dealy.");

    // fretn - note: redirecting of clients to other servers relies on this,
    // ET://someserver.com
    sv_fullmsg = cvarSystem->Get("sv_fullmsg", "Server is full.", CVAR_ARCHIVE,
                                 "Customise the server full message, or redirect to another server : sv_fullmsg OW://host.to.redirect.to:port");

    sv_hibernateTime = cvarSystem->Get("sv_hibernateTime", "0", CVAR_ARCHIVE,
                                       "Switches the server to a hibernation mode in which it uses less CPU power when no player is connected. The value is the time in milliseconds after which it automatically switches to the said state when the last player disconnected from the server. The value zero disables hibernation mode.");
    svs.hibernation.sv_fps = sv_fps->value;

    // oacs extended recording variables
    sv_oacsEnable = cvarSystem->Get("sv_oacsEnable", "0", CVAR_ARCHIVE,
                                    "Enable the extended logging facility");
    sv_oacsPlayersTableEnable = cvarSystem->Get("sv_oacsPlayersTableEnable",
                                "1", CVAR_ARCHIVE, "Enable the extended player identification logging");
    sv_oacsTypesFile = cvarSystem->Get("sv_oacsTypesFile", "oacs/types.txt",
                                       CVAR_ARCHIVE, "Where to save the features types");
    sv_oacsDataFile = cvarSystem->Get("sv_oacsDataFile", "oacs/data.txt",
                                      CVAR_ARCHIVE, "Where to save the features data");
    sv_oacsPlayersTable = cvarSystem->Get("sv_oacsPlayersTable",
                                          "oacs/playerstable.txt", CVAR_ARCHIVE,
                                          "Where to save the players table (if enabled)");
    sv_oacsMinPlayers = cvarSystem->Get("sv_oacsMinPlayers", "1", CVAR_ARCHIVE,
                                        "Minimum number of human players required to begin logging data");
    sv_oacsLabelPassword = cvarSystem->Get("sv_oacsLabelPassword", "",
                                           CVAR_TEMP, "Password necessary for a player to label himself");
    sv_oacsMaxPing = cvarSystem->Get("sv_oacsMaxPing", "700", CVAR_ARCHIVE,
                                     "Max ping to accept interframes (above, the interframe will be dropped until the ping goes down)");
    sv_oacsMaxLastPacketTime = cvarSystem->Get("sv_oacsMaxLastPacketTime",
                               "10000", CVAR_ARCHIVE,
                               "Max last packet time to accept interframes (above, the interframe will be dropped until the LastPacketTime goes down)");

    sv_wh_active = cvarSystem->Get("sv_wh_active", "0", CVAR_ARCHIVE,
                                   "Enable wallhack protection on the server.");
    sv_wh_bbox_horz = cvarSystem->Get("sv_wh_bbox_horz", "60", CVAR_ARCHIVE,
                                      "These is the horizontal dimension (in Quake units) of the players' bounding boxes used for performing line-of-sight traces.");

    if(sv_wh_bbox_horz->integer < 20) {
        cvarSystem->Set("sv_wh_bbox_horz", "10");
    }

    if(sv_wh_bbox_horz->integer > 50) {
        cvarSystem->Set("sv_wh_bbox_horz", "50");
    }

    sv_wh_bbox_vert = cvarSystem->Get("sv_wh_bbox_vert", "60", CVAR_ARCHIVE,
                                      "These is the vertical dimension (in Quake units) of the players' bounding boxes used for performing line-of-sight traces");

    if(sv_wh_bbox_vert->integer < 10) {
        cvarSystem->Set("sv_wh_bbox_vert", "30");
    }

    if(sv_wh_bbox_vert->integer > 50) {
        cvarSystem->Set("sv_wh_bbox_vert", "80");
    }

    sv_wh_check_fov = cvarSystem->Get("sv_wh_check_fov", "0", CVAR_ARCHIVE,
                                      "Enable wallhack protection only when players are in their respective FOV.");

    sv_autoRecDemo = cvarSystem->Get("sv_autoRecDemo", "0", CVAR_ARCHIVE,
                                     "Toggle (default off) auto demo recording of human players.");
    sv_autoRecDemoBots = cvarSystem->Get("sv_autoRecDemoBots", "0",
                                         CVAR_ARCHIVE, "If above is turned on, also record bots.");
    sv_autoRecDemoMaxMaps = cvarSystem->Get("sv_autoRecDemoMaxMaps", "0",
                                            CVAR_ARCHIVE,
                                            " Adjust how many maps, demos will be kept (default 0, probably should set if turning auto record on).");

#if !defined (UPDATE_SERVER)
    idServerWallhackSystemLocal::InitWallhack();
#endif

    // can't ResolveAuthHost() here since networkSystem->Init() hasn't been
    // called yet; works if we move ResolveAuthHost() to platform/systemMain.cpp
    // but that's introducing another #ifdef DEDICATED there, kinda sad; seems
    // that Rambetter added his stuff in Frame() but that incurs (at least
    // some) overhead on each frame; the overhead may not be so bad since the
    // HEARTBEAT stuff is also handled in Frame(), who knows; but we found
    // a better way: Startup() and SpawnServer(), check the code there!

    svs.serverLoad = -1;

#if defined(UPDATE_SERVER)
    Startup();
    ParseVersionMapping();

    // serverid should be different each time
    sv.serverId = com_frameTime + 100;
    sv.restartedServerId =
        sv.serverId; // I suppose the init here is just to be safe
    sv.checksumFeedServerId = sv.serverId;
    cvarSystem->Set("sv_serverid", va("%i", sv.serverId));
    cvarSystem->Set("mapname", "Update");

    // allocate empty config strings
    {
        sint i;

        for(i = 0 ; i < MAX_CONFIGSTRINGS ; i++) {
            sv.configstrings[i].s = CopyString("");
        }
    }
#endif

    // OACS: Initialize the interframe/features variable and write down the extended records structures (types)
    if(sv_oacsEnable->integer == 1) {
#if !defined(UPDATE_SERVER)
        idServerOACSSystemLocal::ExtendedRecordInit();
#endif
    }

    idServerCommunityServer::StartUp();
}


/*
==================
idServerInitSystemLocal::FinalCommand

Used by idServerInitSystemLocal::Shutdown to send a final message to all
connected clients before the server goes down.  The messages are sent immediately,
not just stuck on the outgoing message list, because the server is going
to totally exit after returning from this function.
==================
*/
void idServerInitSystemLocal::FinalCommand(valueType *cmd,
        bool disconnect) {
    sint i, j;
    client_t *cl;

    // send it twice, ignoring rate
    for(j = 0; j < 2; j++) {
        for(i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
            if(cl->state >= CS_CONNECTED) {
                // don't send a disconnect to a local client
                if(cl->netchan.remoteAddress.type != NA_LOOPBACK) {
                    //serverMainSystem->SendServerCommand( cl, "print \"%s\"", message );
                    serverMainSystem->SendServerCommand(cl, "%s", cmd);

                    // ydnar: added this so map changes can use this functionality
                    if(disconnect) {
                        serverMainSystem->SendServerCommand(cl, "disconnect");
                    }
                }

                // force a snapshot to be sent
                cl->nextSnapshotTime = -1;
                serverSnapshotSystemLocal.SendClientSnapshot(cl);
            }
        }
    }
}

/*
================
idServerInitSystemLocal::Shutdown

Called when each game quits,
before idSystemLocal::Quit or idSystemLocal::Error
================
*/
void idServerInitSystemLocal::Shutdown(valueType *finalmsg) {
    if(!com_sv_running || !com_sv_running->integer) {
        return;
    }

    Com_Printf("----- Server Shutdown -----\n");

    networkSystem->LeaveMulticast6();

    if(svs.clients && !com_errorEntered) {
        FinalCommand(va("print \"%s\"", finalmsg), true);
    }

    serverMainSystem->MasterShutdown();
    serverGameSystem->ShutdownGameProgs();
    svs.gameStarted = false;

    // OACS: commit any remaining interframe
    idServerOACSSystemLocal::ExtendedRecordShutdown();

    // free current level
    ClearServer();
    collisionModelManager->ClearMap();

    // free server static data
    if(svs.clients) {
        sint index;

        for(index = 0; index < sv_maxclients->integer; index++) {
            serverClientSystem->FreeClient(&svs.clients[index]);
        }

        //Z_Free( svs.clients );
        free(svs.clients);   // RF, avoid trying to allocate large chunk on a fragmented zone
    }

    ::memset(&svs, 0, sizeof(svs));
    svs.serverLoad = -1;

    cvarSystem->Set("sv_running", "0");

    Com_Printf("---------------------------\n");

    // disconnect any local clients
#ifndef DEDICATED
    CL_Disconnect(false, "Server shutdown");
#endif
}
