////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2005 Stuart Dalton (badcdev@gmail.com)
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
// File name:   s_openal.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

// Console variables specific to OpenAL
convar_t* s_alPrecache;
convar_t* s_alGain;
convar_t* s_alSources;
convar_t* s_alDopplerFactor;
convar_t* s_alDopplerSpeed;
convar_t* s_alMinDistance;
convar_t* s_alMaxDistance;
convar_t* s_alRolloff;
convar_t* s_alGraceDistance;
convar_t* s_alDriver;
convar_t* s_alDevice;
convar_t* s_alAvailableDevices;

bool QAL_Init( pointer libname )
{
    return true;
}

/*
=================
S_AL_Format
=================
*/
static ALuint S_AL_Format( sint width, sint channels )
{
    ALuint format = AL_FORMAT_MONO16;
    
    // Work out format
    if( width == 1 )
    {
        if( channels == 1 )
        {
            format = AL_FORMAT_MONO8;
        }
        else if( channels == 2 )
        {
            format = AL_FORMAT_STEREO8;
        }
    }
    else if( width == 2 )
    {
        if( channels == 1 )
        {
            format = AL_FORMAT_MONO16;
        }
        else if( channels == 2 )
        {
            format = AL_FORMAT_STEREO16;
        }
    }
    
    return format;
}

/*
=================
S_AL_ErrorMsg
=================
*/
static pointer S_AL_ErrorMsg( ALenum error )
{
    switch( error )
    {
        case AL_NO_ERROR:
            return "No error";
        case AL_INVALID_NAME:
            return "Invalid name";
        case AL_INVALID_ENUM:
            return "Invalid enumerator";
        case AL_INVALID_VALUE:
            return "Invalid value";
        case AL_INVALID_OPERATION:
            return "Invalid operation";
        case AL_OUT_OF_MEMORY:
            return "Out of memory";
        default:
            return "Unknown error";
    }
}

typedef struct alSfx_s
{
    valueType filename[MAX_QPATH];
    uint buffer; // OpenAL buffer
    bool isDefault; // Couldn't be loaded - use default FX
    bool inMemory; // Sound is stored in memory
    bool isLocked; // Sound is locked (can not be unloaded)
    sint lastUsedTime; // Time last used
} alSfx_t;

static bool alBuffersInitialised = false;

// Sound effect storage, data structures
#define MAX_SFX 4096
static alSfx_t knownSfx[MAX_SFX];
static sint numSfx = 0;

static sfxHandle_t default_sfx;

/*
=================
S_AL_BufferFindFree

Find a free handle
=================
*/
static sfxHandle_t S_AL_BufferFindFree( void )
{
    sint i;
    
    for( i = 0; i < MAX_SFX; i++ )
    {
        // Got one
        if( knownSfx[i].filename[0] == '\0' )
        {
            if( i >= numSfx )
            {
                numSfx = i + 1;
            }
            return i;
        }
    }
    
    // Shit...
    Com_Error( ERR_FATAL, "S_AL_BufferFindFree: No free sound handles" );
    return -1;
}

/*
=================
S_AL_BufferFind

Find a sound effect if loaded, set up a handle otherwise
=================
*/
static sfxHandle_t S_AL_BufferFind( pointer filename )
{
    // Look it up in the table
    sfxHandle_t sfx = -1;
    sint i;
    
    for( i = 0; i < numSfx; i++ )
    {
        if( !Q_stricmp( knownSfx[i].filename, filename ) )
        {
            sfx = i;
            break;
        }
    }
    
    // Not found in table?
    if( sfx == -1 )
    {
        alSfx_t* ptr;
        
        sfx = S_AL_BufferFindFree();
        
        // Clear and copy the filename over
        ptr = &knownSfx[sfx];
        ::memset( ptr, 0, sizeof( *ptr ) );
        ::strcpy( ptr->filename, filename );
    }
    
    // Return the handle
    return sfx;
}

/*
=================
S_AL_BufferUseDefault
=================
*/
static void S_AL_BufferUseDefault( sfxHandle_t sfx )
{
    if( sfx == default_sfx )
    {
        Com_Error( ERR_FATAL, "Can't load default sound effect %s\n", knownSfx[sfx].filename );
    }
    
    Com_Printf( S_COLOR_YELLOW "WARNING: Using default sound for %s\n", knownSfx[sfx].filename );
    
    knownSfx[sfx].isDefault = true;
    knownSfx[sfx].buffer = knownSfx[default_sfx].buffer;
}

/*
=================
S_AL_BufferUnload
=================
*/
static void S_AL_BufferUnload( sfxHandle_t sfx )
{
    sint error;
    
    if( knownSfx[sfx].filename[0] == '\0' )
    {
        return;
    }
    
    if( !knownSfx[sfx].inMemory )
    {
        return;
    }
    
    // Delete it
    alDeleteBuffers( 1, &knownSfx[sfx].buffer );
    
    if( ( error = alGetError() ) != AL_NO_ERROR )
    {
        Com_Printf( S_COLOR_RED "ERROR: Can't delete sound buffer for %s\n", knownSfx[sfx].filename );
    }
    
    knownSfx[sfx].inMemory = false;
}

/*
=================
S_AL_BufferEvict
=================
*/
static bool S_AL_BufferEvict( void )
{
    sint	i, oldestBuffer = -1;
    sint	oldestTime = idsystem->Milliseconds( );
    
    for( i = 0; i < numSfx; i++ )
    {
        if( !knownSfx[i].filename[0] )
        {
            continue;
        }
        
        if( !knownSfx[i].inMemory )
        {
            continue;
        }
        
        if( knownSfx[ i ].lastUsedTime < oldestTime )
        {
            oldestTime = knownSfx[ i ].lastUsedTime;
            oldestBuffer = i;
        }
    }
    
    if( oldestBuffer >= 0 )
    {
        S_AL_BufferUnload( oldestBuffer );
        return true;
    }
    else
    {
        return false;
    }
}

