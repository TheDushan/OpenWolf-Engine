////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2001 - 2006 Jacek Sieka, arnetheduck on gmail point com
// Copyright(C) 2020 - 2022 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   TigerHash.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <server/serverAutoPrecompiled.hpp>
#elif DEDICATED
#include <server/serverDedPrecompiled.hpp>
#else
#include <framework/precompiled.hpp>
#endif

idTigerHashSystemLocal tigerHashSystemLocal;
idTigerHashSystem *tigerHashSystem = &tigerHashSystemLocal;

/*
===============
idTigerHashSystemLocal::idTigerHashSystemLocal
===============
*/
idTigerHashSystemLocal::idTigerHashSystemLocal(void) {
}

/*
===============
idTigerHashSystemLocal::~idTigerHashSystemLocal
===============
*/
idTigerHashSystemLocal::~idTigerHashSystemLocal(void) {
}

/*
===============
idTigerHashSystemLocal::tiger_compress

The compress function is a function. Requires smaller cache?
===============
*/
void idTigerHashSystemLocal::tiger_compress(uint64 *str, uint64 state[3]) {
    tiger_compress_macro((reinterpret_cast<uint64 *>(str)),
                         (reinterpret_cast<uint64 *>(state)));
}

/*
===============
idTigerHashSystemLocal::tiger
===============
*/
void idTigerHashSystemLocal::tiger(uint64 *str, uint64 length,
                                   uint64 res[3]) {
    register uint64 i, j;
    uchar8 temp[64];

    res[0] = 0x0123456789ABCDEFLL;
    res[1] = 0xFEDCBA9876543210LL;
    res[2] = 0xF096A5B4C3B2E187LL;

    for(i = length; i >= 64; i -= 64) {
#ifdef BIG_ENDIAN

        for(j = 0; j < 64; j++) {
            temp[j ^ 7] = (reinterpret_cast<uchar8 *>(str))[j];
        }

        tiger_compress((reinterpret_cast<uint64 *>(temp)), res);
#else
        tiger_compress(str, res);
#endif
        str += 8;
    }

#ifdef BIG_ENDIAN

    for(j = 0; j < i; j++) {
        temp[j ^ 7] = (reinterpret_cast<uchar8 *>(str))[j];
    }

    temp[j ^ 7] = 0x01;
    j++;

    for(; j & 7; j++) {
        temp[j ^ 7] = 0;
    }

#else

    for(j = 0; j < i; j++) {
        temp[j] = (reinterpret_cast<uchar8 *>(str))[j];
    }

    temp[j++] = 0x01;

    for(; j & 7; j++) {
        temp[j] = 0;
    }

#endif

    if(j > 56) {
        for(; j < 64; j++) {
            temp[j] = 0;
        }

        tiger_compress((reinterpret_cast<uint64 *>(temp)), res);
        j = 0;
    }

    for(; j < 56; j++) {
        temp[j] = 0;
    }

    (reinterpret_cast<uint64 *>((&(temp[56]))))[0] = (static_cast<uint64>
            (length)) << 3;
    tiger_compress((reinterpret_cast<uint64 *>(temp)), res);
}

/*
===============
idTigerHashSystemLocal::GetTigerHash
===============
*/
valueType *idTigerHashSystemLocal::GetTigerHash(valueType *str) {
    static valueType cHash[49];
    uint64 res[3];

    tiger(reinterpret_cast<uint64 *>(str), ::strlen(str), res);

    sprintf(cHash, "%08X%08X%08X%08X%08X%08X",
            static_cast<uint32>(res[0] >> 32),
            static_cast<uint32>(res[0]),
            static_cast<uint32>(res[1] >> 32),
            static_cast<uint32>(res[1]),
            static_cast<uint32>(res[2] >> 32),
            static_cast<uint32>(res[2]));
    return cHash;
}

