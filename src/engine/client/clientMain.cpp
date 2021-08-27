////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   clientMain.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: client main loop
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

clientActive_t  cl;
clientConnection_t clc;
clientStatic_t  cls;
void *cgvm;

extern bool autoupdateChecked;
extern bool autoupdateStarted;
extern netadr_t rcon_address;

idClientMainSystemLocal clientMainLocal;
idClientMainSystemAPI *clientMainSystem = &clientMainLocal;

#define MAX_AVI_BUFFER 2048

static uchar8 buffer[MAX_AVI_BUFFER];
static sint bufIndex;

/*
===============
idClientMainSystemLocal::idClientMainSystemLocal
===============
*/
idClientMainSystemLocal::idClientMainSystemLocal(void) {
}

/*
===============
idClientMainSystemLocal::~idClientMainSystemLocal
===============
*/
idClientMainSystemLocal::~idClientMainSystemLocal(void) {
}

/*
===============
idClientMainSystemLocal::PurgeCache
===============
*/
void idClientMainSystemLocal::PurgeCache(void) {
    cls.doCachePurge = true;
}

/*
===============
idClientMainSystemLocal::DoPurgeCache
===============
*/
void idClientMainSystemLocal::DoPurgeCache(void) {
    if(!cls.doCachePurge) {
        return;
    }

    cls.doCachePurge = false;

    if(!cl_running) {
        return;
    }

    if(!cl_running->integer) {
        return;
    }

    if(!cls.rendererStarted) {
        return;
    }

    //Dushan - FIX ME
    //renderSystem->purgeCache();
}

/*
=====================
idClientMainSystemLocal::ShutdownAll
=====================
*/
void idClientMainSystemLocal::ShutdownAll(bool shutdownRen) {
    // clear sounds
    soundSystem->DisableSounds();

    // download subsystem
    downloadSystem->Shutdown();

    // shutdown CGame
    clientGameSystem->ShutdownCGame();

    // shutdown GUI
    clientGUISystem->ShutdownGUI();


    // shutdown the renderer
    if(shutdownRen) {
        clientRendererSystem->ShutdownRef();
    } else {
        renderSystem->Shutdown(false);      // don't destroy window or context
    }

    //if( renderSystem->purgeCache )
    //{
    //idClientMainSystemLocal::DoPurgeCache();
    //}

    cls.uiStarted = false;
    cls.cgameStarted = false;
    cls.rendererStarted = false;
    cls.soundRegistered = false;

    // Gordon: stop recording on map change etc, demos aren't valid over map changes anyway
    if(clc.demorecording) {
        idClientDemoSystemLocal::StopRecord_f();
    }

    if(clc.waverecording) {
        idClientWaveSystemLocal::WavStopRecord_f();
    }
}

/*
=================
idClientMainSystemLocal::ClearMemory

Called by Com_GameRestart
=================
*/
void idClientMainSystemLocal::ClearMemory(bool shutdownRen) {
    // shutdown all the client stuff
    clientMainLocal.ShutdownAll(shutdownRen);

    // if not running a server clear the whole hunk
    if(!sv_running || !sv_running->integer) {
        // clear the whole hunk
        memorySystem->Clear();
        // clear collision map data
        collisionModelManager->ClearMap();
    } else {
        // clear all the client data on the hunk
        memorySystem->ClearToMark();
    }
}

/*
=================
idClientMainSystemLocal::FlushMemory

Called by idClientMainSystemLocal::MapLoading, idClientMainSystem::LocalConnect_f, idClientMainSystemLocal::PlayDemo_f, and idClientMainSystemLocal::ParseGamestate the only
ways a client gets into a game
Also called by common->Error
=================
*/
void idClientMainSystemLocal::FlushMemory(void) {
    ClearMemory(false);
    StartHunkUsers(false);
}

/*
=====================
idClientMainSystemLocal::MapLoading

A local server is starting to load a map, so update the
screen to let the user know about it, then dump all client
memory on the hunk from cgame, ui, and renderer
=====================
*/
void idClientMainSystemLocal::MapLoading(void) {
    if(!cl_running->integer) {
        return;
    }

    clientConsoleSystem->Close();
    cls.keyCatchers = 0;

    if(cls.state >= CA_CONNECTED && !Q_stricmp(cls.servername, "localhost")) {
        cls.state = CA_CONNECTED;       // so the connect screen is drawn
        ::memset(cls.updateInfoString, 0, sizeof(cls.updateInfoString));
        ::memset(clc.serverMessage, 0, sizeof(clc.serverMessage));
        ::memset(&cl.gameState, 0, sizeof(cl.gameState));
        clc.lastPacketSentTime = -9999;
        clientScreenSystem->UpdateScreen();
    } else {
        // clear nextmap so the cinematic shutdown doesn't execute it
        cvarSystem->Set("nextmap", "");
        clientConsoleCommandSystem->Disconnect(true,  "Loading map");
        Q_strncpyz(cls.servername, "localhost", sizeof(cls.servername));
        cls.state = CA_CHALLENGING;     // so the connect screen is drawn
        cls.keyCatchers = 0;
        clientScreenSystem->UpdateScreen();
        clc.connectTime = -RETRANSMIT_TIMEOUT;
        networkChainSystem->StringToAdr(cls.servername, &clc.serverAddress,
                                        NA_UNSPEC);
        // we don't need a challenge on the localhost

        CheckForResend();
    }
}

