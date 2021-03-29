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
// File name:   snd_al_sources.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <soundSystemAL/sndSystemAL_precompiled.hpp>

typedef struct src_s src_t;
struct src_s
{
    ALuint source;		// OpenAL source object
    sfxHandle_t sfx;	// Sound effect in use
    
    sint lastUse;		// Last time used
    sint priority;		// Priority
    sint entity;		// Owning entity (-1 if none)
    sint channel;		// Associated channel (-1 if none)
    
    sint isActive;		// Is this source currently in use?
    sint isLocked;		// This is locked (un-allocatable)
    sint isLooping;		// Is this a looping effect (attached to an entity)
    sint isTracking;		// Is this object tracking it's owner
    
    bool local;		// Is this local (relative to the cam)
};

#define MAX_SRC 128
static src_t srclist[MAX_SRC];
static sint src_count = 0;
static bool src_inited = false;

static sint ambient_count = 0;

typedef struct sentity_s
{
    vec3_t origin;		// Object position
    
    sint has_sfx;		// Associated sound source
    sint sfx;
    sint touched;		// Sound present this update?
} sentity_t;
static sentity_t entlist[MAX_GENTITIES];

/*
 =================
 idAudioOpenALSystemLocal::src_init

 Startup and shutdown
 =================
 */
bool idAudioOpenALSystemLocal::src_init( void )
{
    sint i;
    sint limit;
    ALenum error;
    
    // Clear the sources data structure
    ::memset( srclist, 0, sizeof( srclist ) );
    src_count = 0;
    
    // Cap s_sources to MAX_SRC
    limit = s_sources->integer;
    if( limit > MAX_SRC )
    {
        limit = MAX_SRC;
    }
    else if( limit < 16 )
    {
        limit = 16;
    }
    
    // Allocate as many sources as possible
    for( i = 0; i < limit; i++ )
    {
        qalGenSources( 1, &srclist[i].source );
        
        if( ( error = qalGetError() ) != AL_NO_ERROR )
        {
            break;
        }
        
        src_count++;
    }
    
    // All done. Print this for informational purposes
    trap_Printf( PRINT_ALL, "allocated %d sources\n", src_count );
    src_inited = true;
    
    return true;
}

/*
=================
idAudioOpenALSystemLocal::src_shutdown
=================
*/
void idAudioOpenALSystemLocal::src_shutdown( void )
{
    sint i;
    
    if( !src_inited )
    {
        return;
    }
    
    // Destroy all the sources
    for( i = 0; i < src_count; i++ )
    {
        if( srclist[i].isLocked )
        {
            trap_Printf( PRINT_DEVELOPER, "Warning: Source %d is locked\n", i );
        }
        
        qalSourceStop( srclist[i].source );
        qalDeleteSources( 1, &srclist[i].source );
    }
    
    ::memset( srclist, 0, sizeof( srclist ) );
    
    src_inited = false;
}

/*
=================
al_src_setup

Source setup
=================
*/
void idAudioOpenALSystemLocal::src_setup( srcHandle_t src, sfxHandle_t sfx, sint priority, sint entity, sint channel, bool local )
{
    ALuint buffer;
    float32 null_vector[] = {0, 0, 0};
    
    // Mark the SFX as used, and grab the raw AL buffer
    buf_use( sfx );
    buffer = buf_get( sfx );
    
    // Set up src struct
    srclist[src].lastUse = idsystem->Milliseconds();
    srclist[src].sfx = sfx;
    srclist[src].priority = priority;
    srclist[src].entity = entity;
    srclist[src].channel = channel;
    srclist[src].isActive = true;
    srclist[src].isLocked = false;
    srclist[src].isLooping = false;
    srclist[src].isTracking = false;
    srclist[src].local = local;
    
    // Set up OpenAL source
    qalSourcei( srclist[src].source, AL_BUFFER, buffer );
    qalSourcef( srclist[src].source, AL_PITCH, 1.0f );
    qalSourcef( srclist[src].source, AL_GAIN, s_gain->value * s_volume->value );
    qalSourcefv( srclist[src].source, AL_POSITION, null_vector );
    qalSourcefv( srclist[src].source, AL_VELOCITY, null_vector );
    qalSourcei( srclist[src].source, AL_LOOPING, AL_FALSE );
    qalSourcef( srclist[src].source, AL_REFERENCE_DISTANCE, s_minDistance->value );
    
    if( local )
    {
        qalSourcei( srclist[src].source, AL_SOURCE_RELATIVE, AL_TRUE );
        qalSourcef( srclist[src].source, AL_ROLLOFF_FACTOR, 0 );
    }
    else
    {
        qalSourcei( srclist[src].source, AL_SOURCE_RELATIVE, AL_FALSE );
        qalSourcef( srclist[src].source, AL_ROLLOFF_FACTOR, s_rolloff->value );
    }
}

