////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2019 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   Memory.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
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

idMemorySystemLocal memorySystemLocal;
idMemorySystem *memorySystem = &memorySystemLocal;

/*
===============
idMemorySystemLocal::idMemorySystemLocal
===============
*/
idMemorySystemLocal::idMemorySystemLocal(void) {
}

/*
===============
idMemorySystemLocal::idMemorySystemLocal
===============
*/
idMemorySystemLocal::~idMemorySystemLocal(void) {
}

/*
========================
idMemorySystemLocal::ClearZone
========================
*/
void idMemorySystemLocal::ClearZone(memzone_t *zone, sint size) {
    memblock_t *block;

    // set the entire zone to one free block

    zone->blocklist.next = zone->blocklist.prev = block = (memblock_t *)(
                               reinterpret_cast<uchar8 *>(zone) + sizeof(memzone_t));
    zone->blocklist.tag = 1;    // in use block
    zone->blocklist.id = 0;
    zone->blocklist.size = 0;
    zone->rover = block;
    zone->size = size;
    zone->used = 0;

    block->prev = block->next = &zone->blocklist;
    block->tag = 0;             // free block
    block->id = ZONEID;
    block->size = size - sizeof(memzone_t);
}

/*
========================
idMemorySystemLocal::Free
========================
*/
void idMemorySystemLocal::Free(void *ptr) {
    memblock_t *block, * other;
    memzone_t *zone;

    if(!ptr) {
        common->Error(ERR_DROP, "idMemorySystemLocal::Free: nullptr pointer");
    }

    block = (memblock_t *)(reinterpret_cast<uchar8 *>(ptr) - sizeof(
                               memblock_t));

    if(block->id != ZONEID) {
        common->Error(ERR_FATAL,
                      "idMemorySystemLocal::Free: freed a pointer without ZONEID");
    }

    if(block->tag == 0) {
        common->Error(ERR_FATAL,
                      "idMemorySystemLocal::Free: freed a freed pointer");
    }

    // if static memory
    if(block->tag == TAG_STATIC) {
        return;
    }

    // check the memory trash tester
    if(*reinterpret_cast<sint *>((reinterpret_cast<uchar8 *>
                                  (block) + block->size - 4)) != ZONEID) {
        common->Error(ERR_FATAL,
                      "idMemorySystemLocal::Free: memory block wrote past end");
    }

    if(block->tag == TAG_SMALL) {
        zone = smallzone;
    } else {
        zone = mainzone;
    }

    zone->used -= block->size;
    // set the block to something that should cause problems
    // if it is referenced...
    ::memset(ptr, 0xaa, block->size - sizeof(*block));

    block->tag = 0;             // mark as free

    other = block->prev;

    if(!other->tag) {
        // merge with previous free block
        other->size += block->size;
        other->next = block->next;
        other->next->prev = other;

        if(block == zone->rover) {
            zone->rover = other;
        }

        block = other;
    }

    zone->rover = block;

    other = block->next;

    if(!other->tag) {
        // merge the next free block onto the end
        block->size += other->size;
        block->next = other->next;
        block->next->prev = block;

        if(other == zone->rover) {
            zone->rover = block;
        }
    }
}

/*
================
idMemorySystemLocal::FreeTags
================
*/
void idMemorySystemLocal::FreeTags(memtag_t tag) {
    sint count;
    memzone_t *zone;

    if(tag == TAG_SMALL) {
        zone = smallzone;
    } else {
        zone = mainzone;
    }

    count = 0;
    // use the rover as our pointer, because
    // memorySystem->Free automatically adjusts it
    zone->rover = zone->blocklist.next;

    do {
        if(zone->rover->tag == tag) {
            count++;
            Free(reinterpret_cast<memzone_t *>(zone->rover) + 1);
            continue;
        }

        zone->rover = zone->rover->next;
    } while(zone->rover != &zone->blocklist);
}

