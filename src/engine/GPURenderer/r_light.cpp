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
// File name:   r_light.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

#define DLIGHT_AT_RADIUS        16
// at the edge of a dlight's influence, this amount of light will be added

#define DLIGHT_MINIMUM_RADIUS   16
// never calculate a range less than this to prevent huge light numbers

#define DLIGHT_SHADOW_MINZDIST 30
// so the shadows won't get to long.

/*
===============
R_TransformDlights

Transforms the origins of an array of dlights.
Used by both the front end (for DlightBmodel) and
the back end (before doing the lighting calculation)
===============
*/
void R_TransformDlights(sint count, dlight_t *dl,
                        orientationr_t *orientation) {
    sint        i;
    vec3_t  temp;

    for(i = 0 ; i < count ; i++, dl++) {
        VectorSubtract(dl->origin, orientation->origin, temp);
        dl->transformed[0] = DotProduct(temp, orientation->axis[0]);
        dl->transformed[1] = DotProduct(temp, orientation->axis[1]);
        dl->transformed[2] = DotProduct(temp, orientation->axis[2]);
    }
}

/*
=============
R_DlightBmodel

Determine which dynamic lights may effect this bmodel
=============
*/
void R_DlightBmodel(bmodel_t *bmodel) {
    sint            i, j;
    dlight_t   *dl;
    sint            mask;
    msurface_t *surf;

    // transform all the lights
    R_TransformDlights(tr.refdef.num_dlights, tr.refdef.dlights,
                       &tr.orientation);

    mask = 0;

    for(i = 0 ; i < tr.refdef.num_dlights ; i++) {
        dl = &tr.refdef.dlights[i];

        // see if the point is close enough to the bounds to matter
        for(j = 0 ; j < 3 ; j++) {
            if(dl->transformed[j] - bmodel->bounds[1][j] > dl->radius) {
                break;
            }

            if(bmodel->bounds[0][j] - dl->transformed[j] > dl->radius) {
                break;
            }
        }

        if(j < 3) {
            continue;
        }

        // we need to check this light
        mask |= 1 << i;
    }

    tr.currentEntity->needDlights = (mask != 0);

    // set the dlight bits in all the surfaces
    for(i = 0 ; i < bmodel->numSurfaces ; i++) {
        surf = tr.world->surfaces + bmodel->firstSurface + i;

        switch(*surf->data) {
            case SF_FACE:
            case SF_GRID:
            case SF_TRIANGLES:
                ((srfBspSurface_t *)surf->data)->dlightBits = mask;
                break;

            default:
                break;
        }
    }
}


/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

extern  convar_t   *r_ambientScale;
extern  convar_t   *r_directedScale;
extern  convar_t   *r_debugLight;

