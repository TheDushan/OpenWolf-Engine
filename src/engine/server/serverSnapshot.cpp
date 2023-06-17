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
// File name:   serverSnapshot.cpp
// Created:
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

idServerSnapshotSystemLocal serverSnapshotSystemLocal;
idServerSnapshotSystem *serverSnapshotSystem = &serverSnapshotSystemLocal;

/*
===============
idServerSnapshotSystemLocal::idServerSnapshotSystemLocal
===============
*/
idServerSnapshotSystemLocal::idServerSnapshotSystemLocal(void) {
}

/*
===============
idServerWorldSystemLocal::~idServerWorldSystemLocal
===============
*/
idServerSnapshotSystemLocal::~idServerSnapshotSystemLocal(void) {
}

/*
=============================================================================
Delta encode a client frame onto the network channel

A normal server packet will look like:

4   sequence number (high bit set if an oversize fragment)
<optional reliable commands>
1   svc_snapshot
4   last client reliable command
4   serverTime
1   lastframe for delta compression
1   snapFlags
1   areaBytes
<areabytes>
<playerstate>
<packetentities>

=============================================================================
*/

/*
=============
idServerSnapshotSystemLocal::EmitPacketEntities

Writes a delta update of an entityState_t list to the message.
=============
*/
void idServerSnapshotSystemLocal::EmitPacketEntities(
    clientSnapshot_t *from, clientSnapshot_t *to, msg_t *msg) {
    sint oldindex, newindex, oldnum, newnum, from_num_entities;
    entityState_t *oldent, *newent;

    // generate the delta update
    if(!from) {
        from_num_entities = 0;
    } else {
        from_num_entities = from->num_entities;
    }

    newent = nullptr;
    oldent = nullptr;
    newindex = 0;
    oldindex = 0;

    while(newindex < to->num_entities || oldindex < from_num_entities) {
        if(newindex >= to->num_entities) {
            newnum = 9999;
        } else {
            newent = &svs.snapshotEntities[(to->first_entity + newindex) %
                                                                         svs.numSnapshotEntities];
            newnum = newent->number;
        }

        if(oldindex >= from_num_entities) {
            oldnum = 9999;
        } else {
            oldent = &svs.snapshotEntities[(from->first_entity + oldindex) %
                                                                           svs.numSnapshotEntities];
            oldnum = oldent->number;
        }

        if(newnum == oldnum) {
            // delta update from old position
            // because the force parm is false, this will not result
            // in any bytes being emited if the entity has not changed at all
            msgToFuncSystem->WriteDeltaEntity(msg, oldent, newent, false);
            oldindex++;
            newindex++;
            continue;
        }

        if(newnum < oldnum) {
            // this is a new entity, send it from the baseline
            msgToFuncSystem->WriteDeltaEntity(msg, &sv.svEntities[newnum].baseline,
                                              newent, true);
            newindex++;
            continue;
        }

        if(newnum > oldnum) {
            // the old entity isn't present in the new message
            msgToFuncSystem->WriteDeltaEntity(msg, oldent, nullptr, true);
            oldindex++;
            continue;
        }
    }

    msgToFuncSystem->WriteBits(msg, (MAX_GENTITIES - 1),
                               GENTITYNUM_BITS);     // end of packetentities
}

