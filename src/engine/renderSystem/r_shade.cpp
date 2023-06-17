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
// File name:   r_shade.cpp
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

  This file deals with applying shaders to surface data in the tess struct.
*/


/*
==================
R_DrawElements

==================
*/

void R_DrawElements(sint numIndexes, sint firstIndex) {
    qglDrawElements(GL_TRIANGLES, numIndexes, GL_INDEX_TYPE,
                    BUFFER_OFFSET(firstIndex * sizeof(uint)));
}


/*
=============================================================

SURFACE SHADERS

=============================================================
*/

shaderCommands_t    tess;


/*
=================
R_BindAnimatedImageToTMU

=================
*/
static void R_BindAnimatedImageToTMU(textureBundle_t *bundle, sint tmu) {
    sint i;

    if(bundle->isVideoMap) {
        clientCinemaSystem->CinemaRunCinematic(bundle->videoMapHandle);
        clientCinemaSystem->UploadCinematic(bundle->videoMapHandle);
        GL_BindToTMU(tr.scratchImage[bundle->videoMapHandle], tmu);
        return;
    }

    if(bundle->numImageAnimations <= 1) {
        GL_BindToTMU(bundle->image[0], tmu);
        return;
    }

    // it is necessary to do this messy calc to make sure animations line up
    // exactly with waveforms of the same frequency
    i = static_cast<sint>(tess.shaderTime * bundle->imageAnimationSpeed *
                          FUNCTABLE_SIZE);
    i >>= FUNCTABLE_SIZE2;

    if(i < 0) {
        i = 0;  // may happen with shader time offsets
    }

    // Windows x86 doesn't load renderer DLL with 64 bit modulus
    //index %= bundle->numImageAnimations;
    while(i >= bundle->numImageAnimations) {
        i -= bundle->numImageAnimations;
    }

    //i %= bundle->numImageAnimations;

    GL_BindToTMU(bundle->image[ i ], tmu);
}


/*
================
DrawTris

Draws triangle outlines for debugging
================
*/
static void DrawTris(shaderCommands_t *input) {
    GL_BindToTMU(tr.whiteImage, TB_COLORMAP);

    GL_State(GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE);
    qglDepthRange(0, 0);

    {
        shaderProgram_t *sp = &tr.textureColorShader;
        vec4_t color;

        GLSL_BindProgram(sp);

        GLSL_SetUniformMat4(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX,
                            glState.modelviewProjection);
        VectorSet4(color, 1, 1, 1, 1);
        GLSL_SetUniformVec4(sp, UNIFORM_COLOR, color);
        GLSL_SetUniformInt(sp, UNIFORM_ALPHATEST, 0);

        R_DrawElements(input->numIndexes, input->firstIndex);
    }

    qglDepthRange(0, 1);

    GL_State(GLS_DEFAULT);
}


/*
================
DrawNormals

Draws vertex normals for debugging
================
*/
static void DrawNormals(shaderCommands_t *input) {
    //FIXME: implement this
}

/*
==============
RB_BeginSurface

We must set some things up before beginning any tesselation,
because a surface may be forced to perform a RB_End due
to overflow.
==============
*/
void RB_BeginSurface(shader_t *shader, sint fogNum, sint cubemapIndex) {

    shader_t *state = (shader->remappedShader) ? shader->remappedShader :
                      shader;

    tess.numIndexes = 0;
    tess.firstIndex = 0;
    tess.numVertexes = 0;
    tess.shader = state;
    tess.fogNum = fogNum;
    tess.cubemapIndex = cubemapIndex;
    tess.dlightBits = 0;        // will be OR'd in by surface functions
    tess.pshadowBits = 0;       // will be OR'd in by surface functions
    tess.xstages = state->stages;
    tess.numPasses = state->numUnfoggedPasses;
    tess.currentStageIteratorFunc = state->optimalStageIteratorFunc;
    tess.useInternalVao = true;
    tess.useCacheVao = false;

    tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

    if(tess.shader->clampTime && tess.shaderTime >= tess.shader->clampTime) {
        tess.shaderTime = tess.shader->clampTime;
    }

    if(backEnd.viewParms.flags & VPF_SHADOWMAP) {
        tess.currentStageIteratorFunc = RB_StageIteratorGeneric;
    }
}



extern float32 EvalWaveForm(const waveForm_t *wf);
extern float32 EvalWaveFormClamped(const waveForm_t *wf);


static void ComputeTexMods(shaderStage_t *pStage, sint bundleNum,
                           float32 *outMatrix, float32 *outOffTurb) {
    sint tm;
    float32 matrix[6], currentmatrix[6];
    textureBundle_t *bundle = &pStage->bundle[bundleNum];

    matrix[0] = 1.0f;
    matrix[2] = 0.0f;
    matrix[4] = 0.0f;
    matrix[1] = 0.0f;
    matrix[3] = 1.0f;
    matrix[5] = 0.0f;

    currentmatrix[0] = 1.0f;
    currentmatrix[2] = 0.0f;
    currentmatrix[4] = 0.0f;
    currentmatrix[1] = 0.0f;
    currentmatrix[3] = 1.0f;
    currentmatrix[5] = 0.0f;

    outMatrix[0] = 1.0f;
    outMatrix[2] = 0.0f;
    outMatrix[1] = 0.0f;
    outMatrix[3] = 1.0f;

    outOffTurb[0] = 0.0f;
    outOffTurb[1] = 0.0f;
    outOffTurb[2] = 0.0f;
    outOffTurb[3] = 0.0f;

    for(tm = 0; tm < bundle->numTexMods ; tm++) {
        switch(bundle->texMods[tm].type) {

            case TMOD_NONE:
                tm = TR_MAX_TEXMODS;        // break out of for loop
                break;

            case TMOD_TURBULENT:
                RB_CalcTurbulentFactors(&bundle->texMods[tm].wave, &outOffTurb[2],
                                        &outOffTurb[3]);
                break;

            case TMOD_ENTITY_TRANSLATE:
                RB_CalcScrollTexMatrix(backEnd.currentEntity->e.shaderTexCoord, matrix);
                break;

            case TMOD_SCROLL:
                RB_CalcScrollTexMatrix(bundle->texMods[tm].scroll,
                                       matrix);
                break;

            case TMOD_SCALE:
                RB_CalcScaleTexMatrix(bundle->texMods[tm].scale,
                                      matrix);
                break;

            case TMOD_STRETCH:
                RB_CalcStretchTexMatrix(&bundle->texMods[tm].wave,
                                        matrix);
                break;

            case TMOD_TRANSFORM:
                RB_CalcTransformTexMatrix(&bundle->texMods[tm],
                                          matrix);
                break;

            case TMOD_ROTATE:
                RB_CalcRotateTexMatrix(bundle->texMods[tm].rotateSpeed,
                                       matrix);
                break;

            default:
                common->Error(ERR_DROP, "ERROR: unknown texmod '%d' in shader '%s'",
                              bundle->texMods[tm].type, tess.shader->name);
                break;
        }

        switch(bundle->texMods[tm].type) {
            case TMOD_NONE:
            case TMOD_TURBULENT:
            default:
                break;

            case TMOD_ENTITY_TRANSLATE:
            case TMOD_SCROLL:
            case TMOD_SCALE:
            case TMOD_STRETCH:
            case TMOD_TRANSFORM:
            case TMOD_ROTATE:
                outMatrix[0] = matrix[0] * currentmatrix[0] + matrix[2] * currentmatrix[1];
                outMatrix[1] = matrix[1] * currentmatrix[0] + matrix[3] * currentmatrix[1];

                outMatrix[2] = matrix[0] * currentmatrix[2] + matrix[2] * currentmatrix[3];
                outMatrix[3] = matrix[1] * currentmatrix[2] + matrix[3] * currentmatrix[3];

                outOffTurb[0] = matrix[0] * currentmatrix[4] + matrix[2] * currentmatrix[5]
                                + matrix[4];
                outOffTurb[1] = matrix[1] * currentmatrix[4] + matrix[3] * currentmatrix[5]
                                + matrix[5];

                currentmatrix[0] = outMatrix[0];
                currentmatrix[1] = outMatrix[1];
                currentmatrix[2] = outMatrix[2];
                currentmatrix[3] = outMatrix[3];
                currentmatrix[4] = outOffTurb[0];
                currentmatrix[5] = outOffTurb[1];
                break;
        }
    }
}


static void ComputeDeformValues(sint *deformGen, vec5_t deformParams) {
    // u_DeformGen
    *deformGen = DGEN_NONE;

    if(!ShaderRequiresCPUDeforms(tess.shader)) {
        deformStage_t  *ds;

        // only support the first one
        ds = &tess.shader->deforms[0];

        switch(ds->deformation) {
            case DEFORM_WAVE:
                *deformGen = ds->deformationWave.func;

                deformParams[0] = ds->deformationWave.base;
                deformParams[1] = ds->deformationWave.amplitude;
                deformParams[2] = ds->deformationWave.phase;
                deformParams[3] = ds->deformationWave.frequency;
                deformParams[4] = ds->deformationSpread;
                break;

            case DEFORM_BULGE:
                *deformGen = DGEN_BULGE;

                deformParams[0] = 0;
                deformParams[1] = ds->bulgeHeight; // amplitude
                deformParams[2] = ds->bulgeWidth;  // phase
                deformParams[3] = ds->bulgeSpeed;  // frequency
                deformParams[4] = 0;
                break;

            default:
                break;
        }
    }
}

float32 DLIGHT_SIZE_MULTIPLIER = 2.5;

#define __SINGLE_PASS__

