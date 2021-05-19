////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   cl_cin.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: video and cinematic playback
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

idClientCinemaSystemLocal clientCinemaLocal;
idClientCinemaSystem *clientCinemaSystem = &clientCinemaLocal;

/*
===============
idClientCinemaSystemLocal::idClientCinemaSystemLocal
===============
*/
idClientCinemaSystemLocal::idClientCinemaSystemLocal(void) {
}

/*
===============
idClientCinemaSystemLocal::~idClientCinemaSystemLocal
===============
*/
idClientCinemaSystemLocal::~idClientCinemaSystemLocal(void) {
}

/*
==============
idClientCinemaSystemLocal::CloseAllVideos
==============
*/
void idClientCinemaSystemLocal::CloseAllVideos(void) {
    sint i;

    for(i = 0 ; i < MAX_VIDEO_HANDLES ; i++) {
        if(cinTable[i].fileName[0] != 0) {
            CinemaStopCinematic(i);
        }
    }
}

/*
==============
idClientCinemaSystemLocal::HandleForVideo
==============
*/
sint idClientCinemaSystemLocal::HandleForVideo(void) {
    sint i;

    for(i = 0 ; i < MAX_VIDEO_HANDLES ; i++) {
        if(cinTable[i].fileName[0] == 0) {
            return i;
        }
    }

    Com_Error(ERR_DROP, "CIN_HandleForVideo: none free");
    return -1;
}

/*
==============
idClientCinemaSystemLocal::RllSetupTable

Allocates and initializes the square table.
==============
*/
void idClientCinemaSystemLocal::RllSetupTable(void) {
    sint z;

    for(z = 0; z < 128; z++) {
        cin.sqrTable[z] = (schar16)(z * z);
        cin.sqrTable[z + 128] = (schar16)(-cin.sqrTable[z]);
    }
}

/*
==============
idClientCinemaSystemLocal::RllDecodeMonoToMono

Decode mono source data into a mono buffer.

Parameters:  from -> buffer holding encoded data
            to ->   buffer to hold decoded data
            size =  number of bytes of input (= # of shorts of output)
            signedOutput = 0 for unsigned output, non-zero for signed output
            flag = flags from asset header

Returns:     Number of samples placed in output buffer
==============
*/
sint32 idClientCinemaSystemLocal::RllDecodeMonoToMono(uchar8 *from,
        schar16 *to, uint size,
        valueType signedOutput, uchar16 flag) {
    uint z;
    schar16 prev; //DAJ was sint

    if(signedOutput) {
        prev =  flag - 0x8000;
    } else {
        prev = flag;
    }

    for(z = 0; z < size; z++) {
        prev = to[z] = (schar16)(prev + cin.sqrTable[from[z]]);
    }

    return size;    //*sizeof(schar16));
}

/*
==============
idClientCinemaSystemLocal::RllDecodeMonoToStereo

Decode mono source data into a stereo buffer. Output is 4 times the number
of bytes in the input.

Parameters:  from -> buffer holding encoded data
            to ->   buffer to hold decoded data
            size =  number of bytes of input (= 1/4 # of bytes of output)
            signedOutput = 0 for unsigned output, non-zero for signed output
            flag = flags from asset header

Returns:     Number of samples placed in output buffer
-----------------------------------------------------------------------------
==============
*/
sint32 idClientCinemaSystemLocal::RllDecodeMonoToStereo(uchar8 *from,
        schar16 *to, uint size,
        valueType signedOutput, uchar16 flag) {
    uint z;
    schar16 prev; //DAJ was sint

    if(signedOutput) {
        prev =  flag - 0x8000;
    } else {
        prev = flag;
    }

    for(z = 0; z < size; z++) {
        prev = (schar16)(prev + cin.sqrTable[from[z]]);
        to[z * 2 + 0] = to[z * 2 + 1] = (schar16)(prev);
    }

    return size;    // * 2 * sizeof(schar16));
}

/*
==============
idClientCinemaSystemLocal::RllDecodeStereoToStereo

Decode stereo source data into a stereo buffer.

Parameters:  from -> buffer holding encoded data
            to ->   buffer to hold decoded data
            size =  number of bytes of input (= 1/2 # of bytes of output)
            signedOutput = 0 for unsigned output, non-zero for signed output
            flag = flags from asset header

Returns:     Number of samples placed in output buffer
==============
*/
sint32 idClientCinemaSystemLocal::RllDecodeStereoToStereo(uchar8 *from,
        schar16 *to, uint size,
        valueType signedOutput, uchar16 flag) {
    uint z;
    uchar8 *zz = from;
    schar16 prevL, prevR;     //DAJ was sint

    if(signedOutput) {
        prevL = (flag & 0xff00) - 0x8000;
        prevR = ((flag & 0x00ff) << 8) - 0x8000;
    } else {
        prevL = flag & 0xff00;
        prevR = (flag & 0x00ff) << 8;
    }

    for(z = 0; z < size; z += 2) {
        prevL = (schar16)(prevL + cin.sqrTable[*zz++]);
        prevR = (schar16)(prevR + cin.sqrTable[*zz++]);
        to[z + 0] = (schar16)(prevL);
        to[z + 1] = (schar16)(prevR);
    }

    return (size >> 1);     //*sizeof(schar16));
}

/*
==============
idClientCinemaSystemLocal::RllDecodeStereoToMono

Decode stereo source data into a mono buffer.

 Parameters:  from -> buffer holding encoded data
              to ->   buffer to hold decoded data
              size =  number of bytes of input (= # of bytes of output)
              signedOutput = 0 for unsigned output, non-zero for signed output
              flag = flags from asset header

 Returns:     Number of samples placed in output buffer
============
*/
sint32 idClientCinemaSystemLocal::RllDecodeStereoToMono(uchar8 *from,
        schar16 *to, uint size,
        valueType signedOutput, uchar16 flag) {
    uint z;
    schar16 prevL, prevR;     //DAJ was sint

    if(signedOutput) {
        prevL = (flag & 0xff00) - 0x8000;
        prevR = ((flag & 0x00ff) << 8) - 0x8000;
    } else {
        prevL = flag & 0xff00;
        prevR = (flag & 0x00ff) << 8;
    }

    for(z = 0; z < size; z += 1) {
        prevL = prevL + cin.sqrTable[from[z * 2]];
        prevR = prevR + cin.sqrTable[from[z * 2 + 1]];
        to[z] = (schar16)((prevL + prevR) / 2);
    }

    return size;
}