/*
==================
idServerSnapshotSystemLocal::WriteSnapshotToClient
==================
*/
void idServerSnapshotSystemLocal::WriteSnapshotToClient(client_t *client,
        msg_t *msg) {
    sint lastframe, i, snapFlags, deltaMessage;
    clientSnapshot_t *frame, *oldframe;

    // this is the snapshot we are creating
    frame = &client->frames[client->netchan.outgoingSequence & PACKET_MASK];

    // bots never acknowledge, but it doesn't matter since the only use case is for serverside demos
    // in which case we can delta against the very last message every time
    deltaMessage = client->deltaMessage;

    if(client->demo.isBot) {
        client->deltaMessage = client->netchan.outgoingSequence;
    }

    // try to use a previous frame as the source for delta compressing the snapshot
    if(client->deltaMessage <= 0 || client->state != CS_ACTIVE) {
        // client is asking for a retransmit
        oldframe = nullptr;
        lastframe = 0;
    } else if(client->netchan.outgoingSequence - client->deltaMessage >=
              (PACKET_BACKUP - 3)) {
        // client hasn't gotten a good message through in a long time
        if(developer->integer) {
            common->Printf("%s: Delta request from out of date packet.\n",
                           client->name);
        }

        oldframe = nullptr;
        lastframe = 0;
    } else if(client->demo.demorecording && client->demo.demowaiting) {
        // demo is waiting for a non-delta-compressed frame for this client, so don't delta compress
        oldframe = nullptr;
        lastframe = 0;
    } else if(client->demo.minDeltaFrame > deltaMessage) {
        // we saved a non-delta frame to the demo and sent it to the client, but the client didn't ack it
        // we can't delta against an old frame that's not in the demo without breaking the demo.  so send
        // non-delta frames until the client acks.
        oldframe = nullptr;
        lastframe = 0;
    } else {
        // we have a valid snapshot to delta from
        oldframe = &client->frames[client->deltaMessage & PACKET_MASK];
        lastframe = client->netchan.outgoingSequence - client->deltaMessage;

        // the snapshot's entities may still have rolled off the buffer, though
        if(oldframe->first_entity <= svs.nextSnapshotEntities -
                svs.numSnapshotEntities) {
            if(developer->integer) {
                common->Printf("%s: Delta request from out of date entities.\n",
                               client->name);
            }

            oldframe = nullptr;
            lastframe = 0;
        }
    }

    if(oldframe == nullptr) {
        if(client->demo.demowaiting) {
            // this is a non-delta frame, so we can delta against it in the demo
            client->demo.minDeltaFrame = client->netchan.outgoingSequence;
        }

        client->demo.demowaiting = false;
    }

    msgToFuncSystem->WriteByte(msg, svc_snapshot);

    // NOTE, MRE: now sent at the start of every message from server to client
    // let the client know which reliable clientCommands we have received
    //msgToFuncSystem->WriteLong( msg, client->lastClientCommand );

    // send over the current server time so the client can drift
    // its view of time to try to match
    if(client->oldServerTime) {
        // The server has not yet got an acknowledgement of the
        // new gamestate from this client, so continue to send it
        // a time as if the server has not restarted. Note from
        // the client's perspective this time is strictly speaking
        // incorrect, but since it'll be busy loading a map at
        // the time it doesn't really matter.
        msgToFuncSystem->WriteLong(msg, sv.time + client->oldServerTime);
    } else {
        msgToFuncSystem->WriteLong(msg, sv.time);
    }

    // what we are delta'ing from
    msgToFuncSystem->WriteByte(msg, lastframe);

    snapFlags = svs.snapFlagServerBit;

    if(client->rateDelayed) {
        snapFlags |= SNAPFLAG_RATE_DELAYED;
    }

    if(client->state != CS_ACTIVE) {
        snapFlags |= SNAPFLAG_NOT_ACTIVE;
    }

    msgToFuncSystem->WriteByte(msg, snapFlags);

    // send over the areabits
    msgToFuncSystem->WriteByte(msg, frame->areabytes);
    msgToFuncSystem->WriteData(msg, frame->areabits, frame->areabytes);

    {
        //sint sz = msg->cursize;
        //sint usz = msg->uncompsize;

        // delta encode the playerstate
        if(oldframe) {
            msgToFuncSystem->WriteDeltaPlayerstate(msg, &oldframe->ps, &frame->ps);
        } else {
            msgToFuncSystem->WriteDeltaPlayerstate(msg, nullptr, &frame->ps);
        }

        //      common->Printf( "Playerstate delta size: %f\n", ((msg->cursize - sz) * sv_fps->integer) / 8.f );
    }

    // delta encode the entities
    EmitPacketEntities(oldframe, frame, msg);

    // padding for rate debugging
    if(sv_padPackets->integer) {
        for(i = 0; i < sv_padPackets->integer; i++) {
            msgToFuncSystem->WriteByte(msg, svc_nop);
        }
    }
}

/*
==================
idServerSnapshotSystemLocal::UpdateServerCommandsToClient

(re)send all server commands the client hasn't acknowledged yet
==================
*/
void idServerSnapshotSystemLocal::UpdateServerCommandsToClient(
    client_t *client, msg_t *msg) {

    UpdateServerCommandsToClients(client, msg, false);
}

/*
==================
idServerSnapshotSystemLocal::UpdateServerCommandsToClients
==================
*/
bool idServerSnapshotSystemLocal::UpdateServerCommandsToClients(
    client_t *client, msg_t *msg, bool allowPartial) {

    sint i, reliableAcknowledge;


    // write any unacknowledged serverCommands
    for(i = client->reliableAcknowledge + 1; i <= client->reliableSequence;
            i++) {
        // msg overflow checks for 4 byte internally; we want to write svc_servercommand (1 byte), the index (4 byte) and the string
        if(allowPartial &&
                msg->maxsize - msg->cursize - 4 < 1 + 4 + (sint)::strlen(
                    client->reliableCommands[i & (MAX_RELIABLE_COMMANDS - 1)])) {
            client->reliableSent = i - 1;
            return false;
        }


        msgToFuncSystem->WriteByte(msg, svc_serverCommand);
        msgToFuncSystem->WriteLong(msg, i);
        msgToFuncSystem->WriteString(msg,
                                     client->reliableCommands[i & (MAX_RELIABLE_COMMANDS -
                                               1)]);
    }

    client->reliableSent = client->reliableSequence;

    return true;
}

/*
=============================================================================
Build a client snapshot structure
=============================================================================
*/

