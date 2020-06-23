////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2019 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of the OpenWolf GPL Source Code.
// OpenWolf Source Code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenWolf Source Code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenWolf Source Code.  If not, see <http://www.gnu.org/licenses/>.
//
// In addition, the OpenWolf Source Code is also subject to certain additional terms.
// You should have received a copy of these additional terms immediately following the
// terms and conditions of the GNU General Public License which accompanied the
// OpenWolf Source Code. If not, please request a copy in writing from id Software
// at the address below.
//
// If you have questions concerning this license or the applicable additional terms,
// you may contact in writing id Software LLC, c/o ZeniMax Media Inc.,
// Suite 120, Rockville, Maryland 20850 USA.
//
// -------------------------------------------------------------------------------------
// File name:   systemMain.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.h>
#elif DEDICATED
#include <null/null_serverprecompiled.h>
#else
#include <framework/precompiled.h>
#endif

idSystemLocal systemLocal;
idSystem* idsystem = &systemLocal;

/*
===============
idSystemLocal::idSystemLocal
===============
*/
idSystemLocal::idSystemLocal( void )
{
}

/*
===============
idSystemLocal::~idSystemLocal
===============
*/
idSystemLocal::~idSystemLocal( void )
{
}

/*
=================
idSystemLocal::SetBinaryPath
=================
*/
void idSystemLocal::SetBinaryPath( pointer path )
{
    Q_strncpyz( binaryPath, path, sizeof( binaryPath ) );
}

/*
=================
idSystemLocal::BinaryPath
=================
*/
valueType* idSystemLocal::BinaryPath( void )
{
    return binaryPath;
}

/*
=================
idSystemLocal::SetDefaultInstallPath
=================
*/
void idSystemLocal::SetDefaultInstallPath( pointer path )
{
    Q_strncpyz( installPath, path, sizeof( installPath ) );
}

/*
=================
idSystemLocal::DefaultInstallPath
=================
*/
valueType* idSystemLocal::DefaultInstallPath( void )
{
    static valueType installdir[MAX_OSPATH];
    
    Com_sprintf( installdir, sizeof( installdir ), "%s", Cwd() );
    
    // Windows
    Q_strreplace( installdir, sizeof( installdir ), "bin/windows", "" );
    Q_strreplace( installdir, sizeof( installdir ), "bin\\windows", "" );
    
    // Linux
    Q_strreplace( installdir, sizeof( installdir ), "bin/unix", "" );
    
    // BSD
    Q_strreplace( installdir, sizeof( installdir ), "bin/bsd", "" );
    
    // MacOS X
    Q_strreplace( installdir, sizeof( installdir ), "bin/macosx", "" );
    
    return installdir;
}

/*
=================
idSystemLocal::SetDefaultLibPath
=================
*/
void idSystemLocal::SetDefaultLibPath( pointer path )
{
    Q_strncpyz( libPath, path, sizeof( libPath ) );
}

/*
=================
idSystemLocal::DefaultLibPath
=================
*/
valueType* idSystemLocal::DefaultLibPath( void )
{
    if( *libPath )
    {
        return libPath;
    }
    else
    {
        return Cwd();
    }
}

/*
=================
idSystemLocal::DefaultAppPath
=================
*/
valueType* idSystemLocal::DefaultAppPath( void )
{
    return BinaryPath();
}

/*
=================
idSystemLocal::Restart_f

Restart the input subsystem
=================
*/
void idSystemLocal::Restart_f( void )
{
    Restart();
}

/*
=================
idSystemLocal::ConsoleInput

Handle new console input
=================
*/
valueType* idSystemLocal::ConsoleInput( void )
{
    return consoleCursesSystem->Input( );
}

/*
=================
idSystemLocal::PIDFileName
=================
*/
valueType* idSystemLocal::PIDFileName( void )
{
    return va( "%s/%s", TempPath( ), PID_FILENAME );
}

/*
=================
idSystemLocal::WritePIDFile

Return true if there is an existing stale PID file
=================
*/
bool idSystemLocal::WritePIDFile( void )
{
    valueType* pidFile = PIDFileName( );
    FILE* f;
    bool stale = false;
    
    // First, check if the pid file is already there
    if( ( f = fopen( pidFile, "r" ) ) != nullptr )
    {
        valueType  pidBuffer[ 64 ] = { 0 };
        sint   pid;
        
        fread( pidBuffer, sizeof( valueType ), sizeof( pidBuffer ) - 1, f );
        fclose( f );
        
        pid = atoi( pidBuffer );
        if( !PIDIsRunning( pid ) )
        {
            stale = true;
        }
    }
    
    if( ( f = fopen( pidFile, "w" ) ) != nullptr )
    {
        fprintf( f, "%d", PID( ) );
        fclose( f );
    }
    else
    {
        Com_Printf( S_COLOR_YELLOW "Couldn't write %s.\n", pidFile );
    }
    
    return stale;
}