/*
==============
idClientCinemaSystemLocal::move8_32
==============
*/
void idClientCinemaSystemLocal::move8_32(uchar8 *src, uchar8 *dst,
        sint spl) {
    sint i;

    for(i = 0; i < 8; ++i) {
        ::memcpy(dst, src, 32);
        src += spl;
        dst += spl;
    }
}

/*
==============
idClientCinemaSystemLocal::move4_32
==============
*/
void idClientCinemaSystemLocal::move4_32(uchar8 *src, uchar8 *dst,
        sint spl) {
    sint i;

    for(i = 0; i < 4; ++i) {
        ::memcpy(dst, src, 16);
        src += spl;
        dst += spl;
    }
}

/*
==============
idClientCinemaSystemLocal::blit4_32
==============
*/
void idClientCinemaSystemLocal::blit8_32(uchar8 *src, uchar8 *dst,
        sint spl) {
    sint i;

    for(i = 0; i < 8; ++i) {
        ::memcpy(dst, src, 32);
        src += 32;
        dst += spl;
    }
}

/*
==============
idClientCinemaSystemLocal::blit4_32
==============
*/
void idClientCinemaSystemLocal::blit4_32(uchar8 *src, uchar8 *dst,
        sint spl) {
    sint i;

    for(i = 0; i < 4; ++i) {
        ::memmove(dst, src, 16);
        src += 16;
        dst += spl;
    }
}

/*
==============
idClientCinemaSystemLocal::blit2_32
==============
*/
void idClientCinemaSystemLocal::blit2_32(uchar8 *src, uchar8 *dst,
        sint spl) {
    ::memcpy(dst, src, 8);
    ::memcpy(dst + spl, src + 8, 8);
}

/*
==============
idClientCinemaSystemLocal::blitVQQuad32fs
==============
*/
void idClientCinemaSystemLocal::blitVQQuad32fs(uchar8 **status,
        uchar8 *data) {
    uchar16 newd, celdata, code;
    uint index, i;
    sint spl;

    newd    = 0;
    celdata = 0;
    index   = 0;

    spl = cinTable[currentHandle].samplesPerLine;

    do {
        if(!newd) {
            newd = 7;
            celdata = data[0] + data[1] * 256;
            data += 2;
        } else {
            newd--;
        }

        code = static_cast<uchar16>(celdata) & 0xc000;
        celdata <<= 2;

        switch(code) {
            case    0x8000:                                                     // vq code
                blit8_32(reinterpret_cast<uchar8 *>(&cinTable[currentHandle].vq8[(*data) *
                                                    128]), status[index], spl);
                data++;
                index += 5;
                break;

            case    0xc000:                                                     // drop
                index++;                                                        // skip 8x8

                for(i = 0; i < 4; i++) {
                    if(!newd) {
                        newd = 7;
                        celdata = data[0] + data[1] * 256;
                        data += 2;
                    } else {
                        newd--;
                    }

                    code = static_cast<uchar16>(celdata) & 0xc000;
                    celdata <<= 2;

                    switch(code) {                                              // code in top two bits of code
                        case    0x8000:                                             // 4x4 vq code
                            blit4_32(reinterpret_cast<uchar8 *>(&cinTable[currentHandle].vq4[(*data) *
                                                                32]), status[index], spl);
                            data++;
                            break;

                        case    0xc000:                                             // 2x2 vq code
                            blit2_32(reinterpret_cast<uchar8 *>(&cinTable[currentHandle].vq2[(*data) *
                                                                8]), status[index], spl);
                            data++;
                            blit2_32(reinterpret_cast<uchar8 *>(&cinTable[currentHandle].vq2[(*data) *
                                                                8]), status[index] + 8, spl);
                            data++;
                            blit2_32(reinterpret_cast<uchar8 *>(&cinTable[currentHandle].vq2[(*data) *
                                                                8]), status[index] + spl * 2, spl);
                            data++;
                            blit2_32(reinterpret_cast<uchar8 *>(&cinTable[currentHandle].vq2[(*data) *
                                                                8]), status[index] + spl * 2 + 8, spl);
                            data++;
                            break;

                        case    0x4000:                                             // motion compensation
                            move4_32(status[index] + cin.mcomp[(*data)], status[index], spl);
                            data++;
                            break;
                    }

                    index++;
                }

                break;

            case    0x4000:                                                     // motion compensation
                move8_32(status[index] + cin.mcomp[(*data)], status[index], spl);
                data++;
                index += 5;
                break;

            case    0x0000:
                index += 5;
                break;
        }
    } while(status[index] != nullptr);
}

/*
==============
idClientCinemaSystemLocal::ROQ_GenYUVTables
==============
*/
void idClientCinemaSystemLocal::ROQ_GenYUVTables(void) {
    float32 t_ub, t_vr, t_ug, t_vg;
    sint32 i;

    t_ub = (1.77200f / 2.0f) * static_cast<float32>(1 << 6) + 0.5f;
    t_vr = (1.40200f / 2.0f) * static_cast<float32>(1 << 6) + 0.5f;
    t_ug = (0.34414f / 2.0f) * static_cast<float32>(1 << 6) + 0.5f;
    t_vg = (0.71414f / 2.0f) * static_cast<float32>(1 << 6) + 0.5f;

    for(i = 0; i < 256; i++) {
        float32 x = static_cast<float32>(2 * i - 255);

        ROQ_UB_tab[i] = static_cast<sint32>((t_ub * x)) + (1 << 5);
        ROQ_VR_tab[i] = static_cast<sint32>((t_vr * x)) + (1 << 5);
        ROQ_UG_tab[i] = static_cast<sint32>((-t_ug * x));
        ROQ_VG_tab[i] = static_cast<sint32>((-t_vg * x)) + (1 << 5);
        ROQ_YY_tab[i] = static_cast<sint32>((i << 6)) | (i >> 2);
    }
}

/*
==============
idClientCinemaSystemLocal::yuv_to_rgb
==============
*/
uchar16 idClientCinemaSystemLocal::yuv_to_rgb(sint32 y, sint32 u,
        sint32 v) {
    sint32 r, g, b, YY = static_cast<sint32>(ROQ_YY_tab[(y)]);

    r = (YY + ROQ_VR_tab[v]) >> 9;
    g = (YY + ROQ_UG_tab[u] + ROQ_VG_tab[v]) >> 8;
    b = (YY + ROQ_UB_tab[u]) >> 9;

    if(r < 0) {
        r = 0;
    }

    if(g < 0) {
        g = 0;
    }

    if(b < 0) {
        b = 0;
    }

    if(r > 31) {
        r = 31;
    }

    if(g > 63) {
        g = 63;
    }

    if(b > 31) {
        b = 31;
    }

    return static_cast<uchar16>((r << 11) + (g << 5) + (b));
}

