////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
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
// File name:   cm_local.h
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CM_LOCAL_H__
#define __CM_LOCAL_H__

#ifndef __Q_SHARED_H__
#include <qcommon/q_shared.h>
#endif
#ifndef __QCOMMON_H__
#include <qcommon/qcommon.h>
#endif
#ifndef __CM_POLYLIB_H__
#include <cm/cm_polylib.h>
#endif
#ifndef __CM_PATCH_H__
#include <cm/cm_patch.h>
#endif

#define MAX_SUBMODELS           512
#define BOX_MODEL_HANDLE        511
#define CAPSULE_MODEL_HANDLE    510

// enable to make the collision detection a bunch faster
#define MRE_OPTIMIZE

typedef struct cbrushedge_s
{
    vec3_t          p0;
    vec3_t          p1;
} cbrushedge_t;

typedef struct
{
    cplane_t*       plane;
    S32             planeNum;
    S32             children[2];	// negative numbers are leafs
    winding_t*      winding;
} cNode_t;

typedef struct
{
    S32		cluster;
    S32     area;
    ptrdiff_t     firstLeafBrush;
    S32     numLeafBrushes;
    ptrdiff_t     firstLeafSurface;
    S32     numLeafSurfaces;
} cLeaf_t;

typedef struct cmodel_s
{
    vec3_t          mins, maxs;
    cLeaf_t         leaf;		// submodels don't reference the main tree
} cmodel_t;

typedef struct
{
    cplane_t*       plane;
    S32             planeNum;
    S32             surfaceFlags;
    S32             shaderNum;
    winding_t*      winding;
} cbrushside_t;


typedef struct
{
    S32             shaderNum;	// the shader that determined the contents
    S32             contents;
    vec3_t          bounds[2];
    S32             numsides;
    cbrushside_t*   sides;
    S32             checkcount;	// to avoid repeated testings
    bool            collided;	// marker for optimisation
    cbrushedge_t*   edges;
    S32             numEdges;
    bool            physicsprocessed;
} cbrush_t;

typedef struct
{
    S32             floodnum;
    S32             floodvalid;
} cArea_t;

typedef struct
{
    UTF8            name[MAX_QPATH];
    S32             numShaders;
    dshader_t*      shaders;
    S32             numBrushSides;
    cbrushside_t*   brushsides;
    S32             numPlanes;
    cplane_t*       planes;
    S32             numNodes;
    cNode_t*        nodes;
    S32             numLeafs;
    cLeaf_t*        leafs;
    S32             numLeafBrushes;
    S32*            leafbrushes;
    S32             numLeafSurfaces;
    S32*            leafsurfaces;
    S32             numSubModels;
    cmodel_t*       cmodels;
    S32             numBrushes;
    cbrush_t*       brushes;
    S32             numClusters;
    S32             clusterBytes;
    U8*           visibility;
    bool        vised;						// if false, visibility is just a single cluster of ffs
    S32             numEntityChars;
    UTF8*           entityString;
    S32             numAreas;
    cArea_t*        areas;
    S32*            areaPortals;				// [ numAreas*numAreas ] reference counts
    S32             numSurfaces;
    cSurface_t**     surfaces;					// non-patches will be nullptr
    S32             floodvalid;
    S32             checkcount;					// incremented on each trace
    bool        perPolyCollision;
} clipMap_t;


// keep 1/8 unit away to keep the position valid before network snapping
// and to avoid various numeric issues
#define SURFACE_CLIP_EPSILON    ( 0.125 )

extern clipMap_t cm;
extern S32      c_pointcontents;
extern S32      c_traces, c_brush_traces, c_patch_traces, c_trisoup_traces;
#ifndef BSPC
extern convar_t*  cm_noAreas;
extern convar_t*  cm_noCurves;
extern convar_t*  cm_playerCurveClip;
extern convar_t*  cm_forceTriangles;
extern convar_t*  cm_optimize;
extern convar_t*  cm_showCurves;
extern convar_t*  cm_showTriangles;
#endif
// cm_test.c

typedef struct
{
    F32		startRadius;
    F32		endRadius;
} biSphere_t;

// Used for oriented capsule collision detection
typedef struct
{
    bool use;
    F32 radius;
    F32 halfheight;
    vec3_t offset;
} sphere_t;

typedef struct
{
    traceType_t		type;
    vec3_t          start;
    vec3_t          end;
    vec3_t          size[2];				// size of the box being swept through the model
    vec3_t          offsets[8];				// [signbits][x] = either size[0][x] or size[1][x]
    F32           maxOffset;				// longest corner length from origin
    vec3_t          extents;				// greatest of abs(size[0]) and abs(size[1])
    vec3_t          bounds[2];				// enclosing box of start and end surrounding by size
    vec3_t          modelOrigin;			// origin of the model tracing through
    S32             contents;				// ored contents of the model tracing through
    bool        isPoint;				// optimized case
    trace_t         trace;					// returned from trace call
    sphere_t        sphere;					// sphere for oriendted capsule collision
    biSphere_t		biSphere;
    bool		testLateralCollision;	// whether or not to test for lateral collision
#ifdef MRE_OPTIMIZE
    cplane_t        tracePlane1;
    cplane_t        tracePlane2;
    F32           traceDist1;
    F32           traceDist2;
    vec3_t          dir;
#endif
} traceWork_t;

