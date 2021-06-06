////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2005 Id Software, Inc.
// Copyright(C) 2000 - 2013 Darklegion Development
// Copyright(C) 2011 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   r_scene.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.hpp>

sint            r_firstSceneDrawSurf;

sint            r_numdlights;
sint            r_firstSceneDlight;

sint            r_numentities;
sint            r_firstSceneEntity;

sint            r_numpolys;
sint            r_firstScenePoly;

sint            r_numpolyverts;


/*
====================
R_InitNextFrame

====================
*/
void R_InitNextFrame(void) {
    // use the other buffers next frame, because another CPU
    // may still be rendering into the current ones
    tr.smpFrame ^= 1;

    backEndData->commands[tr.smpFrame].used = 0;

    r_firstSceneDrawSurf = 0;

    r_numdlights = 0;
    r_firstSceneDlight = 0;

    r_numentities = 0;
    r_firstSceneEntity = 0;

    r_numpolys = 0;
    r_firstScenePoly = 0;

    r_numpolyverts = 0;
}


/*
====================
idRenderSystemLocal::ClearScene
====================
*/
void idRenderSystemLocal::ClearScene(void) {
    r_firstSceneDlight = r_numdlights;
    r_firstSceneEntity = r_numentities;
    r_firstScenePoly = r_numpolys;
}

/*
===========================================================================

DISCRETE POLYS

===========================================================================
*/

/*
=====================
R_AddPolygonSurfaces

Adds all the scene's polys into this view's drawsurf list
=====================
*/
void R_AddPolygonSurfaces(void) {
    sint            i;
    shader_t   *sh;
    srfPoly_t  *poly;
    sint        fogMask;

    tr.currentEntityNum = REFENTITYNUM_WORLD;
    tr.shiftedEntityNum = tr.currentEntityNum << QSORT_REFENTITYNUM_SHIFT;
    fogMask = -((tr.refdef.rdflags & RDF_NOFOG) == 0);

    for(i = 0, poly = tr.refdef.polys; i < tr.refdef.numPolys ; i++, poly++) {
        sh = R_GetShaderByHandle(poly->hShader);
        R_AddDrawSurf((surfaceType_t *)poly, sh, poly->fogIndex & fogMask, false,
                      false, 0 /*cubeMap*/);
    }
}

/*
=====================
idRenderSystemLocal::AddPolyToScene
=====================
*/
void idRenderSystemLocal::AddPolyToScene(qhandle_t hShader, sint numVerts,
        const polyVert_t *verts, sint numPolys) {
    srfPoly_t  *poly;
    sint            i, j;
    sint            fogIndex;
    fog_t      *fog;
    vec3_t      bounds[2];

    if(!tr.registered) {
        return;
    }

    if(!hShader) {
        // This isn't a useful warning, and an hShader of zero isn't a null shader, it's
        // the default shader.
        //clientRendererSystem->RefPrintf( PRINT_WARNING, "WARNING: idRenderSystemLocal::AddPolyToScene: nullptr poly shader\n");
        //return;
    }

    for(j = 0; j < numPolys; j++) {
        if(r_numpolyverts + numVerts > max_polyverts || r_numpolys >= max_polys) {
            /*
            NOTE TTimo this was initially a PRINT_WARNING
            but it happens a lot with high fighting scenes and particles
            since we don't plan on changing the const and making for room for those effects
            simply cut this message to developer only
            */
            static sint lastWarning = 0;

            if(tr.refdef.time / 1000 > lastWarning) {
                lastWarning = tr.refdef.time / 1000;
                clientRendererSystem->RefPrintf(PRINT_DEVELOPER,
                                                "WARNING: idRenderSystemLocal::AddPolyToScene: r_max_polys or r_max_polyverts reached\n");
            }

            return;
        }

        poly = &backEndData->polys[r_numpolys];
        poly->surfaceType = SF_POLY;
        poly->hShader = hShader;
        poly->numVerts = numVerts;
        poly->verts = &backEndData->polyVerts[r_numpolyverts];

        ::memcpy(poly->verts, &verts[numVerts * j], numVerts * sizeof(*verts));

        // done.
        r_numpolys++;
        r_numpolyverts += numVerts;

        // if no world is loaded
        if(tr.world == nullptr) {
            fogIndex = 0;
        }
        // see if it is in a fog volume
        else if(tr.world->numfogs == 1) {
            fogIndex = 0;
        } else {
            // find which fog volume the poly is in
            VectorCopy(poly->verts[0].xyz, bounds[0]);
            VectorCopy(poly->verts[0].xyz, bounds[1]);

            for(i = 1 ; i < poly->numVerts ; i++) {
                AddPointToBounds(poly->verts[i].xyz, bounds[0], bounds[1]);
            }

            for(fogIndex = 1 ; fogIndex < tr.world->numfogs ; fogIndex++) {
                fog = &tr.world->fogs[fogIndex];

                if(bounds[1][0] >= fog->bounds[0][0]
                        && bounds[1][1] >= fog->bounds[0][1]
                        && bounds[1][2] >= fog->bounds[0][2]
                        && bounds[0][0] <= fog->bounds[1][0]
                        && bounds[0][1] <= fog->bounds[1][1]
                        && bounds[0][2] <= fog->bounds[1][2]) {
                    break;
                }
            }

            if(fogIndex == tr.world->numfogs) {
                fogIndex = 0;
            }
        }

        poly->fogIndex = fogIndex;
    }
}