static void ProjectDlightTexture(void) {
    sint l;
    vec5_t deformParams;
    sint deformGen;

    if(!backEnd.refdef.num_dlights) {
        return;
    }

    //#define MAX_SHADER_DLIGHTS 8
#define MAX_SHADER_DLIGHTS 6

    ComputeDeformValues(&deformGen, deformParams);

    sint NUM_PASSES = (backEnd.refdef.num_dlights / MAX_SHADER_DLIGHTS) + 1;

    for(sint i = 0; i < NUM_PASSES; i++) {
        sint START_POS = i * MAX_SHADER_DLIGHTS;
        dlight_t *dl;

        backEnd.pc.c_dlightDraws++;

        shaderProgram_t *sp = &tr.dlightShader[deformGen == DGEN_NONE ? 0 : 1];

        GLSL_BindProgram(sp);

        GLSL_SetUniformMat4(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX,
                            glState.modelviewProjection);

        GLSL_SetUniformFloat(sp, UNIFORM_VERTEXLERP,
                             glState.vertexAttribsInterpolation);

        GLSL_SetUniformInt(sp, UNIFORM_DEFORMGEN, deformGen);

        if(deformGen != DGEN_NONE) {
            GLSL_SetUniformFloat5(sp, UNIFORM_DEFORMPARAMS, deformParams);
            GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);
        }

        for(l = START_POS; l < backEnd.refdef.num_dlights &&
                l - START_POS < MAX_SHADER_DLIGHTS; l++) {
            vec3_t      origin;
            float32     scale;
            float32     radius;
            vec4_t      vector;

            dl = &backEnd.refdef.dlights[l];
            VectorCopy(dl->transformed, origin);
            radius = dl->radius * DLIGHT_SIZE_MULTIPLIER;
            scale = 1.0f / radius;

            vector[0] = (dl->color[0]);
            vector[1] = (dl->color[1]);
            vector[2] = (dl->color[2]);
            vector[3] = 0.2f;
            GLSL_SetUniformVec4(sp, UNIFORM_LIGHTCOLOR + (l - START_POS), vector);

            vector[0] = origin[0];
            vector[1] = origin[1];
            vector[2] = origin[2];
            vector[3] = scale;
            GLSL_SetUniformVec4(sp, UNIFORM_LIGHTORIGIN + (l - START_POS), vector);
        }

        GL_BindToTMU(tr.dlightImage, TB_COLORMAP);

        // include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light
        // where they aren't rendered
        if(dl->additive) {
            GL_State(GLS_ATEST_GT_0 | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE |
                     GLS_DEPTHFUNC_EQUAL);
        } else {
            GL_State(GLS_ATEST_GT_0 | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE |
                     GLS_DEPTHFUNC_EQUAL);
        }

        GLSL_SetUniformInt(sp, UNIFORM_ALPHATEST, 1);

        R_DrawElements(tess.numIndexes, tess.firstIndex);

        backEnd.pc.c_totalIndexes += tess.numIndexes;
        backEnd.pc.c_dlightIndexes += tess.numIndexes;
        backEnd.pc.c_dlightVertexes += tess.numVertexes;
    }
}

static void ComputeShaderColors(shaderStage_t *pStage, vec4_t baseColor,
                                vec4_t vertColor, sint blend) {
    bool isBlend = ((blend & GLS_SRCBLEND_BITS) == GLS_SRCBLEND_DST_COLOR)
                   || ((blend & GLS_SRCBLEND_BITS) == GLS_SRCBLEND_ONE_MINUS_DST_COLOR)
                   || ((blend & GLS_DSTBLEND_BITS) == GLS_DSTBLEND_SRC_COLOR)
                   || ((blend & GLS_DSTBLEND_BITS) == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR);

    bool is2DDraw = backEnd.currentEntity == &backEnd.entity2D;

    float32 overbright = (isBlend ||
                          is2DDraw) ? 1.0f : static_cast<float32>(1 << tr.overbrightBits);

    fog_t *fog;

    baseColor[0] =
        baseColor[1] =
            baseColor[2] =
                baseColor[3] = 1.0f;

    vertColor[0] =
        vertColor[1] =
            vertColor[2] =
                vertColor[3] = 0.0f;

    //
    // rgbGen
    //
    switch(pStage->rgbGen) {
        case CGEN_EXACT_VERTEX:
        case CGEN_EXACT_VERTEX_LIT:
            baseColor[0] =
                baseColor[1] =
                    baseColor[2] =
                        baseColor[3] = 0.0f;

            vertColor[0] =
                vertColor[1] =
                    vertColor[2] = overbright;
            vertColor[3] = 1.0f;
            break;

        case CGEN_CONST:
            baseColor[0] = pStage->constantColor[0] / 255.0f;
            baseColor[1] = pStage->constantColor[1] / 255.0f;
            baseColor[2] = pStage->constantColor[2] / 255.0f;
            baseColor[3] = pStage->constantColor[3] / 255.0f;
            break;

        case CGEN_VERTEX:
        case CGEN_VERTEX_LIT:
            baseColor[0] =
                baseColor[1] =
                    baseColor[2] =
                        baseColor[3] = 0.0f;

            vertColor[0] =
                vertColor[1] =
                    vertColor[2] =
                        vertColor[3] = 1.0f;
            break;

        case CGEN_ONE_MINUS_VERTEX:
            baseColor[0] =
                baseColor[1] =
                    baseColor[2] = 1.0f;

            vertColor[0] =
                vertColor[1] =
                    vertColor[2] = -1.0f;
            break;

        case CGEN_FOG:
            fog = tr.world->fogs + tess.fogNum;

            baseColor[0] = (reinterpret_cast<uchar8 *>(&fog->colorInt))[0] / 255.0f;
            baseColor[1] = (reinterpret_cast<uchar8 *>(&fog->colorInt))[1] / 255.0f;
            baseColor[2] = (reinterpret_cast<uchar8 *>(&fog->colorInt))[2] / 255.0f;
            baseColor[3] = (reinterpret_cast<uchar8 *>(&fog->colorInt))[3] / 255.0f;
            break;

        case CGEN_WAVEFORM:
            baseColor[0] =
                baseColor[1] =
                    baseColor[2] = RB_CalcWaveColorSingle(&pStage->rgbWave);
            break;

        case CGEN_ENTITY:
            if(backEnd.currentEntity) {
                baseColor[0] = (reinterpret_cast<uchar8 *>
                                (backEnd.currentEntity->e.shaderRGBA))[0] / 255.0f;
                baseColor[1] = (reinterpret_cast<uchar8 *>
                                (backEnd.currentEntity->e.shaderRGBA))[1] / 255.0f;
                baseColor[2] = (reinterpret_cast<uchar8 *>
                                (backEnd.currentEntity->e.shaderRGBA))[2] / 255.0f;
                baseColor[3] = (reinterpret_cast<uchar8 *>
                                (backEnd.currentEntity->e.shaderRGBA))[3] / 255.0f;
            }

            break;

        case CGEN_ONE_MINUS_ENTITY:
            if(backEnd.currentEntity) {
                baseColor[0] = 1.0f - (reinterpret_cast<uchar8 *>
                                       (backEnd.currentEntity->e.shaderRGBA))[0] / 255.0f;
                baseColor[1] = 1.0f - (reinterpret_cast<uchar8 *>
                                       (backEnd.currentEntity->e.shaderRGBA))[1] / 255.0f;
                baseColor[2] = 1.0f - (reinterpret_cast<uchar8 *>
                                       (backEnd.currentEntity->e.shaderRGBA))[2] / 255.0f;
                baseColor[3] = 1.0f - (reinterpret_cast<uchar8 *>
                                       (backEnd.currentEntity->e.shaderRGBA))[3] / 255.0f;
            }

            break;

        case CGEN_IDENTITY:
        case CGEN_LIGHTING_DIFFUSE:
            baseColor[0] =
                baseColor[1] =
                    baseColor[2] = overbright;
            break;

        case CGEN_IDENTITY_LIGHTING:
        case CGEN_BAD:
            break;
    }

    //
    // alphaGen
    //
    switch(pStage->alphaGen) {
        case AGEN_SKIP:
            break;

        case AGEN_CONST:
            baseColor[3] = pStage->constantColor[3] / 255.0f;
            vertColor[3] = 0.0f;
            break;

        case AGEN_WAVEFORM:
            baseColor[3] = RB_CalcWaveAlphaSingle(&pStage->alphaWave);
            vertColor[3] = 0.0f;
            break;

        case AGEN_ENTITY:
            if(backEnd.currentEntity) {
                baseColor[3] = (reinterpret_cast<uchar8 *>
                                (backEnd.currentEntity->e.shaderRGBA))[3] / 255.0f;
            }

            vertColor[3] = 0.0f;
            break;

        case AGEN_ONE_MINUS_ENTITY:
            if(backEnd.currentEntity) {
                baseColor[3] = 1.0f - (reinterpret_cast<uchar8 *>
                                       (backEnd.currentEntity->e.shaderRGBA))[3] / 255.0f;
            }

            vertColor[3] = 0.0f;
            break;

        case AGEN_VERTEX:
            baseColor[3] = 0.0f;
            vertColor[3] = 1.0f;
            break;

        case AGEN_ONE_MINUS_VERTEX:
            baseColor[3] = 1.0f;
            vertColor[3] = -1.0f;
            break;

        case AGEN_IDENTITY:
        case AGEN_LIGHTING_SPECULAR:
        case AGEN_PORTAL:
            // Done entirely in vertex program
            baseColor[3] = 1.0f;
            vertColor[3] = 0.0f;
            break;
    }

    // FIXME: find some way to implement this.
#if 0

    // if in greyscale rendering mode turn all color values into greyscale.
    if(r_greyscale->integer) {
        sint scale;

        for(i = 0; i < tess.numVertexes; i++) {
            scale = (tess.svars.colors[i][0] + tess.svars.colors[i][1] +
                     tess.svars.colors[i][2]) / 3;
            tess.svars.colors[i][0] = tess.svars.colors[i][1] = tess.svars.colors[i][2]
                                      = scale;
        }
    }

#endif
}


