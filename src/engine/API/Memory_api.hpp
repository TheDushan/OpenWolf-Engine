////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2019 - 2023 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   Memory_api.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __MEMORY_API_HPP__
#define __MEMORY_API_HPP__

typedef enum {
    TAG_FREE,
    TAG_GENERAL,
    TAG_BOTLIB,
    TAG_RENDERER,
    TAG_SMALL,
    TAG_STATIC
} memtag_t;

//
// idMemorySystem
//
class idMemorySystem {
public:
    virtual void Free(void *ptr) = 0;
    virtual void FreeTags(memtag_t tag) = 0;
    virtual void *Malloc(size_t size) = 0;
    virtual void *TagMalloc(size_t size, memtag_t tag) = 0;
    virtual void *SMalloc(size_t size) = 0;
    virtual void CheckHeap(void) = 0;
    virtual bool CheckMark(void) = 0;
    virtual void TouchMemory(void) = 0;
    virtual void InitSmallZoneMemory(void) = 0;
    virtual void InitZoneMemory(void) = 0;
    virtual void InitHunkMemory(void) = 0;
    virtual uint64 MemoryRemaining(void) = 0;
    virtual void SetMark(void) = 0;
    virtual void ClearToMark(void) = 0;
    virtual void Clear(void) = 0;
    virtual void *Alloc(size_t size, ha_pref preference) = 0;
    virtual void *AllocateTempMemory(size_t size) = 0;
    virtual void FreeTempMemory(void *buf) = 0;
    virtual void ClearTempMemory(void) = 0;
    virtual valueType *CopyString(pointer in) = 0;
    virtual void GetHunkInfo(sint *hunkused, sint *hunkexpected) = 0;
    virtual void ReleaseMemory(void) = 0;
    virtual void FrameReset(void) = 0;
};

extern idMemorySystem *memorySystem;

#endif //!__MEMORY_API_HPP__
