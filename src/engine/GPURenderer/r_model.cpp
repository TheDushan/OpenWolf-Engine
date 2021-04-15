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
// File name:   r_models.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: model loading and caching
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

#define LL(x) x=LittleLong(x)

static bool R_LoadMD3(model_t *mod, sint lod, void *buffer,
                      sint bufferSize, pointer modName);
static bool R_LoadMDR(model_t *mod, void *buffer, sint filesize,
                      pointer name);

/*
====================
R_RegisterMD3
====================
*/
qhandle_t R_RegisterMD3(pointer name, model_t *mod) {
    union {
        uint *u;
        void *v;
    } buf;
    sint         size;
    sint            lod;
    sint            ident;
    bool    loaded = false;
    sint            numLoaded;
    valueType filename[MAX_QPATH], namebuf[MAX_QPATH + 20];
    valueType *fext, defex[] = "md3";

    numLoaded = 0;

    Q_strcpy_s(filename, name);

    fext = strchr(filename, '.');

    if(!fext) {
        fext = defex;
    } else {
        *fext = '\0';
        fext++;
    }

    for(lod = MD3_MAX_LODS - 1 ; lod >= 0 ; lod--) {
        if(lod) {
            Q_vsprintf_s(namebuf, sizeof(namebuf), sizeof(namebuf), "%s_%d.%s",
                         filename, lod, fext);
        } else {
            Q_vsprintf_s(namebuf, sizeof(namebuf), sizeof(namebuf), "%s.%s", filename,
                         fext);
        }

        size = fileSystem->ReadFile(namebuf, &buf.v);

        // We want to know when something is missing
        if(!lod && !buf.u) {
            CL_RefPrintf(PRINT_DEVELOPER, "Could not find MD3 model file %s\n",
                         namebuf);
        }

        if(!buf.u) {
            continue;
        }

        ident = LittleLong(*static_cast<uint *>(buf.u));

        if(ident == MD3_IDENT) {
            loaded = R_LoadMD3(mod, lod, buf.u, size, name);
        } else {
            CL_RefPrintf(PRINT_WARNING, "R_RegisterMD3: unknown fileid for %s\n",
                         name);
        }

        fileSystem->FreeFile(buf.v);

        if(loaded) {
            mod->numLods++;
            numLoaded++;
        } else {
            break;
        }
    }

    if(numLoaded) {
        // duplicate into higher lod spots that weren't
        // loaded, in case the user changes r_lodbias on the fly
        for(lod--; lod >= 0; lod--) {
            mod->numLods++;
            mod->mdv[lod] = mod->mdv[lod + 1];
        }

        return mod->index;
    }

    CL_RefPrintf(PRINT_DEVELOPER, "R_RegisterMD3: couldn't load %s\n", name);

    mod->type = MOD_BAD;
    return 0;
}

/*
====================
R_RegisterMDR
====================
*/
qhandle_t R_RegisterMDR(pointer name, model_t *mod) {
    union {
        uint *u;
        void *v;
    } buf;
    sint    ident;
    bool loaded = false;
    sint filesize;

    filesize = fileSystem->ReadFile(name, (void **) &buf.v);

    if(!buf.u) {
        mod->type = MOD_BAD;
        return 0;
    }

    ident = LittleLong(*static_cast<uint *>(buf.u));

    if(ident == MDR_IDENT) {
        loaded = R_LoadMDR(mod, buf.u, filesize, name);
    }

    fileSystem->FreeFile(buf.v);

    if(!loaded) {
        CL_RefPrintf(PRINT_WARNING, "R_RegisterMDR: couldn't load mdr file %s\n",
                     name);
        mod->type = MOD_BAD;
        return 0;
    }

    return mod->index;
}

/*
====================
R_RegisterIQM
====================
*/
qhandle_t R_RegisterIQM(pointer name, model_t *mod) {
    union {
        uint *u;
        void *v;
    } buf;
    bool loaded = false;
    sint filesize;

    filesize = fileSystem->ReadFile(name, (void **) &buf.v);

    if(!buf.u) {
        mod->type = MOD_BAD;
        return 0;
    }

    loaded = R_LoadIQM(mod, buf.u, filesize, name);

    fileSystem->FreeFile(buf.v);

    if(!loaded) {
        CL_RefPrintf(PRINT_WARNING, "R_RegisterIQM: couldn't load iqm file %s\n",
                     name);
        mod->type = MOD_BAD;
        return 0;
    }

    return mod->index;
}


struct modelExtToLoaderMap_t {
    pointer ext;
    qhandle_t (*ModelLoader)(pointer, model_t *);
};

// Note that the ordering indicates the order of preference used
// when there are multiple models of different formats available
static modelExtToLoaderMap_t modelLoaders[ ] = {
    { "iqm", R_RegisterIQM },
    { "mdr", R_RegisterMDR },
    { "md3", R_RegisterMD3 }
};

static sint numModelLoaders = ARRAY_LEN(modelLoaders);

//===============================================================================

/*
** R_GetModelByHandle
*/
model_t    *R_GetModelByHandle(qhandle_t index) {
    model_t        *mod;

    // out of range gets the defualt model
    if(index < 1 || index >= tr.numModels) {
        return tr.models[0];
    }

    mod = tr.models[index];

    return mod;
}

//===============================================================================

/*
** R_AllocModel
*/
model_t *R_AllocModel(void) {
    model_t        *mod = nullptr;

    if(tr.numModels == MAX_MOD_KNOWN) {
        return nullptr;
    }

    mod = reinterpret_cast<model_t *>(Hunk_Alloc(sizeof(
                                          *tr.models[tr.numModels]), h_low));
    mod->index = tr.numModels;
    tr.models[tr.numModels] = mod;
    tr.numModels++;

    return mod;
}

