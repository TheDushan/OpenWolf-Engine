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
// File name:   cm_patch.cpp
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

sint c_totalPatchBlocks, c_totalPatchSurfaces, c_totalPatchEdges;

const cSurfaceCollide_t *debugSurfaceCollide;
const cFacet_t *debugFacet;
bool debugBlock;
vec3_t debugBlockPoints[4];

/*
=================
CM_ClearLevelPatches
=================
*/
void CM_ClearLevelPatches(void) {
    debugSurfaceCollide = nullptr;
    debugFacet = nullptr;
}

/*
=================
CM_SignbitsForNormal
=================
*/
static sint CM_SignbitsForNormal(vec3_t normal) {
    sint bits, j;

    bits = 0;

    for(j = 0; j < 3; j++) {
        if(normal[j] < 0) {
            bits |= 1 << j;
        }
    }

    return bits;
}

/*
=====================
CM_PlaneFromPoints

Returns false if the triangle is degenrate.
The normal will point out of the clock for clockwise ordered points
=====================
*/
static bool CM_PlaneFromPoints(vec4_t plane, vec3_t a, vec3_t b,
                               vec3_t c) {
    vec3_t d1, d2;

    VectorSubtract(b, a, d1);
    VectorSubtract(c, a, d2);
    CrossProduct(d2, d1, plane);

    if(VectorNormalize(plane) == 0) {
        return false;
    }

    plane[3] = DotProduct(a, plane);
    return true;
}


/*
================================================================================
GRID SUBDIVISION
================================================================================
*/

/*
=================
CM_NeedsSubdivision

Returns true if the given quadratic curve is not flat enough for our
collision detection purposes
=================
*/
static bool CM_NeedsSubdivision(vec3_t a, vec3_t b, vec3_t c) {
    vec3_t          cmid, lmid, delta;
    float32           dist;
    sint             i;

    // calculate the linear midpoint
    for(i = 0; i < 3; i++) {
        lmid[i] = 0.5f * (a[i] + c[i]);
    }

    // calculate the exact curve midpoint
    for(i = 0; i < 3; i++) {
        cmid[i] = 0.5f * (0.5f * (a[i] + b[i]) + 0.5f * (b[i] + c[i]));
    }

    // see if the curve is far enough away from the linear mid
    VectorSubtract(cmid, lmid, delta);
    dist = VectorLengthSquared(delta);

    return (bool)(dist >= SUBDIVIDE_DISTANCE * SUBDIVIDE_DISTANCE);
}

/*
===============
CM_Subdivide

a, b, and c are control points.
the subdivided sequence will be: a, out1, out2, out3, c
===============
*/
static void CM_Subdivide(vec3_t a, vec3_t b, vec3_t c, vec3_t out1,
                         vec3_t out2, vec3_t out3) {
    sint i;

    for(i = 0; i < 3; i++) {
        out1[i] = 0.5f * (a[i] + b[i]);
        out3[i] = 0.5f * (b[i] + c[i]);
        out2[i] = 0.5f * (out1[i] + out3[i]);
    }
}

/*
=================
CM_TransposeGrid

Swaps the rows and columns in place
=================
*/
static void CM_TransposeGrid(cGrid_t *grid) {
    sint             i, j, l;
    vec3_t          temp;
    bool        tempWrap;

    if(grid->width > grid->height) {
        for(i = 0; i < grid->height; i++) {
            for(j = i + 1; j < grid->width; j++) {
                if(j < grid->height) {
                    // swap the value
                    VectorCopy(grid->points[i][j], temp);
                    VectorCopy(grid->points[j][i], grid->points[i][j]);
                    VectorCopy(temp, grid->points[j][i]);
                } else {
                    // just copy
                    VectorCopy(grid->points[j][i], grid->points[i][j]);
                }
            }
        }
    } else {
        for(i = 0; i < grid->width; i++) {
            for(j = i + 1; j < grid->height; j++) {
                if(j < grid->width) {
                    // swap the value
                    VectorCopy(grid->points[j][i], temp);
                    VectorCopy(grid->points[i][j], grid->points[j][i]);
                    VectorCopy(temp, grid->points[i][j]);
                } else {
                    // just copy
                    VectorCopy(grid->points[i][j], grid->points[j][i]);
                }
            }
        }
    }

    l = grid->width;
    grid->width = grid->height;
    grid->height = l;

    tempWrap = grid->wrapWidth;
    grid->wrapWidth = grid->wrapHeight;
    grid->wrapHeight = tempWrap;
}

