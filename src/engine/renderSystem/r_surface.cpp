////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2005 Id Software, Inc.
// Copyright(C) 2000 - 2013 Darklegion Development
// Copyright(C) 2011 - 2023 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of OpenWolf.
//
// OpenWolf is free software; you can redistribute it
// and / or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3 of the License,
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
// File name:   r_surface.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.hpp>

/*
THIS ENTIRE FILE IS BACK END

backEnd.currentEntity will be valid.

Tess_Begin has already been called for the surface's shader.

The modelview matrix will be set.

It is safe to actually issue drawing commands here if you don't want to
use the shader system.
*/

/*
==============
RB_CheckOverflow
==============
*/
void RB_CheckOverflow(sint verts, sint indexes) {
    if(tess.numVertexes + verts < SHADER_MAX_VERTEXES
            && tess.numIndexes + indexes < SHADER_MAX_INDEXES) {
        return;
    }

    RB_EndSurface();

    if(verts >= SHADER_MAX_VERTEXES) {
        common->Error(ERR_DROP, "RB_CheckOverflow: verts > MAX (%d > %d)", verts,
                      SHADER_MAX_VERTEXES);
    }

    if(indexes >= SHADER_MAX_INDEXES) {
        common->Error(ERR_DROP, "RB_CheckOverflow: indices > MAX (%d > %d)",
                      indexes,
                      SHADER_MAX_INDEXES);
    }

    RB_BeginSurface(tess.shader, tess.fogNum, tess.cubemapIndex);
}

void RB_CheckVao(vao_t *vao) {
    if(vao != glState.currentVao) {
        RB_EndSurface();
        RB_BeginSurface(tess.shader, tess.fogNum, tess.cubemapIndex);

        R_BindVao(vao);
    }

    if(vao != tess.vao) {
        tess.useInternalVao = false;
    }
}


/*
==============
RB_AddQuadStampExt
==============
*/
void RB_AddQuadStampExt(vec3_t origin, vec3_t left, vec3_t up,
                        float32 color[4], float32 s1, float32 t1, float32 s2, float32 t2) {
    vec3_t      normal;
    schar16     iNormal[4];
    uchar16    iColor[4];
    sint            ndx;

    RB_CheckVao(tess.vao);

    RB_CHECKOVERFLOW(4, 6);

    ndx = tess.numVertexes;

    // triangle indexes for a simple quad
    tess.indexes[ tess.numIndexes ] = ndx;
    tess.indexes[ tess.numIndexes + 1 ] = ndx + 1;
    tess.indexes[ tess.numIndexes + 2 ] = ndx + 3;

    tess.indexes[ tess.numIndexes + 3 ] = ndx + 3;
    tess.indexes[ tess.numIndexes + 4 ] = ndx + 1;
    tess.indexes[ tess.numIndexes + 5 ] = ndx + 2;

    tess.xyz[ndx][0] = origin[0] + left[0] + up[0];
    tess.xyz[ndx][1] = origin[1] + left[1] + up[1];
    tess.xyz[ndx][2] = origin[2] + left[2] + up[2];

    tess.xyz[ndx + 1][0] = origin[0] - left[0] + up[0];
    tess.xyz[ndx + 1][1] = origin[1] - left[1] + up[1];
    tess.xyz[ndx + 1][2] = origin[2] - left[2] + up[2];

    tess.xyz[ndx + 2][0] = origin[0] - left[0] - up[0];
    tess.xyz[ndx + 2][1] = origin[1] - left[1] - up[1];
    tess.xyz[ndx + 2][2] = origin[2] - left[2] - up[2];

    tess.xyz[ndx + 3][0] = origin[0] + left[0] - up[0];
    tess.xyz[ndx + 3][1] = origin[1] + left[1] - up[1];
    tess.xyz[ndx + 3][2] = origin[2] + left[2] - up[2];


    // constant normal all the way around
    VectorSubtract(vec3_origin, backEnd.viewParms.orientation.axis[0], normal);

    R_VaoPackNormal(iNormal, normal);

    VectorCopy4(iNormal, tess.normal[ndx]);
    VectorCopy4(iNormal, tess.normal[ndx + 1]);
    VectorCopy4(iNormal, tess.normal[ndx + 2]);
    VectorCopy4(iNormal, tess.normal[ndx + 3]);

    // standard square texture coordinates
    VectorSet2(tess.texCoords[ndx], s1, t1);
    VectorSet2(tess.lightCoords[ndx], s1, t1);

    VectorSet2(tess.texCoords[ndx + 1], s2, t1);
    VectorSet2(tess.lightCoords[ndx + 1], s2, t1);

    VectorSet2(tess.texCoords[ndx + 2], s2, t2);
    VectorSet2(tess.lightCoords[ndx + 2], s2, t2);

    VectorSet2(tess.texCoords[ndx + 3], s1, t2);
    VectorSet2(tess.lightCoords[ndx + 3], s1, t2);

    // constant color all the way around
    // should this be identity and let the shader specify from entity?

    R_VaoPackColor(iColor, color);

    VectorCopy4(iColor, tess.color[ndx]);
    VectorCopy4(iColor, tess.color[ndx + 1]);
    VectorCopy4(iColor, tess.color[ndx + 2]);
    VectorCopy4(iColor, tess.color[ndx + 3]);

    tess.numVertexes += 4;
    tess.numIndexes += 6;
}

/*
==============
RB_AddQuadStamp
==============
*/
void RB_AddQuadStamp(vec3_t origin, vec3_t left, vec3_t up,
                     float32 color[4]) {
    RB_AddQuadStampExt(origin, left, up, color, 0, 0, 1, 1);
}