/*
=======================
idServerSnapshotSystemLocal::QsortEntityNumbers
=======================
*/
sint idServerSnapshotSystemLocal::QsortEntityNumbers(const void *a,
        const void *b) {
    sint *ea, *eb;

    ea = const_cast<sint *>(reinterpret_cast<const sint *>(a));
    eb = const_cast<sint *>(reinterpret_cast<const sint *>(b));

    if(*ea == *eb) {
        common->Error(ERR_DROP,
                      "idServerSnapshotSystemLocal::QsortEntityStates: duplicated entity");
    }

    if(*ea < *eb) {
        return -1;
    }

    return 1;
}

/*
===============
idServerSnapshotSystemLocal::AddEntToSnapshot
===============
*/
void idServerSnapshotSystemLocal::AddEntToSnapshot(sharedEntity_t
        *clientEnt, svEntity_t *svEnt, sharedEntity_t *gEnt,
        snapshotEntityNumbers_t *eNums) {
    // if we have already added this entity to this snapshot, don't add again
    if(svEnt->snapshotCounter == sv.snapshotCounter) {
        return;
    }

    svEnt->snapshotCounter = sv.snapshotCounter;

    // if we are full, silently discard entities
    if(eNums->numSnapshotEntities == MAX_SNAPSHOT_ENTITIES) {
        return;
    }

    if(gEnt->r.snapshotCallback) {
        if(!sgame->SnapshotCallback(gEnt->s.number, clientEnt->s.number)) {
            return;
        }
    }

    eNums->snapshotEntities[eNums->numSnapshotEntities] = gEnt->s.number;
    eNums->numSnapshotEntities++;
}