/*
=====================
idClientMainSystemLocal::ClearState

Called before parsing a gamestate
=====================
*/
void idClientMainSystemLocal::ClearState(void) {
    ::memset(&cl, 0, sizeof(cl));
}

/*
===================
idClientMainSystemLocal::ForwardCommandToServer

adds the current command line as a clientCommand
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void idClientMainSystemLocal::ForwardCommandToServer(pointer string) {
    valueType *cmd;

    cmd = cmdSystem->Argv(0);

    // ignore key up commands
    if(cmd[0] == '-') {
        return;
    }

    // no userinfo updates from command line
    if(!strcmp(cmd, "userinfo")) {
        return;
    }

    if(clc.demoplaying || cls.state < CA_CONNECTED || cmd[0] == '+') {
        common->Printf("Unknown command \"%s\"\n", cmd);
        return;
    }

    if(cmdSystem->Argc() > 1) {
        clientReliableCommandsSystem->AddReliableCommand(string);
    } else {
        clientReliableCommandsSystem->AddReliableCommand(cmd);
    }
}

/*
=================
idClientMainSystemLocal::SendPureChecksums
=================
*/
void idClientMainSystemLocal::SendPureChecksums(void) {
    sint i;
    pointer pChecksums;
    valueType cMsg[MAX_INFO_VALUE];

    // if we are pure we need to send back a command with our referenced pk3 checksums
    pChecksums = fileSystem->ReferencedPakPureChecksums();

    // "cp"
    Q_vsprintf_s(cMsg, sizeof(cMsg), sizeof(cMsg), "Va ");
    Q_strcat(cMsg, sizeof(cMsg), va("%d ", cl.serverId));
    Q_strcat(cMsg, sizeof(cMsg), pChecksums);

    for(i = 0; i < 2; i++) {
        cMsg[i] += 13 + (i * 2);
    }

    clientReliableCommandsSystem->AddReliableCommand(cMsg);
}

/*
=================
idClientMainSystemLocal::ResetPureClientAtServer
=================
*/
void idClientMainSystemLocal::ResetPureClientAtServer(void) {
    clientReliableCommandsSystem->AddReliableCommand(va("vdr"));
}

/*
=================
idClientMainSystemLocal::CheckForResend

Resend a connect message if the last one has timed out
=================
*/
void idClientMainSystemLocal::CheckForResend(void) {
    sint port, i;
    valueType info[MAX_INFO_STRING], data[MAX_INFO_STRING];

    // don't send anything if playing back a demo
    if(clc.demoplaying) {
        return;
    }

    // resend if we haven't gotten a reply yet
    if(cls.state != CA_CONNECTING && cls.state != CA_CHALLENGING) {
        return;
    }

    if(cls.realtime - clc.connectTime < RETRANSMIT_TIMEOUT) {
        return;
    }

    clc.connectTime = cls.realtime; // for retransmit requests
    clc.connectPacketCount++;

    switch(cls.state) {
        case CA_CONNECTING:
            if(clc.connectPacketCount == 1) {
                cls.authorizeAuthCookie = rand();
            }

            // requesting a challenge .. IPv6 users always get in as authorize server supports no ipv6.
            if(clc.serverAddress.type == NA_IP) {
                idClientAuthorizationSystemLocal::RequestAuthorization();
            }

            Q_vsprintf_s(data, sizeof(data), sizeof(data), "getchallenge %d %s %s %i",
                         clc.challenge, GAMENAME_FOR_MASTER, cl_guid->string,
                         cls.authorizeAuthCookie);
            networkChainSystem->OutOfBandPrint(NS_CLIENT, clc.serverAddress, "%s",
                                               data);
            break;

        case CA_CHALLENGING:
            // sending back the challenge
            port = cvarSystem->VariableValue("net_qport");

            Q_strncpyz(info, cvarSystem->InfoString(CVAR_USERINFO), sizeof(info));
            Info_SetValueForKey(info, "protocol", va("%i", PROTOCOL_VERSION));
            Info_SetValueForKey(info, "qport", va("%i", port));
            Info_SetValueForKey(info, "challenge", va("%i", clc.challenge));

            Q_strcpy_s(data, "connect ");
            data[8] = '"';

            for(i = 0; i < strlen(info); i++) {
                data[9 + i] = info[i];  // + (clc.challenge)&0x3;
            }

            data[9 + i] = '"';
            data[10 + i] = 0;

            networkChainSystem->OutOfBandData(NS_CLIENT, clc.serverAddress,
                                              reinterpret_cast<uchar8 *>(&data[0]), i + 10);
            // the most current userinfo has been sent, so watch for any
            // newer changes to userinfo variables
            cvar_modifiedFlags &= ~CVAR_USERINFO;
            break;

        default:
            common->Error(ERR_FATAL,
                          "idClientMainSystemLocal::CheckForResend: bad cls.state");
    }
}

