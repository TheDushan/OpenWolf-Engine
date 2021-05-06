////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
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
// File name:   sndSystem_dma.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

void S_Play_f(void);
void S_SoundList_f(void);
void S_Music_f(void);

void S_Update_();
void SOrig_StopAllSounds(void);
void S_UpdateBackgroundTrack(void);

idSoundSystemLocal soundSystemLocal;
idSoundSystem *soundSystem = &soundSystemLocal;

snd_stream_t *s_backgroundStream = nullptr;
static valueType s_backgroundLoop[MAX_QPATH];
//static valueType s_backgroundMusic[MAX_QPATH]; //TTimo: unused


// =======================================================================
// Internal sound data & structures
// =======================================================================

// only begin attenuating sound volumes when outside the FULLVOLUME range
#define SOUND_FULLVOLUME 80

#define SOUND_ATTENUATE 0.0008f

channel_t s_channels[MAX_CHANNELS];
channel_t loop_channels[MAX_CHANNELS];
sint numLoopChannels;

static sint s_soundStarted;
static bool s_soundMuted;

dma_t       dma;

static sint listener_number;
static vec3_t listener_origin;
static vec3_t listener_axis[3];

sint s_soundtime;       // sample PAIRS
sint s_paintedtime;         // sample PAIRS

// MAX_SFX may be larger than MAX_SOUNDS because
// of custom player sounds
#define MAX_SFX  4096
sfx_t s_knownSfx[MAX_SFX];
sint s_numSfx = 0;

#define LOOP_HASH 128
static sfx_t *sfxHash[LOOP_HASH];

static loopSound_t loopSounds[MAX_GENTITIES];
static  channel_t *freelist = nullptr;

sint s_rawend;
portable_samplepair_t s_rawsamples[MAX_RAW_SAMPLES];

// ====================================================================
// User-setable variables
// ====================================================================

/*
===============
idSoundSystemLocal::idSoundSystemLocal
===============
*/
idSoundSystemLocal::idSoundSystemLocal(void) {
}

/*
===============
idSystemLocal::~idSystemLocal
===============
*/
idSoundSystemLocal::~idSoundSystemLocal(void) {
}

/*
=================
S_SoundInfo_f
=================
*/
void S_SoundInfo_f(void) {
    Com_Printf("----- Sound Info -----\n");

    if(!s_soundStarted) {
        Com_Printf("sound system not started\n");
    } else {
        if(s_soundMuted) {
            Com_Printf("sound system is muted\n");
        }

        Com_Printf("%5d stereo\n", dma.channels - 1);
        Com_Printf("%5d samples\n", dma.samples);
        Com_Printf("%5d samplebits\n", dma.samplebits);
        Com_Printf("%5d submission_chunk\n", dma.submission_chunk);
        Com_Printf("%5d speed\n", dma.speed);
        Com_Printf("%p dma buffer\n", static_cast< void * >(dma.buffer));

        if(s_backgroundStream) {
            Com_Printf("Background stream: %s\n", s_backgroundLoop);
        } else {
            Com_Printf("No background file.\n");
        }

    }

    Com_Printf("----------------------\n");
}

/*
===============
idSoundSystemLocal::S_StopAllSounds

I had a problem with AddCommand and pointing to the class member so this is one way of doing
pointing to a function that is a class member
===============
*/
void idSoundSystemLocal::S_StopAllSounds(void) {
    static_cast<idSoundSystemLocal *>(soundSystem)->StopAllSounds();
}

/*
================
S_Init
================
*/
void SOrig_Init(void) {
    bool r;

    cmdSystem->AddCommand("play", S_Play_f, "Play a sound file (play sound)");
    cmdSystem->AddCommand("music", S_Music_f,
                          "Plays specified music file (music music)");
    cmdSystem->AddCommand("s_list", S_SoundList_f,
                          "Display paths and filenames of all sound files as they are played. (replaced soundlist command)");
    cmdSystem->AddCommand("s_info", S_SoundInfo_f,
                          "Display information about sound system (replaced soundinfo command)");
    cmdSystem->AddCommand("s_stop", soundSystemLocal.S_StopAllSounds,
                          "Stop whatever sound that is currently playing from playing. (Replaced stopsound command)");

    r = SNDDMA_Init(s_khz->integer);

    if(r) {
        s_soundStarted = 1;
        s_soundMuted = 1;
        //s_numSfx = 0;

        ::memset(sfxHash, 0, sizeof(sfx_t *)*LOOP_HASH);

        s_soundtime = 0;
        s_paintedtime = 0;

        SOrig_StopAllSounds();

        S_SoundInfo_f();
    }

}

/*
================
S_ChannelFree
================
*/
void S_ChannelFree(channel_t *v) {
    v->thesfx = nullptr;
    *(channel_t **)v = freelist;
    freelist = (channel_t *)v;
}

/*
================
S_ChannelMalloc
================
*/
channel_t  *S_ChannelMalloc(void) {
    channel_t *v;

    if(freelist == nullptr) {
        return nullptr;
    }

    v = freelist;
    freelist = *(channel_t **)freelist;
    v->allocTime = Com_Milliseconds();

    return v;
}

/*
=================
S_ChannelSetup
=================
*/
void S_ChannelSetup(void) {
    channel_t *p, *q;

    // clear all the sounds so they don't
    ::memset(s_channels, 0, sizeof(s_channels));

    p = s_channels;;
    q = p + MAX_CHANNELS;

    while(--q > p) {
        *(channel_t **)q = q - 1;
    }

    *(channel_t **)q = nullptr;
    freelist = p + MAX_CHANNELS - 1;
    //Com_DPrintf( "Channel memory manager started\n" );
}

