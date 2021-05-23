////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   clientConsole_api.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTCONSOLE_API_H__
#define __CLIENTCONSOLE_API_H__

#define NUM_CON_TIMES 4
#define NUMBER_TABS 4
#define CON_TEXTSIZE 163840

typedef struct {
    bool         initialized;

    valueType    text[CON_TEXTSIZE];
    vec4_t       text_color[CON_TEXTSIZE];
    sint         current;   // line where next message will be printed
    sint         x;         // offset in current line for next print
    sint         display;   // bottom of console displays this line

    sint         linewidth; // characters across screen
    sint         totallines;    // total lines in console scrollback

    float32      xadjust;    // for wide aspect screens

    float32      displayFrac;    // aproaches finalFrac at scr_conspeed
    float32      finalFrac;  // 0.0 to 1.0 lines of console to display
    float32      desiredFrac;    // ydnar: for variable console heights

    sint         vislines;  // in scanlines

    sint
    times[NUM_CON_TIMES];  // cls.realtime time the line was generated
    // for transparent notify lines
    vec4_t       color;

    sint         acLength; // Arnout: autocomplete buffer length
} console_t;

extern console_t con[NUMBER_TABS];
extern console_t *activeCon;

//
// idClientConsoleSystemAPI
//
class idClientConsoleSystemAPI {
public:
    virtual void LineAccept(void) = 0;
    virtual void ConsoleSwitch(sint n) = 0;
    virtual void ConsoleNext(sint n) = 0;
    virtual void ToggleConsole_f(void) = 0;
    virtual void OpenConsole_f(void) = 0;
    virtual void ClearNotify(void) = 0;
    virtual void ConsolePrint(valueType *txt) = 0;
    virtual void RunConsole(void) = 0;
    virtual void PageUp(void) = 0;
    virtual void PageDown(void) = 0;
    virtual void Top(void) = 0;
    virtual void Bottom(void) = 0;
    virtual void Close(void) = 0;
    virtual void Init(void) = 0;
    virtual void DrawConsole(void) = 0;
};

extern idClientConsoleSystemAPI *clientConsoleSystem;

#endif // !__CLIENTAVI_API_H__

