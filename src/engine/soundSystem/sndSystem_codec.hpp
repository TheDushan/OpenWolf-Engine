////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2005 Stuart Dalton(badcdev@gmail.com)
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
// File name:   sndSystem_codec.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SND_CODEC_H__
#define __SND_CODEC_H__

typedef struct snd_info_s {
    sint rate;
    sint width;
    sint channels;
    sint samples;
    sint size;
    sint dataofs;
} snd_info_t;

typedef struct snd_codec_s snd_codec_t;

typedef struct snd_stream_s {
    snd_codec_t *codec;
    fileHandle_t file;
    snd_info_t info;
    sint length;
    sint pos;
    void *ptr;
} snd_stream_t;

// Codec functions
typedef void *(*CODEC_LOAD)(pointer filename, snd_info_t *info);
typedef snd_stream_t *(*CODEC_OPEN)(pointer filename);
typedef sint(*CODEC_READ)(snd_stream_t *stream, sint bytes, void *buffer);
typedef void (*CODEC_CLOSE)(snd_stream_t *stream);

// Codec data structure
struct snd_codec_s {
    valueType *ext;
    CODEC_LOAD load;
    CODEC_OPEN open;
    CODEC_READ read;
    CODEC_CLOSE close;
    snd_codec_t *next;
};

/*
* Codec management
*/
void codec_init(void);
void codec_shutdown(void);
void codec_register(snd_codec_t *codec);

/*
 * Util functions (used by codecs)
 */
snd_stream_t *codec_util_open(pointer filename, snd_codec_t *codec);
void codec_util_close(snd_stream_t *stream);

/*
 * WAV Codec
 */
extern snd_codec_t wav_codec;
void *codec_wav_load(pointer filename, snd_info_t *info);
snd_stream_t *codec_wav_open(pointer filename);
void codec_wav_close(snd_stream_t *stream);
sint codec_wav_read(snd_stream_t *stream, sint bytes, void *buffer);

/*
 * Ogg Vorbis codec
 */
extern snd_codec_t ogg_codec;
void *codec_ogg_load(pointer filename, snd_info_t *info);
snd_stream_t *codec_ogg_open(pointer filename);
void codec_ogg_close(snd_stream_t *stream);
sint codec_ogg_read(snd_stream_t *stream, sint bytes, void *buffer);

#endif // !__SND_CODEC_H__
