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

#ifndef __CVARSYSTEM_API_H__
#define __CVARSYSTEM_API_H__

/*
==========================================================
CVARS (console variables)

Many variables can be used for cheating purposes, so when
cheats is zero, force all unspecified variables to their
default values.
==========================================================
*/

//Dushan
typedef enum cvar_flags_s
{
    CVAR_ARCHIVE                   = BIT( 0 ),    // set to cause it to be saved to vars.rc
    // used for system variables, not for player
    // specific configurations
    CVAR_USERINFO                  = BIT( 1 ),    // sent to server on connect or change
    CVAR_SERVERINFO                = BIT( 2 ),    // sent in response to front end requests
    CVAR_SYSTEMINFO                = BIT( 3 ),    // these cvars will be duplicated on all clients
    CVAR_INIT                      = BIT( 4 ),    // don't allow change from console at all,
    // but can be set from the command line
    CVAR_LATCH                     = BIT( 5 ),    // will only change when C code next does
    // a Cvar_Get(), so it can't be changed
    // without proper initialization.  modified
    // will be set, even though the value hasn't
    // changed yet
    CVAR_ROM                       = BIT( 6 ),    // display only, cannot be set by user at all
    CVAR_USER_CREATED              = BIT( 7 ),    // created by a set command
    CVAR_TEMP                      = BIT( 8 ),    // can be set even when cheats are disabled, but is not archived
    CVAR_CHEAT                     = BIT( 9 ),    // can not be changed if cheats are disabled
    CVAR_NORESTART                 = BIT( 10 ),   // do not clear when a cvar_restart is issued
    CVAR_WOLFINFO                  = BIT( 11 ),   // DHM - NERVE :: Like userinfo, but for wolf multiplayer info
    CVAR_UNSAFE                    = BIT( 12 ),   // ydnar: unsafe system cvars (renderer, sound settings, anything that might cause a crash)
    CVAR_SERVERINFO_NOUPDATE       = BIT( 13 ),   // gordon: WONT automatically send this to clients, but server browsers will see it
    CVAR_SHADER                    = BIT( 14 ),   // tell renderer to recompile shaders.
    CVAR_NONEXISTENT	           = 0xFFFFFFFF   // Cvar doesn't exist.
} cvar_flags_t;

// nothing outside the Cvar_*() functions should modify these fields!
typedef struct convar_s
{
    sint flags;
    sint modificationCount;	// incremented each time the cvar is changed
    sint integer;	// atoi( string )
    float32 value;		// atof( string )
    float32 min;
    float32 max;
    valueType* name;
    valueType* string;
    valueType* resetString;	// cvar_restart will reset to this value
    valueType* latchedString;	// for CVAR_LATCH vars
    valueType* description;
    bool modified;	// set each time the cvar is changed
    bool validate;
    bool integral;
    
    struct convar_s*  next;
    struct convar_s*  hashNext;
} convar_t;

#define MAX_CVAR_VALUE_STRING   256

typedef sint cvarHandle_t;

// the modules that run in the virtual machine can't access the convar_t directly,
// so they must ask for structured updates
typedef struct
{
    sint modificationCount;
    sint integer;
    float32 value;
    valueType string[MAX_CVAR_VALUE_STRING];
    cvarHandle_t handle;
} vmConvar_t;

//
// idCVarSystem
//
class idCVarSystem
{
public:
    virtual float32 VariableValue( pointer var_name ) = 0;
    virtual sint VariableIntegerValue( pointer var_name ) = 0;
    virtual valueType* VariableString( pointer var_name ) = 0;
    virtual void VariableStringBuffer( pointer var_name, valueType* buffer, sint bufsize ) = 0;
    virtual void LatchedVariableStringBuffer( pointer var_name, valueType* buffer, sint bufsize ) = 0;
    virtual sint Flags( pointer var_name ) = 0;
    virtual void CommandCompletion( void( *callback )( pointer s ) ) = 0;
    virtual valueType* ClearForeignCharacters( pointer value ) = 0;
    virtual convar_t* Get( pointer var_name, pointer var_value, sint flags, pointer description ) = 0;
    virtual convar_t* GetSet2( pointer var_name, pointer value, bool force ) = 0;
    virtual void Set( pointer var_name, pointer value ) = 0;
    virtual void SetLatched( pointer var_name, pointer value ) = 0;
    virtual void SetValue( pointer var_name, float32 value ) = 0;
    virtual void SetValueSafe( pointer var_name, float32 value ) = 0;
    virtual void SetValueLatched( pointer var_name, float32 value ) = 0;
    virtual void Reset( pointer var_name ) = 0;
    virtual void SetCheatState( void ) = 0;
    virtual bool Command( void ) = 0;
    virtual void WriteVariables( fileHandle_t f ) = 0;
    virtual valueType* InfoString( sint bit ) = 0;
    virtual valueType* InfoString_Big( sint bit ) = 0;
    virtual void InfoStringBuffer( sint bit, valueType* buff, sint buffsize ) = 0;
    virtual void CheckRange( convar_t* var, float32 min, float32 max, bool integral ) = 0;
    virtual void Register( vmConvar_t* vmCvar, pointer varName, pointer defaultValue, sint flags, pointer description ) = 0;
    virtual void Update( vmConvar_t* vmCvar ) = 0;
    virtual void Init( void ) = 0;
    virtual void Shutdown( void ) = 0;
};

extern idCVarSystem* cvarSystem;

#endif // !__CVARSYSTEM_API_H__
