////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
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
// File name:   CmdSystem.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: Quake script command processing module
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.hpp>
#elif DEDICATED
#include <null/null_serverprecompiled.hpp>
#else
#include <framework/precompiled.hpp>
#endif

idCmdSystemLocal cmdSystemLocal;
idCmdSystem *cmdSystem = &cmdSystemLocal;

sint cmd_wait;
cmd_t cmd_text;
uchar8 cmd_text_buf[MAX_CMD_BUFFER];

cmdContext_t cmd;
cmdContext_t savedCmd;

// possible commands to execute
cmd_function_t *cmd_functions;

#ifdef DEDICATED
qkey_t keys[MAX_KEYS];
#endif

/*
===============
idCmdSystemLocal::idCmdSystemLocal
===============
*/
idCmdSystemLocal::idCmdSystemLocal(void) {
}

/*
===============
idCmdSystemLocal::~idCmdSystemLocal
===============
*/
idCmdSystemLocal::~idCmdSystemLocal(void) {
}

/*
============
idCmdSystemLocal::FindCommand
============
*/
cmd_function_t *idCmdSystemLocal::FindCommand(pointer cmdName) {
    cmd_function_t *cmd;

    for(cmd = cmd_functions; cmd; cmd = cmd->next) {
        if(!Q_stricmp(cmdName, cmd->name)) {
            return cmd;
        }
    }

    return nullptr;
}

/*
============
idCmdSystemLocal::Wait

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "cmd use rocket ; +attack ; wait ; -attack ; cmd use blaster"
============
*/
void idCmdSystemLocal::Wait(void) {
    if(cmdSystemLocal.Argc() == 2) {
        cmd_wait = atoi(cmdSystemLocal.Argv(1));

        if(cmd_wait < 0) {
            // ignore the argument
            cmd_wait = 1;
        }
    } else {
        cmd_wait = 1;
    }
}

/*
===============
idCmdSystemLocal::Exec_f
===============
*/
void idCmdSystemLocal::ExecFile(valueType *f) {
    sint i;

    COM_Compress(f);

    cvarSystem->Get("arg_all", cmdSystemLocal.ArgsFrom(2),
                    CVAR_TEMP | CVAR_ROM | CVAR_USER_CREATED, nullptr);
    cvarSystem->Set("arg_all", cmdSystemLocal.ArgsFrom(2));
    cvarSystem->Get("arg_count", va("%i", cmdSystemLocal.Argc() - 2),
                    CVAR_TEMP | CVAR_ROM | CVAR_USER_CREATED, nullptr);
    cvarSystem->Set("arg_count", va("%i", cmdSystemLocal.Argc() - 2));

    for(i = cmdSystemLocal.Argc() - 2; i; i--) {
        cvarSystem->Get(va("arg_%i", i), cmdSystemLocal.Argv(i + 1),
                        CVAR_TEMP | CVAR_ROM | CVAR_USER_CREATED, nullptr);
        cvarSystem->Set(va("arg_%i", i), cmdSystemLocal.Argv(i + 1));
    }

    cmdBufferLocal.InsertText(f);
}

/*
===============
idCmdSystemLocal::Exec_f
===============
*/
void idCmdSystemLocal::Exec_f(void) {
    union {
        valueType *c;
        void *v;
    } f;

    sint len;
    valueType filename[MAX_QPATH];
    fileHandle_t h;
    bool success = false;

    if(cmdSystemLocal.Argc() < 2) {
        Com_Printf("exec <filename> (args) : execute a script file\n");
        return;
    }

    Com_Printf("execing %s\n", cmdSystemLocal.Argv(1));

    Q_strncpyz(filename, cmdSystemLocal.Argv(1), sizeof(filename));
    COM_DefaultExtension(filename, sizeof(filename), ".cfg");

    len = fileSystem->SV_FOpenFileRead(filename, &h);

    if(h) {
        success = true;
        f.v = Hunk_AllocateTempMemory(len + 1);
        fileSystem->Read(f.v, len, h);
        f.c[len] = 0;
        fileSystem->FCloseFile(h);
        ExecFile(f.c);
        Hunk_FreeTempMemory(f.v);
    }

    fileSystem->ReadFile(filename, &f.v);

    if(f.c) {
        success = true;
        ExecFile(f.c);
        fileSystem->FreeFile(f.v);
    }

    if(!success) {
        Com_Printf("couldn't exec %s\n", cmdSystemLocal.Argv(1));
    }
}

/*
===============
idCmdSystemLocal::Vstr

Inserts the current value of a variable as command text
===============
*/
void idCmdSystemLocal::Vstr(void) {
    valueType *v;

    if(cmdSystemLocal.Argc() != 2) {
        Com_Printf("vstr <variablename> : execute a variable command\n");
        return;
    }

    v = cvarSystem->VariableString(cmdSystemLocal.Argv(1));
    cmdBufferLocal.InsertText(va("%s\n", v));
}

/*
===============
modifierMask_t idCmdSystemLocal::getModifierMask

Helper functions for idCmdSystemLocal::If & idCmdSystemLocal::ModCase
===============
*/
modifierMask_t idCmdSystemLocal::getModifierMask(pointer mods) {
    sint i;
    modifierMask_t mask;
    static const modifierMask_t none;
    pointer ptr;

    mask = none;

    --mods;

    while(*++mods == ' ') /* skip leading spaces */;

    ptr = mods;

    while(*ptr) {
        sint invert = (*ptr == '!');

        if(invert) {
            ++ptr;
        }

        for(i = 0; modifierKeys[i].bit; ++i) {
            // is it this modifier?
            if(!Q_strnicmp(ptr, modifierKeys[i].name, modifierKeys[i].count) &&
                    (ptr[modifierKeys[i].count] == ' '
                     || ptr[modifierKeys[i].count] == ',' || ptr[modifierKeys[i].count] == 0)) {
                if(invert) {
                    mask.up |= modifierKeys[i].bit;
                } else {
                    mask.down |= modifierKeys[i].bit;
                }

                if((mask.down & mask.up) & modifierKeys[i].bit) {
                    Com_Printf("can't have %s both pressed and not pressed\n",
                               modifierKeys[i].name);
                    return none;
                }

                // right, parsed a word - skip it, maybe a comma, and any spaces
                ptr += modifierKeys[i].count - 1;

                while(*++ptr == ' ') /**/;

                if(*ptr == ',') {
                    while(*++ptr == ' ') /**/;
                }

                // ready to parse the next one
                break;
            }
        }

        if(!modifierKeys[i].bit) {
            Com_Printf("unknown modifier key name in \"%s\"\n", mods);
            return none;
        }
    }

    for(i = 0; i < NUM_RECOGNISED_MODIFIERS; ++i) {
        if(mask.up & (1 << i)) {
            ++mask.bits;
        }

        if(mask.down & (1 << i)) {
            ++mask.bits;
        }
    }

    return mask;
}