/*
====================
idRenderSystemLocal::RegisterModel

Loads in a model for the given name

Zero will be returned if the model fails to load.
An entry will be retained for failed models as an
optimization to prevent disk rescanning if they are
asked for again.
====================
*/
qhandle_t idRenderSystemLocal::RegisterModel(pointer name) {
    model_t        *mod;
    qhandle_t   hModel;
    bool    orgNameFailed = false;
    sint            orgLoader = -1;
    sint            i;
    valueType       localName[ MAX_QPATH ];
    pointer ext;
    valueType       altName[ MAX_QPATH ];

    if(!name || !name[0]) {
        CL_RefPrintf(PRINT_ALL,
                     "idRenderSystemLocal::RegisterModel: nullptr name\n");
        return 0;
    }

    if(strlen(name) >= MAX_QPATH) {
        CL_RefPrintf(PRINT_ALL, "Model name exceeds MAX_QPATH\n");
        return 0;
    }

    //
    // search the currently loaded models
    //
    for(hModel = 1 ; hModel < tr.numModels; hModel++) {
        mod = tr.models[hModel];

        if(!strcmp(mod->name, name)) {
            if(mod->type == MOD_BAD) {
                return 0;
            }

            return hModel;
        }
    }

    // allocate a new model_t

    if((mod = R_AllocModel()) == nullptr) {
        CL_RefPrintf(PRINT_WARNING,
                     "idRenderSystemLocal::RegisterModel: R_AllocModel() failed for '%s'\n",
                     name);
        return 0;
    }

    // only set the name after the model has been successfully loaded
    Q_strncpyz(mod->name, name, sizeof(mod->name));


    R_IssuePendingRenderCommands();

    mod->type = MOD_BAD;
    mod->numLods = 0;

    //
    // load the files
    //
    Q_strncpyz(localName, name, MAX_QPATH);

    ext = COM_GetExtension(localName);

    if(*ext) {
        // Look for the correct loader and use it
        for(i = 0; i < numModelLoaders; i++) {
            if(!Q_stricmp(ext, modelLoaders[ i ].ext)) {
                // Load
                hModel = modelLoaders[ i ].ModelLoader(localName, mod);
                break;
            }
        }

        // A loader was found
        if(i < numModelLoaders) {
            if(!hModel) {
                // Loader failed, most likely because the file isn't there;
                // try again without the extension
                orgNameFailed = true;
                orgLoader = i;
                COM_StripExtension2(name, localName, MAX_QPATH);
            } else {
                // Something loaded
                return mod->index;
            }
        }
    }

    // Try and find a suitable match using all
    // the model formats supported
    for(i = 0; i < numModelLoaders; i++) {
        if(i == orgLoader) {
            continue;
        }

        Q_vsprintf_s(altName, sizeof(altName), sizeof(altName), "%s.%s", localName,
                     modelLoaders[ i ].ext);

        // Load
        hModel = modelLoaders[ i ].ModelLoader(altName, mod);

        if(hModel) {
            if(orgNameFailed) {
#ifdef _DEBUG
                CL_RefPrintf(PRINT_DEVELOPER,
                             "WARNING: %s not present, using %s instead\n", name, altName);
#endif
            }

            break;
        }
    }

    return hModel;
}

