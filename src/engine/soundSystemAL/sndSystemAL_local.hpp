////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2005 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2005 Stuart Dalton(badcdev@gmail.com)
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
// File name:   sndSystemAL_local.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description: Local definitions for the OpenAL sound system
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SNDSYSTEM_AL_HPP__
#define __SNDSYSTEM_AL_HPP__

/**
 * Console variables
 */
extern convar_t *s_volume;
extern convar_t *s_musicVolume;
extern convar_t *s_doppler;
extern convar_t *s_khz;
extern convar_t *s_precache;
extern convar_t *s_gain;
extern convar_t *s_sources;
extern convar_t *s_dopplerFactor;
extern convar_t *s_dopplerSpeed;
extern convar_t *s_minDistance;
extern convar_t *s_maxDistance;
extern convar_t *s_rolloff;
extern convar_t *s_alDevice;
extern convar_t *s_alAvailableDevices;
extern convar_t *s_alAvailableInputDevices;
extern convar_t *s_alDriver;
extern convar_t *s_alReverbMix;
extern convar_t *s_alReverbDiffusion;
extern convar_t *s_alReverbDecay;

/**
 * Scaling constants
 */
#define POSITION_SCALE 1

/**
 * Source management
 * Uses a simple priority-based approach to allocate channels
 * Ambient or looping sounds may be safely interrupted, and will not
 * cause loss - they will resume as soon as a channel is available.
 * One-shot sounds will be lost if there are insufficient channels.
 */
#define SRCPRI_AMBIENT  0   // Ambient sound effects
#define SRCPRI_ENTITY   1   // Entity sound effects
#define SRCPRI_ONESHOT  2   // One-shot sounds
#define SRCPRI_LOCAL    3   // Local sounds
#define SRCPRI_STREAM   4   // Streams (music, cutscenes)

#define SRC_NO_ENTITY   -1  // Has no associated entity
#define SRC_NO_CHANNEL  -1  // Has no specific channel

typedef sint srcHandle_t;

/**
 * Engine interface
 */

class idAudioOpenALSystemLocal : public idAudioOpenALSystem {
public:
    idAudioOpenALSystemLocal(void);
    ~idAudioOpenALSystemLocal(void);

    virtual bool Init(void);
    virtual void Shutdown(void);
    virtual void StartSound(vec3_t origin, sint entnum, sint entchannel,
                            sfxHandle_t sfx);
    virtual void StartLocalSound(sfxHandle_t sfx, sint channelNum);
    virtual void StartBackgroundTrack(pointer intro, pointer loop);
    virtual void StopBackgroundTrack(void);
    virtual void RawSamples(sint stream, sint samples, sint rate, sint width,
                            sint channels, const uchar8 *data, float32 volume, sint entityNum);
    virtual void StopAllSounds(void);
    virtual void ClearLoopingSounds(bool killall);
    virtual void AddLoopingSound(sint entityNum, const vec3_t origin,
                                 const vec3_t velocity, sfxHandle_t sfx);
    virtual void AddRealLoopingSound(sint entityNum, const vec3_t origin,
                                     const vec3_t velocity, sfxHandle_t sfx);
    virtual void StopLoopingSound(sint entityNum);
    virtual void Respatialize(sint entityNum, const vec3_t origin,
                              vec3_t axis[3], sint inwater);
    virtual void UpdateEntityPosition(sint entityNum, const vec3_t origin);
    virtual void Update(void);
    virtual void DisableSounds(void);
    virtual void BeginRegistration(void);
    virtual sfxHandle_t RegisterSound(pointer sample, bool compressed);
    virtual void ClearSoundBuffer(void);
    virtual sint SoundDuration(sfxHandle_t sfx);
    virtual sint GetVoiceAmplitude(sint entnum);
    virtual sint GetSoundLength(sfxHandle_t sfxHandle);
    virtual sint GetCurrentSoundTime(void);

    // Called at init, shutdown
    static bool src_init(void);
    static void src_shutdown(void);