/*
=================
S_AL_BufferLoad
=================
*/
static void S_AL_BufferLoad( sfxHandle_t sfx )
{
    sint error;
    void* data;
    snd_info_t info;
    uint format;
    
    // Nothing?
    if( knownSfx[sfx].filename[0] == '\0' )
    {
        return;
    }
    
    // Player SFX
    if( knownSfx[sfx].filename[0] == '*' )
    {
        return;
    }
    
    // Already done?
    if( ( knownSfx[sfx].inMemory ) || ( knownSfx[sfx].isDefault ) )
    {
        return;
    }
    
    // Try to load
    data = S_CodecLoad( knownSfx[sfx].filename, &info );
    if( !data )
    {
        S_AL_BufferUseDefault( sfx );
        return;
    }
    
    format = S_AL_Format( info.width, info.channels );
    
    // Create a buffer
    alGenBuffers( 1, &knownSfx[sfx].buffer );
    if( ( error = alGetError() ) != AL_NO_ERROR )
    {
        S_AL_BufferUseDefault( sfx );
        Z_Free( data );
        Com_Printf( S_COLOR_RED "ERROR: Can't create a sound buffer for %s - %s\n", knownSfx[sfx].filename, S_AL_ErrorMsg( error ) );
        return;
    }
    
    // Fill the buffer
    if( info.size == 0 )
    {
        // We have no data to buffer, so buffer silence
        uchar8 dummyData[ 2 ] = { 0 };
        
        alBufferData( knownSfx[sfx].buffer, AL_FORMAT_MONO16, ( void* )dummyData, 2, 22050 );
    }
    else
    {
        alBufferData( knownSfx[sfx].buffer, format, data, info.size, info.rate );
    }
    
    error = alGetError();
    
    // If we ran out of memory, start evicting the least recently used sounds
    while( error == AL_OUT_OF_MEMORY )
    {
        if( !S_AL_BufferEvict( ) )
        {
            S_AL_BufferUseDefault( sfx );
            Z_Free( data );
            Com_Printf( S_COLOR_RED "ERROR: Out of memory loading %s\n", knownSfx[sfx].filename );
            return;
        }
        
        // Try load it again
        alBufferData( knownSfx[sfx].buffer, format, data, info.size, info.rate );
        error = alGetError();
    }
    
    // Some other error condition
    if( error != AL_NO_ERROR )
    {
        S_AL_BufferUseDefault( sfx );
        Z_Free( data );
        Com_Printf( S_COLOR_RED "ERROR: Can't fill sound buffer for %s - %s\n", knownSfx[sfx].filename, S_AL_ErrorMsg( error ) );
        return;
    }
    
    // Free the memory
    Z_Free( data );
    
    // Woo!
    knownSfx[sfx].inMemory = true;
}

/*
=================
S_AL_BufferUse
=================
*/
static void S_AL_BufferUse( sfxHandle_t sfx )
{
    if( knownSfx[sfx].filename[0] == '\0' )
    {
        return;
    }
    
    if( ( !knownSfx[sfx].inMemory ) && ( !knownSfx[sfx].isDefault ) )
    {
        S_AL_BufferLoad( sfx );
    }
    
    knownSfx[sfx].lastUsedTime = idsystem->Milliseconds();
}

/*
=================
S_AL_BufferInit
=================
*/
static bool S_AL_BufferInit( void )
{
    if( alBuffersInitialised )
    {
        return true;
    }
    
    // Clear the hash table, and SFX table
    ::memset( knownSfx, 0, sizeof( knownSfx ) );
    numSfx = 0;
    
    // Load the default sound, and lock it
    default_sfx = S_AL_BufferFind( "sound/null.wav" );
    S_AL_BufferUse( default_sfx );
    knownSfx[default_sfx].isLocked = true;
    
    // All done
    alBuffersInitialised = true;
    return true;
}

/*
=================
S_AL_BufferShutdown
=================
*/
static void S_AL_BufferShutdown( void )
{
    sint i;
    
    if( !alBuffersInitialised )
    {
        return;
    }
    
    // Unlock the default sound effect
    knownSfx[default_sfx].isLocked = false;
    
    // Free all used effects
    for( i = 0; i < numSfx; i++ )
    {
        S_AL_BufferUnload( i );
    }
    
    // Clear the tables
    ::memset( knownSfx, 0, sizeof( knownSfx ) );
    
    // All undone
    alBuffersInitialised = false;
}

/*
=================
S_AL_RegisterSound
=================
*/
static sfxHandle_t S_AL_RegisterSound( pointer sample, bool compressed )
{
    sfxHandle_t sfx = S_AL_BufferFind( sample );
    
    if( s_alPrecache->integer && ( !knownSfx[sfx].inMemory ) && ( !knownSfx[sfx].isDefault ) )
    {
        S_AL_BufferLoad( sfx );
    }
    
    knownSfx[sfx].lastUsedTime = Com_Milliseconds();
    
    return sfx;
}

/*
=================
S_AL_BufferGet

Return's an sfx's buffer
=================
*/
static uint S_AL_BufferGet( sfxHandle_t sfx )
{
    return knownSfx[sfx].buffer;
}

typedef struct src_s
{
    uint alSource; // OpenAL source object
    sfxHandle_t sfx; // Sound effect in use
    
    sint lastUsedTime; // Last time used
    alSrcPriority_t priority; // Priority
    sint entity; // Owning entity (-1 if none)
    sint channel; // Associated channel (-1 if none)
    
    sint isActive; // Is this source currently in use?
    sint isLocked; // This is locked (un-allocatable)
    sint isLooping; // Is this a looping effect (attached to an entity)
    sint isTracking; // Is this object tracking it's owner
    
    float32 curGain; // gain employed if source is within maxdistance.
    float32 scaleGain; // Last gain value for this source. 0 if muted.
    
    bool local; // Is this local (relative to the cam)
} src_t;

#ifdef MACOS_X
#define MAX_SRC 64
#else
#define MAX_SRC 128
#endif
static src_t srcList[MAX_SRC];
static sint srcCount = 0;
static bool alSourcesInitialised = false;
static vec3_t lastListenerOrigin = { 0.0f, 0.0f, 0.0f };

typedef struct sentity_s
{
    vec3_t origin;
    
    sint srcAllocated; // If a src_t has been allocated to this entity
    sint srcIndex;
    
    bool loopAddedThisFrame;
    alSrcPriority_t loopPriority;
    sfxHandle_t loopSfx;
    bool startLoopingSound;
} sentity_t;

static sentity_t entityList[MAX_GENTITIES];

/*
=================
S_AL_SanitiseVector
=================
*/
#define S_AL_SanitiseVector(v) _S_AL_SanitiseVector(v,__LINE__)
static void _S_AL_SanitiseVector( vec3_t v, sint line )
{
    if( Q_isnan( v[ 0 ] ) || Q_isnan( v[ 1 ] ) || Q_isnan( v[ 2 ] ) )
    {
        Com_DPrintf( S_COLOR_YELLOW "WARNING: vector with one or more NaN components "
                     "being passed to OpenAL at %s:%d -- zeroing\n", __FILE__, line );
        VectorClear( v );
    }
}


#define AL_THIRD_PERSON_THRESHOLD_SQ (48.0f*48.0f)

/*
=================
S_AL_ScaleGain
Adapt the gain if necessary to get a quicker fadeout when the source is too far away.
=================
*/

static void S_AL_ScaleGain( src_t* chksrc, vec3_t origin )
{
    float32 distance;
    
    if( !chksrc->local )
    {
        distance = Distance( origin, lastListenerOrigin );
    }
    
    // If we exceed a certain distance, scale the gain linearly until the sound
    // vanishes into nothingness.
    if( !chksrc->local && ( distance -= s_alMaxDistance->value ) > 0 )
    {
        float32 scaleFactor;
        
        if( distance >= s_alGraceDistance->value )
        {
            scaleFactor = 0.0f;
        }
        else
        {
            scaleFactor = 1.0f - distance / s_alGraceDistance->value;
        }
        
        scaleFactor *= chksrc->curGain;
        
        if( chksrc->scaleGain != scaleFactor )
        {
            chksrc->scaleGain = scaleFactor;
            // if(scaleFactor > 0.0f)
            // Com_Printf("%f\n", scaleFactor);
            alSourcef( chksrc->alSource, AL_GAIN, chksrc->scaleGain );
        }
    }
    else if( chksrc->scaleGain != chksrc->curGain )
    {
        chksrc->scaleGain = chksrc->curGain;
        alSourcef( chksrc->alSource, AL_GAIN, chksrc->scaleGain );
    }
}