/*
==============
RB_InstantQuad

based on Tess_InstantQuad from xreal
==============
*/
void RB_InstantQuad2(vec4_t quadVerts[4], vec2_t texCoords[4]) {
    GLimp_LogComment("--- RB_InstantQuad2 ---\n");

    tess.numVertexes = 0;
    tess.numIndexes = 0;
    tess.firstIndex = 0;

    VectorCopy4(quadVerts[0], tess.xyz[tess.numVertexes]);
    VectorCopy2(texCoords[0], tess.texCoords[tess.numVertexes]);
    tess.numVertexes++;

    VectorCopy4(quadVerts[1], tess.xyz[tess.numVertexes]);
    VectorCopy2(texCoords[1], tess.texCoords[tess.numVertexes]);
    tess.numVertexes++;

    VectorCopy4(quadVerts[2], tess.xyz[tess.numVertexes]);
    VectorCopy2(texCoords[2], tess.texCoords[tess.numVertexes]);
    tess.numVertexes++;

    VectorCopy4(quadVerts[3], tess.xyz[tess.numVertexes]);
    VectorCopy2(texCoords[3], tess.texCoords[tess.numVertexes]);
    tess.numVertexes++;

    tess.indexes[tess.numIndexes++] = 0;
    tess.indexes[tess.numIndexes++] = 1;
    tess.indexes[tess.numIndexes++] = 2;
    tess.indexes[tess.numIndexes++] = 0;
    tess.indexes[tess.numIndexes++] = 2;
    tess.indexes[tess.numIndexes++] = 3;

    RB_UpdateTessVao(ATTR_POSITION | ATTR_TEXCOORD);

    R_DrawElements(tess.numIndexes, tess.firstIndex);

    tess.numIndexes = 0;
    tess.numVertexes = 0;
    tess.firstIndex = 0;
}


