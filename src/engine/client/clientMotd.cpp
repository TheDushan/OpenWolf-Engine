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
// File name:   clientMOTD.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

idClientMOTDSystemLocal clientMOTDLocal;

/*
===============
idClientMOTDSystemLocal::idClientMOTDSystemLocal
===============
*/
idClientMOTDSystemLocal::idClientMOTDSystemLocal(void) {
}

/*
===============
idClientMOTDSystemLocal::~idClientMOTDSystemLocal
===============
*/
idClientMOTDSystemLocal::~idClientMOTDSystemLocal(void) {
}

/*
===================
idClientMOTDSystemLocal::RequestMotd
===================
*/
void idClientMOTDSystemLocal::RequestMotd(void) {
    valueType info[MAX_INFO_STRING];

    if(!cl_motd->integer) {
        return;
    }

    if(developer->integer) {
        common->Printf("Resolving %s\n", MASTER_SERVER_NAME);
    }

    switch(networkChainSystem->StringToAdr(MASTER_SERVER_NAME,
                                           &cls.updateServer, NA_UNSPEC)) {
        case 0:
            common->Printf("Couldn't resolve master address\n");
            return;

        case 2:
            cls.updateServer.port = BigShort(PORT_MASTER);

        default:
            break;
    }

    if(developer->integer) {
        common->Printf("%s resolved to %s\n", MASTER_SERVER_NAME,
                       networkSystem->AdrToStringwPort(cls.updateServer));
    }

    info[0] = 0;

    Q_vsprintf_s(cls.updateChallenge, sizeof(cls.updateChallenge),
                 sizeof(cls.updateChallenge), "%i",
                 static_cast<sint>(((static_cast<uint>(rand()) << 16) ^ static_cast<uint>
                                    (rand())) ^ common->Milliseconds()));

    Info_SetValueForKey(info, "challenge", cls.updateChallenge);
    Info_SetValueForKey(info, "renderer", cls.glconfig.renderer_string);
    Info_SetValueForKey(info, "version", com_version->string);

    networkChainSystem->OutOfBandPrint(NS_CLIENT, cls.updateServer,
                                       "getmotd%s", info);
}

/*
===================
idClientMOTDSystemLocal::str_replace
===================
*/
valueType *idClientMOTDSystemLocal::str_replace(pointer string,
        pointer substr,
        pointer replacement) {
    valueType *tok = nullptr;
    valueType *newstr = nullptr;
    valueType *oldstr = nullptr;

    /* if either substr or replacement is nullptr, duplicate string a let caller handle it */
    if(substr == nullptr || replacement == nullptr) {
        return strdup(string);
    }

    newstr = strdup(string);

    while((tok = strstr(newstr, substr))) {
        oldstr = newstr;
        newstr = static_cast<valueType *>(malloc(strlen(oldstr) - strlen(
                                              substr) + strlen(replacement) + 1));

        /*failed to alloc mem, free old string and return nullptr */
        if(newstr == nullptr) {
            free(oldstr);
            return nullptr;
        }

        memcpy(newstr, oldstr, tok - oldstr);
        memcpy(newstr + (tok - oldstr), replacement, strlen(replacement));
        memcpy(newstr + (tok - oldstr) + strlen(replacement), tok + strlen(substr),
               strlen(oldstr) - strlen(substr) - (tok - oldstr));
        memset(newstr + strlen(oldstr) - strlen(substr) + strlen(replacement), 0,
               1);
        free(oldstr);
    }

    return newstr;
}

/*
===================
idClientMOTDSystemLocal::MotdPacket
===================
*/
void idClientMOTDSystemLocal::MotdPacket(netadr_t from, pointer info) {
    pointer v;
    valueType *w;

    // if not from our server, ignore it
    if(!networkSystem->CompareAdr(from, cls.updateServer)) {
        if(developer->integer) {
            common->Printf("MOTD packet from unexpected source\n");
        }

        return;
    }

    if(developer->integer) {
        common->Printf("MOTD packet: %s\n", info);
    }

    while(*info != '\\') {
        info++;
    }

    // check challenge
    v = Info_ValueForKey(info, "challenge");

    if(strcmp(v, cls.updateChallenge)) {
        if(developer->integer) {
            common->Printf("MOTD packet mismatched challenge: "
                           "'%s' != '%s'\n", v, cls.updateChallenge);
        }

        return;
    }

    v = Info_ValueForKey(info, "motd");
    w = str_replace(v, "|", "\n");

    Q_strncpyz(cls.updateInfoString, info, sizeof(cls.updateInfoString));
    cvarSystem->Set("cl_newsString", w);
    free(w);
}
