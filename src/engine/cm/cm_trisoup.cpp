////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2005 Id Software, Inc.
// Copyright(C) 2006 Robert Beckebans <trebor_7@users.sourceforge.net>
// Copyright(C) 2011 - 2022 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of OpenWolf.
//
// OpenWolf is free software; you can redistribute it
// and / or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the License,
// or (at your option) any later version.
//
// OpenWolf is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenWolf; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110 - 1301  USA
//
// -------------------------------------------------------------------------------------
// File name:   cm_trisoup.cpp
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
static bool CM_PlaneFromPoints(vec4_t plane, const vec3_t a,
                               const vec3_t b, const vec3_t c) {
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
PATCH COLLIDE GENERATION
================================================================================
*/

#define USE_HASHING
#define PLANE_HASHES    1024
static cPlane_t *planeHashTable[PLANE_HASHES];

static sint      numPlanes;
static cPlane_t planes[SHADER_MAX_TRIANGLES];

static sint      numFacets;
static cFacet_t facets[MAX_FACETS];

#define NORMAL_EPSILON      0.0001
#define DIST_EPSILON        0.02
#define PLANE_TRI_EPSILON   0.1

/*
==================
CM_PlaneEqual
==================
*/
static sint CM_PlaneEqual(cPlane_t *p, float32 plane[4], sint *flipped) {
    float32           invplane[4];

    if(fabs(p->plane[0] - plane[0]) < NORMAL_EPSILON &&
            fabs(p->plane[1] - plane[1]) < NORMAL_EPSILON &&
            fabs(p->plane[2] - plane[2]) < NORMAL_EPSILON &&
            fabs(p->plane[3] - plane[3]) < DIST_EPSILON) {
        *flipped = false;
        return true;
    }

    VectorNegate(plane, invplane);
    invplane[3] = -plane[3];

    if(fabs(p->plane[0] - invplane[0]) < NORMAL_EPSILON &&
            fabs(p->plane[1] - invplane[1]) < NORMAL_EPSILON &&
            fabs(p->plane[2] - invplane[2]) < NORMAL_EPSILON &&
            fabs(p->plane[3] - invplane[3]) < DIST_EPSILON) {
        *flipped = true;
        return true;
    }

    return false;
}

/*
================
return a hash value for a plane
================
*/
static sint32 CM_GenerateHashValue(vec4_t plane) {
    sint32 hash;

    hash = static_cast<sint>(fabs(plane[3])) / 8;
    hash &= (PLANE_HASHES - 1);

    return hash;
}

/*
================
CM_AddPlaneToHash
================
*/
static void CM_AddPlaneToHash(cPlane_t *p) {
    sint32 hash;

    hash = CM_GenerateHashValue(p->plane);

    p->hashChain = planeHashTable[hash];
    planeHashTable[hash] = p;
}



/*
==================
CM_SnapVector
==================
*/
static void CM_SnapVector(vec3_t normal) {
    sint i;

    for(i = 0; i < 3; i++) {
        if(fabs(normal[i] - 1) < NORMAL_EPSILON) {
            VectorClear(normal);
            normal[i] = 1;
            break;
        }

        if(fabs(normal[i] - -1) < NORMAL_EPSILON) {
            VectorClear(normal);
            normal[i] = -1;
            break;
        }
    }
}

/*
================
CM_CreateNewFloatPlane
================
*/
static sint CM_CreateNewFloatPlane(vec4_t plane) {
#ifndef USE_HASHING

    // add a new plane
    if(numPlanes == SHADER_MAX_TRIANGLES) {
        common->Error(ERR_DROP, "CM_FindPlane: SHADER_MAX_TRIANGLES");
    }

    Vector4Copy(plane, planes[numPlanes].plane);
    planes[numPlanes].signbits = CM_SignbitsForNormal(plane);

    numPlanes++;

    return numPlanes - 1;
#else

    cPlane_t *p;

    // create a new plane
    if(numPlanes == SHADER_MAX_TRIANGLES) {
        common->Error(ERR_DROP, "CM_FindPlane: SHADER_MAX_TRIANGLES");
    }

    p = &planes[numPlanes];
    Vector4Copy(plane, p->plane);

    p->signbits = CM_SignbitsForNormal(plane);

    numPlanes++;

    CM_AddPlaneToHash(p);
    return numPlanes - 1;
#endif
}

/*
==================
CM_FindPlane2
==================
*/
static sint CM_FindPlane2(float32 plane[4], sint *flipped) {
#ifndef USE_HASHING
    sint i;

    // see if the points are close enough to an existing plane
    for(i = 0; i < numPlanes; i++) {
        if(CM_PlaneEqual(&planes[i], plane, flipped)) {
            return i;
        }
    }

    *flipped = false;
    return CM_CreateNewFloatPlane(plane);
#else
    sint             i, hash, h;
    cPlane_t       *p;

    hash = CM_GenerateHashValue(plane);

    // search the border bins as well
    for(i = -1; i <= 1; i++) {
        h = (hash + i) & (PLANE_HASHES - 1);

        for(p = planeHashTable[h]; p; p = p->hashChain) {
            if(CM_PlaneEqual(p, plane, flipped)) {
                return p - planes;
            }
        }
    }

    *flipped = false;
    return CM_CreateNewFloatPlane(plane);
#endif
}

/*
==================
CM_FindPlane
==================
*/
static sint CM_FindPlane(const float32 *p1, const float32 *p2,
                         const float32 *p3) {
    float32           plane[4];
    sint             i;

    if(!CM_PlaneFromPoints(plane, p1, p2, p3)) {
        return -1;
    }

    // use variable i as dummy
    return CM_FindPlane2(plane, &i);
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
CM_GenerateBoundaryForPoints
==================
*/
static sint CM_GenerateBoundaryForPoints(const vec4_t triPlane,
        const vec3_t p1, const vec3_t p2) {
    vec3_t          up;

    VectorMA(p1, 4, triPlane, up);

    return CM_FindPlane(p1, p2, up);
}

/*
===================
CM_SetBorderInward
===================
*/
static void CM_SetBorderInward(cFacet_t *facet, cTriangleSoup_t *triSoup,
                               sint i, sint which) {
    sint             k, l, numPoints;
    float32          *points[4];

    switch(which) {
        case 0:
            points[0] = triSoup->points[i][0];
            points[1] = triSoup->points[i][1];
            points[2] = triSoup->points[i][2];
            numPoints = 3;
            break;

        case 1:
            points[0] = triSoup->points[i][2];
            points[1] = triSoup->points[i][1];
            points[2] = triSoup->points[i][0];
            numPoints = 3;
            break;

        default:
            common->Error(ERR_FATAL, "CM_SetBorderInward: bad parameter %i", which);
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
    sint             i, j, k, l, axis, dir, order, flipped;
    float32           plane[4], d, newplane[4];
    winding_t      *w, *w2;
    vec3_t          mins, maxs, vec, vec2;

    Vector4Copy(planes[facet->surfacePlane].plane, plane);

    w = BaseWindingForPlane(plane, plane[3]);

    for(j = 0; j < facet->numBorders && w; j++) {
        if(facet->borderPlanes[j] == facet->surfacePlane) {
            continue;
        }

        Vector4Copy(planes[facet->borderPlanes[j]].plane, plane);

        if(!facet->borderInward[j]) {
            VectorInverse(plane);
            plane[3] = -plane[3];
        }

        ChopWindingInPlace(&w, plane, plane[3], 0.1f);
    }

    if(!w) {
        return;
    }

    WindingBounds(w, mins, maxs);

    //
    // add the axial planes
    //
    order = 0;

    for(axis = 0; axis < 3; axis++) {
        for(dir = -1; dir <= 1; dir += 2, order++) {
            VectorClear(plane);
            plane[axis] = dir;

            if(dir == 1) {
                plane[3] = maxs[axis];
            } else {
                plane[3] = -mins[axis];
            }

            // if it's the surface plane
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
                if(facet->numBorders > MAX_FACET_BEVELS) {
                    common->Printf("ERROR: too many bevels\n");
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
        VectorSubtract(w->p[j], w->p[k], vec);

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
                for(l = 0; l < w->numpoints; l++) {
                    d = DotProduct(w->p[l], plane) - plane[3];

                    if(d > 0.1) {
                        break;  // point in front
                    }
                }

                if(l < w->numpoints) {
                    continue;
                }

                // if it's the surface plane
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
                    if(facet->numBorders > MAX_FACET_BEVELS) {
                        common->Printf("ERROR: too many bevels\n");
                    }

                    facet->borderPlanes[facet->numBorders] = CM_FindPlane2(plane, &flipped);

                    for(k = 0; k < facet->numBorders; k++) {
                        if(facet->borderPlanes[facet->numBorders] == facet->borderPlanes[k]) {
                            common->Printf("WARNING: bevel plane already used\n");
                        }
                    }

                    facet->borderNoAdjust[facet->numBorders] = false;
                    facet->borderInward[facet->numBorders] = flipped;
                    //
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
                            common->Printf("WARNING: Invalid bevel %f %f %f %f\n", w->p[0][0],
                                           w->p[0][1],
                                           w->p[0][2], w->p[0][3]);
                        }

                        continue;
                    } else {
                        FreeWinding(w2);
                    }

                    //
                    facet->numBorders++;
                    //already got a bevel
                    //                  break;
                }
            }
        }
    }

    FreeWinding(w);

    //add opposite plane
    if(facet->numBorders >= 4 + 6 + 16) {
        common->Printf("ERROR: too many bevels\n");
        return;
    }

    facet->borderPlanes[facet->numBorders] = facet->surfacePlane;
    facet->borderNoAdjust[facet->numBorders] = false;
    facet->borderInward[facet->numBorders] = true;
    facet->numBorders++;
}


