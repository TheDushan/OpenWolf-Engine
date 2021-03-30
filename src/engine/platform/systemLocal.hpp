////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   systemLocal.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef __LINUX__
static bool stdinIsATTY;
// Used to determine where to store user-specific files
static valueType exit_cmdline[MAX_CMD] = "";
/* base time in seconds, that's our origin
timeval:tv_sec is an sint:
assuming this wraps every 0x7fffffff - ~68 years since the Epoch (1970) - we're safe till 2038
using unsigned long data type to work right with Sys_XTimeToSysTime */

static time_t initial_tv_sec = 0;
#endif

static jmp_buf sys_exitframe;
static sint sys_retcode;
static valueType homePath[MAX_OSPATH] = { 0 };
static sint sys_timeBase;
#define MAX_FOUND_FILES 0x1000
#define MEM_THRESHOLD 96*1024*1024
static valueType binaryPath[MAX_OSPATH] = { 0 };
static valueType installPath[MAX_OSPATH] = { 0 };
static valueType libPath[MAX_OSPATH] = { 0 };

#ifdef _WIN32
#define FPUCWMASK1 (_MCW_RC|_MCW_EM)
#define FPUCW (_RC_NEAR|_MCW_EM|_PC_53)
#define FPUCWMASK	(FPUCWMASK1)
#ifndef DEDICATED
static bool SDL_VIDEODRIVER_externallySet = false;
#endif

typedef bool( __stdcall* SteamAPIInit_Type )();
typedef void( __stdcall* SteamAPIShutdown_Type )();
static SteamAPIInit_Type SteamAPI_Init;
static SteamAPIShutdown_Type SteamAPI_Shutdown;
static void* steamLibrary = nullptr;
#endif

#ifdef DEDICATED
#	define PID_FILENAME PRODUCT_NAME_UPPPER "_server.pid"
#else
#	define PID_FILENAME PRODUCT_NAME_UPPPER ".pid"
#endif

#ifndef DEFAULT_BASEDIR
#	ifdef MACOS_X
#		define DEFAULT_BASEDIR idSystemLocal::StripAppBundle(idSystemLocal::BinaryPath())
#	else
#		define DEFAULT_BASEDIR idSystemLocal::BinaryPath()
#	endif
#endif

// Require a minimum version of SDL
#define MINSDL_MAJOR 2
#define MINSDL_MINOR 0
#define MINSDL_PATCH 8

#define MAX_CONSOLE_KEYS 16

enum keyType_t
{
    QUAKE_KEY,
    CHARACTER
};

// We translate axes movement into keypresses
static sint joy_keys[16] =
{
    K_LEFTARROW, K_RIGHTARROW,
    K_UPARROW, K_DOWNARROW,
    K_JOY17, K_JOY18,
    K_JOY19, K_JOY20,
    K_JOY21, K_JOY22,
    K_JOY23, K_JOY24,
    K_JOY25, K_JOY26,
    K_JOY27, K_JOY28
};

// translate hat events into keypresses
// the 4 highest buttons are used for the first hat ...
static sint hat_keys[16] =
{
    K_JOY29, K_JOY30,
    K_JOY31, K_JOY32,
    K_JOY25, K_JOY26,
    K_JOY27, K_JOY28,
    K_JOY21, K_JOY22,
    K_JOY23, K_JOY24,
    K_JOY17, K_JOY18,
    K_JOY19, K_JOY20
};


static struct
{
    bool buttons[16];  // !!! FIXME: these might be too many.
    uint oldaxes;
    sint oldaaxes[MAX_JOYSTICK_AXIS];
    uint oldhats;
} stick_state;

//
// idSystemLocal
//
class idSystemLocal : public idSystem
{
public:
    idSystemLocal();
    ~idSystemLocal();
    
