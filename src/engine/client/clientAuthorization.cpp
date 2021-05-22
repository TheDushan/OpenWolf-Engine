////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   clientAuthorization.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

idClientAuthorizationSystemLocal clientAuthorizationLocal;

/*
===============
idClientAuthorizationSystemLocal::idClientAuthorizationSystemLocal
===============
*/
idClientAuthorizationSystemLocal::idClientAuthorizationSystemLocal(void) {
}

/*
===============
idClientAuthorizationSystemLocal::~idClientAuthorizationSystemLocal
===============
*/
idClientAuthorizationSystemLocal::~idClientAuthorizationSystemLocal(void) {
}

/*
===================
idClientAuthorizationSystemLocal::RequestAuthorization
===================
*/
void idClientAuthorizationSystemLocal::RequestAuthorization(void) {
    sint i, j, l;

    if(!cls.authorizeServer.port) {
        if(developer->integer) {
            Com_Printf("Resolving %s\n", AUTHORIZE_SERVER_NAME);
        }

        if(!networkChainSystem->StringToAdr(AUTHORIZE_SERVER_NAME,
                                            &cls.authorizeServer, NA_UNSPEC)) {
            if(developer->integer) {
                Com_Printf("Couldn't resolve authorization server address\n");
            }

            return;
        }

        cls.authorizeServer.port = BigShort(PORT_AUTHORIZE);

        if(developer->integer) {
            Com_Printf("%s resolved to %i.%i.%i.%i:%i\n", AUTHORIZE_SERVER_NAME,
                       cls.authorizeServer.ip[0], cls.authorizeServer.ip[1],
                       cls.authorizeServer.ip[2], cls.authorizeServer.ip[3],
                       BigShort(cls.authorizeServer.port));
        }
    }

    if(cls.authorizeServer.type == NA_BAD) {
        return;
    }

    networkChainSystem->OutOfBandPrint(NS_CLIENT, cls.authorizeServer,
                                       "getKeyAuthorize %i", cls.authorizeAuthCookie);
}

/*
===================
::AuthPacket
===================
*/
void idClientAuthorizationSystemLocal::AuthPacket(netadr_t from) {
    sint challenge;
    uint type;
    pointer msg;

    // if not from our server, ignore it
    if(!networkSystem->CompareAdr(from, clc.serverAddress)) {
        return;
    }

    challenge = ::atoi(cmdSystem->Argv(1));

    if(challenge != cls.authorizeAuthCookie) {
        return;
    }

    // Packet handler
    type = atoi(cmdSystem->Argv(2));
    msg = cmdSystem->ArgsFrom(3);

    switch(type) {
        case 1:
            msg = (!msg) ? "Awaiting authorization server response.." : msg;
            break;

        case 2:
            if(msg) {
                Com_Error(ERR_DROP, "You cannot enter this server^n!\n\n^zReason:^7\n%s\n",
                          msg);
            } else {
                Com_Error(ERR_DROP, "Authorization failed with 0x884 error.\n");
            }

            return;
            break;

        case 3:
            if(msg) {
                Com_Error(ERR_FATAL, "%s\n", msg);
            } else {
                Com_Error(ERR_FATAL, "Authorization failed with 0x888 error.\n");
            }

            return;
            break;

        default:
            msg = "Awaiting Authorization server response..";
    }

    Q_strncpyz(clc.serverMessage, msg, sizeof(clc.serverMessage));

    Com_Printf("%s", clc.serverMessage);
}