/*
================
idMemorySystemLocal::TagMalloc
================
*/
void *idMemorySystemLocal::TagMalloc(size_t size, memtag_t tag) {
    sint allocSize;
    sint64 extra;
    memblock_t *start, * rover, * _new, * base;
    memzone_t *zone;

    if(!tag) {
        common->Error(ERR_FATAL,
                      "idMemorySystemLocal::TagMalloc: tried to use a 0 tag");
    }

    if(tag == TAG_SMALL) {
        zone = smallzone;
    } else {
        zone = mainzone;
    }

    //
    // scan through the block list looking for the first free block
    // of sufficient size
    //
    size += sizeof(memblock_t); // account for size of block header
    size += 4;                  // space for memory trash tester
    size = PAD(size, sizeof(sint64));   // align to 32/64 bit boundary

    base = rover = zone->rover;
    start = base->prev;


    do {
        if(rover == start) {
            // scaned all the way around the list
            common->Error(ERR_FATAL,
                          "idMemorySystemLocal::TagMalloc: failed on allocation of %i bytes from the %s zone",
                          size, zone == smallzone ? "small" : "main");

            return nullptr;
        }

        if(rover->tag) {
            base = rover = rover->next;
        } else {
            rover = rover->next;
        }
    }     while(base->tag || base->size < size);

    //
    // found a block big enough
    //
    extra = base->size - size;

    if(extra > MINFRAGMENT) {
        // there will be a free fragment after the allocated block
        _new = (memblock_t *)(reinterpret_cast<uchar8 *>(base) + size);
        _new->size = extra;
        _new->tag = 0;          // free block
        _new->prev = base;
        _new->id = ZONEID;
        _new->next = base->next;
        _new->next->prev = _new;
        base->next = _new;
        base->size = size;
    }

    base->tag = tag;            // no longer a free block

    zone->rover = base->next;   // next allocation will start looking here
    zone->used += base->size;   //

    base->id = ZONEID;

    // marker for memory trash testing
    * reinterpret_cast<sint *>(reinterpret_cast<uchar8 *>
                               (base) + base->size - 4) = ZONEID;

    return reinterpret_cast<void *>(reinterpret_cast<uchar8 *>(base) + sizeof(
                                        memblock_t));
}

/*
========================
idMemorySystemLocal::Malloc
========================
*/
void *idMemorySystemLocal::Malloc(size_t size) {
    void *buf;

    buf = TagMalloc(size, TAG_GENERAL);
    ::memset(buf, 0, size);

    return buf;
}

/*
========================
idMemorySystemLocal::SMalloc
========================
*/
void *idMemorySystemLocal::SMalloc(size_t size) {
    return TagMalloc(size, TAG_SMALL);
}

/*
========================
idMemorySystemLocal::CheckHeap
========================
*/
void idMemorySystemLocal::CheckHeap(void) {
    memblock_t     *block;

    for(block = mainzone->blocklist.next;; block = block->next) {
        if(block->next == &mainzone->blocklist) {
            break;              // all blocks have been hit
        }

        if(reinterpret_cast<uchar8 *>(block) + block->size !=
                reinterpret_cast<uchar8 *>(block->next)) {
            common->Error(ERR_FATAL,
                          "idMemorySystemLocal::CheckHeap: block size does not touch the next block");
        }

        if(block->next->prev != block) {
            common->Error(ERR_FATAL,
                          "idMemorySystemLocal::CheckHeap: next block doesn't have proper back link");
        }

        if(!block->tag && !block->next->tag) {
            common->Error(ERR_FATAL,
                          "idMemorySystemLocal::CheckHeap: two consecutive free blocks");
        }
    }
}

/*
========================
idMemorySystemLocal::LogZoneHeap
========================
*/
void idMemorySystemLocal::LogZoneHeap(memzone_t *zone, valueType *name) {
    memblock_t *block;
    valueType            buf[4096];
    uint64 size, allocSize;
    sint numBlocks;

    if(!com_logfile || !fileSystem->Initialized()) {
        return;
    }

    size = numBlocks = 0;

    Q_vsprintf_s(buf, sizeof(buf), sizeof(buf),
                 "\r\n================\r\n%s log\r\n================\r\n", name);
    fileSystem->Write(buf, strlen(buf), com_logfile);

    for(block = zone->blocklist.next; block->next != &zone->blocklist;
            block = block->next) {
        if(block->tag) {
            size += block->size;
            numBlocks++;
        }
    }

    allocSize = numBlocks * sizeof(memblock_t); // + 32 bit alignment

    Q_vsprintf_s(buf, sizeof(buf), sizeof(buf),
                 "%d %s memory in %d blocks\r\n", size, name, numBlocks);
    fileSystem->Write(buf, strlen(buf), com_logfile);

    Q_vsprintf_s(buf, sizeof(buf), sizeof(buf), "%d %s memory overhead\r\n",
                 size - allocSize, name);
    fileSystem->Write(buf, strlen(buf), com_logfile);
}

/*
========================
idMemorySystemLocal::AvailableZoneMemory
========================
*/
sint idMemorySystemLocal::AvailableZoneMemory(const memzone_t *zone) {
    return zone->size - zone->used;
}

