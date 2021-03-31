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
// File name:   qal.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <soundSystemAL/sndSystemAL_precompiled.hpp>

LPALENABLE qalEnable;
LPALDISABLE qalDisable;
LPALISENABLED qalIsEnabled;
LPALGETSTRING qalGetString;
LPALGETBOOLEANV qalGetBooleanv;
LPALGETINTEGERV qalGetIntegerv;
LPALGETFLOATV qalGetFloatv;
LPALGETDOUBLEV qalGetDoublev;
LPALGETBOOLEAN qalGetBoolean;
LPALGETINTEGER qalGetInteger;
LPALGETFLOAT qalGetFloat;
LPALGETDOUBLE qalGetDouble;
LPALGETERROR qalGetError;
LPALISEXTENSIONPRESENT qalIsExtensionPresent;
LPALGETPROCADDRESS qalGetProcAddress;
LPALGETENUMVALUE qalGetEnumValue;
LPALLISTENERF qalListenerf;
LPALLISTENER3F qalListener3f;
LPALLISTENERFV qalListenerfv;
LPALLISTENERI qalListeneri;
LPALGETLISTENERF qalGetListenerf;
LPALGETLISTENER3F qalGetListener3f;
LPALGETLISTENERFV qalGetListenerfv;
LPALGETLISTENERI qalGetListeneri;
LPALGENSOURCES qalGenSources;
LPALDELETESOURCES qalDeleteSources;
LPALISSOURCE qalIsSource;
LPALSOURCEF qalSourcef;
LPALSOURCE3F qalSource3f;
LPALSOURCEFV qalSourcefv;
LPALSOURCEI qalSourcei;
LPALGETSOURCEF qalGetSourcef;
LPALGETSOURCE3F qalGetSource3f;
LPALGETSOURCEFV qalGetSourcefv;
LPALGETSOURCEI qalGetSourcei;
LPALSOURCEPLAYV qalSourcePlayv;
LPALSOURCESTOPV qalSourceStopv;
LPALSOURCEREWINDV qalSourceRewindv;
LPALSOURCEPAUSEV qalSourcePausev;
LPALSOURCEPLAY qalSourcePlay;
LPALSOURCESTOP qalSourceStop;
LPALSOURCEREWIND qalSourceRewind;
LPALSOURCEPAUSE qalSourcePause;
LPALSOURCEQUEUEBUFFERS qalSourceQueueBuffers;
LPALSOURCEUNQUEUEBUFFERS qalSourceUnqueueBuffers;
LPALGENBUFFERS qalGenBuffers;
LPALDELETEBUFFERS qalDeleteBuffers;
LPALISBUFFER qalIsBuffer;
LPALBUFFERDATA qalBufferData;
LPALGETBUFFERF qalGetBufferf;
LPALGETBUFFERI qalGetBufferi;
LPALDOPPLERFACTOR qalDopplerFactor;
LPALDOPPLERVELOCITY qalDopplerVelocity;
LPALDISTANCEMODEL qalDistanceModel;

LPALCCREATECONTEXT qalcCreateContext;
LPALCMAKECONTEXTCURRENT qalcMakeContextCurrent;
LPALCPROCESSCONTEXT qalcProcessContext;
LPALCSUSPENDCONTEXT qalcSuspendContext;
LPALCDESTROYCONTEXT qalcDestroyContext;
LPALCGETCURRENTCONTEXT qalcGetCurrentContext;
LPALCGETCONTEXTSDEVICE qalcGetContextsDevice;
LPALCOPENDEVICE qalcOpenDevice;
LPALCCLOSEDEVICE qalcCloseDevice;
LPALCGETERROR qalcGetError;
LPALCISEXTENSIONPRESENT qalcIsExtensionPresent;
LPALCGETPROCADDRESS qalcGetProcAddress;
LPALCGETENUMVALUE qalcGetEnumValue;
LPALCGETSTRING qalcGetString;
LPALCGETINTEGERV qalcGetIntegerv;
LPALCCAPTUREOPENDEVICE qalcCaptureOpenDevice;
LPALCCAPTURECLOSEDEVICE qalcCaptureCloseDevice;
LPALCCAPTURESTART qalcCaptureStart;
LPALCCAPTURESTOP qalcCaptureStop;
LPALCCAPTURESAMPLES qalcCaptureSamples;

static void* OpenALLib = nullptr;

static bool alinit_fail = false;

/*
=================
GPA
=================
*/
static void* GPA( valueType* str )
{
    void* rv;
    
    rv = SDL_LoadFunction( OpenALLib, str );
    if( !rv )
    {
        trap_Printf( PRINT_ALL, " Can't load symbol %s\n", str );
        alinit_fail = true;
        return nullptr;
    }
    else
    {
        // Dushan : Print some developer information
        // Don't allow this for release
#if defined (_DEBUG)
        trap_Printf( PRINT_ALL, " Loaded symbol %s (0x%08X)\n", str, rv );
#endif
        return rv;
    }
}

