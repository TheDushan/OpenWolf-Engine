////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2019 - 2023 Dusan Jocic <dusanjocic@msn.com>
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

#include <iostream>
#include <string>
#include <stdexcept>

int main(int argc, char **argv) {
    const std::string dynName;
    #if defined (_WIN32)
        dynName = "engine.AMD64.dll";
    #elif defined (__LINUX__)
        dynName = "./engine.x86_64.so";
    #else
        dynName = "./engine.x86_64.dylib";
    #endif
    const std::string engineExportName = "engineMain";

    SDL_Init(0);
    const auto libraryHandle = SDL_LoadObject(dynName.c_str());

    if(libraryHandle == nullptr) {
        throw std::runtime_error("Failed to load library: " + std::string(
                                     SDL_GetError()));
    }

    const auto engineMain = reinterpret_cast<int(*)(int, char **)>
                            (SDL_LoadFunction(libraryHandle, engineExportName.c_str()));

    if(engineMain == nullptr) {
        throw std::runtime_error("Failed to find function: " + std::string(
                                     SDL_GetError()));
    }

    engineMain(argc, argv);

    SDL_UnloadObject(libraryHandle);
    SDL_Quit();

    return 0;
}

