////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   q_math.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: stateless support routines that are included in each code module
//              Some of the vector functions are static inline in q_shared.hpp. q3asm
//              doesn't understand static functions though, so we only want them in
//              one file. That's what this is about.
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.hpp>
#elif DEDICATED
#include <null/null_serverprecompiled.hpp>
#elif defined GUI
#include <GUI/gui_precompiled.hpp>
#elif CGAMEDLL
#include <cgame/cgame_precompiled.hpp>
#elif GAMEDLL
#include <sgame/sgame_precompiled.hpp>
#elif defined (OALAUDIO)
#include <API/soundSystem_api.hpp>
#else
#include <framework/precompiled.hpp>
#endif // !GAMEDLL

// *INDENT-OFF*
vec3_t vec3_origin = {0, 0, 0};

vec3_t axisDefault[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

matrix_t matrixIdentity = {	1, 0, 0, 0,
                            0, 1, 0, 0,
                            0, 0, 1, 0,
                            0, 0, 0, 1
                          };

quat_t quatIdentity = { 0, 0, 0, 1 };


vec4_t colorBlack      =   {0, 0, 0, 1};
vec4_t colorRed        =   {1, 0, 0, 1};
vec4_t colorGreen      =   {0, 1, 0, 1};
vec4_t colorBlue       =   {0, 0, 1, 1};
vec4_t colorYellow     =   {1, 1, 0, 1};
vec4_t colorOrange     =   {1, 0.5, 0, 1};
vec4_t colorMagenta    =   {1, 0, 1, 1};
vec4_t colorCyan       =   {0, 1, 1, 1};
vec4_t colorWhite      =   {1, 1, 1, 1};
vec4_t colorLtGrey     =   {0.75, 0.75, 0.75, 1};
vec4_t colorMdGrey     =   {0.5, 0.5, 0.5, 1};
vec4_t colorDkGrey     =   {0.25, 0.25, 0.25, 1};
vec4_t colorMdRed      =   {0.5, 0, 0, 1};
vec4_t colorMdGreen    =   {0, 0.5, 0, 1};

vec4_t clrBrown =          {0.68f,         0.68f,          0.56f,          1.f};
vec4_t clrBrownDk =        {0.58f * 0.75f, 0.58f * 0.75f,  0.46f * 0.75f,  1.f};
vec4_t clrBrownLine =      {0.0525f,       0.05f,          0.025f,         0.2f};
vec4_t clrBrownLineFull =  {0.0525f,       0.05f,          0.025f,         1.f};

vec4_t g_color_table[MAX_CCODES] =
{
    { 0.00000f, 0.00000f, 0.00000f, 1.00000f },	//0
    { 1.00000f, 0.00000f, 0.00000f, 1.00000f },
    { 0.00000f, 1.00000f, 0.00000f, 1.00000f },
    { 1.00000f, 1.00000f, 0.00000f, 1.00000f },
    { 0.00000f, 0.00000f, 1.00000f, 1.00000f },
    { 0.00000f, 1.00000f, 1.00000f, 1.00000f },
    { 1.00000f, 0.00000f, 1.00000f, 1.00000f },
    { 1.00000f, 1.00000f, 1.00000f, 1.00000f },
    { 1.00000f, 0.50000f, 0.00000f, 1.00000f },
    { 0.60000f, 0.60000f, 1.00000f, 1.00000f },	//9
    { 1.00000f, 0.00000f, 0.00000f, 1.00000f },	//a
    { 1.00000f, 0.13239f, 0.00000f, 1.00000f },
    { 1.00000f, 0.26795f, 0.00000f, 1.00000f },
    { 1.00000f, 0.37829f, 0.00000f, 1.00000f },
    { 1.00000f, 0.50000f, 0.00000f, 1.00000f },
    { 1.00000f, 0.60633f, 0.00000f, 1.00000f },
    { 1.00000f, 0.73205f, 0.00000f, 1.00000f },
    { 1.00000f, 0.84990f, 0.00000f, 1.00000f },
    { 1.00000f, 1.00000f, 0.00000f, 1.00000f },
    { 0.86761f, 1.00000f, 0.00000f, 1.00000f },
    { 0.73205f, 1.00000f, 0.00000f, 1.00000f },
    { 0.62171f, 1.00000f, 0.00000f, 1.00000f },
    { 0.50000f, 1.00000f, 0.00000f, 1.00000f },
    { 0.39367f, 1.00000f, 0.00000f, 1.00000f },
    { 0.26795f, 1.00000f, 0.00000f, 1.00000f },
    { 0.15010f, 1.00000f, 0.00000f, 1.00000f },
    { 0.00000f, 1.00000f, 0.00000f, 1.00000f },
    { 0.00000f, 1.00000f, 0.13239f, 1.00000f },
    { 0.00000f, 1.00000f, 0.26795f, 1.00000f },
    { 0.00000f, 1.00000f, 0.37829f, 1.00000f },
    { 0.00000f, 1.00000f, 0.50000f, 1.00000f },
    { 0.00000f, 1.00000f, 0.60633f, 1.00000f },
    { 0.00000f, 1.00000f, 0.73205f, 1.00000f },
    { 0.00000f, 1.00000f, 0.84990f, 1.00000f },
    { 0.00000f, 1.00000f, 1.00000f, 1.00000f },
    { 0.00000f, 0.86761f, 1.00000f, 1.00000f },
    { 0.00000f, 0.73205f, 1.00000f, 1.00000f },	//A
    { 0.00000f, 0.62171f, 1.00000f, 1.00000f },
    { 0.00000f, 0.50000f, 1.00000f, 1.00000f },
    { 0.00000f, 0.39367f, 1.00000f, 1.00000f },
    { 0.00000f, 0.26795f, 1.00000f, 1.00000f },
    { 0.00000f, 0.15010f, 1.00000f, 1.00000f },
    { 0.00000f, 0.00000f, 1.00000f, 1.00000f },
    { 0.13239f, 0.00000f, 1.00000f, 1.00000f },
    { 0.26795f, 0.00000f, 1.00000f, 1.00000f },
    { 0.37829f, 0.00000f, 1.00000f, 1.00000f },
    { 0.50000f, 0.00000f, 1.00000f, 1.00000f },
    { 0.60633f, 0.00000f, 1.00000f, 1.00000f },
    { 0.73205f, 0.00000f, 1.00000f, 1.00000f },
    { 0.84990f, 0.00000f, 1.00000f, 1.00000f },
    { 1.00000f, 0.00000f, 1.00000f, 1.00000f },
    { 1.00000f, 0.00000f, 0.86761f, 1.00000f },
    { 1.00000f, 0.00000f, 0.73205f, 1.00000f },
    { 1.00000f, 0.00000f, 0.62171f, 1.00000f },
    { 1.00000f, 0.00000f, 0.50000f, 1.00000f },
    { 1.00000f, 0.00000f, 0.39367f, 1.00000f },
    { 1.00000f, 0.00000f, 0.26795f, 1.00000f },
    { 1.00000f, 0.00000f, 0.15010f, 1.00000f },
    { 0.75000f, 0.75000f, 0.75000f, 1.00000f },
    { 0.50000f, 0.50000f, 0.50000f, 1.00000f },
    { 0.25000f, 0.25000f, 0.25000f, 1.00000f },
    { 1.00000f, 0.50000f, 1.00000f, 1.00000f },
};

vec3_t	bytedirs[NUMVERTEXNORMALS] =
{
    { -0.525731f, 0.000000f, 0.850651f }, { -0.442863f, 0.238856f, 0.864188f },
    { -0.295242f, 0.000000f, 0.955423f }, { -0.309017f, 0.500000f, 0.809017f },
    { -0.162460f, 0.262866f, 0.951056f }, { 0.000000f, 0.000000f, 1.000000f },
    { 0.000000f, 0.850651f, 0.525731f }, { -0.147621f, 0.716567f, 0.681718f },
    { 0.147621f, 0.716567f, 0.681718f }, { 0.000000f, 0.525731f, 0.850651f },
    { 0.309017f, 0.500000f, 0.809017f }, { 0.525731f, 0.000000f, 0.850651f },
    { 0.295242f, 0.000000f, 0.955423f }, { 0.442863f, 0.238856f, 0.864188f },
    { 0.162460f, 0.262866f, 0.951056f }, { -0.681718f, 0.147621f, 0.716567f },
    { -0.809017f, 0.309017f, 0.500000f }, { -0.587785f, 0.425325f, 0.688191f },
    { -0.850651f, 0.525731f, 0.000000f }, { -0.864188f, 0.442863f, 0.238856f },
    { -0.716567f, 0.681718f, 0.147621f }, { -0.688191f, 0.587785f, 0.425325f },
    { -0.500000f, 0.809017f, 0.309017f }, { -0.238856f, 0.864188f, 0.442863f },
    { -0.425325f, 0.688191f, 0.587785f }, { -0.716567f, 0.681718f, -0.147621f },
    { -0.500000f, 0.809017f, -0.309017f }, { -0.525731f, 0.850651f, 0.000000f },
    { 0.000000f, 0.850651f, -0.525731f }, { -0.238856f, 0.864188f, -0.442863f },
    { 0.000000f, 0.955423f, -0.295242f }, { -0.262866f, 0.951056f, -0.162460f },
    { 0.000000f, 1.000000f, 0.000000f }, { 0.000000f, 0.955423f, 0.295242f },
    { -0.262866f, 0.951056f, 0.162460f }, { 0.238856f, 0.864188f, 0.442863f },
    { 0.262866f, 0.951056f, 0.162460f }, { 0.500000f, 0.809017f, 0.309017f },
    { 0.238856f, 0.864188f, -0.442863f }, { 0.262866f, 0.951056f, -0.162460f },
    { 0.500000f, 0.809017f, -0.309017f }, { 0.850651f, 0.525731f, 0.000000f },
    { 0.716567f, 0.681718f, 0.147621f }, { 0.716567f, 0.681718f, -0.147621f },
    { 0.525731f, 0.850651f, 0.000000f }, { 0.425325f, 0.688191f, 0.587785f },
    { 0.864188f, 0.442863f, 0.238856f }, { 0.688191f, 0.587785f, 0.425325f },
    { 0.809017f, 0.309017f, 0.500000f }, { 0.681718f, 0.147621f, 0.716567f },
    { 0.587785f, 0.425325f, 0.688191f }, { 0.955423f, 0.295242f, 0.000000f },
    { 1.000000f, 0.000000f, 0.000000f }, { 0.951056f, 0.162460f, 0.262866f },
    { 0.850651f, -0.525731f, 0.000000f }, { 0.955423f, -0.295242f, 0.000000f },
    { 0.864188f, -0.442863f, 0.238856f }, { 0.951056f, -0.162460f, 0.262866f },
    { 0.809017f, -0.309017f, 0.500000f }, { 0.681718f, -0.147621f, 0.716567f },
    { 0.850651f, 0.000000f, 0.525731f }, { 0.864188f, 0.442863f, -0.238856f },
    { 0.809017f, 0.309017f, -0.500000f }, { 0.951056f, 0.162460f, -0.262866f },
    { 0.525731f, 0.000000f, -0.850651f }, { 0.681718f, 0.147621f, -0.716567f },
    { 0.681718f, -0.147621f, -0.716567f }, { 0.850651f, 0.000000f, -0.525731f },
    { 0.809017f, -0.309017f, -0.500000f }, { 0.864188f, -0.442863f, -0.238856f },
    { 0.951056f, -0.162460f, -0.262866f }, { 0.147621f, 0.716567f, -0.681718f },
    { 0.309017f, 0.500000f, -0.809017f }, { 0.425325f, 0.688191f, -0.587785f },
    { 0.442863f, 0.238856f, -0.864188f }, { 0.587785f, 0.425325f, -0.688191f },
    { 0.688191f, 0.587785f, -0.425325f }, { -0.147621f, 0.716567f, -0.681718f },
    { -0.309017f, 0.500000f, -0.809017f }, { 0.000000f, 0.525731f, -0.850651f },
    { -0.525731f, 0.000000f, -0.850651f }, { -0.442863f, 0.238856f, -0.864188f },
    { -0.295242f, 0.000000f, -0.955423f }, { -0.162460f, 0.262866f, -0.951056f },
    { 0.000000f, 0.000000f, -1.000000f }, { 0.295242f, 0.000000f, -0.955423f },
    { 0.162460f, 0.262866f, -0.951056f }, { -0.442863f, -0.238856f, -0.864188f },
    { -0.309017f, -0.500000f, -0.809017f }, { -0.162460f, -0.262866f, -0.951056f },
    { 0.000000f, -0.850651f, -0.525731f }, { -0.147621f, -0.716567f, -0.681718f },
    { 0.147621f, -0.716567f, -0.681718f }, { 0.000000f, -0.525731f, -0.850651f },
    { 0.309017f, -0.500000f, -0.809017f }, { 0.442863f, -0.238856f, -0.864188f },
    { 0.162460f, -0.262866f, -0.951056f }, { 0.238856f, -0.864188f, -0.442863f },
    { 0.500000f, -0.809017f, -0.309017f }, { 0.425325f, -0.688191f, -0.587785f },
    { 0.716567f, -0.681718f, -0.147621f }, { 0.688191f, -0.587785f, -0.425325f },
    { 0.587785f, -0.425325f, -0.688191f }, { 0.000000f, -0.955423f, -0.295242f },
    { 0.000000f, -1.000000f, 0.000000f }, { 0.262866f, -0.951056f, -0.162460f },
    { 0.000000f, -0.850651f, 0.525731f }, { 0.000000f, -0.955423f, 0.295242f },
    { 0.238856f, -0.864188f, 0.442863f }, { 0.262866f, -0.951056f, 0.162460f },
    { 0.500000f, -0.809017f, 0.309017f }, { 0.716567f, -0.681718f, 0.147621f },
    { 0.525731f, -0.850651f, 0.000000f }, { -0.238856f, -0.864188f, -0.442863f },
    { -0.500000f, -0.809017f, -0.309017f }, { -0.262866f, -0.951056f, -0.162460f },
    { -0.850651f, -0.525731f, 0.000000f }, { -0.716567f, -0.681718f, -0.147621f },
    { -0.716567f, -0.681718f, 0.147621f }, { -0.525731f, -0.850651f, 0.000000f },
    { -0.500000f, -0.809017f, 0.309017f }, { -0.238856f, -0.864188f, 0.442863f },
    { -0.262866f, -0.951056f, 0.162460f }, { -0.864188f, -0.442863f, 0.238856f },
    { -0.809017f, -0.309017f, 0.500000f }, { -0.688191f, -0.587785f, 0.425325f },
    { -0.681718f, -0.147621f, 0.716567f }, { -0.442863f, -0.238856f, 0.864188f },
    { -0.587785f, -0.425325f, 0.688191f }, { -0.309017f, -0.500000f, 0.809017f },
    { -0.147621f, -0.716567f, 0.681718f }, { -0.425325f, -0.688191f, 0.587785f },
    { -0.162460f, -0.262866f, 0.951056f }, { 0.442863f, -0.238856f, 0.864188f },
    { 0.162460f, -0.262866f, 0.951056f }, { 0.309017f, -0.500000f, 0.809017f },
    { 0.147621f, -0.716567f, 0.681718f }, { 0.000000f, -0.525731f, 0.850651f },
    { 0.425325f, -0.688191f, 0.587785f }, { 0.587785f, -0.425325f, 0.688191f },
    { 0.688191f, -0.587785f, 0.425325f }, { -0.955423f, 0.295242f, 0.000000f },
    { -0.951056f, 0.162460f, 0.262866f }, { -1.000000f, 0.000000f, 0.000000f },
    { -0.850651f, 0.000000f, 0.525731f }, { -0.955423f, -0.295242f, 0.000000f },
    { -0.951056f, -0.162460f, 0.262866f }, { -0.864188f, 0.442863f, -0.238856f },
    { -0.951056f, 0.162460f, -0.262866f }, { -0.809017f, 0.309017f, -0.500000f },
    { -0.864188f, -0.442863f, -0.238856f }, { -0.951056f, -0.162460f, -0.262866f },
    { -0.809017f, -0.309017f, -0.500000f }, { -0.681718f, 0.147621f, -0.716567f },
    { -0.681718f, -0.147621f, -0.716567f }, { -0.850651f, 0.000000f, -0.525731f },
    { -0.688191f, 0.587785f, -0.425325f }, { -0.587785f, 0.425325f, -0.688191f },
    { -0.425325f, 0.688191f, -0.587785f }, { -0.425325f, -0.688191f, -0.587785f },
    { -0.587785f, -0.425325f, -0.688191f }, { -0.688191f, -0.587785f, -0.425325f }
};

//==============================================================

float32 Q_random( sint* seed )
{
    return ( ( rand() & 0x7FFF ) / ( static_cast<float32>( 0x8000 ) ) );
}

float32 Q_crandom( sint* seed )
{
    return ( 2.0f * ( ( ( rand() & 0x7FFF ) / ( static_cast<float32>( 0x7FFF ) ) ) - 0.5f ) );
}


//=======================================================

uchar8 ClampByte( sint i )
{
    if( i < 0 )
    {
        return 0;
    }
    if( i > 255 )
    {
        return 255;
    }
    return i;
}

schar8 ClampChar( sint i )
{
    if( i < -128 )
    {
        return -128;
    }
    if( i > 127 )
    {
        return 127;
    }
    return i;
}

schar16 ClampShort( sint i )
{
    if( i < -32768 )
    {
        return -32768;
    }
    if( i > 0x7fff )
    {
        return 0x7fff;
    }
    return i;
}

// this isn't a real cheap function to call!
sint DirToByte( vec3_t dir )
{
    sint             i, best;
    float32           d, bestd;

    if( !dir )
    {
        return 0;
    }

    bestd = 0;
    best = 0;
    for( i = 0; i < NUMVERTEXNORMALS; i++ )
    {
        d = DotProduct( dir, bytedirs[i] );
        if( d > bestd )
        {
            bestd = d;
            best = i;
        }
    }

    return best;
}

void ByteToDir( sint b, vec3_t dir )
{
    if( b < 0 || b >= NUMVERTEXNORMALS )
    {
        VectorCopy( vec3_origin, dir );
        return;
    }
    VectorCopy( bytedirs[b], dir );
}

uint ColorBytes4( float32 r, float32 g, float32 b, float32 a )
{
    uint        i;

    ( reinterpret_cast<uchar8*>( & i ) )[0] = static_cast<uchar8>( r * 255 );
    ( reinterpret_cast<uchar8*>( & i ) )[1] = static_cast<uchar8>( g * 255 );
    ( reinterpret_cast<uchar8*>( & i ) )[2] = static_cast<uchar8>( b * 255 );
    ( reinterpret_cast<uchar8*>( & i ) )[3] = static_cast<uchar8>( a * 255 );

    return i;
}

float32 NormalizeColor( const vec3_t in, vec3_t out )
{
    float32           max;

    max = in[0];
    if( in[1] > max )
    {
        max = in[1];
    }
    if( in[2] > max )
    {
        max = in[2];
    }

    if( !max )
    {
        VectorClear( out );
    }
    else
    {
        out[0] = in[0] / max;
        out[1] = in[1] / max;
        out[2] = in[2] / max;
    }
    return max;
}

void ClampColor( vec4_t color )
{
    sint             i;

    for( i = 0; i < 4; i++ )
    {
        if( color[i] < 0 )
            color[i] = 0;

        if( color[i] > 1 )
            color[i] = 1;
    }
}

vec_t PlaneNormalize( vec4_t plane )
{
    vec_t           length, ilength;

    length = sqrt( plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2] );
    if( length == 0 )
    {
        VectorClear( plane );
        return 0;
    }

    ilength = 1.0f / length;
    plane[0] = plane[0] * ilength;
    plane[1] = plane[1] * ilength;
    plane[2] = plane[2] * ilength;
    plane[3] = plane[3] * ilength;

    return length;
}

