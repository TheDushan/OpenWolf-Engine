////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 - 2019 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   cm_api.h
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CM_PUBLIC_H__
#define __CM_PUBLIC_H__

// plane types are used to speed some tests
// 0-2 are axial planes
#define PLANE_X             0
#define PLANE_Y             1
#define PLANE_Z             2
#define PLANE_NON_AXIAL     3
#define PLANE_NON_PLANAR    4

/*
=================
PlaneTypeForNormal
=================
*/

#define PlaneTypeForNormal( x ) ( x[0] == 1.0 ? PLANE_X : ( x[1] == 1.0 ? PLANE_Y : ( x[2] == 1.0 ? PLANE_Z : PLANE_NON_AXIAL ) ) )

// trace->entityNum can also be 0 to (MAX_GENTITIES-1)
// or ENTITYNUM_NONE, ENTITYNUM_WORLD

// markfragments are returned by CM_MarkFragments()
typedef struct
{
    S32 firstPoint;
    S32 numPoints;
} markFragment_t;

typedef struct
{
    vec3_t origin;
    vec3_t axis[3];
} orientation_t;

typedef enum
{
    TT_NONE,
    
    TT_AABB,
    TT_CAPSULE,
    TT_BISPHERE,
    
    TT_NUM_TRACE_TYPES
} traceType_t;

//
// idCollisionModelManager
//
class idCollisionModelManager
{
public:
    virtual void LoadMap( StringEntry name, bool clientload, S32* checksum ) = 0;
    virtual clipHandle_t InlineModel( S32 index ) = 0;      // 0 = world, 1 + are bmodels
    virtual clipHandle_t TempBoxModel( const vec3_t mins, const vec3_t maxs, S32 capsule ) = 0;
    virtual void ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs ) = 0;
    virtual void SetTempBoxModelContents( S32 contents ) = 0;
    virtual S32 NumClusters( void ) = 0;
    virtual S32 NumInlineModels( void ) = 0;
    virtual UTF8* EntityString( void ) = 0;
    
    // returns an ORed contents mask
    virtual S32 PointContents( const vec3_t p, clipHandle_t model ) = 0;
    virtual S32 TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles ) = 0;
    virtual void BoxTrace( trace_t* results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, S32 brushmask, traceType_t type ) = 0;
    virtual void TransformedBoxTrace( trace_t* results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, S32 brushmask, const vec3_t origin, const vec3_t angles, traceType_t type ) = 0;
    virtual void BiSphereTrace( trace_t* results, const vec3_t start, const vec3_t end, F32 startRad, F32 endRad, clipHandle_t model, S32 mask ) = 0;
    virtual void TransformedBiSphereTrace( trace_t* results, const vec3_t start, const vec3_t end, F32 startRad, F32 endRad, clipHandle_t model, S32 mask, const vec3_t origin ) = 0;
    
    virtual U8* ClusterPVS( S32 cluster ) = 0;
    
    virtual S32 PointLeafnum( const vec3_t p ) = 0;
    
    // only returns non-solid leafs
    // overflow if return listsize and if *lastLeaf != list[listsize-1]
    virtual S32 BoxLeafnums( const vec3_t mins, const vec3_t maxs, S32* list, S32 listsize, S32* lastLeaf ) = 0;
    
    virtual S32 LeafCluster( S32 leafnum ) = 0;
    virtual S32 LeafArea( S32 leafnum ) = 0;
    
    virtual void AdjustAreaPortalState( S32 area1, S32 area2, bool open ) = 0;
    virtual bool AreasConnected( S32 area1, S32 area2 ) = 0;
    
    virtual S32 WriteAreaBits( U8* buffer, S32 area ) = 0;
    
    virtual void ClearMap( void ) = 0;
    
    // cm_patch.cpp
    virtual void DrawDebugSurface( void ( *drawPoly )( S32 color, S32 numPoints, F32* points ) ) = 0;
    
    virtual S32 BoxOnPlaneSide( vec3_t emins, vec3_t emaxs, cplane_t* plane ) = 0;
};

extern idCollisionModelManager* collisionModelManager;

#endif // !__CM_PUBLIC_H__
