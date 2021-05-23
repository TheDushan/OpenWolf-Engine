////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 Andrei Drexler, Richard Allen, James Canete
// Copyright(C) 2011 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   r_extrememath.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: For mods that want to extend r_types.h without losing compatibility with original VMs
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_EXTRATYPES_H__
#define __R_EXTRATYPES_H__

// extra refdef flags start at 0x0008
#define RDF_NOFOG       0x0008      // don't apply fog to polys added using RE_AddPolyToScene
#define RDF_EXTRA       0x0010      // Makro - refdefex_t to follow after refdef_t
#define RDF_SUNLIGHT    0x0020      // SmileTheory - render sunlight and shadows

typedef struct {
    float32 blurFactor;
    float32 sunDir[3];
    float32 sunCol[3];
    float32 sunAmbCol[3];
} refdefex_t;

#endif //!__R_EXTRATYPES_H__