/*
===============
idCmdSystemLocal::checkKeysDown
===============
*/
sint idCmdSystemLocal::checkKeysDown(modifierMask_t mask) {
    sint i;

    for(i = 0; modifierKeys[i].bit; ++i) {
        if((mask.down & modifierKeys[i].bit) &&
                keys[modifierKeys[i].index].down == 0) {
            // should be pressed, isn't pressed
            return 0;
        }

        if((mask.up   & modifierKeys[i].bit) && keys[modifierKeys[i].index].down) {
            // should not be pressed, is pressed
            return 0;
        }
    }

    // all (not) pressed as requested
    return 1;
}

/*
===============
idCmdSystemLocal::ModCase

Takes a sequence of modifier/command pairs
Executes the command for the first matching modifier set
===============
*/
void idCmdSystemLocal::ModCase(void) {
    sint argc = cmdSystemLocal.Argc();
    sint index = 0;
    sint max = 0;
    sint count = (argc - 1) / 2;   // round down :-)
    valueType *v;

    sint mods[1 << NUM_RECOGNISED_MODIFIERS];

    // want 'modifierMask_t mods[argc / 2 - 1];' (variable array, C99)
    // but MSVC apparently doesn't like that
    if(argc < 3) {
        Com_Printf("modcase <modifiers> <command> [<modifiers> <command>] ... [<command>]\n");
        return;
    }

    while(index < count) {
        modifierMask_t mask = getModifierMask(cmdSystemLocal.Argv(2 * index + 1));

        if(mask.bits == 0) {
            // parse failure (reported) - abort
            return;
        }

        mods[index] = checkKeysDown(mask) ? mask.bits : 0;

        if(max < mods[index]) {
            max = mods[index];
        }

        ++index;
    }

    // If we have a tail command, use it as default
    v = (argc & 1) ? nullptr : cmdSystemLocal.Argv(argc - 1);

    // Search for a suitable command to execute.
    // Search is done as if the commands are sorted by modifier count
    // (descending) then parameter index no. (ascending).
    for(; max > 0; --max) {
        sint i;

        for(i = 0; i < index; ++i) {
            if(mods[i] == max) {
                v = cmdSystemLocal.Argv(2 * i + 2);
                goto found;
            }
        }
    }

found:

    if(v) {
        if(*v == '/' || *v == '\\') {
            cmdBufferLocal.InsertText(va("%s\n", v + 1));
        } else {
            cmdBufferLocal.InsertText(va("vstr %s\n", v));
        }
    }
}


/*
===============
idCmdSystemLocal::If_f

Compares two values, if true executes the third argument, if false executes the forth
===============
*/
void idCmdSystemLocal::If(void) {
    sint v1;
    sint v2;
    sint argc;
    valueType *v = nullptr;
    valueType *vt;
    valueType *vf = nullptr;
    valueType *op;
#ifndef DEDICATED
    modifierMask_t mask;
#endif

    switch(argc = cmdSystemLocal.Argc()) {
        case 4:
            vf = cmdSystemLocal.Argv(3);

        case 3:
            vt = cmdSystemLocal.Argv(2);
#ifdef DEDICATED
            Com_Printf("if <modifiers>... is not supported on the server -- assuming true.\n");
            v = vt;
#else
            v = cmdSystemLocal.Argv(1);
            mask = getModifierMask(v);

            if(mask.bits == 0) {
                return;
            }

            v = checkKeysDown(mask) ? vt : vf;
#endif
            break;

        case 6:
            vf = cmdSystemLocal.Argv(5);

        case 5:
            vt = cmdSystemLocal.Argv(4);
            v1 = atoi(cmdSystemLocal.Argv(1));
            op = cmdSystemLocal.Argv(2);
            v2 = atoi(cmdSystemLocal.Argv(3));

            if((!strcmp(op, "=") && v1 == v2) ||
                    (!strcmp(op, "!=") && v1 != v2) ||
                    (!strcmp(op, "<") && v1 <  v2) ||
                    (!strcmp(op, "<=") && v1 <= v2) ||
                    (!strcmp(op, ">") && v1 >  v2) ||
                    (!strcmp(op, ">=") && v1 >= v2)) {
                v = vt;
            } else if((!strcmp(op, "=") && v1 != v2) ||
                      (!strcmp(op, "!=") && v1 == v2) ||
                      (!strcmp(op, "<") && v1 >= v2) ||
                      (!strcmp(op, "<=") && v1 >  v2) ||
                      (!strcmp(op, ">") && v1 <= v2) ||
                      (!strcmp(op, ">=") && v1 <  v2)) {
                v = vf;
            } else {
                Com_Printf("invalid operator in if command. valid operators are = != < > >= <=\n");
                return;
            }

            break;

        default:
            Com_Printf("if <value1> <operator> <value2> <cmdthen> (<cmdelse>) : compares the first two values and executes <cmdthen> if true, <cmdelse> if false\n"
                       "if <modifiers> <cmdthen> (<cmdelse>) : check if modifiers are (not) pressed\n"
                       "-- modifiers are %s\n"
                       "-- commands are cvar names unless prefixed with / or \\\n",
                       modifierList);
            return;
    }

    if(v) {
        if(*v == '/' || *v == '\\') {
            cmdBufferLocal.InsertText(va("%s\n", v + 1));
        } else {
            cmdBufferLocal.InsertText(va("vstr %s\n", v));
        }
    }
}

