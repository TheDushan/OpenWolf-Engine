////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2005 - Stuart Dalton (badcdev@gmail.com)
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
// File name:   s_main.cpp
// Version:     v1.02
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.h>

convar_t* s_volume;
convar_t* s_musicVolume;
convar_t* s_doppler;
convar_t* s_backend;

static struct qmutex_t* s_mutex;

static soundInterface_t si;

/*
=================
S_ValidateInterface
=================
*/
static bool S_ValidSoundInterface( soundInterface_t* si )
{
    if( !si->Shutdown )
    {
        return false;
    }
    
    if( !si->StartSound )
    {
        return false;
    }
    
    if( !si->StartLocalSound )
    {
        return false;
    }
    
    if( !si->StartBackgroundTrack )
    {
        return false;
    }
    
    if( !si->StopBackgroundTrack )
    {
        return false;
    }
    
    if( !si->RawSamples )
    {
        return false;
    }
    
    if( !si->StopAllSounds )
    {
        return false;
    }
    
    if( !si->ClearLoopingSounds )
    {
        return false;
    }
    
    if( !si->AddLoopingSound )
    {
        return false;
    }
    
    if( !si->AddRealLoopingSound )
    {
        return false;
    }
    
    if( !si->StopLoopingSound )
    {
        return false;
    }
    
    if( !si->Respatialize )
    {
        return false;
    }
    
    if( !si->UpdateEntityPosition )
    {
        return false;
    }
    
    if( !si->Update )
    {
        return false;
    }
    
    if( !si->DisableSounds )
    {
        return false;
    }
    
    if( !si->BeginRegistration )
    {
        return false;
    }
    
    if( !si->RegisterSound )
    {
        return false;
    }
    
    if( !si->ClearSoundBuffer )
    {
        return false;
    }
    
    if( !si->SoundInfo )
    {
        return false;
    }
    
    if( !si->SoundList )
    {
        return false;
    }
    
    return true;
}

/*
=================
idSoundSystemLocal::StartSound
=================
*/
void idSoundSystemLocal::StartSound( vec3_t origin, sint entnum, sint entchannel, sfxHandle_t sfx )
{
    if( si.StartSound )
    {
        si.StartSound( origin, entnum, entchannel, sfx );
    }
}

/*
=================
idSoundSystemLocal::StartLocalSound
=================
*/
void idSoundSystemLocal::StartLocalSound( sfxHandle_t sfx, sint channelNum )
{
    if( si.StartLocalSound )
    {
        si.StartLocalSound( sfx, channelNum );
    }
}

/*
=================
idSoundSystemLocal::StartBackgroundTrack
=================
*/
void idSoundSystemLocal::StartBackgroundTrack( pointer intro, pointer loop )
{
    if( si.StartBackgroundTrack )
    {
        si.StartBackgroundTrack( intro, loop );
    }
}

/*
=================
idSoundSystemLocal::StopBackgroundTrack
=================
*/
void idSoundSystemLocal::StopBackgroundTrack( void )
{
    if( si.StopBackgroundTrack )
    {
        si.StopBackgroundTrack( );
    }
}

/*
=================
idSoundSystemLocal::RawSamples
=================
*/
void idSoundSystemLocal::RawSamples( sint samples, sint rate, sint width, sint channels, const uchar8* data, float32 volume )
{
    if( si.RawSamples )
    {
        si.RawSamples( samples, rate, width, channels, data, volume );
    }
}

/*
=================
idSoundSystemLocal::StopAllSounds
=================
*/
void idSoundSystemLocal::StopAllSounds( void )
{
    if( si.StopAllSounds )
    {
        si.StopAllSounds( );
    }
}

/*
=================
idSoundSystemLocal::ClearLoopingSounds
=================
*/
void idSoundSystemLocal::ClearLoopingSounds( bool killall )
{
    if( si.ClearLoopingSounds )
    {
        si.ClearLoopingSounds( killall );
    }
}