/*
==============
idClientCinemaSystemLocal::yuv_to_rgb24
==============
*/
void idClientCinemaSystemLocal::yuv_to_rgb24(sint32 y, sint32 u, sint32 v,
        uchar8 *out) {
    sint32 YY = static_cast<sint32>(ROQ_YY_tab[y]);

    sint32 r = (YY + ROQ_VR_tab[v]) >> 6;
    sint32 g = (YY + ROQ_UG_tab[u] + ROQ_VG_tab[v]) >> 6;
    sint32 b = (YY + ROQ_UB_tab[u]) >> 6;

    if(r < 0) {
        r = 0;
    }

    if(g < 0) {
        g = 0;
    }

    if(b < 0) {
        b = 0;
    }

    if(r > 255) {
        r = 255;
    }

    if(g > 255) {
        g = 255;
    }

    if(b > 255) {
        b = 255;
    }

    out[0] = r;
    out[1] = g;
    out[2] = b;
    out[3] = 255;
}

/*
==============
idClientCinemaSystemLocal::decodeCodeBook
==============
*/
void idClientCinemaSystemLocal::decodeCodeBook(uchar8 *input) {
    sint two, four;

    if(!cinTable[currentHandle].roq_flags) {
        two = four = 256;
    } else {
        two = static_cast<uchar16>(cinTable[currentHandle].roq_flags) >> 8;

        if(!two) {
            two = 256;
        }

        four = cinTable[currentHandle].roq_flags & 0xff;
    }

    four *= 2;

    uchar8 *rgb_ptr = reinterpret_cast<uchar8 *>(cinTable[currentHandle].vq2);

    for(sint i = 0; i < two; i++) {
        sint32 y0 = static_cast<sint32>(*input++);
        sint32 y1 = static_cast<sint32>(*input++);
        sint32 y2 = static_cast<sint32>(*input++);
        sint32 y3 = static_cast<sint32>(*input++);
        sint32 cr = static_cast<sint32>(*input++);
        sint32 cb = static_cast<sint32>(*input++);
        yuv_to_rgb24(y0, cr, cb, rgb_ptr);
        yuv_to_rgb24(y1, cr, cb, rgb_ptr + 4);
        yuv_to_rgb24(y2, cr, cb, rgb_ptr + 8);
        yuv_to_rgb24(y3, cr, cb, rgb_ptr + 12);
        rgb_ptr += 16;
    }

    uint *icptr = reinterpret_cast<uint *>(cinTable[currentHandle].vq4);
    uint *idptr = reinterpret_cast<uint *>(cinTable[currentHandle].vq8);

#define VQ2TO4(a,b,c,d) { \
        *c++ = a[0];    \
        *d++ = a[0];    \
        *d++ = a[0];    \
        *c++ = a[1];    \
        *d++ = a[1];    \
        *d++ = a[1];    \
        *c++ = b[0];    \
        *d++ = b[0];    \
        *d++ = b[0];    \
        *c++ = b[1];    \
        *d++ = b[1];    \
        *d++ = b[1];    \
        *d++ = a[0];    \
        *d++ = a[0];    \
        *d++ = a[1];    \
        *d++ = a[1];    \
        *d++ = b[0];    \
        *d++ = b[0];    \
        *d++ = b[1];    \
        *d++ = b[1];    \
        a += 2; b += 2; }

    for(sint i = 0; i < four; i++) {
        uint *iaptr = reinterpret_cast<uint *>(cinTable[currentHandle].vq2) +
                      (*input++) * 4;
        uint *ibptr = reinterpret_cast<uint *>(cinTable[currentHandle].vq2) +
                      (*input++) * 4;

        for(sint j = 0; j < 2; j++) {
            VQ2TO4(iaptr, ibptr, icptr, idptr);
        }
    }
}

/*
==============
idClientCinemaSystemLocal::recurseQuad
==============
*/
void idClientCinemaSystemLocal::recurseQuad(sint32 startX, sint32 startY,
        sint32 quadSize,
        sint32 xOff, sint32 yOff) {
    uchar8 *scroff;
    sint32 bigx, bigy, lowx, lowy, useY;
    sint32 offset;

    offset = cinTable[currentHandle].screenDelta;

    lowx = lowy = 0;
    bigx = cinTable[currentHandle].xsize;
    bigy = cinTable[currentHandle].ysize;

    if(bigx > cinTable[currentHandle].CIN_WIDTH) {
        bigx = cinTable[currentHandle].CIN_WIDTH;
    }

    if(bigy > cinTable[currentHandle].CIN_HEIGHT) {
        bigy = cinTable[currentHandle].CIN_HEIGHT;
    }

    if((startX >= lowx) && (startX + quadSize) <= (bigx) &&
            (startY + quadSize) <= (bigy) && (startY >= lowy) && quadSize <= MAXSIZE) {
        useY = startY;
        scroff = cin.linbuf + (useY + ((cinTable[currentHandle].CIN_HEIGHT - bigy)
                                       >> 1) + yOff) * (cinTable[currentHandle].samplesPerLine) + (((
                                               startX + xOff)) * cinTable[currentHandle].samplesPerPixel);

        cin.qStatus[0][cinTable[currentHandle].onQuad  ] = scroff;
        cin.qStatus[1][cinTable[currentHandle].onQuad++] = scroff + offset;
    }

    if(quadSize != MINSIZE) {
        quadSize >>= 1;
        recurseQuad(startX,          startY, quadSize, xOff, yOff);
        recurseQuad(startX + quadSize, startY, quadSize, xOff, yOff);
        recurseQuad(startX,          startY + quadSize, quadSize, xOff, yOff);
        recurseQuad(startX + quadSize, startY + quadSize, quadSize, xOff, yOff);
    }
}

/*
==============
idClientCinemaSystemLocal::setupQuad
==============
*/
void idClientCinemaSystemLocal::setupQuad(sint32 xOff, sint32 yOff) {
    sint32 numQuadCels, i, x, y;
    uchar8 *temp;

    if(xOff == cin.oldXOff && yOff == cin.oldYOff &&
            cinTable[currentHandle].ysize == cin.oldysize &&
            cinTable[currentHandle].xsize == cin.oldxsize) {
        return;
    }

    cin.oldXOff = xOff;
    cin.oldYOff = yOff;
    cin.oldysize = cinTable[currentHandle].ysize;
    cin.oldxsize = cinTable[currentHandle].xsize;

    numQuadCels  = (cinTable[currentHandle].xsize *
                    cinTable[currentHandle].ysize) / (16);
    numQuadCels += numQuadCels / 4;
    numQuadCels += 64; // for overflow

    cinTable[currentHandle].onQuad = 0;

    for(y = 0; y < static_cast<sint32>(cinTable[currentHandle].ysize); y += 16)
        for(x = 0; x < static_cast<sint32>(cinTable[currentHandle].xsize);
                x += 16) {
            recurseQuad(x, y, 16, xOff, yOff);
        }

    temp = nullptr;

    for(i = (numQuadCels - 64); i < numQuadCels; i++) {
        cin.qStatus[0][i] = temp; // eoq
        cin.qStatus[1][i] = temp; // eoq
    }
}