/*
=================
idAudioOpenALSystemLocal::src_kill
=================
*/
void idAudioOpenALSystemLocal::src_kill( srcHandle_t src )
{
    // I'm not touching it. Unlock it first.
    if( srclist[src].isLocked )
    {
        return;
    }
    
    // Stop it if it's playing
    if( srclist[src].isActive )
    {
        qalSourceStop( srclist[src].source );
    }
    
    // Remove the entity association
    if( ( srclist[src].isLooping ) && ( srclist[src].entity != -1 ) )
    {
        sint ent = srclist[src].entity;
        entlist[ent].has_sfx = 0;
        entlist[ent].sfx = -1;
        entlist[ent].touched = false;
    }
    
    // Remove the buffer
    qalSourcei( srclist[src].source, AL_BUFFER, 0 );
    
    srclist[src].sfx = 0;
    srclist[src].lastUse = 0;
    srclist[src].priority = 0;
    srclist[src].entity = -1;
    srclist[src].channel = -1;
    srclist[src].isActive = false;
    srclist[src].isLocked = false;
    srclist[src].isLooping = false;
    srclist[src].isTracking = false;
}

/*
=================
idAudioOpenALSystemLocal::src_alloc

Source allocation
=================
*/
srcHandle_t idAudioOpenALSystemLocal::src_alloc( sint priority, sint entnum, sint channel )
{
    sint i;
    sint empty = -1;
    sint weakest = -1;
    sint weakest_time = idsystem->Milliseconds();
    sint weakest_pri = 999;
    
    for( i = 0; i < src_count; i++ )
    {
        // If it's locked, we aren't even going to look at it
        if( srclist[i].isLocked )
        {
            continue;
        }
        
        // Is it empty or not?
        if( ( !srclist[i].isActive ) && ( empty == -1 ) )
        {
            empty = i;
        }
        else if( srclist[i].priority < priority )
        {
            // If it's older or has lower priority, flag it as weak
            if( ( srclist[i].priority < weakest_pri ) || ( srclist[i].lastUse < weakest_time ) )
            {
                weakest_pri = srclist[i].priority;
                weakest_time = srclist[i].lastUse;
                weakest = i;
            }
        }
        
        // Is it an exact match, and not on channel 0?
        if( ( srclist[i].entity == entnum ) && ( srclist[i].channel == channel ) && ( channel != 0 ) )
        {
            src_kill( i );
            return i;
        }
    }
    
    // Do we have an empty one?
    if( empty != -1 )
    {
        return empty;
    }
    
    // No. How about an overridable one?
    if( weakest != -1 )
    {
        src_kill( weakest );
        
        return weakest;
    }
    
    // Nothing. Return failure (cries...)
    return -1;
}

/*
=================
idAudioOpenALSystemLocal::src_find

Finds an active source with matching entity and channel numbers
Returns -1 if there isn't one
=================
*/
srcHandle_t idAudioOpenALSystemLocal::src_find( sint entnum, sint channel )
{
    sint i;
    
    for( i = 0; i < src_count; i++ )
    {
        if( !srclist[i].isActive )
        {
            continue;
        }
        
        if( ( srclist[i].entity == entnum ) && ( srclist[i].channel == channel ) )
        {
            return i;
        }
    }
    
    return -1;
}