/*
===================
CM_SetGridWrapWidth

If the left and right columns are exactly equal, set grid->wrapWidth true
===================
*/
static void CM_SetGridWrapWidth(cGrid_t *grid) {
    sint             i, j;
    float32           d;

    for(i = 0; i < grid->height; i++) {
        for(j = 0; j < 3; j++) {
            d = grid->points[0][i][j] - grid->points[grid->width - 1][i][j];

            if(d < -WRAP_POINT_EPSILON || d > WRAP_POINT_EPSILON) {
                break;
            }
        }

        if(j != 3) {
            break;
        }
    }

    if(i == grid->height) {
        grid->wrapWidth = true;
    } else {
        grid->wrapWidth = false;
    }
}

/*
=================
CM_SubdivideGridColumns

Adds columns as necessary to the grid until
all the aproximating points are within SUBDIVIDE_DISTANCE
from the true curve
=================
*/
static void CM_SubdivideGridColumns(cGrid_t *grid) {
    sint i, j, k;

    for(i = 0; i < grid->width - 2;) {
        // grid->points[i][x] is an interpolating control point
        // grid->points[i+1][x] is an aproximating control point
        // grid->points[i+2][x] is an interpolating control point

        //
        // first see if we can collapse the aproximating collumn away
        //
        for(j = 0; j < grid->height; j++) {
            if(CM_NeedsSubdivision(grid->points[i][j], grid->points[i + 1][j],
                                   grid->points[i + 2][j])) {
                break;
            }
        }

        if(j == grid->height) {
            // all of the points were close enough to the linear midpoints
            // that we can collapse the entire column away
            for(j = 0; j < grid->height; j++) {
                // remove the column
                for(k = i + 2; k < grid->width; k++) {
                    VectorCopy(grid->points[k][j], grid->points[k - 1][j]);
                }
            }

            grid->width--;

            // go to the next curve segment
            i++;
            continue;
        }

        //
        // we need to subdivide the curve
        //
        for(j = 0; j < grid->height; j++) {
            vec3_t prev, mid, next;

            // save the control points now
            VectorCopy(grid->points[i][j], prev);
            VectorCopy(grid->points[i + 1][j], mid);
            VectorCopy(grid->points[i + 2][j], next);

            // make room for two additional columns in the grid
            // columns i+1 will be replaced, column i+2 will become i+4
            // i+1, i+2, and i+3 will be generated
            for(k = grid->width - 1; k > i + 1; k--) {
                VectorCopy(grid->points[k][j], grid->points[k + 2][j]);
            }

            // generate the subdivided points
            CM_Subdivide(prev, mid, next, grid->points[i + 1][j],
                         grid->points[i + 2][j], grid->points[i + 3][j]);
        }

        grid->width += 2;

        // the new aproximating point at i+1 may need to be removed
        // or subdivided farther, so don't advance i
    }
}

/*
======================
CM_ComparePoints
======================
*/
#define POINT_EPSILON   0.1
static bool CM_ComparePoints(float32 *a, float32 *b) {
    float32 d;

    d = a[0] - b[0];

    if(d < -POINT_EPSILON || d > POINT_EPSILON) {
        return false;
    }

    d = a[1] - b[1];

    if(d < -POINT_EPSILON || d > POINT_EPSILON) {
        return false;
    }

    d = a[2] - b[2];

    if(d < -POINT_EPSILON || d > POINT_EPSILON) {
        return false;
    }

    return true;
}

/*
=================
CM_RemoveDegenerateColumns

If there are any identical columns, remove them
=================
*/
static void CM_RemoveDegenerateColumns(cGrid_t *grid) {
    sint i, j, k;

    for(i = 0; i < grid->width - 1; i++) {
        for(j = 0; j < grid->height; j++) {
            if(!CM_ComparePoints(grid->points[i][j], grid->points[i + 1][j])) {
                break;
            }
        }

        if(j != grid->height) {
            continue; // not degenerate
        }

        for(j = 0; j < grid->height; j++) {
            // remove the column
            for(k = i + 2; k < grid->width; k++) {
                VectorCopy(grid->points[k][j], grid->points[k - 1][j]);
            }
        }

        grid->width--;

        // check against the next column
        i--;
    }
}

/*
================================================================================
PATCH COLLIDE GENERATION
================================================================================
*/

static sint      numPlanes;
static cPlane_t planes[MAX_PATCH_PLANES];

static cFacet_t facets[MAX_FACETS];

#define NORMAL_EPSILON  0.00015
#define DIST_EPSILON    0.0235

