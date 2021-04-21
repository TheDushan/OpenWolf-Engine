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
// File name:   common.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: misc functions used in client and server
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.hpp>
#elif DEDICATED
#include <null/null_serverprecompiled.hpp>
#else
#include <framework/precompiled.hpp>
#endif

sint demo_protocols[] = { 1, 0 };

#define MAX_NUM_ARGVS   50

#define MIN_DEDICATED_COMHUNKMEGS 64
#define MIN_COMHUNKMEGS 64              // JPW NERVE changed this to 42 for MP, was 56 for team arena and 75 for wolfSP
#define DEF_COMHUNKMEGS 512         // RF, increased this, some maps are exceeding 56mb 
// JPW NERVE changed this for multiplayer back to 42, 56 for depot/mp_cpdepot, 42 for everything else
#define DEF_COMZONEMEGS 64              // RF, increased this from 16, to account for botlib/AAS
#define DEF_COMHUNKMEGS_S   XSTRING(DEF_COMHUNKMEGS)
#define DEF_COMZONEMEGS_S   XSTRING(DEF_COMZONEMEGS)

sint             com_argc;
valueType           *com_argv[MAX_NUM_ARGVS + 1];

jmp_buf
abortframe;     // an ERR_DROP occured, exit the entire frame

static fileHandle_t logfile_;
fileHandle_t    com_journalFile;    // events are written here
fileHandle_t    com_journalDataFile;    // config files are written here

convar_t         *com_crashed =
    nullptr;    // ydnar: set in case of a crash, prevents CVAR_UNSAFE variables from being set from a cfg

//bani - explicit nullptr to make win32 teh happy

convar_t         *com_ignorecrash =
    nullptr;    // bani - let experienced users ignore crashes, explicit nullptr to make win32 teh happy
convar_t         *com_pid;      // bani - process id

convar_t         *com_viewlog;
convar_t         *com_speeds;
convar_t         *com_developer;
convar_t         *com_dedicated;
convar_t         *com_timescale;
convar_t         *com_fixedtime;
convar_t         *com_dropsim;  // 0.0 to 1.0, simulated packet drops
convar_t         *com_journal;
convar_t         *com_timedemo;
convar_t         *com_sv_running;
convar_t         *com_cl_running;
convar_t         *com_showtrace;
convar_t         *com_version;
convar_t *com_logfile;      // 1 = buffer log, 2 = flush after each print
//convar_t    *com_blood;
convar_t         *com_buildScript;  // for automated data building scripts
convar_t         *con_drawnotify;
convar_t         *com_ansiColor;

convar_t         *com_unfocused;
convar_t         *com_minimized;

convar_t         *com_introPlayed;
convar_t         *cl_paused;
convar_t         *sv_paused;
#if defined (DEDICATED)
convar_t       *cl_packetdelay;
#endif
//convar_t         *sv_packetdelay;
convar_t         *com_cameraMode;
convar_t         *com_maxfpsUnfocused;
convar_t         *com_maxfpsMinimized;
convar_t         *com_abnormalExit;

convar_t         *com_recommendedSet;

convar_t         *com_watchdog;
convar_t         *com_watchdog_cmd;

// Rafael Notebook
convar_t         *cl_notebook;

convar_t         *com_hunkused; // Ridah
convar_t         *com_protocol;

#ifndef DEDICATED
convar_t *con_autochat;
#endif

// com_speeds times
sint             time_game;
sint             time_frontend; // renderer frontend time
sint             time_backend;  // renderer backend time

sint             com_frameTime;
sint             com_frameMsec;
sint             com_frameNumber;
sint             com_expectedhunkusage;
sint             com_hunkusedvalue;

bool            com_errorEntered = false;
bool            com_fullyInitialized = false;

valueType            com_errorMessage[MAXPRINTMSG];
void Com_WriteConfiguration(void);
void            Com_WriteConfig_f(void);

//============================================================================

static valueType    *rd_buffer;
static uint64 rd_buffersize;
static bool rd_flushing = false;
static void (*rd_flush)(valueType *buffer);

void Com_BeginRedirect(valueType *buffer, uint64 buffersize,
                       void (*flush)(valueType *)) {
    if(!buffer || !buffersize || !flush) {
        return;
    }

    rd_buffer = buffer;
    rd_buffersize = buffersize;
    rd_flush = flush;

    *rd_buffer = 0;
}

void Com_EndRedirect(void) {
    if(rd_flush) {
        rd_flushing = true;
        rd_flush(rd_buffer);
        rd_flushing = false;
    }

    rd_buffer = nullptr;
    rd_buffersize = 0;
    rd_flush = nullptr;
}

/*
=============
Com_Printf

Both client and server can use this, and it will output
to the apropriate place.

A raw string should NEVER be passed as fmt, because of "%f" type crashers.
=============
*/
void Com_Printf(pointer fmt, ...) {
    va_list argptr;
    valueType msg[MAXPRINTMSG];
    static bool opening_qconsole = false;

    va_start(argptr, fmt);
    Q_vsprintf_s(msg, sizeof(msg) - 1, fmt, argptr);
    va_end(argptr);

    if(rd_buffer && !rd_flushing) {
        if((strlen(msg) + strlen(rd_buffer)) > (rd_buffersize - 1)) {
            rd_flushing = true;
            rd_flush(rd_buffer);
            rd_flushing = false;
            *rd_buffer = 0;
        }

        Q_strcat(rd_buffer, rd_buffersize, msg);
        // show_bug.cgi?id=51
        // only flush the rcon buffer when it's necessary, avoid fragmenting
        //rd_flush(rd_buffer);
        //*rd_buffer = 0;

        return;
    }

    // echo to console if we're not a dedicated server
    if(com_dedicated && !com_dedicated->integer) {
        CL_ConsolePrint(msg);
    }

    // echo to dedicated console and early console
    idsystem->Print(msg);

    // logfile
    if(com_logfile && com_logfile->integer) {
        // TTimo: only open the qconsole.log if the filesystem is in an initialized state
        //   also, avoid recursing in the qconsole.log opening (i.e. if fs_debug is on)
        if(!logfile_ && fileSystem->Initialized() && !opening_qconsole) {
            struct tm *newtime;
            time_t aclock;

            opening_qconsole = true;

            time(&aclock);
            newtime = localtime(&aclock);

            if(com_logfile->integer != 3) {
                logfile_ = fileSystem->FOpenFileWrite("owconsole.log");
            } else {
                logfile_ = fileSystem->FOpenFileAppend("owconsole.log");
            }

            if(logfile_) {
                Com_Printf("logfile opened on %s\n", asctime(newtime));

                if(com_logfile->integer > 1) {
                    // force it to not buffer so we get valid
                    // data even if we are crashing
                    fileSystem->ForceFlush(logfile_);
                }
            } else {
                Com_Printf("Opening qconsole.log failed!\n");
                cvarSystem->SetValue("logfile", 0);
            }
        }

        opening_qconsole = false;

        if(logfile_ && fileSystem->Initialized()) {
            fileSystem->Write(msg, strlen(msg), logfile_);
        }
    }
}

