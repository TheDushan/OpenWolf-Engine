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
// File name:   cm_patch.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CM_PATCH_HPP__
#define __CM_PATCH_HPP__

//#define   CULL_BBOX

/*

This file does not reference any globals, and has these entry points:

void CM_ClearLevelPatches( void );
struct patchCollide_s   *CM_GeneratePatchCollide( sint width, sint height, const vec3_t *points );
void CM_TraceThroughPatchCollide( traceWork_t *tw, const struct patchCollide_s *pc );
bool CM_PositionTestInPatchCollide( traceWork_t *tw, const struct patchCollide_s *pc );
void CM_DrawDebugSurface( void (*drawPoly)(sint color, sint numPoints, flaot *points) );


Issues for collision against curved surfaces:

Surface edges need to be handled differently than surface planes

Plane expansion causes raw surfaces to expand past expanded bounding box

Position test of a volume against a surface is tricky.

Position test of a point against a surface is not well defined, because the surface has no volume.


Tracing leading edge points instead of volumes?
Position test by tracing corner to corner? (8*7 traces -- ouch)

coplanar edges
triangulated patches
degenerate patches

  endcaps
  degenerate

WARNING: this may misbehave with meshes that have rows or columns that only
degenerate a few triangles.  Completely degenerate rows and columns are handled
properly.
*/

typedef struct cPlane_s {
    float32           plane[4];
    sint
    signbits;  // signx + (signy<<1) + (signz<<2), used as lookup during collision
    struct cPlane_s *hashChain;
} cPlane_t;

// 3 or four + 6 axial bevels + 4 or 3 * 4 edge bevels
#define MAX_FACET_BEVELS (4 + 6 + 16)

// a facet is a subdivided element of a patch aproximation or model
typedef struct {
    sint             surfacePlane;
    sint             numBorders;
    sint             borderPlanes[MAX_FACET_BEVELS];
    sint             borderInward[MAX_FACET_BEVELS];
    bool        borderNoAdjust[MAX_FACET_BEVELS];
} cFacet_t;

typedef struct cSurfaceCollide_s {
    vec3_t          bounds[2];
    sint             numPlanes; // surface planes plus edge planes
    cPlane_t       *planes;

    sint             numFacets;
    cFacet_t       *facets;
} cSurfaceCollide_t;

typedef struct {
    sint             checkcount;    // to avoid repeated testings
    sint             surfaceFlags;
    sint             contents;
    cSurfaceCollide_t *sc;
    sint             type;
} cSurface_t;

#define MAX_FACETS          1024
#define MAX_PATCH_PLANES    8192

#define MAX_GRID_SIZE 65

typedef struct {
    sint             width;
    sint             height;
    bool        wrapWidth;
    bool        wrapHeight;
    vec3_t          points[MAX_GRID_SIZE][MAX_GRID_SIZE];   // [width][height]
} cGrid_t;

#define SUBDIVIDE_DISTANCE  16  //4 // never more than this units away from curve
#define PLANE_TRI_EPSILON   0.1
#define WRAP_POINT_EPSILON  0.1

cSurfaceCollide_t *CM_GeneratePatchCollide(sint width, sint height,
        vec3_t *points);
cSurfaceCollide_t *CM_GenerateTriangleSoupCollide(sint numVertexes,
        vec3_t *vertexes, sint numIndexes, sint *indexes);

#endif //!__CM_PATCH_HPP__
