////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2005 id Software LLC, a ZeniMax Media company.
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
// File name:   sndSystemAL_music.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <soundSystemAL/sndSystemAL_precompiled.hpp>

#define BUFFERS 4
#define BUFFER_SIZE 4096

static bool mus_playing = false;
static srcHandle_t source_handle = -1;
static ALuint source;
static ALuint buffers[BUFFERS];

static snd_stream_t *mus_stream;
static valueType s_backgroundLoop[MAX_QPATH];

static uchar8 decode_buffer[BUFFER_SIZE];

/*
=================
idAudioOpenALSystemLocal::mus_source_get

Source aquire / release
=================
*/
void idAudioOpenALSystemLocal::mus_source_get(void) {
    // Allocate a source at high priority
    source_handle = src_alloc(SRCPRI_STREAM, -2, 0);

    if(source_handle == -1) {
        return;
    }

    // Lock the source so nobody else can use it, and get the raw source
    src_lock(source_handle);
    source = src_get(source_handle);

    // Set some source parameters
    qalSource3f(source, AL_POSITION,        0.0, 0.0, 0.0);
    qalSource3f(source, AL_VELOCITY,        0.0, 0.0, 0.0);
    qalSource3f(source, AL_DIRECTION,       0.0, 0.0, 0.0);
    qalSourcef(source, AL_ROLLOFF_FACTOR,  0.0);
    qalSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
}

/*
=================
idAudioOpenALSystemLocal::mus_source_free
=================
*/
void idAudioOpenALSystemLocal::mus_source_free(void) {
    // Release the output source
    src_unlock(source_handle);

    source = 0;
    source_handle = -1;
}

/*
=================
idAudioOpenALSystemLocal::mus_process
=================
*/
bool idAudioOpenALSystemLocal::mus_process(ALuint b) {
    sint l;
    ALuint fmt;

    l = soundSystem->codec_read(mus_stream, BUFFER_SIZE, decode_buffer);

    if(l == 0) {
        soundSystem->codec_close(mus_stream);
        mus_stream = soundSystem->codec_open(s_backgroundLoop);

        if(!mus_stream) {
            soundOpenALSystem->StopBackgroundTrack();
            return false;
        }

        l = soundSystem->codec_read(mus_stream, BUFFER_SIZE, decode_buffer);

        if(!l) {
            return false;
        }
    }

    fmt = format(mus_stream->info.width, mus_stream->info.channels);
    qalBufferData(b, fmt, decode_buffer, l, mus_stream->info.rate);

    return true;
}

/*
=================
idAudioOpenALSystemLocal::StartBackgroundTrack

Background music playback
=================
*/
void idAudioOpenALSystemLocal::StartBackgroundTrack(pointer intro,
        pointer loop) {
    sint i;

    // Stop any existing music that might be playing
    StopBackgroundTrack();

    if(!intro || !intro[0]) {
        intro = loop;
    }

    if(!loop || !loop[0]) {
        loop = intro;
    }

    if((!intro || !intro[0]) && (!intro || !intro[0])) {
        return;
    }

    // Copy the loop over
    ::strncpy(s_backgroundLoop, loop, sizeof(s_backgroundLoop));

    // Open the intro
    mus_stream = soundSystem->codec_open(intro);

    if(!mus_stream) {
        return;
    }

    // Allocate a source
    mus_source_get();

    if(source_handle == -1) {
        return;
    }

    // Generate the buffers
    qalGenBuffers(BUFFERS, buffers);

    // Queue the buffers up
    for(i = 0; i < BUFFERS; i++) {
        mus_process(buffers[i]);
    }

    qalSourceQueueBuffers(source, BUFFERS, buffers);

    // Start playing
    qalSourcePlay(source);

    mus_playing = true;
}

/*
=================
idAudioOpenALSystemLocal::StopBackgroundTrack
=================
*/
void idAudioOpenALSystemLocal::StopBackgroundTrack(void) {
    if(!mus_playing) {
        return;
    }

    // Stop playing
    qalSourceStop(source);

    // De-queue the buffers
    qalSourceUnqueueBuffers(source, BUFFERS, buffers);

    // Destroy the buffers
    qalDeleteBuffers(BUFFERS, buffers);

    // Free the source
    mus_source_free();

    // Unload the stream
    if(mus_stream) {
        soundSystem->codec_close(mus_stream);
    }

    mus_stream = nullptr;

    mus_playing = false;
}

/*
=================
idAudioOpenALSystemLocal::mus_update
=================
*/
void idAudioOpenALSystemLocal::mus_update(void) {
    sint processed;
    ALint state;

    if(!mus_playing) {
        return;
    }

    qalGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

    while(processed--) {
        ALuint b;
        qalSourceUnqueueBuffers(source, 1, &b);

        if(!mus_process(b)) {
            common->Printf("Error processing music data\n");
            soundOpenALSystemLocal.StopBackgroundTrack();
            return;
        }

        qalSourceQueueBuffers(source, 1, &b);
    }

    // If it's not still playing, give it a kick
    qalGetSourcei(source, AL_SOURCE_STATE, &state);

    if(state == AL_STOPPED) {
        qalSourcePlay(source);
    }

    // Set the gain property
    if(s_volume->modified) {
        qalSourcef(source, AL_GAIN, s_gain->value * s_musicVolume->value);
    }
}