/*
=====================
PlaneFromPoints

Returns false if the triangle is degenrate.
The normal will point out of the clock for clockwise ordered points
=====================
*/
bool PlaneFromPoints( vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c, bool cw )
{
    vec3_t          d1, d2;

    VectorSubtract( b, a, d1 );
    VectorSubtract( c, a, d2 );

    if( cw )
    {
        CrossProduct( d2, d1, plane );
    }
    else
    {
        CrossProduct( d1, d2, plane );
    }

    if( VectorNormalize( plane ) == 0 )
    {
        return false;
    }

    plane[3] = DotProduct( a, plane );
    return true;
}

/*
=====================
PlaneFromPoints

Returns false if the triangle is degenrate.
=====================
*/
bool PlaneFromPointsOrder( vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c, bool cw )
{
    vec3_t          d1, d2;

    VectorSubtract( b, a, d1 );
    VectorSubtract( c, a, d2 );

    if( cw )
    {
        CrossProduct( d2, d1, plane );
    }
    else
    {
        CrossProduct( d1, d2, plane );
    }

    if( VectorNormalize( plane ) == 0 )
    {
        return false;
    }

    plane[3] = DotProduct( a, plane );
    return true;
}

bool PlanesGetIntersectionPoint( const vec4_t plane1, const vec4_t plane2, const vec4_t plane3, vec3_t out )
{
    // http://www.cgafaq.info/wiki/Intersection_of_three_planes

    vec3_t	n1, n2, n3;
    vec3_t	n1n2, n2n3, n3n1;
    vec_t	denom;

    VectorNormalize2( plane1, n1 );
    VectorNormalize2( plane2, n2 );
    VectorNormalize2( plane3, n3 );

    CrossProduct( n1, n2, n1n2 );
    CrossProduct( n2, n3, n2n3 );
    CrossProduct( n3, n1, n3n1 );

    denom = DotProduct( n1, n2n3 );

    // check if the denominator is zero (which would mean that no intersection is to be found
    if( denom == 0 )
    {
        // no intersection could be found, return <0,0,0>
        VectorClear( out );
        return false;
    }

    VectorClear( out );

    VectorMA( out, plane1[3], n2n3, out );
    VectorMA( out, plane2[3], n3n1, out );
    VectorMA( out, plane3[3], n1n2, out );

    VectorScale( out, 1.0f / denom, out );

    return true;
}

