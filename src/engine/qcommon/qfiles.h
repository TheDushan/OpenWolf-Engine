////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2010 Robert Beckebans
// Copyright(C) 2011 - 2019 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of the OpenWolf GPL Source Code.
// OpenWolf Source Code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenWolf Source Code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenWolf Source Code.  If not, see <http://www.gnu.org/licenses/>.
//
// In addition, the OpenWolf Source Code is also subject to certain additional terms.
// You should have received a copy of these additional terms immediately following the
// terms and conditions of the GNU General Public License which accompanied the
// OpenWolf Source Code. If not, please request a copy in writing from id Software
// at the address below.
//
// If you have questions concerning this license or the applicable additional terms,
// you may contact in writing id Software LLC, c/o ZeniMax Media Inc.,
// Suite 120, Rockville, Maryland 20850 USA.
//
// -------------------------------------------------------------------------------------
// File name:   qfiles.h
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: quake file formats
//              This file must be identical in the quake and utils directories
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __QFILES_H__
#define __QFILES_H__

// surface geometry should not exceed these limits
#define SHADER_MAX_VERTEXES 10000	// Arnout: 1024+1 (1 buffer for RB_EndSurface overflow check) // JPW NERVE was 4000, 1000 in q3ta
#define SHADER_MAX_INDEXES  ( 6 * SHADER_MAX_VERTEXES )
#define SHADER_MAX_TRIANGLES (SHADER_MAX_INDEXES / 3)

// RB: DON'T USE MAX_QPATH HERE SO WE CAN INCREASE IT !!

/*
========================================================================

TGA files are used for 24/32 bit images

========================================================================
*/

typedef struct _TargaHeader
{
    U8   id_length, colormap_type, image_type;
    U16  colormap_index, colormap_length;
    U8   colormap_size;
    U16  x_origin, y_origin, width, height;
    U8   pixel_size, attributes;
} TargaHeader;



/*
========================================================================

.MD3 triangle model file format

========================================================================
*/

#define MD3_IDENT           ( ( '3' << 24 ) + ( 'P' << 16 ) + ( 'D' << 8 ) + 'I' )
#define MD3_VERSION         15

// limits
#define MD3_MAX_LODS        4
#define MD3_MAX_TRIANGLES   8192	// per surface
#define MD3_MAX_VERTS       4096	// per surface
#define MD3_MAX_SHADERS     256	// per surface
#define MD3_MAX_FRAMES      1024	// per model
#define MD3_MAX_SURFACES    32	// per model
#define MD3_MAX_TAGS        16	// per frame

// vertex scales
#define MD3_XYZ_SCALE       ( 1.0 / 64 )

typedef struct md3Frame_s
{
    vec3_t          bounds[2];
    vec3_t          localOrigin;
    F32           radius;
    UTF8            name[16];
} md3Frame_t;

typedef struct md3Tag_s
{
    UTF8            name[64];	// tag name
    vec3_t          origin;
    vec3_t          axis[3];
} md3Tag_t;

/*
** md3Surface_t
**
** CHUNK			SIZE
** header			sizeof( md3Surface_t )
** shaders			sizeof( md3Shader_t ) * numShaders
** triangles[0]		sizeof( md3Triangle_t ) * numTriangles
** st				sizeof( md3St_t ) * numVerts
** XyzNormals		sizeof( md3XyzNormal_t ) * numVerts * numFrames
*/
typedef struct
{
    S32             ident;		//
    
    UTF8            name[64];	// polyset name
    
    S32             flags;
    S32             numFrames;	// all surfaces in a model should have the same
    
    S32             numShaders;	// all surfaces in a model should have the same
    S32             numVerts;
    
    S32             numTriangles;
    S32             ofsTriangles;
    
    S32             ofsShaders;	// offset from start of md3Surface_t
    S32             ofsSt;		// texture coords are common for all frames
    S32             ofsXyzNormals;	// numVerts * numFrames
    
    S32             ofsEnd;		// next surface follows
} md3Surface_t;