static void ComputeFogValues(vec4_t fogDistanceVector,
                             vec4_t fogDepthVector, float32 *eyeT, glfog_t *glFog) {
    // from RB_CalcFogTexCoords()
    fog_t  *fog;
    vec3_t  local;

    if(glFog) {
        VectorSubtract(backEnd.orientation.origin,
                       backEnd.viewParms.orientation.origin, local);
        fogDistanceVector[0] = -backEnd.orientation.modelMatrix[2];
        fogDistanceVector[1] = -backEnd.orientation.modelMatrix[6];
        fogDistanceVector[2] = -backEnd.orientation.modelMatrix[10];
        fogDistanceVector[3] = DotProduct(local,
                                          backEnd.viewParms.orientation.axis[0]);

        fogDepthVector[0] = glFog->start;
        fogDepthVector[1] = glFog->end;
        fogDepthVector[2] = glFog->density;
        fogDepthVector[3] = 1.0;

        return;
    }


    if(!tess.fogNum) {
        return;
    }

    fog = tr.world->fogs + tess.fogNum;

    VectorSubtract(backEnd.orientation.origin,
                   backEnd.viewParms.orientation.origin, local);
    fogDistanceVector[0] = -backEnd.orientation.modelMatrix[2];
    fogDistanceVector[1] = -backEnd.orientation.modelMatrix[6];
    fogDistanceVector[2] = -backEnd.orientation.modelMatrix[10];
    fogDistanceVector[3] = DotProduct(local,
                                      backEnd.viewParms.orientation.axis[0]);

    // scale the fog vectors based on the fog's thickness
    VectorScale4(fogDistanceVector, fog->tcScale, fogDistanceVector);

    // rotate the gradient vector for this orientation
    if(fog->hasSurface) {
        fogDepthVector[0] = fog->surface[0] * backEnd.orientation.axis[0][0] +
                            fog->surface[1] * backEnd.orientation.axis[0][1] + fog->surface[2] *
                            backEnd.orientation.axis[0][2];
        fogDepthVector[1] = fog->surface[0] * backEnd.orientation.axis[1][0] +
                            fog->surface[1] * backEnd.orientation.axis[1][1] + fog->surface[2] *
                            backEnd.orientation.axis[1][2];
        fogDepthVector[2] = fog->surface[0] * backEnd.orientation.axis[2][0] +
                            fog->surface[1] * backEnd.orientation.axis[2][1] + fog->surface[2] *
                            backEnd.orientation.axis[2][2];
        fogDepthVector[3] = -fog->surface[3] + DotProduct(
                                backEnd.orientation.origin, fog->surface);

        *eyeT = DotProduct(backEnd.orientation.viewOrigin,
                           fogDepthVector) + fogDepthVector[3];
    } else {
        *eyeT = 1;  // non-surface fog always has eye inside
    }
}


static void ComputeFogColorMask(shaderStage_t *pStage,
                                vec4_t fogColorMask) {
    switch(pStage->adjustColorsForFog) {
        case ACFF_MODULATE_RGB:
            fogColorMask[0] =
                fogColorMask[1] =
                    fogColorMask[2] = 1.0f;
            fogColorMask[3] = 0.0f;
            break;

        case ACFF_MODULATE_ALPHA:
            fogColorMask[0] =
                fogColorMask[1] =
                    fogColorMask[2] = 0.0f;
            fogColorMask[3] = 1.0f;
            break;

        case ACFF_MODULATE_RGBA:
            fogColorMask[0] =
                fogColorMask[1] =
                    fogColorMask[2] =
                        fogColorMask[3] = 1.0f;
            break;

        default:
            fogColorMask[0] =
                fogColorMask[1] =
                    fogColorMask[2] =
                        fogColorMask[3] = 0.0f;
            break;
    }
}


static void ForwardDlight(void) {
    sint        l;
    //vec3_t    origin;
    //float32   scale;
    float32 radius;

    sint deformGen;
    vec5_t deformParams;

    vec4_t fogDistanceVector, fogDepthVector = {0, 0, 0, 0};
    float32 eyeT = 0;

    shaderCommands_t *input = &tess;
    shaderStage_t *pStage = tess.xstages[tess.shader->lightingStage];

    if(!pStage) {
        return;
    }

    if(!backEnd.refdef.num_dlights) {
        return;
    }

    ComputeDeformValues(&deformGen, deformParams);

    ComputeFogValues(fogDistanceVector, fogDepthVector, &eyeT, nullptr);

    for(l = 0 ; l < backEnd.refdef.num_dlights ; l++) {
        dlight_t   *dl;
        shaderProgram_t *sp;
        vec4_t vector;
        vec4_t texMatrix;
        vec4_t texOffTurb;

        if(!(tess.dlightBits & (1 << l))) {
            continue;   // this surface definately doesn't have any of this light
        }

        dl = &backEnd.refdef.dlights[l];
        //VectorCopy( dl->transformed, origin );
        radius = dl->radius;
        //scale = 1.0f / radius;

        //if (pStage->glslShaderGroup == tr.lightallShader)
        {
            sint index = pStage->glslShaderIndex;

            index &= ~LIGHTDEF_LIGHTTYPE_MASK;
            index |= LIGHTDEF_USE_LIGHT_VECTOR;

            sp = &tr.lightallShader[index];
        }

        backEnd.pc.c_lightallDraws++;

        GLSL_BindProgram(sp);

        GLSL_SetUniformMat4(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX,
                            glState.modelviewProjection);
        GLSL_SetUniformVec3(sp, UNIFORM_VIEWORIGIN,
                            backEnd.viewParms.orientation.origin);
        GLSL_SetUniformVec3(sp, UNIFORM_LOCALVIEWORIGIN,
                            backEnd.orientation.viewOrigin);

        GLSL_SetUniformFloat(sp, UNIFORM_VERTEXLERP,
                             glState.vertexAttribsInterpolation);

        GLSL_SetUniformInt(sp, UNIFORM_DEFORMGEN, deformGen);

        if(deformGen != DGEN_NONE) {
            GLSL_SetUniformFloat5(sp, UNIFORM_DEFORMPARAMS, deformParams);
            GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);
        }

        if(input->fogNum) {
            vec4_t fogColorMask;

            GLSL_SetUniformVec4(sp, UNIFORM_FOGDISTANCE, fogDistanceVector);
            GLSL_SetUniformVec4(sp, UNIFORM_FOGDEPTH, fogDepthVector);
            GLSL_SetUniformFloat(sp, UNIFORM_FOGEYET, eyeT);

            ComputeFogColorMask(pStage, fogColorMask);

            GLSL_SetUniformVec4(sp, UNIFORM_FOGCOLORMASK, fogColorMask);
        }

        {
            vec4_t baseColor;
            vec4_t vertColor;

            ComputeShaderColors(pStage, baseColor, vertColor,
                                GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);

            GLSL_SetUniformVec4(sp, UNIFORM_BASECOLOR, baseColor);
            GLSL_SetUniformVec4(sp, UNIFORM_VERTCOLOR, vertColor);
        }

        if(pStage->alphaGen == AGEN_PORTAL) {
            GLSL_SetUniformFloat(sp, UNIFORM_PORTALRANGE, tess.shader->portalRange);
        }

        GLSL_SetUniformInt(sp, UNIFORM_COLORGEN, pStage->rgbGen);
        GLSL_SetUniformInt(sp, UNIFORM_ALPHAGEN, pStage->alphaGen);

        GLSL_SetUniformVec3(sp, UNIFORM_DIRECTEDLIGHT, dl->color);

        VectorSet(vector, 0, 0, 0);
        GLSL_SetUniformVec3(sp, UNIFORM_AMBIENTLIGHT, vector);

        VectorCopy(dl->origin, vector);
        vector[3] = 1.0f;
        GLSL_SetUniformVec4(sp, UNIFORM_LIGHTORIGIN, vector);

        GLSL_SetUniformFloat(sp, UNIFORM_LIGHTRADIUS, radius);

        GLSL_SetUniformVec4(sp, UNIFORM_NORMALSCALE, pStage->normalScale);
        GLSL_SetUniformVec4(sp, UNIFORM_SPECULARSCALE, pStage->specularScale);

        // include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light
        // where they aren't rendered
        GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL);
        GLSL_SetUniformInt(sp, UNIFORM_ALPHATEST, 0);

        GLSL_SetUniformMat4(sp, UNIFORM_MODELMATRIX,
                            backEnd.orientation.transformMatrix);

        if(pStage->bundle[TB_DIFFUSEMAP].image[0]) {
            R_BindAnimatedImageToTMU(&pStage->bundle[TB_DIFFUSEMAP], TB_DIFFUSEMAP);
        }

        // bind textures that are sampled and used in the glsl shader, and
        // bind whiteImage to textures that are sampled but zeroed in the glsl shader
        //
        // alternatives:
        //  - use the last bound texture
        //     -> costs more to sample a higher res texture then throw out the result
        //  - disable texture sampling in glsl shader with #ifdefs, as before
        //     -> increases the number of shaders that must be compiled
        //

        if(pStage->bundle[TB_NORMALMAP].image[0]) {
            R_BindAnimatedImageToTMU(&pStage->bundle[TB_NORMALMAP], TB_NORMALMAP);
        } else if(r_normalMapping->integer) {
            GL_BindToTMU(tr.whiteImage, TB_NORMALMAP);
        }

        if(pStage->bundle[TB_SPECULARMAP].image[0]) {
            R_BindAnimatedImageToTMU(&pStage->bundle[TB_SPECULARMAP], TB_SPECULARMAP);
        } else if(r_specularMapping->integer) {
            GL_BindToTMU(tr.whiteImage, TB_SPECULARMAP);
        }

        {
            vec4_t enableTextures;

            VectorSet4(enableTextures, 0.0f, 0.0f, 0.0f, 0.0f);
            GLSL_SetUniformVec4(sp, UNIFORM_ENABLETEXTURES, enableTextures);
        }

        if(r_dlightMode->integer >= 2) {
            GL_BindToTMU(tr.shadowCubemaps[l], TB_SHADOWMAP);
        }

        ComputeTexMods(pStage, TB_DIFFUSEMAP, texMatrix, texOffTurb);
        GLSL_SetUniformVec4(sp, UNIFORM_DIFFUSETEXMATRIX, texMatrix);
        GLSL_SetUniformVec4(sp, UNIFORM_DIFFUSETEXOFFTURB, texOffTurb);

        GLSL_SetUniformInt(sp, UNIFORM_TCGEN0, pStage->bundle[0].tcGen);

        //
        // draw
        //

        R_DrawElements(input->numIndexes, input->firstIndex);

        backEnd.pc.c_totalIndexes += tess.numIndexes;
        backEnd.pc.c_dlightIndexes += tess.numIndexes;
        backEnd.pc.c_dlightVertexes += tess.numVertexes;
    }
}