/*
===================
idClientMainSystemLocal::DisconnectPacket

Sometimes the server can drop the client and the netchan based
disconnect can be lost.  If the client continues to send packets
to the server, the server will send out of band disconnect packets
to the client so it doesn't have to wait for the full timeout period.
===================
*/
void idClientMainSystemLocal::DisconnectPacket(netadr_t from) {
    pointer message;

    if(cls.state < CA_AUTHORIZING) {
        return;
    }

    // if not from our server, ignore it
    if(!networkSystem->CompareAdr(from, clc.netchan.remoteAddress)) {
        return;
    }

    // if we have received packets within three seconds, ignore (it might be a malicious spoof)
    // NOTE TTimo:
    // there used to be a  clc.lastPacketTime = cls.realtime; line in CL_PacketEvent before calling CL_ConnectionLessPacket
    // therefore .. packets never got through this check, clients never disconnected
    // switched the clc.lastPacketTime = cls.realtime to happen after the connectionless packets have been processed
    // you still can't spoof disconnects, cause legal netchan packets will maintain realtime - lastPacketTime below the threshold
    if(cls.realtime - clc.lastPacketTime < 3000) {
        return;
    }

    // if we are doing a disconnected download, leave the 'connecting' screen on with the progress information
    if(!cls.bWWWDlDisconnected) {
        // drop the connection
        message = "Server disconnected for unknown reason\n";
        common->Printf("%s", message);
        cvarSystem->Set("com_errorMessage", message);
        clientConsoleCommandSystem->Disconnect(true, "Disconnect packet");
    } else {
        clientConsoleCommandSystem->Disconnect(false, "Disconnect packet");
        cvarSystem->Set("ui_connecting", "1");
        cvarSystem->Set("ui_dl_running", "1");
    }
}

/*
===================
CL_PrintPackets

an OOB message from server, with potential markups
print OOB are the only messages we handle markups in
[err_dialog]: used to indicate that the connection should be aborted
  no further information, just do an error diagnostic screen afterwards
[err_prot]: HACK. This is a protocol error. The client uses a custom
  protocol error message (client sided) in the diagnostic window.
  The space for the error message on the connection screen is limited
  to 256 chars.
===================
*/
void idClientMainSystemLocal::PrintPacket(netadr_t from, msg_t *msg) {
    valueType *s;

    s = MSG_ReadBigString(msg);

    if(!Q_stricmpn(s, "[err_dialog]", 12)) {
        Q_strncpyz(clc.serverMessage, s + 12, sizeof(clc.serverMessage));
        //cvarSystem->Set("com_errorMessage", clc.serverMessage );
        common->Error(ERR_DROP, "%s", clc.serverMessage);
    } else if(!Q_stricmpn(s, "[err_prot]", 10)) {
        Q_strncpyz(clc.serverMessage, s + 10, sizeof(clc.serverMessage));
        //cvarSystem->Set("com_errorMessage", clientLocalizationSystem->TranslateStringBuf( PROTOCOL_MISMATCH_ERROR_LONG ) );
        common->Error(ERR_DROP, "%s",
                      clientLocalizationSystem->TranslateStringBuf(
                          PROTOCOL_MISMATCH_ERROR_LONG));
    } else if(!Q_stricmpn(s, "[err_update]", 12)) {
        Q_strncpyz(clc.serverMessage, s + 12, sizeof(clc.serverMessage));
        common->Error(ERR_AUTOUPDATE, "%s", clc.serverMessage);
    } else if(!Q_stricmpn(s, "ET://", 5)) {
        // fretn
        Q_strncpyz(clc.serverMessage, s, sizeof(clc.serverMessage));
        cvarSystem->Set("com_errorMessage", clc.serverMessage);
        common->Error(ERR_DROP, "%s", clc.serverMessage);
    } else {
        Q_strncpyz(clc.serverMessage, s, sizeof(clc.serverMessage));
    }

    common->Printf("%s", clc.serverMessage);
}

