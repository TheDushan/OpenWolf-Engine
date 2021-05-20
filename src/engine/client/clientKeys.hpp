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
// File name:   clientKeys.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

#ifndef __CLIENTKEYS_LOCAL_HPP__
#define __CLIENTKEYS_LOCAL_HPP__

/*
key up events are sent even if in console mode
*/

extern bool key_overstrikeMode;
extern qkey_t   keys[MAX_KEYS];

extern field_t  g_consoleField;
extern field_t  chatField;
extern sint      anykeydown;
extern bool chat_team;
extern bool chat_buddy;
extern bool commandMode;

typedef struct {
    valueType    *name;
    sint keynum;
} keyname_t;

// names not in this list can either be lowercase ascii, or '0xnn' hex sequences
static keyname_t keynames[] = {
    {"TAB", K_TAB},
    {"ENTER", K_ENTER},
    {"ESCAPE", K_ESCAPE},
    {"SPACE", K_SPACE},
    {"BACKSPACE", K_BACKSPACE},
    {"UPARROW", K_UPARROW},
    {"DOWNARROW", K_DOWNARROW},
    {"LEFTARROW", K_LEFTARROW},
    {"RIGHTARROW", K_RIGHTARROW},

    {"ALT", K_ALT},
    {"CTRL", K_CTRL},
    {"SHIFT", K_SHIFT},

    {"COMMAND", K_COMMAND},

    {"CAPSLOCK", K_CAPSLOCK},


    {"F1", K_F1},
    {"F2", K_F2},
    {"F3", K_F3},
    {"F4", K_F4},
    {"F5", K_F5},
    {"F6", K_F6},
    {"F7", K_F7},
    {"F8", K_F8},
    {"F9", K_F9},
    {"F10", K_F10},
    {"F11", K_F11},
    {"F12", K_F12},
    {"F13", K_F13},
    {"F14", K_F14},
    {"F15", K_F15},

    {"INS", K_INS},
    {"DEL", K_DEL},
    {"PGDN", K_PGDN},
    {"PGUP", K_PGUP},
    {"HOME", K_HOME},
    {"END", K_END},

    {"MOUSE1", K_MOUSE1},
    {"MOUSE2", K_MOUSE2},
    {"MOUSE3", K_MOUSE3},
    {"MOUSE4", K_MOUSE4},
    {"MOUSE5", K_MOUSE5},

    {"MWHEELUP", K_MWHEELUP },
    {"MWHEELDOWN",   K_MWHEELDOWN },

    {"JOY1", K_JOY1},
    {"JOY2", K_JOY2},
    {"JOY3", K_JOY3},
    {"JOY4", K_JOY4},
    {"JOY5", K_JOY5},
    {"JOY6", K_JOY6},
    {"JOY7", K_JOY7},
    {"JOY8", K_JOY8},
    {"JOY9", K_JOY9},
    {"JOY10", K_JOY10},
    {"JOY11", K_JOY11},
    {"JOY12", K_JOY12},
    {"JOY13", K_JOY13},
    {"JOY14", K_JOY14},
    {"JOY15", K_JOY15},
    {"JOY16", K_JOY16},
    {"JOY17", K_JOY17},
    {"JOY18", K_JOY18},
    {"JOY19", K_JOY19},
    {"JOY20", K_JOY20},
    {"JOY21", K_JOY21},
    {"JOY22", K_JOY22},
    {"JOY23", K_JOY23},
    {"JOY24", K_JOY24},
    {"JOY25", K_JOY25},
    {"JOY26", K_JOY26},
    {"JOY27", K_JOY27},
    {"JOY28", K_JOY28},
    {"JOY29", K_JOY29},
    {"JOY30", K_JOY30},
    {"JOY31", K_JOY31},
    {"JOY32", K_JOY32},

    {"AUX1", K_AUX1},
    {"AUX2", K_AUX2},
    {"AUX3", K_AUX3},
    {"AUX4", K_AUX4},
    {"AUX5", K_AUX5},
    {"AUX6", K_AUX6},
    {"AUX7", K_AUX7},
    {"AUX8", K_AUX8},
    {"AUX9", K_AUX9},
    {"AUX10", K_AUX10},
    {"AUX11", K_AUX11},
    {"AUX12", K_AUX12},
    {"AUX13", K_AUX13},
    {"AUX14", K_AUX14},
    {"AUX15", K_AUX15},
    {"AUX16", K_AUX16},

    {"KP_HOME",          K_KP_HOME },
    {"KP_UPARROW",       K_KP_UPARROW },
    {"KP_PGUP",          K_KP_PGUP },
    {"KP_LEFTARROW", K_KP_LEFTARROW },
    {"KP_5",         K_KP_5 },
    {"KP_RIGHTARROW",    K_KP_RIGHTARROW },
    {"KP_END",           K_KP_END },
    {"KP_DOWNARROW", K_KP_DOWNARROW },
    {"KP_PGDN",          K_KP_PGDN },
    {"KP_ENTER",     K_KP_ENTER },
    {"KP_INS",           K_KP_INS },
    {"KP_DEL",           K_KP_DEL },
    {"KP_SLASH",     K_KP_SLASH },
    {"KP_MINUS",     K_KP_MINUS },
    {"KP_PLUS",          K_KP_PLUS },
    {"KP_NUMLOCK",       K_KP_NUMLOCK },
    {"KP_STAR",          K_KP_STAR },
    {"KP_EQUALS",        K_KP_EQUALS },

    {"PAUSE", K_PAUSE},

    {"SEMICOLON", ';'},   // because a raw semicolon seperates commands

    {"WORLD_0", K_WORLD_0},
    {"WORLD_1", K_WORLD_1},
    {"WORLD_2", K_WORLD_2},
    {"WORLD_3", K_WORLD_3},
    {"WORLD_4", K_WORLD_4},
    {"WORLD_5", K_WORLD_5},
    {"WORLD_6", K_WORLD_6},
    {"WORLD_7", K_WORLD_7},
    {"WORLD_8", K_WORLD_8},
    {"WORLD_9", K_WORLD_9},
    {"WORLD_10", K_WORLD_10},
    {"WORLD_11", K_WORLD_11},
    {"WORLD_12", K_WORLD_12},
    {"WORLD_13", K_WORLD_13},
    {"WORLD_14", K_WORLD_14},
    {"WORLD_15", K_WORLD_15},
    {"WORLD_16", K_WORLD_16},
    {"WORLD_17", K_WORLD_17},
    {"WORLD_18", K_WORLD_18},
    {"WORLD_19", K_WORLD_19},
    {"WORLD_20", K_WORLD_20},
    {"WORLD_21", K_WORLD_21},
    {"WORLD_22", K_WORLD_22},
    {"WORLD_23", K_WORLD_23},
    {"WORLD_24", K_WORLD_24},
    {"WORLD_25", K_WORLD_25},
    {"WORLD_26", K_WORLD_26},
    {"WORLD_27", K_WORLD_27},
    {"WORLD_28", K_WORLD_28},
    {"WORLD_29", K_WORLD_29},
    {"WORLD_30", K_WORLD_30},
    {"WORLD_31", K_WORLD_31},
    {"WORLD_32", K_WORLD_32},
    {"WORLD_33", K_WORLD_33},
    {"WORLD_34", K_WORLD_34},
    {"WORLD_35", K_WORLD_35},
    {"WORLD_36", K_WORLD_36},
    {"WORLD_37", K_WORLD_37},
    {"WORLD_38", K_WORLD_38},
    {"WORLD_39", K_WORLD_39},
    {"WORLD_40", K_WORLD_40},
    {"WORLD_41", K_WORLD_41},
    {"WORLD_42", K_WORLD_42},
    {"WORLD_43", K_WORLD_43},
    {"WORLD_44", K_WORLD_44},
    {"WORLD_45", K_WORLD_45},
    {"WORLD_46", K_WORLD_46},
    {"WORLD_47", K_WORLD_47},
    {"WORLD_48", K_WORLD_48},
    {"WORLD_49", K_WORLD_49},
    {"WORLD_50", K_WORLD_50},
    {"WORLD_51", K_WORLD_51},
    {"WORLD_52", K_WORLD_52},
    {"WORLD_53", K_WORLD_53},
    {"WORLD_54", K_WORLD_54},
    {"WORLD_55", K_WORLD_55},
    {"WORLD_56", K_WORLD_56},
    {"WORLD_57", K_WORLD_57},
    {"WORLD_58", K_WORLD_58},
    {"WORLD_59", K_WORLD_59},
    {"WORLD_60", K_WORLD_60},
    {"WORLD_61", K_WORLD_61},
    {"WORLD_62", K_WORLD_62},
    {"WORLD_63", K_WORLD_63},
    {"WORLD_64", K_WORLD_64},
    {"WORLD_65", K_WORLD_65},
    {"WORLD_66", K_WORLD_66},
    {"WORLD_67", K_WORLD_67},
    {"WORLD_68", K_WORLD_68},
    {"WORLD_69", K_WORLD_69},
    {"WORLD_70", K_WORLD_70},
    {"WORLD_71", K_WORLD_71},
    {"WORLD_72", K_WORLD_72},
    {"WORLD_73", K_WORLD_73},
    {"WORLD_74", K_WORLD_74},
    {"WORLD_75", K_WORLD_75},
    {"WORLD_76", K_WORLD_76},
    {"WORLD_77", K_WORLD_77},
    {"WORLD_78", K_WORLD_78},
    {"WORLD_79", K_WORLD_79},
    {"WORLD_80", K_WORLD_80},
    {"WORLD_81", K_WORLD_81},
    {"WORLD_82", K_WORLD_82},
    {"WORLD_83", K_WORLD_83},
    {"WORLD_84", K_WORLD_84},
    {"WORLD_85", K_WORLD_85},
    {"WORLD_86", K_WORLD_86},
    {"WORLD_87", K_WORLD_87},
    {"WORLD_88", K_WORLD_88},
    {"WORLD_89", K_WORLD_89},
    {"WORLD_90", K_WORLD_90},
    {"WORLD_91", K_WORLD_91},
    {"WORLD_92", K_WORLD_92},
    {"WORLD_93", K_WORLD_93},
    {"WORLD_94", K_WORLD_94},
    {"WORLD_95", K_WORLD_95},

    {"WINDOWS", K_SUPER},
    {"COMPOSE", K_COMPOSE},
    {"MODE", K_MODE},
    {"HELP", K_HELP},
    {"PRINT", K_PRINT},
    {"SYSREQ", K_SYSREQ},
    {"SCROLLOCK", K_SCROLLOCK },
    {"BREAK", K_BREAK},
    {"MENU", K_MENU},
    {"POWER", K_POWER},
    {"EURO", K_EURO},
    {"UNDO", K_UNDO},

    {"XBOX360_A", K_XBOX360_A},
    {"XBOX360_B", K_XBOX360_B},
    {"XBOX360_X", K_XBOX360_X},
    {"XBOX360_Y", K_XBOX360_Y},
    {"XBOX360_LB", K_XBOX360_LB},
    {"XBOX360_RB", K_XBOX360_RB},
    {"XBOX360_START", K_XBOX360_START},
    {"XBOX360_GUIDE", K_XBOX360_GUIDE},
    {"XBOX360_LS", K_XBOX360_LS},
    {"XBOX360_RS", K_XBOX360_RS},
    {"XBOX360_BACK", K_XBOX360_BACK},
    {"XBOX360_LT", K_XBOX360_LT},
    {"XBOX360_RT", K_XBOX360_RT},
    {"XBOX360_DPAD_UP", K_XBOX360_DPAD_UP},
    {"XBOX360_DPAD_RIGHT", K_XBOX360_DPAD_RIGHT},
    {"XBOX360_DPAD_DOWN", K_XBOX360_DPAD_DOWN},
    {"XBOX360_DPAD_LEFT", K_XBOX360_DPAD_LEFT},
    {"XBOX360_DPAD_RIGHTUP", K_XBOX360_DPAD_RIGHTUP},
    {"XBOX360_DPAD_RIGHTDOWN", K_XBOX360_DPAD_RIGHTDOWN},
    {"XBOX360_DPAD_LEFTUP", K_XBOX360_DPAD_LEFTUP},
    {"XBOX360_DPAD_LEFTDOWN", K_XBOX360_DPAD_LEFTDOWN},

    {nullptr, 0}
};