/*
===============
idServerSnapshotSystemLocal::AddEntitiesVisibleFromPoint
===============
*/
void idServerSnapshotSystemLocal::AddEntitiesVisibleFromPoint(
    vec3_t origin, clientSnapshot_t *frame, snapshotEntityNumbers_t *eNums,
    bool portal) {
    uchar8 *clientpvs, *bitvector;
    sint e, i, l, clientarea, clientcluster, leafnum, c_fullsend;
    sharedEntity_t *ent, *playerEnt;
    svEntity_t *svEnt;

    // during an error shutdown message we may need to transmit
    // the shutdown message after the server has shutdown, so
    // specfically check for it
    if(!sv.state) {
        return;
    }

    leafnum = collisionModelManager->PointLeafnum(origin);
    clientarea = collisionModelManager->LeafArea(leafnum);
    clientcluster = collisionModelManager->LeafCluster(leafnum);

    // calculate the visible areas
    frame->areabytes = collisionModelManager->WriteAreaBits(frame->areabits,
                       clientarea);

    clientpvs = collisionModelManager->ClusterPVS(clientcluster);

    c_fullsend = 0;

    playerEnt = serverGameSystem->GentityNum(frame->ps.clientNum);

    if(playerEnt->r.svFlags & SVF_SELF_PORTAL) {
        AddEntitiesVisibleFromPoint(playerEnt->s.origin2, frame, eNums, true);
    }

    for(e = 0; e < sv.num_entities; e++) {
        ent = serverGameSystem->GentityNum(e);

        // never send entities that aren't linked in
        if(!ent->r.linked) {
            continue;
        }

        if(ent->s.number != e) {
            if(developer->integer) {
                common->Printf("FIXING ENT->S.NUMBER!!!\n");
            }

            ent->s.number = e;
        }

        // entities can be flagged to explicitly not be sent to the client
        if(ent->r.svFlags & SVF_NOCLIENT) {
            continue;
        }

        // entities can be flagged to be sent to only one client
        if(ent->r.svFlags & SVF_SINGLECLIENT) {
            if(ent->r.singleClient != frame->ps.clientNum) {
                continue;
            }
        }

        // entities can be flagged to be sent to everyone but one client
        if(ent->r.svFlags & SVF_NOTSINGLECLIENT) {
            if(ent->r.singleClient == frame->ps.clientNum) {
                continue;
            }
        }

        // entities can be flagged to be sent to only a given mask of clients
        if(ent->r.svFlags & SVF_CLIENTMASK) {
            if(frame->ps.clientNum >= MAX_CLIENTS) {
                if(~ent->r.hiMask & (1 << (frame->ps.clientNum - MAX_CLIENTS))) {
                    continue;
                }
            } else {
                if(~ent->r.loMask & (1 << frame->ps.clientNum)) {
                    continue;
                }
            }
        }

#if defined (DEDICATED)

        if(ent->s.eType >= ET_EVENTS) {
            sint eventNumber = 0;
            eventNumber = (ent->s.eType - ET_EVENTS) & ~EV_EVENT_BITS;

            if(eventNumber == EV_JUMP || eventNumber == EV_SWIM ||
                    eventNumber == EV_FOOTSTEP) {
                continue;
            }
        }

#endif

        svEnt = serverGameSystem->SvEntityForGentity(ent);

        // don't double add an entity through portals
        if(svEnt->snapshotCounter == sv.snapshotCounter) {
            continue;
        }

        // broadcast entities are always sent
        if(ent->r.svFlags & SVF_BROADCAST || (e == frame->ps.clientNum)) {
            AddEntToSnapshot(playerEnt, svEnt, ent, eNums);
            continue;
        }

        bitvector = clientpvs;

        // Gordon: just check origin for being in pvs, ignore bmodel extents
        if(ent->r.svFlags & SVF_IGNOREBMODELEXTENTS) {
            if(bitvector[svEnt->originCluster >> 3] & (1 << (svEnt->originCluster &
                    7))) {
                AddEntToSnapshot(playerEnt, svEnt, ent, eNums);
            }

            continue;
        }

        // ignore if not touching a PV leaf
        // check area
        if(!collisionModelManager->AreasConnected(clientarea, svEnt->areanum)) {
            // doors can legally straddle two areas, so
            // we may need to check another one
            if(!collisionModelManager->AreasConnected(clientarea, svEnt->areanum2)) {
                continue;
            }
        }

        // check individual leafs
        if(!svEnt->numClusters) {
            continue;
        }

        l = 0;

        for(i = 0; i < svEnt->numClusters; i++) {
            l = svEnt->clusternums[i];

            if(bitvector[l >> 3] & (1 << (l & 7))) {
                break;
            }
        }

        // if we haven't found it to be visible,
        // check overflow clusters that coudln't be stored
        if(i == svEnt->numClusters) {
            if(svEnt->lastCluster) {
                for(; l <= svEnt->lastCluster; l++) {
                    if(bitvector[l >> 3] & (1 << (l & 7))) {
                        break;
                    }
                }

                if(l == svEnt->lastCluster) {
                    continue;
                }
            } else {
                continue;
            }
        }

        //----(SA) added "visibility dummies"
        if(ent->r.svFlags & SVF_VISDUMMY) {
            sharedEntity_t *ment = 0;

            //find master;
            ment = serverGameSystem->GentityNum(ent->s.otherEntityNum);

            if(ment) {
                svEntity_t *master = 0;

                master = serverGameSystem->SvEntityForGentity(ment);

                if(master->snapshotCounter == sv.snapshotCounter || !ment->r.linked) {
                    continue;
                }

                AddEntToSnapshot(playerEnt, master, ment, eNums);
            }

            // master needs to be added, but not this dummy ent
            continue;
        } else if(ent->r.svFlags & SVF_VISDUMMY_MULTIPLE) {
            sint h;
            sharedEntity_t *ment = 0;
            svEntity_t *master = 0;

            for(h = 0; h < sv.num_entities; h++) {
                ment = serverGameSystem->GentityNum(h);

                if(ment == ent) {
                    continue;
                }

                if(ment) {
                    master = serverGameSystem->SvEntityForGentity(ment);
                } else {
                    continue;
                }

                if(!(ment->r.linked)) {
                    continue;
                }

                if(ment->s.number != h) {
                    if(developer->integer) {
                        common->Printf("FIXING vis dummy multiple ment->S.NUMBER!!!\n");
                    }

                    ment->s.number = h;
                }

                if(ment->r.svFlags & SVF_NOCLIENT) {
                    continue;
                }

                if(master->snapshotCounter == sv.snapshotCounter) {
                    continue;
                }

                if(ment->s.otherEntityNum == ent->s.number) {
                    AddEntToSnapshot(playerEnt, master, ment, eNums);
                }
            }

            continue;
        }

        sharedEntity_t *client;

        if(e < sv_maxclients->integer) {    // client
            if(e == frame->ps.clientNum) {
                continue;
            }

            client = serverGameSystem->GentityNum(frame->ps.clientNum);

#if !defined (UPDATE_SERVER)

            if(sv_wh_active->integer && !portal &&
                    !(client->r.svFlags &
                      SVF_BOT)) {   // playerEnt->r.svFlags & SVF_SELF_PORTAL
                if(!idServerWallhackSystemLocal::CanSee(frame->ps.clientNum, e)) {
                    idServerWallhackSystemLocal::RandomizePos(frame->ps.clientNum, e);
                    AddEntToSnapshot(client, svEnt, ent, eNums);
                    continue;
                }
            }

#endif
        }

        // add it
        AddEntToSnapshot(playerEnt, svEnt, ent, eNums);

        // if its a portal entity, add everything visible from its camera position
        if(ent->r.svFlags & SVF_PORTAL) {
            if(ent->s.generic1) {
                vec3_t dir;

                VectorSubtract(ent->s.origin, origin, dir);

                if(VectorLengthSquared(dir) > static_cast<float32>(ent->s.generic1) *
                        ent->s.generic1) {
                    continue;
                }
            }

            AddEntitiesVisibleFromPoint(ent->s.origin2, frame, eNums, true);
        }

        continue;
    }
}