// =======================================================================
// Shutdown sound engine
// =======================================================================

/*
=================
SOrig_Shutdown
=================
*/
void SOrig_Shutdown(void) {
    if(!s_soundStarted) {
        return;
    }

    SNDDMA_Shutdown();
    SND_shutdown();

    s_soundStarted = 0;
    s_numSfx = 0;

    cmdSystem->RemoveCommand("play");
    cmdSystem->RemoveCommand("music");
    cmdSystem->RemoveCommand("stopsound");
    cmdSystem->RemoveCommand("soundlist");
    cmdSystem->RemoveCommand("soundinfo");
}


// =======================================================================
// Load a sound
// =======================================================================

/*
================
S_HashSFXName

return a hash value for the sfx name
================
*/
static sint32 S_HashSFXName(pointer name) {
    sint    i;
    sint32  hash;
    valueType   letter;

    hash = 0;
    i = 0;

    while(name[i] != '\0') {
        letter = tolower(name[i]);

        if(letter == '.') {
            // don't include extension
            break;
        }

        if(letter == '\\') {
            // damn path names
            letter = '/';
        }

        hash += static_cast<sint32>(letter) * (i + 119);
        i++;
    }

    hash &= (LOOP_HASH - 1);
    return hash;
}

/*
==================
S_FindName

Will allocate a new sfx if it isn't found
==================
*/
static sfx_t *S_FindName(pointer name) {
    sint i;
    sint hash;
    sfx_t *sfx;

    if(!name) {
        Com_Error(ERR_FATAL, "Sound name is a nullptr string\n");
    }

    if(!name[0]) {
        Com_Printf(S_COLOR_YELLOW "WARNING: Sound name is empty\n");
        return nullptr;
    }

    if(strlen(name) >= MAX_QPATH) {
        Com_Printf(S_COLOR_YELLOW "WARNING: Sound name is too long: %s\n", name);
        return nullptr;
    }

    hash = S_HashSFXName(name);

    sfx = sfxHash[hash];

    // see if already loaded
    while(sfx) {
        if(!Q_stricmp(sfx->soundName, name)) {
            return sfx;
        }

        sfx = sfx->next;
    }

    // find a free sfx
    for(i = 0 ; i < s_numSfx ; i++) {
        if(!s_knownSfx[i].soundName[0]) {
            break;
        }
    }

    if(i == s_numSfx) {
        if(s_numSfx == MAX_SFX) {
            Com_Error(ERR_FATAL, "S_FindName: out of sfx_t");
        }

        s_numSfx++;
    }

    sfx = &s_knownSfx[i];
    ::memset(sfx, 0, sizeof(*sfx));
    Q_strcpy_s(sfx->soundName, name);

    sfx->next = sfxHash[hash];
    sfxHash[hash] = sfx;

    return sfx;
}

/*
=================
S_DefaultSound
=================
*/
void S_DefaultSound(sfx_t *sfx) {
    sint i;

    sfx->soundLength = 512;
    sfx->soundData = SND_malloc();
    sfx->soundData->next = nullptr;

    for(i = 0 ; i < sfx->soundLength ; i++) {
        sfx->soundData->sndChunk[i] = i;
    }
}

/*
===================
SOrig_DisableSounds

Disables sounds until the next S_BeginRegistration.
This is called when the hunk is cleared and the sounds
are no longer valid.
===================
*/
void SOrig_DisableSounds(void) {
    SOrig_StopAllSounds();
    s_soundMuted = true;
}

/*
==================
SOrig_SoundDuration
==================
*/
sint SOrig_SoundDuration(sfxHandle_t handle) {
    if(handle < 0 || handle >= s_numSfx) {
        Com_Printf(S_COLOR_YELLOW "S_Base_SoundDuration: handle %i out of range\n",
                   handle);
        return 0;
    }

    return s_knownSfx[ handle ].duration;
}


/*
=====================
SOrig_BeginRegistration
=====================
*/
void SOrig_BeginRegistration(void) {
    s_soundMuted = false;       // we can play again

    if(s_numSfx == 0) {
        SND_setup();

        s_numSfx = 0;
        ::memset(s_knownSfx, 0, sizeof(s_knownSfx));
        ::memset(sfxHash, 0, sizeof(sfx_t *)*LOOP_HASH);

        SOrig_RegisterSound("sound/feedback/hit.wav",
                            false);        // changed to a sound in baseq3
    }
}


/*
==================
S_RegisterSound

Creates a default buzz sound if the file can't be loaded
==================
*/
sfxHandle_t SOrig_RegisterSound(pointer name, bool compressed) {
    sfx_t *sfx;

    compressed = false;

    if(!s_soundStarted) {
        return 0;
    }

    if(strlen(name) >= MAX_QPATH) {
        Com_Printf("Sound name exceeds MAX_QPATH\n");
        return 0;
    }

    sfx = S_FindName(name);

    if(!sfx) {
        return 0;
    }

    if(sfx->soundData) {
        if(sfx->defaultSound) {
            Com_Printf(S_COLOR_YELLOW "WARNING: could not find %s - using default\n",
                       sfx->soundName);
            return 0;
        }

        return ARRAY_INDEX(s_knownSfx, sfx);
    }

    sfx->inMemory = false;
    sfx->soundCompressed = compressed;

    S_memoryLoad(sfx);

    if(sfx->defaultSound) {
        Com_Printf(S_COLOR_YELLOW "WARNING: could not find %s - using default\n",
                   sfx->soundName);
        return 0;
    }

    return ARRAY_INDEX(s_knownSfx, sfx);
}