void RB_InstantQuad(vec4_t quadVerts[4]) {
    vec2_t texCoords[4];

    VectorSet2(texCoords[0], 0.0f, 0.0f);
    VectorSet2(texCoords[1], 1.0f, 0.0f);
    VectorSet2(texCoords[2], 1.0f, 1.0f);
    VectorSet2(texCoords[3], 0.0f, 1.0f);

    GLSL_BindProgram(&tr.textureColorShader);

    GLSL_SetUniformMat4(&tr.textureColorShader,
                        UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
    GLSL_SetUniformVec4(&tr.textureColorShader, UNIFORM_COLOR, colorWhite);

    RB_InstantQuad2(quadVerts, texCoords);
}


/*
==============
RB_SurfaceSprite
==============
*/
static void RB_SurfaceSprite(void) {
    vec3_t      left, up;
    float32     radius;
    float32         colors[4];
    trRefEntity_t  *ent = backEnd.currentEntity;

    // calculate the xyz locations for the four corners
    radius = ent->e.radius;

    if(ent->e.rotation == 0) {
        VectorScale(backEnd.viewParms.orientation.axis[1], radius, left);
        VectorScale(backEnd.viewParms.orientation.axis[2], radius, up);
    } else {
        float32 s, c;
        float32 ang;

        ang = M_PI * ent->e.rotation / 180;
        s = sin(ang);
        c = cos(ang);

        VectorScale(backEnd.viewParms.orientation.axis[1], c * radius, left);
        VectorMA(left, -s * radius, backEnd.viewParms.orientation.axis[2], left);

        VectorScale(backEnd.viewParms.orientation.axis[2], c * radius, up);
        VectorMA(up, s * radius, backEnd.viewParms.orientation.axis[1], up);
    }

    if(backEnd.viewParms.isMirror) {
        VectorSubtract(vec3_origin, left, left);
    }

    VectorScale4(ent->e.shaderRGBA, 1.0f / 255.0f, colors);

    RB_AddQuadStamp(ent->e.origin, left, up, colors);
}


/*
=============
RB_SurfacePolychain
=============
*/
static void RB_SurfacePolychain(srfPoly_t *p) {
    sint i, numv;

    RB_CheckVao(tess.vao);

    if(p->numVerts == 2 && p->hShader == 0) {
        qglBegin(GL_LINES);
        qglColor3f(p->verts[0].modulate[0], p->verts[0].modulate[1],
                   p->verts[0].modulate[2]);
        qglVertex3d(p->verts[0].xyz[0], p->verts[0].xyz[1], p->verts[0].xyz[2]);

        qglColor3f(p->verts[1].modulate[0], p->verts[1].modulate[1],
                   p->verts[1].modulate[2]);
        qglVertex3d(p->verts[1].xyz[0], p->verts[1].xyz[0], p->verts[1].xyz[0]);
        qglEnd();

        return;
    }

    RB_CHECKOVERFLOW(p->numVerts, 3 * (p->numVerts - 2));

    // fan triangles into the tess array
    numv = tess.numVertexes;

    for(i = 0; i < p->numVerts; i++) {
        VectorCopy(p->verts[i].xyz, tess.xyz[numv]);
        tess.texCoords[numv][0] = p->verts[i].st[0];
        tess.texCoords[numv][1] = p->verts[i].st[1];
        tess.color[numv][0] = static_cast<sint>(p->verts[i].modulate[0]) * 257;
        tess.color[numv][1] = static_cast<sint>(p->verts[i].modulate[1]) * 257;
        tess.color[numv][2] = static_cast<sint>(p->verts[i].modulate[2]) * 257;
        tess.color[numv][3] = static_cast<sint>(p->verts[i].modulate[3]) * 257;

        numv++;
    }

    // generate fan indexes into the tess array
    for(i = 0; i < p->numVerts - 2; i++) {
        tess.indexes[tess.numIndexes + 0] = tess.numVertexes;
        tess.indexes[tess.numIndexes + 1] = tess.numVertexes + i + 1;
        tess.indexes[tess.numIndexes + 2] = tess.numVertexes + i + 2;
        tess.numIndexes += 3;
    }

    tess.numVertexes = numv;
}

static void RB_SurfaceVertsAndIndexes(sint numVerts, srfVert_t *verts,
                                      sint numIndexes, uint *indexes, sint dlightBits, sint pshadowBits) {
    sint             i;
    uint      *inIndex;
    srfVert_t      *dv;
    float32          *xyz, *texCoords, *lightCoords;
    schar16        *lightdir;
    schar16        *normal;
    schar16        *tangent;
    uint      *outIndex;
    uchar16       *color;

    RB_CheckVao(tess.vao);

    RB_CHECKOVERFLOW(numVerts, numIndexes);

    inIndex = indexes;
    outIndex = &tess.indexes[ tess.numIndexes ];

    for(i = 0 ; i < numIndexes ; i++) {
        *outIndex++ = tess.numVertexes + *inIndex++;
    }

    tess.numIndexes += numIndexes;

    if(tess.shader->vertexAttribs & ATTR_POSITION) {
        dv = verts;
        xyz = tess.xyz[ tess.numVertexes ];

        for(i = 0 ; i < numVerts ; i++, dv++, xyz += 4) {
            VectorCopy(dv->xyz, xyz);
        }
    }

    if(tess.shader->vertexAttribs & ATTR_NORMAL) {
        dv = verts;
        normal = tess.normal[ tess.numVertexes ];

        for(i = 0 ; i < numVerts ; i++, dv++, normal += 4) {
            VectorCopy4(dv->normal, normal);
        }
    }

    if(tess.shader->vertexAttribs & ATTR_TANGENT) {
        dv = verts;
        tangent = tess.tangent[ tess.numVertexes ];

        for(i = 0 ; i < numVerts ; i++, dv++, tangent += 4) {
            VectorCopy4(dv->tangent, tangent);
        }
    }

    if(tess.shader->vertexAttribs & ATTR_TEXCOORD) {
        dv = verts;
        texCoords = tess.texCoords[tess.numVertexes];

        for(i = 0 ; i < numVerts ; i++, dv++, texCoords += 2) {
            VectorCopy2(dv->st, texCoords);
        }
    }

    if(tess.shader->vertexAttribs & ATTR_LIGHTCOORD) {
        dv = verts;
        lightCoords = tess.lightCoords[ tess.numVertexes ];

        for(i = 0 ; i < numVerts ; i++, dv++, lightCoords += 2) {
            VectorCopy2(dv->lightmap, lightCoords);
        }
    }

    if(tess.shader->vertexAttribs & ATTR_COLOR) {
        dv = verts;
        color = tess.color[ tess.numVertexes ];

        for(i = 0 ; i < numVerts ; i++, dv++, color += 4) {
            VectorCopy4(dv->color, color);
        }
    }

    if(tess.shader->vertexAttribs & ATTR_LIGHTDIRECTION) {
        dv = verts;
        lightdir = tess.lightdir[ tess.numVertexes ];

        for(i = 0 ; i < numVerts ; i++, dv++, lightdir += 4) {
            VectorCopy4(dv->lightdir, lightdir);
        }
    }

#if 0  // nothing even uses vertex dlightbits

    for(i = 0 ; i < numVerts ; i++) {
        tess.vertexDlightBits[ tess.numVertexes + i ] = dlightBits;
    }

#endif

    tess.dlightBits |= dlightBits;
    tess.pshadowBits |= pshadowBits;

    tess.numVertexes += numVerts;
}

static bool RB_SurfaceVaoCached(sint numVerts, srfVert_t *verts,
                                sint numIndexes, uint *indexes, sint dlightBits, sint pshadowBits) {
    bool recycleVertexBuffer = false;
    bool recycleIndexBuffer = false;
    bool endSurface = false;

    if(!(!ShaderRequiresCPUDeforms(tess.shader) && !tess.shader->isSky &&
            !tess.shader->isPortal)) {
        return false;
    }

    if(!numIndexes || !numVerts) {
        return false;
    }

    // Never use a cached surface if the surface uses vertex style colors.
    if(tess.shader->vertexAttribs & ATTR_COLOR) {
        return false;
    }

    VaoCache_BindVao();

    tess.dlightBits |= dlightBits;
    tess.pshadowBits |= pshadowBits;

    VaoCache_CheckAdd(&endSurface, &recycleVertexBuffer, &recycleIndexBuffer,
                      numVerts, numIndexes);

    if(endSurface) {
        RB_EndSurface();
        RB_BeginSurface(tess.shader, tess.fogNum, tess.cubemapIndex);
    }

    if(recycleVertexBuffer) {
        VaoCache_RecycleVertexBuffer();
    }

    if(recycleIndexBuffer) {
        VaoCache_RecycleIndexBuffer();
    }

    if(!tess.numVertexes) {
        VaoCache_InitNewSurfaceSet();
    }

    VaoCache_AddSurface(verts, numVerts, indexes, numIndexes);

    tess.numIndexes += numIndexes;
    tess.numVertexes += numVerts;
    tess.useInternalVao = false;
    tess.useCacheVao = true;

    return true;
}

/*
=============
RB_SurfaceTriangles
=============
*/
static void RB_SurfaceTriangles(srfBspSurface_t *srf) {
    if(RB_SurfaceVaoCached(srf->numVerts, srf->verts, srf->numIndexes,
                           srf->indexes, srf->dlightBits, srf->pshadowBits)) {
        return;
    }

    RB_SurfaceVertsAndIndexes(srf->numVerts, srf->verts, srf->numIndexes,
                              srf->indexes, srf->dlightBits, srf->pshadowBits);
}

/*
=============
RB_SurfaceFoliage
=============
*/
static void RB_SurfaceFoliage(srfFoliage_t *srf) {
    sint o, i;
    vec4_t distanceCull, distanceVector;
    float32 alpha, z, dist, fovScale;
    float32 *xyz;
    uchar16 srcColor[4];
    uchar16 *color;
    vec3_t local;
    foliageInstance_t *instance;
    sint numPlanes;

    // set fov scale
    fovScale = backEnd.viewParms.fovX * (1.0 / 90.0);

    // calculate distance vector
    VectorSubtract(backEnd.orientation.origin,
                   backEnd.viewParms.orientation.origin, local);
    distanceVector[0] = -backEnd.orientation.modelMatrix[2];
    distanceVector[1] = -backEnd.orientation.modelMatrix[6];
    distanceVector[2] = -backEnd.orientation.modelMatrix[10];
    distanceVector[3] = DotProduct(local,
                                   backEnd.viewParms.orientation.axis[0]);

    // attempt distance cull
    Vector4Copy(tess.shader->distanceCull, distanceCull);

    if(distanceCull[1] > 0) {
        //VectorSubtract( srf->localOrigin, viewOrigin, delta );
        //alpha = (distanceCull[ 1 ] - VectorLength( delta ) + srf->radius) * distanceCull[ 3 ];
        z = fovScale * (DotProduct(srf->origin,
                                   distanceVector) + distanceVector[3] - srf->radius);
        alpha = (distanceCull[1] - z) * distanceCull[3];

        if(alpha < distanceCull[2]) {
            return;
        }
    }

    numPlanes = (backEnd.viewParms.flags & VPF_FARPLANEFRUSTUM) ? 5 : 4;

    // iterate through origin list
    instance = srf->instances;

    for(o = 0; o < srf->numInstances; o++, instance++) {
        // fade alpha based on distance between inner and outer radii
        if(distanceCull[1] > 0.0f) {
            // calculate z distance
            z = fovScale * (DotProduct(instance->origin,
                                       distanceVector) + distanceVector[3]);

            if(z < -64.0f) {   // epsilon so close-by foliage doesn't pop in and out
                continue;
            }

            // check against frustum planes
            for(i = 0; i < numPlanes; i++) {
                dist = DotProduct(instance->origin,
                                  backEnd.viewParms.frustum[i].normal) - backEnd.viewParms.frustum[i].dist;

                if(dist < -64.0) {
                    break;
                }
            }

            if(i != numPlanes) {
                continue;
            }

            // radix
            if(o & 1) {
                z *= 1.25;

                if(o & 2) {
                    z *= 1.25;
                }
            }

            // calculate alpha
            alpha = (distanceCull[1] - z) * distanceCull[3];

            if(alpha < distanceCull[2]) {
                continue;
            }

            // set color
            alpha = Com_Clamp(0, 255, alpha * 255);
        } else {
            alpha = 255;
        }

        Vector4Copy(instance->color, srcColor);
        srcColor[3] = alpha * 257;

        // clientRendererSystem->RefPrintf( PRINT_ALL, "Color: %f %f %f %f\n", instance->color[ 0 ], instance->color[ 1 ], instance->color[ 2 ], alpha );

        RB_SurfaceVertsAndIndexes(srf->numVerts, srf->verts, srf->numIndexes,
                                  srf->indexes, srf->dlightBits, srf->pshadowBits);

        // offset xyz
        xyz = tess.xyz[tess.numVertexes - srf->numVerts];

        for(i = 0; i < srf->numVerts; i++, xyz += 4) {
            VectorAdd(xyz, instance->origin, xyz);
        }

        // copy color
        color = tess.color[tess.numVertexes - srf->numVerts];

        for(i = 0; i < srf->numVerts; i++, color += 4) {
            Vector4Copy(srcColor, color);
        }
    }

    // RB_DrawBounds( srf->bounds[ 0 ], srf->bounds[ 1 ] );
}

//================================================================================

/*
==============
RB_SurfaceBeam
==============
*/
static void RB_SurfaceBeam(void) {
#define NUM_BEAM_SEGS 6
    refEntity_t *e;
    shaderProgram_t *sp = &tr.textureColorShader;
    sint    i;
    vec3_t perpvec;
    vec3_t direction, normalized_direction;
    vec3_t  start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
    vec3_t oldorigin, origin;

    e = &backEnd.currentEntity->e;

    oldorigin[0] = e->oldorigin[0];
    oldorigin[1] = e->oldorigin[1];
    oldorigin[2] = e->oldorigin[2];

    origin[0] = e->origin[0];
    origin[1] = e->origin[1];
    origin[2] = e->origin[2];

    normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
    normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
    normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

    if(VectorNormalize(normalized_direction) == 0) {
        return;
    }

    PerpendicularVector(perpvec, normalized_direction);

    VectorScale(perpvec, 4, perpvec);

    for(i = 0; i < NUM_BEAM_SEGS ; i++) {
        RotatePointAroundVector(start_points[i], normalized_direction, perpvec,
                                (360.0f / NUM_BEAM_SEGS)*i);
        //      VectorAdd( start_points[i], origin, start_points[i] );
        VectorAdd(start_points[i], direction, end_points[i]);
    }

    GL_BindToTMU(tr.whiteImage, TB_COLORMAP);

    GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);

    // FIXME: Quake3 doesn't use this, so I never tested it
    tess.numVertexes = 0;
    tess.numIndexes = 0;
    tess.firstIndex = 0;

    for(i = 0; i <= NUM_BEAM_SEGS; i++) {
        VectorCopy(start_points[ i % NUM_BEAM_SEGS ],
                   tess.xyz[tess.numVertexes++]);
        VectorCopy(end_points  [ i % NUM_BEAM_SEGS ],
                   tess.xyz[tess.numVertexes++]);
    }

    for(i = 0; i < NUM_BEAM_SEGS; i++) {
        tess.indexes[tess.numIndexes++] =       i      * 2;
        tess.indexes[tess.numIndexes++] = (i + 1) * 2;
        tess.indexes[tess.numIndexes++] = 1  +  i      * 2;

        tess.indexes[tess.numIndexes++] = 1  +  i      * 2;
        tess.indexes[tess.numIndexes++] = (i + 1) * 2;
        tess.indexes[tess.numIndexes++] = 1  + (i + 1) * 2;
    }

    // FIXME: A lot of this can probably be removed for speed, and refactored into a more convenient function
    RB_UpdateTessVao(ATTR_POSITION);

    GLSL_BindProgram(sp);

    GLSL_SetUniformMat4(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX,
                        glState.modelviewProjection);

    GLSL_SetUniformVec4(sp, UNIFORM_COLOR, colorRed);

    GLSL_SetUniformInt(sp, UNIFORM_ALPHATEST, 0);

    R_DrawElements(tess.numIndexes, tess.firstIndex);

    tess.numIndexes = 0;
    tess.numVertexes = 0;
    tess.firstIndex = 0;
}