void PlaneIntersectRay( const vec3_t rayPos, const vec3_t rayDir, const vec4_t plane, vec3_t res )
{
    vec3_t dir;
    float32 sect;
    float32 distToPlane;
    float32 planeDotRay;

    VectorNormalize2( rayDir, dir );

    distToPlane = DotProduct( plane, rayPos ) - plane[3];
    planeDotRay = DotProduct( plane, dir );
    sect = -( distToPlane ) / planeDotRay;

    VectorMA( rayPos, sect, dir, res );
}
/*
===============
RotatePointAroundVector

This is not implemented very well...
===============
*/
void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float32 degrees )
{
    float32           m[3][3];
    float32           im[3][3];
    float32           zrot[3][3];
    float32           tmpmat[3][3];
    float32           rot[3][3];
    sint             i;
    vec3_t          vr, vup, vf;
    float32           rad;

    vf[0] = dir[0];
    vf[1] = dir[1];
    vf[2] = dir[2];

    if( VectorNormalize( vf ) == 0 || degrees == 0.0f )
    {
        // degenerate case
        VectorCopy( point, dst );
        return;
    }

    PerpendicularVector( vr, dir );
    CrossProduct( vr, vf, vup );

    m[0][0] = vr[0];
    m[1][0] = vr[1];
    m[2][0] = vr[2];

    m[0][1] = vup[0];
    m[1][1] = vup[1];
    m[2][1] = vup[2];

    m[0][2] = vf[0];
    m[1][2] = vf[1];
    m[2][2] = vf[2];

    memcpy( im, m, sizeof( im ) );

    im[0][1] = m[1][0];
    im[0][2] = m[2][0];
    im[1][0] = m[0][1];
    im[1][2] = m[2][1];
    im[2][0] = m[0][2];
    im[2][1] = m[1][2];

    ::memset( zrot, 0, sizeof( zrot ) );
    zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0F;

    rad = DEG2RAD( degrees );
    zrot[0][0] = cos( rad );
    zrot[0][1] = sin( rad );
    zrot[1][0] = -sin( rad );
    zrot[1][1] = cos( rad );

    AxisMultiply( m, zrot, tmpmat );
    AxisMultiply( tmpmat, im, rot );

    for( i = 0; i < 3; i++ )
    {
        dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
    }
}