/*
=================
S_AL_HearingThroughEntity
=================
*/
static bool S_AL_HearingThroughEntity( sint entityNum )
{
    float32 distanceSq;
    
    if( clc.clientNum == entityNum )
    {
        // FIXME: <tim@ngus.net> 28/02/06 This is an outrageous hack to detect
        // whether or not the player is rendering in third person or not. We can't
        // ask the renderer because the renderer has no notion of entities and we
        // can't ask cgame since that would involve changing the API and hence mod
        // compatibility. I don't think there is any way around this, but I'll leave
        // the FIXME just in case anyone has a bright idea.
        distanceSq = DistanceSquared( entityList[ entityNum ].origin, lastListenerOrigin );
        
        if( distanceSq > AL_THIRD_PERSON_THRESHOLD_SQ )
        {
            //we're the player, but third person
            return false;
        }
        else
        {
            //we're the player
            return true;
        }
    }
    else
    {
        //not the player
        return false;
    }
}

/*
=================
S_AL_SrcInit
=================
*/
static bool S_AL_SrcInit( void )
{
    sint i;
    sint limit;
    sint error;
    
    // Clear the sources data structure
    ::memset( srcList, 0, sizeof( srcList ) );
    srcCount = 0;
    
    // Cap s_alSources to MAX_SRC
    limit = s_alSources->integer;
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
        alGenSources( 1, &srcList[i].alSource );
        if( ( error = alGetError() ) != AL_NO_ERROR )
        {
            break;
        }
        srcCount++;
    }
    
    // All done. Print this for informational purposes
    Com_Printf( "Allocated %d sources.\n", srcCount );
    alSourcesInitialised = true;
    return true;
}

/*
=================
S_AL_SrcShutdown
=================
*/
static void S_AL_SrcShutdown( void )
{
    sint i;
    
    if( !alSourcesInitialised )
    {
        return;
    }
    
    // Destroy all the sources
    for( i = 0; i < srcCount; i++ )
    {
        if( srcList[i].isLocked )
        {
            Com_DPrintf( S_COLOR_YELLOW "WARNING: Source %d is locked\n", i );
        }
        
        alSourceStop( srcList[i].alSource );
        alDeleteSources( 1, &srcList[i].alSource );
    }
    
    ::memset( srcList, 0, sizeof( srcList ) );
    
    alSourcesInitialised = false;
}

/*
=================
S_AL_SrcSetup
=================
*/
static void S_AL_SrcSetup( srcHandle_t src, sfxHandle_t sfx, alSrcPriority_t priority, sint entity, sint channel, bool local )
{
    uint buffer;
    src_t* curSource;
    
    // Mark the SFX as used, and grab the raw AL buffer
    S_AL_BufferUse( sfx );
    buffer = S_AL_BufferGet( sfx );
    
    // Set up src struct
    curSource = &srcList[src];
    
    curSource->lastUsedTime = idsystem->Milliseconds();
    curSource->sfx = sfx;
    curSource->priority = priority;
    curSource->entity = entity;
    curSource->channel = channel;
    curSource->isActive = true;
    curSource->isLocked = false;
    curSource->isLooping = false;
    curSource->isTracking = false;
    curSource->curGain = s_alGain->value * s_volume->value;
    curSource->scaleGain = curSource->curGain;
    curSource->local = local;
    
    // Set up OpenAL source
    alSourcei( curSource->alSource, AL_BUFFER, buffer );
    alSourcef( curSource->alSource, AL_PITCH, 1.0f );
    alSourcef( curSource->alSource, AL_GAIN, curSource->curGain );
    alSourcefv( curSource->alSource, AL_POSITION, vec3_origin );
    alSourcefv( curSource->alSource, AL_VELOCITY, vec3_origin );
    alSourcei( curSource->alSource, AL_LOOPING, AL_FALSE );
    alSourcef( curSource->alSource, AL_REFERENCE_DISTANCE, s_alMinDistance->value );
    
    if( local )
    {
        alSourcei( curSource->alSource, AL_SOURCE_RELATIVE, AL_TRUE );
        alSourcef( curSource->alSource, AL_ROLLOFF_FACTOR, 0.0f );
    }
    else
    {
        alSourcei( curSource->alSource, AL_SOURCE_RELATIVE, AL_FALSE );
        alSourcef( curSource->alSource, AL_ROLLOFF_FACTOR, s_alRolloff->value );
    }
}

/*
=================
S_AL_SrcKill
=================
*/
static void S_AL_SrcKill( srcHandle_t src )
{
    // I'm not touching it. Unlock it first.
    if( srcList[src].isLocked )
    {
        return;
    }
    
    // Stop it if it's playing
    if( srcList[src].isActive )
    {
        alSourceStop( srcList[src].alSource );
    }
    
    // Remove the entity association
    if( ( srcList[src].isLooping ) && ( srcList[src].entity != -1 ) )
    {
        sint ent = srcList[src].entity;
        entityList[ent].srcAllocated = false;
        entityList[ent].srcIndex = -1;
        entityList[ent].loopAddedThisFrame = false;
        entityList[ent].startLoopingSound = false;
    }
    
    // Remove the buffer
    alSourcei( srcList[src].alSource, AL_BUFFER, 0 );
    
    srcList[src].sfx = 0;
    srcList[src].lastUsedTime = 0;
    srcList[src].priority = ( alSrcPriority_t )0;
    srcList[src].entity = -1;
    srcList[src].channel = -1;
    srcList[src].isActive = false;
    srcList[src].isLocked = false;
    srcList[src].isLooping = false;
    srcList[src].isTracking = false;
    srcList[src].local = false;
}

/*
=================
S_AL_SrcAlloc
=================
*/
static srcHandle_t S_AL_SrcAlloc( alSrcPriority_t priority, sint entnum, sint channel )
{
    sint i;
    sint empty = -1;
    sint weakest = -1;
    sint weakest_time = idsystem->Milliseconds();
    sint weakest_pri = 999;
    
    for( i = 0; i < srcCount; i++ )
    {
        // If it's locked, we aren't even going to look at it
        if( srcList[i].isLocked )
        {
            continue;
        }
        
        // Is it empty or not?
        if( ( !srcList[i].isActive ) && ( empty == -1 ) )
        {
            empty = i;
        }
        else if( srcList[i].priority < priority )
        {
            // If it's older or has lower priority, flag it as weak
            if( ( srcList[i].priority < weakest_pri ) || ( srcList[i].lastUsedTime < weakest_time ) )
            {
                weakest_pri = srcList[i].priority;
                weakest_time = srcList[i].lastUsedTime;
                weakest = i;
            }
        }
        
        // The channel system is not actually adhered to by baseq3, and not
        // implemented in snd_dma.c, so while the following is strictly correct, it
        // causes incorrect behaviour versus defacto baseq3
#if 0
        // Is it an exact match, and not on channel 0?
        if( ( srcList[i].entity == entnum ) && ( srcList[i].channel == channel ) && ( channel != 0 ) )
        {
            S_AL_SrcKill( i );
            return i;
        }
#endif
    }
    
    // Do we have an empty one?
    if( empty != -1 )
    {
        S_AL_SrcKill( empty );
        return empty;
    }
    
    // No. How about an overridable one?
    if( weakest != -1 )
    {
        S_AL_SrcKill( weakest );
        return weakest;
    }
    
    // Nothing. Return failure (cries...)
    return -1;
}