/*
=============
idServerSnapshotSystemLocalBuildClientSnapshot

Decides which entities are going to be visible to the client, and
copies off the playerstate and areabits.

This properly handles multiple recursive portals, but the render
currently doesn't.

For viewing through other player's eyes, clent can be something other than client->gentity
=============
*/
void idServerSnapshotSystemLocal::BuildClientSnapshot(client_t *client) {
    sint i, clientNum;
    vec3_t org;
    clientSnapshot_t *frame;
    snapshotEntityNumbers_t entityNumbers;
    sharedEntity_t *ent, *clent;
    entityState_t *state;
    svEntity_t *svEnt;
    playerState_t *ps;

    // bump the counter used to prevent double adding
    sv.snapshotCounter++;

    // this is the frame we are creating
    frame = &client->frames[client->netchan.outgoingSequence & PACKET_MASK];

    // clear everything in this snapshot
    entityNumbers.numSnapshotEntities = 0;
    ::memset(frame->areabits, 0, sizeof(frame->areabits));

    // show_bug.cgi?id=62
    frame->num_entities = 0;

    clent = client->gentity;

    if(!clent || client->state == CS_ZOMBIE) {
        return;
    }

    // grab the current playerState_t
    ps = serverGameSystem->GameClientNum(ARRAY_INDEX(svs.clients, client));
    frame->ps = *ps;

    // never send client's own entity, because it can
    // be regenerated from the playerstate
    clientNum = frame->ps.clientNum;

    if(clientNum < 0 || clientNum >= MAX_GENTITIES) {
        common->Error(ERR_DROP, "SV_SvEntityForGentity: bad gEnt");
    }

    svEnt = &sv.svEntities[clientNum];

    svEnt->snapshotCounter = sv.snapshotCounter;

    if(clent->r.svFlags & SVF_SELF_PORTAL_EXCLUSIVE) {
        // find the client's viewpoint
        VectorCopy(clent->s.origin2, org);
    } else {
        VectorCopy(ps->origin, org);
    }

    org[2] += ps->viewheight;

    //----(SA)  added for 'lean'
    // need to account for lean, so areaportal doors draw properly
#if 0

    if(frame->ps.leanf != 0) {
        vec3_t right, v3ViewAngles;

        VectorCopy(ps->viewangles, v3ViewAngles);
        v3ViewAngles[2] += frame->ps.leanf / 2.0f;
        AngleVectors(v3ViewAngles, nullptr, right, nullptr);
        VectorMA(org, frame->ps.leanf, right, org);
    }

#endif
    //----(SA)  end

    // add all the entities directly visible to the eye, which
    // may include portal entities that merge other viewpoints
    AddEntitiesVisibleFromPoint(org, frame,
                                &entityNumbers /*, false, client->netchan.remoteAddress.type == NA_LOOPBACK */,
                                false);

    // if there were portals visible, there may be out of order entities
    // in the list which will need to be resorted for the delta compression
    // to work correctly.  This also catches the error condition
    // of an entity being included twice.
    qsort(entityNumbers.snapshotEntities, entityNumbers.numSnapshotEntities,
          sizeof(entityNumbers.snapshotEntities[0]), QsortEntityNumbers);

    // now that all viewpoint's areabits have been OR'd together, invert
    // all of them to make it a mask vector, which is what the renderer wants
    for(i = 0; i < MAX_MAP_AREA_BYTES / 4; i++) {
        frame->areabits[i] = frame->areabits[i] ^ -1;
    }

    // copy the entity states out
    frame->num_entities = 0;
    frame->first_entity = svs.nextSnapshotEntities;

    for(i = 0; i < entityNumbers.numSnapshotEntities; i++) {
        ent = serverGameSystem->GentityNum(entityNumbers.snapshotEntities[i]);
        state = &svs.snapshotEntities[svs.nextSnapshotEntities %
                                                               svs.numSnapshotEntities];
        *state = ent->s;

#if !defined (UPDATE_SERVER)

        if(sv_wh_active->integer &&
                entityNumbers.snapshotEntities[i] < sv_maxclients->integer) {
            if(idServerWallhackSystemLocal::PositionChanged(
                        entityNumbers.snapshotEntities[i])) {
                idServerWallhackSystemLocal::RestorePos(entityNumbers.snapshotEntities[i]);
            }
        }

#endif

        svs.nextSnapshotEntities++;

        // this should never hit, map should always be restarted first in idServerMainSystemLocal::Frame
        if(svs.nextSnapshotEntities >= 0x7FFFFFFE) {
            common->Error(ERR_FATAL,
                          "idServerSnapshotSystemLocal::BuildClientSnapshot: svs.nextSnapshotEntities wrapped");
        }

        frame->num_entities++;
    }
}

