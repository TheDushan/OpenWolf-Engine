////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2001 - 2006 Jacek Sieka, arnetheduck on gmail point com
// Copyright(C) 2020 - 2023 Dusan Jocic <dusanjocic@msn.com>
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
// -------------------------------------------------------------------------------------
// File name:   TigerHash.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __TIGERHASH_HPP__
#define __TIGERHASH_HPP__

//
// idTigerHashSystemLocal
//
class idTigerHashSystemLocal : public idTigerHashSystem {
public:
    idTigerHashSystemLocal();
    ~idTigerHashSystemLocal();

    virtual valueType *GetTigerHash(valueType *str);

    static void tiger_compress(uint64 *str, uint64 state[3]);
    static void tiger(uint64 *str, uint64 length, uint64 res[3]);

};

extern idTigerHashSystemLocal tigerHashSystemLocal;

#endif //!__TIGERHASH_HPP__