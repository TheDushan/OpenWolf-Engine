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
// File name:   clientKeys.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

field_t g_consoleField;
field_t chatField;
bool chat_team;
bool chat_buddy;
bool commandMode;
bool key_overstrikeMode;
bool consoleButtonWasPressed = false;

sint anykeydown;
qkey_t keys[MAX_KEYS];

idClientKeysSystemLocal clientKeysLocal;
idClientKeysSystem *clientKeysSystem = &clientKeysLocal;

/*
===============
idClientKeysSystemLocal::idClientKeysSystemLocal
===============
*/
idClientKeysSystemLocal::idClientKeysSystemLocal(void) {
}

/*
===============
idClientKeysSystemLocal::~idClientKeysSystemLocal
===============
*/
idClientKeysSystemLocal::~idClientKeysSystemLocal(void) {
}


/*
===============
idClientKeysSystemLocal::CompleteCommand

Tab expansion
===============
*/
void idClientKeysSystemLocal::CompleteCommand(void) {
    field_t *edit;
    edit = &g_consoleField;

    // only look at the first token for completion purposes
    cmdSystem->TokenizeString(edit->buffer);

    cmdCompletionSystem->AutoComplete(edit, "]");
}

/*
====================
idClientKeysSystemLocal::DownEventKey

Handles history and console scrollback
====================
*/
void idClientKeysSystemLocal::DownEventKey(sint key) {
    sint conNum = activeCon - con;
    bool isChat = CON_ISCHAT(conNum);

    if(key == K_ESCAPE) {
        cls.keyCatchers ^= KEYCATCH_CONSOLE;
        commandMode = false;
        return;
    }

    // enter finishes the line
    if(key == K_ENTER || key == K_KP_ENTER) {
        if(g_consoleField.buffer[0]) {
            consoleHistorySystem->Add(g_consoleField.buffer);
        }

        clientConsoleSystem->LineAccept();
        return;
    }

    // clear autocompletion buffer on normal key input
    if((key >= K_SPACE && key <= K_BACKSPACE) || (key == K_LEFTARROW) ||
            (key == K_RIGHTARROW) ||
            (key >= K_KP_LEFTARROW && key <= K_KP_RIGHTARROW) ||
            (key >= K_KP_SLASH && key <= K_KP_PLUS) || (key >= K_KP_STAR &&
                    key <= K_KP_EQUALS)) {
    }

    //----(SA)  added some mousewheel functionality to the console
    if((key == K_MWHEELUP && keys[K_SHIFT].down) || (key == K_UPARROW) ||
            (key == K_KP_UPARROW) ||
            ((tolower(key) == 'p') && keys[K_CTRL].down)) {
        Q_strncpyz(g_consoleField.buffer, consoleHistorySystem->Prev(),
                   sizeof(g_consoleField.buffer));
        g_consoleField.cursor = strlen(g_consoleField.buffer);

        if(g_consoleField.cursor >= g_consoleField.widthInChars) {
            g_consoleField.scroll = g_consoleField.cursor - g_consoleField.widthInChars
                                    + 1;
        } else {
            g_consoleField.scroll = 0;
        }

        return;
    }

    //----(SA)  added some mousewheel functionality to the console
    if((key == K_MWHEELDOWN && keys[K_SHIFT].down) || (key == K_DOWNARROW) ||
            (key == K_KP_DOWNARROW) ||
            ((tolower(key) == 'n') && keys[K_CTRL].down)) {
        pointer history = consoleHistorySystem->Next(g_consoleField.buffer);

        if(history) {
            Q_strncpyz(g_consoleField.buffer, history, sizeof(g_consoleField.buffer));
            g_consoleField.cursor = strlen(g_consoleField.buffer);

            if(g_consoleField.cursor >= g_consoleField.widthInChars) {
                g_consoleField.scroll = g_consoleField.cursor - g_consoleField.widthInChars
                                        + 1;
            } else {
                g_consoleField.scroll = 0;
            }
        } else if(g_consoleField.buffer[0]) {
            consoleHistorySystem->Add(g_consoleField.buffer);
            cmdCompletionSystem->Clear(&g_consoleField);
        }

        return;
    }

    // console scroll only if not using commandMode
    if(!commandMode) {
        // console tab switching
        if(key == K_LEFTARROW && keys[K_ALT].down) {
            clientConsoleSystem->ConsoleNext(-1);
            return;
        } else if(key == K_RIGHTARROW && keys[K_ALT].down) {
            clientConsoleSystem->ConsoleNext(1);
            return;
        }

        // console scrolling
        if(key == K_PGUP) {
            clientConsoleSystem->PageUp();
            return;
        }

        if(key == K_PGDN) {
            clientConsoleSystem->PageDown();
            return;
        }

        if(key == K_MWHEELUP) {
            clientConsoleSystem->PageUp();

            // hold <ctrl> to accelerate scrolling
            if(keys[K_CTRL].down) {
                clientConsoleSystem->PageUp();
                clientConsoleSystem->PageUp();
            }

            return;
        }

        if(key == K_MWHEELDOWN) {
            clientConsoleSystem->PageDown();

            // hold <ctrl> to accelerate scrolling
            if(keys[K_CTRL].down) {
                clientConsoleSystem->PageDown();
                clientConsoleSystem->PageDown();
            }

            return;
        }

        // ctrl-home = top of console
        if(key == K_HOME && keys[K_CTRL].down) {
            clientConsoleSystem->Top();
            return;
        }

        // ctrl-end = bottom of console
        if(key == K_END && keys[K_CTRL].down) {
            clientConsoleSystem->Bottom();
            return;
        }
    }

    // pass to the normal editline routine
    cmdCompletionSystem->KeyDownEvent(&g_consoleField, key);
}

