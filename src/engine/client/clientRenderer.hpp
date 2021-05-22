////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   clientRenderer.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTRENDERER_HPP__
#define __CLIENTRENDERER_HPP__

static sint cl_faketime = 0;

//
// idClientRendererSystemLocal
//
class idClientRendererSystemLocal : public idClientRendererSystemAPI {
public:
    idClientRendererSystemLocal();
    ~idClientRendererSystemLocal();

    virtual void RefPrintf(sint print_level, pointer fmt, ...);
    virtual void *RefMalloc(sint size);
    virtual void RefTagFree(void);
    virtual sint ScaledMilliseconds(void);
    virtual void ShutdownRef(void);

    static void InitRenderer(void);
    static void InitRef(void);
};

extern idClientRendererSystemLocal clientRendererLocal;

#endif //__CLIENTRENDERER_HPP__