/*
=================
idClientMainSystemLocal::ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
void idClientMainSystemLocal::ConnectionlessPacket(netadr_t from,
        msg_t *msg) {
    valueType *s, *c;
    sint challenge = 0;

    MSG_BeginReadingOOB(msg);
    MSG_ReadLong(msg);      // skip the -1

    s = MSG_ReadStringLine(msg);

    cmdSystem->TokenizeString(s);

    c = cmdSystem->Argv(0);

    if(developer->integer) {
        common->Printf("CL packet %s: %s\n", networkSystem->AdrToStringwPort(from),
                       c);
    }

    if(!Q_stricmp(c, "disconnectResponse")) {
        if(cls.state == CA_CONNECTED) {
            clientConsoleCommandSystem->Disconnect(true, "Disconnect response");
        }

        return;
    }

    // challenge from the server we are connecting to
    if(!Q_stricmp(c, "challengeResponse")) {
        if(cls.state != CA_CONNECTING) {
            common->Printf("Unwanted challenge response received.  Ignored.\n");
            return;
        }

        c = cmdSystem->Argv(2);

        if(*c) {
            challenge = atoi(c);
        }

        if(!networkSystem->CompareAdr(from, clc.serverAddress)) {
            // This challenge response is not coming from the expected address.
            // Check whether we have a matching client challenge to prevent
            // connection hi-jacking.

            if(!*c || challenge != clc.challenge) {
                if(developer->integer) {
                    common->Printf("Challenge response received from unexpected source. Ignored.\n");
                }

                return;
            }
        }

        // start sending challenge response instead of challenge request packets
        clc.challenge = atoi(cmdSystem->Argv(1));

        if(cmdSystem->Argc() > 2) {
            clc.onlyVisibleClients = atoi(cmdSystem->Argv(2));        // DHM - Nerve
        } else {
            clc.onlyVisibleClients = 0;
        }

        cls.state = CA_CHALLENGING;
        clc.connectPacketCount = 0;
        clc.connectTime = -99999;

        // take this address as the new server address.  This allows
        // a server proxy to hand off connections to multiple servers
        clc.serverAddress = from;

        if(developer->integer) {
            common->Printf("challengeResponse: %d\n", clc.challenge);
        }

        return;
    }

    // server connection
    if(!Q_stricmp(c, "connectResponse")) {
        if(cls.state >= CA_CONNECTED) {
            common->Printf("Dup connect received.  Ignored.\n");
            return;
        }

        if(cls.state != CA_CHALLENGING) {
            common->Printf("connectResponse packet while not connecting.  Ignored.\n");
            return;
        }

        if(!networkSystem->CompareAdr(from, clc.serverAddress)) {
            common->Printf("connectResponse from a different address.  Ignored.\n");
            common->Printf("%s should have been %s\n",
                           networkSystem->AdrToString(from),
                           networkSystem->AdrToStringwPort(clc.serverAddress));
            return;
        }

        // DHM - Nerve :: If we have completed a connection to the Auto-Update server...
        if(autoupdateChecked &&
                networkSystem->CompareAdr(cls.autoupdateServer, clc.serverAddress)) {
            // Mark the client as being in the process of getting an update
            if(cl_updateavailable->integer) {
                autoupdateStarted = true;
            }
        }

        networkChainSystem->Setup(NS_CLIENT, &clc.netchan, from,
                                  cvarSystem->VariableValue("net_qport"));
        cls.state = CA_CONNECTED;
        clc.lastPacketSentTime = -9999;     // send first packet immediately
        return;
    }

    // server responding to an info broadcast
    if(!Q_stricmp(c, "infoResponse")) {
        idClientBrowserSystemLocal::ServerInfoPacket(from, msg);
        return;
    }

    // server responding to a get playerlist
    if(!Q_stricmp(c, "statusResponse")) {
        idClientBrowserSystemLocal::ServerStatusResponse(from, msg);
        return;
    }

    // a disconnect message from the server, which will happen if the server
    // dropped the connection but it is still getting packets from us
    if(!Q_stricmp(c, "disconnect")) {
        DisconnectPacket(from);
        return;
    }

    // Auth check
#if 0

    if(!Q_stricmp(c, "authStatus")) {
        idClientAuthorizationSystemLocal::AuthPacket(from);
        return;
    }

#endif

    // global MOTD from id
    if(!Q_stricmp(c, "motd")) {
        idClientMOTDSystemLocal::MotdPacket(from, s);
        return;
    }

    // echo request from server
    if(!Q_stricmp(c, "print")) {
        if(networkSystem->CompareAdr(from, clc.serverAddress) ||
                networkSystem->CompareAdr(from, rcon_address)) {
            PrintPacket(from, msg);
        }

        return;
    }

    // DHM - Nerve :: Auto-update server response message
    if(!Q_stricmp(c, "updateResponse")) {
        UpdateInfoPacket(from);
        return;
    }

    // DHM - Nerve

    // NERVE - SMF - bugfix, make this compare first n chars so it doesnt bail if token is parsed incorrectly
    // echo request from server
    if(!Q_strncmp(c, "getserversResponse", 18)) {
        idClientBrowserSystemLocal::ServersResponsePacket(&from, msg, false);
        return;
    }

    // list of servers sent back by a master server (extended)
    if(!Q_strncmp(c, "getserversExtResponse", 21)) {
        idClientBrowserSystemLocal::ServersResponsePacket(&from, msg, true);
        return;
    }

    if(developer->integer) {
        common->Printf("Unknown connectionless packet command.\n");
    }
}

/*
=================
idClientMainSystemLocal::PacketEvent

A packet has arrived from the main event loop
=================
*/
void idClientMainSystemLocal::PacketEvent(netadr_t from, msg_t *msg) {
    sint headerBytes;

    clc.lastPacketTime = cls.realtime;

    if(msg->cursize >= 4 && *reinterpret_cast<sint *>(msg->data) == -1) {
        ConnectionlessPacket(from, msg);
        return;
    }


    if(cls.state < CA_CONNECTED) {
        return;     // can't be a valid sequenced packet
    }

    if(msg->cursize < 4) {
        common->Printf("%s: Runt packet\n", networkSystem->AdrToStringwPort(from));
        return;
    }

    //
    // packet from server
    //
    if(!networkSystem->CompareAdr(from, clc.netchan.remoteAddress)) {
        if(developer->integer) {
            common->Printf("%s:sequenced packet without connection\n"
                           , networkSystem->AdrToStringwPort(from));
        }

        // FIXME: send a client disconnect?
        return;
    }

    if(!clientNetChanSystem->Netchan_Process(&clc.netchan, msg)) {
        return;     // out of order, duplicated, etc
    }

    // the header is different lengths for reliable and unreliable messages
    headerBytes = msg->readcount;

    // track the last message received so it can be returned in
    // client messages, allowing the server to detect a dropped
    // gamestate
    clc.serverMessageSequence = LittleLong(*reinterpret_cast<sint *>
                                           (msg->data));

    clc.lastPacketTime = cls.realtime;
    idClientParseSystemLocal::ParseServerMessage(msg);

    //
    // we don't know if it is ok to save a demo message until
    // after we have parsed the frame
    //

    if(clc.demorecording && !clc.demowaiting) {
        idClientDemoSystemLocal::WriteDemoMessage(msg, headerBytes);
    }
}