/*
================
idClientKeysSystemLocal::Key

In game talk message
================
*/
void idClientKeysSystemLocal::Key(sint key) {
    valueType buffer[MAX_STRING_CHARS];

    if(key == K_ESCAPE) {
        cls.keyCatchers &= ~KEYCATCH_MESSAGE;
        cmdCompletionSystem->Clear(&chatField);
        return;
    }

    if(key == K_ENTER || key == K_KP_ENTER) {
        if(chatField.buffer[0] && cls.state == CA_ACTIVE) {
            if(chat_team) {
                Q_vsprintf_s(buffer, sizeof(buffer), sizeof(buffer), "say_team \"%s\"\n",
                             chatField.buffer);
            } else if(chat_buddy) {
                Q_vsprintf_s(buffer, sizeof(buffer), sizeof(buffer), "say_buddy \"%s\"\n",
                             chatField.buffer);
            } else {
                Q_vsprintf_s(buffer, sizeof(buffer), sizeof(buffer), "say \"%s\"\n",
                             chatField.buffer);
            }

            if(buffer[0]) {
                clientReliableCommandsSystem->AddReliableCommand(buffer);
            }
        }

        cls.keyCatchers &= ~KEYCATCH_MESSAGE;
        cmdCompletionSystem->Clear(&chatField);
        return;
    }

    cmdCompletionSystem->KeyDownEvent(&chatField, key);
}

/*
===================
idClientKeysSystemLocal::GetOverstrikeMode
===================
*/
bool idClientKeysSystemLocal::GetOverstrikeMode(void) {
    return key_overstrikeMode;
}

/*
===================
idClientKeysSystemLocal::SetOverstrikeMode
===================
*/
void idClientKeysSystemLocal::SetOverstrikeMode(bool state) {
    key_overstrikeMode = state;
}


/*
===================
idClientKeysSystemLocal::IsDown
===================
*/
bool idClientKeysSystemLocal::IsDown(sint keynum) {
    if(keynum < 0 || keynum >= MAX_KEYS) {
        return false;
    }

    return keys[keynum].down;
}


/*
===================
idClientKeysSystemLocal::StringToKeynum

Returns a key number to be used to index keys[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.

0x11 will be interpreted as raw hex, which will allow new controlers

to be configured even if they don't have defined names.
===================
*/
sint idClientKeysSystemLocal::StringToKeynum(pointer str) {
    keyname_t   *kn;

    if(!str || !str[0]) {
        return -1;
    }

    if(!str[1]) {
        return tolower(str[0]);
    }

    // check for hex code
    if(strlen(str) == 4) {
        sint n = Com_HexStrToInt(str);

        if(n >= 0) {
            return n;
        }
    }

    // scan for a text match
    for(kn = keynames ; kn->name ; kn++) {
        if(!Q_stricmp(str, kn->name)) {
            return kn->keynum;
        }
    }

    return -1;
}