/*
==================
CM_PlaneEqual
==================
*/
static sint CM_PlaneEqual(cPlane_t *p, float32 plane[4], sint *flipped) {
    float32 invplane[4];

    if(Q_fabs(p->plane[0] - plane[0]) < NORMAL_EPSILON &&
            Q_fabs(p->plane[1] - plane[1]) < NORMAL_EPSILON
            && Q_fabs(p->plane[2] - plane[2]) < NORMAL_EPSILON &&
            Q_fabs(p->plane[3] - plane[3]) < DIST_EPSILON) {
        *flipped = false;
        return true;
    }

    VectorNegate(plane, invplane);
    invplane[3] = -plane[3];

    if(Q_fabs(p->plane[0] - invplane[0]) < NORMAL_EPSILON &&
            Q_fabs(p->plane[1] - invplane[1]) < NORMAL_EPSILON
            && Q_fabs(p->plane[2] - invplane[2]) < NORMAL_EPSILON &&
            Q_fabs(p->plane[3] - invplane[3]) < DIST_EPSILON) {
        *flipped = true;
        return true;
    }

    return false;
}

/*
==================
CM_SnapVector
==================
*/
static void CM_SnapVector(vec3_t normal) {
    sint i;

    for(i = 0; i < 3; i++) {
        if(Q_fabs(normal[i] - 1) < NORMAL_EPSILON) {
            VectorClear(normal);
            normal[i] = 1;
            break;
        }

        if(Q_fabs(normal[i] - -1) < NORMAL_EPSILON) {
            VectorClear(normal);
            normal[i] = -1;
            break;
        }
    }
}

/*
==================
CM_FindPlane2
==================
*/
static sint CM_FindPlane2(float32 plane[4], sint *flipped) {
    sint i;

    // see if the points are close enough to an existing plane
    for(i = 0; i < numPlanes; i++) {
        if(CM_PlaneEqual(&planes[i], plane, flipped)) {
            return i;
        }
    }

    // add a new plane
    if(numPlanes == MAX_PATCH_PLANES) {
        common->Error(ERR_DROP, "CM_FindPlane2: MAX_PATCH_PLANES");
    }

    Vector4Copy(plane, planes[numPlanes].plane);
    planes[numPlanes].signbits = CM_SignbitsForNormal(plane);

    numPlanes++;

    *flipped = false;

    return numPlanes - 1;
}

/*
==================
CM_FindPlane
==================
*/
static sint CM_FindPlane(float32 *p1, float32 *p2, float32 *p3) {
    float32           plane[4], d;
    sint             i;

    if(!CM_PlaneFromPoints(plane, p1, p2, p3)) {
        return -1;
    }

    // see if the points are close enough to an existing plane
    for(i = 0; i < numPlanes; i++) {
        if(DotProduct(plane, planes[i].plane) < 0) {
            continue; // allow backwards planes?
        }

        d = DotProduct(p1, planes[i].plane) - planes[i].plane[3];

        if(d < -PLANE_TRI_EPSILON || d > PLANE_TRI_EPSILON) {
            continue;
        }

        d = DotProduct(p2, planes[i].plane) - planes[i].plane[3];

        if(d < -PLANE_TRI_EPSILON || d > PLANE_TRI_EPSILON) {
            continue;
        }

        d = DotProduct(p3, planes[i].plane) - planes[i].plane[3];

        if(d < -PLANE_TRI_EPSILON || d > PLANE_TRI_EPSILON) {
            continue;
        }

        // found it
        return i;
    }

    // add a new plane
    if(numPlanes == MAX_PATCH_PLANES) {
        common->Error(ERR_DROP, "MAX_PATCH_PLANES");
    }

    Vector4Copy(plane, planes[numPlanes].plane);
    planes[numPlanes].signbits = CM_SignbitsForNormal(plane);

    numPlanes++;

    return numPlanes - 1;
}

/*
==================
CM_PointOnPlaneSide
==================
*/
static sint CM_PointOnPlaneSide(float32 *p, sint planeNum) {
    float32 *plane, d;

    if(planeNum == -1) {
        return SIDE_ON;
    }

    plane = planes[planeNum].plane;

    d = DotProduct(p, plane) - plane[3];

    if(d > PLANE_TRI_EPSILON) {
        return SIDE_FRONT;
    }

    if(d < -PLANE_TRI_EPSILON) {
        return SIDE_BACK;
    }

    return SIDE_ON;
}

/*
==================
CM_GridPlane
==================
*/
static sint CM_GridPlane(sint gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2],
                         sint i, sint j, sint tri) {
    sint p;

    p = gridPlanes[i][j][tri];

    if(p != -1) {
        return p;
    }

    p = gridPlanes[i][j][!tri];

    if(p != -1) {
        return p;
    }

    // should never happen
    common->Printf("WARNING: CM_GridPlane unresolvable\n");
    return -1;
}

