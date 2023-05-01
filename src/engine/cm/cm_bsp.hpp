////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2021 - 2023 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   cm_bsp.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CM_BSP_HPP__
#define __CM_BSP_HPP__

/*
==============================================================================

  .BSP file format

==============================================================================
*/

#define BSP_IDENT (('P' << 24) + ('S' << 16) + ('B' << 8) + 'I') // little-endian "OBSP"
#define Q3_BSP_VERSION          46 // Quake III / Team Arena
#define WOLF_BSP_VERSION        47 // RTCW / WolfET

// there shouldn't be any problem with increasing these values at the
// expense of more memory allocation in the utilities
#ifndef Q3MAP2
#define MAX_MAP_MODELS      0x800
#define MAX_MAP_BRUSHES     0x8000
#define MAX_MAP_ENTITIES    0x1000
#define MAX_MAP_ENTSTRING   0x40000
#define MAX_MAP_SHADERS     0x400

#define MAX_MAP_AREAS       0x100   // MAX_MAP_AREA_BYTES in q_shared must match!
#define MAX_MAP_FOGS        0x100
#define MAX_MAP_PLANES      0x40000
#define MAX_MAP_NODES       0x20000
#define MAX_MAP_BRUSHSIDES  0x100000
#define MAX_MAP_LEAFS       0x20000
#define MAX_MAP_LEAFFACES   0x20000
#define MAX_MAP_LEAFBRUSHES 0x40000
#define MAX_MAP_PORTALS     0x20000
#define MAX_MAP_LIGHTING    0x800000
#define MAX_MAP_LIGHTGRID   0x800000
#define MAX_MAP_VISIBILITY  0x200000
#define MAX_MAP_DRAW_SURFS  0x20000
#define MAX_MAP_DRAW_VERTS  0x80000
#define MAX_MAP_DRAW_INDEXES    0x80000
#endif

// key / value pair sizes in the entities lump
#define MAX_KEY             32
#define MAX_VALUE           1024

// the editor uses these predefined yaw angles to orient entities up or down
#define ANGLE_UP            -1
#define ANGLE_DOWN          -2

#define LIGHTMAP_WIDTH      128
#define LIGHTMAP_HEIGHT     128

#define MAX_WORLD_COORD     ( 128 * 1024 )
#define MIN_WORLD_COORD     ( -128 * 1024 )
#define WORLD_SIZE          ( MAX_WORLD_COORD - MIN_WORLD_COORD )

//=============================================================================


typedef struct {
    sint             fileofs, filelen;
} lump_t;

#define LUMP_ENTITIES       0
#define LUMP_SHADERS        1
#define LUMP_PLANES         2
#define LUMP_NODES          3
#define LUMP_LEAFS          4
#define LUMP_LEAFSURFACES   5
#define LUMP_LEAFBRUSHES    6
#define LUMP_MODELS         7
#define LUMP_BRUSHES        8
#define LUMP_BRUSHSIDES     9
#define LUMP_DRAWVERTS      10
#define LUMP_DRAWINDEXES    11
#define LUMP_FOGS           12
#define LUMP_SURFACES       13
#define LUMP_LIGHTMAPS      14
#define LUMP_LIGHTGRID      15
#define LUMP_VISIBILITY     16
#define HEADER_LUMPS        17

typedef struct {
    sint             ident;
    sint             version;

    lump_t          lumps[HEADER_LUMPS];
} dheader_t;

typedef struct {
    float32           mins[3], maxs[3];
    sint             firstSurface, numSurfaces;
    sint             firstBrush, numBrushes;
} dmodel_t;

// planes x^1 is allways the opposite of plane x

typedef struct {
    float32           normal[3];
    float32           dist;
} dplane_t;

typedef struct {
    sint             planeNum;
    sint
    children[2];   // negative numbers are -(leafs+1), not nodes
    sint             mins[3];   // for frustom culling
    sint             maxs[3];
} dnode_t;

typedef struct {
    sint
    cluster;   // -1 = opaque cluster (do I still store these?)
    sint             area;

    sint             mins[3];   // for frustum culling
    sint             maxs[3];

    sint             firstLeafSurface;
    sint             numLeafSurfaces;

    sint             firstLeafBrush;
    sint             numLeafBrushes;
} dleaf_t;

typedef struct {
    sint             planeNum;  // positive plane side faces out of the leaf
    sint             shaderNum;
} dbrushside_t;

typedef struct {
    valueType            shader[64];
    sint             surfaceFlags;
    sint             contentFlags;
} dshader_t;

typedef struct {
    sint             firstSide;
    sint             numSides;
    sint
    shaderNum; // the shader that determines the contents flags
} dbrush_t;

typedef struct {
    valueType            shader[64];
    sint             brushNum;
    sint
    visibleSide;   // the brush side that ray tests need to clip against (-1 == none)
} dfog_t;

// light grid
typedef struct {
    uchar8            ambient[3];
    uchar8            directed[3];
    uchar8            latLong[2];
} dgridPoint_t;

typedef struct {
    vec3_t xyz;
    float32 st[2];
    float32 lightmap[2];
    vec3_t normal;
    uchar8 color[4];
} drawVert_t;

// Light Style Constants
#define MAXLIGHTMAPS 2
#define LS_NORMAL       0x00
#define LS_UNUSED       0xfe
#define LS_LSNONE       0xff
#define MAX_LIGHT_STYLES        64

#define drawVert_t_cleared(x) drawVert_t (x) = {{0, 0, 0}, {0, 0}, {0, 0}, {0, 0, 0}, {0, 0, 0, 0}}

enum mapSurfaceType_t {
    MST_BAD,
    MST_PLANAR,
    MST_PATCH,
    MST_TRIANGLE_SOUP,
    MST_FLARE,
    MST_FOLIAGE
};

typedef struct {
    sint             shaderNum;
    sint             fogNum;
    sint             surfaceType;

    sint             firstVert;
    sint
    numVerts;  // ydnar: num verts + foliage origins (for cleaner lighting code in q3map)

    sint             firstIndex;
    sint             numIndexes;

    sint             lightmapNum;
    sint             lightmapX, lightmapY;
    sint             lightmapWidth, lightmapHeight;

    vec3_t          lightmapOrigin;
    vec3_t
    lightmapVecs[3];    // for patches, [0] and [1] are lodbounds

    sint             patchWidth;    // ydnar: num foliage instances
    sint             patchHeight;   // ydnar: num foliage mesh verts
} dsurface_t;

//----(SA) added so I didn't change the dsurface_t struct (and thereby the bsp format) for something that doesn't need to be stored in the bsp
typedef struct {
    valueType *lighttarg;
} drsurfaceInternal_t;

//----(SA) end

#endif //!__CM_BSP_HPP__