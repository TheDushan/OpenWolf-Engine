////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2005 - 2006 Tim Angus
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
// File name:   clientAVI.cpp
// Version:     v1.02
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.h>

idClientAVISystemLocal clientAVILocal;
idClientAVISystemAPI* clientAVISystem = &clientAVILocal;

#define MAX_AVI_BUFFER 2048

static uchar8 buffer[MAX_AVI_BUFFER];
static sint bufIndex;

/*
===============
idClientAVISystemLocal::idClientAVISystemLocal
===============
*/
idClientAVISystemLocal::idClientAVISystemLocal( void )
{
}

/*
===============
idClientAVISystemLocal::~idClientAVISystemLocal
===============
*/
idClientAVISystemLocal::~idClientAVISystemLocal( void )
{
}

/*
===============
idClientAVISystemLocal::SafeFS_Write
===============
*/
void idClientAVISystemLocal::SafeFS_Write( const void* buffer, sint len, fileHandle_t f )
{
    if( fileSystem->Write( buffer, len, f ) < len )
    {
        Com_Error( ERR_DROP, "Failed to write avi file\n" );
    }
}

/*
===============
idClientAVISystemLocal::WRITE_STRING
===============
*/
void idClientAVISystemLocal::WRITE_STRING( pointer s )
{
    ::memcpy( &buffer[bufIndex], s, strlen( s ) );
    bufIndex += strlen( s );
}

/*
===============
idClientAVISystemLocal::WRITE_4BYTES
===============
*/
void idClientAVISystemLocal::WRITE_4BYTES( sint x )
{
    buffer[bufIndex + 0] = ( uchar8 )( ( x >> 0 ) & 0xFF );
    buffer[bufIndex + 1] = ( uchar8 )( ( x >> 8 ) & 0xFF );
    buffer[bufIndex + 2] = ( uchar8 )( ( x >> 16 ) & 0xFF );
    buffer[bufIndex + 3] = ( uchar8 )( ( x >> 24 ) & 0xFF );
    bufIndex += 4;
}

/*
===============
idClientAVISystemLocal::WRITE_2BYTES
===============
*/
void idClientAVISystemLocal::WRITE_2BYTES( sint x )
{
    buffer[bufIndex + 0] = ( uchar8 )( ( x >> 0 ) & 0xFF );
    buffer[bufIndex + 1] = ( uchar8 )( ( x >> 8 ) & 0xFF );
    bufIndex += 2;
}

/*
===============
idClientAVISystemLocal::WRITE_1BYTES
===============
*/
void idClientAVISystemLocal::WRITE_1BYTES( sint x )
{
    buffer[bufIndex] = x;
    bufIndex += 1;
}

/*
===============
idClientAVISystemLocal::START_CHUNK
===============
*/
void idClientAVISystemLocal::START_CHUNK( pointer s )
{
    if( afd.chunkStackTop == MAX_RIFF_CHUNKS )
    {
        Com_Error( ERR_DROP, "ERROR: Top of chunkstack breached\n" );
    }
    
    afd.chunkStack[afd.chunkStackTop] = bufIndex;
    afd.chunkStackTop++;
    WRITE_STRING( s );
    WRITE_4BYTES( 0 );
}

/*
===============
idClientAVISystemLocal::END_CHUNK
===============
*/
void idClientAVISystemLocal::END_CHUNK( void )
{
    sint endIndex = bufIndex;
    
    if( afd.chunkStackTop <= 0 )
    {
        Com_Error( ERR_DROP, "ERROR: Bottom of chunkstack breached\n" );
    }
    
    afd.chunkStackTop--;
    bufIndex = afd.chunkStack[afd.chunkStackTop];
    bufIndex += 4;
    WRITE_4BYTES( endIndex - bufIndex - 4 );
    bufIndex = endIndex;
    bufIndex = PAD( bufIndex, 2 );
}

