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
// File name:   cl_main.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: client main loop
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

convar_t         *cl_wavefilerecord;
convar_t         *cl_nodelta;
convar_t         *cl_debugMove;

convar_t         *cl_noprint;
convar_t         *cl_motd;
convar_t         *cl_autoupdate;    // DHM - Nerve

convar_t         *rcon_client_password;
convar_t         *rconAddress;

convar_t         *cl_timeout;
convar_t         *cl_maxpackets;
convar_t         *cl_packetdup;
convar_t         *cl_timeNudge;
convar_t         *cl_showTimeDelta;
convar_t         *cl_freezeDemo;

convar_t         *cl_shownet =
    nullptr; // NERVE - SMF - This is referenced in msg.c and we need to make sure it is nullptr
convar_t         *cl_shownuments;   // DHM - Nerve
convar_t         *cl_showSend;
convar_t         *cl_showServerCommands;    // NERVE - SMF
convar_t         *cl_timedemo;

convar_t         *cl_aviFrameRate;
convar_t         *cl_forceavidemo;

convar_t         *cl_freelook;
convar_t         *cl_sensitivity;
convar_t         *cl_xbox360ControllerAvailable;

convar_t         *cl_mouseAccelOffset;
convar_t         *cl_mouseAccel;
convar_t         *cl_mouseAccelStyle;
convar_t         *cl_showMouseRate;

convar_t         *m_pitch;
convar_t         *m_yaw;
convar_t         *m_forward;
convar_t         *m_side;
convar_t         *m_filter;

convar_t           *j_pitch;
convar_t           *j_yaw;
convar_t           *j_forward;
convar_t           *j_side;
convar_t         *j_up;
convar_t           *j_pitch_axis;
convar_t           *j_yaw_axis;
convar_t           *j_forward_axis;
convar_t           *j_side_axis;
convar_t         *j_up_axis;

convar_t         *cl_activeAction;

convar_t         *cl_autorecord;

convar_t         *cl_motdString;

convar_t         *cl_allowDownload;
convar_t         *cl_wwwDownload;
convar_t         *cl_conXOffset;
convar_t         *cl_inGameVideo;

convar_t         *cl_serverStatusResendTime;
convar_t         *cl_trn;
convar_t         *cl_missionStats;
convar_t         *cl_waitForFire;

// NERVE - SMF - localization
convar_t         *cl_language;
convar_t         *cl_debugTranslation;

// -NERVE - SMF
// DHM - Nerve :: Auto-Update
convar_t         *cl_updateavailable;
convar_t         *cl_updatefiles;

// DHM - Nerve

convar_t         *cl_profile;
convar_t         *cl_defaultProfile;

convar_t         *cl_demorecording; // fretn
convar_t         *cl_demofilename;  // bani
convar_t         *cl_demooffset;    // bani

convar_t         *cl_waverecording; //bani
convar_t         *cl_wavefilename;  //bani
convar_t         *cl_waveoffset;    //bani

convar_t         *cl_packetloss;    //bani
convar_t         *cl_packetdelay;   //bani
//convar_t*         sv_cheats;      //bani

convar_t         *cl_consoleKeys;
convar_t         *cl_consoleFont;
convar_t         *cl_consoleFontSize;
convar_t         *cl_consoleFontKerning;
convar_t           *cl_consolePrompt;

convar_t         *cl_gamename;
convar_t         *cl_altTab;

convar_t         *cl_aviMotionJpeg;
convar_t         *cl_guidServerUniq;

convar_t *cl_guid;

clientActive_t  cl;
clientConnection_t clc;
clientStatic_t  cls;
void           *cgvm;

netadr_t rcon_address;

// DHM - Nerve :: Have we heard from the auto-update server this session?
bool        autoupdateChecked;
bool        autoupdateStarted;

// TTimo : moved from char* to array (was getting the valueType* from va(), broke on big downloads)
valueType            autoupdateFilename[MAX_QPATH];

// "updates" shifted from -7
#define AUTOUPDATE_DIR "ni]Zm^l"
#define AUTOUPDATE_DIR_SHIFT 7

extern void     SV_BotFrame(sint time);
void            CL_CheckForResend(void);
void            CL_ShowIP_f(void);

// fretn
void            CL_WriteWaveClose(void);
void            CL_WavStopRecord_f(void);

void CL_PurgeCache(void) {
    cls.doCachePurge = true;
}

void CL_DoPurgeCache(void) {
    if(!cls.doCachePurge) {
        return;
    }

    cls.doCachePurge = false;

    if(!com_cl_running) {
        return;
    }

    if(!com_cl_running->integer) {
        return;
    }

    if(!cls.rendererStarted) {
        return;
    }

    //Dushan - FIX ME
    //renderSystem->purgeCache();
}

/*
=======================================================================

CLIENT RELIABLE COMMAND COMMUNICATION

=======================================================================
*/

/*
======================
CL_AddReliableCommand

The given command will be transmitted to the server, and is gauranteed to
not have future usercmd_t executed before it is executed
======================
*/
void CL_AddReliableCommand(pointer cmd) {
    sint             index;

    // if we would be losing an old command that hasn't been acknowledged,
    // we must drop the connection
    if(clc.reliableSequence - clc.reliableAcknowledge >
            MAX_RELIABLE_COMMANDS) {
        Com_Error(ERR_DROP, "Client command overflow");
    }

    clc.reliableSequence++;
    index = clc.reliableSequence & (MAX_RELIABLE_COMMANDS - 1);
    Q_strncpyz(clc.reliableCommands[index], cmd,
               sizeof(clc.reliableCommands[index]));
}

/*
======================
CL_ChangeReliableCommand
======================
*/
void CL_ChangeReliableCommand(void) {
    sint             r, index, l;

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
=======================================================================

CLIENT SIDE DEMO RECORDING

=======================================================================
*/

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length
====================
*/
void CL_WriteDemoMessage(msg_t *msg, sint headerBytes) {
    sint             len, swlen;

    // write the packet sequence
    len = clc.serverMessageSequence;
    swlen = LittleLong(len);
    fileSystem->Write(&swlen, 4, clc.demofile);

    // skip the packet sequencing information
    len = msg->cursize - headerBytes;
    swlen = LittleLong(len);
    fileSystem->Write(&swlen, 4, clc.demofile);
    fileSystem->Write(msg->data + headerBytes, len, clc.demofile);
}


/*
====================
CL_StopRecording_f

stop recording a demo
====================
*/
void CL_StopRecord_f(void) {
    sint             len;

    if(!clc.demorecording) {
        Com_Printf("Not recording a demo.\n");
        return;
    }

    // finish up
    len = -1;
    fileSystem->Write(&len, 4, clc.demofile);
    fileSystem->Write(&len, 4, clc.demofile);
    fileSystem->FCloseFile(clc.demofile);
    clc.demofile = 0;

    clc.demorecording = false;
    cvarSystem->Set("cl_demorecording", "0");    // fretn
    cvarSystem->Set("cl_demofilename", "");      // bani
    cvarSystem->Set("cl_demooffset", "0");   // bani
    Com_Printf("Stopped demo.\n");
}

/*
==================
CL_DemoFilename
==================
*/
void CL_DemoFilename(valueType *buf, sint bufSize) {
    time_t rawtime;

    // should really only reach ~19 chars
    valueType timeStr[32] = { 0 };

    ::time(&rawtime);

    // or gmtime
    ::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d_%H-%M-%S",
               localtime(&rawtime));

    Q_vsprintf_s(buf, bufSize, bufSize, "demo%s", timeStr);
}

/*
====================
CL_Record_f

record <demoname>

Begins recording a demo from the current position
====================
*/

static valueType     demoName[MAX_QPATH];   // compiler bug workaround
void CL_Record_f(void) {
    valueType            name[MAX_OSPATH], extension[32];
    sint             len;
    valueType           *s;

    if(cmdSystem->Argc() > 2) {
        Com_Printf("record <demoname>\n");
        return;
    }

    if(clc.demorecording) {
        Com_Printf("Already recording.\n");
        return;
    }

    if(cls.state != CA_ACTIVE) {
        Com_Printf("You must be in a level to record.\n");
        return;
    }

    // ATVI Wolfenstein Misc #479 - changing this to a warning
    // sync 0 doesn't prevent recording, so not forcing it off .. everyone does g_sync 1 ; record ; g_sync 0 ..
    if(networkSystem->IsLocalAddress(clc.serverAddress) &&
            !cvarSystem->VariableValue("g_synchronousClients")) {
        Com_Printf(S_COLOR_YELLOW
                   "WARNING: You should set 'g_synchronousClients 1' for smoother demo recording\n");
    }

    if(cmdSystem->Argc() == 2) {
        s = cmdSystem->Argv(1);
        Q_strncpyz(demoName, s, sizeof(demoName));
        Q_vsprintf_s(name, sizeof(name), sizeof(name), "dm_%d", PROTOCOL_VERSION);
    } else {
        // timestamp the file
        CL_DemoFilename(demoName, sizeof(demoName));

        Q_vsprintf_s(name, sizeof(name), sizeof(name), "demos/%s.%s", demoName,
                     extension);

        if(fileSystem->FileExists(name)) {
            Com_Printf("Record: Couldn't create a file\n");
            return;
        }
    }

    CL_Record(name);
}

void CL_Record(pointer name) {
    sint             i;
    msg_t           buf;
    uchar8            bufData[MAX_MSGLEN];
    entityState_t  *ent;
    entityState_t   nullstate;
    valueType           *s;
    sint             len;

    // open the demo file

    Com_Printf("recording to %s.\n", name);
    clc.demofile = fileSystem->FOpenFileWrite(name);

    if(!clc.demofile) {
        Com_Printf("ERROR: couldn't open.\n");
        return;
    }

    clc.demorecording = true;
    cvarSystem->Set("cl_demorecording", "1");    // fretn
    Q_strncpyz(clc.demoName, demoName, sizeof(clc.demoName));
    cvarSystem->Set("cl_demofilename", clc.demoName);    // bani
    cvarSystem->Set("cl_demooffset", "0");   // bani

    // don't start saving messages until a non-delta compressed message is received
    clc.demowaiting = true;

    // write out the gamestate message
    MSG_Init(&buf, bufData, sizeof(bufData));
    MSG_Bitstream(&buf);

    // NOTE, MRE: all server->client messages now acknowledge
    MSG_WriteLong(&buf, clc.reliableSequence);

    MSG_WriteByte(&buf, svc_gamestate);
    MSG_WriteLong(&buf, clc.serverCommandSequence);

    // configstrings
    for(i = 0; i < MAX_CONFIGSTRINGS; i++) {
        if(!cl.gameState.stringOffsets[i]) {
            continue;
        }

        s = cl.gameState.stringData + cl.gameState.stringOffsets[i];
        MSG_WriteByte(&buf, svc_configstring);
        MSG_WriteShort(&buf, i);
        MSG_WriteBigString(&buf, s);
    }

    // baselines
    ::memset(&nullstate, 0, sizeof(nullstate));

    for(i = 0; i < MAX_GENTITIES; i++) {
        ent = &cl.entityBaselines[i];

        if(!ent->number) {
            continue;
        }

        MSG_WriteByte(&buf, svc_baseline);
        MSG_WriteDeltaEntity(&buf, &nullstate, ent, true);
    }

    MSG_WriteByte(&buf, svc_EOF);

    // finished writing the gamestate stuff

    // write the client num
    MSG_WriteLong(&buf, clc.clientNum);
    // write the checksum feed
    MSG_WriteLong(&buf, clc.checksumFeed);

    // finished writing the client packet
    MSG_WriteByte(&buf, svc_EOF);

    // write it to the demo file
    len = LittleLong(clc.serverMessageSequence - 1);
    fileSystem->Write(&len, 4, clc.demofile);

    len = LittleLong(buf.cursize);
    fileSystem->Write(&len, 4, clc.demofile);
    fileSystem->Write(buf.data, buf.cursize, clc.demofile);

    // the rest of the demo file will be copied from net messages
    CL_AddReliableCommand("getstatus");
}

/*
=======================================================================

CLIENT SIDE DEMO PLAYBACK

=======================================================================
*/

/*
=================
CL_DemoCompleted
=================
*/

void CL_DemoCompleted(void) {
    if(cl_timedemo && cl_timedemo->integer) {
        sint             time;

        time = idsystem->Milliseconds() - clc.timeDemoStart;

        if(time > 0) {
            Com_Printf("%i frames, %3.1f seconds: %3.1f fps\n", clc.timeDemoFrames,
                       time / 1000.0, clc.timeDemoFrames * 1000.0 / time);
        }
    }

    // fretn
    if(clc.waverecording) {
        CL_WriteWaveClose();
        clc.waverecording = false;
    }

    CL_Disconnect_f();
    soundSystem->StopAllSounds();

    CL_NextDemo();

}