static void ProjectPshadowVBOGLSL(void) {
    sint        l;
    vec3_t  origin;
    float32 radius;

    sint deformGen;
    vec5_t deformParams;

    shaderCommands_t *input = &tess;

    if(!backEnd.refdef.num_pshadows) {
        return;
    }

    ComputeDeformValues(&deformGen, deformParams);

    for(l = 0 ; l < backEnd.refdef.num_pshadows ; l++) {
        pshadow_t  *ps;
        shaderProgram_t *sp;
        vec4_t vector;

        if(!(tess.pshadowBits & (1 << l))) {
            continue;   // this surface definately doesn't have any of this shadow
        }

        ps = &backEnd.refdef.pshadows[l];
        VectorCopy(ps->lightOrigin, origin);
        radius = ps->lightRadius;

        sp = &tr.pshadowShader;

        GLSL_BindProgram(sp);

        GLSL_SetUniformMat4(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX,
                            glState.modelviewProjection);

        VectorCopy(origin, vector);
        vector[3] = 1.0f;
        GLSL_SetUniformVec4(sp, UNIFORM_LIGHTORIGIN, vector);

        VectorScale(ps->lightViewAxis[0], 1.0f / ps->viewRadius, vector);
        GLSL_SetUniformVec3(sp, UNIFORM_LIGHTFORWARD, vector);

        VectorScale(ps->lightViewAxis[1], 1.0f / ps->viewRadius, vector);
        GLSL_SetUniformVec3(sp, UNIFORM_LIGHTRIGHT, vector);

        VectorScale(ps->lightViewAxis[2], 1.0f / ps->viewRadius, vector);
        GLSL_SetUniformVec3(sp, UNIFORM_LIGHTUP, vector);

        GLSL_SetUniformFloat(sp, UNIFORM_LIGHTRADIUS, radius);

        // include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light
        // where they aren't rendered
        GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA |
                 GLS_DEPTHFUNC_EQUAL);
        GLSL_SetUniformInt(sp, UNIFORM_ALPHATEST, 0);

        GL_BindToTMU(tr.pshadowMaps[l], TB_DIFFUSEMAP);

        //
        // draw
        //

        R_DrawElements(input->numIndexes, input->firstIndex);

        backEnd.pc.c_totalIndexes += tess.numIndexes;
        //backEnd.pc.c_dlightIndexes += tess.numIndexes;
    }
}

extern bool fogIsOn;

/*
===================
RB_FogPass

Blends a fog texture on top of everything else
===================
*/
static void RB_FogPass(sint wolfFog) {
    fog_t *fog = nullptr;
    vec4_t  color;
    vec4_t  fogDistanceVector, fogDepthVector = { 0, 0, 0, 0 };
    float32 eyeT = 0;
    shaderProgram_t *sp;
    glfog_t *glFog = nullptr;
    sint deformGen;
    vec5_t deformParams;

    if(wolfFog) {
        if(backEnd.projection2D) {
            return;
        }

        if(backEnd.refdef.rdflags & RDF_DRAWINGSKY) {
            if(glfogsettings[FOG_SKY].registered) {
                glFog = &glfogsettings[FOG_SKY];
            }
        }

        if(skyboxportal && backEnd.refdef.rdflags & RDF_SKYBOXPORTAL) {
            if(glfogsettings[FOG_PORTALVIEW].registered) {
                glFog = &glfogsettings[FOG_PORTALVIEW];
            }
        } else {
            if(glfogNum > FOG_NONE) {
                glFog = &glfogsettings[FOG_CURRENT];
            }
        }

        if(!glFog) {
            return;
        }
    }

    ComputeDeformValues(&deformGen, deformParams);

    {
        sint index = 0;

        if(deformGen != DGEN_NONE) {
            index |= FOGDEF_USE_DEFORM_VERTEXES;
        }

        if(glState.vertexAnimation) {
            index |= FOGDEF_USE_VERTEX_ANIMATION;
        } else if(glState.boneAnimation) {
            index |= FOGDEF_USE_BONE_ANIMATION;
        }

        if(wolfFog) {
            if(glFog->mode == GL_LINEAR) {
                index |= FOGDEF_USE_WOLF_FOG_LINEAR;
            } else { // if (glFog->mode == GL_EXP)
                index |= FOGDEF_USE_WOLF_FOG_EXPONENTIAL;
            }
        }

        sp = &tr.fogShader[index];
    }

    backEnd.pc.c_fogDraws++;

    GLSL_BindProgram(sp);

    fog = tr.world->fogs + tess.fogNum;

    GLSL_SetUniformMat4(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX,
                        glState.modelviewProjection);

    GLSL_SetUniformFloat(sp, UNIFORM_VERTEXLERP,
                         glState.vertexAttribsInterpolation);

    if(glState.boneAnimation) {
        GLSL_SetUniformMat4BoneMatrix(sp, UNIFORM_BONEMATRIX, glState.boneMatrix,
                                      glState.boneAnimation);
    }

    GLSL_SetUniformInt(sp, UNIFORM_DEFORMGEN, deformGen);

    if(deformGen != DGEN_NONE) {
        GLSL_SetUniformFloat5(sp, UNIFORM_DEFORMPARAMS, deformParams);
        GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);
    }

    if(wolfFog) {
        color[0] = glFog->color[0];
        color[1] = glFog->color[1];
        color[2] = glFog->color[2];
        color[3] = glFog->color[3];
    } else {
        color[0] = (reinterpret_cast<uchar8 *>(&fog->colorInt))[0] / 255.0f;
        color[1] = (reinterpret_cast<uchar8 *>(&fog->colorInt))[1] / 255.0f;
        color[2] = (reinterpret_cast<uchar8 *>(&fog->colorInt))[2] / 255.0f;
        color[3] = (reinterpret_cast<uchar8 *>(&fog->colorInt))[3] / 255.0f;
    }

    GLSL_SetUniformVec4(sp, UNIFORM_COLOR, color);

    ComputeFogValues(fogDistanceVector, fogDepthVector, &eyeT, glFog);

    GLSL_SetUniformVec4(sp, UNIFORM_FOGDISTANCE, fogDistanceVector);
    GLSL_SetUniformVec4(sp, UNIFORM_FOGDEPTH, fogDepthVector);
    GLSL_SetUniformFloat(sp, UNIFORM_FOGEYET, eyeT);

    if(tess.shader->fogPass == FP_EQUAL) {
        GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA |
                 GLS_DEPTHFUNC_EQUAL);
    } else {
        GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
    }

    GLSL_SetUniformInt(sp, UNIFORM_ALPHATEST, 0);

    R_DrawElements(tess.numIndexes, tess.firstIndex);
}


static uint RB_CalcShaderVertexAttribs(shaderCommands_t *input) {
    uint vertexAttribs = input->shader->vertexAttribs;

    if(glState.vertexAnimation) {
        vertexAttribs |= ATTR_POSITION2;

        if(vertexAttribs & ATTR_NORMAL) {
            vertexAttribs |= ATTR_NORMAL2;
            vertexAttribs |= ATTR_TANGENT2;
        }
    }

    return vertexAttribs;
}

