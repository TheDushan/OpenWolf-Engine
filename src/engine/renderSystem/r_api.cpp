////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 - 2023 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   r_api.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.hpp>

rendererImports_t *imports;
idCollisionModelManager *collisionModelManager;
idFileSystem *fileSystem;
idCVarSystem *cvarSystem;
idCmdBufferSystem *cmdBufferSystem;
idCmdSystem *cmdSystem;
idSystem *idsystem;
idClientAVISystemAPI *clientAVISystem;
idMemorySystem *memorySystem;
idClientCinemaSystem *clientCinemaSystem;
idClientRendererSystemAPI *clientRendererSystem;
idCommon *common;

#if defined (__LINUX__) || defined (__MACOSX__)
extern "C" idRenderSystem *rendererEntry(rendererImports_t *renimports)
#else
Q_EXPORT idRenderSystem *rendererEntry(rendererImports_t *renimports)
#endif
{
    imports = renimports;

    collisionModelManager = imports->collisionModelManager;
    fileSystem = imports->fileSystem;
    cvarSystem = imports->cvarSystem;
    cmdBufferSystem = imports->cmdBufferSystem;
    cmdSystem = imports->cmdSystem;
    idsystem = imports->idsystem;
    memorySystem = imports->memorySystem;
    clientAVISystem = imports->clientAVISystem;
    clientCinemaSystem = imports->clientCinemaSystem;
    clientRendererSystem = imports->clientRendererSystem;
    common = imports->common;

    return renderSystem;
}