/*
=================
R_SetupEntityLightingGrid

=================
*/
static void R_SetupEntityLightingGrid(trRefEntity_t *ent, world_t *world) {
    vec3_t  lightOrigin;
    sint        pos[3];
    sint        i, j;
    uchar8 *gridData;
    float32 frac[3];
    sint        gridStep[3];
    vec3_t  direction;
    float32 totalFactor;

    if(ent->e.renderfx & RF_LIGHTING_ORIGIN) {
        // seperate lightOrigins are needed so an object that is
        // sinking into the ground can still be lit, and so
        // multi-part models can be lit identically
        VectorCopy(ent->e.lightingOrigin, lightOrigin);
    } else {
        VectorCopy(ent->e.origin, lightOrigin);
    }

    VectorSubtract(lightOrigin, world->lightGridOrigin, lightOrigin);

    for(i = 0 ; i < 3 ; i++) {
        float32 v;

        v = lightOrigin[i] * world->lightGridInverseSize[i];
        pos[i] = floor(v);
        frac[i] = v - pos[i];

        if(pos[i] < 0) {
            pos[i] = 0;
        } else if(pos[i] > world->lightGridBounds[i] - 1) {
            pos[i] = world->lightGridBounds[i] - 1;
        }
    }

    VectorClear(ent->ambientLight);
    VectorClear(ent->directedLight);
    VectorClear(direction);

    assert(world->lightGridData);   // nullptr with -nolight maps

    // trilerp the light value
    gridStep[0] = 8;
    gridStep[1] = 8 * world->lightGridBounds[0];
    gridStep[2] = 8 * world->lightGridBounds[0] * world->lightGridBounds[1];
    gridData = world->lightGridData + pos[0] * gridStep[0]
               + pos[1] * gridStep[1] + pos[2] * gridStep[2];

    totalFactor = 0;

    for(i = 0 ; i < 8 ; i++) {
        float32 factor;
        uchar8 *data;
        sint        lat, lng;
        vec3_t  normal;
#if idppc
        float32 d0, d1, d2, d3, d4, d5;
#endif
        factor = 1.0;
        data = gridData;

        for(j = 0 ; j < 3 ; j++) {
            if(i & (1 << j)) {
                if(pos[j] + 1 > world->lightGridBounds[j] - 1) {
                    break; // ignore values outside lightgrid
                }

                factor *= frac[j];
                data += gridStep[j];
            } else {
                factor *= (1.0f - frac[j]);
            }
        }

        if(j != 3) {
            continue;
        }

        if(world->lightGrid16) {
            uchar16 *data16 = world->lightGrid16 + static_cast<sint>
                              (data - world->lightGridData) / 8 * 6;

            if(!(data16[0] + data16[1] + data16[2] + data16[3] + data16[4] +
                    data16[5])) {
                continue;   // ignore samples in walls
            }
        } else {
            if(!(data[0] + data[1] + data[2] + data[3] + data[4] + data[5])) {
                continue;   // ignore samples in walls
            }
        }

        totalFactor += factor;
#if idppc
        d0 = data[0];
        d1 = data[1];
        d2 = data[2];
        d3 = data[3];
        d4 = data[4];
        d5 = data[5];

        ent->ambientLight[0] += factor * d0;
        ent->ambientLight[1] += factor * d1;
        ent->ambientLight[2] += factor * d2;

        ent->directedLight[0] += factor * d3;
        ent->directedLight[1] += factor * d4;
        ent->directedLight[2] += factor * d5;
#else

        if(world->lightGrid16) {
            // FIXME: this is hideous
            uchar16 *data16 = world->lightGrid16 + static_cast<sint>
                              (data - world->lightGridData) / 8 * 6;

            ent->ambientLight[0] += factor * data16[0] / 257.0f;
            ent->ambientLight[1] += factor * data16[1] / 257.0f;
            ent->ambientLight[2] += factor * data16[2] / 257.0f;

            ent->directedLight[0] += factor * data16[3] / 257.0f;
            ent->directedLight[1] += factor * data16[4] / 257.0f;
            ent->directedLight[2] += factor * data16[5] / 257.0f;
        } else {
            ent->ambientLight[0] += factor * data[0];
            ent->ambientLight[1] += factor * data[1];
            ent->ambientLight[2] += factor * data[2];

            ent->directedLight[0] += factor * data[3];
            ent->directedLight[1] += factor * data[4];
            ent->directedLight[2] += factor * data[5];
        }

#endif
        lat = data[7];
        lng = data[6];
        lat *= (FUNCTABLE_SIZE / 256);
        lng *= (FUNCTABLE_SIZE / 256);

        // decode X as cos( lat ) * sin( long )
        // decode Y as sin( lat ) * sin( long )
        // decode Z as cos( long )

        normal[0] = tr.sinTable[(lat + (FUNCTABLE_SIZE / 4))&FUNCTABLE_MASK] *
                    tr.sinTable[lng];
        normal[1] = tr.sinTable[lat] * tr.sinTable[lng];
        normal[2] = tr.sinTable[(lng + (FUNCTABLE_SIZE / 4))&FUNCTABLE_MASK];

        VectorMA(direction, factor, normal, direction);
    }

    if(totalFactor > 0 && totalFactor < 0.99) {
        totalFactor = 1.0f / totalFactor;
        VectorScale(ent->ambientLight, totalFactor, ent->ambientLight);
        VectorScale(ent->directedLight, totalFactor, ent->directedLight);
    }

    VectorScale(ent->ambientLight, r_ambientScale->value, ent->ambientLight);
    VectorScale(ent->directedLight, r_directedScale->value,
                ent->directedLight);

    if(tr.lightGridMulAmbient) {
        VectorScale(ent->ambientLight, tr.lightGridMulAmbient, ent->ambientLight);
    }

    if(tr.lightGridMulDirected) {
        VectorScale(ent->directedLight, tr.lightGridMulDirected,
                    ent->directedLight);
    }

    VectorNormalize2(direction, ent->lightDir);
}