/*
=================
S_memoryLoad
=================
*/
void S_memoryLoad(sfx_t    *sfx) {
    // load the sound file
    if(!S_LoadSound(sfx)) {
        Com_Printf(S_COLOR_YELLOW "WARNING: couldn't load sound: %s\n",
                   sfx->soundName);
        sfx->defaultSound = true;
    }

    sfx->inMemory = true;
}

//=============================================================================

/*
=================
S_SpatializeOrigin

Used for spatializing s_channels
=================
*/
void S_SpatializeOrigin(vec3_t origin, sint master_vol, sint *left_vol,
                        sint *right_vol) {
    vec_t dot, dist, lscale, rscale, scale;
    vec3_t source_vec, vec;

    const float32 dist_mult = SOUND_ATTENUATE;

    // calculate stereo seperation and distance attenuation
    VectorSubtract(origin, listener_origin, source_vec);

    dist = VectorNormalize(source_vec);
    dist -= SOUND_FULLVOLUME;

    if(dist < 0) {
        // close enough to be at full volume
        dist = 0;
    }

    // different attenuation levels
    dist *= dist_mult;

    VectorRotate(source_vec, listener_axis, vec);

    dot = -vec[1];

    if(dma.channels == 1) {
        // no attenuation = no spatialization
        rscale = 1.0f;
        lscale = 1.0f;
    } else {
        rscale = 0.5f * (1.0f + dot);
        lscale = 0.5f * (1.0f - dot);

        if(rscale < 0) {
            rscale = 0;
        }

        if(lscale < 0) {
            lscale = 0;
        }
    }

    // add in distance effect
    scale = (1.0f - dist) * rscale;
    *right_vol = (master_vol * scale);

    if(*right_vol < 0) {
        *right_vol = 0;
    }

    scale = (1.0f - dist) * lscale;
    *left_vol = (master_vol * scale);

    if(*left_vol < 0) {
        *left_vol = 0;
    }
}

// =======================================================================
// Start a sound effect
// =======================================================================

/*
====================
S_StartSound

Validates the parms and ques the sound up
if pos is nullptr, the sound will be dynamically sourced from the entity
Entchannel 0 will never override a playing sound
====================
*/
void SOrig_StartSound(vec3_t origin, sint entityNum, sint entchannel,
                      sfxHandle_t sfxHandle) {
    channel_t *ch;
    sfx_t *sfx;
    sint i, oldest, chosen, time, inplay, allowed;

    if(!s_soundStarted || s_soundMuted) {
        return;
    }

    if(com_minimized->integer || com_unfocused->integer) {
        return;
    }

    if(!origin && (entityNum < 0 || entityNum > MAX_GENTITIES)) {
        Com_Error(ERR_DROP, "S_StartSound: bad entitynum %i", entityNum);
    }

    if(sfxHandle < 0 || sfxHandle >= s_numSfx) {
        Com_Printf(S_COLOR_YELLOW "S_StartSound: handle %i out of range\n",
                   sfxHandle);
        return;
    }

    sfx = &s_knownSfx[ sfxHandle ];

    if(sfx->inMemory == false) {
        S_memoryLoad(sfx);
    }

    if(s_show->integer == 1) {
        Com_Printf("%i : %s\n", s_paintedtime, sfx->soundName);
    }

    time = Com_Milliseconds();

    //  Com_Printf("playing %s\n", sfx->soundName);
    // pick a channel to play on

    allowed = 4;

    if(entityNum == listener_number) {
        allowed = 8;
    }

    ch = s_channels;
    inplay = 0;

    for(i = 0; i < MAX_CHANNELS ; i++, ch++) {
        if(ch[i].entnum == entityNum && ch[i].thesfx == sfx) {
            if(time - ch[i].allocTime < 50) {
                //              if (Cvar_VariableValue( "cg_showmiss" )) {
                //                  Com_Printf("double sound start\n");
                //              }
                return;
            }

            inplay++;
        }
    }

    if(inplay > allowed) {
        return;
    }

    sfx->lastTimeUsed = time;

    ch = S_ChannelMalloc(); // entityNum, entchannel);

    if(!ch) {
        ch = s_channels;

        oldest = sfx->lastTimeUsed;
        chosen = -1;

        for(i = 0 ; i < MAX_CHANNELS ; i++, ch++) {
            if(ch->entnum != listener_number && ch->entnum == entityNum &&
                    ch->allocTime < oldest && ch->entchannel != CHAN_ANNOUNCER) {
                oldest = ch->allocTime;
                chosen = i;
            }
        }

        if(chosen == -1) {
            ch = s_channels;

            for(i = 0 ; i < MAX_CHANNELS ; i++, ch++) {
                if(ch->entnum != listener_number && ch->allocTime < oldest &&
                        ch->entchannel != CHAN_ANNOUNCER) {
                    oldest = ch->allocTime;
                    chosen = i;
                }
            }

            if(chosen == -1) {
                if(ch->entnum == listener_number) {
                    for(i = 0 ; i < MAX_CHANNELS ; i++, ch++) {
                        if(ch->allocTime < oldest) {
                            oldest = ch->allocTime;
                            chosen = i;
                        }
                    }
                }

                if(chosen == -1) {
                    Com_Printf("dropping sound\n");
                    return;
                }
            }
        }

        ch = &s_channels[chosen];
        ch->allocTime = sfx->lastTimeUsed;
    }

    if(origin) {
        VectorCopy(origin, ch->origin);
        ch->fixed_origin = true;
    } else {
        ch->fixed_origin = false;
    }

    ch->master_vol = 127;
    ch->entnum = entityNum;
    ch->thesfx = sfx;
    ch->startSample = START_SAMPLE_IMMEDIATE;
    ch->entchannel = entchannel;
    ch->leftvol =
        ch->master_vol;       // these will get calced at next spatialize
    ch->rightvol = ch->master_vol;      // unless the game isn't running
    ch->doppler = false;
}


