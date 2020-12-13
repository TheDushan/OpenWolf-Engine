////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
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
// File name:   s_local.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __S_LOCAL_H__
#define __S_LOCAL_H__

#define	PAINTBUFFER_SIZE		4096					// this is in samples

#define SND_CHUNK_SIZE			1024					// samples
#define SND_CHUNK_SIZE_FLOAT	(SND_CHUNK_SIZE/2)		// floats
#define SND_CHUNK_SIZE_BYTE		(SND_CHUNK_SIZE*2)		// floats

typedef struct
{
    sint left; // the final values will be clamped to +/- 0x00ffff00 and shifted down
    sint	right;
} portable_samplepair_t;

typedef struct adpcm_state
{
    schar16	sample; /* Previous output value */
    valueType index;	/* Index into stepsize table */
} adpcm_state_t;

typedef	struct sndBuffer_s
{
    schar16 sndChunk[SND_CHUNK_SIZE];
    struct sndBuffer_s* next;
    sint size;
    adpcm_state_t adpcm;
} sndBuffer;

typedef struct sfx_s
{
    sndBuffer* soundData;
    bool defaultSound; // couldn't be loaded, so use buzz
    bool inMemory; // not in Memory
    bool soundCompressed; // not in Memory
    sint soundCompressionMethod;
    sint soundLength;
    sint soundChannels;
    valueType soundName[MAX_QPATH];
    sint lastTimeUsed;
    struct sfx_s* next;
    bool weaponsound;
} sfx_t;

typedef struct
{
    sint channels;
    sint samples; // mono samples in buffer
    sint fullsamples; // samples with all channels in buffer (samples divided by channels)
    sint submission_chunk; // don't mix less than this #
    sint samplebits;
    sint speed;
    uchar8* buffer;
} dma_t;

#define START_SAMPLE_IMMEDIATE	0x7fffffff

#define MAX_DOPPLER_SCALE 50.0f //arbitrary

typedef struct loopSound_s
{
    vec3_t origin;
    vec3_t velocity;
    sfx_t* sfx;
    sint mergeFrame;
    bool active;
    bool kill;
    bool doppler;
    float32 dopplerScale;
    float32 oldDopplerScale;
    sint framenum;
} loopSound_t;

typedef struct
{
    sint vol; // Must be first member due to union (see channel_t)
    sint offset;
    sint bassvol;
    sint bassoffset;
    sint reverbvol;
    sint reverboffset;
} ch_side_t;

typedef struct
{
    sint allocTime;
    sint startSample; // START_SAMPLE_IMMEDIATE = set immediately on next mix
    sint entnum; // to allow overriding a specific sound
    sint entchannel; // to allow overriding a specific sound
    sint master_vol; // 0-255 volume before spatialization
    float32 dopplerScale;
    float32 oldDopplerScale;
    vec3_t origin; // only use if fixed_origin is set
    bool fixed_origin; // use origin instead of fetching entnum's origin
    sfx_t* thesfx; // sfx structure
    bool doppler;
    union
    {
        sint leftvol; // 0-255 volume after spatialization
        ch_side_t l;
    };
    union
    {
        sint rightvol; // 0-255 volume after spatialization
        ch_side_t r;
    };
    vec3_t sodrot;
} channel_t;

#define	WAV_FORMAT_PCM		1

typedef struct
{
    sint format;
    sint rate;
    sint width;
    sint channels;
    sint samples;
    sint dataofs; // chunk starts this many bytes from file start
} wavinfo_t;

