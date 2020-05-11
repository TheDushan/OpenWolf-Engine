////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 - 2019 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   serverCcmds_api.h
// Version:     v1.00
// Created:     04/12/2019
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SERVERCCMDS_API_H__
#define __SERVERCCMDS_API_H__

//
// idServerGameSystem
//
class idServerCcmdsSystem
{
public:
    virtual void TempBanNetAddress( netadr_t address, S32 length ) = 0;
    virtual void AddOperatorCommands( void ) = 0;
    virtual void Heartbeat_f( void ) = 0;
    virtual bool TempBanIsBanned( netadr_t address ) = 0;
};

extern idServerCcmdsSystem* serverCcmdsSystem;

#endif //!__SERVERCCMDS_API_H__
