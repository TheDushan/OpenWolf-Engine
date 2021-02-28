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
// File name:   cl_console.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

sint g_console_field_width = 78;

#define CONSOLE_COLOR '7'
#define DEFAULT_CONSOLE_WIDTH   78
#define NUM_CON_TIMES 4
#define CON_TEXTSIZE 65536

typedef struct
{
    bool        initialized;
    
    schar16         text[CON_TEXTSIZE];
    sint         current;	// line where next message will be printed
    sint         x;			// offset in current line for next print
    sint         display;	// bottom of console displays this line
    
    sint         linewidth;	// characters across screen
    sint         totallines;	// total lines in console scrollback
    
    float32         xadjust;	// for wide aspect screens
    
    float32         displayFrac;	// aproaches finalFrac at scr_conspeed
    float32         finalFrac;	// 0.0 to 1.0 lines of console to display
    float32         desiredFrac;	// ydnar: for variable console heights
    
    sint         vislines;	// in scanlines
    
    sint         times[NUM_CON_TIMES];	// cls.realtime time the line was generated
    // for transparent notify lines
    vec4_t      color;
    
    sint          acLength;	// Arnout: autocomplete buffer length
} console_t;

console_t       con;

convar_t*         con_debug;
convar_t*         con_conspeed;
convar_t*         con_notifytime;
convar_t*         con_autoclear;

// Color and alpha for console
convar_t*		scr_conColorAlpha;
convar_t*		scr_conColorRed;
convar_t*		scr_conColorBlue;
convar_t*		scr_conColorGreen;

// Color and alpha for bar under console
convar_t*		scr_conBarHeight;

convar_t*		scr_conBarColorAlpha;
convar_t*		scr_conBarColorRed;
convar_t*		scr_conBarColorBlue;
convar_t*		scr_conBarColorGreen;

convar_t*		scr_conBarSize;
convar_t*		scr_conHeight;

// DHM - Nerve :: Must hold CTRL + SHIFT + ~ to get console
convar_t*         con_restricted;

vec4_t          console_highlightcolor = { 0.5, 0.5, 0.2, 0.45 };


/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f( void )
{
    con.acLength = 0;
    
    if( con_restricted->integer && ( !keys[K_CTRL].down || !keys[K_SHIFT].down ) )
    {
        return;
    }
    
    // ydnar: persistent console input is more useful
    // Arnout: added cvar
    if( con_autoclear->integer )
    {
        cmdCompletionSystem->Clear( &g_consoleField );
    }
    
    g_consoleField.widthInChars = g_console_field_width;
    
    Con_ClearNotify();
    
    // ydnar: multiple console size support
    if( cls.keyCatchers & KEYCATCH_CONSOLE )
    {
        cls.keyCatchers &= ~KEYCATCH_CONSOLE;
        con.desiredFrac = 0.0;
    }
    else
    {
        cls.keyCatchers |= KEYCATCH_CONSOLE;
        
        // schar16 console
        if( keys[K_CTRL].down )
        {
            con.desiredFrac = ( 5.0 * SMALLCHAR_HEIGHT ) / cls.glconfig.vidHeight;
        }
        // full console
        else if( keys[K_ALT].down )
        {
            con.desiredFrac = 1.0;
        }
        // normal half-screen console
        else
        {
            con.desiredFrac = 0.5;
        }
    }
}