/*
====================
idServerSnapshotSystemLocal::RateMsec

Return the number of msec a given size message is supposed
to take to clear, based on the current rate
TTimo - use sv_maxRate or sv_dl_maxRate depending on regular or downloading client
====================
*/
sint idServerSnapshotSystemLocal::RateMsec(client_t *client,
        sint messageSize) {
    sint rate, rateMsec, maxRate;

    // individual messages will never be larger than fragment size
    if(messageSize > 1500) {
        messageSize = 1500;
    }

    // low watermark for sv_maxRate, never 0 < sv_maxRate < 1000 (0 is no limitation)
    if(sv_maxRate->integer && sv_maxRate->integer < 1000) {
        cvarSystem->Set("sv_MaxRate", "1000");
    }

    rate = client->rate;

    // work on the appropriate max rate (client or download)
    if(!*client->downloadName) {
        maxRate = sv_maxRate->integer;
    } else {
        maxRate = sv_dl_maxRate->integer;
    }

    if(maxRate) {
        if(maxRate < rate) {
            rate = maxRate;
        }
    }

    rateMsec = (messageSize + HEADER_RATE_BYTES) * 1000 / (static_cast<sint>
               (rate * timescale->value));

    return rateMsec;
}

/*
=======================
idServerSnapshotSystemLocal::SendMessageToClient

Called by idServerSnapshotSystemLocal::SendClientSnapshot and idServerSnapshotSystemLocal::SendClientGameState
=======================
*/
void idServerSnapshotSystemLocal::SendMessageToClient(msg_t *msg,
        client_t *client) {
    sint rateMsec;

    while(client->state && client->netchan.unsentFragments) {
        common->Printf("idServerSnapshotSystemLocal::SendMessageToClient [1] for %s, writing out old fragments\n",
                       client->name);
        networkChainSystem->TransmitNextFragment(&client->netchan);
    }

    // record information about the message
    client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSize
        = msg->cursize;
    client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSent
        = svs.time;
    client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageAcked
        = -1;

    // save the message to demo.  this must happen before sending over network as that encodes the backing databuf
    if(client->demo.demorecording && !client->demo.demowaiting) {
        msg_t msgcopy = *msg;
        msgToFuncSystem->WriteByte(&msgcopy, svc_EOF);
        idServerCcmdsSystemLocal::WriteDemoMessage(client, &msgcopy, 0);
    }

    // bots need to have their snapshots built, but
    // they query them directly without needing to be sent
    if(client->demo.isBot) {
        client->netchan.outgoingSequence++;
        client->demo.botReliableAcknowledge = client->reliableSent;
        return;
    }

    // send the datagram
    serverNetChanSystem->NetchanTransmit(client, msg);

    // set nextSnapshotTime based on rate and requested number of updates

    // local clients get snapshots every server frame
    // TTimo - show_bug.cgi?id=491
    // added sv_lanForceRate check
    if(client->netchan.remoteAddress.type == NA_LOOPBACK ||
            (sv_lanForceRate->integer &&
             networkSystem->IsLANAddress(client->netchan.remoteAddress))) {
        client->nextSnapshotTime = svs.time + (static_cast<sint>
                                               (1000.0 / sv_fps->integer * timescale->value));
        return;
    }

    // normal rate / snapshotMsec calculation
    rateMsec = RateMsec(client, msg->cursize);

    // TTimo - during a download, ignore the snapshotMsec
    // the update server on steroids, with this disabled and sv_fps 60, the download can reach 30 kb/s
    // on a regular server, we will still top at 20 kb/s because of sv_fps 20
    if(!*client->downloadName && rateMsec < client->snapshotMsec) {
        // never send more packets than this, no matter what the rate is at
        //rateMsec = client->snapshotMsec;
        rateMsec = client->snapshotMsec * timescale->value;
        client->rateDelayed = false;
    } else {
        client->rateDelayed = true;
    }

    client->nextSnapshotTime = svs.time + (static_cast<sint>
                                           (rateMsec * timescale->value));

    // don't pile up empty snapshots while connecting
    if(client->state != CS_ACTIVE) {
        // a gigantic connection message may have already put the nextSnapshotTime
        // more than a second away, so don't shorten it
        // do shorten if client is downloading
        if(!*client->downloadName &&
                client->nextSnapshotTime < svs.time + (static_cast<sint>
                        (1000.0 * timescale->value))) {
            client->nextSnapshotTime = svs.time + (static_cast<sint>
                                                   (1000 * timescale->value));
        }
    }
}

