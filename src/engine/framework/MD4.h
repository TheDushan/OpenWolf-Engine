////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
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
// File name:   md4.h
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __MD4_H__
#define __MD4_H__

struct mdfour
{
    U32 A, B, C, D;
    U32 totalN;
};

/* NOTE: This code makes no attempt to be fast!

It assumes that a S32 is at least 32 bits long
*/

static struct mdfour* m;

#define F(X,Y,Z) (((X)&(Y)) | ((~(X))&(Z)))
#define G(X,Y,Z) (((X)&(Y)) | ((X)&(Z)) | ((Y)&(Z)))
#define H(X,Y,Z) ((X)^(Y)^(Z))
#define lshift(x,s) (((x)<<(s)) | ((x)>>(32-(s))))

#define ROUND1(a,b,c,d,k,s) a = lshift(a + F(b,c,d) + X[k], s)
#define ROUND2(a,b,c,d,k,s) a = lshift(a + G(b,c,d) + X[k] + 0x5A827999,s)
#define ROUND3(a,b,c,d,k,s) a = lshift(a + H(b,c,d) + X[k] + 0x6ED9EBA1,s)

//
// idServerBotSystemLocal
//
class idMD4SystemLocal : public idMD4System
{
public:
    idMD4SystemLocal();
    ~idMD4SystemLocal();
    
    virtual U32 BlockChecksum( const void* buffer, S32 length );
    
public:
    static void mdfour64( U32* M );
    static void copy64( U32* M, U8* in );
    static void copy4( U8* out, U32 x );
    static void mdfour_tail( U8* in, S32 n );
    static void mdfour_update( struct mdfour* md, U8* in, S32 n );
    static void mdfour_result( struct mdfour* md, U8* out );
    static void mdfour_begin( struct mdfour* md );
    static void mdfour( U8* out, U8* in, S32 n );
    static void mdfour_hex( const U8 md4[16], S32 hex[32] );
};

extern idMD4SystemLocal MD4Local;

#endif //!__MD4_H__