// Interface between Q3 sound "api" and the sound backend
typedef struct
{
    void ( *Shutdown )( void );
    void ( *StartSound )( vec3_t origin, sint entnum, sint entchannel, sfxHandle_t sfx );
    void ( *StartLocalSound )( sfxHandle_t sfx, sint channelNum );
    void ( *StartBackgroundTrack )( pointer intro, pointer loop );
    void ( *StopBackgroundTrack )( void );
    void ( *RawSamples )( sint samples, sint rate, sint width, sint channels, const uchar8* data, float32 volume );
    void ( *StopAllSounds )( void );
    void ( *ClearLoopingSounds )( bool killall );
    void ( *AddLoopingSound )( sint entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
    void ( *AddRealLoopingSound )( sint entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
    void ( *StopLoopingSound )( sint entityNum );
    void ( *Respatialize )( sint entityNum, const vec3_t origin, vec3_t axis[3], sint inwater );
    void ( *UpdateEntityPosition )( sint entityNum, const vec3_t origin );
    void ( *Update )( void );
    void ( *DisableSounds )( void );
    void ( *BeginRegistration )( void );
    sfxHandle_t ( *RegisterSound )( pointer sample, bool compressed );
    void ( *ClearSoundBuffer )( void );
    void ( *SoundInfo )( void );
    void ( *SoundList )( void );
} soundInterface_t;

/*
====================================================================

  SYSTEM SPECIFIC FUNCTIONS

====================================================================
*/

// initializes cycling through a DMA buffer and returns information on it
bool SNDDMA_Init( sint sampleFrequencyInKHz );
bool SNDDMAHD_DevList( void );

// gets the current DMA position
sint SNDDMA_GetDMAPos( void );

// shutdown the DMA xfer.
void SNDDMA_Shutdown( void );

void SNDDMA_BeginPainting( void );

void SNDDMA_Submit( void );

//====================================================================

#define	MAX_CHANNELS			96

extern channel_t s_channels[MAX_CHANNELS];
extern channel_t loop_channels[MAX_CHANNELS];
extern sint numLoopChannels;

extern sint s_paintedtime;
extern sint s_rawend;
extern vec3_t listener_forward;
extern vec3_t listener_right;
extern vec3_t listener_up;
extern dma_t dma;

#define	MAX_RAW_SAMPLES	16384
extern	portable_samplepair_t	s_rawsamples[MAX_RAW_SAMPLES];

extern convar_t* s_volume;
extern convar_t* s_musicVolume;
extern convar_t* s_doppler;
extern convar_t* s_testsound;

bool S_LoadSound( sfx_t* sfx );

void SND_shutdown( void );
void SND_free( sndBuffer* v );
sndBuffer* SND_malloc( void );
void SND_setup( void );

void S_PaintChannels( sint endtime );

void S_memoryLoad( sfx_t* sfx );
portable_samplepair_t* S_GetRawSamplePointer( void );

// spatializes a channel
void S_Spatialize( channel_t* ch );

// adpcm functions
sint  S_AdpcmMemoryNeeded( const wavinfo_t* info );
void S_AdpcmEncodeSound( sfx_t* sfx, schar16* samples );
void S_AdpcmGetSamples( sndBuffer* chunk, schar16* to );

// wavelet function

#define SENTINEL_MULAW_ZERO_RUN 127
#define SENTINEL_MULAW_FOUR_BIT_RUN 126

void S_FreeOldestSound( void );

void encodeWavelet( sfx_t* sfx, schar16* packets );
void decodeWavelet( sndBuffer* stream, schar16* packets );

void encodeMuLaw( sfx_t* sfx, schar16* packets );
extern schar16 mulawToShort[256];

extern schar16* sfxScratchBuffer;
extern sfx_t* sfxScratchPointer;
extern sint sfxScratchIndex;

bool S_Base_Init( soundInterface_t* si );

// OpenAL stuff
typedef enum
{
    SRCPRI_AMBIENT = 0,	// Ambient sound effects
    SRCPRI_ENTITY, // Entity sound effects
    SRCPRI_ONESHOT,	// One-shot sounds
    SRCPRI_LOCAL, // Local sounds
    SRCPRI_STREAM // Streams (music, cutscenes)
} alSrcPriority_t;

typedef sint srcHandle_t;

bool S_AL_Init( soundInterface_t* si );

//
// idSoundSystemLocal
//
class idSoundSystemLocal : public idSoundSystem
{
public:
    virtual void Init( void );
    virtual void Shutdown( void );
    // if origin is nullptr, the sound will be dynamically sourced from the entity
    virtual void StartSound( vec3_t origin, sint entnum, sint entchannel, sfxHandle_t sfx );
    virtual void StartLocalSound( sfxHandle_t sfx, sint channelNum );
    virtual void StartBackgroundTrack( pointer intro, pointer loop );
    virtual void StopBackgroundTrack( void );
    // cinematics and voice-over-network will send raw samples
    // 1.0 volume will be direct output of source samples
    virtual void RawSamples( sint samples, sint rate, sint width, sint channels, const uchar8* data, float32 volume );
    // stop all sounds and the background track
    virtual void StopAllSounds( void );
    // all continuous looping sounds must be added before calling S_Update
    virtual void ClearLoopingSounds( bool killall );
    virtual void AddLoopingSound( sint entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
    virtual void AddRealLoopingSound( sint entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
    virtual void StopLoopingSound( sint entityNum );
    // recompute the reletive volumes for all running sounds
    // reletive to the given entityNum / orientation
    virtual void Respatialize( sint entityNum, const vec3_t origin, vec3_t axis[3], sint inwater );
    // let the sound system know where an entity currently is
    virtual void UpdateEntityPosition( sint entityNum, const vec3_t origin );
    virtual void Update( void );
    virtual void DisableSounds( void );
    virtual void BeginRegistration( void );
    // RegisterSound will allways return a valid sample, even if it
    // has to create a placeholder.  This prevents continuous filesystem
    // checks for missing files
    virtual sfxHandle_t RegisterSound( pointer sample, bool compressed );
    virtual void DisplayFreeMemory( void );
    virtual void ClearSoundBuffer( void );
    virtual sint SoundDuration( sfxHandle_t handle );
    virtual sint GetSoundLength( sfxHandle_t sfxHandle );
    virtual void Reload( void );
    
    static void S_StopAllSounds( void );
};

extern idSoundSystemLocal soundSystemLocal;

#endif //!__S_LOCAL_H__