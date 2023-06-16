////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
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
// File name:   s_local.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SNDSYSTEM_HPP__
#define __SNDSYSTEM_HPP__

#define PAINTBUFFER_SIZE        4096                    // this is in samples

#define SND_CHUNK_SIZE          1024                    // samples
#define SND_CHUNK_SIZE_FLOAT    (SND_CHUNK_SIZE/2)      // floats
#define SND_CHUNK_SIZE_BYTE     (SND_CHUNK_SIZE*2)      // floats

typedef struct {
    sint left;  // the final values will be clamped to +/- 0x00ffff00 and shifted down
    sint right;
} portable_samplepair_t;

typedef struct adpcm_state {
    schar16 sample; /* Previous output value */
    valueType index; /* Index into stepsize table */
} adpcm_state_t;

typedef struct sndBuffer_s {
    schar16 sndChunk[SND_CHUNK_SIZE];
    struct sndBuffer_s *next;
    sint size;
    adpcm_state_t adpcm;
} sndBuffer;

typedef struct sfx_s {
    sndBuffer *soundData;
    bool defaultSound; // couldn't be loaded, so use buzz
    bool inMemory; // not in Memory
    bool soundCompressed; // not in Memory
    sint soundCompressionMethod;
    sint soundLength;
    valueType  soundName[MAX_QPATH];
    sint lastTimeUsed;
    sint duration;
    struct sfx_s *next;
} sfx_t;

typedef struct {
    sint channels;
    sint samples; // mono samples in buffer
    sint fullsamples; // samples with all channels in buffer (samples divided by channels)
    sint submission_chunk; // don't mix less than this #
    sint samplebits;
    sint speed;
    uchar8 *buffer;
    sint isfloat;
} dma_t;

#define START_SAMPLE_IMMEDIATE  0x7fffffff

typedef struct loopSound_s {
    vec3_t origin;
    vec3_t velocity;
    sfx_t *sfx;
    sint mergeFrame;
    bool active;
    bool kill;
    bool doppler;
    float32 dopplerScale;
    float32 oldDopplerScale;
    sint framenum;
} loopSound_t;

typedef struct {
    sint allocTime;
    sint startSample; // START_SAMPLE_IMMEDIATE = set immediately on next mix
    sint entnum; // to allow overriding a specific sound
    sint entchannel; // to allow overriding a specific sound
    sint leftvol; // 0-255 volume after spatialization
    sint rightvol; // 0-255 volume after spatialization
    sint master_vol; // 0-255 volume before spatialization
    float32 dopplerScale;
    float32 oldDopplerScale;
    vec3_t origin; // only use if fixed_origin is set
    bool fixed_origin; // use origin instead of fetching entnum's origin
    sfx_t *thesfx; // sfx structure
    bool doppler;
} channel_t;

#define WAV_FORMAT_PCM      1

typedef struct {
    sint format;
    sint rate;
    sint width;
    sint channels;
    sint samples;
    sint dataofs; // chunk starts this many bytes from file start
} wavinfo_t;


/*
====================================================================

  SYSTEM SPECIFIC FUNCTIONS

====================================================================
*/

// gets the current DMA position
sint SNDDMA_GetDMAPos(void);

// shutdown the DMA xfer.
void SNDDMA_Shutdown(void);

void SNDDMA_BeginPainting(void);

void SNDDMA_Submit(void);

//====================================================================

#define MAX_CHANNELS            96

extern channel_t s_channels[MAX_CHANNELS];
extern channel_t loop_channels[MAX_CHANNELS];
extern sint numLoopChannels;

extern sint s_paintedtime;
extern sint s_rawend;
extern vec3_t listener_forward;
extern vec3_t listener_right;
extern vec3_t listener_up;
extern dma_t dma;

extern uchar8 s_entityTalkAmplitude[MAX_CLIENTS];

#define MAX_RAW_SAMPLES 16384
extern  portable_samplepair_t   s_rawsamples[MAX_RAW_SAMPLES];

bool S_LoadSound(sfx_t *sfx);

void SND_free(sndBuffer *v);
sndBuffer *SND_malloc(void);
void SND_setup(void);
void SND_shutdown(void);

void S_PaintChannels(sint endtime);

void S_memoryLoad(sfx_t *sfx);
portable_samplepair_t *S_GetRawSamplePointer(void);

// spatializes a channel
void S_Spatialize(channel_t *ch);

sint S_GetVoiceAmplitude(sint entityNum);

// adpcm functions
sint  S_AdpcmMemoryNeeded(const wavinfo_t *info);
void S_AdpcmEncodeSound(sfx_t *sfx, schar16 *samples);
void S_AdpcmGetSamples(sndBuffer *chunk, schar16 *to);

// wavelet function

#define SENTINEL_MULAW_ZERO_RUN 127
#define SENTINEL_MULAW_FOUR_BIT_RUN 126

bool S_FreeOldestSound(void);

void encodeWavelet(sfx_t *sfx, schar16 *packets);
void decodeWavelet(sndBuffer *stream, schar16 *packets);

void encodeMuLaw(sfx_t *sfx, schar16 *packets);
extern schar16 mulawToShort[256];

