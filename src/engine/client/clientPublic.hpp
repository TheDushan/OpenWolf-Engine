////////////////////////////////////////////////////////////////////////////////////////
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
// File name:   clientPublic.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: primary header for client
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTPUBLIC_HPP__
#define __CLIENTPUBLIC_HPP__

#if !defined ( DEDICATED ) && !defined ( UPDATE_SERVER ) && !defined ( BSPC )
#endif

#define RETRANSMIT_TIMEOUT  3000    // time between connection packet retransmits

#define LIMBOCHAT_WIDTH     140 // NERVE - SMF - NOTE TTimo buffer size indicator, not related to screen bbox
#define LIMBOCHAT_HEIGHT    7   // NERVE - SMF

// snapshots are a view of the server at a given time
typedef struct {
    bool
    valid;                          // cleared if delta parsing was invalid
    sint
    snapFlags;                     // rate delayed and dropped commands
    sint
    serverTime;                        // server time the message is valid for (in msec)
    sint
    messageNum;                        // copied from netchan->incoming_sequence
    sint
    deltaNum;                      // messageNum the delta is from
    sint
    ping;                          // time from when cmdNum-1 was sent to time packet was reeceived
    uchar8
    areamask[MAX_MAP_AREA_BYTES]; // portalarea visibility bits
    sint
    cmdNum;                            // the next cmdNum the server is expecting
    playerState_t
    ps;                             // complete information about the current player at this time
    sint
    numEntities;                   // all of the entities that need to be presented
    sint
    parseEntitiesNum;              // at the time of this snapshot
    sint
    serverCommandNum;              // execute all commands up to this before
    // making the snapshot current
} clSnapshot_t;

// the parseEntities array must be large enough to hold PACKET_BACKUP frames of
// entities, so that when a delta compressed message arives from the server
// it can be un-deltad from the original

// Dushan
// MAX_GENTITIES is defined as '1<<GENTITYNUM_BITS' which equates to 1024.
// And because of that reason we increased that 4 times (old limit was already 2k)
#define MAX_PARSE_ENTITIES ( PACKET_BACKUP * MAX_GENTITIES * 2 )

// Arnout: for double tapping
typedef struct {
    sint        pressedTime[DT_NUM];
    sint        releasedTime[DT_NUM];
    sint        lastdoubleTap;
} doubleTap_t;

/*
=============================================================================

the clientActive_t structure is wiped completely at every
new gamestate_t, potentially several times during an established connection

=============================================================================
*/

typedef struct {
    sint             p_cmdNumber;   // cl.cmdNumber when packet was sent
    sint             p_serverTime;  // usercmd->serverTime when packet was sent
    sint             p_realtime;        // cls.realtime when packet was sent
} outPacket_t;


// the parseEntities array must be large enough to hold PACKET_BACKUP frames of
// entities, so that when a delta compressed message arives from the server
// it can be un-deltad from the original

// Dushan
// MAX_GENTITIES is defined as '1<<GENTITYNUM_BITS' which equates to 1024.
// And because of that reason we increased that 4 times (old limit was already 2k)
#define MAX_PARSE_ENTITIES ( PACKET_BACKUP * MAX_GENTITIES * 2 )

extern sint      g_console_field_width;

