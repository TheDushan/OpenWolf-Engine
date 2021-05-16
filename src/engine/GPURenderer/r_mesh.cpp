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
// File name:   r_mesh.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: triangle model functions
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

static float32 ProjectRadius(float32 r, vec3_t location) {
    float32 pr;
    float32 dist;
    float32 c;
    vec3_t  p;
    float32 projected[4];

    c = DotProduct(tr.viewParms.orientation.axis[0],
                   tr.viewParms.orientation.origin);
    dist = DotProduct(tr.viewParms.orientation.axis[0], location) - c;

    if(dist <= 0) {
        return 0;
    }

    p[0] = 0;
    p[1] = fabs(r);
    p[2] = -dist;

    projected[0] = p[0] * tr.viewParms.projectionMatrix[0] +
                   p[1] * tr.viewParms.projectionMatrix[4] +
                   p[2] * tr.viewParms.projectionMatrix[8] +
                   tr.viewParms.projectionMatrix[12];

    projected[1] = p[0] * tr.viewParms.projectionMatrix[1] +
                   p[1] * tr.viewParms.projectionMatrix[5] +
                   p[2] * tr.viewParms.projectionMatrix[9] +
                   tr.viewParms.projectionMatrix[13];

    projected[2] = p[0] * tr.viewParms.projectionMatrix[2] +
                   p[1] * tr.viewParms.projectionMatrix[6] +
                   p[2] * tr.viewParms.projectionMatrix[10] +
                   tr.viewParms.projectionMatrix[14];

    projected[3] = p[0] * tr.viewParms.projectionMatrix[3] +
                   p[1] * tr.viewParms.projectionMatrix[7] +
                   p[2] * tr.viewParms.projectionMatrix[11] +
                   tr.viewParms.projectionMatrix[15];


    pr = projected[1] / projected[3];

    if(pr > 1.0f) {
        pr = 1.0f;
    }

    return pr;
}

/*
=============
R_CullModel
=============
*/
static sint R_CullModel(mdvModel_t *model, trRefEntity_t *ent) {
    vec3_t      bounds[2];
    mdvFrame_t *oldFrame, *newFrame;
    sint            i;

    // compute frame pointers
    newFrame = model->frames + ent->e.frame;
    oldFrame = model->frames + ent->e.oldframe;

    // cull bounding sphere ONLY if this is not an upscaled entity
    if(!ent->e.nonNormalizedAxes) {
        if(ent->e.frame == ent->e.oldframe) {
            switch(R_CullLocalPointAndRadius(newFrame->localOrigin,
                                             newFrame->radius)) {
                case CULL_OUT:
                    tr.pc.c_sphere_cull_md3_out++;
                    return CULL_OUT;

                case CULL_IN:
                    tr.pc.c_sphere_cull_md3_in++;
                    return CULL_IN;

                case CULL_CLIP:
                    tr.pc.c_sphere_cull_md3_clip++;
                    break;
            }
        } else {
            sint sphereCull, sphereCullB;

            sphereCull  = R_CullLocalPointAndRadius(newFrame->localOrigin,
                                                    newFrame->radius);

            if(newFrame == oldFrame) {
                sphereCullB = sphereCull;
            } else {
                sphereCullB = R_CullLocalPointAndRadius(oldFrame->localOrigin,
                                                        oldFrame->radius);
            }

            if(sphereCull == sphereCullB) {
                if(sphereCull == CULL_OUT) {
                    tr.pc.c_sphere_cull_md3_out++;
                    return CULL_OUT;
                } else if(sphereCull == CULL_IN) {
                    tr.pc.c_sphere_cull_md3_in++;
                    return CULL_IN;
                } else {
                    tr.pc.c_sphere_cull_md3_clip++;
                }
            }
        }
    }

    // calculate a bounding box in the current coordinate system
    for(i = 0 ; i < 3 ; i++) {
        bounds[0][i] = oldFrame->bounds[0][i] < newFrame->bounds[0][i] ?
                       oldFrame->bounds[0][i] : newFrame->bounds[0][i];
        bounds[1][i] = oldFrame->bounds[1][i] > newFrame->bounds[1][i] ?
                       oldFrame->bounds[1][i] : newFrame->bounds[1][i];
    }

    switch(R_CullLocalBox(bounds)) {
        case CULL_IN:
            tr.pc.c_box_cull_md3_in++;
            return CULL_IN;

        case CULL_CLIP:
            tr.pc.c_box_cull_md3_clip++;
            return CULL_CLIP;

        case CULL_OUT:
        default:
            tr.pc.c_box_cull_md3_out++;
            return CULL_OUT;
    }
}


