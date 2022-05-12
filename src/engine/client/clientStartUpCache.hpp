////////////////////////////////////////////////////////////////////////////////////////
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
// along with OpenWolf Source Code. If not, see <http://www.gnu.org/licenses/>.
//
// -------------------------------------------------------------------------------------
// File name:   clientStartUpCache_api.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTSTARTUPCACHE_HPP__
#define __CLIENTSTARTUPCACHE_HPP__

typedef struct {
    valueType            name[MAX_QPATH];
    sint             hits;
    sint             lastSetIndex;
} cacheItem_t;

enum cacheGroup_t {
    CACHE_SOUNDS,
    CACHE_MODELS,
    CACHE_IMAGES,

    CACHE_NUMGROUPS
};

static cacheItem_t cacheGroups[CACHE_NUMGROUPS] = {
    {{'s', 'o', 'u', 'n', 'd', 0}, CACHE_SOUNDS},
    {{'m', 'o', 'd', 'e', 'l', 0}, CACHE_MODELS},
    {{'i', 'm', 'a', 'g', 'e', 0}, CACHE_IMAGES},
};

#define MAX_CACHE_ITEMS     4096
#define CACHE_HIT_RATIO     0.75    // if hit on this percentage of maps, it'll get cached

static cacheItem_t cacheItems[CACHE_NUMGROUPS][MAX_CACHE_ITEMS];
static sint cacheIndex;

//
// idClientStartUpCacheSystemLocal
//
class idClientStartUpCacheSystemLocal {
public:
    idClientStartUpCacheSystemLocal();
    ~idClientStartUpCacheSystemLocal();

    static void Cache_StartGather_f(void);
    static void Cache_UsedFile_f(void);
    static void Cache_SetIndex_f(void);
    static void Cache_MapChange_f(void);
    static void Cache_EndGather_f(void);

};

extern idClientStartUpCacheSystemLocal clientStartUpCacheLocal;

#endif //__CLIENTSTARTUPCACHE_HPP__