/*
===============
RotatePointArountVertex

Rotate a point around a vertex
===============
*/
void RotatePointAroundVertex( vec3_t pnt, float32 rot_x, float32 rot_y, float32 rot_z, const vec3_t origin )
{
    float32           tmp[11];

    //float32 rad_x, rad_y, rad_z;

    /*rad_x = DEG2RAD( rot_x );
       rad_y = DEG2RAD( rot_y );
       rad_z = DEG2RAD( rot_z ); */

    // move pnt to rel{0,0,0}
    VectorSubtract( pnt, origin, pnt );

    // init temp values
    tmp[0] = sin( rot_x );
    tmp[1] = cos( rot_x );
    tmp[2] = sin( rot_y );
    tmp[3] = cos( rot_y );
    tmp[4] = sin( rot_z );
    tmp[5] = cos( rot_z );
    tmp[6] = pnt[1] * tmp[5];
    tmp[7] = pnt[0] * tmp[4];
    tmp[8] = pnt[0] * tmp[5];
    tmp[9] = pnt[1] * tmp[4];
    tmp[10] = pnt[2] * tmp[3];

    // rotate point
    pnt[0] = ( tmp[3] * ( tmp[8] - tmp[9] ) + pnt[3] * tmp[2] );
    pnt[1] = ( tmp[0] * ( tmp[2] * tmp[8] - tmp[2] * tmp[9] - tmp[10] ) + tmp[1] * ( tmp[7] + tmp[6] ) );
    pnt[2] = ( tmp[1] * ( -tmp[2] * tmp[8] + tmp[2] * tmp[9] + tmp[10] ) + tmp[0] * ( tmp[7] + tmp[6] ) );

    // move pnt back
    VectorAdd( pnt, origin, pnt );
}

/*
===============
RotateAroundDirection
===============
*/
void RotateAroundDirection( vec3_t axis[3], float32 yaw )
{

    // create an arbitrary axis[1]
    PerpendicularVector( axis[1], axis[0] );

    // rotate it around axis[0] by yaw
    if( yaw )
    {
        vec3_t          temp;

        VectorCopy( axis[1], temp );
        RotatePointAroundVector( axis[1], axis[0], temp, yaw );
    }

    // cross to get axis[2]
    CrossProduct( axis[0], axis[1], axis[2] );
}

/*
================
Q_isnan

Don't pass doubles to this
================
*/
sint Q_isnan( float32 x )
{
    union
    {
        float32 f;
        uint i;
    } t;

    t.f = x;
    t.i &= 0x7FFFFFFF;
    t.i = 0x7F800000 - t.i;

    return static_cast<sint>( static_cast<uint>( t.i ) >> 31 );
}

void vectoangles( const vec3_t value1, vec3_t angles )
{
    float32           forward;
    float32           yaw, pitch;

    if( value1[1] == 0 && value1[0] == 0 )
    {
        yaw = 0;
        if( value1[2] > 0 )
        {
            pitch = 90.0f;
        }
        else
        {
            pitch = 270.0f;
        }
    }
    else
    {
        if( value1[0] )
        {
            yaw = ( atan2f( value1[1], value1[0] ) * 180.0f / M_PI );
        }
        else if( value1[1] > 0 )
        {
            yaw = 90.0f;
        }
        else
        {
            yaw = 270.0f;
        }
        if( yaw < 0 )
        {
            yaw += 360.0f;
        }

        forward = sqrt( value1[0] * value1[0] + value1[1] * value1[1] );
        pitch = ( atan2f( value1[2], forward ) * 180.0f / M_PI );
        if( pitch < 0 )
        {
            pitch += 360.0f;
        }
    }

    angles[PITCH] = -pitch;
    angles[YAW] = yaw;
    angles[ROLL] = 0;
}


/*
=================
AnglesToAxis
=================
*/
void AnglesToAxis( const vec3_t angles, vec3_t axis[3] )
{
    vec3_t right;

    // angle vectors returns "right" instead of "y axis"
    AngleVectors( angles, axis[0], right, axis[2] );
    VectorSubtract( vec3_origin, right, axis[1] );
}

void AxisClear( vec3_t axis[3] )
{
    axis[0][0] = 1;
    axis[0][1] = 0;
    axis[0][2] = 0;
    axis[1][0] = 0;
    axis[1][1] = 1;
    axis[1][2] = 0;
    axis[2][0] = 0;
    axis[2][1] = 0;
    axis[2][2] = 1;
}

void AxisCopy( vec3_t in[3], vec3_t out[3] )
{
    VectorCopy( in[0], out[0] );
    VectorCopy( in[1], out[1] );
    VectorCopy( in[2], out[2] );
}

void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal )
{
    float32           d;
    vec3_t          n;
    float32           inv_denom;

    inv_denom = 1.0F / DotProduct( normal, normal );
    d = DotProduct( normal, p ) * inv_denom;

    n[0] = normal[0] * inv_denom;
    n[1] = normal[1] * inv_denom;
    n[2] = normal[2] * inv_denom;

    dst[0] = p[0] - d * n[0];
    dst[1] = p[1] - d * n[1];
    dst[2] = p[2] - d * n[2];
}

