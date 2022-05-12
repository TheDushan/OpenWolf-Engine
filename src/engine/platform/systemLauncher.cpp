////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2019 - 2022 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   systemLauncher.cpp
// Version:     v1.01
// Created:     11/06/2019
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#include <SDL.h>
#include <SDL_loadso.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_loadso.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    const char *symname = nullptr;
    char *engineExportName = "engineMain";
    char *spawnname = nullptr;
    char *dynName = nullptr;
#ifdef _WIN32
    int(__cdecl * engineMain)(int argc, char **argv);
#else
    int (*engineMain)(int argc, char **argv);
#endif

    /* Initialize SDL */
    if(SDL_Init(0) < 0) {
        printf("Couldn't initialize SDL: %s\n", SDL_GetError());
        return 0;
    }

#if defined (_WIN32)
    dynName = "engine.AMD64.dll";
#elif defined (__LINUX__)
    dynName = "./engine.AMD64.so";
#else
    dynName = "./engine.x86_64.dylib";
#endif

    void *libraryName = SDL_LoadObject(dynName);

    printf("libraryHandle config: library(%s) spawn(%s)\n",
           dynName ? dynName : "--", engineExportName ? engineExportName : "--");
    printf("Loading libraryHandle: %s\n",
           libraryName ? "loaded!" : SDL_GetError());

    if(libraryName) {
#ifdef _WIN32
        engineMain = (int(__cdecl *)(int, char **))SDL_LoadFunction(libraryName,
                     engineExportName);
#else
        engineMain = (int (*)(int, char **)) SDL_LoadFunction(libraryName,
                     engineExportName);
#endif

        if(engineMain != nullptr) {
            engineMain(argc, argv);
        } else {
            printf("Cannot read engine export name\n");
        }
    }

    SDL_UnloadObject(libraryName);
    SDL_Quit();

    return 0;
}