/*
===============
idClientAVISystemLocal::WriteAVIHeader
===============
*/
void idClientAVISystemLocal::WriteAVIHeader( void )
{
    bufIndex = 0;
    afd.chunkStackTop = 0;
    
    START_CHUNK( "RIFF" );
    {
        WRITE_STRING( "AVI " );
        {
            START_CHUNK( "LIST" );
            {
                WRITE_STRING( "hdrl" );
                WRITE_STRING( "avih" );
                WRITE_4BYTES( 56 );	//"avih" "chunk" size
                WRITE_4BYTES( afd.framePeriod );	//dwMicroSecPerFrame
                WRITE_4BYTES( afd.maxRecordSize * afd.frameRate );	//dwMaxBytesPerSec
                WRITE_4BYTES( 0 );	//dwReserved1
                WRITE_4BYTES( 0x110 );	//dwFlags bits HAS_INDEX and IS_INTERLEAVED
                WRITE_4BYTES( afd.numVideoFrames );	//dwTotalFrames
                WRITE_4BYTES( 0 );	//dwInitialFrame
                
                if( afd.audio )	//dwStreams
                {
                    WRITE_4BYTES( 2 );
                }
                else
                {
                    WRITE_4BYTES( 1 );
                }
                
                WRITE_4BYTES( afd.maxRecordSize );	//dwSuggestedBufferSize
                WRITE_4BYTES( afd.width );	//dwWidth
                WRITE_4BYTES( afd.height );	//dwHeight
                WRITE_4BYTES( 0 );	//dwReserved[ 0 ]
                WRITE_4BYTES( 0 );	//dwReserved[ 1 ]
                WRITE_4BYTES( 0 );	//dwReserved[ 2 ]
                WRITE_4BYTES( 0 );	//dwReserved[ 3 ]
                
                START_CHUNK( "LIST" );
                {
                    WRITE_STRING( "strl" );
                    WRITE_STRING( "strh" );
                    WRITE_4BYTES( 56 );	//"strh" "chunk" size
                    WRITE_STRING( "vids" );
                    
                    if( afd.motionJpeg )
                    {
                        WRITE_STRING( "MJPG" );
                    }
                    else
                    {
                        WRITE_4BYTES( 0 );	// BI_RGB
                    }
                    
                    WRITE_4BYTES( 0 );	//dwFlags
                    WRITE_4BYTES( 0 );	//dwPriority
                    WRITE_4BYTES( 0 );	//dwInitialFrame
                    
                    WRITE_4BYTES( 1 );	//dwTimescale
                    WRITE_4BYTES( afd.frameRate );	//dwDataRate
                    WRITE_4BYTES( 0 );	//dwStartTime
                    WRITE_4BYTES( afd.numVideoFrames );	//dwDataLength
                    
                    WRITE_4BYTES( afd.maxRecordSize );	//dwSuggestedBufferSize
                    WRITE_4BYTES( -1 );	//dwQuality
                    WRITE_4BYTES( 0 );	//dwSampleSize
                    WRITE_2BYTES( 0 );	//rcFrame
                    WRITE_2BYTES( 0 );	//rcFrame
                    WRITE_2BYTES( afd.width );	//rcFrame
                    WRITE_2BYTES( afd.height );	//rcFrame
                    
                    WRITE_STRING( "strf" );
                    WRITE_4BYTES( 40 );	//"strf" "chunk" size
                    WRITE_4BYTES( 40 );	//biSize
                    WRITE_4BYTES( afd.width );	//biWidth
                    WRITE_4BYTES( afd.height );	//biHeight
                    WRITE_2BYTES( 1 );	//biPlanes
                    WRITE_2BYTES( 24 );	//biBitCount
                    
                    if( afd.motionJpeg )
                    {
                        //biCompression
                        WRITE_STRING( "MJPG" );
                        WRITE_4BYTES( afd.width * afd.height );	//biSizeImage
                    }
                    else
                    {
                        WRITE_4BYTES( 0 );	// BI_RGB
                        WRITE_4BYTES( afd.width * afd.height * 3 );	//biSizeImage
                    }
                    
                    WRITE_4BYTES( 0 );	//biXPelsPetMeter
                    WRITE_4BYTES( 0 );	//biYPelsPetMeter
                    WRITE_4BYTES( 0 );	//biClrUsed
                    WRITE_4BYTES( 0 );	//biClrImportant
                }
                END_CHUNK();
                
                if( afd.audio )
                {
                    START_CHUNK( "LIST" );
                    {
                        WRITE_STRING( "strl" );
                        WRITE_STRING( "strh" );
                        WRITE_4BYTES( 56 );	//"strh" "chunk" size
                        WRITE_STRING( "auds" );
                        WRITE_4BYTES( 0 );	//FCC
                        WRITE_4BYTES( 0 );	//dwFlags
                        WRITE_4BYTES( 0 );	//dwPriority
                        WRITE_4BYTES( 0 );	//dwInitialFrame
                        
                        WRITE_4BYTES( afd.a.sampleSize );	//dwTimescale
                        WRITE_4BYTES( afd.a.sampleSize * afd.a.rate );	//dwDataRate
                        WRITE_4BYTES( 0 );	//dwStartTime
                        WRITE_4BYTES( afd.a.totalBytes / afd.a.sampleSize );	//dwDataLength
                        
                        WRITE_4BYTES( 0 );	//dwSuggestedBufferSize
                        WRITE_4BYTES( -1 );	//dwQuality
                        WRITE_4BYTES( afd.a.sampleSize );	//dwSampleSize
                        WRITE_2BYTES( 0 );	//rcFrame
                        WRITE_2BYTES( 0 );	//rcFrame
                        WRITE_2BYTES( 0 );	//rcFrame
                        WRITE_2BYTES( 0 );	//rcFrame
                        
                        WRITE_STRING( "strf" );
                        WRITE_4BYTES( 18 );	//"strf" "chunk" size
                        WRITE_2BYTES( afd.a.format );	//wFormatTag
                        WRITE_2BYTES( afd.a.channels );	//nChannels
                        WRITE_4BYTES( afd.a.rate );	//nSamplesPerSec
                        WRITE_4BYTES( afd.a.sampleSize * afd.a.rate );	//nAvgBytesPerSec
                        WRITE_2BYTES( afd.a.sampleSize );	//nBlockAlign
                        WRITE_2BYTES( afd.a.bits );	//wBitsPerSample
                        WRITE_2BYTES( 0 );	//cbSize
                    }
                    END_CHUNK();
                }
            }
            END_CHUNK();
            
            afd.moviOffset = bufIndex;
            
            START_CHUNK( "LIST" );
            {
                WRITE_STRING( "movi" );
            }
        }
    }
}