/*
=====================
CM_GenerateFacetFor3Points
=====================
*/
bool CM_GenerateFacetFor3Points(cFacet_t *facet, const vec3_t p1,
                                const vec3_t p2, const vec3_t p3) {
    vec4_t plane;

    // if we can't generate a valid plane for the points, ignore the facet
    //if(!PlaneFromPoints(f->surface, a, b, c, true))
    if(facet->surfacePlane == -1) {
        facet->numBorders = 0;
        return false;
    }

    Vector4Copy(planes[facet->surfacePlane].plane, plane);

    facet->numBorders = 3;

    facet->borderNoAdjust[0] = false;
    facet->borderNoAdjust[1] = false;
    facet->borderNoAdjust[2] = false;

    facet->borderPlanes[0] = CM_GenerateBoundaryForPoints(plane, p1, p2);
    facet->borderPlanes[1] = CM_GenerateBoundaryForPoints(plane, p2, p3);
    facet->borderPlanes[2] = CM_GenerateBoundaryForPoints(plane, p3, p1);

    return true;
}

/*
=====================
CM_GenerateFacetFor4Points
=====================
*/
#define PLANAR_EPSILON  0.1
bool CM_GenerateFacetFor4Points(cFacet_t *facet, const vec3_t p1,
                                const vec3_t p2, const vec3_t p3, const vec3_t p4) {
    float32           dist;
    vec4_t          plane;

    // if we can't generate a valid plane for the points, ignore the facet
    if(facet->surfacePlane == -1) {
        facet->numBorders = 0;
        return false;
    }

    Vector4Copy(planes[facet->surfacePlane].plane, plane);

    // if the fourth point is also on the plane, we can make a quad facet
    dist = DotProduct(p4, plane) - plane[3];

    if(fabs(dist) > PLANAR_EPSILON) {
        facet->numBorders = 0;
        return false;
    }

    facet->numBorders = 4;

    facet->borderNoAdjust[0] = false;
    facet->borderNoAdjust[1] = false;
    facet->borderNoAdjust[2] = false;
    facet->borderNoAdjust[3] = false;

    facet->borderPlanes[0] = CM_GenerateBoundaryForPoints(plane, p1, p2);
    facet->borderPlanes[1] = CM_GenerateBoundaryForPoints(plane, p2, p3);
    facet->borderPlanes[2] = CM_GenerateBoundaryForPoints(plane, p3, p4);
    facet->borderPlanes[3] = CM_GenerateBoundaryForPoints(plane, p4, p1);

    return true;
}

