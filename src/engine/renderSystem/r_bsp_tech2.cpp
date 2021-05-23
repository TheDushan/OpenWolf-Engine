////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2005 Id Software, Inc.
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
// File name:   r_bsp_tech2.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: Loads and prepares a map file for scene rendering.
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.hpp>

idRenderSystemBSPTech2Local renderSystemBSPTechLocal;

/*
===============
idRenderSystemBSPTech2Local::idRenderSystemBSPTech2Local
===============
*/
idRenderSystemBSPTech2Local::idRenderSystemBSPTech2Local(void) {
}

/*
===============
idRenderSystemBSPTech2Local::~idRenderSystemBSPTech2Local
===============
*/
idRenderSystemBSPTech2Local::~idRenderSystemBSPTech2Local(void) {
}

model_t *tech2_loadModel;
static  world_t s_worldData;
static  uchar8 *fileBase;
/*
=================
idRenderSystemBSPTech2Local::Tech2_LoadLighting

Converts the 24 bit lighting down to 8 bit
by taking the brightest component
=================
*/
static void idRenderSystemBSPTech2Local::Tech2_LoadLighting(
    const lump_t *l) {
    sint     i, size;
    byte *in;

    if(!l->filelen) {
        idtech2_loadModel->lightdata = nullptr;
        return;
    }

    size = l->filelen / 3;
    idtech2_loadModel->lightdata = (byte *)Hunk_Alloc(l->filelen, h_low);
    in = (byte *)(fileBase + l->fileofs);

    for(i = 0; i < size; i++, in += 3) {
        if(in[0] > in[1] && in[0] > in[2]) {
            idtech2_loadModel->lightdata[i] = in[0];
        } else if(in[1] > in[0] && in[1] > in[2]) {
            idtech2_loadModel->lightdata[i] = in[1];
        } else {
            idtech2_loadModel->lightdata[i] = in[2];
        }
    }
}

sint     r_idtech2_leaftovis[IDTECH2_MAX_MAP_LEAFS];
sint     r_idtech2_vistoleaf[IDTECH2_MAX_MAP_LEAFS];
sint     r_idtech2_numvisleafs;

void idRenderSystemBSPTech2Local::NumberLeafs(mNode_t *node) {
    idtech2_mLeaf_t *leaf;
    sint         leafnum;

    if(node->contents != -1) {
        leaf = (idtech2_mLeaf_t *)node;
        leafnum = leaf - idtech2_loadModel->leafs;

        if(leaf->contents & CONTENTS_SOLID) {
            return;
        }

        r_idtech2_leaftovis[leafnum] = r_idtech2_numvisleafs;
        r_idtech2_vistoleaf[r_idtech2_numvisleafs] = leafnum;
        r_idtech2_numvisleafs++;
        return;
    }

    NumberLeafs(node->children[0]);
    NumberLeafs(node->children[1]);
}


/*
=================
idRenderSystemBSPTech2Local::Tech2_LoadVisibility
=================
*/
static void idRenderSystemBSPTech2Local::Tech2_LoadVisibility(
    const lump_t *l) {
    sint     i;

    if(!l->filelen) {
        idtech2_loadModel->vis = NULL;
        return;
    }

    idtech2_loadModel->vis = static_Cast<idtech2_dVis_t *>(Hunk_Alloc(
                                 l->filelen, h_low));
    memcpy(idtech2_loadModel->vis, fileBase + l->fileofs, l->filelen);

    idtech2_loadModel->vis->numclusters = LittleLong(
            idtech2_loadModel->vis->numclusters);

    for(i = 0; i < idtech2_loadModel->vis->numclusters; i++) {
        idtech2_loadModel->vis->bitofs[i][0] = LittleLong(
                idtech2_loadModel->vis->bitofs[i][0]);
        idtech2_loadModel->vis->bitofs[i][1] = LittleLong(
                idtech2_loadModel->vis->bitofs[i][1]);
    }
}