/*
=================
QAL_Init
=================
*/
bool QAL_Init( pointer libname )
{
    if( OpenALLib )
    {
        return true;
    }
    
    if( ( OpenALLib = SDL_LoadObject( libname ) ) == 0 )
    {
#ifdef _WIN32
        trap_Printf( PRINT_ALL, " Can't load %s\n", libname );
        return false;
#else
        valueType fn[1024];
        ::strncat( fn, "/", sizeof( fn ) - ::strlen( ::getcwd( fn, sizeof( fn ) ) ) - 1 );
        ::strncat( fn, libname, sizeof( fn ) - ::strlen( fn ) - 1 );
        
        if( ( OpenALLib = SDL_LoadObject( fn ) ) == 0 )
        {
            trap_Printf( PRINT_ALL, " Can't load %s\n", libname );
            return false;
        }
#endif
    }
    
    alinit_fail = false;
    
    qalEnable = reinterpret_cast<LPALENABLE>( GPA( "alEnable" ) );
    qalDisable = reinterpret_cast<LPALDISABLE>( GPA( "alDisable" ) );
    qalIsEnabled = reinterpret_cast<LPALISENABLED>( GPA( "alIsEnabled" ) );
    qalGetString = reinterpret_cast<LPALGETSTRING>( GPA( "alGetString" ) );
    qalGetBooleanv = reinterpret_cast<LPALGETBOOLEANV>( GPA( "alGetBooleanv" ) );
    qalGetIntegerv = reinterpret_cast<LPALGETINTEGERV>( GPA( "alGetIntegerv" ) );
    qalGetFloatv = reinterpret_cast<LPALGETFLOATV>( GPA( "alGetFloatv" ) );
    qalGetDoublev = reinterpret_cast<LPALGETDOUBLEV>( GPA( "alGetDoublev" ) );
    qalGetBoolean = reinterpret_cast<LPALGETBOOLEAN>( GPA( "alGetBoolean" ) );
    qalGetInteger = reinterpret_cast<LPALGETINTEGER>( GPA( "alGetInteger" ) );
    qalGetFloat = reinterpret_cast<LPALGETFLOAT>( GPA( "alGetFloat" ) );
    qalGetDouble = reinterpret_cast<LPALGETDOUBLE>( GPA( "alGetDouble" ) );
    qalGetError = reinterpret_cast<LPALGETERROR>( GPA( "alGetError" ) );
    qalIsExtensionPresent = reinterpret_cast<LPALISEXTENSIONPRESENT>( GPA( "alIsExtensionPresent" ) );
    qalGetProcAddress = reinterpret_cast<LPALGETPROCADDRESS>( GPA( "alGetProcAddress" ) );
    qalGetEnumValue = reinterpret_cast<LPALGETENUMVALUE>( GPA( "alGetEnumValue" ) );
    qalListenerf = reinterpret_cast<LPALLISTENERF>( GPA( "alListenerf" ) );
    qalListener3f = reinterpret_cast<LPALLISTENER3F>( GPA( "alListener3f" ) );
    qalListenerfv = reinterpret_cast<LPALLISTENERFV>( GPA( "alListenerfv" ) );
    qalListeneri = reinterpret_cast<LPALLISTENERI>( GPA( "alListeneri" ) );
    qalGetListenerf = reinterpret_cast<LPALGETLISTENERF>( GPA( "alGetListenerf" ) );
    qalGetListener3f = reinterpret_cast<LPALGETLISTENER3F>( GPA( "alGetListener3f" ) );
    qalGetListenerfv = reinterpret_cast<LPALGETLISTENERFV>( GPA( "alGetListenerfv" ) );
    qalGetListeneri = reinterpret_cast<LPALGETLISTENERI>( GPA( "alGetListeneri" ) );
    qalGenSources = reinterpret_cast<LPALGENSOURCES>( GPA( "alGenSources" ) );
    qalDeleteSources = reinterpret_cast<LPALDELETESOURCES>( GPA( "alDeleteSources" ) );
    qalIsSource = reinterpret_cast<LPALISSOURCE>( GPA( "alIsSource" ) );
    qalSourcef = reinterpret_cast<LPALSOURCEF>( GPA( "alSourcef" ) );
    qalSource3f = reinterpret_cast<LPALSOURCE3F>( GPA( "alSource3f" ) );
    qalSourcefv = reinterpret_cast<LPALSOURCEFV>( GPA( "alSourcefv" ) );
    qalSourcei = reinterpret_cast<LPALSOURCEI>( GPA( "alSourcei" ) );
    qalGetSourcef = reinterpret_cast<LPALGETSOURCEF>( GPA( "alGetSourcef" ) );
    qalGetSource3f = reinterpret_cast<LPALGETSOURCE3F>( GPA( "alGetSource3f" ) );
    qalGetSourcefv = reinterpret_cast<LPALGETSOURCEFV>( GPA( "alGetSourcefv" ) );
    qalGetSourcei = reinterpret_cast<LPALGETSOURCEI>( GPA( "alGetSourcei" ) );
    qalSourcePlayv = reinterpret_cast<LPALSOURCEPLAYV>( GPA( "alSourcePlayv" ) );
    qalSourceStopv = reinterpret_cast<LPALSOURCESTOPV>( GPA( "alSourceStopv" ) );
    qalSourceRewindv = reinterpret_cast<LPALSOURCEREWINDV>( GPA( "alSourceRewindv" ) );
    qalSourcePausev = reinterpret_cast<LPALSOURCEPAUSEV>( GPA( "alSourcePausev" ) );
    qalSourcePlay = reinterpret_cast<LPALSOURCEPLAY>( GPA( "alSourcePlay" ) );
    qalSourceStop = reinterpret_cast<LPALSOURCESTOP>( GPA( "alSourceStop" ) );
    qalSourceRewind = reinterpret_cast<LPALSOURCEREWIND>( GPA( "alSourceRewind" ) );
    qalSourcePause = reinterpret_cast<LPALSOURCEPAUSE>( GPA( "alSourcePause" ) );
    qalSourceQueueBuffers = reinterpret_cast<LPALSOURCEQUEUEBUFFERS>( GPA( "alSourceQueueBuffers" ) );
    qalSourceUnqueueBuffers = reinterpret_cast<LPALSOURCEUNQUEUEBUFFERS>( GPA( "alSourceUnqueueBuffers" ) );
    qalGenBuffers = reinterpret_cast<LPALGENBUFFERS>( GPA( "alGenBuffers" ) );
    qalDeleteBuffers = reinterpret_cast<LPALDELETEBUFFERS>( GPA( "alDeleteBuffers" ) );
    qalIsBuffer = reinterpret_cast<LPALISBUFFER>( GPA( "alIsBuffer" ) );
    qalBufferData = reinterpret_cast<LPALBUFFERDATA>( GPA( "alBufferData" ) );
    qalGetBufferf = reinterpret_cast<LPALGETBUFFERF>( GPA( "alGetBufferf" ) );
    qalGetBufferi = reinterpret_cast<LPALGETBUFFERI>( GPA( "alGetBufferi" ) );
    qalDopplerFactor = reinterpret_cast<LPALDOPPLERFACTOR>( GPA( "alDopplerFactor" ) );
    qalDopplerVelocity = reinterpret_cast<LPALDOPPLERVELOCITY>( GPA( "alDopplerVelocity" ) );
    qalDistanceModel = reinterpret_cast<LPALDISABLE>( GPA( "alDistanceModel" ) );
    
    qalcCreateContext = reinterpret_cast<LPALCCREATECONTEXT>( GPA( "alcCreateContext" ) );
    qalcMakeContextCurrent = reinterpret_cast<LPALCMAKECONTEXTCURRENT>( GPA( "alcMakeContextCurrent" ) );
    qalcProcessContext = reinterpret_cast<LPALCPROCESSCONTEXT>( GPA( "alcProcessContext" ) );
    qalcSuspendContext = reinterpret_cast<LPALCSUSPENDCONTEXT>( GPA( "alcSuspendContext" ) );
    qalcDestroyContext = reinterpret_cast<LPALCDESTROYCONTEXT>( GPA( "alcDestroyContext" ) );
    qalcGetCurrentContext = reinterpret_cast<LPALCGETCURRENTCONTEXT>( GPA( "alcGetCurrentContext" ) );
    qalcGetContextsDevice = reinterpret_cast<LPALCGETCONTEXTSDEVICE>( GPA( "alcGetContextsDevice" ) );
    qalcOpenDevice = reinterpret_cast<LPALCOPENDEVICE>( GPA( "alcOpenDevice" ) );
    qalcCloseDevice = reinterpret_cast<LPALCCLOSEDEVICE>( GPA( "alcCloseDevice" ) );
    qalcGetError = reinterpret_cast<LPALCGETERROR>( GPA( "alcGetError" ) );
    qalcIsExtensionPresent = reinterpret_cast<LPALCISEXTENSIONPRESENT>( GPA( "alcIsExtensionPresent" ) );
    qalcGetProcAddress = reinterpret_cast<LPALCGETPROCADDRESS>( GPA( "alcGetProcAddress" ) );
    qalcGetEnumValue = reinterpret_cast<LPALCGETENUMVALUE>( GPA( "alcGetEnumValue" ) );
    qalcGetString = reinterpret_cast<LPALCGETSTRING>( GPA( "alcGetString" ) );
    qalcGetIntegerv = reinterpret_cast<LPALCGETINTEGERV>( GPA( "alcGetIntegerv" ) );
    qalcCaptureOpenDevice = reinterpret_cast<LPALCCAPTUREOPENDEVICE>( GPA( "alcCaptureOpenDevice" ) );
    qalcCaptureCloseDevice = reinterpret_cast<LPALCCAPTURECLOSEDEVICE>( GPA( "alcCaptureCloseDevice" ) );
    qalcCaptureStart = reinterpret_cast<LPALCCAPTURESTART>( GPA( "alcCaptureStart" ) );
    qalcCaptureStop = reinterpret_cast<LPALCCAPTURESTOP>( GPA( "alcCaptureStop" ) );
    qalcCaptureSamples = reinterpret_cast<LPALCCAPTURESAMPLES>( GPA( "alcCaptureSamples" ) );
    
    if( alinit_fail )
    {
        QAL_Shutdown();
        trap_Printf( PRINT_ALL, " One or more symbols not found\n" );
        return false;
    }
    
    return true;
}

