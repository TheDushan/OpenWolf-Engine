////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2022 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   sndSystem_mem.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

#define DEF_COMSOUNDMEGS "160"

/*
===============================================================================

memory management

===============================================================================
*/

static sndBuffer *buffer = nullptr;
static sndBuffer *freelist = nullptr;
static sint inUse = 0;
static sint totalInUse = 0;

schar16 *sfxScratchBuffer = nullptr;
sfx_t *sfxScratchPointer = nullptr;
sint    sfxScratchIndex = 0;

/*
===============
SND_free
===============
*/
void SND_free(sndBuffer *v) {
    *(sndBuffer **)v = freelist;
    freelist = (sndBuffer *)v;
    inUse += sizeof(sndBuffer);
}

/*
===============
SND_malloc
===============
*/
sndBuffer *SND_malloc(void) {
    sndBuffer *v;
redo:

    if(freelist == nullptr) {
        S_FreeOldestSound();
        goto redo;
    }

    inUse -= sizeof(sndBuffer);
    totalInUse += sizeof(sndBuffer);

    v = freelist;
    freelist = *(sndBuffer **)freelist;
    v->next = nullptr;
    return v;
}

void SND_shutdown(void) {
    free(sfxScratchBuffer);
    free(buffer);
}

/*
===============
SND_setup
===============
*/
void SND_setup() {
    sndBuffer *p, *q;
    convar_t *cv;
    sint scs;

    cv = cvarSystem->Get("com_soundMegs", DEF_COMSOUNDMEGS,
                         CVAR_LATCH | CVAR_ARCHIVE,
                         "Sets the amount of memory (MB) to allocate for loaded sound files");

    scs = (cv->integer * 1536);

    buffer = static_cast<sndBuffer *>(::malloc(scs * sizeof(sndBuffer)));

    // allocate the stack based hunk allocator
    sfxScratchBuffer = static_cast<schar16 *>(malloc(SND_CHUNK_SIZE * sizeof(
                           schar16) *
                       4));    //memorySystem->Alloc(SND_CHUNK_SIZE * sizeof(schar16) * 4);
    sfxScratchPointer = nullptr;

    inUse = scs * sizeof(sndBuffer);
    p = buffer;;
    q = p + scs;

    while(--q > p) {
        *(sndBuffer **)q = q - 1;
    }

    *(sndBuffer **)q = nullptr;
    freelist = p + scs - 1;

    common->Printf("Sound memory manager started\n");
}

/*
================
ResampleSfx

resample / decimate to the current source rate
================
*/
static void ResampleSfx(sfx_t *sfx, sint inrate, sint inwidth,
                        uchar8 *data, bool compressed) {
    sint outcount, srcsample, i, sample, samplefrac, fracstep, part;
    float32 stepscale;
    sndBuffer  *chunk;

    stepscale = static_cast< float32>(inrate) /
                dma.speed;   // this is usually 0.5, 1, or 2

    outcount = sfx->soundLength / stepscale;
    sfx->soundLength = outcount;

    samplefrac = 0;
    fracstep = stepscale * 256;
    chunk = sfx->soundData;

    for(i = 0 ; i < outcount ; i++) {
        srcsample = samplefrac >> 8;
        samplefrac += fracstep;

        if(inwidth == 2) {
            sample = ((reinterpret_cast<schar16 *>(data))[srcsample]);
        } else {
            sample = static_cast<uint>((static_cast<uchar8>(data[srcsample]) - 128) <<
                                       8);
        }

        part  = (i & (SND_CHUNK_SIZE - 1));

        if(part == 0) {
            sndBuffer  *newchunk;
            newchunk = SND_malloc();

            if(chunk == nullptr) {
                sfx->soundData = newchunk;
            } else {
                chunk->next = newchunk;
            }

            chunk = newchunk;
        }

        chunk->sndChunk[part] = sample;
    }
}