/*
==================
CM_SurfaceCollideFromTriangleSoup
==================
*/
static void CM_SurfaceCollideFromTriangleSoup(cTriangleSoup_t *triSoup,
        cSurfaceCollide_t *sc) {
    sint             i, i1, i2, i3;
    float32          *p1, *p2, *p3;

    cFacet_t       *facet;

    numPlanes = 0;
    numFacets = 0;

#ifdef USE_HASHING
    // initialize hash table
    ::memset(planeHashTable, 0, sizeof(planeHashTable));
#endif

    // find the planes for each triangle of the grid
    for(i = 0; i < triSoup->numTriangles; i++) {
        p1 = triSoup->points[i][0];
        p2 = triSoup->points[i][1];
        p3 = triSoup->points[i][2];

        triSoup->trianglePlanes[i] = CM_FindPlane(p1, p2, p3);

        //common->Printf("trianglePlane[%i] = %i\n", i, trianglePlanes[i]);
    }

    // create the borders for each triangle
    for(i = 0; i < triSoup->numTriangles; i++) {
        facet = &facets[numFacets];
        ::memset(facet, 0, sizeof(*facet));

        i1 = triSoup->indexes[i * 3 + 0];
        i2 = triSoup->indexes[i * 3 + 1];
        i3 = triSoup->indexes[i * 3 + 2];

        p1 = triSoup->points[i][0];
        p2 = triSoup->points[i][1];
        p3 = triSoup->points[i][2];

        facet->surfacePlane =
            triSoup->trianglePlanes[i];   //CM_FindPlane(p1, p2, p3);

        // try and make a quad out of two triangles
#if 0

        if(i != triSoup->numTriangles - 1) {
            i4 = triSoup->indexes[i * 3 + 3];
            i5 = triSoup->indexes[i * 3 + 4];
            i6 = triSoup->indexes[i * 3 + 5];

            if(i4 == i3 && i5 == i2) {
                p4 = triSoup->points[i][5]; // vertex at i6

                if(CM_GenerateFacetFor4Points(facet, p1, p2, p4,
                                              p3)) {   //test->facets[count], v1, v2, v4, v3))
                    CM_SetBorderInward(facet, triSoup, i, 0);

                    if(CM_ValidateFacet(facet)) {
                        CM_AddFacetBevels(facet);
                        numFacets++;

                        i++;    // skip next tri
                        continue;
                    }
                }
            }
        }

#endif

        if(CM_GenerateFacetFor3Points(facet, p1, p2, p3)) {
            CM_SetBorderInward(facet, triSoup, i, 0);

            if(CM_ValidateFacet(facet)) {
                CM_AddFacetBevels(facet);
                numFacets++;
            }
        }
    }

    // copy the results out
    sc->numPlanes = numPlanes;
    sc->planes = (cPlane_t *)memorySystem->Alloc(numPlanes * sizeof(
                     *sc->planes),
                 h_high);
    ::memcpy(sc->planes, planes, numPlanes * sizeof(*sc->planes));

    sc->numFacets = numFacets;
    sc->facets = (cFacet_t *)memorySystem->Alloc(numFacets * sizeof(
                     *sc->facets),
                 h_high);
    ::memcpy(sc->facets, facets, numFacets * sizeof(*sc->facets));
}