/*
=================
idSoundSystemLocal::AddLoopingSound
=================
*/
void idSoundSystemLocal::AddLoopingSound( sint entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx )
{
    if( si.AddLoopingSound )
    {
        si.AddLoopingSound( entityNum, origin, velocity, sfx );
    }
}

/*
=================
idSoundSystemLocal::AddRealLoopingSound
=================
*/
void idSoundSystemLocal::AddRealLoopingSound( sint entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx )
{
    if( si.AddRealLoopingSound )
    {
        si.AddRealLoopingSound( entityNum, origin, velocity, sfx );
    }
}

/*
=================
idSoundSystemLocal::StopLoopingSound
=================
*/
void idSoundSystemLocal::StopLoopingSound( sint entityNum )
{
    if( si.StopLoopingSound )
    {
        si.StopLoopingSound( entityNum );
    }
}

/*
=================
idSoundSystemLocal::Respatialize
=================
*/
void idSoundSystemLocal::Respatialize( sint entityNum, const vec3_t origin, vec3_t axis[3], sint inwater )
{
    if( si.Respatialize )
    {
        si.Respatialize( entityNum, origin, axis, inwater );
    }
}

/*
=================
idSoundSystemLocal::UpdateEntityPosition
=================
*/
void idSoundSystemLocal::UpdateEntityPosition( sint entityNum, const vec3_t origin )
{
    if( si.UpdateEntityPosition )
    {
        si.UpdateEntityPosition( entityNum, origin );
    }
}

/*
=================
idSoundSystemLocal::Update
=================
*/
void idSoundSystemLocal::Update( void )
{
    if( si.Update )
    {
        si.Update( );
    }
}

/*
=================
idSoundSystemLocal::DisableSounds
=================
*/
void idSoundSystemLocal::DisableSounds( void )
{
    if( si.DisableSounds )
    {
        si.DisableSounds( );
    }
}

/*
=================
idSoundSystemLocal::BeginRegistration
=================
*/
void idSoundSystemLocal::BeginRegistration( void )
{
    if( si.BeginRegistration )
    {
        si.BeginRegistration( );
    }
}

/*
=================
idSoundSystemLocal::RegisterSound
=================
*/
sfxHandle_t	idSoundSystemLocal::RegisterSound( pointer sample, bool compressed )
{
    if( si.RegisterSound )
    {
        return si.RegisterSound( sample, compressed );
    }
    else
    {
        return 0;
    }
}

/*
=================
idSoundSystemLocal::ClearSoundBuffer
=================
*/
void idSoundSystemLocal::ClearSoundBuffer( void )
{
    if( si.ClearSoundBuffer )
    {
        si.ClearSoundBuffer( );
    }
}

/*
=================
S_SoundInfo
=================
*/
void S_SoundInfo( void )
{
    if( si.SoundInfo )
    {
        si.SoundInfo( );
    }
}

/*
=================
S_SoundList
=================
*/
void S_SoundList( void )
{
    if( si.SoundList )
    {
        si.SoundList( );
    }
}

/*
=================
S_Play_f
=================
*/
void S_Play_f( void )
{
    sint i, c;
    sfxHandle_t	h;
    
    if( !si.RegisterSound || !si.StartLocalSound )
    {
        return;
    }
    
    c = cmdSystem->Argc();
    
    if( c < 2 )
    {
        Com_Printf( "Usage: play <sound filename> [sound filename] [sound filename] ...\n" );
        return;
    }
    
    for( i = 1; i < c; i++ )
    {
        h = si.RegisterSound( cmdSystem->Argv( i ), false );
        
        if( h )
        {
            si.StartLocalSound( h, CHAN_LOCAL_SOUND );
        }
    }
}

