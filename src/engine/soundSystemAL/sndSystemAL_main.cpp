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
// File name:   snd_al_main.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////


#include <soundSystemAL/sndSystemAL_precompiled.hpp>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif


idAudioOpenALSystemLocal soundOpenALSystemLocal;
idAudioOpenALSystem* soundOpenALSystem = &soundOpenALSystemLocal;

static openALImports_t* imports;

/*
=================
idAudioOpenALSystemLocal::format
=================
*/
ALuint idAudioOpenALSystemLocal::format( sint width, sint channels )
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
idAudioOpenALSystemLocal::errormsg

OpenAL error messages
=================
*/
valueType* idAudioOpenALSystemLocal::errormsg( ALenum error )
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

/*
 * Local state variables
 */
static bool snd_shutdown_bug = false;
static ALCdevice* alDevice;
static ALCcontext* alContext;

/*
 * Console variables
 */
convar_t* s_volume;
convar_t* s_musicVolume;
convar_t* s_doppler;
convar_t* s_precache;
convar_t* s_gain;
convar_t* s_sources;
convar_t* s_dopplerFactor;
convar_t* s_dopplerSpeed;
convar_t* s_minDistance;
convar_t* s_rolloff;
convar_t* s_alDevice;
convar_t* s_alAvailableDevices;
convar_t* s_alAvailableInputDevices;


convar_t* s_alDriver;
#if defined( _WIN32 )
#define ALDRIVER_DEFAULT "OpenAL64"
#elif defined( MACOS_X )
#define ALDRIVER_DEFAULT "/System/Library/Frameworks/OpenAL.framework/OpenAL"
#else
#define ALDRIVER_DEFAULT "libopenal.so.1"
#endif

