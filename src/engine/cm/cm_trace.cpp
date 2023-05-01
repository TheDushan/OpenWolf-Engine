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
// File name:   cm_trace.cpp
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

// always use bbox vs. bbox collision and never capsule vs. bbox or vice versa
//#define ALWAYS_BBOX_VS_BBOX
// always use capsule vs. capsule collision and never capsule vs. bbox or vice versa
//#define ALWAYS_CAPSULE_VS_CAPSULE

//#define CAPSULE_DEBUG

/*
===============================================================================

BASIC MATH

===============================================================================
*/

/*
================
RotatePoint
================
*/
void RotatePoint(vec3_t point, /*const */ vec3_t matrix[3]) { // FIXME
    vec3_t tvec;

    VectorCopy(point, tvec);
    point[0] = DotProduct(matrix[0], tvec);
    point[1] = DotProduct(matrix[1], tvec);
    point[2] = DotProduct(matrix[2], tvec);
}

/*
================
TransposeMatrix
================
*/
void TransposeMatrix(/*const */ vec3_t matrix[3],
                                vec3_t transpose[3]) {  // FIXME
    sint i, j;

    for(i = 0; i < 3; i++) {
        for(j = 0; j < 3; j++) {
            transpose[i][j] = matrix[j][i];
        }
    }
}

/*
================
CreateRotationMatrix
================
*/
void CreateRotationMatrix(const vec3_t angles, vec3_t matrix[3]) {
    AngleVectors(angles, matrix[0], matrix[1], matrix[2]);
    VectorInverse(matrix[1]);
}

/*
================
CM_ProjectPointOntoVector
================
*/
void CM_ProjectPointOntoVector(vec3_t point, vec3_t vStart, vec3_t vDir,
                               vec3_t vProj) {
    vec3_t pVec;

    VectorSubtract(point, vStart, pVec);
    // project onto the directional vector for this segment
    VectorMA(vStart, DotProduct(pVec, vDir), vDir, vProj);
}

/*
================
CM_DistanceFromLineSquared
================
*/
float32 CM_DistanceFromLineSquared(vec3_t p, vec3_t lp1, vec3_t lp2,
                                   vec3_t dir) {
    vec3_t          proj, t;
    sint             j;

    CM_ProjectPointOntoVector(p, lp1, dir, proj);

    for(j = 0; j < 3; j++) {
        if((proj[j] > lp1[j] && proj[j] > lp2[j]) || (proj[j] < lp1[j] &&
                proj[j] < lp2[j])) {
            break;
        }
    }

    if(j < 3) {
        if(fabs(proj[j] - lp1[j]) < fabs(proj[j] - lp2[j])) {
            VectorSubtract(p, lp1, t);
        } else {
            VectorSubtract(p, lp2, t);
        }

        return VectorLengthSquared(t);
    }

    VectorSubtract(p, proj, t);
    return VectorLengthSquared(t);
}

/*
================
CM_VectorDistanceSquared
================
*/
float32 CM_VectorDistanceSquared(vec3_t p1, vec3_t p2) {
    vec3_t dir;

    VectorSubtract(p2, p1, dir);
    return VectorLengthSquared(dir);
}

/*
================
SquareRootFloat
================
*/
float32 SquareRootFloat(float32 number) {
    union {
        float32           f;
        sint             i;
    } t;
    float32           x, y;
    const float32     f = 1.5F;

    x = number * 0.5F;
    t.f = number;
    t.i = 0x5f3759df - (t.i >> 1);
    y = t.f;
    y = y * (f - (x * y * y));
    y = y * (f - (x * y * y));
    return number * y;
}


/*
===============================================================================

POSITION TESTING

===============================================================================
*/

/*
================
CM_TestBoxInBrush
================
*/
static void CM_TestBoxInBrush(traceWork_t *tw, cbrush_t *brush) {
    sint             i;
    cplane_t       *plane;
    float64           dist, d1, t;
    cbrushside_t   *side;
    vec3_t          startp;

    if(!brush->numsides) {
        return;
    }

    // special test for axial
    if(tw->bounds[0][0] > brush->bounds[1][0] ||
            tw->bounds[0][1] > brush->bounds[1][1] ||
            tw->bounds[0][2] > brush->bounds[1][2] ||
            tw->bounds[1][0] < brush->bounds[0][0] ||
            tw->bounds[1][1] < brush->bounds[0][1] ||
            tw->bounds[1][2] < brush->bounds[0][2]) {
        return;
    }

    if(tw->type == TT_CAPSULE) {
        // the first six planes are the axial planes, so we only
        // need to test the remainder
        for(i = 6; i < brush->numsides; i++) {
            side = brush->sides + i;
            plane = side->plane;

            // adjust the plane distance apropriately for radius
            dist = plane->dist + tw->sphere.radius;
            // find the closest point on the capsule to the plane
            t = DotProduct(plane->normal, tw->sphere.offset);

            if(t > 0) {
                VectorSubtract(tw->start, tw->sphere.offset, startp);
            } else {
                VectorAdd(tw->start, tw->sphere.offset, startp);
            }

            d1 = DotProduct(startp, plane->normal) - dist;

            // if completely in front of face, no intersection
            if(d1 > 0) {
                return;
            }
        }
    } else {
        // the first six planes are the axial planes, so we only
        // need to test the remainder
        for(i = 6; i < brush->numsides; i++) {
            side = brush->sides + i;
            plane = side->plane;

            // adjust the plane distance apropriately for mins/maxs
            dist = plane->dist - DotProduct(tw->offsets[plane->signbits],
                                            plane->normal);

            d1 = DotProduct(tw->start, plane->normal) - dist;

            // if completely in front of face, no intersection
            if(d1 > 0) {
                return;
            }
        }
    }

    // inside this brush
    tw->trace.startsolid = tw->trace.allsolid = true;
    tw->trace.fraction = 0;
    tw->trace.contents = brush->contents;
}


/*
====================
CM_PositionTestInSurfaceCollide
====================
*/
static bool CM_PositionTestInSurfaceCollide(traceWork_t *tw,
        const cSurfaceCollide_t *sc) {
    sint             i, j;
    float32           offset, t, plane[4];
    cPlane_t       *planes;
    cFacet_t       *facet;
    vec3_t          startp;

    if(tw->isPoint) {
        return false;
    }

    //
    facet = sc->facets;

    for(i = 0; i < sc->numFacets; i++, facet++) {
        planes = &sc->planes[facet->surfacePlane];
        VectorCopy(planes->plane, plane);
        plane[3] = planes->plane[3];

        if(tw->type == TT_CAPSULE) {
            // adjust the plane distance apropriately for radius
            plane[3] += tw->sphere.radius;

            // find the closest point on the capsule to the plane
            t = DotProduct(plane, tw->sphere.offset);

            if(t > 0) {
                VectorSubtract(tw->start, tw->sphere.offset, startp);
            } else {
                VectorAdd(tw->start, tw->sphere.offset, startp);
            }
        } else {
            offset = DotProduct(tw->offsets[planes->signbits], plane);
            plane[3] -= offset;
            VectorCopy(tw->start, startp);
        }

        if(DotProduct(plane, startp) - plane[3] > 0.0f) {
            continue;
        }

        for(j = 0; j < facet->numBorders; j++) {
            planes = &sc->planes[facet->borderPlanes[j]];

            if(facet->borderInward[j]) {
                VectorNegate(planes->plane, plane);
                plane[3] = -planes->plane[3];
            } else {
                VectorCopy(planes->plane, plane);
                plane[3] = planes->plane[3];
            }

            if(tw->type == TT_CAPSULE) {
                // adjust the plane distance apropriately for radius
                plane[3] += tw->sphere.radius;

                // find the closest point on the capsule to the plane
                t = DotProduct(plane, tw->sphere.offset);

                if(t > 0.0f) {
                    VectorSubtract(tw->start, tw->sphere.offset, startp);
                } else {
                    VectorAdd(tw->start, tw->sphere.offset, startp);
                }
            } else {
                // NOTE: this works even though the plane might be flipped because the bbox is centered
                offset = DotProduct(tw->offsets[planes->signbits], plane);
                plane[3] += fabs(offset);
                VectorCopy(tw->start, startp);
            }

            if(DotProduct(plane, startp) - plane[3] > 0.0f) {
                break;
            }
        }

        if(j < facet->numBorders) {
            continue;
        }

        // inside this patch facet
        return true;
    }

    return false;
}



/*
================
CM_TestInLeaf
================
*/
void CM_TestInLeaf(traceWork_t *tw, cLeaf_t *leaf) {
    sint             k, brushnum;
    cbrush_t       *b;
    cSurface_t     *surface;

    // test box position against all brushes in the leaf
    for(k = 0; k < leaf->numLeafBrushes; k++) {
        brushnum = cm.leafbrushes[leaf->firstLeafBrush + k];
        b = &cm.brushes[brushnum];

        if(b->checkcount == cm.checkcount) {
            continue; // already checked this brush in another leaf
        }

        b->checkcount = cm.checkcount;

        if(!(b->contents & tw->contents)) {
            continue;
        }

        CM_TestBoxInBrush(tw, b);

        if(tw->trace.allsolid) {
            return;
        }
    }

    // test against all surfaces
    for(k = 0; k < leaf->numLeafSurfaces; k++) {
        surface = cm.surfaces[cm.leafsurfaces[leaf->firstLeafSurface + k]];

        if(!surface) {
            continue;
        }

        if(surface->checkcount == cm.checkcount) {
            continue; // already checked this surface in another leaf
        }

        surface->checkcount = cm.checkcount;

        if(!(surface->contents & tw->contents)) {
            continue;
        }

#ifndef BSPC

        if(!cm_noCurves->integer) {
            if(surface->type == MST_PATCH && surface->sc &&
                    CM_PositionTestInSurfaceCollide(tw, surface->sc)) {
                tw->trace.startsolid = tw->trace.allsolid = true;
                tw->trace.fraction = 0;
                tw->trace.contents = surface->contents;
                return;
            }
        }

        if(cm.perPolyCollision || cm_forceTriangles->integer) {
            if(surface->type == MST_TRIANGLE_SOUP && surface->sc &&
                    CM_PositionTestInSurfaceCollide(tw, surface->sc)) {
                tw->trace.startsolid = tw->trace.allsolid = true;
                tw->trace.fraction = 0;
                tw->trace.contents = surface->contents;
                return;
            }
        }

#endif
    }
}