/*
==================
CM_EdgePlaneNum
==================
*/
static sint CM_EdgePlaneNum(cGrid_t *grid,
                            sint gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2], sint i, sint j, sint k) {
    float32          *p1, *p2;
    vec3_t          up;
    sint             p;

    switch(k) {
        case 0: // top border
            p1 = grid->points[i][j];
            p2 = grid->points[i + 1][j];
            p = CM_GridPlane(gridPlanes, i, j, 0);

            if(p == -1) {
                common->Printf(" top border %f %f %f\n", grid->points[i + 1][j][0],
                               grid->points[i + 1][j][1], grid->points[i + 1][j][2]);
            }

            VectorMA(p1, 4, planes[p].plane, up);
            return CM_FindPlane(p1, p2, up);

        case 2: // bottom border
            p1 = grid->points[i][j + 1];
            p2 = grid->points[i + 1][j + 1];
            p = CM_GridPlane(gridPlanes, i, j, 1);

            if(p == -1) {
                common->Printf(" bottom border %f %f %f\n", grid->points[i + 1][j][0],
                               grid->points[i + 1][j][1], grid->points[i + 1][j][2]);
            }

            VectorMA(p1, 4, planes[p].plane, up);
            return CM_FindPlane(p2, p1, up);

        case 3: // left border
            p1 = grid->points[i][j];
            p2 = grid->points[i][j + 1];
            p = CM_GridPlane(gridPlanes, i, j, 1);

            if(p == -1) {
                common->Printf(" left border %f %f %f\n", grid->points[i][j][0],
                               grid->points[i][j][1], grid->points[i][j][2]);
            }

            VectorMA(p1, 4, planes[p].plane, up);
            return CM_FindPlane(p2, p1, up);

        case 1: // right border
            p1 = grid->points[i + 1][j];
            p2 = grid->points[i + 1][j + 1];
            p = CM_GridPlane(gridPlanes, i, j, 0);

            if(p == -1) {
                common->Printf(" right border %f %f %f\n", grid->points[i][j][0],
                               grid->points[i][j][1], grid->points[i][j][2]);
            }

            VectorMA(p1, 4, planes[p].plane, up);
            return CM_FindPlane(p1, p2, up);

        case 4: // diagonal out of triangle 0
            p1 = grid->points[i + 1][j + 1];
            p2 = grid->points[i][j];
            p = CM_GridPlane(gridPlanes, i, j, 0);

            if(p == -1) {
                common->Printf(" diagonal out of triangle 0 %f %f %f\n",
                               grid->points[i + 1][j + 1][0], grid->points[i + 1][j + 1][1],
                               grid->points[i + 1][j + 1][2]);
            }

            VectorMA(p1, 4, planes[p].plane, up);
            return CM_FindPlane(p1, p2, up);

        case 5: // diagonal out of triangle 1
            p1 = grid->points[i][j];
            p2 = grid->points[i + 1][j + 1];
            p = CM_GridPlane(gridPlanes, i, j, 1);

            if(p == -1) {
                common->Printf(" diagonal out of triangle 1 %f %f %f\n",
                               grid->points[i + 1][j][0], grid->points[i + 1][j][1],
                               grid->points[i + 1][j][2]);
            }

            VectorMA(p1, 4, planes[p].plane, up);
            return CM_FindPlane(p1, p2, up);
    }

    common->Error(ERR_DROP, "CM_EdgePlaneNum: bad k");
    return -1;
}

/*
===================
CM_SetBorderInward
===================
*/
static void CM_SetBorderInward(cFacet_t *facet, cGrid_t *grid,
                               sint gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2], sint i, sint j,
                               sint which) {
    sint             k, l, numPoints;
    float32          *points[4];

    switch(which) {
        case -1:
            points[0] = grid->points[i][j];
            points[1] = grid->points[i + 1][j];
            points[2] = grid->points[i + 1][j + 1];
            points[3] = grid->points[i][j + 1];
            numPoints = 4;
            break;

        case 0:
            points[0] = grid->points[i][j];
            points[1] = grid->points[i + 1][j];
            points[2] = grid->points[i + 1][j + 1];
            numPoints = 3;
            break;

        case 1:
            points[0] = grid->points[i + 1][j + 1];
            points[1] = grid->points[i][j + 1];
            points[2] = grid->points[i][j];
            numPoints = 3;
            break;

        default:
            common->Error(ERR_FATAL, "CM_SetBorderInward: bad parameter");
            numPoints = 0;
            break;
    }

    for(k = 0; k < facet->numBorders; k++) {
        sint front, back;

        front = 0;
        back = 0;

        for(l = 0; l < numPoints; l++) {
            sint side;

            side = CM_PointOnPlaneSide(points[l], facet->borderPlanes[k]);

            if(side == SIDE_FRONT) {
                front++;
            } else if(side == SIDE_BACK) {
                back++;
            }
        }

        if(front && !back) {
            facet->borderInward[k] = true;
        } else if(back && !front) {
            facet->borderInward[k] = false;
        } else if(!front && !back) {
            // flat side border
            facet->borderPlanes[k] = -1;
        } else {
            // bisecting side border
            if(developer->integer) {
                common->Printf("WARNING: CM_SetBorderInward: mixed plane sides\n");
            }

            facet->borderInward[k] = false;

            if(!debugBlock) {
                debugBlock = true;
                VectorCopy(grid->points[i][j], debugBlockPoints[0]);
                VectorCopy(grid->points[i + 1][j], debugBlockPoints[1]);
                VectorCopy(grid->points[i + 1][j + 1], debugBlockPoints[2]);
                VectorCopy(grid->points[i][j + 1], debugBlockPoints[3]);
            }
        }
    }
}