/*
=================
S_AL_SrcFind

Finds an active source with matching entity and channel numbers
Returns -1 if there isn't one
=================
*/
#if 0
static srcHandle_t S_AL_SrcFind( sint entnum, sint channel )
{
    sint i;
    
    for( i = 0; i < srcCount; i++ )
    {
        if( !srcList[i].isActive )
        {
            continue;
        }
        
        if( ( srcList[i].entity == entnum ) && ( srcList[i].channel == channel ) )
        {
            return i;
        }
    }
    
    return -1;
}
#endif

/*
=================
S_AL_SrcLock

Locked sources will not be automatically reallocated or managed
=================
*/
static void S_AL_SrcLock( srcHandle_t src )
{
    srcList[src].isLocked = true;
}

/*
=================
S_AL_SrcUnlock

Once unlocked, the source may be reallocated again
=================
*/
static void S_AL_SrcUnlock( srcHandle_t src )
{
    srcList[src].isLocked = false;
}

/*
=================
S_AL_UpdateEntityPosition
=================
*/
static void S_AL_UpdateEntityPosition( sint entityNum, const vec3_t origin )
{
    S_AL_SanitiseVector( ( vec_t* )origin );
    
    if( entityNum < 0 || entityNum > MAX_GENTITIES )
    {
        Com_Error( ERR_DROP, "S_UpdateEntityPosition: bad entitynum %i", entityNum );
    }
    
    VectorCopy( origin, entityList[entityNum].origin );
}

/*
=================
S_AL_CheckInput
Check whether input values from mods are out of range.
Necessary for i.g. Western Quake3 mod which is buggy.
=================
*/
static bool S_AL_CheckInput( sint entityNum, sfxHandle_t sfx )
{
    if( entityNum < 0 || entityNum > MAX_GENTITIES )
    {
        Com_Error( ERR_DROP, "S_StartSound: bad entitynum %i", entityNum );
    }
    
    if( sfx < 0 || sfx >= numSfx )
    {
        Com_Printf( S_COLOR_RED "ERROR: S_AL_CheckInput: handle %i out of range\n", sfx );
        return true;
    }
    
    return false;
}

/*
=================
S_AL_StartLocalSound

Play a local (non-spatialized) sound effect
=================
*/
static void S_AL_StartLocalSound( sfxHandle_t sfx, sint channel )
{
    srcHandle_t src;
    
    if( S_AL_CheckInput( 0, sfx ) )
    {
        return;
    }
    
    // Try to grab a source
    src = S_AL_SrcAlloc( SRCPRI_LOCAL, -1, channel );
    
    if( src == -1 )
    {
        return;
    }
    
    // Set up the effect
    S_AL_SrcSetup( src, sfx, SRCPRI_LOCAL, -1, channel, true );
    
    // Start it playing
    alSourcePlay( srcList[src].alSource );
}

/*
=================
S_AL_StartSound

Play a one-shot sound effect
=================
*/
static void S_AL_StartSound( vec3_t origin, sint entnum, sint entchannel, sfxHandle_t sfx )
{
    vec3_t sorigin;
    srcHandle_t src;
    
    if( S_AL_CheckInput( origin ? 0 : entnum, sfx ) )
    {
        return;
    }
    
    // Try to grab a source
    src = S_AL_SrcAlloc( SRCPRI_ONESHOT, entnum, entchannel );
    if( src == -1 )
    {
        return;
    }
    
    // Set up the effect
    if( origin == nullptr )
    {
        if( S_AL_HearingThroughEntity( entnum ) )
        {
            // Where the entity is the local player, play a local sound
            S_AL_SrcSetup( src, sfx, SRCPRI_ONESHOT, entnum, entchannel, true );
            VectorClear( sorigin );
        }
        else
        {
            S_AL_SrcSetup( src, sfx, SRCPRI_ONESHOT, entnum, entchannel, false );
            VectorCopy( entityList[ entnum ].origin, sorigin );
        }
        srcList[ src ].isTracking = true;
    }
    else
    {
        S_AL_SrcSetup( src, sfx, SRCPRI_ONESHOT, entnum, entchannel, false );
        VectorCopy( origin, sorigin );
    }
    
    S_AL_SanitiseVector( sorigin );
    alSourcefv( srcList[ src ].alSource, AL_POSITION, sorigin );
    S_AL_ScaleGain( &srcList[src], sorigin );
    
    // Start it playing
    alSourcePlay( srcList[src].alSource );
}

/*
=================
S_AL_ClearLoopingSounds
=================
*/
static void S_AL_ClearLoopingSounds( bool killall )
{
    sint i;
    
    for( i = 0; i < srcCount; i++ )
    {
        if( ( srcList[i].isLooping ) && ( srcList[i].entity != -1 ) )
        {
            entityList[srcList[i].entity].loopAddedThisFrame = false;
        }
    }
}

/*
=================
S_AL_SrcLoop
=================
*/
static void S_AL_SrcLoop( alSrcPriority_t priority, sfxHandle_t sfx, const vec3_t origin, const vec3_t velocity, sint entityNum )
{
    sint src;
    sentity_t* sent = &entityList[ entityNum ];
    src_t* curSource;
    
    // Do we need to allocate a new source for this entity
    if( !sent->srcAllocated )
    {
        // Try to get a channel
        src = S_AL_SrcAlloc( priority, entityNum, -1 );
        
        if( src == -1 )
        {
            Com_DPrintf( S_COLOR_YELLOW "WARNING: Failed to allocate source "
                         "for loop sfx %d on entity %d\n", sfx, entityNum );
            return;
        }
        
        sent->startLoopingSound = true;
    }
    else
    {
        src = sent->srcIndex;
    }
    
    sent->srcAllocated = true;
    sent->srcIndex = src;
    
    sent->loopPriority = priority;
    sent->loopSfx = sfx;
    
    // If this is not set then the looping sound is removed
    sent->loopAddedThisFrame = true;
    
    curSource = &srcList[src];
    
    // UGH
    // These lines should be called via S_AL_SrcSetup, but we
    // can't call that yet as it buffers sfxes that may change
    // with subsequent calls to S_AL_SrcLoop
    curSource->entity = entityNum;
    curSource->isLooping = true;
    curSource->isActive = true;
    
    if( S_AL_HearingThroughEntity( entityNum ) )
    {
        curSource->local = true;
        
        alSourcefv( curSource->alSource, AL_POSITION, vec3_origin );
        alSourcefv( curSource->alSource, AL_VELOCITY, vec3_origin );
    }
    else
    {
        curSource->local = false;
        
        alSourcefv( curSource->alSource, AL_POSITION, ( float32* )sent->origin );
        alSourcefv( curSource->alSource, AL_VELOCITY, ( float32* )velocity );
        
    }
    
    S_AL_ScaleGain( curSource, sent->origin );
}