void RB_SetMaterialBasedProperties(shaderProgram_t *sp,
                                   shaderStage_t *pStage) {
    vec4_t local1;
    float32 specularScale = 1.0f;
    float32 materialType = 0.0f;
    float32 parallaxScale = 1.0f;
    float32 cubemapScale = 0.0f;
    float32 isMetalic = 0.0f;

    if(pStage->isWater) {
        specularScale = 1.5f;
        materialType = static_cast<float32>(MATERIAL_WATER);
        parallaxScale = 2.0f;
    } else {
        switch(tess.shader->surfaceFlags & MATERIAL_MASK) {
            case MATERIAL_WATER:            // 13           // light covering of water on a surface
                specularScale = 1.0f;
                cubemapScale = 1.5f;
                materialType = static_cast<float32>(MATERIAL_WATER);
                parallaxScale = 2.0f;
                break;

            case MATERIAL_SHORTGRASS:       // 5            // manicured lawn
                specularScale = 0.53f;
                cubemapScale = 0.0f;
                materialType = static_cast<float32>(MATERIAL_SHORTGRASS);
                parallaxScale = 2.5f;
                break;

            case MATERIAL_LONGGRASS:        // 6            // long jungle grass
                specularScale = 0.5f;
                cubemapScale = 0.0f;
                materialType = static_cast<float32>(MATERIAL_LONGGRASS);
                parallaxScale = 3.0f;
                break;

            case MATERIAL_SAND:             // 8            // sandy beach
                specularScale = 0.0f;
                cubemapScale = 0.0f;
                materialType = static_cast<float32>(MATERIAL_SAND);
                parallaxScale = 2.5f;
                break;

            case MATERIAL_CARPET:           // 27           // lush carpet
                specularScale = 0.0f;
                cubemapScale = 0.0f;
                materialType = static_cast<float32>(MATERIAL_CARPET);
                parallaxScale = 2.5f;
                break;

            case MATERIAL_GRAVEL:           // 9            // lots of small stones
                specularScale = 0.0f;
                cubemapScale = 0.0f;
                materialType = static_cast<float32>(MATERIAL_GRAVEL);
                parallaxScale = 3.0f;
                break;

            case MATERIAL_ROCK:             // 23           //
                specularScale = 0.0f;
                cubemapScale = 0.0f;
                materialType = static_cast<float32>(MATERIAL_ROCK);
                parallaxScale = 3.0f;
                break;

            case MATERIAL_TILES:            // 26           // tiled floor
                specularScale = 0.86f;
                cubemapScale = 0.9f;
                materialType = static_cast<float32>(MATERIAL_TILES);
                parallaxScale = 2.5f;
                break;

            case MATERIAL_SOLIDWOOD:        // 1            // freshly cut timber
                specularScale = 0.0f;
                cubemapScale = 0.0f;
                materialType = static_cast<float32>(MATERIAL_SOLIDWOOD);
                parallaxScale = 2.5f;
                break;

            case MATERIAL_HOLLOWWOOD:       // 2            // termite infested creaky wood
                specularScale = 0.0f;
                cubemapScale = 0.0f;
                materialType = static_cast<float32>(MATERIAL_HOLLOWWOOD);
                parallaxScale = 2.5f;
                break;

            case MATERIAL_SOLIDMETAL:       // 3            // solid girders
                specularScale = 0.92f;
                cubemapScale = 0.92f;
                materialType = static_cast<float32>(MATERIAL_SOLIDMETAL);
                parallaxScale = 0.005f;
                isMetalic = 1.0f;
                break;

            case MATERIAL_HOLLOWMETAL:      // 4            // hollow metal machines -- Used for weapons to force lower parallax...
                specularScale = 0.92f;
                cubemapScale = 0.92f;
                materialType = static_cast<float32>(MATERIAL_HOLLOWMETAL);
                parallaxScale = 2.0f;
                isMetalic = 1.0f;
                break;

            case MATERIAL_DRYLEAVES:        // 19           // dried up leaves on the floor
                specularScale = 0.0f;
                cubemapScale = 0.0f;
                materialType = static_cast<float32>(MATERIAL_DRYLEAVES);
                parallaxScale = 0.0f;
                break;

            case MATERIAL_GREENLEAVES:      // 20           // fresh leaves still on a tree
                specularScale = 0.75f;
                cubemapScale = 0.0f;
                materialType = static_cast<float32>(MATERIAL_GREENLEAVES);
                parallaxScale =
                    0.0f; // GreenLeaves should NEVER be parallaxed.. It's used for surfaces with an alpha channel and parallax screws it up...
                break;

            case MATERIAL_FABRIC:           // 21           // Cotton sheets
                specularScale = 0.48f;
                cubemapScale = 0.0f;
                materialType = static_cast<float32>(MATERIAL_FABRIC);
                parallaxScale = 2.5f;
                break;

            case MATERIAL_CANVAS:           // 22           // tent material
                specularScale = 0.45f;
                cubemapScale = 0.0f;
                materialType = static_cast<float32>(MATERIAL_CANVAS);
                parallaxScale = 2.5f;
                break;

            case MATERIAL_MARBLE:           // 12           // marble floors
                specularScale = 0.86f;
                cubemapScale = 1.0f;
                materialType = static_cast<float32>(MATERIAL_MARBLE);
                parallaxScale = 2.0f;
                break;

            case MATERIAL_SNOW:             // 14           // freshly laid snow
                specularScale = 0.65f;
                cubemapScale = 0.0f;
                materialType = static_cast<float32>(MATERIAL_SNOW);
                parallaxScale = 3.0f;
                break;

            case MATERIAL_MUD:              // 17           // wet soil
                specularScale = 0.0f;
                cubemapScale = 0.0f;
                materialType = static_cast<float32>(MATERIAL_MUD);
                parallaxScale = 3.0f;
                break;

            case MATERIAL_DIRT:             // 7            // hard mud
                specularScale = 0.0f;
                cubemapScale = 0.0f;
                materialType = static_cast<float32>(MATERIAL_DIRT);
                parallaxScale = 3.0f;
                break;

            case MATERIAL_CONCRETE:         // 11           // hardened concrete pavement
                specularScale = 0.3;
                cubemapScale = 0.0;
                materialType = static_cast<float32>(MATERIAL_CONCRETE);
                parallaxScale = 3.0;
                break;

            case MATERIAL_FLESH:            // 16           // hung meat, corpses in the world
                specularScale = 0.2f;
                cubemapScale = 0.0f;
                materialType = static_cast<float32>(MATERIAL_FLESH);
                parallaxScale = 1.0f;
                break;

            case MATERIAL_RUBBER:           // 24           // hard tire like rubber
                specularScale = 0.0f;
                cubemapScale = 0.0f;
                materialType = static_cast<float32>(MATERIAL_RUBBER);
                parallaxScale = 1.0f;
                break;

            case MATERIAL_PLASTIC:          // 25           //
                specularScale = 0.88f;
                cubemapScale = 0.5f;
                materialType = static_cast<float32>(MATERIAL_PLASTIC);
                parallaxScale = 1.0f;
                break;

            case MATERIAL_PLASTER:          // 28           // drywall style plaster
                specularScale = 0.4f;
                cubemapScale = 0.0f;
                materialType = static_cast<float32>(MATERIAL_PLASTER);
                parallaxScale = 2.0f;
                break;

            case MATERIAL_SHATTERGLASS:     // 29           // glass with the Crisis Zone style shattering
                specularScale = 0.88f;
                cubemapScale = 1.0f;
                materialType = static_cast<float32>(MATERIAL_SHATTERGLASS);
                parallaxScale = 1.0f;
                break;

            case MATERIAL_ARMOR:            // 30           // body armor
                specularScale = 0.4f;
                cubemapScale = 2.0f;
                materialType = static_cast<float32>(MATERIAL_ARMOR);
                parallaxScale = 2.0f;
                isMetalic = 1.0f;
                break;

            case MATERIAL_ICE:              // 15           // packed snow/solid ice
                specularScale = 0.9f;
                cubemapScale = 0.8f;
                parallaxScale = 2.0f;
                materialType = static_cast<float32>(MATERIAL_ICE);
                break;

            case MATERIAL_GLASS:            // 10           //
                specularScale = 0.95f;
                cubemapScale = 1.0f;
                materialType = static_cast<float32>(MATERIAL_GLASS);
                parallaxScale = 1.0f;
                break;

            case MATERIAL_BPGLASS:          // 18           // bulletproof glass
                specularScale = 0.93f;
                cubemapScale = 0.93f;
                materialType = static_cast<float32>(MATERIAL_BPGLASS);
                parallaxScale = 1.0f;
                break;

            case MATERIAL_COMPUTER:         // 31           // computers/electronic equipment
                specularScale = 0.92f;
                cubemapScale = 0.92f;
                materialType = static_cast<float32>(MATERIAL_COMPUTER);
                parallaxScale = 2.0f;
                break;

            default:
                specularScale = 0.0f;
                cubemapScale = 0.0f;
                materialType = static_cast<float32>(0.0f);
                parallaxScale = 1.0f;
                break;
        }
    }

    bool realNormalMap = false;

    if(pStage->bundle[TB_NORMALMAP].image[0]) {
        realNormalMap = true;
    }

    VectorSet4(local1, parallaxScale,
               static_cast<float32>(pStage->hasSpecular), specularScale, materialType);
    GLSL_SetUniformVec4(sp, UNIFORM_LOCAL1, local1);

    //GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);
    GLSL_SetUniformFloat(sp, UNIFORM_TIME, backEnd.refdef.floatTime);
}

void RB_SetStageImageDimensions(shaderProgram_t *sp,
                                shaderStage_t *pStage) {
    vec2_t dimensions;

    if(!pStage->bundle[0].image[0]) {
        pStage->bundle[0].image[0] = tr.whiteImage; // argh!
    }

    dimensions[0] = pStage->bundle[0].image[0]->width;
    dimensions[1] = pStage->bundle[0].image[0]->height;

    if(pStage->bundle[TB_DIFFUSEMAP].image[0]) {
        dimensions[0] = pStage->bundle[TB_DIFFUSEMAP].image[0]->width;
        dimensions[1] = pStage->bundle[TB_DIFFUSEMAP].image[0]->height;
    } else if(pStage->bundle[TB_NORMALMAP].image[0]) {
        dimensions[0] = pStage->bundle[TB_NORMALMAP].image[0]->width;
        dimensions[1] = pStage->bundle[TB_NORMALMAP].image[0]->height;
    } else if(pStage->bundle[TB_SPECULARMAP].image[0]) {
        dimensions[0] = pStage->bundle[TB_SPECULARMAP].image[0]->width;
        dimensions[1] = pStage->bundle[TB_SPECULARMAP].image[0]->height;
    }

    GLSL_SetUniformVec2(sp, UNIFORM_DIMENSIONS, dimensions);
}