/*
=================
R_LoadMD3
=================
*/
static bool R_LoadMD3(model_t *mod, sint lod, void *buffer,
                      sint bufferSize, pointer modName) {
    sint             f, i;
    uint64 j;
    md3Header_t    *md3Model;
    md3Frame_t     *md3Frame;
    md3Surface_t   *md3Surf;
    md3Shader_t    *md3Shader;
    md3Triangle_t  *md3Tri;
    md3St_t        *md3st;
    md3XyzNormal_t *md3xyz;
    md3Tag_t       *md3Tag;

    mdvModel_t     *mdvModel = nullptr;
    mdvFrame_t     *frame = nullptr;
    mdvSurface_t   *surf = nullptr;//, *surface;
    sint            *shaderIndex = nullptr;
    uint       *tri = nullptr;
    mdvVertex_t    *v = nullptr;
    mdvSt_t        *st = nullptr;
    mdvTag_t       *tag = nullptr;
    mdvTagName_t   *tagName = nullptr;

    sint             version;
    sint             size;

    md3Model = (md3Header_t *) buffer;

    version = LittleLong(md3Model->version);

    if(version != MD3_VERSION) {
        CL_RefPrintf(PRINT_WARNING,
                     "R_LoadMD3: %s has wrong version (%i should be %i)\n", modName, version,
                     MD3_VERSION);
        return false;
    }

    mod->type = MOD_MESH;
    size = LittleLong(md3Model->ofsEnd);
    mod->dataSize += size;
    mdvModel = mod->mdv[lod] = reinterpret_cast<mdvModel_t *>(Hunk_Alloc(
                                   sizeof(mdvModel_t), h_low));

    //  ::memcpy(mod->md3[lod], buffer, LittleLong(md3Model->ofsEnd));

    LL(md3Model->ident);
    LL(md3Model->version);
    LL(md3Model->numFrames);
    LL(md3Model->numTags);
    LL(md3Model->numSurfaces);
    LL(md3Model->ofsFrames);
    LL(md3Model->ofsTags);
    LL(md3Model->ofsSurfaces);
    LL(md3Model->ofsEnd);

    if(md3Model->numFrames < 1) {
        CL_RefPrintf(PRINT_WARNING, "R_LoadMD3: %s has no frames\n", modName);
        return false;
    }

    // swap all the frames
    mdvModel->numFrames = md3Model->numFrames;
    mdvModel->frames = frame = reinterpret_cast<mdvFrame_t *>(Hunk_Alloc(
                                   sizeof(*frame) * md3Model->numFrames, h_low));

    md3Frame = (md3Frame_t *)(reinterpret_cast<uchar8 *>(md3Model) +
                              md3Model->ofsFrames);

    for(i = 0; i < md3Model->numFrames; i++, frame++, md3Frame++) {
        frame->radius = LittleFloat(md3Frame->radius);

        for(j = 0; j < 3; j++) {
            frame->bounds[0][j] = LittleFloat(md3Frame->bounds[0][j]);
            frame->bounds[1][j] = LittleFloat(md3Frame->bounds[1][j]);
            frame->localOrigin[j] = LittleFloat(md3Frame->localOrigin[j]);
        }
    }

    // swap all the tags
    mdvModel->numTags = md3Model->numTags;
    mdvModel->tags = tag = reinterpret_cast<mdvTag_t *>(Hunk_Alloc(sizeof(
                               *tag) * (md3Model->numTags * md3Model->numFrames), h_low));

    md3Tag = (md3Tag_t *)(reinterpret_cast<uchar8 *>(md3Model) +
                          md3Model->ofsTags);

    for(i = 0; i < md3Model->numTags * md3Model->numFrames;
            i++, tag++, md3Tag++) {
        for(j = 0; j < 3; j++) {
            tag->origin[j] = LittleFloat(md3Tag->origin[j]);
            tag->axis[0][j] = LittleFloat(md3Tag->axis[0][j]);
            tag->axis[1][j] = LittleFloat(md3Tag->axis[1][j]);
            tag->axis[2][j] = LittleFloat(md3Tag->axis[2][j]);
        }
    }


    mdvModel->tagNames = tagName = reinterpret_cast<mdvTagName_t *>(Hunk_Alloc(
                                       sizeof(*tagName) * (md3Model->numTags), h_low));

    md3Tag = (md3Tag_t *)(reinterpret_cast<uchar8 *>(md3Model) +
                          md3Model->ofsTags);

    for(i = 0; i < md3Model->numTags; i++, tagName++, md3Tag++) {
        Q_strncpyz(tagName->name, md3Tag->name, sizeof(tagName->name));
    }

    // swap all the surfaces
    mdvModel->numSurfaces = md3Model->numSurfaces;
    mdvModel->surfaces = surf = reinterpret_cast<mdvSurface_t *>(Hunk_Alloc(
                                    sizeof(*surf) * md3Model->numSurfaces, h_low));

    md3Surf = (md3Surface_t *)(reinterpret_cast<uchar8 *>
                               (md3Model) + md3Model->ofsSurfaces);

    for(i = 0; i < md3Model->numSurfaces; i++) {
        LL(md3Surf->ident);
        LL(md3Surf->flags);
        LL(md3Surf->numFrames);
        LL(md3Surf->numShaders);
        LL(md3Surf->numTriangles);
        LL(md3Surf->ofsTriangles);
        LL(md3Surf->numVerts);
        LL(md3Surf->ofsShaders);
        LL(md3Surf->ofsSt);
        LL(md3Surf->ofsXyzNormals);
        LL(md3Surf->ofsEnd);

        if(md3Surf->numVerts >= SHADER_MAX_VERTEXES) {
            CL_RefPrintf(PRINT_WARNING,
                         "R_LoadMD3: %s has more than %i verts on %s (%i).\n",
                         modName, SHADER_MAX_VERTEXES - 1,
                         md3Surf->name[0] ? md3Surf->name : "a surface",
                         md3Surf->numVerts);
            return false;
        }

        if(md3Surf->numTriangles * 3 >= SHADER_MAX_INDEXES) {
            CL_RefPrintf(PRINT_WARNING,
                         "R_LoadMD3: %s has more than %i triangles on %s (%i).\n",
                         modName, (SHADER_MAX_INDEXES / 3) - 1,
                         md3Surf->name[0] ? md3Surf->name : "a surface",
                         md3Surf->numTriangles);
            return false;
        }

        // change to surface identifier
        surf->surfaceType = SF_MDV;

        // give pointer to model for Tess_SurfaceMDX
        surf->model = mdvModel;

        // copy surface name
        Q_strncpyz(surf->name, md3Surf->name, sizeof(surf->name));

        // lowercase the surface name so skin compares are faster
        Q_strlwr(surf->name);

        // strip off a trailing _1 or _2
        // this is a crutch for q3data being a mess
        j = strlen(surf->name);

        if(j > 2 && surf->name[j - 2] == '_') {
            surf->name[j - 2] = 0;
        }

        // register the shaders
        surf->numShaderIndexes = md3Surf->numShaders;
        surf->shaderIndexes = shaderIndex = reinterpret_cast<sint *>(Hunk_Alloc(
                                                sizeof(*shaderIndex) * md3Surf->numShaders, h_low));

        md3Shader = (md3Shader_t *)(reinterpret_cast<uchar8 *>
                                    (md3Surf) + md3Surf->ofsShaders);

        for(j = 0; j < md3Surf->numShaders; j++, shaderIndex++, md3Shader++) {
            shader_t       *sh;

            sh = R_FindShader(md3Shader->name, LIGHTMAP_NONE, true);

            if(sh->defaultShader) {
                *shaderIndex = 0;
            } else {
                *shaderIndex = sh->index;
            }
        }

        // swap all the triangles
        surf->numIndexes = md3Surf->numTriangles * 3;
        surf->indexes = tri = reinterpret_cast<uint *>(Hunk_Alloc(sizeof(
                                  *tri) * 3 * md3Surf->numTriangles, h_low));

        md3Tri = (md3Triangle_t *)(reinterpret_cast<uchar8 *>
                                   (md3Surf) + md3Surf->ofsTriangles);

        for(j = 0; j < md3Surf->numTriangles; j++, tri += 3, md3Tri++) {
            tri[0] = LittleLong(md3Tri->indexes[0]);
            tri[1] = LittleLong(md3Tri->indexes[1]);
            tri[2] = LittleLong(md3Tri->indexes[2]);
        }

        // swap all the XyzNormals
        surf->numVerts = md3Surf->numVerts;
        surf->verts = v = reinterpret_cast<mdvVertex_t *>(Hunk_Alloc(sizeof(*v) *
                          (md3Surf->numVerts * md3Surf->numFrames), h_low));

        md3xyz = (md3XyzNormal_t *)(reinterpret_cast<uchar8 *>
                                    (md3Surf) + md3Surf->ofsXyzNormals);

        for(j = 0; j < md3Surf->numVerts * md3Surf->numFrames;
                j++, md3xyz++, v++) {
            uint lat, lng;
            uchar16 normal;
            vec3_t fNormal;

            v->xyz[0] = LittleShort(md3xyz->xyz[0]) * MD3_XYZ_SCALE;
            v->xyz[1] = LittleShort(md3xyz->xyz[1]) * MD3_XYZ_SCALE;
            v->xyz[2] = LittleShort(md3xyz->xyz[2]) * MD3_XYZ_SCALE;

            normal = LittleShort(md3xyz->normal);

            lat = (normal >> 8) & 0xff;
            lng = (normal & 0xff);
            lat *= (FUNCTABLE_SIZE / 256);
            lng *= (FUNCTABLE_SIZE / 256);

            // decode X as cos( lat ) * sin( long )
            // decode Y as sin( lat ) * sin( long )
            // decode Z as cos( long )

            fNormal[0] = tr.sinTable[(lat + (FUNCTABLE_SIZE / 4))&FUNCTABLE_MASK] *
                         tr.sinTable[lng];
            fNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
            fNormal[2] = tr.sinTable[(lng + (FUNCTABLE_SIZE / 4))&FUNCTABLE_MASK];

            R_VaoPackNormal(v->normal, fNormal);
        }

        // swap all the ST
        surf->st = st = reinterpret_cast<mdvSt_t *>(Hunk_Alloc(sizeof(
                            *st) * md3Surf->numVerts, h_low));

        md3st = (md3St_t *)(reinterpret_cast<uchar8 *>(md3Surf) + md3Surf->ofsSt);

        for(j = 0; j < md3Surf->numVerts; j++, md3st++, st++) {
            st->st[0] = LittleFloat(md3st->st[0]);
            st->st[1] = LittleFloat(md3st->st[1]);
        }

        // calc tangent spaces
        {
            vec3_t *sdirs = (vec3_t *)CL_RefMalloc(sizeof(*sdirs) * surf->numVerts *
                                                   mdvModel->numFrames);
            vec3_t *tdirs = (vec3_t *)CL_RefMalloc(sizeof(*tdirs) * surf->numVerts *
                                                   mdvModel->numFrames);

            for(j = 0, v = surf->verts; j < (surf->numVerts * mdvModel->numFrames);
                    j++, v++) {
                VectorClear(sdirs[j]);
                VectorClear(tdirs[j]);
            }

            for(f = 0; f < mdvModel->numFrames; f++) {
                for(j = 0, tri = surf->indexes; j < surf->numIndexes; j += 3, tri += 3) {
                    vec3_t sdir, tdir;
                    const float32 *v0, *v1, *v2, *t0, *t1, *t2;
                    uint index0, index1, index2;

                    index0 = surf->numVerts * f + tri[0];
                    index1 = surf->numVerts * f + tri[1];
                    index2 = surf->numVerts * f + tri[2];

                    v0 = surf->verts[index0].xyz;
                    v1 = surf->verts[index1].xyz;
                    v2 = surf->verts[index2].xyz;

                    t0 = surf->st[tri[0]].st;
                    t1 = surf->st[tri[1]].st;
                    t2 = surf->st[tri[2]].st;

                    R_CalcTexDirs(sdir, tdir, v0, v1, v2, t0, t1, t2);

                    VectorAdd(sdir, sdirs[index0], sdirs[index0]);
                    VectorAdd(sdir, sdirs[index1], sdirs[index1]);
                    VectorAdd(sdir, sdirs[index2], sdirs[index2]);
                    VectorAdd(tdir, tdirs[index0], tdirs[index0]);
                    VectorAdd(tdir, tdirs[index1], tdirs[index1]);
                    VectorAdd(tdir, tdirs[index2], tdirs[index2]);
                }
            }

            for(j = 0, v = surf->verts; j < (surf->numVerts * mdvModel->numFrames);
                    j++, v++) {
                vec3_t normal;
                vec4_t tangent;

                VectorNormalize(sdirs[j]);
                VectorNormalize(tdirs[j]);

                R_VaoUnpackNormal(normal, v->normal);

                tangent[3] = R_CalcTangentSpace(tangent, nullptr, normal, sdirs[j],
                                                tdirs[j]);

                R_VaoPackTangent(v->tangent, tangent);
            }

            Z_Free(sdirs);
            Z_Free(tdirs);
        }

        // find the next surface
        md3Surf = (md3Surface_t *)(reinterpret_cast<uchar8 *>
                                   (md3Surf) + md3Surf->ofsEnd);
        surf++;
    }

    {
        srfVaoMdvMesh_t *vaoSurf;

        mdvModel->numVaoSurfaces = mdvModel->numSurfaces;
        mdvModel->vaoSurfaces = reinterpret_cast<srfVaoMdvMesh_t *>(Hunk_Alloc(
                                    sizeof(*mdvModel->vaoSurfaces) * mdvModel->numSurfaces, h_low));

        vaoSurf = mdvModel->vaoSurfaces;
        surf = mdvModel->surfaces;

        for(i = 0; i < mdvModel->numSurfaces; i++, vaoSurf++, surf++) {
            uint offset_xyz, offset_st, offset_normal, offset_tangent;
            uint stride_xyz, stride_st, stride_normal, stride_tangent;
            uint dataSize, dataOfs;
            uchar8 *data;

            if(mdvModel->numFrames > 1) {
                // vertex animation, store texcoords first, then position/normal/tangents
                offset_st      = 0;
                offset_xyz     = surf->numVerts * sizeof(vec2_t);
                offset_normal  = offset_xyz + sizeof(vec3_t);
                offset_tangent = offset_normal + sizeof(schar16) * 4;
                stride_st  = sizeof(vec2_t);
                stride_xyz = sizeof(vec3_t) + sizeof(schar16) * 4;
                stride_xyz += sizeof(schar16) * 4;
                stride_normal = stride_tangent = stride_xyz;

                dataSize = offset_xyz + surf->numVerts * mdvModel->numFrames * stride_xyz;
            } else {
                // no animation, interleave everything
                offset_xyz     = 0;
                offset_st      = offset_xyz + sizeof(vec3_t);
                offset_normal  = offset_st + sizeof(vec2_t);
                offset_tangent = offset_normal + sizeof(schar16) * 4;
                stride_xyz = offset_tangent + sizeof(schar16) * 4;
                stride_st = stride_normal = stride_tangent = stride_xyz;

                dataSize = surf->numVerts * stride_xyz;
            }


            data = static_cast<uchar8 *>(CL_RefMalloc(dataSize));
            dataOfs = 0;

            if(mdvModel->numFrames > 1) {
                st = surf->st;

                for(j = 0 ; j < surf->numVerts ; j++, st++) {
                    memcpy(data + dataOfs, &st->st, sizeof(vec2_t));
                    dataOfs += sizeof(st->st);
                }

                v = surf->verts;

                for(j = 0; j < surf->numVerts * mdvModel->numFrames ; j++, v++) {
                    // xyz
                    memcpy(data + dataOfs, &v->xyz, sizeof(vec3_t));
                    dataOfs += sizeof(vec3_t);

                    // normal
                    memcpy(data + dataOfs, &v->normal, sizeof(schar16) * 4);
                    dataOfs += sizeof(schar16) * 4;

                    // tangent
                    memcpy(data + dataOfs, &v->tangent, sizeof(schar16) * 4);
                    dataOfs += sizeof(schar16) * 4;
                }
            } else {
                v = surf->verts;
                st = surf->st;

                for(j = 0; j < surf->numVerts; j++, v++, st++) {
                    // xyz
                    memcpy(data + dataOfs, &v->xyz, sizeof(vec3_t));
                    dataOfs += sizeof(v->xyz);

                    // st
                    memcpy(data + dataOfs, &st->st, sizeof(vec2_t));
                    dataOfs += sizeof(st->st);

                    // normal
                    memcpy(data + dataOfs, &v->normal, sizeof(schar16) * 4);
                    dataOfs += sizeof(schar16) * 4;

                    // tangent
                    memcpy(data + dataOfs, &v->tangent, sizeof(schar16) * 4);
                    dataOfs += sizeof(schar16) * 4;
                }
            }

            vaoSurf->surfaceType = SF_VAO_MDVMESH;
            vaoSurf->mdvModel = mdvModel;
            vaoSurf->mdvSurface = surf;
            vaoSurf->numIndexes = surf->numIndexes;
            vaoSurf->numVerts = surf->numVerts;

            vaoSurf->minIndex = 0;
            vaoSurf->maxIndex = surf->numVerts - 1;

            vaoSurf->vao = R_CreateVao(va("staticMD3Mesh_VAO '%s'", surf->name), data,
                                       dataSize, reinterpret_cast<uchar8 *>(surf->indexes),
                                       surf->numIndexes * sizeof(*surf->indexes), VAO_USAGE_STATIC);

            vaoSurf->vao->attribs[ATTR_INDEX_POSITION].enabled = 1;
            vaoSurf->vao->attribs[ATTR_INDEX_TEXCOORD].enabled = 1;
            vaoSurf->vao->attribs[ATTR_INDEX_NORMAL  ].enabled = 1;
            vaoSurf->vao->attribs[ATTR_INDEX_TANGENT ].enabled = 1;

            vaoSurf->vao->attribs[ATTR_INDEX_POSITION].count = 3;
            vaoSurf->vao->attribs[ATTR_INDEX_TEXCOORD].count = 2;
            vaoSurf->vao->attribs[ATTR_INDEX_NORMAL  ].count = 4;
            vaoSurf->vao->attribs[ATTR_INDEX_TANGENT ].count = 4;

            vaoSurf->vao->attribs[ATTR_INDEX_POSITION].type = GL_FLOAT;
            vaoSurf->vao->attribs[ATTR_INDEX_TEXCOORD].type = GL_FLOAT;
            vaoSurf->vao->attribs[ATTR_INDEX_NORMAL  ].type = GL_SHORT;
            vaoSurf->vao->attribs[ATTR_INDEX_TANGENT ].type = GL_SHORT;

            vaoSurf->vao->attribs[ATTR_INDEX_POSITION].normalized = GL_FALSE;
            vaoSurf->vao->attribs[ATTR_INDEX_TEXCOORD].normalized = GL_FALSE;
            vaoSurf->vao->attribs[ATTR_INDEX_NORMAL  ].normalized = GL_TRUE;
            vaoSurf->vao->attribs[ATTR_INDEX_TANGENT ].normalized = GL_TRUE;

            vaoSurf->vao->attribs[ATTR_INDEX_POSITION].offset = offset_xyz;
            vaoSurf->vao->attribs[ATTR_INDEX_TEXCOORD].offset = offset_st;
            vaoSurf->vao->attribs[ATTR_INDEX_NORMAL  ].offset = offset_normal;
            vaoSurf->vao->attribs[ATTR_INDEX_TANGENT ].offset = offset_tangent;

            vaoSurf->vao->attribs[ATTR_INDEX_POSITION].stride = stride_xyz;
            vaoSurf->vao->attribs[ATTR_INDEX_TEXCOORD].stride = stride_st;
            vaoSurf->vao->attribs[ATTR_INDEX_NORMAL  ].stride = stride_normal;
            vaoSurf->vao->attribs[ATTR_INDEX_TANGENT ].stride = stride_tangent;

            if(mdvModel->numFrames > 1) {
                vaoSurf->vao->attribs[ATTR_INDEX_POSITION2] =
                    vaoSurf->vao->attribs[ATTR_INDEX_POSITION];
                vaoSurf->vao->attribs[ATTR_INDEX_NORMAL2  ] =
                    vaoSurf->vao->attribs[ATTR_INDEX_NORMAL  ];
                vaoSurf->vao->attribs[ATTR_INDEX_TANGENT2 ] =
                    vaoSurf->vao->attribs[ATTR_INDEX_TANGENT ];

                vaoSurf->vao->frameSize = stride_xyz    * surf->numVerts;
            }

            Vao_SetVertexPointers(vaoSurf->vao);

            Z_Free(data);
        }
    }

    return true;
}