//bani
/*
=======================
idServerSnapshotSystemLocal::SendClientIdle

There is no need to send full snapshots to clients who are loading a map.
So we send them "idle" packets with the bare minimum required to keep them on the server.
=======================
*/
void idServerSnapshotSystemLocal::SendClientIdle(client_t *client) {
    uchar8 msg_buf[MAX_MSGLEN];
    msg_t msg;

    msgToFuncSystem->Init(&msg, msg_buf, sizeof(msg_buf));
    msg.allowoverflow = true;

    // NOTE, MRE: all server->client messages now acknowledge
    // let the client know which reliable clientCommands we have received
    msgToFuncSystem->WriteLong(&msg, client->lastClientCommand);

    // (re)send any reliable server commands
    UpdateServerCommandsToClient(client, &msg);

    // send over all the relevant entityState_t
    // and the playerState_t
    //WriteSnapshotToClient( client, &msg );

    // Add any download data if the client is downloading
    serverClientSystem->WriteDownloadToClient(client, &msg);

    // check for overflow
    if(msg.overflowed) {
        common->Printf("idServerSnapshotSystemLocal::SendClientIdle - WARNING: msg overflowed for %s\n",
                       client->name);
        msgToFuncSystem->Clear(&msg);

        serverClientSystem->DropClient(client,
                                       "idServerSnapshotSystemLocal::SendClientIdle - Msg overflowed");
        return;
    }

    SendMessageToClient(&msg, client);

    while(client->netchan.unsentFragments) {
        networkChainSystem->TransmitNextFragment(&client->netchan);
    }

    sv.bpsTotalBytes += msg.cursize;            // NERVE - SMF - net debugging
    sv.ubpsTotalBytes += msg.uncompsize / 8;    // NERVE - SMF - net debugging
}

/*
=======================
idServerSnapshotSystemLocal::SendClientSnapshot

Also called by idServerInitSystemLocal::FinalCommand
=======================
*/
void idServerSnapshotSystemLocal::SendClientSnapshot(client_t *client) {
    uchar8 msg_buf[MAX_MSGLEN];
    msg_t msg, msgBackup;

    //bots dont need snapshots
    if(client->gentity && client->gentity->r.svFlags & SVF_BOT) {
        return;
    }

    //bani
    if(client->state < CS_ACTIVE) {
        // bani - #760 - zombie clients need full snaps so they can still process reliable commands
        // (eg so they can pick up the disconnect reason)
        if(client->state != CS_ZOMBIE) {
            SendClientIdle(client);
            return;
        }
    }

    // build the snapshot
    BuildClientSnapshot(client);

    if(sv_autoRecDemo->integer && !client->demo.demorecording) {
        if(client->netchan.remoteAddress.type != NA_BOT ||
                sv_autoRecDemoBots->integer) {
            idServerCcmdsSystemLocal::BeginAutoRecordDemos();
        }
    }

    // bots need to have their snapshots built, but
    // they query them directly without needing to be sent
    if(client->gentity && client->gentity->r.svFlags & SVF_BOT)
        //if( client->netchan.remoteAddress.type == NA_BOT && !client->demo.demorecording )
    {
        return;
    }

    msgToFuncSystem->Init(&msg, msg_buf, sizeof(msg_buf));
    msg.allowoverflow = true;

    // NOTE, MRE: all server->client messages now acknowledge
    // let the client know which reliable clientCommands we have received
    msgToFuncSystem->WriteLong(&msg, client->lastClientCommand);

    // (re)send any reliable server commands
    if(!UpdateServerCommandsToClients(client, &msg, true)) {
        // If we can't fit all commands in a single message send what we got and
        // don't even try to send entities
        SendMessageToClient(&msg, client);
        return;
    }

    // Backup the msg state in case the snapshot would overflow it
    ::memcpy(&msgBackup, &msg, sizeof(msgBackup));

    // send over all the relevant entityState_t
    // and the playerState_t
    WriteSnapshotToClient(client, &msg);

    if(msg.overflowed && !msgBackup.overflowed) {
        // The entity states were too much and the message overflowed. So send
        // the old state of the message from before we tried to append the
        // entity states. As the net code doesn't send the msg_buf content after
        // the current size of the message we don't have to clear anything and
        // we can just use the old msg values (which point to the updated buffer).
        SendMessageToClient(&msgBackup, client);
        return;
    }

    // Backup the msg state in case the download would overflow it
    ::memcpy(&msgBackup, &msg, sizeof(msgBackup));

    // Add any download data if the client is downloading
    serverClientSystem->WriteDownloadToClient(client, &msg);

    if(msg.overflowed && !msgBackup.overflowed) {
        // Downloads usually don't happen in situations that are likely to have
        // message overflows, but let's make sure and apply the same logic we
        // used for the entity states.
        SendMessageToClient(&msgBackup, client);
        return;
    }

    // check for overflow
    if(msg.overflowed) {
        common->Printf("idServerSnapshotSystemLocal::SendClientSnapshot : WARNING: msg overflowed for %s\n",
                       client->name);
        msgToFuncSystem->Clear(&msg);

        serverClientSystem->DropClient(client,
                                       "idServerSnapshotSystemLocal::SendClientSnapshot : Msg overflowed");
        return;
    }

    SendMessageToClient(&msg, client);

    sv.bpsTotalBytes += msg.cursize;            // NERVE - SMF - net debugging
    sv.ubpsTotalBytes += msg.uncompsize / 8;    // NERVE - SMF - net debugging
}