/*
==============
idClientCinemaSystemLocal::readQuadInfo
==============
*/
void idClientCinemaSystemLocal::readQuadInfo(uchar8 *qData) {
    if(currentHandle < 0) {
        return;
    }

    cinTable[currentHandle].xsize    = qData[0] + qData[1] * 256;
    cinTable[currentHandle].ysize    = qData[2] + qData[3] * 256;
    cinTable[currentHandle].maxsize  = qData[4] + qData[5] * 256;
    cinTable[currentHandle].minsize  = qData[6] + qData[7] * 256;

    cinTable[currentHandle].CIN_HEIGHT = cinTable[currentHandle].ysize;
    cinTable[currentHandle].CIN_WIDTH  = cinTable[currentHandle].xsize;

    cinTable[currentHandle].samplesPerLine = cinTable[currentHandle].CIN_WIDTH
            * cinTable[currentHandle].samplesPerPixel;
    cinTable[currentHandle].screenDelta = cinTable[currentHandle].CIN_HEIGHT *
                                          cinTable[currentHandle].samplesPerLine;

    cinTable[currentHandle].half = false;
    cinTable[currentHandle].smootheddouble = false;

    cinTable[currentHandle].VQ0 = cinTable[currentHandle].VQNormal;
    cinTable[currentHandle].VQ1 = cinTable[currentHandle].VQBuffer;

    cinTable[currentHandle].t[0] = cinTable[currentHandle].screenDelta;
    cinTable[currentHandle].t[1] = -cinTable[currentHandle].screenDelta;

    cinTable[currentHandle].drawX = cinTable[currentHandle].CIN_WIDTH;
    cinTable[currentHandle].drawY = cinTable[currentHandle].CIN_HEIGHT;

    // rage pro is very slow at 512 wide textures, voodoo can't do it at all
    if(cls.glconfig.maxTextureSize <= 256) {
        if(cinTable[currentHandle].drawX > 256) {
            cinTable[currentHandle].drawX = 256;
        }

        if(cinTable[currentHandle].drawY > 256) {
            cinTable[currentHandle].drawY = 256;
        }

        if(cinTable[currentHandle].CIN_WIDTH != 256 ||
                cinTable[currentHandle].CIN_HEIGHT != 256) {
            Com_Printf("HACK: approxmimating cinematic for Rage Pro or Voodoo\n");
        }
    }
}

/*
==============
idClientCinemaSystemLocal::RoQPrepMcomp
==============
*/
void idClientCinemaSystemLocal::RoQPrepMcomp(sint32 xoff, sint32 yoff) {
    sint32 i, j, x, y, temp, temp2;

    i = cinTable[currentHandle].samplesPerLine;
    j = cinTable[currentHandle].samplesPerPixel;

    if(cinTable[currentHandle].xsize == (cinTable[currentHandle].ysize * 4) &&
            !cinTable[currentHandle].half) {
        j = j + j;
        i = i + i;
    }

    for(y = 0; y < 16; y++) {
        temp2 = (y + yoff - 8) * i;

        for(x = 0; x < 16; x++) {
            temp = (x + xoff - 8) * j;
            cin.mcomp[(x * 16) + y] = cinTable[currentHandle].normalBuffer0 -
                                      (temp2 + temp);
        }
    }
}

/*
==============
idClientCinemaSystemLocal::initRoQ
==============
*/
void idClientCinemaSystemLocal::initRoQ(void) {
    if(currentHandle < 0) {
        return;
    }

    cinTable[currentHandle].VQNormal = (void(*)(uchar8 *,
                                        void *))blitVQQuad32fs;
    cinTable[currentHandle].VQBuffer = (void(*)(uchar8 *,
                                        void *))blitVQQuad32fs;
    cinTable[currentHandle].samplesPerPixel = 4;
    ROQ_GenYUVTables();
    RllSetupTable();
}

/*
==============
idClientCinemaSystemLocal::RoQReset
==============
*/
void idClientCinemaSystemLocal::RoQReset(void) {
    if(currentHandle < 0) {
        return;
    }

    // DHM - Properly close file so we don't run out of handles
    fileSystem->FCloseFile(cinTable[currentHandle].iFile);
    cinTable[currentHandle].iFile = 0;
    // dhm - end

    fileSystem->FOpenFileRead(cinTable[currentHandle].fileName,
                              &cinTable[currentHandle].iFile, true);
    // let the background thread start reading ahead
    fileSystem->Read(cin.file, 16, cinTable[currentHandle].iFile);
    RoQ_init();
    cinTable[currentHandle].status = FMV_LOOPED;
}