/*
=================
idRenderSystemBSPTech2Local::Tech2_LoadVertexes
=================
*/
static void idRenderSystemBSPTech2Local::Tech2_LoadVertexes(
    const lump_t *l) {
    idtech2_dVertex_t *in;
    idtech2_mVertex_t *out;
    sint         i, count;

    in = static_cast<idtech2_dVertex_t *>(fileBase + l->fileofs);

    if(l->filelen % sizeof(*in)) {
        Com_Error(ERR_DROP, "%s: funny lump size in %s", __func__,
                  s_worldData.name);
    }

    count = l->filelen / sizeof(*in);
    out = static_cast<idtech2_mVertex_t *>(Hunk_Alloc((count + 8) * sizeof(
            *out),
                                           h_low));    // extra for skybox
    idtech2_loadModel->vertexes = out;
    idtech2_loadModel->numvertexes = count;

    for(i = 0; i < count; i++, in++, out++) {
        out->position[0] = LittleFloat(in->posint[0]);
        out->position[1] = LittleFloat(in->posint[1]);
        out->position[2] = LittleFloat(in->posint[2]);
    }

    Com_Prsintf("loaded %i vertices\n", count);
}

/*
=================
idRenderSystemBSPTech2Local::Tech2_LoadSubmodels
=================
*/
static void idRenderSystemBSPTech2Local::Tech2_LoadSubmodels(
    const lump_t *l) {
    idtech2_dModel_t *in;
    idtech2_mModel_t *out;
    sint         i, j, count;
    sint         lastface;

    in = static_cast<idtech2_dModel_t *>(fileBase + l->fileofs);

    if(l->filelen % sizeof(*in)) {
        Com_Error(ERR_DROP, "%s: funny lump size in %s", __func__,
                  s_worldData.name);
    }

    count = l->filelen / sizeof(*in);
    out = static_cast<idtech2_mModel_t *>(Hunk_Alloc(count * sizeof(*out),
                                          h_low));

    idtech2_loadModel->submodels = out;
    idtech2_loadModel->numsubmodels = count;

    for(i = 0; i < count; i++, in++, out++) {
        for(j = 0; j < 3; j++) {
            // spread the mins / maxs by a pixel
            out->mins[j] = LittleFloat(in->mins[j]) - 1;
            out->maxs[j] = LittleFloat(in->maxs[j]) + 1;
            out->origin[j] = LittleFloat(in->origin[j]);
        }

        out->headnode = LittleLong(in->headnode);
        out->firstface = LittleLong(in->firstface);
        out->numfaces = LittleLong(in->numfaces);
        lastface = out->firstface + out->numfaces;

        if(lastface < out->firstface) {
            Com_Error(ERR_DROP, "%s: bad facenum", __func__);
        }
    }
}

/*
=================
idRenderSystemBSPTech2Local::Tech2_LoadEdges
=================
*/
static void idRenderSystemBSPTech2Local::Tech2_LoadEdges(const lump_t *l) {
    idtech2_dEdge_t *in;
    idtech2_mEdge_t *out;
    ssint        i, count;

    in = static_cast<idtech2_dEdge_t *>(fileBase + l->fileofs);

    if(l->filelen % sizeof(*in)) {
        Com_Error(ERR_DROP, "%s: funny lump size in %s", __func__,
                  s_worldData.name);
    }

    count = l->filelen / sizeof(*in);
    out = static_cast<idtech2_mEdge_t *>Hunk_Alloc((count + 13) * sizeof(*out),
            h_low);
    idtech2_loadModel->edges = out;
    idtech2_loadModel->numedges = count;

    for(i = 0; i < count; i++, in++, out++) {
        out->v[0] = (unsigned short)LittleShort(in->v[0]);
        out->v[1] = (unsigned short)LittleShort(in->v[1]);

        if(out->v[0] >= idtech2_loadModel->numvertexes ||
                out->v[1] >= idtech2_loadModel->numvertexes) {
            Com_Error(ERR_DROP, "%s: bad vertexnum", __func__);
        }
    }
}

/*
 * =======================
 * GL_EndBuildingLightmaps
 * =======================
 */
void GL_EndBuildingLightmaps(void) {

}


