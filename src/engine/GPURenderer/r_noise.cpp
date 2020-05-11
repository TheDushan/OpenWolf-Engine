////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2005 Id Software, Inc.
// Copyright(C) 2000 - 2013 Darklegion Development
// Copyright(C) 2011 - 2018 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of OpenWolf.
//
// OpenWolf is free software; you can redistribute it
// and / or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the License,
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
// File name:   r_noise.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2017, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.h>

#define NOISE_SIZE 256
#define NOISE_MASK ( NOISE_SIZE - 1 )

#define VAL( a ) s_noise_perm[ ( a ) & ( NOISE_MASK )]
#define INDEX( x, y, z, t ) VAL( x + VAL( y + VAL( z + VAL( t ) ) ) )

static F32 s_noise_table[NOISE_SIZE];
static S32 s_noise_perm[NOISE_SIZE];

static F32 GetNoiseValue( S32 x, S32 y, S32 z, S32 t )
{
    S32 index = INDEX( ( S32 ) x, ( S32 ) y, ( S32 ) z, ( S32 ) t );
    
    return s_noise_table[index];
}

void R_NoiseInit( void )
{
    S32 i;
    
    for( i = 0; i < NOISE_SIZE; i++ )
    {
        s_noise_table[i] = ( F32 )( ( ( rand() / ( F32 ) RAND_MAX ) * 2.0 - 1.0 ) );
        s_noise_perm[i] = ( U8 )( rand() / ( F32 ) RAND_MAX * 255 );
    }
}

F32 R_NoiseGet4f( F32 x, F32 y, F32 z, F64 t )
{
    S32 i;
    S32 ix, iy, iz, it;
    F32 fx, fy, fz, ft;
    F32 front[4];
    F32 back[4];
    F32 fvalue, bvalue, value[2], finalvalue;
    
    ix = ( S32 ) floor( x );
    fx = x - ix;
    iy = ( S32 ) floor( y );
    fy = y - iy;
    iz = ( S32 ) floor( z );
    fz = z - iz;
    it = ( S32 ) floor( t );
    ft = t - it;
    
    for( i = 0; i < 2; i++ )
    {
        front[0] = GetNoiseValue( ix, iy, iz, it + i );
        front[1] = GetNoiseValue( ix + 1, iy, iz, it + i );
        front[2] = GetNoiseValue( ix, iy + 1, iz, it + i );
        front[3] = GetNoiseValue( ix + 1, iy + 1, iz, it + i );
        
        back[0] = GetNoiseValue( ix, iy, iz + 1, it + i );
        back[1] = GetNoiseValue( ix + 1, iy, iz + 1, it + i );
        back[2] = GetNoiseValue( ix, iy + 1, iz + 1, it + i );
        back[3] = GetNoiseValue( ix + 1, iy + 1, iz + 1, it + i );
        
        fvalue = LERP( LERP( front[0], front[1], fx ), LERP( front[2], front[3], fx ), fy );
        bvalue = LERP( LERP( back[0], back[1], fx ), LERP( back[2], back[3], fx ), fy );
        
        value[i] = LERP( fvalue, bvalue, fz );
    }
    
    finalvalue = LERP( value[0], value[1], ft );
    
    return finalvalue;
}
