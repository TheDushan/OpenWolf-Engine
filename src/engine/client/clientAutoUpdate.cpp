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
// File name:   clientAutoUpdate.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

idClientAutoUpdateSystemLocal clientAutoUpdateLocal;
idClientAutoUpdateSystemAPI *clientAutoUpdateSystem =
    &clientAutoUpdateLocal;

/*
===============
idClientAutoUpdateSystemLocal::idClientAutoUpdateSystemLocal
===============
*/
idClientAutoUpdateSystemLocal::idClientAutoUpdateSystemLocal(void) {
}

/*
===============
idClientAutoUpdateSystemLocal::~idClientAutoUpdateSystemLocal
===============
*/
idClientAutoUpdateSystemLocal::~idClientAutoUpdateSystemLocal(void) {
}

// DHM - Nerve :: Have we heard from the auto-update server this session?
extern bool autoupdateChecked;
extern bool autoupdateStarted;

// TTimo : moved from char* to array (was getting the valueType* from va(), broke on big downloads)
extern valueType autoupdateFilename[MAX_QPATH];

/*
===============
idClientAutoUpdateSystemLocal::CheckAutoUpdate
===============
*/
void idClientAutoUpdateSystemLocal::CheckAutoUpdate(void) {
#ifndef PRE_RELEASE_DEMO

    if(!cl_autoupdate->integer) {
        return;
    }

    // Only check once per session
    if(autoupdateChecked) {
        if(developer->integer) {
            common->Printf("Updated checked already..");
        }

        return;
    }

    srand(common->Milliseconds());

    // Resolve update server
    if(!networkChainSystem->StringToAdr(cls.autoupdateServerNames[0],
                                        &cls.autoupdateServer, NA_IP)) {
        if(developer->integer) {
            common->Printf("Failed to resolve any Auto-update servers.\n");
        }

        cls.autoUpdateServerChecked[0] = true;

        autoupdateChecked = true;
        return;
    }

    cls.autoupdatServerIndex = 0;

    cls.autoupdatServerFirstIndex = cls.autoupdatServerIndex;

    cls.autoUpdateServerChecked[cls.autoupdatServerIndex] = true;

    cls.autoupdateServer.port = BigShort(PORT_SERVER);

    if(developer->integer) {
        common->Printf("autoupdate server at: %i.%i.%i.%i:%i\n",
                       cls.autoupdateServer.ip[0], cls.autoupdateServer.ip[1],
                       cls.autoupdateServer.ip[2], cls.autoupdateServer.ip[3],
                       BigShort(cls.autoupdateServer.port));
    }

    networkChainSystem->OutOfBandPrint(NS_CLIENT, cls.autoupdateServer,
                                       "getUpdateInfo \"%s\" \"%s\"\n", PRODUCT_VERSION, OS_STRING);

#endif // !PRE_RELEASE_DEMO

    idClientMOTDSystemLocal::RequestMotd();

    autoupdateChecked = true;
}

/*
===============
idClientAutoUpdateSystemLocal::NextUpdateServer
===============
*/
bool idClientAutoUpdateSystemLocal::NextUpdateServer(void) {
    valueType *servername;

#ifdef PRE_RELEASE_DEMO
    return false;
#endif // PRE_RELEASE_DEMO

    if(!cl_autoupdate->integer) {
        return false;
    }

#if 0 //def _DEBUG
    common->Printf(S_COLOR_MAGENTA
                   "Autoupdate hardcoded OFF in debug build\n");
    return false;
#endif

    while(cls.autoUpdateServerChecked[cls.autoupdatServerFirstIndex]) {
        cls.autoupdatServerIndex++;

        if(cls.autoupdatServerIndex > MAX_AUTOUPDATE_SERVERS) {
            cls.autoupdatServerIndex = 0;
        }

        if(cls.autoupdatServerIndex == cls.autoupdatServerFirstIndex) {
            // went through all of them already
            return false;
        }
    }

    servername = cls.autoupdateServerNames[cls.autoupdatServerIndex];

    if(developer->integer) {
        common->Printf("Resolving AutoUpdate Server... ");
    }

    if(!networkChainSystem->StringToAdr(servername, &cls.autoupdateServer,
                                        NA_IP)) {
        if(developer->integer) {
            common->Printf("Couldn't resolve address, trying next one...");
        }

        cls.autoUpdateServerChecked[cls.autoupdatServerIndex] = true;

        return NextUpdateServer();
    }

    cls.autoUpdateServerChecked[cls.autoupdatServerIndex] = true;

    cls.autoupdateServer.port = BigShort(PORT_SERVER);

    if(developer->integer) {
        common->Printf("%i.%i.%i.%i:%i\n", cls.autoupdateServer.ip[0],
                       cls.autoupdateServer.ip[1],
                       cls.autoupdateServer.ip[2], cls.autoupdateServer.ip[3],
                       BigShort(cls.autoupdateServer.port));
    }

    return true;
}

/*
===============
idClientAutoUpdateSystemLocal::GetAutoUpdate
===============
*/
void idClientAutoUpdateSystemLocal::GetAutoUpdate(void) {
    // Don't try and get an update if we haven't checked for one
    if(!autoupdateChecked) {
        return;
    }

    // Make sure there's a valid update file to request
    if(strlen(cl_updatefiles->string) < 5) {
        return;
    }

    if(developer->integer) {
        common->Printf("Connecting to auto-update server...\n");
    }

    soundSystem->StopAllSounds();           // NERVE - SMF

    // starting to load a map so we get out of full screen ui mode
    cvarSystem->Set("r_uiFullScreen", "0");

    // toggle on all the download related cvars
    cvarSystem->Set("cl_allowDownload", "1");    // general flag
    cvarSystem->Set("cl_wwwDownload", "1");      // ftp/http support

    // clear any previous "server full" type messages
    clc.serverMessage[0] = 0;

    if(sv_running->integer) {
        // if running a local server, kill it
        serverInitSystem->Shutdown("Server quit\n");
    }

    // make sure a local server is killed
    cvarSystem->Set("sv_killserver", "1");
    serverMainSystem->Frame(0);

    clientConsoleCommandSystem->Disconnect(true, "Get autoupdate");
    clientConsoleSystem->Close();

    Q_strncpyz(cls.servername, "Auto-Updater", sizeof(cls.servername));

    if(cls.autoupdateServer.type == NA_BAD) {
        common->Printf("Bad server address\n");
        cls.state = CA_DISCONNECTED;
        cvarSystem->Set("ui_connecting", "0");
        return;
    }

    // Copy auto-update server address to Server connect address
    memcpy(&clc.serverAddress, &cls.autoupdateServer, sizeof(netadr_t));

    if(developer->integer) {
        common->Printf("%s resolved to %i.%i.%i.%i:%i\n", cls.servername,
                       clc.serverAddress.ip[0], clc.serverAddress.ip[1],
                       clc.serverAddress.ip[2], clc.serverAddress.ip[3],
                       BigShort(clc.serverAddress.port));
    }

    cls.state = CA_CONNECTING;

    cls.keyCatchers = 0;
    clc.connectTime = -99999;   // CL_CheckForResend() will fire immediately
    clc.connectPacketCount = 0;

    // server connection string
    cvarSystem->Set("cl_currentServerAddress", "Auto-Updater");
}