/*
=================
idRenderSystemBSPTech2Local::Tech2_LoadFaces
=================
*/
static void idRenderSystemBSPTech2Local::Tech2_LoadFaces(const lump_t *l) {
    idtech2_dFace_t *in;
    msurface_t *out;
    ssint            i, count, surfnum;
    ssint            planenum, side;
    ssint            ti;
    ssint            lastedge;

    in = (idtech2_dFace_t *)(fileBase + l->fileofs);

    if(l->filelen % sizeof(*in)) {
        Com_Error(ERR_DROP, "%s: funny lump size in %s", __func__,
                  s_worldData.name);
    }

    count = l->filelen / sizeof(*in);
    out = (msurface_t *)Hunk_Alloc(count * sizeof(*out), h_low);

    s_worldData.surfaces = out;
    s_worldData.numsurfaces = count;

    tr.currentModel = idtech2_loadModel;

    GL_BeginBuildingLightmaps(idtech2_loadModel);

    for(surfnum = 0; surfnum < count; surfnum++, in++, out++) {
        out->firstedge = LittleLong(in->firstedge);
        out->numedges = LittleShort(in->numedges);
        out->flags = 0;
        out->polys = NULL;
        lastedge = out->firstedge + out->numedges;

        if(out->numedges < 3 ||
                lastedge < out->firstedge) {
            Com_Error(ERR_DROP, "%s: bad edgenums", __func__);
        }

        planenum = LittleShort(in->planenum);

        if(planenum > s_worldData.numplanes) {
            Com_Error(ERR_DROP, "%s: bad planenum", __func__);
        }

        side = LittleShort(in->side);

        if(side) {
            out->flags |= SURF_PLANEBACK;
        }

        out->plane = s_worldData.planes + planenum;

        ti = LittleShort(in->texinfo);

        if(ti < 0 || ti >= idtech2_loadModel->numtexinfo) {
            Com_Error(ERR_DROP, "%s: bad texinfo number", __func__);
        }

        out->texinfo = idtech2_loadModel->texinfo + ti;

        CalcSurfaceExtents(out);

        // lighting info

        for(i = 0; i < MAXLIGHTMAPS; i++) {
            out->styles[i] = in->styles[i];
        }

        i = LittleLong(in->lightofs);

        if(i == -1) {
            out->samples = NULL;
        } else {
            out->samples = idtech2_loadModel->lightdata + i;
        }

        // set the drawing flags

        if(out->texinfo->flags & SURF_IDTECH2_WARP) {
            out->flags |= SURF_DRAWTURB;

            for(i = 0; i < 2; i++) {
                out->extents[i] = 16384;
                out->texturemins[i] = -8192;
            }

            GL_SubdivideSurface(out);   // cut up polygon for warps
        }

        // create lightmaps and polygons
        if(!(out->texinfo->flags & (SURF_SKY | SURF_IDTECH2_TRANS33 |
                                    SURF_IDTECH2_TRANS66 |
                                    SURF_IDTECH2_WARP))) {
            GL_CreateSurfaceLightmap(out);
        }

        if(!(out->texinfo->flags & SURF_WARP)) {
            GL_BuildPolygonFromSurface(out);
        }

    }

    GL_EndBuildingLightmaps();
}

static  void R_SetParent(mNode_t *node, mNode_t *parent) {
    node->parent = parent;

    if(node->contents != -1) {
        return;
    }

    R_SetParent(node->children[0], node);
    R_SetParent(node->children[1], node);
}

