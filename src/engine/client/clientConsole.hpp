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
// File name:   clientConsole.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTCONSOLE_LOCAL_HPP__
#define __CLIENTCONSOLE_LOCAL_HPP__

#define CON_ALL 0
#define CON_SYS 1
#define CON_CHAT 2
#define CON_TCHAT 3

// check if this is a chat console
#define CON_ISCHAT(conNum) (conNum >= CON_CHAT)

static sint g_console_field_width = 78;

#define CONSOLE_COLOR '7'
#define DEFAULT_CONSOLE_WIDTH   78

static pointer conTabsNames[NUMBER_TABS] = {
    "All Chat",
    "System Chat",
    "Player Chat",
    "Team Chat"
};

// color indexes in g_color_table
const sint consoleColors[] = {
    1,
    3,
    2,
    5,
    6
};

static vec4_t console_highlightcolor = { 0.5, 0.5, 0.2, 0.45 };
static sint dump_time;
static sint dump_count;

//
// idClientConsoleSystemLocal
//
class idClientConsoleSystemLocal : public idClientConsoleSystemAPI {
public:
    idClientConsoleSystemLocal();
    ~idClientConsoleSystemLocal();

    virtual void LineAccept(void);
    virtual void ConsoleSwitch(sint n);
    virtual void ConsoleNext(sint n);
    virtual void ToggleConsole_f(void);
    virtual void OpenConsole_f(void);
    virtual void ClearNotify(void);
    virtual void ConsolePrint(valueType *txt);
    virtual void RunConsole(void);
    virtual void PageUp(void);
    virtual void PageDown(void);
    virtual void Top(void);
    virtual void Bottom(void);
    virtual void Close(void);
    virtual void Init(void);
    virtual void DrawConsole(void);

public:
    static void ToggleConsole(void);
    static void ToggleMenu_f(void);
    static void Clear_f(void);
    static void Dump_f(void);
    static void Search_f(void);
    static void Grep_f(void);
    static void CheckResize(console_t *con);
    static void MessageMode_f(void);
    static void MessageMode2_f(void);
    static void MessageMode3_f(void);
    static void CommandMode_f(void);
    static void Linefeed(console_t *con,
                         bool skipnotify);
    static void ConsoleTabsInit(void);
    static void ConsolePrintToTabs(valueType *txt,
                                   console_t *con,
                                   bool toCgame);
    static void DrawInput(void);
    static void DrawNotify(void);
    static void DrawSolidConsole(float32 frac);
};

extern idClientConsoleSystemLocal clientConsoleLocal;

#endif // !__CLIENTCONSOLE_LOCAL_HPP__