/*
===================
CM_GenerateTriangleSoupCollide

Creates an internal structure that will be used to perform
collision detection with a triangle soup mesh.

Points is packed as concatenated rows.
===================
*/

cSurfaceCollide_t *CM_GenerateTriangleSoupCollide(sint numVertexes,
        vec3_t *vertexes, sint numIndexes, sint *indexes) {
    cSurfaceCollide_t *sc;
    static             cTriangleSoup_t triSoup;
    sint                i, j;

    if(numVertexes <= 2 || !vertexes || numIndexes <= 2 || !indexes) {
        common->Error(ERR_DROP,
                      "CM_GenerateTriangleSoupCollide: bad parameters: (%i, %p, %i, %p)",
                      numVertexes, vertexes, numIndexes, indexes);
    }

    if(numIndexes > SHADER_MAX_INDEXES) {
        common->Error(ERR_DROP,
                      "CM_GenerateTriangleSoupCollide: source is > SHADER_MAX_TRIANGLES");
    }

    // build a triangle soup
    triSoup.numTriangles = numIndexes / 3;

    for(i = 0; i < triSoup.numTriangles; i++) {
        for(j = 0; j < 3; j++) {
            triSoup.indexes[i * 3 + j] = indexes[i * 3 + j];

            VectorCopy(vertexes[indexes[i * 3 + j]], triSoup.points[i][j]);
        }
    }

    sc = (cSurfaceCollide_t *)memorySystem->Alloc(sizeof(*sc), h_high);
    ClearBounds(sc->bounds[0], sc->bounds[1]);

    for(i = 0; i < triSoup.numTriangles; i++) {
        for(j = 0; j < 3; j++) {
            AddPointToBounds(triSoup.points[i][j], sc->bounds[0], sc->bounds[1]);
        }
    }

    // generate a bsp tree for the surface
    CM_SurfaceCollideFromTriangleSoup(&triSoup, sc);

    // expand by one unit for epsilon purposes
    sc->bounds[0][0] -= 1;
    sc->bounds[0][1] -= 1;
    sc->bounds[0][2] -= 1;

    sc->bounds[1][0] += 1;
    sc->bounds[1][1] += 1;
    sc->bounds[1][2] += 1;

    if(developer->integer) {
        common->Printf("CM_GenerateTriangleSoupCollide: %i planes %i facets\n",
                       sc->numPlanes, sc->numFacets);
    }

    return sc;
}