/*
==================
CM_TestCapsuleInCapsule

capsule inside capsule check
==================
*/
void CM_TestCapsuleInCapsule(traceWork_t *tw, clipHandle_t model) {
    sint             i;
    vec3_t          mins, maxs, top, bottom, p1, p2, tmp, offset,
                    symetricSize[2];
    float32           radius, halfwidth, halfheight, offs, r;

    collisionModelManagerLocal.ModelBounds(model, mins, maxs);

    VectorAdd(tw->start, tw->sphere.offset, top);
    VectorSubtract(tw->start, tw->sphere.offset, bottom);

    for(i = 0; i < 3; i++) {
        offset[i] = (mins[i] + maxs[i]) * 0.5f;
        symetricSize[0][i] = mins[i] - offset[i];
        symetricSize[1][i] = maxs[i] - offset[i];
    }

    halfwidth = symetricSize[1][0];
    halfheight = symetricSize[1][2];
    radius = (halfwidth > halfheight) ? halfheight : halfwidth;
    offs = halfheight - radius;

    r = Square(tw->sphere.radius + radius);
    // check if any of the spheres overlap
    VectorCopy(offset, p1);
    p1[2] += offs;
    VectorSubtract(p1, top, tmp);

    if(VectorLengthSquared(tmp) < r) {
        tw->trace.startsolid = tw->trace.allsolid = true;
        tw->trace.fraction = 0;
    }

    VectorSubtract(p1, bottom, tmp);

    if(VectorLengthSquared(tmp) < r) {
        tw->trace.startsolid = tw->trace.allsolid = true;
        tw->trace.fraction = 0;
    }

    VectorCopy(offset, p2);
    p2[2] -= offs;
    VectorSubtract(p2, top, tmp);

    if(VectorLengthSquared(tmp) < r) {
        tw->trace.startsolid = tw->trace.allsolid = true;
        tw->trace.fraction = 0;
    }

    VectorSubtract(p2, bottom, tmp);

    if(VectorLengthSquared(tmp) < r) {
        tw->trace.startsolid = tw->trace.allsolid = true;
        tw->trace.fraction = 0;
    }

    // if between cylinder up and lower bounds
    if((top[2] >= p1[2] && top[2] <= p2[2]) || (bottom[2] >= p1[2] &&
            bottom[2] <= p2[2])) {
        // 2d coordinates
        top[2] = p1[2] = 0;
        // if the cylinders overlap
        VectorSubtract(top, p1, tmp);

        if(VectorLengthSquared(tmp) < r) {
            tw->trace.startsolid = tw->trace.allsolid = true;
            tw->trace.fraction = 0;
        }
    }
}

/*
==================
CM_TestBoundingBoxInCapsule

bounding box inside capsule check
==================
*/
void CM_TestBoundingBoxInCapsule(traceWork_t *tw, clipHandle_t model) {
    vec3_t          mins, maxs, offset, size[2];
    clipHandle_t    h;
    cmodel_t       *cmod;
    sint             i;

    // mins maxs of the capsule
    collisionModelManagerLocal.ModelBounds(model, mins, maxs);

    // offset for capsule center
    for(i = 0; i < 3; i++) {
        offset[i] = (mins[i] + maxs[i]) * 0.5f;
        size[0][i] = mins[i] - offset[i];
        size[1][i] = maxs[i] - offset[i];
        tw->start[i] -= offset[i];
        tw->end[i] -= offset[i];
    }

    // replace the bounding box with the capsule
    tw->type = TT_CAPSULE;
    tw->sphere.radius = (size[1][0] > size[1][2]) ? size[1][2] : size[1][0];
    tw->sphere.halfheight = size[1][2];
    VectorSet(tw->sphere.offset, 0, 0, size[1][2] - tw->sphere.radius);

    // replace the capsule with the bounding box
    h = collisionModelManagerLocal.TempBoxModel(tw->size[0], tw->size[1],
            false);
    // calculate collision
    cmod = CM_ClipHandleToModel(h);
    CM_TestInLeaf(tw, &cmod->leaf);
}

/*
==================
CM_PositionTest
==================
*/
#define MAX_POSITION_LEAFS  1024
void CM_PositionTest(traceWork_t *tw) {
    sint             leafs[MAX_POSITION_LEAFS], i;
    leafList_t      ll;

    // identify the leafs we are touching
    VectorAdd(tw->start, tw->size[0], ll.bounds[0]);
    VectorAdd(tw->start, tw->size[1], ll.bounds[1]);

    for(i = 0; i < 3; i++) {
        ll.bounds[0][i] -= 1;
        ll.bounds[1][i] += 1;
    }

    ll.count = 0;
    ll.maxcount = MAX_POSITION_LEAFS;
    ll.list = leafs;
    ll.storeLeafs = CM_StoreLeafs;
    ll.lastLeaf = 0;
    ll.overflowed = false;

    cm.checkcount++;

    CM_BoxLeafnums_r(&ll, 0);

    cm.checkcount++;

    // test the contents of the leafs
    for(i = 0; i < ll.count; i++) {
        CM_TestInLeaf(tw, &cm.leafs[leafs[i]]);

        if(tw->trace.allsolid) {
            break;
        }
    }
}

/*
===============================================================================

TRACING

===============================================================================
*/


/*
====================
CM_TracePointThroughSurfaceCollide

  special case for point traces because the surface collide "brushes" have no volume
====================
*/
void CM_TracePointThroughSurfaceCollide(traceWork_t *tw,
                                        const cSurfaceCollide_t *sc) {
    static bool     frontFacing[SHADER_MAX_TRIANGLES];
    static float32    intersection[SHADER_MAX_TRIANGLES];
    float32           intersect, offset, d1, d2;
    const cPlane_t *planes;
    const cFacet_t *facet;
    sint             i, j, k;
#ifndef BSPC
    static convar_t  *cv;
#endif

    if(!tw->isPoint) {
        return;
    }

    // determine the trace's relationship to all planes
    planes = sc->planes;

    for(i = 0; i < sc->numPlanes; i++, planes++) {
        offset = DotProduct(tw->offsets[planes->signbits], planes->plane);
        d1 = DotProduct(tw->start, planes->plane) - planes->plane[3] + offset;
        d2 = DotProduct(tw->end, planes->plane) - planes->plane[3] + offset;

        if(d1 <= 0) {
            frontFacing[i] = false;
        } else {
            frontFacing[i] = true;
        }

        if(d1 == d2) {
            intersection[i] = 99999;
        } else {
            intersection[i] = d1 / (d1 - d2);

            if(intersection[i] <= 0) {
                intersection[i] = 99999;
            }
        }
    }

    // see if any of the surface planes are intersected
    for(i = 0, facet = sc->facets; i < sc->numFacets; i++, facet++) {
        if(!frontFacing[facet->surfacePlane]) {
            continue;
        }

        intersect = intersection[facet->surfacePlane];

        if(intersect < 0) {
            continue; // surface is behind the starting point
        }

        if(intersect > tw->trace.fraction) {
            continue; // already hit something closer
        }

        for(j = 0; j < facet->numBorders; j++) {
            k = facet->borderPlanes[j];

            if(frontFacing[k] ^ facet->borderInward[j]) {
                if(intersection[k] > intersect) {
                    break;
                }
            } else {
                if(intersection[k] < intersect) {
                    break;
                }
            }
        }

        if(j == facet->numBorders) {
            // we hit this facet
#ifndef BSPC
            if(!cv) {
                cv = cvarSystem->Get("r_debugSurfaceUpdate", "1", 0,
                                     "When r_debugSurface is 1, this enables changing the patch mesh surface bounding box according to location.");
            }

            if(cv->integer) {
                debugSurfaceCollide = sc;
                debugFacet = facet;
            }

#endif

            planes = &sc->planes[facet->surfacePlane];

            // calculate intersection with a slight pushoff
            offset = DotProduct(tw->offsets[planes->signbits], planes->plane);
            d1 = DotProduct(tw->start, planes->plane) - planes->plane[3] + offset;
            d2 = DotProduct(tw->end, planes->plane) - planes->plane[3] + offset;
            tw->trace.fraction = (d1 - SURFACE_CLIP_EPSILON) / (d1 - d2);

            if(tw->trace.fraction < 0) {
                tw->trace.fraction = 0;
            }

            VectorCopy(planes->plane, tw->trace.plane.normal);
            tw->trace.plane.dist = planes->plane[3];
        }
    }
}

