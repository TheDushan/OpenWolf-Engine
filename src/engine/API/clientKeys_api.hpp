////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2021 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of OpenWolf.
//
// OpenWolf is free software; you can redistribute it
// and / or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the License,
// or (at your option) any later version.
//
// OpenWolf is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenWolf; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110 - 1301  USA
//
// -------------------------------------------------------------------------------------
// File name:   clientKeys_api.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTKEYS_API_HPP__
#define __CLIENTKEYS_API_HPP__

typedef struct {
    bool down;
    sint repeats; // if > 1, it is autorepeating
    valueType *binding;
    sint hash;
} qkey_t;

//
// idClientKeysSystem
//
class idClientKeysSystem {
public:
    virtual bool GetOverstrikeMode(void) = 0;
    virtual void SetOverstrikeMode(bool state) = 0;
    virtual bool IsDown(sint keynum) = 0;
    virtual sint StringToKeynum(pointer str) = 0;
    virtual void SetBinding(sint keynum, pointer binding) = 0;
    virtual valueType *GetBinding(sint keynum) = 0;
    virtual sint GetKey(pointer binding) = 0;
    virtual void GetBindingByString(pointer binding, sint *key1,
                                    sint *key2) = 0;
    virtual void InitKeyCommands(void) = 0;
    virtual void KeyEvent(sint key, sint down, sint time) = 0;
    virtual void CharEvent(sint key) = 0;
    virtual void WriteBindings(fileHandle_t f) = 0;
    virtual valueType *KeynumToString(sint keynum) = 0;
    virtual void KeynameCompletion(void(*callback)(pointer s)) = 0;
    virtual void ClearStates(void) = 0;
    virtual sint GetCatcher(void) = 0;
    virtual void SetCatcher(sint catcher) = 0;
};

extern idClientKeysSystem *clientKeysSystem;

#endif // !__CLIENTKEYS_API_HPP__