/*
=================
S_AL_AddLoopingSound
=================
*/
static void S_AL_AddLoopingSound( sint entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx )
{
    if( S_AL_CheckInput( entityNum, sfx ) )
    {
        return;
    }
    
    S_AL_SanitiseVector( ( vec_t* )origin );
    S_AL_SanitiseVector( ( vec_t* )velocity );
    S_AL_SrcLoop( SRCPRI_ENTITY, sfx, origin, velocity, entityNum );
}

/*
=================
S_AL_AddRealLoopingSound
=================
*/
static void S_AL_AddRealLoopingSound( sint entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx )
{
    if( S_AL_CheckInput( entityNum, sfx ) )
    {
        return;
    }
    
    S_AL_SanitiseVector( ( vec_t* )origin );
    S_AL_SanitiseVector( ( vec_t* )velocity );
    
    // There are certain maps (*cough* Q3:TA mpterra*) that have large quantities
    // of ET_SPEAKERS in the PVS at any given time. OpenAL can't cope with mixing
    // large numbers of sounds, so this culls them by distance
    if( DistanceSquared( origin, lastListenerOrigin ) > ( s_alMaxDistance->value + s_alGraceDistance->value ) *
            ( s_alMaxDistance->value + s_alGraceDistance->value ) )
    {
        return;
    }
    
    S_AL_SrcLoop( SRCPRI_AMBIENT, sfx, origin, velocity, entityNum );
}

/*
=================
S_AL_StopLoopingSound
=================
*/
static void S_AL_StopLoopingSound( sint entityNum )
{
    if( entityList[entityNum].srcAllocated )
    {
        S_AL_SrcKill( entityList[entityNum].srcIndex );
    }
}

/*
=================
S_AL_SrcUpdate

Update state (move things around, manage sources, and so on)
=================
*/
static void S_AL_SrcUpdate( void )
{
    sint i;
    sint entityNum;
    sint state;
    src_t* curSource;
    
    for( i = 0; i < srcCount; i++ )
    {
        entityNum = srcList[i].entity;
        curSource = &srcList[i];
        
        if( curSource->isLocked )
        {
            continue;
        }
        
        if( !curSource->isActive )
        {
            continue;
        }
        
        // Update source parameters
        if( ( s_alGain->modified ) || ( s_volume->modified ) )
        {
            curSource->curGain = s_alGain->value * s_volume->value;
        }
        
        if( ( s_alRolloff->modified ) && ( !curSource->local ) )
        {
            alSourcef( curSource->alSource, AL_ROLLOFF_FACTOR, s_alRolloff->value );
        }
        
        if( s_alMinDistance->modified )
        {
            alSourcef( curSource->alSource, AL_REFERENCE_DISTANCE, s_alMinDistance->value );
        }
        
        if( curSource->isLooping )
        {
            sentity_t* sent = &entityList[ entityNum ];
            
            // If a looping effect hasn't been touched this frame, kill it
            if( sent->loopAddedThisFrame )
            {
                // The sound has changed without an intervening removal
                if( curSource->isActive && !sent->startLoopingSound &&
                        curSource->sfx != sent->loopSfx )
                {
                    alSourceStop( curSource->alSource );
                    alSourcei( curSource->alSource, AL_BUFFER, 0 );
                    sent->startLoopingSound = true;
                }
                
                // The sound hasn't been started yet
                if( sent->startLoopingSound )
                {
                    S_AL_SrcSetup( i, sent->loopSfx, sent->loopPriority,
                                   entityNum, -1, curSource->local );
                    curSource->isLooping = true;
                    alSourcei( curSource->alSource, AL_LOOPING, AL_TRUE );
                    alSourcePlay( curSource->alSource );
                    
                    sent->startLoopingSound = false;
                }
                
                // Update locality
                if( curSource->local )
                {
                    alSourcei( curSource->alSource, AL_SOURCE_RELATIVE, AL_TRUE );
                    alSourcef( curSource->alSource, AL_ROLLOFF_FACTOR, 0.0f );
                }
                else
                {
                    alSourcei( curSource->alSource, AL_SOURCE_RELATIVE, AL_FALSE );
                    alSourcef( curSource->alSource, AL_ROLLOFF_FACTOR, s_alRolloff->value );
                }
            }
            else
            {
                S_AL_SrcKill( i );
            }
            
            continue;
        }
        
        // Check if it's done, and flag it
        alGetSourcei( curSource->alSource, AL_SOURCE_STATE, &state );
        if( state == AL_STOPPED )
        {
            S_AL_SrcKill( i );
            continue;
        }
        
        // Query relativity of source, don't move if it's true
        alGetSourcei( curSource->alSource, AL_SOURCE_RELATIVE, &state );
        
        // See if it needs to be moved
        if( curSource->isTracking && !state )
        {
            alSourcefv( curSource->alSource, AL_POSITION, entityList[entityNum].origin );
            S_AL_ScaleGain( curSource, entityList[entityNum].origin );
        }
    }
}

/*
=================
S_AL_SrcShutup
=================
*/
static void S_AL_SrcShutup( void )
{
    sint i;
    
    for( i = 0; i < srcCount; i++ )
    {
        S_AL_SrcKill( i );
    }
}

/*
=================
S_AL_SrcGet
=================
*/
static uint S_AL_SrcGet( srcHandle_t src )
{
    return srcList[src].alSource;
}


static srcHandle_t streamSourceHandle = -1;
static bool streamPlaying = false;
static uint streamSource;

/*
=================
S_AL_AllocateStreamChannel
=================
*/
static void S_AL_AllocateStreamChannel( void )
{
    // Allocate a streamSource at high priority
    streamSourceHandle = S_AL_SrcAlloc( SRCPRI_STREAM, -2, 0 );
    if( streamSourceHandle == -1 )
    {
        return;
    }
    
    // Lock the streamSource so nobody else can use it, and get the raw streamSource
    S_AL_SrcLock( streamSourceHandle );
    streamSource = S_AL_SrcGet( streamSourceHandle );
    
    // Set some streamSource parameters
    alSourcei( streamSource, AL_BUFFER,          0 );
    alSourcei( streamSource, AL_LOOPING,         AL_FALSE );
    alSource3f( streamSource, AL_POSITION,        0.0, 0.0, 0.0 );
    alSource3f( streamSource, AL_VELOCITY,        0.0, 0.0, 0.0 );
    alSource3f( streamSource, AL_DIRECTION,       0.0, 0.0, 0.0 );
    alSourcef( streamSource, AL_ROLLOFF_FACTOR,  0.0 );
    alSourcei( streamSource, AL_SOURCE_RELATIVE, AL_TRUE );
}

/*
=================
S_AL_FreeStreamChannel
=================
*/
static void S_AL_FreeStreamChannel( void )
{
    // Release the output streamSource
    S_AL_SrcUnlock( streamSourceHandle );
    
    streamSource = 0;
    streamSourceHandle = -1;
}