/*
========================
idMemorySystemLocal::LogHeap
========================
*/
void idMemorySystemLocal::LogHeap(void) {
    LogZoneHeap(mainzone, "MAIN");
    LogZoneHeap(smallzone, "SMALL");
}

/*
=================
idMemorySystemLocal::Meminfo_f
=================
*/
void idMemorySystemLocal::Meminfo_f(void) {
    memblock_t *block;
    uint64          zoneBytes, zoneBlocks;
    uint64          smallZoneBytes, smallZoneBlocks;
    uint64          botlibBytes, rendererBytes, otherBytes;
    uint64          staticBytes, generalBytes;

    zoneBytes = 0;
    botlibBytes = 0;
    rendererBytes = 0;
    otherBytes = 0;
    staticBytes = 0;
    generalBytes = 0;
    zoneBlocks = 0;

    for(block = mainzone->blocklist.next; ; block = block->next) {
        if(cmdSystem->Argc() != 1) {
            common->Printf("block:%p    size:%7i    tag:%3i\n",
                           block, block->size, block->tag);
        }

        if(block->tag) {
            zoneBytes += block->size;
            zoneBlocks++;

            if(block->tag == TAG_BOTLIB) {
                botlibBytes += block->size;
            } else if(block->tag == TAG_RENDERER) {
                rendererBytes += block->size;
            } else if(block->tag == TAG_STATIC) {
                staticBytes += block->size;
            } else if(block->tag == TAG_GENERAL) {
                generalBytes += block->size;
            } else {
                otherBytes += block->size;
            }
        }

        if(block->next == &mainzone->blocklist) {
            break;          // all blocks have been hit
        }

        if(reinterpret_cast<uchar8 *>(block) + block->size !=
                reinterpret_cast<uchar8 *>(block->next)) {
            common->Printf("ERROR: block size does not touch the next block\n");
        }

        if(block->next->prev != block) {
            common->Printf("ERROR: next block doesn't have proper back link\n");
        }

        if(!block->tag && !block->next->tag) {
            common->Printf("ERROR: two consecutive free blocks\n");
        }
    }

    smallZoneBytes = 0;
    smallZoneBlocks = 0;

    for(block = smallzone->blocklist.next; ; block = block->next) {
        if(block->tag) {
            smallZoneBytes += block->size;
            smallZoneBlocks++;
        }

        if(block->next == &smallzone->blocklist) {
            break;          // all blocks have been hit
        }
    }

    common->Printf("%8i K total hunk\n", s_hunk.memSize / 1024);
    common->Printf("%8i K total zone\n", s_zoneTotal / 1024);
    common->Printf("\n");

    common->Printf("%8i K used hunk (permanent)\n", s_hunk.permTop / 1024);
    common->Printf("%8i K used hunk (temp)\n", s_hunk.tempTop / 1024);
    common->Printf("%8i K used hunk (TOTAL)\n",
                   (s_hunk.permTop + s_hunk.tempTop) / 1024);
    common->Printf("\n");

    common->Printf("%8i K max hunk (permanent)\n", s_hunk.permMax / 1024);
    common->Printf("%8i K max hunk (temp)\n", s_hunk.tempMax / 1024);
    common->Printf("%8i K max hunk (TOTAL)\n",
                   (s_hunk.permMax + s_hunk.tempMax) / 1024);
    common->Printf("\n");

    common->Printf("%8i K max hunk since last memorySystem->Clear\n",
                   s_hunk.maxEver / 1024);
    common->Printf("%8i K hunk mem never touched\n",
                   (s_hunk.memSize - s_hunk.maxEver) / 1024);
    common->Printf("%8i hunk mark value\n", s_hunk.mark);
    common->Printf("\n");

    common->Printf("\n");
    common->Printf("%8i bytes in %i zone blocks\n", zoneBytes, zoneBlocks);
    common->Printf("        %8i bytes in dynamic botlib\n", botlibBytes);
    common->Printf("        %8i bytes in dynamic renderer\n", rendererBytes);
    common->Printf("        %8i bytes in dynamic other\n", otherBytes);
    common->Printf("        %8i bytes in small Zone memory\n", smallZoneBytes);
    common->Printf("        %8i bytes in static server memory\n", staticBytes);
    common->Printf("        %8i bytes in general common memory\n",
                   generalBytes);
}