/*
=================
R_ComputeLOD

=================
*/
sint R_ComputeLOD(trRefEntity_t *ent) {
    float32 radius;
    float32 flod, lodscale;
    float32 projectedRadius;
    mdvFrame_t *frame;
    mdrHeader_t *mdr;
    mdrFrame_t *mdrframe;
    sint lod;

    if(tr.currentModel->numLods < 2) {
        // model has only 1 LOD level, skip computations and bias
        lod = 0;
    } else {
        // multiple LODs exist, so compute projected bounding sphere
        // and use that as a criteria for selecting LOD

        if(tr.currentModel->type == MOD_MDR) {
            sint frameSize;
            mdr = (mdrHeader_t *) tr.currentModel->modelData;
            frameSize = reinterpret_cast<uint64>(&((mdrFrame_t *)
                                                   0)->bones[mdr->numBones]);

            mdrframe = (mdrFrame_t *)(reinterpret_cast<uchar8 *>(mdr) + mdr->ofsFrames
                                      + frameSize * ent->e.frame);

            radius = RadiusFromBounds(mdrframe->bounds[0], mdrframe->bounds[1]);
        } else {
            frame = tr.currentModel->mdv[0]->frames;

            frame += ent->e.frame;

            radius = RadiusFromBounds(frame->bounds[0], frame->bounds[1]);
        }

        if((projectedRadius = ProjectRadius(radius, ent->e.origin)) != 0) {
            lodscale = r_lodscale->value;

            if(lodscale > 20) {
                lodscale = 20;
            }

            flod = 1.0f - projectedRadius * lodscale;
        } else {
            // object intersects near view plane, e.g. view weapon
            flod = 0;
        }

        flod *= tr.currentModel->numLods;
        lod = static_cast<sint>(flod);

        if(lod < 0) {
            lod = 0;
        } else if(lod >= tr.currentModel->numLods) {
            lod = tr.currentModel->numLods - 1;
        }
    }

    lod += r_lodbias->integer;

    if(lod >= tr.currentModel->numLods) {
        lod = tr.currentModel->numLods - 1;
    }

    if(lod < 0) {
        lod = 0;
    }

    return lod;
}

/*
=================
R_ComputeFogNum

=================
*/
sint R_ComputeFogNum(mdvModel_t *model, trRefEntity_t *ent) {
    sint                i, j;
    fog_t          *fog;
    mdvFrame_t     *mdvFrame;
    vec3_t          localOrigin;

    if(tr.refdef.rdflags & RDF_NOWORLDMODEL) {
        return 0;
    }

    // FIXME: non-normalized axis issues
    mdvFrame = model->frames + ent->e.frame;
    VectorAdd(ent->e.origin, mdvFrame->localOrigin, localOrigin);

    for(i = 1 ; i < tr.world->numfogs ; i++) {
        fog = &tr.world->fogs[i];

        for(j = 0 ; j < 3 ; j++) {
            if(localOrigin[j] - mdvFrame->radius >= fog->bounds[1][j]) {
                break;
            }

            if(localOrigin[j] + mdvFrame->radius <= fog->bounds[0][j]) {
                break;
            }
        }

        if(j == 3) {
            return i;
        }
    }

    return 0;
}

