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
// File name:   common.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: misc functions used in client and server
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.h>
#elif DEDICATED
#include <null/null_serverprecompiled.h>
#else
#include <framework/precompiled.h>
#endif

S32 demo_protocols[] = { 1, 0 };

#define MAX_NUM_ARGVS   50

#define MIN_DEDICATED_COMHUNKMEGS 64
#define MIN_COMHUNKMEGS 256				// JPW NERVE changed this to 42 for MP, was 56 for team arena and 75 for wolfSP
#define DEF_COMHUNKMEGS 512				// RF, increased this, some maps are exceeding 56mb 
// JPW NERVE changed this for multiplayer back to 42, 56 for depot/mp_cpdepot, 42 for everything else
#define DEF_COMZONEMEGS 128				// RF, increased this from 16, to account for botlib/AAS
#define DEF_COMHUNKMEGS_S	XSTRING(DEF_COMHUNKMEGS)
#define DEF_COMZONEMEGS_S	XSTRING(DEF_COMZONEMEGS)

S32             com_argc;
UTF8*           com_argv[MAX_NUM_ARGVS + 1];

jmp_buf         abortframe;		// an ERR_DROP occured, exit the entire frame

static fileHandle_t logfile_;
fileHandle_t    com_journalFile;	// events are written here
fileHandle_t    com_journalDataFile;	// config files are written here

convar_t*         com_crashed = NULL;	// ydnar: set in case of a crash, prevents CVAR_UNSAFE variables from being set from a cfg

//bani - explicit NULL to make win32 teh happy

convar_t*         com_ignorecrash = NULL;	// bani - let experienced users ignore crashes, explicit NULL to make win32 teh happy
convar_t*         com_pid;		// bani - process id

convar_t*         com_viewlog;
convar_t*         com_speeds;
convar_t*         com_developer;
convar_t*         com_dedicated;
convar_t*         com_timescale;
convar_t*         com_fixedtime;
convar_t*         com_dropsim;	// 0.0 to 1.0, simulated packet drops
convar_t*         com_journal;
convar_t*         com_maxfps;
convar_t*         com_timedemo;
convar_t*         com_sv_running;
convar_t*         com_cl_running;
convar_t*         com_showtrace;
convar_t*         com_version;
convar_t* com_logfile;		// 1 = buffer log, 2 = flush after each print
//convar_t    *com_blood;
convar_t*         com_buildScript;	// for automated data building scripts
convar_t*         con_drawnotify;
convar_t*         com_ansiColor;

convar_t*         com_unfocused;
convar_t*         com_minimized;

convar_t* com_affinity;

convar_t*         com_introPlayed;
convar_t*         com_logosPlaying;
convar_t*         cl_paused;
convar_t*         sv_paused;
#if defined (DEDICATED)
convar_t*	   	cl_packetdelay;
#endif
//convar_t		   *sv_packetdelay;
convar_t*         com_cameraMode;
convar_t*         com_maxfpsUnfocused;
convar_t*         com_maxfpsMinimized;
convar_t*         com_abnormalExit;

#if defined( _WIN32 ) && defined( _DEBUG )
convar_t*         com_noErrorInterrupt;
#endif
convar_t*         com_recommendedSet;

convar_t*         com_watchdog;
convar_t*         com_watchdog_cmd;

// Rafael Notebook
convar_t*         cl_notebook;

convar_t*         com_hunkused;	// Ridah
convar_t*         com_protocol;

// com_speeds times
S32             time_game;
S32             time_frontend;	// renderer frontend time
S32             time_backend;	// renderer backend time

S32             com_frameTime;
S32             com_frameMsec;
S32             com_frameNumber;
S32             com_expectedhunkusage;
S32             com_hunkusedvalue;

bool			com_errorEntered = false;
bool			com_fullyInitialized = false;

UTF8            com_errorMessage[MAXPRINTMSG];
void Com_WriteConfiguration( void );
void            Com_WriteConfig_f( void );
void            CIN_CloseAllVideos();

//============================================================================

static UTF8*    rd_buffer;
static S32      rd_buffersize;
static bool rd_flushing = false;
static void ( *rd_flush )( UTF8* buffer );

void Com_BeginRedirect( UTF8* buffer, S32 buffersize, void ( *flush )( UTF8* ) )
{
    if( !buffer || !buffersize || !flush )
    {
        return;
    }
    rd_buffer = buffer;
    rd_buffersize = buffersize;
    rd_flush = flush;
    
    *rd_buffer = 0;
}

void Com_EndRedirect( void )
{
    if( rd_flush )
    {
        rd_flushing = true;
        rd_flush( rd_buffer );
        rd_flushing = false;
    }
    
    rd_buffer = NULL;
    rd_buffersize = 0;
    rd_flush = NULL;
}

/*
=============
Com_Printf

Both client and server can use this, and it will output
to the apropriate place.

A raw string should NEVER be passed as fmt, because of "%f" type crashers.
=============
*/
void Com_Printf( StringEntry fmt, ... )
{
    va_list	argptr;
    UTF8 msg[MAXPRINTMSG];
    static bool opening_qconsole = false;
    
    va_start( argptr, fmt );
    Q_vsnprintf( msg, sizeof( msg ) - 1, fmt, argptr );
    va_end( argptr );
    
    if( rd_buffer && !rd_flushing )
    {
        if( ( strlen( msg ) + strlen( rd_buffer ) ) > ( rd_buffersize - 1 ) )
        {
            rd_flushing = true;
            rd_flush( rd_buffer );
            rd_flushing = false;
            *rd_buffer = 0;
        }
        Q_strcat( rd_buffer, rd_buffersize, msg );
        // show_bug.cgi?id=51
        // only flush the rcon buffer when it's necessary, avoid fragmenting
        //rd_flush(rd_buffer);
        //*rd_buffer = 0;
        
        return;
    }
    
    // echo to console if we're not a dedicated server
    if( com_dedicated && !com_dedicated->integer )
    {
        CL_ConsolePrint( msg );
    }
    
    // echo to dedicated console and early console
    idsystem->Print( msg );
    
    // logfile
    if( com_logfile && com_logfile->integer )
    {
        // TTimo: only open the qconsole.log if the filesystem is in an initialized state
        //   also, avoid recursing in the qconsole.log opening (i.e. if fs_debug is on)
        if( !logfile_ && fileSystem->Initialized() && !opening_qconsole )
        {
            struct tm* newtime;
            time_t aclock;
            
            opening_qconsole = true;
            
            time( &aclock );
            newtime = localtime( &aclock );
            
            if( com_logfile->integer != 3 )
            {
                logfile_ = fileSystem->FOpenFileWrite( "owconsole.log" );
            }
            else
            {
                logfile_ = fileSystem->FOpenFileAppend( "owconsole.log" );
            }
            
            if( logfile_ )
            {
                Com_Printf( "logfile opened on %s\n", asctime( newtime ) );
                if( com_logfile->integer > 1 )
                {
                    // force it to not buffer so we get valid
                    // data even if we are crashing
                    fileSystem->ForceFlush( logfile_ );
                }
            }
            else
            {
                Com_Printf( "Opening qconsole.log failed!\n" );
                cvarSystem->SetValue( "logfile", 0 );
            }
        }
        opening_qconsole = false;
        if( logfile_ && fileSystem->Initialized() )
        {
            fileSystem->Write( msg, strlen( msg ), logfile_ );
        }
    }
}

void Com_FatalError( StringEntry error, ... )
{
    va_list argptr;
    UTF8 msg[8192];
    
    va_start( argptr, error );
    Q_vsnprintf( msg, sizeof( msg ), error, argptr );
    va_end( argptr );
    
    Com_Error( ERR_FATAL, msg );
}

void Com_DropError( StringEntry error, ... )
{
    va_list argptr;
    UTF8 msg[8192];
    
    va_start( argptr, error );
    Q_vsnprintf( msg, sizeof( msg ), error, argptr );
    va_end( argptr );
    
    Com_Error( ERR_DROP, msg );
}

void Com_Warning( StringEntry error, ... )
{
    va_list argptr;
    UTF8 msg[8192];
    
    va_start( argptr, error );
    Q_vsnprintf( msg, sizeof( msg ), error, argptr );
    va_end( argptr );
    
    Com_Printf( msg );
}



/*
================
Com_DPrintf

A Com_Printf that only shows up if the "developer" cvar is set
================
*/
void Com_DPrintf( StringEntry fmt, ... )
{
    va_list         argptr;
    UTF8            msg[MAXPRINTMSG];
    
    if( !com_developer || com_developer->integer != 1 )
    {
        return;					// don't confuse non-developers with techie stuff...
    }
    
    va_start( argptr, fmt );
    Q_vsnprintf( msg, sizeof( msg ), fmt, argptr );
    va_end( argptr );
    
    Com_Printf( "%s", msg );
}

/*
=============
Com_Error

Both client and server can use this, and it will
do the apropriate things.
=============
*/
// *INDENT-OFF*
void Com_Error( S32 code, StringEntry fmt, ... )
{
    va_list         argptr;
    static S32      lastErrorTime;
    static S32      errorCount;
    S32             currentTime;
    static bool calledSysError = false;
    
    if( com_errorEntered )
    {
        if( !calledSysError )
        {
            calledSysError = true;
            idsystem->Error( "recursive error after: %s", com_errorMessage );
        }
        return;
    }
    
    com_errorEntered = true;
    
    cvarSystem->Set( "com_errorCode", va( "%i", code ) );
    
    // when we are running automated scripts, make sure we
    // know if anything failed
    if( com_buildScript && com_buildScript->integer )
    {
        code = ERR_FATAL;
    }
    
    // make sure we can get at our local stuff
    fileSystem->PureServerSetLoadedPaks( "", "" );
    
    // if we are getting a solid stream of ERR_DROP, do an ERR_FATAL
    currentTime = idsystem->Milliseconds();
    if( currentTime - lastErrorTime < 100 )
    {
        if( ++errorCount > 3 )
        {
            code = ERR_FATAL;
        }
    }
    else
    {
        errorCount = 0;
    }
    lastErrorTime = currentTime;
    
    va_start( argptr, fmt );
    Q_vsnprintf( com_errorMessage, sizeof( com_errorMessage ), fmt, argptr );
    va_end( argptr );
    
    switch( code )
    {
        case ERR_FATAL:
        case ERR_DROP:
            idsystem->WriteDump( "Debug Dump\nCom_Error: %s", com_errorMessage );
            break;
    }
    
    if( code == ERR_SERVERDISCONNECT )
    {
        CL_Disconnect( true );
        CL_FlushMemory();
        com_errorEntered = false;
        longjmp( abortframe, -1 );
    }
    else if( code == ERR_DROP || code == ERR_DISCONNECT )
    {
        Com_Printf( "********************\nERROR: %s\n********************\n", com_errorMessage );
        serverInitSystem->Shutdown( va( "Server crashed: %s\n", com_errorMessage ) );
        CL_Disconnect( true );
        CL_FlushMemory();
        com_errorEntered = false;
        longjmp( abortframe, -1 );
    }
#ifndef DEDICATED
    else if( code == ERR_AUTOUPDATE )
    {
        CL_Disconnect( true );
        CL_FlushMemory();
        com_errorEntered = false;
        if( !Q_stricmpn( com_errorMessage, "Server is full", 14 ) && CL_NextUpdateServer() )
        {
            CL_GetAutoUpdate();
        }
        else
        {
            longjmp( abortframe, -1 );
        }
    }
#endif
    else
    {
        serverInitSystem->Shutdown( va( "Server fatal crashed: %s\n", com_errorMessage ) );
        CL_Shutdown();
    }
    
    Com_Shutdown( code == ERR_VID_FATAL ? true : false );
    
    calledSysError = true;
    idsystem->Error( "%s", com_errorMessage );
}

/*
=============
Com_Quit_f

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void Com_Quit_f( void )
{
    // don't try to shutdown if we are in a recursive error
    if( !com_errorEntered )
    {
        // Some VMs might execute "quit" command directly,
        // which would trigger an unload of active VM error.
        // idSystemLocal::Quit will kill this process anyways, so
        // a corrupt call stack makes no difference
        serverInitSystem->Shutdown( "Server quit\n" );
//bani
#ifndef DEDICATED
        clientGameSystem->ShutdownCGame();
#endif
        CL_Shutdown();
        Com_Shutdown( false );
        fileSystem->Shutdown( true );
        cmdSystem->Shutdown();
        cvarSystem->Shutdown();
    }
    idsystem->Quit();
}

/*
============================================================================

COMMAND LINE FUNCTIONS

+ characters seperate the commandLine string into multiple console
command lines.

All of these are valid:

quake3 +set test blah +map test
quake3 set test blah+map test
quake3 set test blah + map test

============================================================================
*/

#define MAX_CONSOLE_LINES   32
S32             com_numConsoleLines;
UTF8*           com_consoleLines[MAX_CONSOLE_LINES];

/*
==================
Com_ParseCommandLine

Break it up into multiple console lines
==================
*/
void Com_ParseCommandLine( UTF8* commandLine )
{
    com_consoleLines[0] = commandLine;
    com_numConsoleLines = 1;
    
    while( *commandLine )
    {
        // look for a + seperating character
        // if commandLine came from a file, we might have real line seperators
        if( *commandLine == '+' || *commandLine == '\n' )
        {
            if( com_numConsoleLines == MAX_CONSOLE_LINES )
            {
                return;
            }
            com_consoleLines[com_numConsoleLines] = commandLine + 1;
            com_numConsoleLines++;
            *commandLine = 0;
        }
        commandLine++;
    }
}


/*
===================
Com_SafeMode

Check for "safe" on the command line, which will
skip loading of wolfconfig.cfg
===================
*/
bool Com_SafeMode( void )
{
    S32             i;
    
    for( i = 0; i < com_numConsoleLines; i++ )
    {
        cmdSystem->TokenizeString( com_consoleLines[i] );
        if( !Q_stricmp( cmdSystem->Argv( 0 ), "safe" ) || !Q_stricmp( cmdSystem->Argv( 0 ), "cvar_restart" ) )
        {
            com_consoleLines[i][0] = 0;
            return true;
        }
    }
    return false;
}


/*
===============
Com_StartupVariable

Searches for command line parameters that are set commands.
If match is not NULL, only that cvar will be looked for.
That is necessary because cddir and basedir need to be set
before the filesystem is started, but all other sets shouls
be after execing the config and default.
===============
*/
void Com_StartupVariable( StringEntry match )
{
    S32             i;
    UTF8*           s;
    
    for( i = 0; i < com_numConsoleLines; i++ )
    {
        cmdSystem->TokenizeString( com_consoleLines[i] );
        
        if( ::strcmp( cmdSystem->Argv( 0 ), "set" ) )
        {
            continue;
        }
        
        s = cmdSystem->Argv( 1 );
        
        if( !match || !strcmp( s, match ) )
        {
            if( cvarSystem->Flags( s ) == CVAR_NONEXISTENT )
            {
                cvarSystem->Get( s, cmdSystem->Argv( 2 ), CVAR_USER_CREATED, nullptr );
            }
            else
            {
                cvarSystem->Set( s, cmdSystem->Argv( 2 ) );
            }
        }
    }
}


