////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2005 Id Software, Inc.
// Copyright(C) 2000 - 2013 Darklegion Development
// Copyright(C) 2011 - 2018 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   r_common.h
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2015
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_COMMON_H__
#define __R_COMMON_H__

#if !defined ( DEDICATED ) && !defined ( UPDATE_SERVER )

#ifndef __Q_SHARED_H__
#include <qcommon/q_shared.h>
#endif
#ifndef __IQM_H__
#include <GPURenderer/iqm.h>
#endif
#ifndef __QGL_H__
#include <GPURenderer/qgl.h>
#endif
#ifndef __R_PUBLIC_H__
#include <GPURenderer/r_public.h>
#endif

typedef enum
{
    IMGTYPE_COLORALPHA, // for color, lightmap, diffuse, and specular
    IMGTYPE_NORMAL,
    IMGTYPE_NORMALHEIGHT,
    IMGTYPE_DELUXE, // normals are swizzled, deluxe are not
} imgType_t;

typedef enum
{
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
} imgFlags_t;

typedef struct image_s
{
    UTF8		imgName[MAX_QPATH];		// game path, including extension
    S32			width, height;				// source image
    S32			uploadWidth, uploadHeight;	// after power of two and picmip but not including clamp to MAX_TEXTURE_SIZE
    U32			texnum;					// gl texture binding
    
    S32			frameUsed;			// for texture usage in frame statistics
    
    S32			internalFormat;
    S32			TMU;				// only needed for voodoo2
    
    imgType_t   type;
    S32 /*imgFlags_t*/  flags;
    
    struct image_s*	next;
} image_t;

// any change in the LIGHTMAP_* defines here MUST be reflected in
// R_FindShader() in tr_bsp.c
#define LIGHTMAP_2D         -4	// shader is for 2D rendering
#define LIGHTMAP_BY_VERTEX  -3	// pre-lit triangle models
#define LIGHTMAP_WHITEIMAGE -2
#define LIGHTMAP_NONE       -1

extern vidconfig_t glConfig; // outside of TR since it shouldn't be cleared during ref re-init

//
// cvars
//
extern convar_t* r_stencilbits;			// number of desired stencil bits
extern convar_t* r_depthbits;			// number of desired depth bits
extern convar_t* r_colorbits;			// number of desired color bits, only relevant for fullscreen
extern convar_t* r_alphabits;			// number of desired alpha bits
extern convar_t* r_texturebits;			// number of desired texture bits
extern convar_t* r_ext_multisample;
// 0 = use framebuffer depth
// 16 = use 16-bit textures
// 32 = use 32-bit textures
// all else = error

extern convar_t* r_customwidth;
extern convar_t* r_customheight;
extern convar_t* r_pixelAspect;
extern convar_t* r_noborder;
extern convar_t* r_fullscreen;
extern convar_t* r_ignorehwgamma;		// overrides hardware gamma capabilities
extern convar_t* r_drawBuffer;
extern convar_t* r_swapInterval;
extern convar_t* r_glMajorVersion;  // override GL version autodetect (for testing)
extern convar_t* r_glMinorVersion;
extern convar_t* r_glDebugProfile;
extern convar_t* r_glCoreProfile;
extern convar_t* r_allowExtensions;				// global enable/disable of OpenGL extensions
extern convar_t* r_ext_compressed_textures;		// these control use of specific extensions
extern convar_t* r_ext_multitexture;
extern convar_t* r_ext_compiled_vertex_array;
extern convar_t* r_ext_texture_env_add;

extern convar_t* r_ext_texture_filter_anisotropic;
extern convar_t* r_ext_max_anisotropy;

extern convar_t* r_stereoEnabled;

extern	convar_t*	r_saveFontData;

bool	R_GetModeInfo( S32* width, S32* height, F32* windowAspect, S32 mode );

F32 R_NoiseGet4f( F32 x, F32 y, F32 z, F64 t );
void  R_NoiseInit( void );

image_t* R_FindImageFile( StringEntry name, imgType_t type, S32/*imgFlags_t*/ flags );
image_t* R_CreateImage( StringEntry name, U8* pic, S32 width, S32 height, imgType_t type, S32 flags, S32 internalFormat );

void R_IssuePendingRenderCommands( void );
qhandle_t		 RE_RegisterShaderLightMap( StringEntry name, S32 lightmapIndex );
qhandle_t RE_RegisterShaderFromImage( StringEntry name, S32 lightmapIndex, image_t* image, bool mipRawImage );

// font stuff
void R_InitFreeType( void );
void R_DoneFreeType( void );

/*
=============================================================

IMAGE LOADERS

=============================================================
*/

void R_LoadJPG( StringEntry name, U8** pic, S32* width, S32* height );
void R_LoadPNG( StringEntry name, U8** pic, S32* width, S32* height );
void R_LoadTGA( StringEntry name, U8** pic, S32* width, S32* height );

/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void		GLimp_Init( bool fixedFunction );
void		GLimp_Shutdown( void );
void		GLimp_EndFrame( void );
void		GLimp_LogComment( StringEntry comment );
void		GLimp_Minimize( void );
void		GLimp_SetGamma( U8 red[256], U8 green[256], U8 blue[256] );

bool GLimp_SpawnRenderThread( void ( *function )( void ) );
void* GLimp_RendererSleep( void );
void		GLimp_FrontEndSleep( void );
void		GLimp_WakeRenderer( void* data );
void GLimp_SyncRenderThread( void );


#endif //!DEDICATED

#endif //!__TR_COMMON_H__