/*
=================
R_LoadMDR
=================
*/
static bool R_LoadMDR(model_t *mod, void *buffer, sint filesize,
                      pointer mod_name) {
    sint                    i, j, k, l;
    mdrHeader_t            *pinmodel, *mdr = nullptr;
    mdrFrame_t         *frame;
    mdrLOD_t           *lod, *curlod;
    mdrSurface_t           *surf, *cursurf;
    mdrTriangle_t          *tri, *curtri;
    mdrVertex_t            *v, *curv;
    mdrWeight_t            *weight, *curweight;
    mdrTag_t           *tag, *curtag;
    sint                    size;
    shader_t           *sh;

    pinmodel = (mdrHeader_t *)buffer;

    pinmodel->version = LittleLong(pinmodel->version);

    if(pinmodel->version != MDR_VERSION) {
        CL_RefPrintf(PRINT_WARNING,
                     "R_LoadMDR: %s has wrong version (%i should be %i)\n", mod_name,
                     pinmodel->version, MDR_VERSION);
        return false;
    }

    size = LittleLong(pinmodel->ofsEnd);

    if(size > filesize) {
        CL_RefPrintf(PRINT_WARNING,
                     "R_LoadMDR: Header of %s is broken. Wrong filesize declared!\n", mod_name);
        return false;
    }

    mod->type = MOD_MDR;

    LL(pinmodel->numFrames);
    LL(pinmodel->numBones);
    LL(pinmodel->ofsFrames);

    // This is a model that uses some type of compressed Bones. We don't want to uncompress every bone for each rendered frame
    // over and over again, we'll uncompress it in this function already, so we must adjust the size of the target mdr.
    if(pinmodel->ofsFrames < 0) {
        // mdrFrame_t is larger than mdrCompFrame_t:
        size += pinmodel->numFrames * sizeof(frame->name);
        // now add enough space for the uncompressed bones.
        size += pinmodel->numFrames * pinmodel->numBones * ((sizeof(
                    mdrBone_t) - sizeof(mdrCompBone_t)));
    }

    // simple bounds check
    if(pinmodel->numBones < 0 ||
            sizeof(*mdr) + pinmodel->numFrames * (sizeof(*frame) +
                    (pinmodel->numBones - 1) * sizeof(*frame->bones)) > size) {
        CL_RefPrintf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n",
                     mod_name);
        return false;
    }

    mod->dataSize += size;
    mod->modelData = mdr = reinterpret_cast<mdrHeader_t *>(Hunk_Alloc(size,
                           h_low));

    // Copy all the values over from the file and fix endian issues in the process, if necessary.

    mdr->ident = LittleLong(pinmodel->ident);
    mdr->version =
        pinmodel->version;   // Don't need to swap uchar8 order on this one, we already did above.
    Q_strncpyz(mdr->name, pinmodel->name, sizeof(mdr->name));
    mdr->numFrames = pinmodel->numFrames;
    mdr->numBones = pinmodel->numBones;
    mdr->numLODs = LittleLong(pinmodel->numLODs);
    mdr->numTags = LittleLong(pinmodel->numTags);
    // We don't care about the other offset values, we'll generate them ourselves while loading.

    mod->numLods = mdr->numLODs;

    if(mdr->numFrames < 1) {
        CL_RefPrintf(PRINT_WARNING, "R_LoadMDR: %s has no frames\n", mod_name);
        return false;
    }

    /* The first frame will be put into the first free space after the header */
    frame = (mdrFrame_t *)(mdr + 1);
    mdr->ofsFrames = static_cast<sint>(reinterpret_cast<uchar8 *>
                                       (frame) - reinterpret_cast<uchar8 *>(mdr));

    if(pinmodel->ofsFrames < 0) {
        mdrCompFrame_t *cframe;

        // compressed model...
        cframe = (mdrCompFrame_t *)(reinterpret_cast<uchar8 *>
                                    (pinmodel) - pinmodel->ofsFrames);

        for(i = 0; i < mdr->numFrames; i++) {
            for(j = 0; j < 3; j++) {
                frame->bounds[0][j] = LittleFloat(cframe->bounds[0][j]);
                frame->bounds[1][j] = LittleFloat(cframe->bounds[1][j]);
                frame->localOrigin[j] = LittleFloat(cframe->localOrigin[j]);
            }

            frame->radius = LittleFloat(cframe->radius);
            frame->name[0] = '\0';  // No name supplied in the compressed version.

            for(j = 0; j < mdr->numBones; j++) {
                for(k = 0; k < (sizeof(cframe->bones[j].Comp) / 2); k++) {
                    // Do swapping for the uncompressing functions. They seem to use shorts
                    // values only, so I assume this will work. Never tested it on other
                    // platforms, though.

                    (reinterpret_cast<uchar16 *>(cframe->bones[j].Comp))[k] =
                        LittleShort((reinterpret_cast<uchar16 *>(cframe->bones[j].Comp))[k]);
                }

                /* Now do the actual uncompressing */
                MC_UnCompress(frame->bones[j].matrix, cframe->bones[j].Comp);
            }

            // Next Frame...
            cframe = (mdrCompFrame_t *) &cframe->bones[j];
            frame = (mdrFrame_t *) &frame->bones[j];
        }
    } else {
        mdrFrame_t *curframe;

        // uncompressed model...
        //

        curframe = (mdrFrame_t *)(reinterpret_cast<uchar8 *>(pinmodel) +
                                  pinmodel->ofsFrames);

        // swap all the frames
        for(i = 0 ; i < mdr->numFrames ; i++) {
            for(j = 0; j < 3; j++) {
                frame->bounds[0][j] = LittleFloat(curframe->bounds[0][j]);
                frame->bounds[1][j] = LittleFloat(curframe->bounds[1][j]);
                frame->localOrigin[j] = LittleFloat(curframe->localOrigin[j]);
            }

            frame->radius = LittleFloat(curframe->radius);
            Q_strncpyz(frame->name, curframe->name, sizeof(frame->name));

            for(j = 0; j < static_cast<sint>(mdr->numBones * sizeof(mdrBone_t) / 4);
                    j++) {
                (reinterpret_cast<float32 *>(frame->bones))[j] = LittleFloat((
                            reinterpret_cast<float32 *>(curframe->bones))[j]);
            }

            curframe = (mdrFrame_t *) &curframe->bones[mdr->numBones];
            frame = (mdrFrame_t *) &frame->bones[mdr->numBones];
        }
    }

    // frame should now point to the first free address after all frames.
    lod = (mdrLOD_t *) frame;
    mdr->ofsLODs = static_cast<sint>(reinterpret_cast<uchar8 *>
                                     (lod) - reinterpret_cast<uchar8 *>(mdr));

    curlod = (mdrLOD_t *)(reinterpret_cast<uchar8 *>(pinmodel) + LittleLong(
                              pinmodel->ofsLODs));

    // swap all the LOD's
    for(l = 0 ; l < mdr->numLODs ; l++) {
        // simple bounds check
        if(reinterpret_cast<uchar8 *>((lod) + 1) > reinterpret_cast<uchar8 *>
                (mdr) + size) {
            CL_RefPrintf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n",
                         mod_name);
            return false;
        }

        lod->numSurfaces = LittleLong(curlod->numSurfaces);

        // swap all the surfaces
        surf = (mdrSurface_t *)(lod + 1);
        lod->ofsSurfaces = static_cast<sint>(reinterpret_cast<uchar8 *>
                                             (surf) - reinterpret_cast<uchar8 *>(lod));
        cursurf = (mdrSurface_t *)(reinterpret_cast<uchar8 *>(curlod) + LittleLong(
                                       curlod->ofsSurfaces));

        for(i = 0 ; i < lod->numSurfaces ; i++) {
            // simple bounds check
            if(reinterpret_cast<uchar8 *>((surf) + 1) > reinterpret_cast<uchar8 *>
                    (mdr) + size) {
                CL_RefPrintf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n",
                             mod_name);
                return false;
            }

            // first do some copying stuff

            surf->ident = SF_MDR;
            Q_strncpyz(surf->name, cursurf->name, sizeof(surf->name));
            Q_strncpyz(surf->shader, cursurf->shader, sizeof(surf->shader));

            surf->ofsHeader = reinterpret_cast<uchar8 *>(mdr) -
                              reinterpret_cast<uchar8 *>(surf);

            surf->numVerts = LittleLong(cursurf->numVerts);
            surf->numTriangles = LittleLong(cursurf->numTriangles);
            // numBoneReferences and BoneReferences generally seem to be unused

            // now do the checks that may fail.
            if(surf->numVerts >= SHADER_MAX_VERTEXES) {
                CL_RefPrintf(PRINT_WARNING,
                             "R_LoadMDR: %s has more than %i verts on %s (%i).\n",
                             mod_name, SHADER_MAX_VERTEXES - 1,
                             surf->name[0] ? surf->name : "a surface",
                             surf->numVerts);
                return false;
            }

            if(surf->numTriangles * 3 >= SHADER_MAX_INDEXES) {
                CL_RefPrintf(PRINT_WARNING,
                             "R_LoadMDR: %s has more than %i triangles on %s (%i).\n",
                             mod_name, (SHADER_MAX_INDEXES / 3) - 1,
                             surf->name[0] ? surf->name : "a surface",
                             surf->numTriangles);
                return false;
            }

            // lowercase the surface name so skin compares are faster
            Q_strlwr(surf->name);

            // register the shaders
            sh = R_FindShader(surf->shader, LIGHTMAP_NONE, true);

            if(sh->defaultShader) {
                surf->shaderIndex = 0;
            } else {
                surf->shaderIndex = sh->index;
            }

            // now copy the vertexes.
            v = (mdrVertex_t *)(surf + 1);
            surf->ofsVerts = static_cast<sint>(reinterpret_cast<uchar8 *>
                                               (v) - reinterpret_cast<uchar8 *>(surf));
            curv = (mdrVertex_t *)(reinterpret_cast<uchar8 *>(cursurf) + LittleLong(
                                       cursurf->ofsVerts));

            for(j = 0; j < surf->numVerts; j++) {
                LL(curv->numWeights);

                // simple bounds check
                if(curv->numWeights < 0 ||
                        reinterpret_cast<uchar8 *>(v + 1) + (curv->numWeights - 1) * sizeof(
                            *weight) > reinterpret_cast<uchar8 *>(mdr) + size) {
                    CL_RefPrintf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n",
                                 mod_name);
                    return false;
                }

                v->normal[0] = LittleFloat(curv->normal[0]);
                v->normal[1] = LittleFloat(curv->normal[1]);
                v->normal[2] = LittleFloat(curv->normal[2]);

                v->texCoords[0] = LittleFloat(curv->texCoords[0]);
                v->texCoords[1] = LittleFloat(curv->texCoords[1]);

                v->numWeights = curv->numWeights;
                weight = &v->weights[0];
                curweight = &curv->weights[0];

                // Now copy all the weights
                for(k = 0; k < v->numWeights; k++) {
                    weight->boneIndex = LittleLong(curweight->boneIndex);
                    weight->boneWeight = LittleFloat(curweight->boneWeight);

                    weight->offset[0] = LittleFloat(curweight->offset[0]);
                    weight->offset[1] = LittleFloat(curweight->offset[1]);
                    weight->offset[2] = LittleFloat(curweight->offset[2]);

                    weight++;
                    curweight++;
                }

                v = (mdrVertex_t *) weight;
                curv = (mdrVertex_t *) curweight;
            }

            // we know the offset to the triangles now:
            tri = (mdrTriangle_t *) v;
            surf->ofsTriangles = static_cast<sint>(reinterpret_cast<uchar8 *>
                                                   (tri) - reinterpret_cast<uchar8 *>(surf));
            curtri = (mdrTriangle_t *)(reinterpret_cast<uchar8 *>
                                       (cursurf) + LittleLong(cursurf->ofsTriangles));

            // simple bounds check
            if(surf->numTriangles < 0 ||
                    reinterpret_cast<uchar8 *>(tri) + surf->numTriangles >
                    reinterpret_cast<uchar8 *>(mdr) + size) {
                CL_RefPrintf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n",
                             mod_name);
                return false;
            }

            for(j = 0; j < surf->numTriangles; j++) {
                tri->indexes[0] = LittleLong(curtri->indexes[0]);
                tri->indexes[1] = LittleLong(curtri->indexes[1]);
                tri->indexes[2] = LittleLong(curtri->indexes[2]);

                tri++;
                curtri++;
            }

            // tri now points to the end of the surface.
            surf->ofsEnd = reinterpret_cast<uchar8 *>(tri) -
                           reinterpret_cast<uchar8 *>(surf);
            surf = (mdrSurface_t *) tri;

            // find the next surface.
            cursurf = (mdrSurface_t *)(reinterpret_cast<uchar8 *>
                                       (cursurf) + LittleLong(cursurf->ofsEnd));
        }

        // surf points to the next lod now.
        lod->ofsEnd = static_cast<sint>(reinterpret_cast<uchar8 *>
                                        (surf) - reinterpret_cast<uchar8 *>(lod));
        lod = (mdrLOD_t *) surf;

        // find the next LOD.
        curlod = (mdrLOD_t *)(reinterpret_cast<uchar8 *>(curlod) + LittleLong(
                                  curlod->ofsEnd));
    }

    // lod points to the first tag now, so update the offset too.
    tag = (mdrTag_t *) lod;
    mdr->ofsTags = static_cast<sint>(reinterpret_cast<uchar8 *>
                                     (tag) - reinterpret_cast<uchar8 *>(mdr));
    curtag = (mdrTag_t *)(reinterpret_cast<uchar8 *>(pinmodel) + LittleLong(
                              pinmodel->ofsTags));

    // simple bounds check
    if(mdr->numTags < 0 ||
            reinterpret_cast<uchar8 *>(tag) + mdr->numTags >
            reinterpret_cast<uchar8 *>(mdr) + size) {
        CL_RefPrintf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n",
                     mod_name);
        return false;
    }

    for(i = 0 ; i < mdr->numTags ; i++) {
        tag->boneIndex = LittleLong(curtag->boneIndex);
        Q_strncpyz(tag->name, curtag->name, sizeof(tag->name));

        tag++;
        curtag++;
    }

    // And finally we know the real offset to the end.
    mdr->ofsEnd = static_cast<sint>(reinterpret_cast<uchar8 *>
                                    (tag) - reinterpret_cast<uchar8 *>(mdr));

    // phew! we're done.

    return true;
}