/*
==============
idClientCinemaSystemLocal::RoQInterrupt
==============
*/
void idClientCinemaSystemLocal::RoQInterrupt(void) {
    uchar8 *framedata;
    schar16 sbuf[32768];
    sint ssize;

    if(currentHandle < 0) {
        return;
    }

    fileSystem->Read(cin.file, cinTable[currentHandle].RoQFrameSize + 8,
                     cinTable[currentHandle].iFile);

    if(cinTable[currentHandle].RoQPlayed >= cinTable[currentHandle].ROQSize) {
        if(cinTable[currentHandle].holdAtEnd == false) {
            if(cinTable[currentHandle].looping) {
                RoQReset();
            } else {
                cinTable[currentHandle].status = FMV_EOF;
            }
        } else {
            cinTable[currentHandle].status = FMV_IDLE;
        }

        return;
    }

    framedata = cin.file;
    //
    // new frame is ready
    //
redump:

    switch(cinTable[currentHandle].roq_id) {
        case    ROQ_QUAD_VQ:
            if((cinTable[currentHandle].numQuads & 1)) {
                cinTable[currentHandle].normalBuffer0 = cinTable[currentHandle].t[1];
                RoQPrepMcomp(cinTable[currentHandle].roqF0, cinTable[currentHandle].roqF1);
                cinTable[currentHandle].VQ1(reinterpret_cast<uchar8 *>(cin.qStatus[1]),
                                            framedata);
                cinTable[currentHandle].buf =   cin.linbuf +
                                                cinTable[currentHandle].screenDelta;
            } else {
                cinTable[currentHandle].normalBuffer0 = cinTable[currentHandle].t[0];
                RoQPrepMcomp(cinTable[currentHandle].roqF0, cinTable[currentHandle].roqF1);
                cinTable[currentHandle].VQ0(reinterpret_cast<uchar8 *>(cin.qStatus[0]),
                                            framedata);
                cinTable[currentHandle].buf =   cin.linbuf;
            }

            if(cinTable[currentHandle].numQuads == 0) {             // first frame
                memcpy(cin.linbuf + cinTable[currentHandle].screenDelta, cin.linbuf,
                       cinTable[currentHandle].samplesPerLine * cinTable[currentHandle].ysize);
            }

            cinTable[currentHandle].numQuads++;
            cinTable[currentHandle].dirty = true;
            break;

        case    ROQ_CODEBOOK:
            decodeCodeBook(framedata);
            break;

        case    ZA_SOUND_MONO:
            if(!cinTable[currentHandle].silent) {
                ssize = clientCinemaLocal.RllDecodeMonoToStereo(framedata, sbuf,
                        cinTable[currentHandle].RoQFrameSize, 0,
                        static_cast<uchar16>(cinTable[currentHandle].roq_flags));
                soundSystem->RawSamples(0, ssize, 22050, 2, 1,
                                        reinterpret_cast<uchar8 *>(sbuf), 1.0f, 1.0f);
                cinTable[currentHandle].sound = 1;
            }

            break;

        case    ZA_SOUND_STEREO:
            if(!cinTable[currentHandle].silent) {
                if(cinTable[currentHandle].numQuads == -1) {
                    soundSystem->Update();

                    if(developer->integer) {
                        Com_Printf("S_Update: Setting rawend to %i\n", s_soundtime);
                    }

                    s_rawend = s_soundtime;         //DAJ added
                }

                ssize = clientCinemaLocal.RllDecodeStereoToStereo(framedata, sbuf,
                        cinTable[currentHandle].RoQFrameSize, 0,
                        static_cast<uchar16>(cinTable[currentHandle].roq_flags));
                soundSystem->RawSamples(0, ssize, 22050, 2, 2,
                                        reinterpret_cast<uchar8 *>(sbuf), 1.0f, 1.0f);
                cinTable[currentHandle].sound = 1;
            }

            break;

        case    ROQ_QUAD_INFO:
            if(cinTable[currentHandle].numQuads == -1) {
                readQuadInfo(framedata);
                setupQuad(0, 0);
                cinTable[currentHandle].startTime = cinTable[currentHandle].lastTime =
                                                        CL_ScaledMilliseconds();
            }

            if(cinTable[currentHandle].numQuads != 1) {
                cinTable[currentHandle].numQuads = 0;
            }

            break;

        case    ROQ_PACKET:
            cinTable[currentHandle].inMemory = cinTable[currentHandle].roq_flags;
            cinTable[currentHandle].RoQFrameSize = 0;               // for header
            break;

        case    ROQ_QUAD_HANG:
            cinTable[currentHandle].RoQFrameSize = 0;
            break;

        case    ROQ_QUAD_JPEG:
            break;

        default:
            cinTable[currentHandle].status = FMV_EOF;
            break;
    }

    //
    // read in next frame data
    //
    if(cinTable[currentHandle].RoQPlayed >= cinTable[currentHandle].ROQSize) {
        if(cinTable[currentHandle].holdAtEnd == false) {
            if(cinTable[currentHandle].looping) {
                RoQReset();
            } else {
                cinTable[currentHandle].status = FMV_EOF;
            }
        } else {
            cinTable[currentHandle].status = FMV_IDLE;
        }

        return;
    }

    framedata        += cinTable[currentHandle].RoQFrameSize;
    cinTable[currentHandle].roq_id       = framedata[0] + framedata[1] * 256;
    cinTable[currentHandle].RoQFrameSize = framedata[2] + framedata[3] * 256 +
                                           framedata[4] * 65536;
    cinTable[currentHandle].roq_flags    = framedata[6] + framedata[7] * 256;
    cinTable[currentHandle].roqF0        = static_cast< schar8 >(framedata[7]);
    cinTable[currentHandle].roqF1        = static_cast<schar8>(framedata[6]);

    if(cinTable[currentHandle].RoQFrameSize > 65536 ||
            cinTable[currentHandle].roq_id == 0x1084) {
        if(developer->integer) {
            Com_Printf("roq_size>65536||roq_id==0x1084\n");
        }

        cinTable[currentHandle].status = FMV_EOF;

        if(cinTable[currentHandle].looping) {
            RoQReset();
        }

        return;
    }

    if(cinTable[currentHandle].inMemory &&
            (cinTable[currentHandle].status != FMV_EOF)) {
        cinTable[currentHandle].inMemory--;
        framedata += 8;
        goto redump;
    }

    //
    // one more frame hits the dust
    //
    //  assert(cinTable[currentHandle].RoQFrameSize <= 65536);
    //  r = fileSystem->Read( cin.file, cinTable[currentHandle].RoQFrameSize+8, cinTable[currentHandle].iFile );
    cinTable[currentHandle].RoQPlayed   += cinTable[currentHandle].RoQFrameSize
                                           + 8;
}

/*
==============
idClientCinemaSystemLocal::RoQShutdown
==============
*/
void idClientCinemaSystemLocal::RoQ_init(void) {
    cinTable[currentHandle].startTime = cinTable[currentHandle].lastTime =
                                            CL_ScaledMilliseconds();

    cinTable[currentHandle].RoQPlayed = 24;

    /*  get frame rate */
    cinTable[currentHandle].roqFPS   = cin.file[ 6] + cin.file[ 7] * 256;

    if(!cinTable[currentHandle].roqFPS) {
        cinTable[currentHandle].roqFPS = 30;
    }

    cinTable[currentHandle].numQuads = -1;

    cinTable[currentHandle].roq_id      = cin.file[ 8] + cin.file[ 9] * 256;
    cinTable[currentHandle].RoQFrameSize    = cin.file[10] + cin.file[11] * 256
            + cin.file[12] * 65536;
    cinTable[currentHandle].roq_flags   = cin.file[14] + cin.file[15] * 256;

    if(cinTable[currentHandle].RoQFrameSize > 65536 ||
            !cinTable[currentHandle].RoQFrameSize) {
        return;
    }

}