/*
==================
CM_ValidateFacet

If the facet isn't bounded by its borders, we screwed up.
==================
*/
static bool CM_ValidateFacet(cFacet_t *facet) {
    float32           plane[4];
    sint             j;
    winding_t      *w;
    vec3_t          bounds[2];

    if(facet->surfacePlane == -1) {
        return false;
    }

    Vector4Copy(planes[facet->surfacePlane].plane, plane);
    w = BaseWindingForPlane(plane, plane[3]);

    for(j = 0; j < facet->numBorders && w; j++) {
        if(facet->borderPlanes[j] == -1) {
            FreeWinding(w);
            return false;
        }

        Vector4Copy(planes[facet->borderPlanes[j]].plane, plane);

        if(!facet->borderInward[j]) {
            VectorSubtract(vec3_origin, plane, plane);
            plane[3] = -plane[3];
        }

        ChopWindingInPlace(&w, plane, plane[3], 0.1f);
    }

    if(!w) {
        return false; // winding was completely chopped away
    }

    // see if the facet is unreasonably large
    WindingBounds(w, bounds[0], bounds[1]);
    FreeWinding(w);

    for(j = 0; j < 3; j++) {
        if(bounds[1][j] - bounds[0][j] > MAX_WORLD_COORD) {
            return false; // we must be missing a plane
        }

        if(bounds[0][j] >= MAX_WORLD_COORD) {
            return false;
        }

        if(bounds[1][j] <= MIN_WORLD_COORD) {
            return false;
        }
    }

    return true; // winding is fine
}