/*
=================
idAudioOpenALSystemLocal::src_lock

Locks and unlocks a source
Locked sources will not be automatically reallocated or managed
Once unlocked, the source may be reallocated again
=================
*/
void idAudioOpenALSystemLocal::src_lock( srcHandle_t src )
{
    srclist[src].isLocked = true;
}

/*
=================
idAudioOpenALSystemLocal::src_unlock
=================
*/
void idAudioOpenALSystemLocal::src_unlock( srcHandle_t src )
{
    srclist[src].isLocked = false;
}

/*
=================
idAudioOpenALSystemLocal::UpdateEntityPosition

Entity position management
=================
*/
void idAudioOpenALSystemLocal::UpdateEntityPosition( sint entityNum, const vec3_t origin )
{
    if( entityNum < 0 || entityNum > MAX_GENTITIES )
    {
        trap_Error( ERR_DROP, "S_UpdateEntityPosition: bad entitynum %i", entityNum );
    }
    
    VectorCopy( origin, entlist[entityNum].origin );
}

/*
=================
idAudioOpenALSystemLocal::StartLocalSound

Play a local (non-spatialized) sound effect
=================
*/
void idAudioOpenALSystemLocal::StartLocalSound( sfxHandle_t sfx, sint channel )
{
    // Try to grab a source
    srcHandle_t src = src_alloc( SRCPRI_LOCAL, -1, channel );
    if( src == -1 )
    {
        return;
    }
    
    // Set up the effect
    src_setup( src, sfx, SRCPRI_LOCAL, -1, channel, true );
    
    // Start it playing
    qalSourcePlay( srclist[src].source );
}

/*
=================
idAudioOpenALSystemLocal::StartSound

Play a one-shot sound effect
=================
*/
void idAudioOpenALSystemLocal::StartSound( vec3_t origin, sint entnum, sint entchannel, sfxHandle_t sfx )
{
    vec3_t sorigin;
    
    // Try to grab a source
    srcHandle_t src = src_alloc( SRCPRI_ONESHOT, entnum, entchannel );
    if( src == -1 )
    {
        return;
    }
    
    // Set up the effect
    src_setup( src, sfx, SRCPRI_ONESHOT, entnum, entchannel, false );
    
    if( origin == nullptr )
    {
        srclist[src].isTracking = true;
        VectorScale( entlist[entnum].origin, POSITION_SCALE, sorigin );
    }
    else
    {
        VectorScale( origin, POSITION_SCALE, sorigin );
    }
    
    qalSourcefv( srclist[src].source, AL_POSITION, sorigin );
    
    // Start it playing
    qalSourcePlay( srclist[src].source );
}

/*
=================
idAudioOpenALSystemLocal::SoundDuration
=================
*/
sint idAudioOpenALSystemLocal::SoundDuration( sfxHandle_t sfx )
{
    return duration( sfx );
}

/*
=================
idAudioOpenALSystemLocal::SoundDuration

Start a looping sound effect
=================
*/
void idAudioOpenALSystemLocal::ClearLoopingSounds( bool killall )
{
    sint i;
    
    for( i = 0; i < src_count; i++ )
    {
        if( ( srclist[i].isLooping ) && ( srclist[i].entity != -1 ) )
        {
            entlist[srclist[i].entity].touched = false;
        }
    }
}

/*
=================
idAudioOpenALSystemLocal::src_loop
=================
*/
void idAudioOpenALSystemLocal::src_loop( sint priority, sfxHandle_t sfx, const vec3_t origin, const vec3_t velocity, sint entnum )
{
    sint src;
    bool need_to_play = false;
    vec3_t sorigin;
    
    // Do we need to start a new sound playing?
    if( !entlist[entnum].has_sfx )
    {
        // Try to get a channel
        ambient_count++;
        src = src_alloc( priority, entnum, -1 );
        
        if( src == -1 )
        {
            return;
        }
        
        need_to_play = true;
    }
    else if( srclist[entlist[entnum].sfx].sfx != sfx )
    {
        // Need to restart. Just re-use this channel
        src = entlist[entnum].sfx;
        src_kill( src );
        need_to_play = true;
    }
    else
    {
        src = entlist[entnum].sfx;
    }
    
    if( need_to_play )
    {
        // Set up the effect
        src_setup( src, sfx, priority, entnum, -1, false );
        qalSourcei( srclist[src].source, AL_LOOPING, AL_TRUE );
        srclist[src].isLooping = true;
        
        // Set up the entity
        entlist[entnum].has_sfx = true;
        entlist[entnum].sfx = src;
        need_to_play = true;
    }
    
    // Set up the position and velocity
    VectorScale( entlist[entnum].origin, POSITION_SCALE, sorigin );
    qalSourcefv( srclist[src].source, AL_POSITION, sorigin );
    qalSourcefv( srclist[src].source, AL_VELOCITY, velocity );
    
    // Flag it
    entlist[entnum].touched = true;
    
    // Play if need be
    if( need_to_play )
    {
        qalSourcePlay( srclist[src].source );
    }
}