//=============================================================================

/*
** idRenderSystemLocal::Init
*/
void idRenderSystemLocal::Init(vidconfig_t *glconfigOut) {
    sint    i;

    R_Init();

    *glconfigOut = glConfig;

    R_IssuePendingRenderCommands();

    tr.visIndex = 0;

    // force markleafs to regenerate
    for(i = 0; i < MAX_VISCOUNTS; i++) {
        tr.visClusters[i] = -2;
    }

    R_ClearFlares();
    renderSystemLocal.ClearScene();

    tr.registered = true;
}

//=============================================================================

/*
===============
R_ModelInit
===============
*/
void R_ModelInit(void) {
    model_t        *mod;

    // leave a space for nullptr model
    tr.numModels = 0;

    mod = R_AllocModel();
    mod->type = MOD_BAD;
}


/*
================
R_Modellist_f
================
*/
void R_Modellist_f(void) {
    sint        i, j;
    model_t    *mod;
    sint        total;
    sint        lods;

    total = 0;

    for(i = 1 ; i < tr.numModels; i++) {
        mod = tr.models[i];
        lods = 1;

        for(j = 1 ; j < MD3_MAX_LODS ; j++) {
            if(mod->mdv[j] && mod->mdv[j] != mod->mdv[j - 1]) {
                lods++;
            }
        }

        CL_RefPrintf(PRINT_ALL, "%8i : (%i) %s\n", mod->dataSize, lods, mod->name);
        total += mod->dataSize;
    }

    CL_RefPrintf(PRINT_ALL, "%8i : Total models\n", total);

#if 0       // not working right with new hunk

    if(tr.world) {
        CL_RefPrintf(PRINT_ALL, "\n%8i : %s\n", tr.world->dataSize,
                     tr.world->name);
    }

#endif
}