static void RB_IterateStagesGeneric(shaderCommands_t *input) {
    sint stage;
    glfog_t *glFog;
    vec4_t fogDistanceVector, fogDepthVector = {0, 0, 0, 0};
    float32 eyeT = 0;

    sint deformGen;
    vec5_t deformParams;

    bool renderToCubemap = tr.renderCubeFbo &&
                           glState.currentFBO == tr.renderCubeFbo;

    ComputeDeformValues(&deformGen, deformParams);

    for(stage = 0; stage < MAX_SHADER_STAGES; stage++) {
        shaderStage_t *pStage = input->xstages[stage];
        shaderProgram_t *sp;
        vec4_t texMatrix;
        vec4_t texOffTurb;

        if(!pStage) {
            break;
        }

        glFog = nullptr;

        if(pStage->bundle[0].tcGen == TCGEN_ENVIRONMENT_MAPPED
                || pStage->bundle[0].image[0]->flags & IMGFLAG_CUBEMAP) {
            GL_TextureMode(5U, pStage->bundle[0].image[0]);
        }

        if(backEnd.depthFill) {
            if(pStage->glslShaderGroup == tr.lightallShader) {
                sint index = 0;

                if(backEnd.currentEntity && backEnd.currentEntity != &tr.worldEntity) {
                    if(glState.boneAnimation) {
                        index |= LIGHTDEF_ENTITY_BONE_ANIMATION;
                    } else {
                        index |= LIGHTDEF_ENTITY_VERTEX_ANIMATION;
                    }
                }

                if(pStage->stateBits & GLS_ATEST_BITS) {
                    index |= LIGHTDEF_USE_TCGEN_AND_TCMOD;
                }

                sp = &pStage->glslShaderGroup[index];
            } else {
                sint shaderAttribs = 0;

                if(tess.shader->numDeforms && !ShaderRequiresCPUDeforms(tess.shader)) {
                    shaderAttribs |= GENERICDEF_USE_DEFORM_VERTEXES;
                }

                if(glState.vertexAnimation) {
                    shaderAttribs |= GENERICDEF_USE_VERTEX_ANIMATION;
                } else if(glState.boneAnimation) {
                    shaderAttribs |= GENERICDEF_USE_BONE_ANIMATION;
                }

                if(pStage->stateBits & GLS_ATEST_BITS) {
                    shaderAttribs |= GENERICDEF_USE_TCGEN_AND_TCMOD;
                }

                sp = &tr.genericShader[shaderAttribs];
            }
        } else if(pStage->glslShaderGroup == tr.lightallShader) {
            sint index = pStage->glslShaderIndex;

            if(backEnd.currentEntity && backEnd.currentEntity != &tr.worldEntity) {
                if(glState.boneAnimation) {
                    index |= LIGHTDEF_ENTITY_BONE_ANIMATION;
                } else {
                    index |= LIGHTDEF_ENTITY_VERTEX_ANIMATION;
                }
            }

            if(r_sunlightMode->integer &&
                    (backEnd.viewParms.flags & VPF_USESUNLIGHT) &&
                    (index & LIGHTDEF_LIGHTTYPE_MASK)) {
                index |= LIGHTDEF_USE_SHADOWMAP;
            }

            if(r_lightmap->integer &&
                    ((index & LIGHTDEF_LIGHTTYPE_MASK) == LIGHTDEF_USE_LIGHTMAP)) {
                index = LIGHTDEF_USE_TCGEN_AND_TCMOD;
            }

            sp = &pStage->glslShaderGroup[index];

            backEnd.pc.c_lightallDraws++;
        } else {
            if(pStage->adjustColorsForFog && !backEnd.projection2D) {
                if(!tess.shader->noFog || pStage->isFogged) {
                    if(backEnd.refdef.rdflags & RDF_DRAWINGSKY) {
                        if(glfogsettings[FOG_SKY].registered) {
                            glFog = &glfogsettings[FOG_SKY];
                        }
                    }

                    if(skyboxportal && backEnd.refdef.rdflags & RDF_SKYBOXPORTAL) {
                        if(glfogsettings[FOG_PORTALVIEW].registered) {
                            glFog = &glfogsettings[FOG_PORTALVIEW];
                        }
                    } else {
                        if(glfogNum > FOG_NONE) {
                            glFog = &glfogsettings[FOG_CURRENT];
                        }
                    }
                }
            }

            sp = GLSL_GetGenericShaderProgram(stage, glFog);

            backEnd.pc.c_genericDraws++;
        }

        GLSL_BindProgram(sp);

        if(pStage->isWater) {
            if(stage > 0) {
                break;
            }

            sp = &tr.waterShader;
            pStage->glslShaderGroup = &tr.waterShader;
            GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);
            GLSL_BindProgram(sp);
            vec4_t loc0;

            VectorSet4(loc0, static_cast<float32>(2.0f), 0, 0,
                       0);     // force it to use the old water fx...

            GLSL_SetUniformVec4(sp, UNIFORM_LOCAL0, loc0);
            RB_SetMaterialBasedProperties(sp, pStage);

            GLSL_SetUniformFloat(sp, UNIFORM_TIME,
                                 static_cast<float32>(tess.shaderTime));

        } else {
            if(!sp || !sp->program) {
                pStage->glslShaderGroup = tr.lightallShader;
                sp = &pStage->glslShaderGroup[0];
            }

            RB_SetMaterialBasedProperties(sp, pStage);

            GLSL_BindProgram(sp);
        }

        RB_SetMaterialBasedProperties(sp, pStage);

        GLSL_BindProgram(sp);

        RB_SetStageImageDimensions(sp, pStage);

        GLSL_SetUniformMat4(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX,
                            glState.modelviewProjection);
        GLSL_SetUniformVec3(sp, UNIFORM_VIEWORIGIN,
                            backEnd.viewParms.orientation.origin);
        GLSL_SetUniformVec3(sp, UNIFORM_LOCALVIEWORIGIN,
                            backEnd.orientation.viewOrigin);

        GLSL_SetUniformFloat(sp, UNIFORM_VERTEXLERP,
                             glState.vertexAttribsInterpolation);

        if(glState.boneAnimation) {
            GLSL_SetUniformMat4BoneMatrix(sp, UNIFORM_BONEMATRIX, glState.boneMatrix,
                                          glState.boneAnimation);
        }

        GLSL_SetUniformInt(sp, UNIFORM_DEFORMGEN, deformGen);

        if(deformGen != DGEN_NONE) {
            GLSL_SetUniformFloat5(sp, UNIFORM_DEFORMPARAMS, deformParams);
            GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);
        }

        if(input->fogNum || glFog) {
            ComputeFogValues(fogDistanceVector, fogDepthVector, &eyeT, glFog);

            GLSL_SetUniformVec4(sp, UNIFORM_FOGDISTANCE, fogDistanceVector);
            GLSL_SetUniformVec4(sp, UNIFORM_FOGDEPTH, fogDepthVector);
            GLSL_SetUniformFloat(sp, UNIFORM_FOGEYET, eyeT);
        }

        GL_State(pStage->stateBits);

        if((pStage->stateBits & GLS_ATEST_BITS) == GLS_ATEST_GT_0) {
            GLSL_SetUniformInt(sp, UNIFORM_ALPHATEST, 1);
        } else if((pStage->stateBits & GLS_ATEST_BITS) == GLS_ATEST_LT_80) {
            GLSL_SetUniformInt(sp, UNIFORM_ALPHATEST, 2);
        } else if((pStage->stateBits & GLS_ATEST_BITS) == GLS_ATEST_GE_80) {
            GLSL_SetUniformInt(sp, UNIFORM_ALPHATEST, 3);
        } else {
            GLSL_SetUniformInt(sp, UNIFORM_ALPHATEST, 0);
        }


        {
            vec4_t baseColor;
            vec4_t vertColor;

            ComputeShaderColors(pStage, baseColor, vertColor, pStage->stateBits);

            GLSL_SetUniformVec4(sp, UNIFORM_BASECOLOR, baseColor);
            GLSL_SetUniformVec4(sp, UNIFORM_VERTCOLOR, vertColor);
        }

        if(pStage->rgbGen == CGEN_LIGHTING_DIFFUSE) {
            vec4_t vec;

            VectorScale(backEnd.currentEntity->ambientLight, 1.0f / 255.0f, vec);
            GLSL_SetUniformVec3(sp, UNIFORM_AMBIENTLIGHT, vec);

            VectorScale(backEnd.currentEntity->directedLight, 1.0f / 255.0f, vec);
            GLSL_SetUniformVec3(sp, UNIFORM_DIRECTEDLIGHT, vec);

            VectorCopy(backEnd.currentEntity->lightDir, vec);
            vec[3] = 0.0f;
            GLSL_SetUniformVec4(sp, UNIFORM_LIGHTORIGIN, vec);
            GLSL_SetUniformVec3(sp, UNIFORM_MODELLIGHTDIR,
                                backEnd.currentEntity->modelLightDir);

            GLSL_SetUniformFloat(sp, UNIFORM_LIGHTRADIUS, 0.0f);
        }

        if(pStage->alphaGen == AGEN_PORTAL) {
            GLSL_SetUniformFloat(sp, UNIFORM_PORTALRANGE, tess.shader->portalRange);
        }

        GLSL_SetUniformInt(sp, UNIFORM_COLORGEN, pStage->rgbGen);
        GLSL_SetUniformInt(sp, UNIFORM_ALPHAGEN, pStage->alphaGen);

        if(input->fogNum || glFog) {
            vec4_t fogColorMask;

            ComputeFogColorMask(pStage, fogColorMask);

            GLSL_SetUniformVec4(sp, UNIFORM_FOGCOLORMASK, fogColorMask);
        }

        if(r_lightmap->integer) {
            vec4_t v;
            VectorSet4(v, 1.0f, 0.0f, 0.0f, 1.0f);
            GLSL_SetUniformVec4(sp, UNIFORM_DIFFUSETEXMATRIX, v);
            VectorSet4(v, 0.0f, 0.0f, 0.0f, 0.0f);
            GLSL_SetUniformVec4(sp, UNIFORM_DIFFUSETEXOFFTURB, v);

            GLSL_SetUniformInt(sp, UNIFORM_TCGEN0, TCGEN_LIGHTMAP);
        } else {
            ComputeTexMods(pStage, TB_DIFFUSEMAP, texMatrix, texOffTurb);
            GLSL_SetUniformVec4(sp, UNIFORM_DIFFUSETEXMATRIX, texMatrix);
            GLSL_SetUniformVec4(sp, UNIFORM_DIFFUSETEXOFFTURB, texOffTurb);

            GLSL_SetUniformInt(sp, UNIFORM_TCGEN0, pStage->bundle[0].tcGen);

            if(pStage->bundle[0].tcGen == TCGEN_VECTOR) {
                vec3_t vec;

                VectorCopy(pStage->bundle[0].tcGenVectors[0], vec);
                GLSL_SetUniformVec3(sp, UNIFORM_TCGEN0VECTOR0, vec);
                VectorCopy(pStage->bundle[0].tcGenVectors[1], vec);
                GLSL_SetUniformVec3(sp, UNIFORM_TCGEN0VECTOR1, vec);
            }
        }

        GLSL_SetUniformMat4(sp, UNIFORM_MODELMATRIX,
                            backEnd.orientation.transformMatrix);

        GLSL_SetUniformVec4(sp, UNIFORM_NORMALSCALE, pStage->normalScale);

        {
            vec4_t specularScale;
            Vector4Copy(pStage->specularScale, specularScale);

            if(renderToCubemap) {
                // force specular to nonmetal if rendering cubemaps
                if(r_pbr->integer) {
                    specularScale[1] = 0.0f;
                }
            }

            GLSL_SetUniformVec4(sp, UNIFORM_SPECULARSCALE, specularScale);
        }

        //GLSL_SetUniformFloat(sp, UNIFORM_MAPLIGHTSCALE, backEnd.refdef.mapLightScale);

        //
        // do multitexture
        //
        if(backEnd.depthFill) {
            if(!(pStage->stateBits & GLS_ATEST_BITS)) {
                GL_BindToTMU(tr.whiteImage, TB_COLORMAP);
            } else if(pStage->bundle[TB_COLORMAP].image[0] != 0) {
                R_BindAnimatedImageToTMU(&pStage->bundle[TB_COLORMAP], TB_COLORMAP);
            }
        } else if(pStage->glslShaderGroup == tr.lightallShader) {
            sint i;
            vec4_t enableTextures;

            if(r_sunlightMode->integer &&
                    (backEnd.viewParms.flags & VPF_USESUNLIGHT) &&
                    (pStage->glslShaderIndex & LIGHTDEF_LIGHTTYPE_MASK)) {
                // FIXME: screenShadowImage is nullptr if no framebuffers
                if(tr.screenShadowImage) {
                    GL_BindToTMU(tr.screenShadowImage, TB_SHADOWMAP);
                }

                GLSL_SetUniformVec3(sp, UNIFORM_PRIMARYLIGHTAMBIENT,
                                    backEnd.refdef.sunAmbCol);

                if(r_pbr->integer) {
                    vec3_t color;

                    color[0] = backEnd.refdef.sunCol[0] * backEnd.refdef.sunCol[0];
                    color[1] = backEnd.refdef.sunCol[1] * backEnd.refdef.sunCol[1];
                    color[2] = backEnd.refdef.sunCol[2] * backEnd.refdef.sunCol[2];
                    GLSL_SetUniformVec3(sp, UNIFORM_PRIMARYLIGHTCOLOR, color);
                } else {
                    GLSL_SetUniformVec3(sp, UNIFORM_PRIMARYLIGHTCOLOR, backEnd.refdef.sunCol);
                }

                GLSL_SetUniformVec4(sp, UNIFORM_PRIMARYLIGHTORIGIN,
                                    backEnd.refdef.sunDir);
            }

            VectorSet4(enableTextures, 0, 0, 0, 0);

            if((r_lightmap->integer == 1 || r_lightmap->integer == 2) &&
                    pStage->bundle[TB_LIGHTMAP].image[0]) {
                for(i = 0; i < NUM_TEXTURE_BUNDLES; i++) {
                    if(i == TB_COLORMAP) {
                        R_BindAnimatedImageToTMU(&pStage->bundle[TB_LIGHTMAP], i);
                    } else {
                        GL_BindToTMU(tr.whiteImage, i);
                    }
                }
            } else if(r_lightmap->integer == 3 &&
                      pStage->bundle[TB_DELUXEMAP].image[0]) {
                for(i = 0; i < NUM_TEXTURE_BUNDLES; i++) {
                    if(i == TB_COLORMAP) {
                        R_BindAnimatedImageToTMU(&pStage->bundle[TB_DELUXEMAP], i);
                    } else {
                        GL_BindToTMU(tr.whiteImage, i);
                    }
                }
            } else {
                bool light = (pStage->glslShaderIndex & LIGHTDEF_LIGHTTYPE_MASK) != 0;
                bool fastLight = !(r_normalMapping->integer || r_specularMapping->integer);

                if(pStage->bundle[TB_DIFFUSEMAP].image[0]) {
                    R_BindAnimatedImageToTMU(&pStage->bundle[TB_DIFFUSEMAP], TB_DIFFUSEMAP);
                }

                if(pStage->bundle[TB_LIGHTMAP].image[0]) {
                    R_BindAnimatedImageToTMU(&pStage->bundle[TB_LIGHTMAP], TB_LIGHTMAP);
                }

                // bind textures that are sampled and used in the glsl shader, and
                // bind whiteImage to textures that are sampled but zeroed in the glsl shader
                //
                // alternatives:
                //  - use the last bound texture
                //     -> costs more to sample a higher res texture then throw out the result
                //  - disable texture sampling in glsl shader with #ifdefs, as before
                //     -> increases the number of shaders that must be compiled
                //
                if(light && !fastLight) {
                    if(pStage->bundle[TB_NORMALMAP].image[0]) {
                        R_BindAnimatedImageToTMU(&pStage->bundle[TB_NORMALMAP], TB_NORMALMAP);
                        enableTextures[0] = 1.0f;
                    } else if(r_normalMapping->integer) {
                        GL_BindToTMU(tr.whiteImage, TB_NORMALMAP);
                    }

                    if(pStage->bundle[TB_DELUXEMAP].image[0]) {
                        R_BindAnimatedImageToTMU(&pStage->bundle[TB_DELUXEMAP], TB_DELUXEMAP);
                        enableTextures[1] = 1.0f;
                    } else if(r_deluxeMapping->integer) {
                        GL_BindToTMU(tr.whiteImage, TB_DELUXEMAP);
                    }

                    if(pStage->bundle[TB_SPECULARMAP].image[0]) {
                        R_BindAnimatedImageToTMU(&pStage->bundle[TB_SPECULARMAP], TB_SPECULARMAP);
                        enableTextures[2] = 1.0f;
                    } else if(r_specularMapping->integer) {
                        GL_BindToTMU(tr.whiteImage, TB_SPECULARMAP);
                    }
                }

                enableTextures[3] = (r_cubeMapping->integer &&
                                     !(tr.viewParms.flags & VPF_NOCUBEMAPS) &&
                                     input->cubemapIndex) ? 1.0f : 0.0f;
            }

            GLSL_SetUniformVec4(sp, UNIFORM_ENABLETEXTURES, enableTextures);
        } else if(pStage->bundle[TB_LIGHTMAP].image[0] != 0) {
            R_BindAnimatedImageToTMU(&pStage->bundle[0], 0);
            R_BindAnimatedImageToTMU(&pStage->bundle[1], 1);
        } else {
            //
            // set state
            //
            R_BindAnimatedImageToTMU(&pStage->bundle[0], 0);
        }

        //
        // testing cube map
        //
        if(!(tr.viewParms.flags & VPF_NOCUBEMAPS) && input->cubemapIndex &&
                r_cubeMapping->integer) {
            vec4_t vec;
            cubemap_t *cubemap = &tr.cubemaps[input->cubemapIndex - 1];

            // FIXME: cubemap image could be nullptr if cubemap isn't renderer or loaded
            if(cubemap->image) {
                GL_BindToTMU(cubemap->image, TB_CUBEMAP);
            }

            GL_BindToTMU(tr.envBrdfImage, TB_ENVBRDFMAP);

            VectorSubtract(cubemap->origin, backEnd.viewParms.orientation.origin, vec);
            vec[3] = 1.0f;

            VectorScale4(vec, 1.0f / cubemap->parallaxRadius, vec);

            GLSL_SetUniformVec4(sp, UNIFORM_CUBEMAPINFO, vec);
        }

        //
        // draw
        //
        R_DrawElements(input->numIndexes, input->firstIndex);

        // allow skipping out to show just lightmaps during development
        if(r_lightmap->integer && (pStage->bundle[0].isLightmap ||
                                   pStage->bundle[1].isLightmap)) {
            break;
        }

        if(backEnd.depthFill) {
            break;
        }
    }
}


