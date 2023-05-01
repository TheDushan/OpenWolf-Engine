////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2023 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of the OpenWolf GPL Source Code.
// OpenWolf Source Code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenWolf Source Code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenWolf Source Code.  If not, see <http://www.gnu.org/licenses/>.
//
// In addition, the OpenWolf Source Code is also subject to certain additional terms.
// You should have received a copy of these additional terms immediately following the
// terms and conditions of the GNU General Public License which accompanied the
// OpenWolf Source Code. If not, please request a copy in writing from id Software
// at the address below.
//
// If you have questions concerning this license or the applicable additional terms,
// you may contact in writing id Software LLC, c/o ZeniMax Media Inc.,
// Suite 120, Rockville, Maryland 20850 USA.
//
// -------------------------------------------------------------------------------------
// File name:   systemSound.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

bool snd_inited = false;

/* The audio callback. All the magic happens here. */
static sint dmapos = 0;
static sint dmasize = 0;

static bool use_custom_memset = false;
static SDL_AudioDeviceID sdlPlaybackDevice;

/*
===============
Snd_Memset
===============
*/

#ifdef __linux__

#ifdef Snd_Memset
#undef Snd_Memset
#endif
void Snd_Memset(void *dest, const sint val, const uint32 count) {
    sint *pDest;
    sint i, iterate;

    if(!use_custom_memset) {
        ::memset(dest, val, count);
        return;
    }

    iterate = count / sizeof(sint);
    pDest = static_cast< sint * >(dest);

    for(i = 0; i < iterate; i++) {
        pDest[i] = val;
    }
}

#endif

/*
===============
SNDDMA_AudioCallback
===============
*/
static void SNDDMA_AudioCallback(void *userdata, Uint8 *stream, sint len) {
    sint             pos = (dmapos * (dma.samplebits / 8));

    if(pos >= dmasize) {
        dmapos = pos = 0;
    }

    if(!snd_inited) {                /* shouldn't happen, but just in case... */
        memset(stream, '\0', len);
        return;
    } else {
        sint             tobufend = dmasize - pos;  /* bytes to buffer's end. */
        sint             len1 = len;
        sint             len2 = 0;

        if(len1 > tobufend) {
            len1 = tobufend;
            len2 = len - len1;
        }

        memcpy(stream, dma.buffer + pos, len1);

        if(len2 <= 0) {
            dmapos += (len1 / (dma.samplebits / 8));
        } else {                /* wraparound? */
            memcpy(stream + len1, dma.buffer, len2);
            dmapos = (len2 / (dma.samplebits / 8));
        }
    }

    if(dmapos >= dmasize) {
        dmapos = 0;
    }
}

static struct {
    uchar16          enumFormat;
    pointer  stringFormat;
} formatToStringTable[] = {
    {
        AUDIO_U8, "AUDIO_U8"
    },
    {
        AUDIO_S8, "AUDIO_S8"
    },
    {
        AUDIO_U16LSB, "AUDIO_U16LSB"
    },
    {
        AUDIO_S16LSB, "AUDIO_S16LSB"
    },
    {
        AUDIO_U16MSB, "AUDIO_U16MSB"
    },
    {
        AUDIO_S16MSB, "AUDIO_S16MSB"
    },
    {
        AUDIO_S32LSB, "AUDIO_S32LSB"
    },
    {
        AUDIO_S32MSB, "AUDIO_S32MSB"
    },
    {
        AUDIO_F32LSB, "AUDIO_F32LSB"
    },
    {
        AUDIO_F32MSB, "AUDIO_F32MSB"
    }
};

static const uint32 formatToStringTableSize = sizeof(
            formatToStringTable) / sizeof(formatToStringTable[0]);

/*
===============
SNDDMA_PrintAudiospec
===============
*/
static void SNDDMA_PrintAudiospec(pointer str, const SDL_AudioSpec *spec) {
    pointer fmt = nullptr;

    common->Printf("%s:\n", str);

    for(uint32 i = 0; i < formatToStringTableSize; i++) {
        if(spec->format == formatToStringTable[i].enumFormat) {
            fmt = formatToStringTable[i].stringFormat;
        }
    }

    if(fmt) {
        common->Printf("  Format:   %s\n", fmt);
    } else {
        common->Printf("  Format:   " S_COLOR_RED "UNKNOWN (%d)\n",
                       static_cast<sint>(spec->format));
    }

    common->Printf("  Freq:     %d\n", static_cast<sint>(spec->freq));
    common->Printf("  Samples:  %d\n", static_cast<sint>(spec->samples));
    common->Printf("  Channels: %d\n", static_cast<sint>(spec->channels));
}

/*
===============
SNDDMA_ExpandSampleFrequencyKHzToHz
===============
*/
static sint SNDDMA_ExpandSampleFrequencyKHzToHz(sint khz) {
    switch(khz) {
        default:
        case 44:
            return 44100;

        case 22:
            return 22050;

        case 11:
            return 11025;
    }
}