//================================================================================

static void DoRailCore(const vec3_t start, const vec3_t end,
                       const vec3_t up, float32 len, float32 spanWidth) {
    float32     spanWidth2;
    sint            vbase;
    float32     t = len / 256.0f;

    RB_CheckVao(tess.vao);

    RB_CHECKOVERFLOW(4, 6);

    vbase = tess.numVertexes;

    spanWidth2 = -spanWidth;

    // FIXME: use quad stamp?
    VectorMA(start, spanWidth, up, tess.xyz[tess.numVertexes]);
    tess.texCoords[tess.numVertexes][0] = 0;
    tess.texCoords[tess.numVertexes][1] = 0;
    tess.color[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0] *
                                      0.25f * 257.0f;
    tess.color[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1] *
                                      0.25f * 257.0f;
    tess.color[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2] *
                                      0.25f * 257.0f;
    tess.numVertexes++;

    VectorMA(start, spanWidth2, up, tess.xyz[tess.numVertexes]);
    tess.texCoords[tess.numVertexes][0] = 0;
    tess.texCoords[tess.numVertexes][1] = 1;
    tess.color[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0] *
                                      257;
    tess.color[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1] *
                                      257;
    tess.color[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2] *
                                      257;
    tess.numVertexes++;

    VectorMA(end, spanWidth, up, tess.xyz[tess.numVertexes]);

    tess.texCoords[tess.numVertexes][0] = t;
    tess.texCoords[tess.numVertexes][1] = 0;
    tess.color[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0] *
                                      257;
    tess.color[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1] *
                                      257;
    tess.color[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2] *
                                      257;
    tess.numVertexes++;

    VectorMA(end, spanWidth2, up, tess.xyz[tess.numVertexes]);
    tess.texCoords[tess.numVertexes][0] = t;
    tess.texCoords[tess.numVertexes][1] = 1;
    tess.color[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0] *
                                      257;
    tess.color[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1] *
                                      257;
    tess.color[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2] *
                                      257;
    tess.numVertexes++;

    tess.indexes[tess.numIndexes++] = vbase;
    tess.indexes[tess.numIndexes++] = vbase + 1;
    tess.indexes[tess.numIndexes++] = vbase + 2;

    tess.indexes[tess.numIndexes++] = vbase + 2;
    tess.indexes[tess.numIndexes++] = vbase + 1;
    tess.indexes[tess.numIndexes++] = vbase + 3;
}

