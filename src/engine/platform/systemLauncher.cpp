////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2019 - 2025 Dusan Jocic <dusanjocic@msn.com>
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
// Version:     v1.02
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

#include <iostream>
#include <string>
#include <memory>

#ifdef _WIN32
constexpr const char* ENGINE_LIB = "engine.AMD64.dll";
#elif defined(__LINUX__)
constexpr const char* ENGINE_LIB = "./engine.x86_64.so";
#else
constexpr const char* ENGINE_LIB = "./engine.x86_64.dylib";
#endif

constexpr const char* ENGINE_EXPORT_FUNC = "engineMain";

typedef int (*EngineMainFunc)(int, char**);

/*
=================
LoadEngine
=================
*/
bool LoadEngine(std::unique_ptr<void, decltype(&SDL_UnloadObject)>& libraryHandle, EngineMainFunc& engineMain) {
    libraryHandle.reset(SDL_LoadObject(ENGINE_LIB));

    if (!libraryHandle) {
        std::cerr << "[ERROR] Failed to load library: " << SDL_GetError() << std::endl;
        return false;
    }

    engineMain = reinterpret_cast<EngineMainFunc>(SDL_LoadFunction(libraryHandle.get(), ENGINE_EXPORT_FUNC));

    if (!engineMain) {
        std::cerr << "[ERROR] Failed to find function: " << SDL_GetError() << std::endl;
        return false;
    }

    return true;
}

/*
=================
main
=================
*/
int main(int argc, char** argv) {
    if (SDL_Init(0) != 0) {
        std::cerr << "[ERROR] SDL initialization failed: " << SDL_GetError() << std::endl;
        return EXIT_FAILURE;
    }

    std::unique_ptr<void, decltype(&SDL_UnloadObject)> libraryHandle(nullptr, SDL_UnloadObject);
    EngineMainFunc engineMain = nullptr;

    if (!LoadEngine(libraryHandle, engineMain)) {
        std::cerr << "[ERROR] Engine failed to load. Exiting..." << std::endl;
        SDL_Quit();
        return EXIT_FAILURE;
    }

    engineMain(argc, argv);

    SDL_Quit();
    return EXIT_SUCCESS;
}