typedef struct
{
    UTF8            name[64];
    S32             shaderIndex;	// for in-game use
} md3Shader_t;

typedef struct
{
    S32             indexes[3];
} md3Triangle_t;

typedef struct
{
    F32           st[2];
} md3St_t;

typedef struct
{
    S16           xyz[3];
    S16           normal;
} md3XyzNormal_t;

typedef struct
{
    S32             ident;
    S32             version;
    
    UTF8            name[64];	// model name
    
    S32             flags;
    
    S32             numFrames;
    S32             numTags;
    S32             numSurfaces;
    
    S32             numSkins;
    
    S32             ofsFrames;	// offset for first frame
    S32             ofsTags;	// numFrames * numTags
    S32             ofsSurfaces;	// first surface, others follow
    
    S32             ofsEnd;		// end of file
} md3Header_t;

/*
========================================================================

.tag tag file format

========================================================================
*/

#define TAG_IDENT           ( ( '1' << 24 ) + ( 'G' << 16 ) + ( 'A' << 8 ) + 'T' )
#define TAG_VERSION         1

typedef struct
{
    S32             ident;
    S32             version;
    
    S32             numTags;
    
    S32             ofsEnd;
} tagHeader_t;

typedef struct
{
    UTF8            filename[64];
    S32             start;
    S32             count;
} tagHeaderExt_t;

// Ridah, mesh compression
/*
==============================================================================

MDC file format

==============================================================================
*/

#define MDC_IDENT           ( ( 'C' << 24 ) + ( 'P' << 16 ) + ( 'D' << 8 ) + 'I' )
#define MDC_VERSION         2

// version history:
// 1 - original
// 2 - changed tag structure so it only lists the names once

typedef struct
{
    U32    ofsVec;		// offset direction from the last base frame
//  U16  ofsVec;
} mdcXyzCompressed_t;

typedef struct
{
    UTF8            name[64];	// tag name
} mdcTagName_t;

#define MDC_TAG_ANGLE_SCALE ( 360.0 / 32700.0 )

typedef struct
{
    S16           xyz[3];
    S16           angles[3];
} mdcTag_t;

/*
** mdcSurface_t
**
** CHUNK			SIZE
** header			sizeof( md3Surface_t )
** shaders			sizeof( md3Shader_t ) * numShaders
** triangles[0]		sizeof( md3Triangle_t ) * numTriangles
** st				sizeof( md3St_t ) * numVerts
** XyzNormals		sizeof( md3XyzNormal_t ) * numVerts * numBaseFrames
** XyzCompressed	sizeof( mdcXyzCompressed ) * numVerts * numCompFrames
** frameBaseFrames	sizeof( S16 ) * numFrames
** frameCompFrames	sizeof( S16 ) * numFrames (-1 if frame is a baseFrame)
*/
typedef struct
{
    S32             ident;		//
    
    UTF8            name[64];	// polyset name
    
    S32             flags;
    S32             numCompFrames;	// all surfaces in a model should have the same
    S32             numBaseFrames;	// ditto
    
    S32             numShaders;	// all surfaces in a model should have the same
    S32             numVerts;
    
    S32             numTriangles;
    S32             ofsTriangles;
    
    S32             ofsShaders;	// offset from start of md3Surface_t
    S32             ofsSt;		// texture coords are common for all frames
    S32             ofsXyzNormals;	// numVerts * numBaseFrames
    S32             ofsXyzCompressed;	// numVerts * numCompFrames
    
    S32             ofsFrameBaseFrames;	// numFrames
    S32             ofsFrameCompFrames;	// numFrames
    
    S32             ofsEnd;		// next surface follows
} mdcSurface_t;

typedef struct
{
    S32             ident;
    S32             version;
    
    UTF8            name[64];	// model name
    
    S32             flags;
    
    S32             numFrames;
    S32             numTags;
    S32             numSurfaces;
    
    S32             numSkins;
    
    S32             ofsFrames;	// offset for first frame, stores the bounds and localOrigin
    S32             ofsTagNames;	// numTags
    S32             ofsTags;	// numFrames * numTags
    S32             ofsSurfaces;	// first surface, others follow
    
    S32             ofsEnd;		// end of file
} mdcHeader_t;

