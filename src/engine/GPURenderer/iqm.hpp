////////////////////////////////////////////////////////////////////////////////////////
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
// File name:   iqm.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
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
    valueType magic[16];
    uint version;
    uint filesize;
    uint flags;
    uint num_text, ofs_text;
    uint num_meshes, ofs_meshes;
    uint num_vertexarrays, num_vertexes, ofs_vertexarrays;
    uint num_triangles, ofs_triangles, ofs_adjacency;
    uint num_joints, ofs_joints;
    uint num_poses, ofs_poses;
    uint num_anims, ofs_anims;
    uint num_frames, num_framechannels, ofs_frames, ofs_bounds;
    uint num_comment, ofs_comment;
    uint num_extensions, ofs_extensions;
} iqmHeader_t;

typedef struct iqmmesh
{
    uint name;
    uint material;
    uint first_vertex, num_vertexes;
    uint first_triangle, num_triangles;
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
    uint vertex[3];
} iqmTriangle_t;

typedef struct iqmjoint
{
    uint name;
    sint parent;
    float32 translate[3], rotate[4], scale[3];
} iqmJoint_t;

typedef struct iqmpose
{
    sint parent;
    uint mask;
    float32 channeloffset[10];
    float32 channelscale[10];
} iqmPose_t;

typedef struct iqmanim
{
    uint name;
    uint first_frame, num_frames;
    float32 framerate;
    uint flags;
} iqmAnim_t;

enum
{
    IQM_LOOP = 1 << 0
};

typedef struct iqmvertexarray
{
    uint type;
    uint flags;
    uint format;
    uint size;
    uint offset;
} iqmVertexArray_t;

typedef struct iqmbounds
{
    float32 bbmin[3], bbmax[3];
    float32 xyradius, radius;
} iqmBounds_t;

#endif