/*
===============
idCmdSystemLocal::Math

Compares two cvars, if true vstr the third, if false vstr the forth
===============
*/
void idCmdSystemLocal::Math(void) {
    valueType *v;
    valueType *v1;
    valueType *v2;
    valueType *op;

    if(cmdSystemLocal.Argc() == 3) {
        v = cmdSystemLocal.Argv(1);
        op = cmdSystemLocal.Argv(2);

        if(!strcmp(op, "++")) {
            cvarSystem->SetValueSafe(v, (cvarSystem->VariableValue(v) + 1));
        } else if(!strcmp(op, "--")) {
            cvarSystem->SetValueSafe(v, (cvarSystem->VariableValue(v) - 1));
        } else {
            Com_Printf("math <variableToSet> = <variable1> <operator> <variable2>\nmath <variableToSet> <operator> <variable1>\nmath <variableToSet> ++\nmath <variableToSet> --\nvalid operators are + - * / \n");
            return;
        }
    } else if(cmdSystemLocal.Argc() == 4) {
        v = cmdSystemLocal.Argv(1);
        op = cmdSystemLocal.Argv(2);
        v1 = cmdSystemLocal.Argv(3);

        if(!strcmp(op, "+")) {
            cvarSystem->SetValueSafe(v,
                                     (cvarSystem->VariableValue(v) + cvarSystem->VariableValue(v1)));
        } else if(!strcmp(op, "-")) {
            cvarSystem->SetValueSafe(v,
                                     (cvarSystem->VariableValue(v) - cvarSystem->VariableValue(v1)));
        } else if(!strcmp(op, "*")) {
            cvarSystem->SetValueSafe(v,
                                     (cvarSystem->VariableValue(v) * cvarSystem->VariableValue(v1)));
        } else if(!strcmp(op, "/")) {
            if(!(cvarSystem->VariableValue(v1) == 0)) {
                cvarSystem->SetValueSafe(v,
                                         (cvarSystem->VariableValue(v) / cvarSystem->VariableValue(v1)));
            }
        } else {
            Com_Printf("math <variableToSet> = <variable1> <operator> <variable2>\nmath <variableToSet> <operator> <variable1>\nmath <variableToSet> ++\nmath <variableToSet> --\nvalid operators are + - * / \n");
            return;
        }
    } else if(cmdSystemLocal.Argc() == 6) {
        v = cmdSystemLocal.Argv(1);
        v1 = cmdSystemLocal.Argv(3);
        op = cmdSystemLocal.Argv(4);
        v2 = cmdSystemLocal.Argv(5);

        if(!strcmp(op, "+")) {
            cvarSystem->SetValueSafe(v,
                                     (cvarSystem->VariableValue(v1) + cvarSystem->VariableValue(v2)));
        } else if(!strcmp(op, "-")) {
            cvarSystem->SetValueSafe(v,
                                     (cvarSystem->VariableValue(v1) - cvarSystem->VariableValue(v2)));
        } else if(!strcmp(op, "*")) {
            cvarSystem->SetValueSafe(v,
                                     (cvarSystem->VariableValue(v1) * cvarSystem->VariableValue(v2)));
        } else if(!strcmp(op, "/")) {
            if(!(cvarSystem->VariableValue(v2) == 0)) {
                cvarSystem->SetValueSafe(v,
                                         (cvarSystem->VariableValue(v1) / cvarSystem->VariableValue(v2)));
            }
        } else {
            Com_Printf("math <variableToSet> = <variable1> <operator> <variable2>\nmath <variableToSet> <operator> <variable1>\nmath <variableToSet> ++\nmath <variableToSet> --\nvalid operators are + - * / \n");
            return;
        }
    } else {
        Com_Printf("math <variableToSet> = <variable1> <operator> <variable2>\nmath <variableToSet> <operator> <variable1>\nmath <variableToSet> ++\nmath <variableToSet> --\nvalid operators are + - * / \n");
        return;
    }
}


/*
===============
idCmdSystemLocal::Strcmp

Compares two strings, if true executes the third argument, if false executes the forth
===============
*/
void idCmdSystemLocal::Strcmp(void) {
    valueType *v;
    valueType *v1;
    valueType *v2;
    valueType *vt;
    valueType *vf;
    valueType *op;

    if((cmdSystemLocal.Argc() == 6) || (cmdSystemLocal.Argc() == 5)) {
        v1 = cmdSystemLocal.Argv(1);
        op = cmdSystemLocal.Argv(2);
        v2 = cmdSystemLocal.Argv(3);
        vt = cmdSystemLocal.Argv(4);

        if((!strcmp(op, "=") && !strcmp(v1, v2)) || (!strcmp(op, "!=") &&
                strcmp(v1, v2))) {
            v = vt;
        } else if((!strcmp(op, "=") && strcmp(v1, v2)) || (!strcmp(op, "!=") &&
                  !strcmp(v1, v2))) {
            if(cmdSystemLocal.Argc() == 6) {
                vf = cmdSystemLocal.Argv(5);
                v = vf;
            } else {
                return;
            }
        } else {
            Com_Printf("invalid operator in strcmp command. valid operators are = != \n");
            return;
        }
    } else {
        Com_Printf("Strcmp <string1> <operator> <string2> <cmdthen> (<cmdelse>) : compares the first two strings and executes <cmdthen> if true, <cmdelse> if false\n");
        return;
    }

    cmdBufferLocal.InsertText(va("%s\n", v));
}

/*
===============
idCmdSystemLocal::Concat

concatenates cvars together
===============
*/
void idCmdSystemLocal::Concat(void) {
    valueType *v;
    valueType *v1;
    valueType *v2;
    valueType vc[MAX_CVAR_VALUE_STRING];

    if(cmdSystemLocal.Argc() != 4) {
        Com_Printf("concat <variableToSet> <variable1> <variable2> : concatenates variable1 and variable2 and sets the result to variableToSet\n");
        return;
    }

    v  = cmdSystemLocal.Argv(1);
    v1 = cvarSystem->VariableString(cmdSystemLocal.Argv(2));
    v2 = cvarSystem->VariableString(cmdSystemLocal.Argv(3));

    Q_vsprintf_s(vc, sizeof(vc), sizeof(vc), "%s%s", v1, v2);

    cvarSystem->Set(cmdSystemLocal.Argv(1), vc);
}

/*
===============
idCmdSystemLocal::Calc

Does math and displays the value into the chat/console, this is used for basic math functions
===============
*/
void idCmdSystemLocal::Calc(void) {
    valueType *arg1;
    valueType *arg2;
    valueType *func;

    if(cmdSystemLocal.Argc() < 3) {
        Com_Printf("calc <number> <function> <number>, accepted functions: +, -, /, */x\n");
        return;
    }

    arg1 = cmdSystemLocal.Argv(1);
    func = cmdSystemLocal.Argv(2);
    arg2 = cmdSystemLocal.Argv(3);

    // Add
    if(!strcmp(func, "+")) {
        Com_Printf("%s %s %s = %f\n", arg1, func, arg2, (atof(arg1) + atof(arg2)));
        return;
    }

    // Subtract
    else if(!strcmp(func, "-")) {
        Com_Printf("%s %s %s = %f\n", arg1, func, arg2, (atof(arg1) - atof(arg2)));
        return;
    }

    // Divide
    else if(!strcmp(func, "/")) {
        if(atof(arg2) == 0.f) {
            Com_Printf("Cannot divide by zero!\n");
            return;
        }

        Com_Printf("%s %s %s = %f\n", arg1, func, arg2, (atof(arg1) / atof(arg2)));
        return;
    }

    // Multiply
    else if(!strcmp(func, "*") || !strcmp(func, "x")) {
        Com_Printf("%s %s %s = %f\n", arg1, func, arg2, (atof(arg1) * atof(arg2)));
        return;
    }

    // Invalid function, help the poor guy out
    Com_Printf("calc <number> <function> <number>, accepted functions: +, -, /, */x\n");
}

