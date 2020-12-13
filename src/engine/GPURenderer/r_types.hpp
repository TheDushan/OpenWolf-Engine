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
// File name:   r_types.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_TYPES_H__
#define __R_TYPES_H__

#define	MAX_DLIGHTS		32		// can't be increased, because bit flags are used on surfaces

#define	REFENTITYNUM_BITS	10		// can't be increased without changing drawsurf bit packing
#define	REFENTITYNUM_MASK	((1<<REFENTITYNUM_BITS) - 1)
// the last N-bit number (2^REFENTITYNUM_BITS - 1) is reserved for the special world refentity,
//  and this is reflected by the value of MAX_REFENTITIES (which therefore is not a power-of-2)
#define	MAX_REFENTITIES		((1<<REFENTITYNUM_BITS) - 1)
#define	REFENTITYNUM_WORLD	((1<<REFENTITYNUM_BITS) - 1)

// renderfx flags
#define	RF_MINLIGHT		0x0001		// allways have some light (viewmodel, some items)
#define	RF_THIRD_PERSON		0x0002		// don't draw through eyes, only mirrors (player bodies, chat sprites)
#define	RF_FIRST_PERSON		0x0004		// only draw through eyes (view weapon, damage blood blob)
#define	RF_DEPTHHACK		0x0008		// for view weapon Z crunching

#define RF_CROSSHAIR		0x0010		// This item is a cross hair and will draw over everything similar to
// DEPTHHACK in stereo rendering mode, with the difference that the
// projection matrix won't be hacked to reduce the stereo separation as
// is done for the gun.

#define	RF_NOSHADOW		0x0040		// don't add stencil shadows

#define RF_LIGHTING_ORIGIN	0x0080		// use refEntity->lightingOrigin instead of refEntity->origin
// for lighting.  This allows entities to sink into the floor
// with their origin going solid, and allows all parts of a
// player to get the same lighting

#define	RF_SHADOW_PLANE		0x0100		// use refEntity->shadowPlane
#define	RF_WRAP_FRAMES		0x0200		// mod the model frames by the maxframes to allow continuous
// animation without needing to know the frame count

// refdef flags
#define RDF_NOWORLDMODEL	0x0001		// used for player configuration screen
#define RDF_HYPERSPACE		0x0004		// teleportation effect
#define	RDF_UNDERWATER	1024      //

typedef struct
{
    vec3_t		xyz;
    float32		st[2];
    uchar8		modulate[4];
} polyVert_t;

typedef struct poly_s
{
    qhandle_t			hShader;
    sint					numVerts;
    polyVert_t*			verts;
} poly_t;

typedef enum
{
    RT_MODEL,
    RT_POLY,
    RT_SPRITE,
    RT_BEAM,
    RT_RAIL_CORE,
    RT_RAIL_RINGS,
    RT_LIGHTNING,
    RT_PORTALSURFACE,		// doesn't draw anything, just info for portals
    
    RT_MAX_REF_ENTITY_TYPE
} refEntityType_t;

typedef struct
{
    refEntityType_t	reType;
    sint			renderfx;
    
    qhandle_t	hModel;				// opaque type outside refresh
    
    // most recent data
    vec3_t		lightingOrigin;		// so multi-part models can be lit identically (RF_LIGHTING_ORIGIN)
    float32		shadowPlane;		// projection shadows go here, stencils go slightly lower
    
    vec3_t		axis[3];			// rotation vectors
    bool	    nonNormalizedAxes;	// axis are not normalized, i.e. they have scale
    float32		origin[3];			// also used as MODEL_BEAM's "from"
    sint			frame;				// also used as MODEL_BEAM's diameter
    
    // previous data for frame interpolation
    float32		oldorigin[3];		// also used as MODEL_BEAM's "to"
    sint			oldframe;
    float32		backlerp;			// 0.0 = current, 1.0 = old
    
    // texturing
    sint			skinNum;			// inline skin index
    qhandle_t	customSkin;			// nullptr for default skin
    qhandle_t	customShader;		// use one image for the entire thing
    
    // misc
    uchar8		shaderRGBA[4];		// colors used by rgbgen entity shaders
    float32		shaderTexCoord[2];	// texture coordinates used by tcMod entity modifiers
    float32		shaderTime;			// subtracted from refdef time to control effect start times
    
    // extra sprite information
    float32		radius;
    float32		rotation;
} refEntity_t;


