////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 - 2023 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   CVarSystem.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description: dynamic variable tracking
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CVARSYSTEM_HPP__
#define __CVARSYSTEM_HPP__

extern sint cvar_modifiedFlags;

//
// idFileSystemLocal
//
class idCVarSystemLocal : public idCVarSystem {
public:
    idCVarSystemLocal(void);
    ~idCVarSystemLocal(void);

    static sint32 generateHashValue(pointer fname);
    static bool ValidateString(pointer s);
    virtual convar_t *FindVar(pointer var_name);
    virtual float32 VariableValue(pointer var_name);
    virtual sint VariableIntegerValue(pointer var_name);
    virtual valueType *VariableString(pointer var_name);
    virtual void VariableStringBuffer(pointer var_name, valueType *buffer,
                                      uint64 bufsize);
    virtual void LatchedVariableStringBuffer(pointer var_name,
            valueType *buffer, uint64 bufsize);
    virtual sint Flags(pointer var_name);
    virtual void CommandCompletion(void(*callback)(pointer s));
    virtual valueType *ClearForeignCharacters(pointer value);
    virtual convar_t *Get(pointer var_name, pointer var_value, sint flags,
                          pointer description);
    virtual convar_t *GetSet2(pointer var_name, pointer value, bool force);
    virtual void Set(pointer var_name, pointer value);
    virtual void SetLatched(pointer var_name, pointer value);
    virtual void SetValue(pointer var_name, float32 value);
    virtual void SetValueSafe(pointer var_name, float32 value);
    virtual void SetValueLatched(pointer var_name, float32 value);
    virtual void Reset(pointer var_name);
    virtual void SetCheatState(void);
    virtual bool Command(void);
    static void Toggle_f(void);
    static void Cycle_f(void);
    static void Set_f(void);
    static void SetU_f(void);
    static void SetS_f(void);
    static void SetA_f(void);
    static void Reset_f(void);
    virtual void WriteVariables(fileHandle_t f);
    static void List_f(void);
    static void Restart_f(void);
    virtual valueType *InfoString(sint bit);
    virtual valueType *InfoString_Big(sint bit);
    virtual void InfoStringBuffer(sint bit, valueType *buff, uint64 buffsize);
    virtual void CheckRange(convar_t *var, float32 min, float32 max,
                            bool integral);
    virtual void Register(vmConvar_t *vmCvar, pointer varName,
                          pointer defaultValue, sint flags, pointer description);
    virtual void Update(vmConvar_t *vmCvar);
    static void CompleteCvarName(valueType *args, sint argNum);
    virtual void Init(void);
    virtual void Shutdown(void);
    static pointer Validate(convar_t *var, pointer value, bool warn);
};

extern idCVarSystemLocal cvarSystemLocal;

#endif // !__CVARSYSTEM_HPP__