/*
================
MakeNormalVectors

Given a normalized forward vector, create two
other perpendicular vectors
================
*/
void MakeNormalVectors( const vec3_t forward, vec3_t right, vec3_t up )
{
    float32           d;

    // this rotate and negate guarantees a vector
    // not colinear with the original
    right[1] = -forward[0];
    right[2] = forward[1];
    right[0] = forward[2];

    d = DotProduct( right, forward );
    VectorMA( right, -d, forward, right );
    VectorNormalize( right );
    CrossProduct( right, forward, up );
}


void VectorRotate( vec3_t in, vec3_t matrix[3], vec3_t out )
{
    out[0] = DotProduct( in, matrix[0] );
    out[1] = DotProduct( in, matrix[1] );
    out[2] = DotProduct( in, matrix[2] );
}

/*
===============
LerpAngle
===============
*/
float32 LerpAngle( float32 from, float32 to, float32 frac )
{
    if( to - from > 180 )
    {
        to -= 360;
    }
    if( to - from < -180 )
    {
        to += 360;
    }

    return ( from + frac * ( to - from ) );
}

/*
=================
LerpPosition

=================
*/

void LerpPosition( vec3_t start, vec3_t end, float32 frac, vec3_t out )
{
    vec3_t          dist;

    VectorSubtract( end, start, dist );
    VectorMA( start, frac, dist, out );
}

/*
=================
AngleSubtract

Always returns a value from -180 to 180
=================
*/
float32 AngleSubtract( float32 a1, float32 a2 )
{
    float32           a = a1 - a2;

    while( a > 180 )
    {
        a -= 360;
    }
    while( a < -180 )
    {
        a += 360;
    }
    return a;
}


void AnglesSubtract( vec3_t v1, vec3_t v2, vec3_t v3 )
{
    v3[0] = AngleSubtract( v1[0], v2[0] );
    v3[1] = AngleSubtract( v1[1], v2[1] );
    v3[2] = AngleSubtract( v1[2], v2[2] );
}


float32 AngleMod( float32 a )
{
    return ( ( 360.0f / 65536 ) * ( static_cast<sint>( a * ( 65536 / 360.0f ) ) & 65535 ) );
}

/*
=================
AngleNormalize2Pi

returns angle normalized to the range [0 <= angle < 2*M_PI]
=================
*/
float32 AngleNormalize2Pi( float32 angle )
{
    return DEG2RAD( AngleNormalize360( RAD2DEG( angle ) ) );
}

/*
=================
AngleNormalize360

returns angle normalized to the range [0 <= angle < 360]
=================
*/
float32 AngleNormalize360( float32 angle )
{
    return ( 360.0f / 65536 ) * ( static_cast<sint>( angle * ( 65536 / 360.0f ) ) & 65535 );
}


/*
=================
AngleNormalize180

returns angle normalized to the range [-180 < angle <= 180]
=================
*/
float32 AngleNormalize180( float32 angle )
{
    angle = AngleNormalize360( angle );
    if( angle > 180.0f )
    {
        angle -= 360.0f;
    }
    return angle;
}


/*
=================
AngleDelta

returns the normalized delta from angle1 to angle2
=================
*/
float32 AngleDelta( float32 angle1, float32 angle2 )
{
    return AngleNormalize180( angle1 - angle2 );
}


/*
=================
AngleBetweenVectors

returns the angle between two vectors normalized to the range [0 <= angle <= 180]
=================
*/
float32 AngleBetweenVectors( const vec3_t a, const vec3_t b )
{
    vec_t           alen, blen;

    alen = VectorLength( a );
    blen = VectorLength( b );

    if( !alen || !blen )
        return 0;

    // complete dot product of two vectors a, b is |a| * |b| * cos(angle)
    // this results in:
    //
    // angle = acos( (a * b) / (|a| * |b|) )
    return RAD2DEG( Q_acos( DotProduct( a, b ) / ( alen * blen ) ) );
}


//============================================================

/*
=================
SetPlaneSignbits
=================
*/
void SetPlaneSignbits( cplane_t* out )
{
    sint             bits, j;

    // for fast box on planeside test
    bits = 0;
    for( j = 0; j < 3; j++ )
    {
        if( out->normal[j] < 0 )
        {
            bits |= 1 << j;
        }
    }
    out->signbits = bits;
}

/*
=================
RadiusFromBounds
=================
*/
float32 RadiusFromBounds( const vec3_t mins, const vec3_t maxs )
{
    sint             i;
    vec3_t          corner;
    float32           a, b;

    for( i = 0; i < 3; i++ )
    {
        a = Q_fabs( mins[i] );
        b = Q_fabs( maxs[i] );
        corner[i] = a > b ? a : b;
    }

    return VectorLength( corner );
}

void ZeroBounds( vec3_t mins, vec3_t maxs )
{
    mins[0] = mins[1] = mins[2] = 0;
    maxs[0] = maxs[1] = maxs[2] = 0;
}

void ClearBounds( vec3_t mins, vec3_t maxs )
{
    mins[0] = mins[1] = mins[2] = 99999;
    maxs[0] = maxs[1] = maxs[2] = -99999;
}

void AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs )
{
    if( v[0] < mins[0] )
    {
        mins[0] = v[0];
    }
    if( v[0] > maxs[0] )
    {
        maxs[0] = v[0];
    }

    if( v[1] < mins[1] )
    {
        mins[1] = v[1];
    }
    if( v[1] > maxs[1] )
    {
        maxs[1] = v[1];
    }

    if( v[2] < mins[2] )
    {
        mins[2] = v[2];
    }
    if( v[2] > maxs[2] )
    {
        maxs[2] = v[2];
    }
}

bool PointInBounds( const vec3_t v, const vec3_t mins, const vec3_t maxs )
{
    if( v[0] < mins[0] )
    {
        return false;
    }
    if( v[0] > maxs[0] )
    {
        return false;
    }

    if( v[1] < mins[1] )
    {
        return false;
    }
    if( v[1] > maxs[1] )
    {
        return false;
    }

    if( v[2] < mins[2] )
    {
        return false;
    }
    if( v[2] > maxs[2] )
    {
        return false;
    }

    return true;
}

void BoundsAdd( vec3_t mins, vec3_t maxs, const vec3_t mins2, const vec3_t maxs2 )
{
    if( mins2[0] < mins[0] )
    {
        mins[0] = mins2[0];
    }
    if( mins2[1] < mins[1] )
    {
        mins[1] = mins2[1];
    }
    if( mins2[2] < mins[2] )
    {
        mins[2] = mins2[2];
    }

    if( maxs2[0] > maxs[0] )
    {
        maxs[0] = maxs2[0];
    }
    if( maxs2[1] > maxs[1] )
    {
        maxs[1] = maxs2[1];
    }
    if( maxs2[2] > maxs[2] )
    {
        maxs[2] = maxs2[2];
    }
}

bool BoundsIntersect( const vec3_t mins, const vec3_t maxs, const vec3_t mins2, const vec3_t maxs2 )
{
    if( maxs[0] < mins2[0] ||
            maxs[1] < mins2[1] || maxs[2] < mins2[2] || mins[0] > maxs2[0] || mins[1] > maxs2[1] || mins[2] > maxs2[2] )
    {
        return false;
    }

    return true;
}