//=============================================================================


/*
================
R_GetTag
================
*/
static mdvTag_t *R_GetTag(mdvModel_t *mod, sint frame, pointer _tagName) {
    sint             i;
    mdvTag_t       *tag;
    mdvTagName_t   *tagName;

    if(frame >= mod->numFrames) {
        // it is possible to have a bad frame while changing models, so don't error
        frame = mod->numFrames - 1;
    }

    tag = mod->tags + frame * mod->numTags;
    tagName = mod->tagNames;

    for(i = 0; i < mod->numTags; i++, tag++, tagName++) {
        if(!strcmp(tagName->name, _tagName)) {
            return tag;
        }
    }

    return nullptr;
}

mdvTag_t *R_GetAnimTag(mdrHeader_t *mod, sint framenum, pointer tagName,
                       mdvTag_t *dest) {
    sint                i, j, k;
    uint64 frameSize;
    mdrFrame_t     *frame;
    mdrTag_t       *tag;

    if(framenum >= mod->numFrames) {
        // it is possible to have a bad frame while changing models, so don't error
        framenum = mod->numFrames - 1;
    }

    tag = (mdrTag_t *)(reinterpret_cast<uchar8 *>(mod) + mod->ofsTags);

    for(i = 0 ; i < mod->numTags ; i++, tag++) {
        if(!strcmp(tag->name, tagName)) {
            // uncompressed model...
            //
            frameSize = reinterpret_cast<sint64>((&((mdrFrame_t *)
                                                    0)->bones[ mod->numBones ]));
            frame = (mdrFrame_t *)(reinterpret_cast<uchar8 *>(mod) + mod->ofsFrames +
                                   framenum * frameSize);

            for(j = 0; j < 3; j++) {
                for(k = 0; k < 3; k++) {
                    dest->axis[j][k] = frame->bones[tag->boneIndex].matrix[k][j];
                }
            }

            dest->origin[0] = frame->bones[tag->boneIndex].matrix[0][3];
            dest->origin[1] = frame->bones[tag->boneIndex].matrix[1][3];
            dest->origin[2] = frame->bones[tag->boneIndex].matrix[2][3];

            return dest;
        }
    }

    return nullptr;
}