#define BIND_HASH_SIZE 1024

//
// idClientKeysSystemLocal
//
class idClientKeysSystemLocal : public idClientKeysSystem {
public:
    idClientKeysSystemLocal();
    ~idClientKeysSystemLocal();

    virtual bool GetOverstrikeMode(void);
    virtual void SetOverstrikeMode(bool state);
    virtual bool IsDown(sint keynum);
    virtual sint StringToKeynum(pointer str);
    virtual void SetBinding(sint keynum, pointer binding);
    virtual valueType *GetBinding(sint keynum);
    virtual sint GetKey(pointer binding);
    virtual void GetBindingByString(pointer binding, sint *key1, sint *key2);
    virtual void InitKeyCommands(void);
    virtual void KeyEvent(sint key, sint down, sint time);
    virtual void CharEvent(sint key);
    virtual void WriteBindings(fileHandle_t f);
    virtual valueType *KeynumToString(sint keynum);
    virtual void KeynameCompletion(void(*callback)(pointer s));
    virtual void ClearStates(void);
    virtual sint GetCatcher(void);
    virtual void SetCatcher(sint catcher);

public:
    static void CompleteCommand(void);
    static void DownEventKey(sint key);
    static void Key(sint key);
    static sint32 generateHashValue(pointer fname);
    static void Unbind_f(void);
    static void Unbindall_f(void);
    static void Bind_f(void);
    static void EditBind_f(void);
    static void Bindlist_f(void);
    static void CompleteUnbind(valueType *args, sint argNum);
    static void CompleteBind(valueType *args, sint argNum);
    static void CompleteEditbind(valueType *args, sint argNum);
    static void AddKeyUpCommands(sint key, valueType *kb, sint time);
    static void KeyCharEvents(sint ch);
};

extern idClientKeysSystemLocal clientKeysLocal;

#endif // !__CLIENTKEYS_LOCAL_HPP__
