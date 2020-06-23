////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2020 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   MD5.cpp
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

idMD5SystemLocal MD5SystemLocal;
idMD5System* MD5System = &MD5SystemLocal;

/*
===============
idMD5SystemLocal::idMD5SystemLocal
===============
*/
idMD5SystemLocal::idMD5SystemLocal( void )
{
}

/*
===============
idMD5SystemLocal::~idMD5SystemLocal
===============
*/
idMD5SystemLocal::~idMD5SystemLocal( void )
{
}

/*
====================
idMD5SystemLocal::MD5File
====================
*/
valueType* idMD5SystemLocal::MD5File( pointer fn, sint length, pointer prefix, sint prefix_len )
{
    sint i, filelen = 0, r = 0, total = 0;
    static valueType final[33] = { "" };
    uchar8 digest[16] = { "" }, buffer[2048];
    fileHandle_t f;
    MD5_CTX md5;
    
    Q_strncpyz( final, "", sizeof( final ) );
    
    filelen = fileSystem->SV_FOpenFileRead( fn, &f );
    
    if( !f )
    {
        return final;
    }
    
    if( filelen < 1 )
    {
        fileSystem->FCloseFile( f );
        return final;
    }
    
    if( filelen < length || !length )
    {
        length = filelen;
    }
    
    MD5_Init( &md5 );
    
    if( prefix_len && *prefix )
    {
        MD5_Update( &md5, ( uchar8* )prefix, prefix_len );
    }
    
    for( ;; )
    {
        r = fileSystem->Read( buffer, sizeof( buffer ), f );
        if( r < 1 )
        {
            break;
        }
        
        if( r + total > length )
        {
            r = length - total;
        }
        
        total += r;
        
        MD5_Update( &md5, buffer, r );
        
        if( r < sizeof( buffer ) || total >= length )
        {
            break;
        }
    }
    
    fileSystem->FCloseFile( f );
    
    MD5_Final( digest, &md5 );
    
    final[0] = '\0';
    
    for( i = 0; i < 16; i++ )
    {
        Q_strcat( final, sizeof( final ), va( "%02X", digest[i] ) );
    }
    
    return final;
}