/*
=================
idSystemLocal::Exit

Single exit point (regular exit or in case of error)
=================
*/
void idSystemLocal::Exit( sint exitCode )
{
    consoleCursesSystem->Shutdown( );
    
    if( exitCode < 2 )
    {
        // Normal exit
        remove( PIDFileName( ) );
    }
    
    exit( exitCode );
    
#ifndef DEDICATED
    SDL_Quit();
#endif
}

/*
=================
idSystemLocal::Quit
=================
*/
void idSystemLocal::Quit( void )
{
    Exit( 0 );
}

/*
=================
idSystemLocal::Init
=================
*/
void idSystemLocal::Init( void )
{
    cmdSystem->AddCommand( "in_restart", &idSystemLocal::Restart, "Restarts all the input drivers, dinput, joystick, etc." );
    cvarSystem->Set( "arch", OS_STRING " " ARCH_STRING );
    cvarSystem->Set( "username", GetCurrentUser( ) );
}

/*
=================
Host_RecordError
=================
*/
void idSystemLocal::RecordError( pointer msg )
{
    msg;
}

/*
=================
idSystemLocal::WriteDump
=================
*/
void idSystemLocal::WriteDump( pointer fmt, ... )
{
#if defined( _WIN32 )

#ifndef DEVELOPER
    if( com_developer && com_developer->integer )
#endif
    {
        //this memory should live as long as the SEH is doing its thing...I hope
        static valueType msg[2048];
        va_list vargs;
        
        /*
        	Do our own vsnprintf as using va's will change global state
        	that might be pertinent to the dump.
        */
        
        va_start( vargs, fmt );
        Q_vsnprintf( msg, sizeof( msg ) - 1, fmt, vargs );
        va_end( vargs );
        
        msg[sizeof( msg ) - 1] = 0; //ensure null termination
        
        RecordError( msg );
    }
    
#endif
}

/*
=================
idSystemLocal::Print
=================
*/
void idSystemLocal::Print( pointer msg )
{
    consoleLoggingSystem->LogWrite( msg );
    consoleCursesSystem->Print( msg );
}

/*
=================
idSystemLocal::Error
=================
*/
void idSystemLocal::Error( pointer error, ... )
{
    va_list argptr;
    valueType string[4096];
    
    va_start( argptr, error );
    Q_vsnprintf( string, sizeof( string ), error, argptr );
    va_end( argptr );
    
    // Print text in the console window/box
    Print( string );
    Print( "\n" );
    
    CL_Shutdown( );
    ErrorDialog( string );
    
    Exit( 3 );
}

/*
=================
idSystemLocal::UnloadDll
=================
*/
void idSystemLocal::UnloadDll( void* dllHandle )
{
    if( !dllHandle )
    {
        Com_Printf( "idSystemLocal::UnloadDll(nullptr)\n" );
        return;
    }
    
    SDL_UnloadObject( dllHandle );
}
/*
=================
idSystemLocal::GetDLLName

Used to load a development dll instead of a virtual machine
=================
*/
valueType* idSystemLocal::GetDLLName( pointer name )
{
    //Dushan - I have no idea what this mess is and what I made it before
    return va( "%s" ARCH_STRING DLL_EXT, name );
}