/*
===================
idClientKeysSystemLocal::KeynumToString

Returns a string (either a single ascii char, a K_* name, or a 0x11 hex string) for the
given keynum.
===================
*/
valueType *idClientKeysSystemLocal::KeynumToString(sint keynum) {
    keyname_t   *kn;
    static valueType tinystr[5];
    sint i, j;

    if(keynum == -1) {
        return "<KEY NOT FOUND>";
    }

    if(keynum < 0 || keynum >= MAX_KEYS) {
        return "<OUT OF RANGE>";
    }

    // check for printable ascii (don't use quote)
    if(keynum > 32 && keynum < 127 && keynum != '"' && keynum != ';') {
        tinystr[0] = static_cast<valueType>(keynum);
        tinystr[1] = 0;
        return tinystr;
    }

    // check for a key string
    for(kn = keynames ; kn->name ; kn++) {
        if(keynum == kn->keynum) {
            return kn->name;
        }
    }

    // make a hex string
    i = keynum >> 4;
    j = keynum & 15;

    tinystr[0] = '0';
    tinystr[1] = 'x';
    tinystr[2] = static_cast<valueType>(i > 9 ? i - 10 + 'a' : i + '0');
    tinystr[3] = static_cast<valueType>(j > 9 ? j - 10 + 'a' : j + '0');
    tinystr[4] = 0;

    return tinystr;
}

/*
===================
idClientKeysSystemLocal::SetBinding
===================
*/
sint32 idClientKeysSystemLocal::generateHashValue(pointer fname) {
    sint i;
    sint32 hash;

    if(!fname) {
        return 0;
    }

    hash = 0;
    i = 0;

    while(fname[i] != '\0') {
        hash += static_cast<sint32>(fname[i]) * (i + 119);
        i++;
    }

    hash &= (BIND_HASH_SIZE - 1);
    return hash;
}

/*
===================
idClientKeysSystemLocal::SetBinding
===================
*/
void idClientKeysSystemLocal::SetBinding(sint keynum, pointer binding) {

    valueType *lcbinding;    // fretn - make a copy of our binding lowercase
    // so name toggle scripts work again: bind x name BzZIfretn?
    // resulted into bzzifretn?

    if(keynum < 0 || keynum >= MAX_KEYS) {
        return;
    }

    // free old bindings
    if(keys[ keynum ].binding) {
        memorySystem->Free(keys[ keynum ].binding);
    }

    // allocate memory for new binding
    keys[keynum].binding = memorySystem->CopyString(binding);
    lcbinding = memorySystem->CopyString(binding);
    Q_strlwr(lcbinding);   // saves doing it on all the generateHashValues in Key_GetBindingByString

    keys[keynum].hash = generateHashValue(lcbinding);

    // consider this like modifying an archived cvar, so the
    // file write will be triggered at the next oportunity
    cvar_modifiedFlags |= CVAR_ARCHIVE;
}

/*
===================
idClientKeysSystemLocal::GetBinding
===================
*/
valueType *idClientKeysSystemLocal::GetBinding(sint keynum) {
    if(keynum < 0 || keynum >= MAX_KEYS) {
        return "";
    }

    return keys[ keynum ].binding;
}

/*
===================
idClientKeysSystemLocal::GetBinding

binding MUST be lower case
===================
*/
void idClientKeysSystemLocal::GetBindingByString(pointer binding,
        sint *key1, sint *key2) {
    sint i;
    sint hash = generateHashValue(binding);

    *key1 = -1;
    *key2 = -1;

    for(i = 0; i < MAX_KEYS; i++) {
        if(keys[i].hash == hash && !Q_stricmp(binding, keys[i].binding)) {
            if(*key1 == -1) {
                *key1 = i;
            } else if(*key2 == -1) {
                *key2 = i;
                return;
            }
        }
    }
}

/*
===================
idClientKeysSystemLocal::GetKey
===================
*/
sint idClientKeysSystemLocal::GetKey(pointer binding) {
    sint i;

    if(binding) {
        for(i = 0 ; i < MAX_KEYS ; i++) {
            if(keys[i].binding && Q_stricmp(binding, keys[i].binding) == 0) {
                return i;
            }
        }
    }

    return -1;
}

/*
===================
idClientKeysSystemLocal::Unbind_f
===================
*/
void idClientKeysSystemLocal::Unbind_f(void) {
    sint b;

    if(cmdSystem->Argc() != 2) {
        Com_Printf("unbind <key> : remove commands from a key\n");
        return;
    }

    b = clientKeysLocal.StringToKeynum(cmdSystem->Argv(1));

    if(b == -1) {
        Com_Printf("\"%s\" isn't a valid key\n", cmdSystem->Argv(1));
        return;
    }

    clientKeysLocal.SetBinding(b, "");
}