/*
===============
SNDDMA_Init
===============
*/
bool SNDDMA_Init(sint sampleFrequencyInKHz) {
    SDL_AudioSpec desired;
    SDL_AudioSpec obtained;
    sint tmp, samples;

    if(snd_inited) {
        return true;
    }

    common->Printf("SDL_Init( SDL_INIT_AUDIO )... ");

    if(SDL_Init(SDL_INIT_AUDIO) != 0) {
        common->Printf("FAILED (%s)\n", SDL_GetError());
        return false;
    }

    common->Printf("OK\n");

    common->Printf("SDL audio driver is \"%s\".\n",
                   SDL_GetCurrentAudioDriver());

    ::memset(&desired, '\0', sizeof(desired));
    ::memset(&obtained, '\0', sizeof(obtained));

    tmp = (s_sdlBits->integer);

    if((tmp != 16) && (tmp != 8)) {
        tmp = 16;
    }

    desired.freq = SNDDMA_ExpandSampleFrequencyKHzToHz(sampleFrequencyInKHz);
    desired.format = ((tmp == 16) ? AUDIO_S16SYS : AUDIO_U8);

    desired.channels = s_sdlChannels->integer;

    // just pick a sane default.
    if(desired.freq <= 11025) {
        samples = 128;
    } else if(desired.freq <= 22050) {
        samples = 256;
    } else if(desired.freq <= 44100) {
        samples = 512;
    } else {
        samples = 1024;    // (*shrug*)
    }

    if(s_sdlDevSamps->integer) {
        tmp = s_sdlDevSamps->integer;
    } else {
        tmp = samples * desired.channels;
    }

    // round up to a power of two
    if(tmp & (tmp - 1)) {
        sint val;

        for(val = 1; val < tmp; val <<= 1);

        tmp = val;
    }

    desired.samples = tmp;
    desired.callback = SNDDMA_AudioCallback;

    sdlPlaybackDevice = SDL_OpenAudioDevice(nullptr, SDL_FALSE, &desired,
                                            &obtained, SDL_AUDIO_ALLOW_ANY_CHANGE);

    if(sdlPlaybackDevice == 0) {
        common->Printf("SDL_OpenAudioDevice() failed: %s\n", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return false;
    }

    SNDDMA_PrintAudiospec("SDL_AudioSpec", &obtained);

    // dma.samples needs to be big, or id's mixer will just refuse to
    //  work at all; we need to keep it significantly bigger than the
    //  amount of SDL callback samples, and just copy a little each time
    //  the callback runs.
    if(s_sdlMixSamps->integer) {
        tmp = s_sdlMixSamps->integer;
    } else {
        tmp = 32 * samples * obtained.channels;
    }

    // samples must be divisible by number of channels
    tmp -= tmp % obtained.channels;

    dmapos = 0;
    dma.samplebits = SDL_AUDIO_BITSIZE(obtained.format);
    dma.isfloat = SDL_AUDIO_ISFLOAT(obtained.format);
    dma.channels = obtained.channels;
    dma.samples = tmp;
    dma.fullsamples = dma.samples / dma.channels;
    dma.submission_chunk = 1;
    dma.speed = obtained.freq;
    dmasize = (dma.samples * (dma.samplebits / 8));
    dma.buffer = static_cast<uchar8 *>(calloc(1, dmasize));

    if(!dma.buffer) {
        common->Printf("Unable to allocate dma buffer\n");
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return false;
    }

    common->Printf("Starting SDL audio callback...\n");
    SDL_PauseAudioDevice(sdlPlaybackDevice, 0);  // start callback.

    common->Printf("SDL audio initialized.\n");
    snd_inited = true;
    return true;
}

/*
===============
SNDDMA_GetDMAPos
===============
*/
sint SNDDMA_GetDMAPos(void) {
    return dmapos;
}

/*
===============
SNDDMA_Shutdown
===============
*/
void SNDDMA_Shutdown(void) {
    if(sdlPlaybackDevice != 0) {
        common->Printf("Closing SDL audio playback device...\n");
        SDL_CloseAudioDevice(sdlPlaybackDevice);
        common->Printf("SDL audio playback device closed.\n");
        sdlPlaybackDevice = 0;
    }

    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    free(dma.buffer);
    dma.buffer = nullptr;
    dmapos = dmasize = 0;
    snd_inited = false;
    common->Printf("SDL audio shut down.\n");
}

/*
===============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void SNDDMA_Submit(void) {
    SDL_UnlockAudioDevice(sdlPlaybackDevice);
}

/*
===============
SNDDMA_BeginPainting
===============
*/
void SNDDMA_BeginPainting(void) {
    SDL_LockAudioDevice(sdlPlaybackDevice);
}