/*
=================
al_src_loop
=================
*/
void idAudioOpenALSystemLocal::AddLoopingSound( sint entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx )
{
    src_loop( SRCPRI_AMBIENT, sfx, origin, velocity, entityNum );
}

/*
=================
idAudioOpenALSystemLocal::AddRealLoopingSound
=================
*/
void idAudioOpenALSystemLocal::AddRealLoopingSound( sint entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx )
{
    src_loop( SRCPRI_ENTITY, sfx, origin, velocity, entityNum );
}

/*
=================
idAudioOpenALSystemLocal::StopLoopingSound
=================
*/
void idAudioOpenALSystemLocal::StopLoopingSound( sint entityNum )
{
    if( entlist[entityNum].has_sfx )
    {
        src_kill( entlist[entityNum].sfx );
    }
}

/*
=================
idAudioOpenALSystemLocal::src_update

Update state (move things around, manage sources, and so on)
=================
*/
void idAudioOpenALSystemLocal::src_update( void )
{
    sint i;
    sint ent;
    ALint state;
    
    for( i = 0; i < src_count; i++ )
    {
        if( srclist[i].isLocked )
        {
            continue;
        }
        
        if( !srclist[i].isActive )
        {
            continue;
        }
        
        // Check if it's done, and flag it
        qalGetSourcei( srclist[i].source, AL_SOURCE_STATE, &state );
        if( state == AL_STOPPED )
        {
            src_kill( i );
            continue;
        }
        
        // Update source parameters
        if( ( s_gain->modified ) || ( s_volume->modified ) )
        {
            qalSourcef( srclist[i].source, AL_GAIN, s_gain->value * s_volume->value );
        }
        
        if( ( s_rolloff->modified ) && ( !srclist[i].local ) )
        {
            qalSourcef( srclist[i].source, AL_ROLLOFF_FACTOR, s_rolloff->value );
        }
        
        if( s_minDistance->modified )
        {
            qalSourcef( srclist[i].source, AL_REFERENCE_DISTANCE, s_minDistance->value );
        }
        
        ent = srclist[i].entity;
        
        // If a looping effect hasn't been touched this frame, kill it
        if( srclist[i].isLooping )
        {
            if( !entlist[ent].touched )
            {
                ambient_count--;
                src_kill( i );
            }
            continue;
        }
        
        // See if it needs to be moved
        if( srclist[i].isTracking )
        {
            vec3_t sorigin;
            VectorScale( entlist[ent].origin, POSITION_SCALE, sorigin );
            qalSourcefv( srclist[i].source, AL_POSITION, entlist[ent].origin );
        }
    }
}

/*
=================
idAudioOpenALSystemLocal::src_shutup
=================
*/
void idAudioOpenALSystemLocal::src_shutup( void )
{
    sint i;
    
    for( i = 0; i < src_count; i++ )
    {
        src_kill( i );
    }
}

/*
=================
idAudioOpenALSystemLocal::src_get
=================
*/
ALuint idAudioOpenALSystemLocal::src_get( srcHandle_t src )
{
    return srclist[src].source;
}


/*
======================
idAudioOpenALSystemLocal::GetVoiceAmplitude
======================
*/
sint idAudioOpenALSystemLocal::GetVoiceAmplitude( sint entnum )
{
    return 0;
}