/*
===================
idClientKeysSystemLocal::Unbindall_f
===================
*/
void  idClientKeysSystemLocal::Unbindall_f(void) {
    sint i;

    for(i = 0 ; i < MAX_KEYS; i++)
        if(keys[i].binding) {
            clientKeysLocal.SetBinding(i, "");
        }
}

/*
===================
idClientKeysSystemLocal::Bind_f
===================
*/
void idClientKeysSystemLocal::Bind_f(void) {
    sint c, b;

    c = cmdSystem->Argc();

    if(c < 2) {
        Com_Printf("bind <key> [command] : attach a command to a key\n");
        return;
    }

    b = clientKeysLocal.StringToKeynum(cmdSystem->Argv(1));

    if(b == -1) {
        Com_Printf("\"%s\" isn't a valid key\n", cmdSystem->Argv(1));
        return;
    }

    if(c == 2) {
        if(keys[b].binding) {
            Com_Printf("\"%s\" = \"%s\"\n", clientKeysLocal.KeynumToString(b),
                       keys[b].binding);
        } else {
            Com_Printf("\"%s\" is not bound\n", clientKeysLocal.KeynumToString(b));
        }

        return;
    }

    // set to 3rd arg onwards, unquoted from raw
    clientKeysLocal.SetBinding(b, Com_UnquoteStr(cmdSystem->FromNth(2)));
}

/*
===================
idClientKeysSystemLocal::EditBind_f
===================
*/
void idClientKeysSystemLocal::EditBind_f(void) {
    valueType *buf;
    /*const*/
    valueType *key, *binding, *keyq;
    sint b;

    b = cmdSystem->Argc();

    if(b != 2) {
        Com_Printf("editbind <key> : edit a key binding (in the in-game console)");
        return;
    }

    key = cmdSystem->Argv(1);
    b = clientKeysLocal.StringToKeynum(key);

    if(b == -1) {
        Com_Printf("\"%s\" isn't a valid key\n", key);
        return;
    }

    binding = clientKeysLocal.GetBinding(b);

    keyq = (const_cast<valueType *>(reinterpret_cast<pointer>
                                    (Com_QuoteStr(key))));       // <- static buffer
    buf = reinterpret_cast< valueType *>(malloc(8 + strlen(keyq) + strlen(
            binding)));
    Q_vsprintf_s(buf, sizeof(buf), sizeof(buf), "/bind %s %s", keyq, binding);

    clientConsoleSystem->OpenConsole_f();
    cmdCompletionSystem->Set(&g_consoleField, buf);
    free(buf);
}

/*
============
idClientKeysSystemLocal::WriteBindings

Writes lines containing "bind key value"
============
*/
void idClientKeysSystemLocal::WriteBindings(fileHandle_t f) {
    sint i;

    fileSystem->Printf(f, "unbindall\n");

    for(i = 0 ; i < MAX_KEYS ; i++) {
        if(keys[i].binding && keys[i].binding[0]) {
            // quote the string if it contains ; but no "
            fileSystem->Printf(f, "bind %s %s\n", KeynumToString(i),
                               Com_QuoteStr(keys[i].binding));
        }

    }
}


/*
============
idClientKeysSystemLocal::Bindlist_f
============
*/
void idClientKeysSystemLocal::Bindlist_f(void) {
    sint i;

    for(i = 0 ; i < MAX_KEYS ; i++) {
        if(keys[i].binding && keys[i].binding[0]) {
            Com_Printf("%s = %s\n", clientKeysLocal.KeynumToString(i),
                       keys[i].binding);
        }
    }
}

/*
============
idClientKeysSystemLocal::KeynameCompletion
============
*/
void idClientKeysSystemLocal::KeynameCompletion(void(*callback)(
            pointer s)) {
    sint i;

    for(i = 0; keynames[ i ].name != nullptr; i++) {
        callback(keynames[ i ].name);
    }
}

/*
====================
Key_CompleteUnbind
====================
*/
static void Key_CompleteUnbind(valueType *args, sint argNum) {
    if(argNum == 2) {
        // Skip "unbind "
        valueType *p = Com_SkipTokens(args, 1, " ");

        if(p > args) {
            cmdCompletionSystem->CompleteKeyname();
        }
    }
}

/*
====================
idClientKeysSystemLocal::CompleteBind
====================
*/
void idClientKeysSystemLocal::CompleteBind(valueType *args, sint argNum) {
    valueType *p;

    if(argNum == 2) {
        // Skip "bind "
        p = Com_SkipTokens(args, 1, " ");

        if(p > args) {
            cmdCompletionSystem->CompleteKeyname();
        }
    } else if(argNum >= 3) {
        // Skip "bind <key> "
        p = Com_SkipTokens(args, 2, " ");

        if(p > args) {
            cmdCompletionSystem->CompleteCommand(p, true, true);
        }
    }
}