/*
===============
idClientAVISystemLocal::OpenAVIForWriting

Creates an AVI file and gets it into a state where
writing the actual data can begin
===============
*/
bool idClientAVISystemLocal::OpenAVIForWriting( pointer fileName )
{
    if( afd.fileOpen )
    {
        return false;
    }
    
    ::memset( &afd, 0, sizeof( aviFileData_t ) );
    
    // Don't start if a framerate has not been chosen
    if( cl_aviFrameRate->integer <= 0 )
    {
        Com_Printf( S_COLOR_RED "cl_aviFrameRate must be >= 1\n" );
        return false;
    }
    
    if( ( afd.f = fileSystem->FOpenFileWrite( fileName ) ) <= 0 )
    {
        return false;
    }
    
    if( ( afd.idxF = fileSystem->FOpenFileWrite( va( "%s" INDEX_FILE_EXTENSION, fileName ) ) ) <= 0 )
    {
        fileSystem->FCloseFile( afd.f );
        return false;
    }
    
    Q_strncpyz( afd.fileName, fileName, MAX_QPATH );
    
    afd.frameRate = cl_aviFrameRate->integer;
    afd.framePeriod = ( sint )( 1000000.0f / afd.frameRate );
    afd.width = cls.glconfig.vidWidth;
    afd.height = cls.glconfig.vidHeight;
    
    if( cl_aviMotionJpeg->integer )
    {
        afd.motionJpeg = true;
    }
    else
    {
        afd.motionJpeg = false;
    }
    
    // Buffers only need to store RGB pixels.
    // Allocate a bit more space for the capture buffer to account for possible
    // padding at the end of pixel lines, and padding for alignment
#define MAX_PACK_LEN 16
    afd.cBuffer = ( uchar8* )Z_Malloc( ( afd.width * 3 + MAX_PACK_LEN - 1 ) * afd.height + MAX_PACK_LEN - 1 );
    // raw avi files have pixel lines start on 4-uchar8 boundaries
    afd.eBuffer = ( uchar8* )Z_Malloc( PAD( afd.width * 3, AVI_LINE_PADDING ) * afd.height );
    
    afd.a.rate = dma.speed;
    afd.a.format = WAV_FORMAT_PCM;
    afd.a.channels = dma.channels;
    afd.a.bits = dma.samplebits;
    afd.a.sampleSize = ( afd.a.bits / 8 ) * afd.a.channels;
    
    if( afd.a.rate % afd.frameRate )
    {
        sint suggestRate = afd.frameRate;
        
        while( ( afd.a.rate % suggestRate ) && suggestRate >= 1 )
        {
            suggestRate--;
        }
        
        Com_Printf( S_COLOR_YELLOW "WARNING: cl_aviFrameRate is not a divisor " "of the audio rate, suggest %d\n", suggestRate );
    }
    
    if( !cvarSystem->VariableIntegerValue( "s_initsound" ) )
    {
        afd.audio = false;
    }
    else if( Q_stricmp( cvarSystem->VariableString( "s_backend" ), "OpenAL" ) )
    {
        if( afd.a.bits == 16 && afd.a.channels == 2 )
        {
            afd.audio = true;
        }
        else
        {
            afd.audio = false;	//FIXME: audio not implemented for this case
        }
    }
    else
    {
        afd.audio = false;
        Com_Printf( S_COLOR_YELLOW "WARNING: Audio capture is not supported "
                    "with OpenAL. Set s_useOpenAL to 0 for audio capture\n" );
    }
    
    // This doesn't write a real header, but allocates the
    // correct amount of space at the beginning of the file
    WriteAVIHeader();
    
    SafeFS_Write( buffer, bufIndex, afd.f );
    afd.fileSize = bufIndex;
    
    bufIndex = 0;
    START_CHUNK( "idx1" );
    SafeFS_Write( buffer, bufIndex, afd.idxF );
    
    afd.moviSize = 4;			// For the "movi"
    afd.fileOpen = true;
    
    return true;
}

