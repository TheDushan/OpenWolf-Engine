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
// File name:   clientAVI.h
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTAVI_LOCAL_H__
#define __CLIENTAVI_LOCAL_H__

#define INDEX_FILE_EXTENSION ".index.dat"

#define MAX_RIFF_CHUNKS 16

typedef struct audioFormat_s
{
    S32 rate;
    S32 format;
    S32 channels;
    S32 bits;
    S32 sampleSize;
    S32 totalBytes;
} audioFormat_t;

typedef struct aviFileData_s
{
    S32 fileSize;
    S32 moviOffset;
    S32 moviSize;
    S32 numIndices;
    S32 frameRate;
    S32 framePeriod;
    S32 width, height;
    S32 numVideoFrames;
    S32 maxRecordSize;
    S32 numAudioFrames;
    S32 chunkStack[MAX_RIFF_CHUNKS];
    S32 chunkStackTop;
    UTF8 fileName[MAX_QPATH];
    U8* cBuffer;
    U8* eBuffer;
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
    
    virtual void WriteAVIVideoFrame( const U8* imageBuffer, S32 size );
    virtual void WriteAVIAudioFrame( const U8* pcmBuffer, S32 size );
    virtual void TakeVideoFrame( void );
    virtual bool CloseAVI( void );
    virtual bool VideoRecording( void );
    virtual bool OpenAVIForWriting( StringEntry fileName );
    
    static void SafeFS_Write( const void* buffer, S32 len, fileHandle_t f );
    static void WRITE_STRING( StringEntry s );
    static void WRITE_2BYTES( S32 x );
    static void WRITE_4BYTES( S32 x );
    static void WRITE_1BYTES( S32 x );
    static void START_CHUNK( StringEntry s );
    static void END_CHUNK( void );
    static void WriteAVIHeader( void );
    static bool CheckFileSize( S32 bytesToAdd );
};

extern idClientAVISystemLocal clientAVILocal;

#endif // !__CLIENTAVI_LOCAL_H__

