////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2021 - 2022 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   clientReliableCommands.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTRELIABLECOMMANDS_HPP__
#define __CLIENTRELIABLECOMMANDS_HPP__

//
// idClientReliableCommandsSystemLocal
//
class idClientReliableCommandsSystemLocal : public
    idClientReliableCommandsSystemAPI {
public:
    idClientReliableCommandsSystemLocal();
    ~idClientReliableCommandsSystemLocal();

    virtual void AddReliableCommand(pointer cmd);

    static void ChangeReliableCommand(void);
    static void MakeMonkeyDoLaundry(void);
};

extern idClientReliableCommandsSystemLocal clientReliableCommandsLocal;

#endif //__CLIENTRELIABLECOMMANDS_HPP__