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
// File name:   clientCinema.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description: video and cinematic playback
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENCINEMA_HPP__
#define __CLIENCINEMA_HPP__

#define MAXSIZE             8
#define MINSIZE             4

#define DEFAULT_CIN_WIDTH   512
#define DEFAULT_CIN_HEIGHT  512

#define LETTERBOX_OFFSET 105

#define ROQ_QUAD            0x1000
#define ROQ_QUAD_INFO       0x1001
#define ROQ_CODEBOOK        0x1002
#define ROQ_QUAD_VQ         0x1011
#define ROQ_QUAD_JPEG       0x1012
#define ROQ_QUAD_HANG       0x1013
#define ROQ_PACKET          0x1030
#define ZA_SOUND_MONO       0x1020
#define ZA_SOUND_STEREO     0x1021

#define MAX_VIDEO_HANDLES   16

extern sint s_soundtime;

#define CIN_STREAM 0    //DAJ const for the sound stream used for cinematics


/******************************************************************************
*
* Class:        trFMV
*
* Description:  RoQ/RnR manipulation routines
*               not entirely complete for first run
*
******************************************************************************/

static sint32 ROQ_YY_tab[256];
static sint32 ROQ_UB_tab[256];
static sint32 ROQ_UG_tab[256];
static sint32 ROQ_VG_tab[256];
static sint32 ROQ_VR_tab[256];

typedef struct {
    uchar8 linbuf[DEFAULT_CIN_WIDTH * DEFAULT_CIN_HEIGHT * 4 * 2];
    uchar8 file[65536];
    schar16 sqrTable[256];

    sint    mcomp[256];
    uchar8                *qStatus[2][32768];

    sint32 oldXOff, oldYOff, oldysize, oldxsize;

    sint currentHandle;
} cinematics_t;

typedef struct {
    valueType fileName[MAX_OSPATH];
    sint CIN_WIDTH, CIN_HEIGHT;
    sint xpos, ypos, width, height;
    bool looping, holdAtEnd, dirty, alterGameState, silent, shader, letterBox,
         sound;
    fileHandle_t iFile;
    e_status status;
    sint startTime;
    sint lastTime;
    sint32 tfps;
    sint32 RoQPlayed;
    sint32 ROQSize;
    uint RoQFrameSize;
    sint32 onQuad;
    sint32 numQuads;
    sint32 samplesPerLine;
    uint roq_id;
    sint32 screenDelta;

    void (*VQ0)(uchar8 *status, void *qdata);
    void (*VQ1)(uchar8 *status, void *qdata);
    void (*VQNormal)(uchar8 *status, void *qdata);
    void (*VQBuffer)(uchar8 *status, void *qdata);

    sint32 samplesPerPixel;                               // defaults to 2
    uchar8               *gray;
    uint xsize, ysize, maxsize, minsize;

    bool half, smootheddouble;
    sint inMemory;
    sint32 normalBuffer0;
    sint32 roq_flags;
    sint32 roqF0;
    sint32 roqF1;
    sint32 t[2];
    sint32 roqFPS;
    sint playonwalls;
    uchar8               *buf;
    sint32 drawX, drawY;
    uchar16 vq2[256 * 16 * 4];
    uchar16 vq4[256 * 64 * 4];
    uchar16 vq8[256 * 256 * 4];
} cin_cache;

#define VQ2TO4( a,b,c,d ) { \
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

#define VQ2TO2( a,b,c,d ) { \
        *c++ = *a;  \
        *d++ = *a;  \
        *d++ = *a;  \
        *c++ = *b;  \
        *d++ = *b;  \
        *d++ = *b;  \
        *d++ = *a;  \
        *d++ = *a;  \
        *d++ = *b;  \
        *d++ = *b;  \
        a++; b++; }


static cinematics_t cin;
static cin_cache cinTable[MAX_VIDEO_HANDLES];
static sint currentHandle = -1;
static sint CL_handle = -1;


//
// idClientCinemaSystemLocal
//
class idClientCinemaSystemLocal : public idClientCinemaSystem {
public:
    idClientCinemaSystemLocal();
    ~idClientCinemaSystemLocal();

    virtual sint PlayCinematic(pointer arg, sint x, sint y, sint w, sint h,
                               sint systemBits);
    virtual void SetExtents(sint handle, sint x, sint y, sint w, sint h);
    virtual void SetLooping(sint handle, bool loop);
    virtual void ResampleCinematic(sint handle, sint *buf2);
    virtual void CinemaDrawCinematic(sint handle);
    virtual void DrawCinematic(void);
    virtual void RunCinematic(void);
    virtual void StopCinematic(void);
    virtual void UploadCinematic(sint handle);
    virtual void CloseAllVideos(void);
    virtual e_status CinemaRunCinematic(sint handle);
    virtual e_status CinemaStopCinematic(sint handle);

    static void PlayCinematic_f(void);
    static sint HandleForVideo(void);
    static void RllSetupTable(void);
    sint32 RllDecodeMonoToMono(uchar8 *from, schar16 *to, uint size,
                               valueType signedOutput, uchar16 flag);
    sint32 RllDecodeMonoToStereo(uchar8 *from, schar16 *to, uint size,
                                 valueType signedOutput, uchar16 flag);
    sint32 RllDecodeStereoToStereo(uchar8 *from, schar16 *to, uint size,
                                   valueType signedOutput, uchar16 flag);
    sint32 RllDecodeStereoToMono(uchar8 *from, schar16 *to, uint size,
                                 valueType signedOutput, uchar16 flag);
    static void move8_32(uchar8 *src, uchar8 *dst, sint spl);
    static void move4_32(uchar8 *src, uchar8 *dst, sint spl);
    static void blit8_32(uchar8 *src, uchar8 *dst, sint spl);
    static void blit4_32(uchar8 *src, uchar8 *dst, sint spl);
    static void blit2_32(uchar8 *src, uchar8 *dst, sint spl);
    static void blitVQQuad32fs(uchar8 **status, uchar8 *data);
    static void ROQ_GenYUVTables(void);
    static uchar16 yuv_to_rgb(sint32 y, sint32 u, sint32 v);
    static void yuv_to_rgb24(sint32 y, sint32 u, sint32 v, uchar8 *out);
    static void decodeCodeBook(uchar8 *input);
    static void recurseQuad(sint32 startX, sint32 startY, sint32 quadSize,
                            sint32 xOff, sint32 yOff);
    static void setupQuad(sint32 xOff, sint32 yOff);
    static void readQuadInfo(uchar8 *qData);
    static void RoQPrepMcomp(sint32 xoff, sint32 yoff);
    static void initRoQ(void);
    static void RoQReset(void);
    static void RoQInterrupt(void);
    static void RoQ_init(void);
    static void RoQShutdown(void);
};

extern idClientCinemaSystemLocal clientCinemaLocal;

#endif // !__CLIENCINEMA_HPP__