/*
===============
idCmdSystemLocal::Echo

Just prints the rest of the line to the console
===============
*/
void idCmdSystemLocal::Echo(void) {
    Com_Printf("%s\n", cmdSystemLocal.Args());
}

/*
===============
idCmdSystemLocal::Undelay

Removes a pending delay with a given name
===============
*/
void idCmdSystemLocal::Undelay(void) {
    sint i;
    valueType *find, *limit;

    // Check if the call is valid
    if(cmdSystemLocal.Argc() < 1) {
        Com_Printf("undelay <name> (command)\nremoves all commands with <name> in them.\nif (command) is specified, the removal will be limited only to delays whose commands contain (command).\n");
        return;
    }

    find = cmdSystemLocal.Argv(1);
    limit = cmdSystemLocal.Argv(2);

    for(i = 0; (i < MAX_DELAYED_COMMANDS); i++) {
        if(delayedCommands[i].delay != CMD_DELAY_UNUSED &&
                ::strstr(delayedCommands[i].name, find)
                && ::strstr(delayedCommands[i].text,
                            limit)) {  // the limit test will always pass if limit is a null string
            delayedCommands[i].delay = CMD_DELAY_UNUSED;
        }
    }
}

/*
===============
idCmdSystemLocal::UndelayAll

Removes all pending delays
===============
*/
void idCmdSystemLocal::UndelayAll(void) {
    sint i;

    for(i = 0; (i < MAX_DELAYED_COMMANDS); i++) {
        delayedCommands[i].delay = CMD_DELAY_UNUSED;
    }
}

/*
===============
idCmdSystemLocal::Delay

Delays a comand
===============
*/
void idCmdSystemLocal::Delay(void) {
    sint i, delay, type, lastchar;
    valueType *raw_delay, *name, *cmd;
    bool availiable_cmd = false;

    // Check if the call is valid
    if(cmdSystemLocal.Argc() < 2) {
        Com_Printf("delay (name) <delay in milliseconds> <command>\ndelay <delay in frames>f <command>\nexecutes <command> after the delay\n");
        return;
    }

    raw_delay = cmdSystemLocal.Argv(1);

    if(!isdigit(raw_delay[0])) {
        name = raw_delay;
        raw_delay = cmdSystemLocal.Argv(2);
        cmd = cmdSystemLocal.ArgsFrom(3);
    } else {
        name = "";
        cmd = cmdSystemLocal.ArgsFrom(2);
    }

    delay = atoi(raw_delay);

    if(delay < 1) {
        Com_Printf("delay: the delay must be a positive integer");
        return;
    }

    //search for an unused slot
    for(i = 0; (i < MAX_DELAYED_COMMANDS); i++) {
        if(delayedCommands[i].delay == CMD_DELAY_UNUSED) {
            availiable_cmd = true;
            break;
        }
    }

    if(!availiable_cmd) {
        Com_Printf("WARNING: Maximum amount of delayed commands reached.");
        return;
    }

    lastchar = strlen(raw_delay) - 1;

    if(raw_delay[ lastchar ] == 'f') {
        delay += CMD_DELAY_FRAME_FIRE;
        type = CMD_DELAY_FRAME;
    } else {
        type = CMD_DELAY_MSEC;
        delay += idsystem->Milliseconds();
    }

    delayedCommands[i].delay = delay;
    delayedCommands[i].type = (cmdDelayType_t)type;

    Q_strncpyz(delayedCommands[i].text, cmd, MAX_CMD_LINE);
    Q_strncpyz(delayedCommands[i].name, name, MAX_CMD_LINE);
}

/*
===============
idCmdSystemLocal::Random

Give a random integer
===============
*/
void idCmdSystemLocal::Random(void) {
    sint v1, v2;

    if(cmdSystemLocal.Argc() == 4) {
        v1 = atoi(cmdSystemLocal.Argv(2));
        v2 = atoi(cmdSystemLocal.Argv(3));

        cvarSystem->SetValueLatched(cmdSystemLocal.Argv(1),
                                    static_cast<sint>(rand() / static_cast<float32>(RAND_MAX) * (MAX(v1,
                                            v2) - MIN(v1, v2)) + MIN(v1, v2)));
    } else {
        Com_Printf("random <variableToSet> <value1> <value2>\n");
    }
}

/*
=============================================================================
ALIASES
=============================================================================
*/

/*
============
idCmdSystemLocal::RunAlias
============
*/
void idCmdSystemLocal::RunAlias(void) {
    valueType *name = cmdSystemLocal.Argv(0);
    valueType *args = cmdSystemLocal.ArgsFrom(1);
    cmd_alias_t *alias;

    // Find existing alias
    for(alias = cmd_aliases; alias; alias = alias->next) {
        if(!Q_stricmp(name, alias->name)) {
            break;
        }
    }

    if(!alias) {
        Com_Error(ERR_FATAL, "Alias: Alias %s doesn't exist", name);
    }

    cmdBufferLocal.InsertText(va("%s %s", alias->exec, args));
}

/*
============
idCmdSystemLocal::WriteAliases
============
*/
void idCmdSystemLocal::WriteAliases(fileHandle_t f) {
    valueType buffer[1024] = "clearaliases\n";
    cmd_alias_t *alias = cmd_aliases;

    fileSystem->Write(buffer, strlen(buffer), f);

    while(alias) {
        Q_vsprintf_s(buffer, sizeof(buffer), sizeof(buffer), "alias %s \"%s\"\n",
                     alias->name, EscapeString(alias->exec));
        fileSystem->Write(buffer, strlen(buffer), f);
        alias = alias->next;
    }
}

/*
============
idCmdSystemLocal::AliasList
============
*/
void idCmdSystemLocal::AliasList(void) {
    sint    i;
    cmd_alias_t *alias;
    valueType *match;

    if(cmdSystemLocal.Argc() > 1) {
        match = cmdSystemLocal.Argv(1);
    } else {
        match = nullptr;
    }

    i = 0;

    for(alias = cmd_aliases; alias; alias = alias->next) {
        if(match && !Com_Filter(match, alias->name, false)) {
            continue;
        }

        Com_Printf("%s ==> %s\n", alias->name, alias->exec);

        i++;
    }

    Com_Printf("%i aliases\n", i);
}

/*
============
idCmdSystemLocal::ClearAliases
============
*/
void idCmdSystemLocal::ClearAliases(void) {
    cmd_alias_t *alias = cmd_aliases;
    cmd_alias_t *next;

    while(alias) {
        next = alias->next;

        cmdSystemLocal.RemoveCommand(alias->name);

        Z_Free(alias->name);
        Z_Free(alias->exec);
        Z_Free(alias);

        alias = next;
    }

    cmd_aliases = nullptr;

    // update autogen.cfg
    cvar_modifiedFlags |= CVAR_ARCHIVE;
}