/*
====================
idClientKeysSystemLocal::CompleteEditbind
====================
*/
void idClientKeysSystemLocal::CompleteEditbind(valueType *args,
        sint argNum) {
    valueType *p;

    p = Com_SkipTokens(args, 1, " ");

    if(p > args) {
        cmdCompletionSystem->CompleteKeyname();
    }
}

/*
===================
idClientKeysSystemLocal::InitKeyCommands
===================
*/
void idClientKeysSystemLocal::InitKeyCommands(void) {

    Com_Printf("----- idClientKeysSystemLocal::InitKeyCommands -------\n");
    // register our functions
    cmdSystem->AddCommand("bind", Bind_f,
                          "Used for assigning keys to actions. Bind x �weaponbank 3");
    cmdSystem->SetCommandCompletionFunc("bind", CompleteBind);
    cmdSystem->AddCommand("unbind", Unbind_f,
                          "Displays list of cvars in console");
    cmdSystem->SetCommandCompletionFunc("unbind", Key_CompleteUnbind);
    cmdSystem->AddCommand("unbindall", Unbindall_f,
                          "For unassigning all commands etc from ALL keys. /unbindall");
    cmdSystem->AddCommand("bindlist", Bindlist_f,
                          "Displays list of cvars in console");
    cmdSystem->AddCommand("editbind", EditBind_f,
                          "Used for editing already binded key");
    cmdSystem->SetCommandCompletionFunc("editbind", CompleteEditbind);
}

/*
===================
idClientKeysSystemLocal::AddKeyUpCommands
===================
*/
void idClientKeysSystemLocal::AddKeyUpCommands(sint key, valueType *kb,
        sint time) {
    sint i;
    valueType button[1024], *buttonPtr;
    valueType   cmd[1024];
    bool keyevent;

    if(!kb) {
        return;
    }

    keyevent = false;
    buttonPtr = button;

    for(i = 0; ; i++) {
        if(kb[i] == ';' || !kb[i]) {
            *buttonPtr = '\0';

            if(button[0] == '+') {
                // button commands add keynum and time as parms so that multiple
                // sources can be discriminated and subframe corrected
                Q_vsprintf_s(cmd, sizeof(cmd), sizeof(cmd), "-%s %i %i\n", button + 1, key,
                             time);
                cmdBufferSystem->AddText(cmd);
                keyevent = true;
            } else {
                if(keyevent) {
                    // down-only command
                    cmdBufferSystem->AddText(button);
                    cmdBufferSystem->AddText("\n");
                }
            }

            buttonPtr = button;

            while((kb[i] <= ' ' || kb[i] == ';') && kb[i] != 0) {
                i++;
            }
        }

        *buttonPtr++ = kb[i];

        if(!kb[i]) {
            break;
        }
    }
}

