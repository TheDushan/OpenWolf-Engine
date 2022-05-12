////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2022 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   cm_local.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CM_LOCAL_HPP__
#define __CM_LOCAL_HPP__

#define MAX_SUBMODELS           512
#define BOX_MODEL_HANDLE        511
#define CAPSULE_MODEL_HANDLE    510

// enable to make the collision detection a bunch faster
#define MRE_OPTIMIZE

typedef struct cbrushedge_s {
    vec3_t          p0;
    vec3_t          p1;
} cbrushedge_t;

typedef struct {
    cplane_t       *plane;
    sint             planeNum;
    sint             children[2];   // negative numbers are leafs
    winding_t      *winding;
} cNode_t;

typedef struct {
    sint        cluster;
    sint     area;
    sint64     firstLeafBrush;
    sint     numLeafBrushes;
    sint64     firstLeafSurface;
    sint     numLeafSurfaces;
} cLeaf_t;

typedef struct cmodel_s {
    vec3_t          mins, maxs;
    cLeaf_t         leaf;       // submodels don't reference the main tree
} cmodel_t;

typedef struct {
    cplane_t       *plane;
    sint             planeNum;
    sint             surfaceFlags;
    sint             shaderNum;
    winding_t      *winding;
} cbrushside_t;


typedef struct {
    sint             shaderNum; // the shader that determined the contents
    sint             contents;
    vec3_t          bounds[2];
    sint             numsides;
    cbrushside_t   *sides;
    sint             checkcount;    // to avoid repeated testings
    bool            collided;   // marker for optimisation
    cbrushedge_t   *edges;
    sint             numEdges;
    bool            physicsprocessed;
} cbrush_t;

typedef struct {
    sint             floodnum;
    sint             floodvalid;
} cArea_t;

typedef struct {
    valueType            name[MAX_QPATH];
    sint             numShaders;
    dshader_t      *shaders;
    sint             numBrushSides;
    cbrushside_t   *brushsides;
    sint             numPlanes;
    cplane_t       *planes;
    sint             numNodes;
    cNode_t        *nodes;
    sint             numLeafs;
    cLeaf_t        *leafs;
    sint             numLeafBrushes;
    sint            *leafbrushes;
    sint             numLeafSurfaces;
    sint            *leafsurfaces;
    sint             numSubModels;
    cmodel_t       *cmodels;
    sint             numBrushes;
    cbrush_t       *brushes;
    sint             numClusters;
    sint             clusterBytes;
    uchar8           *visibility;
    bool
    vised;                      // if false, visibility is just a single cluster of ffs
    sint             numEntityChars;
    valueType           *entityString;
    sint             numAreas;
    cArea_t        *areas;
    sint
    *areaPortals;               // [ numAreas*numAreas ] reference counts
    sint             numSurfaces;
    cSurface_t     **surfaces;                  // non-patches will be nullptr
    sint             floodvalid;
    sint
    checkcount;                    // incremented on each trace
    bool        perPolyCollision;
} clipMap_t;


// keep 1/8 unit away to keep the position valid before network snapping
// and to avoid various numeric issues
#define SURFACE_CLIP_EPSILON    ( 0.125f )

extern clipMap_t cm;
extern sint      c_pointcontents;
extern sint      c_traces, c_brush_traces, c_patch_traces,
       c_trisoup_traces;

// cm_test.c

typedef struct {
    float32     startRadius;
    float32     endRadius;
} biSphere_t;

// Used for oriented capsule collision detection
typedef struct {
    bool use;
    float32 radius;
    float32 halfheight;
    vec3_t offset;
} sphere_t;

typedef struct {
    traceType_t     type;
    vec3_t          start;
    vec3_t          end;
    vec3_t
    size[2];                // size of the box being swept through the model
    vec3_t
    offsets[8];             // [signbits][x] = either size[0][x] or size[1][x]
    float32
    maxOffset;                // longest corner length from origin
    vec3_t
    extents;                // greatest of abs(size[0]) and abs(size[1])
    vec3_t
    bounds[2];              // enclosing box of start and end surrounding by size
    vec3_t
    modelOrigin;            // origin of the model tracing through
    sint
    contents;              // ored contents of the model tracing through
    bool        isPoint;                // optimized case
    trace_t         trace;                  // returned from trace call
    sphere_t
    sphere;                 // sphere for oriendted capsule collision
    biSphere_t      biSphere;
    bool
    testLateralCollision;   // whether or not to test for lateral collision
#ifdef MRE_OPTIMIZE
    cplane_t        tracePlane1;
    cplane_t        tracePlane2;
    float32           traceDist1;
    float32           traceDist2;
    vec3_t          dir;
#endif
} traceWork_t;