/*
==================
CM_AddFacetBevels
==================
*/
static void CM_AddFacetBevels(cFacet_t *facet) {
    sint i, j, k, l, axis, dir, flipped;
    float32 plane[4], minBack, newplane[4];
    winding_t *w, *w2;
    vec3_t mins, maxs, vec, vec2;
    float64 d, d1[3], d2[3];

    Vector4Copy(planes[facet->surfacePlane].plane, plane);

    w = BaseWindingForPlane(plane, plane[3]);

    for(j = 0; j < facet->numBorders && w; j++) {
        if(facet->borderPlanes[j] == facet->surfacePlane) {
            continue;
        }

        Vector4Copy(planes[facet->borderPlanes[j]].plane, plane);

        if(!facet->borderInward[j]) {
            VectorSubtract(vec3_origin, plane, plane);
            plane[3] = -plane[3];
        }

        ChopWindingInPlace(&w, plane, plane[3], 0.1f);
    }

    if(!w) {
        return;
    }

    WindingBounds(w, mins, maxs);

    // add the axial planes
    for(axis = 0; axis < 3; axis++) {
        for(dir = -1; dir <= 1; dir += 2) {
            VectorClear(plane);
            plane[axis] = dir;

            if(dir == 1) {
                plane[3] = maxs[axis];
            } else {
                plane[3] = -mins[axis];
            }

            //if it's the surface plane
            if(CM_PlaneEqual(&planes[facet->surfacePlane], plane, &flipped)) {
                continue;
            }

            // see if the plane is allready present
            for(i = 0; i < facet->numBorders; i++) {
                if(dir > 0) {
                    if(planes[facet->borderPlanes[i]].plane[axis] >= 0.9999f) {
                        break;
                    }
                } else {
                    if(planes[facet->borderPlanes[i]].plane[axis] <= -0.9999f) {
                        break;
                    }
                }
            }

            if(i == facet->numBorders) {
                if(facet->numBorders >= MAX_FACET_BEVELS) {
                    common->Printf("ERROR: too many bevels\n");
                    continue;
                }

                facet->borderPlanes[facet->numBorders] = CM_FindPlane2(plane, &flipped);
                facet->borderNoAdjust[facet->numBorders] = false;
                facet->borderInward[facet->numBorders] = flipped;
                facet->numBorders++;
            }
        }
    }

    //
    // add the edge bevels
    //
    // test the non-axial plane edges
    for(j = 0; j < w->numpoints; j++) {
        k = (j + 1) % w->numpoints;
        VectorCopy(w->p[j], d1);
        VectorCopy(w->p[k], d2);
        VectorSubtract(d1, d2, vec);

        //if it's a degenerate edge
        if(VectorNormalize(vec) < 0.5) {
            continue;
        }

        CM_SnapVector(vec);

        for(k = 0; k < 3; k++) {
            if(vec[k] == -1 || vec[k] == 1) {
                break; // axial
            }
        }

        if(k < 3) {
            continue; // only test non-axial edges
        }

        // try the six possible slanted axials from this edge
        for(axis = 0; axis < 3; axis++) {
            for(dir = -1; dir <= 1; dir += 2) {
                // construct a plane
                VectorClear(vec2);
                vec2[axis] = dir;
                CrossProduct(vec, vec2, plane);

                if(VectorNormalize(plane) < 0.5) {
                    continue;
                }

                plane[3] = DotProduct(w->p[j], plane);

                // if all the points of the facet winding are
                // behind this plane, it is a proper edge bevel
                minBack = 0.0f;

                for(l = 0; l < w->numpoints; l++) {
                    d = DotProduct(w->p[l], plane) - plane[3];

                    if(d > 0.1) {
                        break;    // point in front
                    }

                    if(d < minBack) {
                        minBack = d;
                    }
                }

                // if some point was at the front
                if(l < w->numpoints) {
                    continue;
                }

                // if no points at the back then the winding is on the bevel plane
                if(minBack > -0.1f) {
                    break;
                }

                //if it's the surface plane
                if(CM_PlaneEqual(&planes[facet->surfacePlane], plane, &flipped)) {
                    continue;
                }

                // see if the plane is allready present
                for(i = 0; i < facet->numBorders; i++) {
                    if(CM_PlaneEqual(&planes[facet->borderPlanes[i]], plane, &flipped)) {
                        break;
                    }
                }

                if(i == facet->numBorders) {
                    if(facet->numBorders >= MAX_FACET_BEVELS) {
                        common->Printf("ERROR: too many bevels\n");
                        continue;
                    }

                    facet->borderPlanes[facet->numBorders] = CM_FindPlane2(plane, &flipped);

                    for(k = 0; k < facet->numBorders; k++) {
                        if(facet->borderPlanes[facet->numBorders] == facet->borderPlanes[k]) {
                            common->Printf("WARNING: bevel plane already used\n");
                        }
                    }

                    facet->borderNoAdjust[facet->numBorders] = false;
                    facet->borderInward[facet->numBorders] = flipped;

                    w2 = CopyWinding(w);
                    Vector4Copy(planes[facet->borderPlanes[facet->numBorders]].plane,
                                newplane);

                    if(!facet->borderInward[facet->numBorders]) {
                        VectorNegate(newplane, newplane);
                        newplane[3] = -newplane[3];
                    }

                    ChopWindingInPlace(&w2, newplane, newplane[3], 0.1f);

                    if(!w2) {
                        if(developer->integer) {
                            common->Printf("WARNING: Invalid patch %f %f %f %f\n", w->p[0][0],
                                           w->p[0][1],
                                           w->p[0][2], w->p[0][3]);
                        }

                        continue;
                    } else {
                        FreeWinding(w2);
                    }

                    facet->numBorders++;
                    //already got a bevel
                    //                  break;
                }
            }
        }
    }

    FreeWinding(w);

#ifndef BSPC

    // add opposite plane
    if(facet->numBorders >= 4 + 6 + 16) {
        common->Printf("ERROR: too many bevels\n");
        return;
    }

    facet->borderPlanes[facet->numBorders] = facet->surfacePlane;
    facet->borderNoAdjust[facet->numBorders] = false;
    facet->borderInward[facet->numBorders] = true;
    facet->numBorders++;
#endif
}

enum edgeName_t {
    EN_TOP,
    EN_RIGHT,
    EN_BOTTOM,
    EN_LEFT
};