/*
=================
S_Music_f
=================
*/
void S_Music_f( void )
{
    sint c;
    
    if( !si.StartBackgroundTrack )
    {
        return;
    }
    
    c = cmdSystem->Argc();
    
    if( c == 2 )
    {
        si.StartBackgroundTrack( cmdSystem->Argv( 1 ), nullptr );
    }
    else if( c == 3 )
    {
        si.StartBackgroundTrack( cmdSystem->Argv( 1 ), cmdSystem->Argv( 2 ) );
    }
    else
    {
        Com_Printf( "music <musicfile> [loopfile]\n" );
        return;
    }
    
}

/*
===============
idSoundSystemLocal::S_StopAllSounds

I had a problem with AddCommand and pointing to the class member so this is one way of doing
pointing to a function that is a class member
===============
*/
void idSoundSystemLocal::S_StopAllSounds( void )
{
    static_cast<idSoundSystemLocal*>( soundSystem )->StopAllSounds();
}

/*
=================
idSoundSystemLocal::Init
=================
*/
void idSoundSystemLocal::Init( void )
{
    convar_t* cv;
    bool started = false;
    
    Com_Printf( "------ Initializing Sound ------\n" );
    
    s_volume = cvarSystem->Get( "s_volume", "0.8", CVAR_ARCHIVE, "Sets volume of the game sounds, multiplier value (0.0 to 1.0)" );
    s_musicVolume = cvarSystem->Get( "s_musicvolume", "0.25", CVAR_ARCHIVE, "Sets volume of the music, multiplier value (0.0 to 1.0)" );
    s_doppler = cvarSystem->Get( "s_doppler", "1", CVAR_ARCHIVE, "Toggle doppler effect" );
    s_backend = cvarSystem->Get( "s_backend", "", CVAR_ROM, "Sound backend" );
    
    cv = cvarSystem->Get( "s_initsound", "1", 0, "Toggle whether sound is initialized or not (on next game)" );
    if( !cv->integer )
    {
        Com_Printf( "Sound disabled.\n" );
    }
    else
    {
        S_CodecInit( );
        
        cmdSystem->AddCommand( "play", S_Play_f, "Plays a sound fx file" );
        cmdSystem->AddCommand( "music", S_Music_f, "Plays a music file" );
        cmdSystem->AddCommand( "s_list", S_SoundList, "Lists all cached sound and music files" );
        cmdSystem->AddCommand( "s_stop", S_StopAllSounds, "Stops all sounds including music" );
        cmdSystem->AddCommand( "s_info", S_SoundInfo, "Display information about the sound backend" );
        
        cv = cvarSystem->Get( "s_useOpenAL", "1", CVAR_ARCHIVE, "Use the OpenAL sound backend if available" );
        if( cv->integer )
        {
            //OpenAL
            started = S_AL_Init( &si );
            cvarSystem->Set( "s_backend", "OpenAL" );
        }
        
        if( !started )
        {
            started = S_Base_Init( &si );
            cvarSystem->Set( "s_backend", "base" );
        }
        
        if( started )
        {
            if( !S_ValidSoundInterface( &si ) )
            {
                Com_Error( ERR_FATAL, "Sound interface invalid." );
            }
            
            S_SoundInfo( );
            Com_Printf( "Sound initialization successful.\n" );
        }
        else
        {
            Com_Printf( "Sound initialization failed.\n" );
        }
    }
    
    Com_Printf( "--------------------------------\n" );
}

/*
=================
idSoundSystemLocal::Shutdown
=================
*/
void idSoundSystemLocal::Shutdown( void )
{
    if( si.Shutdown )
    {
        si.Shutdown( );
    }
    
    ::memset( &si, 0, sizeof( soundInterface_t ) );
    
    cmdSystem->RemoveCommand( "play" );
    cmdSystem->RemoveCommand( "music" );
    cmdSystem->RemoveCommand( "s_list" );
    cmdSystem->RemoveCommand( "s_stop" );
    cmdSystem->RemoveCommand( "s_info" );
    
    S_CodecShutdown( );
}