extern schar16 *sfxScratchBuffer;
extern sfx_t *sfxScratchPointer;
extern sint sfxScratchIndex;

extern idAudioOpenALSystem *soundOpenALSystem;

void SOrig_Init(void);
void SOrig_Shutdown(void);
void SOrig_StartSound(vec3_t origin, sint entnum, sint entchannel,
                      sfxHandle_t sfx);
void SOrig_StartLocalSound(sfxHandle_t sfx, sint channelNum);
void SOrig_StartBackgroundTrack(pointer intro, pointer loop);
void SOrig_StopBackgroundTrack(void);
void SOrig_RawSamples(sint stream, sint samples, sint rate, sint width,
                      sint s_channels, const uchar8 *data, float32 volume, sint entityNum);
void SOrig_StopAllSounds(void);
void SOrig_ClearLoopingSounds(bool killall);
void SOrig_AddLoopingSound(sint entityNum, const vec3_t origin,
                           const vec3_t velocity, sfxHandle_t sfx);
void SOrig_AddRealLoopingSound(sint entityNum, const vec3_t origin,
                               const vec3_t velocity, sfxHandle_t sfx);
void SOrig_StopLoopingSound(sint entityNum);
void SOrig_Respatialize(sint entityNum, const vec3_t origin,
                        vec3_t axis[3], sint inwater);
void SOrig_UpdateEntityPosition(sint entityNum, const vec3_t origin);
void SOrig_Update(void);
void SOrig_DisableSounds(void);
void SOrig_BeginRegistration(void);
sfxHandle_t SOrig_RegisterSound(pointer sample, bool compressed);
void SOrig_ClearSoundBuffer(void);
sint SOrig_SoundDuration(sfxHandle_t handle);
sint SOrig_GetVoiceAmplitude(sint entnum);
sint SOrig_GetSoundLength(sfxHandle_t sfxHandle);

sint SOrig_GetCurrentSoundTime(void);

// initializes cycling through a DMA buffer and returns information on it
bool SNDDMA_Init(sint sampleFrequencyInKHz);

//
// idSoundSystemLocal
//
class idSoundSystemLocal : public idSoundSystem {
public:
    /*
    ===============
    idSystemLocal::idSystemLocal
    ===============
    */
    idSoundSystemLocal(void);
    ~idSoundSystemLocal(void);

    virtual void Init(void);
    virtual void Shutdown(void);
    // if origin is nullptr, the sound will be dynamically sourced from the entity
    virtual void StartSound(vec3_t origin, sint entnum, sint entchannel,
                            sfxHandle_t sfx);
    virtual void StartLocalSound(sfxHandle_t sfx, sint channelNum);
    virtual void StartBackgroundTrack(pointer intro, pointer loop);
    virtual void StopBackgroundTrack(void);
    // cinematics and voice-over-network will send raw samples
    // 1.0 volume will be direct output of source samples
    virtual void RawSamples(sint stream, sint samples, sint rate, sint width,
                            sint channels, const uchar8 *data, float32 volume, sint entityNum);
    // stop all sounds and the background track
    virtual void StopAllSounds(void);
    // all continuous looping sounds must be added before calling S_Update
    virtual void ClearLoopingSounds(bool killall);
    virtual void AddLoopingSound(sint entityNum, const vec3_t origin,
                                 const vec3_t velocity, sfxHandle_t sfx);
    virtual void AddRealLoopingSound(sint entityNum, const vec3_t origin,
                                     const vec3_t velocity, sfxHandle_t sfx);
    virtual void StopLoopingSound(sint entityNum);
    // recompute the reletive volumes for all running sounds
    // reletive to the given entityNum / orientation
    virtual void Respatialize(sint entityNum, const vec3_t origin,
                              vec3_t axis[3], sint inwater);
    // let the sound system know where an entity currently is
    virtual void UpdateEntityPosition(sint entityNum, const vec3_t origin);
    virtual void Update(void);
    virtual void DisableSounds(void);
    virtual void BeginRegistration(void);
    // RegisterSound will allways return a valid sample, even if it
    // has to create a placeholder.  This prevents continuous filesystem
    // checks for missing files
    virtual sfxHandle_t RegisterSound(pointer sample, bool compressed);
    virtual void DisplayFreeMemory(void);
    virtual void ClearSoundBuffer(void);
    virtual sint SoundDuration(sfxHandle_t handle);
    virtual sint GetSoundLength(sfxHandle_t sfxHandle);
    virtual void Reload(void);
    virtual sint GetVoiceAmplitude(sint entnum);
    virtual sint GetCurrentSoundTime(void);

    virtual void *codec_load(pointer filename, snd_info_t *info);
    virtual snd_stream_t *codec_open(pointer filename);
    virtual void codec_close(snd_stream_t *stream);
    virtual sint codec_read(snd_stream_t *stream, sint bytes, void *buffer);
    virtual void SndPrintf(sint print_level, pointer fmt, ...);

    static void S_StopAllSounds(void);
};

extern idSoundSystemLocal soundSystemLocal;

#endif //!__SNDSYSTEM_HPP__