typedef struct {
    sint
    timeoutcount;                                              // it requres several frames in a timeout condition

    // to disconnect, preventing debugging breaks from
    // causing immediate disconnects on continue
    clSnapshot_t
    snapServer;                                                     // latest received from server
    sint
    serverTime;                                                    // may be paused during play
    sint
    oldServerTime;                                             // to prevent time from flowing bakcwards
    sint
    oldFrameServerTime;                                            // to check tournament restarts
    sint
    serverTimeDelta;                                           // cl.serverTime = cls.realtime + cl.serverTimeDelta
    // this value changes as net lag varies
    bool
    extrapolatedSnapshot;                                       // set if any cgame frame has been forced to extrapolate
    // cleared when CL_AdjustTimeDelta looks at it
    bool
    newSnapshots;                                               // set on parse of any valid packet
    gameState_t
    gameState;                                                  // configstrings
    valueType
    mapname[MAX_QPATH];                                            // extracted from CS_SERVERINFO
    sint
    parseEntitiesNum;                                          // index (not anded off) into cl_parse_entities[]
    sint             mouseDx[2],
                     mouseDy[2];                                        // added to by mouse events
    sint             mouseIndex;
    sint
    joystickAxis[MAX_JOYSTICK_AXIS];                           // set by joystick events

    // cgame communicates a few values to the client system
    sint
    cgameUserCmdValue;                                         // current weapon to add to usercmd_t
    sint
    cgameFlags;                                                    // flags that can be set by the gamecode
    float32           cgameSensitivity;
    sint
    cgameMpIdentClient;                                            // NERVE - SMF
    vec3_t
    cgameClientLerpOrigin;                                      // DHM - Nerve

    // cmds[cmdNumber] is the predicted command, [cmdNumber-1] is the last
    // properly generated command
    usercmd_t
    cmds[CMD_BACKUP];                                           // each mesage will send several old cmds
    sint
    cmdNumber;                                                 // incremented each frame, because multiple

    // frames may need to be packed into a single packet
    // Arnout: double tapping
    doubleTap_t     doubleTap;
    outPacket_t
    outPackets[PACKET_BACKUP];                                  // information about each packet we have sent out

    // the client maintains its own idea of view angles, which are
    // sent to the server each frame.  It is cleared to 0 upon entering each level.
    // the server sends a delta each frame which is added to the locally
    // tracked view angles to account for standing on rotating objects,
    // and teleport direction changes
    vec3_t          viewangles;
    sint
    serverId;                                                  // included in each client message so the server

    // can tell if it is for a prior map_restart
    // big stuff at end of structure so most offsets are 15 bits or less
    clSnapshot_t    snapshots[PACKET_BACKUP];
    entityState_t
    entityBaselines[MAX_GENTITIES];                             // for delta compression when not in previous frame
    entityState_t   parseEntities[MAX_PARSE_ENTITIES];

    // NERVE - SMF
    // NOTE TTimo - UI uses LIMBOCHAT_WIDTH strings (140),
    // but for the processing in idClientMainSystemLocal::AddToLimboChat we need some safe room
    valueType            limboChatMsgs[LIMBOCHAT_HEIGHT][LIMBOCHAT_WIDTH * 3 +
            1];
    sint             limboChatPos;
    bool        corruptedTranslationFile;
    valueType            translationVersion[MAX_STRING_TOKENS];
    // -NERVE - SMF

    bool        cameraMode;
} clientActive_t;

extern clientActive_t cl;

/*
=============================================================================

the clientConnection_t structure is wiped when disconnecting from a server,
either to go to a full screen console, play a demo, or connect to a different server

A connection can be to either a server through the network layer or a
demo through a file.

=============================================================================
*/

typedef struct {
    sint                        clientNum;
    sint
    lastPacketSentTime;                                         // for retransmits during connection
    sint
    lastPacketTime;                                             // for timeouts
    netadr_t                serverAddress;
    sint
    connectTime;                                                // for connection retransmits
    sint
    connectPacketCount;                                         // for display on connection dialog
    valueType
    serverMessage[MAX_STRING_TOKENS];                           // for display on connection dialog
    sint
    challenge;                                                  // from the server to use for connecting
    sint
    checksumFeed;                                               // from the server for checksum calculations
    sint
    onlyVisibleClients;                                         // DHM - Nerve
    // these are our reliable messages that go to the server
    sint                        reliableSequence;
    sint
    reliableAcknowledge;                                        // the last one the server has executed
    // TTimo - NOTE: incidentally, reliableCommands[0] is never used (always start at reliableAcknowledge+1)
    valueType
    reliableCommands[MAX_RELIABLE_COMMANDS][MAX_TOKEN_CHARS];
    // server message (unreliable) and command (reliable) sequence
    // numbers are NOT cleared at level changes, but continue to
    // increase as long as the connection is valid
    // message sequence is used by both the network layer and the
    // delta compression layer
    sint                        serverMessageSequence;
    // reliable messages received from server
    sint                        serverCommandSequence;
    sint
    lastExecutedServerCommand;                                  // last server command grabbed or executed with idClientGameSystemLocal::GetServerCommand
    valueType
    serverCommands[MAX_RELIABLE_COMMANDS][MAX_TOKEN_CHARS];
    // file transfer from server
    fileHandle_t            download;
    sint                        downloadNumber;
    sint
    downloadBlock;                                              // block we are waiting for
    sint
    downloadCount;                                              // how many bytes we got
    sint
    downloadSize;                                               // how many bytes we got
    sint
    downloadFlags;                                              // misc download behaviour flags sent by the server
    valueType
    downloadList[MAX_INFO_STRING];                              // list of paks we need to download

    // www downloading
    bool
    bWWWDl;                                                     // we have a www download going
    bool
    bWWWDlAborting;                                             // disable the CL_WWWDownload until server gets us a gamestate (used for aborts)
    valueType
    redirectedList[MAX_INFO_STRING];                            // list of files that we downloaded through a redirect since last FS_ComparePaks
    valueType
    badChecksumList[MAX_INFO_STRING];                           // list of files for which wwwdl redirect is broken (wrong checksum)
    valueType                   newsString[MAX_NEWS_STRING];

    // demo information
    valueType                   demoName[MAX_QPATH];
    bool                demorecording;
    bool                demoplaying;
    bool
    demowaiting;                                                // don't record until a non-delta message is received
    bool                firstDemoFrameSkipped;
    fileHandle_t            demofile;

    bool                waverecording;
    fileHandle_t            wavefile;
    sint                        wavetime;

    sint                        timeDemoFrames; // counter of rendered frames
    sint
    timeDemoStart;  // cls.realtime before first frame
    sint
    timeDemoBaseTime;   // each frame will be at this time + frameNum * 50

    // big stuff at end of structure so most offsets are 15 bits or less
    netchan_t               netchan;
} clientConnection_t;

