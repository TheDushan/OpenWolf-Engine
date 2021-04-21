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
// File name:   clientCinema_api.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTCINEMA_API_HPP__
#define __CLIENTCINEMA_API_HPP__

//
// idClientCinemaSystem
//
class idClientCinemaSystem {
public:
    virtual sint PlayCinematic(pointer arg, sint x, sint y, sint w, sint h,
                               sint systemBits) = 0;
    virtual void SetExtents(sint handle, sint x, sint y, sint w, sint h) = 0;
    virtual void SetLooping(sint handle, bool loop) = 0;
    virtual void ResampleCinematic(sint handle, sint *buf2) = 0;
    virtual void CinemaDrawCinematic(sint handle) = 0;
    virtual void DrawCinematic(void) = 0;
    virtual void RunCinematic(void) = 0;
    virtual void StopCinematic(void) = 0;
    virtual void UploadCinematic(sint handle) = 0;
    virtual void CloseAllVideos(void) = 0;
    virtual e_status CinemaRunCinematic(sint handle) = 0;
    virtual e_status CinemaStopCinematic(sint handle) = 0;
};

extern idClientCinemaSystem *clientCinemaSystem;

#endif // !__CLIENTCINEMA_API_HPP__