/*
=================
S_AL_RawSamples
=================
*/
static void S_AL_RawSamples( sint samples, sint rate, sint width, sint channels, const uchar8* data, float32 volume )
{
    uint buffer;
    uint format;
    
    format = S_AL_Format( width, channels );
    
    // Create the streamSource if necessary
    if( streamSourceHandle == -1 )
    {
        S_AL_AllocateStreamChannel();
        
        // Failed?
        if( streamSourceHandle == -1 )
        {
            Com_Printf( S_COLOR_RED "ERROR: Can't allocate streaming streamSource\n" );
            return;
        }
    }
    
    // Create a buffer, and stuff the data into it
    alGenBuffers( 1, &buffer );
    alBufferData( buffer, format, ( void* )data, ( samples * width * channels ), rate );
    
    // Shove the data onto the streamSource
    alSourceQueueBuffers( streamSource, 1, &buffer );
    
    // Volume
    alSourcef( streamSource, AL_GAIN, volume * s_volume->value * s_alGain->value );
}

/*
=================
S_AL_StreamUpdate
=================
*/
static void S_AL_StreamUpdate( void )
{
    sint numBuffers;
    sint state;
    
    if( streamSourceHandle == -1 )
    {
        return;
    }
    
    // Un-queue any buffers, and delete them
    alGetSourcei( streamSource, AL_BUFFERS_PROCESSED, &numBuffers );
    while( numBuffers-- )
    {
        uint buffer;
        alSourceUnqueueBuffers( streamSource, 1, &buffer );
        alDeleteBuffers( 1, &buffer );
    }
    
    // Start the streamSource playing if necessary
    alGetSourcei( streamSource, AL_BUFFERS_QUEUED, &numBuffers );
    
    alGetSourcei( streamSource, AL_SOURCE_STATE, &state );
    if( state == AL_STOPPED )
    {
        streamPlaying = false;
        
        // If there are no buffers queued up, release the streamSource
        if( !numBuffers )
        {
            S_AL_FreeStreamChannel();
        }
    }
    
    if( !streamPlaying && numBuffers )
    {
        alSourcePlay( streamSource );
        streamPlaying = true;
    }
}

/*
=================
S_AL_StreamDie
=================
*/
static void S_AL_StreamDie( void )
{
    if( streamSourceHandle == -1 )
    {
        return;
    }
    
    streamPlaying = false;
    alSourceStop( streamSource );
    S_AL_FreeStreamChannel();
}

#define NUM_MUSIC_BUFFERS	4
#define	MUSIC_BUFFER_SIZE 4096

static bool musicPlaying = false;
static srcHandle_t musicSourceHandle = -1;
static uint musicSource;
static uint musicBuffers[NUM_MUSIC_BUFFERS];

static snd_stream_t* mus_stream;
static snd_stream_t* intro_stream;
static valueType s_backgroundLoop[MAX_QPATH];

static uchar8 decode_buffer[MUSIC_BUFFER_SIZE];

/*
=================
S_AL_MusicSourceGet
=================
*/
static void S_AL_MusicSourceGet( void )
{
    // Allocate a musicSource at high priority
    musicSourceHandle = S_AL_SrcAlloc( SRCPRI_STREAM, -2, 0 );
    if( musicSourceHandle == -1 )
    {
        return;
    }
    
    // Lock the musicSource so nobody else can use it, and get the raw musicSource
    S_AL_SrcLock( musicSourceHandle );
    musicSource = S_AL_SrcGet( musicSourceHandle );
    
    // Set some musicSource parameters
    alSource3f( musicSource, AL_POSITION,        0.0, 0.0, 0.0 );
    alSource3f( musicSource, AL_VELOCITY,        0.0, 0.0, 0.0 );
    alSource3f( musicSource, AL_DIRECTION,       0.0, 0.0, 0.0 );
    alSourcef( musicSource, AL_ROLLOFF_FACTOR,  0.0 );
    alSourcei( musicSource, AL_SOURCE_RELATIVE, AL_TRUE );
}

/*
=================
S_AL_MusicSourceFree
=================
*/
static void S_AL_MusicSourceFree( void )
{
    // Release the output musicSource
    S_AL_SrcUnlock( musicSourceHandle );
    
    musicSource = 0;
    musicSourceHandle = -1;
}

/*
=================
S_AL_CloseMusicFiles
=================
*/
static void S_AL_CloseMusicFiles( void )
{
    if( intro_stream )
    {
        S_CodecCloseStream( intro_stream );
        intro_stream = nullptr;
    }
    
    if( mus_stream )
    {
        S_CodecCloseStream( mus_stream );
        mus_stream = nullptr;
    }
}

/*
=================
S_AL_StopBackgroundTrack
=================
*/
static void S_AL_StopBackgroundTrack( void )
{
    if( !musicPlaying )
    {
        return;
    }
    
    // Stop playing
    alSourceStop( musicSource );
    
    // De-queue the musicBuffers
    alSourceUnqueueBuffers( musicSource, NUM_MUSIC_BUFFERS, musicBuffers );
    
    // Destroy the musicBuffers
    alDeleteBuffers( NUM_MUSIC_BUFFERS, musicBuffers );
    
    // Free the musicSource
    S_AL_MusicSourceFree();
    
    // Unload the stream
    S_AL_CloseMusicFiles();
    
    musicPlaying = false;
}

/*
=================
S_AL_MusicProcess
=================
*/
static void S_AL_MusicProcess( uint b )
{
    sint error;
    sint l;
    uint format;
    snd_stream_t* curstream;
    
    if( intro_stream )
    {
        curstream = intro_stream;
    }
    else
    {
        curstream = mus_stream;
    }
    
    if( !curstream )
    {
        return;
    }
    
    l = S_CodecReadStream( curstream, MUSIC_BUFFER_SIZE, decode_buffer );
    
    // Run out data to read, start at the beginning again
    if( l == 0 )
    {
        S_CodecCloseStream( curstream );
        
        // the intro stream just finished playing so we don't need to reopen
        // the music stream.
        if( intro_stream )
        {
            intro_stream = nullptr;
        }
        else
        {
            mus_stream = S_CodecOpenStream( s_backgroundLoop );
        }
        
        curstream = mus_stream;
        
        if( !curstream )
        {
            S_AL_StopBackgroundTrack();
            return;
        }
        
        l = S_CodecReadStream( curstream, MUSIC_BUFFER_SIZE, decode_buffer );
    }
    
    format = S_AL_Format( curstream->info.width, curstream->info.channels );
    
    if( l == 0 )
    {
        // We have no data to buffer, so buffer silence
        uchar8 dummyData[ 2 ] = { 0 };
        
        alBufferData( b, AL_FORMAT_MONO16, ( void* )dummyData, 2, 22050 );
    }
    else
    {
        alBufferData( b, format, decode_buffer, l, curstream->info.rate );
    }
    
    if( ( error = alGetError( ) ) != AL_NO_ERROR )
    {
        S_AL_StopBackgroundTrack( );
        Com_Printf( S_COLOR_RED "ERROR: while buffering data for music stream - %s\n", S_AL_ErrorMsg( error ) );
        return;
    }
}