/*
===============
LogLight
===============
*/
static void LogLight(trRefEntity_t *ent) {
    sint    max1, max2;

    if(!(ent->e.renderfx & RF_FIRST_PERSON)) {
        return;
    }

    max1 = ent->ambientLight[0];

    if(ent->ambientLight[1] > max1) {
        max1 = ent->ambientLight[1];
    } else if(ent->ambientLight[2] > max1) {
        max1 = ent->ambientLight[2];
    }

    max2 = ent->directedLight[0];

    if(ent->directedLight[1] > max2) {
        max2 = ent->directedLight[1];
    } else if(ent->directedLight[2] > max2) {
        max2 = ent->directedLight[2];
    }

    CL_RefPrintf(PRINT_ALL, "amb:%i  dir:%i\n", max1, max2);
}

/*
=================
R_AddLightToEntity

Adds a light to a entity, there is a fixed number of lights that can
effect a entity.
=================
*/

void R_AddLightToEntity(trRefEntity_t *ent, dlight_t *light,
                        float32 dist) {
    // Fill all the light slots first.
    for(sint i = 0; i < MAX_ENTITY_LIGHTS; i++) {
        if(ent->lights[i] == nullptr) {
            ent->lights[i] = light;
            return;
        }
    }

    // See if the light is closer than any of the current that effect this entity.
    for(sint i = 0; i < MAX_ENTITY_LIGHTS; i++) {
        if(dist < ent->lightdist[i]) {
            ent->lights[i] = light;
            ent->lightdist[i] = dist;
            return;
        }
    }
}

/*
=================
R_SetupEntityLighting

Calculates all the lighting values that will be used
by the Calc_* functions
=================
*/
void R_SetupEntityLighting(const trRefdef_t *refdef, trRefEntity_t *ent) {
    sint                i;
    dlight_t       *dl;
    float32         power;
    vec3_t          dir;
    float32         d;
    vec3_t          lightDir;
    vec3_t          lightOrigin;

    // lighting calculations
    if(ent->lightingCalculated) {
        return;
    }

    ent->lightingCalculated = true;

    for(i = 0; i < MAX_ENTITY_LIGHTS; i++) {
        ent->lights[i] = nullptr;
        ent->lightdist[i] = 9999;
    }

    //
    // trace a sample point down to find ambient light
    //
    if(ent->e.renderfx & RF_LIGHTING_ORIGIN) {
        // seperate lightOrigins are needed so an object that is
        // sinking into the ground can still be lit, and so
        // multi-part models can be lit identically
        VectorCopy(ent->e.lightingOrigin, lightOrigin);
    } else {
        VectorCopy(ent->e.origin, lightOrigin);
    }

    // if NOWORLDMODEL, only use dynamic lights (menu system, etc)
    if(!(refdef->rdflags & RDF_NOWORLDMODEL)
            && tr.world->lightGridData) {
        R_SetupEntityLightingGrid(ent, tr.world);
    } else {
        ent->ambientLight[0] = ent->ambientLight[1] =
                                   ent->ambientLight[2] = tr.identityLight * 150;
        ent->directedLight[0] = ent->directedLight[1] =
                                    ent->directedLight[2] = tr.identityLight * 150;
        VectorCopy(tr.sunDirection, ent->lightDir);
    }

    // bonus items and view weapons have a fixed minimum add
    if(!r_hdr->integer /* ent->e.renderfx & RF_MINLIGHT */) {
        // give everything a minimum light add
        ent->ambientLight[0] += tr.identityLight * 32;
        ent->ambientLight[1] += tr.identityLight * 32;
        ent->ambientLight[2] += tr.identityLight * 32;
    }

    //
    // modify the light by dynamic lights
    //
    d = VectorLength(ent->directedLight);
    VectorScale(ent->lightDir, d, lightDir);

    for(i = 0 ; i < refdef->num_dlights ; i++) {
        dl = &refdef->dlights[i];
        VectorSubtract(dl->origin, lightOrigin, dir);

        if(r_pbr->integer) {
            d = VectorNormalize(dir);
            float32 sqrd = d * d;
            float32 sqrr = dl->radius * dl->radius;
            float32 factor = sqrd / sqrr;
            factor = Com_Clamp(0.0f, 1.0f, (1.0f - (factor * factor)));
            factor *= factor;

            d = factor / d;
            d *= DLIGHT_AT_RADIUS * dl->radius;
        } else {
            d = VectorNormalize(dir);
            power = DLIGHT_AT_RADIUS * (dl->radius * dl->radius);

            if(d < DLIGHT_MINIMUM_RADIUS) {
                d = DLIGHT_MINIMUM_RADIUS;
            }

            d = power / (d * d);
        }

        R_AddLightToEntity(ent, dl, d);

        VectorMA(ent->directedLight, d, dl->color, ent->directedLight);
        VectorMA(lightDir, d, dir, lightDir);
    }

    // clamp lights
    // FIXME: old renderer clamps (ambient + NL * directed) per vertex
    //        check if that's worth implementing
    {
        float32 r, g, b, max;

        r = ent->ambientLight[0];
        g = ent->ambientLight[1];
        b = ent->ambientLight[2];

        max = MAX(MAX(r, g), b);

        if(max > 255.0f) {
            max = 255.0f / max;
            ent->ambientLight[0] *= max;
            ent->ambientLight[1] *= max;
            ent->ambientLight[2] *= max;
        }

        r = ent->directedLight[0];
        g = ent->directedLight[1];
        b = ent->directedLight[2];

        max = MAX(MAX(r, g), b);

        if(max > 255.0f) {
            max = 255.0f / max;
            ent->directedLight[0] *= max;
            ent->directedLight[1] *= max;
            ent->directedLight[2] *= max;
        }
    }


    if(r_debugLight->integer) {
        LogLight(ent);
    }

    // save out the uchar8 packet version
    (reinterpret_cast<uchar8 *>(&ent->ambientLightInt))[0] = static_cast<sint>
            (ent->ambientLight[0]);
    (reinterpret_cast<uchar8 *>(&ent->ambientLightInt))[1] = static_cast<sint>
            (ent->ambientLight[1]);
    (reinterpret_cast<uchar8 *>(&ent->ambientLightInt))[2] = static_cast<sint>
            (ent->ambientLight[2]);
    (reinterpret_cast<uchar8 *>(&ent->ambientLightInt))[3] = 0xff;

    // transform the direction to local space
    VectorNormalize(lightDir);
    ent->modelLightDir[0] = DotProduct(lightDir, ent->e.axis[0]);
    ent->modelLightDir[1] = DotProduct(lightDir, ent->e.axis[1]);
    ent->modelLightDir[2] = DotProduct(lightDir, ent->e.axis[2]);

    VectorCopy(lightDir, ent->lightDir);
}