/*
==================
idClientMainSystemLocal::CheckTimeout
==================
*/
void idClientMainSystemLocal::CheckTimeout(void) {
    //
    // check timeout
    //
    if((!cl_paused->integer || !sv_paused->integer)
            && cls.state >= CA_CONNECTED && cls.state != CA_CINEMATIC &&
            cls.realtime - clc.lastPacketTime > cl_timeout->value * 1000) {
        if(++cl.timeoutcount > 5) {
            // timeoutcount saves debugger
            cvarSystem->Set("com_errorMessage", "Server connection timed out.");
            clientConsoleCommandSystem->Disconnect(true, "server timed out");
            return;
        }
    } else {
        cl.timeoutcount = 0;
    }
}

/*
==================
idClientMainSystemLocal::CheckUserinfo
==================
*/
void idClientMainSystemLocal::CheckUserinfo(void) {
    // don't add reliable commands when not yet connected
    if(cls.state < CA_CHALLENGING) {
        return;
    }

    // don't overflow the reliable command buffer when paused on a local server
    if(cl_paused->integer && sv_paused->integer && sv_running->integer) {
        return;
    }

    // send a reliable userinfo update if needed
    if(cvar_modifiedFlags & CVAR_USERINFO) {
        cvar_modifiedFlags &= ~CVAR_USERINFO;
        clientReliableCommandsSystem->AddReliableCommand(
            va("userinfo \"%s\"",
               cvarSystem->InfoString(CVAR_USERINFO)));
    }
}

/*
==================
idClientMainSystemLocal::Frame
==================
*/
void idClientMainSystemLocal::Frame(sint msec) {
    if(!cl_running->integer) {
        return;
    }

    if(cls.realtime >= cls.lastTimeDraw + 1) {
        cls.lastTimeDraw = cls.realtime;
    }

    if(cls.state == CA_DISCONNECTED && !(cls.keyCatchers & KEYCATCH_UI) &&
            !sv_running->integer && clc.demoplaying && uivm) {
        // if disconnected, bring up the menu
        soundSystem->StopAllSounds();
        uiManager->SetActiveMenu(UIMENU_MAIN);
    }

    // if recording an avi, lock to a fixed fps
    if(clientAVISystem->VideoRecording() && cl_aviFrameRate->integer && msec) {
        if(!clientAVISystem->VideoRecording()) {
            idClientConsoleCommandsSystemLocal::Video_f();
        }

        // save the current screen
        if(cls.state == CA_ACTIVE || cl_forceavidemo->integer) {
            clientAVISystem->TakeVideoFrame();

            // fixed time for next frame'
            msec = static_cast<sint>(ceil((1000.0f / cl_aviFrameRate->value) *
                                          timescale->value));

            if(msec == 0) {
                msec = 1;
            }
        }
    }

    idClientReliableCommandsSystemLocal::MakeMonkeyDoLaundry();

    // save the msec before checking pause
    cls.realFrametime = msec;

    // decide the simulation time
    cls.frametime = msec;

    cls.realtime += cls.frametime;

    if(timegraph->integer) {
        clientScreenSystem->DebugGraph(cls.realFrametime * 0.25f, 0);
    }

    // see if we need to update any userinfo
    CheckUserinfo();

    // if we haven't gotten a packet in a long time,
    // drop the connection
    CheckTimeout();

    // wwwdl download may survive a server disconnect
    if((cls.state == CA_CONNECTED && clc.bWWWDl) || cls.bWWWDlDisconnected) {
        idClientDownloadSystemLocal::WWWDownload();
    }

    // send intentions now
    clientInputSystem->SendCmd();

    // resend a connection request if necessary
    CheckForResend();

    // decide on the serverTime to render
    clientGameSystem->SetCGameTime();

    // update the screen
    clientScreenSystem->UpdateScreen();

    // update the sound
    soundSystem->Update();

    // advance local effects for next frame
    clientCinemaSystem->RunCinematic();

    clientConsoleSystem->RunConsole();

    cls.framecount++;
}

/*
============================
idClientMainSystemLocal::StartHunkUsers

After the server has cleared the hunk, these will need to be restarted
This is the only place that any of these functions are called from
============================
*/
void idClientMainSystemLocal::StartHunkUsers(bool rendererOnly) {
    if(!cl_running) {
        return;
    }

    if(!cl_running->integer) {
        return;
    }

    if(!cls.rendererStarted) {
        cls.rendererStarted = true;
        idClientRendererSystemLocal::InitRenderer();
    }

    if(rendererOnly) {
        return;
    }

    if(!cls.soundStarted) {
        cls.soundStarted = true;
        soundSystem->Init();
    }

    if(!cls.soundRegistered) {
        cls.soundRegistered = true;
        soundSystem->BeginRegistration();
    }

    if(dedicated->integer) {
        return;
    }

    if(!cls.uiStarted) {
        cls.uiStarted = true;
        clientGUISystem->InitGUI();
    }
}