#define	MAX_RENDER_STRINGS			8
#define	MAX_RENDER_STRING_LENGTH	32

typedef struct
{
    sint			x, y, width, height;
    float32		fov_x, fov_y;
    vec3_t		vieworg;
    vec3_t		viewaxis[3];		// transformation matrix
    
    // time in milliseconds for shader effects and other time dependent rendering issues
    sint			time;
    
    sint			rdflags;			// RDF_NOWORLDMODEL, etc
    
    // 1 bits will prevent the associated area from rendering at all
    uchar8		areamask[MAX_MAP_AREA_BYTES];
    
    // text messages for deform text shaders
    valueType		text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];
} refdef_t;


typedef enum
{
    STEREO_CENTER,
    STEREO_LEFT,
    STEREO_RIGHT
} stereoFrame_t;


/*
** vidconfig_t
**
** Contains variables specific to the OpenGL configuration
** being run right now.  These are constant once the OpenGL
** subsystem is initialized.
*/
typedef enum
{
    TC_NONE,
    TC_S3TC,  // this is for the GL_S3_s3tc extension.
    TC_S3TC_ARB,  // this is for the GL_EXT_texture_compression_s3tc extension.
    TC_EXT_COMP_S3TC
} textureCompression_t;

typedef enum
{
    GLDRV_UNKNOWN = -1,
    GLDRV_ICD,					// driver is integrated with window system
    // WARNING: there are tests that check for
    // > GLDRV_ICD for minidriverness, so this
    // should always be the lowest value in this
    // enum set
    GLDRV_STANDALONE,			// driver is a non-3Dfx standalone driver
    GLDRV_VOODOO,				// driver is a 3Dfx standalone driver
    
// XreaL BEGIN
    GLDRV_OPENGL3,				// new driver system
    GLDRV_MESA,					// crap
// XreaL END

} glDriverType_t;

typedef enum
{
    GLHW_UNKNOWN = -1,
    GLHW_GENERIC,				// where everthing works the way it should
    GLHW_3DFX_2D3D,				// Voodoo Banshee or Voodoo3, relevant since if this is
    // the hardware type then there can NOT exist a secondary
    // display adapter
    GLHW_RIVA128,				// where you can't interpolate alpha
    GLHW_RAGEPRO,				// where you can't modulate alpha on alpha textures
    GLHW_PERMEDIA2,				// where you don't have src*dst
    
// XreaL BEGIN
    GLHW_ATI,					// where you don't have proper GLSL support
    GLHW_ATI_DX10,				// ATI Radeon HD series DX10 hardware
    GLHW_NV_DX10				// Geforce 8/9 class DX10 hardware
// XreaL END

} glHardwareType_t;

typedef struct
{
    valueType					renderer_string[MAX_STRING_CHARS];
    valueType					vendor_string[MAX_STRING_CHARS];
    valueType					version_string[MAX_STRING_CHARS];
    valueType					extensions_string[BIG_INFO_STRING];
    
    sint						maxTextureSize;			// queried from GL
    sint						numTextureUnits;		// multitexture ability
    
    sint						colorBits, depthBits, stencilBits;
    
    glDriverType_t			driverType;
    glHardwareType_t		hardwareType;
    
    bool				deviceSupportsGamma;
    sint/*textureCompression_t*/	textureCompression;
    bool				textureEnvAddAvailable;
    
    sint						vidWidth, vidHeight;
    // aspect is the screen's physical width / height, which may be different
    // than scrWidth / scrHeight if the pixels are non-square
    // normal screens should be 4/3, but wide aspect monitors may be 16/9
    float32					windowAspect;
    float32					displayAspect;
    
    sint						displayFrequency;
    
    // synonymous with "does rendering consume the entire screen?", therefore
    // a Voodoo or Voodoo2 will have this set to TRUE, as will a Win32 ICD that
    // used CDS.
    bool				isFullscreen;
    bool				stereoEnabled;
    bool				textureFilterAnisotropic;
    sint							maxAnisotropy;
    bool smpActive;
} vidconfig_t;

#endif	//!__R_TYPES_H__