    // Finds a free sound source
    // If required, this will kick a sound off
    // If you don't use it immediately, it may still be reallocated
    static srcHandle_t src_alloc(sint priority, sint entnum, sint channel);

    // Finds an active source with matching entity and channel numbers
    // Returns -1 if there isn't one
    static srcHandle_t src_find(sint entnum, sint channel);

    // Locks and unlocks a source
    // Locked sources will not be automatically reallocated or managed
    // Once unlocked, the source may be reallocated again
    static void src_lock(srcHandle_t src);
    static void src_unlock(srcHandle_t src);

    // Entity position management
    static void src_entity_move(sint entnum, const vec3_t origin);

    // Local sound effect
    static void src_local(sfxHandle_t sfx, sint channel);

    // Play a one-shot sound effect
    static void src_play(sfxHandle_t sfx, vec3_t origin, sint entnum,
                         sint entchannel);

    // Start a looping sound effect
    static void src_loop_clear(void);
    static void src_loop(sint priority, sfxHandle_t sfx, const vec3_t origin,
                         const vec3_t velocity, sint entnum);
    static void src_loop_stop(sint entnum);

    // Sound duration
    static sint duration(sfxHandle_t sfx);

    // Update state (move things around, manage sources, and so on)
    static void src_update(void);

    // Shut the hell up
    static void src_shutup(void);

    // Grab the raw source object
    static ALuint src_get(srcHandle_t src);

    /**
     * Music
     */
    static void mus_init(void);
    static void mus_shutdown(void);
    static void mus_update(void);

    /**
     * Stream
     */
    static void stream_raw(sint samples, sint rate, sint width, sint channels,
                           const uchar8 *data, float32 volume);
    static void stream_update(void);
    static void stream_die(void);
    /**
    * Util
    */
    static ALuint format(sint width, sint channels);

    /**
     * Error messages
     */
    static valueType *errormsg(ALenum error);

    /**
     * Buffer management
     */
    // Called at init, shutdown
    static bool buf_init(void);
    static void buf_shutdown(void);
    static sfxHandle_t buf_find_free(void);
    static sfxHandle_t buf_find(pointer filename);

    // Register and unregister sound effects
    // If s_precache is 0, sound files will be loaded when they are used
    // Otherwise, they will be loaded on registration
    static sfxHandle_t buf_register(pointer filename);

    // Set up a sound effect for usage
    // This reloads the sound effect if necessary, and keeps track of it's usage
    static void buf_use(sfxHandle_t sfx);
    static void buf_use_default(sfxHandle_t sfx);

    // Internal use - actually allocates and deallocates the buffers
    static void buf_load(sfxHandle_t sfx);
    static void buf_unload(sfxHandle_t sfx);
    static bool buf_evict(void);

    // Grabs the OpenAL buffer from a sfxHandle_t
    static ALuint buf_get(sfxHandle_t sfx);
    static valueType *buf_get_name(sfxHandle_t sfx);

    static void free_channel(void);
    static void allocate_channel(void);
    static void src_setup(srcHandle_t src, sfxHandle_t sfx, sint priority,
                          sint entity, sint channel, bool local);
    static void src_kill(srcHandle_t src);
    static void mus_source_get(void);
    static void mus_source_free(void);
    static bool mus_process(ALuint b);

    static void Play_f(void);
    static void Music_f(void);

    static bool HearingThroughEntity(sint entityNum);
    static void InitEFX(void);
    static void ShutdownEFX(void);
};

extern idAudioOpenALSystemLocal soundOpenALSystemLocal;


void trap_Printf(sint printLevel, pointer fmt, ...);
void trap_Error(errorParm_t errorLevel, pointer fmt, ...);
void *trap_Hunk_Alloc(uint64 size, ha_pref preference);
void trap_Hunk_FreeTempMemory(void *buf);
void *trap_Malloc(uint64 size);
void trap_Free(void *ptr);

#endif //!__SNDSYSTEMAL_HPP__