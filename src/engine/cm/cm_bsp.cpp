////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2021 - 2023 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of OpenWolf.
//
// OpenWolf is free software; you can redistribute it
// and / or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3 of the License,
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
// File name:   cm_bsp.cpp
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

static sint ibsp[] = { Q3_BSP_VERSION, WOLF_BSP_VERSION };

/*
=================
idCollisionModelManagerLocal::IsBSPSupported
=================
*/
bool idCollisionModelManagerLocal::IsBSPSupported(const sint version,
        const bool dropError) {
    sint i;
    bool bspOK = false;

    for(i = 0 ; i < ARRAY_LEN(ibsp) ; i++) {
        if(ibsp[i] == version) {
            bspOK = true;
            break;
        }
    }

    if(!bspOK && dropError) {
        common->Error(ERR_DROP, "ERROR: Unsupported BSP version (%i)", version);
    } else {
        return bspOK;
    }
}
