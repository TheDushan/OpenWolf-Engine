////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
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
// File name:   cmd.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: Quake script command processing module
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CMDSYSTEM_H__
#define __CMDSYSTEM_H__

#define MAX_CMD_BUFFER 131072
#define MAX_CMD_LINE 1024

typedef void( *xcommand_t )( void );
typedef void( *completionFunc_t )( UTF8* args, S32 argNum );

typedef struct
{
    S32 maxsize;
    S32 cursize;
    U8* data;
} cmd_t;

extern S32 cmd_wait;
extern cmd_t cmd_text;
extern U8 cmd_text_buf[MAX_CMD_BUFFER];

typedef struct cmd_function_s
{
    struct cmd_function_s* next;
    UTF8* name;
    xcommand_t function;
    completionFunc_t complete;
    UTF8* desc;
} cmd_function_t;

typedef struct cmdContext_s
{
    S32	argc;
    UTF8* argv[MAX_STRING_TOKENS];	// points into cmd.tokenized
    UTF8 tokenized[BIG_INFO_STRING + MAX_STRING_TOKENS];	// will have 0 bytes inserted
    UTF8 cmd[BIG_INFO_STRING]; // the original command we received (no token processing)
} cmdContext_t;

extern cmdContext_t cmd;
extern cmdContext_t savedCmd;

// possible commands to execute
extern cmd_function_t* cmd_functions;

/*
===============
Helper functions for Cmd_If_f & Cmd_ModCase
===============
*/
#ifdef DEDICATED
static const UTF8 modifierList[] = "not supported in the dedicated server";
#else
static const UTF8 modifierList[] = "shift, ctrl, alt, command/cmd, mode, super; ! negates; e.g. shift,!alt";
#endif
static const struct
{
    UTF8 name[8];
    U16 count;
    U16 bit;
    U32 index;
} modifierKeys[] =
{
    { "shift",   5,  1, K_SHIFT },
    { "ctrl",    4,  2, K_CTRL },
    { "alt",     3,  4, K_ALT },
    { "command", 7,  8, K_COMMAND },
    { "cmd",     3,  8, K_COMMAND },
    { "mode",    4, 16, K_MODE },
    { "super",   5, 32, K_SUPER },
    { "",        0,  0, 0 }
};

// Following is no. of bits required for modifiers in the above list
#define NUM_RECOGNISED_MODIFIERS 6

typedef struct
{
    U16 down, up;
    S32 bits;
} modifierMask_t;

typedef struct cmd_alias_s
{
    struct cmd_alias_s*	next;
    UTF8*				name;
    UTF8*				exec;
} cmd_alias_t;

static cmd_alias_t*	cmd_aliases = NULL;

//
// idServerBotSystemLocal
//
class idCmdSystemLocal : public idCmdSystem
{
public:
    idCmdSystemLocal();
    ~idCmdSystemLocal();
    
    virtual void WriteAliases( fileHandle_t f );
    virtual S32 Argc( void );
    virtual UTF8* ArgsFrom( S32 arg );
    virtual UTF8* Argv( S32 arg );
    virtual UTF8* Args( void );
    virtual void TokenizeString( StringEntry text_in );
    virtual void TokenizeStringIgnoreQuotes( StringEntry text_in );
    virtual void AddCommand( StringEntry cmd_name, xcommand_t function, StringEntry cmd_desc );
    virtual void SetCommandCompletionFunc( StringEntry command, completionFunc_t complete );
    virtual void RemoveCommand( StringEntry cmd_name );
    virtual void CommandCompletion( void( *callback )( StringEntry s ) );
    virtual void CompleteArgument( StringEntry command, UTF8* args, S32 argNum );
    virtual void ExecuteString( StringEntry text );
    virtual void Init( void );
    virtual void Shutdown( void );
    virtual UTF8* Cmd( void );
    virtual void AliasCompletion( void( *callback )( StringEntry s ) );
    virtual void DelayCompletion( void( *callback )( StringEntry s ) );
    virtual void SaveCmdContext( void );
    virtual void RestoreCmdContext( void );
    virtual UTF8* FromNth( S32 count );
    virtual void ArgvBuffer( S32 arg, UTF8* buffer, S32 bufferLength );
    virtual void ArgsBuffer( UTF8* buffer, S32 bufferLength );
    virtual void LiteralArgsBuffer( UTF8* buffer, S32 bufferLength );
    
    cmd_function_t* FindCommand( StringEntry cmdName );
    static void RunAlias( void );
    static void Alias( void );
    UTF8* EscapeString( StringEntry in );
    void TokenizeStringParseCvar( StringEntry text_in );
    static void List( void );
    static void CompleteCfgName( UTF8* args, S32 argNum );
    static void CompleteAliasName( UTF8* args, S32 argNum );
    static void CompleteConcat( UTF8* args, S32 argNum );
    static void CompleteIf( UTF8* args, S32 argNum );
    static void CompleteDelay( UTF8* args, S32 argNum );
    static void CompleteUnDelay( UTF8* args, S32 argNum );
    static void Wait( void );
    static void Exec_f( void );
    static void Vstr( void );
    static void ExecFile( UTF8* f );
    static modifierMask_t getModifierMask( StringEntry mods );
    static S32 checkKeysDown( modifierMask_t mask );
    static void ModCase( void );
    static void If( void );
    static void Math( void );
    static void Strcmp( void );
    static void Concat( void );
    static void Calc( void );
    static void Echo( void );
    static void Undelay( void );
    static void UndelayAll( void );
    static void Delay( void );
    static void Random( void );
    static void AliasList( void );
    static void ClearAliases( void );
    static void UnAlias( void );
    static void TokenizeString2( StringEntry text_in, bool ignoreQuotes, bool parseCvar );
};

extern idCmdSystemLocal cmdSystemLocal;

#endif //!__CMDSYSTEM_H__