/*
==============
idClientCinemaSystemLocal::RoQShutdown
==============
*/
void idClientCinemaSystemLocal::RoQShutdown(void) {
    pointer s;

    if(!cinTable[currentHandle].buf) {
        return;
    }

    if(cinTable[currentHandle].status == FMV_IDLE) {
        return;
    }

    if(developer->integer) {
        Com_Printf("idClientCinemaSystemLocal::RoQShutdown: finished cinematic\n");
    }

    cinTable[currentHandle].status = FMV_IDLE;

    if(cinTable[currentHandle].iFile) {
        fileSystem->FCloseFile(cinTable[currentHandle].iFile);
        cinTable[currentHandle].iFile = 0;
    }

    if(cinTable[currentHandle].alterGameState) {
        cls.state = CA_DISCONNECTED;
        // we can't just do a vstr nextmap, because
        // if we are aborting the intro cinematic with
        // a devmap command, nextmap would be valid by
        // the time it was referenced
        s = cvarSystem->VariableString("nextmap");

        if(s[0]) {
            cmdBufferSystem->ExecuteText(EXEC_APPEND, va("%s\n", s));
            cvarSystem->Set("nextmap", "");
        }

        CL_handle = -1;
    }

    cinTable[currentHandle].fileName[0] = 0;
    currentHandle = -1;
}

/*
==================
idClientCinemaSystemLocal::CinemaStopCinematic
==================
*/
e_status idClientCinemaSystemLocal::CinemaStopCinematic(sint handle) {

    if(handle < 0 || handle >= MAX_VIDEO_HANDLES ||
            cinTable[handle].status == FMV_EOF) {
        return FMV_EOF;
    }

    currentHandle = handle;

    if(developer->integer) {
        Com_Printf("trFMV::stop(), closing %s\n",
                   cinTable[currentHandle].fileName);
    }

    if(!cinTable[currentHandle].buf) {
        return FMV_EOF;
    }

    if(cinTable[currentHandle].alterGameState) {
        if(cls.state != CA_CINEMATIC) {
            return cinTable[currentHandle].status;
        }
    }

    cinTable[currentHandle].status = FMV_EOF;
    RoQShutdown();

    return FMV_EOF;
}

/*
==================
idClientCinemaSystemLocal::CinemaRunCinematic

Fetch and decompress the pending frame
==================
*/
e_status idClientCinemaSystemLocal::CinemaRunCinematic(sint handle) {
    sint start = 0;
    sint thisTime = 0;
    sint played = 0;

    if(handle < 0 || handle >= MAX_VIDEO_HANDLES ||
            cinTable[handle].status == FMV_EOF) {
        return FMV_EOF;
    }

    if(cin.currentHandle != handle) {
        currentHandle = handle;
        cin.currentHandle = currentHandle;
        cinTable[currentHandle].status = FMV_EOF;
        RoQReset();
    }

    if(cinTable[handle].playonwalls < -1) {
        return cinTable[handle].status;
    }

    currentHandle = handle;

    if(cinTable[currentHandle].alterGameState) {
        if(cls.state != CA_CINEMATIC) {
            return cinTable[currentHandle].status;
        }
    }

    if(cinTable[currentHandle].status == FMV_IDLE) {
        return cinTable[currentHandle].status;
    }

    thisTime = CL_ScaledMilliseconds();

    if(cinTable[currentHandle].shader &&
            (thisTime - cinTable[currentHandle].lastTime) > 100) {
        cinTable[currentHandle].startTime += thisTime -
                                             cinTable[currentHandle].lastTime;
    }

    //----(SA)  modified to use specified fps for roq's
    cinTable[currentHandle].tfps = (((CL_ScaledMilliseconds() -
                                      cinTable[currentHandle].startTime) * cinTable[currentHandle].roqFPS) /
                                    1000);

    start = cinTable[currentHandle].startTime;

    while((cinTable[currentHandle].tfps != cinTable[currentHandle].numQuads) &&
            (cinTable[currentHandle].status == FMV_PLAY)) {
        RoQInterrupt();

        if(start != cinTable[currentHandle].startTime) {
            cinTable[currentHandle].tfps = (((CL_ScaledMilliseconds() -
                                              cinTable[currentHandle].startTime) * cinTable[currentHandle].roqFPS) /
                                            1000);

            start = cinTable[currentHandle].startTime;
        }

        played = 1;
    }

    //DAJ added 's
    if(played && cinTable[currentHandle].sound) {
        if(s_rawend < s_soundtime && (s_soundtime - s_rawend) < 100) {
            cinTable[currentHandle].startTime -= (s_soundtime - s_rawend);

            do {
                RoQInterrupt();
            } while(s_rawend < s_soundtime &&

                    cinTable[currentHandle].status == FMV_PLAY);
        }
    }


    //----(SA)  end

    cinTable[currentHandle].lastTime = thisTime;

    if(cinTable[currentHandle].status == FMV_LOOPED) {
        cinTable[currentHandle].status = FMV_PLAY;
    }

    if(cinTable[currentHandle].status == FMV_EOF) {
        if(cinTable[currentHandle].looping) {
            RoQReset();
        } else {
            RoQShutdown();
            return FMV_EOF;
        }
    }

    return cinTable[currentHandle].status;
}

