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
// File name:   serverWorld.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: world query functions
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <server/serverAutoPrecompiled.hpp>
#elif DEDICATED
#include <server/serverDedPrecompiled.hpp>
#else
#include <framework/precompiled.hpp>
#endif

idServerWorldSystemLocal serverWorldSystemLocal;
idServerWorldSystem *serverWorldSystem = &serverWorldSystemLocal;

/*
===============
idServerWorldSystemLocal::idServerWorldSystemLocal
===============
*/
idServerWorldSystemLocal::idServerWorldSystemLocal(void) {
}

/*
===============
idServerWorldSystemLocal::~idServerWorldSystemLocal
===============
*/
idServerWorldSystemLocal::~idServerWorldSystemLocal(void) {
}

/*
================
idServerWorldSystemLocal::ClipHandleForEntity

Returns a headnode that can be used for testing or clipping to a
given entity.  If the entity is a bsp model, the headnode will
be returned, otherwise a custom box tree will be constructed.
================
*/
clipHandle_t idServerWorldSystemLocal::ClipHandleForEntity(
    const sharedEntity_t *ent) {
    if(ent->r.bmodel) {
        // explicit hulls in the BSP model
        return collisionModelManager->InlineModel(ent->s.modelindex);
    }

    if(ent->r.svFlags & SVF_CAPSULE) {
        // create a temp capsule from bounding box sizes
        return collisionModelManager->TempBoxModel(ent->r.mins, ent->r.maxs, true);
    }

    // create a temp tree from bounding box sizes
    return collisionModelManager->TempBoxModel(ent->r.mins, ent->r.maxs,
            false);
}

/*
===============
idServerWorldSystemLocal::SectorList_f
===============
*/
void idServerWorldSystemLocal::SectorList_f(void) {
    sint i, c;
    worldSector_t *sec;
    svEntity_t *ent;

    for(i = 0; i < AREA_NODES; i++) {
        sec = &sv_worldSectors[i];

        c = 0;

        for(ent = sec->entities; ent; ent = ent->nextEntityInWorldSector) {
            c++;
        }

        Com_Printf("sector %i: %i entities\n", i, c);
    }
}

/*
===============
idServerWorldSystemLocal::CreateworldSector

Builds a uniformly subdivided tree for the given world size
===============
*/
worldSector_t *idServerWorldSystemLocal::CreateworldSector(sint depth,
        vec3_t mins, vec3_t maxs) {
    worldSector_t *anode = &sv_worldSectors[sv_numworldSectors];
    vec3_t size, mins1, maxs1, mins2, maxs2;

    sv_numworldSectors++;

    if(depth == AREA_DEPTH) {
        anode->axis = -1;
        anode->children[0] = anode->children[1] = nullptr;
        return anode;
    }

    VectorSubtract(maxs, mins, size);

    if(size[0] > size[1]) {
        anode->axis = 0;
    } else {
        anode->axis = 1;
    }

    anode->dist = 0.5 * (maxs[anode->axis] + mins[anode->axis]);
    VectorCopy(mins, mins1);
    VectorCopy(mins, mins2);
    VectorCopy(maxs, maxs1);
    VectorCopy(maxs, maxs2);

    maxs1[anode->axis] = mins2[anode->axis] = anode->dist;

    anode->children[0] = CreateworldSector(depth + 1, mins2, maxs2);
    anode->children[1] = CreateworldSector(depth + 1, mins1, maxs1);

    return anode;
}

/*
===============
idServerWorldSystemLocal::ClearWorld
===============
*/
void idServerWorldSystemLocal::ClearWorld(void) {
    vec3_t mins, maxs;
    clipHandle_t h;

    memset(sv_worldSectors, 0, sizeof(sv_worldSectors));
    sv_numworldSectors = 0;

    for(uint i = 0; i < ARRAY_LEN(sv.svEntities); i++) {
        sv.svEntities[i].worldSector = nullptr;
        sv.svEntities[i].nextEntityInWorldSector = nullptr;
    }

    // get world map bounds
    h = collisionModelManager->InlineModel(0);
    collisionModelManager->ModelBounds(h, mins, maxs);
    CreateworldSector(0, mins, maxs);
}