/*
==================
SOrig_StartLocalSound
==================
*/
void SOrig_StartLocalSound(sfxHandle_t sfxHandle, sint channelNum) {
    if(!s_soundStarted || s_soundMuted) {
        return;
    }

    if(sfxHandle < 0 || sfxHandle >= s_numSfx) {
        Com_Printf(S_COLOR_YELLOW "S_StartLocalSound: handle %i out of range\n",
                   sfxHandle);
        return;
    }

    SOrig_StartSound(nullptr, listener_number, channelNum, sfxHandle);
}


/*
==================
SOrig_ClearSoundBuffer

If we are about to perform file access, clear the buffer
so sound doesn't stutter.
==================
*/
void SOrig_ClearSoundBuffer(void) {
    sint clear;

    if(!s_soundStarted) {
        return;
    }

    // stop looping sounds
    ::memset(loopSounds, 0, MAX_GENTITIES * sizeof(loopSound_t));
    ::memset(loop_channels, 0, MAX_CHANNELS * sizeof(channel_t));
    numLoopChannels = 0;

    S_ChannelSetup();

    s_rawend = 0;

    if(dma.samplebits == 8) {
        clear = 0x80;
    } else {
        clear = 0;
    }

    SNDDMA_BeginPainting();

    if(dma.buffer) {
        // TTimo: due to a particular bug workaround in linux sound code,
        //   have to optionally use a custom C implementation of ::memset
        //   not affecting win32, we have #define Snd_Memset ::memset
        // https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=371
        //Snd_Memset( dma.buffer, clear, dma.samples * dma.samplebits / 8 );
        ::memset(dma.buffer, clear, dma.samples * dma.samplebits / 8);
    }

    SNDDMA_Submit();
}

/*
==================
SOrig_StopAllSounds
==================
*/
void SOrig_StopAllSounds(void) {
    if(!s_soundStarted) {
        return;
    }

    // stop the background music
    SOrig_StopBackgroundTrack();

    SOrig_ClearSoundBuffer();
}

/*
==============================================================

continuous looping sounds are added each frame

==============================================================
*/

void SOrig_StopLoopingSound(sint entityNum) {
    loopSounds[entityNum].active = false;
    //  loopSounds[entityNum].sfx = 0;
    loopSounds[entityNum].kill = false;
}

/*
==================
SOrig_ClearLoopingSounds
==================
*/
void SOrig_ClearLoopingSounds(bool killall) {
    sint i;

    for(i = 0 ; i < MAX_GENTITIES ; i++) {
        if(killall || loopSounds[i].kill == true || (loopSounds[i].sfx &&
                loopSounds[i].sfx->soundLength == 0)) {
            loopSounds[i].kill = false;
            SOrig_StopLoopingSound(i);
        }
    }

    numLoopChannels = 0;
}

/*
==================
SOrig_AddLoopingSound

Called during entity generation for a frame
Include velocity in case I get around to doing doppler...
==================
*/
void SOrig_AddLoopingSound(sint entityNum, const vec3_t origin,
                           const vec3_t velocity, sfxHandle_t sfxHandle) {
    sfx_t *sfx;

    if(!s_soundStarted || s_soundMuted) {
        return;
    }

    if(sfxHandle < 0 || sfxHandle >= s_numSfx) {
        Com_Printf(S_COLOR_YELLOW "S_AddLoopingSound: handle %i out of range\n",
                   sfxHandle);
        return;
    }

    sfx = &s_knownSfx[ sfxHandle ];

    if(sfx->inMemory == false) {
        S_memoryLoad(sfx);
    }

    if(!sfx->soundLength) {
        Com_Error(ERR_DROP, "%s has length 0", sfx->soundName);
    }

    VectorCopy(origin, loopSounds[entityNum].origin);
    VectorCopy(velocity, loopSounds[entityNum].velocity);
    loopSounds[entityNum].active = true;
    loopSounds[entityNum].kill = true;
    loopSounds[entityNum].doppler = false;
    loopSounds[entityNum].oldDopplerScale = 1.0;
    loopSounds[entityNum].dopplerScale = 1.0;
    loopSounds[entityNum].sfx = sfx;

    if(s_doppler->integer && VectorLengthSquared(velocity) > 0.0) {
        vec3_t out;
        float32 lena, lenb;

        loopSounds[entityNum].doppler = true;
        lena = DistanceSquared(loopSounds[listener_number].origin,
                               loopSounds[entityNum].origin);
        VectorAdd(loopSounds[entityNum].origin, loopSounds[entityNum].velocity,
                  out);
        lenb = DistanceSquared(loopSounds[listener_number].origin, out);

        if((loopSounds[entityNum].framenum + 1) != cls.framecount) {
            loopSounds[entityNum].oldDopplerScale = 1.0;
        } else {
            loopSounds[entityNum].oldDopplerScale = loopSounds[entityNum].dopplerScale;
        }

        loopSounds[entityNum].dopplerScale = lenb / (lena * 100);

        if(loopSounds[entityNum].dopplerScale <= 1.0) {
            // don't bother doing the math
            loopSounds[entityNum].doppler = false;
        }
    }

    loopSounds[entityNum].framenum = cls.framecount;
}