/*
=================
idRenderSystemLocal::LightForPoint
=================
*/
bool idRenderSystemLocal::LightForPoint(vec3_t point, vec3_t ambientLight,
                                        vec3_t directedLight, vec3_t lightDir) {
    trRefEntity_t ent;

    if(tr.world->lightGridData == nullptr) {
        return false;
    }

    ::memset(&ent, 0, sizeof(ent));
    VectorCopy(point, ent.e.origin);
    R_SetupEntityLightingGrid(&ent, tr.world);
    VectorCopy(ent.ambientLight, ambientLight);
    VectorCopy(ent.directedLight, directedLight);
    VectorCopy(ent.lightDir, lightDir);

    return true;
}


bool R_LightDirForPoint(vec3_t point, vec3_t lightDir, vec3_t normal,
                        world_t *world) {
    trRefEntity_t ent;

    if(world->lightGridData == nullptr) {
        return false;
    }

    ::memset(&ent, 0, sizeof(ent));
    VectorCopy(point, ent.e.origin);
    R_SetupEntityLightingGrid(&ent, world);

    if(DotProduct(ent.lightDir, normal) > 0.2f) {
        VectorCopy(ent.lightDir, lightDir);
    } else {
        VectorCopy(normal, lightDir);
    }

    return true;
}


sint R_CubemapForPoint(vec3_t point) {
    sint cubemapIndex = -1;

    if(r_cubeMapping->integer && tr.numCubemaps) {
        sint i;
        vec_t shortest = static_cast<float32>(WORLD_SIZE) * static_cast<float32>
                         (WORLD_SIZE);

        for(i = 0; i < tr.numCubemaps; i++) {
            vec3_t diff;
            vec_t length;

            VectorSubtract(point, tr.cubemaps[i].origin, diff);
            length = DotProduct(diff, diff);

            if(shortest > length) {
                shortest = length;
                cubemapIndex = i;
            }
        }
    }

    return cubemapIndex + 1;
}