/*
============
idCmdSystemLocal::UnAlias
============
*/
void idCmdSystemLocal::UnAlias(void) {
    cmd_alias_t *alias, **back;
    pointer name;

    // Get args
    if(cmdSystemLocal.Argc() < 2) {
        Com_Printf("unalias <name> : delete an alias\n");
        return;
    }

    name = cmdSystemLocal.Argv(1);

    back = &cmd_aliases;

    while(1) {
        alias = *back;

        if(!alias) {
            return;
        }

        if(!Q_stricmp(name, alias->name)) {
            *back = alias->next;

            Z_Free(alias->name);
            Z_Free(alias->exec);
            Z_Free(alias);

            cmdSystemLocal.RemoveCommand(name);

            // update autogen.cfg
            cvar_modifiedFlags |= CVAR_ARCHIVE;

            return;
        }

        back = &alias->next;
    }
}

/*
============
idCmdSystemLocal::Alias
============
*/
void idCmdSystemLocal::Alias(void) {
    cmd_alias_t *alias;
    pointer name;

    // Get args
    if(cmdSystemLocal.Argc() < 2) {
        Com_Printf("alias <name> : show an alias\n");
        Com_Printf("alias <name> <exec> : create an alias\n");
        return;
    }

    name = cmdSystemLocal.Argv(1);

    // Find existing alias
    for(alias = cmd_aliases; alias; alias = alias->next) {
        if(!Q_stricmp(name, alias->name)) {
            break;
        }
    }

    // Modify/create an alias
    if(cmdSystemLocal.Argc() > 2) {
        cmd_function_t *cmd;

        // Crude protection from infinite loops
        if(!Q_stricmp(cmdSystemLocal.Argv(2), name)) {
            Com_Printf("Can't make an alias to itself\n");
            return;
        }

        // Don't allow overriding builtin commands
        cmd = cmdSystemLocal.FindCommand(name);

        if(cmd && cmd->function != RunAlias) {
            Com_Printf("Can't override a builtin function with an alias\n");
            return;
        }

        // Create/update an alias
        if(!alias) {
            alias = (cmd_alias_t *)S_Malloc(sizeof(cmd_alias_t));
            alias->name = CopyString(name);
            alias->exec = CopyString(cmdSystemLocal.ArgsFrom(2));
            alias->next = cmd_aliases;
            cmd_aliases = alias;
            cmdSystemLocal.AddCommand(name, RunAlias, nullptr);
        } else {
            // Reallocate the exec string
            Z_Free(alias->exec);
            alias->exec = CopyString(cmdSystemLocal.ArgsFrom(2));
            cmdSystemLocal.AddCommand(name, RunAlias, nullptr);
        }
    }

    // Show the alias
    if(!alias) {
        Com_Printf("Alias %s does not exist\n", name);
    } else if(cmdSystemLocal.Argc() == 2) {
        Com_Printf("%s ==> %s\n", alias->name, alias->exec);
    }

    // update autogen.cfg
    cvar_modifiedFlags |= CVAR_ARCHIVE;
}

/*
============
idCmdSystemLocal::AliasCompletion
============
*/
void idCmdSystemLocal::AliasCompletion(void(*callback)(pointer s)) {
    cmd_alias_t    *alias;

    for(alias = cmd_aliases ; alias ; alias = alias->next) {
        callback(alias->name);
    }
}

/*
============
idCmdSystemLocal::DelayCompletion
============
*/
void idCmdSystemLocal::DelayCompletion(void(*callback)(pointer s)) {
    sint i;

    for(i = 0; i < MAX_DELAYED_COMMANDS; i++) {
        if(delayedCommands[i].delay != CMD_DELAY_UNUSED) {
            callback(delayedCommands[i].name);
        }
    }
}

/*
=============================================================================
COMMAND EXECUTION
=============================================================================
*/

/*
============
idCmdSystemLocal::SaveCmdContext
============
*/
void idCmdSystemLocal::SaveCmdContext(void) {
    ::memcpy(&savedCmd, &cmd, sizeof(cmdContext_t));
}

/*
============
idCmdSystemLocal::RestoreCmdContext
============
*/
void idCmdSystemLocal::RestoreCmdContext(void) {
    ::memcpy(&cmd, &savedCmd, sizeof(cmdContext_t));
}

/*
============
idCmdSystemLocal::Argc
============
*/
sint idCmdSystemLocal::Argc(void) {
    return cmd.argc;
}

/*
============
idCmdSystemLocal::Argv
============
*/
valueType *idCmdSystemLocal::Argv(sint arg) {
    if(arg >= cmd.argc) {
        return static_cast<valueType *>("\0");
    }

    return cmd.argv[arg];
}

/*
============
idCmdSystemLocal::ArgvBuffer

The interpreted versions use this because
they can't have pointers returned to them
============
*/
void idCmdSystemLocal::ArgvBuffer(sint arg, valueType *buffer,
                                  uint64 bufferLength) {
    Q_strncpyz(buffer, cmdSystemLocal.Argv(arg), bufferLength);
}

/*
============
idCmdSystemLocal::Args

Returns a single string containing argv(1) to argv(argc()-1)
============
*/
valueType *idCmdSystemLocal::Args(void) {
    sint i;
    static valueType cmd_args[MAX_STRING_CHARS];

    cmd_args[0] = 0;

    for(i = 1; i < cmd.argc; i++) {
        ::strcat(cmd_args, cmd.argv[i]);

        if(i != cmd.argc - 1) {
            ::strcat(cmd_args, " ");
        }
    }

    return cmd_args;
}

/*
============
idCmdSystemLocal::Args

Returns a single string containing argv(arg) to argv(argc()-1)
============
*/
valueType *idCmdSystemLocal::ArgsFrom(sint arg) {
    sint i;
    static valueType cmd_args[BIG_INFO_STRING];

    cmd_args[0] = 0;

    if(arg < 0) {
        arg = 0;
    }

    for(i = arg; i < cmd.argc; i++) {
        ::strcat(cmd_args, cmd.argv[i]);

        if(i != cmd.argc - 1) {
            ::strcat(cmd_args, " ");
        }
    }

    return cmd_args;
}

/*
============
idCmdSystemLocal::ArgsBuffer

The interpreted versions use this because
they can't have pointers returned to them
============
*/
void idCmdSystemLocal::ArgsBuffer(valueType *buffer, uint64 bufferLength) {
    Q_strncpyz(buffer, cmdSystemLocal.Args(), bufferLength);
}