/*
=======================
idServerSnapshotSystemLocal::SendClientMessages
=======================
*/
void idServerSnapshotSystemLocal::SendClientMessages(void) {
    sint i, numclients = 0; // NERVE - SMF - net debugging
    client_t *c;

    sv.bpsTotalBytes = 0; // NERVE - SMF - net debugging
    sv.ubpsTotalBytes = 0; // NERVE - SMF - net debugging

    // Gordon: update any changed configstrings from this frame
    serverInitSystem->UpdateConfigStrings();

    // send a message to each connected client
    for(i = 0; i < sv_maxclients->integer; i++) {
        c = &svs.clients[i];

        // do not send a packet to a democlient, this will cause the engine to crash
        if(!c->state) {
            // not connected
            continue;
        }

        if(svs.time - c->nextSnapshotTime < c->snapshotMsec *
                timescale->value) {
            // It's not time yet
            continue;
        }

        // rain - changed <= CS_ZOMBIE to < CS_ZOMBIE so that the
        // disconnect reason is properly sent in the network stream
        if(c->state < CS_ZOMBIE) {
            // not connected
            continue;
        }

        // RF, needed to insert this otherwise bots would cause error drops in sv_net_chan.c:
        // --> "netchan queue is not properly initialized in SV_Netchan_TransmitNextFragment\n"
        if(c->gentity && c->gentity->r.svFlags & SVF_BOT) {
            continue;
        }

        // NERVE - SMF - net debugging
        numclients++;

        // send additional message fragments if the last message
        // was too large to send at once
        if(c->netchan.unsentFragments) {
            c->nextSnapshotTime = svs.time + RateMsec(c,
                                  c->netchan.unsentLength - c->netchan.unsentFragmentStart);
            serverNetChanSystem->NetchanTransmitNextFragment(c);
            continue;
        }

        // generate and send a new message
        SendClientSnapshot(c);
    }

    // NERVE - SMF - net debugging
    if(sv_showAverageBPS->integer && numclients > 0) {
        float32 ave = 0, uave = 0;

        for(i = 0; i < MAX_BPS_WINDOW - 1; i++) {
            sv.bpsWindow[i] = sv.bpsWindow[i + 1];
            ave += sv.bpsWindow[i];

            sv.ubpsWindow[i] = sv.ubpsWindow[i + 1];
            uave += sv.ubpsWindow[i];
        }

        sv.bpsWindow[MAX_BPS_WINDOW - 1] = sv.bpsTotalBytes;
        ave += sv.bpsTotalBytes;

        sv.ubpsWindow[MAX_BPS_WINDOW - 1] = sv.ubpsTotalBytes;
        uave += sv.ubpsTotalBytes;

        if(sv.bpsTotalBytes >= sv.bpsMaxBytes) {
            sv.bpsMaxBytes = sv.bpsTotalBytes;
        }

        if(sv.ubpsTotalBytes >= sv.ubpsMaxBytes) {
            sv.ubpsMaxBytes = sv.ubpsTotalBytes;
        }

        sv.bpsWindowSteps++;

        if(sv.bpsWindowSteps >= MAX_BPS_WINDOW) {
            float32 comp_ratio;

            sv.bpsWindowSteps = 0;

            ave = (ave / static_cast<float32>(MAX_BPS_WINDOW));
            uave = (uave / static_cast<float32>(MAX_BPS_WINDOW));

            comp_ratio = (1 - ave / uave) * 100.f;
            sv.ucompAve += comp_ratio;
            sv.ucompNum++;

            if(developer->integer) {
                common->Printf("bpspc(%2.0f) bps(%2.0f) pk(%i) ubps(%2.0f) upk(%i) cr(%2.2f) acr(%2.2f)\n",
                               ave / static_cast<float32>(numclients), ave, sv.bpsMaxBytes, uave,
                               sv.ubpsMaxBytes, comp_ratio,
                               sv.ucompAve / sv.ucompNum);
            }
        }
    }

    // -NERVE - SMF
}

/*
=======================
idServerSnapshotSystemLocal::CheckClientUserinfoTimer
=======================
*/
void idServerSnapshotSystemLocal::CheckClientUserinfoTimer(void) {
    sint i;
    valueType bigbuffer[ MAX_INFO_STRING * 2];
    client_t *cl;

    for(i = 0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++) {
        if(!cl->state) {
            // not connected
            continue;
        }

        if((sv_floodProtect->integer) && (svs.time >= cl->nextReliableUserTime) &&
                (cl->state >= CS_ACTIVE) && (cl->userinfobuffer[0] != 0)) {
            //We have something in the buffer
            //and its time to process it
            Q_vsprintf_s(bigbuffer, sizeof(bigbuffer), sizeof(bigbuffer),
                         "userinfo \"%s\"", cl->userinfobuffer);

            cmdSystem->TokenizeString(bigbuffer);
            serverClientLocal.UpdateUserinfo_f(cl);
        }
    }
}