// done.

/*
==============================================================================

MDS file format (Wolfenstein Skeletal Format)

==============================================================================
*/

#define MDS_IDENT           ( ( 'W' << 24 ) + ( 'S' << 16 ) + ( 'D' << 8 ) + 'M' )
#define MDS_VERSION         4
#define MDS_MAX_VERTS       6000
#define MDS_MAX_TRIANGLES   8192
#define MDS_MAX_BONES       128
#define MDS_MAX_SURFACES    32
#define MDS_MAX_TAGS        128

#define MDS_TRANSLATION_SCALE   ( 1.0 / 64 )

typedef struct
{
    S32             boneIndex;	// these are indexes into the boneReferences,
    F32           boneWeight;	// not the global per-frame bone list
    vec3_t          offset;
} mdsWeight_t;

typedef struct
{
    vec3_t          normal;
    vec2_t          texCoords;
    S32             numWeights;
    S32             fixedParent;	// stay equi-distant from this parent
    F32           fixedDist;
    mdsWeight_t     weights[1];	// variable sized
} mdsVertex_t;

typedef struct
{
    S32             indexes[3];
} mdsTriangle_t;

typedef struct
{
    S32             ident;
    
    UTF8            name[64];	// polyset name
    UTF8            shader[64];
    S32             shaderIndex;	// for in-game use
    
    S32             minLod;
    
    S32             ofsHeader;	// this will be a negative number
    
    S32             numVerts;
    S32             ofsVerts;
    
    S32             numTriangles;
    S32             ofsTriangles;
    
    S32             ofsCollapseMap;	// numVerts * S32
    
    // Bone references are a set of ints representing all the bones
    // present in any vertex weights for this surface.  This is
    // needed because a model may have surfaces that need to be
    // drawn at different sort times, and we don't want to have
    // to re-interpolate all the bones for each surface.
    S32             numBoneReferences;
    S32             ofsBoneReferences;
    
    S32             ofsEnd;		// next surface follows
} mdsSurface_t;

typedef struct
{
    //F32     angles[3];
    //F32     ofsAngles[2];
    S16           angles[4];	// to be converted to axis at run-time (this is also better for lerping)
    S16           ofsAngles[2];	// PITCH/YAW, head in this direction from parent to go to the offset position
} mdsBoneFrameCompressed_t;

// NOTE: this only used at run-time
typedef struct
{
    F32           matrix[3][3];	// 3x3 rotation
    vec3_t          translation;	// translation vector
} mdsBoneFrame_t;

typedef struct
{
    vec3_t          bounds[2];	// bounds of all surfaces of all LOD's for this frame
    vec3_t          localOrigin;	// midpoint of bounds, used for sphere cull
    F32           radius;		// dist from localOrigin to corner
    vec3_t          parentOffset;	// one bone is an ascendant of all other bones, it starts the hierachy at this position
    mdsBoneFrameCompressed_t bones[1];	// [numBones]
} mdsFrame_t;

typedef struct
{
    S32             numSurfaces;
    S32             ofsSurfaces;	// first surface, others follow
    S32             ofsEnd;		// next lod follows
} mdsLOD_t;

typedef struct
{
    UTF8            name[64];	// name of tag
    F32           torsoWeight;
    S32             boneIndex;	// our index in the bones
} mdsTag_t;

#define BONEFLAG_TAG        1	// this bone is actually a tag

typedef struct
{
    UTF8            name[64];	// name of bone
    S32             parent;		// not sure if this is required, no harm throwing it in
    F32           torsoWeight;	// scale torso rotation about torsoParent by this
    F32           parentDist;
    S32             flags;
} mdsBoneInfo_t;