/*
==================
idClientCinemaSystemLocal::PlayCinematic
==================
*/
sint idClientCinemaSystemLocal::PlayCinematic(pointer arg, sint x, sint y,
        sint w, sint h,
        sint systemBits) {
    uchar16 RoQID;
    valueType name[MAX_OSPATH];
    sint i;

    if(strstr(arg, "/") == nullptr && strstr(arg, "\\") == nullptr) {
        Q_vsprintf_s(name, sizeof(name), sizeof(name), "video/%s", arg);
    } else {
        Q_vsprintf_s(name, sizeof(name), sizeof(name), "%s", arg);
    }

    if(!(systemBits & CIN_system)) {
        for(i = 0 ; i < MAX_VIDEO_HANDLES ; i++) {
            if(!Q_stricmp(cinTable[i].fileName, name)) {
                return i;
            }
        }
    }

    if(developer->integer) {
        Com_Printf("idClientCinemaSystemLocal::PlayCinematic( %s )\n", arg);
    }

    ::memset(&cin, 0, sizeof(cinematics_t));
    currentHandle = HandleForVideo();

    cin.currentHandle = currentHandle;

    Q_strcpy_s(cinTable[currentHandle].fileName, name);

    cinTable[currentHandle].ROQSize = 0;
    cinTable[currentHandle].ROQSize = fileSystem->FOpenFileRead(
                                          cinTable[currentHandle].fileName, &cinTable[currentHandle].iFile, true);

    if(cinTable[currentHandle].ROQSize <= 0) {
        if(developer->integer) {
            Com_Printf("play(%s), ROQSize<=0\n", arg);
        }

        cinTable[currentHandle].fileName[0] = 0;
        return -1;
    }

    SetExtents(currentHandle, x, y, w, h);
    SetLooping(currentHandle, (systemBits & CIN_loop) != 0);

    cinTable[currentHandle].CIN_HEIGHT = DEFAULT_CIN_HEIGHT;
    cinTable[currentHandle].CIN_WIDTH  =  DEFAULT_CIN_WIDTH;
    cinTable[currentHandle].holdAtEnd = (systemBits & CIN_hold) != 0;
    cinTable[currentHandle].alterGameState = (systemBits & CIN_system) != 0;
    cinTable[currentHandle].playonwalls = 1;
    cinTable[currentHandle].silent = (systemBits & CIN_silent) != 0;
    cinTable[currentHandle].shader = (systemBits & CIN_shader) != 0;
    cinTable[currentHandle].letterBox = (systemBits & CIN_letterBox) != 0;
    cinTable[currentHandle].sound = 0;

    if(cinTable[currentHandle].alterGameState) {
        // close the menu
        if(uivm) {
            uiManager->SetActiveMenu(UIMENU_NONE);
        }
    } else {
        cinTable[currentHandle].playonwalls = cl_inGameVideo->integer;
    }

    initRoQ();

    fileSystem->Read(cin.file, 16, cinTable[currentHandle].iFile);

    RoQID = static_cast<uchar16>((cin.file[0])) + static_cast<uchar16>((
                cin.file[1])) * 256;

    if(RoQID == 0x1084) {
        RoQ_init();
        //      fileSystem->Read (cin.file, cinTable[currentHandle].RoQFrameSize+8, cinTable[currentHandle].iFile);

        cinTable[currentHandle].status = FMV_PLAY;

        if(developer->integer) {
            Com_Printf("trFMV::play(), playing %s\n", arg);
        }

        if(cinTable[currentHandle].alterGameState) {
            cls.state = CA_CINEMATIC;
        }

        clientConsoleSystem->Close();

        if(developer->integer) {
            Com_Printf("Setting rawend to %i\n", s_soundtime);
        }

        if(!cinTable[currentHandle].silent) {
            s_rawend = s_soundtime;
        }

        return currentHandle;
    }

    if(developer->integer) {
        Com_Printf("trFMV::play(), invalid RoQ ID\n");
    }

    RoQShutdown();
    return -1;
}

/*
==============
idClientCinemaSystemLocal::SetExtents
==============
*/
void idClientCinemaSystemLocal::SetExtents(sint handle, sint x, sint y,
        sint w, sint h) {
    if(handle < 0 || handle >= MAX_VIDEO_HANDLES ||
            cinTable[handle].status == FMV_EOF) {
        return;
    }

    cinTable[handle].xpos = x;
    cinTable[handle].ypos = y;
    cinTable[handle].width = w;
    cinTable[handle].height = h;
    cinTable[handle].dirty = true;
}

/*
==============
idClientCinemaSystemLocal::SetLooping
==============
*/
void idClientCinemaSystemLocal::SetLooping(sint handle, bool loop) {
    if(handle < 0 || handle >= MAX_VIDEO_HANDLES ||
            cinTable[handle].status == FMV_EOF) {
        return;
    }

    cinTable[handle].looping = loop;
}

/*
==================
idClientCinemaSystemLocal::ResampleCinematic

Resample cinematic to 256x256 and store in buf2
==================
*/
void idClientCinemaSystemLocal::ResampleCinematic(sint handle,
        sint *buf2) {
    sint ix, iy, *buf3, xm, ym, ll;
    uchar8 *buf;

    buf = cinTable[handle].buf;

    xm = cinTable[handle].CIN_WIDTH / 256;
    ym = cinTable[handle].CIN_HEIGHT / 256;
    ll = 8;

    if(cinTable[handle].CIN_WIDTH == 512) {
        ll = 9;
    }

    buf3 = reinterpret_cast<sint *>(buf);

    if(xm == 2 && ym == 2) {
        uchar8 *bc2, *bc3;
        sint    ic, iiy;

        bc2 = reinterpret_cast<uchar8 *>(buf2);
        bc3 = reinterpret_cast<uchar8 *>(buf3);

        for(iy = 0; iy < 256; iy++) {
            iiy = iy << 12;

            for(ix = 0; ix < 2048; ix += 8) {
                for(ic = ix; ic < (ix + 4); ic++) {
                    *bc2 = (bc3[iiy + ic] + bc3[iiy + 4 + ic] + bc3[iiy + 2048 + ic] + bc3[iiy
                            + 2048 + 4 + ic]) >> 2;
                    bc2++;
                }
            }
        }
    } else if(xm == 2 && ym == 1) {
        uchar8 *bc2, *bc3;
        sint    ic, iiy;

        bc2 = reinterpret_cast<uchar8 *>(buf2);
        bc3 = reinterpret_cast<uchar8 *>(buf3);

        for(iy = 0; iy < 256; iy++) {
            iiy = iy << 11;

            for(ix = 0; ix < 2048; ix += 8) {
                for(ic = ix; ic < (ix + 4); ic++) {
                    *bc2 = (bc3[iiy + ic] + bc3[iiy + 4 + ic]) >> 1;
                    bc2++;
                }
            }
        }
    } else {
        for(iy = 0; iy < 256; iy++) {
            for(ix = 0; ix < 256; ix++) {
                buf2[(iy << 8) + ix] = buf3[((iy * ym) << ll) + (ix * xm)];
            }
        }
    }
}