typedef struct leafList_s
{
    S32             count;
    S32             maxcount;
    bool        overflowed;
    S32*            list;
    vec3_t          bounds[2];
    S32             lastLeaf;	// for overflows where each leaf can't be stored individually
    void ( *storeLeafs )( struct leafList_s* ll, S32 nodenum );
} leafList_t;

#define SUBDIVIDE_DISTANCE      16      //4 // never more than this units away from curve
#define PLANE_TRI_EPSILON       0.1
#define WRAP_POINT_EPSILON      0.1


cSurfaceCollide_t* CM_GeneratePatchCollide( S32 width, S32 height, vec3_t* points );
void            CM_ClearLevelPatches( void );

// cm_trisoup.c

typedef struct
{
    S32		numTriangles;
    S32     indexes[SHADER_MAX_INDEXES];
    S32     trianglePlanes[SHADER_MAX_TRIANGLES];
    vec3_t  points[SHADER_MAX_TRIANGLES][3];
} cTriangleSoup_t;


//cSurfaceCollide_t* CM_GenerateTriangleSoupCollide( S32 numVertexes, vec3_t* vertexes, S32 numIndexes, S32* indexes );


// cm_test.c
extern const cSurfaceCollide_t* debugSurfaceCollide;
extern const cFacet_t* debugFacet;
extern bool debugBlock;
extern vec3_t   debugBlockPoints[4];

S32             CM_BoxBrushes( const vec3_t mins, const vec3_t maxs, cbrush_t** list, S32 listsize );
void            CM_StoreLeafs( leafList_t* ll, S32 nodenum );
void            CM_StoreBrushes( leafList_t* ll, S32 nodenum );
void            CM_BoxLeafnums_r( leafList_t* ll, S32 nodenum );
cmodel_t*       CM_ClipHandleToModel( clipHandle_t handle );
bool        CM_BoundsIntersect( const vec3_t mins, const vec3_t maxs, const vec3_t mins2, const vec3_t maxs2 );
bool        CM_BoundsIntersectPoint( const vec3_t mins, const vec3_t maxs, const vec3_t point );

//
// idCollisionModelManagerLocal
//
class idCollisionModelManagerLocal : public idCollisionModelManager
{
public:
    virtual void LoadMap( StringEntry name, bool clientload, S32* checksum );
    virtual clipHandle_t InlineModel( S32 index );      // 0 = world, 1 + are bmodels
    virtual clipHandle_t TempBoxModel( const vec3_t mins, const vec3_t maxs, S32 capsule );
    virtual void SetTempBoxModelContents( S32 contents );
    virtual void ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs );
    
    virtual S32 NumClusters( void );
    virtual S32 NumInlineModels( void );
    virtual UTF8* EntityString( void );
    
    // returns an ORed contents mask
    virtual S32 PointContents( const vec3_t p, clipHandle_t model );
    virtual S32 TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles );
    virtual void BoxTrace( trace_t* results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, S32 brushmask, traceType_t type );
    virtual void TransformedBoxTrace( trace_t* results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, S32 brushmask, const vec3_t origin, const vec3_t angles, traceType_t type );
    virtual void BiSphereTrace( trace_t* results, const vec3_t start, const vec3_t end, F32 startRad, F32 endRad, clipHandle_t model, S32 mask );
    virtual void TransformedBiSphereTrace( trace_t* results, const vec3_t start, const vec3_t end, F32 startRad, F32 endRad, clipHandle_t model, S32 mask, const vec3_t origin );
    virtual U8* ClusterPVS( S32 cluster );
    
    virtual S32 PointLeafnum( const vec3_t p );
    
    // only returns non-solid leafs
    // overflow if return listsize and if *lastLeaf != list[listsize-1]
    virtual S32 BoxLeafnums( const vec3_t mins, const vec3_t maxs, S32* list, S32 listsize, S32* lastLeaf );
    
    virtual S32 LeafCluster( S32 leafnum );
    virtual S32 LeafArea( S32 leafnum );
    
    virtual void AdjustAreaPortalState( S32 area1, S32 area2, bool open );
    virtual bool AreasConnected( S32 area1, S32 area2 );
    
    virtual S32 WriteAreaBits( U8* buffer, S32 area );
    
    virtual void ClearMap( void );
    
    virtual void DrawDebugSurface( void ( *drawPoly )( S32 color, S32 numPoints, F32* points ) );
    
    virtual S32 BoxOnPlaneSide( vec3_t emins, vec3_t emaxs, cplane_t* plane );
};

extern idCollisionModelManagerLocal collisionModelManagerLocal;

#endif // !__CM_LOCAL_H__ 