typedef struct
{
    S32             ident;
    S32             version;
    
    UTF8            name[64];	// model name
    
    F32           lodScale;
    F32           lodBias;
    
    // frames and bones are shared by all levels of detail
    S32             numFrames;
    S32             numBones;
    S32             ofsFrames;	// md4Frame_t[numFrames]
    S32             ofsBones;	// mdsBoneInfo_t[numBones]
    S32             torsoParent;	// index of bone that is the parent of the torso
    
    S32             numSurfaces;
    S32             ofsSurfaces;
    
    // tag data
    S32             numTags;
    S32             ofsTags;	// mdsTag_t[numTags]
    
    S32             ofsEnd;		// end of file
} mdsHeader_t;

/*
==============================================================================

MDM file format (Wolfenstein Skeletal Mesh)

version history:
	2 - initial version
	3 - removed all frame data, this format is pure mesh and bone references now

==============================================================================
*/

#define MDM_IDENT           ( ( 'W' << 24 ) + ( 'M' << 16 ) + ( 'D' << 8 ) + 'M' )
#define MDM_VERSION         3
#define MDM_MAX_VERTS       6000
#define MDM_MAX_TRIANGLES   8192
#define MDM_MAX_SURFACES    32
#define MDM_MAX_TAGS        128

#define MDM_TRANSLATION_SCALE   ( 1.0 / 64 )

typedef struct
{
    S32             boneIndex;	// these are indexes into the boneReferences,
    F32           boneWeight;	// not the global per-frame bone list
    vec3_t          offset;
} mdmWeight_t;

typedef struct
{
    vec3_t          normal;
    vec2_t          texCoords;
    S32             numWeights;
    mdmWeight_t     weights[1];	// variable sized
} mdmVertex_t;

typedef struct
{
    S32             indexes[3];
} mdmTriangle_t;

typedef struct
{
    S32             ident;
    
    UTF8            name[64];	// polyset name
    UTF8            shader[64];
    S32             shaderIndex;	// for in-game use
    
    S32             minLod;
    
    S32             ofsHeader;	// this will be a negative number
    
    S32             numVerts;
    S32             ofsVerts;
    
    S32             numTriangles;
    S32             ofsTriangles;
    
    S32             ofsCollapseMap;	// numVerts * S32
    
    // Bone references are a set of ints representing all the bones
    // present in any vertex weights for this surface.  This is
    // needed because a model may have surfaces that need to be
    // drawn at different sort times, and we don't want to have
    // to re-interpolate all the bones for each surface.
    S32             numBoneReferences;
    S32             ofsBoneReferences;
    
    S32             ofsEnd;		// next surface follows
} mdmSurface_t;

/*typedef struct {
	vec3_t		bounds[2];			// bounds of all surfaces of all LOD's for this frame
	vec3_t		localOrigin;		// midpoint of bounds, used for sphere cull
	F32		radius;				// dist from localOrigin to corner
	vec3_t		parentOffset;		// one bone is an ascendant of all other bones, it starts the hierachy at this position
} mdmFrame_t;*/

typedef struct
{
    S32             numSurfaces;
    S32             ofsSurfaces;	// first surface, others follow
    S32             ofsEnd;		// next lod follows
} mdmLOD_t;

/*typedef struct {
	UTF8		name[64];	// name of tag
	F32		torsoWeight;
	S32			boneIndex;			// our index in the bones

	S32			numBoneReferences;
	S32			ofsBoneReferences;

	S32			ofsEnd;				// next tag follows
} mdmTag_t;*/

// Tags always only have one parent bone
typedef struct
{
    UTF8            name[64];	// name of tag
    vec3_t          axis[3];
    
    S32             boneIndex;
    vec3_t          offset;
    
    S32             numBoneReferences;
    S32             ofsBoneReferences;
    
    S32             ofsEnd;		// next tag follows
} mdmTag_t;