/*
================
idRenderSystemLocal::LerpTag
================
*/
sint idRenderSystemLocal::LerpTag(orientation_t *tag, qhandle_t handle,
                                  sint startFrame, sint endFrame, float32 frac, pointer tagName) {
    mdvTag_t   *start, *end;
    mdvTag_t    start_space, end_space;
    sint        i;
    float32     frontLerp, backLerp;
    model_t        *model;

    model = R_GetModelByHandle(handle);

    if(!model->mdv[0]) {
        if(model->type == MOD_MDR) {
            start = R_GetAnimTag((mdrHeader_t *) model->modelData, startFrame, tagName,
                                 &start_space);
            end = R_GetAnimTag((mdrHeader_t *) model->modelData, endFrame, tagName,
                               &end_space);
        } else if(model->type == MOD_IQM) {
            return R_IQMLerpTag(tag, (iqmData_t *)model->modelData, startFrame,
                                endFrame, frac, tagName);
        } else {
            start = end = nullptr;
        }
    } else {
        start = R_GetTag(model->mdv[0], startFrame, tagName);
        end = R_GetTag(model->mdv[0], endFrame, tagName);
    }

    if(!start || !end) {
        AxisClear(tag->axis);
        VectorClear(tag->origin);
        return false;
    }

    frontLerp = frac;
    backLerp = 1.0f - frac;

    for(i = 0 ; i < 3 ; i++) {
        tag->origin[i] = start->origin[i] * backLerp +  end->origin[i] * frontLerp;
        tag->axis[0][i] = start->axis[0][i] * backLerp +  end->axis[0][i] *
                          frontLerp;
        tag->axis[1][i] = start->axis[1][i] * backLerp +  end->axis[1][i] *
                          frontLerp;
        tag->axis[2][i] = start->axis[2][i] * backLerp +  end->axis[2][i] *
                          frontLerp;
    }

    VectorNormalize(tag->axis[0]);
    VectorNormalize(tag->axis[1]);
    VectorNormalize(tag->axis[2]);
    return true;
}


