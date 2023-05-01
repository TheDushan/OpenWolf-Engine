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
// File name:   soundSystem_api.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SOUNDSYSTEM_API_HPP__
#define __SOUNDSYSTEM_API_HPP__

#define SND_API_VERSION 1

#if defined (GAMEDLL) || defined (CGAMEDLL) || defined (GUI)
typedef struct snd_info_s snd_info_t;
typedef struct snd_codec_s snd_codec_t;
typedef struct snd_stream_s snd_stream_t;
#endif

//
// idSoundSystem
//
class idSoundSystem {
public:
    virtual void Init(void) = 0;
    virtual void Shutdown(void) = 0;
    // if origin is nullptr, the sound will be dynamically sourced from the entity
    virtual void StartSound(vec3_t origin, sint entnum, sint entchannel,
                            sfxHandle_t sfx) = 0;
    virtual void StartLocalSound(sfxHandle_t sfx, sint channelNum) = 0;
    virtual void StartBackgroundTrack(pointer intro, pointer loop) = 0;
    virtual void StopBackgroundTrack(void) = 0;
    // cinematics and voice-over-network will send raw samples
    // 1.0 volume will be direct output of source samples
    virtual void RawSamples(sint stream, sint samples, sint rate, sint width,
                            sint channels, const uchar8 *data, float32 volume, sint entityNum) = 0;
    // stop all sounds and the background track
    virtual void StopAllSounds(void) = 0;
    // all continuous looping sounds must be added before calling S_Update
    virtual void ClearLoopingSounds(bool killall) = 0;
    virtual void AddLoopingSound(sint entityNum, const vec3_t origin,
                                 const vec3_t velocity, sfxHandle_t sfx) = 0;
    virtual void AddRealLoopingSound(sint entityNum, const vec3_t origin,
                                     const vec3_t velocity, sfxHandle_t sfx) = 0;
    virtual void StopLoopingSound(sint entityNum) = 0;
    // recompute the reletive volumes for all running sounds
    // reletive to the given entityNum / orientation
    virtual void Respatialize(sint entityNum, const vec3_t origin,
                              vec3_t axis[3], sint inwater) = 0;
    // let the sound system know where an entity currently is
    virtual void UpdateEntityPosition(sint entityNum, const vec3_t origin) = 0;
    virtual void Update(void) = 0;
    virtual void DisableSounds(void) = 0;
    virtual void BeginRegistration(void) = 0;
    // RegisterSound will allways return a valid sample, even if it
    // has to create a placeholder.  This prevents continuous filesystem
    // checks for missing files
    virtual sfxHandle_t RegisterSound(pointer sample, bool compressed) = 0;
    virtual void DisplayFreeMemory(void) = 0;
    virtual void ClearSoundBuffer(void) = 0;
    virtual sint SoundDuration(sfxHandle_t handle) = 0;
    virtual sint GetSoundLength(sfxHandle_t sfxHandle) = 0;
    virtual void Reload(void) = 0;
    virtual sint GetCurrentSoundTime(void) = 0;
    virtual void *codec_load(pointer filename, snd_info_t *info) = 0;
    virtual snd_stream_t *codec_open(pointer filename) = 0;
    virtual void codec_close(snd_stream_t *stream) = 0;
    virtual sint codec_read(snd_stream_t *stream, sint bytes,
                            void *buffer) = 0;
    virtual void SndPrintf(sint print_level, pointer fmt, ...) = 0;
};

extern idSoundSystem *soundSystem;

class idAudioOpenALSystem {
public:
    virtual bool Init(void) = 0;
    virtual void Shutdown(void) = 0;
    virtual void StartSound(vec3_t origin, sint entnum, sint entchannel,
                            sfxHandle_t sfx) = 0;
    virtual void StartLocalSound(sfxHandle_t sfx, sint channelNum) = 0;
    virtual void StartBackgroundTrack(pointer intro, pointer loop) = 0;
    virtual void StopBackgroundTrack(void) = 0;
    virtual void RawSamples(sint stream, sint samples, sint rate, sint width,
                            sint channels, const uchar8 *data, float32 volume, sint entityNum) = 0;
    virtual void StopAllSounds(void) = 0;
    virtual void ClearLoopingSounds(bool killall) = 0;
    virtual void AddLoopingSound(sint entityNum, const vec3_t origin,
                                 const vec3_t velocity, sfxHandle_t sfx) = 0;
    virtual void AddRealLoopingSound(sint entityNum, const vec3_t origin,
                                     const vec3_t velocity, sfxHandle_t sfx) = 0;
    virtual void StopLoopingSound(sint entityNum) = 0;
    virtual void Respatialize(sint entityNum, const vec3_t origin,
                              vec3_t axis[3], sint inwater) = 0;
    virtual void UpdateEntityPosition(sint entityNum, const vec3_t origin) = 0;
    virtual void Update(void) = 0;
    virtual void DisableSounds(void) = 0;
    virtual void BeginRegistration(void) = 0;
    virtual sfxHandle_t RegisterSound(pointer sample, bool compressed) = 0;
    virtual void ClearSoundBuffer(void) = 0;
    virtual sint SoundDuration(sfxHandle_t sfx) = 0;
    virtual sint GetVoiceAmplitude(sint entnum) = 0;
    virtual sint GetSoundLength(sfxHandle_t sfxHandle) = 0;
    virtual sint GetCurrentSoundTime(void) = 0;
};

extern idAudioOpenALSystem *soundOpenALSystem;

// Imported functions
typedef struct openALImports_s {
#if !defined (DEDICATED) || !defined (GAMEDLL) || !defined (CGAMEDLL) || !defined (GUI)
    idAudioOpenALSystem *soundOpenALSystem;
    idSoundSystem *soundSystem;
    idFileSystem *fileSystem;
    idCVarSystem *cvarSystem;
    idCmdSystem *cmdSystem;
    idParseSystem *parseSystem;
    idSystem *idsystem;
    idMemorySystem *memorySystem;
    idCommon *common;
#endif
} openALImports_t;

#endif // !__SOUNDSYSTEM_API_HPP__