/*
====================
idClientMainSystemLocal::Init
====================
*/
void idClientMainSystemLocal::Init(void) {
    common->Printf("----- idClientMainSystemLocal::Init -----\n");

    clientConsoleSystem->Init();

    ClearState();

    cls.state = CA_DISCONNECTED;    // no longer CA_UNINITIALIZED

    cls.realtime = 0;

    clientInputSystem->InitInput();

    Q_strncpyz(cls.autoupdateServerNames[0], AUTOUPDATE_SERVER1_NAME,
               MAX_QPATH);
    Q_strncpyz(cls.autoupdateServerNames[1], AUTOUPDATE_SERVER2_NAME,
               MAX_QPATH);
    Q_strncpyz(cls.autoupdateServerNames[2], AUTOUPDATE_SERVER3_NAME,
               MAX_QPATH);
    Q_strncpyz(cls.autoupdateServerNames[3], AUTOUPDATE_SERVER4_NAME,
               MAX_QPATH);
    Q_strncpyz(cls.autoupdateServerNames[4], AUTOUPDATE_SERVER5_NAME,
               MAX_QPATH);
    // DHM - Nerve

    //
    // register our commands
    //
    cmdSystem->AddCommand("cmd",
                          &idClientConsoleCommandsSystemLocal::ForwardToServer_f,
                          "Commands in the console");
    cmdSystem->AddCommand("configstrings",
                          &idClientConsoleCommandsSystemLocal::Configstrings_f,
                          "Returns config strings");
    cmdSystem->AddCommand("clientinfo",
                          &idClientConsoleCommandsSystemLocal::Clientinfo_f,
                          "Returns some info about your client in the console, probably also read by server");
    cmdSystem->AddCommand("snd_reload",
                          &idClientConsoleCommandsSystemLocal::Snd_Reload_f,
                          "Reloads all sound files");
    cmdSystem->AddCommand("snd_restart",
                          &idClientConsoleCommandsSystemLocal::Snd_Restart_f,
                          "Restarts sound engine");
    cmdSystem->AddCommand("vid_restart",
                          &idClientConsoleCommandsSystemLocal::Vid_Restart_f,
                          "Reloads the video/rendering etc, required for some cvar settings to take actual effect.");
    cmdSystem->AddCommand("ui_restart",
                          &idClientConsoleCommandsSystemLocal::UI_Restart_f,
                          "Reloads all menu files");
    cmdSystem->AddCommand("disconnect",
                          &idClientConsoleCommandsSystemLocal::Disconnect_f,
                          "Command to disconnect from a server");
    cmdSystem->AddCommand("record", &idClientDemoSystemLocal::Record_f,
                          "Starts recording a demo");
    cmdSystem->AddCommand("demo", idClientDemoSystemLocal::PlayDemo_f,
                          "For loading a demo for playback: /demo demofilename");
    cmdSystem->SetCommandCompletionFunc("demo",
                                        idClientDemoSystemLocal::CompleteDemoName);
    cmdSystem->AddCommand("cinematic",
                          &idClientCinemaSystemLocal::PlayCinematic_f,
                          "[/cinematic] will play the intro movie. Doesnt work in game");
    cmdSystem->AddCommand("stoprecord", idClientDemoSystemLocal::StopRecord_f,
                          "Stops recording a demo");
    cmdSystem->AddCommand("connect",
                          &idClientConsoleCommandsSystemLocal::Connect_f,
                          "Used for connecting to a server, /connect ip.ad.dre.ss:port");
    cmdSystem->AddCommand("reconnect",
                          &idClientConsoleCommandsSystemLocal::Reconnect_f,
                          "Reconnect to the most recent server you tried to connect to");
    cmdSystem->AddCommand("localservers",
                          &idClientBrowserSystemLocal::LocalServers,
                          "Scans for LAN connected servers");
    cmdSystem->AddCommand("globalservers",
                          &idClientBrowserSystemLocal::GlobalServers,
                          "Command to scan for all servers, internet & LAN");
    cmdSystem->AddCommand("openurl",
                          &idClientConsoleCommandsSystemLocal::OpenUrl_f, "Opens a URL");
    cmdSystem->AddCommand("rcon", &idClientConsoleCommandsSystemLocal::Rcon_f,
                          "Prepend to issue to remote-console, i.e. Send what follows to the server as command.");
    cmdSystem->AddCommand("setenv",
                          &idClientConsoleCommandsSystemLocal::Setenv_f,
                          "It is used to define the value of environment variables");
    cmdSystem->AddCommand("ping", &idClientBrowserSystemLocal::Ping,
                          "For pinging server - /ping [server]");
    cmdSystem->AddCommand("serverstatus",
                          &idClientBrowserSystemLocal::ServerStatus,
                          "Returns serverinfo plus basic player info");
    cmdSystem->AddCommand("showip", &idClientBrowserSystemLocal::ShowIP,
                          "Returns your IP in console");
    cmdSystem->AddCommand("fs_openedList",
                          &idClientConsoleCommandsSystemLocal::OpenedPK3List_f,
                          "Holds what .pk3's are open / in Main or mod folder");
    cmdSystem->AddCommand("fs_referencedList",
                          &idClientConsoleCommandsSystemLocal::ReferencedPK3List_f,
                          "Holds what .pk3s are referenced/loaded");

    // Ridah, startup-caching system
    cmdSystem->AddCommand("cache_startgather",
                          &idClientStartUpCacheSystemLocal::Cache_StartGather_f,
                          "Startup-caching system");
    cmdSystem->AddCommand("cache_usedfile",
                          &idClientStartUpCacheSystemLocal::Cache_UsedFile_f,
                          "Startup-caching system");
    cmdSystem->AddCommand("cache_setindex",
                          &idClientStartUpCacheSystemLocal::Cache_SetIndex_f,
                          "Startup-caching system");
    cmdSystem->AddCommand("cache_mapchange",
                          &idClientStartUpCacheSystemLocal::Cache_MapChange_f,
                          "Startup-caching system");
    cmdSystem->AddCommand("cache_endgather",
                          &idClientStartUpCacheSystemLocal::Cache_EndGather_f,
                          "Startup-caching system");

    cmdSystem->AddCommand("updatehunkusage",
                          &idClientGameSystemLocal::UpdateLevelHunkUsage,
                          "Update hunk memory usage");
    cmdSystem->AddCommand("updatescreen",
                          &idClientConsoleCommandsSystemLocal::UpdateScreen,
                          "Toggle command to update the screen");
    // done.

    cmdSystem->AddCommand("SaveTranslations",
                          &idClientLocalizationSystemLocal::SaveTranslations_f,
                          "Toggle command to save translations");    // NERVE - SMF - localization
    cmdSystem->AddCommand("SaveNewTranslations",
                          &idClientLocalizationSystemLocal::SaveNewTranslations_f,
                          "Toggle command to save new translations");  // NERVE - SMF - localization
    cmdSystem->AddCommand("LoadTranslations",
                          &idClientLocalizationSystemLocal::LoadTranslations_f,
                          "Toggle command to load translation");     // NERVE - SMF - localization

    cmdSystem->AddCommand("setRecommended",
                          &idClientConsoleCommandsSystemLocal::SetRecommended_f,
                          "Applies the settings that were reccommended for your system");
    cmdSystem->AddCommand("userinfo",
                          &idClientConsoleCommandsSystemLocal::Userinfo_f,
                          "This info is transmitted by client to the server on connect, and if changed.");
    cmdSystem->AddCommand("wav_record", &idClientWaveSystemLocal::WavRecord_f,
                          "Command to record .wav audio file");
    cmdSystem->AddCommand("wav_stoprecord",
                          &idClientWaveSystemLocal::WavStopRecord_f,
                          "Command to stop recording a .wav audio file");

    cmdSystem->AddCommand("video",
                          &idClientConsoleCommandsSystemLocal::Video_f,
                          "Toggle use of video in game");
    cmdSystem->AddCommand("stopvideo",
                          &idClientConsoleCommandsSystemLocal::StopVideo_f,
                          "Toggle stop video in the game");

    idClientRendererSystemLocal::InitRef();

    clientScreenSystem->Init();

    cmdBufferSystem->Execute();

    cvarSystem->Set("cl_running", "1");

    idClientGUIDSystemLocal::GenerateGUIDKey();

    //Dushan
    cl_guid = cvarSystem->Get("cl_guid", "NO_GUID", CVAR_USERINFO | CVAR_ROM,
                              "Enable GUID userinfo identification");
    idClientGUIDSystemLocal::UpdateGUID(nullptr, 0);

    // DHM - Nerve
    autoupdateChecked = false;
    autoupdateStarted = false;

    clientLocalizationSystem->InitTranslation(); // NERVE - SMF - localization

    common->Printf("----- Client Initialization Complete -----\n");
}