/*
==================
CM_PatchCollideFromGrid
==================
*/
static void CM_SurfaceCollideFromGrid(cGrid_t *grid,
                                      cSurfaceCollide_t *sc) {
    sint i, j, gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2], borders[4];
    bool noAdjust[4];
    float32 *p1, *p2, *p3;
    cFacet_t *facet;
    sint numFacets;
    //facets = (cFacet_t*)memorySystem->Malloc(MAX_FACETS);

    numPlanes = 0;
    numFacets = 0;

    // find the planes for each triangle of the grid
    for(i = 0; i < grid->width - 1; i++) {
        for(j = 0; j < grid->height - 1; j++) {
            p1 = grid->points[i][j];
            p2 = grid->points[i + 1][j];
            p3 = grid->points[i + 1][j + 1];
            gridPlanes[i][j][0] = CM_FindPlane(p1, p2, p3);

            p1 = grid->points[i + 1][j + 1];
            p2 = grid->points[i][j + 1];
            p3 = grid->points[i][j];
            gridPlanes[i][j][1] = CM_FindPlane(p1, p2, p3);
        }
    }

    // create the borders for each facet
    for(i = 0; i < grid->width - 1; i++) {
        for(j = 0; j < grid->height - 1; j++) {

            borders[EN_TOP] = -1;

            if(j > 0) {
                borders[EN_TOP] = gridPlanes[i][j - 1][1];
            } else if(grid->wrapHeight) {
                borders[EN_TOP] = gridPlanes[i][grid->height - 2][1];
            }

            noAdjust[EN_TOP] = (borders[EN_TOP] == gridPlanes[i][j][0]);

            if(borders[EN_TOP] == -1 || noAdjust[EN_TOP]) {
                borders[EN_TOP] = CM_EdgePlaneNum(grid, gridPlanes, i, j, 0);
            }

            borders[EN_BOTTOM] = -1;

            if(j < grid->height - 2) {
                borders[EN_BOTTOM] = gridPlanes[i][j + 1][0];
            } else if(grid->wrapHeight) {
                borders[EN_BOTTOM] = gridPlanes[i][0][0];
            }

            noAdjust[EN_BOTTOM] = (borders[EN_BOTTOM] == gridPlanes[i][j][1]);

            if(borders[EN_BOTTOM] == -1 || noAdjust[EN_BOTTOM]) {
                borders[EN_BOTTOM] = CM_EdgePlaneNum(grid, gridPlanes, i, j, 2);
            }

            borders[EN_LEFT] = -1;

            if(i > 0) {
                borders[EN_LEFT] = gridPlanes[i - 1][j][0];
            } else if(grid->wrapWidth) {
                borders[EN_LEFT] = gridPlanes[grid->width - 2][j][0];
            }

            noAdjust[EN_LEFT] = (borders[EN_LEFT] == gridPlanes[i][j][1]);

            if(borders[EN_LEFT] == -1 || noAdjust[EN_LEFT]) {
                borders[EN_LEFT] = CM_EdgePlaneNum(grid, gridPlanes, i, j, 3);
            }

            borders[EN_RIGHT] = -1;

            if(i < grid->width - 2) {
                borders[EN_RIGHT] = gridPlanes[i + 1][j][1];
            } else if(grid->wrapWidth) {
                borders[EN_RIGHT] = gridPlanes[0][j][1];
            }

            noAdjust[EN_RIGHT] = (borders[EN_RIGHT] == gridPlanes[i][j][0]);

            if(borders[EN_RIGHT] == -1 || noAdjust[EN_RIGHT]) {
                borders[EN_RIGHT] = CM_EdgePlaneNum(grid, gridPlanes, i, j, 1);
            }

            if(numFacets == MAX_FACETS) {
                common->Error(ERR_DROP, "MAX_FACETS");
            }

            facet = &facets[numFacets];
            ::memset(facet, 0, sizeof(*facet));

            if(gridPlanes[i][j][0] == gridPlanes[i][j][1]) {
                if(gridPlanes[i][j][0] == -1) {
                    continue; // degenrate
                }

                facet->surfacePlane = gridPlanes[i][j][0];
                facet->numBorders = 4;
                facet->borderPlanes[0] = borders[EN_TOP];
                facet->borderNoAdjust[0] = (bool)noAdjust[EN_TOP];
                facet->borderPlanes[1] = borders[EN_RIGHT];
                facet->borderNoAdjust[1] = (bool)noAdjust[EN_RIGHT];
                facet->borderPlanes[2] = borders[EN_BOTTOM];
                facet->borderNoAdjust[2] = (bool)noAdjust[EN_BOTTOM];
                facet->borderPlanes[3] = borders[EN_LEFT];
                facet->borderNoAdjust[3] = (bool)noAdjust[EN_LEFT];
                CM_SetBorderInward(facet, grid, gridPlanes, i, j, -1);

                if(CM_ValidateFacet(facet)) {
                    CM_AddFacetBevels(facet);
                    numFacets++;
                }
            } else {
                // two seperate triangles
                facet->surfacePlane = gridPlanes[i][j][0];
                facet->numBorders = 3;
                facet->borderPlanes[0] = borders[EN_TOP];
                facet->borderNoAdjust[0] = (bool)noAdjust[EN_TOP];
                facet->borderPlanes[1] = borders[EN_RIGHT];
                facet->borderNoAdjust[1] = (bool)noAdjust[EN_RIGHT];
                facet->borderPlanes[2] = gridPlanes[i][j][1];

                if(facet->borderPlanes[2] == -1) {
                    facet->borderPlanes[2] = borders[EN_BOTTOM];

                    if(facet->borderPlanes[2] == -1) {
                        facet->borderPlanes[2] = CM_EdgePlaneNum(grid, gridPlanes, i, j, 4);
                    }
                }

                CM_SetBorderInward(facet, grid, gridPlanes, i, j, 0);

                if(CM_ValidateFacet(facet)) {
                    CM_AddFacetBevels(facet);
                    numFacets++;
                }

                if(numFacets == MAX_FACETS) {
                    common->Error(ERR_DROP, "MAX_FACETS");
                }

                facet = &facets[numFacets];
                ::memset(facet, 0, sizeof(*facet));

                facet->surfacePlane = gridPlanes[i][j][1];
                facet->numBorders = 3;
                facet->borderPlanes[0] = borders[EN_BOTTOM];
                facet->borderNoAdjust[0] = (bool)noAdjust[EN_BOTTOM];
                facet->borderPlanes[1] = borders[EN_LEFT];
                facet->borderNoAdjust[1] = (bool)noAdjust[EN_LEFT];
                facet->borderPlanes[2] = gridPlanes[i][j][0];

                if(facet->borderPlanes[2] == -1) {
                    facet->borderPlanes[2] = borders[EN_TOP];

                    if(facet->borderPlanes[2] == -1) {
                        facet->borderPlanes[2] = CM_EdgePlaneNum(grid, gridPlanes, i, j, 5);
                    }
                }

                CM_SetBorderInward(facet, grid, gridPlanes, i, j, 1);

                if(CM_ValidateFacet(facet)) {
                    CM_AddFacetBevels(facet);
                    numFacets++;
                }
            }
        }
    }

    // copy the results out
    sc->numPlanes = numPlanes;
    sc->numFacets = numFacets;

    if(numFacets) {
        sc->facets = (cFacet_t *)memorySystem->Alloc(numFacets * sizeof(
                         *sc->facets),
                     h_high);
        ::memcpy(sc->facets, facets, numFacets * sizeof(*sc->facets));
    } else {
        sc->facets = 0;
    }

    sc->planes = (cPlane_t *)memorySystem->Alloc(numPlanes * sizeof(
                     *sc->planes),
                 h_high);
    ::memcpy(sc->planes, planes, numPlanes * sizeof(*sc->planes));

    //memorySystem->Free(facets);
}