    virtual valueType* DefaultHomePath( valueType* buffer, sint size );
    virtual sint Milliseconds( void );
    virtual bool RandomBytes( uchar8* string, uint64 len );
    virtual valueType* GetCurrentUser( void );
    virtual bool LowPhysicalMemory( void );
    virtual bool Mkdir( pointer path );
    virtual valueType** ListFiles( pointer directory, pointer extension, valueType* filter, sint* numfiles, bool wantsubs );
    virtual void FreeFileList( valueType** list );
    virtual void Sleep( sint msec );
    virtual bool OpenUrl( pointer url );
    virtual dialogResult_t Dialog( dialogType_t type, pointer message, pointer title );
    virtual void GLimpSafeInit( void );
    virtual void GLimpInit( void );
    virtual void StartProcess( valueType* exeName, bool doexit );
    virtual void OpenURL( pointer url, bool doexit );
    virtual bool IsNumLockDown( void );
    virtual void Chmod( valueType* file, sint mode );
    virtual valueType* SysGetClipboardData( void );
    virtual valueType* DefaultInstallPath( void );
    virtual valueType* DefaultLibPath( void );
    virtual valueType* DefaultAppPath( void );
    virtual void Restart_f( void );
    virtual valueType* ConsoleInput( void );
    virtual bool WritePIDFile( void );
    virtual void Quit( void );
    virtual void Init( void );
    virtual void WriteDump( pointer fmt, ... );
    virtual void Print( pointer msg );
    virtual void Error( pointer error, ... );
    virtual void UnloadDll( void* dllHandle );
    virtual valueType* GetDLLName( pointer name );
    virtual void* LoadDll( pointer name );
    virtual void* GetProcAddress( void* dllhandle, pointer name );
    virtual void SysSnapVector( float32* v );
    virtual void Init( void* windowData );
    virtual void Shutdown( void );
    virtual valueType* Cwd( void );
    
    static void SetBinaryPath( pointer path );
    static void SetFloatEnv( void );
    static pointer TempPath( void );
    static pointer Basename( valueType* path );
    static pointer Dirname( valueType* path );
    static void ListFilteredFiles( pointer basedir, valueType* subdirs, valueType* filter, valueType** list, sint* numfiles );
    static void ErrorDialog( pointer error );
    static void resetTime( void );
    static void PlatformInit( void );
    static void SetEnv( pointer name, pointer value );
    static sint PID( void );
    static bool PIDIsRunning( sint pid );
    static void DoStartProcess( valueType* cmdline );
    static bool Fork( pointer path, pointer cmdLine );
    static valueType* BinaryPath( void );
    static void SetDefaultInstallPath( pointer path );
    static void SetDefaultLibPath( pointer path );
    static valueType* PIDFileName( void );
    static void Exit( sint exitCode );
    static void RecordError( pointer msg );
    static void ParseArgs( sint argc, valueType** argv );
    static pointer SignalToString( sint sig );
    static void SigHandler( sint signal );
    static void InitJoystick( void );
    static void ShutdownJoystick( void );
    static void JoyMove( sint eventTime );
    static void ProcessEvents( sint eventTime );
    static void Frame( void );
    static void InitKeyLockStates( void );
    static void Restart( void );
    static void PrintKey( const SDL_Keysym* keysym, keyNum_t key, bool down );
    static bool IsConsoleKey( keyNum_t key, sint character );
    static keyNum_t TranslateSDLToQ3Key( SDL_Keysym* keysym, bool down );
    static void GobbleMotionEvents( void );
    static void ActivateMouse( void );
    static void DeactivateMouse( void );
    static sint CtrlHandler( uint32 sig );
    static bool strgtr( pointer s0, pointer s1 );
    static sint ZenityCommand( dialogType_t type, pointer message, pointer title );
    static sint KdialogCommand( dialogType_t type, pointer message, pointer title );
    static sint XmessageCommand( dialogType_t type, pointer message, pointer title );
};

extern idSystemLocal systemLocal;
