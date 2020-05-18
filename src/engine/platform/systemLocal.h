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
// File name:   systemLocal.h
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef __LINUX__
static bool stdinIsATTY;
// Used to determine where to store user-specific files
static UTF8 exit_cmdline[MAX_CMD] = "";
/* base time in seconds, that's our origin
timeval:tv_sec is an S32:
assuming this wraps every 0x7fffffff - ~68 years since the Epoch (1970) - we're safe till 2038
using unsigned long data type to work right with Sys_XTimeToSysTime */

static time_t initial_tv_sec = 0;
#endif

static UTF8 homePath[MAX_OSPATH] = { 0 };
static S32 sys_timeBase;
#define MAX_FOUND_FILES 0x1000
#define MEM_THRESHOLD 96*1024*1024
static UTF8 binaryPath[MAX_OSPATH] = { 0 };
static UTF8 installPath[MAX_OSPATH] = { 0 };
static UTF8 libPath[MAX_OSPATH] = { 0 };

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

typedef enum
{
    QUAKE_KEY,
    CHARACTER
} keyType_t;

// We translate axes movement into keypresses
static S32 joy_keys[16] =
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
static S32 hat_keys[16] =
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
    U32 oldaxes;
    S32 oldaaxes[MAX_JOYSTICK_AXIS];
    U32 oldhats;
} stick_state;

//
// idSystemLocal
//
class idSystemLocal : public idSystem
{
public:
    idSystemLocal();
    ~idSystemLocal();
    
    virtual UTF8* DefaultHomePath( UTF8* buffer, S32 size );
    virtual S32 Milliseconds( void );
    virtual bool RandomBytes( U8* string, S32 len );
    virtual UTF8* GetCurrentUser( void );
    virtual bool LowPhysicalMemory( void );
    virtual bool Mkdir( StringEntry path );
    virtual UTF8** ListFiles( StringEntry directory, StringEntry extension, UTF8* filter, S32* numfiles, bool wantsubs );
    virtual void FreeFileList( UTF8** list );
    virtual void Sleep( S32 msec );
    virtual bool OpenUrl( StringEntry url );
    virtual dialogResult_t Dialog( dialogType_t type, StringEntry message, StringEntry title );
    virtual void GLimpSafeInit( void );
    virtual void GLimpInit( void );
    virtual void StartProcess( UTF8* exeName, bool doexit );
    virtual void OpenURL( StringEntry url, bool doexit );
    virtual bool IsNumLockDown( void );
    virtual void Chmod( UTF8* file, S32 mode );
    virtual UTF8* SysGetClipboardData( void );
    virtual UTF8* DefaultInstallPath( void );
    virtual UTF8* DefaultLibPath( void );
    virtual UTF8* DefaultAppPath( void );
    virtual void Restart_f( void );
    virtual UTF8* ConsoleInput( void );
    virtual bool WritePIDFile( void );
    virtual void Quit( void );
    virtual void Init( void );
    virtual void WriteDump( StringEntry fmt, ... );
    virtual void Print( StringEntry msg );
    virtual void Error( StringEntry error, ... );
    virtual void UnloadDll( void* dllHandle );
    virtual UTF8* GetDLLName( StringEntry name );
    virtual void* LoadDll( StringEntry name );
    virtual void* GetProcAddress( void* dllhandle, StringEntry name );
    virtual void SysSnapVector( F32* v );
    virtual void Init( void* windowData );
    virtual void Shutdown( void );
    
    static void SetBinaryPath( StringEntry path );
    static void SetFloatEnv( void );
    static StringEntry TempPath( void );
    static StringEntry Basename( UTF8* path );
    static StringEntry Dirname( UTF8* path );
    static UTF8* Cwd( void );
    static void ListFilteredFiles( StringEntry basedir, UTF8* subdirs, UTF8* filter, UTF8** list, S32* numfiles );
    static void ErrorDialog( StringEntry error );
    static void resetTime( void );
    static void PlatformInit( void );
    static void SetEnv( StringEntry name, StringEntry value );
    static S32 PID( void );
    static bool PIDIsRunning( S32 pid );
    static void DoStartProcess( UTF8* cmdline );
    static bool Fork( StringEntry path, StringEntry cmdLine );
    static UTF8* BinaryPath( void );
    static void SetDefaultInstallPath( StringEntry path );
    static void SetDefaultLibPath( StringEntry path );
    static UTF8* PIDFileName( void );
    static void Exit( S32 exitCode );
    static void RecordError( StringEntry msg );
    static void ParseArgs( S32 argc, UTF8** argv );
    static StringEntry SignalToString( S32 sig );
    static void SigHandler( S32 signal );
    static F32 roundfloat( F32 n );
    static void InitJoystick( void );
    static void ShutdownJoystick( void );
    static void JoyMove( S32 eventTime );
    static void ProcessEvents( S32 eventTime );
    static void Frame( void );
    static void InitKeyLockStates( void );
    static void Restart( void );
    static void PrintKey( const SDL_Keysym* keysym, keyNum_t key, bool down );
    static bool IsConsoleKey( keyNum_t key, S32 character );
    static keyNum_t TranslateSDLToQ3Key( SDL_Keysym* keysym, bool down );
    static void GobbleMotionEvents( void );
    static void ActivateMouse( void );
    static void DeactivateMouse( void );
    static S32 CtrlHandler( U64 sig );
    static bool strgtr( StringEntry s0, StringEntry s1 );
    static S32 ZenityCommand( dialogType_t type, StringEntry message, StringEntry title );
    static S32 KdialogCommand( dialogType_t type, StringEntry message, StringEntry title );
    static S32 XmessageCommand( dialogType_t type, StringEntry message, StringEntry title );
};

extern idSystemLocal systemLocal;