/*
=================
idSystemLocal::LoadDll

Used to load a development dll instead of a virtual machine
#1 look in fs_homepath
#2 look in fs_basepath
#4 look in fs_libpath under FreeBSD
=================
*/
void* idSystemLocal::LoadDll( pointer name )
{
    void* libHandle = nullptr;
    static sint lastWarning = 0;
    valueType* basepath;
    valueType* homepath;
    valueType* gamedir;
    valueType* fn;
    valueType filename[MAX_QPATH];
    
    assert( name );
    
    Q_strncpyz( filename, GetDLLName( name ), sizeof( filename ) );
    
    basepath = cvarSystem->VariableString( "fs_basepath" );
    homepath = cvarSystem->VariableString( "fs_homepath" );
    gamedir = cvarSystem->VariableString( "fs_game" );
    
    fn = fileSystem->BuildOSPath( basepath, gamedir, filename );
    
#if !defined( DEDICATED )
    // if the server is pure, extract the dlls from the bin.pk3 so
    // that they can be referenced
    if( cvarSystem->VariableValue( "sv_pure" ) && Q_stricmp( name, "sgame" ) )
    {
        fileSystem->CL_ExtractFromPakFile( homepath, gamedir, filename );
    }
#endif
    
    libHandle = SDL_LoadObject( fn );
    
    if( !libHandle )
    {
        Com_DPrintf( "idSystemLocal::LoadDll(%s) failed: \"%s\"\n", fn, SDL_GetError() );
        if( homepath[0] )
        {
            Com_DPrintf( "idSystemLocal::LoadDll(%s) failed: \"%s\"\n", fn, SDL_GetError() );
            fn = fileSystem->BuildOSPath( homepath, gamedir, filename );
            libHandle = SDL_LoadObject( fn );
        }
        if( !libHandle )
        {
            Com_DPrintf( "idSystemLocal::LoadDll(%s) failed: \"%s\"\n", fn, SDL_GetError() );
            if( !libHandle )
            {
                Com_DPrintf( "idSystemLocal::LoadDll(%s) failed: \"%s\"\n", fn, SDL_GetError() );
                // now we try base
                fn = fileSystem->BuildOSPath( basepath, BASEGAME, filename );
                libHandle = SDL_LoadObject( fn );
                if( !libHandle )
                {
                    if( homepath[0] )
                    {
                        Com_DPrintf( "idSystemLocal::LoadDll(%s) failed: \"%s\"\n", fn, SDL_GetError() );
                        fn = fileSystem->BuildOSPath( homepath, BASEGAME, filename );
                        libHandle = SDL_LoadObject( fn );
                    }
                    if( !libHandle )
                    {
                        Com_DPrintf( "idSystemLocal::LoadDll(%s) failed: \"%s\"\n", fn, SDL_GetError() );
                        if( !libHandle )
                        {
                            Com_DPrintf( "idSystemLocal::LoadDll(%s) failed: \"%s\"\n", fn, SDL_GetError() );
                            return nullptr;
                        }
                    }
                }
            }
        }
    }
    
    return libHandle;
}

/*
=================
idSystemLocal::GetProcAddress
=================
*/
void* idSystemLocal::GetProcAddress( void* dllhandle, pointer name )
{
    return SDL_LoadFunction( dllhandle, name );
}

/*
=================
idSystemLocal::ParseArgs
=================
*/
void idSystemLocal::ParseArgs( sint argc, valueType** argv )
{
    sint i;
    
    if( argc == 2 )
    {
        if( !strcmp( argv[1], "--version" ) || !strcmp( argv[1], "-v" ) )
        {
            pointer date = __DATE__;
#ifdef DEDICATED
            fprintf( stdout, Q3_VERSION " dedicated server (%s)\n", date );
#else
            fprintf( stdout, Q3_VERSION " client (%s)\n", date );
#endif
            Exit( 0 );
        }
    }
    
    for( i = 1; i < argc; i++ )
    {
        if( !strcmp( argv[i], "+nocurses" ) )
        {
            break;
        }
    }
}

/*
================
idSystemLocal::SignalToString
================
*/
pointer idSystemLocal::SignalToString( sint sig )
{
    switch( sig )
    {
        case SIGINT:
            return "Terminal interrupt signal";
        case SIGILL:
            return "Illegal instruction";
        case SIGFPE:
            return "Erroneous arithmetic operation";
        case SIGSEGV:
            return "Invalid memory reference";
        case SIGTERM:
            return "Termination signal";
#if defined (_WIN32)
        case SIGBREAK:
            return "Control-break";
#endif
        case SIGABRT:
            return "Process abort signal";
        default:
            return "unknown";
    }
}

/*
=================
idSystemLocal::SigHandler
=================
*/
void idSystemLocal::SigHandler( sint signal )
{
    static bool signalcaught = false;
    
    if( signalcaught )
    {
        Com_Printf( "DOUBLE SIGNAL FAULT: Received signal %d: \"%s\", exiting...\n", signal, SignalToString( signal ) );
        exit( 1 );
    }
    else
    {
        signalcaught = true;
#ifndef DEDICATED
        CL_Shutdown( );
#endif
        serverInitSystem->Shutdown( va( "Received signal %d", signal ) );
    }
    
    if( signal == SIGTERM || signal == SIGINT )
    {
        Exit( 1 );
    }
    else
    {
        Exit( 2 );
    }
}

/*
================
idSystemLocal::SnapVector
================
*/
float32 idSystemLocal::roundfloat( float32 n )
{
#ifdef _WIN32
    return ( n < 0.0f ) ? ceilf( n - 0.5f ) : floorf( n + 0.5f );
#endif
}

