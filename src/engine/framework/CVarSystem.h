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
// File name:   CVarSystem.h
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: dynamic variable tracking
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CVARSYSTEM_H__
#define __CVARSYSTEM_H__

extern S32 cvar_modifiedFlags;

//
// idFileSystemLocal
//
class idCVarSystemLocal : public idCVarSystem
{
public:
    idCVarSystemLocal( void );
    ~idCVarSystemLocal( void );
    
    static S64 generateHashValue( StringEntry fname );
    static bool ValidateString( StringEntry s );
    static convar_t* FindVar( StringEntry var_name );
    virtual F32 VariableValue( StringEntry var_name );
    virtual S32 VariableIntegerValue( StringEntry var_name );
    virtual UTF8* VariableString( StringEntry var_name );
    virtual void VariableStringBuffer( StringEntry var_name, UTF8* buffer, S32 bufsize );
    virtual void LatchedVariableStringBuffer( StringEntry var_name, UTF8* buffer, S32 bufsize );
    virtual S32 Flags( StringEntry var_name );
    virtual void CommandCompletion( void( *callback )( StringEntry s ) );
    virtual UTF8* ClearForeignCharacters( StringEntry value );
    virtual convar_t* Get( StringEntry var_name, StringEntry var_value, S32 flags, StringEntry description );
    virtual convar_t* GetSet2( StringEntry var_name, StringEntry value, bool force );
    virtual void Set( StringEntry var_name, StringEntry value );
    virtual void SetLatched( StringEntry var_name, StringEntry value );
    virtual void SetValue( StringEntry var_name, F32 value );
    virtual void SetValueSafe( StringEntry var_name, F32 value );
    virtual void SetValueLatched( StringEntry var_name, F32 value );
    virtual void Reset( StringEntry var_name );
    virtual void SetCheatState( void );
    virtual bool Command( void );
    static void Toggle_f( void );
    static void Cycle_f( void );
    static void Set_f( void );
    static void SetU_f( void );
    static void SetS_f( void );
    static void SetA_f( void );
    static void Reset_f( void );
    virtual void WriteVariables( fileHandle_t f );
    static void List_f( void );
    static void Restart_f( void );
    virtual UTF8* InfoString( S32 bit );
    virtual UTF8* InfoString_Big( S32 bit );
    virtual void InfoStringBuffer( S32 bit, UTF8* buff, S32 buffsize );
    virtual void CheckRange( convar_t* var, F32 min, F32 max, bool integral );
    virtual void Register( vmConvar_t* vmCvar, StringEntry varName, StringEntry defaultValue, S32 flags, StringEntry description );
    virtual void Update( vmConvar_t* vmCvar );
    static void CompleteCvarName( UTF8* args, S32 argNum );
    virtual void Init( void );
    virtual void Shutdown( void );
    static const UTF8* Validate( convar_t* var, StringEntry value, bool warn );
};

extern idCVarSystemLocal cvarSystemLocal;


#endif // !__CVARSYSTEM_H__
