////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2018 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   consoleHistory.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CONSOLE_HISTORY_H__
#define __CONSOLE_HISTORY_H__

#define CON_HISTORY 64
#ifdef DEDICATED
#define CON_HISTORY_FILE "conhistory_server"
#else
#define CON_HISTORY_FILE "conhistory"
#endif

static valueType history[CON_HISTORY][MAX_EDIT_LINE];
static sint hist_current = -1, hist_next = 0;

//
// idConsoleHistoryLocal
//
class idConsoleHistoryLocal : public idConsoleHistorySystem
{
public:
    idConsoleHistoryLocal();
    ~idConsoleHistoryLocal();
    
    virtual void Load( void );
    virtual void Save( void );
    virtual void Add( pointer field );
    virtual pointer Prev( void );
    virtual pointer Next( pointer field );
};

extern idConsoleHistoryLocal consoleHistoryLocal;

#endif //!__CONSOLE_HISTORY_H__