void idSystemLocal::SysSnapVector( float32* v )
{
#ifdef _WIN32
    __m128 vf0, vf1, vf2;
    __m128i vi;
    DWORD mxcsr;
    
    mxcsr = _mm_getcsr();
    vf0 = _mm_setr_ps( v[0], v[1], v[2], 0.0f );
    
    _mm_setcsr( mxcsr & ~0x6000 ); // enforce rounding mode to "round to nearest"
    
    vi = _mm_cvtps_epi32( vf0 );
    vf0 = _mm_cvtepi32_ps( vi );
    
    vf1 = _mm_shuffle_ps( vf0, vf0, _MM_SHUFFLE( 1, 1, 1, 1 ) );
    vf2 = _mm_shuffle_ps( vf0, vf0, _MM_SHUFFLE( 2, 2, 2, 2 ) );
    
    _mm_setcsr( mxcsr ); // restore rounding mode
    
    _mm_store_ss( &v[0], vf0 );
    _mm_store_ss( &v[1], vf1 );
    _mm_store_ss( &v[2], vf2 );
#else
    v[0] = round( v[0] );
    v[1] = round( v[1] );
    v[2] = round( v[2] );
#endif
}

/*
=================
main
=================
*/
#if defined (DEDICATED)
sint main( sint argc, valueType** argv )
#elif defined (__LINUX__)
extern "C" sint engineMain( sint argc, valueType * *argv )
#else
Q_EXPORT sint engineMain( sint argc, valueType * *argv )
#endif
{
    sint i;
    valueType commandLine[ MAX_STRING_CHARS ] = { 0 };
    
#ifndef DEDICATED
    // SDL version check
    
    // Compile time
#	if !SDL_VERSION_ATLEAST(MINSDL_MAJOR,MINSDL_MINOR,MINSDL_PATCH)
#		error A more recent version of SDL is required
#	endif
    
    // Run time
    SDL_version ver;
    SDL_GetVersion( &ver );
    
#define MINSDL_VERSION \
	XSTRING(MINSDL_MAJOR) "." \
	XSTRING(MINSDL_MINOR) "." \
	XSTRING(MINSDL_PATCH)
    
    if( SDL_VERSIONNUM( ver.major, ver.minor, ver.patch ) <
            SDL_VERSIONNUM( MINSDL_MAJOR, MINSDL_MINOR, MINSDL_PATCH ) )
    {
        systemLocal.Dialog( DT_ERROR, va( "SDL version " MINSDL_VERSION " or greater is required, "
                                          "but only version %d.%d.%d was found. You may be able to obtain a more recent copy "
                                          "from http://www.libsdl.org/.", ver.major, ver.minor, ver.patch ), "SDL Library Too Old" );
                                          
        systemLocal.Exit( 1 );
    }
#endif
    
    idSystemLocal::PlatformInit( );
    
    // Set the initial time base
    systemLocal.Milliseconds( );
    
    idSystemLocal::ParseArgs( argc, argv );
    
    idSystemLocal::SetBinaryPath( idSystemLocal::Dirname( argv[ 0 ] ) );
    idSystemLocal::SetDefaultInstallPath( DEFAULT_BASEDIR );
    idSystemLocal::SetDefaultLibPath( DEFAULT_BASEDIR );
    
    // Concatenate the command line for passing to Com_Init
    for( i = 1; i < argc; i++ )
    {
        const bool containsSpaces = ( bool )( strchr( argv[i], ' ' ) != nullptr );
        
        if( !::strcmp( argv[ i ], "+nocurses" ) )
        {
            continue;
        }
        
        if( !::strcmp( argv[ i ], "+showconsole" ) )
        {
            continue;
        }
        
        if( containsSpaces )
        {
            Q_strcat( commandLine, sizeof( commandLine ), "\"" );
        }
        
        Q_strcat( commandLine, sizeof( commandLine ), argv[ i ] );
    }
    
    consoleCursesSystem->Init();
    
    Com_Init( commandLine );
    NET_Init( );
    
    signal( SIGILL, systemLocal.SigHandler );
    signal( SIGFPE, systemLocal.SigHandler );
    signal( SIGSEGV, systemLocal.SigHandler );
    signal( SIGTERM, systemLocal.SigHandler );
    signal( SIGINT, systemLocal.SigHandler );
    
    while( 1 )
    {
        systemLocal.Frame( );
        Com_Frame( );
    }
    
    return 0;
}

