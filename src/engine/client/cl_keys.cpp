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
// File name:   cl_keys.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

/*
key up events are sent even if in console mode
*/

field_t g_consoleField;
field_t chatField;
bool chat_team;
bool chat_buddy;
bool commandMode;
bool key_overstrikeMode;

sint anykeydown;
qkey_t keys[MAX_KEYS];

typedef struct
{
    valueType*    name;
    sint keynum;
} keyname_t;

// names not in this list can either be lowercase ascii, or '0xnn' hex sequences
keyname_t keynames[] =
{
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

/*
===============
CompleteCommand

Tab expansion
===============
*/
static void CompleteCommand( void )
{
    field_t* edit;
    edit = &g_consoleField;
    
    // only look at the first token for completion purposes
    cmdSystem->TokenizeString( edit->buffer );
    
    cmdCompletionSystem->AutoComplete( edit, "]" );
}

/*
====================
Console_DownEventKey

Handles history and console scrollback
====================
*/
void Console_DownEventKey( sint key )
{
    sint conNum = activeCon - con;
    bool isChat = CON_ISCHAT( conNum );
    
    if( key == K_ESCAPE )
    {
        cls.keyCatchers ^= KEYCATCH_CONSOLE;
        commandMode = false;
        return;
    }
    
    // enter finishes the line
    if( key == K_ENTER || key == K_KP_ENTER )
    {
        Con_LineAccept();
        return;
    }
    
    // clear autocompletion buffer on normal key input
    if( ( key >= K_SPACE && key <= K_BACKSPACE ) || ( key == K_LEFTARROW ) || ( key == K_RIGHTARROW ) ||
            ( key >= K_KP_LEFTARROW && key <= K_KP_RIGHTARROW ) ||
            ( key >= K_KP_SLASH && key <= K_KP_PLUS ) || ( key >= K_KP_STAR && key <= K_KP_EQUALS ) )
    {
    }
    
    //----(SA)	added some mousewheel functionality to the console
    if( ( key == K_MWHEELUP && keys[K_SHIFT].down ) || ( key == K_UPARROW ) || ( key == K_KP_UPARROW ) ||
            ( ( tolower( key ) == 'p' ) && keys[K_CTRL].down ) )
    {
        Q_strncpyz( g_consoleField.buffer, consoleHistorySystem->Prev(), sizeof( g_consoleField.buffer ) );
        g_consoleField.cursor = strlen( g_consoleField.buffer );
        if( g_consoleField.cursor >= g_consoleField.widthInChars )
        {
            g_consoleField.scroll = g_consoleField.cursor - g_consoleField.widthInChars + 1;
        }
        else
        {
            g_consoleField.scroll = 0;
        }
        return;
    }
    
    //----(SA)	added some mousewheel functionality to the console
    if( ( key == K_MWHEELDOWN && keys[K_SHIFT].down ) || ( key == K_DOWNARROW ) || ( key == K_KP_DOWNARROW ) ||
            ( ( tolower( key ) == 'n' ) && keys[K_CTRL].down ) )
    {
        pointer history = consoleHistorySystem->Next( g_consoleField.buffer );
        if( history )
        {
            Q_strncpyz( g_consoleField.buffer, history, sizeof( g_consoleField.buffer ) );
            g_consoleField.cursor = strlen( g_consoleField.buffer );
            if( g_consoleField.cursor >= g_consoleField.widthInChars )
            {
                g_consoleField.scroll = g_consoleField.cursor - g_consoleField.widthInChars + 1;
            }
            else
            {
                g_consoleField.scroll = 0;
            }
        }
        else if( g_consoleField.buffer[0] )
        {
            consoleHistorySystem->Add( g_consoleField.buffer );
            cmdCompletionSystem->Clear( &g_consoleField );
        }
        return;
    }
    
    // console scroll only if not using commandMode
    if( !commandMode )
    {
        // console tab switching
        if( key == K_LEFTARROW && keys[K_ALT].down )
        {
            Con_ConsoleNext( -1 );
            return;
        }
        else if( key == K_RIGHTARROW && keys[K_ALT].down )
        {
            Con_ConsoleNext( 1 );
            return;
        }
        
        // console scrolling
        if( key == K_PGUP )
        {
            Con_PageUp();
            return;
        }
        
        if( key == K_PGDN )
        {
            Con_PageDown();
            return;
        }
        
        if( key == K_MWHEELUP )
        {
            Con_PageUp();
            
            // hold <ctrl> to accelerate scrolling
            if( keys[K_CTRL].down )
            {
                Con_PageUp();
                Con_PageUp();
            }
            
            return;
        }
        
        if( key == K_MWHEELDOWN )
        {
            Con_PageDown();
            
            // hold <ctrl> to accelerate scrolling
            if( keys[K_CTRL].down )
            {
                Con_PageDown();
                Con_PageDown();
            }
            
            return;
        }
        
        // ctrl-home = top of console
        if( key == K_HOME && keys[K_CTRL].down )
        {
            Con_Top();
            return;
        }
        
        // ctrl-end = bottom of console
        if( key == K_END && keys[K_CTRL].down )
        {
            Con_Bottom();
            return;
        }
    }
    
    // pass to the normal editline routine
    cmdCompletionSystem->KeyDownEvent( &g_consoleField, key );
}

//============================================================================


/*
================
Message_Key

In game talk message
================
*/
void Message_Key( sint key )
{
    valueType buffer[MAX_STRING_CHARS];
    
    if( key == K_ESCAPE )
    {
        cls.keyCatchers &= ~KEYCATCH_MESSAGE;
        cmdCompletionSystem->Clear( &chatField );
        return;
    }
    
    if( key == K_ENTER || key == K_KP_ENTER )
    {
        if( chatField.buffer[0] && cls.state == CA_ACTIVE )
        {
            if( chat_team )
            {
                Q_vsprintf_s( buffer, sizeof( buffer ), sizeof( buffer ), "say_team \"%s\"\n", chatField.buffer );
            }
            else if( chat_buddy )
            {
                Q_vsprintf_s( buffer, sizeof( buffer ), sizeof( buffer ), "say_buddy \"%s\"\n", chatField.buffer );
            }
            else
            {
                Q_vsprintf_s( buffer, sizeof( buffer ), sizeof( buffer ), "say \"%s\"\n", chatField.buffer );
            }
            
            if( buffer[0] )
            {
                CL_AddReliableCommand( buffer );
            }
        }
        cls.keyCatchers &= ~KEYCATCH_MESSAGE;
        cmdCompletionSystem->Clear( &chatField );
        return;
    }
    
    cmdCompletionSystem->KeyDownEvent( &chatField, key );
}

//============================================================================


bool Key_GetOverstrikeMode( void )
{
    return key_overstrikeMode;
}


void Key_SetOverstrikeMode( bool state )
{
    key_overstrikeMode = state;
}


/*
===================
Key_IsDown
===================
*/
bool Key_IsDown( sint keynum )
{
    if( keynum < 0 || keynum >= MAX_KEYS )
    {
        return false;
    }
    
    return keys[keynum].down;
}


/*
===================
Key_StringToKeynum

Returns a key number to be used to index keys[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.

0x11 will be interpreted as raw hex, which will allow new controlers

to be configured even if they don't have defined names.
===================
*/
sint Key_StringToKeynum( pointer str )
{
    keyname_t*   kn;
    
    if( !str || !str[0] )
    {
        return -1;
    }
    if( !str[1] )
    {
        return tolower( str[0] );
    }
    
    // check for hex code
    if( strlen( str ) == 4 )
    {
        sint n = Com_HexStrToInt( str );
        
        if( n >= 0 )
        {
            return n;
        }
    }
    
    // scan for a text match
    for( kn = keynames ; kn->name ; kn++ )
    {
        if( !Q_stricmp( str, kn->name ) )
            return kn->keynum;
    }
    
    return -1;
}

/*
===================
Key_KeynumToString

Returns a string (either a single ascii char, a K_* name, or a 0x11 hex string) for the
given keynum.
===================
*/
valueType* Key_KeynumToString( sint keynum )
{
    keyname_t*   kn;
    static valueType tinystr[5];
    sint i, j;
    
    if( keynum == -1 )
    {
        return "<KEY NOT FOUND>";
    }
    
    if( keynum < 0 || keynum >= MAX_KEYS )
    {
        return "<OUT OF RANGE>";
    }
    
    // check for printable ascii (don't use quote)
    if( keynum > 32 && keynum < 127 && keynum != '"' && keynum != ';' )
    {
        tinystr[0] = static_cast<valueType>( keynum );
        tinystr[1] = 0;
        return tinystr;
    }
    
    // check for a key string
    for( kn = keynames ; kn->name ; kn++ )
    {
        if( keynum == kn->keynum )
        {
            return kn->name;
        }
    }
    
    // make a hex string
    i = keynum >> 4;
    j = keynum & 15;
    
    tinystr[0] = '0';
    tinystr[1] = 'x';
    tinystr[2] = static_cast<valueType>( i > 9 ? i - 10 + 'a' : i + '0' );
    tinystr[3] = static_cast<valueType>( j > 9 ? j - 10 + 'a' : j + '0' );
    tinystr[4] = 0;
    
    return tinystr;
}

#define BIND_HASH_SIZE 1024

static sint32 generateHashValue( pointer fname )
{
    sint i;
    sint32 hash;
    
    if( !fname )
    {
        return 0;
    }
    
    hash = 0;
    i = 0;
    while( fname[i] != '\0' )
    {
        hash += static_cast<sint32>( fname[i] ) * ( i + 119 );
        i++;
    }
    hash &= ( BIND_HASH_SIZE - 1 );
    return hash;
}

/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding( sint keynum, pointer binding )
{

    valueType* lcbinding;    // fretn - make a copy of our binding lowercase
    // so name toggle scripts work again: bind x name BzZIfretn?
    // resulted into bzzifretn?
    
    if( keynum < 0 || keynum >= MAX_KEYS )
    {
        return;
    }
    
    // free old bindings
    if( keys[ keynum ].binding )
    {
        Z_Free( keys[ keynum ].binding );
    }
    
    // allocate memory for new binding
    keys[keynum].binding = CopyString( binding );
    lcbinding = CopyString( binding );
    Q_strlwr( lcbinding ); // saves doing it on all the generateHashValues in Key_GetBindingByString
    
    keys[keynum].hash = generateHashValue( lcbinding );
    
    // consider this like modifying an archived cvar, so the
    // file write will be triggered at the next oportunity
    cvar_modifiedFlags |= CVAR_ARCHIVE;
}


/*
===================
Key_GetBinding
===================
*/
valueType* Key_GetBinding( sint keynum )
{
    if( keynum < 0 || keynum >= MAX_KEYS )
    {
        return "";
    }
    
    return keys[ keynum ].binding;
}

// binding MUST be lower case
void Key_GetBindingByString( pointer binding, sint* key1, sint* key2 )
{
    sint i;
    sint hash = generateHashValue( binding );
    
    *key1 = -1;
    *key2 = -1;
    
    for( i = 0; i < MAX_KEYS; i++ )
    {
        if( keys[i].hash == hash && !Q_stricmp( binding, keys[i].binding ) )
        {
            if( *key1 == -1 )
            {
                *key1 = i;
            }
            else if( *key2 == -1 )
            {
                *key2 = i;
                return;
            }
        }
    }
}

/*
===================
Key_GetKey
===================
*/

sint Key_GetKey( pointer binding )
{
    sint i;
    
    if( binding )
    {
        for( i = 0 ; i < MAX_KEYS ; i++ )
        {
            if( keys[i].binding && Q_stricmp( binding, keys[i].binding ) == 0 )
            {
                return i;
            }
        }
    }
    return -1;
}

/*
===================
Key_Unbind_f
===================
*/
void Key_Unbind_f( void )
{
    sint b;
    
    if( cmdSystem->Argc() != 2 )
    {
        Com_Printf( "unbind <key> : remove commands from a key\n" );
        return;
    }
    
    b = Key_StringToKeynum( cmdSystem->Argv( 1 ) );
    if( b == -1 )
    {
        Com_Printf( "\"%s\" isn't a valid key\n", cmdSystem->Argv( 1 ) );
        return;
    }
    
    Key_SetBinding( b, "" );
}

/*
===================
Key_Unbindall_f
===================
*/
void Key_Unbindall_f( void )
{
    sint i;
    
    for( i = 0 ; i < MAX_KEYS; i++ )
        if( keys[i].binding )
        {
            Key_SetBinding( i, "" );
        }
}


/*
===================
Key_Bind_f
===================
*/
void Key_Bind_f( void )
{
    sint c, b;
    
    c = cmdSystem->Argc();
    
    if( c < 2 )
    {
        Com_Printf( "bind <key> [command] : attach a command to a key\n" );
        return;
    }
    b = Key_StringToKeynum( cmdSystem->Argv( 1 ) );
    if( b == -1 )
    {
        Com_Printf( "\"%s\" isn't a valid key\n", cmdSystem->Argv( 1 ) );
        return;
    }
    
    if( c == 2 )
    {
        if( keys[b].binding )
        {
            Com_Printf( "\"%s\" = \"%s\"\n", Key_KeynumToString( b ), keys[b].binding );
        }
        else
        {
            Com_Printf( "\"%s\" is not bound\n", Key_KeynumToString( b ) );
        }
        return;
    }
    
    // set to 3rd arg onwards, unquoted from raw
    Key_SetBinding( b, Com_UnquoteStr( cmdSystem->FromNth( 2 ) ) );
}

/*
===================
Key_EditBind_f
===================
*/
void Key_EditBind_f( void )
{
    valueType* buf;
    /*const*/
    valueType* key, *binding, *keyq;
    sint b;
    
    b = cmdSystem->Argc();
    if( b != 2 )
    {
        Com_Printf( "editbind <key> : edit a key binding (in the in-game console)" );
        return;
    }
    key = cmdSystem->Argv( 1 );
    b = Key_StringToKeynum( key );
    if( b == -1 )
    {
        Com_Printf( "\"%s\" isn't a valid key\n", key );
        return;
    }
    
    binding = Key_GetBinding( b );
    
    keyq = ( const_cast<valueType*>( reinterpret_cast<const valueType*>( Com_QuoteStr( key ) ) ) ); // <- static buffer
    buf = reinterpret_cast< valueType*>( malloc( 8 + strlen( keyq ) + strlen( binding ) ) );
    Q_vsprintf_s( buf, sizeof( buf ), sizeof( buf ), "/bind %s %s", keyq, binding );
    
    Con_OpenConsole_f();
    cmdCompletionSystem->Set( &g_consoleField, buf );
    free( buf );
}


/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings( fileHandle_t f )
{
    sint i;
    
    fileSystem->Printf( f, "unbindall\n" );
    
    for( i = 0 ; i < MAX_KEYS ; i++ )
    {
        if( keys[i].binding && keys[i].binding[0] )
        {
            // quote the string if it contains ; but no "
            fileSystem->Printf( f, "bind %s %s\n", Key_KeynumToString( i ), Com_QuoteStr( keys[i].binding ) );
        }
        
    }
}


/*
============
Key_Bindlist_f

============
*/
void Key_Bindlist_f( void )
{
    sint i;
    
    for( i = 0 ; i < MAX_KEYS ; i++ )
    {
        if( keys[i].binding && keys[i].binding[0] )
        {
            Com_Printf( "%s = %s\n", Key_KeynumToString( i ), keys[i].binding );
        }
    }
}

/*
============
Key_KeynameCompletion
============
*/
void Key_KeynameCompletion( void( *callback )( pointer s ) )
{
    sint		i;
    
    for( i = 0; keynames[ i ].name != nullptr; i++ )
        callback( keynames[ i ].name );
}

/*
====================
Key_CompleteUnbind
====================
*/
static void Key_CompleteUnbind( valueType* args, sint argNum )
{
    if( argNum == 2 )
    {
        // Skip "unbind "
        valueType* p = Com_SkipTokens( args, 1, " " );
        
        if( p > args )
            cmdCompletionSystem->CompleteKeyname( );
    }
}

/*
====================
Key_CompleteBind
====================
*/
void Key_CompleteBind( valueType* args, sint argNum )
{
    valueType* p;
    
    if( argNum == 2 )
    {
        // Skip "bind "
        p = Com_SkipTokens( args, 1, " " );
        
        if( p > args )
            cmdCompletionSystem->CompleteKeyname( );
    }
    else if( argNum >= 3 )
    {
        // Skip "bind <key> "
        p = Com_SkipTokens( args, 2, " " );
        
        if( p > args )
            cmdCompletionSystem->CompleteCommand( p, true, true );
    }
}

static void Key_CompleteEditbind( valueType* args, sint argNum )
{
    valueType* p;
    
    p = Com_SkipTokens( args, 1, " " );
    
    if( p > args )
    {
        cmdCompletionSystem->CompleteKeyname();
    }
}


/*
===================
CL_InitKeyCommands
===================
*/
void CL_InitKeyCommands( void )
{
    // register our functions
    cmdSystem->AddCommand( "bind", Key_Bind_f, "Used for assigning keys to actions. Bind x “weaponbank 3" );
    cmdSystem->SetCommandCompletionFunc( "bind", Key_CompleteBind );
    cmdSystem->AddCommand( "unbind", Key_Unbind_f, "Displays list of cvars in console" );
    cmdSystem->SetCommandCompletionFunc( "unbind", Key_CompleteUnbind );
    cmdSystem->AddCommand( "unbindall", Key_Unbindall_f, "For unassigning all commands etc from ALL keys. /unbindall" );
    cmdSystem->AddCommand( "bindlist", Key_Bindlist_f, "Displays list of cvars in console" );
    cmdSystem->AddCommand( "editbind", Key_EditBind_f, "Used for editing already binded key" );
    cmdSystem->SetCommandCompletionFunc( "editbind", Key_CompleteEditbind );
}

/*
===================
CL_AddKeyUpCommands
===================
*/
void CL_AddKeyUpCommands( sint key, valueType* kb, sint time )
{
    sint i;
    valueType button[1024], *buttonPtr;
    valueType	cmd[1024];
    bool keyevent;
    
    if( !kb )
    {
        return;
    }
    keyevent = false;
    buttonPtr = button;
    for( i = 0; ; i++ )
    {
        if( kb[i] == ';' || !kb[i] )
        {
            *buttonPtr = '\0';
            if( button[0] == '+' )
            {
                // button commands add keynum and time as parms so that multiple
                // sources can be discriminated and subframe corrected
                Q_vsprintf_s( cmd, sizeof( cmd ), sizeof( cmd ), "-%s %i %i\n", button + 1, key, time );
                cmdBufferSystem->AddText( cmd );
                keyevent = true;
            }
            else
            {
                if( keyevent )
                {
                    // down-only command
                    cmdBufferSystem->AddText( button );
                    cmdBufferSystem->AddText( "\n" );
                }
            }
            buttonPtr = button;
            while( ( kb[i] <= ' ' || kb[i] == ';' ) && kb[i] != 0 )
            {
                i++;
            }
        }
        *buttonPtr++ = kb[i];
        if( !kb[i] )
        {
            break;
        }
    }
}


/*
===================
CL_KeyEvent

Called by the system for both key up and key down events
===================
*/
//static consoleCount = 0;
// fretn
bool consoleButtonWasPressed = false;

void CL_KeyEvent( sint key, sint down, sint time )
{
    valueType*    kb;
    valueType cmd[1024];
    bool bypassMenu = false;       // NERVE - SMF
    bool onlybinds = false;
    
    if( !key )
    {
        return;
    }
    
    switch( key )
    {
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
            if( idsystem->IsNumLockDown() )
            {
                onlybinds = true;
            }
            break;
    }
    
    
    // update auto-repeat status and BUTTON_ANY status
    keys[key].down = ( bool )down;
    
    if( down )
    {
        keys[key].repeats++;
        if( keys[key].repeats == 1 )
        {
            anykeydown++;
        }
    }
    else
    {
        keys[key].repeats = 0;
        anykeydown--;
        if( anykeydown < 0 )
        {
            anykeydown = 0;
        }
    }
    
    if( key == K_ENTER )
    {
        if( down )
        {
            if( keys[K_ALT].down )
            {
                Key_ClearStates();
                
                // don't repeat fullscreen toggle when keys are held down
                if( keys[K_ENTER].repeats > 1 )
                {
                    return;
                }
                
                if( cvarSystem->VariableValue( "r_fullscreen" ) == 0 )
                {
                    Com_Printf( "Switching to fullscreen rendering\n" );
                    cvarSystem->Set( "r_fullscreen", "1" );
                }
                else
                {
                    Com_Printf( "Switching to windowed rendering\n" );
                    cvarSystem->Set( "r_fullscreen", "0" );
                }
                cmdBufferSystem->ExecuteText( EXEC_APPEND, "vid_restart\n" );
                return;
            }
        }
    }
    
#if defined (MACOS_X)
    if( down && keys[ K_COMMAND ].down )
    {
    
        if( key == 'f' )
        {
            Key_ClearStates();
            cmdBufferSystem->ExecuteText( EXEC_APPEND, "toggle r_fullscreen\nvid_restart\n" );
            return;
        }
        else if( key == 'q' )
        {
            Key_ClearStates();
            cmdBufferSystem->ExecuteText( EXEC_APPEND, "quit\n" );
            return;
        }
        else if( key == K_TAB )
        {
            Key_ClearStates();
            cvarSystem->SetValue( "r_minimize", 1 );
            return;
        }
    }
#endif
    
    if( cl_altTab->integer && keys[K_ALT].down && key == K_TAB )
    {
        Key_ClearStates();
        cvarSystem->SetValue( "r_minimize", 1 );
        return;
    }
    
    // console key is hardcoded, so the user can never unbind it
    if( key == K_CONSOLE || ( keys[K_SHIFT].down && key == K_ESCAPE ) )
    {
        if( !down )
        {
            return;
        }
        Con_ToggleConsole_f();
        Key_ClearStates();
        return;
    }
    
//----(SA)	added
    if( cl.cameraMode )
    {
        if( !( cls.keyCatchers & ( KEYCATCH_UI | KEYCATCH_CONSOLE ) ) )       // let menu/console handle keys if necessary
        {
            // in cutscenes we need to handle keys specially (pausing not allowed in camera mode)
            if( ( key == K_ESCAPE ||
                    key == K_SPACE ||
                    key == K_ENTER ) && down )
            {
                CL_AddReliableCommand( "cameraInterrupt" );
                return;
            }
            
            // eat all keys
            if( down )
            {
                return;
            }
        }
        
        if( ( cls.keyCatchers & KEYCATCH_CONSOLE ) && key == K_ESCAPE )
        {
            // don't allow menu starting when console is down and camera running
            return;
        }
    }
    //----(SA)	end
    
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
    if( key == K_ESCAPE && down )
    {
        //If console is active then ESC should close console
        if( cls.keyCatchers & KEYCATCH_CONSOLE )
        {
            Con_ToggleConsole_f();
            Key_ClearStates();
            return;
        }
        
        if( cls.keyCatchers & KEYCATCH_MESSAGE )
        {
            // clear message mode
            Message_Key( key );
            return;
        }
        
        // escape always gets out of CGAME stuff
        if( cls.keyCatchers & KEYCATCH_CGAME )
        {
            cls.keyCatchers &= ~KEYCATCH_CGAME;
            cgame->EventHandling( CGAME_EVENT_NONE, true );
            return;
        }
        
        if( !( cls.keyCatchers & KEYCATCH_UI ) )
        {
            if( cls.state == CA_ACTIVE && !clc.demoplaying )
            {
                // Arnout: on request
                if( cls.keyCatchers & KEYCATCH_CONSOLE )     // get rid of the console
                {
                    Con_ToggleConsole_f();
                }
                else
                {
                    uiManager->SetActiveMenu( UIMENU_INGAME );
                }
            }
            else
            {
                CL_Disconnect_f();
                soundSystem->StopAllSounds();
                uiManager->SetActiveMenu( UIMENU_MAIN );
            }
            return;
        }
        
        uiManager->KeyEvent( key, down );
        return;
    }
    
    //
    // key up events only perform actions if the game key binding is
    // a button command (leading + sign).  These will be processed even in
    // console mode and menu mode, to keep the character from continuing
    // an action started before a mode switch.
    //
    if( !down )
    {
        kb = keys[key].binding;
        if( kb && kb[0] == '+' )
        {
            // button commands add keynum and time as parms so that multiple
            // sources can be discriminated and subframe corrected
            Q_vsprintf_s( cmd, sizeof( cmd ), sizeof( cmd ), "-%s %i %i\n", kb + 1, key, time );
            cmdBufferSystem->AddText( cmd );
        }
        
        if( cls.keyCatchers & KEYCATCH_UI )
        {
            if( !onlybinds || uiManager->WantsBindKeys() )
            {
                uiManager->KeyEvent( key, down );
            }
        }
        else if( cls.keyCatchers & KEYCATCH_CGAME )
        {
            if( !onlybinds || cgame->WantsBindKeys() )
            {
                cgame->KeyEvent( key, down );
            }
        }
        
        return;
    }
    
    // NERVE - SMF - if we just want to pass it along to game
    if( cl_bypassMouseInput && cl_bypassMouseInput->integer )
    {
        if( ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_MOUSE3 || key == K_MOUSE4 || key == K_MOUSE5 ) )
        {
            if( cl_bypassMouseInput->integer == 1 )
            {
                bypassMenu = true;
            }
        }
        else if( ( cls.keyCatchers & KEYCATCH_UI && !clientGUISystem->checkKeyExec( key ) ) || ( cls.keyCatchers & KEYCATCH_CGAME && !clientGameSystem->CGameCheckKeyExec( key ) ) )
        {
            bypassMenu = true;
        }
    }
    
    //
    // key up events only perform actions if the game key binding is
    // a button command (leading + sign).  These will be processed even in
    // console mode and menu mode, to keep the character from continuing
    // an action started before a mode switch.
    //
    if( !down )
    {
        kb = keys[key].binding;
        
        CL_AddKeyUpCommands( key, kb, time );
        
        if( cls.keyCatchers & ( KEYCATCH_UI | KEYCATCH_BUG ) )
        {
            uiManager->KeyEvent( key, down );
        }
        else if( cls.keyCatchers & KEYCATCH_CGAME && cgvm )
        {
            cgame->KeyEvent( key, down );
        }
        
        return;
    }
    
    // distribute the key down event to the apropriate handler
    if( cls.keyCatchers & KEYCATCH_CONSOLE )
    {
        if( !onlybinds )
        {
            Console_DownEventKey( key );
        }
    }
    else if( cls.keyCatchers & ( KEYCATCH_UI | KEYCATCH_BUG ) )
    {
        uiManager->KeyEvent( key, down );
    }
    else if( cls.keyCatchers & KEYCATCH_UI && !bypassMenu )
    {
        if( !onlybinds || uiManager->WantsBindKeys() )
        {
            uiManager->KeyEvent( key, down );
        }
    }
    else if( cls.keyCatchers & KEYCATCH_CGAME && !bypassMenu )
    {
        if( cgvm )
        {
            if( !onlybinds || cgame->WantsBindKeys() )
            {
                cgame->KeyEvent( key, down );
            }
        }
    }
    else if( cls.keyCatchers & KEYCATCH_MESSAGE )
    {
        if( !onlybinds )
        {
            Message_Key( key );
        }
    }
    else if( cls.state == CA_DISCONNECTED )
    {
        if( !onlybinds )
        {
            Console_DownEventKey( key );
        }
    }
    else
    {
        // send the bound action
        kb = keys[key].binding;
        if( !kb )
        {
            if( key >= 200 )
            {
                Com_Printf( "%s is unbound, use controls menu to set.\n"
                            , Key_KeynumToString( key ) );
            }
        }
        else if( kb[0] == '+' )
        {
            // button commands add keynum and time as parms so that multiple
            // sources can be discriminated and subframe corrected
            Q_vsprintf_s( cmd, sizeof( cmd ), sizeof( cmd ), "%s %i %i\n", kb, key, time );
            cmdBufferSystem->AddText( cmd );
        }
        else
        {
            // down-only command
            cmdBufferSystem->AddText( kb );
            cmdBufferSystem->AddText( "\n" );
        }
    }
}