/*
============
Cmd_LiteralArgsBuffer

The interpreted versions use this because
they can't have pointers returned to them
============
*/
void idCmdSystemLocal::LiteralArgsBuffer(valueType *buffer,
        uint64 bufferLength) {
    Q_strncpyz(buffer, cmd.cmd, bufferLength);
}

/*
============
idCmdSystemLocal::Cmd

Retrieve the unmodified command string
For rcon use when you want to transmit without altering quoting
ATVI Wolfenstein Misc #284
============
*/
valueType *idCmdSystemLocal::Cmd(void) {
    return cmd.cmd;
}

/*
============
idCmdSystemLocal::FromNth

Retrieve the unmodified command string
For rcon use when you want to transmit without altering quoting
ATVI Wolfenstein Misc #284
============
*/
valueType *idCmdSystemLocal::FromNth(sint count) {
    valueType *ret = cmd.cmd - 1;
    sint i = 0, q = 0;

    while(count && *++ret) {
        if(!q && *ret == ' ') {
            // space found outside quotation marks
            i = 1;
        }

        if(i && *ret != ' ') {
            // non-space found after space outside quotation marks
            i = 0;

            // one word fewer to scan
            --count;
        }

        if(*ret == '"') {
            // found a quotation mark
            q = !q;
        } else if(*ret == '\\' && ret[1] == '"') {
            ++ret;
        }
    }

    return ret;
}


/*
============
idCmdSystemLocal::EscapeString

Escape all \$ in a string into \$$
============
*/
valueType *idCmdSystemLocal::EscapeString(pointer in) {
    static valueType buffer[MAX_STRING_CHARS];
    valueType *out = buffer;

    while(*in) {
        if(out + 3 - buffer >= sizeof(buffer)) {
            break;
        }

        if(in[0] == '\\' && in[1] == '$') {
            out[0] = '\\';
            out[1] = '$';
            out[2] = '$';
            in += 2;
            out += 3;
        } else {
            *out++ = *in++;
        }
    }

    *out = '\0';

    return buffer;
}
/*
============
idCmdSystemLocal::TokenizeString2

Parses the given string into command line tokens.
The text is copied to a seperate buffer and 0 characters
are inserted in the apropriate place, The argv array
will point into this temporary buffer.

// NOTE TTimo define that to track tokenization issues
============
*/
//#define TKN_DBG
void idCmdSystemLocal::TokenizeString2(pointer text_in, bool ignoreQuotes,
                                       bool parseCvar) {
    valueType *text;
    valueType *textOut;
    pointer cvarName;
    valueType buffer[ BIG_INFO_STRING ];

#ifdef TKN_DBG
    // FIXME TTimo blunt hook to try to find the tokenization of userinfo
    Com_DPrintf("idCmdSystemLocal::TokenizeString: %s\n", text_in);
#endif

    // clear previous args
    cmd.argc = 0;
    cmd.cmd[ 0 ] = '\0';

    if(!text_in) {
        return;
    }

    // parse for cvar substitution
    if(parseCvar) {
        Q_strncpyz(buffer, text_in, sizeof(buffer));
        text = buffer;
        textOut = cmd.cmd;

        while(*text) {
            if(text[0] != '\\' || text[1] != '$') {
                if(textOut == sizeof(cmd.cmd) + cmd.cmd - 1) {
                    break;
                }

                *textOut++ = *text++;
                continue;
            }

            text += 2;
            cvarName = text;

            while(*text && *text != '\\') {
                text++;
            }

            if(*text == '\\') {
                *text = 0;

                if(cvarSystem->Flags(cvarName) != CVAR_NONEXISTENT) {
                    valueType cvarValue[ MAX_CVAR_VALUE_STRING ];
                    valueType *badchar;

                    cvarSystem->VariableStringBuffer(cvarName, cvarValue, sizeof(cvarValue));

                    do {
                        badchar = strchr(cvarValue, ';');

                        if(badchar) {
                            *badchar = '.';
                        } else {
                            badchar = strchr(cvarValue, '\n');

                            if(badchar) {
                                *badchar = '.';
                            }
                        }
                    } while(badchar);

                    Q_strncpyz(textOut, cvarValue, sizeof(cmd.cmd) - (textOut - cmd.cmd));

                    while(*textOut) {
                        textOut++;
                    }

                    if(textOut == sizeof(cmd.cmd) + cmd.cmd - 1) {
                        break;
                    }
                } else {
                    cvarName -= 2;

                    while(*cvarName && textOut < sizeof(cmd.cmd) + cmd.cmd - 1) {
                        *textOut++ = *cvarName++;
                    }

                    if(textOut == sizeof(cmd.cmd) + cmd.cmd - 1) {
                        break;
                    }

                    *textOut++ = '\\';
                }

                text++;
            } else {
                cvarName -= 2;

                while(*cvarName && textOut < sizeof(cmd.cmd) + cmd.cmd - 1) {
                    *textOut++ = *cvarName++;
                }

                if(textOut == sizeof(cmd.cmd) + cmd.cmd - 1) {
                    break;
                }
            }
        }

        *textOut = 0;

        // "\$$" --> "\$"
        text = textOut = cmd.cmd;

        while(text[0]) {
            if(text[0] == '\\'  && text[1]  && text[1] == '$' && text[2] &&
                    text[2] == '$') {
                textOut[0] = '\\';
                textOut[1] = '$';
                textOut += 2;
                text += 3;
            } else {
                *textOut++ = *text++;
            }
        }

        *textOut = '\0';
    } else {
        Q_strncpyz(cmd.cmd, text_in, sizeof(cmd.cmd));
    }

    text = cmd.cmd;
    textOut = cmd.tokenized;

    while(1) {
        if(cmd.argc == MAX_STRING_TOKENS) {
            // this is usually something malicious
            return;
        }

        while(1) {
            // skip whitespace
            while(*text > '\0' && *text <= ' ') {
                text++;
            }

            if(!*text) {
                // all tokens parsed
                return;
            }

            // skip // comments
            if(text[0] == '/' && text[1] == '/') {
                // all tokens parsed
                return;
            }

            // skip /* */ comments
            if(text[0] == '/' && text[1] == '*') {
                while(*text && (text[0] != '*' || text[1] != '/')) {
                    text++;
                }

                if(!*text) {
                    // all tokens parsed
                    return;
                }

                text += 2;
            } else {
                // we are ready to parse a token
                break;
            }
        }

        // handle quote escaping
        if(!ignoreQuotes && text[0] == '\\' && text[1] == '"') {
            *textOut++ = '"';
            text += 2;
            continue;
        }

        // handle quoted strings
        if(!ignoreQuotes && *text == '"') {
            cmd.argv[cmd.argc] = textOut;
            cmd.argc++;
            text++;

            while(*text && *text != '"') {
                if(text[0] == '\\' && text[1] == '"') {
                    *textOut++ = '"';
                    text += 2;
                    continue;
                }

                *textOut++ = *text++;
            }

            *textOut++ = 0;

            if(!*text) {
                // all tokens parsed
                return;
            }

            text++;

            continue;
        }

        // regular token
        cmd.argv[cmd.argc] = textOut;
        cmd.argc++;

        // skip until whitespace, quote, or command
        while(*text > ' ' || *text < '\0') {
            if(!ignoreQuotes && text[0] == '\\' && text[1] == '"') {
                *textOut++ = '"';
                text += 2;
                continue;
            }

            if(!ignoreQuotes && text[0] == '"') {
                break;
            }

            if(text[0] == '/' && text[1] == '/') {
                break;
            }

            // skip /* */ comments
            if(text[0] == '/' && text[1] == '*') {
                break;
            }

            *textOut++ = *text++;
        }

        *textOut++ = 0;

        if(!*text) {
            // all tokens parsed
            return;
        }
    }

}