static void DoRailDiscs(sint numSegs, const vec3_t start, const vec3_t dir,
                        const vec3_t right, const vec3_t up) {
    sint i;
    vec3_t  pos[4];
    vec3_t  v;
    sint        spanWidth = r_railWidth->integer;
    float32 c, s;
    float32     scale;

    if(numSegs > 1) {
        numSegs--;
    }

    if(!numSegs) {
        return;
    }

    scale = 0.25;

    for(i = 0; i < 4; i++) {
        c = cos(DEG2RAD(45 + i * 90));
        s = sin(DEG2RAD(45 + i * 90));
        v[0] = (right[0] * c + up[0] * s) * scale * spanWidth;
        v[1] = (right[1] * c + up[1] * s) * scale * spanWidth;
        v[2] = (right[2] * c + up[2] * s) * scale * spanWidth;
        VectorAdd(start, v, pos[i]);

        if(numSegs > 1) {
            // offset by 1 segment if we're doing a long distance shot
            VectorAdd(pos[i], dir, pos[i]);
        }
    }

    RB_CheckVao(tess.vao);

    for(i = 0; i < numSegs; i++) {
        sint j;

        RB_CHECKOVERFLOW(4, 6);

        for(j = 0; j < 4; j++) {
            VectorCopy(pos[j], tess.xyz[tess.numVertexes]);
            tess.texCoords[tess.numVertexes][0] = (j < 2);
            tess.texCoords[tess.numVertexes][1] = (j && j != 3);
            tess.color[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0] *
                                              257;
            tess.color[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1] *
                                              257;
            tess.color[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2] *
                                              257;
            tess.numVertexes++;

            VectorAdd(pos[j], dir, pos[j]);
        }

        tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 0;
        tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 1;
        tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 3;
        tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 3;
        tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 1;
        tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 2;
    }
}

/*
** RB_SurfaceRailRinges
*/
static void RB_SurfaceRailRings(void) {
    refEntity_t *e;
    sint            numSegs;
    sint            len;
    vec3_t      vec;
    vec3_t      right, up;
    vec3_t      start, end;

    e = &backEnd.currentEntity->e;

    VectorCopy(e->oldorigin, start);
    VectorCopy(e->origin, end);

    // compute variables
    VectorSubtract(end, start, vec);
    len = VectorNormalize(vec);
    MakeNormalVectors(vec, right, up);
    numSegs = (len) / r_railSegmentLength->value;

    if(numSegs <= 0) {
        numSegs = 1;
    }

    VectorScale(vec, r_railSegmentLength->value, vec);

    DoRailDiscs(numSegs, start, vec, right, up);
}

/*
** RB_SurfaceRailCore
*/
static void RB_SurfaceRailCore(void) {
    refEntity_t *e;
    sint            len;
    vec3_t      right;
    vec3_t      vec;
    vec3_t      start, end;
    vec3_t      v1, v2;

    e = &backEnd.currentEntity->e;

    VectorCopy(e->oldorigin, start);
    VectorCopy(e->origin, end);

    VectorSubtract(end, start, vec);
    len = VectorNormalize(vec);

    // compute side vector
    VectorSubtract(start, backEnd.viewParms.orientation.origin, v1);
    VectorNormalize(v1);
    VectorSubtract(end, backEnd.viewParms.orientation.origin, v2);
    VectorNormalize(v2);
    CrossProduct(v1, v2, right);
    VectorNormalize(right);

    DoRailCore(start, end, right, len, r_railCoreWidth->integer);
}

/*
** RB_SurfaceLightningBolt
*/
static void RB_SurfaceLightningBolt(void) {
    refEntity_t *e;
    sint            len;
    vec3_t      right;
    vec3_t      vec;
    vec3_t      start, end;
    vec3_t      v1, v2;
    sint            i;

    e = &backEnd.currentEntity->e;

    VectorCopy(e->oldorigin, end);
    VectorCopy(e->origin, start);

    // compute variables
    VectorSubtract(end, start, vec);
    len = VectorNormalize(vec);

    // compute side vector
    VectorSubtract(start, backEnd.viewParms.orientation.origin, v1);
    VectorNormalize(v1);
    VectorSubtract(end, backEnd.viewParms.orientation.origin, v2);
    VectorNormalize(v2);
    CrossProduct(v1, v2, right);
    VectorNormalize(right);

    for(i = 0 ; i < 4 ; i++) {
        vec3_t  temp;

        DoRailCore(start, end, right, len, 8);
        RotatePointAroundVector(temp, vec, right, 45);
        VectorCopy(temp, right);
    }
}


