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
// File name:   cmdCompletion_api.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CMDCOMPLETION_API_H__
#define __CMDCOMPLETION_API_H__

/*
==============================================================
Edit fields and command line history/completion
==============================================================
*/

#define MAX_EDIT_LINE   256
typedef struct
{
    uint64 cursor, scroll, widthInChars;
    valueType buffer[MAX_EDIT_LINE];
} field_t;

//
// idCmdCompletionSystem
//
class idCmdCompletionSystem
{
public:
    virtual void CompleteKeyname( void ) = 0;
    virtual void CompleteCgame( sint argNum ) = 0;
    virtual void CompleteFilename( pointer dir, pointer ext, bool stripExt ) = 0;
    virtual void CompleteAlias( void ) = 0;
    virtual void CompleteDelay( void ) = 0;
    virtual void CompleteCommand( valueType* cmd, bool doCommands, bool doCvars ) = 0;
    virtual void AutoComplete( field_t* field, pointer prompt ) = 0;
    virtual void Clear( field_t* edit ) = 0;
    virtual void Set( field_t* edit, pointer content ) = 0;
    virtual void WordDelete( field_t* edit ) = 0;
    virtual void Draw( field_t* edit, sint x, sint y, bool showCursor, bool noColorEscape, float32 alpha ) = 0;
    virtual void BigDraw( field_t* edit, sint x, sint y, bool showCursor, bool noColorEscape ) = 0;
    virtual void KeyDownEvent( field_t* edit, sint key ) = 0;
    virtual void CharEvent( field_t* edit, valueType ch ) = 0;
};

extern idCmdCompletionSystem* cmdCompletionSystem;

#endif //__COMMANDLINECOMPLETION_API_H__