/*
===============
idClientMainSystemLocal::Shutdown
===============
*/
void idClientMainSystemLocal::Shutdown(void) {
    static bool recursive = false;

    // check whether the client is running at all.
    if(!(cl_running && cl_running->integer)) {
        return;
    }

    common->Printf("----- idClientMainSystemLocal::Shutdown -----\n");

    if(recursive) {
        common->Printf("WARNING: Recursive idClientMainSystemLocal::Shutdown called!\n");
        return;
    }

    recursive = true;

    // fretn - write wav header when we quit
    if(clc.waverecording) {
        idClientWaveSystemLocal::WavStopRecord_f();
    }

    clientConsoleCommandSystem->Disconnect(true, "client shutdown");

    downloadSystem->Shutdown();
    clientRendererSystem->ShutdownRef();

    clientGUISystem->ShutdownGUI();

    soundSystem->Shutdown();

    cmdSystem->RemoveCommand("cmd");
    cmdSystem->RemoveCommand("configstrings");
    cmdSystem->RemoveCommand("snd_reload");
    cmdSystem->RemoveCommand("snd_restart");
    cmdSystem->RemoveCommand("vid_restart");
    cmdSystem->RemoveCommand("disconnect");
    cmdSystem->RemoveCommand("record");
    cmdSystem->RemoveCommand("demo");
    cmdSystem->RemoveCommand("cinematic");
    cmdSystem->RemoveCommand("stoprecord");
    cmdSystem->RemoveCommand("connect");
    cmdSystem->RemoveCommand("localservers");
    cmdSystem->RemoveCommand("globalservers");
    cmdSystem->RemoveCommand("rcon");
    cmdSystem->RemoveCommand("setenv");
    cmdSystem->RemoveCommand("ping");
    cmdSystem->RemoveCommand("serverstatus");
    cmdSystem->RemoveCommand("showip");
    cmdSystem->RemoveCommand("model");

    // Ridah, startup-caching system
    cmdSystem->RemoveCommand("cache_startgather");
    cmdSystem->RemoveCommand("cache_usedfile");
    cmdSystem->RemoveCommand("cache_setindex");
    cmdSystem->RemoveCommand("cache_mapchange");
    cmdSystem->RemoveCommand("cache_endgather");

    cmdSystem->RemoveCommand("updatehunkusage");
    cmdSystem->RemoveCommand("wav_record");
    cmdSystem->RemoveCommand("wav_stoprecord");
    // done.
    cmdSystem->RemoveCommand("userinfo");

    soundSystem->Shutdown();
    cvarSystem->Set("cl_running", "0");

    recursive = false;

    ::memset(&cls, 0, sizeof(cls));
    clientGUISystem->SetCatcher(0);

    common->Printf("-----------------------\n");
}