/*
===============
idMemorySystemLocal::TouchMemory

Touch all known used data to make sure it is paged in
===============
*/
void idMemorySystemLocal::TouchMemory(void) {
    sint        start, end;
    uint64      i, j;
    uint64      sum;
    memblock_t *block;

    CheckHeap();

    start = idsystem->Milliseconds();

    sum = 0;

    for(block = mainzone->blocklist.next; ; block = block->next) {
        if(block->tag) {
            j = block->size >> 2;

            for(i = 0; i < j; i += 64) {                // only need to touch each page
                sum += (reinterpret_cast<sint *>(block))[i];
            }
        }

        if(block->next == &mainzone->blocklist) {
            break;          // all blocks have been hit
        }
    }

    end = idsystem->Milliseconds();

    common->Printf("idMemorySystemLocal::TouchMemory: %i msec\n", end - start);
}

/*
=================
idMemorySystemLocal::InitSmallZoneMemory
=================
*/
void idMemorySystemLocal::InitSmallZoneMemory(void) {
    s_smallZoneTotal = 512 * 1024;

    // bk001205 - was malloc
    smallzone = (memzone_t *)calloc(s_smallZoneTotal, 1);

    if(!smallzone) {
        common->Error(ERR_FATAL, "Small zone data failed to allocate %1.1f megs",
                      static_cast<float32>(s_smallZoneTotal) / (1024 * 1024));
    }

    ClearZone(smallzone, s_smallZoneTotal);

    return;
}

/*
=================
idMemorySystemLocal::InitZoneMemory
=================
*/
void idMemorySystemLocal::InitZoneMemory(void) {
    convar_t *cv;

    // allocate the random block zone
    common->StartupVariable("com_zoneMegs"); // config files and command line options haven't been taken in account yet
    cv = cvarSystem->Get("com_zoneMegs", DEF_COMZONEMEGS_S, CVAR_INIT,
                         "Sets the amount of memory reserved for the game.");

    if(cv->integer < DEF_COMZONEMEGS) {
        s_zoneTotal = 1024 * 1024 * DEF_COMZONEMEGS;
    } else {
        s_zoneTotal = cv->integer * 1024 * 1024;
    }

    // bk001205 - was malloc
    mainzone = (memzone_t *)calloc(s_zoneTotal, 1);

    if(!mainzone) {
        common->Error(ERR_FATAL, "Zone data failed to allocate %i megs",
                      s_zoneTotal / (1024 * 1024));
    }

    ClearZone(mainzone, s_zoneTotal);

}

/*
=================
idMemorySystemLocal::InitHunkMemory
=================
*/
void idMemorySystemLocal::InitHunkMemory(void) {
    convar_t *cv;
    sint nMinAlloc;
    valueType *pMsg = nullptr;

    ::memset(&s_hunk, 0, sizeof(s_hunk));

    // make sure the file system has allocated and "not" freed any temp blocks
    // this allows the config and product id files ( journal files too ) to be loaded
    // by the file system without redunant routines in the file system utilizing different
    // memory systems
    if(fileSystem->LoadStack() != 0) {
        common->Error(ERR_FATAL,
                      "Hunk initialization failed. File system load stack not zero");
    }

    // allocate the stack based hunk allocator
    cv = cvarSystem->Get("com_hunkMegs", DEF_COMHUNKMEGS_S,
                         CVAR_LATCH | CVAR_ARCHIVE,
                         "Sets the amount of memory reserved for the game, including com_soundMegs and com_zoneMeg");

    // if we are not dedicated min allocation is 56, otherwise min is 1
    if(dedicated && dedicated->integer) {
        nMinAlloc = MIN_DEDICATED_COMHUNKMEGS;
        pMsg = "Minimum com_hunkMegs for a dedicated server is %i, allocating %i megs.\n";
    } else {
        nMinAlloc = MIN_COMHUNKMEGS;
        pMsg = "Minimum com_hunkMegs is %i, allocating %i megs.\n";
    }

    if(cv->integer < nMinAlloc) {
        s_hunk.memSize = 1024 * 1024 * nMinAlloc;
        common->Printf(pMsg, nMinAlloc, s_hunk.memSize / (1024 * 1024));
    } else {
        s_hunk.memSize = cv->integer * 1024 * 1024;
    }

    // bk001205 - was malloc
    s_hunk.originalRaw = static_cast<uchar8 *>(calloc(s_hunk.memSize + 63, 1));

    if(!s_hunk.originalRaw) {
        common->Error(ERR_FATAL, "Hunk data failed to allocate %i megs",
                      s_hunk.memSize / (1024 * 1024));
    }

    // cacheline align
    s_hunk.mem = reinterpret_cast<uchar8 *>(((sint64)s_hunk.originalRaw + 63) &
                                            ~63);

    Clear();

    cmdSystem->AddCommand("meminfo", &idMemorySystemLocal::Meminfo_f,
                          "Shows memory usage in the console");
}

