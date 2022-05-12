////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2022 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   cmdSystem.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description: Quake script command processing module
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CMDSYSTEM_HPP__
#define __CMDSYSTEM_HPP__

#define MAX_CMD_BUFFER 131072
#define MAX_CMD_LINE 1024

typedef void(*xcommand_t)(void);
typedef void(*completionFunc_t)(valueType *args, sint argNum);

typedef struct {
    uint64 maxsize, cursize;
    uchar8 *data;
} cmd_t;

extern sint cmd_wait;
extern cmd_t cmd_text;
extern uchar8 cmd_text_buf[MAX_CMD_BUFFER];

typedef struct cmd_function_s {
    struct cmd_function_s *next;
    valueType *name;
    xcommand_t function;
    completionFunc_t complete;
    valueType *desc;
} cmd_function_t;

typedef struct cmdContext_s {
    sint    argc;
    valueType *argv[MAX_STRING_TOKENS]; // points into cmd.tokenized
    valueType tokenized[BIG_INFO_STRING +
                                        MAX_STRING_TOKENS];   // will have 0 bytes inserted
    valueType cmd[BIG_INFO_STRING]; // the original command we received (no token processing)
} cmdContext_t;

extern cmdContext_t cmd;
extern cmdContext_t savedCmd;

// possible commands to execute
extern cmd_function_t *cmd_functions;

/*
===============
Helper functions for Cmd_If_f & Cmd_ModCase
===============
*/
#ifdef DEDICATED
static const valueType modifierList[] =
    "not supported in the dedicated server";
#else
static const valueType modifierList[] =
    "shift, ctrl, alt, command/cmd, mode, super; ! negates; e.g. shift,!alt";
#endif
static const struct {
    valueType name[8];
    uchar16 count;
    uchar16 bit;
    uint index;
} modifierKeys[] = {
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

typedef struct {
    uchar16 down, up;
    sint bits;
} modifierMask_t;

typedef struct cmd_alias_s {
    struct cmd_alias_s *next;
    valueType              *name;
    valueType              *exec;
} cmd_alias_t;

static cmd_alias_t *cmd_aliases = nullptr;

//
// idServerBotSystemLocal
//
class idCmdSystemLocal : public idCmdSystem {
public:
    idCmdSystemLocal();
    ~idCmdSystemLocal();

    virtual void WriteAliases(fileHandle_t f);
    virtual sint Argc(void);
    virtual valueType *ArgsFrom(sint arg);
    virtual valueType *Argv(sint arg);
    virtual valueType *Args(void);
    virtual void TokenizeString(pointer text_in);
    virtual void TokenizeStringIgnoreQuotes(pointer text_in);
    virtual void AddCommand(pointer cmd_name, xcommand_t function,
                            pointer cmd_desc);
    virtual void SetCommandCompletionFunc(pointer command,
                                          completionFunc_t complete);
    virtual void RemoveCommand(pointer cmd_name);
    virtual void CommandCompletion(void(*callback)(pointer s));
    virtual void CompleteArgument(pointer command, valueType *args,
                                  sint argNum);
    virtual void ExecuteString(pointer text);
    virtual void Init(void);
    virtual void Shutdown(void);
    virtual valueType *Cmd(void);
    virtual void AliasCompletion(void(*callback)(pointer s));
    virtual void DelayCompletion(void(*callback)(pointer s));
    virtual void SaveCmdContext(void);
    virtual void RestoreCmdContext(void);
    virtual valueType *FromNth(sint count);
    virtual void ArgvBuffer(sint arg, valueType *buffer, uint64 bufferLength);
    virtual void ArgsBuffer(valueType *buffer, uint64 bufferLength);
    virtual void LiteralArgsBuffer(valueType *buffer, uint64 bufferLength);

    cmd_function_t *FindCommand(pointer cmdName);
    static void RunAlias(void);
    static void Alias(void);
    valueType *EscapeString(pointer in);
    void TokenizeStringParseCvar(pointer text_in);
    static void List(void);
    static void CompleteCfgName(valueType *args, sint argNum);
    static void CompleteAliasName(valueType *args, sint argNum);
    static void CompleteConcat(valueType *args, sint argNum);
    static void CompleteIf(valueType *args, sint argNum);
    static void CompleteDelay(valueType *args, sint argNum);
    static void CompleteUnDelay(valueType *args, sint argNum);
    static void Wait(void);
    static void Exec_f(void);
    static void Vstr(void);
    static void ExecFile(valueType *f);
    static modifierMask_t getModifierMask(pointer mods);
    static sint checkKeysDown(modifierMask_t mask);
    static void ModCase(void);
    static void If(void);
    static void Math(void);
    static void Strcmp(void);
    static void Concat(void);
    static void Calc(void);
    static void Echo(void);
    static void Undelay(void);
    static void UndelayAll(void);
    static void Delay(void);
    static void Random(void);
    static void AliasList(void);
    static void ClearAliases(void);
    static void UnAlias(void);
    static void TokenizeString2(pointer text_in, bool ignoreQuotes,
                                bool parseCvar);
};

extern idCmdSystemLocal cmdSystemLocal;

#endif //!__CMDSYSTEM_HPP__