typedef struct
{
    S32             ident;
    S32             version;
    
    UTF8            name[64];	// model name
    /*	UTF8		bonesfile[64];	// bone file
    
    #ifdef UTILS
    	S32			skel;
    #else
    	// dummy in file, set on load to link to MDX
    	qhandle_t	skel;
    #endif // UTILS
    */
    F32           lodScale;
    F32           lodBias;
    
    // frames and bones are shared by all levels of detail
    /*	S32			numFrames;
    	S32			ofsFrames;			// mdmFrame_t[numFrames]
    */
    S32             numSurfaces;
    S32             ofsSurfaces;
    
    // tag data
    S32             numTags;
    S32             ofsTags;
    
    S32             ofsEnd;		// end of file
} mdmHeader_t;

/*
==============================================================================

MDX file format (Wolfenstein Skeletal Data)

version history:
	1 - initial version
	2 - moved parentOffset from the mesh to the skeletal data file

==============================================================================
*/

#define MDX_IDENT           ( ( 'W' << 24 ) + ( 'X' << 16 ) + ( 'D' << 8 ) + 'M' )
#define MDX_VERSION         2
#define MDX_MAX_BONES       128

typedef struct
{
    vec3_t          bounds[2];	// bounds of this frame
    vec3_t          localOrigin;	// midpoint of bounds, used for sphere cull
    F32           radius;		// dist from localOrigin to corner
    vec3_t          parentOffset;	// one bone is an ascendant of all other bones, it starts the hierachy at this position
} mdxFrame_t;

typedef struct
{
    //F32     angles[3];
    //F32     ofsAngles[2];
    S16           angles[4];	// to be converted to axis at run-time (this is also better for lerping)
    S16           ofsAngles[2];	// PITCH/YAW, head in this direction from parent to go to the offset position
} mdxBoneFrameCompressed_t;

// NOTE: this only used at run-time
// FIXME: do we really need this?
typedef struct
{
    F32           matrix[3][3];	// 3x3 rotation
    vec3_t          translation;	// translation vector
} mdxBoneFrame_t;

typedef struct
{
    UTF8            name[64];	// name of bone
    S32             parent;		// not sure if this is required, no harm throwing it in
    F32           torsoWeight;	// scale torso rotation about torsoParent by this
    F32           parentDist;
    S32             flags;
} mdxBoneInfo_t;

typedef struct
{
    S32             ident;
    S32             version;
    
    UTF8            name[64];	// model name
    
    // bones are shared by all levels of detail
    S32             numFrames;
    S32             numBones;
    S32             ofsFrames;	// (mdxFrame_t + mdxBoneFrameCompressed_t[numBones]) * numframes
    S32             ofsBones;	// mdxBoneInfo_t[numBones]
    S32             torsoParent;	// index of bone that is the parent of the torso
    
    S32             ofsEnd;		// end of file
} mdxHeader_t;

/*
==============================================================================

  .BSP file format

==============================================================================
*/

#define BSP_IDENT (('P' << 24) + ('S' << 16) + ('B' << 8) + 'I') // little-endian "OBSP"
#define BSP_VERSION 46

// there shouldn't be any problem with increasing these values at the
// expense of more memory allocation in the utilities
#ifndef Q3MAP2
//#define   MAX_MAP_MODELS      0x400
#define MAX_MAP_MODELS      0x800
#define MAX_MAP_BRUSHES     16384
#define MAX_MAP_ENTITIES    4096
#define MAX_MAP_ENTSTRING   0x40000
#define MAX_MAP_SHADERS     0x400

#define MAX_MAP_AREAS       0x100	// MAX_MAP_AREA_BYTES in q_shared must match!
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

#ifndef Q3MAP2
#define MAX_WORLD_COORD     ( 128 * 1024 )
#define MIN_WORLD_COORD     ( -128 * 1024 )
#define WORLD_SIZE          ( MAX_WORLD_COORD - MIN_WORLD_COORD )
#endif

//=============================================================================


typedef struct
{
    S32             fileofs, filelen;
} lump_t;

