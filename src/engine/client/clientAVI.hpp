////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2005 - 2006 Tim Angus
// Copyright(C) 2011 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   clientAVI.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTAVI_LOCAL_H__
#define __CLIENTAVI_LOCAL_H__

#define INDEX_FILE_EXTENSION ".index.dat"

#define MAX_RIFF_CHUNKS 16

typedef struct audioFormat_s
{
    sint rate;
    sint format;
    sint channels;
    sint bits;
    sint sampleSize;
    sint totalBytes;
} audioFormat_t;

typedef struct aviFileData_s
{
    sint fileSize;
    sint moviOffset;
    sint moviSize;
    sint numIndices;
    sint frameRate;
    sint framePeriod;
    sint width, height;
    sint numVideoFrames;
    sint maxRecordSize;
    sint numAudioFrames;
    sint chunkStack[MAX_RIFF_CHUNKS];
    sint chunkStackTop;
    valueType fileName[MAX_QPATH];
    uchar8* cBuffer;
    uchar8* eBuffer;
    bool fileOpen;
    bool motionJpeg;
    bool audio;
    fileHandle_t f;
    fileHandle_t idxF;
    audioFormat_t a;
} aviFileData_t;

static aviFileData_t afd;

//
// idServerBotSystemLocal
//
class idClientAVISystemLocal : public idClientAVISystemAPI
{
public:
    idClientAVISystemLocal();
    ~idClientAVISystemLocal();
    
    virtual void WriteAVIVideoFrame( const uchar8* imageBuffer, sint size );
    virtual void WriteAVIAudioFrame( const uchar8* pcmBuffer, sint size );
    virtual void TakeVideoFrame( void );
    virtual bool CloseAVI( void );
    virtual bool VideoRecording( void );
    virtual bool OpenAVIForWriting( pointer fileName );
    
    static void SafeFS_Write( const void* buffer, sint len, fileHandle_t f );
    static void WRITE_STRING( pointer s );
    static void WRITE_2BYTES( sint x );
    static void WRITE_4BYTES( sint x );
    static void WRITE_1BYTES( sint x );
    static void START_CHUNK( pointer s );
    static void END_CHUNK( void );
    static void WriteAVIHeader( void );
    static bool CheckFileSize( sint bytesToAdd );
};

extern idClientAVISystemLocal clientAVILocal;

#endif // !__CLIENTAVI_LOCAL_H__