//=================================================================================


/*
=====================
idRenderSystemLocal::AddRefEntityToScene
=====================
*/
void idRenderSystemLocal::AddRefEntityToScene(const refEntity_t *ent) {
    vec3_t cross;

    if(!tr.registered) {
        return;
    }

    if(r_numentities >= MAX_REFENTITIES) {
        clientRendererSystem->RefPrintf(PRINT_DEVELOPER,
                                        "idRenderSystemLocal::AddRefEntityToScene: Dropping refEntity, reached MAX_REFENTITIES\n");
        return;
    }

    if(Q_isnan(ent->origin[0]) || Q_isnan(ent->origin[1]) ||
            Q_isnan(ent->origin[2])) {
        static bool firstTime = true;

        if(firstTime) {
            firstTime = false;
            clientRendererSystem->RefPrintf(PRINT_WARNING,
                                            "idRenderSystemLocal::AddRefEntityToScene passed a refEntity which has an origin with a NaN component\n");
        }

        return;
    }

    if(ent->reType < 0 || ent->reType >= RT_MAX_REF_ENTITY_TYPE) {
        Com_Error(ERR_DROP,
                  "idRenderSystemLocal::AddRefEntityToScene: bad reType %i", ent->reType);
    }

    backEndData->entities[r_numentities].e = *ent;
    backEndData->entities[r_numentities].lightingCalculated = false;

    CrossProduct(ent->axis[0], ent->axis[1], cross);
    backEndData->entities[r_numentities].mirrored = (DotProduct(ent->axis[2],
            cross) < 0.f);

    r_numentities++;
}


/*
=====================
RE_AddDynamicLightToScene
=====================
*/
void RE_AddDynamicLightToScene(const vec3_t org, float32 intensity,
                               float32 r, float32 g, float32 b, sint additive) {
    dlight_t   *dl;

    if(!tr.registered) {
        return;
    }

    if(r_numdlights >= MAX_DLIGHTS) {
        return;
    }

    if(intensity <= 0) {
        return;
    }

    dl = &backEndData->dlights[r_numdlights++];
    VectorCopy(org, dl->origin);
    dl->radius = intensity;
    dl->color[0] = r;
    dl->color[1] = g;
    dl->color[2] = b;
    dl->additive = additive;
}

/*
=====================
idRenderSystemLocal::AddLightToScene
=====================
*/
void idRenderSystemLocal::AddLightToScene(const vec3_t org,
        float32 intensity, float32 r, float32 g, float32 b) {
    RE_AddDynamicLightToScene(org, intensity, r, g, b, false);
}