/*
=================
S_AL_StartBackgroundTrack
=================
*/
static void S_AL_StartBackgroundTrack( pointer intro, pointer loop )
{
    sint i;
    bool issame;
    
    // Stop any existing music that might be playing
    S_AL_StopBackgroundTrack();
    
    if( ( !intro || !*intro ) && ( !loop || !*loop ) )
    {
        return;
    }
    
    // Allocate a musicSource
    S_AL_MusicSourceGet();
    
    if( musicSourceHandle == -1 )
    {
        return;
    }
    
    if( !loop || !*loop )
    {
        loop = intro;
        issame = true;
    }
    else if( intro && *intro && !strcmp( intro, loop ) )
    {
        issame = true;
    }
    else
    {
        issame = false;
    }
    
    // Copy the loop over
    ::strncpy( s_backgroundLoop, loop, sizeof( s_backgroundLoop ) );
    
    if( !issame )
    {
        // Open the intro and don't mind whether it succeeds.
        // The important part is the loop.
        intro_stream = S_CodecOpenStream( intro );
    }
    else
    {
        intro_stream = nullptr;
    }
    
    mus_stream = S_CodecOpenStream( s_backgroundLoop );
    if( !mus_stream )
    {
        S_AL_CloseMusicFiles();
        S_AL_MusicSourceFree();
        return;
    }
    
    // Generate the musicBuffers
    alGenBuffers( NUM_MUSIC_BUFFERS, musicBuffers );
    
    // Queue the musicBuffers up
    for( i = 0; i < NUM_MUSIC_BUFFERS; i++ )
    {
        S_AL_MusicProcess( musicBuffers[i] );
    }
    
    alSourceQueueBuffers( musicSource, NUM_MUSIC_BUFFERS, musicBuffers );
    
    // Set the initial gain property
    alSourcef( musicSource, AL_GAIN, s_alGain->value * s_musicVolume->value );
    
    // Start playing
    alSourcePlay( musicSource );
    
    musicPlaying = true;
}

/*
=================
S_AL_MusicUpdate
=================
*/
static void S_AL_MusicUpdate( void )
{
    sint numBuffers;
    sint state;
    
    if( !musicPlaying )
    {
        return;
    }
    
    alGetSourcei( musicSource, AL_BUFFERS_PROCESSED, &numBuffers );
    while( numBuffers-- )
    {
        ALuint b;
        alSourceUnqueueBuffers( musicSource, 1, &b );
        S_AL_MusicProcess( b );
        alSourceQueueBuffers( musicSource, 1, &b );
    }
    
    // Hitches can cause OpenAL to be starved of buffers when streaming.
    // If this happens, it will stop playback. This restarts the source if
    // it is no longer playing, and if there are buffers available
    alGetSourcei( musicSource, AL_SOURCE_STATE, &state );
    alGetSourcei( musicSource, AL_BUFFERS_QUEUED, &numBuffers );
    
    if( state == AL_STOPPED && numBuffers )
    {
        Com_DPrintf( S_COLOR_YELLOW "Restarted OpenAL music\n" );
        alSourcePlay( musicSource );
    }
    
    // Set the gain property
    alSourcef( musicSource, AL_GAIN, s_alGain->value * s_musicVolume->value );
}


// Local state variables
static ALCdevice* alDevice;
static ALCcontext* alContext;

#ifdef _WIN32
#define ALDRIVER_DEFAULT "OpenAL32.dll"
#define ALDEVICE_DEFAULT "Generic Software"
#elif defined(MACOS_X)
#define ALDRIVER_DEFAULT "/System/Library/Frameworks/OpenAL.framework/OpenAL"
#else
#define ALDRIVER_DEFAULT "libopenal.so"
#endif

/*
=================
S_AL_StopAllSounds
=================
*/
static void S_AL_StopAllSounds( void )
{
    S_AL_SrcShutup();
    S_AL_StopBackgroundTrack();
    S_AL_StreamDie();
}

/*
=================
S_AL_Respatialize
=================
*/
static void S_AL_Respatialize( sint entityNum, const vec3_t origin, vec3_t axis[3], sint inwater )
{
    float32 velocity[3] = {0.0f, 0.0f, 0.0f};
    float32 orientation[6];
    vec3_t sorigin;
    
    VectorCopy( origin, sorigin );
    S_AL_SanitiseVector( sorigin );
    
    S_AL_SanitiseVector( axis[ 0 ] );
    S_AL_SanitiseVector( axis[ 1 ] );
    S_AL_SanitiseVector( axis[ 2 ] );
    
    orientation[0] = axis[0][0];
    orientation[1] = axis[0][1];
    orientation[2] = axis[0][2];
    orientation[3] = axis[2][0];
    orientation[4] = axis[2][1];
    orientation[5] = axis[2][2];
    
    VectorCopy( sorigin, lastListenerOrigin );
    
    // Set OpenAL listener paramaters
    alListenerfv( AL_POSITION, ( float32* )sorigin );
    alListenerfv( AL_VELOCITY, velocity );
    alListenerfv( AL_ORIENTATION, orientation );
}

/*
=================
S_AL_Update
=================
*/
static void S_AL_Update( void )
{
    // Update SFX channels
    S_AL_SrcUpdate();
    
    // Update streams
    S_AL_StreamUpdate();
    S_AL_MusicUpdate();
    
    // Doppler
    if( s_doppler->modified )
    {
        s_alDopplerFactor->modified = true;
        s_doppler->modified = false;
    }
    
    // Doppler parameters
    if( s_alDopplerFactor->modified )
    {
        if( s_doppler->integer )
        {
            alDopplerFactor( s_alDopplerFactor->value );
        }
        else
        {
            alDopplerFactor( 0.0f );
        }
        
        s_alDopplerFactor->modified = false;
    }
    
    if( s_alDopplerSpeed->modified )
    {
        alDopplerVelocity( s_alDopplerSpeed->value );
        s_alDopplerSpeed->modified = false;
    }
    
    // Clear the modified flags on the other cvars
    s_alGain->modified = false;
    s_volume->modified = false;
    s_musicVolume->modified = false;
    s_alMinDistance->modified = false;
    s_alRolloff->modified = false;
}

/*
=================
S_AL_DisableSounds
=================
*/
static void S_AL_DisableSounds( void )
{
    S_AL_StopAllSounds();
}

/*
=================
S_AL_BeginRegistration
=================
*/
static void S_AL_BeginRegistration( void )
{
}

/*
=================
S_AL_ClearSoundBuffer
=================
*/
static void S_AL_ClearSoundBuffer( void )
{
}

/*
=================
S_AL_SoundList
=================
*/
static void S_AL_SoundList( void )
{
}

/*
=================
S_AL_SoundInfo
=================
*/
static
void S_AL_SoundInfo( void )
{
    Com_Printf( "OpenAL info:\n" );
    Com_Printf( "  Vendor:     %s\n", alGetString( AL_VENDOR ) );
    Com_Printf( "  Version:    %s\n", alGetString( AL_VERSION ) );
    Com_Printf( "  Renderer:   %s\n", alGetString( AL_RENDERER ) );
    Com_Printf( "  Extensions: %s\n", alGetString( AL_EXTENSIONS ) );
    if( alcIsExtensionPresent( nullptr, "ALC_ENUMERATION_EXT" ) )
    {
        Com_Printf( "  Device:     %s\n", alcGetString( alDevice, ALC_DEVICE_SPECIFIER ) );
        Com_Printf( "Available Devices:\n%s", s_alAvailableDevices->string );
    }
}