bool BoundsIntersectSphere( const vec3_t mins, const vec3_t maxs, const vec3_t origin, vec_t radius )
{
    if( origin[0] - radius > maxs[0] ||
            origin[0] + radius < mins[0] ||
            origin[1] - radius > maxs[1] ||
            origin[1] + radius < mins[1] || origin[2] - radius > maxs[2] || origin[2] + radius < mins[2] )
    {
        return false;
    }

    return true;
}

bool BoundsIntersectPoint( const vec3_t mins, const vec3_t maxs, const vec3_t origin )
{
    if( origin[0] > maxs[0] ||
            origin[0] < mins[0] || origin[1] > maxs[1] || origin[1] < mins[1] || origin[2] > maxs[2] || origin[2] < mins[2] )
    {
        return false;
    }

    return true;
}

sint VectorCompare( const vec3_t v1, const vec3_t v2 )
{
    if( v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2] )
    {
        return 0;
    }

    return 1;
}

vec_t VectorNormalize( vec3_t v )
{
    float32           length, ilength;

    length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    length = sqrt( length );

    if( length )
    {
        ilength = 1 / length;
        v[0] *= ilength;
        v[1] *= ilength;
        v[2] *= ilength;
    }

    return length;
}

static ID_INLINE float32 Q_rsqrtApprox( const float32 number )
{
    union
    {
        float32 f;
        sint i;
    } t;
    float32 y;
    float32 x2;
    const float32 threehalfs = 1.5F;

    x2 = number * 0.5F;
    t.f = number;
    /* what the fuck? */
    t.i = 0x5f3759df - ( t.i >> 1 );
    y = t.f;
    /* 1st iteration */
    y = y * ( threehalfs - ( x2 * y * y ) );
    /* 2nd iteration */
    y = y * ( threehalfs - ( x2 * y * y ) );
    return y;
}

//
// fast vector normalize routine that does not check to make sure
// that length != 0, nor does it return length
//
void VectorNormalizeFast( vec3_t v )
{
    const float32 ilength = Q_rsqrtApprox( DotProduct( v, v ) );
    v[0] *= ilength;
    v[1] *= ilength;
    v[2] *= ilength;
}

/* Used to compare floats when rounding errors could occur  */
#ifndef EQUAL
#define EQUAL(a,b) (fabsf((a)-(b))<0.0000000001f)
#endif

vec_t VectorNormalize2( const vec3_t v, vec3_t out )
{
    float32 length;

    length = DotProduct( v, v );
    length = sqrt( length );
    if( !EQUAL( length, 0.0f ) )
    {
        const float32 ilength = 1.0f / length;
        out[0] = v[0] * ilength;
        out[1] = v[1] * ilength;
        out[2] = v[2] * ilength;
    }

    return length;
}

void _VectorMA( const vec3_t veca, float32 scale, const vec3_t vecb, vec3_t vecc )
{
    vecc[0] = veca[0] + scale * vecb[0];
    vecc[1] = veca[1] + scale * vecb[1];
    vecc[2] = veca[2] + scale * vecb[2];
}