/*
===================
idClientKeysSystemLocal::KeyEvent

Called by the system for both key up and key down events
===================
*/
void idClientKeysSystemLocal::KeyEvent(sint key, sint down, sint time) {
    valueType    *kb;
    valueType cmd[1024];
    bool bypassMenu = false;       // NERVE - SMF
    bool onlybinds = false;

    if(!key) {
        return;
    }

    switch(key) {
        case K_KP_PGUP:
        case K_KP_EQUALS:
        case K_KP_5:
        case K_KP_LEFTARROW:
        case K_KP_UPARROW:
        case K_KP_RIGHTARROW:
        case K_KP_DOWNARROW:
        case K_KP_END:
        case K_KP_PGDN:
        case K_KP_INS:
        case K_KP_DEL:
        case K_KP_HOME:
            if(idsystem->IsNumLockDown()) {
                onlybinds = true;
            }

            break;
    }


    // update auto-repeat status and BUTTON_ANY status
    keys[key].down = (bool)down;

    if(down) {
        keys[key].repeats++;

        if(keys[key].repeats == 1) {
            anykeydown++;
        }
    } else {
        keys[key].repeats = 0;
        anykeydown--;

        if(anykeydown < 0) {
            anykeydown = 0;
        }
    }

    if(key == K_ENTER) {
        if(down) {
            if(keys[K_ALT].down) {
                ClearStates();

                // don't repeat fullscreen toggle when keys are held down
                if(keys[K_ENTER].repeats > 1) {
                    return;
                }

                if(r_fullscreen->integer == 0) {
                    Com_Printf("Switching to fullscreen rendering\n");
                    cvarSystem->Set("r_fullscreen", "1");
                } else {
                    Com_Printf("Switching to windowed rendering\n");
                    cvarSystem->Set("r_fullscreen", "0");
                }

                cmdBufferSystem->ExecuteText(EXEC_APPEND, "vid_restart\n");
                return;
            }
        }
    }

#if defined (MACOS_X)

    if(down && keys[ K_COMMAND ].down) {

        if(key == 'f') {
            ClearStates();
            cmdBufferSystem->ExecuteText(EXEC_APPEND,
                                         "toggle r_fullscreen\nvid_restart\n");
            return;
        } else if(key == 'q') {
            ClearStates();
            cmdBufferSystem->ExecuteText(EXEC_APPEND, "quit\n");
            return;
        } else if(key == K_TAB) {
            ClearStates();
            cvarSystem->SetValue("r_minimize", 1);
            return;
        }
    }

#endif

    if(cl_altTab->integer && keys[K_ALT].down && key == K_TAB) {
        ClearStates();
        cvarSystem->SetValue("r_minimize", 1);
        return;
    }

    // are we waiting to clear stats and move to briefing screen
    //DAJ BUG in dedicated cl_waitForFire don't exist
    if(down && cl_waitForFire && cl_waitForFire->integer) {
        // get rid of the console
        if(cls.keyCatchers & KEYCATCH_CONSOLE) {
            clientConsoleSystem->ToggleConsole_f();
        }

        // clear all input controls
        clientInputSystem->ClearKeys();

        // allow only attack command input
        kb = keys[key].binding;

        if(!Q_stricmp(kb, "+attack")) {
            // clear the stats out, ignore the keypress
            // just remove the stats, but still wait until we're done loading, and player has hit fire to begin playing
            cvarSystem->Set("g_missionStats", "xx");
            cvarSystem->Set("cl_waitForFire", "0");
        }

        // no buttons while waiting
        return;
    }

    // are we waiting to begin the level
    //DAJ BUG in dedicated cl_missionStats don't exist
    if(down && cl_missionStats && cl_missionStats->string[0] &&
            cl_missionStats->string[1]) {
        // get rid of the consol
        if(cls.keyCatchers & KEYCATCH_CONSOLE) {
            clientConsoleSystem->ToggleConsole_f();
        }

        // clear all input controls
        clientInputSystem->ClearKeys();

        // allow only attack command input
        kb = keys[key].binding;

        if(!Q_stricmp(kb, "+attack")) {
            // clear the stats out, ignore the keypress
            cvarSystem->Set("com_expectedhunkusage", "-1");
            cvarSystem->Set("g_missionStats", "0");
        }

        // no buttons while waiting
        return;
    }

    // console key is hardcoded, so the user can never unbind it
    if(key == K_CONSOLE || (keys[K_SHIFT].down && key == K_ESCAPE)) {
        if(!down) {
            return;
        }

        clientConsoleSystem->ToggleConsole_f();
        ClearStates();
        return;
    }

    //----(SA)  added
    if(cl.cameraMode) {
        if(!(cls.keyCatchers & (KEYCATCH_UI |
                                KEYCATCH_CONSOLE))) {           // let menu/console handle keys if necessary
            // in cutscenes we need to handle keys specially (pausing not allowed in camera mode)
            if((key == K_ESCAPE ||
                    key == K_SPACE ||
                    key == K_ENTER) && down) {
                clientReliableCommandsSystem->AddReliableCommand("cameraInterrupt");
                return;
            }

            // eat all keys
            if(down) {
                return;
            }
        }

        if((cls.keyCatchers & KEYCATCH_CONSOLE) && key == K_ESCAPE) {
            // don't allow menu starting when console is down and camera running
            return;
        }
    }

    //----(SA)  end

    // most keys during demo playback will bring up the menu, but non-ascii

    // keys can still be used for bound actions
    /*
    if ( down && ( key < 128 || key == K_MOUSE1 )
         && ( clc.demoplaying || cls.state == CA_CINEMATIC ) && !cls.keyCatchers ) {

        cvarSystem->Set( "nextdemo","" );
        key = K_ESCAPE;
    }
    */

    // escape is always handled special
    if(key == K_ESCAPE && down) {
        //If console is active then ESC should close console
        if(cls.keyCatchers & KEYCATCH_CONSOLE) {
            clientConsoleSystem->ToggleConsole_f();
            ClearStates();
            return;
        }

        if(cls.keyCatchers & KEYCATCH_MESSAGE) {
            // clear message mode
            Key(key);
            return;
        }

        // escape always gets out of CGAME stuff
        if(cls.keyCatchers & KEYCATCH_CGAME) {
            cls.keyCatchers &= ~KEYCATCH_CGAME;
            cgame->EventHandling(CGAME_EVENT_NONE, true);
            return;
        }

        if(!(cls.keyCatchers & KEYCATCH_UI)) {
            if(cls.state == CA_ACTIVE && !clc.demoplaying) {
                // Arnout: on request
                if(cls.keyCatchers & KEYCATCH_CONSOLE) {     // get rid of the console
                    clientConsoleSystem->ToggleConsole_f();
                } else {
                    uiManager->SetActiveMenu(UIMENU_INGAME);
                }
            } else {
                idClientConsoleCommandsSystemLocal::Disconnect_f();
                soundSystem->StopAllSounds();
                uiManager->SetActiveMenu(UIMENU_MAIN);
            }

            return;
        }

        uiManager->KeyEvent(key, down);
        return;
    }

    //
    // key up events only perform actions if the game key binding is
    // a button command (leading + sign).  These will be processed even in
    // console mode and menu mode, to keep the character from continuing
    // an action started before a mode switch.
    //
    if(!down) {
        kb = keys[key].binding;

        if(kb && kb[0] == '+') {
            // button commands add keynum and time as parms so that multiple
            // sources can be discriminated and subframe corrected
            Q_vsprintf_s(cmd, sizeof(cmd), sizeof(cmd), "-%s %i %i\n", kb + 1, key,
                         time);
            cmdBufferSystem->AddText(cmd);
        }

        if(cls.keyCatchers & KEYCATCH_UI) {
            if(!onlybinds || uiManager->WantsBindKeys()) {
                uiManager->KeyEvent(key, down);
            }
        } else if(cls.keyCatchers & KEYCATCH_CGAME) {
            if(!onlybinds || cgame->WantsBindKeys()) {
                cgame->KeyEvent(key, down);
            }
        }

        return;
    }

    // NERVE - SMF - if we just want to pass it along to game
    if(cl_bypassMouseInput && cl_bypassMouseInput->integer) {
        if((key == K_MOUSE1 || key == K_MOUSE2 || key == K_MOUSE3 ||
                key == K_MOUSE4 || key == K_MOUSE5)) {
            if(cl_bypassMouseInput->integer == 1) {
                bypassMenu = true;
            }
        } else if((cls.keyCatchers & KEYCATCH_UI &&
                   !clientGUISystem->checkKeyExec(key)) ||
                  (cls.keyCatchers & KEYCATCH_CGAME &&
                   !clientGameSystem->CGameCheckKeyExec(key))) {
            bypassMenu = true;
        }
    }

    //
    // key up events only perform actions if the game key binding is
    // a button command (leading + sign).  These will be processed even in
    // console mode and menu mode, to keep the character from continuing
    // an action started before a mode switch.
    //
    if(!down) {
        kb = keys[key].binding;

        AddKeyUpCommands(key, kb, time);

        if(cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_BUG)) {
            uiManager->KeyEvent(key, down);
        } else if(cls.keyCatchers & KEYCATCH_CGAME && cgvm) {
            cgame->KeyEvent(key, down);
        }

        return;
    }

    // distribute the key down event to the apropriate handler
    if(cls.keyCatchers & KEYCATCH_CONSOLE) {
        if(!onlybinds) {
            DownEventKey(key);
        }
    } else if(cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_BUG)) {
        uiManager->KeyEvent(key, down);
    } else if(cls.keyCatchers & KEYCATCH_UI && !bypassMenu) {
        if(!onlybinds || uiManager->WantsBindKeys()) {
            uiManager->KeyEvent(key, down);
        }
    } else if(cls.keyCatchers & KEYCATCH_CGAME && !bypassMenu) {
        if(cgvm) {
            if(!onlybinds || cgame->WantsBindKeys()) {
                cgame->KeyEvent(key, down);
            }
        }
    } else if(cls.keyCatchers & KEYCATCH_MESSAGE) {
        if(!onlybinds) {
            Key(key);
        }
    } else if(cls.state == CA_DISCONNECTED) {
        if(!onlybinds) {
            DownEventKey(key);
        }
    } else {
        // send the bound action
        kb = keys[key].binding;

        if(!kb) {
            if(key >= 200) {
                Com_Printf("%s is unbound, use controls menu to set.\n"
                           , KeynumToString(key));
            }
        } else if(kb[0] == '+') {
            // button commands add keynum and time as parms so that multiple
            // sources can be discriminated and subframe corrected
            Q_vsprintf_s(cmd, sizeof(cmd), sizeof(cmd), "%s %i %i\n", kb, key, time);
            cmdBufferSystem->AddText(cmd);
        } else {
            // down-only command
            cmdBufferSystem->AddText(kb);
            cmdBufferSystem->AddText("\n");
        }
    }
}

