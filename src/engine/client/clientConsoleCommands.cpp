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
// File name:   clientConsoleCommands.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

idClientConsoleCommandsSystemLocal clientConsoleCommandsLocal;
idClientConsoleCommandsSystemAPI *clientConsoleCommandSystem =
    &clientConsoleCommandsLocal;

/*
===============
idClientConsoleCommandsSystemLocal::idClientConsoleCommandsSystemLocal
===============
*/
idClientConsoleCommandsSystemLocal::idClientConsoleCommandsSystemLocal(
    void) {
}

/*
===============
idClientConsoleCommandsSystemLocal::~idClientConsoleCommandsSystemLocal
===============
*/
idClientConsoleCommandsSystemLocal::~idClientConsoleCommandsSystemLocal(
    void) {
}

netadr_t rcon_address;
valueType cl_reconnectArgs[MAX_OSPATH] = { 0 };

extern bool autoupdateChecked;
extern bool autoupdateStarted;
extern valueType autoupdateFilename[MAX_QPATH];

/*
==================
idClientConsoleCommandsSystemLocal::ForwardToServer_f
==================
*/
void idClientConsoleCommandsSystemLocal::ForwardToServer_f(void) {
    if(cls.state != CA_ACTIVE || clc.demoplaying) {
        Com_Printf("Not connected to a server.\n");
        return;
    }

    // don't forward the first argument
    if(cmdSystem->Argc() > 1) {
        clientReliableCommandsSystem->AddReliableCommand(cmdSystem->Args());
    }
}

/*
==================
idClientConsoleCommandsSystemLocal::Configstrings_f
==================
*/
void idClientConsoleCommandsSystemLocal::Configstrings_f(void) {
    sint i, ofs;

    if(cls.state != CA_ACTIVE) {
        Com_Printf("Not connected to a server.\n");
        return;
    }

    for(i = 0; i < MAX_CONFIGSTRINGS; i++) {
        ofs = cl.gameState.stringOffsets[i];

        if(!ofs) {
            continue;
        }

        Com_Printf("%4i: %s\n", i, cl.gameState.stringData + ofs);
    }
}

/*
==============
idClientConsoleCommandsSystemLocal::Clientnfo_f
==============
*/
void idClientConsoleCommandsSystemLocal::Clientinfo_f(void) {
    Com_Printf("--------- Client Information ---------\n");
    Com_Printf("state: %i\n", cls.state);
    Com_Printf("Server: %s\n", cls.servername);
    Com_Printf("User info settings:\n");
    Info_Print(cvarSystem->InfoString(CVAR_USERINFO));
    Com_Printf("--------------------------------------\n");
}

/*
=================
idClientConsoleCommandsSystemLocal::Snd_Reload_f

Reloads sounddata from disk, retains soundhandles.
=================
*/
void idClientConsoleCommandsSystemLocal::Snd_Reload_f(void) {
    soundSystem->Reload();
}

/*
=================
idClientConsoleCommandsSystemLocal::Snd_Restart_f

Restart the sound subsystem
The cgame and game must also be forced to restart because
handles will be invalid
=================
*/
void idClientConsoleCommandsSystemLocal::Snd_Restart_f(void) {
    soundSystem->Shutdown();
    soundSystem->Init();

    Vid_Restart_f();
}

