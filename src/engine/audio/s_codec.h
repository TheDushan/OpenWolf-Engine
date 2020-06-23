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
// File name:   s_codec.h
// Version:     v1.02
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SND_CODEC_H__
#define __SND_CODEC_H__

typedef struct snd_info_s
{
    sint rate;
    sint width;
    sint channels;
    sint samples;
    sint size;
    sint dataofs;
} snd_info_t;

typedef struct snd_codec_s snd_codec_t;

typedef struct snd_stream_s
{
    snd_codec_t* codec;
    fileHandle_t file;
    snd_info_t info;
    sint length;
    sint pos;
    void* ptr;
} snd_stream_t;

// Codec functions
typedef void* ( *CODEC_LOAD )( pointer filename, snd_info_t* info );
typedef snd_stream_t* ( *CODEC_OPEN )( pointer filename );
typedef sint( *CODEC_READ )( snd_stream_t* stream, sint bytes, void* buffer );
typedef void ( *CODEC_CLOSE )( snd_stream_t* stream );

// Codec data structure
struct snd_codec_s
{
    valueType* ext;
    CODEC_LOAD load;
    CODEC_OPEN open;
    CODEC_READ read;
    CODEC_CLOSE close;
    snd_codec_t* next;
};

// Codec management
void S_CodecInit( void );
void S_CodecShutdown( void );
void S_CodecRegister( snd_codec_t* codec );
void* S_CodecLoad( pointer filename, snd_info_t* info );
snd_stream_t* S_CodecOpenStream( pointer filename );
void S_CodecCloseStream( snd_stream_t* stream );
sint S_CodecReadStream( snd_stream_t* stream, sint bytes, void* buffer );

// Util functions (used by codecs)
snd_stream_t* S_CodecUtilOpen( pointer filename, snd_codec_t* codec );
void S_CodecUtilClose( snd_stream_t* stream );

// WAV Codec
extern snd_codec_t wav_codec;
void* S_WAV_CodecLoad( pointer filename, snd_info_t* info );
snd_stream_t* S_WAV_CodecOpenStream( pointer filename );
void S_WAV_CodecCloseStream( snd_stream_t* stream );
sint S_WAV_CodecReadStream( snd_stream_t* stream, sint bytes, void* buffer );

// Ogg Vorbis codec
extern snd_codec_t ogg_codec;
void* S_OGG_CodecLoad( pointer filename, snd_info_t* info );
snd_stream_t* S_OGG_CodecOpenStream( pointer filename );
void S_OGG_CodecCloseStream( snd_stream_t* stream );
sint S_OGG_CodecReadStream( snd_stream_t* stream, sint bytes, void* buffer );

#endif // !__SND_CODEC_H__