static void LerpMeshVertexes(mdvSurface_t *surf, float32 backlerp) {
    float32 *outXyz;
    schar16 *outNormal, *outTangent;
    mdvVertex_t *newVerts;
    sint        vertNum;

    newVerts = surf->verts + backEnd.currentEntity->e.frame * surf->numVerts;

    outXyz =     tess.xyz[tess.numVertexes];
    outNormal =  tess.normal[tess.numVertexes];
    outTangent = tess.tangent[tess.numVertexes];

    if(backlerp == 0) {
        //
        // just copy the vertexes
        //

        for(vertNum = 0 ; vertNum < surf->numVerts ; vertNum++) {
            VectorCopy(newVerts->xyz,    outXyz);
            VectorCopy4(newVerts->normal, outNormal);
            VectorCopy4(newVerts->tangent, outTangent);

            newVerts++;
            outXyz += 4;
            outNormal += 4;
            outTangent += 4;
        }
    } else {
        //
        // interpolate and copy the vertex and normal
        //

        mdvVertex_t *oldVerts;

        oldVerts = surf->verts + backEnd.currentEntity->e.oldframe *
                   surf->numVerts;

        for(vertNum = 0 ; vertNum < surf->numVerts ; vertNum++) {
            VectorLerp(newVerts->xyz,    oldVerts->xyz,    backlerp, outXyz);

            outNormal[0] = (schar16)(newVerts->normal[0] * (1.0f - backlerp) +
                                     oldVerts->normal[0] * backlerp);
            outNormal[1] = (schar16)(newVerts->normal[1] * (1.0f - backlerp) +
                                     oldVerts->normal[1] * backlerp);
            outNormal[2] = (schar16)(newVerts->normal[2] * (1.0f - backlerp) +
                                     oldVerts->normal[2] * backlerp);
            outNormal[3] = 0;

            outTangent[0] = (schar16)(newVerts->tangent[0] * (1.0f - backlerp) +
                                      oldVerts->tangent[0] * backlerp);
            outTangent[1] = (schar16)(newVerts->tangent[1] * (1.0f - backlerp) +
                                      oldVerts->tangent[1] * backlerp);
            outTangent[2] = (schar16)(newVerts->tangent[2] * (1.0f - backlerp) +
                                      oldVerts->tangent[2] * backlerp);
            outTangent[3] = newVerts->tangent[3];

            newVerts++;
            oldVerts++;
            outXyz += 4;
            outNormal += 4;
            outTangent += 4;
        }
    }

}


/*
=============
RB_SurfaceMesh
=============
*/
static void RB_SurfaceMesh(mdvSurface_t *surface) {
    sint                j;
    float32         backlerp;
    mdvSt_t            *texCoords;
    sint                Bob, Doug;
    sint                numVerts;

    if(backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame) {
        backlerp = 0;
    } else {
        backlerp = backEnd.currentEntity->e.backlerp;
    }

    RB_CheckVao(tess.vao);

    RB_CHECKOVERFLOW(surface->numVerts, surface->numIndexes);

    LerpMeshVertexes(surface, backlerp);

    Bob = tess.numIndexes;
    Doug = tess.numVertexes;

    for(j = 0 ; j < surface->numIndexes ; j++) {
        tess.indexes[Bob + j] = Doug + surface->indexes[j];
    }

    tess.numIndexes += surface->numIndexes;

    texCoords = surface->st;

    numVerts = surface->numVerts;

    for(j = 0; j < numVerts; j++) {
        tess.texCoords[Doug + j][0] = texCoords[j].st[0];
        tess.texCoords[Doug + j][1] = texCoords[j].st[1];
        // FIXME: fill in lightmapST for completeness?
    }

    tess.numVertexes += surface->numVerts;

}

/*
=============
RB_SurfaceObject
=============
*/
void RB_SurfaceObject(objectSurface_t *surf) {
    sint index = tess.numIndexes, i = 0;
    sint vindex = tess.numVertexes;
    objectFace_t *face;

    for(face = surf->faces; i < surf->numFaces; i++, face++) {
        VectorSet4(tess.xyz[vindex + 0], (*face)[0].vertex[0],
                   (*face)[0].vertex[1], (*face)[0].vertex[2], 0);
        VectorSet4(tess.xyz[vindex + 1], (*face)[1].vertex[0],
                   (*face)[1].vertex[1], (*face)[1].vertex[2], 0);
        VectorSet4(tess.xyz[vindex + 2], (*face)[2].vertex[0],
                   (*face)[2].vertex[1], (*face)[2].vertex[2], 0);
        tess.texCoords[vindex + 0][0] = (*face)[0].uv[0];
        tess.texCoords[vindex + 0][1] = (*face)[0].uv[1];
        tess.texCoords[vindex + 1][0] = (*face)[1].uv[0];
        tess.texCoords[vindex + 1][1] = (*face)[1].uv[1];
        tess.texCoords[vindex + 2][0] = (*face)[2].uv[0];
        tess.texCoords[vindex + 2][1] = (*face)[2].uv[1];
        VectorSet4(tess.normal[vindex + 0], (*face)[0].normal[0],
                   (*face)[0].normal[1], (*face)[0].normal[2], 0);
        VectorSet4(tess.normal[vindex + 1], (*face)[1].normal[0],
                   (*face)[1].normal[1], (*face)[1].normal[2], 0);
        VectorSet4(tess.normal[vindex + 2], (*face)[2].normal[0],
                   (*face)[2].normal[1], (*face)[2].normal[2], 0);
        tess.indexes[index + 0] = vindex + 0;
        tess.indexes[index + 1] = vindex + 1;
        tess.indexes[index + 2] = vindex + 2;
        index += 3;
        vindex += 3;
        tess.numIndexes += 3;
        tess.numVertexes += 3;
    }
}


/*
==============
RB_SurfaceFace
==============
*/
static void RB_SurfaceFace(srfBspSurface_t *srf) {
    if(RB_SurfaceVaoCached(srf->numVerts, srf->verts, srf->numIndexes,
                           srf->indexes, srf->dlightBits, srf->pshadowBits)) {
        return;
    }

    RB_SurfaceVertsAndIndexes(srf->numVerts, srf->verts, srf->numIndexes,
                              srf->indexes, srf->dlightBits, srf->pshadowBits);
}


static float32  LodErrorForVolume(vec3_t local, float32 radius) {
    vec3_t      world;
    float32     d;

    // never let it go negative
    if(r_lodCurveError->value < 0) {
        return 0;
    }

    world[0] = local[0] * backEnd.orientation.axis[0][0] + local[1] *
               backEnd.orientation.axis[1][0] +
               local[2] * backEnd.orientation.axis[2][0] + backEnd.orientation.origin[0];
    world[1] = local[0] * backEnd.orientation.axis[0][1] + local[1] *
               backEnd.orientation.axis[1][1] +
               local[2] * backEnd.orientation.axis[2][1] + backEnd.orientation.origin[1];
    world[2] = local[0] * backEnd.orientation.axis[0][2] + local[1] *
               backEnd.orientation.axis[1][2] +
               local[2] * backEnd.orientation.axis[2][2] + backEnd.orientation.origin[2];

    VectorSubtract(world, backEnd.viewParms.orientation.origin, world);
    d = DotProduct(world, backEnd.viewParms.orientation.axis[0]);

    if(d < 0) {
        d = -d;
    }

    d -= radius;

    if(d < 1) {
        d = 1;
    }

    return r_lodCurveError->value / d;
}