/*
===============
idClientAVISystemLocal::CheckFileSize
===============
*/
bool idClientAVISystemLocal::CheckFileSize( sint bytesToAdd )
{
    uint newFileSize;
    
    newFileSize = afd.fileSize +	        // Current file size
                  bytesToAdd +			    // What we want to add
                  ( afd.numIndices * 16 ) +	// The index
                  4;						// The index size
                  
    // I assume all the operating systems
    // we target can handle a 2Gb file
    if( newFileSize > INT_MAX )
    {
        // Close the current file...
        clientAVILocal.CloseAVI();
        
        // ...And open a new one
        clientAVILocal.OpenAVIForWriting( va( "%s_", afd.fileName ) );
        
        return true;
    }
    
    return false;
}

/*
===============
idClientAVISystemLocal::WriteAVIVideoFrame
===============
*/
void idClientAVISystemLocal::WriteAVIVideoFrame( const uchar8* imageBuffer, sint size )
{
    sint chunkOffset = afd.fileSize - afd.moviOffset - 8;
    sint chunkSize = 8 + size;
    sint paddingSize = PAD( size, 2 ) - size;
    uchar8 padding[4] = { 0 };
    
    if( !afd.fileOpen )
    {
        return;
    }
    
    // Chunk header + contents + padding
    if( CheckFileSize( 8 + size + 2 ) )
    {
        return;
    }
    
    bufIndex = 0;
    WRITE_STRING( "00dc" );
    WRITE_4BYTES( size );
    
    SafeFS_Write( buffer, 8, afd.f );
    SafeFS_Write( imageBuffer, size, afd.f );
    SafeFS_Write( padding, paddingSize, afd.f );
    afd.fileSize += ( chunkSize + paddingSize );
    
    afd.numVideoFrames++;
    afd.moviSize += ( chunkSize + paddingSize );
    
    if( size > afd.maxRecordSize )
    {
        afd.maxRecordSize = size;
    }
    
    // Index
    bufIndex = 0;
    WRITE_STRING( "00dc" );		//dwIdentifier
    WRITE_4BYTES( 0x00000010 );	//dwFlags (all frames are KeyFrames)
    WRITE_4BYTES( chunkOffset );	//dwOffset
    WRITE_4BYTES( size );			//dwLength
    SafeFS_Write( buffer, 16, afd.idxF );
    
    afd.numIndices++;
}

#define PCM_BUFFER_SIZE 44100

