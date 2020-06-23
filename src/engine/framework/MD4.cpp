////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 - 2020 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   md4.cpp
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.h>
#elif DEDICATED
#include <null/null_serverprecompiled.h>
#else
#include <framework/precompiled.h>
#endif

idMD4SystemLocal MD4SystemLocal;
idMD4System* MD4System = &MD4SystemLocal;

/*
===============
idMD4SystemLocal::idMD4SystemLocal
===============
*/
idMD4SystemLocal::idMD4SystemLocal( void )
{
}

/*
===============
idMD4SystemLocal::~idMD4SystemLocal
===============
*/
idMD4SystemLocal::~idMD4SystemLocal( void )
{
}

/*
===============
idMD4SystemLocal::BlockChecksum
===============
*/
uint idMD4SystemLocal::BlockChecksum( const void* buffer, sint length )
{
    sint digest[4];
    uint val;
    MD4_CTX		ctx;
    
    MD4_Init( &ctx );
    MD4_Update( &ctx, ( uchar8* )buffer, length );
    MD4_Final( ( uchar8* )digest, &ctx );
    
    val = digest[0] ^ digest[1] ^ digest[2] ^ digest[3];
    
    return val;
}

