////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   clientWave.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: Wave file saving functions
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

idClientWaveSystemLocal clientWaveLocal;

/*
===============
idClientWaveSystemLocal::idClientWaveSystemLocal
===============
*/
idClientWaveSystemLocal::idClientWaveSystemLocal(void) {
}

/*
===============
idClientWaveSystemLocal::~idClientWaveSystemLocal
===============
*/
idClientWaveSystemLocal::~idClientWaveSystemLocal(void) {
}

/*
==================
idClientWaveSystemLocal::WavFilename
==================
*/
void idClientWaveSystemLocal::WavFilename(sint number,
        valueType *fileName) {
    if(number < 0 || number > 9999) {
        Q_vsprintf_s(fileName, MAX_OSPATH, MAX_OSPATH,
                     "wav9999");   // fretn - removed .tga
        return;
    }

    Q_vsprintf_s(fileName, MAX_OSPATH, MAX_OSPATH, "wav%04i", number);
}

void idClientWaveSystemLocal::WriteWaveHeader(void) {
    memset(&hdr, 0, sizeof(hdr));

    hdr.ChunkID = 0x46464952;   // "RIFF"
    hdr.ChunkSize = 0;          // total filesize - 8 bytes
    hdr.Format = 0x45564157;    // "WAVE"

    hdr.Subchunk1ID = 0x20746d66;   // "fmt "
    hdr.Subchunk1Size = 16;     // 16 = pcm
    hdr.AudioFormat = 1;        // 1 = linear quantization
    hdr.NumChannels = 2;        // 2 = stereo

    hdr.SampleRate = dma.speed;

    hdr.BitsPerSample = 16;     // 16bits

    // SampleRate * NumChannels * BitsPerSample/8
    hdr.ByteRate = hdr.SampleRate * hdr.NumChannels * (hdr.BitsPerSample / 8);

    // NumChannels * BitsPerSample/8
    hdr.BlockAlign = hdr.NumChannels * (hdr.BitsPerSample / 8);

    hdr.Subchunk2ID = 0x61746164;   // "data"

    hdr.Subchunk2Size = 0;      // NumSamples * NumChannels * BitsPerSample/8

    // ...
    fileSystem->Write(&hdr.ChunkID, 44, clc.wavefile);
}

/*
==============
idClientWaveSystemLocal::WriteWaveOpen
==============
*/
void idClientWaveSystemLocal::WriteWaveOpen(void) {
    // we will just save it as a 16bit stereo 22050kz pcm file
    sint len;
    valueType name[MAX_OSPATH], *s;

    if(cmdSystem->Argc() > 2) {
        common->Printf("wav_record <wavname>\n");
        return;
    }

    if(clc.waverecording) {
        common->Printf("Already recording a wav file\n");
        return;
    }

    // yes ... no ? leave it up to them imo
    //if (cl_avidemo.integer)
    //  return;

    if(cmdSystem->Argc() == 2) {
        s = cmdSystem->Argv(1);
        Q_strncpyz(wavName, s, sizeof(wavName));
        Q_vsprintf_s(name, sizeof(name), sizeof(name), "wav/%s.wav", wavName);
    } else {
        sint             number;

        // I STOLE THIS
        for(number = 0; number <= 9999; number++) {
            WavFilename(number, wavName);
            Q_vsprintf_s(name, sizeof(name), sizeof(name), "wav/%s.wav", wavName);

            len = fileSystem->FileExists(name);

            if(len <= 0) {
                // file doesn't exist
                break;
            }
        }
    }

    common->Printf("recording to %s.\n", name);
    clc.wavefile = fileSystem->FOpenFileWrite(name);

    if(!clc.wavefile) {
        common->Printf("ERROR: couldn't open %s for writing.\n", name);
        return;
    }

    WriteWaveHeader();
    clc.wavetime = -1;

    clc.waverecording = true;

    cvarSystem->Set("cl_waverecording", "1");
    cvarSystem->Set("cl_wavefilename", wavName);
    cvarSystem->Set("cl_waveoffset", "0");
}

/*
==============
idClientWaveSystemLocal::WriteWaveClose
==============
*/
void idClientWaveSystemLocal::WriteWaveClose(void) {
    common->Printf("Stopped recording\n");

    hdr.Subchunk2Size = hdr.NumSamples * hdr.NumChannels *
                        (hdr.BitsPerSample / 8);
    hdr.ChunkSize = 36 + hdr.Subchunk2Size;

    fileSystem->Seek(clc.wavefile, 4, FS_SEEK_SET);
    fileSystem->Write(&hdr.ChunkSize, 4, clc.wavefile);
    fileSystem->Seek(clc.wavefile, 40, FS_SEEK_SET);
    fileSystem->Write(&hdr.Subchunk2Size, 4, clc.wavefile);

    // and we're outta here
    fileSystem->FCloseFile(clc.wavefile);
    clc.wavefile = 0;
}

/*
==============
idClientWaveSystemLocal::WavRecord_f
==============
*/
void idClientWaveSystemLocal::WavRecord_f(void) {
    if(clc.wavefile) {
        common->Printf("Already recording a wav file\n");
        return;
    }

    WriteWaveOpen();
}

/*
==============
idClientWaveSystemLocal::WavStopRecord_f
==============
*/
void idClientWaveSystemLocal::WavStopRecord_f(void) {
    if(!clc.wavefile) {
        common->Printf("Not recording a wav file\n");
        return;
    }

    WriteWaveClose();
    cvarSystem->Set("cl_waverecording", "0");
    cvarSystem->Set("cl_wavefilename", "");
    cvarSystem->Set("cl_waveoffset", "0");
    clc.waverecording = false;
}

