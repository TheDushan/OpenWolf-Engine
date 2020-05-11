////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2018 - 2019 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   serverCcmds.h
// Version:     v1.02
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CONSOLECURSES_H__
#define __CONSOLECURSES_H__

#ifdef _WIN32
#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#endif
#endif

#define TITLE "^4---[ ^3" CLIENT_WINDOW_TITLE " Console ^4]---"
#define PROMPT "^3-> "
#define INPUT_SCROLL 15
#define LOG_SCROLL 5
#define MAX_LOG_LINES 1024
#define LOG_BUF_SIZE 65536

static bool curses_on = false;
static field_t input_field;
static WINDOW* borderwin;
static WINDOW* logwin;
static WINDOW* inputwin;
static WINDOW* scrollwin;
static WINDOW* clockwin;

static UTF8 logbuf[LOG_BUF_SIZE];
static UTF8* insert = logbuf;
static S32 scrollline = 0;
static S32 lastline = 1;

// The special characters look good on the win32 console but suck on other consoles
#ifdef _WIN32
#define SCRLBAR_CURSOR ACS_BLOCK
#define SCRLBAR_LINE ACS_VLINE
#define SCRLBAR_UP ACS_UARROW
#define SCRLBAR_DOWN ACS_DARROW
#else
#define SCRLBAR_CURSOR '#'
#define SCRLBAR_LINE ACS_VLINE
#define SCRLBAR_UP ACS_HLINE
#define SCRLBAR_DOWN ACS_HLINE
#endif

#define LOG_LINES (LINES - 4)
#define LOG_COLS (COLS - 3)

//
// idConsoleCursesLocal
//
class idConsoleCursesLocal : public idConsoleCursesSystem
{
public:
    idConsoleCursesLocal();
    ~idConsoleCursesLocal();
    
    virtual void Shutdown( void );
    virtual void Clear_f( void );
    virtual void Init( void );
    virtual UTF8* Input( void );
    virtual void Print( StringEntry msg );
    
    static void SetColor( WINDOW* win, S32 color );
    static void UpdateCursor( void );
    static void DrawScrollBar( void );
    static void ColorPrint( WINDOW* win, StringEntry msg, bool stripcodes );
    static void UpdateClock( void );
    static void Resize( void );
};

extern idConsoleCursesLocal consoleCursesLocal;

#endif //!__CONSOLECURSES_H__
