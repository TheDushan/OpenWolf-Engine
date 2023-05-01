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
// File name:   Memory.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __MEMORY_HPP__
#define __MEMORY_HPP__

#define MIN_DEDICATED_COMHUNKMEGS 64
#define MIN_COMHUNKMEGS 64              // JPW NERVE changed this to 42 for MP, was 56 for team arena and 75 for wolfSP
#define DEF_COMHUNKMEGS 512         // RF, increased this, some maps are exceeding 56mb 
// JPW NERVE changed this for multiplayer back to 42, 56 for depot/mp_cpdepot, 42 for everything else
#define DEF_COMZONEMEGS 64              // RF, increased this from 16, to account for botlib/AAS
#define DEF_COMHUNKMEGS_S   XSTRING(DEF_COMHUNKMEGS)
#define DEF_COMZONEMEGS_S   XSTRING(DEF_COMZONEMEGS)

/*
==============================================================================
ZONE MEMORY ALLOCATION

There is never any space between memblocks, and there will never be two
contiguous free memblocks.

The rover can be left pointing at a non-empty block

The zone calls are pretty much only used for small strings and structures,
all big things are allocated on the hunk.
==============================================================================
*/

#define ZONEID  0x1d4a11
#define MINFRAGMENT 64

typedef struct zonedebug_s {
    valueType *label;
    valueType *file;
    sint line;
    uint64 allocSize;
} zonedebug_t;

typedef struct memblock_s {
    uint64 size;        // including the header and possibly tiny fragments
    sint tag;       // a tag of 0 is a free block
    struct memblock_s *next, * prev;
    sint id;            // should be ZONEID
#ifdef ZONE_DEBUG
    zonedebug_t     d;
#endif
} memblock_t;

typedef struct {
    uint64 size;        // total bytes malloced, including header
    uint64 used;        // total bytes used
    memblock_t      blocklist;  // start / end cap for linked list
    memblock_t *rover;
} memzone_t;

// main zone for all "dynamic" memory allocation
static memzone_t *mainzone;

// we also have a small zone for small allocations that would only
// fragment the main zone (think of cvar and cmd strings)
static memzone_t *smallzone;

// static mem blocks to reduce a lot of small zone overhead
typedef struct memstatic_s {
    memblock_t      b;
    uchar8            mem[2];
} memstatic_t;

// bk001204 - initializer brackets
static memstatic_t emptystring = { {(sizeof(memblock_t) + 2 + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}
    , {'\0', '\0'}
};

static memstatic_t numberstring[] = {
    {   {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}
        , {'0', '\0'}
    }
    ,
    {   {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}
        , {'1', '\0'}
    }
    ,
    {   {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}
        , {'2', '\0'}
    }
    ,
    {   {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}
        , {'3', '\0'}
    }
    ,
    {   {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}
        , {'4', '\0'}
    }
    ,
    {   {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}
        , {'5', '\0'}
    }
    ,
    {   {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}
        , {'6', '\0'}
    }
    ,
    {   {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}
        , {'7', '\0'}
    }
    ,
    {   {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}
        , {'8', '\0'}
    }
    ,
    {   {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}
        , {'9', '\0'}
    }
};

/*
==============================================================================

Goals:
    reproducable without history effects -- no out of memory errors on weird map to map changes
    allow restarting of the client without fragmentation
    minimize total pages in use at run time
    minimize total pages needed during load time

  Single block of memory with stack allocators coming from both ends towards the middle.

  One side is designated the temporary memory allocator.

  Temporary memory can be allocated and freed in any order.

  A highwater mark is kept of the most in use at any time.

  When there is no temporary memory allocated, the permanent and temp sides
  can be switched, allowing the already touched temp memory to be used for
  permanent storage.

  Temp memory must never be allocated on two ends at once, or fragmentation
  could occur.

  If we have any in-use temp memory, additional temp allocations must come from
  that side.

  If not, we can choose to make either side the new temp side and push future
  permanent allocations to the other side.  Permanent allocations should be
  kept on the side that has the current greatest wasted highwater mark.

==============================================================================
*/

#define HUNK_MAGIC  0x89537892
#define HUNK_FREE_MAGIC 0x89537893

typedef struct {
    sint             magic;
    uint64             size;
} hunkHeader_t;

typedef struct {
    uint64             mark;
    uint64             permanent;
    uint64             temp;
    uint64             tempHighwater;
} hunkUsed_t;

typedef struct hunkblock_s {
    uint64             size;
    uchar8            printed;
    struct hunkblock_s *next;
    valueType *label;
    valueType *file;
    sint             line;
} hunkblock_t;

static struct {
    hunkblock_t *blocks;

    uchar8 *mem, * original, * originalRaw;
    uint64  memSize;

    uint64  permTop, permMax;
    uint64  tempTop, tempMax;

    uint64  maxEver;

    uint64  mark;
} s_hunk;

static hunkblock_t *hunkblocks;

static hunkUsed_t hunk_low, hunk_high;
static hunkUsed_t *hunk_permanent, * hunk_temp;

static uint64 s_zoneTotal;
static uint64 s_smallZoneTotal;

extern fileHandle_t logfile_;

//
// idMemorySystemLocal
//
class idMemorySystemLocal : public idMemorySystem {
public:
    idMemorySystemLocal(void);
    ~idMemorySystemLocal(void);

    virtual void Free(void *ptr);
    virtual void FreeTags(memtag_t tag);
    virtual void *Malloc(size_t size);
    virtual void *TagMalloc(size_t size, memtag_t tag);
    virtual void *SMalloc(size_t size);
    virtual void CheckHeap(void) ;
    virtual bool CheckMark(void);
    virtual void TouchMemory(void);
    virtual void InitSmallZoneMemory(void);
    virtual void InitZoneMemory(void);
    virtual void InitHunkMemory(void);
    virtual uint64 MemoryRemaining(void);
    virtual void SetMark(void);
    virtual void ClearToMark(void);
    virtual void Clear(void);
    virtual void *Alloc(size_t size, ha_pref preference);
    virtual void *AllocateTempMemory(size_t size);
    virtual void FreeTempMemory(void *buf);
    virtual void ClearTempMemory(void);
    virtual valueType *CopyString(pointer in);
    virtual void GetHunkInfo(sint *hunkused, sint *hunkexpected);
    virtual void ReleaseMemory(void);
    virtual void FrameReset(void);

    static void ClearZone(memzone_t *zone, sint size);
    static void LogZoneHeap(memzone_t *zone, valueType *name);
    static sint AvailableZoneMemory(const memzone_t *zone);
    static void LogHeap(void);
    static void Meminfo_f(void);
    static void FrameInit(void);
};

extern idMemorySystemLocal memorySystemLocal;

#endif //!__MEMORY_HPP__