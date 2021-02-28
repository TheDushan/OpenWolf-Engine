////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   cmdCompletion.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CMDCOMPLETION_H__
#define __CMDCOMPLETION_H__

static valueType     shortestMatch[MAX_TOKEN_CHARS];
static sint      matchCount;

// field we are working on, passed to idCmdCompletionLocal::CompleteCommand (&g_consoleCommand for instance)
static field_t* completionField;
static pointer completionPrompt;

//
// idCmdCompletionLocal
//
class idCmdCompletionLocal : public idCmdCompletionSystem
{
public:
    idCmdCompletionLocal();
    ~idCmdCompletionLocal();
    
    virtual void CompleteKeyname( void );
    virtual void CompleteCgame( sint argNum );
    virtual void CompleteFilename( pointer dir, pointer ext, bool stripExt );
    virtual void CompleteAlias( void );
    virtual void CompleteDelay( void );
    virtual void CompleteCommand( valueType* cmd, bool doCommands, bool doCvars );
    virtual void AutoComplete( field_t* field, pointer prompt );
    virtual void Clear( field_t* edit );
    virtual void Set( field_t* edit, pointer content );
    virtual void WordDelete( field_t* edit );
    virtual void Draw( field_t* edit, sint x, sint y, bool showCursor, bool noColorEscape, float32 alpha );
    virtual void BigDraw( field_t* edit, sint x, sint y, bool showCursor, bool noColorEscape );
    virtual void KeyDownEvent( field_t* edit, sint key );
    virtual void CharEvent( field_t* edit, sint ch );
    
    static void FindMatches( pointer s );
    static void PrintMatches( pointer s );
    static void PrintCvarMatches( pointer s );
    static valueType* FindFirstSeparator( valueType* s );
    static bool Complete( void );
    static void VariableSizeDraw( field_t* edit, sint x, sint y, sint size, bool showCursor, bool noColorEscape, float32 alpha );
    static void Paste( field_t* edit );
};

extern idCmdCompletionLocal cmdCompletionLocal;

#endif //__COMMANDLINECOMPLETION_H__