/*
================
ResampleSfx

resample / decimate to the current source rate
================
*/
static sint ResampleSfxRaw(schar16 *sfx, sint inrate, sint inwidth,
                           sint samples, uchar8 *data) {
    sint outcount, srcsample, i, sample, samplefrac, fracstep;
    float32 stepscale;

    stepscale = static_cast<float32>(inrate) /
                dma.speed;    // this is usually 0.5, 1, or 2

    outcount = samples / stepscale;

    samplefrac = 0;
    fracstep = stepscale * 256;

    for(i = 0 ; i < outcount ; i++) {
        srcsample = samplefrac >> 8;
        samplefrac += fracstep;

        if(inwidth == 2) {
            sample = LittleShort((reinterpret_cast<schar16 *>(data))[srcsample]);
        } else {
            sample = static_cast<sint>((static_cast<uchar8>(data[srcsample]) - 128) <<
                                       8);
        }

        sfx[i] = sample;
    }

    return outcount;
}

/*
==============
S_LoadSound

The filename may be different than sfx->name in the case
of a forced fallback of a player specific sound
==============
*/
bool S_LoadSound(sfx_t *sfx) {
    uchar8 *data;
    schar16 *samples;
    snd_info_t  info;
    //sint      size;

    // player specific sounds are never directly loaded
    if(sfx->soundName[0] == '*') {
        return false;
    }

    // load it in
    data = static_cast<uchar8 *>(soundSystemLocal.codec_load(sfx->soundName,
                                 &info));

    if(!data) {
        return false;
    }

    if(info.width == 1) {
        if(developer->integer) {
            common->Printf(S_COLOR_YELLOW "WARNING: %s is a 8 bit wav file\n",
                           sfx->soundName);
        }
    }

    if(info.rate != 22050) {
        if(developer->integer) {
            common->Printf(S_COLOR_YELLOW "WARNING: %s is not a 22kHz wav file\n",
                           sfx->soundName);
        }
    }

    samples = static_cast<schar16 *>(memorySystem->AllocateTempMemory(
                                         info.samples * sizeof(schar16) * 2));

    sfx->lastTimeUsed = common->Milliseconds() + 1;

    // each of these compression schemes works just fine
    // but the 16bit quality is much nicer and with a local
    // install assured we can rely upon the sound memory
    // manager to do the right thing for us and page
    // sound in as needed

    if(sfx->soundCompressed == true) {
        sfx->soundCompressionMethod = 1;
        sfx->soundData = nullptr;
        sfx->soundLength = ResampleSfxRaw(samples, info.rate, info.width,
                                          info.samples, (data + info.dataofs));
        S_AdpcmEncodeSound(sfx, samples);
#if 0
    } else if(info.samples > (SND_CHUNK_SIZE * 16) && info.width > 1) {
        sfx->soundCompressionMethod = 3;
        sfx->soundData = nullptr;
        sfx->soundLength = ResampleSfxRaw(samples, info.rate, info.width,
                                          info.samples, (data + info.dataofs));
        encodeMuLaw(sfx, samples);
    } else if(info.samples > (SND_CHUNK_SIZE * 6400) && info.width > 1) {
        sfx->soundCompressionMethod = 2;
        sfx->soundData = nullptr;
        sfx->soundLength = ResampleSfxRaw(samples, info.rate, info.width,
                                          info.samples, (data + info.dataofs));
        encodeWavelet(sfx, samples);
#endif
    } else {
        sfx->soundCompressionMethod = 0;
        sfx->soundLength = info.samples;
        sfx->soundData = nullptr;
        ResampleSfx(sfx, info.rate, info.width, data + info.dataofs, false);
    }

    memorySystem->FreeTempMemory(samples);
    memorySystem->FreeTempMemory(data);

    return true;
}

/*
===============
idSoundSystemLocal::DisplayFreeMemory
===============
*/
void idSoundSystemLocal::DisplayFreeMemory(void) {
    common->Printf("%d bytes free sound buffer memory, %d total used\n", inUse,
                   totalInUse);
}