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
// File name:   sndSystem_codec_wav.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

/*
=================
FGetLittleLong
=================
*/
static sint FGetLittleLong(fileHandle_t f) {
    sint v;

    fileSystem->Read(&v, sizeof(v), f);

    return LittleLong(v);
}

/*
=================
FGetLittleShort
=================
*/
static sint FGetLittleShort(fileHandle_t f) {
    schar16 v;

    fileSystem->Read(&v, sizeof(v), f);

    return LittleShort(v);
}

/*
=================
readChunkInfo
=================
*/
static sint readChunkInfo(fileHandle_t f, valueType *name) {
    sint len, r;

    name[4] = 0;

    r = fileSystem->Read(name, 4, f);

    if(r != 4) {
        return 0;
    }

    len = FGetLittleLong(f);

    if(len < 0 || len > 0xffffffff) {
        return 0;
    }

    // pad to word boundary
    len = (len + 1) & ~1;
    return len;
}

/*
=================
skipChunk
=================
*/
static void skipChunk(fileHandle_t f, sint length) {
    uchar8 buffer[32 * 1024];

    while(length > 0) {
        sint toread = length;

        if(toread > sizeof(buffer)) {
            toread = sizeof(buffer);
        }

        fileSystem->Read(buffer, toread, f);
        length -= toread;
    }
}

/*
=================
S_FindWavChunk

returns the length of the data in the chunk, or 0 if not found
=================
*/
static sint S_FindWavChunk(fileHandle_t f, valueType *chunk) {
    valueType name[5];
    sint len;

    // This is a bit dangerous...
    while(1) {
        len = readChunkInfo(f, name);

        // Read failure?
        if(len == 0) {
            return 0;
        }

        // If this is the right chunk, return
        if(!Q_strncmp(name, chunk, 4)) {
            return len;
        }

        len = PAD(len, 2);

        // Not the right chunk - skip it
        fileSystem->Seek(f, len, FS_SEEK_CUR);
    }

    return -1;
}

/*
=================
S_ByteSwapRawSamples
=================
*/
static void S_ByteSwapRawSamples(sint samples, sint width, sint s_channels,
                                 const uchar8 *data) {
    sint i;

    if(width != 2) {
        return;
    }

    if(LittleShort(256) == 256) {
        return;
    }

    if(s_channels == 2) {
        samples <<= 1;
    }

    for(i = 0 ; i < samples ; i++) {
        (const_cast<schar16 *>(reinterpret_cast<const schar16 *>
                               (data)))[i] = LittleShort((const_cast<schar16 *>
                                             (reinterpret_cast<const schar16 *>(data)))[i]);
    }
}

/*
=================
read_wav_header
=================
*/
static bool read_wav_header(fileHandle_t file, snd_info_t *info) {
    valueType dump[16];
    sint wav_format, fmtlen = 0;

    // skip the riff wav header
    fileSystem->Read(dump, 12, file);

    // Scan for the format chunk
    if((fmtlen = S_FindWavChunk(file, "fmt ")) == 0) {
        common->Printf("No fmt chunk\n");
        return false;
    }

    // Save the parameters
    wav_format = FGetLittleShort(file);
    info->channels = FGetLittleShort(file);
    info->rate = FGetLittleLong(file);
    FGetLittleLong(file);
    FGetLittleShort(file);
    info->width = FGetLittleShort(file) / 8;
    info->dataofs = 0;

    // Skip the rest of the format chunk if required
    if(fmtlen > 16) {
        fmtlen -= 16;
        skipChunk(file, fmtlen);
    }

    // Scan for the data chunk
    if((info->size = S_FindWavChunk(file, "data")) == 0) {
        common->Printf("No data chunk\n");
        return false;
    }

    info->samples = (info->size / info->width) / info->channels;

    return true;
}

/*
 * WAV codec
 */
snd_codec_t wav_codec = {
    ".wav",
    codec_wav_load,
    codec_wav_open,
    codec_wav_read,
    codec_wav_close,
    nullptr
};

/*
=================
codec_wav_load
=================
*/
void *codec_wav_load(pointer filename, snd_info_t *info) {
    fileHandle_t file;
    void *buffer;

    // Try to open the file
    fileSystem->FOpenFileRead(filename, &file, true);

    if(!file) {
        common->Printf("Can't read sound file %s\n", filename);
        return nullptr;
    }

    // Read the RIFF header
    if(!read_wav_header(file, info)) {
        fileSystem->FCloseFile(file);
        common->Printf("Can't understand wav file %s\n", filename);
        return nullptr;
    }

    // Allocate some memory
    buffer = memorySystem->AllocateTempMemory(info->size);

    if(!buffer) {
        fileSystem->FCloseFile(file);
        common->Printf(S_COLOR_RED "ERROR: Out of memory reading \"%s\"\n",
                       filename);
        return nullptr;
    }

    // Read, byteswap
    fileSystem->Read(buffer, info->size, file);
    S_ByteSwapRawSamples(info->samples, info->width, info->channels,
                         (uchar8 *)buffer);

    // Close and return
    fileSystem->FCloseFile(file);
    return buffer;
}

/*
=================
codec_wav_open
=================
*/
snd_stream_t *codec_wav_open(pointer filename) {
    snd_stream_t *rv;

    // Open
    rv = codec_util_open(filename, &wav_codec);

    if(!rv) {
        return nullptr;
    }

    // Read the RIFF header
    if(!read_wav_header(rv->file, &rv->info)) {
        codec_util_close(rv);
        return nullptr;
    }

    return rv;
}

/*
=================
codec_wav_close
=================
*/
void codec_wav_close(snd_stream_t *stream) {
    codec_util_close(stream);
}

/*
=================
codec_wav_close
=================
*/
sint codec_wav_read(snd_stream_t *stream, sint bytes, void *buffer) {
    sint remaining = stream->info.size - stream->pos, samples;

    if(remaining <= 0) {
        return 0;
    }

    if(bytes > remaining) {
        bytes = remaining;
    }

    stream->pos += bytes;
    samples = (bytes / stream->info.width) / stream->info.channels;
    fileSystem->Read(buffer, bytes, stream->file);
    S_ByteSwapRawSamples(samples, stream->info.width, stream->info.channels,
                         static_cast<const uchar8 *>(buffer));

    return bytes;
}