extern clientConnection_t clc;

/*
==================================================================

the clientStatic_t structure is never wiped, and is used even when
no client connection is active at all

==================================================================
*/

typedef struct {
    netadr_t        adr;
    sint             start;
    sint             time;
    valueType            info[MAX_INFO_STRING];
} ping_t;

typedef struct {
    netadr_t        adr;
    valueType            hostName[MAX_NAME_LENGTH];
    sint             load;
    valueType            mapName[MAX_NAME_LENGTH];
    valueType            game[MAX_NAME_LENGTH];
    sint             netType;
    sint             gameType;
    sint             clients;
    sint             maxClients;
    sint             minPing;
    sint             maxPing;
    sint             ping;
    bool        visible;
    sint             allowAnonymous;
    sint             friendlyFire;              // NERVE - SMF
    sint             maxlives;                  // NERVE - SMF
    sint             needpass;
    sint             antilag;                   // TTimo
    sint             weaprestrict;
    sint             balancedteams;
    valueType            gameName[MAX_NAME_LENGTH]; // Arnout
} serverInfo_t;

typedef struct {
    connstate_t
    state;                                                          // connection status
    sint
    keyCatchers;                                                   // bit flags
    bool
    doCachePurge;                                                   // Arnout: empty the renderer cache as soon as possible
    valueType
    servername[MAX_OSPATH];                                            // name of server from original connect (used by reconnect)
    // when the server clears the hunk, all of these must be restarted
    bool            rendererStarted;
    bool            soundStarted;
    bool            soundRegistered;
    bool            uiStarted;
    bool            cgameStarted;
    sint             framecount;
    sint
    frametime;                                                     // msec since last frame
    sint
    realtime;                                                      // ignores pause
    sint
    realFrametime;                                                 // ignoring pause, so console always works
    sint            lastTimeDraw;
    sint             numlocalservers;
    serverInfo_t    localServers[MAX_OTHER_SERVERS];
    sint             numglobalservers;
    serverInfo_t    globalServers[MAX_GLOBAL_SERVERS];
    // additional global servers
    sint             numGlobalServerAddresses;
    netadr_t        globalServerAddresses[MAX_GLOBAL_SERVERS];
    sint             numfavoriteservers;
    serverInfo_t    favoriteServers[MAX_OTHER_SERVERS];
    sint
    pingUpdateSource;                                              // source currently pinging or updating
    sint             masterNum;
    // update server info
    netadr_t        updateServer;
    valueType            updateChallenge[MAX_TOKEN_CHARS];
    valueType            updateInfoString[MAX_INFO_STRING];
    netadr_t        authorizeServer;
    sint authorizeAuthCookie;
    // DHM - Nerve :: Auto-update Info
    valueType
    autoupdateServerNames[MAX_AUTOUPDATE_SERVERS][MAX_QPATH];
    netadr_t        autoupdateServer;
    bool        autoUpdateServerChecked[MAX_AUTOUPDATE_SERVERS];
    sint
    autoupdatServerFirstIndex;                                     // to know when we went through all of them
    sint
    autoupdatServerIndex;                                          // to cycle through them
    // rendering info
    vidconfig_t      glconfig;
    qhandle_t       charSetShader;
    qhandle_t       whiteShader;
    qhandle_t       consoleShader;
    qhandle_t
    consoleShader2;                                                 // NERVE - SMF - merged from WolfSP
    bool        useLegacyConsoleFont;
    fontInfo_t      consoleFont;
    // www downloading
    // in the static stuff since this may have to survive server disconnects
    // if new stuff gets added, idClientDownloadSystemLocal::ClearStaticDownload code needs to be updated for clear up
    bool
    bWWWDlDisconnected;                                             // keep going with the download after server disconnect
    valueType            downloadName[MAX_OSPATH];
    valueType
    downloadTempName[MAX_OSPATH];                                  // in wwwdl mode, this is OS path (it's a qpath otherwise)
    valueType
    originalDownloadName[MAX_QPATH];                               // if we get a redirect, keep a copy of the original file path
    bool
    downloadRestart;                                                // if true, we need to do another FS_Restart because we downloaded a pak
    sint lastVidRestart;
    struct {
        fileHandle_t    chat;
    } log;
} clientStatic_t;

extern clientStatic_t cls;

#endif //!__CLIENTPUBLIC_HPP__
