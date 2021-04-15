////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2005 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2005 Stuart Dalton(badcdev@gmail.com)
// Copyright(C) 2011 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   audio_al_buffers.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <soundSystemAL/sndSystemAL_precompiled.hpp>

typedef struct sfx_s sfx_t;
struct sfx_s {
    valueType filename[MAX_QPATH];
    ALuint buffer; // OpenAL buffer
    bool isDefault; // Couldn't be loaded - use default FX
    bool inMemory; // Sound is stored in memory
    bool isLocked; // Sound is locked (can not be unloaded)
    sint used; // Time last used
    sfx_t *next; // Next entry in hash list
    sint duration;
    snd_info_t info; // information for this sound like rate, sample count..
};

static bool al_buffer_inited = false;

/**
 * Sound effect storage, data structures
 */
#define MAX_SFX 4096
static sfx_t knownSfx[MAX_SFX];
static sint numSfx;

static sfxHandle_t default_sfx;

/*
=================
idAudioOpenALSystemLocal::buf_find_free
=================
*/
sfxHandle_t idAudioOpenALSystemLocal::buf_find_free(void) {
    sint i;

    for(i = 0; i < MAX_SFX; i++) {
        // Got one
        if(knownSfx[i].filename[0] == '\0') {
            return i;
        }
    }

    // Shit...
    trap_Error(ERR_FATAL,
               "idAudioOpenALSystemLocal::buf_find_free: No free sound handles");
    return -1;
}

/*
=================
idAudioOpenALSystemLocal::buf_find

Find a sound effect if loaded, set up a handle otherwise
=================
*/
sfxHandle_t idAudioOpenALSystemLocal::buf_find(pointer filename) {
    // Look it up in the hash table
    sfxHandle_t sfx = -1;
    sint i;

    for(i = 0; i < MAX_SFX; i++) {
        if(!strcmp(knownSfx[i].filename, filename)) {
            sfx = i;
            break;
        }
    }

    // Not found in hash table?
    if(sfx == -1) {
        sfx_t *ptr;

        sfx = buf_find_free();

        // Clear and copy the filename over
        ptr = &knownSfx[sfx];

        ::memset(ptr, 0, sizeof(*ptr));
        ::strcpy(ptr->filename, filename);

        numSfx++;
    }

    // Return the handle
    return sfx;
}

/*
=================
idAudioOpenALSystemLocal::buf_init

Initialisation and shutdown
Called at init, shutdown
=================
*/
bool idAudioOpenALSystemLocal::buf_init(void) {
    sfxHandle_t default_sfx;

    if(al_buffer_inited) {
        return true;
    }

    // Clear the hash table, and SFX table
    ::memset(knownSfx, 0, sizeof(knownSfx));
    numSfx = 0;

    // Load the default sound, and lock it
    default_sfx = buf_find("sound/feedback/hit.wav");
    buf_use(default_sfx);
    knownSfx[default_sfx].isLocked = true;

    // All done
    al_buffer_inited = true;
    return true;
}

/*
=================
idAudioOpenALSystemLocal::buf_shutdown
=================
*/
void idAudioOpenALSystemLocal::buf_shutdown(void) {
    sint i;

    if(!al_buffer_inited) {
        return;
    }

    // Unlock the default sound effect
    knownSfx[default_sfx].isLocked = false;

    // Free all used effects
    for(i = 0; i < MAX_SFX; i++) {
        buf_unload(i);
    }

    // Clear the tables
    ::memset(knownSfx, 0, sizeof(knownSfx));

    // All undone
    al_buffer_inited = false;
}

/*
=================
idAudioOpenALSystemLocal::RegisterSound

Registration
=================
*/
sfxHandle_t idAudioOpenALSystemLocal::RegisterSound(pointer sample,
        bool compressed) {
    sfxHandle_t sfx = buf_find(sample);

    if((s_precache->integer == 1) && (!knownSfx[sfx].inMemory) &&
            (!knownSfx[sfx].isDefault)) {
        buf_load(sfx);
    }

    knownSfx[sfx].used = idsystem->Milliseconds();

    return sfx;
}

/*
=================
idAudioOpenALSystemLocal::buf_load

Usage counter
=================
*/
void idAudioOpenALSystemLocal::buf_use(sfxHandle_t sfx) {
    if(knownSfx[sfx].filename[0] == '\0') {
        return;
    }

    if((!knownSfx[sfx].inMemory) && (!knownSfx[sfx].isDefault)) {
        buf_load(sfx);
    }

    knownSfx[sfx].used = idsystem->Milliseconds();
}

/*
=================
idAudioOpenALSystemLocal::buf_load

Loading and unloading
=================
*/
void idAudioOpenALSystemLocal::buf_use_default(sfxHandle_t sfx) {
    if(sfx == default_sfx) {
        trap_Error(ERR_FATAL, "Can't load default sound effect %s\n",
                   knownSfx[sfx].filename);
    }

    trap_Printf(PRINT_ALL, "Warning: Using default sound for %s\n",
                knownSfx[sfx].filename);

    knownSfx[sfx].isDefault = true;
    knownSfx[sfx].buffer = knownSfx[default_sfx].buffer;
}