static void RB_RenderShadowmap(shaderCommands_t *input) {
    sint deformGen;
    vec5_t deformParams;

    ComputeDeformValues(&deformGen, deformParams);

    {
        shaderProgram_t *sp = &tr.shadowmapShader[0];

        if(glState.vertexAnimation) {
            sp = &tr.shadowmapShader[SHADOWMAPDEF_USE_VERTEX_ANIMATION];
        } else if(glState.boneAnimation) {
            sp = &tr.shadowmapShader[SHADOWMAPDEF_USE_BONE_ANIMATION];
        }

        vec4_t vector;

        GLSL_BindProgram(sp);

        GLSL_SetUniformMat4(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX,
                            glState.modelviewProjection);

        GLSL_SetUniformMat4(sp, UNIFORM_MODELMATRIX,
                            backEnd.orientation.transformMatrix);

        GLSL_SetUniformFloat(sp, UNIFORM_VERTEXLERP,
                             glState.vertexAttribsInterpolation);

        if(glState.boneAnimation) {
            GLSL_SetUniformMat4BoneMatrix(sp, UNIFORM_BONEMATRIX, glState.boneMatrix,
                                          glState.boneAnimation);
        }

        GLSL_SetUniformInt(sp, UNIFORM_DEFORMGEN, deformGen);

        if(deformGen != DGEN_NONE) {
            GLSL_SetUniformFloat5(sp, UNIFORM_DEFORMPARAMS, deformParams);
            GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);
        }

        VectorCopy(backEnd.viewParms.orientation.origin, vector);
        vector[3] = 1.0f;
        GLSL_SetUniformVec4(sp, UNIFORM_LIGHTORIGIN, vector);
        GLSL_SetUniformFloat(sp, UNIFORM_LIGHTRADIUS, backEnd.viewParms.zFar);

        GL_State(0);
        GLSL_SetUniformInt(sp, UNIFORM_ALPHATEST, 0);

        //
        // do multitexture
        //
        //if ( pStage->glslShaderGroup )
        {
            //
            // draw
            //

            R_DrawElements(input->numIndexes, input->firstIndex);
        }
    }
}



