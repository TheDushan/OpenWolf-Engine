////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2005 Stuart Dalton(badcdev@gmail.com)
// Copyright(C) 2011 - 2019 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   s_codec.cpp
// Version:     v1.02
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.h>

static snd_codec_t* codecs;

/*
=================
S_FileExtension
=================
*/
static UTF8* S_FileExtension( StringEntry fni )
{
    // we should search from the ending to the last '/'
    UTF8* fn = const_cast<UTF8*>( fni + ::strlen( fni ) - 1 );
    UTF8* eptr = NULL;
    
    while( *fn != '/' && fn != fni )
    {
        if( *fn == '.' )
        {
            eptr = fn;
        }
        
        fn--;
    }
    
    return eptr;
}

/*
=================
S_FindCodecForFile

Select an appropriate codec for a file based on its extension
=================
*/
static snd_codec_t* S_FindCodecForFile( StringEntry filename )
{
    UTF8* ext = S_FileExtension( filename );
    snd_codec_t* codec = codecs;
    
    if( !ext )
    {
        // No extension - auto-detect
        while( codec )
        {
            UTF8 fn[MAX_QPATH];
            
            // there is no extension so we do not need to subtract 4 chars
            Q_strncpyz( fn, filename, MAX_QPATH );
            COM_DefaultExtension( fn, MAX_QPATH, codec->ext );
            
            // Check it exists
            if( fileSystem->ReadFile( fn, NULL ) != -1 )
            {
                return codec;
            }
            
            // Nope. Next!
            codec = codec->next;
        }
        
        // Nothin'
        return NULL;
    }
    
    while( codec )
    {
        if( !Q_stricmp( ext, codec->ext ) )
        {
            return codec;
        }
        codec = codec->next;
    }
    
    return NULL;
}

/*
=================
S_CodecInit
=================
*/
void S_CodecInit( void )
{
    codecs = NULL;
    S_CodecRegister( &wav_codec );
    S_CodecRegister( &ogg_codec );
}

/*
=================
S_CodecShutdown
=================
*/
void S_CodecShutdown( void )
{
    codecs = NULL;
}

/*
=================
S_CodecRegister
=================
*/
void S_CodecRegister( snd_codec_t* codec )
{
    codec->next = codecs;
    codecs = codec;
}

/*
=================
S_CodecLoad
=================
*/
void* S_CodecLoad( StringEntry filename, snd_info_t* info )
{
    snd_codec_t* codec;
    UTF8 fn[MAX_QPATH];
    
    codec = S_FindCodecForFile( filename );
    if( !codec )
    {
        Com_Printf( "Unknown extension for %s\n", filename );
        return NULL;
    }
    
    ::strncpy( fn, filename, sizeof( fn ) );
    COM_DefaultExtension( fn, sizeof( fn ), codec->ext );
    
    return codec->load( fn, info );
}

/*
=================
S_CodecOpenStream
=================
*/
snd_stream_t* S_CodecOpenStream( StringEntry filename )
{
    snd_codec_t* codec;
    UTF8 fn[MAX_QPATH];
    
    codec = S_FindCodecForFile( filename );
    if( !codec )
    {
        Com_Printf( "Unknown extension for %s\n", filename );
        return NULL;
    }
    
    ::strncpy( fn, filename, sizeof( fn ) );
    COM_DefaultExtension( fn, sizeof( fn ), codec->ext );
    
    return codec->open( fn );
}

void S_CodecCloseStream( snd_stream_t* stream )
{
    stream->codec->close( stream );
}

S32 S_CodecReadStream( snd_stream_t* stream, S32 bytes, void* buffer )
{
    return stream->codec->read( stream, bytes, buffer );
}

//=======================================================================
// Util functions (used by codecs)

/*
=================
S_CodecUtilOpen
=================
*/
snd_stream_t* S_CodecUtilOpen( StringEntry filename, snd_codec_t* codec )
{
    snd_stream_t* stream;
    fileHandle_t hnd;
    S32 length;
    
    // Try to open the file
    length = fileSystem->FOpenFileRead( filename, &hnd, true );
    if( !hnd )
    {
        Com_Printf( "Can't read sound file %s\n", filename );
        return NULL;
    }
    
    // Allocate a stream
    stream = static_cast<snd_stream_t*>( Z_Malloc( sizeof( snd_stream_t ) ) );
    if( !stream )
    {
        fileSystem->FCloseFile( hnd );
        return NULL;
    }
    
    // Copy over, return
    stream->codec = codec;
    stream->file = hnd;
    stream->length = length;
    return stream;
}

/*
=================
S_CodecUtilClose
=================
*/
void S_CodecUtilClose( snd_stream_t* stream )
{
    fileSystem->FCloseFile( stream->file );
    Z_Free( stream );
}
