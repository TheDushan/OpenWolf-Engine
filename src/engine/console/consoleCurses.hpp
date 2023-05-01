////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2018 - 2023 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   consoleCurses.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CONSOLECURSES_HPP__
#define __CONSOLECURSES_HPP__

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
static WINDOW *borderwin;
static WINDOW *logwin;
static WINDOW *inputwin;
static WINDOW *scrollwin;
static WINDOW *clockwin;

static valueType logbuf[LOG_BUF_SIZE];
static valueType *insert = logbuf;
static sint scrollline = 0;
static sint lastline = 1;

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
class idConsoleCursesLocal : public idConsoleCursesSystem {
public:
    idConsoleCursesLocal();
    ~idConsoleCursesLocal();

    virtual void Shutdown(void);
    virtual void Clear_f(void);
    virtual void Init(void);
    virtual valueType *Input(void);
    virtual void Print(pointer msg);

    static void SetColor(WINDOW *win, sint color);
    static void UpdateCursor(void);
    static void DrawScrollBar(void);
    static void ColorPrint(WINDOW *win, pointer msg, bool stripcodes);
    static void UpdateClock(void);
    static void Resize(void);
};

extern idConsoleCursesLocal consoleCursesLocal;

#endif //!__CONSOLECURSES_HPP__