/*
=================
idClientConsoleCommandsSystemLocal::Vid_Restart_f

Restart the video subsystem

we also have to reload the UI and CGame because the renderer
doesn't know what graphics to reload
=================
*/
void idClientConsoleCommandsSystemLocal::Vid_Restart_f(void) {
    if(cls.lastVidRestart) {
        if(::abs(cls.lastVidRestart - idsystem->Milliseconds()) < 500) {
            // do not allow video restart right after cgame init
            return;
        }
    }

    // settings may have changed so stop recording now
    if(clientAVISystem->VideoRecording()) {
        cvarSystem->Set("cl_avidemo", "0");
        clientAVISystem->CloseAVI();
    }

    // RF, don't show percent bar, since the memory usage will just sit at the same level anyway
    com_expectedhunkusage = -1;

    // don't let them loop during the restart
    soundSystem->StopAllSounds();

    // shutdown the UI
    clientGUISystem->ShutdownGUI();

    // shutdown the CGame
    clientGameSystem->ShutdownCGame();

    // shutdown the renderer and clear the renderer interface
    clientRendererSystem->ShutdownRef();

    // client is no longer pure untill new checksums are sent
    clientMainSystem->ResetPureClientAtServer();

    // clear pak references
    fileSystem->ClearPakReferences(FS_UI_REF | FS_CGAME_REF);

    // reinitialize the filesystem if the game directory or checksum has changed
    fileSystem->ConditionalRestart(clc.checksumFeed);

    // all sound handles are now invalid
    soundSystem->BeginRegistration();

    cls.rendererStarted = false;
    cls.uiStarted = false;
    cls.cgameStarted = false;
    cls.soundRegistered = false;
    autoupdateChecked = false;

    // unpause so the cgame definately gets a snapshot and renders a frame
    cvarSystem->Set("cl_paused", "0");

    // if not running a server clear the whole hunk
    if(!sv_running->integer) {
        //collisionModelManager->ClearMap();
        // clear the whole hunk
        memorySystem->Clear();
    } else {
        // clear all the client data on the hunk
        memorySystem->ClearToMark();
    }

    // initialize the renderer interface
    idClientRendererSystemLocal::InitRef();

    // startup all the client stuff
    clientMainSystem->StartHunkUsers(false);

#ifdef _WIN32
    idsystem->Restart_f();
#endif

    // start the cgame if connected
    if(cls.state > CA_CONNECTED && cls.state != CA_CINEMATIC) {
        cls.cgameStarted = true;
        clientGameSystem->InitCGame();

        // send pure checksums
        clientMainSystem->SendPureChecksums();
    }
}

/*
=================
idClientConsoleCommandsSystemLocal::UI_Restart_f

Restart the ui subsystem
=================
*/
void idClientConsoleCommandsSystemLocal::UI_Restart_f(void) {
    // NERVE - SMF
    // shutdown the GUI
    clientGUISystem->ShutdownGUI();

    autoupdateChecked = false;

    // init the UI
    clientGUISystem->InitGUI();
}

/*
==================
idClientConsoleCommandsSystemLocal::Disconnect_f
==================
*/
void idClientConsoleCommandsSystemLocal::Disconnect_f(void) {
    clientCinemaSystem->StopCinematic();
    cvarSystem->Set("savegame_loading", "0");
    cvarSystem->Set("g_reloading", "0");

    if(cls.state != CA_DISCONNECTED && cls.state != CA_CINEMATIC) {
        if(cmdSystem->Argc() > 1) {
            valueType reason[MAX_STRING_CHARS] = { 0 };
            Q_strncpyz(reason, cmdSystem->Argv(1), sizeof(reason));
            Q_strstrip(reason, "\r\n;\"", nullptr);
            clientConsoleCommandsLocal.Disconnect(true, reason);
        } else {
            clientConsoleCommandsLocal.Disconnect(true, nullptr);
        }
    }
}