typedef struct leafList_s {
    sint             count;
    sint             maxcount;
    bool        overflowed;
    sint            *list;
    vec3_t          bounds[2];
    sint
    lastLeaf;  // for overflows where each leaf can't be stored individually
    void (*storeLeafs)(struct leafList_s *ll, sint nodenum);
} leafList_t;

#define SUBDIVIDE_DISTANCE      16      //4 // never more than this units away from curve
#define PLANE_TRI_EPSILON       0.1
#define WRAP_POINT_EPSILON      0.1


cSurfaceCollide_t *CM_GeneratePatchCollide(sint width, sint height,
        vec3_t *points);
void            CM_ClearLevelPatches(void);

// cm_trisoup.c

typedef struct {
    sint        numTriangles;
    sint     indexes[SHADER_MAX_INDEXES];
    sint     trianglePlanes[SHADER_MAX_TRIANGLES];
    vec3_t  points[SHADER_MAX_TRIANGLES][3];
} cTriangleSoup_t;


//cSurfaceCollide_t* CM_GenerateTriangleSoupCollide( sint numVertexes, vec3_t* vertexes, sint numIndexes, sint* indexes );


// cm_test.c
extern const cSurfaceCollide_t *debugSurfaceCollide;
extern const cFacet_t *debugFacet;
extern bool debugBlock;
extern vec3_t   debugBlockPoints[4];

sint             CM_BoxBrushes(const vec3_t mins, const vec3_t maxs,
                               cbrush_t **list, sint listsize);
void            CM_StoreLeafs(leafList_t *ll, sint nodenum);
void            CM_StoreBrushes(leafList_t *ll, sint nodenum);
void            CM_BoxLeafnums_r(leafList_t *ll, sint nodenum);
cmodel_t       *CM_ClipHandleToModel(clipHandle_t handle);
bool        CM_BoundsIntersect(const vec3_t mins, const vec3_t maxs,
                               const vec3_t mins2, const vec3_t maxs2);
bool        CM_BoundsIntersectPoint(const vec3_t mins, const vec3_t maxs,
                                    const vec3_t point);

//
// idCollisionModelManagerLocal
//
class idCollisionModelManagerLocal : public idCollisionModelManager {
public:
    virtual void LoadMap(pointer name, bool clientload, sint *checksum);
    virtual clipHandle_t InlineModel(sint
                                     index);        // 0 = world, 1 + are bmodels
    virtual clipHandle_t TempBoxModel(const vec3_t mins, const vec3_t maxs,
                                      sint capsule);
    virtual void SetTempBoxModelContents(sint contents);
    virtual void ModelBounds(clipHandle_t model, vec3_t mins, vec3_t maxs);

    virtual sint NumClusters(void);
    virtual sint NumInlineModels(void);
    virtual valueType *EntityString(void);

    // returns an ORed contents mask
    virtual sint PointContents(const vec3_t p, clipHandle_t model);
    virtual sint TransformedPointContents(const vec3_t p, clipHandle_t model,
                                          const vec3_t origin, const vec3_t angles);
    virtual void BoxTrace(trace_t *results, const vec3_t start,
                          const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model,
                          sint brushmask, traceType_t type);
    virtual void TransformedBoxTrace(trace_t *results, const vec3_t start,
                                     const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model,
                                     sint brushmask, const vec3_t origin, const vec3_t angles,
                                     traceType_t type);
    virtual void BiSphereTrace(trace_t *results, const vec3_t start,
                               const vec3_t end, float32 startRad, float32 endRad, clipHandle_t model,
                               sint mask);
    virtual void TransformedBiSphereTrace(trace_t *results, const vec3_t start,
                                          const vec3_t end, float32 startRad, float32 endRad, clipHandle_t model,
                                          sint mask, const vec3_t origin);
    virtual uchar8 *ClusterPVS(sint cluster);

    virtual sint PointLeafnum(const vec3_t p);

    // only returns non-solid leafs
    // overflow if return listsize and if *lastLeaf != list[listsize-1]
    virtual sint BoxLeafnums(const vec3_t mins, const vec3_t maxs, sint *list,
                             sint listsize, sint *lastLeaf);

    virtual sint LeafCluster(sint leafnum);
    virtual sint LeafArea(sint leafnum);

    virtual void AdjustAreaPortalState(sint area1, sint area2, bool open);
    virtual bool AreasConnected(sint area1, sint area2);

    virtual sint WriteAreaBits(uchar8 *buffer, sint area);

    virtual void ClearMap(void);

    virtual void DrawDebugSurface(void (*drawPoly)(sint color, sint numPoints,
                                  float32 *points));

    virtual sint BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, cplane_t *plane);
};

extern idCollisionModelManagerLocal collisionModelManagerLocal;

#endif // !__CM_LOCAL_HPP__ 