/*
===================
CM_GeneratePatchCollide

Creates an internal structure that will be used to perform
collision detection with a patch mesh.

Points is packed as concatenated rows.
===================
*/
cSurfaceCollide_t *CM_GeneratePatchCollide(sint width, sint height,
        vec3_t *points) {
    cSurfaceCollide_t  *sc;
    cGrid_t             grid;
    sint                 i, j;

    if(width <= 2 || height <= 2 || !points) {
        common->Error(ERR_DROP,
                      "CM_GeneratePatchFacets: bad parameters: (%i, %i, %p)",
                      width, height, static_cast<void *>(points));
    }

    if(!(width & 1) || !(height & 1)) {
        common->Error(ERR_DROP,
                      "CM_GeneratePatchFacets: even sizes are invalid for quadratic meshes");
    }

    if(width > MAX_GRID_SIZE || height > MAX_GRID_SIZE) {
        common->Error(ERR_DROP,
                      "CM_GeneratePatchFacets: source is > MAX_GRID_SIZE");
    }

    // build a grid
    grid.width = width;
    grid.height = height;
    grid.wrapWidth = false;
    grid.wrapHeight = false;

    for(i = 0; i < width; i++) {
        for(j = 0; j < height; j++) {
            VectorCopy(points[j * width + i], grid.points[i][j]);
        }
    }

    // subdivide the grid
    CM_SetGridWrapWidth(&grid);
    CM_SubdivideGridColumns(&grid);
    CM_RemoveDegenerateColumns(&grid);

    CM_TransposeGrid(&grid);

    CM_SetGridWrapWidth(&grid);
    CM_SubdivideGridColumns(&grid);
    CM_RemoveDegenerateColumns(&grid);

    // we now have a grid of points exactly on the curve
    // the aproximate surface defined by these points will be
    // collided against
    sc = (cSurfaceCollide_t *)memorySystem->Alloc(sizeof(*sc), h_high);
    ClearBounds(sc->bounds[0], sc->bounds[1]);

    for(i = 0; i < grid.width; i++) {
        for(j = 0; j < grid.height; j++) {
            AddPointToBounds(grid.points[i][j], sc->bounds[0], sc->bounds[1]);
        }
    }

    c_totalPatchBlocks += (grid.width - 1) * (grid.height - 1);

    // generate a bsp tree for the surface
    CM_SurfaceCollideFromGrid(&grid, sc);

    // expand by one unit for epsilon purposes
    sc->bounds[0][0] -= 1;
    sc->bounds[0][1] -= 1;
    sc->bounds[0][2] -= 1;

    sc->bounds[1][0] += 1;
    sc->bounds[1][1] += 1;
    sc->bounds[1][2] += 1;

    return sc;
}