/*
================
idClientConsoleCommandsSystemLocal::Connect_f
================
*/
void idClientConsoleCommandsSystemLocal::Connect_f(void) {
    sint argc = cmdSystem->Argc();
    valueType server[MAX_OSPATH];
    pointer serverString;
    netadrtype_t family = NA_UNSPEC;

    if(argc != 2 && argc != 3) {
        Com_Printf("usage: connect [-4|-6] server\n");
        return;
    }

    // save arguments for reconnect
    Q_strncpyz(cl_reconnectArgs, cmdSystem->Args(), sizeof(cl_reconnectArgs));

    if(argc == 2) {
        Q_strncpyz(server, cmdSystem->Argv(1), sizeof(server));
    } else {
        if(!strcmp(cmdSystem->Argv(1), "-4")) {
            family = NA_IP;
        } else if(!strcmp(cmdSystem->Argv(1), "-6")) {
            family = NA_IP6;
        } else {
            Com_Printf("warning: only -4 or -6 as address type understood.\n");
        }

        Q_strncpyz(server, cmdSystem->Argv(2), sizeof(server));
    }

    soundSystem->StopAllSounds();      // NERVE - SMF

    // starting to load a map so we get out of full screen ui mode
    cvarSystem->Set("r_uiFullScreen", "0");
    cvarSystem->Set("ui_connecting", "1");

    // fire a message off to the motd server
    idClientMOTDSystemLocal::RequestMotd();

    // clear any previous "server full" type messages
    clc.serverMessage[0] = 0;

    if(sv_running->integer && !strcmp(server, "localhost")) {
        // if running a local server, kill it
        serverInitSystem->Shutdown("Server quit\n");
    }

    // make sure a local server is killed
    cvarSystem->Set("sv_killserver", "1");
    serverMainSystem->Frame(0);

    clientConsoleCommandsLocal.Disconnect(true,  "Joining another server");
    clientConsoleSystem->Close();

    Q_strncpyz(cls.servername, server, sizeof(cls.servername));

    if(!networkChainSystem->StringToAdr(cls.servername, &clc.serverAddress,
                                        family)) {
        Com_Printf("Bad server address\n");
        cls.state = CA_DISCONNECTED;
        cvarSystem->Set("ui_connecting", "0");
        return;
    }

    if(clc.serverAddress.port == 0) {
        clc.serverAddress.port = BigShort(PORT_SERVER);
    }

    serverString = networkSystem->AdrToStringwPort(clc.serverAddress);
    Com_Printf("%s resolved to %s\n", cls.servername, serverString);

    if(cl_guidServerUniq->integer) {
        idClientGUIDSystemLocal::UpdateGUID(serverString, strlen(serverString));
    } else {
        idClientGUIDSystemLocal::UpdateGUID(nullptr, 0);
    }

    // if we aren't playing on a lan, we needto authenticate
    // with the cd key
    if(networkSystem->IsLocalAddress(clc.serverAddress)) {
        cls.state = CA_CHALLENGING;
    } else {
        cls.state = CA_CONNECTING;

        // Set a client challenge number that ideally is mirrored back by the server.
        clc.challenge = ((rand() << 16) ^ rand()) ^ Com_Milliseconds();
    }

    cvarSystem->Set("cl_avidemo", "0");

    // show_bug.cgi?id=507
    // prepare to catch a connection process that would turn bad
    cvarSystem->Set("com_errorDiagnoseIP",
                    networkSystem->AdrToString(clc.serverAddress));
    // ATVI Wolfenstein Misc #439
    // we need to setup a correct default for this, otherwise the first val we set might reappear
    cvarSystem->Set("com_errorMessage", "");

    cls.keyCatchers = 0;
    clc.connectTime = -99999;   // CL_CheckForResend() will fire immediately
    clc.connectPacketCount = 0;

    // server connection string
    cvarSystem->Set("cl_currentServerAddress", server);
    cvarSystem->Set("cl_currentServerIP", serverString);

    // Gordon: um, couldnt this be handled
    // NERVE - SMF - reset some cvars
    cvarSystem->Set("mp_playerType", "0");
    cvarSystem->Set("mp_currentPlayerType", "0");
    cvarSystem->Set("mp_weapon", "0");
    cvarSystem->Set("mp_team", "0");
    cvarSystem->Set("mp_currentTeam", "0");

    cvarSystem->Set("ui_limboOptions", "0");
    cvarSystem->Set("ui_limboPrevOptions", "0");
    cvarSystem->Set("ui_limboObjective", "0");
    // -NERVE - SMF

}

