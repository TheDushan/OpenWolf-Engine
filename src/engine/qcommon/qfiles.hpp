////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2010 - Robert Beckebans
// Copyright(C) 2011 - 2023 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   qfiles.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description: quake file formats
//              This file must be identical in the quake and utils directories
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __QFILES_HPP__
#define __QFILES_HPP__

// surface geometry should not exceed these limits
#define SHADER_MAX_VERTEXES 10000   // Arnout: 1024+1 (1 buffer for RB_EndSurface overflow check) // JPW NERVE was 4000, 1000 in q3ta
#define SHADER_MAX_INDEXES  ( 6 * SHADER_MAX_VERTEXES )
#define SHADER_MAX_TRIANGLES (SHADER_MAX_INDEXES / 3)

// RB: DON'T USE MAX_QPATH HERE SO WE CAN INCREASE IT !!

/*
========================================================================

TGA files are used for 24/32 bit images

========================================================================
*/

typedef struct _TargaHeader {
    uchar8   id_length, colormap_type, image_type;
    uchar16  colormap_index, colormap_length;
    uchar8   colormap_size;
    uchar16  x_origin, y_origin, width, height;
    uchar8   pixel_size, attributes;
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
#define MD3_MAX_TRIANGLES   8192    // per surface
#define MD3_MAX_VERTS       4096    // per surface
#define MD3_MAX_SHADERS     256 // per surface
#define MD3_MAX_FRAMES      1024    // per model
#define MD3_MAX_SURFACES    32  // per model
#define MD3_MAX_TAGS        16  // per frame

// vertex scales
#define MD3_XYZ_SCALE       ( 1.0 / 64 )

typedef struct md3Frame_s {
    vec3_t          bounds[2];
    vec3_t          localOrigin;
    float32           radius;
    valueType            name[16];
} md3Frame_t;

typedef struct md3Tag_s {
    valueType            name[64];  // tag name
    vec3_t          origin;
    vec3_t          axis[3];
} md3Tag_t;

/*
** md3Surface_t
**
** CHUNK            SIZE
** header           sizeof( md3Surface_t )
** shaders          sizeof( md3Shader_t ) * numShaders
** triangles[0]     sizeof( md3Triangle_t ) * numTriangles
** st               sizeof( md3St_t ) * numVerts
** XyzNormals       sizeof( md3XyzNormal_t ) * numVerts * numFrames
*/
typedef struct {
    sint             ident;     //

    valueType            name[64];  // polyset name

    sint             flags;
    sint             numFrames; // all surfaces in a model should have the same

    sint
    numShaders;    // all surfaces in a model should have the same
    sint             numVerts;

    sint             numTriangles;
    sint             ofsTriangles;

    sint             ofsShaders;    // offset from start of md3Surface_t
    sint             ofsSt;     // texture coords are common for all frames
    sint             ofsXyzNormals; // numVerts * numFrames

    sint             ofsEnd;        // next surface follows
} md3Surface_t;

typedef struct {
    valueType            name[64];
    sint             shaderIndex;   // for in-game use
} md3Shader_t;

typedef struct {
    sint             indexes[3];
} md3Triangle_t;

typedef struct {
    float32           st[2];
} md3St_t;

typedef struct {
    schar16           xyz[3];
    schar16           normal;
} md3XyzNormal_t;

typedef struct {
    sint             ident;
    sint             version;

    valueType            name[64];  // model name

    sint             flags;

    sint             numFrames;
    sint             numTags;
    sint             numSurfaces;

    sint             numSkins;

    sint             ofsFrames; // offset for first frame
    sint             ofsTags;   // numFrames * numTags
    sint             ofsSurfaces;   // first surface, others follow

    sint             ofsEnd;        // end of file
} md3Header_t;

/*
========================================================================

.tag tag file format

========================================================================
*/

#define TAG_IDENT           ( ( '1' << 24 ) + ( 'G' << 16 ) + ( 'A' << 8 ) + 'T' )
#define TAG_VERSION         1

typedef struct {
    sint             ident;
    sint             version;

    sint             numTags;

    sint             ofsEnd;
} tagHeader_t;

typedef struct {
    valueType            filename[64];
    sint             start;
    sint             count;
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

typedef struct {
    uint    ofsVec;     // offset direction from the last base frame
    //  uchar16  ofsVec;
} mdcXyzCompressed_t;

typedef struct {
    valueType            name[64];  // tag name
} mdcTagName_t;

#define MDC_TAG_ANGLE_SCALE ( 360.0 / 32700.0 )

typedef struct {
    schar16           xyz[3];
    schar16           angles[3];
} mdcTag_t;

/*
** mdcSurface_t
**
** CHUNK            SIZE
** header           sizeof( md3Surface_t )
** shaders          sizeof( md3Shader_t ) * numShaders
** triangles[0]     sizeof( md3Triangle_t ) * numTriangles
** st               sizeof( md3St_t ) * numVerts
** XyzNormals       sizeof( md3XyzNormal_t ) * numVerts * numBaseFrames
** XyzCompressed    sizeof( mdcXyzCompressed ) * numVerts * numCompFrames
** frameBaseFrames  sizeof( schar16 ) * numFrames
** frameCompFrames  sizeof( schar16 ) * numFrames (-1 if frame is a baseFrame)
*/
typedef struct {
    sint             ident;     //

    valueType            name[64];  // polyset name

    sint             flags;
    sint
    numCompFrames; // all surfaces in a model should have the same
    sint             numBaseFrames; // ditto

    sint
    numShaders;    // all surfaces in a model should have the same
    sint             numVerts;

    sint             numTriangles;
    sint             ofsTriangles;

    sint             ofsShaders;    // offset from start of md3Surface_t
    sint             ofsSt;     // texture coords are common for all frames
    sint             ofsXyzNormals; // numVerts * numBaseFrames
    sint             ofsXyzCompressed;  // numVerts * numCompFrames

    sint             ofsFrameBaseFrames;    // numFrames
    sint             ofsFrameCompFrames;    // numFrames

    sint             ofsEnd;        // next surface follows
} mdcSurface_t;

typedef struct {
    sint             ident;
    sint             version;

    valueType            name[64];  // model name

    sint             flags;

    sint             numFrames;
    sint             numTags;
    sint             numSurfaces;

    sint             numSkins;

    sint
    ofsFrames; // offset for first frame, stores the bounds and localOrigin
    sint             ofsTagNames;   // numTags
    sint             ofsTags;   // numFrames * numTags
    sint             ofsSurfaces;   // first surface, others follow

    sint             ofsEnd;        // end of file
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

typedef struct {
    sint             boneIndex; // these are indexes into the boneReferences,
    float32           boneWeight;   // not the global per-frame bone list
    vec3_t          offset;
} mdsWeight_t;

typedef struct {
    vec3_t          normal;
    vec2_t          texCoords;
    sint             numWeights;
    sint             fixedParent;   // stay equi-distant from this parent
    float32           fixedDist;
    mdsWeight_t     weights[1]; // variable sized
} mdsVertex_t;

typedef struct {
    sint             indexes[3];
} mdsTriangle_t;

typedef struct {
    sint             ident;

    valueType            name[64];  // polyset name
    valueType            shader[64];
    sint             shaderIndex;   // for in-game use

    sint             minLod;

    sint             ofsHeader; // this will be a negative number

    sint             numVerts;
    sint             ofsVerts;

    sint             numTriangles;
    sint             ofsTriangles;

    sint             ofsCollapseMap;    // numVerts * sint

    // Bone references are a set of ints representing all the bones
    // present in any vertex weights for this surface.  This is
    // needed because a model may have surfaces that need to be
    // drawn at different sort times, and we don't want to have
    // to re-interpolate all the bones for each surface.
    sint             numBoneReferences;
    sint             ofsBoneReferences;

    sint             ofsEnd;        // next surface follows
} mdsSurface_t;

typedef struct {
    //float32     angles[3];
    //float32     ofsAngles[2];
    schar16
    angles[4];    // to be converted to axis at run-time (this is also better for lerping)
    schar16
    ofsAngles[2]; // PITCH/YAW, head in this direction from parent to go to the offset position
} mdsBoneFrameCompressed_t;

// NOTE: this only used at run-time
typedef struct {
    float32           matrix[3][3]; // 3x3 rotation
    vec3_t          translation;    // translation vector
} mdsBoneFrame_t;

typedef struct {
    vec3_t
    bounds[2];  // bounds of all surfaces of all LOD's for this frame
    vec3_t          localOrigin;    // midpoint of bounds, used for sphere cull
    float32           radius;       // dist from localOrigin to corner
    vec3_t
    parentOffset;   // one bone is an ascendant of all other bones, it starts the hierachy at this position
    mdsBoneFrameCompressed_t bones[1];  // [numBones]
} mdsFrame_t;

typedef struct {
    sint             numSurfaces;
    sint             ofsSurfaces;   // first surface, others follow
    sint             ofsEnd;        // next lod follows
} mdsLOD_t;

typedef struct {
    valueType            name[64];  // name of tag
    float32           torsoWeight;
    sint             boneIndex; // our index in the bones
} mdsTag_t;

#define BONEFLAG_TAG        1   // this bone is actually a tag

typedef struct {
    valueType            name[64];  // name of bone
    sint
    parent;        // not sure if this is required, no harm throwing it in
    float32
    torsoWeight;  // scale torso rotation about torsoParent by this
    float32           parentDist;
    sint             flags;
} mdsBoneInfo_t;

typedef struct {
    sint             ident;
    sint             version;

    valueType            name[64];  // model name

    float32           lodScale;
    float32           lodBias;

    // frames and bones are shared by all levels of detail
    sint             numFrames;
    sint             numBones;
    sint             ofsFrames; // md4Frame_t[numFrames]
    sint             ofsBones;  // mdsBoneInfo_t[numBones]
    sint
    torsoParent;   // index of bone that is the parent of the torso

    sint             numSurfaces;
    sint             ofsSurfaces;

    // tag data
    sint             numTags;
    sint             ofsTags;   // mdsTag_t[numTags]

    sint             ofsEnd;        // end of file
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

typedef struct {
    sint             boneIndex; // these are indexes into the boneReferences,
    float32           boneWeight;   // not the global per-frame bone list
    vec3_t          offset;
} mdmWeight_t;

typedef struct {
    vec3_t          normal;
    vec2_t          texCoords;
    sint             numWeights;
    mdmWeight_t     weights[1]; // variable sized
} mdmVertex_t;

typedef struct {
    sint             indexes[3];
} mdmTriangle_t;

typedef struct {
    sint             ident;

    valueType            name[64];  // polyset name
    valueType            shader[64];
    sint             shaderIndex;   // for in-game use

    sint             minLod;

    sint             ofsHeader; // this will be a negative number

    sint             numVerts;
    sint             ofsVerts;

    sint             numTriangles;
    sint             ofsTriangles;

    sint             ofsCollapseMap;    // numVerts * sint

    // Bone references are a set of ints representing all the bones
    // present in any vertex weights for this surface.  This is
    // needed because a model may have surfaces that need to be
    // drawn at different sort times, and we don't want to have
    // to re-interpolate all the bones for each surface.
    sint             numBoneReferences;
    sint             ofsBoneReferences;

    sint             ofsEnd;        // next surface follows
} mdmSurface_t;

/*typedef struct {
    vec3_t      bounds[2];          // bounds of all surfaces of all LOD's for this frame
    vec3_t      localOrigin;        // midpoint of bounds, used for sphere cull
    float32     radius;             // dist from localOrigin to corner
    vec3_t      parentOffset;       // one bone is an ascendant of all other bones, it starts the hierachy at this position
} mdmFrame_t;*/

typedef struct {
    sint             numSurfaces;
    sint             ofsSurfaces;   // first surface, others follow
    sint             ofsEnd;        // next lod follows
} mdmLOD_t;

/*typedef struct {
    valueType       name[64];   // name of tag
    float32     torsoWeight;
    sint            boneIndex;          // our index in the bones

    sint            numBoneReferences;
    sint            ofsBoneReferences;

    sint            ofsEnd;             // next tag follows
} mdmTag_t;*/

// Tags always only have one parent bone
typedef struct {
    valueType            name[64];  // name of tag
    vec3_t          axis[3];

    sint             boneIndex;
    vec3_t          offset;

    sint             numBoneReferences;
    sint             ofsBoneReferences;

    sint             ofsEnd;        // next tag follows
} mdmTag_t;

typedef struct {
    sint             ident;
    sint             version;

    valueType            name[64];  // model name
    /*  valueType       bonesfile[64];  // bone file

    #ifdef UTILS
        sint            skel;
    #else
        // dummy in file, set on load to link to MDX
        qhandle_t   skel;
    #endif // UTILS
    */
    float32           lodScale;
    float32           lodBias;

    // frames and bones are shared by all levels of detail
    /*  sint            numFrames;
        sint            ofsFrames;          // mdmFrame_t[numFrames]
    */
    sint             numSurfaces;
    sint             ofsSurfaces;

    // tag data
    sint             numTags;
    sint             ofsTags;

    sint             ofsEnd;        // end of file
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

typedef struct {
    vec3_t          bounds[2];  // bounds of this frame
    vec3_t          localOrigin;    // midpoint of bounds, used for sphere cull
    float32           radius;       // dist from localOrigin to corner
    vec3_t
    parentOffset;   // one bone is an ascendant of all other bones, it starts the hierachy at this position
} mdxFrame_t;

typedef struct {
    //float32     angles[3];
    //float32     ofsAngles[2];
    schar16
    angles[4];    // to be converted to axis at run-time (this is also better for lerping)
    schar16
    ofsAngles[2]; // PITCH/YAW, head in this direction from parent to go to the offset position
} mdxBoneFrameCompressed_t;

// NOTE: this only used at run-time
// FIXME: do we really need this?
typedef struct {
    float32           matrix[3][3]; // 3x3 rotation
    vec3_t          translation;    // translation vector
} mdxBoneFrame_t;

typedef struct {
    valueType            name[64];  // name of bone
    sint
    parent;        // not sure if this is required, no harm throwing it in
    float32
    torsoWeight;  // scale torso rotation about torsoParent by this
    float32           parentDist;
    sint             flags;
} mdxBoneInfo_t;

typedef struct {
    sint             ident;
    sint             version;

    valueType            name[64];  // model name

    // bones are shared by all levels of detail
    sint             numFrames;
    sint             numBones;
    sint
    ofsFrames; // (mdxFrame_t + mdxBoneFrameCompressed_t[numBones]) * numframes
    sint             ofsBones;  // mdxBoneInfo_t[numBones]
    sint
    torsoParent;   // index of bone that is the parent of the torso

    sint             ofsEnd;        // end of file
} mdxHeader_t;

/*
==============================================================================

  Wavefront .OBJ model file format

==============================================================================
*/

typedef struct objElement_s {
    float32 *vertex;
    float32 *uv;
    float32 *normal;
} objectElement_t;

typedef objectElement_t objectFace_t[3];

typedef struct objectSurface_s {
    sint ident;
    valueType shader[MAX_QPATH];
    sint shaderIndex;
    sint numFaces;
    objectFace_t *faces;
} objectSurface_t;

typedef struct objectModel_s {
    valueType name[MAX_QPATH];
    sint numVerts;
    sint numUVs;
    sint numNormals;
    sint numSurfaces;
    float32 *verts;
    float32 *UVs;
    float32 *normals;
    objectSurface_t *surfaces;
    vec3_t mins, maxs;
} objectModel_t;

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

#define MDR_IDENT   (('5'<<24)+('M'<<16)+('D'<<8)+'R')
#define MDR_VERSION 2
#define MDR_MAX_BONES   128

typedef struct {
    sint            boneIndex;  // these are indexes into the boneReferences,
    float32        boneWeight;      // not the global per-frame bone list
    vec3_t      offset;
} mdrWeight_t;

typedef struct {
    vec3_t      normal;
    vec2_t      texCoords;
    sint            numWeights;
    mdrWeight_t weights[1];     // variable sized
} mdrVertex_t;

typedef struct {
    sint            indexes[3];
} mdrTriangle_t;

typedef struct {
    sint            ident;

    valueType       name[MAX_QPATH];    // polyset name
    valueType       shader[MAX_QPATH];
    sint            shaderIndex;    // for in-game use

    sint            ofsHeader;  // this will be a negative number

    sint            numVerts;
    sint            ofsVerts;

    sint            numTriangles;
    sint            ofsTriangles;

    // Bone references are a set of ints representing all the bones
    // present in any vertex weights for this surface.  This is
    // needed because a model may have surfaces that need to be
    // drawn at different sort times, and we don't want to have
    // to re-interpolate all the bones for each surface.
    sint            numBoneReferences;
    sint            ofsBoneReferences;

    sint            ofsEnd;     // next surface follows
} mdrSurface_t;

typedef struct {
    float32     matrix[3][4];
} mdrBone_t;

typedef struct {
    vec3_t
    bounds[2];      // bounds of all surfaces of all LOD's for this frame
    vec3_t      localOrigin;        // midpoint of bounds, used for sphere cull
    float32     radius;         // dist from localOrigin to corner
    valueType       name[16];
    mdrBone_t   bones[1];       // [numBones]
} mdrFrame_t;

typedef struct {
    uchar8 Comp[24]; // MC_COMP_BYTES is in MatComp.h, but don't want to couple
} mdrCompBone_t;

typedef struct {
    vec3_t
    bounds[2];      // bounds of all surfaces of all LOD's for this frame
    vec3_t
    localOrigin;        // midpoint of bounds, used for sphere cull
    float32           radius;           // dist from localOrigin to corner
    mdrCompBone_t   bones[1];       // [numBones]
} mdrCompFrame_t;

typedef struct {
    sint            numSurfaces;
    sint            ofsSurfaces;        // first surface, others follow
    sint            ofsEnd;             // next lod follows
} mdrLOD_t;

typedef struct {
    sint                     boneIndex;
    valueType            name[32];
} mdrTag_t;

typedef struct {
    sint            ident;
    sint            version;

    valueType       name[MAX_QPATH];    // model name

    // frames and bones are shared by all levels of detail
    sint            numFrames;
    sint            numBones;
    sint            ofsFrames;          // mdrFrame_t[numFrames]

    // each level of detail has completely separate sets of surfaces
    sint            numLODs;
    sint            ofsLODs;

    sint                     numTags;
    sint                     ofsTags;

    sint            ofsEnd;             // end of file
} mdrHeader_t;

#endif //!!__QFILES_HPP__