/*
====================
CM_CheckFacetPlane
====================
*/
sint CM_CheckFacetPlane(float32 *plane, vec3_t start, vec3_t end,
                        float32 *enterFrac, float32 *leaveFrac, sint *hit) {
    float32 d1, d2, f;

    *hit = false;

    d1 = DotProduct(start, plane) - plane[3];
    d2 = DotProduct(end, plane) - plane[3];

    // if completely in front of face, no intersection with the entire facet
    if(d1 > 0 && (d2 >= SURFACE_CLIP_EPSILON || d2 >= d1)) {
        return false;
    }

    // if it doesn't cross the plane, the plane isn't relevant
    if(d1 <= 0 && d2 <= 0) {
        return true;
    }

    // crosses face
    if(d1 > d2) {
        // enter
        f = (d1 - SURFACE_CLIP_EPSILON) / (d1 - d2);

        if(f < 0) {
            f = 0;
        }

        //always favor previous plane hits and thus also the surface plane hit
        if(f > *enterFrac) {
            *enterFrac = f;
            *hit = true;
        }
    } else {
        // leave
        f = (d1 + SURFACE_CLIP_EPSILON) / (d1 - d2);

        if(f > 1) {
            f = 1;
        }

        if(f < *leaveFrac) {
            *leaveFrac = f;
        }
    }

    return true;
}

/*
====================
CM_TraceThroughSurfaceCollide
====================
*/
void CM_TraceThroughSurfaceCollide(traceWork_t *tw,
                                   const cSurfaceCollide_t *sc) {
    sint             i, j, hit, hitnum;
    float32           offset, enterFrac, leaveFrac, t, plane[4] = { 0, 0, 0, 0 },
            bestplane[4] = { 0, 0, 0, 0 };
    cPlane_t       *planes;
    cFacet_t       *facet;
    vec3_t          startp, endp;
    static convar_t  *cv;

    if(!CM_BoundsIntersect(tw->bounds[0], tw->bounds[1], sc->bounds[0],
                           sc->bounds[1])) {
        return;
    }

    if(tw->isPoint) {
        CM_TracePointThroughSurfaceCollide(tw, sc);
        return;
    }

    for(i = 0, facet = sc->facets; i < sc->numFacets; i++, facet++) {
        enterFrac = -1.0;
        leaveFrac = 1.0;
        hitnum = -1;

        planes = &sc->planes[facet->surfacePlane];
        VectorCopy(planes->plane, plane);
        plane[3] = planes->plane[3];

        if(tw->type == TT_CAPSULE) {
            // adjust the plane distance appropriately for radius
            plane[3] += tw->sphere.radius;

            // find the closest point on the capsule to the plane
            t = DotProduct(plane, tw->sphere.offset);

            if(t > 0.0f) {
                VectorSubtract(tw->start, tw->sphere.offset, startp);
                VectorSubtract(tw->end, tw->sphere.offset, endp);
            } else {
                VectorAdd(tw->start, tw->sphere.offset, startp);
                VectorAdd(tw->end, tw->sphere.offset, endp);
            }
        } else {
            offset = DotProduct(tw->offsets[planes->signbits], plane);
            plane[3] -= offset;
            VectorCopy(tw->start, startp);
            VectorCopy(tw->end, endp);
        }

        if(!CM_CheckFacetPlane(plane, startp, endp, &enterFrac, &leaveFrac,
                               &hit)) {
            continue;
        }

        if(hit) {
            Vector4Copy(plane, bestplane);
        }

        for(j = 0; j < facet->numBorders; j++) {
            planes = &sc->planes[facet->borderPlanes[j]];

            if(facet->borderInward[j]) {
                VectorNegate(planes->plane, plane);
                plane[3] = -planes->plane[3];
            } else {
                VectorCopy(planes->plane, plane);
                plane[3] = planes->plane[3];
            }

            if(tw->type == TT_CAPSULE) {
                // adjust the plane distance apropriately for radius
                plane[3] += tw->sphere.radius;

                // find the closest point on the capsule to the plane
                t = DotProduct(plane, tw->sphere.offset);

                if(t > 0.0f) {
                    VectorSubtract(tw->start, tw->sphere.offset, startp);
                    VectorSubtract(tw->end, tw->sphere.offset, endp);
                } else {
                    VectorAdd(tw->start, tw->sphere.offset, startp);
                    VectorAdd(tw->end, tw->sphere.offset, endp);
                }
            } else {
                // NOTE: this works even though the plane might be flipped because the bbox is centered
                offset = DotProduct(tw->offsets[planes->signbits], plane);
                plane[3] += fabs(offset);
                VectorCopy(tw->start, startp);
                VectorCopy(tw->end, endp);
            }

            if(!CM_CheckFacetPlane(plane, startp, endp, &enterFrac, &leaveFrac,
                                   &hit)) {
                break;
            }

            if(hit) {
                hitnum = j;
                Vector4Copy(plane, bestplane);
            }
        }

        if(j < facet->numBorders) {
            continue;
        }

        //never clip against the back side
        if(hitnum == facet->numBorders - 1) {
            continue;
        }

        if(enterFrac < leaveFrac && enterFrac >= 0) {
            if(enterFrac < tw->trace.fraction) {
                if(enterFrac < 0) {
                    enterFrac = 0;
                }

#ifndef BSPC

                if(!cv) {
                    cv = cvarSystem->Get("r_debugSurfaceUpdate", "1", 0,
                                         "When r_debugSurface is 1, this enables changing the patch mesh surface bounding box according to location.");
                }

                if(cv && cv->integer) {
                    debugSurfaceCollide = sc;
                    debugFacet = facet;
                }

#endif

                tw->trace.fraction = enterFrac;
                VectorCopy(bestplane, tw->trace.plane.normal);
                tw->trace.plane.dist = bestplane[3];
            }
        }
    }
}

/*
================
CM_TraceThroughSurface
================
*/
void CM_TraceThroughSurface(traceWork_t *tw, cSurface_t *surface) {
    float32 oldFrac;

    oldFrac = tw->trace.fraction;

#ifndef BSPC

    if(!cm_noCurves->integer && surface->type == MST_PATCH && surface->sc) {
        CM_TraceThroughSurfaceCollide(tw, surface->sc);
        c_patch_traces++;
    }

    if((cm.perPolyCollision || cm_forceTriangles->integer) &&
            surface->type == MST_TRIANGLE_SOUP && surface->sc) {
        CM_TraceThroughSurfaceCollide(tw, surface->sc);
        c_trisoup_traces++;
    }

#endif

    if(tw->trace.fraction < oldFrac) {
        tw->trace.surfaceFlags = surface->surfaceFlags;
        tw->trace.contents = surface->contents;
    }
}

/*
================
CM_CalcTraceBounds
================
*/
static void CM_CalcTraceBounds(traceWork_t *tw, bool expand) {
    sint i;

    if(tw->sphere.use) {
        for(i = 0; i < 3; i++) {
            if(tw->start[i] < tw->end[i]) {
                tw->bounds[0][i] = tw->start[i] - Q_fabs(tw->sphere.offset[i]) -
                                   tw->sphere.radius;
                tw->bounds[1][i] = tw->start[i] + tw->trace.fraction * tw->dir[i] + Q_fabs(
                                       tw->sphere.offset[i]) + tw->sphere.radius;
            } else {
                tw->bounds[0][i] = tw->start[i] + tw->trace.fraction * tw->dir[i] - Q_fabs(
                                       tw->sphere.offset[i]) - tw->sphere.radius;
                tw->bounds[1][i] = tw->start[i] + Q_fabs(tw->sphere.offset[i]) +
                                   tw->sphere.radius;
            }
        }
    } else {
        for(i = 0; i < 3; i++) {
            if(tw->start[i] < tw->end[i]) {
                tw->bounds[0][i] = tw->start[i] + tw->size[0][i];
                tw->bounds[1][i] = tw->start[i] + tw->trace.fraction * tw->dir[i] +
                                   tw->size[1][i];
            } else {
                tw->bounds[0][i] = tw->start[i] + tw->trace.fraction * tw->dir[i] +
                                   tw->size[0][i];
                tw->bounds[1][i] = tw->start[i] + tw->size[1][i];
            }
        }
    }

    if(expand) {
        // expand for epsilon
        for(i = 0; i < 3; i++) {
            tw->bounds[0][i] -= 1.0f;
            tw->bounds[1][i] += 1.0f;
        }
    }
}

/*
================
CM_BoxDistanceFromPlane
================
*/
static float32 CM_BoxDistanceFromPlane(vec3_t center, vec3_t extents,
                                       cplane_t *plane) {
    float32 d1, d2;

    d1 = DotProduct(center, plane->normal) - plane->dist;
    d2 = Q_fabs(extents[0] * plane->normal[0]) +
         Q_fabs(extents[1] * plane->normal[1]) +
         Q_fabs(extents[2] * plane->normal[2]);

    if(d1 - d2 > 0.0f) {
        return d1 - d2;
    }

    if(d1 + d2 < 0.0f) {
        return d1 + d2;
    }

    return 0.0f;
}

/*
================
CM_BoxDistanceFromPlane
================
*/
static sint CM_TraceThroughBounds(traceWork_t *tw, vec3_t mins,
                                  vec3_t maxs) {
    sint i;
    vec3_t center, extents;

    for(i = 0; i < 3; i++) {
        if(mins[i] > tw->bounds[1][i]) {
            return false;
        }

        if(maxs[i] < tw->bounds[0][i]) {
            return false;
        }
    }

    VectorAdd(mins, maxs, center);
    VectorScale(center, 0.5f, center);
    VectorSubtract(maxs, center, extents);

    if(Q_fabs(CM_BoxDistanceFromPlane(center, extents,
                                      &tw->tracePlane1)) > tw->traceDist1) {
        return false;
    }

    if(Q_fabs(CM_BoxDistanceFromPlane(center, extents,
                                      &tw->tracePlane2)) > tw->traceDist2) {
        return false;
    }

    // trace might go through the bounds
    return true;
}