/*
=================
R_AddMD3Surfaces

=================
*/
void R_AddMD3Surfaces(trRefEntity_t *ent) {
    sint                i;
    mdvModel_t     *model = nullptr;
    mdvSurface_t   *surface = nullptr;
    shader_t       *shader = nullptr;
    sint                cull;
    sint                lod;
    sint                fogNum;
    sint             cubemapIndex;
    bool        personalModel;

    // don't add third_person objects if not in a portal
    personalModel = (ent->e.renderfx & RF_THIRD_PERSON) &&
                    !(tr.viewParms.isPortal
                      || (tr.viewParms.flags & (VPF_SHADOWMAP | VPF_DEPTHSHADOW)));

    if(ent->e.renderfx & RF_WRAP_FRAMES) {
        ent->e.frame %= tr.currentModel->mdv[0]->numFrames;
        ent->e.oldframe %= tr.currentModel->mdv[0]->numFrames;
    }

    //
    // Validate the frames so there is no chance of a crash.
    // This will write directly into the entity structure, so
    // when the surfaces are rendered, they don't need to be
    // range checked again.
    //
    if((ent->e.frame >= tr.currentModel->mdv[0]->numFrames)
            || (ent->e.frame < 0)
            || (ent->e.oldframe >= tr.currentModel->mdv[0]->numFrames)
            || (ent->e.oldframe < 0)) {
        CL_RefPrintf(PRINT_DEVELOPER,
                     "R_AddMD3Surfaces: no such frame %d to %d for '%s'\n",
                     ent->e.oldframe, ent->e.frame,
                     tr.currentModel->name);
        ent->e.frame = 0;
        ent->e.oldframe = 0;
    }

    //
    // compute LOD
    //
    lod = R_ComputeLOD(ent);

    model = tr.currentModel->mdv[lod];

    //
    // cull the entire model if merged bounding box of both frames
    // is outside the view frustum.
    //
    cull = R_CullModel(model, ent);

    if(cull == CULL_OUT) {
        return;
    }

    //
    // set up lighting now that we know we aren't culled
    //
    if(!personalModel || r_shadows->integer > 1) {
        R_SetupEntityLighting(&tr.refdef, ent);
    }

    //
    // see if we are in a fog volume
    //
    fogNum = R_ComputeFogNum(model, ent);

    cubemapIndex = R_CubemapForPoint(ent->e.origin);

    //
    // draw all surfaces
    //
    surface = model->surfaces;

    for(i = 0 ; i < model->numSurfaces ; i++) {

        if(ent->e.customShader) {
            shader = R_GetShaderByHandle(ent->e.customShader);
        } else if(ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins) {
            skin_t *skin;
            sint        j;

            skin = R_GetSkinByHandle(ent->e.customSkin);

            // match the surface name to something in the skin file
            shader = tr.defaultShader;

            for(j = 0 ; j < skin->numSurfaces ; j++) {
                // the names have both been lowercased
                if(!strcmp(skin->surfaces[j].name, surface->name)) {
                    shader = skin->surfaces[j].shader;
                    break;
                }
            }

            if(shader == tr.defaultShader) {
                CL_RefPrintf(PRINT_DEVELOPER,
                             "WARNING: no shader for surface %s in skin %s\n", surface->name,
                             skin->name);
            } else if(shader->defaultShader) {
                CL_RefPrintf(PRINT_DEVELOPER, "WARNING: shader %s in skin %s not found\n",
                             shader->name, skin->name);
            }

            //} else if ( surface->numShaders <= 0 ) {
            //shader = tr.defaultShader;
        } else {
            //md3Shader = (md3Shader_t *) ( (uchar8 *)surface + surface->ofsShaders );
            //md3Shader += ent->e.skinNum % surface->numShaders;
            //shader = tr.shaders[ md3Shader->shaderIndex ];
            shader = tr.shaders[ surface->shaderIndexes[ ent->e.skinNum %
                                                         surface->numShaderIndexes ] ];
        }

        // we will add shadows even if the main object isn't visible in the view

        // stencil shadows can't do personal models unless I polyhedron clip
        if(!personalModel
                && r_shadows->integer == 2
                && fogNum == 0
                && !(ent->e.renderfx & (RF_NOSHADOW | RF_DEPTHHACK))
                && shader->sort == SS_OPAQUE) {
            R_AddDrawSurf((surfaceType_t *)&model->vaoSurfaces[i], tr.shadowShader, 0,
                          false, false, 0);
        }

        // projection shadows work fine with personal models
        if(r_shadows->integer == 3
                && fogNum == 0
                && (ent->e.renderfx & RF_SHADOW_PLANE)
                && shader->sort == SS_OPAQUE) {
            R_AddDrawSurf((surfaceType_t *)&model->vaoSurfaces[i],
                          tr.projectionShadowShader, 0, false, false, 0);
        }

        // don't add third_person objects if not viewing through a portal
        if(!personalModel) {
            R_AddDrawSurf((surfaceType_t *)&model->vaoSurfaces[i], shader, fogNum,
                          false, false, cubemapIndex);
        }

        surface++;
    }

}

void R_AddObjectSurfaces(trRefEntity_t *ent) {
    objectModel_t *mod = tr.currentModel->object;
    objectSurface_t *surf;
    shader_t *shader = 0;
    bool personalModel;
    sint fogNum;

    // don't add third_person objectects if not in a portal
    personalModel = (bool)((ent->e.renderfx & RF_THIRD_PERSON) &&
                           !tr.viewParms.isPortal);

    //
    // set up lighting now that we know we aren't culled
    //
    if(!personalModel || r_shadows->integer > 1) {
        R_SetupEntityLighting(&tr.refdef, ent);
    }

    //
    // see if we are in a fog volume
    //
    fogNum = 0;

    //
    // draw all surfaces
    //

    surf = mod->surfaces;

    for(int i = 0; i < mod->numSurfaces; surf++, i++) {
        if(ent->e.customShader) {
            shader = R_GetShaderByHandle(ent->e.customShader);
        } else if(surf->shaderIndex) {
            shader = tr.shaders[surf->shaderIndex];
        } else {
            shader = tr.defaultShader;
        }

        // we will add shadows even if the main object isn't visible in the view

        // stencil shadows can't do personal models unless I polyhedron clip
        if(!personalModel
                && r_shadows->integer == 2
                && fogNum == 0
                && !(ent->e.renderfx & (RF_NOSHADOW | RF_DEPTHHACK))
                && shader->sort == SS_OPAQUE) {
            R_AddDrawSurf((surfaceType_t *)surf, tr.shadowShader, 0, false, false, 0);
        }

        // projection shadows work fine with personal models
        if(r_shadows->integer == 3
                && fogNum == 0
                && (ent->e.renderfx & RF_SHADOW_PLANE)
                && shader->sort == SS_OPAQUE) {
            R_AddDrawSurf((surfaceType_t *)surf, tr.projectionShadowShader, 0, false,
                          false, 0);
        }

        // don't add third_person objects if not viewing through a portal
        if(!personalModel) {
            R_AddDrawSurf((surfaceType_t *)surf, shader, fogNum, false, false, 0);
        }
    }

}
