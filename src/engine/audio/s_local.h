////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2005 Stuart Dalton(badcdev@gmail.com)
// Copyright(C) 2011 - 2019 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   s_local.h
// Version:     v1.02
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <qcommon/q_shared.h>
#include <qcommon/qcommon.h>
#include <API/sound_api.h>

#define	PAINTBUFFER_SIZE		4096					// this is in samples

#define SND_CHUNK_SIZE			1024					// samples
#define SND_CHUNK_SIZE_FLOAT	(SND_CHUNK_SIZE/2)		// floats
#define SND_CHUNK_SIZE_BYTE		(SND_CHUNK_SIZE*2)		// floats

typedef struct
{
    S32 left; // the final values will be clamped to +/- 0x00ffff00 and shifted down
    S32	right;
} portable_samplepair_t;

typedef struct adpcm_state
{
    S16	sample; /* Previous output value */
    UTF8 index;	/* Index into stepsize table */
} adpcm_state_t;

typedef	struct sndBuffer_s
{
    S16 sndChunk[SND_CHUNK_SIZE];
    struct sndBuffer_s* next;
    S32 size;
    adpcm_state_t adpcm;
} sndBuffer;

typedef struct sfx_s
{
    sndBuffer* soundData;
    bool defaultSound; // couldn't be loaded, so use buzz
    bool inMemory; // not in Memory
    bool soundCompressed; // not in Memory
    S32 soundCompressionMethod;
    S32 soundLength;
    S32 soundChannels;
    UTF8 soundName[MAX_QPATH];
    S32 lastTimeUsed;
    struct sfx_s* next;
    bool weaponsound;
} sfx_t;

typedef struct
{
    S32 channels;
    S32 samples; // mono samples in buffer
    S32 submission_chunk; // don't mix less than this #
    S32 samplebits;
    S32 speed;
    U8* buffer;
} dma_t;

#define START_SAMPLE_IMMEDIATE	0x7fffffff

#define MAX_DOPPLER_SCALE 50.0f //arbitrary

typedef struct loopSound_s
{
    vec3_t origin;
    vec3_t velocity;
    sfx_t* sfx;
    S32 mergeFrame;
    bool active;
    bool kill;
    bool doppler;
    F32 dopplerScale;
    F32 oldDopplerScale;
    S32 framenum;
} loopSound_t;

typedef struct
{
    S32 vol; // Must be first member due to union (see channel_t)
    S32 offset;
    S32 bassvol;
    S32 bassoffset;
    S32 reverbvol;
    S32 reverboffset;
} ch_side_t;

typedef struct
{
    S32 allocTime;
    S32 startSample; // START_SAMPLE_IMMEDIATE = set immediately on next mix
    S32 entnum; // to allow overriding a specific sound
    S32 entchannel; // to allow overriding a specific sound
    S32 master_vol; // 0-255 volume before spatialization
    F32 dopplerScale;
    F32 oldDopplerScale;
    vec3_t origin; // only use if fixed_origin is set
    bool fixed_origin; // use origin instead of fetching entnum's origin
    sfx_t* thesfx; // sfx structure
    bool doppler;
    union
    {
        S32 leftvol; // 0-255 volume after spatialization
        ch_side_t l;
    };
    union
    {
        S32 rightvol; // 0-255 volume after spatialization
        ch_side_t r;
    };
    vec3_t sodrot;
} channel_t;

#define	WAV_FORMAT_PCM		1

typedef struct
{
    S32 format;
    S32 rate;
    S32 width;
    S32 channels;
    S32 samples;
    S32 dataofs; // chunk starts this many bytes from file start
} wavinfo_t;