/*
=================
QAL_Shutdown
=================
*/
void QAL_Shutdown( void )
{
    if( OpenALLib )
    {
        SDL_UnloadObject( OpenALLib );
        OpenALLib = nullptr;
    }
    
    qalEnable = nullptr;
    qalDisable = nullptr;
    qalIsEnabled = nullptr;
    qalGetString = nullptr;
    qalGetBooleanv = nullptr;
    qalGetIntegerv = nullptr;
    qalGetFloatv = nullptr;
    qalGetDoublev = nullptr;
    qalGetBoolean = nullptr;
    qalGetInteger = nullptr;
    qalGetFloat = nullptr;
    qalGetDouble = nullptr;
    qalGetError = nullptr;
    qalIsExtensionPresent = nullptr;
    qalGetProcAddress = nullptr;
    qalGetEnumValue = nullptr;
    qalListenerf = nullptr;
    qalListener3f = nullptr;
    qalListenerfv = nullptr;
    qalListeneri = nullptr;
    qalGetListenerf = nullptr;
    qalGetListener3f = nullptr;
    qalGetListenerfv = nullptr;
    qalGetListeneri = nullptr;
    qalGenSources = nullptr;
    qalDeleteSources = nullptr;
    qalIsSource = nullptr;
    qalSourcef = nullptr;
    qalSource3f = nullptr;
    qalSourcefv = nullptr;
    qalSourcei = nullptr;
    qalGetSourcef = nullptr;
    qalGetSource3f = nullptr;
    qalGetSourcefv = nullptr;
    qalGetSourcei = nullptr;
    qalSourcePlayv = nullptr;
    qalSourceStopv = nullptr;
    qalSourceRewindv = nullptr;
    qalSourcePausev = nullptr;
    qalSourcePlay = nullptr;
    qalSourceStop = nullptr;
    qalSourceRewind = nullptr;
    qalSourcePause = nullptr;
    qalSourceQueueBuffers = nullptr;
    qalSourceUnqueueBuffers = nullptr;
    qalGenBuffers = nullptr;
    qalDeleteBuffers = nullptr;
    qalIsBuffer = nullptr;
    qalBufferData = nullptr;
    qalGetBufferf = nullptr;
    qalGetBufferi = nullptr;
    qalDopplerFactor = nullptr;
    qalDopplerVelocity = nullptr;
    qalDistanceModel = nullptr;
    
    qalcCreateContext = nullptr;
    qalcMakeContextCurrent = nullptr;
    qalcProcessContext = nullptr;
    qalcSuspendContext = nullptr;
    qalcDestroyContext = nullptr;
    qalcGetCurrentContext = nullptr;
    qalcGetContextsDevice = nullptr;
    qalcOpenDevice = nullptr;
    qalcCloseDevice = nullptr;
    qalcGetError = nullptr;
    qalcIsExtensionPresent = nullptr;
    qalcGetProcAddress = nullptr;
    qalcGetEnumValue = nullptr;
    qalcGetString = nullptr;
    qalcGetIntegerv = nullptr;
    qalcCaptureOpenDevice = nullptr;
    qalcCaptureCloseDevice = nullptr;
    qalcCaptureStart = nullptr;
    qalcCaptureStop = nullptr;
    qalcCaptureSamples = nullptr;
}