/*
===============
idClientAVISystemLocal::WriteAVIAudioFrame
===============
*/
void idClientAVISystemLocal::WriteAVIAudioFrame( const uchar8* pcmBuffer, sint size )
{
    static uchar8 pcmCaptureBuffer[PCM_BUFFER_SIZE] = { 0 };
    static sint bytesInBuffer = 0;
    
    if( !afd.audio )
    {
        return;
    }
    
    if( !afd.fileOpen )
    {
        return;
    }
    
    // Chunk header + contents + padding
    if( CheckFileSize( 8 + bytesInBuffer + size + 2 ) )
    {
        return;
    }
    
    if( bytesInBuffer + size > PCM_BUFFER_SIZE )
    {
        Com_Printf( S_COLOR_YELLOW "WARNING: Audio capture buffer overflow -- truncating\n" );
        size = PCM_BUFFER_SIZE - bytesInBuffer;
    }
    
    ::memcpy( &pcmCaptureBuffer[bytesInBuffer], pcmBuffer, size );
    bytesInBuffer += size;
    
    // Only write if we have a frame's worth of audio
    if( bytesInBuffer >= ( sint )ceil( ( float32 )afd.a.rate / ( float32 )afd.frameRate ) * afd.a.sampleSize )
    {
        sint chunkOffset = afd.fileSize - afd.moviOffset - 8;
        sint chunkSize = 8 + bytesInBuffer;
        sint paddingSize = PAD( bytesInBuffer, 2 ) - bytesInBuffer;
        uchar8 padding[4] = { 0 };
        
        bufIndex = 0;
        WRITE_STRING( "01wb" );
        WRITE_4BYTES( bytesInBuffer );
        
        SafeFS_Write( buffer, 8, afd.f );
        SafeFS_Write( pcmBuffer, bytesInBuffer, afd.f );
        SafeFS_Write( padding, paddingSize, afd.f );
        afd.fileSize += ( chunkSize + paddingSize );
        
        afd.numAudioFrames++;
        afd.moviSize += ( chunkSize + paddingSize );
        afd.a.totalBytes = +bytesInBuffer;
        
        // Index
        bufIndex = 0;
        WRITE_STRING( "01wb" );	//dwIdentifier
        WRITE_4BYTES( 0 ); //dwFlags
        WRITE_4BYTES( chunkOffset ); //dwOffset
        WRITE_4BYTES( bytesInBuffer ); //dwLength
        SafeFS_Write( buffer, 16, afd.idxF );
        
        afd.numIndices++;
        
        bytesInBuffer = 0;
    }
}

/*
===============
idClientAVISystemLocal::TakeVideoFrame
===============
*/
void idClientAVISystemLocal::TakeVideoFrame( void )
{
    // AVI file isn't open
    if( !afd.fileOpen )
    {
        return;
    }
    
    renderSystem->TakeVideoFrame( afd.width, afd.height, afd.cBuffer, afd.eBuffer, afd.motionJpeg );
}

/*
===============
idClientAVISystemLocal::CloseAVI

Closes the AVI file and writes an index chunk
===============
*/
bool idClientAVISystemLocal::CloseAVI( void )
{
    sint indexRemainder;
    sint indexSize = afd.numIndices * 16;
    pointer idxFileName = va( "%s" INDEX_FILE_EXTENSION, afd.fileName );
    
    // AVI file isn't open
    if( !afd.fileOpen )
    {
        return false;
    }
    
    afd.fileOpen = false;
    
    fileSystem->Seek( afd.idxF, 4, FS_SEEK_SET );
    bufIndex = 0;
    WRITE_4BYTES( indexSize );
    SafeFS_Write( buffer, bufIndex, afd.idxF );
    fileSystem->FCloseFile( afd.idxF );
    
    // Write index
    // Open the temp index file
    if( ( indexSize = fileSystem->FOpenFileRead( idxFileName, &afd.idxF, true ) ) <= 0 )
    {
        fileSystem->FCloseFile( afd.f );
        return false;
    }
    
    indexRemainder = indexSize;
    
    // Append index to end of avi file
    while( indexRemainder > MAX_AVI_BUFFER )
    {
        fileSystem->Read( buffer, MAX_AVI_BUFFER, afd.idxF );
        SafeFS_Write( buffer, MAX_AVI_BUFFER, afd.f );
        afd.fileSize += MAX_AVI_BUFFER;
        indexRemainder -= MAX_AVI_BUFFER;
    }
    
    fileSystem->Read( buffer, indexRemainder, afd.idxF );
    SafeFS_Write( buffer, indexRemainder, afd.f );
    afd.fileSize += indexRemainder;
    fileSystem->FCloseFile( afd.idxF );
    
    // Remove temp index file
    fileSystem->HomeRemove( idxFileName );
    
    // Write the real header
    fileSystem->Seek( afd.f, 0, FS_SEEK_SET );
    WriteAVIHeader();
    
    bufIndex = 4;
    WRITE_4BYTES( afd.fileSize - 8 ); // "RIFF" size
    
    bufIndex = afd.moviOffset + 4; // Skip "LIST"
    WRITE_4BYTES( afd.moviSize );
    
    SafeFS_Write( buffer, bufIndex, afd.f );
    
    Z_Free( afd.cBuffer );
    Z_Free( afd.eBuffer );
    fileSystem->FCloseFile( afd.f );
    
    Com_Printf( "Wrote %d:%d frames to %s\n", afd.numVideoFrames, afd.numAudioFrames, afd.fileName );
    
    return true;
}

/*
===============
idClientAVISystemLocal::VideoRecording
===============
*/
bool idClientAVISystemLocal::VideoRecording( void )
{
    return afd.fileOpen;
}