/*
================
CM_TraceThroughBrush
================
*/
void CM_TraceThroughBrush(traceWork_t *tw, cbrush_t *brush) {
    sint             i;
    cplane_t       *plane, *clipplane;
    float64           dist, enterFrac, leaveFrac, d1, d2, f, t;
    bool        getout, startout;
    cbrushside_t   *side, *leadside;
    vec3_t          startp, endp;

    enterFrac = -1.0;
    leaveFrac = 1.0;
    clipplane = nullptr;

    if(!brush->numsides) {
        return;
    }

    c_brush_traces++;

    getout = false;
    startout = false;

    leadside = nullptr;

    if(tw->type == TT_BISPHERE) {
        //
        // compare the trace against all planes of the brush
        // find the latest time the trace crosses a plane towards the interior
        // and the earliest time the trace crosses a plane towards the exterior
        //
        for(i = 0; i < brush->numsides; i++) {
            side = brush->sides + i;
            plane = side->plane;

            // adjust the plane distance apropriately for radius
            d1 = DotProduct(tw->start,
                            plane->normal) - (plane->dist + tw->biSphere.startRadius);
            d2 = DotProduct(tw->end,
                            plane->normal) - (plane->dist + tw->biSphere.endRadius);

            if(d2 > 0) {
                getout = true;  // endpoint is not in solid
            }

            if(d1 > 0) {
                startout = true;
            }

            // if completely in front of face, no intersection with the entire brush
            if(d1 > 0 && (d2 >= SURFACE_CLIP_EPSILON || d2 >= d1)) {
                return;
            }

            // if it doesn't cross the plane, the plane isn't relevent
            if(d1 <= 0 && d2 <= 0) {
                continue;
            }

            brush->collided = true;

            // crosses face
            if(d1 > d2) {
                // enter
                f = (d1 - SURFACE_CLIP_EPSILON) / (d1 - d2);

                if(f < 0) {
                    f = 0;
                }

                if(f > enterFrac) {
                    enterFrac = f;
                    clipplane = plane;
                    leadside = side;
                }
            } else {
                // leave
                f = (d1 + SURFACE_CLIP_EPSILON) / (d1 - d2);

                if(f > 1) {
                    f = 1;
                }

                if(f < leaveFrac) {
                    leaveFrac = f;
                }
            }
        }
    } else if(tw->type == TT_CAPSULE) {
        //
        // compare the trace against all planes of the brush
        // find the latest time the trace crosses a plane towards the interior
        // and the earliest time the trace crosses a plane towards the exterior
        //
        for(i = 0; i < brush->numsides; i++) {
            side = brush->sides + i;
            plane = side->plane;

            // adjust the plane distance appropriately for radius
            dist = plane->dist + tw->sphere.radius;

            // find the closest point on the capsule to the plane
            t = DotProduct(plane->normal, tw->sphere.offset);

            if(t > 0) {
                VectorSubtract(tw->start, tw->sphere.offset, startp);
                VectorSubtract(tw->end, tw->sphere.offset, endp);
            } else {
                VectorAdd(tw->start, tw->sphere.offset, startp);
                VectorAdd(tw->end, tw->sphere.offset, endp);
            }

            d1 = DotProduct(startp, plane->normal) - dist;
            d2 = DotProduct(endp, plane->normal) - dist;

            if(d2 > 0) {
                getout = true;  // endpoint is not in solid
            }

            if(d1 > 0) {
                startout = true;
            }

            // if completely in front of face, no intersection with the entire brush
            if(d1 > 0 && (d2 >= SURFACE_CLIP_EPSILON || d2 >= d1)) {
                return;
            }

            // if it doesn't cross the plane, the plane isn't relevant
            if(d1 <= 0 && d2 <= 0) {
                continue;
            }

            brush->collided = true;

            // crosses face
            if(d1 > d2) { // enter
                f = (d1 - SURFACE_CLIP_EPSILON) / (d1 - d2);

                if(f < 0) {
                    f = 0;
                }

                if(f > enterFrac) {
                    enterFrac = f;
                    clipplane = plane;
                    leadside = side;
                }
            } else { // leave
                f = (d1 + SURFACE_CLIP_EPSILON) / (d1 - d2);

                if(f > 1) {
                    f = 1;
                }

                if(f < leaveFrac) {
                    leaveFrac = f;
                }
            }
        }
    } else {
        //
        // compare the trace against all planes of the brush
        // find the latest time the trace crosses a plane towards the interior
        // and the earliest time the trace crosses a plane towards the exterior
        //
        for(i = 0; i < brush->numsides; i++) {
            side = brush->sides + i;
            plane = side->plane;

            // adjust the plane distance appropriately for mins/maxs
            dist = plane->dist - DotProduct(tw->offsets[plane->signbits],
                                            plane->normal);

            d1 = DotProduct(tw->start, plane->normal) - dist;
            d2 = DotProduct(tw->end, plane->normal) - dist;

            if(d2 > 0) {
                getout = true;  // endpoint is not in solid
            }

            if(d1 > 0) {
                startout = true;
            }

            // if completely in front of face, no intersection with the entire brush
            if(d1 > 0 && (d2 >= SURFACE_CLIP_EPSILON || d2 >= d1)) {
                return;
            }

            // if it doesn't cross the plane, the plane isn't relevant
            if(d1 <= 0 && d2 <= 0) {
                continue;
            }

            brush->collided = true;

            // crosses face
            if(d1 > d2) { // enter
                f = (d1 - SURFACE_CLIP_EPSILON) / (d1 - d2);

                if(f < 0) {
                    f = 0;
                }

                if(f > enterFrac) {
                    enterFrac = f;
                    clipplane = plane;
                    leadside = side;
                }
            } else { // leave
                f = (d1 + SURFACE_CLIP_EPSILON) / (d1 - d2);

                if(f > 1) {
                    f = 1;
                }

                if(f < leaveFrac) {
                    leaveFrac = f;
                }
            }
        }
    }

    //
    // all planes have been checked, and the trace was not
    // completely outside the brush
    //
    if(!startout) { // original point was inside brush
        tw->trace.startsolid = true;

        if(!getout) {
            tw->trace.allsolid = true;
            tw->trace.fraction = 0;
            tw->trace.contents = brush->contents;
        }

        return;
    }

    if(enterFrac < leaveFrac) {
        if(enterFrac > -1 && enterFrac < tw->trace.fraction) {
            if(enterFrac < 0) {
                enterFrac = 0;
            }

            tw->trace.fraction = enterFrac;

            if(clipplane != nullptr) {
                tw->trace.plane = *clipplane;
            }

            if(leadside != nullptr) {
                tw->trace.surfaceFlags = leadside->surfaceFlags;
            }

            tw->trace.contents = brush->contents;
        }
    }
}

/*
================
CM_ProximityToBrush
================
*/

static void CM_ProximityToBrush(traceWork_t *tw, cbrush_t *brush) {
    float32           dist, minDist = 1e+10f, s, t, sAtMin = 0.0f,
                            radius = 0.0f, fraction;
    sint             i;
    cbrushedge_t   *edge;
    traceWork_t     tw2;

    // cheapish purely linear trace to test for intersection
    ::memset(&tw2, 0, sizeof(tw2));

    tw2.trace.fraction = 1.0f;
    tw2.type = TT_CAPSULE;
    tw2.sphere.radius = 0.0f;
    VectorClear(tw2.sphere.offset);
    VectorCopy(tw->start, tw2.start);
    VectorCopy(tw->end, tw2.end);

    CM_TraceThroughBrush(&tw2, brush);

    if(tw2.trace.fraction == 1.0f && !tw2.trace.allsolid &&
            !tw2.trace.startsolid) {
        for(i = 0; i < brush->numEdges; i++) {
            edge = &brush->edges[i];

            dist = DistanceBetweenLineSegmentsSquared(tw->start, tw->end, edge->p0,
                    edge->p1, &s, &t);

            if(dist < minDist) {
                minDist = dist;
                sAtMin = s;
            }
        }

        if(tw->type == TT_BISPHERE) {
            radius = tw->biSphere.startRadius + (sAtMin * (tw->biSphere.endRadius -
                                                 tw->biSphere.startRadius));
        } else if(tw->type == TT_CAPSULE) {
            radius = tw->sphere.radius;
        } else if(tw->type == TT_AABB) {
            //FIXME
        }

        fraction = minDist / (radius * radius);

        if(fraction < tw->trace.lateralFraction) {
            tw->trace.lateralFraction = fraction;
        }
    } else {
        tw->trace.lateralFraction = 0.0f;
    }
}

/*
================
CM_ProximityToSurface
================
*/
static void CM_ProximityToSurface(traceWork_t *tw, cSurface_t *surface) {
    traceWork_t tw2;

    // cheapish purely linear trace to test for intersection
    ::memset(&tw2, 0, sizeof(tw2));

    tw2.trace.fraction = 1.0f;
    tw2.type = TT_CAPSULE;
    tw2.sphere.radius = 0.0f;
    VectorClear(tw2.sphere.offset);
    VectorCopy(tw->start, tw2.start);
    VectorCopy(tw->end, tw2.end);

    CM_TraceThroughSurface(&tw2, surface);

    if(tw2.trace.fraction == 1.0f && !tw2.trace.allsolid &&
            !tw2.trace.startsolid) {
        //FIXME: implement me
    } else {
        tw->trace.lateralFraction = 0.0f;
    }
}