/*
==================
SOrig_AddRealLoopingSound

Called during entity generation for a frame
Include velocity in case I get around to doing doppler...
==================
*/
void SOrig_AddRealLoopingSound(sint entityNum, const vec3_t origin,
                               const vec3_t velocity, sfxHandle_t sfxHandle) {
    sfx_t *sfx;

    if(!s_soundStarted || s_soundMuted) {
        return;
    }

    if(sfxHandle < 0 || sfxHandle >= s_numSfx) {
        Com_Printf(S_COLOR_YELLOW
                   "S_AddRealLoopingSound: handle %i out of range\n", sfxHandle);
        return;
    }

    sfx = &s_knownSfx[ sfxHandle ];

    if(sfx->inMemory == false) {
        S_memoryLoad(sfx);
    }

    if(!sfx->soundLength) {
        Com_Error(ERR_DROP, "%s has length 0", sfx->soundName);
    }

    VectorCopy(origin, loopSounds[entityNum].origin);
    VectorCopy(velocity, loopSounds[entityNum].velocity);

    loopSounds[entityNum].sfx = sfx;
    loopSounds[entityNum].active = true;
    loopSounds[entityNum].kill = false;
    loopSounds[entityNum].doppler = false;
}

/*
==================
S_AddLoopSounds

Spatialize all of the looping sounds.
All sounds are on the same cycle, so any duplicates can just
sum up the channel multipliers.
==================
*/
void S_AddLoopSounds(void) {
    sint i, j, time, left_total, right_total, left, right;
    channel_t *ch;
    loopSound_t *loop, *loop2;
    static sint loopFrame;

    numLoopChannels = 0;

    time = Com_Milliseconds();

    loopFrame++;

    for(i = 0 ; i < MAX_GENTITIES ; i++) {
        loop = &loopSounds[i];

        if(!loop->active || loop->mergeFrame == loopFrame) {
            // already merged into an earlier sound
            continue;
        }

        if(loop->kill) {
            // 3d
            S_SpatializeOrigin(loop->origin, 127, &left_total, &right_total);
        } else {
            // sphere
            S_SpatializeOrigin(loop->origin, 90,  &left_total, &right_total);
        }

        loop->sfx->lastTimeUsed = time;

        for(j = (i + 1); j < MAX_GENTITIES ; j++) {
            loop2 = &loopSounds[j];

            if(!loop2->active || loop2->doppler || loop2->sfx != loop->sfx) {
                continue;
            }

            loop2->mergeFrame = loopFrame;

            if(loop2->kill) {
                // 3d
                S_SpatializeOrigin(loop2->origin, 127, &left, &right);
            } else {
                // sphere
                S_SpatializeOrigin(loop2->origin, 90,  &left, &right);
            }

            loop2->sfx->lastTimeUsed = time;
            left_total += left;
            right_total += right;
        }

        if(left_total == 0 && right_total == 0) {
            // not audible
            continue;
        }

        // allocate a channel
        ch = &loop_channels[numLoopChannels];

        if(left_total > 255) {
            left_total = 255;
        }

        if(right_total > 255) {
            right_total = 255;
        }

        ch->master_vol = 127;
        ch->leftvol = left_total;
        ch->rightvol = right_total;
        ch->thesfx = loop->sfx;
        ch->doppler = loop->doppler;
        ch->dopplerScale = loop->dopplerScale;
        ch->oldDopplerScale = loop->oldDopplerScale;
        numLoopChannels++;

        if(numLoopChannels == MAX_CHANNELS) {
            return;
        }
    }
}

//=============================================================================

/*
=================
S_ByteSwapRawSamples

If raw data has been loaded in little endien binary form, this must be done.
If raw data was calculated, as with ADPCM, this should not be called.
=================
*/
void S_ByteSwapRawSamples(sint samples, sint width, sint s_channels,
                          const uchar8 *data) {
    sint i;

    if(width != 2) {
        return;
    }

    if(LittleShort(256) == 256) {
        return;
    }

    if(s_channels == 2) {
        samples <<= 1;
    }

    for(i = 0 ; i < samples ; i++) {
        (const_cast<schar16 *>(reinterpret_cast<const schar16 *>
                               (data)))[i] = LittleShort((const_cast<schar16 *>
                                             (reinterpret_cast<const schar16 *>(data)))[i]);
    }
}

/*
=================
S_GetRawSamplePointer
=================
*/
portable_samplepair_t *S_GetRawSamplePointer(void) {
    return s_rawsamples;
}

