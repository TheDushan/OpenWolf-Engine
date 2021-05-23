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
#include <server/serverAutoPrecompiled.hpp>
#elif DEDICATED
#include <server/serverDedPrecompiled.hpp>
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

fileHandle_t com_logfile;
fileHandle_t com_journalFile;    // events are written here
fileHandle_t com_journalDataFile;    // config files are written here

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
    if(dedicated && !dedicated->integer) {
#if !defined (DEDICATED) && !defined (UPDATE_SERVER)
        clientConsoleSystem->ConsolePrint(msg);
#endif
    }

    Q_StripIndentMarker(msg);

    // echo to dedicated console and early console
    idsystem->Print(msg);

    // logfile
    if(logfile && logfile->integer) {
        // TTimo: only open the qconsole.log if the filesystem is in an initialized state
        //   also, avoid recursing in the qconsole.log opening (i.e. if fs_debug is on)
        if(!com_logfile && fileSystem->Initialized() && !opening_qconsole) {
            struct tm *newtime;
            time_t aclock;

            opening_qconsole = true;

            time(&aclock);
            newtime = localtime(&aclock);

            if(logfile->integer != 3) {
                com_logfile = fileSystem->FOpenFileWrite("owconsole.log");
            } else {
                com_logfile = fileSystem->FOpenFileAppend("owconsole.log");
            }

            if(logfile) {
                Com_Printf("logfile opened on %s\n", asctime(newtime));

                if(logfile->integer > 1) {
                    // force it to not buffer so we get valid
                    // data even if we are crashing
                    fileSystem->ForceFlush(com_logfile);
                }
            } else {
                Com_Printf("Opening qconsole.log failed!\n");
                cvarSystem->SetValue("logfile", 0);
            }
        }

        opening_qconsole = false;

        if(com_logfile && fileSystem->Initialized()) {
            fileSystem->Write(msg, strlen(msg), com_logfile);
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

    // ERR_DROP causes dedicated servers to enter an inoperable state
    // instead of crashing completely and being restarted.
    if (dedicated && dedicated->integer > 0)
    {
        code = ERR_FATAL;
    }

    va_start( argptr, fmt );
    Q_vsprintf_s( com_errorMessage, sizeof( com_errorMessage ), fmt, argptr );
    va_end( argptr );

    switch (code)
    {
        case ERR_SERVERDISCONNECT:
        {
            serverInitSystem->Shutdown("Server disconnected");

#ifndef DEDICATED
            clientConsoleCommandSystem->Disconnect(true, nullptr);
            clientMainSystem->FlushMemory();
#endif

            // make sure we can get at our local stuff
            fileSystem->PureServerSetLoadedPaks("", "");

            com_errorEntered = false;
            longjmp(abortframe, -1);
        }
        case ERR_FATAL:
        case ERR_DROP:
        {
            Com_Printf("********************\nERROR: %s\n********************\n", com_errorMessage);

            serverInitSystem->Shutdown(va("Server crashed: %s\n", com_errorMessage));
            idsystem->WriteDump("Debug Dump\nCom_Error: %s", com_errorMessage);
#ifndef DEDICATED
            clientConsoleCommandSystem->Disconnect(true, "Server crashed");
            clientMainSystem->FlushMemory();
#endif

            // make sure we can get at our local stuff
            fileSystem->PureServerSetLoadedPaks("", "");

            com_errorEntered = false;
            longjmp(abortframe, -1);
        }
#ifndef DEDICATED
        case ERR_AUTOUPDATE:
        {
            serverInitSystem->Shutdown("Autoupdate server disconnected");

#ifndef DEDICATED
            clientConsoleCommandSystem->Disconnect(true, "Autoupdate server crashed");
#endif

            clientMainSystem->FlushMemory();

            // make sure we can get at our local stuff
            fileSystem->PureServerSetLoadedPaks("", "");

            com_errorEntered = false;

            if (!Q_stricmpn(com_errorMessage, "Server is full", 14) && clientAutoUpdateSystem->NextUpdateServer())
            {
                clientAutoUpdateSystem->GetAutoUpdate();
            }
            else
            {
                longjmp(abortframe, -1);
            }
        }
#endif
        default:
        {
#ifndef DEDICATED
            clientMainSystem->Shutdown();
#endif
            serverInitSystem->Shutdown(va("Server fatal crashed: %s\n", com_errorMessage));
        }
    }

    Com_Shutdown( code == ERR_VID_FATAL ? true : false );

    idsystem->Error( "%s", com_errorMessage );
}


typedef struct com_color_defs_s
{
    char const *name;
    char const *code;
} com_color_defs_t;

static const com_color_defs_t color_definitions[] =
{
    {"Black", "0"},
    {"Red", "1"},
    {"Green", "2"},
    {"Yellow", "3"},
    {"Blue", "4"},
    {"Cyan", "5"},
    {"Magenta", "6"},
    {"White", "7"},
    {"Gray", "8"},
    {"Orange", "9"},
    {"Rose Bud", "a"},
    {"Pale Green", "b"},
    {"Pale Golden", "c"},
    {"Columbia Blue", "d"},
    {"Pale Turquoise", "e"},
    {"Pale Violet Red", "f"},
    {"Palace Pale White" , "g"},
    {"Olive", "h"},
    {"Tomato", "i"},
    {"Lime", "j"},
    {"Lemon", "k"},
    {"Blue Berry", "l"},
    {"Turquoise", "m"},
    {"Wild Watermelon", "n"},
    {"Saltpan", "o"},
    {"Gray Chateau", "p"},
    {"Rust", "q"},
    {"Copper Green", "r"},
    {"Gold", "s"},
    {"Steel Blue", "t"},
    {"Steel Gray", "u"},
    {"Bronze", "v"},
    {"Silver", "w"},
    {"Dark Gray", "x"},
    {"Dark Orange", "y"},
    {"Dark Green", "z"},
    {"Red Orange", "A"},
    {"Forest Green", "B"},
    {"Bright Sun", "C"},
    {"Medium Slate Blue", "D"},
    {"Celeste", "E"},
    {"Ironstone", "F"},
    {"Timberwolf", "G"},
    {"Onyx", "H"},
    {"Rosewood", "I"},
    {"Kokoda", "J"},
    {"Porsche", "K"},
    {"Cloud Burst", "L"},
    {"Blue Diane", "M"},
    {"Rope", "N"},
    {"Blonde", "O"},
    {"Smokey Black", "P"},
    {"American Rose", "Q"},
    {"Neon Green", "R"},
    {"Neon Yellow", "S"},
    {"Ultramarine", "T"},
    {"Turquoise Blue", "U"},
    {"Dark Magenta", "V"},
    {"Magic Mint", "W"},
    {"Light Gray", "X"},
    {"Light Salmon", "Y"},
    {"Light Green", "Z"},
};

static sint color_definitions_length = ARRAY_LEN(color_definitions);

/*
==============
Com_Colors_f
==============
*/
void Com_Colors_f(void) {
    sint row, i, j;

    Com_Printf("^3 %-20s %-6s %-20s %-6s^7\n\n", "Color", "Code", "Color", "Code");

    for(row = 0; row < ((sint)ceilf(((float32)color_definitions_length) / 2.0f)); row++) {
        for(i = (row * 2), j = 0; i < color_definitions_length && j < 2; i++, j++) {
            Com_Printf(
                " ^%s%-20s ^^%-5s",
                color_definitions[i].code,
                color_definitions[i].name,
                color_definitions[i].code);
        }
        Com_Printf("^7\n");
    }

    Com_Printf("\n^3 %-20s %-6s %-20s %-6s^7\n\n", "Color", "Code", "Color", "Code");

    Com_Printf("\n^3To escape from the color code escape and print ^5^^^3 as and ordinary character, use ^5^^^^^3.\n\n");
    Com_Printf("^3The format for the hardcoded standard color codes is ^5^^x^3, where x is alphanumeric (^7[^50-9^7][^5a-z^7][^5A-Z^7]^3).\n\n");
    Com_Printf("^3The short format for custom hexadecimal defined color codes is ^5^^#xxx^3, where x is a hexadecimal (^7[^50-9^7][^5a-f^7][^5A-F^7]^3).\n\n");
    Com_Printf("^3The long format for custom hexadecimal defined color codes (which has even more possible colors) is ^5^^##xxxxxx^3, where x is a hexadecimal (^7[^50-9^7][^5a-f^7][^5A-F^7]^3).^7\n\n\n");
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
        clientMainSystem->Shutdown();
#endif
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
    if (tms)
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
    else {
        ::memset(qtime, 0, sizeof(qtime_t));
    }
    return t;
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
    if( !journal->integer )
    {
        if(journal->string && journal->string[ 0 ] == '_' )
        {
            Com_Printf( "Replaying journaled events\n" );
            fileSystem->FOpenFileRead( va( "journal%s.dat", journal->string ), &com_journalFile, true );
            fileSystem->FOpenFileRead( va( "journal_data%s.dat", journal->string ), &com_journalDataFile, true );
            journal->integer = 2;
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

        if(journal->integer == 1 )
        {
            Com_Printf( "Journaling events\n" );
            com_journalFile		= fileSystem->FOpenFileWrite( va( "journal_%04d.dat", i ) );
            com_journalDataFile	= fileSystem->FOpenFileWrite( va( "journal_data_%04d.dat", i ) );
        }
        else if(journal->integer == 2 )
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

static pointer Com_ShowEventName(sysEventType_t eventType) {

    static pointer eventNames[SYSE_MAX] = {
        "SE_NONE",
        "SE_KEY",
        "SE_CHAR",
        "SE_MOUSE",
        "SE_JOYSTICK_AXIS",
        "SE_CONSOLE",
        "SE_PACKET"
    };

    if (eventType >= SYSE_MAX) {
        return "SE_UNKNOWN";
    }
    else {
        return eventNames[eventType];
    }
}


/*
================
Com_QueueEvent

A time of 0 will get the current time
Ptr should either be null, or point to a block of data that can
be freed by the game later.
================
*/
void Com_QueueEvent( sint evTime, sysEventType_t evType, sint value, sint value2, sint ptrLength, void* ptr )
{
    sysEvent_t*  ev;

    // combine mouse movement with previous mouse event
    if(evType == SYSE_MOUSE && eventHead != eventTail )
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
        Com_Printf("Com_ShowEventName(%s, time=%i): overflow\n", Com_ShowEventName(evType), evTime);

        // we are discarding an event, but don't leak memory
        if( ev->evPtr )
        {
            memorySystem->Free( ev->evPtr );
        }
        eventTail++;
    }

    eventHead++;

    if( time == 0 )
    {
        evTime = idsystem->Milliseconds();
    }

    ev->evTime = evTime;
    ev->evType = evType;
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
        b = static_cast<valueType*>( memorySystem->Malloc( len ) );
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
        buf = ( netadr_t* )memorySystem->Malloc( len );
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

#if defined (DEDICATED)
    idsystem->Sleep(1);
#endif

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
    if(journal->integer == 2 )
    {
        r = fileSystem->Read( &ev, sizeof( ev ), com_journalFile );
        if( r != sizeof( ev ) )
        {
            //Com_Error( ERR_FATAL, "Error reading from journal file" );
            journal->integer = 0;
            ev.evType = SYSE_NONE;
        }
        if( ev.evPtrLength )
        {
            ev.evPtr = memorySystem->Malloc( ev.evPtrLength );
            r = fileSystem->Read( ev.evPtr, ev.evPtrLength, com_journalFile );
            if( r != ev.evPtrLength )
            {
                //Com_Error( ERR_FATAL, "Error reading from journal file" );
                journal->integer = 0;
                ev.evType = SYSE_NONE;
            }
        }
    }
    else
    {
        ev = Com_GetSystemEvent();

        // write the journal value out if needed
        if(journal->integer == 1 )
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
            memorySystem->Free( ev->evPtr );
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
#if !defined (DEDICATED) && !defined (UPDATE_SERVER)
                clientMainSystem->PacketEvent( evFrom, &buf );
#endif
            }

            while( networkChainSystem->GetLoopPacket( NS_SERVER, &evFrom, &buf ) )
            {
                // if the server just shut down, flush the events
                if(sv_running->integer )
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
#if !defined (DEDICATED) && !defined (UPDATE_SERVER)
                clientKeysSystem->KeyEvent( ev.evValue, ev.evValue2, ev.evTime );
#endif
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

                clientKeysSystem->CharEvent( ev.evValue );
#endif
                break;
            case SYSE_MOUSE:
#if !defined (DEDICATED) && !defined (UPDATE_SERVER)
                clientInputSystem->MouseEvent( ev.evValue, ev.evValue2, ev.evTime );
#endif
                break;
            case SYSE_JOYSTICK_AXIS:
#if !defined (DEDICATED) && !defined (UPDATE_SERVER)
                clientInputSystem->JoystickEvent( ev.evValue, ev.evValue2, ev.evTime );
#endif
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
                if(sv_running->integer )
                {
                    Com_RunAndTimeServerPacket( &evFrom, &buf );
                }
                else
                {
#if !defined (DEDICATED) && !defined (UPDATE_SERVER)
                    clientMainSystem->PacketEvent( evFrom, &buf );
#endif
                }
                break;
        }

        // free any block data
        if( ev.evPtr )
        {
            memorySystem->Free( ev.evPtr );
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
    bool goodVideo;

    // will use this for recommended settings as well.. do i outside the lower check so it gets done even with command line stuff

    goodVideo = ( bool )(com_highSettings && com_highSettings->integer );

    if( goodVideo )
    {
        Com_Printf( "Found high quality video and high quality CPU\n" );
        cmdBufferSystem->AddText( "exec ultra.cfg\n" );
        cvarSystem->Set( "com_recommended", "2" );
    }
    else
    {
        Com_Printf( "Found low quality video and low quality CPU\n" );
        cmdBufferSystem->AddText( "exec low.cfg\n" );
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

    memorySystem->InitSmallZoneMemory();
    cvarSystem->Init();

    // prepare enough of the subsystems to handle
    // cvar and command buffer management
    Com_ParseCommandLine( commandLine );

    cmdBufferSystem->Init();

    memorySystem->InitZoneMemory();
    cmdSystem->Init();

    // override anything from the config files with command line args
    Com_StartupVariable( nullptr );

    // get the developer cvar set as early as possible
    Com_StartupVariable( "developer" );

    // bani: init this early
    Com_StartupVariable( "com_ignorecrash" );

    // done early so bind command exists
#if !defined (DEDICATED)
    clientKeysSystem->InitKeyCommands();
#endif

#ifdef _WIN32
    _setmaxstdio( 2048 );
#endif

    Com_InitCommonConsoleVars();
    cvarSystem->Set("developer", "0");

    fileSystem->InitFilesystem();

    Com_InitJournaling();

    Com_GetGameInfo();

#ifndef UPDATE_SERVER
    cmdBufferSystem->AddText( "exec default.cfg\n" );
    cmdBufferSystem->AddText( "exec language.lang\n" );	// NERVE - SMF

    // skip the q3config.cfg if "safe" is on the command line
    if( !Com_SafeMode() )
    {
        valueType* cl_profileStr = cl_profile->string;

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

                    cl_profileStr = cl_defaultProfile->string;
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

    // allocate the stack based hunk allocator
    memorySystem->InitHunkMemory();

    // if any archived cvars are modified after this, we will trigger a writing
    // of the config file
    cvar_modifiedFlags &= ~CVAR_ARCHIVE;

    com_hunkusedvalue = 0;

    if(developer && developer->integer )
    {
        cmdSystem->AddCommand( "error", Com_Error_f, "Just throw a fatal error to test error shutdown procedures" );
        cmdSystem->AddCommand( "crash", Com_Crash_f, "A way to force a bus error for development reasons" );
        cmdSystem->AddCommand( "freeze", Com_Freeze_f, "Just freeze in place for a given number of seconds to test error recovery" );
    }
    cmdSystem->AddCommand( "quit", Com_Quit_f, "Quits the running game, and exits application completely. For server also see killserver" );
    cmdSystem->AddCommand("colors", Com_Colors_f, "Displays help for working with chat colors");
    cmdSystem->AddCommand( "writeconfig", Com_WriteConfig_f, "Saves all current settings to the specified file, if none specified then uses owconfig.cfg" );

    s = va( "%s %s %s %s", PRODUCT_NAME, OS_STRING, OS_STRING, __DATE__ );

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
    networkChainSystem->Init( Com_Milliseconds() );

    serverInitSystem->Init();

    consoleHistorySystem->Load();

    dedicated->modified = false;
    if( !dedicated->integer )
    {
#ifndef DEDICATED
        clientMainSystem->Init();
#endif
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

#if !defined (DEDICATED) && !defined (UPDATE_SERVER)
    clientMainSystem->StartHunkUsers(false);
#endif

    if( !dedicated->integer && !Com_AddStartupCommands() )
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
#if !defined (DEDICATED) && !defined (UPDATE_SERVER)
    clientKeysSystem->WriteBindings( f );
#endif
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
    valueType* cl_profileStr = cl_profile->string;

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
    if(fixedtime->integer )
    {
        msec = fixedtime->integer;
    }
    else if(timescale->value )
    {
        msec *= timescale->value;
//  } else if (com_cameraMode->integer) {
//      msec *= timescale->value;
    }

    // don't let it scale below 1 msec
    if( msec < 1 && timescale->value )
    {
        msec = 1;
    }

    if(dedicated->integer )
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
    else if( !sv_running->integer )
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

    //
    // main event loop
    //
    if( com_speeds->integer )
    {
        timeBeforeFirstEvents = idsystem->Milliseconds();
    }

    // we may want to spin here if things are going too fast
    if( !dedicated->integer && !com_timedemo->integer )
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
    if(dedicated->modified )
    {
        // get the latched value
        cvarSystem->Get( "dedicated", "0", 0, "Sets server type: 1 dedicated LAN, 2 dedicated internet, 0 listen (play & serve)" );
        dedicated->modified = false;
        if( !dedicated->integer )
        {
#ifndef DEDICATED
            clientMainSystem->Init();
#endif
        }
        else
        {
#ifndef DEDICATED
            clientMainSystem->Shutdown();
#endif
        }
    }

    //
    // client system
    //
    if( !dedicated->integer )
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

#if !defined (DEDICATED) && !defined (UPDATE_SERVER)
        clientMainSystem->Frame( msec );
#endif

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
    if(dedicated->integer && !sv_running->integer && com_watchdog->integer )
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
    memorySystem->FrameReset();
}

/*
=================
Com_Shutdown
=================
*/
void Com_Shutdown( bool badProfile )
{
    valueType* cl_profileStr = cl_profile->string;

    networkSystem->Shutdown();

    collisionModelManager->ClearMap();

    if(com_logfile)
    {
        fileSystem->FCloseFile(com_logfile);
        com_logfile = 0;
        logfile->integer = 0;//don't open up the log file again!!
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

/*
==================
Com_rgb_to_hsl

Converts from the red green blue color space to hue saturation luminosity
==================
*/
void Com_rgb_to_hsl(vec4_t rgb, vec4_t hsl) {
    float32 max_color, min_color, chroma;

    //keep the alpha the same
    hsl[3] = rgb[3];

    max_color = MAX(MAX(rgb[0], rgb[1]), rgb[2]);
    min_color = MIN(MIN(rgb[0], rgb[1]), rgb[2]);
    chroma = max_color - min_color;

    //calc luminosity
    hsl[2] = (max_color + min_color) / 2;

    //calc hue and saturation
    if(chroma == 0) {
        //0 chroma means this color is grey, so hue and saturation are 0
        hsl[0] = 0;
        hsl[1] = 0;
    } else {
        if(max_color == rgb[0]) {
            hsl[0] = fmod(((rgb[1] - rgb[2]) / chroma), 6);
            if(hsl[0] < 0) {
                hsl[0] = (6 - fmod(fabs(hsl[0]), 6));
            }
        } else if(max_color == rgb[1]) {
            hsl[0] = (rgb[2] - rgb[0]) / chroma + 2;
        } else {
            hsl[0] = (rgb[0] - rgb[1]) / chroma + 4;
        }

        hsl[0] = hsl[0] / 6;
        hsl[1] = 1 - fabs(2 * hsl[2] - 1);
	}
}

/*
==================
Com_hsl_to_rgb

Converts from the hue saturation luminosity color space to red green blue
==================
*/
void Com_hsl_to_rgb(vec4_t hsl, vec4_t rgb) {

    //keep the alpha the same
    rgb[3] = hsl[3];

    if(hsl[1] == 0) {
        // 0 saturation means this color is grey
        VectorSet(rgb, hsl[2], hsl[2], hsl[2]);
    } else {
        float32 chroma, h_, x, m;

        chroma = (1 - fabs(2 * hsl[2] - 1)) * hsl[1];
        h_ = hsl[0] * 6;
        x = chroma * (1 - fabs((fmod(h_, 2)) - 1));
        m = hsl[2] - roundf((chroma/2) * 10000000000) / 10000000000.0;

        if(h_ >= 0 && h_ < 1) {
            VectorSet(rgb, (chroma + m), (x + m), m);
        } else if(h_ >= 1 && h_ < 2) {
            VectorSet(rgb, (x + m), (chroma + m), m);
        } else if(h_ >= 2 && h_ < 3) {
            VectorSet(rgb, m, (chroma + m), (x + m));
        } else if(h_ >= 3 && h_ < 4) {
            VectorSet(rgb, m, (x + m), (chroma + m));
        } else if(h_ >= 4 && h_ < 5) {
            VectorSet(rgb, (x + m), m, (chroma + m));
        } else if(h_ >= 5 && h_ < 6) {
            VectorSet(rgb, (chroma + m), m, (x + m));
        }
    }
}