/*
===============
SV_UnlinkEntity
===============
*/
void idServerWorldSystemLocal::UnlinkEntity(sharedEntity_t *gEnt) {
    svEntity_t *ent, *scan;
    worldSector_t *ws;

    ent = serverGameSystem->SvEntityForGentity(gEnt);

    gEnt->r.linked = false;

    ws = ent->worldSector;

    if(!ws) {
        return; // not linked in anywhere
    }

    ent->worldSector = nullptr;

    if(ws->entities == ent) {
        ws->entities = ent->nextEntityInWorldSector;
        return;
    }

    for(scan = ws->entities; scan; scan = scan->nextEntityInWorldSector) {
        if(scan->nextEntityInWorldSector == ent) {
            scan->nextEntityInWorldSector = ent->nextEntityInWorldSector;
            return;
        }
    }

    Com_Printf("WARNING: idServerWorldSystemLocal::UnlinkEntity: not found in worldSector\n");
}

/*
===============
idServerWorldSystemLocal::LinkEntity
===============
*/
void idServerWorldSystemLocal::LinkEntity(sharedEntity_t *gEnt) {
    sint leafs[MAX_TOTAL_ENT_LEAFS], cluster, num_leafs, i, j, k, area,
         lastLeaf;
    float32 *origin, *angles;
    worldSector_t *node;
    svEntity_t *ent;

    ent = serverGameSystem->SvEntityForGentity(gEnt);

    // Ridah, sanity check for possible currentOrigin being reset bug
    if(!gEnt->r.bmodel && VectorCompare(gEnt->r.currentOrigin, vec3_origin)) {
        Com_DPrintf("WARNING: BBOX entity is being linked at world origin, this is probably a bug\n");
    }

    if(ent->worldSector) {
        // unlink from old position
        UnlinkEntity(gEnt);
    }

    // encode the size into the entityState_t for client prediction
    if(gEnt->r.bmodel) {
        // a solid_box will never create this value
        gEnt->s.solid = SOLID_BMODEL;

        // Gordon: for the origin only bmodel checks
        ent->originCluster = collisionModelManager->LeafCluster(
                                 collisionModelManager->PointLeafnum(gEnt->r.currentOrigin));
    } else if(gEnt->r.contents & (CONTENTS_SOLID | CONTENTS_BODY)) {
        // assume that x/y are equal and symetric
        i = gEnt->r.maxs[0];

        if(i < 1) {
            i = 1;
        }

        if(i > 255) {
            i = 255;
        }

        // z is not symetric
        j = (-gEnt->r.mins[2]);

        if(j < 1) {
            j = 1;
        }

        if(j > 255) {
            j = 255;
        }

        // and z maxs can be negative...
        k = (gEnt->r.maxs[2] + 32);

        if(k < 1) {
            k = 1;
        }

        if(k > 255) {
            k = 255;
        }

        gEnt->s.solid = (k << 16) | (j << 8) | i;

        if(gEnt->s.solid == SOLID_BMODEL) {
            gEnt->s.solid = (k << 16) | (j << 8) | i - 1;
        }
    } else {
        gEnt->s.solid = 0;
    }

    // get the position
    origin = gEnt->r.currentOrigin;
    angles = gEnt->r.currentAngles;

    // set the abs box
    if(gEnt->r.bmodel && (angles[0] || angles[1] || angles[2])) {
        // expand for rotation
        float32 max;
        sint i;

        max = RadiusFromBounds(gEnt->r.mins, gEnt->r.maxs);

        for(i = 0; i < 3; i++) {
            gEnt->r.absmin[i] = origin[i] - max;
            gEnt->r.absmax[i] = origin[i] + max;
        }
    } else {
        // normal
        VectorAdd(origin, gEnt->r.mins, gEnt->r.absmin);
        VectorAdd(origin, gEnt->r.maxs, gEnt->r.absmax);
    }

    // because movement is clipped an epsilon away from an actual edge,
    // we must fully check even when bounding boxes don't quite touch
    gEnt->r.absmin[0] -= 1;
    gEnt->r.absmin[1] -= 1;
    gEnt->r.absmin[2] -= 1;
    gEnt->r.absmax[0] += 1;
    gEnt->r.absmax[1] += 1;
    gEnt->r.absmax[2] += 1;

    // link to PVS leafs
    ent->numClusters = 0;
    ent->lastCluster = 0;
    ent->areanum = -1;
    ent->areanum2 = -1;

    //get all leafs, including solids
    num_leafs = collisionModelManager->BoxLeafnums(gEnt->r.absmin,
                gEnt->r.absmax, leafs, MAX_TOTAL_ENT_LEAFS, &lastLeaf);

    // if none of the leafs were inside the map, the
    // entity is outside the world and can be considered unlinked
    if(!num_leafs) {
        return;
    }

    // set areas, even from clusters that don't fit in the entity array
    for(i = 0; i < num_leafs; i++) {
        area = collisionModelManager->LeafArea(leafs[i]);

        if(area != -1) {
            // doors may legally straggle two areas,
            // but nothing should evern need more than that
            if(ent->areanum != -1 && ent->areanum != area) {
                if(ent->areanum2 != -1 && ent->areanum2 != area &&
                        sv.state == SS_LOADING) {
                    Com_DPrintf("Object %i touching 3 areas at %f %f %f\n", gEnt->s.number,
                                gEnt->r.absmin[0], gEnt->r.absmin[1], gEnt->r.absmin[2]);
                }

                ent->areanum2 = area;
            } else {
                ent->areanum = area;
            }
        }
    }

    // store as many explicit clusters as we can
    ent->numClusters = 0;

    for(i = 0; i < num_leafs; i++) {
        cluster = collisionModelManager->LeafCluster(leafs[i]);

        if(cluster != -1) {
            ent->clusternums[ent->numClusters++] = cluster;

            if(ent->numClusters == MAX_ENT_CLUSTERS) {
                break;
            }
        }
    }

    // store off a last cluster if we need to
    if(i != num_leafs) {
        ent->lastCluster = collisionModelManager->LeafCluster(lastLeaf);
    }

    gEnt->r.linkcount++;

    // find the first world sector node that the ent's box crosses
    node = sv_worldSectors;

    while(1) {
        if(node->axis == -1) {
            break;
        }

        if(gEnt->r.absmin[node->axis] > node->dist) {
            node = node->children[0];
        } else if(gEnt->r.absmax[node->axis] < node->dist) {
            node = node->children[1];
        } else {
            // crosses the node
            break;
        }
    }

    // link it in
    ent->worldSector = node;
    ent->nextEntityInWorldSector = node->entities;
    node->entities = ent;

    gEnt->r.linked = true;
}

