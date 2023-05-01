////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2018 - 2023 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   serverWorld.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SERVERWORLD_HPP__
#define __SERVERWORLD_HPP__

/*
===============================================================================
ENTITY CHECKING

To avoid linearly searching through lists of entities during environment testing,
the world is carved up with an evenly spaced, axially aligned bsp tree.  Entities
are kept in chains either at the final leafs, or at the first node that splits
them, which prevents having to deal with multiple fragments of a single entity.
===============================================================================
*/

typedef struct worldSector_s {
    sint    axis; // -1 = leaf node
    float32 dist;
    struct worldSector_s *children[2];
    svEntity_t *entities;
} worldSector_t;

#define AREA_DEPTH 4
#define AREA_NODES 64

static worldSector_t sv_worldSectors[AREA_NODES];
static sint sv_numworldSectors;

#define MAX_TOTAL_ENT_LEAFS 128

/*
============================================================================
AREA QUERY

Fills in a list of all entities who's absmin / absmax intersects the given
bounds.  This does NOT mean that they actually touch in the case of bmodels.
============================================================================
*/

typedef struct {
    const float32 *mins;
    const float32 *maxs;
    sint *list;
    sint count, maxcount;
} areaParms_t;

typedef struct {
    vec3_t boxmins, boxmaxs; // enclose the test object along entire move
    const float32 *mins;
    const float32 *maxs; // size of the moving object
    const float32 *start;
    vec3_t end;
    trace_t trace;
    sint passEntityNum;
    sint contentmask;
    traceType_t collisionType;
} moveclip_t;

// FIXME: Copied from cm_local.hpp
#define BOX_MODEL_HANDLE 511

//
// idServerWorldSystemLocal
//
class idServerWorldSystemLocal : public idServerWorldSystem {
public:
    // called after the world model has been loaded, before linking any entities
    virtual void UnlinkEntity(sharedEntity_t *gEnt);
    virtual void LinkEntity(sharedEntity_t *gEnt);
    virtual sint AreaEntities(const vec3_t mins, const vec3_t maxs,
                              sint *entityList, sint maxcount);
    virtual sint PointContents(const vec3_t p, sint passEntityNum);
    virtual void Trace(trace_t *results, const vec3_t start, const vec3_t mins,
                       const vec3_t maxs, const vec3_t end, sint passEntityNum, sint contentmask,
                       traceType_t type);
public:
    idServerWorldSystemLocal();
    ~idServerWorldSystemLocal();

    // Needs to be called any time an entity changes origin, mins, maxs,
    // or solid.  Automatically unlinks if needed.
    // sets ent->v.absmin and ent->v.absmax
    // sets ent->leafnums[] for pvs determination even if the entity
    // is not solid
    static clipHandle_t ClipHandleForEntity(const sharedEntity_t *ent);
    static void SectorList_f(void);
    static worldSector_t *CreateworldSector(sint depth, vec3_t mins,
                                            vec3_t maxs);
    static void ClearWorld(void);
    static void AreaEntities_r(worldSector_t *node, areaParms_t *ap);
    static void ClipToEntity(trace_t *trace, const vec3_t start,
                             const vec3_t mins, const vec3_t maxs, const vec3_t end, sint entityNum,
                             sint contentmask, traceType_t type);
    static void ClipMoveToEntities(moveclip_t *clip);
};

extern idServerWorldSystemLocal serverWorldSystemLocal;

#endif //!__SERVERGAME_HPP__