/*
================
idClientConsoleCommandsSystemLocal::Reconnect_f
================
*/
void idClientConsoleCommandsSystemLocal::Reconnect_f(void) {
    if(!::strlen(cl_reconnectArgs)) {
        return;
    }

    cmdBufferSystem->AddText(va("connect %s\n", cl_reconnectArgs));
}

/*
==================
idClientConsoleCommandsSystemLocal::OpenUrl_f
==================
*/
void idClientConsoleCommandsSystemLocal::OpenUrl_f(void) {
    pointer url;

    if(cmdSystem->Argc() != 2) {
        Com_Printf("Usage: openurl <url>\n");
        return;
    }

    url = cmdSystem->Argv(1);

    {
        /*
            FixMe: URL sanity checks.

            Random sanity checks. Scott: if you've got some magic URL
            parsing and validating functions USE THEM HERE, this code
            is a placeholder!!!
        */
        sint i;
        pointer u;

        pointer allowPrefixes[] = { "http://", "https://", "" };
        pointer allowDomains[2] = { "localhost", 0 };

        u = url;

        for(i = 0; i < lengthof(allowPrefixes); i++) {
            pointer p = allowPrefixes[i];
            uint32 len = strlen(p);

            if(Q_strncmp(u, p, len) == 0) {
                u += len;
                break;
            }
        }

        if(i == lengthof(allowPrefixes)) {
            /*
                This really won't ever hit because of the "" at the end
                of the allowedPrefixes array. As I said above, placeholder
                code: fix it later!
            */
            Com_Printf("Invalid URL prefix.\n");
            return;
        }

        for(i = 0; i < lengthof(allowDomains); i++) {
            uint32 len;
            pointer d = allowDomains[i];

            if(!d) {
                break;
            }

            len = strlen(d);

            if(Q_strncmp(u, d, len) == 0) {
                u += len;
                break;
            }
        }

        if(i == lengthof(allowDomains)) {
            Com_Printf("Invalid domain.\n");
            return;
        }

        /* my kingdom for a regex */
        for(i = 0; i < strlen(url); i++) {
            if(!(
                        (url[i] >= 'a' && url[i] <= 'z') ||   // lower case alpha
                        (url[i] >= 'A' && url[i] <= 'Z') ||   // upper case alpha
                        (url[i] >= '0' && url[i] <= '9') ||   //numeric
                        (url[i] == '/') || (url[i] == ':') ||     // / and : chars
                        (url[i] == '.') || (url[i] == '&') ||     // . and & chars
                        (url[i] == ';')                          // ; char
                    )) {
                Com_Printf("Invalid URL\n");
                return;
            }
        }
    }

    if(!idsystem->OpenUrl(url)) {
        Com_Printf("System error opening URL\n");
    }
}

/*
=====================
idClientConsoleCommandsSystemLocal::Rcon_f

Send the rest of the command line over as
an unconnected command.
=====================
*/
#define MAX_RCON_MESSAGE 1024
void idClientConsoleCommandsSystemLocal::Rcon_f(void) {
    valueType message[MAX_RCON_MESSAGE];

    if(!strlen(rcon_client_password->string)) {
        Com_Printf("You must set 'rcon_password' before\n"
                   "issuing an rcon command.\n");
        return;
    }

    message[0] = -1;
    message[1] = -1;
    message[2] = -1;
    message[3] = -1;
    message[4] = 0;

    Q_strcat(message, MAX_RCON_MESSAGE, "rcon ");

    Q_strcat(message, MAX_RCON_MESSAGE, rcon_client_password->string);
    Q_strcat(message, MAX_RCON_MESSAGE, " ");

    // ATVI Wolfenstein Misc #284
    Q_strcat(message, MAX_RCON_MESSAGE, cmdSystem->Cmd() + 5);

    if(cls.state >= CA_CONNECTED) {
        rcon_address = clc.netchan.remoteAddress;
    } else {
        if(!::strlen(rconAddress->string)) {
            Com_Printf("You must either be connected,\n"
                       "or set the 'rconAddress' cvar\n"
                       "to issue rcon commands\n");

            return;
        }

        networkChainSystem->StringToAdr(rconAddress->string, &rcon_address,
                                        NA_UNSPEC);

        if(rcon_address.port == 0) {
            rcon_address.port = BigShort(PORT_SERVER);
        }
    }

    networkChainSystem->SendPacket(NS_CLIENT, ::strlen(message) + 1, message,
                                   rcon_address);
}

