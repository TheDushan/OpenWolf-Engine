////////////////////////////////////////////////////////////////////////////////////////
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
// File name:   iqm.h
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2015
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __IQM_H__
#define __IQM_H__

#define IQM_MAGIC "INTERQUAKEMODEL"
#define IQM_VERSION 2

#define	IQM_MAX_JOINTS		128

typedef struct iqmheader
{
    UTF8 magic[16];
    U32 version;
    U32 filesize;
    U32 flags;
    U32 num_text, ofs_text;
    U32 num_meshes, ofs_meshes;
    U32 num_vertexarrays, num_vertexes, ofs_vertexarrays;
    U32 num_triangles, ofs_triangles, ofs_adjacency;
    U32 num_joints, ofs_joints;
    U32 num_poses, ofs_poses;
    U32 num_anims, ofs_anims;
    U32 num_frames, num_framechannels, ofs_frames, ofs_bounds;
    U32 num_comment, ofs_comment;
    U32 num_extensions, ofs_extensions;
} iqmHeader_t;

typedef struct iqmmesh
{
    U32 name;
    U32 material;
    U32 first_vertex, num_vertexes;
    U32 first_triangle, num_triangles;
} iqmMesh_t;

enum
{
    IQM_POSITION     = 0,
    IQM_TEXCOORD     = 1,
    IQM_NORMAL       = 2,
    IQM_TANGENT      = 3,
    IQM_BLENDINDEXES = 4,
    IQM_BLENDWEIGHTS = 5,
    IQM_COLOR        = 6,
    IQM_CUSTOM       = 0x10
};

enum
{
    IQM_BYTE   = 0,
    IQM_UBYTE  = 1,
    IQM_SHORT  = 2,
    IQM_USHORT = 3,
    IQM_INT    = 4,
    IQM_UINT   = 5,
    IQM_HALF   = 6,
    IQM_FLOAT  = 7,
    IQM_DOUBLE = 8,
};

typedef struct iqmtriangle
{
    U32 vertex[3];
} iqmTriangle_t;

typedef struct iqmjoint
{
    U32 name;
    S32 parent;
    F32 translate[3], rotate[4], scale[3];
} iqmJoint_t;

typedef struct iqmpose
{
    S32 parent;
    U32 mask;
    F32 channeloffset[10];
    F32 channelscale[10];
} iqmPose_t;

typedef struct iqmanim
{
    U32 name;
    U32 first_frame, num_frames;
    F32 framerate;
    U32 flags;
} iqmAnim_t;

enum
{
    IQM_LOOP = 1 << 0
};

typedef struct iqmvertexarray
{
    U32 type;
    U32 flags;
    U32 format;
    U32 size;
    U32 offset;
} iqmVertexArray_t;

typedef struct iqmbounds
{
    F32 bbmin[3], bbmax[3];
    F32 xyradius, radius;
} iqmBounds_t;

#endif
