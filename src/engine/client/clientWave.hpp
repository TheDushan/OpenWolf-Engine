////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2021 - 2023 Dusan Jocic <dusanjocic@msn.com>
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
// along with OpenWolf Source Code. If not, see <http://www.gnu.org/licenses/>.
//
// -------------------------------------------------------------------------------------
// File name:   clientWave.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTWAVE_HPP__
#define __CLIENTWAVE_HPP__

typedef struct wav_hdr_s {
    uint    ChunkID;    // big endian
    uint    ChunkSize;  // little endian
    uint    Format;     // big endian

    uint    Subchunk1ID;    // big endian
    uint    Subchunk1Size;  // little endian
    uchar16  AudioFormat;   // little endian
    uchar16  NumChannels;   // little endian
    uint    SampleRate; // little endian
    uint    ByteRate;   // little endian
    uchar16  BlockAlign;    // little endian
    uchar16  BitsPerSample; // little endian

    uint    Subchunk2ID;    // big endian
    uint    Subchunk2Size;  // little indian ;)

    uint    NumSamples;
} wav_hdr_t;

static wav_hdr_t hdr;

static valueType     wavName[MAX_QPATH];    // compiler bug workaround

//
// idClientWaveSystemLocal
//
class idClientWaveSystemLocal {
public:
    idClientWaveSystemLocal();
    ~idClientWaveSystemLocal();

    static void WavFilename(sint number, valueType *fileName);
    static void WriteWaveHeader(void);
    static void WriteWaveOpen(void);
    static void WriteWaveClose(void);
    static void WavRecord_f(void);
    static void WavStopRecord_f(void);
};

extern idClientWaveSystemLocal clientWaveLocal;

#endif //__CLIENTWAVE_HPP__