/*
====================
idServerWorldSystemLocal::AreaEntities_r
====================
*/
void idServerWorldSystemLocal::AreaEntities_r(worldSector_t *node,
        areaParms_t *ap) {
    sint count;
    svEntity_t *check, *next;
    sharedEntity_t *gcheck;

    count = 0;

    for(check = node->entities; check; check = next) {
        next = check->nextEntityInWorldSector;

        gcheck = serverGameSystem->GEntityForSvEntity(check);

        if(!gcheck) {
            continue;
        }

        if(!gcheck->r.linked) {
            continue;
        }

        if(gcheck->r.absmin[0] > ap->maxs[0] ||
                gcheck->r.absmin[1] > ap->maxs[1] || gcheck->r.absmin[2] > ap->maxs[2] ||
                gcheck->r.absmax[0] < ap->mins[0] || gcheck->r.absmax[1] < ap->mins[1] ||
                gcheck->r.absmax[2] < ap->mins[2]) {
            continue;
        }

        if(ap->count >= ap->maxcount) {
            Com_DPrintf("idServerWorldSystemLocal::AreaEntities: MAXCOUNT\n");
            return;
        }

        ap->list[ap->count] = ARRAY_INDEX(sv.svEntities, check);
        ap->count++;
    }

    if(node->axis == -1) {
        return; // terminal node
    }

    // recurse down both sides
    if(ap->maxs[node->axis] > node->dist) {
        AreaEntities_r(node->children[0], ap);
    }

    if(ap->mins[node->axis] < node->dist) {
        AreaEntities_r(node->children[1], ap);
    }
}

/*
================
idServerWorldSystemLocal::AreaEntities
================
*/
sint idServerWorldSystemLocal::AreaEntities(const vec3_t mins,
        const vec3_t maxs, sint *entityList, sint maxcount) {
    areaParms_t ap;

    ap.mins = mins;
    ap.maxs = maxs;
    ap.list = entityList;
    ap.count = 0;
    ap.maxcount = maxcount;

    AreaEntities_r(sv_worldSectors, &ap);

    return ap.count;
}