/*
=======================
idClientMainSystemLocal::OpenURL
=======================
*/
void idClientMainSystemLocal::OpenURL(pointer url) {
    if(!url || !strlen(url)) {
        common->Printf("%s",
                       clientLocalizationSystem->TranslateStringBuf("invalid/empty URL\n"));
        return;
    }

    idsystem->OpenURL(url, true);
}

/*
===================
idClientMainSystemLocal::UpdateInfoPacket
===================
*/
void idClientMainSystemLocal::UpdateInfoPacket(netadr_t from) {
    if(cls.autoupdateServer.type == NA_BAD) {
        common->Printf("idClientMainSystemLocal::UpdateInfoPacket:  Auto-Updater has bad address\n");
        return;
    }

    if(developer->integer) {
        common->Printf("Auto-Updater resolved to %i.%i.%i.%i:%i\n",
                       cls.autoupdateServer.ip[0], cls.autoupdateServer.ip[1],
                       cls.autoupdateServer.ip[2], cls.autoupdateServer.ip[3],
                       BigShort(cls.autoupdateServer.port));
    }

    if(!networkSystem->CompareAdr(from, cls.autoupdateServer)) {
        if(developer->integer) {
            common->Printf("idClientMainSystemLocal::UpdateInfoPacket:  Received packet from %i.%i.%i.%i:%i\n",
                           from.ip[0], from.ip[1], from.ip[2], from.ip[3],
                           BigShort(from.port));
        }

        return;
    }

    cvarSystem->Set("cl_updateavailable", cmdSystem->Argv(1));

    if(!Q_stricmp(cl_updateavailable->string, "1")) {
        cvarSystem->Set("cl_updatefiles", cmdSystem->Argv(2));
        uiManager->SetActiveMenu(UIMENU_WM_AUTOUPDATE);
    }
}

/*
=======================
idClientMainSystemLocal::AddToLimboChat
=======================
*/
void idClientMainSystemLocal::AddToLimboChat(pointer str) {
    sint i, len, chatHeight;
    valueType lastcolor[10];
    valueType *p, * ls;

    chatHeight = LIMBOCHAT_HEIGHT;
    cl.limboChatPos = LIMBOCHAT_HEIGHT - 1;
    len = 0;

    // copy old strings
    for(i = cl.limboChatPos; i > 0; i--) {
        Q_strcpy_s(cl.limboChatMsgs[i], cl.limboChatMsgs[i - 1]);
    }

    // copy new string
    p = cl.limboChatMsgs[0];
    *p = 0;

    Q_strncpyz(lastcolor, "^7\0", sizeof(lastcolor));

    ls = nullptr;

    while(*str) {
        if(len > LIMBOCHAT_WIDTH - 1) {
            break;
        }

        if(Q_IsColorString(str)) {
            sint j;
            const sint color_string_length = Q_ColorStringLength(str);

            for(j = 0; j < color_string_length; j++) {
                lastcolor[j] = *str;
                *p++ = *str++;
            }

            lastcolor[j] = '\0';
            continue;
        }

        if(*str == ' ') {
            ls = p;
        }

        *p++ = *str++;
        len++;
    }

    *p = 0;
}

/*
=======================
idClientMainSystemLocal::AddToLimboChat
=======================
*/
void idClientMainSystemLocal::LogPrintf(fileHandle_t fileHandle,
                                        pointer fmt, ...) {
    va_list argptr;
    valueType string[1024] = { 0 };
    uint64 len;
    time_t rawtime;
    time(&rawtime);

    strftime(string, sizeof(string), "[%Y-%m-%d] [%H:%M:%S] ",
             localtime(&rawtime));

    len = ::strlen(string);

    va_start(argptr, fmt);
    Q_vsprintf_s(string + len, sizeof(string) - len, fmt,
                 argptr);
    va_end(argptr);

    if(!fileHandle) {
        return;
    }

    fileSystem->Write(string, strlen(string), fileHandle);
}