/*
============
SOrig_RawSamples

Music streaming
============
*/
void SOrig_RawSamples(sint stream, sint samples, sint rate, sint width,
                      sint s_channels, const uchar8 *data, float32 volume, sint entityNum) {
    sint i, src, dst, intVolume;
    float32 scale;

    if(!s_soundStarted || s_soundMuted) {
        return;
    }

    intVolume = 256 * volume;

    if(s_rawend < s_soundtime) {
        Com_DPrintf("S_RawSamples: resetting minimum: %i < %i\n", s_rawend,
                    s_soundtime);
        s_rawend = s_soundtime;
    }

    scale = static_cast<float32>(rate) / dma.speed;

    //Com_Printf ("%i < %i < %i\n", s_soundtime, s_paintedtime, s_rawend);
    if(s_channels == 2 && width == 2) {
        if(scale == 1.0) {
            // optimized case
            for(i = 0 ; i < samples ; i++) {
                dst = s_rawend & (MAX_RAW_SAMPLES - 1);
                s_rawend++;
                s_rawsamples[dst].left = (const_cast<schar16 *>
                                          (reinterpret_cast<const schar16 *>(data)))[i * 2] * intVolume;
                s_rawsamples[dst].right = (const_cast<schar16 *>
                                           (reinterpret_cast<const schar16 *>(data)))[i * 2 + 1] * intVolume;
            }
        } else {
            for(i = 0 ; ; i++) {
                src = i * scale;

                if(src >= samples) {
                    break;
                }

                dst = s_rawend & (MAX_RAW_SAMPLES - 1);
                s_rawend++;
                s_rawsamples[dst].left = (const_cast<schar16 *>
                                          (reinterpret_cast<const schar16 *>(data)))[src * 2] * intVolume;
                s_rawsamples[dst].right = (const_cast<schar16 *>
                                           (reinterpret_cast<const schar16 *>(data)))[src * 2 + 1] * intVolume;
            }
        }
    } else if(s_channels == 1 && width == 2) {
        for(i = 0 ; ; i++) {
            src = i * scale;

            if(src >= samples) {
                break;
            }

            dst = s_rawend & (MAX_RAW_SAMPLES - 1);
            s_rawend++;
            s_rawsamples[dst].left = (const_cast<schar16 *>
                                      (reinterpret_cast<const schar16 *>(data)))[src] * intVolume;
            s_rawsamples[dst].right = (const_cast<schar16 *>
                                       (reinterpret_cast<const schar16 *>(data)))[src] * intVolume;
        }
    } else if(s_channels == 2 && width == 1) {
        intVolume *= 256;

        for(i = 0 ; ; i++) {
            src = i * scale;

            if(src >= samples) {
                break;
            }

            dst = s_rawend & (MAX_RAW_SAMPLES - 1);
            s_rawend++;
            s_rawsamples[dst].left = (const_cast<valueType *>
                                      (reinterpret_cast<pointer>(data)))[src * 2] * intVolume;
            s_rawsamples[dst].right = (const_cast<valueType *>
                                       (reinterpret_cast<pointer>(data)))[src * 2 + 1] * intVolume;
        }
    } else if(s_channels == 1 && width == 1) {
        intVolume *= 256;

        for(i = 0 ; ; i++) {
            src = i * scale;

            if(src >= samples) {
                break;
            }

            dst = s_rawend & (MAX_RAW_SAMPLES - 1);
            s_rawend++;
            s_rawsamples[dst].left = (const_cast<uchar8 *>
                                      (reinterpret_cast<const uchar8 *>(data))[src] - 128) * intVolume;
            s_rawsamples[dst].right = (const_cast<uchar8 *>
                                       (reinterpret_cast<const uchar8 *>(data))[src] - 128) * intVolume;
        }
    }

    if(s_rawend > s_soundtime + MAX_RAW_SAMPLES) {
        Com_DPrintf("S_RawSamples: overflowed %i > %i\n", s_rawend, s_soundtime);
    }
}

//=============================================================================

/*
=====================
SOrig_UpdateEntityPosition

let the sound system know where an entity currently is
======================
*/
void SOrig_UpdateEntityPosition(sint entityNum, const vec3_t origin) {
    if(entityNum < 0 || entityNum > MAX_GENTITIES) {
        Com_Error(ERR_DROP, "S_UpdateEntityPosition: bad entitynum %i", entityNum);
    }

    VectorCopy(origin, loopSounds[entityNum].origin);
}


/*
============
SOrig_Respatialize

Change the volumes of all the playing sounds for changes in their positions
============
*/
void SOrig_Respatialize(sint entityNum, const vec3_t head, vec3_t axis[3],
                        sint inwater) {
    sint i;
    channel_t *ch;
    vec3_t origin;

    if(!s_soundStarted || s_soundMuted) {
        return;
    }

    listener_number = entityNum;
    VectorCopy(head, listener_origin);
    VectorCopy(axis[0], listener_axis[0]);
    VectorCopy(axis[1], listener_axis[1]);
    VectorCopy(axis[2], listener_axis[2]);

    // update spatialization for dynamic sounds
    ch = s_channels;

    for(i = 0 ; i < MAX_CHANNELS ; i++, ch++) {
        if(!ch->thesfx) {
            continue;
        }

        // anything coming from the view entity will always be full volume
        if(ch->entnum == listener_number) {
            ch->leftvol = ch->master_vol;
            ch->rightvol = ch->master_vol;
        } else {
            if(ch->fixed_origin) {
                VectorCopy(ch->origin, origin);
            } else {
                VectorCopy(loopSounds[ ch->entnum ].origin, origin);
            }

            S_SpatializeOrigin(origin, ch->master_vol, &ch->leftvol, &ch->rightvol);
        }
    }

    // add loopsounds
    S_AddLoopSounds();
}


/*
========================
S_ScanChannelStarts

Returns true if any new sounds were started since the last mix
========================
*/
bool S_ScanChannelStarts(void) {
    channel_t *ch;
    sint i;
    bool newSamples;

    newSamples = false;
    ch = s_channels;

    for(i = 0; i < MAX_CHANNELS ; i++, ch++) {
        if(!ch->thesfx) {
            continue;
        }

        // if this channel was just started this frame,
        // set the sample count to it begins mixing
        // into the very first sample
        if(ch->startSample == START_SAMPLE_IMMEDIATE) {
            ch->startSample = s_paintedtime;
            newSamples = true;
            continue;
        }

        // if it is completely finished by now, clear it
        if(ch->startSample + (ch->thesfx->soundLength) <= s_paintedtime) {
            S_ChannelFree(ch);
        }
    }

    return newSamples;
}

/*
============
SOrig_Update

Called once each time through the main loop
============
*/
void SOrig_Update(void) {
    sint i, total;
    channel_t *ch;

    if(!s_soundStarted || s_soundMuted) {
        //Com_DPrintf( "not started or muted\n" );
        return;
    }

    //
    // debugging output
    //
    if(s_show->integer == 2) {
        total = 0;
        ch = s_channels;

        for(i = 0 ; i < MAX_CHANNELS; i++, ch++) {
            if(ch->thesfx && (ch->leftvol || ch->rightvol)) {
                Com_Printf("%i %i %s\n", ch->leftvol, ch->rightvol, ch->thesfx->soundName);
                total++;
            }
        }

        Com_Printf("----(%i)---- painted: %i\n", total, s_paintedtime);
    }

    // add raw data from streamed samples
    S_UpdateBackgroundTrack();

    // mix some sound
    S_Update_();
}