/*
=================
idAudioOpenALSystemLocal::Init
=================
*/
bool idAudioOpenALSystemLocal::Init( void )
{
    pointer device = nullptr;
    
    // Original console variables
    s_volume = cvarSystem->Get( "s_volume", "0.8", CVAR_ARCHIVE, "Sound FX Volume " );
    s_musicVolume = cvarSystem->Get( "s_musicvolume", "0.25", CVAR_ARCHIVE, "Music volume level 0=off" );
    s_doppler = cvarSystem->Get( "s_doppler", "1", CVAR_ARCHIVE, "Toggle doppler effect" );
    
    // New console variables
    s_precache = cvarSystem->Get( "al_precache", "1", CVAR_ARCHIVE, "Cache OpenAL sounds before use" );
    s_gain = cvarSystem->Get( "al_gain", "0.4", CVAR_ARCHIVE, "The value of AL_GAIN for each source" );
    s_sources = cvarSystem->Get( "al_sources", "64", CVAR_ARCHIVE, "The total number of sources (memory) to allocate" );
    s_dopplerFactor = cvarSystem->Get( "al_dopplerfactor", "1.0", CVAR_ARCHIVE, "The value passed to alDopplerFactor" );
    s_dopplerSpeed = cvarSystem->Get( "al_dopplerspeed", "2200", CVAR_ARCHIVE, "The value passed to alDopplerVelocity" );
    s_minDistance = cvarSystem->Get( "al_mindistance", "80", CVAR_ARCHIVE, "The value of AL_REFERENCE_DISTANCE for each source" );
    s_rolloff = cvarSystem->Get( "al_rolloff", "0.25", CVAR_ARCHIVE, "The value of AL_ROLLOFF_FACTOR for each source" );
    s_alDevice = cvarSystem->Get( "al_device", "", CVAR_ARCHIVE | CVAR_LATCH, "Which OpenAL device to use" );
    s_alDriver = cvarSystem->Get( "al_driver", ALDRIVER_DEFAULT, CVAR_ARCHIVE, "Which OpenAL library to use" );
    
    // Load QAL
    if( !QAL_Init( s_alDriver->string ) )
    {
        trap_Printf( PRINT_ALL, "not initializing.\n" );
        return false;
    }
    
    // Open default device
    device = s_alDevice->string;
    if( device && !*device )
    {
        device = nullptr;
    }
    
    if( qalcIsExtensionPresent( nullptr, "ALC_ENUMERATION_EXT" ) )
    {
        valueType devicenames[1024] = "";
        pointer devicelist;
        pointer defaultdevice;
        sint curlen;
        
        // get all available devices + the default device name.
        devicelist = qalcGetString( nullptr, ALC_DEVICE_SPECIFIER );
        defaultdevice = qalcGetString( nullptr, ALC_DEFAULT_DEVICE_SPECIFIER );
        
#ifdef _WIN32
        // check whether the default device is generic hardware. If it is, change to
        // Generic Software as that one works more reliably with various sound systems.
        // If it's not, use OpenAL's default selection as we don't want to ignore
        // native hardware acceleration.
        if( !device && !strcmp( defaultdevice, "Generic Hardware" ) )
        {
            device = "Generic Software";
        }
#endif
        
        // dump a list of available devices to a cvar for the user to see.
        while( ( curlen = strlen( devicelist ) ) )
        {
            ::strcat( devicenames, devicelist );
            ::strcat( devicenames, "\n" );
            
            devicelist += curlen + 1;
        }
        s_alAvailableDevices = cvarSystem->Get( "al_AvailableDevices", devicenames, CVAR_ROM | CVAR_NORESTART, "List of available OpenAL devices" );
    }
    
    alDevice = qalcOpenDevice( device );
    if( !alDevice && device )
    {
        trap_Printf( PRINT_ALL,  "Failed to open OpenAL device '%s', trying default.\n", device );
        alDevice = qalcOpenDevice( nullptr );
    }
    
    if( !alDevice )
    {
        QAL_Shutdown();
        trap_Printf( PRINT_ALL,  "Failed to open OpenAL device.\n" );
        return false;
    }
    
    
    // Create OpenAL context
    alContext = qalcCreateContext( alDevice, nullptr );
    if( !alContext )
    {
        QAL_Shutdown();
        
        qalcCloseDevice( alDevice );
        trap_Printf( PRINT_ALL, "Failed to create context\n" );
        return false;
    }
    
    qalcMakeContextCurrent( alContext );
    qalcProcessContext( alContext );
    
    // Print OpenAL information
    trap_Printf( PRINT_ALL, "OpenAL initialised\n" );
    trap_Printf( PRINT_ALL, "  Vendor:     %s\n", qalGetString( AL_VENDOR ) );
    trap_Printf( PRINT_ALL, "  Version:    %s\n", qalGetString( AL_VERSION ) );
    trap_Printf( PRINT_ALL, "  Renderer:   %s\n", qalGetString( AL_RENDERER ) );
    trap_Printf( PRINT_ALL, "  AL Extensions: %s\n", qalGetString( AL_EXTENSIONS ) );
    trap_Printf( PRINT_ALL,  "  ALC Extensions: %s\n", qalcGetString( alDevice, ALC_EXTENSIONS ) );
    
    if( qalcIsExtensionPresent( nullptr, "ALC_ENUMERATION_EXT" ) )
    {
        trap_Printf( PRINT_ALL, "  Device:     %s\n", qalcGetString( alDevice, ALC_DEVICE_SPECIFIER ) );
        trap_Printf( PRINT_ALL, "Available Devices:\n%s", s_alAvailableDevices->string );
    }
    
    // Check for Linux shutdown race condition
    if( !::strcmp( qalGetString( AL_VENDOR ), "J. Valenzuela" ) )
    {
        snd_shutdown_bug = true;
    }
    
    // Initialize sources, buffers, music
    buf_init();
    src_init();
    
    // Set up OpenAL parameters (doppler, etc)
    qalDistanceModel( AL_INVERSE_DISTANCE_CLAMPED );
    qalDopplerFactor( s_dopplerFactor->value );
    qalDopplerVelocity( s_dopplerSpeed->value );;
    
    // Add commands
    cmdSystem->AddCommand( "play", Play_f, "play a sound file (play sound)" );
    cmdSystem->AddCommand( "music", Music_f, "Plays specified music file (music music)" );
    
    // Init successful
    trap_Printf( PRINT_ALL, "initialization successful\n" );
    
    return true;
}

/*
=================
idAudioOpenALSystemLocal::Shutdown
=================
*/
void idAudioOpenALSystemLocal::Shutdown( void )
{
    // Remove commands
    cmdSystem->RemoveCommand( "music" );
    cmdSystem->RemoveCommand( "play" );
    
    // Shut down everything
    stream_die();
    
    StopBackgroundTrack();
    
    src_shutdown();
    buf_shutdown();
    
    if( !snd_shutdown_bug )
    {
        qalcMakeContextCurrent( nullptr );
    }
    
    qalcDestroyContext( alContext );
    qalcCloseDevice( alDevice );
    
    QAL_Shutdown();
}

/*
=================
idAudioOpenALSystemLocal::StopAllSounds
=================
*/
void idAudioOpenALSystemLocal::StopAllSounds( void )
{
    src_shutup();
    
    StopBackgroundTrack();
}

/*
=================
idAudioOpenALSystemLocal::Respatialize
=================
*/
void idAudioOpenALSystemLocal::Respatialize( sint entityNum, const vec3_t origin, vec3_t axis[3], sint inwater )
{
    // Axis[0] = Forward
    // Axis[2] = Up
    float32 velocity[] = {0.0f, 0.0f, 0.0f};
    float32 orientation[] = {axis[0][0], axis[0][1], axis[0][2], axis[2][0], axis[2][1], axis[2][2] };
    vec3_t sorigin;
    
    // Set OpenAL listener paramaters
    VectorScale( origin, POSITION_SCALE, sorigin );
    qalListenerfv( AL_POSITION, origin );
    qalListenerfv( AL_VELOCITY, velocity );
    qalListenerfv( AL_ORIENTATION, orientation );
}