/*
=============
RB_SurfaceGrid

Just copy the grid of points and triangulate
=============
*/
static void RB_SurfaceGrid(srfBspSurface_t *srf) {
    sint        i, j;
    float32    *xyz;
    float32    *texCoords, *lightCoords;
    schar16 *normal;
    schar16 *tangent;
    uchar16 *color;
    schar16 *lightdir;
    srfVert_t  *dv;
    sint        rows, irows, vrows;
    sint        used;
    sint        widthTable[MAX_GRID_SIZE];
    sint        heightTable[MAX_GRID_SIZE];
    float32 lodError;
    sint        lodWidth, lodHeight;
    sint        numVertexes;
    sint        dlightBits;
    sint     pshadowBits;
    //sint      *vDlightBits;

    if(RB_SurfaceVaoCached(srf->numVerts, srf->verts, srf->numIndexes,
                           srf->indexes, srf->dlightBits, srf->pshadowBits)) {
        return;
    }

    RB_CheckVao(tess.vao);

    dlightBits = srf->dlightBits;
    tess.dlightBits |= dlightBits;

    pshadowBits = srf->pshadowBits;
    tess.pshadowBits |= pshadowBits;

    // determine the allowable discrepance
    lodError = LodErrorForVolume(srf->lodOrigin, srf->lodRadius);

    // determine which rows and columns of the subdivision
    // we are actually going to use
    widthTable[0] = 0;
    lodWidth = 1;

    for(i = 1 ; i < srf->width - 1 ; i++) {
        if(srf->widthLodError[i] <= lodError) {
            widthTable[lodWidth] = i;
            lodWidth++;
        }
    }

    widthTable[lodWidth] = srf->width - 1;
    lodWidth++;

    heightTable[0] = 0;
    lodHeight = 1;

    for(i = 1 ; i < srf->height - 1 ; i++) {
        if(srf->heightLodError[i] <= lodError) {
            heightTable[lodHeight] = i;
            lodHeight++;
        }
    }

    heightTable[lodHeight] = srf->height - 1;
    lodHeight++;


    // very large grids may have more points or indexes than can be fit
    // in the tess structure, so we may have to issue it in multiple passes

    used = 0;

    while(used < lodHeight - 1) {
        // see how many rows of both verts and indexes we can add without overflowing
        do {
            vrows = (SHADER_MAX_VERTEXES - tess.numVertexes) / lodWidth;
            irows = (SHADER_MAX_INDEXES - tess.numIndexes) / (lodWidth * 6);

            // if we don't have enough space for at least one strip, flush the buffer
            if(vrows < 2 || irows < 1) {
                RB_EndSurface();
                RB_BeginSurface(tess.shader, tess.fogNum, tess.cubemapIndex);
            } else {
                break;
            }
        } while(1);

        rows = irows;

        if(vrows < irows + 1) {
            rows = vrows - 1;
        }

        if(used + rows > lodHeight) {
            rows = lodHeight - used;
        }

        numVertexes = tess.numVertexes;

        xyz = tess.xyz[numVertexes];
        normal = tess.normal[numVertexes];
        tangent = tess.tangent[numVertexes];
        texCoords = tess.texCoords[numVertexes];
        lightCoords = tess.lightCoords[numVertexes];
        color = tess.color[numVertexes];
        lightdir = tess.lightdir[numVertexes];
        //vDlightBits = &tess.vertexDlightBits[numVertexes];

        for(i = 0 ; i < rows ; i++) {
            for(j = 0 ; j < lodWidth ; j++) {
                dv = srf->verts + heightTable[ used + i ] * srf->width
                     + widthTable[ j ];

                if(tess.shader->vertexAttribs & ATTR_POSITION) {
                    VectorCopy(dv->xyz, xyz);
                    xyz += 4;
                }

                if(tess.shader->vertexAttribs & ATTR_NORMAL) {
                    VectorCopy4(dv->normal, normal);
                    normal += 4;
                }

                if(tess.shader->vertexAttribs & ATTR_TANGENT) {
                    VectorCopy4(dv->tangent, tangent);
                    tangent += 4;
                }

                if(tess.shader->vertexAttribs & ATTR_TEXCOORD) {
                    VectorCopy2(dv->st, texCoords);
                    texCoords += 2;
                }

                if(tess.shader->vertexAttribs & ATTR_LIGHTCOORD) {
                    VectorCopy2(dv->lightmap, lightCoords);
                    lightCoords += 2;
                }

                if(tess.shader->vertexAttribs & ATTR_COLOR) {
                    VectorCopy4(dv->color, color);
                    color += 4;
                }

                if(tess.shader->vertexAttribs & ATTR_LIGHTDIRECTION) {
                    VectorCopy4(dv->lightdir, lightdir);
                    lightdir += 4;
                }

                //*vDlightBits++ = dlightBits;
            }
        }


        // add the indexes
        {
            sint        numIndexes;
            sint        w, h;

            h = rows - 1;
            w = lodWidth - 1;
            numIndexes = tess.numIndexes;

            for(i = 0 ; i < h ; i++) {
                for(j = 0 ; j < w ; j++) {
                    sint        v1, v2, v3, v4;

                    // vertex order to be reckognized as tristrips
                    v1 = numVertexes + i * lodWidth + j + 1;
                    v2 = v1 - 1;
                    v3 = v2 + lodWidth;
                    v4 = v3 + 1;

                    tess.indexes[numIndexes] = v2;
                    tess.indexes[numIndexes + 1] = v3;
                    tess.indexes[numIndexes + 2] = v1;

                    tess.indexes[numIndexes + 3] = v1;
                    tess.indexes[numIndexes + 4] = v3;
                    tess.indexes[numIndexes + 5] = v4;
                    numIndexes += 6;
                }
            }

            tess.numIndexes = numIndexes;
        }

        tess.numVertexes += rows * lodWidth;

        used += rows - 1;
    }
}


/*
===========================================================================

nullptr MODEL

===========================================================================
*/

/*
===================
RB_SurfaceAxis

Draws x/y/z lines from the origin for orientation debugging
===================
*/
static void RB_SurfaceAxis(void) {
    // FIXME: implement this
#if 0
    GL_BindToTMU(tr.whiteImage, TB_COLORMAP);
    GL_State(GLS_DEFAULT);
    qglLineWidth(3);
    qglBegin(GL_LINES);
    qglColor3f(1, 0, 0);
    qglVertex3f(0, 0, 0);
    qglVertex3f(16, 0, 0);
    qglColor3f(0, 1, 0);
    qglVertex3f(0, 0, 0);
    qglVertex3f(0, 16, 0);
    qglColor3f(0, 0, 1);
    qglVertex3f(0, 0, 0);
    qglVertex3f(0, 0, 16);
    qglEnd();
    qglLineWidth(1);
#endif
}

