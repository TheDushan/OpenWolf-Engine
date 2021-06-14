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
// File name:   clientRenderer.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

idClientRendererSystemLocal clientRendererLocal;
idClientRendererSystemAPI *clientRendererSystem = &clientRendererLocal;

/*
===============
idClientRendererSystemLocal::idClientRendererSystemLocal
===============
*/
idClientRendererSystemLocal::idClientRendererSystemLocal(void) {
}

/*
===============
idClientRendererSystemLocal::~idClientRendererSystemLocal
===============
*/
idClientRendererSystemLocal::~idClientRendererSystemLocal(void) {
}

idRenderSystem *renderSystem;
idRenderSystem *(*rendererEntry)(rendererImports_t *imports);

void *rendererName;
static rendererImports_t exports;

/*
================
idClientRendererSystemLocal::RefPrintf

DLL glue
================
*/
void idClientRendererSystemLocal::RefPrintf(sint print_level, pointer fmt,
        ...) {
    va_list argptr;
    valueType msg[MAXPRINTMSG];

    va_start(argptr, fmt);
    Q_vsprintf_s(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);

    if(print_level == PRINT_ALL) {
        Com_Printf("%s", msg);
    } else if(print_level == PRINT_WARNING) {
        Com_Printf(S_COLOR_YELLOW "%s", msg);    // yellow
    }

    if(developer->integer) {
        if(print_level == PRINT_DEVELOPER) {
            Com_Printf(S_COLOR_RED "%s", msg);      // red
        }
    }
}

/*
============
idClientRendererSystemLocal::InitRenderer
============
*/
void idClientRendererSystemLocal::InitRenderer(void) {
    fileHandle_t f;

    // this sets up the renderer and calls R_Init
    renderSystem->Init(&cls.glconfig);

    // load character sets
    cls.charSetShader = renderSystem->RegisterShader("gfx/2d/bigchars");

    cls.useLegacyConsoleFont = true;

    // Register console font specified by cl_consoleFont, if any
    // filehandle is unused but forces fileSystem->FOpenFileRead() to heed purecheck because it does not when filehandle is nullptr
    if(cl_consoleFont->string[0]) {
        if(fileSystem->FOpenFileByMode(cl_consoleFont->string, &f, FS_READ) >= 0) {
            renderSystem->RegisterFont(cl_consoleFont->string,
                                       cl_consoleFontSize->integer, &cls.consoleFont);
            cls.useLegacyConsoleFont = false;
        }

        fileSystem->FCloseFile(f);
    }

    cls.whiteShader = renderSystem->RegisterShader("white");
    cls.consoleShader = renderSystem->RegisterShader("console");

    g_console_field_width = cls.glconfig.vidWidth / SMALLCHAR_WIDTH - 2;
    g_consoleField.widthInChars = g_console_field_width;

    ::srand(Com_Milliseconds());
}

/*
============
idClientRendererSystemLocal::RefMalloc
============
*/
void *idClientRendererSystemLocal::RefMalloc(sint size) {
    return memorySystem->TagMalloc(size, TAG_RENDERER);
}

/*
============
idClientRendererSystemLocal::RefTagFree
============
*/
void idClientRendererSystemLocal::RefTagFree(void) {
    memorySystem->FreeTags(TAG_RENDERER);
    return;
}

/*
============
idClientRendererSystemLocal::ScaledMilliseconds
============
*/
sint idClientRendererSystemLocal::ScaledMilliseconds(void) {
#ifdef NDEBUG
    return ++cl_faketime;
#else
    return idsystem->Milliseconds() * timescale->value;
#endif
}

/*
====================
idClientMainSystemLocal::InitExportTable
====================
*/
void idClientRendererSystemLocal::InitExportTable(void) {
    exports.Printf = Com_Printf;
    exports.Error = Com_Error;
    exports.collisionModelManager = collisionModelManager;
    exports.fileSystem = fileSystem;
    exports.cvarSystem = cvarSystem;
    exports.cmdBufferSystem = cmdBufferSystem;
    exports.cmdSystem = cmdSystem;
    exports.idsystem = idsystem;
    exports.clientAVISystem = clientAVISystem;
    exports.memorySystem = memorySystem;
    exports.clientAVISystem = clientAVISystem;
    exports.clientCinemaSystem = clientCinemaSystem;
    exports.clientRendererSystem = clientRendererSystem;
}

/*
============
idClientRendererSystemLocal::InitRef
============
*/ 
static void *rendererLib = nullptr;
void idClientRendererSystemLocal::InitRef(void) {
    valueType dllName[MAX_OSPATH];

    Com_Printf("----- idClientRendererSystemLocal::InitRef ----\n");

    ::snprintf(dllName, sizeof(dllName), "renderSystem." ARCH_STRING DLL_EXT);

    Com_Printf("Loading \"%s\"...\n", dllName);

    if((rendererLib = SDL_LoadObject(dllName)) == 0) {
        valueType fn[1024];

        Q_strncpyz(fn, idsystem->Cwd(), sizeof(fn));

        ::strncat(fn, "/", sizeof(fn) - (sint)Q_strlen(fn) - 1);
        ::strncat(fn, dllName, sizeof(fn) - (sint)Q_strlen(fn) - 1);

        Com_Printf("Loading \"%s\"...", fn);

        if((rendererLib = SDL_LoadObject(fn)) == 0) {
            Com_Error(ERR_FATAL, "failed:\n\"%s\"", SDL_GetError());
        }
    }

    // Get the entry point.
    rendererEntry = (idRenderSystem * (QDECL *)(rendererImports_t *))
                    idsystem->GetProcAddress(rendererLib, "rendererEntry");

    if(!rendererEntry) {
        Com_Error(ERR_FATAL, "rendererEntry on RenderSystem failed.\n");
    }

    // Init the export table.
    InitExportTable();

    renderSystem = rendererEntry(&exports);

    Com_Printf("-------------------------------\n");
    // unpause so the cgame definately gets a snapshot and renders a frame
    cvarSystem->Set("cl_paused", "0");
}

/*
============
idClientRendererSystemLocal::ShutdownRef
============
*/
void idClientRendererSystemLocal::ShutdownRef(void) {
    renderSystem->Shutdown(true);
    cls.rendererStarted = false;
}