/*
==================
CL_KeyCharEvents
==================
*/
static void CL_KeyCharEvents( sint ch )
{
    // ctrl-L clears screen
    if( ch == KEYBOARDCTRL( 'l' ) )
    {
        cmdBufferSystem->AddText( "clear\n" );
        return;
    }
    
    // alt-n, alt-p switches console
    if( keys[K_ALT].down )
    {
        switch( ch )
        {
            case 'p':
            case 'P':
                Con_ConsoleNext( -1 );
                return;
            case 'n':
            case 'N':
                Con_ConsoleNext( 1 );
                return;
        }
    }
    
    // tab completes command or switches console
    if( ch == '\t' )
    {
        if( keys[K_SHIFT].down )
        {
            Con_ConsoleNext( -1 );
        }
        else if( keys[K_CTRL].down )
        {
            Con_ConsoleNext( 1 );
        }
        else
        {
            sint conNum = activeCon - con;
            
            // autocomplete only for non-chat consoles
            if( conNum != CON_CHAT && conNum != CON_TCHAT )
                CompleteCommand();
        }
        return;
    }
    
    // pass to the normal editline routine
    cmdCompletionSystem->CharEvent( &g_consoleField, ch );
}


/*
===================
CL_CharEvent

Normal keyboard characters, already shifted / capslocked / etc
===================
*/
void CL_CharEvent( sint key )
{
    // the console key should never be used as a char
    // ydnar: added uk equivalent of shift+`
    // the RIGHT way to do this would be to have certain keys disable the equivalent SYSE_CHAR event
    
    // fretn - this should be fixed in Com_EventLoop
    // but I can't be arsed to leave this as is
    
    if( key == static_cast<uchar8>( '`' ) || key == static_cast<uchar8>( '~' ) || key == static_cast<uchar8>( '¬' ) )
    {
        return;
    }
    
    // distribute the key down event to the apropriate handler
    if( cls.keyCatchers & KEYCATCH_CONSOLE )
    {
        CL_KeyCharEvents( key );
    }
    else if( cls.keyCatchers & ( KEYCATCH_UI | KEYCATCH_BUG ) )
    {
        uiManager->KeyEvent( key | K_CHAR_FLAG, true );
    }
    else if( cls.keyCatchers & KEYCATCH_UI )
    {
        uiManager->KeyEvent( key | K_CHAR_FLAG, true );
    }
    else if( cls.keyCatchers & KEYCATCH_CGAME )
    {
        cgame->KeyEvent( key | K_CHAR_FLAG, true );
    }
    else if( cls.keyCatchers & KEYCATCH_MESSAGE )
    {
        cmdCompletionSystem->CharEvent( &chatField, key );
    }
    else if( cls.state == CA_DISCONNECTED )
    {
        cmdCompletionSystem->CharEvent( &g_consoleField, key );
    }
}


/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates( void )
{
    sint i;
    
    anykeydown = 0;
    
    for( i = 0 ; i < MAX_KEYS ; i++ )
    {
        if( keys[i].down )
        {
            CL_KeyEvent( i, false, 0 );
            
        }
        keys[i].down = ( bool )0;
        keys[i].repeats = 0;
    }
}