/*
==================
idClientKeysSystemLocal::KeyCharEvents
==================
*/
void idClientKeysSystemLocal::KeyCharEvents(sint ch) {
    // ctrl-L clears screen
    if(ch == KEYBOARDCTRL('l')) {
        cmdBufferSystem->AddText("clear\n");
        return;
    }

    // alt-n, alt-p switches console
    if(keys[K_ALT].down) {
        switch(ch) {
            case 'p':
            case 'P':
                clientConsoleSystem->ConsoleNext(-1);
                return;

            case 'n':
            case 'N':
                clientConsoleSystem->ConsoleNext(1);
                return;
        }
    }

    // tab completes command or switches console
    if(ch == '\t') {
        if(keys[K_SHIFT].down) {
            clientConsoleSystem->ConsoleNext(-1);
        } else if(keys[K_CTRL].down) {
            clientConsoleSystem->ConsoleNext(1);
        } else {
            sint conNum = activeCon - con;

            // autocomplete only for non-chat consoles
            if(conNum != CON_CHAT && conNum != CON_TCHAT) {
                CompleteCommand();
            }
        }

        return;
    }

    // switch console
    if(ch >= '0' && ch <= '9' && keys[K_ALT].down) {
        // console numbers start at one, last one is 10 (accessed through 0)
        sint n = ch == '0' ? 10 : ch - '1';

        clientConsoleSystem->ConsoleSwitch(n);
        return;
    }

    // pass to the normal editline routine
    cmdCompletionSystem->CharEvent(&g_consoleField, ch);
}

