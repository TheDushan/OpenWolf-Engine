////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2005 Stuart Dalton (badcdev@gmail.com)
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
// File name:   sndSystem_wvelet.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

#define C0 0.4829629131445341
#define C1 0.8365163037378079
#define C2 0.2241438680420134
#define C3 -0.1294095225512604

/*
===============
daub4
===============
*/
void daub4(float32 b[], uint32 n, sint isign) {
    float32 wksp[4097], *a = b - 1; // numerical recipies so a[1] = b[0]

    uint32 nh, nh1, i, j;

    if(n < 4) {
        return;
    }

    nh1 = (nh = n >> 1) + 1;

    if(isign >= 0) {
        for(i = 1, j = 1; j <= n - 3; j += 2, i++) {
            wksp[i]    = C0 * a[j] + C1 * a[j + 1] + C2 * a[j + 2] + C3 * a[j + 3];
            wksp[i + nh] = C3 * a[j] - C2 * a[j + 1] + C1 * a[j + 2] - C0 * a[j + 3];
        }

        wksp[i   ] = C0 * a[n - 1] + C1 * a[n] + C2 * a[1] + C3 * a[2];
        wksp[i + nh] = C3 * a[n - 1] - C2 * a[n] + C1 * a[1] - C0 * a[2];
    } else {
        wksp[1] = C2 * a[nh] + C1 * a[n] + C0 * a[1] + C3 * a[nh1];
        wksp[2] = C3 * a[nh] - C0 * a[n] + C1 * a[1] - C2 * a[nh1];

        for(i = 1, j = 3; i < nh; i++) {
            wksp[j++] = C2 * a[i] + C1 * a[i + nh] + C0 * a[i + 1] + C3 * a[i + nh1];
            wksp[j++] = C3 * a[i] - C0 * a[i + nh] + C1 * a[i + 1] - C2 * a[i + nh1];
        }
    }

    for(i = 1; i <= n; i++) {
        a[i] = wksp[i];
    }
}

/*
===============
wt1
===============
*/
void wt1(float32 a[], uint32 n, sint isign) {
    uint32 nn;
    sint inverseStartLength = n / 4;

    if(n < inverseStartLength) {
        return;
    }

    if(isign >= 0) {
        for(nn = n; nn >= inverseStartLength; nn >>= 1) {
            daub4(a, nn, isign);
        }
    } else {
        for(nn = inverseStartLength; nn <= n; nn <<= 1) {
            daub4(a, nn, isign);
        }
    }
}

/* The number of bits required by each value */
static uchar8 numBits[] = {
    0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
};

/*
===============
MuLawEncode
===============
*/
uchar8 MuLawEncode(schar16 s) {
    uint32 adjusted;
    uchar8 sign, exponent, mantissa;

    sign = (s < 0) ? 0 : 0x80;

    if(s < 0) {
        s = -s;
    }

    adjusted = static_cast<sint32>(s) << (16 - sizeof(schar16) * 8);
    adjusted += 128L + 4L;

    if(adjusted > 32767) {
        adjusted = 32767;
    }

    exponent = numBits[(adjusted >> 7) & 0xff] - 1;
    mantissa = (adjusted >> (exponent + 3)) & 0xf;
    return ~(sign | (exponent << 4) | mantissa);
}

/*
===============
MuLawDecode
===============
*/
schar16 MuLawDecode(uchar8 uLaw) {
    sint32 adjusted;
    uchar8 exponent, mantissa;

    uLaw = ~uLaw;
    exponent = (uLaw >> 4) & 0x7;
    mantissa = (uLaw & 0xf) + 16;
    adjusted = (mantissa << (exponent + 3)) - 128 - 4;

    return (uLaw & 0x80) ? adjusted : -adjusted;
}

schar16 mulawToShort[256];
static bool madeTable = false;

static sint NXStreamCount;

/*
===============
NXPutc
===============
*/
void NXPutc(uchar8 *stream, valueType out) {
    stream[NXStreamCount++] = out;
}