/*
============
idCmdSystemLocal::TokenizeString
============
*/
void idCmdSystemLocal::TokenizeString(pointer text_in) {
    TokenizeString2(text_in, false, false);
}

/*
============
idCmdSystemLocal::TokenizeStringIgnoreQuotes
============
*/
void idCmdSystemLocal::TokenizeStringIgnoreQuotes(pointer text_in) {
    TokenizeString2(text_in, true, false);
}

/*
============
idCmdSystemLocal::TokenizeStringParseCvar
============
*/
void idCmdSystemLocal::TokenizeStringParseCvar(pointer text_in) {
    TokenizeString2(text_in, false, true);
}

/*
============
idCmdSystemLocal::AddCommand
============
*/
void idCmdSystemLocal::AddCommand(pointer cmd_name, xcommand_t function,
                                  pointer cmd_desc) {
    cmd_function_t *cmd;

    // fail if the command already exists
    for(cmd = cmd_functions; cmd; cmd = cmd->next) {
        if(!strcmp(cmd_name, cmd->name)) {
            // allow completion-only commands to be silently doubled
            if(function != nullptr) {
                Com_Printf("idCmdSystemLocal::AddCommand: %s already defined\n", cmd_name);
            }

            return;
        }
    }

    // use a small malloc to avoid zone fragmentation
    cmd = (cmd_function_t *)S_Malloc(sizeof(cmd_function_t));
    cmd->name = CopyString(cmd_name);
    cmd->desc = CopyString(cmd_desc);
    cmd->function = function;
    cmd->next = cmd_functions;
    cmd->complete = nullptr;
    cmd_functions = cmd;
}

/*
============
idCmdSystemLocal::SetCommandCompletionFunc
============
*/
void idCmdSystemLocal::SetCommandCompletionFunc(pointer command,
        completionFunc_t complete) {
    cmd_function_t *cmd;

    for(cmd = cmd_functions; cmd; cmd = cmd->next) {
        if(!Q_stricmp(command, cmd->name)) {
            cmd->complete = complete;
        }
    }
}

/*
============
idCmdSystemLocal::RemoveCommand
============
*/
void idCmdSystemLocal::RemoveCommand(pointer cmd_name) {
    cmd_function_t **back = &cmd_functions;

    while(1) {
        cmd_function_t *cmd = *back;

        if(!cmd) {
            // command wasn't active
            return;
        }

        if(!strcmp(cmd_name, cmd->name)) {
            *back = cmd->next;

            if(cmd->name) {
                Z_Free(cmd->name);
            }

            Z_Free(cmd);
            return;
        }

        back = &cmd->next;
    }
}

/*
============
idCmdSystemLocal::CommandCompletion
============
*/
void idCmdSystemLocal::CommandCompletion(void (*callback)(pointer s)) {
    cmd_function_t *cmd;

    for(cmd = cmd_functions; cmd; cmd = cmd->next) {
        callback(cmd->name);
    }
}

/*
============
idCmdSystemLocal::CompleteArgument
============
*/
void idCmdSystemLocal::CompleteArgument(pointer command, valueType *args,
                                        sint argNum) {
    cmd_function_t *cmd;

    for(cmd = cmd_functions; cmd; cmd = cmd->next) {
        if(!Q_stricmp(command, cmd->name) && cmd->complete) {
            cmd->complete(args, argNum);
        }
    }
}

/*
============
idCmdSystemLocal::ExecuteString

A complete command line has been parsed, so try to execute it
============
*/
void idCmdSystemLocal::ExecuteString(pointer text) {
    // execute the command line
    TokenizeStringParseCvar(text);

    if(!cmdSystemLocal.Argc()) {
        // no tokens
        return;
    }

    // check registered command functions
    cmd_function_t *cmdFunc, **prev;

    for(prev = &cmd_functions; *prev; prev = &cmdFunc->next) {
        cmdFunc = *prev;

        if(!Q_stricmp(cmd.argv[0], cmdFunc->name)) {
            // rearrange the links so that the command will be
            // near the head of the list next time it is used
            *prev = cmdFunc->next;
            cmdFunc->next = cmd_functions;
            cmd_functions = cmdFunc;

            // perform the action
            if(!cmdFunc->function) {
                // let the cgame or game handle it
                break;
            } else {
                cmdFunc->function();
            }

            return;
        }
    }

    // check cvars
    if(cvarSystem->Command()) {
        return;
    }

    // check client game commands
    if(com_cl_running && com_cl_running->integer
#ifndef DEDICATED
            && clientGameSystem->GameCommand()
#endif // !DEDICATED
      ) {
        return;
    }

    // check server game commands
    if(com_sv_running && com_sv_running->integer &&
            serverGameSystem->GameCommand()) {
        return;
    }

    // check ui commands
    if(com_cl_running && com_cl_running->integer
#ifndef DEDICATED
            && clientGUISystem->GameCommand()
#endif // !DEDICATED
      ) {
        return;
    }

    // send it as a server command if we are connected
    // this will usually result in a chat message
    CL_ForwardCommandToServer(text);
}

/*
============
idCmdSystemLocal::List
============
*/
void idCmdSystemLocal::List(void) {
    sint i;
    cmd_function_t *cmd;
    valueType *match;

    if(cmdSystemLocal.Argc() > 1) {
        match = cmdSystemLocal.Argv(1);
    } else {
        match = nullptr;
    }

    i = 0;

    for(cmd = cmd_functions; cmd; cmd = cmd->next) {
        if(match && !Com_Filter(match, cmd->name, false)) {
            continue;
        }

        Com_Printf("%s\n", cmd->name);
        i++;
    }

    Com_Printf("%i commands\n", i);
}