vec_t _DotProduct( const vec3_t v1, const vec3_t v2 )
{
    return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

void _VectorSubtract( const vec3_t veca, const vec3_t vecb, vec3_t out )
{
    out[0] = veca[0] - vecb[0];
    out[1] = veca[1] - vecb[1];
    out[2] = veca[2] - vecb[2];
}

void _VectorAdd( const vec3_t veca, const vec3_t vecb, vec3_t out )
{
    out[0] = veca[0] + vecb[0];
    out[1] = veca[1] + vecb[1];
    out[2] = veca[2] + vecb[2];
}

void _VectorCopy( const vec3_t in, vec3_t out )
{
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
}

void _VectorScale( const vec3_t in, vec_t scale, vec3_t out )
{
    out[0] = in[0] * scale;
    out[1] = in[1] * scale;
    out[2] = in[2] * scale;
}

void CrossProduct( const vec3_t v1, const vec3_t v2, vec3_t cross )
{
    cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
    cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
    cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

vec_t VectorLength( const vec3_t v )
{
    return sqrt( v[0] * v[0] + v[1] * v[1] + v[2] * v[2] );
}

vec_t VectorLengthSquared( const vec3_t v )
{
    return ( v[0] * v[0] + v[1] * v[1] + v[2] * v[2] );
}

vec_t Distance( const vec3_t p1, const vec3_t p2 )
{
    vec3_t          v;

    VectorSubtract( p2, p1, v );
    return VectorLength( v );
}

vec_t DistanceSquared( const vec3_t p1, const vec3_t p2 )
{
    vec3_t          v;

    VectorSubtract( p2, p1, v );
    return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}

void VectorInverse( vec3_t v )
{
    v[0] = -v[0];
    v[1] = -v[1];
    v[2] = -v[2];
}

void Vector4Scale( const vec4_t in, vec_t scale, vec4_t out )
{
    out[0] = in[0] * scale;
    out[1] = in[1] * scale;
    out[2] = in[2] * scale;
    out[3] = in[3] * scale;
}

sint NearestPowerOfTwo( sint val )
{
    sint             answer;

    for( answer = 1; answer < val; answer <<= 1 )
        ;
    return answer;
}

sint Q_log2( sint val )
{
    sint             answer;

    answer = 0;
    while( ( val >>= 1 ) != 0 )
    {
        answer++;
    }
    return answer;
}

/*
=====================
Q_acos

the msvc acos doesn't always return a value between -PI and PI:

sint i;
i = 1065353246;
acos(*(float32*) &i) == -1.#IND0
=====================
*/
float32 Q_acos( float32 c )
{
    float32           angle;

    angle = acos( c );

    if( angle > M_PI )
    {
        return static_cast<float32>( M_PI );
    }
    if( angle < -M_PI )
    {
        return static_cast<float32>( M_PI );
    }
    return angle;
}

/*
=================
PlaneTypeForNormal
=================
*/
/*
sint	PlaneTypeForNormal (vec3_t normal) {
	if ( normal[0] == 1.0 )
		return PLANE_X;
	if ( normal[1] == 1.0 )
		return PLANE_Y;
	if ( normal[2] == 1.0 )
		return PLANE_Z;

	return PLANE_NON_AXIAL;
}
*/


/*
================
AxisMultiply
================
*/
void AxisMultiply( float32 in1[3][3], float32 in2[3][3], float32 out[3][3] )
{
    out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
    out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
    out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
    out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
    out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
    out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
    out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
    out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
    out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
}


void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up )
{
    float32           angle;
    static float32    sr, sp, sy, cr, cp, cy;

    // static to help MS compiler fp bugs

    angle = angles[YAW] * ( M_PI * 2 / 360.0f );
    sy = sin( angle );
    cy = cos( angle );

    angle = angles[PITCH] * ( M_PI * 2 / 360.0f );
    sp = sin( angle );
    cp = cos( angle );

    angle = angles[ROLL] * ( M_PI * 2 / 360.0f );
    sr = sin( angle );
    cr = cos( angle );

    if( forward )
    {
        forward[0] = cp * cy;
        forward[1] = cp * sy;
        forward[2] = -sp;
    }
    if( right )
    {
        right[0] = ( -1 * sr * sp * cy + -1 * cr * -sy );
        right[1] = ( -1 * sr * sp * sy + -1 * cr * cy );
        right[2] = -1 * sr * cp;
    }
    if( up )
    {
        up[0] = ( cr * sp * cy + -sr * -sy );
        up[1] = ( cr * sp * sy + -sr * cy );
        up[2] = cr * cp;
    }
}

/*
=================
PerpendicularVector

assumes "src" is normalized
=================
*/
void PerpendicularVector( vec3_t dst, const vec3_t src )
{
    sint             pos;
    sint             i;
    float32           minelem = 1.0F;
    vec3_t          tempvec;

    /*
     ** find the smallest magnitude axially aligned vector
     */
    for( pos = 0, i = 0; i < 3; i++ )
    {
        if( Q_fabs( src[i] ) < minelem )
        {
            pos = i;
            minelem = Q_fabs( src[i] );
        }
    }
    tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
    tempvec[pos] = 1.0F;

    /*
     ** project the point onto the plane defined by src
     */
    ProjectPointOnPlane( dst, tempvec, src );

    /*
     ** normalize the result
     */
    VectorNormalize( dst );
}

// Ridah
/*
=================
GetPerpendicularViewVector

  Used to find an "up" vector for drawing a sprite so that it always faces the view as best as possible
=================
*/
void GetPerpendicularViewVector( const vec3_t point, const vec3_t p1, const vec3_t p2, vec3_t up )
{
    vec3_t          v1, v2;

    VectorSubtract( point, p1, v1 );
    VectorNormalize( v1 );

    VectorSubtract( point, p2, v2 );
    VectorNormalize( v2 );

    CrossProduct( v1, v2, up );
    VectorNormalize( up );
}

/*
================
ProjectPointOntoVector
================
*/
void ProjectPointOntoVector( vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj )
{
    vec3_t          pVec, vec;

    VectorSubtract( point, vStart, pVec );
    VectorSubtract( vEnd, vStart, vec );
    VectorNormalize( vec );
    // project onto the directional vector for this segment
    VectorMA( vStart, DotProduct( pVec, vec ), vec, vProj );
}

#define LINE_DISTANCE_EPSILON 1e-05f

/*
================
DistanceBetweenLineSegmentsSquared
Return the smallest distance between two line segments, squared
================
*/

vec_t DistanceBetweenLineSegmentsSquared( const vec3_t sP0, const vec3_t sP1,
        const vec3_t tP0, const vec3_t tP1, float32* s, float32* t )
{
    vec3_t          sMag, tMag, diff;
    float32           a, b, c, d, e;
    float32           D;
    float32           sN, sD;
    float32           tN, tD;
    vec3_t          separation;

    VectorSubtract( sP1, sP0, sMag );
    VectorSubtract( tP1, tP0, tMag );
    VectorSubtract( sP0, tP0, diff );
    a = DotProduct( sMag, sMag );
    b = DotProduct( sMag, tMag );
    c = DotProduct( tMag, tMag );
    d = DotProduct( sMag, diff );
    e = DotProduct( tMag, diff );
    sD = tD = D = a * c - b * b;

    if( D < LINE_DISTANCE_EPSILON )
    {
        // the lines are almost parallel
        sN = 0.0;				// force using point P0 on segment S1
        sD = 1.0;				// to prevent possible division by 0.0 later
        tN = e;
        tD = c;
    }
    else
    {
        // get the closest points on the infinite  lines
        sN = ( b * e - c * d );
        tN = ( a * e - b * d );

        if( sN < 0.0 )
        {
            // sN < 0 => the s=0 edge is visible
            sN = 0.0;
            tN = e;
            tD = c;
        }
        else if( sN > sD )
        {
            // sN > sD => the s=1 edge is visible
            sN = sD;
            tN = e + b;
            tD = c;
        }
    }

    if( tN < 0.0 )
    {
        // tN < 0 => the t=0 edge is visible
        tN = 0.0;

        // recompute sN for this edge
        if( -d < 0.0 )
            sN = 0.0;
        else if( -d > a )
            sN = sD;
        else
        {
            sN = -d;
            sD = a;
        }
    }
    else if( tN > tD )
    {
        // tN > tD => the t=1 edge is visible
        tN = tD;

        // recompute sN for this edge
        if( ( -d + b ) < 0.0 )
            sN = 0;
        else if( ( -d + b ) > a )
            sN = sD;
        else
        {
            sN = ( -d + b );
            sD = a;
        }
    }

    // finally do the division to get *s and *t
    *s = ( fabs( sN ) < LINE_DISTANCE_EPSILON ? 0.0f : sN / sD );
    *t = ( fabs( tN ) < LINE_DISTANCE_EPSILON ? 0.0f : tN / tD );

    // get the difference of the two closest points
    VectorScale( sMag, *s, sMag );
    VectorScale( tMag, *t, tMag );
    VectorAdd( diff, sMag, separation );
    VectorSubtract( separation, tMag, separation );

    return VectorLengthSquared( separation );

}

/*
================
DistanceBetweenLineSegments

Return the smallest distance between two line segments
================
*/

vec_t DistanceBetweenLineSegments( const vec3_t sP0, const vec3_t sP1, const vec3_t tP0, const vec3_t tP1, float32* s, float32* t )
{
    return ( vec_t ) sqrt( DistanceBetweenLineSegmentsSquared( sP0, sP1, tP0, tP1, s, t ) );
}

/*
================
ProjectPointOntoVectorBounded
================
*/
void ProjectPointOntoVectorBounded( vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj )
{
    vec3_t          pVec, vec;
    sint             j;

    VectorSubtract( point, vStart, pVec );
    VectorSubtract( vEnd, vStart, vec );
    VectorNormalize( vec );
    // project onto the directional vector for this segment
    VectorMA( vStart, DotProduct( pVec, vec ), vec, vProj );
    // check bounds
    for( j = 0; j < 3; j++ )
        if( ( vProj[j] > vStart[j] && vProj[j] > vEnd[j] ) || ( vProj[j] < vStart[j] && vProj[j] < vEnd[j] ) )
        {
            break;
        }
    if( j < 3 )
    {
        if( Q_fabs( vProj[j] - vStart[j] ) < Q_fabs( vProj[j] - vEnd[j] ) )
        {
            VectorCopy( vStart, vProj );
        }
        else
        {
            VectorCopy( vEnd, vProj );
        }
    }
}

/*
================
DistanceFromLineSquared
================
*/
float32 DistanceFromLineSquared( vec3_t p, vec3_t lp1, vec3_t lp2 )
{
    vec3_t          proj, t;
    sint             j;

    ProjectPointOntoVector( p, lp1, lp2, proj );
    for( j = 0; j < 3; j++ )
        if( ( proj[j] > lp1[j] && proj[j] > lp2[j] ) || ( proj[j] < lp1[j] && proj[j] < lp2[j] ) )
        {
            break;
        }
    if( j < 3 )
    {
        if( Q_fabs( proj[j] - lp1[j] ) < Q_fabs( proj[j] - lp2[j] ) )
        {
            VectorSubtract( p, lp1, t );
        }
        else
        {
            VectorSubtract( p, lp2, t );
        }
        return VectorLengthSquared( t );
    }
    VectorSubtract( p, proj, t );
    return VectorLengthSquared( t );
}

/*
================
DistanceFromVectorSquared
================
*/
float32 DistanceFromVectorSquared( vec3_t p, vec3_t lp1, vec3_t lp2 )
{
    vec3_t          proj, t;

    ProjectPointOntoVector( p, lp1, lp2, proj );
    VectorSubtract( p, proj, t );
    return VectorLengthSquared( t );
}

float32 vectoyaw( const vec3_t vec )
{
    float32           yaw;

    if( vec[YAW] == 0 && vec[PITCH] == 0 )
    {
        yaw = 0;
    }
    else
    {
        if( vec[PITCH] )
        {
            yaw = ( atan2( vec[YAW], vec[PITCH] ) * 180 / M_PI );
        }
        else if( vec[YAW] > 0 )
        {
            yaw = 90;
        }
        else
        {
            yaw = 270;
        }
        if( yaw < 0 )
        {
            yaw += 360;
        }
    }

    return yaw;
}

/*
=================
AxisToAngles

  Used to convert the MD3 tag axis to MDC tag angles, which are much smaller

  This doesn't have to be fast, since it's only used for conversion in utils, try to avoid
  using this during gameplay
=================
*/
void AxisToAngles( /*const*/ vec3_t axis[3], vec3_t angles )
{
    float32 length1;
    float32 yaw, pitch, roll = 0.0f;

    if( axis[0][1] == 0 && axis[0][0] == 0 )
    {
        yaw = 0;
        if( axis[0][2] > 0 )
        {
            pitch = 90;
        }
        else
        {
            pitch = 270;
        }
    }
    else
    {
        if( axis[0][0] )
        {
            yaw = ( atan2( axis[0][1], axis[0][0] ) * 180 / M_PI );
        }
        else if( axis[0][1] > 0 )
        {
            yaw = 90;
        }
        else
        {
            yaw = 270;
        }
        if( yaw < 0 )
        {
            yaw += 360;
        }

        length1 = sqrt( axis[0][0] * axis[0][0] + axis[0][1] * axis[0][1] );
        pitch = ( atan2( axis[0][2], length1 ) * 180 / M_PI );
        if( pitch < 0 )
        {
            pitch += 360;
        }

        roll = ( atan2( axis[1][2], axis[2][2] ) * 180 / M_PI );
        if( roll < 0 )
        {
            roll += 360;
        }
    }

    angles[PITCH] = -pitch;
    angles[YAW] = yaw;
    angles[ROLL] = roll;
}

float32 VectorDistance( vec3_t v1, vec3_t v2 )
{
    vec3_t          dir;

    VectorSubtract( v2, v1, dir );
    return VectorLength( dir );
}

float32 VectorDistanceSquared( vec3_t v1, vec3_t v2 )
{
    vec3_t          dir;

    VectorSubtract( v2, v1, dir );
    return VectorLengthSquared( dir );
}

// done.
/*
================
VectorMaxComponent

Return the biggest component of some vector
================
*/
float32 VectorMaxComponent( vec3_t v )
{
    float32 biggest = v[ 0 ];

    if( v[ 1 ] > biggest )
        biggest = v[ 1 ];

    if( v[ 2 ] > biggest )
        biggest = v[ 2 ];

    return biggest;
}

/*
================
VectorMinComponent

Return the smallest component of some vector
================
*/
float32 VectorMinComponent( vec3_t v )
{
    float32 smallest = v[ 0 ];

    if( v[ 1 ] < smallest )
        smallest = v[ 1 ];

    if( v[ 2 ] < smallest )
        smallest = v[ 2 ];

    return smallest;
}

void MatrixFromAngles( matrix_t m, vec_t pitch, vec_t yaw, vec_t roll )
{
    static float32    sr, sp, sy, cr, cp, cy;

    // static to help MS compiler fp bugs
    sp = sin( DEG2RAD( pitch ) );
    cp = cos( DEG2RAD( pitch ) );

    sy = sin( DEG2RAD( yaw ) );
    cy = cos( DEG2RAD( yaw ) );

    sr = sin( DEG2RAD( roll ) );
    cr = cos( DEG2RAD( roll ) );

    m[ 0] = cp * cy;
    m[ 4] = ( sr * sp * cy + cr * -sy );
    m[ 8] = ( cr * sp * cy + -sr * -sy );
    m[12] = 0;
    m[ 1] = cp * sy;
    m[ 5] = ( sr * sp * sy + cr * cy );
    m[ 9] = ( cr * sp * sy + -sr * cy );
    m[13] = 0;
    m[ 2] = -sp;
    m[ 6] = sr * cp;
    m[10] = cr * cp;
    m[14] = 0;
    m[ 3] = 0;
    m[ 7] = 0;
    m[11] = 0;
    m[15] = 1;
}

void MatrixSetupTransformFromRotation( matrix_t m, const matrix_t rot, const vec3_t origin )
{
    m[ 0] = rot[ 0];
    m[ 4] = rot[ 4];
    m[ 8] = rot[ 8];
    m[12] = origin[0];
    m[ 1] = rot[ 1];
    m[ 5] = rot[ 5];
    m[ 9] = rot[ 9];
    m[13] = origin[1];
    m[ 2] = rot[ 2];
    m[ 6] = rot[ 6];
    m[10] = rot[10];
    m[14] = origin[2];
    m[ 3] = 0;
    m[ 7] = 0;
    m[11] = 0;
    m[15] = 1;
}

void MatrixAffineInverse( const matrix_t in, matrix_t out )
{
#if 0
    MatrixCopy( in, out );
    MatrixInverse( out );
#else
    // Tr3B - cleaned up
    out[ 0] = in[ 0];
    out[ 4] = in[ 1];
    out[ 8] = in[ 2];
    out[ 1] = in[ 4];
    out[ 5] = in[ 5];
    out[ 9] = in[ 6];
    out[ 2] = in[ 8];
    out[ 6] = in[ 9];
    out[10] = in[10];
    out[ 3] = 0;
    out[ 7] = 0;
    out[11] = 0;
    out[15] = 1;

    out[12] = -( in[12] * out[ 0] + in[13] * out[ 4] + in[14] * out[ 8] );
    out[13] = -( in[12] * out[ 1] + in[13] * out[ 5] + in[14] * out[ 9] );
    out[14] = -( in[12] * out[ 2] + in[13] * out[ 6] + in[14] * out[10] );
#endif
}

void MatrixTransformNormal( const matrix_t m, const vec3_t in, vec3_t out )
{
    out[ 0] = m[ 0] * in[ 0] + m[ 4] * in[ 1] + m[ 8] * in[ 2];
    out[ 1] = m[ 1] * in[ 0] + m[ 5] * in[ 1] + m[ 9] * in[ 2];
    out[ 2] = m[ 2] * in[ 0] + m[ 6] * in[ 1] + m[10] * in[ 2];
}

void MatrixTransformNormal2( const matrix_t m, vec3_t inout )
{
    vec3_t          tmp;

    tmp[ 0] = m[ 0] * inout[ 0] + m[ 4] * inout[ 1] + m[ 8] * inout[ 2];
    tmp[ 1] = m[ 1] * inout[ 0] + m[ 5] * inout[ 1] + m[ 9] * inout[ 2];
    tmp[ 2] = m[ 2] * inout[ 0] + m[ 6] * inout[ 1] + m[10] * inout[ 2];

    VectorCopy( tmp, inout );
}

void MatrixTransformPoint( const matrix_t m, const vec3_t in, vec3_t out )
{
    out[ 0] = m[ 0] * in[ 0] + m[ 4] * in[ 1] + m[ 8] * in[ 2] + m[12];
    out[ 1] = m[ 1] * in[ 0] + m[ 5] * in[ 1] + m[ 9] * in[ 2] + m[13];
    out[ 2] = m[ 2] * in[ 0] + m[ 6] * in[ 1] + m[10] * in[ 2] + m[14];
}