/*
==================
idClientConsoleCommandsSystemLocal::Setenv_f

Mostly for controlling voodoo environment variables
==================
*/
void idClientConsoleCommandsSystemLocal::Setenv_f(void) {
    sint argc = cmdSystem->Argc();

    if(argc > 2) {
        valueType buffer[1024];
        sint i;

        Q_strcpy_s(buffer, cmdSystem->Argv(1));
        ::strcat(buffer, "=");

        for(i = 2; i < argc; i++) {
            ::strcat(buffer, cmdSystem->Argv(i));
            ::strcat(buffer, " ");
        }

        Q_putenv(buffer);
    } else if(argc == 2) {
        valueType *env = getenv(cmdSystem->Argv(1));

        if(env) {
            Com_Printf("%s=%s\n", cmdSystem->Argv(1), env);
        } else {
            Com_Printf("%s undefined\n", cmdSystem->Argv(1));
        }
    }
}

/*
==================
idClientConsoleCommandsSystemLocal::PK3List_f
==================
*/
void idClientConsoleCommandsSystemLocal::OpenedPK3List_f(void) {
    Com_Printf("Opened PK3 Names: %s\n", fileSystem->LoadedPakNames());
}

/*
==================
idClientConsoleCommandsSystemLocal::PureList_f
==================
*/
void idClientConsoleCommandsSystemLocal::ReferencedPK3List_f(void) {
    Com_Printf("Referenced PK3 Names: %s\n", fileSystem->ReferencedPakNames());
}

/*
================
idClientConsoleCommandsSystemLocal::SetRecommended_f
================
*/
void idClientConsoleCommandsSystemLocal::SetRecommended_f(void) {
    Com_SetRecommended();
}


/*
==============
idClientConsoleCommandsSystemLocal::Userinfo_f
==============
*/
void idClientConsoleCommandsSystemLocal::Userinfo_f(void) {
    //do nothing kthxbye
}

/*
===============
idClientConsoleCommandsSystemLocal::Video_f

video
video [filename]
===============
*/
void idClientConsoleCommandsSystemLocal::Video_f(void) {
    valueType filename[MAX_OSPATH];
    sint i, last;

    if(!clc.demoplaying) {
        Com_Printf("The video command can only be used when playing back demos\n");
        return;
    }

    if(cmdSystem->Argc() == 2) {
        // explicit filename
        Q_vsprintf_s(filename, MAX_OSPATH, MAX_OSPATH, "videos/%s.avi",
                     cmdSystem->Argv(1));
    } else {
        // scan for a free filename
        for(i = 0; i <= 9999; i++) {
            sint             a, b, c, d;

            last = i;

            a = last / 1000;
            last -= a * 1000;
            b = last / 100;
            last -= b * 100;
            c = last / 10;
            last -= c * 10;
            d = last;

            Q_vsprintf_s(filename, MAX_OSPATH, MAX_OSPATH, "videos/video%d%d%d%d.avi",
                         a, b, c, d);

            if(!fileSystem->FileExists(filename)) {
                break;    // file doesn't exist
            }
        }

        if(i > 9999) {
            Com_Printf(S_COLOR_RED "ERROR: no free file names to create video\n");
            return;
        }
    }

    clientAVISystem->OpenAVIForWriting(filename);
}

/*
===============
idClientConsoleCommandsSystemLocal::StopVideo_f
===============
*/
void idClientConsoleCommandsSystemLocal::StopVideo_f(void) {
    clientAVISystem->CloseAVI();
}