/*
=================
idRenderSystemBSPTech2Local::Tech2_LoadNodes
=================
*/
static void idRenderSystemBSPTech2Local::Tech2_LoadNodes(
    const lump_t *nodeLump) {
    sint         i, j, count, p;
    idtech2_dNode_t *in;
    mNode_t *out;

    in = (idtech2_dNode_t *)(fileBase + nodeLump->fileofs);

    if(nodeLump->filelen % sizeof(*in)) {
        Com_Error(ERR_DROP, "%s: funny lump size in %s", __func__,
                  s_worldData.name);
    }

    count = nodeLump->filelen / sizeof(*in);
    out = (mNode_t *)Hunk_Alloc(count * sizeof(*out), h_low);

    s_worldData.nodes = out;
    s_worldData.numnodes = count;

    for(i = 0; i < count; i++, in++, out++) {
        for(j = 0; j < 3; j++) {
            out->minmaxs[j] = LittleShort(in->mins[j]);
            out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
        }

        p = LittleLong(in->planeNum);
        out->plane = s_worldData.planes + p;

        out->firstsurface = LittleShort(in->firstface);
        out->numsurfaces = LittleShort(in->numfaces);
        out->contents = CONTENTS_NODE;

        for(j = 0; j < 2; j++) {
            p = LittleLong(in->children[j]);

            if(p >= 0) {
                out->children[j] = s_worldData.nodes + p;
            } else {
                out->children[j] = (mNode_t *)(idtech2_loadModel->leafs + (-1 - p));
            }
        }
    }

    R_SetParent(s_worldData.nodes, NULL);   // sets nodes and leafs
}

/*
=================
idRenderSystemBSPTech2Local::Tech2_LoadLeafs
=================
*/
static void idRenderSystemBSPTech2Local::Tech2_LoadLeafs(
    const lump_t *leafLump) {
    idtech2_dLeaf_t *in;
    idtech2_mLeaf_t *out;
    sint         i, j, count, p;
    sint         lastmarksurface;

    in = (idtech2_dLeaf_t *)(fileBase + leafLump->fileofs);

    if(leafLump->filelen % sizeof(*in)) {
        Com_Error(ERR_DROP, "%s: funny lump size in %s", __func__,
                  s_worldData.name);
    }

    count = leafLump->filelen / sizeof(*in);
    out = (idtech2_mLeaf_t *)Hunk_Alloc(count * sizeof(*out), h_low);

    idtech2_loadModel->leafs = out;
    idtech2_loadModel->numleafs = count;

    for(i = 0; i < count; i++, in++, out++) {
        for(j = 0; j < 3; j++) {
            out->minmaxs[j] = LittleShort(in->mins[j]);
            out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
        }

        p = LittleLong(in->contents);
        out->contents = p;

        out->cluster = LittleShort(in->cluster);
        out->area = LittleShort(in->area);

        if(idtech2_loadModel->vis != NULL) {
            if(out->cluster < -1 ||
                    out->cluster >= idtech2_loadModel->vis->numclusters) {
                Com_Error(ERR_DROP, "%s: bad cluster", __func__);
            }
        }

        out->firstmarksurface = (msurface_t **)s_worldData.marksurfaces +
                                (unsigned short)LittleShort(
                                    in->firstLeafSurface);
        out->nummarksurfaces = LittleShort(in->numLeafSurfaces);
        lastmarksurface = (unsigned short)LittleShort(in->firstLeafSurface) +
                          out->nummarksurfaces;

        if(lastmarksurface > s_worldData.nummarksurfaces) {
            Com_Error(ERR_DROP, "%s: bad leaf face index", __func__);
        }
    }
}

/*
=================
idRenderSystemBSPTech2Local::Tech2_LoadMarkSurfaces
=================
*/
static void idRenderSystemBSPTech2Local::Tech2_LoadMarkSurfaces(
    const lump_t *l) {
    sint         i, j, count;
    short *in;
    msurface_t **out;

    in = (short *)(fileBase + l->fileofs);

    if(l->filelen % sizeof(*in)) {
        Com_Error(ERR_DROP, "%s: funny lump size in %s", __func__,
                  s_worldData.name);
    }

    count = l->filelen / sizeof(*in);
    out = (msurface_t **)Hunk_Alloc(count * sizeof(*out), h_low);

    s_worldData.marksurfaces = (sint *)out;
    s_worldData.nummarksurfaces = count;

    for(i = 0; i < count; i++) {
        j = LittleShort(in[i]);

        if(j < 0 || j >= s_worldData.numsurfaces) {
            Com_Error(ERR_DROP, "%s: bad surface number", __func__);
        }

        out[i] = s_worldData.surfaces + j;
    }
}


