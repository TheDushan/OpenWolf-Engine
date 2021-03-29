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
            return qfalse;
        }
#endif
    }
    
    alinit_fail = false;
    
    qalEnable = static_cast<LPALENABLE>( GPA( "alEnable" ) );
    qalDisable = static_cast<LPALDISABLE>( GPA( "alDisable" ) );
    qalIsEnabled = static_cast<LPALISENABLED>( GPA( "alIsEnabled" ) );
    qalGetString = static_cast<LPALGETSTRING>( GPA( "alGetString" ) );
    qalGetBooleanv = static_cast<LPALGETBOOLEANV>( GPA( "alGetBooleanv" ) );
    qalGetIntegerv = static_cast<LPALGETINTEGERV>( GPA( "alGetIntegerv" ) );
    qalGetFloatv = static_cast<LPALGETFLOATV>( GPA( "alGetFloatv" ) );
    qalGetDoublev = static_cast<LPALGETDOUBLEV>( GPA( "alGetDoublev" ) );
    qalGetBoolean = static_cast<LPALGETBOOLEAN>( GPA( "alGetBoolean" ) );
    qalGetInteger = static_cast<LPALGETINTEGER>( GPA( "alGetInteger" ) );
    qalGetFloat = static_cast<LPALGETFLOAT>( GPA( "alGetFloat" ) );
    qalGetDouble = static_cast<LPALGETDOUBLE>( GPA( "alGetDouble" ) );
    qalGetError = static_cast<LPALGETERROR>( GPA( "alGetError" ) );
    qalIsExtensionPresent = static_cast<LPALISEXTENSIONPRESENT>( GPA( "alIsExtensionPresent" ) );
    qalGetProcAddress = static_cast<LPALGETPROCADDRESS>( GPA( "alGetProcAddress" ) );
    qalGetEnumValue = static_cast<LPALGETENUMVALUE>( GPA( "alGetEnumValue" ) );
    qalListenerf = static_cast<LPALLISTENERF>( GPA( "alListenerf" ) );
    qalListener3f = static_cast<LPALLISTENER3F>( GPA( "alListener3f" ) );
    qalListenerfv = static_cast<LPALLISTENERFV>( GPA( "alListenerfv" ) );
    qalListeneri = static_cast<LPALLISTENERI>( GPA( "alListeneri" ) );
    qalGetListenerf = static_cast<LPALGETLISTENERF>( GPA( "alGetListenerf" ) );
    qalGetListener3f = static_cast<LPALGETLISTENER3F>( GPA( "alGetListener3f" ) );
    qalGetListenerfv = static_cast<LPALGETLISTENERFV>( GPA( "alGetListenerfv" ) );
    qalGetListeneri = static_cast<LPALGETLISTENERI>( GPA( "alGetListeneri" ) );
    qalGenSources = static_cast<LPALGENSOURCES>( GPA( "alGenSources" ) );
    qalDeleteSources = static_cast<LPALDELETESOURCES>( GPA( "alDeleteSources" ) );
    qalIsSource = static_cast<LPALISSOURCE>( GPA( "alIsSource" ) );
    qalSourcef = static_cast<LPALSOURCEF>( GPA( "alSourcef" ) );
    qalSource3f = static_cast<LPALSOURCE3F>( GPA( "alSource3f" ) );
    qalSourcefv = static_cast<LPALSOURCEFV>( GPA( "alSourcefv" ) );
    qalSourcei = static_cast<LPALSOURCEI>( GPA( "alSourcei" ) );
    qalGetSourcef = static_cast<LPALGETSOURCEF>( GPA( "alGetSourcef" ) );
    qalGetSource3f = static_cast<LPALGETSOURCE3F>( GPA( "alGetSource3f" ) );
    qalGetSourcefv = static_cast<LPALGETSOURCEFV>( GPA( "alGetSourcefv" ) );
    qalGetSourcei = static_cast<LPALGETSOURCEI>( GPA( "alGetSourcei" ) );
    qalSourcePlayv = static_cast<LPALSOURCEPLAYV>( GPA( "alSourcePlayv" ) );
    qalSourceStopv = static_cast<LPALSOURCESTOPV>( GPA( "alSourceStopv" ) );
    qalSourceRewindv = static_cast<LPALSOURCEREWINDV>( GPA( "alSourceRewindv" ) );
    qalSourcePausev = static_cast<LPALSOURCEPAUSEV>( GPA( "alSourcePausev" ) );
    qalSourcePlay = static_cast<LPALSOURCEPLAY>( GPA( "alSourcePlay" ) );
    qalSourceStop = static_cast<LPALSOURCESTOP>( GPA( "alSourceStop" ) );
    qalSourceRewind = static_cast<LPALSOURCEREWIND>( GPA( "alSourceRewind" ) );
    qalSourcePause = static_cast<LPALSOURCEPAUSE>( GPA( "alSourcePause" ) );
    qalSourceQueueBuffers = static_cast<LPALSOURCEQUEUEBUFFERS>( GPA( "alSourceQueueBuffers" ) );
    qalSourceUnqueueBuffers = static_cast<LPALSOURCEUNQUEUEBUFFERS>( GPA( "alSourceUnqueueBuffers" ) );
    qalGenBuffers = static_cast<LPALGENBUFFERS>( GPA( "alGenBuffers" ) );
    qalDeleteBuffers = static_cast<LPALDELETEBUFFERS>( GPA( "alDeleteBuffers" ) );
    qalIsBuffer = static_cast<LPALISBUFFER>( GPA( "alIsBuffer" ) );
    qalBufferData = static_cast<LPALBUFFERDATA>( GPA( "alBufferData" ) );
    qalGetBufferf = static_cast<LPALGETBUFFERF>( GPA( "alGetBufferf" ) );
    qalGetBufferi = static_cast<LPALGETBUFFERI>( GPA( "alGetBufferi" ) );
    qalDopplerFactor = static_cast<LPALDOPPLERFACTOR>( GPA( "alDopplerFactor" ) );
    qalDopplerVelocity = static_cast<LPALDOPPLERVELOCITY>( GPA( "alDopplerVelocity" ) );
    qalDistanceModel = static_cast<LPALDISABLE>( GPA( "alDistanceModel" ) );
    
    qalcCreateContext = static_cast<LPALCCREATECONTEXT>( GPA( "alcCreateContext" ) );
    qalcMakeContextCurrent = static_cast<LPALCMAKECONTEXTCURRENT>( GPA( "alcMakeContextCurrent" ) );
    qalcProcessContext = static_cast<LPALCPROCESSCONTEXT>( GPA( "alcProcessContext" ) );
    qalcSuspendContext = static_cast<LPALCSUSPENDCONTEXT>( GPA( "alcSuspendContext" ) );
    qalcDestroyContext = static_cast<LPALCDESTROYCONTEXT>( GPA( "alcDestroyContext" ) );
    qalcGetCurrentContext = static_cast<LPALCGETCURRENTCONTEXT>( GPA( "alcGetCurrentContext" ) );
    qalcGetContextsDevice = static_cast<LPALCGETCONTEXTSDEVICE>( GPA( "alcGetContextsDevice" ) );
    qalcOpenDevice = static_cast<LPALCOPENDEVICE>( GPA( "alcOpenDevice" ) );
    qalcCloseDevice = static_cast<LPALCCLOSEDEVICE>( GPA( "alcCloseDevice" ) );
    qalcGetError = static_cast<LPALCGETERROR>( GPA( "alcGetError" ) );
    qalcIsExtensionPresent = static_cast<LPALCISEXTENSIONPRESENT>( GPA( "alcIsExtensionPresent" ) );
    qalcGetProcAddress = static_cast<LPALCGETPROCADDRESS>( GPA( "alcGetProcAddress" ) );
    qalcGetEnumValue = static_cast<LPALCGETENUMVALUE>( GPA( "alcGetEnumValue" ) );
    qalcGetString = static_cast<LPALCGETSTRING>( GPA( "alcGetString" ) );
    qalcGetIntegerv = static_cast<LPALCGETINTEGERV>( GPA( "alcGetIntegerv" ) );
    qalcCaptureOpenDevice = static_cast<LPALCCAPTUREOPENDEVICE>( GPA( "alcCaptureOpenDevice" ) );
    qalcCaptureCloseDevice = static_cast<LPALCCAPTURECLOSEDEVICE>( GPA( "alcCaptureCloseDevice" ) );
    qalcCaptureStart = static_cast<LPALCCAPTURESTART>( GPA( "alcCaptureStart" ) );
    qalcCaptureStop = static_cast<LPALCCAPTURESTOP>( GPA( "alcCaptureStop" ) );
    qalcCaptureSamples = static_cast<LPALCCAPTURESAMPLES>( GPA( "alcCaptureSamples" ) );
    
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
