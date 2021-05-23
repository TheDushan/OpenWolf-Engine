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
// File name:   r_common.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_COMMON_H__
#define __R_COMMON_H__

#if !defined ( DEDICATED ) && !defined ( UPDATE_SERVER )

enum imgType_t {
    IMGTYPE_COLORALPHA, // for color, lightmap, diffuse, and specular
    IMGTYPE_NORMAL,
    IMGTYPE_NORMALHEIGHT,
    IMGTYPE_DELUXE, // normals are swizzled, deluxe are not
};

enum imgFlags_t {
    IMGFLAG_NONE           = 0x0000,
    IMGFLAG_MIPMAP         = 0x0001,
    IMGFLAG_PICMIP         = 0x0002,
    IMGFLAG_CUBEMAP        = 0x0004,
    IMGFLAG_NO_COMPRESSION = 0x0010,
    IMGFLAG_NOLIGHTSCALE   = 0x0020,
    IMGFLAG_CLAMPTOEDGE    = 0x0040,
    IMGFLAG_GENNORMALMAP   = 0x0100,
    IMGFLAG_MUTABLE        = 0x0200,
    IMGFLAG_SRGB           = 0x0400,
};

#define MIP_RAW_IMAGE ( IMGFLAG_MIPMAP | IMGFLAG_PICMIP )

typedef struct image_s {
    valueType       imgName[MAX_QPATH];     // game path, including extension
    sint            width, height;              // source image
    sint            uploadWidth,
                    uploadHeight;  // after power of two and picmip but not including clamp to MAX_TEXTURE_SIZE
    uint            texnum;                 // gl texture binding

    sint
    frameUsed;          // for texture usage in frame statistics

    sint            internalFormat;
    sint            TMU;                // only needed for voodoo2

    imgType_t   type;
    sint /*imgFlags_t*/  flags;

    struct image_s *next;
} image_t;

// any change in the LIGHTMAP_* defines here MUST be reflected in
// R_FindShader() in tr_bsp.c
#define LIGHTMAP_2D         -4  // shader is for 2D rendering
#define LIGHTMAP_BY_VERTEX  -3  // pre-lit triangle models
#define LIGHTMAP_WHITEIMAGE -2
#define LIGHTMAP_NONE       -1

extern vidconfig_t
glConfig; // outside of TR since it shouldn't be cleared during ref re-init

bool    R_GetModeInfo(sint *width, sint *height, float32 *windowAspect,
                      sint mode);

float32 R_NoiseGet4f(float32 x, float32 y, float32 z, float64 t);
void  R_NoiseInit(void);

image_t *R_FindImageFile(pointer name, imgType_t type,
                         sint/*imgFlags_t*/ flags);
image_t *R_CreateImage(pointer name, uchar8 *pic, sint width, sint height,
                       imgType_t type, sint flags, sint internalFormat);

void R_IssuePendingRenderCommands(void);
qhandle_t        RE_RegisterShaderLightMap(pointer name,
        sint lightmapIndex);
qhandle_t RE_RegisterShaderFromImage(pointer name, sint lightmapIndex,
                                     image_t *image, bool mipRawImage);

// font stuff
void R_InitFreeType(void);
void R_DoneFreeType(void);

/*
=============================================================

IMAGE LOADERS

=============================================================
*/

void R_LoadJPG(pointer name, uchar8 **pic, sint *width, sint *height);
void R_LoadPNG(pointer name, uchar8 **pic, sint *width, sint *height);
void R_LoadTGA(pointer name, uchar8 **pic, sint *width, sint *height);

/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void        GLimp_Init(bool fixedFunction);
void        GLimp_Shutdown(void);
void        GLimp_EndFrame(void);
void        GLimp_LogComment(pointer comment);
void        GLimp_Minimize(void);
void        GLimp_SetGamma(uchar8 red[256], uchar8 green[256],
                           uchar8 blue[256]);

bool GLimp_SpawnRenderThread(void (*function)(void));
void *GLimp_RendererSleep(void);
void        GLimp_FrontEndSleep(void);
void        GLimp_WakeRenderer(void *data);
void GLimp_SyncRenderThread(void);


#endif //!DEDICATED

#endif //!__TR_COMMON_H__