/*
=================
S_GetSoundtime
=================
*/
void S_GetSoundtime(void) {
    sint samplepos;
    static sint buffers, oldsamplepos;

    if(clientAVISystem->VideoRecording()) {
        s_soundtime += static_cast<sint>(ceil(dma.speed / cl_aviFrameRate->value));
        return;
    }

    // it is possible to miscount buffers if it has wrapped twice between
    // calls to S_Update.  Oh well.
    samplepos = SNDDMA_GetDMAPos();

    if(samplepos < oldsamplepos) {
        // buffer wrapped
        buffers++;

        // time to chop things off to avoid 32 bit limits
        if(s_paintedtime > 0x40000000) {
            buffers = 0;
            s_paintedtime = dma.fullsamples;
            SOrig_StopAllSounds();
        }
    }

    oldsamplepos = samplepos;

    s_soundtime = buffers * dma.fullsamples + samplepos / dma.channels;

#if 0

    // check to make sure that we haven't overshot
    if(s_paintedtime < s_soundtime) {
        Com_DPrintf("S_Update_ : overflow\n");
        s_paintedtime = s_soundtime;
    }

#endif

    if(dma.submission_chunk < 256) {
        s_paintedtime = s_soundtime + s_mixPreStep->value * dma.speed;
    } else {
        s_paintedtime = s_soundtime + dma.submission_chunk;
    }
}

/*
=================
S_Update_
=================
*/
void S_Update_(void) {
    uint endtime;
    static float32 lastTime = 0.0f;
    float32 ma, op, thisTime, sane;
    static sint ot = -1;

    if(!s_soundStarted || s_soundMuted) {
        return;
    }

    thisTime = Com_Milliseconds();

    // Updates s_soundtime
    S_GetSoundtime();

    if(s_soundtime == ot) {
        return;
    }

    ot = s_soundtime;

    // clear any sound effects that end before the current time,
    // and start any new sounds
    S_ScanChannelStarts();

    sane = thisTime - lastTime;

    if(sane < 11) {
        // 85hz
        sane = 11;
    }

    ma = s_mixahead->value * dma.speed;
    op = s_mixPreStep->value + sane * dma.speed * 0.01;

    if(op < ma) {
        ma = op;
    }

    // mix ahead of current position
    endtime = s_soundtime + ma;

    // mix to an even submission block size
    endtime = (endtime + dma.submission_chunk - 1)
              & ~(dma.submission_chunk - 1);

    // never mix more than the complete buffer
    if(endtime - s_soundtime > dma.fullsamples) {
        endtime = s_soundtime + dma.fullsamples;
    }

    SNDDMA_BeginPainting();

    S_PaintChannels(endtime);

    SNDDMA_Submit();

    lastTime = thisTime;
}

/*
===============================================================================

console functions

===============================================================================
*/

/*
=================
S_Play_f
=================
*/
void S_Play_f(void) {
    sint i;
    sfxHandle_t h;
    valueType name[256];

    i = 1;

    while(i < cmdSystem->Argc()) {
        if(!Q_strrchr(cmdSystem->Argv(i), '.')) {
            Q_vsprintf_s(name, sizeof(name), sizeof(name), "%s.wav",
                         cmdSystem->Argv(1));
        } else {
            Q_strncpyz(name, cmdSystem->Argv(i), sizeof(name));
        }

        h = SOrig_RegisterSound(name, false);

        if(h) {
            SOrig_StartLocalSound(h, CHAN_LOCAL_SOUND);
        }

        i++;
    }
}

/*
=================
S_Music_f
=================
*/
void S_Music_f(void) {
    sint        c;

    c = cmdSystem->Argc();

    if(c == 2) {
        SOrig_StartBackgroundTrack(cmdSystem->Argv(1), cmdSystem->Argv(1));
        s_backgroundLoop[0] = 0;
    } else if(c == 3) {
        SOrig_StartBackgroundTrack(cmdSystem->Argv(1), cmdSystem->Argv(2));
    } else {
        Com_Printf("music <musicfile> [loopfile]\n");
        return;
    }

}

/*
=================
S_SoundList_f
=================
*/
void S_SoundList_f(void) {
    sint i, size, total;
    sfx_t *sfx;
    valueType type[4][16], mem[2][16];

    strcpy(type[0], "16bit");
    strcpy(type[1], "adpcm");
    strcpy(type[2], "daub4");
    strcpy(type[3], "mulaw");
    strcpy(mem[0], "paged out");
    strcpy(mem[1], "resident ");
    total = 0;

    for(sfx = s_knownSfx, i = 0 ; i < s_numSfx ; i++, sfx++) {
        size = sfx->soundLength;
        total += size;
        Com_Printf("%6i[%s] : %s[%s]\n", size, type[sfx->soundCompressionMethod],
                   sfx->soundName, mem[sfx->inMemory]);
    }

    Com_Printf("Total resident: %i\n", total);

    soundSystemLocal.DisplayFreeMemory();
}

/*
===============================================================================

background music functions

===============================================================================
*/

/*
=================
S_StopBackgroundTrack
=================
*/
void SOrig_StopBackgroundTrack(void) {
    if(!s_backgroundStream) {
        return;
    }

    soundSystemLocal.codec_close(s_backgroundStream);
    s_backgroundStream = nullptr;
    s_rawend = 0;
}