void Com_FatalError(pointer error, ...) {
    va_list argptr;
    valueType msg[8192];

    va_start(argptr, error);
    Q_vsprintf_s(msg, sizeof(msg), error, argptr);
    va_end(argptr);

    Com_Error(ERR_FATAL, msg);
}

void Com_DropError(pointer error, ...) {
    va_list argptr;
    valueType msg[8192];

    va_start(argptr, error);
    Q_vsprintf_s(msg, sizeof(msg), error, argptr);
    va_end(argptr);

    Com_Error(ERR_DROP, msg);
}

void Com_Warning(pointer error, ...) {
    va_list argptr;
    valueType msg[8192];

    va_start(argptr, error);
    Q_vsprintf_s(msg, sizeof(msg), error, argptr);
    va_end(argptr);

    Com_Printf(msg);
}



/*
================
Com_DPrintf

A Com_Printf that only shows up if the "developer" cvar is set
================
*/
void Com_DPrintf(pointer fmt, ...) {
    va_list         argptr;
    valueType            msg[MAXPRINTMSG];

    if(!com_developer || com_developer->integer != 1) {
        return;                 // don't confuse non-developers with techie stuff...
    }

    va_start(argptr, fmt);
    Q_vsprintf_s(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);

    Com_Printf("%s", msg);
}