/*
=================
Com_AddStartupCommands

Adds command line parameters as script statements
Commands are seperated by + signs

Returns true if any late commands were added, which
will keep the demoloop from immediately starting
=================
*/
bool Com_AddStartupCommands( void )
{
    S32             i;
    bool        added;
    
    added = false;
    // quote every token, so args with semicolons can work
    for( i = 0; i < com_numConsoleLines; i++ )
    {
        if( !com_consoleLines[i] || !com_consoleLines[i][0] )
        {
            continue;
        }
        
        // set commands won't override menu startup
        if( Q_stricmpn( com_consoleLines[i], "set", 3 ) )
        {
            added = true;
        }
        cmdBufferSystem->AddText( com_consoleLines[i] );
        cmdBufferSystem->AddText( "\n" );
    }
    
    return added;
}


//============================================================================

void Info_Print( StringEntry s )
{
    UTF8            key[BIG_INFO_VALUE];
    UTF8            value[BIG_INFO_VALUE];
    UTF8*           o;
    S32             l;
    
    if( *s == '\\' )
    {
        s++;
    }
    while( *s )
    {
        o = key;
        while( *s && *s != '\\' )
            *o++ = *s++;
            
        l = o - key;
        if( l < 20 )
        {
            memset( o, ' ', 20 - l );
            key[20] = 0;
        }
        else
        {
            *o = 0;
        }
        Com_Printf( "%s ", key );
        
        if( !*s )
        {
            Com_Printf( "MISSING VALUE\n" );
            return;
        }
        
        o = value;
        s++;
        while( *s && *s != '\\' )
            *o++ = *s++;
        *o = 0;
        
        if( *s )
        {
            s++;
        }
        Com_Printf( "%s\n", value );
    }
}

/*
============
Com_Filter
============
*/
S32 Com_Filter( UTF8* filter, UTF8* name, S32 casesensitive )
{
    UTF8            buf[MAX_TOKEN_CHARS];
    UTF8*           ptr;
    S32             i, found;
    
    while( *filter )
    {
        if( *filter == '*' )
        {
            filter++;
            for( i = 0; *filter; i++ )
            {
                if( *filter == '*' || *filter == '?' )
                {
                    break;
                }
                buf[i] = *filter;
                filter++;
            }
            buf[i] = '\0';
            if( strlen( buf ) )
            {
                ptr = Com_StringContains( name, buf, casesensitive );
                if( !ptr )
                {
                    return false;
                }
                name = ptr + strlen( buf );
            }
        }
        else if( *filter == '?' )
        {
            filter++;
            name++;
        }
        else if( *filter == '[' && *( filter + 1 ) == '[' )
        {
            filter++;
        }
        else if( *filter == '[' )
        {
            filter++;
            found = false;
            while( *filter && !found )
            {
                if( *filter == ']' && *( filter + 1 ) != ']' )
                {
                    break;
                }
                if( *( filter + 1 ) == '-' && *( filter + 2 ) && ( *( filter + 2 ) != ']' || *( filter + 3 ) == ']' ) )
                {
                    if( casesensitive )
                    {
                        if( *name >= *filter && *name <= *( filter + 2 ) )
                        {
                            found = true;
                        }
                    }
                    else
                    {
                        if( toupper( *name ) >= toupper( *filter ) && toupper( *name ) <= toupper( *( filter + 2 ) ) )
                        {
                            found = true;
                        }
                    }
                    filter += 3;
                }
                else
                {
                    if( casesensitive )
                    {
                        if( *filter == *name )
                        {
                            found = true;
                        }
                    }
                    else
                    {
                        if( toupper( *filter ) == toupper( *name ) )
                        {
                            found = true;
                        }
                    }
                    filter++;
                }
            }
            if( !found )
            {
                return false;
            }
            while( *filter )
            {
                if( *filter == ']' && *( filter + 1 ) != ']' )
                {
                    break;
                }
                filter++;
            }
            filter++;
            name++;
        }
        else
        {
            if( casesensitive )
            {
                if( *filter != *name )
                {
                    return false;
                }
            }
            else
            {
                if( toupper( *filter ) != toupper( *name ) )
                {
                    return false;
                }
            }
            filter++;
            name++;
        }
    }
    return true;
}

/*
============
Com_FilterPath
============
*/
S32 Com_FilterPath( UTF8* filter, UTF8* name, S32 casesensitive )
{
    S32             i;
    UTF8            new_filter[MAX_QPATH];
    UTF8            new_name[MAX_QPATH];
    
    for( i = 0; i < MAX_QPATH - 1 && filter[i]; i++ )
    {
        if( filter[i] == '\\' || filter[i] == ':' )
        {
            new_filter[i] = '/';
        }
        else
        {
            new_filter[i] = filter[i];
        }
    }
    new_filter[i] = '\0';
    for( i = 0; i < MAX_QPATH - 1 && name[i]; i++ )
    {
        if( name[i] == '\\' || name[i] == ':' )
        {
            new_name[i] = '/';
        }
        else
        {
            new_name[i] = name[i];
        }
    }
    new_name[i] = '\0';
    return Com_Filter( new_filter, new_name, casesensitive );
}



/*
================
Com_RealTime
================
*/
S32 Com_RealTime( qtime_t* qtime )
{
    time_t          t;
    struct tm*      tms;
    
    t = time( NULL );
    if( !qtime )
    {
        return t;
    }
    tms = localtime( &t );
    if( tms )
    {
        qtime->tm_sec = tms->tm_sec;
        qtime->tm_min = tms->tm_min;
        qtime->tm_hour = tms->tm_hour;
        qtime->tm_mday = tms->tm_mday;
        qtime->tm_mon = tms->tm_mon;
        qtime->tm_year = tms->tm_year;
        qtime->tm_wday = tms->tm_wday;
        qtime->tm_yday = tms->tm_yday;
        qtime->tm_isdst = tms->tm_isdst;
    }
    return t;
}


/*
==============================================================================

						ZONE MEMORY ALLOCATION

There is never any space between memblocks, and there will never be two
contiguous free memblocks.

The rover can be left pointing at a non-empty block

The zone calls are pretty much only used for small strings and structures,
all big things are allocated on the hunk.
==============================================================================
*/

#define ZONEID  0x1d4a11
#define MINFRAGMENT 64

typedef struct zonedebug_s
{
    UTF8*           label;
    UTF8*           file;
    S32             line;
    S32             allocSize;
} zonedebug_t;

typedef struct memblock_s
{
    S32             size;		// including the header and possibly tiny fragments
    S32             tag;		// a tag of 0 is a free block
    struct memblock_s* next, *prev;
    S32             id;			// should be ZONEID
#ifdef ZONE_DEBUG
    zonedebug_t     d;
#endif
} memblock_t;

typedef struct
{
    S32             size;		// total bytes malloced, including header
    S32             used;		// total bytes used
    memblock_t      blocklist;	// start / end cap for linked list
    memblock_t*     rover;
} memzone_t;

// main zone for all "dynamic" memory allocation
memzone_t*      mainzone;

// we also have a small zone for small allocations that would only
// fragment the main zone (think of cvar and cmd strings)
memzone_t*      smallzone;

void            Z_CheckHeap( void );

/*
========================
Z_ClearZone
========================
*/
void Z_ClearZone( memzone_t* zone, S32 size )
{
    memblock_t*     block;
    
    // set the entire zone to one free block
    
    zone->blocklist.next = zone->blocklist.prev = block = ( memblock_t* )( ( U8* ) zone + sizeof( memzone_t ) );
    zone->blocklist.tag = 1;	// in use block
    zone->blocklist.id = 0;
    zone->blocklist.size = 0;
    zone->rover = block;
    zone->size = size;
    zone->used = 0;
    
    block->prev = block->next = &zone->blocklist;
    block->tag = 0;				// free block
    block->id = ZONEID;
    block->size = size - sizeof( memzone_t );
}

/*
========================
Z_Free
========================
*/
void Z_Free( void* ptr )
{
    memblock_t*     block, *other;
    memzone_t*      zone;
    
    if( !ptr )
    {
        Com_Error( ERR_DROP, "Z_Free: NULL pointer" );
    }
    
    block = ( memblock_t* )( ( U8* ) ptr - sizeof( memblock_t ) );
    if( block->id != ZONEID )
    {
        Com_Error( ERR_FATAL, "Z_Free: freed a pointer without ZONEID" );
    }
    if( block->tag == 0 )
    {
        Com_Error( ERR_FATAL, "Z_Free: freed a freed pointer" );
    }
    
    // if static memory
    if( block->tag == TAG_STATIC )
    {
        return;
    }
    
    // check the memory trash tester
    if( *( S32* )( ( U8* ) block + block->size - 4 ) != ZONEID )
    {
        Com_Error( ERR_FATAL, "Z_Free: memory block wrote past end" );
    }
    
    if( block->tag == TAG_SMALL )
    {
        zone = smallzone;
    }
    else
    {
        zone = mainzone;
    }
    
    zone->used -= block->size;
    // set the block to something that should cause problems
    // if it is referenced...
    ::memset( ptr, 0xaa, block->size - sizeof( *block ) );
    
    block->tag = 0;				// mark as free
    
    other = block->prev;
    if( !other->tag )
    {
        // merge with previous free block
        other->size += block->size;
        other->next = block->next;
        other->next->prev = other;
        if( block == zone->rover )
        {
            zone->rover = other;
        }
        block = other;
    }
    
    zone->rover = block;
    
    other = block->next;
    if( !other->tag )
    {
        // merge the next free block onto the end
        block->size += other->size;
        block->next = other->next;
        block->next->prev = block;
        if( other == zone->rover )
        {
            zone->rover = block;
        }
    }
}


/*
================
Z_FreeTags
================
*/
void Z_FreeTags( memtag_t tag )
{
    S32             count;
    memzone_t*      zone;
    
    if( tag == TAG_SMALL )
    {
        zone = smallzone;
    }
    else
    {
        zone = mainzone;
    }
    count = 0;
    // use the rover as our pointer, because
    // Z_Free automatically adjusts it
    zone->rover = zone->blocklist.next;
    do
    {
        if( zone->rover->tag == tag )
        {
            count++;
            Z_Free( ( void* )( zone->rover + 1 ) );
            continue;
        }
        zone->rover = zone->rover->next;
    }
    while( zone->rover != &zone->blocklist );
}

/*
================
Z_TagMalloc
================
*/

memblock_t*     debugblock;		// RF, jusy so we can track a block to find out when it's getting trashed

