////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2021 - 2022 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   clientGUID.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

idClientGUIDSystemLocal clientGUIDLocal;

/*
===============
idClientGUIDSystemLocal::idClientGUIDSystemLocal
===============
*/
idClientGUIDSystemLocal::idClientGUIDSystemLocal(void) {
}

/*
===============
idClientGUIDSystemLocal::~idClientGUIDSystemLocal
===============
*/
idClientGUIDSystemLocal::~idClientGUIDSystemLocal(void) {
}

/*
====================
idClientGUIDSystemLocal::UpdateGUID

update cl_guid using ETKEY_FILE and optional prefix
====================
*/
void idClientGUIDSystemLocal::UpdateGUID(pointer prefix,
        uint64 prefix_len) {
    sint len;
    fileHandle_t f;

    len = fileSystem->SV_FOpenFileRead(GUIDKEY_FILE, &f);
    fileSystem->FCloseFile(f);

    if(len != GUIDKEY_SIZE) {
        cvarSystem->Set("cl_guid", "");
    } else {
        cvarSystem->Set("cl_guid", MD5System->MD5File(GUIDKEY_FILE, GUIDKEY_SIZE,
                        prefix, prefix_len));
    }
}

/*
===============
idClientGUIDSystemLocalGenerateGUIDKey
===============
*/
void idClientGUIDSystemLocal::GenerateGUIDKey(void) {
    sint len = 0;
    uchar8   buff[GUIDKEY_SIZE];
    fileHandle_t    f;

    len = fileSystem->SV_FOpenFileRead(GUIDKEY_FILE, &f);
    fileSystem->FCloseFile(f);

    if(len == GUIDKEY_SIZE) {
        common->Printf("GUIDKEY found.\n");
        return;
    } else {
        if(len > 0) {
            common->Printf("GUIDKEY file size != %d, regenerating\n", GUIDKEY_SIZE);
        }

        common->Printf("GUIDKEY building random string\n");
        common->RandomBytes(buff, sizeof(buff));

        f = fileSystem->SV_FOpenFileWrite(GUIDKEY_FILE);

        if(!f) {
            common->Printf("GUIDKEY could not open %s for write\n", GUIDKEY_FILE);
            return;
        }

        fileSystem->Write(buff, sizeof(buff), f);
        fileSystem->FCloseFile(f);
        common->Printf("GUIDKEY generated\n");
    }
}