/*
====================
idServerWorldSystemLocal::ClipToEntity
====================
*/
void idServerWorldSystemLocal::ClipToEntity(trace_t *trace,
        const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
        sint entityNum, sint contentmask, traceType_t type) {
    sharedEntity_t *touch;
    clipHandle_t clipHandle;
    float32 *origin, *angles;

    touch = serverGameSystem->GentityNum(entityNum);

    ::memset(trace, 0, sizeof(trace_t));

    // if it doesn't have any brushes of a type we
    // are looking for, ignore it
    if(!(contentmask & touch->r.contents)) {
        trace->fraction = 1.0;
        return;
    }

    // might intersect, so do an exact clip
    clipHandle = ClipHandleForEntity(touch);

    origin = touch->r.currentOrigin;
    angles = touch->r.currentAngles;

    if(!touch->r.bmodel) {
        angles = vec3_origin;   // boxes don't rotate
    }

    collisionModelManager->TransformedBoxTrace(trace,
            const_cast<float32 *>(start), const_cast<float32 *>(end),
            const_cast<float32 *>(mins),
            const_cast<float32 *>(maxs), clipHandle, contentmask, origin, angles,
            type);

    if(trace->fraction < 1) {
        trace->entityNum = touch->s.number;
    }
}

/*
====================
idServerWorldSystemLocal::ClipMoveToEntities
====================
*/
void idServerWorldSystemLocal::ClipMoveToEntities(moveclip_t *clip) {
    sint i, num, touchlist[MAX_GENTITIES], passOwnerNum;
    sharedEntity_t *touch;
    trace_t trace;
    clipHandle_t clipHandle;
    float32 *origin, *angles;

    num = serverWorldSystemLocal.AreaEntities(clip->boxmins, clip->boxmaxs,
            touchlist, MAX_GENTITIES);

    if(clip->passEntityNum != ENTITYNUM_NONE) {
        passOwnerNum = (serverGameSystem->GentityNum(
                            clip->passEntityNum))->r.ownerNum;

        if(passOwnerNum == ENTITYNUM_NONE) {
            passOwnerNum = -1;
        }
    } else {
        passOwnerNum = -1;
    }

    for(i = 0; i < num; i++) {
        if(clip->trace.allsolid) {
            return;
        }

        touch = serverGameSystem->GentityNum(touchlist[i]);

        // see if we should ignore this entity
        if(clip->passEntityNum != ENTITYNUM_NONE) {
            if(touchlist[i] == clip->passEntityNum) {
                // don't clip against the pass entity
                continue;
            }

            // don't clip against own missiles
            if(touch->r.ownerNum == clip->passEntityNum
                    // don't clip against other missiles from our owner
                    || touch->r.ownerNum == passOwnerNum) {
                continue;
            }
        }

        // if it doesn't have any brushes of a type we
        // are looking for, ignore it
        if(!(clip->contentmask & touch->r.contents)) {
            continue;
        }

        // might intersect, so do an exact clip
        clipHandle = ClipHandleForEntity(touch);

        if(clipHandle == 0) {
            continue;
        }

        // If clipping against BBOX, set to correct contents
        if(clipHandle == BOX_MODEL_HANDLE) {
            collisionModelManager->SetTempBoxModelContents(touch->r.contents);
        }

        origin = touch->r.currentOrigin;
        angles = touch->r.currentAngles;

        if(!touch->r.bmodel) {
            // boxes don't rotate
            angles = vec3_origin;
        }

        collisionModelManager->TransformedBoxTrace(&trace,
                (const_cast<float32 *>(reinterpret_cast<const float32 *>(clip->start))),
                (const_cast<float32 *>(reinterpret_cast<const float32 *>(clip->end))),
                (const_cast<float32 *>(reinterpret_cast<const float32 *>(clip->mins))),
                (const_cast<float32 *>(reinterpret_cast<const float32 *>(clip->maxs))),
                clipHandle,
                clip->contentmask,
                origin, angles,
                clip->collisionType);

        if(trace.allsolid) {
            clip->trace.allsolid = true;
            clip->trace.entityNum = touch->s.number;
        } else if(trace.startsolid) {
            clip->trace.startsolid = true;
            clip->trace.entityNum = touch->s.number;

            clip->trace.entityNum = touch->s.number;
        }

        if(trace.fraction < clip->trace.fraction) {
            bool oldStart;

            // make sure we keep a startsolid from a previous trace
            oldStart = clip->trace.startsolid;

            trace.entityNum = touch->s.number;
            clip->trace = trace;
            clip->trace.startsolid = static_cast<uint>(clip->trace.startsolid) |
                                     static_cast<uint>(oldStart);
        }

        // Reset contents to default
        if(clipHandle == BOX_MODEL_HANDLE) {
            collisionModelManager->SetTempBoxModelContents(CONTENTS_BODY);
        }
    }
}