/*
=============
Com_Error

Both client and server can use this, and it will
do the apropriate things.
=============
*/
// *INDENT-OFF*
void Com_Error( sint code, pointer fmt, ... )
{
    va_list         argptr;
    static sint      lastErrorTime;
    static sint      errorCount;
    sint             currentTime;

    if( com_errorEntered )
    {
        idsystem->Error( "recursive error after: %s", com_errorMessage );
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
    Q_vsprintf_s( com_errorMessage, sizeof( com_errorMessage ), fmt, argptr );
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
        serverInitSystem->Shutdown( "Server disconnected" );

#ifndef DEDICATED
        CL_Disconnect( true, nullptr );
#endif

        CL_FlushMemory();

        // make sure we can get at our local stuff
        fileSystem->PureServerSetLoadedPaks( "", "" );

        com_errorEntered = false;
        longjmp( abortframe, -1 );
    }
    else if( code == ERR_DROP )
    {
        Com_Printf( "********************\nERROR: %s\n********************\n", com_errorMessage );

        serverInitSystem->Shutdown( va( "Server crashed: %s\n", com_errorMessage ) );

#ifndef DEDICATED
        CL_Disconnect( true, "Server crashed" );
#endif
        CL_FlushMemory();

        // make sure we can get at our local stuff
        fileSystem->PureServerSetLoadedPaks( "", "" );

        com_errorEntered = false;
        longjmp( abortframe, -1 );
    }
#ifndef DEDICATED
    else if( code == ERR_AUTOUPDATE )
    {
        serverInitSystem->Shutdown( "Autoupdate server disconnected" );

#ifndef DEDICATED
        CL_Disconnect( true, "Autoupdate server crashed" );
#endif

        CL_FlushMemory();

        // make sure we can get at our local stuff
        fileSystem->PureServerSetLoadedPaks( "", "" );

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

        cmdSystem->Shutdown();
        cvarSystem->Shutdown();
        fileSystem->Shutdown( true );
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
sint             com_numConsoleLines;
valueType*           com_consoleLines[MAX_CONSOLE_LINES];

/*
==================
Com_ParseCommandLine

Break it up into multiple console lines
==================
*/
void Com_ParseCommandLine( valueType* commandLine )
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
    sint             i;

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
If match is not nullptr, only that cvar will be looked for.
That is necessary because cddir and basedir need to be set
before the filesystem is started, but all other sets shouls
be after execing the config and default.
===============
*/
void Com_StartupVariable( pointer match )
{
    sint             i;
    valueType*           s;

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
    sint             i;
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

void Info_Print( pointer s )
{
    valueType key[BIG_INFO_VALUE];
    valueType value[BIG_INFO_VALUE];
    valueType* o;
    sint64 l;

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
sint Com_Filter( valueType* filter, valueType* name, sint casesensitive )
{
    valueType            buf[MAX_TOKEN_CHARS];
    valueType*           ptr;
    sint             i, found;

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
sint Com_FilterPath( valueType* filter, valueType* name, sint casesensitive )
{
    sint             i;
    valueType            new_filter[MAX_QPATH];
    valueType            new_name[MAX_QPATH];

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
sint Com_RealTime( qtime_t* qtime )
{
    time_t          t;
    struct tm*      tms;

    t = time( nullptr );
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
    valueType* label;
    valueType* file;
    sint line;
    uint64 allocSize;
} zonedebug_t;

typedef struct memblock_s
{
    uint64 size;		// including the header and possibly tiny fragments
    sint tag;		// a tag of 0 is a free block
    struct memblock_s* next, *prev;
    sint id;			// should be ZONEID
#ifdef ZONE_DEBUG
    zonedebug_t     d;
#endif
} memblock_t;

typedef struct
{
    uint64 size;		// total bytes malloced, including header
    uint64 used;		// total bytes used
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
void Z_ClearZone( memzone_t* zone, uint64 size )
{
    memblock_t*     block;

    // set the entire zone to one free block

    zone->blocklist.next = zone->blocklist.prev = block = ( memblock_t* )( reinterpret_cast<uchar8*>( zone ) + sizeof( memzone_t ) );
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
        Com_Error( ERR_DROP, "Z_Free: nullptr pointer" );
    }

    block = ( memblock_t* )( reinterpret_cast<uchar8*>( ptr ) - sizeof( memblock_t ) );
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
    if( *reinterpret_cast<sint*>( ( reinterpret_cast<uchar8*>( block ) + block->size - 4 ) ) != ZONEID )
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
    sint             count;
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
            Z_Free( reinterpret_cast<memzone_t*>( zone->rover ) + 1 );
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
void*           Z_TagMallocDebug( uint64 size, memtag_t tag, valueType* label, valueType* file, sint line )
{
#else
void*           Z_TagMalloc( uint64 size, memtag_t tag )
{
#endif
    sint allocSize;
    sint64 extra;
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
    size = PAD( size, sizeof( sint64 ) );	// align to 32/64 bit boundary

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
            return nullptr;
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
        _new = ( memblock_t* )( reinterpret_cast<uchar8*>( base ) + size );
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
    * reinterpret_cast<sint*>( reinterpret_cast<uchar8*>( base ) + base->size - 4 ) = ZONEID;

    return reinterpret_cast< void* >( reinterpret_cast<uchar8*>( base ) + sizeof( memblock_t ) );
}

/*
========================
Z_Malloc
========================
*/
#ifdef ZONE_DEBUG
void*           Z_MallocDebug( uint64 size, valueType* label, valueType* file, sint line )
{
#else
void*           Z_Malloc( uint64 size )
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
void*           S_MallocDebug( uint64 size, valueType* label, valueType* file, sint line )
{
    return Z_TagMallocDebug( size, TAG_SMALL, label, file, line );
}
#else
void*           S_Malloc( uint64 size )
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
        if( reinterpret_cast<uchar8*>( block ) + block->size != reinterpret_cast<uchar8*>( block->next ) )
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
void Z_LogZoneHeap( memzone_t* zone, valueType* name )
{
#ifdef ZONE_DEBUG
    valueType            dump[32], *ptr;
    sint             i, j;
#endif
    memblock_t*     block;
    valueType            buf[4096];
    uint64 size, allocSize;
    sint numBlocks;

    if( !logfile_ || !fileSystem->Initialized() )
    {
        return;
    }
    size = allocSize = numBlocks = 0;
    Q_vsprintf_s( buf, sizeof( buf ), sizeof( buf ), "\r\n================\r\n%s log\r\n================\r\n", name );
    fileSystem->Write( buf, strlen( buf ), logfile_ );
    for( block = zone->blocklist.next; block->next != &zone->blocklist; block = block->next )
    {
        if( block->tag )
        {
#ifdef ZONE_DEBUG
            ptr = ( reinterpret_cast<valueType*>( block ) + sizeof( memblock_t ) );
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
            Q_vsprintf_s( buf, sizeof( buf ), sizeof( buf ), "size = %8d: %s, line: %d (%s) [%s]\r\n", block->d.allocSize, block->d.file,
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
    Q_vsprintf_s( buf, sizeof( buf ), sizeof( buf ), "%d %s memory in %d blocks\r\n", size, name, numBlocks );
    fileSystem->Write( buf, strlen( buf ), logfile_ );
    Q_vsprintf_s( buf, sizeof( buf ), sizeof( buf ), "%d %s memory overhead\r\n", size - allocSize, name );
    fileSystem->Write( buf, strlen( buf ), logfile_ );
}

/*
========================
Z_AvailableZoneMemory
========================
*/
static uint64 Z_AvailableZoneMemory( const memzone_t* zone )
{
    return zone->size - zone->used;
}

/*
========================
Z_AvailableMemory
========================
*/
uint64 Z_AvailableMemory( void )
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
    uchar8            mem[2];
} memstatic_t;

// bk001204 - initializer brackets
memstatic_t     emptystring = { {( sizeof( memblock_t ) + 2 + 3 )& ~3, TAG_STATIC, nullptr, nullptr, ZONEID}
    , {'\0', '\0'}
};
memstatic_t     numberstring[] =
{
    {   {( sizeof( memstatic_t ) + 3 )& ~3, TAG_STATIC, nullptr, nullptr, ZONEID}
        , {'0', '\0'}
    }
    ,
    {   {( sizeof( memstatic_t ) + 3 )& ~3, TAG_STATIC, nullptr, nullptr, ZONEID}
        , {'1', '\0'}
    }
    ,
    {   {( sizeof( memstatic_t ) + 3 )& ~3, TAG_STATIC, nullptr, nullptr, ZONEID}
        , {'2', '\0'}
    }
    ,
    {   {( sizeof( memstatic_t ) + 3 )& ~3, TAG_STATIC, nullptr, nullptr, ZONEID}
        , {'3', '\0'}
    }
    ,
    {   {( sizeof( memstatic_t ) + 3 )& ~3, TAG_STATIC, nullptr, nullptr, ZONEID}
        , {'4', '\0'}
    }
    ,
    {   {( sizeof( memstatic_t ) + 3 )& ~3, TAG_STATIC, nullptr, nullptr, ZONEID}
        , {'5', '\0'}
    }
    ,
    {   {( sizeof( memstatic_t ) + 3 )& ~3, TAG_STATIC, nullptr, nullptr, ZONEID}
        , {'6', '\0'}
    }
    ,
    {   {( sizeof( memstatic_t ) + 3 )& ~3, TAG_STATIC, nullptr, nullptr, ZONEID}
        , {'7', '\0'}
    }
    ,
    {   {( sizeof( memstatic_t ) + 3 )& ~3, TAG_STATIC, nullptr, nullptr, ZONEID}
        , {'8', '\0'}
    }
    ,
    {   {( sizeof( memstatic_t ) + 3 )& ~3, TAG_STATIC, nullptr, nullptr, ZONEID}
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
valueType* CopyString( pointer in )
{
    valueType*           out;

    if( !in[0] )
    {
        return ( reinterpret_cast<valueType*>( &emptystring ) ) + sizeof( memblock_t );
    }
    else if( !in[1] )
    {
        if( in[0] >= '0' && in[0] <= '9' )
        {
            return ( reinterpret_cast<valueType*>( &numberstring[in[0] - '0'] ) ) + sizeof( memblock_t );
        }
    }
    out = reinterpret_cast<valueType*>( S_Malloc( strlen( in ) + 1 ) );
    ::strcpy( out, in );
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
    sint             magic;
    uint64             size;
} hunkHeader_t;

typedef struct
{
    uint64             mark;
    uint64             permanent;
    uint64             temp;
    uint64             tempHighwater;
} hunkUsed_t;

typedef struct hunkblock_s
{
    uint64             size;
    uchar8            printed;
    struct hunkblock_s* next;
    valueType*           label;
    valueType*           file;
    sint             line;
} hunkblock_t;

static struct
{
    hunkblock_t* blocks;

    uchar8*	mem, *original;
    uint64	memSize;

    uint64	permTop, permMax;
    uint64	tempTop, tempMax;

    uint64	maxEver;

    uint64	mark;
} s_hunk;

static hunkblock_t* hunkblocks;

static hunkUsed_t hunk_low, hunk_high;
static hunkUsed_t* hunk_permanent, *hunk_temp;

static uchar8*    s_hunkData = nullptr;
static uint64      s_hunkTotal;

static uint64      s_zoneTotal;
static uint64      s_smallZoneTotal;

/*
=================
Com_Meminfo_f
=================
*/
void Com_Meminfo_f( void )
{
    memblock_t*	block;
    uint64			zoneBytes, zoneBlocks;
    uint64			smallZoneBytes, smallZoneBlocks;
    uint64			botlibBytes, rendererBytes, otherBytes;
    uint64			staticBytes, generalBytes;

    zoneBytes = 0;
    botlibBytes = 0;
    rendererBytes = 0;
    otherBytes = 0;
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
        if( reinterpret_cast<uchar8*>( block ) + block->size != reinterpret_cast<uchar8*>( block->next ) )
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
    sint		start, end;
    uint64		i, j;
    uint64		sum;
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
                sum += ( reinterpret_cast<sint*>( block ) )[i];
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
        Com_Error( ERR_FATAL, "Small zone data failed to allocate %1.1f megs", static_cast<float32>( s_smallZoneTotal ) / ( 1024 * 1024 ) );
    }
    Z_ClearZone( smallzone, s_smallZoneTotal );

    return;
}

void Com_InitZoneMemory( void )
{
    convar_t*	cv;

    // allocate the random block zone
    Com_StartupVariable( "com_zoneMegs" ); // config files and command line options haven't been taken in account yet
    cv = cvarSystem->Get( "com_zoneMegs", DEF_COMZONEMEGS_S, CVAR_INIT, "Sets the amount of memory reserved for the game." );

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
    valueType		buf[4096];
    uint64 size;
    sint numBlocks;

    if( !logfile_ || !fileSystem->Initialized() )
        return;
    size = 0;
    numBlocks = 0;
    Q_vsprintf_s( buf, sizeof( buf ), sizeof( buf ), "\r\n================\r\nHunk log\r\n================\r\n" );
    fileSystem->Write( buf, strlen( buf ), logfile_ );
    for( block = s_hunk.blocks; block; block = block->next )
    {
#ifdef HUNK_DEBUG
        Q_vsprintf_s( buf, sizeof( buf ), sizeof( buf ), "size = %8d: %s, line: %d (%s)\r\n", block->size, block->file, block->line, block->label );
        fileSystem->Write( buf, strlen( buf ), logfile_ );
#endif
        size += block->size;
        numBlocks++;
    }
    Q_vsprintf_s( buf, sizeof( buf ), sizeof( buf ), "%d Hunk memory\r\n", size );
    fileSystem->Write( buf, strlen( buf ), logfile_ );
    Q_vsprintf_s( buf, sizeof( buf ), sizeof( buf ), "%d hunk blocks\r\n", numBlocks );
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
    valueType		buf[4096];
    uint64 size, locsize;
    sint numBlocks;

    if( !logfile_ || !fileSystem->Initialized() )
        return;
    for( block = s_hunk.blocks ; block; block = block->next )
    {
        block->printed = false;
    }
    size = 0;
    numBlocks = 0;
    Q_vsprintf_s( buf, sizeof( buf ), sizeof( buf ), "\r\n================\r\nHunk Small log\r\n================\r\n" );
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
        Q_vsprintf_s( buf, sizeof( buf ), sizeof( buf ), "size = %8d: %s, line: %d (%s)\r\n", locsize, block->file, block->line, block->label );
        fileSystem->Write( buf, strlen( buf ), logfile_ );
#endif
        size += block->size;
        numBlocks++;
    }
    Q_vsprintf_s( buf, sizeof( buf ), sizeof( buf ), "%d Hunk memory\r\n", size );
    fileSystem->Write( buf, strlen( buf ), logfile_ );
    Q_vsprintf_s( buf, sizeof( buf ), sizeof( buf ), "%d hunk blocks\r\n", numBlocks );
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
    sint nMinAlloc;
    valueType* pMsg = nullptr;

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
    cv = cvarSystem->Get( "com_hunkMegs", DEF_COMHUNKMEGS_S, CVAR_LATCH | CVAR_ARCHIVE, "Sets the amount of memory reserved for the game, including com_soundMegs and com_zoneMeg" );

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
    s_hunk.original = static_cast<uchar8*>( calloc( s_hunk.memSize + 63, 1 ) );
    if( !s_hunk.original )
    {
        Com_Error( ERR_FATAL, "Hunk data failed to allocate %i megs", s_hunk.memSize / ( 1024 * 1024 ) );
    }
    // cacheline align
    s_hunk.mem = reinterpret_cast<uchar8*>( ( ( sint64 )s_hunk.original + 63 ) & ~63 );

    Hunk_Clear();

    cmdSystem->AddCommand( "meminfo", Com_Meminfo_f, "Shows memory usage in the console" );
#ifdef ZONE_DEBUG
    cmdSystem->AddCommand( "zonelog", Z_LogHeap, "Showing zone log" );
#endif
#ifdef HUNK_DEBUG
    cmdSystem->AddCommand( "hunklog", Hunk_Log, "Showing hunk log" );
    cmdSystem->AddCommand( "hunksmalllog", Hunk_SmallLog, "Showing hunk small log" );
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
uint64 Hunk_MemoryRemaining( void )
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

Used before for bots
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
    clientCinemaSystem->CloseAllVideos();
#endif

    s_hunk.permTop = 0;
    s_hunk.permMax = 0;
    s_hunk.tempTop = 0;
    s_hunk.tempMax = 0;
    s_hunk.maxEver = 0;
    s_hunk.mark = 0;

    Com_Printf( "Hunk_Clear: reset the hunk ok\n" );

#ifdef HUNK_DEBUG
    s_hunk.blocks = nullptr;
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
void* Hunk_AllocDebug( uint64 size, ha_pref preference, valueType* label, valueType* file, sint line )
{
#else
void* Hunk_Alloc( uint64 size, ha_pref preference )
{
#endif
    void*	buf;

    if( s_hunk.mem == nullptr )
    {
        Com_Error( ERR_FATAL, "Hunk_Alloc: Hunk memory system not initialized" );
    }

#ifdef HUNK_DEBUG
    size += sizeof( hunkblock_t );
#endif

    // round to cacheline
    size = ( size + 63 ) & ~63;

    if( s_hunk.permTop + s_hunk.tempTop + size > s_hunk.memSize )
    {
#ifdef HUNK_DEBUG
        Hunk_Log();
        Hunk_SmallLog();

        Com_Error( ERR_DROP, "Hunk_Alloc failed on %i: %s, line: %d (%s)", size, file, line, label );
#else
        Com_Error( ERR_DROP, "Hunk_Alloc failed on %i", size );
#endif
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
        buf = ( reinterpret_cast<uchar8*>( buf ) ) + sizeof( hunkblock_t );
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
void* Hunk_AllocateTempMemory( uint64 size )
{
    void*		buf;
    hunkHeader_t*	hdr;

    // return a Z_Malloc'd block if the hunk has not been initialized
    // this allows the config and product id files ( journal files too ) to be loaded
    // by the file system without redunant routines in the file system utilizing different
    // memory systems
    if( s_hunk.mem == nullptr )
    {
        return Z_Malloc( size );
    }

    Hunk_SwapBanks();

    size = PAD( size, sizeof( sint64 ) ) + sizeof( hunkHeader_t );

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
    buf = reinterpret_cast<void*>( hdr + 1 );

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
    if( reinterpret_cast<uchar8*>( hdr ) == s_hunk.mem + s_hunk.memSize - s_hunk.tempTop )
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

static uchar8* s_frameStackLoc = 0;
static uchar8* s_frameStackBase = 0;
static uchar8* s_frameStackEnd = 0;

static void Hunk_FrameInit( void )
{
    sint megs = cvarSystem->Get( "com_hunkFrameMegs", "1", CVAR_LATCH | CVAR_ARCHIVE, "Sets the amount of memory reserved for the game" )->integer;
    uint32 cb;

    if( megs < 1 )
        megs = 1;

    cb = 1024 * 1024 * megs;

    s_frameStackBase = static_cast<uchar8*>( Hunk_Alloc( cb, h_low ) );
    s_frameStackEnd = s_frameStackBase + cb;

    s_frameStackLoc = s_frameStackBase;
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
static sint      com_pushedEventsHead = 0;
static sint      com_pushedEventsTail = 0;

// bk001129 - static
static sysEvent_t com_pushedEvents[MAX_PUSHED_EVENTS];

/*
=================
Com_InitJournaling
=================
*/
void Com_InitJournaling( void )
{
    sint i;

    Com_StartupVariable( "journal" );
    com_journal = cvarSystem->Get( "journal", "0", CVAR_INIT, "Use in command line to record 'demo' of everything you do in application. '+set journal 1' to record; 2 for playback. journaldata.dat & journal.dat are the files it creates, they get very large quickly. Files will also store cfgs loaded." );
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
            valueType f[MAX_OSPATH];
            Q_vsprintf_s( f, sizeof( f ), sizeof( f ), "journal_%04d.dat", i );
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
static sint         eventHead = 0;
static sint         eventTail = 0;
static uchar8        sys_packetReceived[ MAX_MSGLEN ];

/*
================
Com_QueueEvent

A time of 0 will get the current time
Ptr should either be null, or point to a block of data that can
be freed by the game later.
================
*/
void Com_QueueEvent( sint time, sysEventType_t type, sint value, sint value2, sint ptrLength, void* ptr )
{
    sysEvent_t*  ev;

    // combine mouse movement with previous mouse event
    if( type == SYSE_MOUSE && eventHead != eventTail )
    {
        ev = &eventQueue[( eventHead + MAX_QUEUED_EVENTS - 1 ) & MASK_QUEUED_EVENTS];

        if( ev->evType == SYSE_MOUSE )
        {
            ev->evValue += value;
            ev->evValue2 += value2;
            return;
        }
    }

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
    valueType*        s;
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
        valueType*  b;
        uint64 len;

        len = strlen( s ) + 1;
        b = static_cast<valueType*>( Z_Malloc( len ) );
        Q_strncpyz( b, s, len );
        Com_QueueEvent( 0, SYSE_CONSOLE, 0, 0, len, b );
    }

    // check for network packets
    MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );
    adr.type = NA_UNSPEC;
    if( networkSystem->GetPacket( &adr, &netmsg ) )
    {
        netadr_t*  buf;
        uint64 len;

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
    sint			r;
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
    static sint      printedWarning = 0;	// bk001129 - init, bk001204 - explicit sint

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
    sint             t1, t2, msec;

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

sint Com_EventLoop( void )
{
    sysEvent_t      ev;
    netadr_t        evFrom;
    static uchar8       bufData[MAX_MSGLEN];
    msg_t           buf;

    MSG_Init( &buf, bufData, sizeof( bufData ) );

    while( 1 )
    {
        ev = Com_GetEvent();

        // if no more events are available
        if( ev.evType == SYSE_NONE )
        {
            // manually send packet events for the loopback channel
            while( networkChainSystem->GetLoopPacket( NS_CLIENT, &evFrom, &buf ) )
            {
                CL_PacketEvent( evFrom, &buf );
            }

            while( networkChainSystem->GetLoopPacket( NS_SERVER, &evFrom, &buf ) )
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
                if( ( static_cast<valueType*>( ev.evPtr ) )[0] == '\\' || ( static_cast<valueType*>( ev.evPtr ) )[0] == '/' )
                {
                    cmdBufferSystem->AddText( static_cast<valueType*>( ev.evPtr ) + 1 );
                }
                else
                {
                    cmdBufferSystem->AddText( static_cast<valueType*>( ev.evPtr ) );
                }
                break;
            case SYSE_PACKET:
                // this cvar allows simulation of connections that
                // drop a lot of packets.  Note that loopback connections
                // don't go through here at all.
                if( com_dropsim->value > 0 )
                {
                    static sint      seed;

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
                if( static_cast<uint>( buf.cursize ) > buf.maxsize )
                {
                    Com_Printf( "Com_EventLoop: oversize packet\n" );
                    continue;
                }
                memcpy( buf.data, reinterpret_cast<uchar8*>( ( netadr_t* ) ev.evPtr + 1 ), buf.cursize );
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
sint Com_Milliseconds( void )
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
    float32           s;
    sint             start, now;

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
    *( volatile sint* )0 = 0x12345678;
}

void Com_SetRecommended( void )
{
    convar_t* r_highQualityVideo, *com_recommended;
    bool goodVideo;

    // will use this for recommended settings as well.. do i outside the lower check so it gets done even with command line stuff
    r_highQualityVideo = cvarSystem->Get( "r_highQualityVideo", "1", CVAR_ARCHIVE, "Setting high quality settings" );
    com_recommended = cvarSystem->Get( "com_recommended", "-1", CVAR_ARCHIVE, "Setting the recommended settings" );
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
    valueType*           f, *buf;
    valueType*           token;

    memset( &com_gameInfo, 0, sizeof( com_gameInfo ) );

    if( fileSystem->ReadFile( "gameinfo.dat", ( void** )&f ) > 0 )
    {
        buf = f;

        while( ( token = COM_Parse( &buf ) ) != nullptr && token[0] )
        {
            if( !Q_stricmp( token, "spEnabled" ) )
            {
                com_gameInfo.spEnabled = true;
            }
            else if( !Q_stricmp( token, "spGameTypes" ) )
            {
                while( ( token = COM_ParseExt( &buf, false ) ) != nullptr && token[0] )
                {
                    com_gameInfo.spGameTypes |= ( 1 << atoi( token ) );
                }
            }
            else if( !Q_stricmp( token, "defaultSPGameType" ) )
            {
                if( ( token = COM_ParseExt( &buf, false ) ) != nullptr && token[0] )
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

                while( ( token = COM_ParseExt( &buf, false ) ) != nullptr && token[0] )
                {
                    com_gameInfo.coopGameTypes |= ( 1 << atoi( token ) );
                }
            }
            else if( !Q_stricmp( token, "defaultCoopGameType" ) )
            {
                if( ( token = COM_ParseExt( &buf, false ) ) != nullptr && token[0] )
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
                if( ( token = COM_ParseExt( &buf, false ) ) != nullptr && token[0] )
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
                if( ( token = COM_ParseExt( &buf, false ) ) != nullptr && token[0] )
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
bool Com_CheckProfile( valueType* profile_path )
{
    fileHandle_t    f;
    valueType            f_data[32];
    sint             f_pid;

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
extern valueType     fs_gamedir[MAX_OSPATH];
valueType            last_fs_gamedir[MAX_OSPATH];
valueType            last_profile_path[MAX_OSPATH];

//bani - track profile changes, delete old profile.pid if we change fs_game(dir)
//hackish, we fiddle with fs_gamedir to make fileSystem->* calls work "right"
void Com_TrackProfile( valueType* profile_path )
{
    valueType            temp_fs_gamedir[MAX_OSPATH];

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
bool Com_WriteProfile( valueType* profile_path )
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
void Com_Init( valueType* commandLine )
{
    valueType*           s;
    sint             pid;

    // TTimo gcc warning: variable `safeMode' might be clobbered by `longjmp' or `vfork'
    volatile bool safeMode = true;

    Com_Printf( "%s %s %s\n%s\n", PRODUCT_NAME, PLATFORM_STRING, __DATE__, commandLine );

    if( setjmp( abortframe ) )
    {
        idsystem->Error( "Error during initialization" );
    }

    // Clear queues
    ::memset( &eventQueue[0], 0, MAX_QUEUED_EVENTS * sizeof( sysEvent_t ) );
    ::memset( &sys_packetReceived[0], 0, MAX_MSGLEN * sizeof( uchar8 ) );

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
    Com_StartupVariable( nullptr );

    // get the developer cvar set as early as possible
    Com_StartupVariable( "developer" );

    // bani: init this early
    Com_StartupVariable( "com_ignorecrash" );
    com_ignorecrash = cvarSystem->Get( "com_ignorecrash", "0", 0, "Tells the client override unsafe cvars that result from crash (often also from running modifications as ET didnt delete pid file)" );

    // ydnar: init crashed variable as early as possible
    com_crashed = cvarSystem->Get( "com_crashed", "0", CVAR_TEMP, "Force a error for development reasons but never really trialed this." );

    // bani: init pid
#ifdef _WIN32
    pid = GetCurrentProcessId();
#else
    pid = getpid();
#endif
    s = va( "%d", pid );
    com_pid = cvarSystem->Get( "com_pid", s, CVAR_ROM, "Process id" );

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
        valueType*           cl_profileStr = cvarSystem->VariableString( "cl_profile" );

        safeMode = false;
        if( com_gameInfo.usesProfiles )
        {
            if( !cl_profileStr[0] )
            {
                valueType*           defaultProfile = nullptr;

                fileSystem->ReadFile( "profiles/defaultprofile.dat", ( void** )&defaultProfile );

                if( defaultProfile )
                {
                    valueType*           text_p = defaultProfile;
                    valueType*           token = COM_Parse( &text_p );

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
    Com_StartupVariable( nullptr );

#ifdef UPDATE_SERVER
    com_dedicated = cvarSystem->Get( "dedicated", "1", CVAR_LATCH, "Sets server type: 1 dedicated LAN, 2 dedicated internet, 0 listen (play & serve)" );
#elif DEDICATED
    // TTimo: default to internet dedicated, not LAN dedicated
    com_dedicated = cvarSystem->Get( "dedicated", "2", CVAR_ROM, "Sets server type: 1 dedicated LAN, 2 dedicated internet, 0 listen (play & serve)" );
    cvarSystem->CheckRange( com_dedicated, 1, 2, true );
#else
    com_dedicated = cvarSystem->Get( "dedicated", "0", CVAR_LATCH, "Sets server type: 1 dedicated LAN, 2 dedicated internet, 0 listen (play & serve)" );
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

    com_logfile = cvarSystem->Get( "logfile", "0", CVAR_TEMP, "Toggles saving a logfile" );

    // Gordon: no need to latch this in ET, our recoil is framerate independant
//  com_blood = cvarSystem->Get ("com_blood", "1", CVAR_ARCHIVE, "Enable blood mist effects."); // Gordon: no longer used?

    com_developer = cvarSystem->Get( "developer", "0", CVAR_TEMP, "Enable/disable (1/0) developer mode, allows cheats and so on." );

    com_timescale = cvarSystem->Get( "timescale", "1", CVAR_CHEAT | CVAR_SYSTEMINFO, "Increase to fast forward through demos, or decrease for slow motion(fraction of 1)." );
    com_fixedtime = cvarSystem->Get( "fixedtime", "0", CVAR_CHEAT, "Toggle the rendering of every frame. The game will wait until each frame is completely rendered before sending the next frame." );
    com_showtrace = cvarSystem->Get( "com_showtrace", "0", CVAR_CHEAT, "Toggle display of packet traces. 0=disables,1=toggles." );
    com_dropsim = cvarSystem->Get( "com_dropsim", "0", CVAR_CHEAT, "For testing simulates packet loss during communication drops" );
    com_viewlog = cvarSystem->Get( "viewlog", "0", CVAR_CHEAT, "Toggle the display of the startup console window over the game screen" );
    com_speeds = cvarSystem->Get( "com_speeds", "0", 0, "Toggle display of frame counter, all, sv, cl, gm, rf, and bk whatever they are" );
    com_timedemo = cvarSystem->Get( "timedemo", "0", CVAR_CHEAT, "When set to 1 times a demo and returns frames per second like a benchmark" );
    com_cameraMode = cvarSystem->Get( "com_cameraMode", "0", CVAR_CHEAT, "Seems to toggle the view of your player model off and on when in 3D camera view " );

    com_watchdog = cvarSystem->Get( "com_watchdog", "60", CVAR_ARCHIVE, "Watchdog checks for a map not being loaded" );
    com_watchdog_cmd = cvarSystem->Get( "com_watchdog_cmd", "", CVAR_ARCHIVE, "Sets what to do when watchdog finds a map is not loaded" );

    cl_paused = cvarSystem->Get( "cl_paused", "0", CVAR_ROM, "Variable holds the status of the paused flag on the client side" );
    sv_paused = cvarSystem->Get( "sv_paused", "0", CVAR_ROM, "Allow the game to be paused from the server console?" );
    com_sv_running = cvarSystem->Get( "sv_running", "0", CVAR_ROM, "Variable flag tells the console whether or not a local server is running" );
    com_cl_running = cvarSystem->Get( "cl_running", "0", CVAR_ROM, "Variable which shows whether or not a client game is running or whether we are in server/client mode (read only)" );
    com_buildScript = cvarSystem->Get( "com_buildScript", "0", 0, "Automated data building scripts" );

    con_drawnotify = cvarSystem->Get( "con_drawnotify", "0", CVAR_CHEAT, "Draws the last few lines of output transparently over the game top." );

    com_introPlayed = cvarSystem->Get( "com_introplayed", "0", CVAR_ARCHIVE, "Whether or not the intro for the game has been played." );
    com_ansiColor = cvarSystem->Get( "com_ansiColor", "0", CVAR_ARCHIVE, "Enable use of ANSI escape codes in the tty" );
    com_recommendedSet = cvarSystem->Get( "com_recommendedSet", "0", CVAR_ARCHIVE, "For determining what the recommended performance settings are, non-user." );

    com_unfocused = cvarSystem->Get( "com_unfocused", "0", CVAR_ROM, "Automatically toggled when the game window is unfocused." );
    com_minimized = cvarSystem->Get( "com_minimized", "0", CVAR_ROM, "Automatically toggled when the game window is minimized." );
    com_maxfpsUnfocused = cvarSystem->Get( "com_maxfpsUnfocused", "0", CVAR_ARCHIVE, "Maximum frames per second when unfocused" );
    com_maxfpsMinimized = cvarSystem->Get( "com_maxfpsMinimized", "0", CVAR_ARCHIVE, "Maximum frames per second when minimized" );
    com_abnormalExit = cvarSystem->Get( "com_abnormalExit", "0", CVAR_ROM, "As Application crashed it will start with safe video settings" );

    com_hunkused = cvarSystem->Get( "com_hunkused", "0", 0, "Tells you the amound of hunk currently being used." );
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
        cmdSystem->AddCommand( "error", Com_Error_f, "Just throw a fatal error to test error shutdown procedures" );
        cmdSystem->AddCommand( "crash", Com_Crash_f, "A way to force a bus error for development reasons" );
        cmdSystem->AddCommand( "freeze", Com_Freeze_f, "Just freeze in place for a given number of seconds to test error recovery" );
    }
    cmdSystem->AddCommand( "quit", Com_Quit_f, "Quits the running game, and exits application completely. For server also see killserver" );
    cmdSystem->AddCommand( "writeconfig", Com_WriteConfig_f, "Saves all current settings to the specified file, if none specified then uses owconfig.cfg" );

    s = va( "%s %s %s %s", PRODUCT_NAME, OS_STRING, OS_STRING, __DATE__ );
    com_version = cvarSystem->Get( "version", s, CVAR_ROM | CVAR_SERVERINFO, "Records all info about the application version: build number, build date, win/linux etc" );
    com_protocol = cvarSystem->Get( "protocol", va( "%i", PROTOCOL_VERSION ), CVAR_SERVERINFO | CVAR_ARCHIVE, "Returns the current protocol (changes with patches)." );

#ifndef DEDICATED
    con_autochat = cvarSystem->Get( "con_autochat", "1", CVAR_ARCHIVE, "Set to 0 to disable sending console input text as chat when there is not a slash at the beginning." );
#endif

    idsystem->Init();

    if( idsystem->WritePIDFile( ) )
    {
#ifndef DEDICATED
        pointer message = "The last time " CLIENT_WINDOW_TITLE " ran, "
                          "it didn't exit properly. This may be due to inappropriate video "
                          "settings. Would you like to start with \"safe\" video settings?";

        if( idsystem->Dialog( DT_YES_NO, message, "Abnormal Exit" ) == DR_YES )
        {
            cvarSystem->Set( "com_abnormalExit", "1" );
        }
#endif
    }

    // pick a port value that should be nice and random
    networkChainSystem->Init( Com_Milliseconds() & 0xffff );

    serverInitSystem->Init();

    consoleHistorySystem->Load();

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

    if( !com_dedicated->integer && !Com_AddStartupCommands() )
    {
#if 1
        //Dushan turned this all off until someone create something better
        cmdBufferSystem->AddText( "cinematic splash.roq\n" );
#if 0
        cvarSystem->Set( "nextmap", "cinematic avlogo.roq" );

        if( !com_introPlayed->integer )
        {
            cvarSystem->Set( com_introPlayed->name, "1" );
            cvarSystem->Set( "nextmap", "cinematic splash.roq" );
        }
#endif
#endif
    }

    com_fullyInitialized = true;

    Com_Printf( "--- Common Initialization Complete ---\n" );
}

//==================================================================

void Com_WriteConfigToFile( pointer filename )
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
    valueType*           cl_profileStr = cvarSystem->VariableString( "cl_profile" );

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
    valueType            filename[MAX_QPATH];

    Q_strncpyz( filename, cmdSystem->Argv( 1 ), sizeof( filename ) );
    COM_DefaultExtension( filename, sizeof( filename ), ".cfg" );

    if( cmdSystem->Argc() != 2 )
    {
        Com_Printf( "Usage: writeconfig <filename>\n" );
        return;
    }

    Com_Printf( "Writing %s.\n", filename );
    Com_WriteConfigToFile( filename );
}

/*
================
Com_ModifyMsec
================
*/
sint Com_ModifyMsec( sint msec )
{
    sint             clampTime;

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
    sint             msec, minMsec;
    static sint      lastTime;
    sint             key;
    sint             timeBeforeFirstEvents;
    sint             timeBeforeServer;
    sint             timeBeforeEvents;
    sint             timeBeforeClient;
    sint             timeAfter;
    static sint      watchdogTime = 0;
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
        sint timeRemaining = minMsec - msec;

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
        cvarSystem->Get( "dedicated", "0", 0, "Sets server type: 1 dedicated LAN, 2 dedicated internet, 0 listen (play & serve)" );
        com_dedicated->modified = false;
        if( !com_dedicated->integer )
        {
            CL_Init();
        }
        else
        {
            CL_Shutdown();
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

    networkChainSystem->FlushPacketQueue();

    //
    // report timing information
    //
    if( com_speeds->integer )
    {
        sint             all, sv, sev, cev, cl;

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

        extern sint c_traces, c_brush_traces, c_patch_traces, c_trisoup_traces;
        extern sint c_pointcontents;

        Com_Printf( "%4i traces  (%ib %ip %it) %4i points\n", c_traces, c_brush_traces, c_patch_traces, c_trisoup_traces, c_pointcontents );
        c_traces = 0;
        c_brush_traces = 0;
        c_patch_traces = 0;
        c_trisoup_traces = 0;
        c_pointcontents = 0;
    }

    com_frameNumber++;

    //reset the frame memory stack
    Hunk_FrameReset();
}

/*
=================
Com_Shutdown
=================
*/
void Com_Shutdown( bool badProfile )
{
    valueType* cl_profileStr = cvarSystem->VariableString( "cl_profile" );

    networkSystem->Shutdown();

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

void Com_GetHunkInfo( sint* hunkused, sint* hunkexpected )
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
void Com_RandomBytes( uchar8* string, sint len )
{
    sint i;

    if( idsystem->RandomBytes( string, len ) )
    {
        return;
    }

    Com_Printf( "Com_RandomBytes: using weak randomization\n" );

    for( i = 0; i < len; i++ )
    {
        string[i] = static_cast<uchar8>( rand() % 255 );
    }
}
