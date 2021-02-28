////////////////////////////////////////////////////////////////////////////////////////
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
// File name:   CVarSystem_api.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CMDSYSTEM_API_H__
#define __CMDSYSTEM_API_H__

typedef void( *completionFunc_t )( valueType* args, sint argNum );
typedef void( *xcommand_t )( void );
//
// idCVarSystem
//
class idCmdSystem
{
public:
    virtual void WriteAliases( fileHandle_t f ) = 0;
    virtual sint Argc( void ) = 0;
    virtual valueType* ArgsFrom( sint arg ) = 0;
    virtual valueType* Argv( sint arg ) = 0;
    virtual valueType* Args( void ) = 0;
    virtual void TokenizeString( pointer text_in ) = 0;
    virtual void TokenizeStringIgnoreQuotes( pointer text_in ) = 0;
    virtual void AddCommand( pointer cmd_name, xcommand_t function, pointer cmd_desc ) = 0;
    virtual void SetCommandCompletionFunc( pointer command, completionFunc_t complete ) = 0;
    virtual void RemoveCommand( pointer cmd_name ) = 0;
    virtual void CommandCompletion( void( *callback )( pointer s ) ) = 0;
    virtual void CompleteArgument( pointer command, valueType* args, sint argNum ) = 0;
    virtual void ExecuteString( pointer text ) = 0;
    virtual void Init( void ) = 0;
    virtual void Shutdown( void ) = 0;
    virtual valueType* Cmd( void ) = 0;
    virtual void AliasCompletion( void( *callback )( pointer s ) ) = 0;
    virtual void DelayCompletion( void( *callback )( pointer s ) ) = 0;
    virtual void SaveCmdContext( void ) = 0;
    virtual void RestoreCmdContext( void ) = 0;
    virtual valueType* FromNth( sint count ) = 0;
    virtual void ArgvBuffer( sint arg, valueType* buffer, uint64 bufferLength ) = 0;
    virtual void ArgsBuffer( valueType* buffer, uint64 bufferLength ) = 0;
    virtual void LiteralArgsBuffer( valueType* buffer, uint64 bufferLength ) = 0;
};

extern idCmdSystem* cmdSystem;

#endif // !__CVARSYSTEM_API_H__