/*
=================
idRenderSystemBSPTech2Local::Tech2_LoadSurfEdges
=================
*/
static void idRenderSystemBSPTech2Local::Tech2_LoadSurfEdges(
    const lump_t *l) {
    sint     i, count;
    sint *in, * out;

    in = (sint *)(fileBase + l->fileofs);

    if(l->filelen % sizeof(*in)) {
        Com_Error(ERR_DROP, "%s: funny lump size in %s", __func__,
                  s_worldData.name);
    }

    count = l->filelen / sizeof(*in);

    if(count < 1 || count >= IDTECH2_MAX_MAP_SURFEDGES) {
        Com_Error(ERR_DROP, "%s: bad surfedges count in %s: %i", __func__,
                  s_worldData.name, count);
    }

    out = (sint *)Hunk_Alloc(count * sizeof(*out), h_low);

    idtech2_loadModel->surfedges = out;
    idtech2_loadModel->numsurfedges = count;

    for(i = 0; i < count; i++) {
        out[i] = LittleLong(in[i]);

        if(out[i] >= idtech2_loadModel->numedges) {
            Com_Error(ERR_DROP, "%s: bad edge index", __func__);
        }
    }
}


/*
=================
idRenderSystemBSPTech2Local::Tech2_LoadPlanes
=================
*/
static void idRenderSystemBSPTech2Local::Tech2_LoadPlanes(
    const lump_t *l) {
    sint            i, j;
    cPlane_t *out;
    idtech2_dPlane_t *in;
    sint            count;
    sint            bits;

    in = (idtech2_dPlane_t *)(fileBase + l->fileofs);

    if(l->filelen % sizeof(*in)) {
        Com_Error(ERR_DROP, "%s: funny lump size in %s", __func__,
                  s_worldData.name);
    }

    count = l->filelen / sizeof(*in);
    out = (cPlane_t *)Hunk_Alloc(count * 2 * sizeof(*out), h_low);

    s_worldData.planes = out;
    s_worldData.numplanes = count;

    for(i = 0; i < count; i++, in++, out++) {
        bits = 0;

        for(j = 0; j < 3; j++) {
            out->normal[j] = LittleFloat(in->normal[j]);

            if(out->normal[j] < 0) {
                bits |= 1 << j;
            }
        }

        out->dist = LittleFloat(in->dist);
        out->type = LittleLong(in->type);
        out->signbits = bits;
    }
}

/*
=================
idRenderSystemBSPTech2Local::Tech2_LoadTexInfo
=================
*/
static void idRenderSystemBSPTech2Local::Tech2_LoadTexInfo(
    const lump_t *l) {
    idtech2_texInfo_t *in;
    idtech2_mTexinfo_t *out, * step;
    sint             i, j, count;
    valueType            name[MAX_QPATH];
    sint             next;

    in = (idtech2_texInfo_t *)(fileBase + l->fileofs);

    if(l->filelen % sizeof(*in)) {
        Com_Error(ERR_DROP, "%s: funny lump size in %s", __func__,
                  s_worldData.name);
    }

    count = l->filelen / sizeof(*in);
    out = (idtech2_mTexinfo_t *)Hunk_Alloc(count * sizeof(*out), h_low);

    idtech2_loadModel->texinfo = out;
    idtech2_loadModel->numtexinfo = count;

    for(i = 0; i < count; i++, in++, out++) {
        for(j = 0; j < 8; j++) {
            out->vecs[0][j] = LittleFloat(in->vecs[0][j]);
        }

        out->flags = LittleLong(in->flags);
        next = LittleLong(in->nexttexinfo);

        if(next > 0) {
            if(next >= count) {
                Com_Error(ERR_DROP, "%s: bad anim chain", __func__);
            }

            out->next = idtech2_loadModel->texinfo + next;
        } else {
            out->next = NULL;
        }

        Q_vsprintf_s(name, sizeof(name), sizeof(name), "textures/%s.wal",
                     in->texture);
    }

    // count animation frames
    for(i = 0; i < count; i++) {
        out = &idtech2_loadModel->texinfo[i];
        out->numframes = 1;

        for(step = out->next; step && step != out; step = step->next) {
            out->numframes++;
        }
    }
}