/*
====================
idRenderSystemLocal::ModelBounds
====================
*/
void idRenderSystemLocal::ModelBounds(qhandle_t handle, vec3_t mins,
                                      vec3_t maxs) {
    model_t        *model;

    model = R_GetModelByHandle(handle);

    if(model->type == MOD_BRUSH) {
        VectorCopy(model->bmodel->bounds[0], mins);
        VectorCopy(model->bmodel->bounds[1], maxs);

        return;
    } else if(model->type == MOD_MESH) {
        mdvModel_t *header;
        mdvFrame_t *frame;

        header = model->mdv[0];
        frame = header->frames;

        VectorCopy(frame->bounds[0], mins);
        VectorCopy(frame->bounds[1], maxs);

        return;
    } else if(model->type == MOD_MDR) {
        mdrHeader_t    *header;
        mdrFrame_t *frame;

        header = (mdrHeader_t *)model->modelData;
        frame = (mdrFrame_t *)(reinterpret_cast<uchar8 *>(header) +
                               header->ofsFrames);

        VectorCopy(frame->bounds[0], mins);
        VectorCopy(frame->bounds[1], maxs);

        return;
    } else if(model->type == MOD_IQM) {
        iqmData_t *iqmData;

        iqmData = (iqmData_t *)model->modelData;

        if(iqmData->bounds) {
            VectorCopy(iqmData->bounds, mins);
            VectorCopy(iqmData->bounds + 3, maxs);
            return;
        }
    }

    VectorClear(mins);
    VectorClear(maxs);
}