/*
=================
idAudioOpenALSystemLocal::Update
=================
*/
void idAudioOpenALSystemLocal::Update( void )
{
    // Update SFX channels
    src_update();
    
    // Update streams
    stream_update();
    mus_update();
    
    // Doppler
    if( s_doppler->modified )
    {
        s_dopplerFactor->modified = true;
        s_doppler->modified = false;
    }
    
    // Doppler parameters
    if( s_dopplerFactor->modified )
    {
        if( s_doppler->integer )
        {
            qalDopplerFactor( s_dopplerFactor->value );
        }
        else
        {
            qalDopplerFactor( 0.0f );
        }
        
        s_dopplerFactor->modified = false;
    }
    
    if( s_dopplerSpeed->modified )
    {
        qalDopplerVelocity( s_dopplerSpeed->value );
        s_dopplerSpeed->modified = false;
    }
    
    // Clear the modified flags on the other cvars
    s_gain->modified = false;
    s_volume->modified = false;
    s_musicVolume->modified = false;
    s_minDistance->modified = false;
    s_rolloff->modified = false;
}

/*
=================
idAudioOpenALSystemLocal::StopAllSounds
=================
*/
void idAudioOpenALSystemLocal::DisableSounds( void )
{
    StopAllSounds();
}

/*
=================
idAudioOpenALSystemLocal::BeginRegistration
=================
*/
void idAudioOpenALSystemLocal::BeginRegistration( void )
{
}

/*
=================
idAudioOpenALSystemLocal::ClearSoundBuffer
=================
*/
void idAudioOpenALSystemLocal::ClearSoundBuffer( void )
{
}

idSoundSystem* soundSystem;
idFileSystem* fileSystem;
idCVarSystem* cvarSystem;
idCmdSystem* cmdSystem;
idSystem* idsystem;
idParseSystem* ParseSystem;

#ifdef __LINUX__
extern "C" idAudioOpenALSystem* GetSndAPI( openALImports_t* oalimports )
#else
Q_EXPORT idAudioOpenALSystem* GetSndAPI( openALImports_t* oalimports )
#endif
{
    imports = oalimports;
    
    soundSystem = imports->soundSystem;
    fileSystem = imports->fileSystem;
    cvarSystem = imports->cvarSystem;
    cmdSystem = imports->cmdSystem;
    idsystem = imports->idsystem;
    ParseSystem = imports->parseSystem;
    
    return soundOpenALSystem;
}

/*
=================
idAudioOpenALSystemLocal::Play_f
=================
*/
void idAudioOpenALSystemLocal::Play_f( void )
{
    sint i;
    sfxHandle_t	h;
    valueType name[256];
    
    i = 1;
    while( i < cmdSystem->Argc() )
    {
        if( !::strrchr( cmdSystem->Argv( i ), '.' ) )
        {
            snprintf( name, sizeof( name ), "%s.wav", cmdSystem->Argv( 1 ) );
        }
        else
        {
            strncpy( name, cmdSystem->Argv( i ), sizeof( name ) );
        }
        
        h = soundOpenALSystem->RegisterSound( name, false );
        
        if( h )
        {
            soundOpenALSystem->StartLocalSound( h, CHAN_LOCAL_SOUND );
        }
        
        i++;
    }
}

/*
=================
idAudioOpenALSystemLocal::Music_f
=================
*/
void idAudioOpenALSystemLocal::Music_f( void )
{
    sint c;
    
    c = cmdSystem->Argc();
    
    if( c == 2 )
    {
        soundOpenALSystem->StartBackgroundTrack( cmdSystem->Argv( 1 ), cmdSystem->Argv( 1 ) );
    }
    else if( c == 3 )
    {
        soundOpenALSystem->StartBackgroundTrack( cmdSystem->Argv( 1 ), cmdSystem->Argv( 2 ) );
    }
    else
    {
        trap_Printf( PRINT_ALL, "music <musicfile> [loopfile]\n" );
        return;
    }
}

void trap_Printf( sint printLevel, pointer fmt, ... )
{
    imports->Printf( printLevel, fmt );
}

void trap_Error( sint errorLevel, pointer fmt, ... )
{
    imports->Error( errorLevel, fmt );
}
void* Hunk_AllocateTempMemory( uint64 size )
{
    return imports->Hunk_AllocateTempMemory( size );
}

void* trap_Hunk_Alloc( uint64 size, ha_pref preference )
{
    return imports->Hunk_Alloc( size, preference );
}

void trap_Hunk_FreeTempMemory( void* buf )
{
    imports->Hunk_FreeTempMemory( buf );
}

void* trap_Malloc( uint64 size )
{
    return imports->Malloc( size );
}

void trap_Free( void* ptr )
{
    imports->Free( ptr );
}