/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f( void )
{
    sint i;
    
    chat_team = false;
    cmdCompletionSystem->Clear( &chatField );
    chatField.widthInChars = 30;
    
    for( i = 1; i < cmdSystem->Argc(); i++ )
    {
        Q_vsprintf_s( chatField.buffer, MAX_SAY_TEXT, MAX_SAY_TEXT, "%s%s ", chatField.buffer, cmdSystem->Argv( i ) );
    }
    chatField.cursor += strlen( chatField.buffer );
    
    cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f( void )
{
    sint i;
    
    chat_team = true;
    cmdCompletionSystem->Clear( &chatField );
    chatField.widthInChars = 25;
    
    for( i = 1; i < cmdSystem->Argc(); i++ )
    {
        Q_vsprintf_s( chatField.buffer, MAX_SAY_TEXT, MAX_SAY_TEXT, "%s%s ", chatField.buffer, cmdSystem->Argv( i ) );
    }
    chatField.cursor += strlen( chatField.buffer );
    
    cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode3_f
================
*/
void Con_MessageMode3_f( void )
{
    sint i;
    
    chat_team = false;
    chat_buddy = true;
    cmdCompletionSystem->Clear( &chatField );
    chatField.widthInChars = 26;
    
    for( i = 1; i < cmdSystem->Argc(); i++ )
    {
        Q_vsprintf_s( chatField.buffer, MAX_SAY_TEXT, MAX_SAY_TEXT, "%s%s ", chatField.buffer, cmdSystem->Argv( i ) );
    }
    chatField.cursor += strlen( chatField.buffer );
    cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

void Con_OpenConsole_f( void )
{
    if( !( cls.keyCatchers & KEYCATCH_CONSOLE ) )
    {
        Con_ToggleConsole_f();
    }
}

pointer Con_GetText( sint console )
{
    if( console >= 0 && con.text )
    {
        return ( valueType* )con.text;
    }
    else
    {
        return nullptr;
    }
}

/*
===================
Con_ToggleMenu_f
===================
*/
void Con_ToggleMenu_f( void )
{
    CL_KeyEvent( K_ESCAPE, true, idsystem->Milliseconds() );
    CL_KeyEvent( K_ESCAPE, false, idsystem->Milliseconds() );
}

/*
================
Con_Clear_f
================
*/
void Con_Clear_f( void )
{
    sint             i;
    
    for( i = 0; i < CON_TEXTSIZE; i++ )
    {
        con.text[i] = ( ColorIndex( CONSOLE_COLOR ) << 8 ) | ' ';
    }
    
    Con_Bottom();				// go to end
}

static sint dump_time;
static sint dump_count;
/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
void Con_Dump_f( void )
{
    sint             l, x, i;
    schar16*          line;
    fileHandle_t    f;
    sint		bufferlen;
    valueType*	buffer;
    valueType	filename[MAX_QPATH];
    valueType*	ss;
    sint		ilen, isub;
    valueType	name[MAX_QPATH];
    
    if( cmdSystem->Argc() < 1 || cmdSystem->Argc() > 2 || !Q_stricmp( cmdSystem->Argv( 1 ), "?" ) )
    {
        Com_Printf( "Usage", "condump", "<filename>" );
        return;
    }
    
    //name = Cmd_Argv( 1 );
    strncpy( name, cmdSystem->Argv( 1 ), sizeof( name ) );
    if( !strlen( name ) )
    {
        qtime_t	time;
        valueType*	count = ( dump_time == cls.realtime / 1000 ) ? va( "(%d)", dump_count++ + 2 ) : "";
        Com_RealTime( &time );
        
        Q_vsprintf_s( name, sizeof( name ), sizeof( name ), "condump%04d%02d%02d%02d%02d%02d%s",
                      time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec, count );
                      
        if( dump_time != cls.realtime / 1000 ) dump_count = 0;
        dump_time = cls.realtime / 1000;
    }
    
    ss = strstr( name, "logs/" );
    isub = ss ? strlen( ss ) : 0;
    ilen = strlen( name );
    
    if( ( ilen - isub ) != 0 )
    {
        Q_vsprintf_s( filename, sizeof( filename ), sizeof( filename ), "%s%s", "logs/", name );
    }
    else
        Q_strncpyz( filename, name, sizeof( filename ) );
        
    //Q_strncpyz( filename, name, sizeof(filename) );
    COM_DefaultExtension( filename, sizeof( filename ), ".txt" );
    
    f = fileSystem->FOpenFileWrite( filename );
    if( !f )
    {
        Com_Printf( "ERROR: couldn't open %s.\n", filename );
        return;
    }
    
    // skip empty lines
    for( l = con.current - con.totallines + 1; l <= con.current; l++ )
    {
        line = con.text + ( l % con.totallines ) * con.linewidth;
        for( x = 0; x < con.linewidth; x++ )
            if( ( line[x] & 0xff ) != ' ' )
                break;
        if( x != con.linewidth )
            break;
    }
    
#ifdef _WIN32
    bufferlen = con.linewidth + 3 * sizeof( valueType );
#else
    bufferlen = con.linewidth + 2 * sizeof( valueType );
#endif
    
    buffer = static_cast<valueType*>( Hunk_AllocateTempMemory( bufferlen ) );
    
    // write the remaining lines
    buffer[bufferlen - 1] = 0;
    for( ; l <= con.current; l++ )
    {
        line = con.text + ( l % con.totallines ) * con.linewidth;
        for( i = 0; i < con.linewidth; i++ )
            buffer[i] = line[i] & 0xff;
        for( x = con.linewidth - 1; x >= 0; x-- )
        {
            if( buffer[x] == ' ' )
                buffer[x] = 0;
            else
                break;
        }
#ifdef _WIN32
        Q_strcat( buffer, bufferlen, "\r\n" );
#else
        Q_strcat( buffer, bufferlen, "\n" );
#endif
        fileSystem->Write( buffer, strlen( buffer ), f );
    }
    
    Com_Printf( S_COLOR_YELLOW  "Dumped console text to " S_COLOR_RED "%s" S_COLOR_BLUE "." S_COLOR_WHITE "\n", filename );
    
    Hunk_FreeTempMemory( buffer );
    fileSystem->FCloseFile( f );
}

/*
================
Con_Search_f

Scroll up to the first console line containing a string
================
*/
void Con_Search_f( void )
{
    sint		l, i, x;
    schar16*	line;
    valueType	buffer[MAXPRINTMSG];
    sint		direction;
    sint		c = cmdSystem->Argc();
    
    if( c < 2 )
    {
        Com_Printf( "usage: %s <string1> <string2> <...>\n", cmdSystem->Argv( 0 ) );
        return;
    }
    
    if( !Q_stricmp( cmdSystem->Argv( 0 ), "searchDown" ) )
    {
        direction = 1;
    }
    else
    {
        direction = -1;
    }
    
    // check the lines
    buffer[con.linewidth] = 0;
    for( l = con.display - 1 + direction; l <= con.current && con.current - l < con.totallines; l += direction )
    {
        line = con.text + ( l % con.totallines ) * con.linewidth;
        for( i = 0; i < con.linewidth; i++ )
            buffer[i] = line[i] & 0xff;
        for( x = con.linewidth - 1 ; x >= 0 ; x-- )
        {
            if( buffer[x] == ' ' )
                buffer[x] = 0;
            else
                break;
        }
        // Don't search commands
        for( i = 1; i < c; i++ )
        {
            if( Q_stristr( buffer, cmdSystem->Argv( i ) ) )
            {
                con.display = l + 1;
                if( con.display > con.current )
                    con.display = con.current;
                return;
            }
        }
    }
}


/*
================
Con_Grep_f

Find all console lines containing a string
================
*/
void Con_Grep_f( void )
{
    sint		l, x, i;
    schar16*	line;
    valueType	buffer[1024];
    valueType	buffer2[1024];
    valueType	printbuf[CON_TEXTSIZE];
    valueType*	search;
    valueType	lastcolor;
    
    if( cmdSystem->Argc() != 2 )
    {
        Com_Printf( "usage: grep <string>\n" );
        return;
    }
    
    // skip empty lines
    for( l = con.current - con.totallines + 1 ; l <= con.current ; l++ )
    {
        line = con.text + ( l % con.totallines ) * con.linewidth;
        for( x = 0 ; x < con.linewidth ; x++ )
            if( ( line[x] & 0xff ) != ' ' )
                break;
        if( x != con.linewidth )
            break;
    }
    
    // check the remaining lines
    buffer[con.linewidth] = 0;
    search = cmdSystem->Argv( 1 );
    printbuf[0] = '\0';
    lastcolor = 7;
    for( ; l <= con.current ; l++ )
    {
        line = con.text + ( l % con.totallines ) * con.linewidth;
        for( i = 0, x = 0; i < con.linewidth; i++ )
        {
            if( line[i] >> 8 != lastcolor )
            {
                lastcolor = line[i] >> 8;
                buffer[x++] = Q_COLOR_ESCAPE;
                buffer[x++] = lastcolor + '0';
            }
            buffer[x++] = line[i] & 0xff;
        }
        for( x = con.linewidth - 1 ; x >= 0 ; x-- )
        {
            if( buffer[x] == ' ' )
                buffer[x] = 0;
            else
                break;
        }
        // Don't search commands
        Q_strcpy_s( buffer2, buffer );
        Q_CleanStr( buffer2 );
        
        if( Q_stristr( buffer2, search ) )
        {
            strcat( printbuf, buffer );
            strcat( printbuf, "\n" );
        }
    }
    if( printbuf[0] )
        Com_Printf( "%s", printbuf );
}



/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify( void )
{
    sint             i;
    
    for( i = 0; i < NUM_CON_TIMES; i++ )
    {
        con.times[i] = 0;
    }
}



/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize( void )
{
    sint i, j, width, oldwidth, oldtotallines, numlines, numchars;
    schar16	tbuf[CON_TEXTSIZE];
    
    if( cls.glconfig.vidWidth )
    {
        width = ( cls.glconfig.vidWidth - 30 ) / clientScreenSystem->ConsoleFontCharWidth( 'W' );
        
        g_consoleField.widthInChars = width - Q_PrintStrlen( cl_consolePrompt->string ) - 1;
    }
    else
    {
        width = 0;
    }
    
    if( width == con.linewidth )
        return;
        
    if( con.linewidth < 1 )			// video hasn't been initialized yet
    {
        width = DEFAULT_CONSOLE_WIDTH;
        con.linewidth = width;
        con.totallines = CON_TEXTSIZE / con.linewidth;
        for( i = 0; i < CON_TEXTSIZE; i++ )
        
            con.text[i] = ( ColorIndex( COLOR_WHITE ) << 8 ) | ' ';
    }
    else
    {
        oldwidth = con.linewidth;
        con.linewidth = width;
        oldtotallines = con.totallines;
        con.totallines = CON_TEXTSIZE / con.linewidth;
        numlines = oldtotallines;
        
        if( con.totallines < numlines )
            numlines = con.totallines;
            
        numchars = oldwidth;
        
        if( con.linewidth < numchars )
            numchars = con.linewidth;
            
        ::memcpy( tbuf, con.text, CON_TEXTSIZE * sizeof( schar16 ) );
        for( i = 0; i < CON_TEXTSIZE; i++ )
        {
            con.text[i] = ( ColorIndex( COLOR_WHITE ) << 8 ) | ' ';
        }
        
        for( i = 0 ; i < numlines ; i++ )
        {
            for( j = 0 ; j < numchars ; j++ )
            {
                con.text[( con.totallines - 1 - i ) * con.linewidth + j] = tbuf[( ( con.current - i + oldtotallines ) % oldtotallines ) * oldwidth + j];
            }
        }
    }
    
    con.current = con.totallines - 1;
    con.display = con.current;
}


/*
================
Con_Init
================
*/
void Con_Init( void )
{
    Com_Printf( "----- Console Initialization -------\n" );
    
    con_notifytime = cvarSystem->Get( "con_notifytime", "7", 0, "^1Defines how long messages (from players or the system) are on the screen." );
    con_conspeed = cvarSystem->Get( "scr_conspeed", "3", 0, "^1Set how fast the console goes up and down." );
    con_debug = cvarSystem->Get( "con_debug", "0", CVAR_ARCHIVE, "^1Toggle console debugging." );
    con_autoclear = cvarSystem->Get( "con_autoclear", "1", CVAR_ARCHIVE, "^1Toggles clearing of unfinished text after closing console." );
    con_restricted = cvarSystem->Get( "con_restricted", "0", CVAR_INIT, "^1Toggles clearing of unfinished text after closing console." );
    
    scr_conColorAlpha = cvarSystem->Get( "scr_conColorAlpha", "0.5", CVAR_ARCHIVE, "^1Defines the backgroud Alpha color of the console." );
    scr_conColorRed = cvarSystem->Get( "scr_conColorRed", "0", CVAR_ARCHIVE, "^1Defines the backgroud Red color of the console." );
    scr_conColorBlue = cvarSystem->Get( "scr_conColorBlue", "0.3", CVAR_ARCHIVE, "^1Defines the backgroud Blue color of the console." );
    scr_conColorGreen = cvarSystem->Get( "scr_conColorGreen", "0.23", CVAR_ARCHIVE, "^1Defines the backgroud Green color of the console." );
    
    scr_conBarHeight = cvarSystem->Get( "scr_conBarHeight", "2", CVAR_ARCHIVE, "^1Defines the bar height of the console." );
    
    scr_conBarColorAlpha = cvarSystem->Get( "scr_conBarColorAlpha", "0.3", CVAR_ARCHIVE, "^1Defines the bar Alpha color of the console." );
    scr_conBarColorRed = cvarSystem->Get( "scr_conBarColorRed", "1", CVAR_ARCHIVE, "^1Defines the bar Red color of the console." );
    scr_conBarColorBlue = cvarSystem->Get( "scr_conBarColorBlue", "1", CVAR_ARCHIVE, "^1Defines the bar Blue color of the console." );
    scr_conBarColorGreen = cvarSystem->Get( "scr_conBarColorGreen", "1", CVAR_ARCHIVE, "^1Defines the bar Green color of the console." );
    
    scr_conHeight = cvarSystem->Get( "scr_conHeight", "50", CVAR_ARCHIVE, "^1Console height size." );
    
    scr_conBarSize = cvarSystem->Get( "scr_conBarSize", "2", CVAR_ARCHIVE, "^1Console bar size." );
    
    // Done defining cvars for console colors
    
    cmdCompletionSystem->Clear( &g_consoleField );
    g_consoleField.widthInChars = g_console_field_width;
    
    cmdSystem->AddCommand( "toggleConsole", Con_ToggleConsole_f, "^1Opens or closes the console." );
    cmdSystem->AddCommand( "togglemenu", Con_ToggleMenu_f, "^1Show/hide the menu" );
    cmdSystem->AddCommand( "clear", Con_Clear_f, "^1Clear console history." );
    cmdSystem->AddCommand( "condump", Con_Dump_f, "^1Dumps the contents of the console to a text file." );
    cmdSystem->AddCommand( "search", Con_Search_f, "^1Find the text you are looking for." );
    cmdSystem->AddCommand( "searchDown", Con_Search_f, "^1Scroll the console to find the text you are looking for." );
    cmdSystem->AddCommand( "grep", Con_Grep_f, "^1Find the text you are looking for." );
    
    // ydnar: these are deprecated in favor of cgame/ui based version
    cmdSystem->AddCommand( "clMessageMode", Con_MessageMode_f, "^1(global chat), without the convenient pop-up box. Also: ‘say’." );
    cmdSystem->AddCommand( "clMessageMode2", Con_MessageMode2_f, "^1(teamchat), without the convenient pop-up box. Also: ‘say_team’." );
    cmdSystem->AddCommand( "clMessageMode3", Con_MessageMode3_f, "^1(fireteam chat), without the convenient pop-up box." );
    
    Com_Printf( "Console initialized.\n" );
}


/*
===============
Con_Linefeed
===============
*/
void Con_Linefeed( bool skipnotify )
{
    sint             i;
    
    // mark time for transparent overlay
    if( con.current >= 0 )
    {
        if( skipnotify )
        {
            con.times[con.current % NUM_CON_TIMES] = 0;
        }
        else
        {
            con.times[con.current % NUM_CON_TIMES] = cls.realtime;
        }
    }
    
    con.x = 0;
    if( con.display == con.current )
    {
        con.display++;
    }
    con.current++;
    for( i = 0; i < con.linewidth; i++ )
        con.text[( con.current % con.totallines ) * con.linewidth + i] = ( ColorIndex( CONSOLE_COLOR ) << 8 ) | ' ';
}

/*
================
CL_ConsolePrint

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
#if defined( _WIN32 ) && defined( NDEBUG )
#pragma optimize( "g", off )	// SMF - msvc totally screws this function up with optimize on
#endif

void CL_ConsolePrint( valueType* txt )
{
    sint y, c, l;
    valueType color;
    bool skipnotify = false;	// NERVE - SMF
    sint prev;		// NERVE - SMF
    
    // NERVE - SMF - work around for text that shows up in console but not in notify
    if( !Q_strncmp( txt, "[skipnotify]", 12 ) )
    {
        skipnotify = true;
        txt += 12;
    }
    
    if( txt[0] == '*' )
    {
        skipnotify = true;
        txt += 1;
    }
    
    // for some demos we don't want to ever show anything on the console
    if( cl_noprint && cl_noprint->integer )
    {
        return;
    }
    
    if( !con.initialized )
    {
        con.color[0] = con.color[1] = con.color[2] = con.color[3] = 1.0f;
        con.linewidth = -1;
        Con_CheckResize();
        con.initialized = true;
    }
    
    if( !skipnotify && !( cls.keyCatchers & KEYCATCH_CONSOLE ) && strncmp( txt, "EXCL: ", 6 ) )
    {
        // feed the text to cgame
        cmdSystem->SaveCmdContext( );
        cmdSystem->TokenizeString( txt );
        clientGameSystem->GameConsoleText( );
        cmdSystem->RestoreCmdContext( );
    }
    
    color = ColorIndex( CONSOLE_COLOR );
    
    while( ( c = *txt ) != 0 )
    {
        if( Q_IsColorString( txt ) )
        {
            if( *( txt + 1 ) == COLOR_NULL )
            {
                color = ColorIndex( CONSOLE_COLOR );
            }
            else
            {
                color = ColorIndex( *( txt + 1 ) );
            }
            txt += 2;
            continue;
        }
        
        // count word length
        for( l = 0; l < con.linewidth; l++ )
        {
            if( txt[l] <= ' ' && txt[l] >= 0 )
            {
                break;
            }
            if( l > 0 )
            {
                if( txt[l - 1] <= ';' ) break;
                if( txt[l - 1] <= ',' ) break;
            }
        }
        
        // word wrap
        if( l != con.linewidth && ( con.x + l >= con.linewidth ) )
        {
            Con_Linefeed( skipnotify );
            
        }
        
        txt++;
        
        switch( c )
        {
            case '\n':
                Con_Linefeed( skipnotify );
                break;
            case '\r':
                con.x = 0;
                break;
            default:			// display character and advance
                y = con.current % con.totallines;
                // rain - sign extension caused the character to carry over
                // into the color info for high ascii chars; casting c to unsigned
                con.text[y * con.linewidth + con.x] = ( color << 8 ) | ( uchar8 )c;
                con.x++;
                if( con.x >= con.linewidth )
                {
                    Con_Linefeed( skipnotify );
                }
                break;
        }
    }
    
    // mark time for transparent overlay
    if( con.current >= 0 )
    {
        // NERVE - SMF
        if( skipnotify )
        {
            prev = con.current % NUM_CON_TIMES - 1;
            if( prev < 0 )
            {
                prev = NUM_CON_TIMES - 1;
            }
            con.times[prev] = 0;
        }
        else
        {
            // -NERVE - SMF
            con.times[con.current % NUM_CON_TIMES] = cls.realtime;
        }
    }
}

#if defined( _WIN32 ) && defined( NDEBUG )
#pragma optimize( "g", on )		// SMF - re-enabled optimization
#endif

/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

Draw the editline after a ] prompt
================
*/
void Con_DrawInput( void )
{
    sint		y;
    valueType	prompt[ MAX_STRING_CHARS ];
    vec4_t	color;
    qtime_t realtime;
    
    if( cls.state != CA_DISCONNECTED && !( cls.keyCatchers & KEYCATCH_CONSOLE ) )
    {
        return;
    }
    
    Com_RealTime( &realtime );
    
    y = con.vislines - ( clientScreenSystem->ConsoleFontCharHeight() * 2 ) + 2 ;
    
    Q_vsprintf_s( prompt, sizeof( prompt ), sizeof( prompt ), "^0[^3%02d%c%02d^0]^7 %s", realtime.tm_hour, ( realtime.tm_sec & 1 ) ? ':' : ' ', realtime.tm_min, cl_consolePrompt->string );
    
    color[0] = 1.0f;
    color[1] = 1.0f;
    color[2] = 1.0f;
    color[3] = con.displayFrac * 2.0f;
    
    clientScreenSystem->DrawSmallStringExt( con.xadjust + cl_conXOffset->integer, y + 10, prompt, color, false, false );
    
    Q_CleanStr( prompt );
    cmdCompletionSystem->Draw( &g_consoleField, con.xadjust + cl_conXOffset->integer + clientScreenSystem->ConsoleFontStringWidth( prompt, ::strlen( prompt ) ), y + 10, true, true, color[3] );
}


/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify( void )
{
    sint             x, v;
    schar16*            text;
    sint             i;
    sint             time;
    sint             skip;
    sint             currentColor;
    
    currentColor = 7;
    renderSystem->SetColor( g_color_table[currentColor] );
    
    v = 0;
    for( i = con.current - NUM_CON_TIMES + 1; i <= con.current; i++ )
    {
        if( i < 0 )
        {
            continue;
        }
        time = con.times[i % NUM_CON_TIMES];
        if( time == 0 )
        {
            continue;
        }
        time = cls.realtime - time;
        if( time >= con_notifytime->value * 1000 )
        {
            continue;
        }
        text = con.text + ( i % con.totallines ) * con.linewidth;
        
        if( cl.snapServer.ps.pm_type != PM_INTERMISSION && cls.keyCatchers & ( KEYCATCH_UI | KEYCATCH_CGAME ) )
        {
            continue;
        }
        
        for( x = 0; x < con.linewidth; x++ )
        {
            if( ( text[x] & 0xff ) == ' ' )
            {
                continue;
            }
            if( ( ( text[x] >> 8 ) & COLOR_BITS ) != currentColor )
            {
                currentColor = ( text[x] >> 8 ) & COLOR_BITS;
                renderSystem->SetColor( g_color_table[currentColor] );
            }
            clientScreenSystem->DrawSmallChar( cl_conXOffset->integer + con.xadjust + ( x + 1 ) * SMALLCHAR_WIDTH, v, text[x] & 0xff );
        }
        
        v += SMALLCHAR_HEIGHT;
    }
    
    renderSystem->SetColor( nullptr );
    
    if( cls.keyCatchers & ( KEYCATCH_UI | KEYCATCH_CGAME ) )
    {
        return;
    }
    
    // draw the chat line
    if( cls.keyCatchers & KEYCATCH_MESSAGE )
    {
        if( chat_team )
        {
            valueType            buf[128];
            
            CL_TranslateString( "say_team:", buf );
            clientScreenSystem->DrawBigString( 8, v, buf, 1.0f, false );
            skip = strlen( buf ) + 2;
        }
        else if( chat_buddy )
        {
            valueType            buf[128];
            
            CL_TranslateString( "say_fireteam:", buf );
            clientScreenSystem->DrawBigString( 8, v, buf, 1.0f, false );
            skip = strlen( buf ) + 2;
        }
        else
        {
            valueType            buf[128];
            
            CL_TranslateString( "say:", buf );
            clientScreenSystem->DrawBigString( 8, v, buf, 1.0f, false );
            skip = strlen( buf ) + 1;
        }
        
        cmdCompletionSystem->BigDraw( &chatField, skip * BIGCHAR_WIDTH, 232, true, true );
        
        v += BIGCHAR_HEIGHT;
    }
    
}

/*
================
Con_DrawSolidConsole

Draws the console with the solid background
================
*/
void Con_DrawSolidConsole( float32 frac )
{
    sint				i, x, y;
    sint				rows;
    schar16*			text;
    sint				row;
    sint				lines;
    sint				currentColor;
    vec4_t			color;
    float32           totalwidth;
    float32           currentWidthLocation = 0;
    
    lines = cls.glconfig.vidHeight * frac;
    
    con.xadjust = 15;
    
    clientScreenSystem->AdjustFrom640( &con.xadjust, nullptr, nullptr, nullptr );
    
    color[0] = scr_conColorRed->value;
    color[1] = scr_conColorGreen->value;
    color[2] = scr_conColorBlue->value;
    color[3] = frac * 2 * scr_conColorAlpha->value;
    clientScreenSystem->FillRect( 10, 10, 620, 460 * scr_conHeight->integer * 0.01, color );
    
    color[0] = scr_conBarColorRed->value;
    color[1] = scr_conBarColorGreen->value;
    color[2] = scr_conBarColorBlue->value;
    color[3] = frac * 2 * scr_conBarColorAlpha->value;
    clientScreenSystem->FillRect( 10, 10, 620, 1, color );	//top
    clientScreenSystem->FillRect( 10, 460 * scr_conHeight->integer * 0.01 + 10, 621, 1, color );	//bottom
    clientScreenSystem->FillRect( 10, 10, 1, 460 * scr_conHeight->integer * 0.01, color );	//left
    clientScreenSystem->FillRect( 630, 10, 1, 460 * scr_conHeight->integer * 0.01, color );	//right
    
    // draw the version number
    color[0] = 1.0f;
    color[1] = 1.0f;
    color[2] = 1.0f;
    color[3] = frac * 2.0f;
    renderSystem->SetColor( color );
    
    // version string
    i = strlen( PRODUCT_VERSION );
    totalwidth = clientScreenSystem->ConsoleFontStringWidth( PRODUCT_VERSION, i ) + cl_conXOffset->integer;
    totalwidth += 30;
    
    for( x = 0 ; x < i ; x++ )
    {
        clientScreenSystem->DrawConsoleFontChar( cls.glconfig.vidWidth - totalwidth + currentWidthLocation, lines - clientScreenSystem->ConsoleFontCharHeight() * 2, PRODUCT_VERSION[x] );
        currentWidthLocation += clientScreenSystem->ConsoleFontCharWidth( PRODUCT_VERSION[x] );
    }
    
    // engine string
    i = strlen( ENGINE_NAME );
    totalwidth = clientScreenSystem->ConsoleFontStringWidth( ENGINE_NAME, i ) + cl_conXOffset->integer;
    totalwidth += 30;
    
    currentWidthLocation = 0;
    for( x = 0 ; x < i ; x++ )
    {
        clientScreenSystem->DrawConsoleFontChar( cls.glconfig.vidWidth - totalwidth + currentWidthLocation, lines - clientScreenSystem->ConsoleFontCharHeight(), ENGINE_NAME[x] );
        currentWidthLocation += clientScreenSystem->ConsoleFontCharWidth( ENGINE_NAME[x] );
    }
    
    // draw the text
    con.vislines = lines;
    
    // rows of text to draw
    rows = ( lines ) / clientScreenSystem->ConsoleFontCharHeight() - 3;
    rows++;
    
    y = lines - ( clientScreenSystem->ConsoleFontCharHeight() * 3 ) + 10;
    
    // draw from the bottom up
    if( con.display != con.current )
    {
        // draw arrows to show the buffer is backscrolled
        color[0] = 1.0f;
        color[1] = 0.0f;
        color[2] = 0.0f;
        color[3] = frac * 2.0f;
        renderSystem->SetColor( color );
        for( x = 0 ; x < con.linewidth - 4; x += 4 )
            clientScreenSystem->DrawConsoleFontChar( con.xadjust + ( x + 1 ) * clientScreenSystem->ConsoleFontCharWidth( '^' ), y, '^' );
        y -= clientScreenSystem->ConsoleFontCharHeight();
        rows--;
    }
    
    row = con.display;
    
    if( con.x == 0 )
    {
        row--;
    }
    
    currentColor = 7;
    color[0] = g_color_table[currentColor][0];
    color[1] = g_color_table[currentColor][1];
    color[2] = g_color_table[currentColor][2];
    color[3] = frac * 2.0f;
    renderSystem->SetColor( color );
    
    for( i = 0 ; i < rows ; i++, y -= clientScreenSystem->ConsoleFontCharHeight(), row-- )
    {
        float32 currentWidthLocation = cl_conXOffset->integer;
        
        if( row < 0 )
            break;
        if( con.current - row >= con.totallines )
        {
            // past scrollback wrap point
            continue;
        }
        
        text = con.text + ( row % con.totallines ) * con.linewidth;
        
        for( x = 0 ; x < con.linewidth ; x++ )
        {
            if( ( ( text[x] >> 8 ) & 31 ) != currentColor )
            {
                currentColor = ( text[x] >> 8 ) & 31;
                color[0] = g_color_table[currentColor][0];
                color[1] = g_color_table[currentColor][1];
                color[2] = g_color_table[currentColor][2];
                color[3] = frac * 2.0f;
                renderSystem->SetColor( color );
            }
            
            clientScreenSystem->DrawConsoleFontChar( con.xadjust + currentWidthLocation, y, text[x] & 0xff );
            currentWidthLocation += clientScreenSystem->ConsoleFontCharWidth( text[x] & 0xff );
        }
    }
    
    // draw the input prompt, user text, and cursor if desired
    Con_DrawInput();
    
    renderSystem->SetColor( nullptr );
}
extern convar_t*  con_drawnotify;

/*
==================
Con_DrawConsole
==================
*/
void Con_DrawConsole( void )
{
    // check for console width changes from a vid mode change
    Con_CheckResize();
    
    // if disconnected, render console full screen
    if( cls.state == CA_DISCONNECTED )
    {
        if( !( cls.keyCatchers & ( KEYCATCH_UI | KEYCATCH_CGAME ) ) )
        {
            Con_DrawSolidConsole( 1.0 );
            return;
        }
    }
    
    if( con.displayFrac )
    {
        Con_DrawSolidConsole( con.displayFrac );
    }
    else
    {
        // draw notify lines
        if( cls.state == CA_ACTIVE && con_drawnotify->integer )
        {
            Con_DrawNotify();
        }
    }
}

//================================================================

/*
==================
Con_RunConsole

Scroll it up or down
==================
*/
void Con_RunConsole( void )
{
    // decide on the destination height of the console
    if( cls.keyCatchers & KEYCATCH_CONSOLE )
    {
        con.finalFrac = 0.5f;
    }
    else
        con.finalFrac = 0.0f;				// none visible
        
    // scroll towards the destination height
    if( con.finalFrac < con.displayFrac )
    {
        con.displayFrac -= con_conspeed->value * ( float32 )cls.realFrametime / 1000.0f;
        if( con.finalFrac > con.displayFrac )
            con.displayFrac = con.finalFrac;
            
    }
    else if( con.finalFrac > con.displayFrac )
    {
        con.displayFrac += con_conspeed->value * ( float32 )cls.realFrametime / 1000.0f;
        if( con.finalFrac < con.displayFrac )
            con.displayFrac = con.finalFrac;
    }
    
}


void Con_PageUp( void )
{
    con.display -= 2;
    if( con.current - con.display >= con.totallines )
    {
        con.display = con.current - con.totallines + 1;
    }
}

void Con_PageDown( void )
{
    con.display += 2;
    if( con.display > con.current )
    {
        con.display = con.current;
    }
}

void Con_Top( void )
{
    con.display = con.totallines;
    if( con.current - con.display >= con.totallines )
    {
        con.display = con.current - con.totallines + 1;
    }
}

void Con_Bottom( void )
{
    con.display = con.current;
}


void Con_Close( void )
{
    if( !com_cl_running->integer )
    {
        return;
    }
    cmdCompletionSystem->Clear( &g_consoleField );
    Con_ClearNotify();
    cls.keyCatchers &= ~KEYCATCH_CONSOLE;
    con.finalFrac = 0;			// none visible
    con.displayFrac = 0;
}