//===========================================================================

/*
====================
RB_SurfaceEntity

Entities that have a single procedurally generated surface
====================
*/
static void RB_SurfaceEntity(surfaceType_t *surfType) {
    switch(backEnd.currentEntity->e.reType) {
        case RT_SPRITE:
            RB_SurfaceSprite();
            break;

        case RT_BEAM:
            RB_SurfaceBeam();
            break;

        case RT_RAIL_CORE:
            RB_SurfaceRailCore();
            break;

        case RT_RAIL_RINGS:
            RB_SurfaceRailRings();
            break;

        case RT_LIGHTNING:
            RB_SurfaceLightningBolt();
            break;

        default:
            RB_SurfaceAxis();
            break;
    }
}

static void RB_SurfaceBad(surfaceType_t *surfType) {
    clientRendererSystem->RefPrintf(PRINT_ALL, "Bad surface tesselated.\n");
}

static void RB_SurfaceFlare(srfFlare_t *surf) {
    if(r_flares->integer) {
        RB_AddFlare(surf, tess.fogNum, surf->origin, surf->color, surf->normal);
    }
}

void RB_SurfaceVaoMdvMesh(srfVaoMdvMesh_t *surface) {
    //mdvModel_t     *mdvModel;
    //mdvSurface_t   *mdvSurface;
    refEntity_t    *refEnt;

    GLimp_LogComment("--- RB_SurfaceVaoMdvMesh ---\n");

    if(ShaderRequiresCPUDeforms(tess.shader)) {
        RB_SurfaceMesh(surface->mdvSurface);
        return;
    }

    if(!surface->vao) {
        return;
    }

    //RB_CheckVao(surface->vao);
    RB_EndSurface();
    RB_BeginSurface(tess.shader, tess.fogNum, tess.cubemapIndex);

    R_BindVao(surface->vao);

    tess.useInternalVao = false;

    tess.numIndexes = surface->numIndexes;
    tess.numVertexes = surface->numVerts;

    //mdvModel = surface->mdvModel;
    //mdvSurface = surface->mdvSurface;

    refEnt = &backEnd.currentEntity->e;

    glState.vertexAttribsInterpolation = (refEnt->oldframe == refEnt->frame) ?
                                         0.0f : refEnt->backlerp;

    if(surface->mdvModel->numFrames > 1) {
        sint frameOffset, attribIndex;
        vaoAttrib_t *vAtb;

        glState.vertexAnimation = true;

        if(glRefConfig.vertexArrayObject) {
            qglBindBuffer(GL_ARRAY_BUFFER, surface->vao->vertexesVBO);
        }

        frameOffset    = refEnt->frame * surface->vao->frameSize;

        attribIndex = ATTR_INDEX_POSITION;
        vAtb = &surface->vao->attribs[attribIndex];
        qglVertexAttribPointer(attribIndex, vAtb->count, vAtb->type,
                               vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset + frameOffset));

        attribIndex = ATTR_INDEX_NORMAL;
        vAtb = &surface->vao->attribs[attribIndex];
        qglVertexAttribPointer(attribIndex, vAtb->count, vAtb->type,
                               vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset + frameOffset));

        attribIndex = ATTR_INDEX_TANGENT;
        vAtb = &surface->vao->attribs[attribIndex];
        qglVertexAttribPointer(attribIndex, vAtb->count, vAtb->type,
                               vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset + frameOffset));

        frameOffset = refEnt->oldframe * surface->vao->frameSize;

        attribIndex = ATTR_INDEX_POSITION2;
        vAtb = &surface->vao->attribs[attribIndex];
        qglVertexAttribPointer(attribIndex, vAtb->count, vAtb->type,
                               vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset + frameOffset));

        attribIndex = ATTR_INDEX_NORMAL2;
        vAtb = &surface->vao->attribs[attribIndex];
        qglVertexAttribPointer(attribIndex, vAtb->count, vAtb->type,
                               vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset + frameOffset));

        attribIndex = ATTR_INDEX_TANGENT2;
        vAtb = &surface->vao->attribs[attribIndex];
        qglVertexAttribPointer(attribIndex, vAtb->count, vAtb->type,
                               vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset + frameOffset));


        if(!glRefConfig.vertexArrayObject) {
            attribIndex = ATTR_INDEX_TEXCOORD;
            vAtb = &surface->vao->attribs[attribIndex];
            qglVertexAttribPointer(attribIndex, vAtb->count, vAtb->type,
                                   vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset));
        }
    }

    RB_EndSurface();

    // So we don't lerp surfaces that shouldn't be lerped
    glState.vertexAnimation = false;
}

static void RB_SurfaceSkip(void *surf) {
}


void (*rb_surfaceTable[SF_NUM_SURFACE_TYPES])(void *) = {
    (void(*)(void *))RB_SurfaceBad,               // SF_BAD,
    (void(*)(void *))RB_SurfaceSkip,              // SF_SKIP,
    (void(*)(void *))RB_SurfaceFace,              // SF_FACE,
    (void(*)(void *))RB_SurfaceGrid,              // SF_GRID,
    (void(*)(void *))RB_SurfaceTriangles,         // SF_TRIANGLES,
    (void(*)(void *))RB_SurfaceFoliage,         // SF_FOLIAGE,
    (void(*)(void *))RB_SurfacePolychain,         // SF_POLY,
    (void(*)(void *))RB_SurfaceMesh,              // SF_MDV,
    (void(*)(void *))RB_MDRSurfaceAnim,           // SF_MDR,
    (void(*)(void *))RB_IQMSurfaceAnim,           // SF_IQM,
    (void(*)(void *))RB_SurfaceObject,         // SF_OBJECT,
    (void(*)(void *))RB_SurfaceFlare,         // SF_FLARE,
    (void(*)(void *))RB_SurfaceEntity,            // SF_ENTITY
    (void(*)(void *))RB_SurfaceVaoMdvMesh,      // SF_VAO_MDVMESH
    (void(*)(void *))RB_IQMSurfaceAnimVao,     // SF_VAO_IQM
};