/*
==================
idClientCinemaSystemLocal::CinemaDrawCinematic
==================
*/
void idClientCinemaSystemLocal::CinemaDrawCinematic(sint handle) {
    float32 x, y, w, h, xscale, yscale;
    uchar8    *buf;

    if(handle < 0 || handle >= MAX_VIDEO_HANDLES ||
            cinTable[handle].status == FMV_EOF) {
        return;
    }

    if(!cinTable[handle].buf) {
        return;
    }

    x = cinTable[handle].xpos;
    y = cinTable[handle].ypos;
    w = cinTable[handle].width;
    h = cinTable[handle].height;
    buf = cinTable[handle].buf;

    xscale = yscale = cls.glconfig.vidHeight / 480.0;
    w *= xscale;
    h *= yscale;
    x = 0.5 * (cls.glconfig.vidWidth - w);
    y *= yscale;

    if(cinTable[handle].letterBox) {
        float32 barheight;
        float32 vh;
        vh = static_cast<float32>(cls.glconfig.vidHeight);

        barheight = (static_cast<float32>(LETTERBOX_OFFSET) / 480.0f) *
                    vh;     //----(SA)    added

        renderSystem->SetColor(&colorBlack[0]);
        //      renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, LETTERBOX_OFFSET, 0, 0, 0, 0, cls.whiteShader );
        //      renderSystem->DrawStretchPic( 0, SCREEN_HEIGHT-LETTERBOX_OFFSET, SCREEN_WIDTH, LETTERBOX_OFFSET, 0, 0, 0, 0, cls.whiteShader );
        //----(SA)  adjust for 640x480
        renderSystem->DrawStretchPic(0, 0, w, barheight, 0, 0, 0, 0,
                                     cls.whiteShader);
        renderSystem->DrawStretchPic(0, vh - barheight - 1, w, barheight + 1, 0, 0,
                                     0, 0, cls.whiteShader);
    }

    if(cinTable[handle].dirty &&
            (cinTable[handle].CIN_WIDTH != cinTable[handle].drawX ||
             cinTable[handle].CIN_HEIGHT != cinTable[handle].drawY)) {
        sint *buf2;

        buf2 = static_cast< sint *>(Hunk_AllocateTempMemory(256 * 256 * 4));

        ResampleCinematic(handle, buf2);

        renderSystem->DrawStretchRaw(x, y, w, h, 256, 256,
                                     reinterpret_cast<uchar8 *>(buf2), handle, true);
        cinTable[handle].dirty = false;
        Hunk_FreeTempMemory(buf2);
        return;
    }

    renderSystem->DrawStretchRaw(x, y, w, h, cinTable[handle].drawX,
                                 cinTable[handle].drawY, buf, handle, cinTable[handle].dirty);
    cinTable[handle].dirty = false;
}

/*
==============
idClientCinemaSystemLocal::PlayCinematic_f
==============
*/
void idClientCinemaSystemLocal::PlayCinematic_f(void) {
    valueType    *arg, *s;
    sint bits = CIN_system;

    // don't allow this while on server
    if(cls.state > CA_DISCONNECTED && cls.state <= CA_ACTIVE) {
        return;
    }

    if(developer->integer) {
        Com_Printf("idClientCinemaSystemLocal::PlayCinematic_f\n");
    }

    if(cls.state == CA_CINEMATIC) {
        clientCinemaSystem->StopCinematic();
    }

    arg = cmdSystem->Argv(1);
    s = cmdSystem->Argv(2);

    if((s && s[0] == '1') || Q_stricmp(arg, "demoend.roq") == 0 ||
            Q_stricmp(arg, "end.roq") == 0) {
        bits |= CIN_hold;
    }

    if(s && s[0] == '2') {
        bits |= CIN_loop;
    }

    if(s && s[0] == '3') {
        bits |= CIN_letterBox;
    }

    soundSystem->StopAllSounds();

    if(bits) {
        CL_handle = clientCinemaSystem->PlayCinematic(arg, 0, LETTERBOX_OFFSET,
                    SCREEN_WIDTH,
                    SCREEN_HEIGHT - (LETTERBOX_OFFSET * 2), bits);
    } else {
        CL_handle = clientCinemaSystem->PlayCinematic(arg, 0, 0, SCREEN_WIDTH,
                    SCREEN_HEIGHT,
                    bits);
    }

    if(CL_handle >= 0) {
        do {
            clientCinemaSystem->RunCinematic();
        } while(cinTable[currentHandle].buf == nullptr &&

                // wait for first frame (load codebook and sound)
                cinTable[currentHandle].status ==
                FMV_PLAY);
    }
}

/*
==============
idClientCinemaSystemLocal::DrawCinematic
==============
*/
void idClientCinemaSystemLocal::DrawCinematic(void) {
    if(CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES) {
        CinemaDrawCinematic(CL_handle);
    }
}

/*
==============
idClientCinemaSystemLocal::RunCinematic
==============
*/
void idClientCinemaSystemLocal::RunCinematic(void) {
    if(CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES) {
        CinemaRunCinematic(CL_handle);
    }
}

/*
==============
idClientCinemaSystemLocal::StopCinematic
==============
*/
void idClientCinemaSystemLocal::StopCinematic(void) {
    if(CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES) {
        CinemaStopCinematic(CL_handle);
        soundSystem->StopAllSounds();
        CL_handle = -1;
    }
}

/*
==============
idClientCinemaSystemLocal::UploadCinematic
==============
*/
void idClientCinemaSystemLocal::UploadCinematic(sint handle) {
    if(handle >= 0 && handle < MAX_VIDEO_HANDLES) {
        if(!cinTable[handle].buf) {
            return;
        }

        if(cinTable[handle].playonwalls <= 0 && cinTable[handle].dirty) {
            if(cinTable[handle].playonwalls == 0) {
                cinTable[handle].playonwalls = -1;
            } else {
                if(cinTable[handle].playonwalls == -1) {
                    cinTable[handle].playonwalls = -2;
                } else {
                    cinTable[handle].dirty = false;
                }
            }
        }

        // Resample the video if needed
        if(cinTable[handle].dirty &&
                (cinTable[handle].CIN_WIDTH != cinTable[handle].drawX ||
                 cinTable[handle].CIN_HEIGHT != cinTable[handle].drawY)) {
            sint *buf2;

            buf2 = static_cast<sint *>(Hunk_AllocateTempMemory(256 * 256 * 4));

            ResampleCinematic(handle, buf2);

            renderSystem->UploadCinematic(cinTable[handle].CIN_WIDTH,
                                          cinTable[handle].CIN_HEIGHT, 256, 256, reinterpret_cast<uchar8 *>(buf2),
                                          handle, true);
            cinTable[handle].dirty = false;
            Hunk_FreeTempMemory(buf2);
        } else {
            // Upload video at normal resolution
            renderSystem->UploadCinematic(cinTable[handle].CIN_WIDTH,
                                          cinTable[handle].CIN_HEIGHT, cinTable[handle].drawX,
                                          cinTable[handle].drawY,
                                          cinTable[handle].buf, handle, cinTable[handle].dirty);
            cinTable[handle].dirty = false;
        }

        if(cl_inGameVideo->integer == 0 && cinTable[handle].playonwalls == 1) {
            cinTable[handle].playonwalls--;
        } else if(cl_inGameVideo->integer != 0 &&
                  cinTable[handle].playonwalls != 1) {
            cinTable[handle].playonwalls = 1;
        }
    }
}