/*
=================
S_AL_Shutdown
=================
*/
static void S_AL_Shutdown( void )
{
    // Shut down everything
    S_AL_StreamDie( );
    S_AL_StopBackgroundTrack( );
    S_AL_SrcShutdown( );
    S_AL_BufferShutdown( );
    
    alcDestroyContext( alContext );
    alcCloseDevice( alDevice );
}

/*
=================
S_AL_Init
=================
*/
bool S_AL_Init( soundInterface_t* si )
{
    bool enumsupport, founddev = false;
    
    if( !si )
    {
        return false;
    }
    
    // New console variables
    s_alPrecache = cvarSystem->Get( "s_alPrecache", "1", CVAR_ARCHIVE, "Cache OpenAL sounds before use" );
    s_alGain = cvarSystem->Get( "s_alGain", "0.4", CVAR_ARCHIVE, "The value of AL_GAIN for each source" );
    s_alSources = cvarSystem->Get( "s_alSources", "96", CVAR_ARCHIVE, "the total number of sources (memory) to allocate" );
    s_alDopplerFactor = cvarSystem->Get( "s_alDopplerFactor", "1.0", CVAR_ARCHIVE, "the value passed to alDopplerFactor" );
    s_alDopplerSpeed = cvarSystem->Get( "s_alDopplerSpeed", "2200", CVAR_ARCHIVE, "the value passed to alDopplerVelocity" );
    s_alMinDistance = cvarSystem->Get( "s_alMinDistance", "120", CVAR_CHEAT, "the value of AL_REFERENCE_DISTANCE for each source" );
    s_alMaxDistance = cvarSystem->Get( "s_alMaxDistance", "1024", CVAR_CHEAT, "the maximum distance before sounds start to become inaudible." );
    s_alRolloff = cvarSystem->Get( "s_alRolloff", "2", CVAR_CHEAT, "the value of AL_ROLLOFF_FACTOR for each source" );
    s_alGraceDistance = cvarSystem->Get( "s_alGraceDistance", "512", CVAR_CHEAT, "after having passed MaxDistance, length until sounds are completely inaudible" );
    
    s_alDriver = cvarSystem->Get( "s_alDriver", ALDRIVER_DEFAULT, CVAR_ARCHIVE, "which OpenAL library to use" );
    s_alDevice = cvarSystem->Get( "s_alDevice", "", CVAR_ARCHIVE | CVAR_LATCH, "which OpenAL device to use" );
    
    // Load AL
    if( !QAL_Init( s_alDriver->string ) )
    {
        Com_Printf( "Failed to load library: \"%s\".\n", s_alDriver->string );
        return false;
    }
    
    // Device enumeration support (extension is implemented reasonably only on Windows right now).
    if( ( enumsupport = alcIsExtensionPresent( nullptr, "ALC_ENUMERATION_EXT" ) ) )
    {
        valueType devicenames[1024] = "";
        pointer devicelist;
        pointer defaultdevice;
        sint curlen;
        
        // get all available devices + the default device name.
        devicelist = alcGetString( nullptr, ALC_DEVICE_SPECIFIER );
        defaultdevice = alcGetString( nullptr, ALC_DEFAULT_DEVICE_SPECIFIER );
        
#ifdef _WIN32
        // check whether the default device is generic hardware. If it is, change to
        // Generic Software as that one works more reliably with various sound systems.
        // If it's not, use OpenAL's default selection as we don't want to ignore
        // native hardware acceleration.
        if( !strcmp( defaultdevice, "Generic Hardware" ) )
        {
            s_alDevice = cvarSystem->Get( "s_alDevice", ALDEVICE_DEFAULT, CVAR_ARCHIVE | CVAR_LATCH, "which OpenAL device to use" );
        }
        else
#endif
        {
            s_alDevice = cvarSystem->Get( "s_alDevice", defaultdevice, CVAR_ARCHIVE | CVAR_LATCH, "which OpenAL device to use" );
        }
        
        // dump a list of available devices to a cvar for the user to see.
        while( ( curlen = strlen( devicelist ) ) )
        {
            Q_strcat( devicenames, sizeof( devicenames ), devicelist );
            Q_strcat( devicenames, sizeof( devicenames ), "\n" );
            
            // check whether the device we want to load is available at all.
            if( !strcmp( s_alDevice->string, devicelist ) )
            {
                founddev = true;
            }
            
            devicelist += curlen + 1;
        }
        
        s_alAvailableDevices = cvarSystem->Get( "s_alAvailableDevices", devicenames, CVAR_ROM | CVAR_NORESTART, "list of available OpenAL devices" );
        
        if( !founddev )
        {
            //Cvar_ForceReset( "s_alDevice" );
            founddev = 1;
        }
    }
    
    if( founddev )
    {
        alDevice = alcOpenDevice( s_alDevice->string );
    }
    else
    {
        alDevice = alcOpenDevice( nullptr );
    }
    
    if( !alDevice )
    {
        Com_Printf( "Failed to open OpenAL device.\n" );
        return false;
    }
    
    if( enumsupport )
    {
        cvarSystem->Set( "s_alDevice", alcGetString( alDevice, ALC_DEVICE_SPECIFIER ) );
    }
    
    // Create OpenAL context
    alContext = alcCreateContext( alDevice, nullptr );
    if( !alContext )
    {
        alcCloseDevice( alDevice );
        Com_Printf( "Failed to create OpenAL context.\n" );
        return false;
    }
    
    alcMakeContextCurrent( alContext );
    
    // Initialize sources, buffers, music
    S_AL_BufferInit( );
    S_AL_SrcInit( );
    
    // Set up OpenAL parameters (doppler, etc)
    alDistanceModel( AL_INVERSE_DISTANCE_CLAMPED );
    alDopplerFactor( s_alDopplerFactor->value );
    alDopplerVelocity( s_alDopplerSpeed->value );
    
    si->Shutdown = S_AL_Shutdown;
    si->StartSound = S_AL_StartSound;
    si->StartLocalSound = S_AL_StartLocalSound;
    si->StartBackgroundTrack = S_AL_StartBackgroundTrack;
    si->StopBackgroundTrack = S_AL_StopBackgroundTrack;
    si->RawSamples = S_AL_RawSamples;
    si->StopAllSounds = S_AL_StopAllSounds;
    si->ClearLoopingSounds = S_AL_ClearLoopingSounds;
    si->AddLoopingSound = S_AL_AddLoopingSound;
    si->AddRealLoopingSound = S_AL_AddRealLoopingSound;
    si->StopLoopingSound = S_AL_StopLoopingSound;
    si->Respatialize = S_AL_Respatialize;
    si->UpdateEntityPosition = S_AL_UpdateEntityPosition;
    si->Update = S_AL_Update;
    si->DisableSounds = S_AL_DisableSounds;
    si->BeginRegistration = S_AL_BeginRegistration;
    si->RegisterSound = S_AL_RegisterSound;
    si->ClearSoundBuffer = S_AL_ClearSoundBuffer;
    si->SoundInfo = S_AL_SoundInfo;
    si->SoundList = S_AL_SoundList;
    
    return true;
}