#define	LUMP_ENTITIES		0
#define	LUMP_SHADERS		1
#define	LUMP_PLANES			2
#define	LUMP_NODES			3
#define	LUMP_LEAFS			4
#define	LUMP_LEAFSURFACES	5
#define	LUMP_LEAFBRUSHES	6
#define	LUMP_MODELS			7
#define	LUMP_BRUSHES		8
#define	LUMP_BRUSHSIDES		9
#define	LUMP_DRAWVERTS		10
#define	LUMP_DRAWINDEXES	11
#define	LUMP_FOGS			12
#define	LUMP_SURFACES		13
#define	LUMP_LIGHTMAPS		14
#define	LUMP_LIGHTGRID		15
#define	LUMP_VISIBILITY		16
#define	HEADER_LUMPS		17

typedef struct
{
    S32             ident;
    S32             version;
    
    lump_t          lumps[HEADER_LUMPS];
} dheader_t;

typedef struct
{
    F32           mins[3], maxs[3];
    S32             firstSurface, numSurfaces;
    S32             firstBrush, numBrushes;
} dmodel_t;

// planes x^1 is allways the opposite of plane x

typedef struct
{
    F32           normal[3];
    F32           dist;
} dplane_t;

typedef struct
{
    S32             planeNum;
    S32             children[2];	// negative numbers are -(leafs+1), not nodes
    S32             mins[3];	// for frustom culling
    S32             maxs[3];
} dnode_t;

typedef struct
{
    S32             cluster;	// -1 = opaque cluster (do I still store these?)
    S32             area;
    
    S32             mins[3];	// for frustum culling
    S32             maxs[3];
    
    S32             firstLeafSurface;
    S32             numLeafSurfaces;
    
    S32             firstLeafBrush;
    S32             numLeafBrushes;
} dleaf_t;

typedef struct
{
    S32             planeNum;	// positive plane side faces out of the leaf
    S32             shaderNum;
} dbrushside_t;

typedef struct
{
    UTF8            shader[64];
    S32             surfaceFlags;
    S32             contentFlags;
} dshader_t;

typedef struct
{
    S32             firstSide;
    S32             numSides;
    S32             shaderNum;	// the shader that determines the contents flags
} dbrush_t;

typedef struct
{
    UTF8            shader[64];
    S32             brushNum;
    S32             visibleSide;	// the brush side that ray tests need to clip against (-1 == none)
} dfog_t;

// light grid
typedef struct
{
    U8            ambient[3];
    U8            directed[3];
    U8            latLong[2];
} dgridPoint_t;

typedef struct
{
    vec3_t xyz;
    F32 st[2];
    F32 lightmap[2];
    vec3_t normal;
    U8 color[4];
} drawVert_t;

#define drawVert_t_cleared(x) drawVert_t (x) = {{0, 0, 0}, {0, 0}, {0, 0}, {0, 0, 0}, {0, 0, 0, 0}}

#ifndef Q3MAP2
typedef enum
{
    MST_BAD,
    MST_PLANAR,
    MST_PATCH,
    MST_TRIANGLE_SOUP,
    MST_FLARE,
    MST_FOLIAGE
} mapSurfaceType_t;
#endif

typedef struct
{
    S32             shaderNum;
    S32             fogNum;
    S32             surfaceType;
    
    S32             firstVert;
    S32             numVerts;	// ydnar: num verts + foliage origins (for cleaner lighting code in q3map)
    
    S32             firstIndex;
    S32             numIndexes;
    
    S32             lightmapNum;
    S32             lightmapX, lightmapY;
    S32             lightmapWidth, lightmapHeight;
    
    vec3_t          lightmapOrigin;
    vec3_t          lightmapVecs[3];	// for patches, [0] and [1] are lodbounds
    
    S32             patchWidth;	// ydnar: num foliage instances
    S32             patchHeight;	// ydnar: num foliage mesh verts
} dsurface_t;

//----(SA) added so I didn't change the dsurface_t struct (and thereby the bsp format) for something that doesn't need to be stored in the bsp
typedef struct
{
    UTF8*           lighttarg;
} drsurfaceInternal_t;