// Interface between Q3 sound "api" and the sound backend
typedef struct
{
    void ( *Shutdown )( void );
    void ( *StartSound )( vec3_t origin, S32 entnum, S32 entchannel, sfxHandle_t sfx );
    void ( *StartLocalSound )( sfxHandle_t sfx, S32 channelNum );
    void ( *StartBackgroundTrack )( StringEntry intro, StringEntry loop );
    void ( *StopBackgroundTrack )( void );
    void ( *RawSamples )( S32 samples, S32 rate, S32 width, S32 channels, const U8* data, F32 volume );
    void ( *StopAllSounds )( void );
    void ( *ClearLoopingSounds )( bool killall );
    void ( *AddLoopingSound )( S32 entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
    void ( *AddRealLoopingSound )( S32 entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
    void ( *StopLoopingSound )( S32 entityNum );
    void ( *Respatialize )( S32 entityNum, const vec3_t origin, vec3_t axis[3], S32 inwater );
    void ( *UpdateEntityPosition )( S32 entityNum, const vec3_t origin );
    void ( *Update )( void );
    void ( *DisableSounds )( void );
    void ( *BeginRegistration )( void );
    sfxHandle_t ( *RegisterSound )( StringEntry sample, bool compressed );
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
bool SNDDMA_Init( S32 sampleFrequencyInKHz );
bool SNDDMAHD_DevList( void );

// gets the current DMA position
S32 SNDDMA_GetDMAPos( void );

// shutdown the DMA xfer.
void SNDDMA_Shutdown( void );

void SNDDMA_BeginPainting( void );

void SNDDMA_Submit( void );

//====================================================================

#define	MAX_CHANNELS			96

extern channel_t s_channels[MAX_CHANNELS];
extern channel_t loop_channels[MAX_CHANNELS];
extern S32 numLoopChannels;

extern S32 s_paintedtime;
extern S32 s_rawend;
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

void SND_free( sndBuffer* v );
sndBuffer* SND_malloc( void );
void SND_setup( void );

void S_PaintChannels( S32 endtime );

void S_memoryLoad( sfx_t* sfx );
portable_samplepair_t* S_GetRawSamplePointer( void );

// spatializes a channel
void S_Spatialize( channel_t* ch );

// adpcm functions
S32  S_AdpcmMemoryNeeded( const wavinfo_t* info );
void S_AdpcmEncodeSound( sfx_t* sfx, S16* samples );
void S_AdpcmGetSamples( sndBuffer* chunk, S16* to );

// wavelet function

#define SENTINEL_MULAW_ZERO_RUN 127
#define SENTINEL_MULAW_FOUR_BIT_RUN 126

void S_FreeOldestSound( void );

void encodeWavelet( sfx_t* sfx, S16* packets );
void decodeWavelet( sndBuffer* stream, S16* packets );

void encodeMuLaw( sfx_t* sfx, S16* packets );
extern S16 mulawToShort[256];

extern S16* sfxScratchBuffer;
extern sfx_t* sfxScratchPointer;
extern S32 sfxScratchIndex;

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

typedef S32 srcHandle_t;

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
    virtual void StartSound( vec3_t origin, S32 entnum, S32 entchannel, sfxHandle_t sfx );
    virtual void StartLocalSound( sfxHandle_t sfx, S32 channelNum );
    virtual void StartBackgroundTrack( StringEntry intro, StringEntry loop );
    virtual void StopBackgroundTrack( void );
    // cinematics and voice-over-network will send raw samples
    // 1.0 volume will be direct output of source samples
    virtual void RawSamples( S32 samples, S32 rate, S32 width, S32 channels, const U8* data, F32 volume );
    // stop all sounds and the background track
    virtual void StopAllSounds( void );
    // all continuous looping sounds must be added before calling S_Update
    virtual void ClearLoopingSounds( bool killall );
    virtual void AddLoopingSound( S32 entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
    virtual void AddRealLoopingSound( S32 entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
    virtual void StopLoopingSound( S32 entityNum );
    // recompute the reletive volumes for all running sounds
    // reletive to the given entityNum / orientation
    virtual void Respatialize( S32 entityNum, const vec3_t origin, vec3_t axis[3], S32 inwater );
    // let the sound system know where an entity currently is
    virtual void UpdateEntityPosition( S32 entityNum, const vec3_t origin );
    virtual void Update( void );
    virtual void DisableSounds( void );
    virtual void BeginRegistration( void );
    // RegisterSound will allways return a valid sample, even if it
    // has to create a placeholder.  This prevents continuous filesystem
    // checks for missing files
    virtual sfxHandle_t RegisterSound( StringEntry sample, bool compressed );
    virtual void DisplayFreeMemory( void );
    virtual void ClearSoundBuffer( void );
    virtual S32 SoundDuration( sfxHandle_t handle );
    virtual S32 GetSoundLength( sfxHandle_t sfxHandle );
    virtual void Reload( void );
    
    static void S_StopAllSounds( void );
};

extern idSoundSystemLocal soundSystemLocal;