/*
==================
idCmdSystemLocal::CompleteCfgName
==================
*/
extern pointer completionString;
void idCmdSystemLocal::CompleteCfgName(valueType *args, sint argNum) {
    if(argNum == 2) {
        sint i = 0;
        valueType *s = args, *token = s;
        pointer pos = nullptr;

        for(i = 0; i < argNum; i++) {
            s = COM_Parse(&token);
        }

        if((pos = Q_stristr(s, "/"))) {
            valueType realdir[MAX_QPATH] = { 0 };
            Q_strncpyz(realdir, s, /*strlen(s)-*/(pos - s) + 1);
            completionString = pos + 1;
            cmdCompletionSystem->CompleteFilename(realdir, "cfg", true);
        } else {
            cmdCompletionSystem->CompleteFilename("", "cfg", true);
        }
    }
}

/*
==================
idCmdSystemLocal::CompleteAliasName
==================
*/
void idCmdSystemLocal::CompleteAliasName(valueType *args, sint argNum) {
    if(argNum == 2) {
        cmdCompletionSystem->CompleteAlias();
    }
}

/*
==================
idCmdSystemLocal::CompleteConcat
==================
*/
void idCmdSystemLocal::CompleteConcat(valueType *args, sint argNum) {
    // Skip
    valueType *p = Com_SkipTokens(args, argNum - 1, " ");

    if(p > args) {
        cmdCompletionSystem->CompleteCommand(p, false, true);
    }
}

/*
==================
idCmdSystemLocal::CompleteIf
==================
*/
void idCmdSystemLocal::CompleteIf(valueType *args, sint argNum) {
    if(argNum == 5 || argNum == 6) {
        // Skip
        valueType *p = Com_SkipTokens(args, argNum - 1, " ");

        if(p > args) {
            cmdCompletionSystem->CompleteCommand(p, false, true);
        }
    }
}

/*
==================
idCmdSystemLocal::CompleteDelay
==================
*/
void idCmdSystemLocal::CompleteDelay(valueType *args, sint argNum) {
    if(argNum == 3 || argNum == 4) {
        // Skip "delay "
        valueType *p = Com_SkipTokens(args, 1, " ");

        if(p > args) {
            cmdCompletionSystem->CompleteCommand(p, true, true);
        }
    }
}

/*
==================
idCmdSystemLocal::CompleteUnDelay
==================
*/
void idCmdSystemLocal::CompleteUnDelay(valueType *args, sint argNum) {
    if(argNum == 2) {
        cmdCompletionSystem->CompleteDelay();
    }
}

/*
============
idCmdSystemLocal::Init
============
*/
void idCmdSystemLocal::Init(void) {
    AddCommand("cmdlist", &idCmdSystemLocal::List,
               "Reports to the console all console commands listed.");
    AddCommand("exec", &idCmdSystemLocal::Exec_f,
               "Loads a configuration file.");
    SetCommandCompletionFunc("exec", &idCmdSystemLocal::CompleteCfgName);
    AddCommand("vstr", &idCmdSystemLocal::Vstr,
               "Executes a variable command created with set, seta, sets or setu.");
    SetCommandCompletionFunc("vstr", &idCVarSystemLocal::CompleteCvarName);
    AddCommand("echo", &idCmdSystemLocal::Echo,
               "Echoes text to the console on the local machine.");
    AddCommand("wait", &idCmdSystemLocal::Wait,
               "Waits the specified number of game tics (0.1 seconds?). If no parameter is used it will default to a single game tic. Used in command strings to allow enough time for a command to be executed before proceeding to the next.");
#ifndef DEDICATED
    AddCommand("modcase", &idCmdSystemLocal::ModCase,
               "Executes the command for the first matching modifier set");
#endif
    AddCommand("if", &idCmdSystemLocal::If,
               "Compares two values, if true executes the third argument, if false executes the forth");
    SetCommandCompletionFunc("if", &idCmdSystemLocal::CompleteIf);
    AddCommand("calc", &idCmdSystemLocal::Calc,
               "Does math and displays the value into the chat/console, this is used for basic math functions");
    AddCommand("math", &idCmdSystemLocal::Math,
               "Compares two cvars, if true vstr the third, if false vstr the forth");
    SetCommandCompletionFunc("math", &idCVarSystemLocal::CompleteCvarName);
    AddCommand("concat", &idCmdSystemLocal::Concat,
               "concatenates cvars together");
    SetCommandCompletionFunc("concat", &idCmdSystemLocal::CompleteConcat);
    AddCommand("strcmp", &idCmdSystemLocal::Strcmp,
               "Compares two strings, if true executes the third argument, if false executes the forth");
    SetCommandCompletionFunc("strcmp", &idCmdSystemLocal::CompleteIf);
    AddCommand("alias", &idCmdSystemLocal::Alias, "Show an alias");
    SetCommandCompletionFunc("alias", &idCmdSystemLocal::CompleteAliasName);
    AddCommand("unalias", &idCmdSystemLocal::UnAlias, "Delete an alias");
    SetCommandCompletionFunc("unalias", &idCmdSystemLocal::CompleteAliasName);
    AddCommand("aliaslist", &idCmdSystemLocal::AliasList,
               "Shows list of an alias");
    AddCommand("clearaliases", &idCmdSystemLocal::ClearAliases,
               "Clear the aliases");
    AddCommand("delay", &idCmdSystemLocal::Delay, "Delays a command");
    SetCommandCompletionFunc("delay", &idCmdSystemLocal::CompleteDelay);
    AddCommand("undelay", &idCmdSystemLocal::Undelay,
               "Removes a pending delay with a given name");
    SetCommandCompletionFunc("undelay", &idCmdSystemLocal::CompleteUnDelay);
    AddCommand("undelayAll", &idCmdSystemLocal::UndelayAll,
               "Removes all pending delays");
    AddCommand("random", &idCmdSystemLocal::Random, "Give a random integer");
}

/*
============
idCmdSystemLocal::Shutdown
============
*/
void idCmdSystemLocal::Shutdown(void) {
    RemoveCommand("cmdlist");
    RemoveCommand("exec");
    RemoveCommand("vstr");
    RemoveCommand("echo");
    RemoveCommand("wait");
#ifndef DEDICATED
    RemoveCommand("modcase");
#endif
    RemoveCommand("if");
    RemoveCommand("calc");
    RemoveCommand("math");
    RemoveCommand("concat");
    RemoveCommand("strcmp");
    RemoveCommand("alias");
    RemoveCommand("unalias");
    RemoveCommand("aliaslist");
    RemoveCommand("clearaliases");
    RemoveCommand("delay");
    RemoveCommand("undelay");
    RemoveCommand("undelayAll");
    RemoveCommand("random");
    RemoveCommand("userinfo");
}