/*
====================
idMemorySystemLocal::MemoryRemaining
====================
*/
uint64 idMemorySystemLocal::MemoryRemaining(void) {
    return s_hunk.memSize - s_hunk.permTop - s_hunk.tempTop;
}

/*
===================
idMemorySystemLocal::SetMark

The server calls this after the level and game VM have been loaded
===================
*/
void idMemorySystemLocal::SetMark(void) {
    s_hunk.mark = s_hunk.permTop;
}

/*
=================
idMemorySystemLocal::ClearToMark

The client calls this before starting a vid_restart or snd_restart
=================
*/
void idMemorySystemLocal::ClearToMark(void) {
    s_hunk.permTop = s_hunk.mark;
    s_hunk.permMax = s_hunk.permTop;

    s_hunk.tempMax = s_hunk.tempTop = 0;
}

/*
=================
idMemorySystemLocal::CheckMark

Used before for bots
=================
*/
bool idMemorySystemLocal::CheckMark(void) {
    return s_hunk.mark ? true : false;
}

/*
=================
idMemorySystemLocal::Clear

The server calls this before shutting down or loading a new map
=================
*/
void idMemorySystemLocal::Clear(void) {
#ifndef DEDICATED
    clientGameSystem->ShutdownCGame();
    clientGUISystem->ShutdownGUI();
#endif

    serverGameSystem->ShutdownGameProgs();

#ifndef DEDICATED
    clientCinemaSystem->CloseAllVideos();
#endif

    s_hunk.permTop = 0;
    s_hunk.permMax = 0;
    s_hunk.tempTop = 0;
    s_hunk.tempMax = 0;
    s_hunk.maxEver = 0;
    s_hunk.mark = 0;

    common->Printf("idMemorySystemLocal::Clear: reset the hunk ok\n");

    //stake out a chunk for the frame temp data
    FrameInit();
}

/*
=================
idMemorySystemLocal::Alloc

Allocate permanent (until the hunk is cleared) memory
=================
*/
void *idMemorySystemLocal::Alloc(size_t size, ha_pref preference) {
    void *buf;

    if(s_hunk.mem == nullptr) {
        common->Error(ERR_FATAL,
                      "idMemorySystemLocal::Alloc: Hunk memory system not initialized");
    }

    // round to cacheline
    size = (size + 63) & ~63;

    if(s_hunk.permTop + s_hunk.tempTop + size > s_hunk.memSize) {
        common->Error(ERR_DROP, "idMemorySystemLocal::Alloc failed on %i", size);
    }

    buf = s_hunk.mem + s_hunk.permTop;
    s_hunk.permTop += size;

    if(s_hunk.permTop > s_hunk.permMax) {
        s_hunk.permMax = s_hunk.permTop;
    }

    if(s_hunk.permTop + s_hunk.tempTop > s_hunk.maxEver) {
        s_hunk.maxEver = s_hunk.permTop + s_hunk.tempTop;
    }

    ::memset(buf, 0, size);

    return buf;
}

/*
=================
idMemorySystemLocal::AllocateTempMemory

This is used by the file loading system.
Multiple files can be loaded in temporary memory.
When the files-in-use count reaches zero, all temp memory will be deleted
=================
*/
void *idMemorySystemLocal::AllocateTempMemory(size_t size) {
    void *buf;
    hunkHeader_t *hdr;

    // return a memorySystem->Malloc'd block if the hunk has not been initialized
    // this allows the config and product id files ( journal files too ) to be loaded
    // by the file system without redunant routines in the file system utilizing different
    // memory systems
    if(s_hunk.mem == nullptr) {
        return Malloc(size);
    }

    size = PAD(size, sizeof(sint64)) + sizeof(hunkHeader_t);

    if(s_hunk.permTop + s_hunk.tempTop + size > s_hunk.memSize) {
        common->Error(ERR_DROP,
                      "idMemorySystemLocal::AllocateTempMemory: failed on %i", size);
    }

    s_hunk.tempTop += size;
    buf = s_hunk.mem + s_hunk.memSize - s_hunk.tempTop;

    if(s_hunk.tempTop > s_hunk.tempMax) {
        s_hunk.tempMax = s_hunk.tempTop;
    }

    if(s_hunk.permTop + s_hunk.tempTop > s_hunk.maxEver) {
        s_hunk.maxEver = s_hunk.permTop + s_hunk.tempTop;
    }

    hdr = (hunkHeader_t *)buf;
    buf = reinterpret_cast<void *>(hdr + 1);

    hdr->magic = HUNK_MAGIC;
    hdr->size = size;

    // don't bother clearing, because we are going to load a file over it
    return buf;
}