#ifdef ZONE_DEBUG
void*           Z_TagMallocDebug( size_t size, memtag_t tag, UTF8* label, UTF8* file, S32 line )
{
#else
void*           Z_TagMalloc( size_t size, memtag_t tag )
{
#endif
    S32             extra, allocSize;
    memblock_t*     start, *rover, *_new, *base;
    memzone_t*      zone;
    
    if( !tag )
    {
        Com_Error( ERR_FATAL, "Z_TagMalloc: tried to use a 0 tag" );
    }
    
    if( tag == TAG_SMALL )
    {
        zone = smallzone;
    }
    else
    {
        zone = mainzone;
    }
    
    allocSize = size;
    //
    // scan through the block list looking for the first free block
    // of sufficient size
    //
    size += sizeof( memblock_t );	// account for size of block header
    size += 4;					// space for memory trash tester
    size = PAD( size, sizeof( intptr_t ) );	// align to 32/64 bit boundary
    
    base = rover = zone->rover;
    start = base->prev;
    
    
    do
    {
        if( rover == start )
        {
#ifdef ZONE_DEBUG
            Z_LogHeap();
#endif
            // scaned all the way around the list
            Com_Error( ERR_FATAL, "Z_Malloc: failed on allocation of %i bytes from the %s zone",
                       size, zone == smallzone ? "small" : "main" );
            return NULL;
        }
        if( rover->tag )
        {
            base = rover = rover->next;
        }
        else
        {
            rover = rover->next;
        }
    }
    while( base->tag || base->size < size );
    
    //
    // found a block big enough
    //
    extra = base->size - size;
    if( extra > MINFRAGMENT )
    {
        // there will be a free fragment after the allocated block
        _new = ( memblock_t* )( ( U8* ) base + size );
        _new->size = extra;
        _new->tag = 0;			// free block
        _new->prev = base;
        _new->id = ZONEID;
        _new->next = base->next;
        _new->next->prev = _new;
        base->next = _new;
        base->size = size;
    }
    
    base->tag = tag;			// no longer a free block
    
    zone->rover = base->next;	// next allocation will start looking here
    zone->used += base->size;	//
    
    base->id = ZONEID;
    
#ifdef ZONE_DEBUG
    base->d.label = label;
    base->d.file = file;
    base->d.line = line;
    base->d.allocSize = allocSize;
#endif
    
    // marker for memory trash testing
    *( S32* )( ( U8* ) base + base->size - 4 ) = ZONEID;
    
    return ( void* )( ( U8* ) base + sizeof( memblock_t ) );
}

/*
========================
Z_Malloc
========================
*/
#ifdef ZONE_DEBUG
void*           Z_MallocDebug( size_t size, UTF8* label, UTF8* file, S32 line )
{
#else
void*           Z_Malloc( size_t size )
{
#endif
    void*           buf;
    
    //Z_CheckHeap ();   // DEBUG
    
#ifdef ZONE_DEBUG
    buf = Z_TagMallocDebug( size, TAG_GENERAL, label, file, line );
#else
    buf = Z_TagMalloc( size, TAG_GENERAL );
#endif
    ::memset( buf, 0, size );
    
    return buf;
}

#ifdef ZONE_DEBUG
void*           S_MallocDebug( size_t size, UTF8* label, UTF8* file, S32 line )
{
    return Z_TagMallocDebug( size, TAG_SMALL, label, file, line );
}
#else
void*           S_Malloc( size_t size )
{
    return Z_TagMalloc( size, TAG_SMALL );
}
#endif

/*
========================
Z_CheckHeap
========================
*/
void Z_CheckHeap( void )
{
    memblock_t*     block;
    
    for( block = mainzone->blocklist.next;; block = block->next )
    {
        if( block->next == &mainzone->blocklist )
        {
            break;				// all blocks have been hit
        }
        if( ( U8* ) block + block->size != ( U8* ) block->next )
        {
            Com_Error( ERR_FATAL, "Z_CheckHeap: block size does not touch the next block\n" );
        }
        if( block->next->prev != block )
        {
            Com_Error( ERR_FATAL, "Z_CheckHeap: next block doesn't have proper back link\n" );
        }
        if( !block->tag && !block->next->tag )
        {
            Com_Error( ERR_FATAL, "Z_CheckHeap: two consecutive free blocks\n" );
        }
    }
}

/*
========================
Z_LogZoneHeap
========================
*/
void Z_LogZoneHeap( memzone_t* zone, UTF8* name )
{
#ifdef ZONE_DEBUG
    UTF8            dump[32], *ptr;
    S32             i, j;
#endif
    memblock_t*     block;
    UTF8            buf[4096];
    S32             size, allocSize, numBlocks;
    
    if( !logfile_ || !fileSystem->Initialized() )
    {
        return;
    }
    size = allocSize = numBlocks = 0;
    Com_sprintf( buf, sizeof( buf ), "\r\n================\r\n%s log\r\n================\r\n", name );
    fileSystem->Write( buf, strlen( buf ), logfile_ );
    for( block = zone->blocklist.next; block->next != &zone->blocklist; block = block->next )
    {
        if( block->tag )
        {
#ifdef ZONE_DEBUG
            ptr = ( ( UTF8* )block ) + sizeof( memblock_t );
            j = 0;
            for( i = 0; i < 20 && i < block->d.allocSize; i++ )
            {
                if( ptr[i] >= 32 && ptr[i] < 127 )
                {
                    dump[j++] = ptr[i];
                }
                else
                {
                    dump[j++] = '_';
                }
            }
            dump[j] = '\0';
            Com_sprintf( buf, sizeof( buf ), "size = %8d: %s, line: %d (%s) [%s]\r\n", block->d.allocSize, block->d.file,
                         block->d.line, block->d.label, dump );
            fileSystem->Write( buf, strlen( buf ), logfile_ );
            allocSize += block->d.allocSize;
#endif
            size += block->size;
            numBlocks++;
        }
    }
#ifdef ZONE_DEBUG
    // subtract debug memory
    size -= numBlocks * sizeof( zonedebug_t );
#else
    allocSize = numBlocks * sizeof( memblock_t );	// + 32 bit alignment
#endif
    Com_sprintf( buf, sizeof( buf ), "%d %s memory in %d blocks\r\n", size, name, numBlocks );
    fileSystem->Write( buf, strlen( buf ), logfile_ );
    Com_sprintf( buf, sizeof( buf ), "%d %s memory overhead\r\n", size - allocSize, name );
    fileSystem->Write( buf, strlen( buf ), logfile_ );
}

/*
========================
Z_AvailableZoneMemory
========================
*/
static S32 Z_AvailableZoneMemory( const memzone_t* zone )
{
#ifdef USE_MULTI_SEGMENT
    return zone->totalSize - zone->totalUsed;
#else
    return zone->size - zone->used;
#endif
}

/*
========================
Z_AvailableMemory
========================
*/
S32 Z_AvailableMemory( void )
{
    return Z_AvailableZoneMemory( mainzone );
}

/*
========================
Z_LogHeap
========================
*/
void Z_LogHeap( void )
{
    Z_LogZoneHeap( mainzone, "MAIN" );
    Z_LogZoneHeap( smallzone, "SMALL" );
}

// static mem blocks to reduce a lot of small zone overhead
typedef struct memstatic_s
{
    memblock_t      b;
    U8            mem[2];
} memstatic_t;

// bk001204 - initializer brackets
memstatic_t     emptystring = { {( sizeof( memblock_t ) + 2 + 3 )& ~3, TAG_STATIC, NULL, NULL, ZONEID}
    , {'\0', '\0'}
};
memstatic_t     numberstring[] =
{
    {   {( sizeof( memstatic_t ) + 3 )& ~3, TAG_STATIC, NULL, NULL, ZONEID}
        , {'0', '\0'}
    }
    ,
    {   {( sizeof( memstatic_t ) + 3 )& ~3, TAG_STATIC, NULL, NULL, ZONEID}
        , {'1', '\0'}
    }
    ,
    {   {( sizeof( memstatic_t ) + 3 )& ~3, TAG_STATIC, NULL, NULL, ZONEID}
        , {'2', '\0'}
    }
    ,
    {   {( sizeof( memstatic_t ) + 3 )& ~3, TAG_STATIC, NULL, NULL, ZONEID}
        , {'3', '\0'}
    }
    ,
    {   {( sizeof( memstatic_t ) + 3 )& ~3, TAG_STATIC, NULL, NULL, ZONEID}
        , {'4', '\0'}
    }
    ,
    {   {( sizeof( memstatic_t ) + 3 )& ~3, TAG_STATIC, NULL, NULL, ZONEID}
        , {'5', '\0'}
    }
    ,
    {   {( sizeof( memstatic_t ) + 3 )& ~3, TAG_STATIC, NULL, NULL, ZONEID}
        , {'6', '\0'}
    }
    ,
    {   {( sizeof( memstatic_t ) + 3 )& ~3, TAG_STATIC, NULL, NULL, ZONEID}
        , {'7', '\0'}
    }
    ,
    {   {( sizeof( memstatic_t ) + 3 )& ~3, TAG_STATIC, NULL, NULL, ZONEID}
        , {'8', '\0'}
    }
    ,
    {   {( sizeof( memstatic_t ) + 3 )& ~3, TAG_STATIC, NULL, NULL, ZONEID}
        , {'9', '\0'}
    }
};

/*
========================
CopyString

 NOTE:	never write over the memory CopyString returns because
		memory from a memstatic_t might be returned
========================
*/
UTF8* CopyString( StringEntry in )
{
    UTF8*           out;
    
    if( !in[0] )
    {
        return ( ( UTF8* )&emptystring ) + sizeof( memblock_t );
    }
    else if( !in[1] )
    {
        if( in[0] >= '0' && in[0] <= '9' )
        {
            return ( ( UTF8* )&numberstring[in[0] - '0'] ) + sizeof( memblock_t );
        }
    }
    out = ( UTF8* )S_Malloc( strlen( in ) + 1 );
    strcpy( out, in );
    return out;
}

/*
==============================================================================

Goals:
	reproducable without history effects -- no out of memory errors on weird map to map changes
	allow restarting of the client without fragmentation
	minimize total pages in use at run time
	minimize total pages needed during load time

  Single block of memory with stack allocators coming from both ends towards the middle.

  One side is designated the temporary memory allocator.

  Temporary memory can be allocated and freed in any order.

  A highwater mark is kept of the most in use at any time.

  When there is no temporary memory allocated, the permanent and temp sides
  can be switched, allowing the already touched temp memory to be used for
  permanent storage.

  Temp memory must never be allocated on two ends at once, or fragmentation
  could occur.

  If we have any in-use temp memory, additional temp allocations must come from
  that side.

  If not, we can choose to make either side the new temp side and push future
  permanent allocations to the other side.  Permanent allocations should be
  kept on the side that has the current greatest wasted highwater mark.

==============================================================================
*/

#define HUNK_MAGIC  0x89537892
#define HUNK_FREE_MAGIC 0x89537893

typedef struct
{
    S32             magic;
    size_t             size;
} hunkHeader_t;

typedef struct
{
    S32             mark;
    S32             permanent;
    S32             temp;
    S32             tempHighwater;
} hunkUsed_t;

typedef struct hunkblock_s
{
    size_t             size;
    U8            printed;
    struct hunkblock_s* next;
    UTF8*           label;
    UTF8*           file;
    S32             line;
} hunkblock_t;

static struct
{
    hunkblock_t* blocks;
    
    U8*	mem, *original;
    U64	memSize;
    
    U64	permTop, permMax;
    U64	tempTop, tempMax;
    
    U64	maxEver;
    
    U64	mark;
} s_hunk;

static hunkblock_t* hunkblocks;

static hunkUsed_t hunk_low, hunk_high;
static hunkUsed_t* hunk_permanent, *hunk_temp;

static U8*    s_hunkData = NULL;
static S32      s_hunkTotal;

static S32      s_zoneTotal;
static S32      s_smallZoneTotal;

/*
=================
Com_Meminfo_f
=================
*/
void Com_Meminfo_f( void )
{
    memblock_t*	block;
    S32			zoneBytes, zoneBlocks;
    S32			smallZoneBytes, smallZoneBlocks;
    S32			botlibBytes, rendererBytes, otherBytes;
    S32			cryptoBytes, staticBytes, generalBytes;
    
    zoneBytes = 0;
    botlibBytes = 0;
    rendererBytes = 0;
    otherBytes = 0;
    cryptoBytes = 0;
    staticBytes = 0;
    generalBytes = 0;
    zoneBlocks = 0;
    for( block = mainzone->blocklist.next ; ; block = block->next )
    {
        if( cmdSystem->Argc() != 1 )
        {
            Com_Printf( "block:%p    size:%7i    tag:%3i\n",
                        block, block->size, block->tag );
        }
        if( block->tag )
        {
            zoneBytes += block->size;
            zoneBlocks++;
            if( block->tag == TAG_BOTLIB )
            {
                botlibBytes += block->size;
            }
            else if( block->tag == TAG_RENDERER )
            {
                rendererBytes += block->size;
            }
            else if( block->tag == TAG_CRYPTO )
            {
                cryptoBytes += block->size;
            }
            else if( block->tag == TAG_STATIC )
            {
                staticBytes += block->size;
            }
            else if( block->tag == TAG_GENERAL )
            {
                generalBytes += block->size;
            }
            else
                otherBytes += block->size;
        }
        
        if( block->next == &mainzone->blocklist )
        {
            break;			// all blocks have been hit
        }
        if( ( U8* )block + block->size != ( U8* )block->next )
        {
            Com_Printf( "ERROR: block size does not touch the next block\n" );
        }
        if( block->next->prev != block )
        {
            Com_Printf( "ERROR: next block doesn't have proper back link\n" );
        }
        if( !block->tag && !block->next->tag )
        {
            Com_Printf( "ERROR: two consecutive free blocks\n" );
        }
    }
    
    smallZoneBytes = 0;
    smallZoneBlocks = 0;
    for( block = smallzone->blocklist.next ; ; block = block->next )
    {
        if( block->tag )
        {
            smallZoneBytes += block->size;
            smallZoneBlocks++;
        }
        
        if( block->next == &smallzone->blocklist )
        {
            break;			// all blocks have been hit
        }
    }
    
    Com_Printf( "%8i K total hunk\n", s_hunk.memSize / 1024 );
    Com_Printf( "%8i K total zone\n", s_zoneTotal / 1024 );
    Com_Printf( "\n" );
    
    Com_Printf( "%8i K used hunk (permanent)\n", s_hunk.permTop / 1024 );
    Com_Printf( "%8i K used hunk (temp)\n", s_hunk.tempTop / 1024 );
    Com_Printf( "%8i K used hunk (TOTAL)\n", ( s_hunk.permTop + s_hunk.tempTop ) / 1024 );
    Com_Printf( "\n" );
    
    Com_Printf( "%8i K max hunk (permanent)\n", s_hunk.permMax / 1024 );
    Com_Printf( "%8i K max hunk (temp)\n", s_hunk.tempMax / 1024 );
    Com_Printf( "%8i K max hunk (TOTAL)\n", ( s_hunk.permMax + s_hunk.tempMax ) / 1024 );
    Com_Printf( "\n" );
    
    Com_Printf( "%8i K max hunk since last Hunk_Clear\n", s_hunk.maxEver / 1024 );
    Com_Printf( "%8i K hunk mem never touched\n", ( s_hunk.memSize - s_hunk.maxEver ) / 1024 );
    Com_Printf( "%8i hunk mark value\n", s_hunk.mark );
    Com_Printf( "\n" );
    
    Com_Printf( "\n" );
    Com_Printf( "%8i bytes in %i zone blocks\n", zoneBytes, zoneBlocks	);
    Com_Printf( "        %8i bytes in dynamic botlib\n", botlibBytes );
    Com_Printf( "        %8i bytes in dynamic renderer\n", rendererBytes );
    Com_Printf( "        %8i bytes in dynamic other\n", otherBytes );
    Com_Printf( "        %8i bytes in small Zone memory\n", smallZoneBytes );
    Com_Printf( "        %8i bytes in cryto client memory\n", cryptoBytes );
    Com_Printf( "        %8i bytes in static server memory\n", staticBytes );
    Com_Printf( "        %8i bytes in general common memory\n", generalBytes );
}

/*
===============
Com_TouchMemory

Touch all known used data to make sure it is paged in
===============
*/
void Com_TouchMemory( void )
{
    S32		start, end;
    S32		i, j;
    S32		sum;
    memblock_t*	block;
    
    Z_CheckHeap();
    
    start = idsystem->Milliseconds();
    
    sum = 0;
    
    for( block = mainzone->blocklist.next ; ; block = block->next )
    {
        if( block->tag )
        {
            j = block->size >> 2;
            for( i = 0 ; i < j ; i += 64 )  				// only need to touch each page
            {
                sum += ( ( S32* )block )[i];
            }
        }
        if( block->next == &mainzone->blocklist )
        {
            break;			// all blocks have been hit
        }
    }
    
    end = idsystem->Milliseconds();
    
    Com_Printf( "Com_TouchMemory: %i msec\n", end - start );
}



/*
=================
Com_InitZoneMemory
=================
*/
void Com_InitSmallZoneMemory( void )
{
    s_smallZoneTotal = 512 * 1024;
    // bk001205 - was malloc
    smallzone = ( memzone_t* )calloc( s_smallZoneTotal, 1 );
    if( !smallzone )
    {
        Com_Error( ERR_FATAL, "Small zone data failed to allocate %1.1f megs", ( F32 )s_smallZoneTotal / ( 1024 * 1024 ) );
    }
    Z_ClearZone( smallzone, s_smallZoneTotal );
    
    return;
}

void Com_InitZoneMemory( void )
{
    convar_t*	cv;
    
    // allocate the random block zone
    Com_StartupVariable( "com_zoneMegs" ); // config files and command line options haven't been taken in account yet
    cv = cvarSystem->Get( "com_zoneMegs", DEF_COMZONEMEGS_S, CVAR_INIT, "description" );
    
    if( cv->integer < 20 )
    {
        s_zoneTotal = 1024 * 1024 * 16;
    }
    else
    {
        s_zoneTotal = cv->integer * 1024 * 1024;
    }
    
    // bk001205 - was malloc
    mainzone = ( memzone_t* )calloc( s_zoneTotal, 1 );
    if( !mainzone )
    {
        Com_Error( ERR_FATAL, "Zone data failed to allocate %i megs", s_zoneTotal / ( 1024 * 1024 ) );
    }
    Z_ClearZone( mainzone, s_zoneTotal );
    
}

/*
=================
Hunk_Log
=================
*/
void Hunk_Log( void )
{
    hunkblock_t*	block;
    UTF8		buf[4096];
    S32 size, numBlocks;
    
    if( !logfile_ || !fileSystem->Initialized() )
        return;
    size = 0;
    numBlocks = 0;
    Com_sprintf( buf, sizeof( buf ), "\r\n================\r\nHunk log\r\n================\r\n" );
    fileSystem->Write( buf, strlen( buf ), logfile_ );
    for( block = s_hunk.blocks; block; block = block->next )
    {
#ifdef HUNK_DEBUG
        Com_sprintf( buf, sizeof( buf ), "size = %8d: %s, line: %d (%s)\r\n", block->size, block->file, block->line, block->label );
        fileSystem->Write( buf, strlen( buf ), logfile_ );
#endif
        size += block->size;
        numBlocks++;
    }
    Com_sprintf( buf, sizeof( buf ), "%d Hunk memory\r\n", size );
    fileSystem->Write( buf, strlen( buf ), logfile_ );
    Com_sprintf( buf, sizeof( buf ), "%d hunk blocks\r\n", numBlocks );
    fileSystem->Write( buf, strlen( buf ), logfile_ );
}

/*
=================
Hunk_SmallLog
=================
*/
void Hunk_SmallLog( void )
{
    hunkblock_t*	block, *block2;
    UTF8		buf[4096];
    S32 size, locsize, numBlocks;
    
    if( !logfile_ || !fileSystem->Initialized() )
        return;
    for( block = s_hunk.blocks ; block; block = block->next )
    {
        block->printed = false;
    }
    size = 0;
    numBlocks = 0;
    Com_sprintf( buf, sizeof( buf ), "\r\n================\r\nHunk Small log\r\n================\r\n" );
    fileSystem->Write( buf, strlen( buf ), logfile_ );
    for( block = s_hunk.blocks; block; block = block->next )
    {
        if( block->printed )
        {
            continue;
        }
        locsize = block->size;
        for( block2 = block->next; block2; block2 = block2->next )
        {
            if( block->line != block2->line )
            {
                continue;
            }
            if( Q_stricmp( block->file, block2->file ) )
            {
                continue;
            }
            size += block2->size;
            locsize += block2->size;
            block2->printed = true;
        }
#ifdef HUNK_DEBUG
        Com_sprintf( buf, sizeof( buf ), "size = %8d: %s, line: %d (%s)\r\n", locsize, block->file, block->line, block->label );
        fileSystem->Write( buf, strlen( buf ), logfile_ );
#endif
        size += block->size;
        numBlocks++;
    }
    Com_sprintf( buf, sizeof( buf ), "%d Hunk memory\r\n", size );
    fileSystem->Write( buf, strlen( buf ), logfile_ );
    Com_sprintf( buf, sizeof( buf ), "%d hunk blocks\r\n", numBlocks );
    fileSystem->Write( buf, strlen( buf ), logfile_ );
}

/*
=================
Com_InitZoneMemory
=================
*/
void Com_InitHunkMemory( void )
{
    convar_t*	cv;
    S32 nMinAlloc;
    UTF8* pMsg = NULL;
    
    ::memset( &s_hunk, 0, sizeof( s_hunk ) );
    
    // make sure the file system has allocated and "not" freed any temp blocks
    // this allows the config and product id files ( journal files too ) to be loaded
    // by the file system without redunant routines in the file system utilizing different
    // memory systems
    if( fileSystem->LoadStack() != 0 )
    {
        Com_Error( ERR_FATAL, "Hunk initialization failed. File system load stack not zero" );
    }
    
    // allocate the stack based hunk allocator
    cv = cvarSystem->Get( "com_hunkMegs", DEF_COMHUNKMEGS_S, CVAR_LATCH | CVAR_ARCHIVE, "description" );
    
    // if we are not dedicated min allocation is 56, otherwise min is 1
    if( com_dedicated && com_dedicated->integer )
    {
        nMinAlloc = MIN_DEDICATED_COMHUNKMEGS;
        pMsg = "Minimum com_hunkMegs for a dedicated server is %i, allocating %i megs.\n";
    }
    else
    {
        nMinAlloc = MIN_COMHUNKMEGS;
        pMsg = "Minimum com_hunkMegs is %i, allocating %i megs.\n";
    }
    
    if( cv->integer < nMinAlloc )
    {
        s_hunk.memSize = 1024 * 1024 * nMinAlloc;
        Com_Printf( pMsg, nMinAlloc, s_hunk.memSize / ( 1024 * 1024 ) );
    }
    else
    {
        s_hunk.memSize = cv->integer * 1024 * 1024;
    }
    
    // bk001205 - was malloc
    s_hunk.original = ( U8* )calloc( s_hunk.memSize + 31, 1 );
    if( !s_hunk.original )
    {
        Com_Error( ERR_FATAL, "Hunk data failed to allocate %i megs", s_hunk.memSize / ( 1024 * 1024 ) );
    }
    // cacheline align
    s_hunk.mem = ( U8* )( ( ( intptr_t )s_hunk.original + 31 ) & ~31 );
    
    Hunk_Clear();
    
    cmdSystem->AddCommand( "meminfo", Com_Meminfo_f, "description" );
#ifdef ZONE_DEBUG
    cmdSystem->AddCommand( "zonelog", Z_LogHeap, "description" );
#endif
#ifdef HUNK_DEBUG
    cmdSystem->AddCommand( "hunklog", Hunk_Log, "description" );
    cmdSystem->AddCommand( "hunksmalllog", Hunk_SmallLog, "description" );
#endif
}

void Com_ReleaseMemory( void )
{
    if( s_hunk.original )
        free( s_hunk.original );
    ::memset( &s_hunk, 0, sizeof( s_hunk ) );
    
    if( smallzone )
    {
        free( smallzone );
        smallzone = 0;
    }
    
    if( mainzone )
    {
        free( mainzone );
        mainzone = 0;
    }
}

/*
====================
Hunk_MemoryRemaining
====================
*/
S32	Hunk_MemoryRemaining( void )
{
    return s_hunk.memSize - s_hunk.permTop - s_hunk.tempTop;
}

/*
===================
Hunk_SetMark

The server calls this after the level and game VM have been loaded
===================
*/
void Hunk_SetMark( void )
{
    s_hunk.mark = s_hunk.permTop;
}

/*
=================
Hunk_ClearToMark

The client calls this before starting a vid_restart or snd_restart
=================
*/
void Hunk_ClearToMark( void )
{
    s_hunk.permTop = s_hunk.mark;
    s_hunk.permMax = s_hunk.permTop;
    
    s_hunk.tempMax = s_hunk.tempTop = 0;
}

/*
=================
Hunk_CheckMark
=================
*/
bool Hunk_CheckMark( void )
{
    return s_hunk.mark ? true : false;
}

/*
=================
Hunk_Clear

The server calls this before shutting down or loading a new map
=================
*/

static void Hunk_FrameInit( void );

void Hunk_Clear( void )
{
#ifndef DEDICATED
    clientGameSystem->ShutdownCGame();
    clientGUISystem->ShutdownGUI();
#endif
    
    serverGameSystem->ShutdownGameProgs();
    
#ifndef DEDICATED
    CIN_CloseAllVideos();
#endif
    
    s_hunk.permTop = 0;
    s_hunk.permMax = 0;
    s_hunk.tempTop = 0;
    s_hunk.tempMax = 0;
    s_hunk.maxEver = 0;
    s_hunk.mark = 0;
    
    Com_Printf( "Hunk_Clear: reset the hunk ok\n" );
    
#ifdef HUNK_DEBUG
    s_hunk.blocks = NULL;
#endif
    
    //stake out a chunk for the frame temp data
    Hunk_FrameInit();
}

static void Hunk_SwapBanks( void ) { }

/*
=================
Hunk_Alloc

Allocate permanent (until the hunk is cleared) memory
=================
*/
#ifdef HUNK_DEBUG
void* Hunk_AllocDebug( size_t size, ha_pref preference, UTF8* label, UTF8* file, S32 line )
{
#else
void* Hunk_Alloc( size_t size, ha_pref preference )
{
#endif
    void*	buf;
    
    if( s_hunk.mem == NULL )
    {
        Com_Error( ERR_FATAL, "Hunk_Alloc: Hunk memory system not initialized" );
    }
    
#ifdef HUNK_DEBUG
    size += sizeof( hunkblock_t );
#endif
    
    // round to cacheline
    size = ( size + 31 ) & ~31;
    
    if( s_hunk.permTop + s_hunk.tempTop + size > s_hunk.memSize )
    {
#ifdef HUNK_DEBUG
        Hunk_Log();
        Hunk_SmallLog();
#endif
        Com_Error( ERR_DROP, "Hunk_Alloc failed on %i", size );
    }
    
    buf = s_hunk.mem + s_hunk.permTop;
    s_hunk.permTop += size;
    
    if( s_hunk.permTop > s_hunk.permMax )
        s_hunk.permMax = s_hunk.permTop;
        
    if( s_hunk.permTop + s_hunk.tempTop > s_hunk.maxEver )
        s_hunk.maxEver = s_hunk.permTop + s_hunk.tempTop;
        
    ::memset( buf, 0, size );
    
#ifdef HUNK_DEBUG
    {
        hunkblock_t* block;
        
        block = ( hunkblock_t* ) buf;
        block->size = size - sizeof( hunkblock_t );
        block->file = file;
        block->label = label;
        block->line = line;
        block->next = s_hunk.blocks;
        s_hunk.blocks = block;
        buf = ( ( U8* ) buf ) + sizeof( hunkblock_t );
    }
#endif
    
    return buf;
}

/*
=================
Hunk_AllocateTempMemory

This is used by the file loading system.
Multiple files can be loaded in temporary memory.
When the files-in-use count reaches zero, all temp memory will be deleted
=================
*/
void* Hunk_AllocateTempMemory( size_t size )
{
    void*		buf;
    hunkHeader_t*	hdr;
    
    // return a Z_Malloc'd block if the hunk has not been initialized
    // this allows the config and product id files ( journal files too ) to be loaded
    // by the file system without redunant routines in the file system utilizing different
    // memory systems
    if( s_hunk.mem == NULL )
    {
        return Z_Malloc( size );
    }
    
    Hunk_SwapBanks();
    
    size = PAD( size, sizeof( intptr_t ) ) + sizeof( hunkHeader_t );
    
    if( s_hunk.permTop + s_hunk.tempTop + size > s_hunk.memSize )
    {
        Com_Error( ERR_DROP, "Hunk_AllocateTempMemory: failed on %i", size );
    }
    
    s_hunk.tempTop += size;
    buf = s_hunk.mem + s_hunk.memSize - s_hunk.tempTop;
    
    if( s_hunk.tempTop > s_hunk.tempMax )
        s_hunk.tempMax = s_hunk.tempTop;
        
    if( s_hunk.permTop + s_hunk.tempTop > s_hunk.maxEver )
        s_hunk.maxEver = s_hunk.permTop + s_hunk.tempTop;
        
    hdr = ( hunkHeader_t* )buf;
    buf = ( void* )( hdr + 1 );
    
    hdr->magic = HUNK_MAGIC;
    hdr->size = size;
    
    // don't bother clearing, because we are going to load a file over it
    return buf;
}


/*
==================
Hunk_FreeTempMemory
==================
*/
void Hunk_FreeTempMemory( void* buf )
{
    hunkHeader_t*	hdr;
    
    // free with Z_Free if the hunk has not been initialized
    // this allows the config and product id files ( journal files too ) to be loaded
    // by the file system without redunant routines in the file system utilizing different
    // memory systems
    if( !s_hunk.mem )
    {
        Z_Free( buf );
        return;
    }
    
    
    hdr = ( hunkHeader_t* )buf - 1;
    if( hdr->magic != HUNK_MAGIC )
    {
        Com_Error( ERR_FATAL, "Hunk_FreeTempMemory: bad magic" );
    }
    
    hdr->magic = HUNK_FREE_MAGIC;
    
    // this only works if the files are freed in stack order,
    // otherwise the memory will stay around until Hunk_ClearTempMemory
    if( ( U8* )hdr == s_hunk.mem + s_hunk.memSize - s_hunk.tempTop )
    {
        s_hunk.tempTop -= hdr->size;
    }
}


/*
=================
Hunk_ClearTempMemory

The temp space is no longer needed.  If we have left more
touched but unused memory on this side, have future
permanent allocs use this side.
=================
*/
void Hunk_ClearTempMemory( void )
{
    if( s_hunk.mem )
    {
        s_hunk.tempTop = 0;
        s_hunk.tempMax = 0;
    }
}

static U8* s_frameStackLoc = 0;
static U8* s_frameStackBase = 0;
static U8* s_frameStackEnd = 0;

static void Hunk_FrameInit( void )
{
    S32 megs = cvarSystem->Get( "com_hunkFrameMegs", "1", CVAR_LATCH | CVAR_ARCHIVE, "description" )->integer;
    U64 cb;
    
    if( megs < 1 )
        megs = 1;
        
    cb = 1024 * 1024 * megs;
    
    s_frameStackBase = ( U8* )Hunk_Alloc( cb, h_low );
    s_frameStackEnd = s_frameStackBase + cb;
    
    s_frameStackLoc = s_frameStackBase;
}


void* Hunk_FrameAlloc( U64 cb )
{
    void* ret;
    
    if( s_frameStackLoc + cb >= s_frameStackEnd )
        //out of frame stack memory
        return 0;
        
    ret = s_frameStackLoc;
    s_frameStackLoc += cb;
    
    //::memset( ret, 0, cb );
    return ret;
}

void Hunk_FrameReset( void )
{
    s_frameStackLoc = s_frameStackBase;
}

/*
===================================================================

EVENTS AND JOURNALING

In addition to these events, .cfg files are also copied to the
journaled file
===================================================================
*/

// bk001129 - here we go again: upped from 64
// Dushan, 512
#define MAX_PUSHED_EVENTS               512
// bk001129 - init, also static
static S32      com_pushedEventsHead = 0;
static S32      com_pushedEventsTail = 0;

// bk001129 - static
static sysEvent_t com_pushedEvents[MAX_PUSHED_EVENTS];

/*
=================
Com_InitJournaling
=================
*/
void Com_InitJournaling( void )
{
    S32 i;
    
    Com_StartupVariable( "journal" );
    com_journal = cvarSystem->Get( "journal", "0", CVAR_INIT, "description" );
    if( !com_journal->integer )
    {
        if( com_journal->string && com_journal->string[ 0 ] == '_' )
        {
            Com_Printf( "Replaying journaled events\n" );
            fileSystem->FOpenFileRead( va( "journal%s.dat", com_journal->string ), &com_journalFile, true );
            fileSystem->FOpenFileRead( va( "journal_data%s.dat", com_journal->string ), &com_journalDataFile, true );
            com_journal->integer = 2;
        }
        else
            return;
    }
    else
    {
        for( i = 0; i <= 9999 ; i++ )
        {
            UTF8 f[MAX_OSPATH];
            Com_sprintf( f, sizeof( f ), "journal_%04d.dat", i );
            if( !fileSystem->FileExists( f ) )
                break;
        }
        
        if( com_journal->integer == 1 )
        {
            Com_Printf( "Journaling events\n" );
            com_journalFile		= fileSystem->FOpenFileWrite( va( "journal_%04d.dat", i ) );
            com_journalDataFile	= fileSystem->FOpenFileWrite( va( "journal_data_%04d.dat", i ) );
        }
        else if( com_journal->integer == 2 )
        {
            i--;
            Com_Printf( "Replaying journaled events\n" );
            fileSystem->FOpenFileRead( va( "journal_%04d.dat", i ), &com_journalFile, true );
            fileSystem->FOpenFileRead( va( "journal_data_%04d.dat", i ), &com_journalDataFile, true );
        }
    }
    
    if( !com_journalFile || !com_journalDataFile )
    {
        cvarSystem->Set( "journal", "0" );
        
        if( com_journalFile )
        {
            fileSystem->FCloseFile( com_journalFile );
        }
        
        if( com_journalDataFile )
        {
            fileSystem->FCloseFile( com_journalDataFile );
        }
        
        com_journalFile = 0;
        com_journalDataFile = 0;
        Com_Printf( "Couldn't open journal files\n" );
    }
}

/*
========================================================================
EVENT LOOP
========================================================================
*/

#define MAX_QUEUED_EVENTS  256
#define MASK_QUEUED_EVENTS ( MAX_QUEUED_EVENTS - 1 )

static sysEvent_t  eventQueue[ MAX_QUEUED_EVENTS ];
static S32         eventHead = 0;
static S32         eventTail = 0;
static U8        sys_packetReceived[ MAX_MSGLEN ];

/*
================
Com_QueueEvent

A time of 0 will get the current time
Ptr should either be null, or point to a block of data that can
be freed by the game later.
================
*/
void Com_QueueEvent( S32 time, sysEventType_t type, S32 value, S32 value2, S32 ptrLength, void* ptr )
{
    sysEvent_t*  ev;
    
    ev = &eventQueue[ eventHead & MASK_QUEUED_EVENTS ];
    
    if( eventHead - eventTail >= MAX_QUEUED_EVENTS )
    {
        Com_DPrintf( "Com_QueueEvent: overflow\n" );
        // we are discarding an event, but don't leak memory
        if( ev->evPtr )
        {
            Z_Free( ev->evPtr );
        }
        eventTail++;
    }
    
    eventHead++;
    
    if( time == 0 )
    {
        time = idsystem->Milliseconds();
    }
    
    ev->evTime = time;
    ev->evType = type;
    ev->evValue = value;
    ev->evValue2 = value2;
    ev->evPtrLength = ptrLength;
    ev->evPtr = ptr;
}

/*
================
Com_GetSystemEvent

================
*/
sysEvent_t Com_GetSystemEvent( void )
{
    sysEvent_t  ev;
    UTF8*        s;
    msg_t       netmsg;
    netadr_t    adr;
    
    // return if we have data
    if( eventHead > eventTail )
    {
        eventTail++;
        return eventQueue[( eventTail - 1 ) & MASK_QUEUED_EVENTS ];
    }
    
    // check for console commands
    s = idsystem->ConsoleInput();
    if( s )
    {
        UTF8*  b;
        S32   len;
        
        len = strlen( s ) + 1;
        b = ( UTF8* )Z_Malloc( len );
        Q_strncpyz( b, s, len - 1 );
        Com_QueueEvent( 0, SYSE_CONSOLE, 0, 0, len, b );
    }
    
    // check for network packets
    MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );
    adr.type = NA_UNSPEC;
    if( Net_GetPacket( &adr, &netmsg ) )
    {
        netadr_t*  buf;
        S32       len;
        
        // copy out to a seperate buffer for qeueing
        len = sizeof( netadr_t ) + netmsg.cursize;
        buf = ( netadr_t* )Z_Malloc( len );
        *buf = adr;
        memcpy( buf + 1, &netmsg.data[netmsg.readcount], netmsg.cursize - netmsg.readcount );
        Com_QueueEvent( 0, SYSE_PACKET, 0, 0, len, buf );
    }
    
    // return if we have data
    if( eventHead > eventTail )
    {
        eventTail++;
        return eventQueue[( eventTail - 1 ) & MASK_QUEUED_EVENTS ];
    }
    
    // create an empty event to return
    memset( &ev, 0, sizeof( ev ) );
    ev.evTime = idsystem->Milliseconds();
    
    return ev;
}


/*
=================
Com_GetRealEvent
=================
*/
sysEvent_t	Com_GetRealEvent( void )
{
    S32			r;
    sysEvent_t	ev;
    
    // either get an event from the system or the journal file
    if( com_journal->integer == 2 )
    {
        r = fileSystem->Read( &ev, sizeof( ev ), com_journalFile );
        if( r != sizeof( ev ) )
        {
            //Com_Error( ERR_FATAL, "Error reading from journal file" );
            com_journal->integer = 0;
            ev.evType = SYSE_NONE;
        }
        if( ev.evPtrLength )
        {
            ev.evPtr = Z_Malloc( ev.evPtrLength );
            r = fileSystem->Read( ev.evPtr, ev.evPtrLength, com_journalFile );
            if( r != ev.evPtrLength )
            {
                //Com_Error( ERR_FATAL, "Error reading from journal file" );
                com_journal->integer = 0;
                ev.evType = SYSE_NONE;
            }
        }
    }
    else
    {
        ev = Com_GetSystemEvent();
        
        // write the journal value out if needed
        if( com_journal->integer == 1 )
        {
            r = fileSystem->Write( &ev, sizeof( ev ), com_journalFile );
            if( r != sizeof( ev ) )
            {
                Com_Error( ERR_FATAL, "Error writing to journal file" );
            }
            if( ev.evPtrLength )
            {
                r = fileSystem->Write( ev.evPtr, ev.evPtrLength, com_journalFile );
                if( r != ev.evPtrLength )
                {
                    Com_Error( ERR_FATAL, "Error writing to journal file" );
                }
            }
        }
    }
    
    return ev;
}


/*
=================
Com_InitPushEvent
=================
*/
// bk001129 - added
void Com_InitPushEvent( void )
{
    // clear the static buffer array
    // this requires SYSE_NONE to be accepted as a valid but NOP event
    memset( com_pushedEvents, 0, sizeof( com_pushedEvents ) );
    // reset counters while we are at it
    // beware: GetEvent might still return an SYSE_NONE from the buffer
    com_pushedEventsHead = 0;
    com_pushedEventsTail = 0;
}


/*
=================
Com_PushEvent
=================
*/
void Com_PushEvent( sysEvent_t* _event )
{
    sysEvent_t*     ev;
    static S32      printedWarning = 0;	// bk001129 - init, bk001204 - explicit S32
    
    ev = &com_pushedEvents[com_pushedEventsHead & ( MAX_PUSHED_EVENTS - 1 )];
    
    if( com_pushedEventsHead - com_pushedEventsTail >= MAX_PUSHED_EVENTS )
    {
    
        // don't print the warning constantly, or it can give time for more...
        if( !printedWarning )
        {
            printedWarning = true;
            Com_Printf( "WARNING: Com_PushEvent overflow\n" );
        }
        
        if( ev->evPtr )
        {
            Z_Free( ev->evPtr );
        }
        com_pushedEventsTail++;
    }
    else
    {
        printedWarning = false;
    }
    
    *ev = *_event;
    com_pushedEventsHead++;
}

/*
=================
Com_GetEvent
=================
*/
sysEvent_t Com_GetEvent( void )
{
    if( com_pushedEventsHead > com_pushedEventsTail )
    {
        com_pushedEventsTail++;
        return com_pushedEvents[( com_pushedEventsTail - 1 ) & ( MAX_PUSHED_EVENTS - 1 )];
    }
    return Com_GetRealEvent();
}

/*
=================
Com_RunAndTimeServerPacket
=================
*/
void Com_RunAndTimeServerPacket( netadr_t* evFrom, msg_t* buf )
{
    S32             t1, t2, msec;
    
    t1 = 0;
    
    if( com_speeds->integer )
    {
        t1 = idsystem->Milliseconds();
    }
    
    serverMainSystem->PacketEvent( *evFrom, buf );
    
    if( com_speeds->integer )
    {
        t2 = idsystem->Milliseconds();
        msec = t2 - t1;
        if( com_speeds->integer == 3 )
        {
            Com_Printf( "idServerMainSystemLocal::PacketEvent time: %i\n", msec );
        }
    }
}

/*
=================
Com_EventLoop

Returns last event time
=================
*/

#ifndef DEDICATED
extern bool consoleButtonWasPressed;
#endif

S32 Com_EventLoop( void )
{
    sysEvent_t      ev;
    netadr_t        evFrom;
    static U8       bufData[MAX_MSGLEN];
    msg_t           buf;
    
    MSG_Init( &buf, bufData, sizeof( bufData ) );
    
    while( 1 )
    {
        NET_FlushPacketQueue();
        ev = Com_GetEvent();
        
        // if no more events are available
        if( ev.evType == SYSE_NONE )
        {
            // manually send packet events for the loopback channel
            while( NET_GetLoopPacket( NS_CLIENT, &evFrom, &buf ) )
            {
                CL_PacketEvent( evFrom, &buf );
            }
            
            while( NET_GetLoopPacket( NS_SERVER, &evFrom, &buf ) )
            {
                // if the server just shut down, flush the events
                if( com_sv_running->integer )
                {
                    Com_RunAndTimeServerPacket( &evFrom, &buf );
                }
            }
            
            return ev.evTime;
        }
        
        
        switch( ev.evType )
        {
            default:
                // bk001129 - was ev.evTime
                Com_Error( ERR_FATAL, "Com_EventLoop: bad event type %i", ev.evType );
                break;
            case SYSE_NONE:
                break;
            case SYSE_KEY:
                CL_KeyEvent( ev.evValue, ev.evValue2, ev.evTime );
                break;
            case SYSE_CHAR:
#ifndef DEDICATED
                // fretn
                // we just pressed the console button,
                // so ignore this event
                // this prevents chars appearing at console input
                // when you just opened it
                if( consoleButtonWasPressed )
                {
                    consoleButtonWasPressed = false;
                    break;
                }
#endif
                CL_CharEvent( ev.evValue );
                break;
            case SYSE_MOUSE:
                CL_MouseEvent( ev.evValue, ev.evValue2, ev.evTime );
                break;
            case SYSE_JOYSTICK_AXIS:
                CL_JoystickEvent( ev.evValue, ev.evValue2, ev.evTime );
                break;
            case SYSE_CONSOLE:
                //cmdBufferSystem->AddText( "\n" );
                if( ( ( UTF8* )ev.evPtr )[0] == '\\' || ( ( UTF8* )ev.evPtr )[0] == '/' )
                {
                    cmdBufferSystem->AddText( ( UTF8* )ev.evPtr + 1 );
                }
                else
                {
                    cmdBufferSystem->AddText( ( UTF8* )ev.evPtr );
                }
                break;
            case SYSE_PACKET:
                // this cvar allows simulation of connections that
                // drop a lot of packets.  Note that loopback connections
                // don't go through here at all.
                if( com_dropsim->value > 0 )
                {
                    static S32      seed;
                    
                    if( Q_random( &seed ) < com_dropsim->value )
                    {
                        break;	// drop this packet
                    }
                }
                
                evFrom = *( netadr_t* ) ev.evPtr;
                buf.cursize = ev.evPtrLength - sizeof( evFrom );
                
                // we must copy the contents of the message out, because
                // the event buffers are only large enough to hold the
                // exact payload, but channel messages need to be large
                // enough to hold fragment reassembly
                if( ( U32 )buf.cursize > buf.maxsize )
                {
                    Com_Printf( "Com_EventLoop: oversize packet\n" );
                    continue;
                }
                memcpy( buf.data, ( U8* )( ( netadr_t* ) ev.evPtr + 1 ), buf.cursize );
                if( com_sv_running->integer )
                {
                    Com_RunAndTimeServerPacket( &evFrom, &buf );
                }
                else
                {
                    CL_PacketEvent( evFrom, &buf );
                }
                break;
        }
        
        // free any block data
        if( ev.evPtr )
        {
            Z_Free( ev.evPtr );
        }
    }
    
    return 0;					// never reached
}

/*
================
Com_Milliseconds

Can be used for profiling, but will be journaled accurately
================
*/
S32 Com_Milliseconds( void )
{
    sysEvent_t      ev;
    
    // get events and push them until we get a null event with the current time
    do
    {
    
        ev = Com_GetRealEvent();
        if( ev.evType != SYSE_NONE )
        {
            Com_PushEvent( &ev );
        }
    }
    while( ev.evType != SYSE_NONE );
    
    return ev.evTime;
}

//============================================================================

/*
=============
Com_Error_f

Just throw a fatal error to
test error shutdown procedures
=============
*/
static void Com_Error_f( void )
{
    if( cmdSystem->Argc() > 1 )
    {
        Com_Error( ERR_DROP, "Testing drop error" );
    }
    else
    {
        Com_Error( ERR_FATAL, "Testing fatal error" );
    }
}


/*
=============
Com_Freeze_f

Just freeze in place for a given number of seconds to test
error recovery
=============
*/
static void Com_Freeze_f( void )
{
    F32           s;
    S32             start, now;
    
    if( cmdSystem->Argc() != 2 )
    {
        Com_Printf( "freeze <seconds>\n" );
        return;
    }
    s = atof( cmdSystem->Argv( 1 ) );
    
    start = Com_Milliseconds();
    
    while( 1 )
    {
        now = Com_Milliseconds();
        if( ( now - start ) / 1000.0f > s )
        {
            break;
        }
    }
}

/*
=================
Com_Crash_f

A way to force a bus error for development reasons
=================
*/
static void Com_Crash_f( void )
{
    *( volatile S32* )0 = 0x12345678;
}

void Com_SetRecommended()
{
    convar_t*         r_highQualityVideo, *com_recommended;
    bool        goodVideo;
    
    // will use this for recommended settings as well.. do i outside the lower check so it gets done even with command line stuff
    r_highQualityVideo = cvarSystem->Get( "r_highQualityVideo", "1", CVAR_ARCHIVE, "description" );
    com_recommended = cvarSystem->Get( "com_recommended", "-1", CVAR_ARCHIVE, "description" );
    goodVideo = ( bool )( r_highQualityVideo && r_highQualityVideo->integer );
    
    if( goodVideo )
    {
        Com_Printf( "Found high quality video and slow CPU\n" );
        cmdBufferSystem->AddText( "exec preset_fast.cfg\n" );
        cvarSystem->Set( "com_recommended", "2" );
    }
    else
    {
        Com_Printf( "Found low quality video and slow CPU\n" );
        cmdBufferSystem->AddText( "exec preset_fastest.cfg\n" );
        cvarSystem->Set( "com_recommended", "3" );
    }
    
}

// Arnout: gameinfo, to let the engine know which gametypes are SP and if we should use profiles.
// This can't be dependant on gamecode as we sometimes need to know about it when no game-modules
// are loaded
gameInfo_t      com_gameInfo;

void Com_GetGameInfo()
{
    UTF8*           f, *buf;
    UTF8*           token;
    
    memset( &com_gameInfo, 0, sizeof( com_gameInfo ) );
    
    if( fileSystem->ReadFile( "gameinfo.dat", ( void** )&f ) > 0 )
    {
        buf = f;
        
        while( ( token = COM_Parse( &buf ) ) != NULL && token[0] )
        {
            if( !Q_stricmp( token, "spEnabled" ) )
            {
                com_gameInfo.spEnabled = true;
            }
            else if( !Q_stricmp( token, "spGameTypes" ) )
            {
                while( ( token = COM_ParseExt( &buf, false ) ) != NULL && token[0] )
                {
                    com_gameInfo.spGameTypes |= ( 1 << atoi( token ) );
                }
            }
            else if( !Q_stricmp( token, "defaultSPGameType" ) )
            {
                if( ( token = COM_ParseExt( &buf, false ) ) != NULL && token[0] )
                {
                    com_gameInfo.defaultSPGameType = atoi( token );
                }
                else
                {
                    fileSystem->FreeFile( f );
                    Com_Error( ERR_FATAL, "Com_GetGameInfo: bad syntax." );
                }
            }
            else if( !Q_stricmp( token, "coopGameTypes" ) )
            {
            
                while( ( token = COM_ParseExt( &buf, false ) ) != NULL && token[0] )
                {
                    com_gameInfo.coopGameTypes |= ( 1 << atoi( token ) );
                }
            }
            else if( !Q_stricmp( token, "defaultCoopGameType" ) )
            {
                if( ( token = COM_ParseExt( &buf, false ) ) != NULL && token[0] )
                {
                    com_gameInfo.defaultCoopGameType = atoi( token );
                }
                else
                {
                    fileSystem->FreeFile( f );
                    Com_Error( ERR_FATAL, "Com_GetGameInfo: bad syntax." );
                }
            }
            else if( !Q_stricmp( token, "defaultGameType" ) )
            {
                if( ( token = COM_ParseExt( &buf, false ) ) != NULL && token[0] )
                {
                    com_gameInfo.defaultGameType = atoi( token );
                }
                else
                {
                    fileSystem->FreeFile( f );
                    Com_Error( ERR_FATAL, "Com_GetGameInfo: bad syntax." );
                }
            }
            else if( !Q_stricmp( token, "usesProfiles" ) )
            {
                if( ( token = COM_ParseExt( &buf, false ) ) != NULL && token[0] )
                {
                    com_gameInfo.usesProfiles = ( bool )atoi( token );
                }
                else
                {
                    fileSystem->FreeFile( f );
                    Com_Error( ERR_FATAL, "Com_GetGameInfo: bad syntax." );
                }
            }
            else
            {
                fileSystem->FreeFile( f );
                Com_Error( ERR_FATAL, "Com_GetGameInfo: bad syntax." );
            }
        }
        
        // all is good
        fileSystem->FreeFile( f );
    }
}

// bani - checks if profile.pid is valid
// return true if it is
// return false if it isn't(!)
bool Com_CheckProfile( UTF8* profile_path )
{
    fileHandle_t    f;
    UTF8            f_data[32];
    S32             f_pid;
    
    //let user override this
    if( com_ignorecrash->integer )
    {
        return true;
    }
    
    if( fileSystem->FOpenFileRead( profile_path, &f, true ) < 0 )
    {
        //no profile found, we're ok
        return true;
    }
    
    if( fileSystem->Read( &f_data, sizeof( f_data ) - 1, f ) < 0 )
    {
        //b0rk3d!
        fileSystem->FCloseFile( f );
        //try to delete corrupted pid file
        fileSystem->Delete( profile_path );
        return false;
    }
    
    f_pid = atoi( f_data );
    if( f_pid != com_pid->integer )
    {
        //pid doesn't match
        fileSystem->FCloseFile( f );
        return false;
    }
    
    //we're all ok
    fileSystem->FCloseFile( f );
    return true;
}

//bani - from files.c
extern UTF8     fs_gamedir[MAX_OSPATH];
UTF8            last_fs_gamedir[MAX_OSPATH];
UTF8            last_profile_path[MAX_OSPATH];

//bani - track profile changes, delete old profile.pid if we change fs_game(dir)
//hackish, we fiddle with fs_gamedir to make fileSystem->* calls work "right"
void Com_TrackProfile( UTF8* profile_path )
{
    UTF8            temp_fs_gamedir[MAX_OSPATH];
    
//  Com_Printf( "Com_TrackProfile: Tracking profile [%s] [%s]\n", fs_gamedir, profile_path );
    //have we changed fs_game(dir)?
    if( strcmp( last_fs_gamedir, fs_gamedir ) )
    {
        if( strlen( last_fs_gamedir ) && strlen( last_profile_path ) )
        {
            //save current fs_gamedir
            Q_strncpyz( temp_fs_gamedir, fs_gamedir, sizeof( temp_fs_gamedir ) );
            //set fs_gamedir temporarily to make fileSystem->* stuff work "right"
            Q_strncpyz( fs_gamedir, last_fs_gamedir, sizeof( fs_gamedir ) );
            if( fileSystem->FileExists( last_profile_path ) )
            {
                Com_Printf( "Com_TrackProfile: Deleting old pid file [%s] [%s]\n", fs_gamedir, last_profile_path );
                fileSystem->Delete( last_profile_path );
            }
            //restore current fs_gamedir
            Q_strncpyz( fs_gamedir, temp_fs_gamedir, sizeof( fs_gamedir ) );
        }
        //and save current vars for future reference
        Q_strncpyz( last_fs_gamedir, fs_gamedir, sizeof( last_fs_gamedir ) );
        Q_strncpyz( last_profile_path, profile_path, sizeof( last_profile_path ) );
    }
}

// bani - writes pid to profile
// returns true if successful
// returns false if not(!!)
bool Com_WriteProfile( UTF8* profile_path )
{
    fileHandle_t    f;
    
    if( fileSystem->FileExists( profile_path ) )
    {
        fileSystem->Delete( profile_path );
    }
    
    f = fileSystem->FOpenFileWrite( profile_path );
    if( f < 0 )
    {
        Com_Printf( "Com_WriteProfile: Can't write %s.\n", profile_path );
        return false;
    }
    
    fileSystem->Printf( f, "%d", com_pid->integer );
    
    fileSystem->FCloseFile( f );
    
    //track profile changes
    Com_TrackProfile( profile_path );
    
    return true;
}

/*
=================
Com_Init
=================
*/
void Com_Init( UTF8* commandLine )
{
    UTF8*           s;
    S32             pid;
    
    // TTimo gcc warning: variable `safeMode' might be clobbered by `longjmp' or `vfork'
    volatile bool safeMode = true;
    
    Com_Printf( "%s %s %s\n%s\n", Q3_VERSION, ARCH_STRING, __DATE__, commandLine );
    
    if( setjmp( abortframe ) )
    {
        idsystem->Error( "Error during initialization" );
    }
    
    // Clear queues
    ::memset( &eventQueue[0], 0, MAX_QUEUED_EVENTS * sizeof( sysEvent_t ) );
    ::memset( &sys_packetReceived[0], 0, MAX_MSGLEN * sizeof( U8 ) );
    
    // bk001129 - do this before anything else decides to push events
    Com_InitPushEvent();
    
    Com_InitSmallZoneMemory();
    cvarSystem->Init();
    
    // prepare enough of the subsystems to handle
    // cvar and command buffer management
    Com_ParseCommandLine( commandLine );
    
    cmdBufferSystem->Init();
    
    Com_InitZoneMemory();
    cmdSystem->Init();
    
    // override anything from the config files with command line args
    Com_StartupVariable( NULL );
    
    // get the developer cvar set as early as possible
    Com_StartupVariable( "developer" );
    
    // bani: init this early
    Com_StartupVariable( "com_ignorecrash" );
    com_ignorecrash = cvarSystem->Get( "com_ignorecrash", "0", 0, "description" );
    
    // ydnar: init crashed variable as early as possible
    com_crashed = cvarSystem->Get( "com_crashed", "0", CVAR_TEMP, "description" );
    
    // bani: init pid
#ifdef _WIN32
    pid = GetCurrentProcessId();
#else
    pid = getpid();
#endif
    s = va( "%d", pid );
    com_pid = cvarSystem->Get( "com_pid", s, CVAR_ROM, "description" );
    
    // done early so bind command exists
    CL_InitKeyCommands();
    
#ifdef _WIN32
    _setmaxstdio( 2048 );
#endif
    
    fileSystem->InitFilesystem();
    
    Com_InitJournaling();
    
    Com_GetGameInfo();
    
#ifndef UPDATE_SERVER
    cmdBufferSystem->AddText( "exec default.cfg\n" );
    cmdBufferSystem->AddText( "exec language.lang\n" );	// NERVE - SMF
    
    // skip the q3config.cfg if "safe" is on the command line
    if( !Com_SafeMode() )
    {
        UTF8*           cl_profileStr = cvarSystem->VariableString( "cl_profile" );
        
        safeMode = false;
        if( com_gameInfo.usesProfiles )
        {
            if( !cl_profileStr[0] )
            {
                UTF8*           defaultProfile = NULL;
                
                fileSystem->ReadFile( "profiles/defaultprofile.dat", ( void** )&defaultProfile );
                
                if( defaultProfile )
                {
                    UTF8*           text_p = defaultProfile;
                    UTF8*           token = COM_Parse( &text_p );
                    
                    if( token && *token )
                    {
                        cvarSystem->Set( "cl_defaultProfile", token );
                        cvarSystem->Set( "cl_profile", token );
                    }
                    
                    fileSystem->FreeFile( defaultProfile );
                    
                    cl_profileStr = cvarSystem->VariableString( "cl_defaultProfile" );
                }
            }
            
            if( cl_profileStr[0] )
            {
                // bani - check existing pid file and make sure it's ok
                if( !Com_CheckProfile( va( "profiles/%s/profile.pid", cl_profileStr ) ) )
                {
#ifndef _DEBUG
                    Com_Printf( "^3WARNING: profile.pid found for profile '%s' - system settings will revert to defaults\n",
                                cl_profileStr );
                    // ydnar: set crashed state
                    cmdBufferSystem->AddText( "set com_crashed 1\n" );
#endif
                }
                
                // bani - write a new one
                if( !Com_WriteProfile( va( "profiles/%s/profile.pid", cl_profileStr ) ) )
                {
                    Com_Printf( "^3WARNING: couldn't write profiles/%s/profile.pid\n", cl_profileStr );
                }
                
                // exec the config
                cmdBufferSystem->AddText( va( "exec profiles/%s/%s\n", cl_profileStr, CONFIG_NAME ) );
            }
        }
        else
        {
            cmdBufferSystem->AddText( va( "exec %s\n", CONFIG_NAME ) );
        }
    }
    
    cmdBufferSystem->AddText( "exec autoexec.cfg\n" );
#endif
    
    // ydnar: reset crashed state
    cmdBufferSystem->AddText( "set com_crashed 0\n" );
    
    // execute the queued commands
    cmdBufferSystem->Execute();
    
    // override anything from the config files with command line args
    Com_StartupVariable( NULL );
    
#ifdef UPDATE_SERVER
    com_dedicated = cvarSystem->Get( "dedicated", "1", CVAR_LATCH, "description" );
#elif DEDICATED
    // TTimo: default to internet dedicated, not LAN dedicated
    com_dedicated = cvarSystem->Get( "dedicated", "2", CVAR_ROM, "description" );
    cvarSystem->CheckRange( com_dedicated, 1, 2, true );
#else
    com_dedicated = cvarSystem->Get( "dedicated", "0", CVAR_LATCH, "description" );
    cvarSystem->CheckRange( com_dedicated, 0, 2, true );
#endif
    // allocate the stack based hunk allocator
    Com_InitHunkMemory();
    
    // if any archived cvars are modified after this, we will trigger a writing
    // of the config file
    cvar_modifiedFlags &= ~CVAR_ARCHIVE;
    
    //
    // init commands and vars
    //
    
    com_logfile = cvarSystem->Get( "logfile", "0", CVAR_TEMP, "" );
    
    // Gordon: no need to latch this in ET, our recoil is framerate independant
    com_maxfps = cvarSystem->Get( "com_maxfps", "333", CVAR_ARCHIVE /*|CVAR_LATCH */, "description" );
//  com_blood = cvarSystem->Get ("com_blood", "1", CVAR_ARCHIVE, "description"); // Gordon: no longer used?

    com_developer = cvarSystem->Get( "developer", "0", CVAR_TEMP, "description" );
    
    com_timescale = cvarSystem->Get( "timescale", "1", CVAR_CHEAT | CVAR_SYSTEMINFO, "description" );
    com_fixedtime = cvarSystem->Get( "fixedtime", "0", CVAR_CHEAT, "description" );
    com_showtrace = cvarSystem->Get( "com_showtrace", "0", CVAR_CHEAT, "description" );
    com_dropsim = cvarSystem->Get( "com_dropsim", "0", CVAR_CHEAT, "description" );
    com_viewlog = cvarSystem->Get( "viewlog", "0", CVAR_CHEAT, "description" );
    com_speeds = cvarSystem->Get( "com_speeds", "0", 0, "description" );
    com_timedemo = cvarSystem->Get( "timedemo", "0", CVAR_CHEAT, "description" );
    com_cameraMode = cvarSystem->Get( "com_cameraMode", "0", CVAR_CHEAT, "description" );
    
    com_watchdog = cvarSystem->Get( "com_watchdog", "60", CVAR_ARCHIVE, "description" );
    com_watchdog_cmd = cvarSystem->Get( "com_watchdog_cmd", "", CVAR_ARCHIVE, "description" );
    
    cl_paused = cvarSystem->Get( "cl_paused", "0", CVAR_ROM, "description" );
    sv_paused = cvarSystem->Get( "sv_paused", "0", CVAR_ROM, "description" );
    com_sv_running = cvarSystem->Get( "sv_running", "0", CVAR_ROM, "description" );
    com_cl_running = cvarSystem->Get( "cl_running", "0", CVAR_ROM, "description" );
    com_buildScript = cvarSystem->Get( "com_buildScript", "0", 0, "description" );
    
    con_drawnotify = cvarSystem->Get( "con_drawnotify", "0", CVAR_CHEAT, "description" );
    
    com_introPlayed = cvarSystem->Get( "com_introplayed", "0", CVAR_ARCHIVE, "description" );
    com_ansiColor = cvarSystem->Get( "com_ansiColor", "0", CVAR_ARCHIVE, "description" );
    com_logosPlaying = cvarSystem->Get( "com_logosPlaying", "0", CVAR_ROM, "description" );
    com_recommendedSet = cvarSystem->Get( "com_recommendedSet", "0", CVAR_ARCHIVE, "description" );
    
    com_unfocused = cvarSystem->Get( "com_unfocused", "0", CVAR_ROM, "description" );
    com_minimized = cvarSystem->Get( "com_minimized", "0", CVAR_ROM, "description" );
    com_affinity = cvarSystem->Get( "com_affinity", "1", CVAR_ARCHIVE, "description" );
    com_maxfpsUnfocused = cvarSystem->Get( "com_maxfpsUnfocused", "0", CVAR_ARCHIVE, "description" );
    com_maxfpsMinimized = cvarSystem->Get( "com_maxfpsMinimized", "0", CVAR_ARCHIVE, "description" );
    com_abnormalExit = cvarSystem->Get( "com_abnormalExit", "0", CVAR_ROM, "description" );
    
    cvarSystem->Get( "savegame_loading", "0", CVAR_ROM, "description" );
    
#if defined( _WIN32 ) && defined( _DEBUG )
    com_noErrorInterrupt = cvarSystem->Get( "com_noErrorInterrupt", "0", 0, "description" );
#endif
    
    com_hunkused = cvarSystem->Get( "com_hunkused", "0", 0, "description" );
    com_hunkusedvalue = 0;
    
    if( com_dedicated->integer )
    {
        if( !com_viewlog->integer )
        {
            cvarSystem->Set( "viewlog", "0" );
        }
    }
    
    if( com_developer && com_developer->integer )
    {
        cmdSystem->AddCommand( "error", Com_Error_f, "description" );
        cmdSystem->AddCommand( "crash", Com_Crash_f, "description" );
        cmdSystem->AddCommand( "freeze", Com_Freeze_f, "description" );
    }
    cmdSystem->AddCommand( "quit", Com_Quit_f, "description" );
    cmdSystem->AddCommand( "writeconfig", Com_WriteConfig_f, "description" );
    
    s = va( "%s %s %s %s", Q3_VERSION, ARCH_STRING, OS_STRING, __DATE__ );
    com_version = cvarSystem->Get( "version", s, CVAR_ROM | CVAR_SERVERINFO, "description" );
    com_protocol = cvarSystem->Get( "protocol", va( "%i", ETPROTOCOL_VERSION ), CVAR_SERVERINFO | CVAR_ARCHIVE, "description" );
    
    idsystem->Init();
    
    if( idsystem->WritePIDFile( ) )
    {
#ifndef DEDICATED
        StringEntry message = "The last time " CLIENT_WINDOW_TITLE " ran, "
                              "it didn't exit properly. This may be due to inappropriate video "
                              "settings. Would you like to start with \"safe\" video settings?";
                              
        if( idsystem->Dialog( DT_YES_NO, message, "Abnormal Exit" ) == DR_YES )
        {
            cvarSystem->Set( "com_abnormalExit", "1" );
        }
#endif
    }
    
    Netchan_Init( Com_Milliseconds() & 0xffff );	// pick a port value that should be nice and random
    serverInitSystem->Init();
    Hist_Load();
    
    com_dedicated->modified = false;
    if( !com_dedicated->integer )
    {
        CL_Init();
    }
    
    // set com_frameTime so that if a map is started on the
    // command line it will still be able to count on com_frameTime
    // being random enough for a serverid
    com_frameTime = Com_Milliseconds();
    
    // add + commands from command line
    if( !Com_AddStartupCommands() )
    {
        // if the user didn't give any commands, run default action
    }
    
    CL_StartHunkUsers();
    
    // NERVE - SMF - force recommendedSet and don't do vid_restart if in safe mode
    if( !com_recommendedSet->integer && !safeMode )
    {
        Com_SetRecommended();
        cmdBufferSystem->ExecuteText( EXEC_APPEND, "vid_restart\n" );
    }
    cvarSystem->Set( "com_recommendedSet", "1" );
    
    if( !com_dedicated->integer && !Com_AddStartupCommands() )
    {
#if 1
        //Dushan turned this all off until someone create something better
        cvarSystem->Set( "com_logosPlaying", "1" );
        
        cmdBufferSystem->AddText( "cinematic splash.roq\n" );
        
        cvarSystem->Set( "nextmap", "cinematic avlogo.roq" );
        
        if( !com_introPlayed->integer )
        {
            cvarSystem->Set( com_introPlayed->name, "1" );
            cvarSystem->Set( "nextmap", "cinematic splash.roq" );
        }
#endif
    }
    
    com_fullyInitialized = true;
    
    Com_Printf( "--- Common Initialization Complete ---\n" );
}

//==================================================================

void Com_WriteConfigToFile( StringEntry filename )
{
    fileHandle_t    f;
    
    f = fileSystem->FOpenFileWrite( filename );
    if( !f )
    {
        Com_Printf( "Couldn't write %s.\n", filename );
        return;
    }
    
    fileSystem->Printf( f, "// generated by OpenWolf, do not modify\n" );
    fileSystem->Printf( f, "//\n" );
    fileSystem->Printf( f, "// Key Bindings\n" );
    fileSystem->Printf( f, "//\n" );
    Key_WriteBindings( f );
    fileSystem->Printf( f, "//\n" );
    fileSystem->Printf( f, "// Cvars\n" );
    fileSystem->Printf( f, "//\n" );
    cvarSystem->WriteVariables( f );
    fileSystem->Printf( f, "//\n" );
    fileSystem->Printf( f, "// Aliases\n" );
    fileSystem->Printf( f, "//\n" );
    cmdSystem->WriteAliases( f );
    
    //close file
    fileSystem->FCloseFile( f );
}


/*
===============
Com_WriteConfiguration

Writes key bindings and archived cvars to config file if modified
===============
*/
void Com_WriteConfiguration( void )
{
    UTF8*           cl_profileStr = cvarSystem->VariableString( "cl_profile" );
    
    // if we are quiting without fully initializing, make sure
    // we don't write out anything
    if( !com_fullyInitialized )
    {
        return;
    }
    
    if( !( cvar_modifiedFlags & CVAR_ARCHIVE ) )
    {
        return;
    }
    cvar_modifiedFlags &= ~CVAR_ARCHIVE;
    
    if( com_gameInfo.usesProfiles && cl_profileStr[0] )
    {
        Com_WriteConfigToFile( va( "profiles/%s/%s", cl_profileStr, CONFIG_NAME ) );
    }
    else
    {
        Com_WriteConfigToFile( CONFIG_NAME );
    }
}


/*
===============
Com_WriteConfig_f

Write the config file to a specific name
===============
*/
void Com_WriteConfig_f( void )
{
    UTF8            filename[MAX_QPATH];
    
    if( cmdSystem->Argc() != 2 )
    {
        Com_Printf( "Usage: writeconfig <filename>\n" );
        return;
    }
    
    Q_strncpyz( filename, cmdSystem->Argv( 1 ), sizeof( filename ) );
    COM_DefaultExtension( filename, sizeof( filename ), ".cfg" );
    Com_Printf( "Writing %s.\n", filename );
    Com_WriteConfigToFile( filename );
}

/*
================
Com_ModifyMsec
================
*/
S32 Com_ModifyMsec( S32 msec )
{
    S32             clampTime;
    
    //
    // modify time for debugging values
    //
    if( com_fixedtime->integer )
    {
        msec = com_fixedtime->integer;
    }
    else if( com_timescale->value )
    {
        msec *= com_timescale->value;
//  } else if (com_cameraMode->integer) {
//      msec *= com_timescale->value;
    }
    
    // don't let it scale below 1 msec
    if( msec < 1 && com_timescale->value )
    {
        msec = 1;
    }
    
    if( com_dedicated->integer )
    {
        // dedicated servers don't want to clamp for a much longer
        // period, because it would mess up all the client's views
        // of time.
        if( msec > 500 && msec < 500000 )
        {
            // hibernation mode cause this
            //if( !svs.hibernation.enabled )
            {
                Com_Printf( "Hitch warning: %i msec frame time\n", msec );
            }
        }
        clampTime = 5000;
    }
    else if( !com_sv_running->integer )
    {
        // clients of remote servers do not want to clamp time, because
        // it would skew their view of the server's time temporarily
        clampTime = 5000;
    }
    else
    {
        // for local single player gaming
        // we may want to clamp the time to prevent players from
        // flying off edges when something hitches.
        clampTime = 200;
    }
    
    if( msec > clampTime )
    {
        msec = clampTime;
    }
    
    return msec;
}

/*
=================
Com_Frame
=================
*/
void Com_Frame( void )
{
    S32             msec, minMsec;
    static S32      lastTime;
    S32             key;
    S32             timeBeforeFirstEvents;
    S32             timeBeforeServer;
    S32             timeBeforeEvents;
    S32             timeBeforeClient;
    S32             timeAfter;
    static S32      watchdogTime = 0;
    static bool		watchWarn = false;
    
    if( setjmp( abortframe ) )
    {
        return;					// an ERR_DROP was thrown
    }
    
    // bk001204 - init to zero.
    //  also:  might be clobbered by `longjmp' or `vfork'
    timeBeforeFirstEvents = 0;
    timeBeforeServer = 0;
    timeBeforeEvents = 0;
    timeBeforeClient = 0;
    timeAfter = 0;
    
    // old net chan encryption key
    key = 0x87243987;
    
    // Don't write config on Update Server
#if !defined (UPDATE_SERVER)
    // write config file if anything changed
    Com_WriteConfiguration();
#endif
    
    // if "viewlog" has been modified, show or hide the log console
    if( com_viewlog->modified )
    {
        com_viewlog->modified = false;
    }
    
    //
    // main event loop
    //
    if( com_speeds->integer )
    {
        timeBeforeFirstEvents = idsystem->Milliseconds();
    }
    
    // we may want to spin here if things are going too fast
    if( !com_dedicated->integer && !com_timedemo->integer )
    {
        if( com_minimized->integer && com_maxfpsMinimized->integer > 0 )
        {
            minMsec = 1000 / com_maxfpsMinimized->integer;
        }
        else if( com_unfocused->integer && com_maxfpsUnfocused->integer > 0 )
        {
            minMsec = 1000 / com_maxfpsUnfocused->integer;
        }
        else if( com_maxfps->integer > 0 )
        {
            minMsec = 1000 / com_maxfps->integer;
        }
        else
        {
            minMsec = 1;
        }
    }
    else
    {
        minMsec = 1;
    }
    
    msec = minMsec;
    do
    {
        S32 timeRemaining = minMsec - msec;
        
        // The existing idSystemLocal::Sleep implementations aren't really
        // precise enough to be of use beyond 100fps
        // FIXME: implement a more precise sleep (RDTSC or something)
        if( timeRemaining >= 10 )
            idsystem->Sleep( timeRemaining );
            
        com_frameTime = Com_EventLoop();
        if( lastTime > com_frameTime )
        {
            lastTime = com_frameTime;	// possible on first frame
        }
        msec = com_frameTime - lastTime;
    }
    while( msec < minMsec );
    
    cmdBufferSystem->Execute();
    cmdDelaySystem->Frame();
    
    lastTime = com_frameTime;
    
    // mess with msec if needed
    com_frameMsec = msec;
    msec = Com_ModifyMsec( msec );
    
    //
    // server side
    //
    if( com_speeds->integer )
    {
        timeBeforeServer = idsystem->Milliseconds();
    }
    
    serverMainSystem->Frame( msec );
    
    // if "dedicated" has been modified, start up
    // or shut down the client system.
    // Do this after the server may have started,
    // but before the client tries to auto-connect
    if( com_dedicated->modified )
    {
        // get the latched value
        cvarSystem->Get( "dedicated", "0", 0, "description" );
        com_dedicated->modified = false;
        if( !com_dedicated->integer )
        {
            serverInitSystem->Shutdown( "dedicated set to 0" );
            CL_FlushMemory();
        }
    }
    
    //
    // client system
    //
    if( !com_dedicated->integer )
    {
        //
        // run event loop a second time to get server to client packets
        // without a frame of latency
        //
        if( com_speeds->integer )
        {
            timeBeforeEvents = idsystem->Milliseconds();
        }
        Com_EventLoop();
        cmdBufferSystem->Execute();
        
        //
        // client side
        //
        if( com_speeds->integer )
        {
            timeBeforeClient = idsystem->Milliseconds();
        }
        
        CL_Frame( msec );
        
        if( com_speeds->integer )
        {
            timeAfter = idsystem->Milliseconds();
        }
    }
    else
    {
        timeAfter = idsystem->Milliseconds();
    }
    
    //
    // watchdog
    //
    if( com_dedicated->integer && !com_sv_running->integer && com_watchdog->integer )
    {
        if( watchdogTime == 0 )
        {
            watchdogTime = idsystem->Milliseconds();
        }
        else
        {
            if( !watchWarn && idsystem->Milliseconds() - watchdogTime > ( com_watchdog->integer - 4 ) * 1000 )
            {
                Com_Printf( "WARNING: watchdog will trigger in 4 seconds\n" );
                watchWarn = true;
            }
            else if( idsystem->Milliseconds() - watchdogTime > com_watchdog->integer * 1000 )
            {
                Com_Printf( "Idle Server with no map - triggering watchdog\n" );
                watchdogTime = 0;
                watchWarn = false;
                if( com_watchdog_cmd->string[0] == '\0' )
                {
                    cmdBufferSystem->AddText( "quit\n" );
                }
                else
                {
                    cmdBufferSystem->AddText( va( "%s\n", com_watchdog_cmd->string ) );
                }
            }
        }
    }
    
    NET_FlushPacketQueue();
    
    //
    // report timing information
    //
    if( com_speeds->integer )
    {
        S32             all, sv, sev, cev, cl;
        
        all = timeAfter - timeBeforeServer;
        sv = timeBeforeEvents - timeBeforeServer;
        sev = timeBeforeServer - timeBeforeFirstEvents;
        cev = timeBeforeClient - timeBeforeEvents;
        cl = timeAfter - timeBeforeClient;
        sv -= time_game;
        cl -= time_frontend + time_backend;
        
        Com_Printf( "frame:%i all:%3i sv:%3i sev:%3i cev:%3i cl:%3i gm:%3i rf:%3i bk:%3i\n",
                    com_frameNumber, all, sv, sev, cev, cl, time_game, time_frontend, time_backend );
    }
    
    //
    // trace optimization tracking
    //
    if( com_showtrace->integer )
    {
    
        extern S32 c_traces, c_brush_traces, c_patch_traces, c_trisoup_traces;
        extern S32 c_pointcontents;
        
        Com_Printf( "%4i traces  (%ib %ip %it) %4i points\n", c_traces, c_brush_traces, c_patch_traces, c_trisoup_traces, c_pointcontents );
        c_traces = 0;
        c_brush_traces = 0;
        c_patch_traces = 0;
        c_trisoup_traces = 0;
        c_pointcontents = 0;
    }
    
    com_frameNumber++;
}

/*
=================
Com_Shutdown
=================
*/
void Com_Shutdown( bool badProfile )
{
    UTF8* cl_profileStr = cvarSystem->VariableString( "cl_profile" );
    
    NET_Shutdown();
    
    collisionModelManager->ClearMap();
    
    if( logfile_ )
    {
        fileSystem->FCloseFile( logfile_ );
        logfile_ = 0;
        com_logfile->integer = 0;//don't open up the log file again!!
    }
    
    
    // write config file if anything changed
    Com_WriteConfiguration();
    
    // delete pid file
    if( com_gameInfo.usesProfiles && cl_profileStr[0] && !badProfile )
    {
        if( fileSystem->FileExists( va( "profiles/%s/profile.pid", cl_profileStr ) ) )
        {
            fileSystem->Delete( va( "profiles/%s/profile.pid", cl_profileStr ) );
        }
    }
    
    if( com_journalFile )
    {
        fileSystem->FCloseFile( com_journalFile );
        com_journalFile = 0;
    }
}

//------------------------------------------------------------------------

/*
===========================================
command line completion
===========================================
*/

/*
==================
Field_Clear
==================
*/
void Field_Clear( field_t* edit )
{
    memset( edit->buffer, 0, MAX_EDIT_LINE );
    edit->cursor = 0;
    edit->scroll = 0;
}

/*
==================
Field_Set
==================
*/
void Field_Set( field_t* edit, StringEntry content )
{
    memset( edit->buffer, 0, MAX_EDIT_LINE );
    strncpy( edit->buffer, content, MAX_EDIT_LINE );
    edit->cursor = strlen( edit->buffer );
    if( edit->cursor > edit->widthInChars )
    {
        edit->scroll = edit->cursor - edit->widthInChars;
    }
    else
    {
        edit->scroll = 0;
    }
}

/*
==================
Field_WordDelete
==================
*/
void Field_WordDelete( field_t* edit )
{
    while( edit->cursor )
    {
        if( edit->buffer[edit->cursor - 1] != ' ' )
        {
            edit->buffer[edit->cursor - 1] = 0;
            edit->cursor--;
        }
        else
        {
            edit->cursor--;
            if( edit->buffer[edit->cursor - 1] != ' ' )
                return;
        }
    }
}


StringEntry completionString;
static UTF8     shortestMatch[MAX_TOKEN_CHARS];
static S32      matchCount;

// field we are working on, passed to Field_CompleteCommand (&g_consoleCommand for instance)
static field_t* completionField;
static StringEntry completionPrompt;

/*
===============
FindMatches
===============
*/
static void FindMatches( StringEntry s )
{
    S32             i;
    
    if( Q_stricmpn( s, completionString, strlen( completionString ) ) )
    {
        return;
    }
    matchCount++;
    if( matchCount == 1 )
    {
        Q_strncpyz( shortestMatch, s, sizeof( shortestMatch ) );
        return;
    }
    
    // cut shortestMatch to the amount common with s
    // was wrong when s had fewer chars than shortestMatch
    i = 0;
    do
    {
        if( tolower( shortestMatch[i] ) != tolower( s[i] ) )
        {
            shortestMatch[i] = 0;
        }
    }
    while( s[i++] );
}


/*
===============
PrintMatches
===============
*/
static void PrintMatches( StringEntry s )
{
    if( !Q_stricmpn( s, shortestMatch, strlen( shortestMatch ) ) )
    {
        Com_Printf( "    %s\n", s );
    }
}

/*
===============
PrintCvarMatches
===============
*/
static void PrintCvarMatches( StringEntry s )
{
    if( !Q_stricmpn( s, shortestMatch, strlen( shortestMatch ) ) )
    {
        Com_Printf( "    %-32s ^7\"%s^7\"\n", s, cvarSystem->VariableString( s ) );
    }
}


/*
===============
Field_FindFirstSeparator
===============
*/
static UTF8* Field_FindFirstSeparator( UTF8* s )
{
    S32 i;
    
    for( i = 0; i < strlen( s ); i++ )
    {
        if( s[ i ] == ';' )
            return &s[ i ];
    }
    
    return NULL;
}

/*
===============
Field_Complete
===============
*/
static bool Field_Complete( void )
{
    S32 completionOffset;
    
    if( matchCount == 0 )
        return true;
        
    completionOffset = strlen( completionField->buffer ) - strlen( completionString );
    
    Q_strncpyz( &completionField->buffer[ completionOffset ], shortestMatch, sizeof( completionField->buffer ) - completionOffset );
    
    completionField->cursor = strlen( completionField->buffer );
    
    if( matchCount == 1 )
    {
        Q_strcat( completionField->buffer, sizeof( completionField->buffer ), " " );
        completionField->cursor++;
        return true;
    }
    
    Com_Printf( "%s^7%s\n", completionPrompt, completionField->buffer );
    
    return false;
}

#ifndef DEDICATED
/*
===============
Field_CompleteKeyname
===============
*/
void Field_CompleteKeyname( void )
{
    matchCount = 0;
    shortestMatch[ 0 ] = 0;
    
    Key_KeynameCompletion( FindMatches );
    
    if( !Field_Complete( ) )
        Key_KeynameCompletion( PrintMatches );
}

/*
===============
Field_CompleteCgame
===============
*/
void Field_CompleteCgame( S32 argNum )
{
    matchCount = 0;
    shortestMatch[ 0 ] = 0;
    
    clientGameSystem->CgameCompletion( FindMatches, argNum );
    
    if( !Field_Complete() )
    {
        clientGameSystem->CgameCompletion( PrintMatches, argNum );
    }
}
#endif

/*
===============
Field_CompleteFilename
===============
*/
void Field_CompleteFilename( StringEntry dir,
                             StringEntry ext, bool stripExt )
{
    matchCount = 0;
    shortestMatch[ 0 ] = 0;
    
    fileSystem->FilenameCompletion( dir, ext, stripExt, FindMatches );
    
    if( !Field_Complete( ) )
        fileSystem->FilenameCompletion( dir, ext, stripExt, PrintMatches );
}

/*
===============
Field_CompleteAlias
===============
*/
void Field_CompleteAlias( void )
{
    matchCount = 0;
    shortestMatch[ 0 ] = 0;
    
    cmdSystem->AliasCompletion( FindMatches );
    
    if( !Field_Complete( ) )
        cmdSystem->AliasCompletion( PrintMatches );
}

/*
===============
Field_CompleteDelay
===============
*/
void Field_CompleteDelay( void )
{
    matchCount = 0;
    shortestMatch[ 0 ] = 0;
    
    cmdSystem->DelayCompletion( FindMatches );
    
    if( !Field_Complete( ) )
        cmdSystem->DelayCompletion( PrintMatches );
}

/*
===============
Field_CompleteCommand
===============
*/
void Field_CompleteCommand( UTF8* cmd, bool doCommands, bool doCvars )
{
    S32 completionArgument = 0;
    
    // Skip leading whitespace and quotes
    cmd = Com_SkipCharset( cmd, " \"" );
    
    cmdSystem->TokenizeStringIgnoreQuotes( cmd );
    completionArgument = cmdSystem->Argc( );
    
    // If there is trailing whitespace on the cmd
    if( *( cmd + strlen( cmd ) - 1 ) == ' ' )
    {
        completionString = "";
        completionArgument++;
    }
    else
    {
        completionString = cmdSystem->Argv( completionArgument - 1 );
    }
    
#ifndef DEDICATED
    // Unconditionally add a '\' to the start of the buffer
    if( completionField->buffer[ 0 ] &&
            completionField->buffer[ 0 ] != '\\' )
    {
        if( completionField->buffer[ 0 ] != '/' )
        {
            // Buffer is full, refuse to complete
            if( strlen( completionField->buffer ) + 1 >= sizeof( completionField->buffer ) )
                return;
                
            memmove( &completionField->buffer[ 1 ],
                     &completionField->buffer[ 0 ],
                     strlen( completionField->buffer ) + 1 );
            completionField->cursor++;
        }
        
        completionField->buffer[ 0 ] = '\\';
    }
#endif
    
    if( completionArgument > 1 )
    {
        StringEntry baseCmd = cmdSystem->Argv( 0 );
        UTF8* p;
        
#ifndef DEDICATED
        // This should always be true
        if( baseCmd[ 0 ] == '\\' || baseCmd[ 0 ] == '/' )
            baseCmd++;
#endif
            
        if( ( p = Field_FindFirstSeparator( cmd ) ) )
            Field_CompleteCommand( p + 1, true, true ); // Compound command
        else
            cmdSystem->CompleteArgument( baseCmd, cmd, completionArgument );
    }
    else
    {
        if( completionString[0] == '\\' || completionString[0] == '/' )
            completionString++;
            
        matchCount = 0;
        shortestMatch[ 0 ] = 0;
        
        if( strlen( completionString ) == 0 )
            return;
            
        if( doCommands )
            cmdSystem->CommandCompletion( FindMatches );
            
        if( doCvars )
            cvarSystem->CommandCompletion( FindMatches );
            
        if( !Field_Complete( ) )
        {
            // run through again, printing matches
            if( doCommands )
                cmdSystem->CommandCompletion( PrintMatches );
                
            if( doCvars )
                cvarSystem->CommandCompletion( PrintCvarMatches );
        }
    }
}

/*
===============
Field_AutoComplete

Perform Tab expansion
===============
*/
void Field_AutoComplete( field_t* field, StringEntry prompt )
{
    completionField = field;
    completionPrompt = prompt;
    
    Field_CompleteCommand( completionField->buffer, true, true );
}

void Com_GetHunkInfo( S32* hunkused, S32* hunkexpected )
{
    *hunkused = com_hunkusedvalue;
    *hunkexpected = com_expectedhunkusage;
}

/*
==================
Com_RandomBytes

fills string array with len radom bytes, peferably from the OS randomizer
==================
*/
void Com_RandomBytes( U8* string, S32 len )
{
    S32 i;
    
    if( idsystem->RandomBytes( string, len ) )
    {
        return;
    }
    
    Com_Printf( "Com_RandomBytes: using weak randomization\n" );
    
    for( i = 0; i < len; i++ )
    {
        string[i] = ( U8 )( rand() % 255 );
    }
}

#define CON_HISTORY 64
#define CON_HISTORY_FILE "conhistory"
static UTF8 history[CON_HISTORY][MAX_EDIT_LINE];
static S32 hist_current, hist_next;

/*
==================
Hist_Load
==================
*/
void Hist_Load( void )
{
    S32 i;
    fileHandle_t f;
    UTF8* buf, *end;
    UTF8 buffer[sizeof( history )];
    
    fileSystem->SV_FOpenFileRead( CON_HISTORY_FILE, &f );
    if( !f )
    {
        Com_Printf( "Couldn't read %s.\n", CON_HISTORY_FILE );
        return;
    }
    fileSystem->Read( buffer, sizeof( buffer ), f );
    fileSystem->FCloseFile( f );
    
    buf = buffer;
    for( i = 0; i < CON_HISTORY; i++ )
    {
        end = strchr( buf, '\n' );
        if( !end )
        {
            end = buf + strlen( buf );
            Q_strncpyz( history[i], buf, sizeof( history[0] ) );
            break;
        }
        else
            *end = '\0';
        Q_strncpyz( history[i], buf, sizeof( history[0] ) );
        buf = end + 1;
        if( !*buf )
            break;
    }
    
    if( i > CON_HISTORY )
        i = CON_HISTORY;
    hist_current = hist_next = i + 1;
}

/*
==================
Hist_Save
==================
*/
static void Hist_Save( void )
{
    S32 i;
    fileHandle_t f;
    
    f = fileSystem->SV_FOpenFileWrite( CON_HISTORY_FILE );
    if( !f )
    {
        Com_Printf( "Couldn't write %s.\n", CON_HISTORY_FILE );
        return;
    }
    
    i = hist_next % CON_HISTORY;
    do
    {
        UTF8* buf;
        if( !history[i][0] )
        {
            i = ( i + 1 ) % CON_HISTORY;
            continue;
        }
        buf = va( "%s\n", history[i] );
        fileSystem->Write( buf, strlen( buf ), f );
        i = ( i + 1 ) % CON_HISTORY;
    }
    while( i != ( hist_next - 1 ) % CON_HISTORY );
    
    fileSystem->FCloseFile( f );
}

/*
==================
Hist_Add
==================
*/
void Hist_Add( StringEntry field )
{
    if( !strcmp( field, history[( hist_current - 1 ) % CON_HISTORY] ) )
    {
        hist_current = hist_next;
        return;
    }
    
    Q_strncpyz( history[hist_next % CON_HISTORY], field, sizeof( history[0] ) );
    hist_next++;
    hist_current = hist_next;
    Hist_Save();
}

/*
==================
Hist_Prev
==================
*/
StringEntry Hist_Prev( void )
{
    if( ( hist_current - 1 ) % CON_HISTORY != hist_next % CON_HISTORY &&
            history[( hist_current - 1 ) % CON_HISTORY][0] )
        hist_current--;
    return history[hist_current % CON_HISTORY];
}

/*
==================
Hist_Next
==================
*/
StringEntry Hist_Next( void )
{
    if( hist_current % CON_HISTORY != hist_next % CON_HISTORY )
        hist_current++;
    if( hist_current % CON_HISTORY == hist_next % CON_HISTORY )
        return NULL;
    return history[hist_current % CON_HISTORY];
}