/*
=====================
idRenderSystemLocal::AddAdditiveLightToScene
=====================
*/
void idRenderSystemLocal::AddAdditiveLightToScene(const vec3_t org,
        float32 intensity, float32 r, float32 g, float32 b) {
    RE_AddDynamicLightToScene(org, intensity, r, g, b, true);
}


void RE_BeginScene(const refdef_t *fd) {
    ::memcpy(tr.refdef.text, fd->text, sizeof(tr.refdef.text));

    tr.refdef.x = fd->x;
    tr.refdef.y = fd->y;
    tr.refdef.width = fd->width;
    tr.refdef.height = fd->height;

    if(fd->rdflags & RDF_NOWORLDMODEL) {
        tr.refdef.fov_x = fd->fov_x;
        tr.refdef.fov_y = fd->fov_y;
    } else {
        tr.refdef.fov_x = atan2(480 * tr.refdef.width,
                                640 * tr.refdef.height / tan(fd->fov_x / 360 * M_PI)) * 360 / M_PI;
        tr.refdef.fov_y = atan2(tr.refdef.height,
                                tr.refdef.width / tan(tr.refdef.fov_x / 360 * M_PI)) * 360 / M_PI;
    }

    VectorCopy(fd->vieworg, tr.refdef.vieworg);
    VectorCopy(fd->viewaxis[0], tr.refdef.viewaxis[0]);
    VectorCopy(fd->viewaxis[1], tr.refdef.viewaxis[1]);
    VectorCopy(fd->viewaxis[2], tr.refdef.viewaxis[2]);

    tr.refdef.time = fd->time;
    tr.refdef.rdflags = fd->rdflags;

    // copy the areamask data over and note if it has changed, which
    // will force a reset of the visible leafs even if the view hasn't moved
    tr.refdef.areamaskModified = false;

    if(!(tr.refdef.rdflags & RDF_NOWORLDMODEL)) {
        sint        areaDiff;
        sint        i;

        // compare the area bits
        areaDiff = 0;

        for(i = 0 ; i < MAX_MAP_AREA_BYTES / 4 ; i++) {
            areaDiff |= tr.refdef.areamask[i] ^ fd->areamask[i];
            tr.refdef.areamask[i] = fd->areamask[i];
        }

        if(areaDiff) {
            // a door just opened or something
            tr.refdef.areamaskModified = true;
        }
    }

    tr.refdef.sunDir[3] = 0.0f;
    tr.refdef.sunCol[3] = 1.0f;
    tr.refdef.sunAmbCol[3] = 1.0f;

    VectorCopy(tr.sunDirection, tr.refdef.sunDir);

    if((tr.refdef.rdflags & RDF_NOWORLDMODEL) || !(r_depthPrepass->value)) {
        VectorSet(tr.refdef.sunCol, 0, 0, 0);
        VectorSet(tr.refdef.sunAmbCol, 0, 0, 0);
    } else {
        float32 scale = (1 << r_mapOverBrightBits->integer) / 255.0f;

        if(r_forceSun->integer) {
            VectorScale(tr.sunLight, scale * r_forceSunLightScale->value,
                        tr.refdef.sunCol);
        } else {
            VectorScale(tr.sunLight, scale, tr.refdef.sunCol);
        }

        if(r_sunlightMode->integer == 1) {
            tr.refdef.sunAmbCol[0] =
                tr.refdef.sunAmbCol[1] =
                    tr.refdef.sunAmbCol[2] = r_forceSun->integer ?
                                             r_forceSunAmbientScale->value : tr.sunShadowScale;
        } else {
            if(r_forceSun->integer) {
                VectorScale(tr.sunLight, scale * r_forceSunAmbientScale->value,
                            tr.refdef.sunAmbCol);
            } else {
                VectorScale(tr.sunLight, scale * tr.sunShadowScale, tr.refdef.sunAmbCol);
            }
        }
    }

    if(r_forceAutoExposure->integer) {
        tr.refdef.autoExposureMinMax[0] = r_forceAutoExposureMin->value;
        tr.refdef.autoExposureMinMax[1] = r_forceAutoExposureMax->value;
    } else {
        tr.refdef.autoExposureMinMax[0] = tr.autoExposureMinMax[0];
        tr.refdef.autoExposureMinMax[1] = tr.autoExposureMinMax[1];
    }

    if(r_forceToneMap->integer) {
        tr.refdef.toneMinAvgMaxLinear[0] = powf(2.0f, r_forceToneMapMin->value);
        tr.refdef.toneMinAvgMaxLinear[1] = powf(2.0f, r_forceToneMapAvg->value);
        tr.refdef.toneMinAvgMaxLinear[2] = powf(2.0f, r_forceToneMapMax->value);
    } else {
        tr.refdef.toneMinAvgMaxLinear[0] = powf(2.0f, tr.toneMinAvgMaxLevel[0]);
        tr.refdef.toneMinAvgMaxLinear[1] = powf(2.0f, tr.toneMinAvgMaxLevel[1]);
        tr.refdef.toneMinAvgMaxLinear[2] = powf(2.0f, tr.toneMinAvgMaxLevel[2]);
    }

    // Makro - copy exta info if present
    if(fd->rdflags & RDF_EXTRA) {
        const refdefex_t *extra = (const refdefex_t *)(fd + 1);

        tr.refdef.blurFactor = extra->blurFactor;

        if(fd->rdflags & RDF_SUNLIGHT) {
            VectorCopy(extra->sunDir,    tr.refdef.sunDir);
            VectorCopy(extra->sunCol,    tr.refdef.sunCol);
            VectorCopy(extra->sunAmbCol, tr.refdef.sunAmbCol);
        }
    } else {
        tr.refdef.blurFactor = 0.0f;
    }

    // derived info

    tr.refdef.floatTime = tr.refdef.time * 0.001;

    tr.refdef.numDrawSurfs = r_firstSceneDrawSurf;
    tr.refdef.drawSurfs = backEndData->drawSurfs;

    tr.refdef.num_entities = r_numentities - r_firstSceneEntity;
    tr.refdef.entities = &backEndData->entities[r_firstSceneEntity];

    tr.refdef.num_dlights = r_numdlights - r_firstSceneDlight;
    tr.refdef.dlights = &backEndData->dlights[r_firstSceneDlight];

    tr.refdef.numPolys = r_numpolys - r_firstScenePoly;
    tr.refdef.polys = &backEndData->polys[r_firstScenePoly];

    tr.refdef.num_pshadows = 0;
    tr.refdef.pshadows = &backEndData->pshadows[0];

    // turn off dynamic lighting globally by clearing all the
    // dlights if it needs to be disabled or if vertex lighting is enabled
    if(r_dynamiclight->integer == 0 || r_vertexLight->integer == 1) {
        tr.refdef.num_dlights = 0;
    }

    // a single frame may have multiple scenes draw inside it --
    // a 3D game view, 3D status bar renderings, 3D menus, etc.
    // They need to be distinguished by the light flare code, because
    // the visibility state for a given surface may be different in
    // each scene / view.
    tr.frameSceneNum++;
    tr.sceneCount++;
}


