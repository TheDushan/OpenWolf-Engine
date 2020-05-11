////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 - 2019 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   CVarSystem_api.h
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CMDSYSTEM_API_H__
#define __CMDSYSTEM_API_H__

typedef void( *completionFunc_t )( UTF8* args, S32 argNum );
typedef void( *xcommand_t )( void );
//
// idCVarSystem
//
class idCmdSystem
{
public:
    virtual void WriteAliases( fileHandle_t f ) = 0;
    virtual S32 Argc( void ) = 0;
    virtual UTF8* ArgsFrom( S32 arg ) = 0;
    virtual UTF8* Argv( S32 arg ) = 0;
    virtual UTF8* Args( void ) = 0;
    virtual void TokenizeString( StringEntry text_in ) = 0;
    virtual void TokenizeStringIgnoreQuotes( StringEntry text_in ) = 0;
    virtual void AddCommand( StringEntry cmd_name, xcommand_t function, StringEntry cmd_desc ) = 0;
    virtual void SetCommandCompletionFunc( StringEntry command, completionFunc_t complete ) = 0;
    virtual void RemoveCommand( StringEntry cmd_name ) = 0;
    virtual void CommandCompletion( void( *callback )( StringEntry s ) ) = 0;
    virtual void CompleteArgument( StringEntry command, UTF8* args, S32 argNum ) = 0;
    virtual void ExecuteString( StringEntry text ) = 0;
    virtual void Init( void ) = 0;
    virtual void Shutdown( void ) = 0;
    virtual UTF8* Cmd( void ) = 0;
    virtual void AliasCompletion( void( *callback )( StringEntry s ) ) = 0;
    virtual void DelayCompletion( void( *callback )( StringEntry s ) ) = 0;
    virtual void SaveCmdContext( void ) = 0;
    virtual void RestoreCmdContext( void ) = 0;
    virtual UTF8* FromNth( S32 count ) = 0;
    virtual void ArgvBuffer( S32 arg, UTF8* buffer, S32 bufferLength ) = 0;
    virtual void ArgsBuffer( UTF8* buffer, S32 bufferLength ) = 0;
    virtual void LiteralArgsBuffer( UTF8* buffer, S32 bufferLength ) = 0;
};

extern idCmdSystem* cmdSystem;

#endif // !__CVARSYSTEM_API_H__