/*
===================
idClientKeysSystemLocal::CharEvent

Normal keyboard characters, already shifted / capslocked / etc
===================
*/
void idClientKeysSystemLocal::CharEvent(sint key) {
    // the console key should never be used as a char
    // ydnar: added uk equivalent of shift+`
    // the RIGHT way to do this would be to have certain keys disable the equivalent SYSE_CHAR event

    // fretn - this should be fixed in Com_EventLoop
    // but I can't be arsed to leave this as is

    if(key == static_cast<uchar8>('`') || key == static_cast<uchar8>('~') ||
            key == static_cast<uchar8>('�')) {
        return;
    }

    // distribute the key down event to the apropriate handler
    if(cls.keyCatchers & KEYCATCH_CONSOLE) {
        KeyCharEvents(key);
    } else if(cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_BUG)) {
        uiManager->KeyEvent(key | K_CHAR_FLAG, true);
    } else if(cls.keyCatchers & KEYCATCH_UI) {
        uiManager->KeyEvent(key | K_CHAR_FLAG, true);
    } else if(cls.keyCatchers & KEYCATCH_CGAME) {
        cgame->KeyEvent(key | K_CHAR_FLAG, true);
    } else if(cls.keyCatchers & KEYCATCH_MESSAGE) {
        cmdCompletionSystem->CharEvent(&chatField, key);
    } else if(cls.state == CA_DISCONNECTED) {
        cmdCompletionSystem->CharEvent(&g_consoleField, key);
    }
}

/*
===================
idClientKeysSystemLocal::ClearStates
===================
*/
void idClientKeysSystemLocal::ClearStates(void) {
    sint i;

    anykeydown = 0;

    for(i = 0 ; i < MAX_KEYS ; i++) {
        if(keys[i].down) {
            clientKeysLocal.KeyEvent(i, false, 0);

        }

        keys[i].down = (bool)0;
        keys[i].repeats = 0;
    }
}

/*
====================
idClientKeysSystemLocal::GetCatcher
====================
*/
sint idClientKeysSystemLocal::GetCatcher(void) {
    return cls.keyCatchers;
}

/*
====================
idClientKeysSystemLocal::SetCatcher
====================
*/
void idClientKeysSystemLocal::SetCatcher(sint catcher) {
    // If the catcher state is changing, clear all key states
    if(catcher != cls.keyCatchers) {
        ClearStates();
    }

    cls.keyCatchers = catcher;
}