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
#include <server/serverAutoPrecompiled.hpp>
#elif DEDICATED
#include <server/serverDedPrecompiled.hpp>
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

    if(dedicated->integer) {
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
    if(dedicated->integer) {
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

    // if not running a dedicated server idClientMainSystemLocal::MapLoading will connect the client to the server
    // also print some status stuff
#if !defined (DEDICATED)
    clientMainSystem->MapLoading();

    // make sure all the client stuff is unloaded
    clientMainSystem->ShutdownAll(false);
#endif

    // clear the whole hunk because we're (re)loading the server
    Hunk_Clear();

    // clear collision map data     // (SA) NOTE: TODO: used in missionpack
    collisionModelManager->ClearMap();

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
    svs.nextSnapshotEntities = 0;

    // allocate the snapshot entities
    svs.snapshotEntities = new entityState_s[svs.numSnapshotEntities];

    ::memset(svs.snapshotEntities, 0,
             sizeof(entityState_t) * svs.numSnapshotEntities);

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
    //if (developer->integer) {
    //Com_Printf("idServerInitSystemLocal::SpawnServer checksum feed: %p\n", sv.checksumFeed);
    //}

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

    idServerCommunityServer::SaveStatistics();
    idServerCommunityServer::InitStatistics();

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

    serverCcmdsLocal.AddOperatorCommands();

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
    if(!sv_running || !sv_running->integer) {
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
    clientConsoleCommandSystem->Disconnect(false, "Server shutdown");
#endif
}
