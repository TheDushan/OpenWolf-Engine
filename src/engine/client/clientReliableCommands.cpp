////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2021 - 2022 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of the OpenWolf GPL Source Code.
// OpenWolf Source Code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenWolf Source Code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenWolf Source Code.  If not, see <http://www.gnu.org/licenses/>.
//
// In addition, the OpenWolf Source Code is also subject to certain additional terms.
// You should have received a copy of these additional terms immediately following the
// terms and conditions of the GNU General Public License which accompanied the
// OpenWolf Source Code. If not, please request a copy in writing from id Software
// at the address below.
//
// If you have questions concerning this license or the applicable additional terms,
// you may contact in writing id Software LLC, c/o ZeniMax Media Inc.,
// Suite 120, Rockville, Maryland 20850 USA.
//
// -------------------------------------------------------------------------------------
// File name:   clientReliableCommands.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

idClientReliableCommandsSystemLocal clientReliableCommandsLocal;
idClientReliableCommandsSystemAPI *clientReliableCommandsSystem =
    &clientReliableCommandsLocal;

/*
===============
idClientReliableCommandsSystemLocal::idClientReliableCommandsSystemLocal
===============
*/
idClientReliableCommandsSystemLocal::idClientReliableCommandsSystemLocal(
    void) {
}

/*
===============
idClientReliableCommandsSystemLocal::~idClientReliableCommandsSystemLocal
===============
*/
idClientReliableCommandsSystemLocal::~idClientReliableCommandsSystemLocal(
    void) {
}

/*
======================
idClientReliableCommandsSystemLocal::AddReliableCommand

The given command will be transmitted to the server, and is gauranteed to
not have future usercmd_t executed before it is executed
======================
*/
void idClientReliableCommandsSystemLocal::AddReliableCommand(pointer cmd) {
    sint index;

    // if we would be losing an old command that hasn't been acknowledged,
    // we must drop the connection
    if(clc.reliableSequence - clc.reliableAcknowledge >
            MAX_RELIABLE_COMMANDS) {
        common->Error(ERR_DROP, "Client command overflow");
    }

    clc.reliableSequence++;
    index = clc.reliableSequence & (MAX_RELIABLE_COMMANDS - 1);
    Q_strncpyz(clc.reliableCommands[index], cmd,
               sizeof(clc.reliableCommands[index]));
}

/*
======================
idClientReliableCommandsSystemLocal::ChangeReliableCommand
======================
*/
void idClientReliableCommandsSystemLocal::ChangeReliableCommand(void) {
    sint r, index, l;

    // NOTE TTimo: what is the randomize for?
    r = clc.reliableSequence - (random() * 5);
    index = clc.reliableSequence & (MAX_RELIABLE_COMMANDS - 1);
    l = strlen(clc.reliableCommands[index]);

    if(l >= MAX_STRING_CHARS - 1) {
        l = MAX_STRING_CHARS - 2;
    }

    clc.reliableCommands[index][l] = '\n';
    clc.reliableCommands[index][l + 1] = '\0';
}

/*
======================
idClientReliableCommandsSystemLocal::MakeMonkeyDoLaundry
======================
*/
void idClientReliableCommandsSystemLocal::MakeMonkeyDoLaundry(void) {
    if(idsystem->MonkeyShouldBeSpanked()) {
        if(!(cls.framecount & 255)) {
            if(random() < 0.1f) {
                ChangeReliableCommand();
            }
        }
    }
}