/*
================
CM_TraceThroughLeaf
================
*/
void CM_TraceThroughLeaf(traceWork_t *tw, cLeaf_t *leaf) {
    sint             k, brushnum;
    cbrush_t       *brush;
    cSurface_t     *surface;
    float32 fraction;

    // trace line against all brushes in the leaf
    for(k = 0; k < leaf->numLeafBrushes; k++) {
        brushnum = cm.leafbrushes[leaf->firstLeafBrush + k];

        brush = &cm.brushes[brushnum];

        if(brush->checkcount == cm.checkcount) {
            continue; // already checked this brush in another leaf
        }

        brush->checkcount = cm.checkcount;

        if(!(brush->contents & tw->contents)) {
            continue;
        }

        brush->collided = false;

        if(!CM_BoundsIntersect(tw->bounds[0], tw->bounds[1], brush->bounds[0],
                               brush->bounds[1])) {
            continue;
        }

#ifndef BSPC

        if(cm_optimize->integer)
#endif
        {
            if(!CM_TraceThroughBounds(tw, brush->bounds[0], brush->bounds[1])) {
                continue;
            }
        }

        fraction = tw->trace.fraction;

        CM_TraceThroughBrush(tw, brush);

        if(!tw->trace.fraction) {
            tw->trace.lateralFraction = 0.0f;
            return;
        }

        if(tw->trace.fraction < fraction) {
            CM_CalcTraceBounds(tw, true);
        }
    }

    // trace line against all surfaces in the leaf
#ifdef BSPC

    if(1) {
#else

    if(!cm_noCurves->integer) {
#endif

        for(k = 0; k < leaf->numLeafSurfaces; k++) {
            if(!(cm.surfaces[cm.leafsurfaces[leaf->firstLeafSurface + k]])) {
                continue;
            }

            surface = cm.surfaces[cm.leafsurfaces[leaf->firstLeafSurface + k]];

            if(!surface) {
                continue;
            }

            if(surface->checkcount == cm.checkcount) {
                continue; // already checked this surface in another leaf
            }

            surface->checkcount = cm.checkcount;

            if(!(surface->contents & tw->contents)) {
                continue;
            }

            if(!CM_BoundsIntersect(tw->bounds[0], tw->bounds[1],
                                   surface->sc->bounds[0], surface->sc->bounds[1])) {
                continue;
            }

#ifndef BSPC

            if(cm_optimize->integer)
#endif
            {
                if(!CM_TraceThroughBounds(tw, surface->sc->bounds[0],
                                          surface->sc->bounds[1])) {
                    continue;
                }
            }

            fraction = tw->trace.fraction;

            CM_TraceThroughSurface(tw, surface);

            if(!tw->trace.fraction) {
                tw->trace.lateralFraction = 0.0f;
                return;
            }

            if(tw->trace.fraction < fraction) {
                CM_CalcTraceBounds(tw, true);
            }
        }
    }

    if(tw->testLateralCollision && tw->trace.fraction < 1.0f) {
        for(k = 0; k < leaf->numLeafBrushes; k++) {
            brushnum = cm.leafbrushes[leaf->firstLeafBrush + k];

            brush = &cm.brushes[brushnum];

            // This brush never collided, so don't bother
            if(!brush->collided) {
                continue;
            }

            if(!(brush->contents & tw->contents)) {
                continue;
            }

            CM_ProximityToBrush(tw, brush);

            if(!tw->trace.lateralFraction) {
                return;
            }
        }

        for(k = 0; k < leaf->numLeafSurfaces; k++) {
            surface = cm.surfaces[cm.leafsurfaces[leaf->firstLeafSurface + k]];

            if(!surface) {
                continue;
            }

            if(!(surface->contents & tw->contents)) {
                continue;
            }

            CM_ProximityToSurface(tw, surface);

            if(!tw->trace.lateralFraction) {
                return;
            }
        }
    }
}

#define RADIUS_EPSILON      1.0f

/*
================
CM_TraceThroughSphere

get the first intersection of the ray with the sphere
================
*/
void CM_TraceThroughSphere(traceWork_t *tw, vec3_t origin, float32 radius,
                           vec3_t start, vec3_t end) {
    float32           l1, l2, length, scale, fraction, /*a,*/ b, c, d, sqrtd;
    vec3_t          v1, dir, intersection;

    // if inside the sphere
    VectorSubtract(start, origin, dir);
    l1 = VectorLengthSquared(dir);

    if(l1 < Square(radius)) {
        tw->trace.fraction = 0;
        tw->trace.startsolid = true;
        // test for allsolid
        VectorSubtract(end, origin, dir);
        l1 = VectorLengthSquared(dir);

        if(l1 < Square(radius)) {
            tw->trace.allsolid = true;
        }

        return;
    }

    //
    VectorSubtract(end, start, dir);
    length = VectorNormalize(dir);
    //
    l1 = CM_DistanceFromLineSquared(origin, start, end, dir);
    VectorSubtract(end, origin, v1);
    l2 = VectorLengthSquared(v1);

    // if no intersection with the sphere and the end point is at least an epsilon away
    if(l1 >= Square(radius) && l2 > Square(radius + SURFACE_CLIP_EPSILON)) {
        return;
    }

    VectorSubtract(start, origin, v1);
    // dir is normalized so a = 1
    //a = 1.0f; //dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2];
    b = 2.0f * (dir[0] * v1[0] + dir[1] * v1[1] + dir[2] * v1[2]);
    c = v1[0] * v1[0] + v1[1] * v1[1] + v1[2] * v1[2] - (radius +
            RADIUS_EPSILON) * (radius + RADIUS_EPSILON);

    d = b * b - 4.0f * c;       // * a;

    if(d > 0) {
        sqrtd = SquareRootFloat(d);
        // = (- b + sqrtd) * 0.5f; // / (2.0f * a);
        fraction = (-b - sqrtd) * 0.5f;      // / (2.0f * a);

        if(fraction < 0) {
            fraction = 0;
        } else {
            fraction /= length;
        }

        if(fraction < tw->trace.fraction) {
            tw->trace.fraction = fraction;
            VectorSubtract(end, start, dir);
            VectorMA(start, fraction, dir, intersection);
            VectorSubtract(intersection, origin, dir);
#ifdef CAPSULE_DEBUG
            l2 = VectorLength(dir);

            if(l2 < radius) {
                sint bah = 1;
            }

#endif
            scale = 1 / (radius + RADIUS_EPSILON);
            VectorScale(dir, scale, dir);
            VectorCopy(dir, tw->trace.plane.normal);
            VectorAdd(tw->modelOrigin, intersection, intersection);
            tw->trace.plane.dist = DotProduct(tw->trace.plane.normal, intersection);
            tw->trace.contents = CONTENTS_BODY;
        }
    } else if(d == 0) {
        //t1 = (- b ) / 2;
        // slide along the sphere
    }

    // no intersection at all
}

/*
================
CM_TraceThroughVerticalCylinder

get the first intersection of the ray with the cylinder
the cylinder extends halfheight above and below the origin
================
*/
void CM_TraceThroughVerticalCylinder(traceWork_t *tw, vec3_t origin,
                                     float32 radius, float32 halfheight, vec3_t start, vec3_t end) {
    float32           length, scale, fraction, l1, l2, /*a,*/ b, c, d, sqrtd;
    vec3_t          v1, dir, start2d, end2d, org2d, intersection;

    // 2d coordinates
    VectorSet(start2d, start[0], start[1], 0);
    VectorSet(end2d, end[0], end[1], 0);
    VectorSet(org2d, origin[0], origin[1], 0);

    // if between lower and upper cylinder bounds
    if(start[2] <= origin[2] + halfheight &&
            start[2] >= origin[2] - halfheight) {
        // if inside the cylinder
        VectorSubtract(start2d, org2d, dir);
        l1 = VectorLengthSquared(dir);

        if(l1 < Square(radius)) {
            tw->trace.fraction = 0;
            tw->trace.startsolid = true;
            VectorSubtract(end2d, org2d, dir);
            l1 = VectorLengthSquared(dir);

            if(l1 < Square(radius)) {
                tw->trace.allsolid = true;
            }

            return;
        }
    }

    //
    VectorSubtract(end2d, start2d, dir);
    length = VectorNormalize(dir);
    //
    l1 = CM_DistanceFromLineSquared(org2d, start2d, end2d, dir);
    VectorSubtract(end2d, org2d, v1);
    l2 = VectorLengthSquared(v1);

    // if no intersection with the cylinder and the end point is at least an epsilon away
    if(l1 >= Square(radius) && l2 > Square(radius + SURFACE_CLIP_EPSILON)) {
        return;
    }

    VectorSubtract(start, origin, v1);
    // dir is normalized so we can use a = 1
    //a = 1.0f; // * (dir[0] * dir[0] + dir[1] * dir[1]);
    b = 2.0f * (v1[0] * dir[0] + v1[1] * dir[1]);
    c = v1[0] * v1[0] + v1[1] * v1[1] - (radius + RADIUS_EPSILON) *
        (radius + RADIUS_EPSILON);

    d = b * b - 4.0f * c;// * a;

    if(d > 0) {
        sqrtd = SquareRootFloat(d);
        // = (- b + sqrtd) * 0.5f;// / (2.0f * a);
        fraction = (-b - sqrtd) * 0.5f;      // / (2.0f * a);

        if(fraction < 0) {
            fraction = 0;
        } else {
            fraction /= length;
        }

        if(fraction < tw->trace.fraction) {
            VectorSubtract(end, start, dir);
            VectorMA(start, fraction, dir, intersection);

            // if the intersection is between the cylinder lower and upper bound
            if(intersection[2] <= origin[2] + halfheight &&
                    intersection[2] >= origin[2] - halfheight) {
                tw->trace.fraction = fraction;
                VectorSubtract(intersection, origin, dir);
                dir[2] = 0;
#ifdef CAPSULE_DEBUG
                l2 = VectorLength(dir);

                if(l2 <= radius) {
                    sint bah = 1;
                }

#endif
                scale = 1 / (radius + RADIUS_EPSILON);
                VectorScale(dir, scale, dir);
                VectorCopy(dir, tw->trace.plane.normal);
                VectorAdd(tw->modelOrigin, intersection, intersection);
                tw->trace.plane.dist = DotProduct(tw->trace.plane.normal, intersection);
                tw->trace.contents = CONTENTS_BODY;
            }
        }
    } else if(d == 0) {
        //t[0] = (- b ) / 2 * a;
        // slide along the cylinder
    }

    // no intersection at all
}

/*
================
CM_TraceCapsuleThroughCapsule

capsule vs. capsule collision (not rotated)
================
*/
void CM_TraceCapsuleThroughCapsule(traceWork_t *tw, clipHandle_t model) {
    sint             i;
    vec3_t          mins, maxs, top, bottom, starttop, startbottom, endtop,
                    endbottom, offset, symetricSize[2];
    float32           radius, halfwidth, halfheight, offs, h;

    collisionModelManagerLocal.ModelBounds(model, mins, maxs);

    // test trace bounds vs. capsule bounds
    if(tw->bounds[0][0] > maxs[0] + RADIUS_EPSILON ||
            tw->bounds[0][1] > maxs[1] + RADIUS_EPSILON ||
            tw->bounds[0][2] > maxs[2] + RADIUS_EPSILON ||
            tw->bounds[1][0] < mins[0] - RADIUS_EPSILON ||
            tw->bounds[1][1] < mins[1] - RADIUS_EPSILON ||
            tw->bounds[1][2] < mins[2] - RADIUS_EPSILON) {
        return;
    }

    // top origin and bottom origin of each sphere at start and end of trace
    VectorAdd(tw->start, tw->sphere.offset, starttop);
    VectorSubtract(tw->start, tw->sphere.offset, startbottom);
    VectorAdd(tw->end, tw->sphere.offset, endtop);
    VectorSubtract(tw->end, tw->sphere.offset, endbottom);

    // calculate top and bottom of the capsule spheres to collide with
    for(i = 0; i < 3; i++) {
        offset[i] = (mins[i] + maxs[i]) * 0.5f;
        symetricSize[0][i] = mins[i] - offset[i];
        symetricSize[1][i] = maxs[i] - offset[i];
    }

    halfwidth = symetricSize[1][0];
    halfheight = symetricSize[1][2];
    radius = (halfwidth > halfheight) ? halfheight : halfwidth;
    offs = halfheight - radius;
    VectorCopy(offset, top);
    top[2] += offs;
    VectorCopy(offset, bottom);
    bottom[2] -= offs;
    // expand radius of spheres
    radius += tw->sphere.radius;

    // if there is horizontal movement
    if(tw->start[0] != tw->end[0] || tw->start[1] != tw->end[1]) {
        // height of the expanded cylinder is the height of both cylinders minus the radius of both spheres
        h = halfheight + tw->sphere.halfheight - radius;

        // if the cylinder has a height
        if(h > 0) {
            // test for collisions between the cylinders
            CM_TraceThroughVerticalCylinder(tw, offset, radius, h, tw->start, tw->end);
        }
    }

    // test for collision between the spheres
    CM_TraceThroughSphere(tw, top, radius, startbottom, endbottom);
    CM_TraceThroughSphere(tw, bottom, radius, starttop, endtop);
}

/*
================
CM_TraceBoundingBoxThroughCapsule

bounding box vs. capsule collision
================
*/
void CM_TraceBoundingBoxThroughCapsule(traceWork_t *tw,
                                       clipHandle_t model) {
    vec3_t          mins, maxs, offset, size[2];
    clipHandle_t    h;
    cmodel_t       *cmod;
    sint             i;

    // mins maxs of the capsule
    collisionModelManagerLocal.ModelBounds(model, mins, maxs);

    // offset for capsule center
    for(i = 0; i < 3; i++) {
        offset[i] = (mins[i] + maxs[i]) * 0.5f;
        size[0][i] = mins[i] - offset[i];
        size[1][i] = maxs[i] - offset[i];
        tw->start[i] -= offset[i];
        tw->end[i] -= offset[i];
    }

    // replace the bounding box with the capsule
    tw->type = TT_CAPSULE;
    tw->sphere.radius = (size[1][0] > size[1][2]) ? size[1][2] : size[1][0];
    tw->sphere.halfheight = size[1][2];
    VectorSet(tw->sphere.offset, 0, 0, size[1][2] - tw->sphere.radius);

    // replace the capsule with the bounding box
    h = collisionModelManagerLocal.TempBoxModel(tw->size[0], tw->size[1],
            false);

    // calculate collision
    cmod = CM_ClipHandleToModel(h);
    CM_TraceThroughLeaf(tw, &cmod->leaf);
}

//=========================================================================================

/*
==================
CM_TraceThroughTree

Traverse all the contacted leafs from the start to the end position.
If the trace is a point, they will be exactly in order, but for larger
trace volumes it is possible to hit something in a later leaf with
a smaller intercept fraction.
==================
*/
static void CM_TraceThroughTree(traceWork_t *tw, sint num, float32 p1f,
                                float32 p2f, vec3_t p1, vec3_t p2) {
    float32           t1, t2, offset, frac, frac2, idist, midf;
    cNode_t        *node;
    cplane_t       *plane;
    vec3_t          mid;
    sint             side;

    if(tw->trace.fraction <= p1f) {
        return; // already hit something nearer
    }

    // if < 0, we are in a leaf node
    if(num < 0) {
        CM_TraceThroughLeaf(tw, &cm.leafs[-1 - num]);
        return;
    }

    //
    // find the point distances to the seperating plane
    // and the offset for the size of the box
    //
    node = cm.nodes + num;
    plane = node->plane;

    // adjust the plane distance apropriately for mins/maxs
    if(plane->type < 3) {
        t1 = p1[plane->type] - plane->dist;
        t2 = p2[plane->type] - plane->dist;
        offset = tw->extents[plane->type];
    } else {
        t1 = DotProduct(plane->normal, p1) - plane->dist;
        t2 = DotProduct(plane->normal, p2) - plane->dist;

        if(tw->isPoint) {
            offset = 0;
        } else {
            offset = tw->maxOffset;
        }
    }

    // see which sides we need to consider
    if(t1 >= offset + 1 && t2 >= offset + 1) {
        CM_TraceThroughTree(tw, node->children[0], p1f, p2f, p1, p2);
        return;
    }

    if(t1 < -offset - 1 && t2 < -offset - 1) {
        CM_TraceThroughTree(tw, node->children[1], p1f, p2f, p1, p2);
        return;
    }

    // put the crosspoint SURFACE_CLIP_EPSILON pixels on the near side
    if(t1 < t2) {
        idist = 1.0 / (t1 - t2);
        side = 1;
        frac2 = (t1 + offset + SURFACE_CLIP_EPSILON) * idist;
        frac = (t1 - offset + SURFACE_CLIP_EPSILON) * idist;
    } else if(t1 > t2) {
        idist = 1.0 / (t1 - t2);
        side = 0;
        frac2 = (t1 - offset - SURFACE_CLIP_EPSILON) * idist;
        frac = (t1 + offset + SURFACE_CLIP_EPSILON) * idist;
    } else {
        side = 0;
        frac = 1;
        frac2 = 0;
    }

    // move up to the node
    if(frac < 0) {
        frac = 0;
    }

    if(frac > 1) {
        frac = 1;
    }

    midf = p1f + (p2f - p1f) * frac;

    mid[0] = p1[0] + frac * (p2[0] - p1[0]);
    mid[1] = p1[1] + frac * (p2[1] - p1[1]);
    mid[2] = p1[2] + frac * (p2[2] - p1[2]);

    CM_TraceThroughTree(tw, node->children[side], p1f, midf, p1, mid);

    // go past the node
    if(frac2 < 0) {
        frac2 = 0;
    } else if(frac2 > 1) {
        frac2 = 1;
    }

    midf = p1f + (p2f - p1f) * frac2;

    mid[0] = p1[0] + frac2 * (p2[0] - p1[0]);
    mid[1] = p1[1] + frac2 * (p2[1] - p1[1]);
    mid[2] = p1[2] + frac2 * (p2[2] - p1[2]);

    CM_TraceThroughTree(tw, node->children[side ^ 1], midf, p2f, mid, p2);
}

//======================================================================

/*
==================
CM_Trace
==================
*/
static void CM_Trace(trace_t *results, const vec3_t start,
                     const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model,
                     const vec3_t origin, sint brushmask, traceType_t type, sphere_t *sphere) {
    sint             i;
    traceWork_t     tw;
    vec3_t          offset;
    cmodel_t       *cmod;
    bool            positionTest;
    vec3_t          dir;
    float32             dist;

    cmod = CM_ClipHandleToModel(model);

    cm.checkcount++;            // for multi-check avoidance

    c_traces++;                 // for statistics, may be zeroed

    // fill in a default trace
    ::memset(&tw, 0, sizeof(tw));
    tw.trace.fraction =
        1;      // assume it goes the entire distance until shown otherwise
    VectorCopy(origin, tw.modelOrigin);
    tw.type = type;

    if(!cm.numNodes) {
        *results = tw.trace;

        return; // map not loaded, shouldn't happen
    }

    // allow nullptr to be passed in for 0,0,0
    if(!mins) {
        mins = vec3_origin;
    }

    if(!maxs) {
        maxs = vec3_origin;
    }

    // set basic parms
    tw.contents = brushmask;

    // adjust so that mins and maxs are always symetric, which
    // avoids some complications with plane expanding of rotated
    // bmodels
    for(i = 0; i < 3; i++) {
        offset[i] = (mins[i] + maxs[i]) * 0.5f;
        tw.size[0][i] = mins[i] - offset[i];
        tw.size[1][i] = maxs[i] - offset[i];
        tw.start[i] = start[i] + offset[i];
        tw.end[i] = end[i] + offset[i];
    }

    // if a sphere is already specified
    if(sphere) {
        tw.sphere = *sphere;
    } else {
        tw.sphere.radius = (tw.size[1][0] > tw.size[1][2]) ? tw.size[1][2] :
                           tw.size[1][0];
        tw.sphere.halfheight = tw.size[1][2];
        VectorSet(tw.sphere.offset, 0, 0, tw.size[1][2] - tw.sphere.radius);
    }

    positionTest = (start[0] == end[0] && start[1] == end[1] &&
                    start[2] == end[2]);

    tw.maxOffset = tw.size[1][0] + tw.size[1][1] + tw.size[1][2];

    // tw.offsets[signbits] = vector to apropriate corner from origin
    tw.offsets[0][0] = tw.size[0][0];
    tw.offsets[0][1] = tw.size[0][1];
    tw.offsets[0][2] = tw.size[0][2];

    tw.offsets[1][0] = tw.size[1][0];
    tw.offsets[1][1] = tw.size[0][1];
    tw.offsets[1][2] = tw.size[0][2];

    tw.offsets[2][0] = tw.size[0][0];
    tw.offsets[2][1] = tw.size[1][1];
    tw.offsets[2][2] = tw.size[0][2];

    tw.offsets[3][0] = tw.size[1][0];
    tw.offsets[3][1] = tw.size[1][1];
    tw.offsets[3][2] = tw.size[0][2];

    tw.offsets[4][0] = tw.size[0][0];
    tw.offsets[4][1] = tw.size[0][1];
    tw.offsets[4][2] = tw.size[1][2];

    tw.offsets[5][0] = tw.size[1][0];
    tw.offsets[5][1] = tw.size[0][1];
    tw.offsets[5][2] = tw.size[1][2];

    tw.offsets[6][0] = tw.size[0][0];
    tw.offsets[6][1] = tw.size[1][1];
    tw.offsets[6][2] = tw.size[1][2];

    tw.offsets[7][0] = tw.size[1][0];
    tw.offsets[7][1] = tw.size[1][1];
    tw.offsets[7][2] = tw.size[1][2];

    // check for point special case
    if(tw.size[0][0] == 0.0f && tw.size[0][1] == 0.0f &&
            tw.size[0][2] == 0.0f) {
        tw.isPoint = true;
        VectorClear(tw.extents);
    } else {
        tw.isPoint = false;
        tw.extents[0] = tw.size[1][0];
        tw.extents[1] = tw.size[1][1];
        tw.extents[2] = tw.size[1][2];
    }

    if(positionTest) {
        CM_CalcTraceBounds(&tw, false);
    } else {
        VectorSubtract(tw.end, tw.start, dir);
        VectorCopy(dir, tw.dir);
        VectorNormalize(dir);
        MakeNormalVectors(dir, tw.tracePlane1.normal, tw.tracePlane2.normal);
        tw.tracePlane1.dist = DotProduct(tw.tracePlane1.normal, tw.start);
        tw.tracePlane2.dist = DotProduct(tw.tracePlane2.normal, tw.start);

        if(tw.isPoint) {
            tw.traceDist1 = tw.traceDist2 = 1.0f;
        } else {
            tw.traceDist1 = tw.traceDist2 = 0.0f;

            for(i = 0; i < 8; i++) {
                dist = Q_fabs(DotProduct(tw.tracePlane1.normal,
                                         tw.offsets[i]) - tw.tracePlane1.dist);

                if(dist > tw.traceDist1) {
                    tw.traceDist1 = dist;
                }

                dist = Q_fabs(DotProduct(tw.tracePlane2.normal,
                                         tw.offsets[i]) - tw.tracePlane2.dist);

                if(dist > tw.traceDist2) {
                    tw.traceDist2 = dist;
                }
            }

            // expand for epsilon
            tw.traceDist1 += 1.0f;
            tw.traceDist2 += 1.0f;
        }

        CM_CalcTraceBounds(&tw, true);
    }

    // check for position test special case
    if(positionTest) {
        if(model) {
#ifdef ALWAYS_BBOX_VS_BBOX      // FIXME - compile time flag?

            if(model == BOX_MODEL_HANDLE || model == CAPSULE_MODEL_HANDLE) {
                tw.type = TT_AABB;
                CM_TestInLeaf(&tw, &cmod->leaf);
            } else
#elif defined(ALWAYS_CAPSULE_VS_CAPSULE)
            if(model == BOX_MODEL_HANDLE || model == CAPSULE_MODEL_HANDLE) {
                CM_TestCapsuleInCapsule(&tw, model);
            } else
#endif
                if(model == CAPSULE_MODEL_HANDLE) {
                    if(tw.type == TT_CAPSULE) {
                        CM_TestCapsuleInCapsule(&tw, model);
                    } else {
                        CM_TestBoundingBoxInCapsule(&tw, model);
                    }
                } else {
                    CM_TestInLeaf(&tw, &cmod->leaf);
                }
        } else {
            CM_PositionTest(&tw);
        }
    } else {
        //
        // general sweeping through world
        //
        if(model) {
#ifdef ALWAYS_BBOX_VS_BBOX

            if(model == BOX_MODEL_HANDLE || model == CAPSULE_MODEL_HANDLE) {
                tw.type = TT_AABB;
                CM_TraceThroughLeaf(&tw, &cmod->leaf);
            } else
#elif defined(ALWAYS_CAPSULE_VS_CAPSULE)
            if(model == BOX_MODEL_HANDLE || model == CAPSULE_MODEL_HANDLE) {
                CM_TraceCapsuleThroughCapsule(&tw, model);
            } else
#endif
                if(model == CAPSULE_MODEL_HANDLE) {
                    if(tw.type == TT_CAPSULE) {
                        CM_TraceCapsuleThroughCapsule(&tw, model);
                    } else {
                        CM_TraceBoundingBoxThroughCapsule(&tw, model);
                    }
                } else {
                    CM_TraceThroughLeaf(&tw, &cmod->leaf);
                }
        } else {
            CM_TraceThroughTree(&tw, 0, 0, 1, tw.start, tw.end);
        }
    }

    // generate endpos from the original, unmodified start/end
    if(tw.trace.fraction == 1) {
        VectorCopy(end, tw.trace.endpos);
    } else {
        VectorLerp(start, end, tw.trace.fraction, tw.trace.endpos);
    }

    // If allsolid is set (was entirely inside something solid), the plane is not valid.
    // If fraction == 1.0, we never hit anything, and thus the plane is not valid.
    // Otherwise, the normal on the plane should have unit length
    //assert( tw.trace.allsolid ||
    //        tw.trace.fraction == 1.0 ||
    //        VectorLengthSquared( tw.trace.plane.normal ) > 0.9999 );
    *results = tw.trace;
}

/*
==================
idCollisionModelManagerLocal::BoxTrace
==================
*/
void idCollisionModelManagerLocal::BoxTrace(trace_t *results,
        const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs,
        clipHandle_t model, sint brushmask, traceType_t type) {
    CM_Trace(results, start, end, mins, maxs, model, vec3_origin, brushmask,
             type, nullptr);
}

/*
==================
idCollisionModelManagerLocal::TransformedBoxTrace

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
void idCollisionModelManagerLocal::TransformedBoxTrace(trace_t *results,
        const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs,
        clipHandle_t model, sint brushmask, const vec3_t origin,
        const vec3_t angles, traceType_t type) {
    trace_t         trace;
    vec3_t          start_l, end_l;
    vec3_t          startRotated, endRotated;
    bool            rotated;
    vec3_t          offset;
    vec3_t          symetricSize[2];
    matrix_t        rotation, transform, inverse;
    sint             i;
    float32           halfwidth;
    float32           halfheight;
    float32           t;
    sphere_t        sphere;

    if(!mins) {
        mins = vec3_origin;
    }

    if(!maxs) {
        maxs = vec3_origin;
    }

    // adjust so that mins and maxs are always symmetric, which
    // avoids some complications with plane expanding of rotated
    // bmodels
    for(i = 0; i < 3; i++) {
        offset[i] = (mins[i] + maxs[i]) * 0.5f;
        symetricSize[0][i] = mins[i] - offset[i];
        symetricSize[1][i] = maxs[i] - offset[i];
        start_l[i] = start[i] + offset[i];
        end_l[i] = end[i] + offset[i];
    }

    // rotate start and end into the models frame of reference
    if(model != BOX_MODEL_HANDLE && model != CAPSULE_MODEL_HANDLE &&
            (angles[0] || angles[1] || angles[2])) {
        rotated = true;
    } else {
        rotated = false;
    }

    halfwidth = symetricSize[1][0];
    halfheight = symetricSize[1][2];

    sphere.use = false;
    sphere.radius = (halfwidth > halfheight) ? halfheight : halfwidth;
    sphere.halfheight = halfheight;
    t = halfheight - sphere.radius;

    if(rotated) {
        // rotation on trace line (start-end) instead of rotating the bmodel
        // NOTE: This is still incorrect for bounding boxes because the actual bounding
        //       box that is swept through the model is not rotated. We cannot rotate
        //       the bounding box or the bmodel because that would make all the brush
        //       bevels invalid.
        //       However this is correct for capsules since a capsule itself is rotated too.
#ifndef BSPC
        MatrixFromAngles(rotation, angles[PITCH], angles[YAW], angles[ROLL]);
        MatrixSetupTransformFromRotation(transform, rotation, origin);
        MatrixAffineInverse(transform, inverse);

        // transform trace line into the clipModel's space
        MatrixTransformPoint(inverse, start_l, startRotated);
        MatrixTransformPoint(inverse, end_l, endRotated);
#endif

        // extract up vector from the rotation matrix as rotated sphere offset for capsule
#if 0
        sphere.offset[0] = matrix[0][2] * t;
        sphere.offset[1] = -matrix[1][2] * t;
        sphere.offset[2] = matrix[2][2] * t;
#elif 0
        sphere.offset[0] = rotation[ 2] * t;
        sphere.offset[1] = -rotation[ 6] * t;
        sphere.offset[2] = rotation[10] * t;
#elif 0
        sphere.offset[0] = inverse[ 2] * t;
        sphere.offset[1] = -inverse[ 6] * t;
        sphere.offset[2] = inverse[10] * t;

#else
        sphere.offset[0] = rotation[ 8] * t;
        sphere.offset[1] = rotation[ 9] * t;
        sphere.offset[2] = rotation[10] * t;
#endif
    } else {
        VectorSubtract(start_l, origin, startRotated);
        VectorSubtract(end_l, origin, endRotated);

        VectorSet(sphere.offset, 0, 0, t);
    }

    // sweep the box through the model
    CM_Trace(&trace, startRotated, endRotated, symetricSize[0],
             symetricSize[1], model, origin, brushmask, type, &sphere);

    // if the bmodel was rotated and there was a collision
    if(rotated && trace.fraction != 1.0) {
        // Tr3B: we rotated our trace into the space of the clipModel
        // so we have to rotate the trace plane normal back to world space
#ifndef BSPC
        MatrixTransformNormal2(rotation, trace.plane.normal);
#endif
    }

    // re-calculate the end position of the trace because the trace.endpos
    // calculated by CM_Trace could be rotated and have an offset
    VectorLerp(start, end, trace.fraction, trace.endpos);

    *results = trace;
}

/*
==================
idCollisionModelManagerLocal::BiSphereTrace
==================
*/
void idCollisionModelManagerLocal::BiSphereTrace(trace_t *results,
        const vec3_t start, const vec3_t end, float32 startRad, float32 endRad,
        clipHandle_t model, sint mask) {
    float32           largestRadius = startRad > endRad ? startRad : endRad;
    sint             i;
    traceWork_t     tw;
    cmodel_t       *cmod;

    cmod = CM_ClipHandleToModel(model);

    cm.checkcount++;            // for multi-check avoidance

    c_traces++;                 // for statistics, may be zeroed

    // fill in a default trace
    ::memset(&tw, 0, sizeof(tw));
    tw.trace.fraction =
        1.0f;   // assume it goes the entire distance until shown otherwise
    VectorCopy(vec3_origin, tw.modelOrigin);
    tw.type = TT_BISPHERE;
    tw.testLateralCollision = true;
    tw.trace.lateralFraction = 1.0f;

    if(!cm.numNodes) {
        *results = tw.trace;

        return;                 // map not loaded, shouldn't happen
    }

    // set basic parms
    tw.contents = mask;

    VectorCopy(start, tw.start);
    VectorCopy(end, tw.end);

    tw.biSphere.startRadius = startRad;
    tw.biSphere.endRadius = endRad;

    //
    // calculate bounds
    //
    for(i = 0; i < 3; i++) {
        if(tw.start[i] < tw.end[i]) {
            tw.bounds[0][i] = tw.start[i] - tw.biSphere.startRadius;
            tw.bounds[1][i] = tw.end[i] + tw.biSphere.endRadius;
        } else {
            tw.bounds[0][i] = tw.end[i] + tw.biSphere.endRadius;
            tw.bounds[1][i] = tw.start[i] - tw.biSphere.startRadius;
        }
    }

    tw.isPoint = false;
    tw.extents[0] = largestRadius;
    tw.extents[1] = largestRadius;
    tw.extents[2] = largestRadius;

    //
    // general sweeping through world
    //
    if(model) {
        CM_TraceThroughLeaf(&tw, &cmod->leaf);
    } else {
        CM_TraceThroughTree(&tw, 0, 0.0f, 1.0f, tw.start, tw.end);
    }

    // generate endpos from the original, unmodified start/end
    if(tw.trace.fraction == 1.0f) {
        VectorCopy(end, tw.trace.endpos);
    } else {
        for(i = 0; i < 3; i++) {
            tw.trace.endpos[i] = start[i] + tw.trace.fraction * (end[i] - start[i]);
        }
    }

    // If allsolid is set (was entirely inside something solid), the plane is not valid.
    // If fraction == 1.0, we never hit anything, and thus the plane is not valid.
    // Otherwise, the normal on the plane should have unit length

    // Tr3B: these asserts don't make sense as it is the task of the gamecode to check if the trace was successful or not
    //  assert(!tw.trace.allsolid);
    //  assert(tw.trace.fraction != 1.0);
    //  assert(VectorLength(tw.trace.plane.normal) > 0.9999);

    *results = tw.trace;
}

/*
==================
idCollisionModelManagerLocal::TransformedBiSphereTrace

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
void idCollisionModelManagerLocal::TransformedBiSphereTrace(
    trace_t *results, const vec3_t start, const vec3_t end, float32 startRad,
    float32 endRad, clipHandle_t model, sint mask, const vec3_t origin) {
    trace_t         trace;
    vec3_t          start_l, end_l;

    // subtract origin offset
    VectorSubtract(start, origin, start_l);
    VectorSubtract(end, origin, end_l);

    collisionModelManagerLocal.BiSphereTrace(&trace, start_l, end_l, startRad,
            endRad, model, mask);

    // re-calculate the end position of the trace because the trace.endpos
    // calculated by CM_BiSphereTrace could be rotated and have an offset
    trace.endpos[0] = start[0] + trace.fraction * (end[0] - start[0]);
    trace.endpos[1] = start[1] + trace.fraction * (end[1] - start[1]);
    trace.endpos[2] = start[2] + trace.fraction * (end[2] - start[2]);

    *results = trace;
}

/*
=======================================================================
DEBUGGING
=======================================================================
*/

/*
==================
idCollisionModelManagerLocal::DrawDebugSurface

Called from the renderer
==================
*/
void idCollisionModelManagerLocal::DrawDebugSurface(void (*drawPoly)(
            sint color, sint numPoints, float32 *points)) {
#ifndef BSPC
    static convar_t           *cv, *cv2;
#endif
    const cSurfaceCollide_t *pc;
    cFacet_t                *facet;
    winding_t               *w;
    sint                      i, j, k, curplanenum, planenum, curinward,
                              inward;
    float32                    plane[4];

#ifndef BSPC

    if(!cv2) {
        cv2 = cvarSystem->Get("r_debugSurface", "0", 0,
                              "Debugging tool: shows bounding boxes of patch mesh surfaces that are targeted by the camera view.");
    }

#endif

    if(!debugSurfaceCollide) {
        return;
    }

#ifndef BSPC

    if(!cv) {
        cv = cvarSystem->Get("cm_debugSize", "2", 0,
                             "Sets the size of the debug surface grid for curved surfaces when r_debugsurface is 1.");
    }

#endif

    pc = debugSurfaceCollide;

    for(i = 0, facet = pc->facets; i < pc->numFacets; i++, facet++) {
        for(k = 0; k < facet->numBorders + 1; k++) {
            if(k < facet->numBorders) {
                planenum = facet->borderPlanes[k];
                inward = facet->borderInward[k];
            } else {
                planenum = facet->surfacePlane;
                inward = false;
            }

            Vector4Copy(pc->planes[planenum].plane, plane);

            if(inward) {
                VectorInverse(plane);
                plane[3] = -plane[3];
            }

            w = BaseWindingForPlane(plane, plane[3]);

            for(j = 0; j < facet->numBorders + 1 && w; j++) {
                if(j < facet->numBorders) {
                    curplanenum = facet->borderPlanes[j];
                    curinward = facet->borderInward[j];
                } else {
                    curplanenum = facet->surfacePlane;
                    curinward = false;
                }

                if(curplanenum == planenum) {
                    continue;
                }

                Vector4Copy(pc->planes[curplanenum].plane, plane);

                if(!curinward) {
                    VectorInverse(plane);
                    plane[3] = -plane[3];
                }

                ChopWindingInPlace(&w, plane, plane[3], 0.1f);
            }

            if(w) {
                if(facet == debugFacet) {
                    drawPoly(4, w->numpoints, w->p[0]);

                    if(developer->integer) {
                        common->Printf("blue facet has %d border planes\n", facet->numBorders);
                    }
                } else {
                    drawPoly(1, w->numpoints, w->p[0]);
                }

                FreeWinding(w);
            } else {
                if(developer->integer) {
                    common->Printf("winding chopped away by border planes\n");
                }
            }
        }
    }
}