//----(SA) end

/*
==============================================================================

MDR file format

==============================================================================
*/

/*
 * Here are the definitions for Ravensoft's model format of md4. Raven stores their
 * playermodels in .mdr files, in some games, which are pretty much like the md4
 * format implemented by ID soft. It seems like ID's original md4 stuff is not used at all.
 * MDR is being used in EliteForce, JediKnight2 and Soldiers of Fortune2 (I think).
 * So this comes in handy for anyone who wants to make it possible to load player
 * models from these games.
 * This format has bone tags, which is similar to the thing you have in md3 I suppose.
 * Raven has released their version of md3view under GPL enabling me to add support
 * to this codebase. Thanks to Steven Howes aka Skinner for helping with example
 * source code.
 *
 * - Thilo Schulz (arny@ats.s.bawue.de)
 */

#define MDR_IDENT	(('5'<<24)+('M'<<16)+('D'<<8)+'R')
#define MDR_VERSION	2
#define	MDR_MAX_BONES	128

typedef struct
{
    S32			boneIndex;	// these are indexes into the boneReferences,
    F32		   boneWeight;		// not the global per-frame bone list
    vec3_t		offset;
} mdrWeight_t;

typedef struct
{
    vec3_t		normal;
    vec2_t		texCoords;
    S32			numWeights;
    mdrWeight_t	weights[1];		// variable sized
} mdrVertex_t;

typedef struct
{
    S32			indexes[3];
} mdrTriangle_t;

typedef struct
{
    S32			ident;
    
    UTF8		name[MAX_QPATH];	// polyset name
    UTF8		shader[MAX_QPATH];
    S32			shaderIndex;	// for in-game use
    
    S32			ofsHeader;	// this will be a negative number
    
    S32			numVerts;
    S32			ofsVerts;
    
    S32			numTriangles;
    S32			ofsTriangles;
    
    // Bone references are a set of ints representing all the bones
    // present in any vertex weights for this surface.  This is
    // needed because a model may have surfaces that need to be
    // drawn at different sort times, and we don't want to have
    // to re-interpolate all the bones for each surface.
    S32			numBoneReferences;
    S32			ofsBoneReferences;
    
    S32			ofsEnd;		// next surface follows
} mdrSurface_t;

typedef struct
{
    F32		matrix[3][4];
} mdrBone_t;

typedef struct
{
    vec3_t		bounds[2];		// bounds of all surfaces of all LOD's for this frame
    vec3_t		localOrigin;		// midpoint of bounds, used for sphere cull
    F32		radius;			// dist from localOrigin to corner
    UTF8		name[16];
    mdrBone_t	bones[1];		// [numBones]
} mdrFrame_t;

typedef struct
{
    U8 Comp[24]; // MC_COMP_BYTES is in MatComp.h, but don't want to couple
} mdrCompBone_t;

typedef struct
{
    vec3_t          bounds[2];		// bounds of all surfaces of all LOD's for this frame
    vec3_t          localOrigin;		// midpoint of bounds, used for sphere cull
    F32           radius;			// dist from localOrigin to corner
    mdrCompBone_t   bones[1];		// [numBones]
} mdrCompFrame_t;

typedef struct
{
    S32			numSurfaces;
    S32			ofsSurfaces;		// first surface, others follow
    S32			ofsEnd;				// next lod follows
} mdrLOD_t;

typedef struct
{
    S32                     boneIndex;
    UTF8            name[32];
} mdrTag_t;

typedef struct
{
    S32			ident;
    S32			version;
    
    UTF8		name[MAX_QPATH];	// model name
    
    // frames and bones are shared by all levels of detail
    S32			numFrames;
    S32			numBones;
    S32			ofsFrames;			// mdrFrame_t[numFrames]
    
    // each level of detail has completely separate sets of surfaces
    S32			numLODs;
    S32			ofsLODs;
    
    S32                     numTags;
    S32                     ofsTags;
    
    S32			ofsEnd;				// end of file
} mdrHeader_t;

#endif