/*
** RB_StageIteratorGeneric
*/
void RB_StageIteratorGeneric(void) {
    shaderCommands_t *input;
    uint vertexAttribs = 0;

    input = &tess;

    if(!input->numVertexes || !input->numIndexes) {
        return;
    }

    if(tess.useInternalVao) {
        RB_DeformTessGeometry();
    }

    vertexAttribs = RB_CalcShaderVertexAttribs(input);

    if(tess.useInternalVao) {
        RB_UpdateTessVao(vertexAttribs);
    } else {
        backEnd.pc.c_staticVaoDraws++;
    }

    //
    // log this call
    //
    if(r_logFile->integer) {
        // don't just call LogComment, or we will get
        // a call to va() every frame!
        GLimp_LogComment(reinterpret_cast< valueType * >
                         (va(nullptr, "--- RB_StageIteratorGeneric( %s ) ---\n",
                             tess.shader->name)));
    }

    //
    // set face culling appropriately
    //
    if(input->shader->cullType == CT_TWO_SIDED) {
        GL_Cull(CT_TWO_SIDED);
    } else {
        bool cullFront = (input->shader->cullType == CT_FRONT_SIDED);

        if(backEnd.viewParms.flags & VPF_DEPTHSHADOW) {
            cullFront = !cullFront;
        }

        if(backEnd.viewParms.isMirror) {
            cullFront = !cullFront;
        }

        if(backEnd.currentEntity && backEnd.currentEntity->mirrored) {
            cullFront = !cullFront;
        }

        if(cullFront) {
            GL_Cull(CT_FRONT_SIDED);
        } else {
            GL_Cull(CT_BACK_SIDED);
        }
    }

    // set polygon offset if necessary
    if(input->shader->polygonOffset) {
        qglEnable(GL_POLYGON_OFFSET_FILL);
    }

    //
    // Set up any special shaders needed for this surface/contents type...
    //
    if((tess.shader->contentFlags & CONTENTS_WATER)) {
        if(input->xstages[0]->isWater !=
                true)   // In case it is already set, no need looping more then once on the same shader...
            for(sint stage = 0; stage < MAX_SHADER_STAGES; stage++)
                if(input->xstages[stage]) {
                    input->xstages[stage]->isWater = true;
                }
    }

    //
    // render depth if in depthfill mode
    //
    if(backEnd.depthFill) {
        RB_IterateStagesGeneric(input);

        //
        // reset polygon offset
        //
        if(input->shader->polygonOffset) {
            qglDisable(GL_POLYGON_OFFSET_FILL);
        }

        return;
    }

    //
    // render shadowmap if in shadowmap mode
    //
    if(backEnd.viewParms.flags & VPF_SHADOWMAP) {
        if(input->shader->sort == SS_OPAQUE) {
            RB_RenderShadowmap(input);
        }

        //
        // reset polygon offset
        //
        if(input->shader->polygonOffset) {
            qglDisable(GL_POLYGON_OFFSET_FILL);
        }

        return;
    }

    //
    //
    // call shader function
    //
    RB_IterateStagesGeneric(input);

    //
    // now do any dynamic lighting needed. UQ1: A generic method to rule them all... A SANE real world style lighting with a blacklist - not a whitelist!
    //
    if(!(tess.shader->surfaceFlags & (/*SURF_NODLIGHT |*/ SURF_SKY))) {
        switch(static_cast<sint>(tess.shader->sort)) {
            case SS_PORTAL:
            case SS_ENVIRONMENT: // is this really always a skybox???
            case SS_SEE_THROUGH:

            //case SS_FOG: // hmm... these??? i sorta like the idea of lighting up fog particles myself...
            case SS_BLEND0:
            case SS_BLEND1:
            case SS_BLEND2:
            case SS_BLEND3:
            case SS_BLEND6:
                break;

            default:
                if(r_dlightMode->integer >= 2) {
                    ForwardDlight();
                } else {
                    ProjectDlightTexture();
                }

                //
                // pshadows!
                //
                if(r_shadows->integer == 4 && tess.pshadowBits &&
                        !(tess.shader->surfaceFlags & (/*SURF_NODLIGHT |*/ SURF_SKY))) {
                    ProjectPshadowVBOGLSL();
                }

                break;
        }
    }

    //
    // now do fog
    //
    if(tess.fogNum && tess.shader->fogPass) {
        RB_FogPass(0);
    }

    //
    // RTCW fog
    // may not match original RTCW fog, since that's done per stage
    //
    if(tess.shader->fogPass && tess.shader->sort <= SS_OPAQUE) {
        sint stage, stageFog = 0;

        if(tess.shader->noFog) {
            // make sure at least one stage has fog
            for(stage = 0; stage < MAX_SHADER_STAGES; stage++) {
                shaderStage_t *pStage = tess.xstages[stage];

                if(!pStage) {
                    break;
                }

                if(pStage->isFogged) {
                    stageFog = 1;
                    break;
                }
            }
        }

        if(!tess.shader->noFog || stageFog) {
            RB_FogPass(1);
        }
    }

    //
    // reset polygon offset
    //
    if(input->shader->polygonOffset) {
        qglDisable(GL_POLYGON_OFFSET_FILL);
    }
}

/*
** RB_EndSurface
*/
void RB_EndSurface(void) {
    shaderCommands_t *input;

    input = &tess;

    if(input->numIndexes == 0 || input->numVertexes == 0) {
        return;
    }

    if(input->indexes[SHADER_MAX_INDEXES - 1] != 0) {
        common->Error(ERR_DROP, "RB_EndSurface() - SHADER_MAX_INDEXES hit");
    }

    if(input->xyz[SHADER_MAX_VERTEXES - 1][0] != 0) {
        common->Error(ERR_DROP, "RB_EndSurface() - SHADER_MAX_VERTEXES hit");
    }

    if(tess.shader == tr.shadowShader) {
        RB_ShadowTessEnd();
        return;
    }

    // for debugging of sort order issues, stop rendering after a given sort value
    if(r_debugSort->integer && r_debugSort->integer < tess.shader->sort) {
        return;
    }

    if(skyboxportal) {
        // world
        if(!(backEnd.refdef.rdflags & RDF_SKYBOXPORTAL)) {
            if(tess.currentStageIteratorFunc ==
                    RB_StageIteratorSky) {   // don't process these tris at all
                return;
            }
        }
        // portal sky
        else {
            if(!drawskyboxportal) {
                if(!(tess.currentStageIteratorFunc ==
                        RB_StageIteratorSky)) {   // only process sky tris
                    return;
                }
            }
        }
    }

    if(tess.useCacheVao) {
        // upload indexes now
        VaoCache_Commit();
    }

    //
    // update performance counters
    //
    backEnd.pc.c_shaders++;
    backEnd.pc.c_vertexes += tess.numVertexes;
    backEnd.pc.c_indexes += tess.numIndexes;
    backEnd.pc.c_totalIndexes += tess.numIndexes * tess.numPasses;

    //
    // call off to shader specific tess end function
    //
    tess.currentStageIteratorFunc();

    //
    // draw debugging stuff
    //
    if(r_showtris->integer) {
        DrawTris(input);
    }

    if(r_shownormals->integer) {
        DrawNormals(input);
    }

    // clear shader so we can tell we don't have any unclosed surfaces
    tess.numIndexes = 0;
    tess.numVertexes = 0;
    tess.firstIndex = 0;

    GLimp_LogComment("----------\n");
}