/*
=================
CL_ReadDemoMessage
=================
*/

void CL_ReadDemoMessage(void) {
    sint             r;
    msg_t           buf;
    uchar8            bufData[MAX_MSGLEN];
    sint             s;

    if(!clc.demofile) {
        CL_DemoCompleted();
        return;
    }

    // get the sequence number
    r = fileSystem->Read(&s, 4, clc.demofile);

    if(r != 4) {
        CL_DemoCompleted();
        return;
    }

    clc.serverMessageSequence = LittleLong(s);

    // init the message
    MSG_Init(&buf, bufData, sizeof(bufData));

    // get the length
    r = fileSystem->Read(&buf.cursize, 4, clc.demofile);

    if(r != 4) {
        CL_DemoCompleted();
        return;
    }

    buf.cursize = LittleLong(buf.cursize);

    if(buf.cursize == -1) {
        CL_DemoCompleted();
        return;
    }

    if(buf.cursize > buf.maxsize) {
        Com_Error(ERR_DROP, "CL_ReadDemoMessage: demoMsglen > MAX_MSGLEN");
    }

    r = fileSystem->Read(buf.data, buf.cursize, clc.demofile);

    if(r != buf.cursize) {
        Com_Printf("Demo file was truncated.\n");
        CL_DemoCompleted();
        return;
    }

    clc.lastPacketTime = cls.realtime;
    buf.readcount = 0;

    idClientParseSystemLocal::ParseServerMessage(&buf);
}

/*
====================

  Wave file saving functions

  FIXME: make this actually work

====================
*/

/*
==================
CL_DemoFilename
==================
*/
void CL_WavFilename(sint number, valueType *fileName) {
    if(number < 0 || number > 9999) {
        Q_vsprintf_s(fileName, MAX_OSPATH, MAX_OSPATH,
                     "wav9999");   // fretn - removed .tga
        return;
    }

    Q_vsprintf_s(fileName, MAX_OSPATH, MAX_OSPATH, "wav%04i", number);
}

typedef struct wav_hdr_s {
    uint    ChunkID;    // big endian
    uint    ChunkSize;  // little endian
    uint    Format;     // big endian

    uint    Subchunk1ID;    // big endian
    uint    Subchunk1Size;  // little endian
    uchar16  AudioFormat;   // little endian
    uchar16  NumChannels;   // little endian
    uint    SampleRate; // little endian
    uint    ByteRate;   // little endian
    uchar16  BlockAlign;    // little endian
    uchar16  BitsPerSample; // little endian

    uint    Subchunk2ID;    // big endian
    uint    Subchunk2Size;  // little indian ;)

    uint    NumSamples;
} wav_hdr_t;

wav_hdr_t       hdr;

static void CL_WriteWaveHeader(void) {
    memset(&hdr, 0, sizeof(hdr));

    hdr.ChunkID = 0x46464952;   // "RIFF"
    hdr.ChunkSize = 0;          // total filesize - 8 bytes
    hdr.Format = 0x45564157;    // "WAVE"

    hdr.Subchunk1ID = 0x20746d66;   // "fmt "
    hdr.Subchunk1Size = 16;     // 16 = pcm
    hdr.AudioFormat = 1;        // 1 = linear quantization
    hdr.NumChannels = 2;        // 2 = stereo

    hdr.SampleRate = dma.speed;

    hdr.BitsPerSample = 16;     // 16bits

    // SampleRate * NumChannels * BitsPerSample/8
    hdr.ByteRate = hdr.SampleRate * hdr.NumChannels * (hdr.BitsPerSample / 8);

    // NumChannels * BitsPerSample/8
    hdr.BlockAlign = hdr.NumChannels * (hdr.BitsPerSample / 8);

    hdr.Subchunk2ID = 0x61746164;   // "data"

    hdr.Subchunk2Size = 0;      // NumSamples * NumChannels * BitsPerSample/8

    // ...
    fileSystem->Write(&hdr.ChunkID, 44, clc.wavefile);
}

static valueType     wavName[MAX_QPATH];    // compiler bug workaround
void CL_WriteWaveOpen(void) {
    // we will just save it as a 16bit stereo 22050kz pcm file

    valueType            name[MAX_OSPATH];
    sint             len;
    valueType           *s;

    if(cmdSystem->Argc() > 2) {
        Com_Printf("wav_record <wavname>\n");
        return;
    }

    if(clc.waverecording) {
        Com_Printf("Already recording a wav file\n");
        return;
    }

    // yes ... no ? leave it up to them imo
    //if (cl_avidemo.integer)
    //  return;

    if(cmdSystem->Argc() == 2) {
        s = cmdSystem->Argv(1);
        Q_strncpyz(wavName, s, sizeof(wavName));
        Q_vsprintf_s(name, sizeof(name), sizeof(name), "wav/%s.wav", wavName);
    } else {
        sint             number;

        // I STOLE THIS
        for(number = 0; number <= 9999; number++) {
            CL_WavFilename(number, wavName);
            Q_vsprintf_s(name, sizeof(name), sizeof(name), "wav/%s.wav", wavName);

            len = fileSystem->FileExists(name);

            if(len <= 0) {
                break;          // file doesn't exist
            }
        }
    }

    Com_Printf("recording to %s.\n", name);
    clc.wavefile = fileSystem->FOpenFileWrite(name);

    if(!clc.wavefile) {
        Com_Printf("ERROR: couldn't open %s for writing.\n", name);
        return;
    }

    CL_WriteWaveHeader();
    clc.wavetime = -1;

    clc.waverecording = true;

    cvarSystem->Set("cl_waverecording", "1");
    cvarSystem->Set("cl_wavefilename", wavName);
    cvarSystem->Set("cl_waveoffset", "0");
}

void CL_WriteWaveClose(void) {
    Com_Printf("Stopped recording\n");

    hdr.Subchunk2Size = hdr.NumSamples * hdr.NumChannels *
                        (hdr.BitsPerSample / 8);
    hdr.ChunkSize = 36 + hdr.Subchunk2Size;

    fileSystem->Seek(clc.wavefile, 4, FS_SEEK_SET);
    fileSystem->Write(&hdr.ChunkSize, 4, clc.wavefile);
    fileSystem->Seek(clc.wavefile, 40, FS_SEEK_SET);
    fileSystem->Write(&hdr.Subchunk2Size, 4, clc.wavefile);

    // and we're outta here
    fileSystem->FCloseFile(clc.wavefile);
    clc.wavefile = 0;
}

/*
====================
CL_CompleteDemoName
====================
*/
static void CL_CompleteDemoName(valueType *args, sint argNum) {
    if(argNum == 2) {
        valueType demoExt[ 16 ];

        Q_vsprintf_s(demoExt, sizeof(demoExt), sizeof(demoExt), ".dm_%d",
                     PROTOCOL_VERSION);
        cmdCompletionSystem->CompleteFilename("demos", demoExt, true);
    }
}

/*
====================
CL_PlayDemo_f

demo <demoname>
====================
*/
void CL_PlayDemo_f(void) {
    sint prot_ver;
    valueType name[MAX_OSPATH], extension[32], *arg;

    if(cmdSystem->Argc() < 2) {
        Com_Printf("playdemo <demo name>\n");
        return;
    }

    // make sure a local server is killed
    cvarSystem->Set("sv_killserver", "1");

    arg = cmdSystem->Args();

    CL_Disconnect(true, "Playing demo");

    //CL_FlushMemory();   //----(SA)  MEM NOTE: in missionpack, this is moved to CL_DownloadsComplete

    // open the demo file
    Q_vsprintf_s(extension, sizeof(extension), sizeof(extension), ".dm_%d",
                 PROTOCOL_VERSION);

    if(!Q_stricmp(arg + strlen(arg) - strlen(extension), extension)) {
        Q_vsprintf_s(name, sizeof(name), sizeof(name), "demos/%s", arg);
    } else {
        Q_vsprintf_s(name, sizeof(name), sizeof(name), "demos/%s.dm_%d", arg,
                     PROTOCOL_VERSION);
    }

    fileSystem->FOpenFileRead(name, &clc.demofile, true);

    if(!clc.demofile) {
        if(!Q_stricmp(arg, "(null)")) {
            Com_Error(ERR_DROP, "No demo selected.", name);
        } else {
            Com_Error(ERR_DROP, "couldn't open %s", name);
        }

        return;
    }

    Q_strncpyz(clc.demoName, cmdSystem->Argv(1), sizeof(clc.demoName));

    Con_Close();

    cls.state = CA_CONNECTED;
    clc.demoplaying = true;

    if(cvarSystem->VariableValue("cl_wavefilerecord")) {
        CL_WriteWaveOpen();
    }

    Q_strncpyz(cls.servername, cmdSystem->Argv(1), sizeof(cls.servername));

    // read demo messages until connected
    while(cls.state >= CA_CONNECTED && cls.state < CA_PRIMED) {
        CL_ReadDemoMessage();
    }

    // don't get the first snapshot this frame, to prevent the long
    // time from the gamestate load from messing causing a time skip
    clc.firstDemoFrameSkipped = false;
    //  if (clc.waverecording) {
    //      CL_WriteWaveClose();
    //      clc.waverecording = false;
    //  }
}

/*
==================
CL_NextDemo

Called when a demo or cinematic finishes
If the "nextdemo" cvar is set, that command will be issued
==================
*/
void CL_NextDemo(void) {
    valueType            v[MAX_STRING_CHARS];

    Q_strncpyz(v, cvarSystem->VariableString("nextdemo"), sizeof(v));
    v[MAX_STRING_CHARS - 1] = 0;
    Com_DPrintf("CL_NextDemo: %s\n", v);

    if(!v[0]) {
        CL_FlushMemory();
        return;
    }

    cvarSystem->Set("nextdemo", "");
    cmdBufferSystem->AddText(v);
    cmdBufferSystem->AddText("\n");
    cmdBufferSystem->Execute();
}

/*
==================
CL_DemoName

Returns the name of the demo
==================
*/
void CL_DemoName(valueType *buffer, sint size) {
    if(clc.demoplaying || clc.demorecording) {
        Q_strncpyz(buffer, clc.demoName, size);
    } else if(size >= 1) {
        buffer[ 0 ] = '\0';
    }
}


//======================================================================

/*
=====================
CL_ShutdownAll
=====================
*/
void CL_ShutdownAll(void) {
    // clear sounds
    soundSystem->DisableSounds();

    // download subsystem
    downloadSystem->Shutdown();

    // shutdown CGame
    clientGameSystem->ShutdownCGame();

    // shutdown GUI
    clientGUISystem->ShutdownGUI();

    // shutdown the renderer
    renderSystem->Shutdown(false);

    //if( renderSystem->purgeCache )
    //{
    //CL_DoPurgeCache();
    //}

    cls.uiStarted = false;
    cls.cgameStarted = false;
    cls.rendererStarted = false;
    cls.soundRegistered = false;

    // Gordon: stop recording on map change etc, demos aren't valid over map changes anyway
    if(clc.demorecording) {
        CL_StopRecord_f();
    }

    if(clc.waverecording) {
        CL_WavStopRecord_f();
    }
}

/*
=================
CL_FlushMemory

Called by CL_MapLoading, CL_Connect_f, CL_PlayDemo_f, and CL_ParseGamestate the only
ways a client gets into a game
Also called by Com_Error
=================
*/
void CL_FlushMemory(void) {
    // shutdown all the client stuff
    CL_ShutdownAll();

    // if not running a server clear the whole hunk
    if(!com_sv_running->integer) {
        // clear the whole hunk
        Hunk_Clear();
        // clear collision map data
        collisionModelManager->ClearMap();
    } else {
        // clear all the client data on the hunk
        Hunk_ClearToMark();
    }

    CL_StartHunkUsers();
}

/*
=====================
CL_MapLoading

A local server is starting to load a map, so update the
screen to let the user know about it, then dump all client
memory on the hunk from cgame, ui, and renderer
=====================
*/
void CL_MapLoading(void) {
    if(!com_cl_running->integer) {
        return;
    }

    Con_Close();
    cls.keyCatchers = 0;

    // if we are already connected to the local host, stay connected
    if(cls.state >= CA_CONNECTED && !Q_stricmp(cls.servername, "localhost")) {
        cls.state = CA_CONNECTED;       // so the connect screen is drawn
        memset(cls.updateInfoString, 0, sizeof(cls.updateInfoString));
        memset(clc.serverMessage, 0, sizeof(clc.serverMessage));
        memset(&cl.gameState, 0, sizeof(cl.gameState));
        clc.lastPacketSentTime = -9999;
        clientScreenSystem->UpdateScreen();
    } else {
        // clear nextmap so the cinematic shutdown doesn't execute it
        cvarSystem->Set("nextmap", "");
        CL_Disconnect(true,  "Loading map");
        Q_strncpyz(cls.servername, "localhost", sizeof(cls.servername));
        cls.state = CA_CHALLENGING;     // so the connect screen is drawn
        cls.keyCatchers = 0;
        clientScreenSystem->UpdateScreen();
        clc.connectTime = -RETRANSMIT_TIMEOUT;
        networkChainSystem->StringToAdr(cls.servername, &clc.serverAddress,
                                        NA_UNSPEC);
        // we don't need a challenge on the localhost

        CL_CheckForResend();
    }
}

