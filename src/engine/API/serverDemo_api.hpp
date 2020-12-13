////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   serverDemo_api.hpp
// Created:     11/24/2018
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SERVERDEMO_API_H__
#define __SERVERDEMO_API_H__

//
// idServerWorldSystem
//
class idServerDemoSystem
{
public:
    virtual void DemoWriteServerCommand( pointer str ) = 0;
    virtual void DemoWriteGameCommand( sint cmd, pointer str ) = 0;
    virtual void DemoWriteFrame( void ) = 0;
    virtual void DemoReadFrame( void ) = 0;
    virtual void DemoStartRecord( void ) = 0;
    virtual void DemoStopRecord( void ) = 0;
    virtual void DemoStartPlayback( void ) = 0;
    virtual void DemoStopPlayback( void ) = 0;
};

extern idServerDemoSystem* serverDemoSystem;

#endif //!__SERVERWORLD_API_H__