/*
=================
idAudioOpenALSystemLocal::buf_load
=================
*/
void idAudioOpenALSystemLocal::buf_load(sfxHandle_t sfx) {
    ALenum error;

    void *data;
    snd_info_t info;
    ALuint fmt;

    // Nothing?
    if(knownSfx[sfx].filename[0] == '\0') {
        return;
    }

    // Player SFX
    if(knownSfx[sfx].filename[0] == '*') {
        return;
    }

    // Already done?
    if((knownSfx[sfx].inMemory) || (knownSfx[sfx].isDefault)) {
        return;
    }

    // Try to load
    data = soundSystem->codec_load(knownSfx[sfx].filename, &info);

    if(!data) {
        trap_Printf(PRINT_ALL, "Can't load %s\n", knownSfx[sfx].filename);
        buf_use_default(sfx);
        return;
    }

    fmt = format(info.width, info.channels);

    // Create a buffer
    qalGenBuffers(1, &knownSfx[sfx].buffer);

    if((error = qalGetError()) != AL_NO_ERROR) {
        buf_use_default(sfx);

        trap_Hunk_FreeTempMemory(data);
        trap_Printf(PRINT_ALL, "Can't create a sound buffer for %s - %s\n",
                    knownSfx[sfx].filename, errormsg(error));
        return;
    }

    // Fill the buffer
    qalGetError();
    qalBufferData(knownSfx[sfx].buffer, fmt, data, info.size, info.rate);
    error = qalGetError();

    // If we ran out of memory, start evicting the least recently used sounds
    while(error == AL_OUT_OF_MEMORY) {
        bool rv = buf_evict();

        if(!rv) {
            buf_use_default(sfx);

            trap_Hunk_FreeTempMemory(data);
            trap_Printf(PRINT_ALL, "Out of memory loading %s\n",
                        knownSfx[sfx].filename);
            return;
        }

        // Try load it again
        qalGetError();
        qalBufferData(knownSfx[sfx].buffer, fmt, data, info.size, info.rate);
        error = qalGetError();
    }

    // Some other error condition
    if(error != AL_NO_ERROR) {
        buf_use_default(sfx);

        trap_Hunk_FreeTempMemory(data);
        trap_Printf(PRINT_ALL, "Can't fill sound buffer for %s - %s",
                    knownSfx[sfx].filename, errormsg(error));
        return;
    }

    // Free the memory
    trap_Hunk_FreeTempMemory(data);

    // Woo!
    knownSfx[sfx].inMemory = true;
}

/*
=================
idAudioOpenALSystemLocal::duration
=================
*/
sint idAudioOpenALSystemLocal::duration(sfxHandle_t sfx) {
    if(sfx < 0 || sfx >= numSfx) {
        trap_Printf(PRINT_ALL,
                    "ERROR: idAudioOpenALSystemLocal::SoundDuration: handle %i out of range\n",
                    sfx);
        return 0;
    }

    return knownSfx[sfx].duration;
}

/*
=================
idAudioOpenALSystemLocal::buf_unload
=================
*/
void idAudioOpenALSystemLocal::buf_unload(sfxHandle_t sfx) {
    ALenum error;

    if(knownSfx[sfx].filename[0] == '\0') {
        return;
    }

    if(!knownSfx[sfx].inMemory) {
        return;
    }

    // Delete it
    qalDeleteBuffers(1, &knownSfx[sfx].buffer);

    if((error = qalGetError()) != AL_NO_ERROR) {
        trap_Printf(PRINT_ALL, "Can't delete sound buffer for %s",
                    knownSfx[sfx].filename);
    }

    knownSfx[sfx].inMemory = false;
}

/*
=================
idAudioOpenALSystemLocal::buf_evict
=================
*/
bool idAudioOpenALSystemLocal::buf_evict(void) {
    // Doesn't work yet, so if OpenAL reports that you're out of memory, you'll just get
    // "Catastrophic sound memory exhaustion". Whoops.
    return false;
}

/*
=================
idAudioOpenALSystemLocal::buf_get

Buffer grabbage
=================
*/
ALuint idAudioOpenALSystemLocal::buf_get(sfxHandle_t sfx) {
    return knownSfx[sfx].buffer;
}

/*
=================
idAudioOpenALSystemLocal::buf_get_name
=================
*/
valueType *idAudioOpenALSystemLocal::buf_get_name(sfxHandle_t sfx) {
    return knownSfx[sfx].filename;
}

/*
======================
idAudioOpenALSystemLocal::GetSoundLength

Returns how long the sound lasts in milliseconds
======================
*/
sint idAudioOpenALSystemLocal::GetSoundLength(sfxHandle_t sfxHandle) {
    if(sfxHandle < 0 || sfxHandle >= numSfx) {
        trap_Printf(PRINT_WARNING, "S_StartSound: handle %i out of range\n",
                    sfxHandle);
        return -1;
    }

    return (sint)(((float32)knownSfx[sfxHandle].info.samples /
                   (float32)knownSfx[sfxHandle].info.rate) * 1000.0f);
}

/*
======================
idAudioOpenALSystemLocal::GetCurrentSoundTime

Returns how long the sound lasts in milliseconds
======================
*/
sint idAudioOpenALSystemLocal::GetCurrentSoundTime(void) {
    return idsystem->Milliseconds();
}