/*
===============
idClientConsoleCommandsSystemLocal::UpdateScreen

I had a problem with AddCommand and pointing to the class member so this is one way of doing
pointing to a function that is a class member
===============
*/
void idClientConsoleCommandsSystemLocal::UpdateScreen(void) {
    static_cast<idClientScreenSystemLocal *>
    (clientScreenSystem)->UpdateScreen();
}

/*
=====================
idClientConsoleCommandsSystemLocal::Disconnect

Called when a connection, demo, or cinematic is being terminated.
Goes from a connected state to either a menu state or a console state
Sends a disconnect message to the server
This is also called on Com_Error and Com_Quit, so it shouldn't cause any errors
=====================
*/
void idClientConsoleCommandsSystemLocal::Disconnect(bool showMainMenu,
        pointer reason) {
    if(!cl_running || !cl_running->integer) {
        return;
    }

    // shutting down the client so enter full screen ui mode
    cvarSystem->Set("r_uiFullScreen", "1");

    // Stop demo recording
    if(clc.demorecording) {
        idClientDemoSystemLocal::StopRecord_f();
    }

    if(!cls.bWWWDlDisconnected) {
        if(clc.download) {
            fileSystem->FCloseFile(clc.download);
            clc.download = 0;
        }

        *cls.downloadTempName = *cls.downloadName = 0;
        cvarSystem->Set("cl_downloadName", "");

        autoupdateStarted = false;
        autoupdateFilename[0] = '\0';
    }

    if(clc.demofile) {
        fileSystem->FCloseFile(clc.demofile);
        clc.demofile = 0;
    }

    if(cgvm) {
        // do that right after we rendered last video frame
        clientGameSystem->ShutdownCGame();
    }

    cvarSystem->Set("timescale", "1");

    clientCinemaSystem->StopCinematic();
    soundSystem->ClearSoundBuffer();

    // Remove pure paks
    fileSystem->PureServerSetLoadedPaks("", "");
    fileSystem->PureServerSetReferencedPaks("", "");

    fileSystem->ClearPakReferences(FS_GENERAL_REF | FS_UI_REF | FS_CGAME_REF);

    if(uivm && showMainMenu) {
        uiManager->SetActiveMenu(UIMENU_NONE);
    }

    // send a disconnect message to the server
    // send it a few times in case one is dropped
    if(cls.state >= CA_CONNECTED) {
        valueType cmd[MAX_STRING_CHARS] = { 0 }, * pCmd = cmd;

        if(reason) {
            Q_vsprintf_s(cmd, sizeof(cmd), sizeof(cmd), "disconnect %s", reason);
        } else {
            pCmd = "disconnect";
        }

        clientReliableCommandsSystem->AddReliableCommand(pCmd);
        clientInputSystem->WritePacket();
        clientInputSystem->WritePacket();
        clientInputSystem->WritePacket();
    }

    clientMainSystem->ClearState();

    // wipe the client connection
    ::memset(&clc, 0, sizeof(clc));

    if(!cls.bWWWDlDisconnected) {
        idClientDownloadSystemLocal::ClearStaticDownload();
    }

    // allow cheats locally
    //cvarSystem->Set( "sv_cheats", "1" );

    // not connected to a pure server anymore
    cl_connectedToPureServer = false;

    // stop recording any video
    if(clientAVISystem->VideoRecording()) {
        // finish rendering current frame
        //clientScreenSystem->UpdateScreen();
        clientAVISystem->CloseAVI();
    }

    // show_bug.cgi?id=589
    // don't try a restart if uivm is nullptr, as we might be in the middle of a restart already
    if(uivm && cls.state > CA_DISCONNECTED) {
        // restart the GUI
        cls.state = CA_DISCONNECTED;

        // shutdown the GUI
        clientGUISystem->ShutdownGUI();

        // init the UI
        clientGUISystem->InitGUI();
    } else {
        cls.state = CA_DISCONNECTED;
    }

    idClientGUIDSystemLocal::UpdateGUID(nullptr, 0);
}