void RE_EndScene() {
    // the next scene rendered in this frame will tack on after this one
    r_firstSceneDrawSurf = tr.refdef.numDrawSurfs;
    r_firstSceneEntity = r_numentities;
    r_firstSceneDlight = r_numdlights;
    r_firstScenePoly = r_numpolys;
}

/*
=====================
idRenderSystemLocal::RenderScene

Draw a 3D view into a part of the window, then return
to 2D drawing.

Rendering a scene may require multiple views to be rendered
to handle mirrors,
=====================
*/

void idRenderSystemLocal::RenderScene(const refdef_t *fd) {
    viewParms_t     parms;
    sint                startTime;

    if(!tr.registered) {
        return;
    }

    GLimp_LogComment("====== idRenderSystemLocal::RenderScene =====\n");

    if(r_norefresh->integer) {
        return;
    }

    startTime = clientRendererSystem->ScaledMilliseconds();

    if(!tr.world && !(fd->rdflags & RDF_NOWORLDMODEL)) {
        Com_Error(ERR_DROP,
                  "idRenderSystemLocal::RenderScene: nullptr worldmodel");
    }

    RE_BeginScene(fd);

    // SmileTheory: playing with shadow mapping
    if(!(fd->rdflags & RDF_NOWORLDMODEL) && tr.refdef.num_dlights &&
            r_dlightMode->integer >= 2) {
        R_RenderDlightCubemaps(fd);
    }

    /* playing with more shadows */
    if(glRefConfig.framebufferObject && !(fd->rdflags & RDF_NOWORLDMODEL) &&
            r_shadows->integer == 4) {
        R_RenderPshadowMaps(fd);
    }

    // playing with even more shadows
    if(glRefConfig.framebufferObject && r_sunlightMode->integer &&
            !(fd->rdflags & RDF_NOWORLDMODEL) && (r_forceSun->integer ||
                    tr.sunShadows)) {
        if(r_shadowCascadeZFar->integer != 0) {
            R_RenderSunShadowMaps(fd, 0);
            R_RenderSunShadowMaps(fd, 1);
            R_RenderSunShadowMaps(fd, 2);
        } else {
            Mat4Zero(tr.refdef.sunShadowMvp[0]);
            Mat4Zero(tr.refdef.sunShadowMvp[1]);
            Mat4Zero(tr.refdef.sunShadowMvp[2]);
        }

        // only rerender last cascade if sun has changed position
        if(r_forceSun->integer == 2 ||
                !VectorCompare(tr.refdef.sunDir, tr.lastCascadeSunDirection)) {
            VectorCopy(tr.refdef.sunDir, tr.lastCascadeSunDirection);
            R_RenderSunShadowMaps(fd, 3);
            Mat4Copy(tr.refdef.sunShadowMvp[3], tr.lastCascadeSunMvp);
        } else {
            Mat4Copy(tr.lastCascadeSunMvp, tr.refdef.sunShadowMvp[3]);
        }
    }

    // playing with cube maps
    // this is where dynamic cubemaps would be rendered
    if(0) { //( glRefConfig.framebufferObject && !( fd->rdflags & RDF_NOWORLDMODEL ))
        sint i, j;

        for(i = 0; i < tr.numCubemaps; i++) {
            for(j = 0; j < 6; j++) {
                R_RenderCubemapSide(i, j, true);
            }
        }
    }

    // setup view parms for the initial view
    //
    // set up viewport
    // The refdef takes 0-at-the-top y coordinates, so
    // convert to GL's 0-at-the-bottom space
    //
    ::memset(&parms, 0, sizeof(parms));
    parms.viewportX = tr.refdef.x;
    parms.viewportY = glConfig.vidHeight - (tr.refdef.y + tr.refdef.height);
    parms.viewportWidth = tr.refdef.width;
    parms.viewportHeight = tr.refdef.height;
    parms.isPortal = false;

    parms.fovX = tr.refdef.fov_x;
    parms.fovY = tr.refdef.fov_y;

    parms.stereoFrame = tr.refdef.stereoFrame;

    VectorCopy(fd->vieworg, parms.orientation.origin);
    VectorCopy(fd->viewaxis[0], parms.orientation.axis[0]);
    VectorCopy(fd->viewaxis[1], parms.orientation.axis[1]);
    VectorCopy(fd->viewaxis[2], parms.orientation.axis[2]);

    VectorCopy(fd->vieworg, parms.pvsOrigin);

    if(!(fd->rdflags & RDF_NOWORLDMODEL) && r_depthPrepass->value &&
            ((r_forceSun->integer) || tr.sunShadows)) {
        parms.flags = VPF_USESUNLIGHT;
    }

    R_RenderView(&parms);

    if(!(fd->rdflags & RDF_NOWORLDMODEL)) {
        R_AddPostProcessCmd();
    }

    RE_EndScene();

    tr.frontEndMsec += clientRendererSystem->ScaledMilliseconds() - startTime;
}
