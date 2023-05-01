////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2005 Stuart Dalton(badcdev@gmail.com)
// Copyright(C) 2011 - 2023 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   sndSystem_codec.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

static snd_codec_t *codecs;

/*
=================
findExtension
Searching
=================
*/
valueType *findExtension(pointer fni) {
    valueType *fn = const_cast<valueType *>(fni);
    valueType *eptr = nullptr;

    while(*fn) {
        if(*fn == '.') {
            eptr = fn;
        }

        fn++;
    }

    return eptr;
}

/*
=================
findCodec
=================
*/
static snd_codec_t *findCodec(pointer filename) {
    valueType *ext = findExtension(filename);
    snd_codec_t *codec = codecs;

    if(!ext) {
        // No extension - auto-detect
        while(codec) {
            valueType fn[MAX_QPATH];

            Q_strncpyz(fn, filename, sizeof(fn) - 4);
            COM_DefaultExtension(fn, sizeof(fn), codec->ext);

            // Check it exists
            if(fileSystem->ReadFile(fn, nullptr) != -1) {
                return codec;
            }

            // Nope. Next!
            codec = codec->next;
        }

        // Nothin'
        return nullptr;
    }

    while(codec) {
        if(!Q_stricmp(ext, codec->ext)) {
            return codec;
        }

        codec = codec->next;
    }

    return nullptr;
}

/*
=================
codec_init
Codec management
=================
*/
void codec_init(void) {
    codecs = nullptr;
    codec_register(&wav_codec);
    codec_register(&ogg_codec);
}

/*
=================
codec_shutdown
=================
*/
void codec_shutdown() {
    codecs = nullptr;
}

/*
=================
codec_register
=================
*/
void codec_register(snd_codec_t *codec) {
    codec->next = codecs;
    codecs = codec;
}

/*
=================
idSoundSystemLocal::codec_load
=================
*/
void *idSoundSystemLocal::codec_load(pointer filename, snd_info_t *info) {
    snd_codec_t *codec;
    valueType fn[MAX_QPATH];

    codec = findCodec(filename);

    if(!codec) {
        common->Printf("Unknown extension for %s\n", filename);
        return nullptr;
    }

    ::strncpy(fn, filename, sizeof(fn));
    COM_DefaultExtension(fn, sizeof(fn), codec->ext);

    return codec->load(fn, info);
}

/*
=================
codec_open
=================
*/
snd_stream_t *idSoundSystemLocal::codec_open(pointer filename) {
    snd_codec_t *codec;
    valueType fn[MAX_QPATH];

    codec = findCodec(filename);

    if(!codec) {
        common->Printf("Unknown extension for %s\n", filename);
        return nullptr;
    }

    ::strncpy(fn, filename, sizeof(fn));
    COM_DefaultExtension(fn, sizeof(fn), codec->ext);

    return codec->open(fn);
}

/*
=================
idSoundSystemLocal::codec_close
=================
*/
void idSoundSystemLocal::codec_close(snd_stream_t *stream) {
    stream->codec->close(stream);
}

/*
=================
codec_read
=================
*/
sint idSoundSystemLocal::codec_read(snd_stream_t *stream, sint bytes,
                                    void *buffer) {
    return stream->codec->read(stream, bytes, buffer);
}

/*
=================
codec_util_open
Util functions (used by codecs)
=================
*/
snd_stream_t *codec_util_open(pointer filename, snd_codec_t *codec) {
    snd_stream_t *stream;
    fileHandle_t hnd;
    sint length;

    // Try to open the file
    length = fileSystem->FOpenFileRead(filename, &hnd, true);

    if(!hnd) {
        common->Printf("Can't read sound file %s\n", filename);
        return nullptr;
    }

    // Allocate a stream
    stream = static_cast<snd_stream_t *>(calloc(1, sizeof(snd_stream_t)));

    if(!stream) {
        fileSystem->FCloseFile(hnd);
        return nullptr;
    }

    // Copy over, return
    stream->codec = codec;
    stream->file = hnd;
    stream->length = length;
    return stream;
}

/*
=================
codec_util_close
=================
*/
void codec_util_close(snd_stream_t *stream) {
    fileSystem->FCloseFile(stream->file);
    free(stream);
}