/*
===============
encodeWavelet
===============
*/
void encodeWavelet(sfx_t *sfx, schar16 *packets) {
    float32 wksp[4097], temp;
    sint i, samples, size;
    sndBuffer *newchunk, *chunk;
    uchar8 *out;

    if(!madeTable) {
        for(i = 0; i < 256; i++) {
            mulawToShort[i] = static_cast<float32>(MuLawDecode(static_cast< uchar8 >
                                                   (i)));
        }

        madeTable = true;
    }

    chunk = nullptr;

    samples = sfx->soundLength;

    while(samples > 0) {
        size = samples;

        if(size > (SND_CHUNK_SIZE * 2)) {
            size = (SND_CHUNK_SIZE * 2);
        }

        if(size < 4) {
            size = 4;
        }

        newchunk = SND_malloc();

        if(sfx->soundData == nullptr) {
            sfx->soundData = newchunk;
        } else {
            chunk->next = newchunk;
        }

        chunk = newchunk;

        for(i = 0; i < size; i++) {
            wksp[i] = *packets;
            packets++;
        }

        wt1(wksp, size, 1);
        out = reinterpret_cast<uchar8 *>(chunk->sndChunk);

        for(i = 0; i < size; i++) {
            temp = wksp[i];

            if(temp > 32767) {
                temp = 32767;
            } else if(temp < -32768) {
                temp = -32768;
            }

            out[i] = MuLawEncode(static_cast< schar16 >(temp));
        }

        chunk->size = size;
        samples -= size;
    }
}

/*
===============
decodeWavelet
===============
*/
void decodeWavelet(sndBuffer *chunk, schar16 *to) {
    float32 wksp[4097];
    sint    i;
    uchar8 *out;

    sint size = chunk->size;

    out = reinterpret_cast<uchar8 *>(chunk->sndChunk);

    for(i = 0; i < size; i++) {
        wksp[i] = mulawToShort[out[i]];
    }

    wt1(wksp, size, -1);

    if(!to) {
        return;
    }

    for(i = 0; i < size; i++) {
        to[i] = wksp[i];
    }
}

/*
===============
encodeMuLaw
===============
*/
void encodeMuLaw(sfx_t *sfx, schar16 *packets) {
    sint  i, samples, size, grade, poop;
    sndBuffer *newchunk, *chunk;
    uchar8 *out;

    if(!madeTable) {
        for(i = 0; i < 256; i++) {
            mulawToShort[i] = static_cast<float32>(MuLawDecode(static_cast<uchar8>
                                                   (i)));
        }

        madeTable = true;
    }

    chunk = nullptr;
    samples = sfx->soundLength;
    grade = 0;

    while(samples > 0) {
        size = samples;

        if(size > (SND_CHUNK_SIZE * 2)) {
            size = (SND_CHUNK_SIZE * 2);
        }

        newchunk = SND_malloc();

        if(sfx->soundData == nullptr) {
            sfx->soundData = newchunk;
        } else {
            chunk->next = newchunk;
        }

        chunk = newchunk;
        out = reinterpret_cast<uchar8 *>(chunk->sndChunk);

        for(i = 0; i < size; i++) {
            poop = packets[0] + grade;

            if(poop > 32767) {
                poop = 32767;
            } else if(poop < -32768) {
                poop = -32768;
            }

            out[i] = MuLawEncode((schar16)poop);
            grade = poop - mulawToShort[out[i]];
            packets++;
        }

        chunk->size = size;
        samples -= size;
    }
}

/*
===============
decodeMuLaw
===============
*/
void decodeMuLaw(sndBuffer *chunk, schar16 *to) {
    sint    i;
    uchar8 *out;

    sint size = chunk->size;

    out = reinterpret_cast<uchar8 *>(chunk->sndChunk);

    for(i = 0; i < size; i++) {
        to[i] = mulawToShort[out[i]];
    }
}