/*
==================
idMemorySystemLocal::FreeTempMemory
==================
*/
void idMemorySystemLocal::FreeTempMemory(void *buf) {
    hunkHeader_t *hdr;

    // free with memorySystem->Free if the hunk has not been initialized
    // this allows the config and product id files ( journal files too ) to be loaded
    // by the file system without redunant routines in the file system utilizing different
    // memory systems
    if(!s_hunk.mem) {
        Free(buf);
        return;
    }


    hdr = (hunkHeader_t *)buf - 1;

    if(hdr->magic != HUNK_MAGIC) {
        common->Error(ERR_FATAL, "memorySystem->FreeTempMemory: bad magic");
    }

    hdr->magic = HUNK_FREE_MAGIC;

    // this only works if the files are freed in stack order,
    // otherwise the memory will stay around until Hunk_ClearTempMemory
    if(reinterpret_cast<uchar8 *>(hdr) == s_hunk.mem + s_hunk.memSize -
            s_hunk.tempTop) {
        s_hunk.tempTop -= hdr->size;
    }
}

/*
=================
idMemorySystemLocal::ClearTempMemory

The temp space is no longer needed.  If we have left more
touched but unused memory on this side, have future
permanent allocs use this side.
=================
*/
void idMemorySystemLocal::ClearTempMemory(void) {
    if(s_hunk.mem) {
        s_hunk.tempTop = 0;
        s_hunk.tempMax = 0;
    }
}

static uchar8 *s_frameStackLoc = 0;
static uchar8 *s_frameStackBase = 0;
static uchar8 *s_frameStackEnd = 0;

/*
=================
idMemorySystemLocal::FrameInit
=================
*/
void idMemorySystemLocal::FrameInit(void) {
    sint megs = cvarSystem->Get("com_hunkFrameMegs", "1",
                                CVAR_LATCH | CVAR_ARCHIVE,
                                "Sets the amount of memory reserved for the game")->integer;
    uint32 cb;

    if(megs < 1) {
        megs = 1;
    }

    cb = 1024 * 1024 * megs;

    s_frameStackBase = static_cast<uchar8 *>(memorySystemLocal.Alloc(cb,
                       h_low));
    s_frameStackEnd = s_frameStackBase + cb;

    s_frameStackLoc = s_frameStackBase;
}

/*
========================
idMemorySystemLocal::CopyString

never write over the memory memorySystem->CopyString returns because
memory from a memstatic_t might be returned
========================
*/
valueType *idMemorySystemLocal::CopyString(pointer in) {
    valueType *out;

    if(!in[0]) {
        return (reinterpret_cast<valueType *>(&emptystring)) + sizeof(memblock_t);
    } else if(!in[1]) {
        if(in[0] >= '0' && in[0] <= '9') {
            return (reinterpret_cast<valueType *>(&numberstring[in[0] - '0'])) +
                   sizeof(memblock_t);
        }
    }

    out = reinterpret_cast<valueType *>(SMalloc(::strlen(in) + 1));
    ::strcpy(out, in);
    return out;
}

/*
========================
idMemorySystemLocal::Shutdown
========================
*/
void idMemorySystemLocal::GetHunkInfo(sint *hunkused, sint *hunkexpected) {
    *hunkused = com_hunkusedvalue;
    *hunkexpected = com_expectedhunkusage;
}

/*
========================
idMemorySystemLocal::ReleaseMemory
========================
*/
void idMemorySystemLocal::ReleaseMemory(void) {
    if(s_hunk.original) {
        free(s_hunk.original);
    }

    if(s_hunk.originalRaw) {
        free(s_hunk.originalRaw);
    }

    ::memset(&s_hunk, 0, sizeof(s_hunk));

    if(smallzone) {
        free(smallzone);
        smallzone = 0;
    }

    if(mainzone) {
        free(mainzone);
        mainzone = 0;
    }
}

/*
========================
idMemorySystemLocal::ReleaseMemory
========================
*/
void idMemorySystemLocal::FrameReset(void) {
    s_frameStackLoc = s_frameStackBase;
}
