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
// File name:   s_codec_wav.cpp
// Version:     v1.02
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.h>

/*
=================
FGetLittleLong
=================
*/
static S32 FGetLittleLong( fileHandle_t f )
{
    S32 v;
    
    fileSystem->Read( &v, sizeof( v ), f );
    
    return LittleLong( v );
}

/*
=================
FGetLittleShort
=================
*/
static S16 FGetLittleShort( fileHandle_t f )
{
    S16	v;
    
    fileSystem->Read( &v, sizeof( v ), f );
    
    return LittleShort( v );
}

/*
=================
S_ReadChunkInfo
=================
*/
static S32 S_ReadChunkInfo( fileHandle_t f, UTF8* name )
{
    S32 len, r;
    
    name[4] = 0;
    
    r = fileSystem->Read( name, 4, f );
    if( r != 4 )
    {
        return -1;
    }
    
    len = FGetLittleLong( f );
    if( len < 0 )
    {
        Com_Printf( S_COLOR_YELLOW "WARNING: Negative chunk length\n" );
        return -1;
    }
    
    return len;
}

/*
=================
S_FindRIFFChunk

Returns the length of the data in the chunk, or -1 if not found
=================
*/
static S32 S_FindRIFFChunk( fileHandle_t f, UTF8* chunk )
{
    UTF8 name[5];
    S32 len;
    
    while( ( len = S_ReadChunkInfo( f, name ) ) >= 0 )
    {
        // If this is the right chunk, return
        if( !Q_strncmp( name, chunk, 4 ) )
        {
            return len;
        }
        
        len = PAD( len, 2 );
        
        // Not the right chunk - skip it
        fileSystem->Seek( f, len, FS_SEEK_CUR );
    }
    
    return -1;
}

/*
=================
S_ByteSwapRawSamples
=================
*/
static void S_ByteSwapRawSamples( S32 samples, S32 width, S32 s_channels, const U8* data )
{
    S32 i;
    
    if( width != 2 )
    {
        return;
    }
    
    if( LittleShort( 256 ) == 256 )
    {
        return;
    }
    
    if( s_channels == 2 )
    {
        samples <<= 1;
    }
    
    for( i = 0 ; i < samples ; i++ )
    {
        ( ( S16* )data )[i] = LittleShort( ( ( S16* )data )[i] );
    }
}

/*
=================
S_ReadRIFFHeader
=================
*/
static bool S_ReadRIFFHeader( fileHandle_t file, snd_info_t* info )
{
    UTF8 dump[16];
    S32 bits;
    S32 fmtlen = 0;
    
    // skip the riff wav header
    fileSystem->Read( dump, 12, file );
    
    // Scan for the format chunk
    if( ( fmtlen = S_FindRIFFChunk( file, "fmt " ) ) < 0 )
    {
        Com_Printf( S_COLOR_RED "ERROR: Couldn't find \"fmt\" chunk\n" );
        return false;
    }
    
    // Save the parameters
    FGetLittleShort( file );
    info->channels = FGetLittleShort( file );
    info->rate = FGetLittleLong( file );
    FGetLittleLong( file );
    FGetLittleShort( file );
    bits = FGetLittleShort( file );
    
    if( bits < 8 )
    {
        Com_Printf( S_COLOR_RED "ERROR: Less than 8 bit sound is not supported\n" );
        return false;
    }
    
    info->width = bits / 8;
    info->dataofs = 0;
    
    // Skip the rest of the format chunk if required
    if( fmtlen > 16 )
    {
        fmtlen -= 16;
        fileSystem->Seek( file, fmtlen, FS_SEEK_CUR );
    }
    
    // Scan for the data chunk
    if( ( info->size = S_FindRIFFChunk( file, "data" ) ) < 0 )
    {
        Com_Printf( S_COLOR_RED "ERROR: Couldn't find \"data\" chunk\n" );
        return false;
    }
    
    info->samples = ( info->size / info->width ) / info->channels;
    
    return true;
}

// WAV codec
snd_codec_t wav_codec =
{
    ".wav",
    S_WAV_CodecLoad,
    S_WAV_CodecOpenStream,
    S_WAV_CodecReadStream,
    S_WAV_CodecCloseStream,
    nullptr
};

/*
=================
S_WAV_CodecLoad
=================
*/
void* S_WAV_CodecLoad( StringEntry filename, snd_info_t* info )
{
    fileHandle_t file;
    void* buffer;
    
    // Try to open the file
    fileSystem->FOpenFileRead( filename, &file, true );
    if( !file )
    {
        Com_Printf( S_COLOR_RED "ERROR: Could not open \"%s\"\n", filename );
        return nullptr;
    }
    
    // Read the RIFF header
    if( !S_ReadRIFFHeader( file, info ) )
    {
        fileSystem->FCloseFile( file );
        Com_Printf( S_COLOR_RED "ERROR: Incorrect/unsupported format in \"%s\"\n", filename );
        return nullptr;
    }
    
    // Allocate some memory
    buffer = Z_Malloc( info->size );
    if( !buffer )
    {
        fileSystem->FCloseFile( file );
        Com_Printf( S_COLOR_RED "ERROR: Out of memory reading \"%s\"\n", filename );
        return nullptr;
    }
    
    // Read, byteswap
    fileSystem->Read( buffer, info->size, file );
    S_ByteSwapRawSamples( info->samples, info->width, info->channels, static_cast< U8* >( buffer ) );
    
    // Close and return
    fileSystem->FCloseFile( file );
    return buffer;
}

/*
=================
S_WAV_CodecOpenStream
=================
*/
snd_stream_t* S_WAV_CodecOpenStream( StringEntry filename )
{
    snd_stream_t* rv;
    
    // Open
    rv = S_CodecUtilOpen( filename, &wav_codec );
    if( !rv )
    {
        return nullptr;
    }
    
    // Read the RIFF header
    if( !S_ReadRIFFHeader( rv->file, &rv->info ) )
    {
        S_CodecUtilClose( rv );
        return nullptr;
    }
    
    return rv;
}

/*
=================
S_WAV_CodecCloseStream
=================
*/
void S_WAV_CodecCloseStream( snd_stream_t* stream )
{
    S_CodecUtilClose( stream );
}

/*
=================
S_WAV_CodecReadStream
=================
*/
S32 S_WAV_CodecReadStream( snd_stream_t* stream, S32 bytes, void* buffer )
{
    S32 remaining = stream->info.size - stream->pos;
    S32 samples;
    
    if( remaining <= 0 )
    {
        return 0;
    }
    
    if( bytes > remaining )
    {
        bytes = remaining;
    }
    
    stream->pos += bytes;
    samples = ( bytes / stream->info.width ) / stream->info.channels;
    fileSystem->Read( buffer, bytes, stream->file );
    S_ByteSwapRawSamples( samples, stream->info.width, stream->info.channels, static_cast<const U8*>( buffer ) );
    
    return bytes;
}
