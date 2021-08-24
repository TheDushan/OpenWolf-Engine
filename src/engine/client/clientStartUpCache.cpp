////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   clientStartUpCache.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

idClientStartUpCacheSystemLocal clientStartUpCacheLocal;

/*
===============
idClientStartUpCacheSystemLocal::idClientStartUpCacheSystemLocal
===============
*/
idClientStartUpCacheSystemLocal::idClientStartUpCacheSystemLocal(void) {
}

/*
===============
idClientStartUpCacheSystemLocal::~idClientStartUpCacheSystemLocal
===============
*/
idClientStartUpCacheSystemLocal::~idClientStartUpCacheSystemLocal(void) {
}

/*
===============
idClientStartUpCacheSystemLocal::Cache_StartGather_f
===============
*/
void idClientStartUpCacheSystemLocal::Cache_StartGather_f(void) {
    cacheIndex = 0;
    memset(cacheItems, 0, sizeof(cacheItems));

    cvarSystem->Set("cl_cacheGathering", "1");
}

/*
===============
idClientStartUpCacheSystemLocal::Cache_UsedFile_f
===============
*/
void idClientStartUpCacheSystemLocal::Cache_UsedFile_f(void) {
    valueType            groupStr[MAX_QPATH];
    valueType            itemStr[MAX_QPATH];
    sint             i, group;
    cacheItem_t    *item;

    if(cmdSystem->Argc() < 2) {
        common->Error(ERR_DROP, "usedfile without enough parameters\n");
        return;
    }

    Q_strcpy_s(groupStr, cmdSystem->Argv(1));

    Q_strcpy_s(itemStr, cmdSystem->Argv(2));

    for(i = 3; i < cmdSystem->Argc(); i++) {
        strcat(itemStr, " ");
        strcat(itemStr, cmdSystem->Argv(i));
    }

    Q_strlwr(itemStr);

    // find the cache group
    for(i = 0; i < CACHE_NUMGROUPS; i++) {
        if(!Q_strncmp(groupStr, cacheGroups[i].name, MAX_QPATH)) {
            break;
        }
    }

    if(i == CACHE_NUMGROUPS) {
        common->Error(ERR_DROP, "usedfile without a valid cache group\n");
        return;
    }

    // see if it's already there
    group = i;

    for(i = 0, item = cacheItems[group]; i < MAX_CACHE_ITEMS; i++, item++) {
        if(!item->name[0]) {
            // didn't find it, so add it here
            Q_strncpyz(item->name, itemStr, MAX_QPATH);

            if(cacheIndex > 9999) {
                // hack, but yeh
                item->hits = cacheIndex;
            } else {
                item->hits++;
            }

            item->lastSetIndex = cacheIndex;
            break;
        }

        if(item->name[0] == itemStr[0] &&
                !Q_strncmp(item->name, itemStr, MAX_QPATH)) {
            if(item->lastSetIndex != cacheIndex) {
                item->hits++;
                item->lastSetIndex = cacheIndex;
            }

            break;
        }
    }
}

/*
===============
idClientStartUpCacheSystemLocal::Cache_SetIndex_f
===============
*/
void idClientStartUpCacheSystemLocal::Cache_SetIndex_f(void) {
    if(cmdSystem->Argc() < 2) {
        common->Error(ERR_DROP, "setindex needs an index\n");
        return;
    }

    cacheIndex = atoi(cmdSystem->Argv(1));
}

/*
===============
idClientStartUpCacheSystemLocal::Cache_MapChange_f
===============
*/
void idClientStartUpCacheSystemLocal::Cache_MapChange_f(void) {
    cacheIndex++;
}

/*
===============
idClientStartUpCacheSystemLocal::Cache_EndGather_f
===============
*/
void idClientStartUpCacheSystemLocal::Cache_EndGather_f(void) {
    // save the frequently used files to the cache list file
    sint             i, j, handle, cachePass;
    valueType            filename[MAX_QPATH];

    cachePass = static_cast<sint>(floor(static_cast<float32>
                                        (cacheIndex) * CACHE_HIT_RATIO));

    for(i = 0; i < CACHE_NUMGROUPS; i++) {
        Q_strncpyz(filename, cacheGroups[i].name, MAX_QPATH);
        Q_strcat(filename, MAX_QPATH, ".cache");

        handle = fileSystem->FOpenFileWrite(filename);

        for(j = 0; j < MAX_CACHE_ITEMS; j++) {
            // if it's a valid filename, and it's been hit enough times, cache it
            if(cacheItems[i][j].hits >= cachePass &&
                    strstr(cacheItems[i][j].name, "/")) {
                fileSystem->Write(cacheItems[i][j].name, strlen(cacheItems[i][j].name),
                                  handle);
                fileSystem->Write("\n", 1, handle);
            }
        }

        fileSystem->FCloseFile(handle);
    }

    cvarSystem->Set("cl_cacheGathering", "0");
}