/*
==================
idServerWorldSystemLocal::Trace

Moves the given mins/maxs volume through the world from start to end.
passEntityNum and entities owned by passEntityNum are explicitly not checked.
==================
*/
void idServerWorldSystemLocal::Trace(trace_t *results, const vec3_t start,
                                     const vec3_t mins, const vec3_t maxs, const vec3_t end, sint passEntityNum,
                                     sint contentmask, traceType_t type) {
    sint i;
    moveclip_t clip;

    if(!mins) {
        mins = vec3_origin;
    }

    if(!maxs) {
        maxs = vec3_origin;
    }

    ::memset(&clip, 0, sizeof(moveclip_t));

    // clip to world
    collisionModelManager->BoxTrace(&clip.trace, start, end, mins, maxs, 0,
                                    contentmask, type);
    clip.trace.entityNum = clip.trace.fraction != 1.0 ? ENTITYNUM_WORLD :
                           ENTITYNUM_NONE;

    if(clip.trace.fraction == 0 || passEntityNum == -2) {
        *results = clip.trace;
        // blocked immediately by the world
        return;
    }

    clip.contentmask = contentmask;
    clip.start = start;
    //  VectorCopy( clip.trace.endpos, clip.end );
    VectorCopy(end, clip.end);
    clip.mins = mins;
    clip.maxs = maxs;
    clip.passEntityNum = passEntityNum;
    clip.collisionType = type;

    // create the bounding box of the entire move
    // we can limit it to the part of the move not
    // already clipped off by the world, which can be
    // a significant savings for line of sight and shot traces
    for(i = 0; i < 3; i++) {
        if(end[i] > start[i]) {
            clip.boxmins[i] = clip.start[i] + clip.mins[i] - 1;
            clip.boxmaxs[i] = clip.end[i] + clip.maxs[i] + 1;
        } else {
            clip.boxmins[i] = clip.end[i] + clip.mins[i] - 1;
            clip.boxmaxs[i] = clip.start[i] + clip.maxs[i] + 1;
        }
    }

    // clip to other solid entities
    ClipMoveToEntities(&clip);

    *results = clip.trace;
}

/*
=============
idServerWorldSystemLocal::PointContents
=============
*/
sint idServerWorldSystemLocal::PointContents(const vec3_t p,
        sint passEntityNum) {
    sint touch[MAX_GENTITIES], i, num, contents, c2;
    float32 *angles;
    sharedEntity_t *hit;
    clipHandle_t clipHandle;

    // get base contents from world
    contents = collisionModelManager->PointContents(p, 0);

    // or in contents from all the other entities
    num = serverWorldSystemLocal.AreaEntities(p, p, touch, MAX_GENTITIES);

    for(i = 0; i < num; i++) {
        if(touch[i] == passEntityNum) {
            continue;
        }

        hit = serverGameSystem->GentityNum(touch[i]);

        // might intersect, so do an exact clip
        clipHandle = ClipHandleForEntity(hit);

        // ydnar: non-worldspawn entities must not use world as clip model!
        if(clipHandle == 0) {
            continue;
        }

        angles = hit->r.currentAngles;

        if(!hit->r.bmodel) {
            angles = vec3_origin; // boxes don't rotate
        }

        c2 = collisionModelManager->TransformedPointContents(p, clipHandle,
                hit->r.currentOrigin, hit->r.currentAngles);

        // Gordon: s.origin/angles is base origin/angles, need to use the current origin/angles for moving entity based water,
        // or water locks in movement start position.
        //c2 = collisionModelManager->TransformedPointContents (p, clipHandle, hit->s.origin, hit->s.angles);

        contents |= c2;
    }

    return contents;
}