/*
=====================
CL_ClearState

Called before parsing a gamestate
=====================
*/
void CL_ClearState(void) {
    ::memset(&cl, 0, sizeof(cl));
}

/*
====================
CL_UpdateGUID

update cl_guid using ETKEY_FILE and optional prefix
====================
*/
static void CL_UpdateGUID(pointer prefix, uint64 prefix_len) {
    fileHandle_t    f;
    sint             len;

    len = fileSystem->SV_FOpenFileRead(GUIDKEY_FILE, &f);
    fileSystem->FCloseFile(f);

    if(len != GUIDKEY_SIZE) {
        cvarSystem->Set("cl_guid", "");
    } else {
        cvarSystem->Set("cl_guid", MD5System->MD5File(GUIDKEY_FILE, GUIDKEY_SIZE,
                        prefix, prefix_len));
    }
}
/*
=====================
CL_ClearStaticDownload
Clear download information that we keep in cls (disconnected download support)
=====================
*/
void CL_ClearStaticDownload(void) {
    assert(!cls.bWWWDlDisconnected);     // reset before calling
    cls.downloadRestart = false;
    cls.downloadTempName[0] = '\0';
    cls.downloadName[0] = '\0';
    cls.originalDownloadName[0] = '\0';
}

/*
=====================
CL_Disconnect

Called when a connection, demo, or cinematic is being terminated.
Goes from a connected state to either a menu state or a console state
Sends a disconnect message to the server
This is also called on Com_Error and Com_Quit, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect(bool showMainMenu, pointer reason) {
    if(!com_cl_running || !com_cl_running->integer) {
        return;
    }

    // shutting down the client so enter full screen ui mode
    cvarSystem->Set("r_uiFullScreen", "1");

    // Stop demo recording
    if(clc.demorecording) {
        CL_StopRecord_f();
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

        CL_AddReliableCommand(pCmd);
        CL_WritePacket();
        CL_WritePacket();
        CL_WritePacket();
    }

    CL_ClearState();

    // wipe the client connection
    ::memset(&clc, 0, sizeof(clc));

    if(!cls.bWWWDlDisconnected) {
        CL_ClearStaticDownload();
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

    CL_UpdateGUID(nullptr, 0);
}

/*
===================
CL_ForwardCommandToServer

adds the current command line as a clientCommand
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void CL_ForwardCommandToServer(pointer string) {
    valueType           *cmd;

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
        Com_Printf("Unknown command \"%s\"\n", cmd);
        return;
    }

    if(cmdSystem->Argc() > 1) {
        CL_AddReliableCommand(string);
    } else {
        CL_AddReliableCommand(cmd);
    }
}

/*
==================
CL_OpenUrl_f
==================
*/
void CL_OpenUrl_f(void) {
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
===================
CL_RequestMotd

===================
*/
void CL_RequestMotd(void) {
    valueType       info[MAX_INFO_STRING];

    if(!cl_motd->integer) {
        return;
    }

    Com_DPrintf("Resolving %s\n", MASTER_SERVER_NAME);

    switch(networkChainSystem->StringToAdr(MASTER_SERVER_NAME,
                                           &cls.updateServer, NA_UNSPEC)) {
        case 0:
            Com_Printf("Couldn't resolve master address\n");
            return;

        case 2:
            cls.updateServer.port = BigShort(PORT_MASTER);

        default:
            break;
    }

    Com_DPrintf("%s resolved to %s\n", MASTER_SERVER_NAME,
                networkSystem->AdrToStringwPort(cls.updateServer));

    info[0] = 0;

    Q_vsprintf_s(cls.updateChallenge, sizeof(cls.updateChallenge),
                 sizeof(cls.updateChallenge), "%i",
                 ((rand() << 16) ^ rand()) ^ Com_Milliseconds());

    Info_SetValueForKey(info, "challenge", cls.updateChallenge);
    Info_SetValueForKey(info, "renderer", cls.glconfig.renderer_string);
    Info_SetValueForKey(info, "version", com_version->string);

    networkChainSystem->OutOfBandPrint(NS_CLIENT, cls.updateServer,
                                       "getmotd%s", info);
}

/*
===================
CL_RequestAuthorization
===================
*/
void CL_RequestAuthorization(void) {
    sint i, j, l;

    if(!cls.authorizeServer.port) {
        Com_DPrintf("Resolving %s\n", AUTHORIZE_SERVER_NAME);

        if(!networkChainSystem->StringToAdr(AUTHORIZE_SERVER_NAME,
                                            &cls.authorizeServer, NA_UNSPEC)) {
            Com_DPrintf("Couldn't resolve authorization server address\n");
            return;
        }

        cls.authorizeServer.port = BigShort(PORT_AUTHORIZE);
        Com_DPrintf("%s resolved to %i.%i.%i.%i:%i\n", AUTHORIZE_SERVER_NAME,
                    cls.authorizeServer.ip[0], cls.authorizeServer.ip[1],
                    cls.authorizeServer.ip[2], cls.authorizeServer.ip[3],
                    BigShort(cls.authorizeServer.port));
    }

    if(cls.authorizeServer.type == NA_BAD) {
        return;
    }

    networkChainSystem->OutOfBandPrint(NS_CLIENT, cls.authorizeServer,
                                       "getKeyAuthorize %i", cls.authorizeAuthCookie);
}

/*
======================================================================

CONSOLE COMMANDS

======================================================================
*/

/*
==================
CL_ForwardToServer_f
==================
*/
void CL_ForwardToServer_f(void) {
    if(cls.state != CA_ACTIVE || clc.demoplaying) {
        Com_Printf("Not connected to a server.\n");
        return;
    }

    // don't forward the first argument
    if(cmdSystem->Argc() > 1) {
        CL_AddReliableCommand(cmdSystem->Args());
    }
}

/*
==================
CL_Setenv_f

Mostly for controlling voodoo environment variables
==================
*/
void CL_Setenv_f(void) {
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
CL_Disconnect_f
==================
*/
void CL_Disconnect_f(void) {
    clientCinemaSystem->StopCinematic();
    cvarSystem->Set("savegame_loading", "0");
    cvarSystem->Set("g_reloading", "0");

    if(cls.state != CA_DISCONNECTED && cls.state != CA_CINEMATIC) {
        if(cmdSystem->Argc() > 1) {
            valueType reason[MAX_STRING_CHARS] = { 0 };
            Q_strncpyz(reason, cmdSystem->Argv(1), sizeof(reason));
            Q_strstrip(reason, "\r\n;\"", nullptr);
            CL_Disconnect(true, reason);
        } else {
            CL_Disconnect(true, nullptr);
        }
    }
}


/*
================
CL_Reconnect_f

================
*/
void CL_Reconnect_f(void) {
    if(!strlen(cls.servername) || !strcmp(cls.servername, "localhost")) {
        Com_Printf("Can't reconnect to localhost.\n");
        return;
    }

    cmdBufferSystem->AddText(va("connect %s\n", cls.servername));
}

/*
================
CL_Connect_f

================
*/
void CL_Connect_f(void) {
    valueType          *server;
    pointer     serverString;
    sint                argc = cmdSystem->Argc();
    netadrtype_t    family = NA_UNSPEC;

    if(argc != 2 && argc != 3) {
        Com_Printf("usage: connect [-4|-6] server\n");
        return;
    }

    if(argc == 2) {
        server = cmdSystem->Argv(1);
    } else {
        if(!strcmp(cmdSystem->Argv(1), "-4")) {
            family = NA_IP;
        } else if(!strcmp(cmdSystem->Argv(1), "-6")) {
            family = NA_IP6;
        } else {
            Com_Printf("warning: only -4 or -6 as address type understood.\n");
        }

        server = cmdSystem->Argv(2);
    }

    soundSystem->StopAllSounds();      // NERVE - SMF

    // starting to load a map so we get out of full screen ui mode
    cvarSystem->Set("r_uiFullScreen", "0");
    cvarSystem->Set("ui_connecting", "1");

    // fire a message off to the motd server
    CL_RequestMotd();

    // clear any previous "server full" type messages
    clc.serverMessage[0] = 0;

    if(com_sv_running->integer && !strcmp(server, "localhost")) {
        // if running a local server, kill it
        serverInitSystem->Shutdown("Server quit\n");
    }

    // make sure a local server is killed
    cvarSystem->Set("sv_killserver", "1");
    serverMainSystem->Frame(0);

    CL_Disconnect(true,  "Joining another server");
    Con_Close();

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
        CL_UpdateGUID(serverString, strlen(serverString));
    } else {
        CL_UpdateGUID(nullptr, 0);
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
=====================
CL_Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
#define MAX_RCON_MESSAGE 1024
void CL_Rcon_f(void) {
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
=================
CL_SendPureChecksums
=================
*/
void CL_SendPureChecksums(void) {
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

    CL_AddReliableCommand(cMsg);
}

/*
=================
CL_ResetPureClientAtServer
=================
*/
void CL_ResetPureClientAtServer(void) {
    CL_AddReliableCommand(va("vdr"));
}

/*
=================
CL_Vid_Restart_f

Restart the video subsystem

we also have to reload the UI and CGame because the renderer
doesn't know what graphics to reload
=================
*/

void CL_Vid_Restart_f(void) {
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
    CL_ShutdownRef();

    // client is no longer pure untill new checksums are sent
    CL_ResetPureClientAtServer();

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
    if(!com_sv_running->integer) {
        //collisionModelManager->ClearMap();
        // clear the whole hunk
        Hunk_Clear();
    } else {
        // clear all the client data on the hunk
        Hunk_ClearToMark();
    }

    // initialize the renderer interface
    CL_InitRef();

    // startup all the client stuff
    CL_StartHunkUsers();

#ifdef _WIN32
    idsystem->Restart_f();
#endif

    // start the cgame if connected
    if(cls.state > CA_CONNECTED && cls.state != CA_CINEMATIC) {
        cls.cgameStarted = true;
        clientGameSystem->InitCGame();

        // send pure checksums
        CL_SendPureChecksums();
    }
}

/*
=================
CL_UI_Restart_f

Restart the ui subsystem
=================
*/
void CL_UI_Restart_f(void) {
    // NERVE - SMF
    // shutdown the GUI
    clientGUISystem->ShutdownGUI();

    autoupdateChecked = false;

    // init the UI
    clientGUISystem->InitGUI();
}

/*
=================
CL_Snd_Reload_f

Reloads sounddata from disk, retains soundhandles.
=================
*/
void CL_Snd_Reload_f(void) {
    soundSystem->Reload();
}


/*
=================
CL_Snd_Restart_f

Restart the sound subsystem
The cgame and game must also be forced to restart because
handles will be invalid
=================
*/
void CL_Snd_Restart_f(void) {
    soundSystem->Shutdown();
    soundSystem->Init();

    CL_Vid_Restart_f();
}


/*
==================
CL_PK3List_f
==================
*/
void CL_OpenedPK3List_f(void) {
    Com_Printf("Opened PK3 Names: %s\n", fileSystem->LoadedPakNames());
}

/*
==================
CL_PureList_f
==================
*/
void CL_ReferencedPK3List_f(void) {
    Com_Printf("Referenced PK3 Names: %s\n", fileSystem->ReferencedPakNames());
}

/*
==================
CL_Configstrings_f
==================
*/
void CL_Configstrings_f(void) {
    sint             i;
    sint             ofs;

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
CL_Clientinfo_f
==============
*/
void CL_Clientinfo_f(void) {
    Com_Printf("--------- Client Information ---------\n");
    Com_Printf("state: %i\n", cls.state);
    Com_Printf("Server: %s\n", cls.servername);
    Com_Printf("User info settings:\n");
    Info_Print(cvarSystem->InfoString(CVAR_USERINFO));
    Com_Printf("--------------------------------------\n");
}

/*
==============
CL_WavRecord_f
==============
*/

void CL_WavRecord_f(void) {
    if(clc.wavefile) {
        Com_Printf("Already recording a wav file\n");
        return;
    }

    CL_WriteWaveOpen();
}

/*
==============
CL_WavStopRecord_f
==============
*/

void CL_WavStopRecord_f(void) {
    if(!clc.wavefile) {
        Com_Printf("Not recording a wav file\n");
        return;
    }

    CL_WriteWaveClose();
    cvarSystem->Set("cl_waverecording", "0");
    cvarSystem->Set("cl_wavefilename", "");
    cvarSystem->Set("cl_waveoffset", "0");
    clc.waverecording = false;
}


// XreaL BEGIN =======================================================

/*
===============
CL_Video_f

video
video [filename]
===============
*/
void CL_Video_f(void) {
    valueType            filename[MAX_OSPATH];
    sint             i, last;

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
CL_StopVideo_f
===============
*/
void CL_StopVideo_f(void) {
    clientAVISystem->CloseAVI();
}

/*
=================
CL_DownloadsComplete

Called when all downloading has been completed
=================
*/
void CL_DownloadsComplete(void) {
#ifndef _WIN32
    valueType    *fs_write_path;
#endif
    valueType    *fn;


    // DHM - Nerve :: Auto-update (not finished yet)
    if(autoupdateStarted) {

        if(autoupdateFilename[0] && (strlen(autoupdateFilename) > 4)) {
#ifdef _WIN32
            // win32's Sys_StartProcess prepends the current dir
            fn = va("%s/%s", fileSystem->ShiftStr(AUTOUPDATE_DIR,
                                                  AUTOUPDATE_DIR_SHIFT), autoupdateFilename);
#else
            fs_write_path = cvarSystem->VariableString("fs_homepath");
            fn = fileSystem->BuildOSPath(fs_write_path,
                                         fileSystem->ShiftStr(AUTOUPDATE_DIR, AUTOUPDATE_DIR_SHIFT),
                                         autoupdateFilename);
#ifdef __LINUX__
            idsystem->Chmod(fn, S_IXUSR);
#endif
#endif
            idsystem->StartProcess(fn, true);
        }

        autoupdateStarted = false;
        CL_Disconnect(true, "Downloads complete");
        return;
    }

    // if we downloaded files we need to restart the file system
    if(cls.downloadRestart) {
        cls.downloadRestart = false;

        fileSystem->Restart(
            clc.checksumFeed);   // We possibly downloaded a pak, restart the file system to load it

        if(!cls.bWWWDlDisconnected) {
            // inform the server so we get new gamestate info
            CL_AddReliableCommand("donedl");
        }

        // we can reset that now
        cls.bWWWDlDisconnected = false;
        CL_ClearStaticDownload();

        // by sending the donedl command we request a new gamestate
        // so we don't want to load stuff yet
        return;
    }

    if(cls.bWWWDlDisconnected) {
        cls.bWWWDlDisconnected = false;
        CL_ClearStaticDownload();
        return;
    }

    // let the client game init and load data
    cls.state = CA_LOADING;

    // Pump the loop, this may change gamestate!
    Com_EventLoop();

    // if the gamestate was changed by calling Com_EventLoop
    // then we loaded everything already and we don't want to do it again.
    if(cls.state != CA_LOADING) {
        return;
    }

    // starting to load a map so we get out of full screen ui mode
    cvarSystem->Set("r_uiFullScreen", "0");

    // flush client memory and start loading stuff
    // this will also (re)load the UI
    // if this is a local client then only the client part of the hunk
    // will be cleared, note that this is done after the hunk mark has been set
    CL_FlushMemory();

    // initialize the CGame
    cls.cgameStarted = true;
    clientGameSystem->InitCGame();

    // set pure checksums
    CL_SendPureChecksums();

    CL_WritePacket();
    CL_WritePacket();
    CL_WritePacket();
}

/*
=================
CL_BeginDownload

Requests a file to download from the server.  Stores it in the current
game directory.
=================
*/
void CL_BeginDownload(pointer localName, pointer remoteName) {

    Com_DPrintf("***** CL_BeginDownload *****\n"
                "Localname: %s\n" "Remotename: %s\n" "****************************\n",
                localName, remoteName);

    Q_strncpyz(cls.downloadName, localName, sizeof(cls.downloadName));
    Q_vsprintf_s(cls.downloadTempName, sizeof(cls.downloadTempName),
                 sizeof(cls.downloadTempName), "%s.tmp", localName);

    // Set so UI gets access to it
    cvarSystem->Set("cl_downloadName", remoteName);
    cvarSystem->Set("cl_downloadSize", "0");
    cvarSystem->Set("cl_downloadCount", "0");
    cvarSystem->SetValue("cl_downloadTime", cls.realtime);

    clc.downloadBlock = 0;      // Starting new file
    clc.downloadCount = 0;

    CL_AddReliableCommand(va("download %s", remoteName));
}

/*
=================
CL_NextDownload

A download completed or failed
=================
*/
void CL_NextDownload(void) {
    valueType           *s;
    valueType           *remoteName, *localName;

    // We are looking to start a download here
    if(*clc.downloadList) {
        s = clc.downloadList;

        // format is:
        //  @remotename@localname@remotename@localname, etc.

        if(*s == '@') {
            s++;
        }

        remoteName = s;

        if((s = strchr(s, '@')) == nullptr) {
            CL_DownloadsComplete();
            return;
        }

        *s++ = 0;
        localName = s;

        if((s = strchr(s, '@')) != nullptr) {
            *s++ = 0;
        } else {
            s = localName + strlen(localName);   // point at the nul byte

        }

        CL_BeginDownload(localName, remoteName);

        cls.downloadRestart = true;

        // move over the rest
        memmove(clc.downloadList, s, strlen(s) + 1);

        return;
    }

    CL_DownloadsComplete();
}

/*
=================
CL_InitDownloads

After receiving a valid game state, we valid the cgame and local zip files here
and determine if we need to download them
=================
*/
void CL_InitDownloads(void) {
#ifndef PRE_RELEASE_DEMO
    valueType            missingfiles[1024];
    valueType           *dir = fileSystem->ShiftStr(AUTOUPDATE_DIR,
                               AUTOUPDATE_DIR_SHIFT);

    // TTimo
    // init some of the www dl data
    clc.bWWWDl = false;
    clc.bWWWDlAborting = false;
    cls.bWWWDlDisconnected = false;
    CL_ClearStaticDownload();

    if(autoupdateStarted &&
            networkSystem->CompareAdr(cls.autoupdateServer, clc.serverAddress)) {
        if(strlen(cl_updatefiles->string) > 4) {
            Q_strncpyz(autoupdateFilename, cl_updatefiles->string,
                       sizeof(autoupdateFilename));
            Q_strncpyz(clc.downloadList, va("@%s/%s@%s/%s", dir,
                                            cl_updatefiles->string, dir, cl_updatefiles->string),
                       MAX_INFO_STRING);
            cls.state = CA_CONNECTED;
            CL_NextDownload();
            return;
        }
    } else {
        // whatever autodownlad configuration, store missing files in a cvar, use later in the ui maybe
        if(fileSystem->ComparePaks(missingfiles, sizeof(missingfiles), false)) {
            cvarSystem->Set("com_missingFiles", missingfiles);
        } else {
            cvarSystem->Set("com_missingFiles", "");
        }

        // reset the redirect checksum tracking
        clc.redirectedList[0] = '\0';

        if(cl_allowDownload->integer &&
                fileSystem->ComparePaks(clc.downloadList, sizeof(clc.downloadList),
                                        true)) {
            // this gets printed to UI, i18n
            Com_Printf(clientLocalizationSystem->TranslateStringBuf("Need paks: %s\n"),
                       clc.downloadList);

            if(*clc.downloadList) {
                // if autodownloading is not enabled on the server
                cls.state = CA_CONNECTED;
                CL_NextDownload();
                return;
            }
        }
    }

#endif

    CL_DownloadsComplete();
}

/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out
=================
*/
void CL_CheckForResend(void) {
    sint             port, i;
    valueType            info[MAX_INFO_STRING];
    valueType            data[MAX_INFO_STRING];

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
                CL_RequestAuthorization();
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
            Com_Error(ERR_FATAL, "CL_CheckForResend: bad cls.state");
    }
}

/*
===================
CL_DisconnectPacket

Sometimes the server can drop the client and the netchan based
disconnect can be lost.  If the client continues to send packets
to the server, the server will send out of band disconnect packets
to the client so it doesn't have to wait for the full timeout period.
===================
*/
void CL_DisconnectPacket(netadr_t from) {
    pointer     message;

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
        message = "Server disconnected for unknown reason";
        Com_Printf("%s", message);
        cvarSystem->Set("com_errorMessage", message);
        CL_Disconnect(true, "Disconnect packet");
    } else {
        CL_Disconnect(false, "Disconnect packet");
        cvarSystem->Set("ui_connecting", "1");
        cvarSystem->Set("ui_dl_running", "1");
    }
}


valueType *str_replace(pointer string, pointer substr,
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
CL_AuthPacket
===================
*/
void CL_AuthPacket(netadr_t from) {
    sint challenge;
    uint type;
    valueType *msg;

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

/*
===================
CL_MotdPacket
===================
*/
void CL_MotdPacket(netadr_t from, pointer info) {
    pointer v;
    valueType *w;

    // if not from our server, ignore it
    if(!networkSystem->CompareAdr(from, cls.updateServer)) {
        Com_DPrintf("MOTD packet from unexpected source\n");
        return;
    }

    Com_DPrintf("MOTD packet: %s\n", info);

    while(*info != '\\') {
        info++;
    }

    // check challenge
    v = Info_ValueForKey(info, "challenge");

    if(strcmp(v, cls.updateChallenge)) {
        Com_DPrintf("MOTD packet mismatched challenge: "
                    "'%s' != '%s'\n", v, cls.updateChallenge);
        return;
    }

    v = Info_ValueForKey(info, "motd");
    w = str_replace(v, "|", "\n");

    Q_strncpyz(cls.updateInfoString, info, sizeof(cls.updateInfoString));
    cvarSystem->Set("cl_newsString", w);
    free(w);
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
void CL_PrintPacket(netadr_t from, msg_t *msg) {
    valueType           *s;

    s = MSG_ReadBigString(msg);

    if(!Q_stricmpn(s, "[err_dialog]", 12)) {
        Q_strncpyz(clc.serverMessage, s + 12, sizeof(clc.serverMessage));
        //cvarSystem->Set("com_errorMessage", clc.serverMessage );
        Com_Error(ERR_DROP, "%s", clc.serverMessage);
    } else if(!Q_stricmpn(s, "[err_prot]", 10)) {
        Q_strncpyz(clc.serverMessage, s + 10, sizeof(clc.serverMessage));
        //cvarSystem->Set("com_errorMessage", clientLocalizationSystem->TranslateStringBuf( PROTOCOL_MISMATCH_ERROR_LONG ) );
        Com_Error(ERR_DROP, "%s",
                  clientLocalizationSystem->TranslateStringBuf(
                      PROTOCOL_MISMATCH_ERROR_LONG));
    } else if(!Q_stricmpn(s, "[err_update]", 12)) {
        Q_strncpyz(clc.serverMessage, s + 12, sizeof(clc.serverMessage));
        Com_Error(ERR_AUTOUPDATE, "%s", clc.serverMessage);
    } else if(!Q_stricmpn(s, "ET://", 5)) {
        // fretn
        Q_strncpyz(clc.serverMessage, s, sizeof(clc.serverMessage));
        cvarSystem->Set("com_errorMessage", clc.serverMessage);
        Com_Error(ERR_DROP, "%s", clc.serverMessage);
    } else {
        Q_strncpyz(clc.serverMessage, s, sizeof(clc.serverMessage));
    }

    Com_Printf("%s", clc.serverMessage);
}

/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
void CL_ConnectionlessPacket(netadr_t from, msg_t *msg) {
    valueType *s, *c;
    sint challenge = 0;

    MSG_BeginReadingOOB(msg);
    MSG_ReadLong(msg);      // skip the -1

    s = MSG_ReadStringLine(msg);

    cmdSystem->TokenizeString(s);

    c = cmdSystem->Argv(0);

    Com_DPrintf("CL packet %s: %s\n", networkSystem->AdrToStringwPort(from),
                c);

    if(!Q_stricmp(c, "disconnectResponse")) {
        if(cls.state == CA_CONNECTED) {
            CL_Disconnect(true, "Disconnect response");
        }

        return;
    }

    // challenge from the server we are connecting to
    if(!Q_stricmp(c, "challengeResponse")) {
        if(cls.state != CA_CONNECTING) {
            Com_Printf("Unwanted challenge response received.  Ignored.\n");
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
                Com_DPrintf("Challenge response received from unexpected source. Ignored.\n");
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
        Com_DPrintf("challengeResponse: %d\n", clc.challenge);
        return;
    }

    // server connection
    if(!Q_stricmp(c, "connectResponse")) {
        if(cls.state >= CA_CONNECTED) {
            Com_Printf("Dup connect received.  Ignored.\n");
            return;
        }

        if(cls.state != CA_CHALLENGING) {
            Com_Printf("connectResponse packet while not connecting.  Ignored.\n");
            return;
        }

        if(!networkSystem->CompareAdr(from, clc.serverAddress)) {
            Com_Printf("connectResponse from a different address.  Ignored.\n");
            Com_Printf("%s should have been %s\n", networkSystem->AdrToString(from),
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
        CL_DisconnectPacket(from);
        return;
    }

    // Auth check
    if(!Q_stricmp(c, "authStatus")) {
        CL_AuthPacket(from);
        return;
    }

    // global MOTD from id
    if(!Q_stricmp(c, "motd")) {
        CL_MotdPacket(from, s);
        return;
    }

    // echo request from server
    if(!Q_stricmp(c, "print")) {
        if(networkSystem->CompareAdr(from, clc.serverAddress) ||
                networkSystem->CompareAdr(from, rcon_address)) {
            CL_PrintPacket(from, msg);
        }

        return;
    }

    // DHM - Nerve :: Auto-update server response message
    if(!Q_stricmp(c, "updateResponse")) {
        CL_UpdateInfoPacket(from);
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

    Com_DPrintf("Unknown connectionless packet command.\n");
}


/*
=================
CL_PacketEvent

A packet has arrived from the main event loop
=================
*/
void CL_PacketEvent(netadr_t from, msg_t *msg) {
    sint headerBytes;

    if(msg->cursize >= 4 && *reinterpret_cast<sint *>(msg->data) == -1) {
        CL_ConnectionlessPacket(from, msg);
        return;
    }

    clc.lastPacketTime = cls.realtime;

    if(cls.state < CA_CONNECTED) {
        return;     // can't be a valid sequenced packet
    }

    if(msg->cursize < 4) {
        Com_Printf("%s: Runt packet\n", networkSystem->AdrToStringwPort(from));
        return;
    }

    //
    // packet from server
    //
    if(!networkSystem->CompareAdr(from, clc.netchan.remoteAddress)) {
        Com_DPrintf("%s:sequenced packet without connection\n"
                    , networkSystem->AdrToStringwPort(from));
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
        CL_WriteDemoMessage(msg, headerBytes);
    }
}

/*
==================
CL_CheckTimeout

==================
*/
void CL_CheckTimeout(void) {
    //
    // check timeout
    //
    if((!cl_paused->integer || !sv_paused->integer)
            && cls.state >= CA_CONNECTED && cls.state != CA_CINEMATIC &&
            cls.realtime - clc.lastPacketTime > cl_timeout->value * 1000) {
        if(++cl.timeoutcount > 5) {
            // timeoutcount saves debugger
            cvarSystem->Set("com_errorMessage", "Server connection timed out.");
            CL_Disconnect(true, "server timed out");
            return;
        }
    } else {
        cl.timeoutcount = 0;
    }
}


//============================================================================

/*
==================
CL_CheckUserinfo

==================
*/
void CL_CheckUserinfo(void) {
    // don't add reliable commands when not yet connected
    if(cls.state < CA_CHALLENGING) {
        return;
    }

    // don't overflow the reliable command buffer when paused on a local server
    if(cl_paused->integer && sv_paused->integer && com_sv_running->integer) {
        return;
    }

    // send a reliable userinfo update if needed
    if(cvar_modifiedFlags & CVAR_USERINFO) {
        cvar_modifiedFlags &= ~CVAR_USERINFO;
        CL_AddReliableCommand(va("userinfo \"%s\"",
                                 cvarSystem->InfoString(CVAR_USERINFO)));
    }
}

/*
==================
CL_WWWDownload
==================
*/
void CL_WWWDownload(void) {
    valueType           *to_ospath;
    dlStatus_t      ret;
    static bool bAbort = false;

    if(clc.bWWWDlAborting) {
        if(!bAbort) {
            Com_DPrintf("CL_WWWDownload: WWWDlAborting\n");
            bAbort = true;
        }

        return;
    }

    if(bAbort) {
        Com_DPrintf("CL_WWWDownload: WWWDlAborting done\n");
        bAbort = false;
    }

    ret = downloadSystem->DownloadLoop();

    if(ret == DL_CONTINUE) {
        return;
    }

    if(ret == DL_DONE) {
        // taken from CL_ParseDownload
        // we work with OS paths
        clc.download = 0;
        to_ospath = fileSystem->BuildOSPath(
                        cvarSystem->VariableString("fs_homepath"), cls.originalDownloadName, "");
        to_ospath[strlen(to_ospath) - 1] = '\0';

        if(rename(cls.downloadTempName, to_ospath)) {
            fileSystem->FSCopyFile(cls.downloadTempName, to_ospath);
            remove(cls.downloadTempName);
        }

        *cls.downloadTempName = *cls.downloadName = 0;
        cvarSystem->Set("cl_downloadName", "");

        if(cls.bWWWDlDisconnected) {
            // for an auto-update in disconnected mode, we'll be spawning the setup in CL_DownloadsComplete
            if(!autoupdateStarted) {
                // reconnect to the server, which might send us to a new disconnected download
                cmdBufferSystem->ExecuteText(EXEC_APPEND, "reconnect\n");
            }
        } else {
            CL_AddReliableCommand("wwwdl done");

            // tracking potential web redirects leading us to wrong checksum - only works in connected mode
            if(strlen(clc.redirectedList) + strlen(cls.originalDownloadName) + 1 >=
                    sizeof(clc.redirectedList)) {
                // just to be safe
                Com_Printf("ERROR: redirectedList overflow (%s)\n", clc.redirectedList);
            } else {
                strcat(clc.redirectedList, "@");
                strcat(clc.redirectedList, cls.originalDownloadName);
            }
        }
    } else {
        if(cls.bWWWDlDisconnected) {
            // in a connected download, we'd tell the server about failure and wait for a reply
            // but in this case we can't get anything from server
            // if we just reconnect it's likely we'll get the same disconnected download message, and error out again
            // this may happen for a regular dl or an auto update
            pointer     error = va("Download failure while getting '%s'\n",
                                   cls.downloadName);  // get the msg before clearing structs

            cls.bWWWDlDisconnected =
                false; // need clearing structs before ERR_DROP, or it goes into endless reload
            CL_ClearStaticDownload();
            Com_Error(ERR_DROP, "%s", error);
        } else {
            // see CL_ParseDownload, same abort strategy
            Com_Printf("Download failure while getting '%s'\n", cls.downloadName);
            CL_AddReliableCommand("wwwdl fail");
            clc.bWWWDlAborting = true;
        }

        return;
    }

    clc.bWWWDl = false;
    CL_NextDownload();
}

/*
==================
CL_WWWBadChecksum

FS code calls this when doing fileSystem->ComparePaks
we can detect files that we got from a www dl redirect with a wrong checksum
this indicates that the redirect setup is broken, and next dl attempt should NOT redirect
==================
*/
bool CL_WWWBadChecksum(pointer pakname) {
    if(strstr(clc.redirectedList, va("@%s", pakname))) {
        Com_Printf("WARNING: file %s obtained through download redirect has wrong checksum\n",
                   pakname);
        Com_Printf("         this likely means the server configuration is broken\n");

        if(strlen(clc.badChecksumList) + strlen(pakname) + 1 >= sizeof(
                    clc.badChecksumList)) {
            Com_Printf("ERROR: badChecksumList overflowed (%s)\n",
                       clc.badChecksumList);
            return false;
        }

        strcat(clc.badChecksumList, "@");
        strcat(clc.badChecksumList, pakname);
        Com_DPrintf("bad checksums: %s\n", clc.badChecksumList);
        return true;
    }

    return false;
}

/*
==================
CL_Frame
==================
*/
void CL_Frame(sint msec) {
    if(!com_cl_running->integer) {
        return;
    }

    if(cls.realtime >= cls.lastTimeDraw + 1) {
        cls.lastTimeDraw = cls.realtime;
    }

    if(cls.state == CA_DISCONNECTED && !(cls.keyCatchers & KEYCATCH_UI) &&
            !com_sv_running->integer && clc.demoplaying && uivm) {
        // if disconnected, bring up the menu
        soundSystem->StopAllSounds();
        uiManager->SetActiveMenu(UIMENU_MAIN);
    }

    // if recording an avi, lock to a fixed fps
    if(clientAVISystem->VideoRecording() && cl_aviFrameRate->integer && msec) {
        if(!clientAVISystem->VideoRecording()) {
            CL_Video_f();
        }

        // save the current screen
        if(cls.state == CA_ACTIVE || cl_forceavidemo->integer) {
            clientAVISystem->TakeVideoFrame();

            // fixed time for next frame'
            msec = static_cast<sint>(ceil((1000.0f / cl_aviFrameRate->value) *
                                          com_timescale->value));

            if(msec == 0) {
                msec = 1;
            }
        }
    }

    // save the msec before checking pause
    cls.realFrametime = msec;

    // decide the simulation time
    cls.frametime = msec;

    cls.realtime += cls.frametime;

    if(cl_timegraph->integer) {
        clientScreenSystem->DebugGraph(cls.realFrametime * 0.25f, 0);
    }

    // see if we need to update any userinfo
    CL_CheckUserinfo();

    // if we haven't gotten a packet in a long time,
    // drop the connection
    CL_CheckTimeout();

    // wwwdl download may survive a server disconnect
    if((cls.state == CA_CONNECTED && clc.bWWWDl) || cls.bWWWDlDisconnected) {
        CL_WWWDownload();
    }

    // send intentions now
    CL_SendCmd();

    // resend a connection request if necessary
    CL_CheckForResend();

    // decide on the serverTime to render
    clientGameSystem->SetCGameTime();

    // update the screen
    clientScreenSystem->UpdateScreen();

    // update the sound
    soundSystem->Update();

    // advance local effects for next frame
    clientCinemaSystem->RunCinematic();

    Con_RunConsole();

    cls.framecount++;
}


//============================================================================
// Ridah, startup-caching system
typedef struct {
    valueType            name[MAX_QPATH];
    sint             hits;
    sint             lastSetIndex;
} cacheItem_t;

enum cacheGroup_t {
    CACHE_SOUNDS,
    CACHE_MODELS,
    CACHE_IMAGES,

    CACHE_NUMGROUPS
};

static cacheItem_t cacheGroups[CACHE_NUMGROUPS] = {
    {{'s', 'o', 'u', 'n', 'd', 0}, CACHE_SOUNDS},
    {{'m', 'o', 'd', 'e', 'l', 0}, CACHE_MODELS},
    {{'i', 'm', 'a', 'g', 'e', 0}, CACHE_IMAGES},
};

#define MAX_CACHE_ITEMS     4096
#define CACHE_HIT_RATIO     0.75    // if hit on this percentage of maps, it'll get cached

static sint      cacheIndex;
static cacheItem_t cacheItems[CACHE_NUMGROUPS][MAX_CACHE_ITEMS];

static void CL_Cache_StartGather_f(void) {
    cacheIndex = 0;
    memset(cacheItems, 0, sizeof(cacheItems));

    cvarSystem->Set("cl_cacheGathering", "1");
}

static void CL_Cache_UsedFile_f(void) {
    valueType            groupStr[MAX_QPATH];
    valueType            itemStr[MAX_QPATH];
    sint             i, group;
    cacheItem_t    *item;

    if(cmdSystem->Argc() < 2) {
        Com_Error(ERR_DROP, "usedfile without enough parameters\n");
        return;
    }

    Q_strcpy_s(groupStr, cmdSystem->Argv(1));

    Q_strcpy_s(itemStr, cmdSystem->Argv(2));

    for(i = 3; i < cmdSystem->Argc(); i++) {
        strcat(itemStr, " ");
        strcat(itemStr, cmdSystem->Argv(i));
    }

    Q_strlwr(itemStr);

    // find the cache group
    for(i = 0; i < CACHE_NUMGROUPS; i++) {
        if(!Q_strncmp(groupStr, cacheGroups[i].name, MAX_QPATH)) {
            break;
        }
    }

    if(i == CACHE_NUMGROUPS) {
        Com_Error(ERR_DROP, "usedfile without a valid cache group\n");
        return;
    }

    // see if it's already there
    group = i;

    for(i = 0, item = cacheItems[group]; i < MAX_CACHE_ITEMS; i++, item++) {
        if(!item->name[0]) {
            // didn't find it, so add it here
            Q_strncpyz(item->name, itemStr, MAX_QPATH);

            if(cacheIndex > 9999) {
                // hack, but yeh
                item->hits = cacheIndex;
            } else {
                item->hits++;
            }

            item->lastSetIndex = cacheIndex;
            break;
        }

        if(item->name[0] == itemStr[0] &&
                !Q_strncmp(item->name, itemStr, MAX_QPATH)) {
            if(item->lastSetIndex != cacheIndex) {
                item->hits++;
                item->lastSetIndex = cacheIndex;
            }

            break;
        }
    }
}

static void CL_Cache_SetIndex_f(void) {
    if(cmdSystem->Argc() < 2) {
        Com_Error(ERR_DROP, "setindex needs an index\n");
        return;
    }

    cacheIndex = atoi(cmdSystem->Argv(1));
}

static void CL_Cache_MapChange_f(void) {
    cacheIndex++;
}

static void CL_Cache_EndGather_f(void) {
    // save the frequently used files to the cache list file
    sint             i, j, handle, cachePass;
    valueType            filename[MAX_QPATH];

    cachePass = static_cast<sint>(floor(static_cast<float32>
                                        (cacheIndex) * CACHE_HIT_RATIO));

    for(i = 0; i < CACHE_NUMGROUPS; i++) {
        Q_strncpyz(filename, cacheGroups[i].name, MAX_QPATH);
        Q_strcat(filename, MAX_QPATH, ".cache");

        handle = fileSystem->FOpenFileWrite(filename);

        for(j = 0; j < MAX_CACHE_ITEMS; j++) {
            // if it's a valid filename, and it's been hit enough times, cache it
            if(cacheItems[i][j].hits >= cachePass &&
                    strstr(cacheItems[i][j].name, "/")) {
                fileSystem->Write(cacheItems[i][j].name, strlen(cacheItems[i][j].name),
                                  handle);
                fileSystem->Write("\n", 1, handle);
            }
        }

        fileSystem->FCloseFile(handle);
    }

    cvarSystem->Set("cl_cacheGathering", "0");
}

// done.
//============================================================================

/*
================
CL_SetRecommended_f
================
*/
void CL_SetRecommended_f(void) {
    Com_SetRecommended();
}

/*
================
CL_RefPrintf

DLL glue
================
*/
void CL_RefPrintf(sint print_level, pointer fmt, ...) {
    va_list         argptr;
    valueType            msg[MAXPRINTMSG];

    va_start(argptr, fmt);
    Q_vsprintf_s(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);

    if(print_level == PRINT_ALL) {
        Com_Printf("%s", msg);
    } else if(print_level == PRINT_WARNING) {
        Com_Printf(S_COLOR_YELLOW "%s", msg);    // yellow
    } else if(print_level == PRINT_DEVELOPER) {
        Com_DPrintf(S_COLOR_RED "%s", msg);      // red
    }
}

/*
============
CL_InitRenderer
============
*/
void CL_InitRenderer(void) {
    fileHandle_t f;

    // this sets up the renderer and calls R_Init
    renderSystem->Init(&cls.glconfig);

    // load character sets
    cls.charSetShader = renderSystem->RegisterShader("gfx/2d/bigchars");

    cls.useLegacyConsoleFont = true;

    // Register console font specified by cl_consoleFont, if any
    // filehandle is unused but forces fileSystem->FOpenFileRead() to heed purecheck because it does not when filehandle is nullptr
    if(cl_consoleFont->string[0]) {
        if(fileSystem->FOpenFileByMode(cl_consoleFont->string, &f, FS_READ) >= 0) {
            renderSystem->RegisterFont(cl_consoleFont->string,
                                       cl_consoleFontSize->integer, &cls.consoleFont);
            cls.useLegacyConsoleFont = false;
        }

        fileSystem->FCloseFile(f);
    }

    cls.whiteShader = renderSystem->RegisterShader("white");
    cls.consoleShader = renderSystem->RegisterShader("console");

    g_console_field_width = cls.glconfig.vidWidth / SMALLCHAR_WIDTH - 2;
    g_consoleField.widthInChars = g_console_field_width;

    ::srand(Com_Milliseconds());
}

/*
============================
CL_StartHunkUsers

After the server has cleared the hunk, these will need to be restarted
This is the only place that any of these functions are called from
============================
*/
void CL_StartHunkUsers(void) {
    if(!com_cl_running) {
        return;
    }

    if(!com_cl_running->integer) {
        return;
    }

    if(!cls.rendererStarted) {
        cls.rendererStarted = true;
        CL_InitRenderer();
    }

    if(!cls.soundStarted) {
        cls.soundStarted = true;
        soundSystem->Init();
    }

    if(!cls.soundRegistered) {
        cls.soundRegistered = true;
        soundSystem->BeginRegistration();
    }

    if(com_dedicated->integer) {
        return;
    }

    if(!cls.uiStarted) {
        cls.uiStarted = true;
        clientGUISystem->InitGUI();
    }
}

// DHM - Nerve
void CL_CheckAutoUpdate(void) {
#ifndef PRE_RELEASE_DEMO

    if(!cl_autoupdate->integer) {
        return;
    }

    // Only check once per session
    if(autoupdateChecked) {
        Com_DPrintf("Updated checked already..");
        return;
    }

    srand(Com_Milliseconds());

    // Resolve update server
    if(!networkChainSystem->StringToAdr(cls.autoupdateServerNames[0],
                                        &cls.autoupdateServer, NA_IP)) {
        Com_DPrintf("Failed to resolve any Auto-update servers.\n");

        cls.autoUpdateServerChecked[0] = true;

        autoupdateChecked = true;
        return;
    }

    cls.autoupdatServerIndex = 0;

    cls.autoupdatServerFirstIndex = cls.autoupdatServerIndex;

    cls.autoUpdateServerChecked[cls.autoupdatServerIndex] = true;

    cls.autoupdateServer.port = BigShort(PORT_SERVER);
    Com_DPrintf("autoupdate server at: %i.%i.%i.%i:%i\n",
                cls.autoupdateServer.ip[0], cls.autoupdateServer.ip[1],
                cls.autoupdateServer.ip[2], cls.autoupdateServer.ip[3],
                BigShort(cls.autoupdateServer.port));

    networkChainSystem->OutOfBandPrint(NS_CLIENT, cls.autoupdateServer,
                                       "getUpdateInfo \"%s\" \"%s\"\n", PRODUCT_VERSION, OS_STRING);

#endif // !PRE_RELEASE_DEMO

    CL_RequestMotd();

    autoupdateChecked = true;
}

bool CL_NextUpdateServer(void) {
    valueType        *servername;

#ifdef PRE_RELEASE_DEMO
    return false;
#endif // PRE_RELEASE_DEMO

    if(!cl_autoupdate->integer) {
        return false;
    }

#if 0 //def _DEBUG
    Com_Printf(S_COLOR_MAGENTA "Autoupdate hardcoded OFF in debug build\n");
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

    Com_DPrintf("Resolving AutoUpdate Server... ");

    if(!networkChainSystem->StringToAdr(servername, &cls.autoupdateServer,
                                        NA_IP)) {
        Com_DPrintf("Couldn't resolve address, trying next one...");

        cls.autoUpdateServerChecked[cls.autoupdatServerIndex] = true;

        return CL_NextUpdateServer();
    }

    cls.autoUpdateServerChecked[cls.autoupdatServerIndex] = true;

    cls.autoupdateServer.port = BigShort(PORT_SERVER);
    Com_DPrintf("%i.%i.%i.%i:%i\n", cls.autoupdateServer.ip[0],
                cls.autoupdateServer.ip[1],
                cls.autoupdateServer.ip[2], cls.autoupdateServer.ip[3],
                BigShort(cls.autoupdateServer.port));

    return true;
}

void CL_GetAutoUpdate(void) {
    // Don't try and get an update if we haven't checked for one
    if(!autoupdateChecked) {
        return;
    }

    // Make sure there's a valid update file to request
    if(strlen(cl_updatefiles->string) < 5) {
        return;
    }

    Com_DPrintf("Connecting to auto-update server...\n");

    soundSystem->StopAllSounds();           // NERVE - SMF

    // starting to load a map so we get out of full screen ui mode
    cvarSystem->Set("r_uiFullScreen", "0");

    // toggle on all the download related cvars
    cvarSystem->Set("cl_allowDownload", "1");    // general flag
    cvarSystem->Set("cl_wwwDownload", "1");      // ftp/http support

    // clear any previous "server full" type messages
    clc.serverMessage[0] = 0;

    if(com_sv_running->integer) {
        // if running a local server, kill it
        serverInitSystem->Shutdown("Server quit\n");
    }

    // make sure a local server is killed
    cvarSystem->Set("sv_killserver", "1");
    serverMainSystem->Frame(0);

    CL_Disconnect(true, "Get autoupdate");
    Con_Close();

    Q_strncpyz(cls.servername, "Auto-Updater", sizeof(cls.servername));

    if(cls.autoupdateServer.type == NA_BAD) {
        Com_Printf("Bad server address\n");
        cls.state = CA_DISCONNECTED;
        cvarSystem->Set("ui_connecting", "0");
        return;
    }

    // Copy auto-update server address to Server connect address
    memcpy(&clc.serverAddress, &cls.autoupdateServer, sizeof(netadr_t));

    Com_DPrintf("%s resolved to %i.%i.%i.%i:%i\n", cls.servername,
                clc.serverAddress.ip[0], clc.serverAddress.ip[1],
                clc.serverAddress.ip[2], clc.serverAddress.ip[3],
                BigShort(clc.serverAddress.port));

    cls.state = CA_CONNECTING;

    cls.keyCatchers = 0;
    clc.connectTime = -99999;   // CL_CheckForResend() will fire immediately
    clc.connectPacketCount = 0;

    // server connection string
    cvarSystem->Set("cl_currentServerAddress", "Auto-Updater");
}

// DHM - Nerve

/*
============
CL_RefMalloc
============
*/
void *CL_RefMalloc(sint size) {
    return Z_TagMalloc(size, TAG_RENDERER);
}

/*
============
CL_RefTagFree
============
*/
void CL_RefTagFree(void) {
    Z_FreeTags(TAG_RENDERER);
    return;
}

static sint cl_faketime = 0;
sint CL_ScaledMilliseconds(void) {
#ifdef NDEBUG
    return ++cl_faketime;
#else
    return idsystem->Milliseconds() * com_timescale->value;
#endif
}

/*
============
CL_InitRef

RB: changed to load the renderer from a .dll
============
*/
void CL_InitRef(void) {
    Com_Printf("----- Initializing Renderer ----\n");

    // unpause so the cgame definately gets a snapshot and renders a frame
    cvarSystem->Set("cl_paused", "0");
}

/*
============
CL_ShutdownRef
============
*/
void CL_ShutdownRef(void) {
    renderSystem->Shutdown(true);
    cls.rendererStarted = false;
}

/*
===============
CL_GenerateQKey

test to see if a valid ETKEY_FILE exists.  If one does not, try to generate
it by filling it with 2048 bytes of random data.
===============
*/
static void CL_GenerateGUIDKey(void) {
    sint             len = 0;
    uchar8   buff[GUIDKEY_SIZE];
    fileHandle_t    f;

    len = fileSystem->SV_FOpenFileRead(GUIDKEY_FILE, &f);
    fileSystem->FCloseFile(f);

    if(len == GUIDKEY_SIZE) {
        Com_Printf("GUIDKEY found.\n");
        return;
    } else {
        if(len > 0) {
            Com_Printf("GUIDKEY file size != %d, regenerating\n", GUIDKEY_SIZE);
        }

        Com_Printf("GUIDKEY building random string\n");
        Com_RandomBytes(buff, sizeof(buff));

        f = fileSystem->SV_FOpenFileWrite(GUIDKEY_FILE);

        if(!f) {
            Com_Printf("GUIDKEY could not open %s for write\n", GUIDKEY_FILE);
            return;
        }

        fileSystem->Write(buff, sizeof(buff), f);
        fileSystem->FCloseFile(f);
        Com_Printf("GUIDKEY generated\n");
    }
}

/*
==============
CL_Userinfo_f
==============
*/
void CL_Userinfo_f(void) {
    //do nothing kthxbye
}

/*
===============
CL_UpdateScreen

I had a problem with AddCommand and pointing to the class member so this is one way of doing
pointing to a function that is a class member
===============
*/
void CL_UpdateScreen(void) {
    static_cast<idClientScreenSystemLocal *>
    (clientScreenSystem)->UpdateScreen();
}

/*
====================
CL_Init
====================
*/
void CL_Init(void) {
    Com_Printf("----- Client Initialization -----\n");

    Con_Init();

    CL_ClearState();

    cls.state = CA_DISCONNECTED;    // no longer CA_UNINITIALIZED

    cls.realtime = 0;

    CL_InitInput();

    //
    // register our variables
    //

    cl_noprint = cvarSystem->Get("cl_noprint", "0", 0,
                                 "At 1, it doesnt print to the console. Doesnt affect ingame-console messages.");
    cl_motd = cvarSystem->Get("cl_motd", "1", 0,
                              "Wether to get the motd string from tmaphe master server");
    cl_autoupdate = cvarSystem->Get("cl_autoupdate", "1", CVAR_ARCHIVE,
                                    "Automatic game update checks on launch.");

    cl_timeout = cvarSystem->Get("cl_timeout", "200", 0,
                                 "Set the inactivity time before a client is disconnected (timed out)");

    cl_wavefilerecord = cvarSystem->Get("cl_wavefilerecord", "0", CVAR_TEMP,
                                        "Toggle recording a .wav audio file upon loading a demo. Suggest setting to 0 in autoexec.cfg");

    cl_timeNudge = cvarSystem->Get("cl_timeNudge", "0", CVAR_ARCHIVE,
                                   "Supposed to be for adjusting prediction for your ping.");
    cl_shownet = cvarSystem->Get("cl_shownet", "0", CVAR_TEMP,
                                 "Display network quality graph");
    cl_shownuments = cvarSystem->Get("cl_shownuments", "0", CVAR_TEMP,
                                     "Display the number of entities in each packet");
    cl_showServerCommands = cvarSystem->Get("cl_showServerCommands", "0", 0,
                                            "Show server commands");
    cl_showSend = cvarSystem->Get("cl_showSend", "0", CVAR_TEMP,
                                  "Shows each packet");
    cl_showTimeDelta = cvarSystem->Get("cl_showTimeDelta", "0", CVAR_TEMP,
                                       "Shows time difference between each packet");
    cl_freezeDemo = cvarSystem->Get("cl_freezeDemo", "0", CVAR_TEMP,
                                    "Pauses demo playback");
    rcon_client_password = cvarSystem->Get("rconPassword", "", CVAR_TEMP,
                                           "Password for remote console access");
    cl_activeAction = cvarSystem->Get("activeAction", "", CVAR_TEMP,
                                      "Perform the specified when joining server");
    cl_autorecord = cvarSystem->Get("cl_autorecord", "0", CVAR_TEMP,
                                    "At 1, then it will start/stop recording a demo at the start/end of each match.");

    cl_timedemo = cvarSystem->Get("timedemo", "0", 0,
                                  "Set to 1 to enable timedemo mode,for benchmarking purposes");
    cl_forceavidemo = cvarSystem->Get("cl_forceavidemo", "0", 0,
                                      "Forces all demo recording into a sequence of screenshots in TGA format.");
    cl_aviFrameRate = cvarSystem->Get("cl_aviFrameRate", "25", CVAR_ARCHIVE,
                                      "Framerate to use when capturing video");

    cl_aviMotionJpeg = cvarSystem->Get("cl_aviMotionJpeg", "1", CVAR_ARCHIVE,
                                       "Use the MJPEG codec when capturing video");

    rconAddress = cvarSystem->Get("rconAddress", "", 0,
                                  "Alternate server address to remotely access via rcon protocol");

    cl_yawspeed = cvarSystem->Get("cl_yawspeed", "140", CVAR_ARCHIVE,
                                  "Sets turn speed when using +left and +right.");
    cl_pitchspeed = cvarSystem->Get("cl_pitchspeed", "140", CVAR_ARCHIVE,
                                    "Turn speed when using keyboard to look up/down");
    cl_anglespeedkey = cvarSystem->Get("cl_anglespeedkey", "1.5", 0,
                                       "When pressing +left or +right keys, this sets the speed that the view angle turns");

    cl_maxpackets = cvarSystem->Get("cl_maxpackets", "30", CVAR_ARCHIVE,
                                    "Cap for data packet transmissions (upstream)");
    cl_packetdup = cvarSystem->Get("cl_packetdup", "1", CVAR_ARCHIVE,
                                   "Number of duplicates for every data packet sent upstream, minimised packetloss");

    cl_run = cvarSystem->Get("cl_run", "1", CVAR_ARCHIVE,
                             "Toggle 'always run' setting");
    cl_sensitivity = cvarSystem->Get("sensitivity", "5", CVAR_ARCHIVE,
                                     "Used for setting the mouse sensitivity");
    cl_mouseAccel = cvarSystem->Get("cl_mouseAccel", "0", CVAR_ARCHIVE,
                                    "Toggles mouse accelleration");
    cl_freelook = cvarSystem->Get("cl_freelook", "1", CVAR_ARCHIVE,
                                  "Look around using the mouse");

    cl_xbox360ControllerAvailable =
        cvarSystem->Get("in_xbox360ControllerAvailable", "0", CVAR_ROM,
                        "Use Xbox 360 controller if it is avaliable");

    // 0: legacy mouse acceleration
    // 1: new implementation

    cl_mouseAccelStyle = cvarSystem->Get("cl_mouseAccelStyle", "0",
                                         CVAR_ARCHIVE, "Sets mouse acceleration style.");
    // offset for the power function (for style 1, ignored otherwise)
    // this should be set to the max rate value
    cl_mouseAccelOffset = cvarSystem->Get("cl_mouseAccelOffset", "5",
                                          CVAR_ARCHIVE, "Sets mouse acceleration sensitivity offset.");

    cl_showMouseRate = cvarSystem->Get("cl_showmouserate", "0", 0,
                                       "Show how fast you move the mouse in ratio to sensitivity setting.");

    cl_allowDownload = cvarSystem->Get("cl_allowDownload", "1", CVAR_ARCHIVE,
                                       "Toggles downloading missing files from the server");
    cl_wwwDownload = cvarSystem->Get("cl_wwwDownload", "1",
                                     CVAR_USERINFO | CVAR_ARCHIVE,
                                     "Toggles downloading missing files from a www file server");

    cl_profile = cvarSystem->Get("cl_profile", "", CVAR_ROM,
                                 "Stores which player profile is being used");
    cl_defaultProfile = cvarSystem->Get("cl_defaultProfile", "", CVAR_ROM,
                                        "Sets what player profile is to be used by default when loading the game");

    // init autoswitch so the ui will have it correctly even
    // if the cgame hasn't been started
    // -NERVE - SMF - disabled autoswitch by default
    //cvarSystem->Get( "cg_autoswitch", "0", CVAR_ARCHIVE, "Toggles automatically chaning weapon when current one is out of ammo" );

    // Rafael - particle switch
    //cvarSystem->Get( "cg_wolfparticles", "1", CVAR_ARCHIVE, "Toggles display of particle effects – e.g. Explosions, some smoke effects." );
    // done

    cl_conXOffset = cvarSystem->Get("cl_conXOffset", "3", 0,
                                    "Supposed to move the on-screen console text up/down");
    cl_inGameVideo = cvarSystem->Get("r_inGameVideo", "1", CVAR_ARCHIVE,
                                     "Toggle use of video clips in game");

    cl_serverStatusResendTime = cvarSystem->Get("cl_serverStatusResendTime",
                                "750", 0, "Sets the amount of time (in milliseconds) between heartbeats sent to the master server");

    // RF
    cl_recoilPitch = cvarSystem->Get("cg_recoilPitch", "0", CVAR_ROM,
                                     "Sets the amount of the recoil pitch");

    cl_bypassMouseInput = cvarSystem->Get("cl_bypassMouseInput", "0", 0,
                                          "Automatically toggled when there are items (e.g. Vchat menu) on screen, to maintain mouse focus being in game, instead of in menu.");     //CVAR_ROM );          // NERVE - SMF

    cl_doubletapdelay = cvarSystem->Get("cl_doubletapdelay", "100",
                                        CVAR_ARCHIVE,
                                        "	Sets the delay between keypresses required to be a double-tap");    // Arnout: double tap

    m_pitch = cvarSystem->Get("m_pitch", "0.022", CVAR_ARCHIVE,
                              "Sets the mouse pitch (up/down)");
    m_yaw = cvarSystem->Get("m_yaw", "0.022", CVAR_ARCHIVE,
                            "Sets the mouse yaw (left/right)");
    m_forward = cvarSystem->Get("m_forward", "0.25", CVAR_ARCHIVE,
                                "Sets the mouse yaw (forward)");
    m_side = cvarSystem->Get("m_side", "0.25", CVAR_ARCHIVE,
                             "Sets the mouse yaw (side)");
    m_filter = cvarSystem->Get("m_filter", "0", CVAR_ARCHIVE,
                               "Toggles mouse filter (mouse smoothing)");

    j_pitch = cvarSystem->Get("j_pitch", "0.022", CVAR_ARCHIVE,
                              "Sets joystics pitch");
    j_yaw = cvarSystem->Get("j_yaw", "-0.022", CVAR_ARCHIVE,
                            "Sets joystics yaw");
    j_forward = cvarSystem->Get("j_forward", "-0.25", CVAR_ARCHIVE,
                                "Sets joystics forward");
    j_side = cvarSystem->Get("j_side", "0.25", CVAR_ARCHIVE,
                             "Sets joystics side");
    j_up = cvarSystem->Get("j_up", "1", CVAR_ARCHIVE, "Sets joystics up");
    j_pitch_axis = cvarSystem->Get("j_pitch_axis", "3", CVAR_ARCHIVE,
                                   "Sets joystics axis pitch");
    j_yaw_axis = cvarSystem->Get("j_yaw_axis", "4", CVAR_ARCHIVE,
                                 "Sets joystics axis yaw");
    j_forward_axis = cvarSystem->Get("j_forward_axis", "1", CVAR_ARCHIVE,
                                     "Sets joystics axis forward");
    j_side_axis = cvarSystem->Get("j_side_axis", "0", CVAR_ARCHIVE,
                                  "Sets joystics axis side");
    j_up_axis = cvarSystem->Get("j_up_axis", "2", CVAR_ARCHIVE,
                                "Sets joystics axis up");

    cvarSystem->CheckRange(j_pitch_axis, 0, MAX_JOYSTICK_AXIS - 1, true);
    cvarSystem->CheckRange(j_yaw_axis, 0, MAX_JOYSTICK_AXIS - 1, true);
    cvarSystem->CheckRange(j_forward_axis, 0, MAX_JOYSTICK_AXIS - 1, true);
    cvarSystem->CheckRange(j_side_axis, 0, MAX_JOYSTICK_AXIS - 1, true);
    cvarSystem->CheckRange(j_up_axis, 0, MAX_JOYSTICK_AXIS - 1, true);

    cl_motdString = cvarSystem->Get("cl_motdString", "", CVAR_ROM,
                                    "Message of the day string.");

    // ~ and `, as keys and characters
    cl_consoleKeys = cvarSystem->Get("cl_consoleKeys", "~ ` 0x7e 0x60",
                                     CVAR_ARCHIVE, "Toggle button for opening in-game console");

    cl_consoleFont = cvarSystem->Get("cl_consoleFont", "",
                                     CVAR_ARCHIVE | CVAR_LATCH,
                                     "The outline font used for the in-game console.	");
    cl_consoleFontSize = cvarSystem->Get("cl_consoleFontSize", "16",
                                         CVAR_ARCHIVE | CVAR_LATCH,
                                         "The size of the console font (if using an outline font).");
    cl_consoleFontKerning = cvarSystem->Get("cl_consoleFontKerning", "0",
                                            CVAR_ARCHIVE, "The width of the console font char.");
    cl_consolePrompt = cvarSystem->Get("cl_consolePrompt", "^3->",
                                       CVAR_ARCHIVE, "The console prompt line.");

    cl_gamename = cvarSystem->Get("cl_gamename", GAMENAME_FOR_MASTER,
                                  CVAR_TEMP,
                                  "Gamename sent to master server in getservers[Ext] query and infoResponse, infostring value.Also used for filtering local network games.");
    cl_altTab = cvarSystem->Get("cl_altTab", "1", CVAR_ARCHIVE,
                                "Allow ⎇ Alt + Tab ↹ out of application (⌘ Cmd + Tab ↹ on a Mac)");

    //bani - make these cvars visible to cgame
    cl_demorecording = cvarSystem->Get("cl_demorecording", "0", CVAR_ROM,
                                       "This CVAR is Read Only, you can use it in-game though, basically if you have turned recording on in-game, you can use this CVAR to make sure it is recording in the console, if it says 1 then its recording.");
    cl_demofilename = cvarSystem->Get("cl_demofilename", "", CVAR_ROM,
                                      "This CVAR is Read Only, you can use it in-game though, basically if you have turned recording on in-game, you can use this CVAR to see what the name of the demo is in the console.");
    cl_demooffset = cvarSystem->Get("cl_demooffset", "0", CVAR_ROM,
                                    "This CVAR is Read Only, you can use it in-game though, basically if you have turned recording on in-game, you can use this CVAR to see what the current camera offset is in the console, it changes based on seconds, but you cant view its offset realtime, you have to continually type the CVAR in the console..");
    cl_waverecording = cvarSystem->Get("cl_waverecording", "0", CVAR_ROM,
                                       "This is not changeable, it just tells you when you type cl_waverecording in console if its recording or not, if it is it will say 1 if it isnt it will say 0.");
    cl_wavefilename = cvarSystem->Get("cl_wavefilename", "", CVAR_ROM,
                                      "If you type 1, i assume it shows the name of the current .wav audio file being recorded, EDIT: tested yes it does this.");
    cl_waveoffset = cvarSystem->Get("cl_waveoffset", "0", CVAR_ROM,
                                    "When 1, if you are recording a .wav audio file it just prints to the console information about the wave offset.");

    //bani
    cl_packetloss = cvarSystem->Get("cl_packetloss", "0", CVAR_CHEAT,
                                    "Mimic packet loss.");
    cl_packetdelay = cvarSystem->Get("cl_packetdelay", "0", CVAR_CHEAT,
                                     "Mimic packet dealy.");

    cvarSystem->Get("cl_maxPing", "800", CVAR_ARCHIVE,
                    "This will NOT show servers with a ping higher than the chosen setting in the server browser.");

    cl_guidServerUniq = cvarSystem->Get("cl_guidServerUniq", "1", CVAR_ARCHIVE,
                                        "Use a unique guid value per server.");

    // userinfo
    cvarSystem->Get("name", idsystem->GetCurrentUser(),
                    CVAR_USERINFO | CVAR_ARCHIVE, "Sets the name of the player");
    cvarSystem->Get("rate", "25000", CVAR_USERINFO | CVAR_ARCHIVE,
                    "Cap on the connection bandwidth to use, 1000=~1KB/s. For 56k use about 4000, broadband 25000");     // Dushan - changed from 5000
    cvarSystem->Get("snaps", "20", CVAR_USERINFO | CVAR_ARCHIVE,
                    "snapshots for server to send you, leave at 20.");
    cvarSystem->Get("cg_version", PRODUCT_NAME, CVAR_ROM | CVAR_USERINFO,
                    "Displays client game version.");
    cvarSystem->Get("password", "", CVAR_USERINFO,
                    "Used for setting password required for some servers");
    cvarSystem->Get("cg_predictItems", "1", CVAR_ARCHIVE,
                    "Toggle use of prediction for picking up items.");

    //----(SA) added
    //cvarSystem->Get( "cg_autoactivate", "1", CVAR_ARCHIVE, "Toggles automatically picking up items (paks, weapons etc)" );
    //----(SA) end

    // cgame might not be initialized before menu is used
    cvarSystem->Get("cg_viewsize", "100", CVAR_ARCHIVE,
                    "Supposed to be for setting the % of screen actually displaying rendered game. Might have been useful for using a lower-res ET while using a native resolution on TFT screens");

    //cvarSystem->Get( "cg_autoReload", "1", CVAR_ARCHIVE, "Toggles automatically reloading weapon when clip becomes empty" );

    //cl_missionStats = cvarSystem->Get( "g_missionStats", "0", CVAR_ROM, "Mission statistics" );
    cl_waitForFire = cvarSystem->Get("cl_waitForFire", "0", CVAR_ROM,
                                     "Wait for the fire.");

    // NERVE - SMF - localization
    cl_language = cvarSystem->Get("cl_language", "0", CVAR_ARCHIVE,
                                  "Stores the language. English is 0");
    cl_debugTranslation = cvarSystem->Get("cl_debugTranslation", "0", 0,
                                          "Debug translations");
    // -NERVE - SMF

    // DHM - Nerve :: Auto-update
    cl_updateavailable = cvarSystem->Get("cl_updateavailable", "0", CVAR_ROM,
                                         "Show if update is available");
    cl_updatefiles = cvarSystem->Get("cl_updatefiles", "", CVAR_ROM,
                                     "Sset when there is a download an update patch");

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
    cmdSystem->AddCommand("cmd", CL_ForwardToServer_f,
                          "Commands in the console");
    cmdSystem->AddCommand("configstrings", CL_Configstrings_f,
                          "Returns config strings");
    cmdSystem->AddCommand("clientinfo", CL_Clientinfo_f,
                          "Returns some info about your client in the console, probably also read by server");
    cmdSystem->AddCommand("snd_reload", CL_Snd_Reload_f,
                          "Reloads all sound files");
    cmdSystem->AddCommand("snd_restart", CL_Snd_Restart_f,
                          "Restarts sound engine");
    cmdSystem->AddCommand("vid_restart", CL_Vid_Restart_f,
                          "Reloads the video/rendering etc, required for some cvar settings to take actual effect.");
    cmdSystem->AddCommand("ui_restart", CL_UI_Restart_f,
                          "Reloads all menu files");
    cmdSystem->AddCommand("disconnect", CL_Disconnect_f,
                          "Command to disconnect from a server");
    cmdSystem->AddCommand("record", CL_Record_f, "Starts recording a demo");
    cmdSystem->AddCommand("demo", CL_PlayDemo_f,
                          "For loading a demo for playback: /demo demofilename");
    cmdSystem->SetCommandCompletionFunc("demo", CL_CompleteDemoName);
    cmdSystem->AddCommand("cinematic",
                          &idClientCinemaSystemLocal::PlayCinematic_f,
                          "[/cinematic] will play the intro movie. Doesnt work in game");
    cmdSystem->AddCommand("stoprecord", CL_StopRecord_f,
                          "Stops recording a demo");
    cmdSystem->AddCommand("connect", CL_Connect_f,
                          "Used for connecting to a server, /connect ip.ad.dre.ss:port");
    cmdSystem->AddCommand("reconnect", CL_Reconnect_f,
                          "Reconnect to the most recent server you tried to connect to");
    cmdSystem->AddCommand("localservers",
                          &idClientBrowserSystemLocal::LocalServers,
                          "Scans for LAN connected servers");
    cmdSystem->AddCommand("globalservers",
                          &idClientBrowserSystemLocal::GlobalServers,
                          "Command to scan for all servers, internet & LAN");
    cmdSystem->AddCommand("openurl", CL_OpenUrl_f, "Opens a URL");
    cmdSystem->AddCommand("rcon", CL_Rcon_f,
                          "Prepend to issue to remote-console, i.e. Send what follows to the server as command.");
    cmdSystem->AddCommand("setenv", CL_Setenv_f,
                          "It is used to define the value of environment variables");
    cmdSystem->AddCommand("ping", &idClientBrowserSystemLocal::Ping,
                          "For pinging server - /ping [server]");
    cmdSystem->AddCommand("serverstatus",
                          &idClientBrowserSystemLocal::ServerStatus,
                          "Returns serverinfo plus basic player info");
    cmdSystem->AddCommand("showip", &idClientBrowserSystemLocal::ShowIP,
                          "Returns your IP in console");
    cmdSystem->AddCommand("fs_openedList", CL_OpenedPK3List_f,
                          "Holds what .pk3's are open / in Main or mod folder");
    cmdSystem->AddCommand("fs_referencedList", CL_ReferencedPK3List_f,
                          "Holds what .pk3s are referenced/loaded");

    // Ridah, startup-caching system
    cmdSystem->AddCommand("cache_startgather", CL_Cache_StartGather_f,
                          "Startup-caching system");
    cmdSystem->AddCommand("cache_usedfile", CL_Cache_UsedFile_f,
                          "Startup-caching system");
    cmdSystem->AddCommand("cache_setindex", CL_Cache_SetIndex_f,
                          "Startup-caching system");
    cmdSystem->AddCommand("cache_mapchange", CL_Cache_MapChange_f,
                          "Startup-caching system");
    cmdSystem->AddCommand("cache_endgather", CL_Cache_EndGather_f,
                          "Startup-caching system");

    cmdSystem->AddCommand("updatehunkusage",
                          &idClientGameSystemLocal::UpdateLevelHunkUsage,
                          "Update hunk memory usage");
    cmdSystem->AddCommand("updatescreen", CL_UpdateScreen,
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

    cmdSystem->AddCommand("setRecommended", CL_SetRecommended_f,
                          "Applies the settings that were reccommended for your system");
    cmdSystem->AddCommand("userinfo", CL_Userinfo_f,
                          "This info is transmitted by client to the server on connect, and if changed.");
    cmdSystem->AddCommand("wav_record", CL_WavRecord_f,
                          "Command to record .wav audio file");
    cmdSystem->AddCommand("wav_stoprecord", CL_WavStopRecord_f,
                          "Command to stop recording a .wav audio file");

    cmdSystem->AddCommand("video", CL_Video_f, "Toggle use of video in game");
    cmdSystem->AddCommand("stopvideo", CL_StopVideo_f,
                          "Toggle stop video in the game");

    CL_InitRef();

    clientScreenSystem->Init();

    cmdBufferSystem->Execute();

    cvarSystem->Set("cl_running", "1");

    CL_GenerateGUIDKey();
    //Dushan
    cl_guid = cvarSystem->Get("cl_guid", "NO_GUID", CVAR_USERINFO | CVAR_ROM,
                              "Enable GUID userinfo identification");
    CL_UpdateGUID(nullptr, 0);

    // DHM - Nerve
    autoupdateChecked = false;
    autoupdateStarted = false;

    clientLocalizationSystem->InitTranslation(); // NERVE - SMF - localization

    Com_Printf("----- Client Initialization Complete -----\n");
}


/*
===============
CL_Shutdown
===============
*/
void CL_Shutdown(void) {
    static bool recursive = false;

    // check whether the client is running at all.
    if(!(com_cl_running && com_cl_running->integer)) {
        return;
    }

    Com_Printf("----- CL_Shutdown -----\n");

    if(recursive) {
        printf("recursive shutdown\n");
        return;
    }

    recursive = true;

    if(clc.waverecording) {                              // fretn - write wav header when we quit
        CL_WavStopRecord_f();
    }

    CL_Disconnect(true, "client shutdown");

    downloadSystem->Shutdown();
    CL_ShutdownRef();

    clientGUISystem->ShutdownGUI();

    soundSystem->Shutdown();

    cmdSystem->RemoveCommand("cmd");
    cmdSystem->RemoveCommand("configstrings");
    cmdSystem->RemoveCommand("userinfo");
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

    soundSystem->Shutdown();
    cvarSystem->Set("cl_running", "0");

    recursive = false;

    ::memset(&cls, 0, sizeof(cls));
    clientGUISystem->SetCatcher(0);

    Com_Printf("-----------------------\n");
}

/*
=======================
CL_OpenURLForCvar
=======================
*/
void CL_OpenURL(pointer url) {
    if(!url || !strlen(url)) {
        Com_Printf("%s",
                   clientLocalizationSystem->TranslateStringBuf("invalid/empty URL\n"));
        return;
    }

    idsystem->OpenURL(url, true);
}

// Gordon: TEST TEST TEST
/*
==================
BotImport_DrawPolygon
==================
*/
void BotImport_DrawPolygon(sint color, sint numpoints, float32 *points) {
    //renderSystem->DrawDebugPolygon( color, numpoints, points );
}

/*
===================
CL_UpdateInfoPacket
===================
*/
void CL_UpdateInfoPacket(netadr_t from) {

    if(cls.autoupdateServer.type == NA_BAD) {
        Com_DPrintf("CL_UpdateInfoPacket:  Auto-Updater has bad address\n");
        return;
    }

    Com_DPrintf("Auto-Updater resolved to %i.%i.%i.%i:%i\n",
                cls.autoupdateServer.ip[0], cls.autoupdateServer.ip[1],
                cls.autoupdateServer.ip[2], cls.autoupdateServer.ip[3],
                BigShort(cls.autoupdateServer.port));

    if(!networkSystem->CompareAdr(from, cls.autoupdateServer)) {
        Com_DPrintf("CL_UpdateInfoPacket:  Received packet from %i.%i.%i.%i:%i\n",
                    from.ip[0], from.ip[1], from.ip[2], from.ip[3],
                    BigShort(from.port));
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
CL_AddToLimboChat
=======================
*/
void CL_AddToLimboChat(pointer str) {
    sint i, len, lastcolor, chatHeight;
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

    lastcolor = '7';

    ls = nullptr;

    while(*str) {
        if(len > LIMBOCHAT_WIDTH - 1) {
            break;
        }

        if(Q_IsColorString(str)) {
            *p++ = *str++;
            lastcolor = *str;
            *p++ = *str++;
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