/*
=================
S_StartBackgroundTrack
=================
*/
void SOrig_StartBackgroundTrack(pointer intro, pointer loop) {
    if(!intro) {
        intro = "";
    }

    if(!loop || !loop[0]) {
        loop = intro;
    }

    Com_DPrintf("S_StartBackgroundTrack( %s, %s )\n", intro, loop);

    if(!intro[0]) {
        return;
    }

    ::memmove(s_backgroundLoop, loop, sizeof(s_backgroundLoop));

    // close the background track, but DON'T reset s_rawend
    // if restarting the same back ground track
    if(s_backgroundStream) {
        soundSystemLocal.codec_close(s_backgroundStream);
        s_backgroundStream = nullptr;
    }

    // Open stream
    s_backgroundStream = soundSystemLocal.codec_open(intro);

    if(!s_backgroundStream) {
        Com_Printf(S_COLOR_YELLOW "WARNING: couldn't open music file %s\n", intro);
        return;
    }

    if(s_backgroundStream->info.channels != 2 ||
            s_backgroundStream->info.rate != 22050) {
        Com_Printf(S_COLOR_YELLOW "WARNING: music file %s is not 22k stereo\n",
                   intro);
    }
}

/*
=================
S_UpdateBackgroundTrack
=================
*/
void S_UpdateBackgroundTrack(void) {
    sint bufferSamples, fileSamples, fileBytes, r;
    uchar8 raw[30000];      // just enough to fit in a mac stack frame
    static float32 musicVolume = 0.5f;

    if(!s_backgroundStream) {
        return;
    }

    // graeme see if this is OK
    musicVolume = (musicVolume + (s_musicVolume->value * 2)) / 4.0f;

    // don't bother playing anything if musicvolume is 0
    if(musicVolume <= 0) {
        return;
    }

    // see how many samples should be copied into the raw buffer
    if(s_rawend < s_soundtime) {
        s_rawend = s_soundtime;
    }

    while(s_rawend < s_soundtime + MAX_RAW_SAMPLES) {
        bufferSamples = MAX_RAW_SAMPLES - (s_rawend - s_soundtime);

        // decide how much data needs to be read from the file
        fileSamples = bufferSamples * s_backgroundStream->info.rate / dma.speed;

        if(!fileSamples) {
            return;
        }

        // our max buffer size
        fileBytes = fileSamples * (s_backgroundStream->info.width *
                                   s_backgroundStream->info.channels);

        if(fileBytes > sizeof(raw)) {
            fileBytes = sizeof(raw);
            fileSamples = fileBytes / (s_backgroundStream->info.width *
                                       s_backgroundStream->info.channels);
        }

        // Read
        r = soundSystemLocal.codec_read(s_backgroundStream, fileBytes, raw);

        if(r < fileBytes) {
            fileBytes = r;
            fileSamples = r / (s_backgroundStream->info.width *
                               s_backgroundStream->info.channels);
        }

        if(r > 0) {
            // add to raw buffer
            SOrig_RawSamples(0, fileSamples, s_backgroundStream->info.rate,
                             s_backgroundStream->info.width, s_backgroundStream->info.channels, raw,
                             s_musicVolume->value, -1);
        } else {
            // loop
            if(s_backgroundLoop[0]) {
                soundSystemLocal.codec_close(s_backgroundStream);
                s_backgroundStream = nullptr;

                SOrig_StartBackgroundTrack(s_backgroundLoop, s_backgroundLoop);

                if(!s_backgroundStream) {
                    return;
                }
            } else {
                SOrig_StopBackgroundTrack();
                return;
            }
        }

    }
}

/*
======================
S_FreeOldestSound
======================
*/
void S_FreeOldestSound(void) {
    sint i, oldest, used;
    sfx_t *sfx;
    sndBuffer  *buffer, *nbuffer;

    oldest = Com_Milliseconds();
    used = 0;

    for(i = 1 ; i < s_numSfx ; i++) {
        sfx = &s_knownSfx[i];

        if(sfx->inMemory && sfx->lastTimeUsed < oldest) {
            used = i;
            oldest = sfx->lastTimeUsed;
        }
    }

    sfx = &s_knownSfx[used];

    Com_DPrintf("S_FreeOldestSound: freeing sound %s\n", sfx->soundName);

    buffer = sfx->soundData;

    while(buffer != nullptr) {
        nbuffer = buffer->next;
        SND_free(buffer);
        buffer = nbuffer;
    }

    sfx->inMemory = false;
    sfx->soundData = nullptr;
}

/*
======================
S_FreeOldestSound
======================
*/
sint SOrig_GetSoundLength(sfxHandle_t sfxHandle) {
    if(sfxHandle < 0 || sfxHandle >= s_numSfx) {
        Com_DPrintf(S_COLOR_YELLOW "S_StartSound: handle %i out of range\n",
                    sfxHandle);
        return -1;
    }

    return static_cast<sint>(static_cast<float32>
                             (s_knownSfx[sfxHandle].soundLength) / dma.speed * 1000.0f);
}


/*
======================
S_GetCurrentSoundTime

For looped sound synchronisation
======================
*/
sint SOrig_GetCurrentSoundTime(void) {
    return s_soundtime + dma.speed;
}

/*
================
void idSoundSystemLocal::Reload
================
*/
void idSoundSystemLocal::Reload(void) {
    sfx_t *sfx;
    sint i;

    if(!s_soundStarted) {
        return;
    }

    Com_Printf("reloading sounds...\n");

    soundSystem->StopAllSounds();

    for(sfx = s_knownSfx, i = 0; i < s_numSfx; i++, sfx++) {
        sfx->inMemory = false;
        S_memoryLoad(sfx);
    }
}
