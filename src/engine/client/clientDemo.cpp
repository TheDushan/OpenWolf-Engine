////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2021 - 2023 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   clientDemo.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

idClientDemoSystemLocal clientDemoLocal;

/*
===============
idClientDemoSystemLocal::idClientDemoSystemLocal
===============
*/
idClientDemoSystemLocal::idClientDemoSystemLocal(void) {
}

/*
===============
idClientDemoSystemLocal::~idClientDemoSystemLocal
===============
*/
idClientDemoSystemLocal::~idClientDemoSystemLocal(void) {
}

/*
====================
idClientDemoSystemLocal::WriteDemoMessage

Dumps the current net message, prefixed by the length
====================
*/
void idClientDemoSystemLocal::WriteDemoMessage(msg_t *msg,
        sint headerBytes) {
    sint len, swlen;

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
idClientDemoSystemLocal::StopRecording_f

stop recording a demo
====================
*/
void idClientDemoSystemLocal::StopRecord_f(void) {
    sint len;

    if(!clc.demorecording) {
        common->Printf("Not recording a demo.\n");
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
    common->Printf("Stopped demo.\n");
}

/*
==================
idClientDemoSystemLocal::DemoFilename
==================
*/
void idClientDemoSystemLocal::DemoFilename(valueType *buf, sint bufSize) {
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
idClientDemoSystemLocal::Record_f

record <demoname>

Begins recording a demo from the current position
====================
*/
void idClientDemoSystemLocal::Record_f(void) {
    sint len;
    valueType name[MAX_OSPATH], extension[32], *s;

    if(cmdSystem->Argc() > 2) {
        common->Printf("record <demoname>\n");
        return;
    }

    if(clc.demorecording) {
        common->Printf("Already recording.\n");
        return;
    }

    if(cls.state != CA_ACTIVE) {
        common->Printf("You must be in a level to record.\n");
        return;
    }

    // ATVI Wolfenstein Misc #479 - changing this to a warning
    // sync 0 doesn't prevent recording, so not forcing it off .. everyone does g_sync 1 ; record ; g_sync 0 ..
    if(networkSystem->IsLocalAddress(clc.serverAddress) &&
            !cvarSystem->VariableValue("g_synchronousClients")) {
        common->Printf(S_COLOR_YELLOW
                       "WARNING: You should set 'g_synchronousClients 1' for smoother demo recording\n");
    }

    if(cmdSystem->Argc() == 2) {
        s = cmdSystem->Argv(1);
        Q_strncpyz(demoName, s, sizeof(demoName));
        Q_vsprintf_s(name, sizeof(name), sizeof(name), "dm_%d", PROTOCOL_VERSION);
    } else {
        // timestamp the file
        idClientDemoSystemLocal::DemoFilename(demoName, sizeof(demoName));

        Q_vsprintf_s(name, sizeof(name), sizeof(name), "demos/%s.%s", demoName,
                     extension);

        if(fileSystem->FileExists(name)) {
            common->Printf("Record: Couldn't create a file\n");
            return;
        }
    }

    Record(name);
}

void idClientDemoSystemLocal::Record(pointer name) {
    sint             i;
    msg_t           buf;
    uchar8            bufData[MAX_MSGLEN];
    entityState_t  *ent;
    entityState_t   nullstate;
    valueType           *s;
    sint             len;

    // open the demo file

    common->Printf("recording to %s.\n", name);
    clc.demofile = fileSystem->FOpenFileWrite(name);

    if(!clc.demofile) {
        common->Printf("ERROR: couldn't open.\n");
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
    msgToFuncSystem->Init(&buf, bufData, sizeof(bufData));
    msgToFuncSystem->Bitstream(&buf);

    // NOTE, MRE: all server->client messages now acknowledge
    msgToFuncSystem->WriteLong(&buf, clc.reliableSequence);

    msgToFuncSystem->WriteByte(&buf, svc_gamestate);
    msgToFuncSystem->WriteLong(&buf, clc.serverCommandSequence);

    // configstrings
    for(i = 0; i < MAX_CONFIGSTRINGS; i++) {
        if(!cl.gameState.stringOffsets[i]) {
            continue;
        }

        s = cl.gameState.stringData + cl.gameState.stringOffsets[i];
        msgToFuncSystem->WriteByte(&buf, svc_configstring);
        msgToFuncSystem->WriteShort(&buf, i);
        msgToFuncSystem->WriteBigString(&buf, s);
    }

    // baselines
    ::memset(&nullstate, 0, sizeof(nullstate));

    for(i = 0; i < MAX_GENTITIES; i++) {
        ent = &cl.entityBaselines[i];

        if(!ent->number) {
            continue;
        }

        msgToFuncSystem->WriteByte(&buf, svc_baseline);
        msgToFuncSystem->WriteDeltaEntity(&buf, &nullstate, ent, true);
    }

    msgToFuncSystem->WriteByte(&buf, svc_EOF);

    // finished writing the gamestate stuff

    // write the client num
    msgToFuncSystem->WriteLong(&buf, clc.clientNum);
    // write the checksum feed
    msgToFuncSystem->WriteLong(&buf, clc.checksumFeed);

    // finished writing the client packet
    msgToFuncSystem->WriteByte(&buf, svc_EOF);

    // write it to the demo file
    len = LittleLong(clc.serverMessageSequence - 1);
    fileSystem->Write(&len, 4, clc.demofile);

    len = LittleLong(buf.cursize);
    fileSystem->Write(&len, 4, clc.demofile);
    fileSystem->Write(buf.data, buf.cursize, clc.demofile);

    // the rest of the demo file will be copied from net messages
    clientReliableCommandsSystem->AddReliableCommand("getstatus");
}

/*
=================
idClientDemoSystemLocal::DemoCompleted
=================
*/
void idClientDemoSystemLocal::DemoCompleted(void) {
    if(timedemo && timedemo->integer) {
        sint             time;

        time = idsystem->Milliseconds() - clc.timeDemoStart;

        if(time > 0) {
            common->Printf("%i frames, %3.1f seconds: %3.1f fps\n", clc.timeDemoFrames,
                           time / 1000.0, clc.timeDemoFrames * 1000.0 / time);
        }
    }

    // fretn
    if(clc.waverecording) {
        idClientWaveSystemLocal::WriteWaveClose();
        clc.waverecording = false;
    }

    idClientConsoleCommandsSystemLocal::Disconnect_f();
    soundSystem->StopAllSounds();

    NextDemo();

}

/*
=================
idClientDemoSystemLocal::ReadDemoMessage
=================
*/
void idClientDemoSystemLocal::ReadDemoMessage(void) {
    sint             r;
    msg_t           buf;
    uchar8            bufData[MAX_MSGLEN];
    sint             s;

    if(!clc.demofile) {
        DemoCompleted();
        return;
    }

    // get the sequence number
    r = fileSystem->Read(&s, 4, clc.demofile);

    if(r != 4) {
        DemoCompleted();
        return;
    }

    clc.serverMessageSequence = LittleLong(s);

    // init the message
    msgToFuncSystem->Init(&buf, bufData, sizeof(bufData));

    // get the length
    r = fileSystem->Read(&buf.cursize, 4, clc.demofile);

    if(r != 4) {
        DemoCompleted();
        return;
    }

    buf.cursize = LittleLong(buf.cursize);

    if(buf.cursize == -1) {
        DemoCompleted();
        return;
    }

    if(buf.cursize > buf.maxsize) {
        common->Error(ERR_DROP,
                      "idClientDemoSystemLocal::ReadDemoMessage: demoMsglen > MAX_MSGLEN");
    }

    r = fileSystem->Read(buf.data, buf.cursize, clc.demofile);

    if(r != buf.cursize) {
        common->Printf("Demo file was truncated.\n");
        DemoCompleted();
        return;
    }

    clc.lastPacketTime = cls.realtime;
    buf.readcount = 0;

    idClientParseSystemLocal::ParseServerMessage(&buf);
}

/*
====================
idClientDemoSystemLocal::CompleteDemoName
====================
*/
void idClientDemoSystemLocal::CompleteDemoName(valueType *args,
        sint argNum) {
    if(argNum == 2) {
        valueType demoExt[ 16 ];

        Q_vsprintf_s(demoExt, sizeof(demoExt), sizeof(demoExt), ".dm_%d",
                     PROTOCOL_VERSION);
        cmdCompletionSystem->CompleteFilename("demos", demoExt, true);
    }
}

/*
====================
idClientDemoSystemLocal::PlayDemo_f

demo <demoname>
====================
*/
void idClientDemoSystemLocal::PlayDemo_f(void) {
    sint prot_ver;
    valueType name[MAX_OSPATH], extension[32], arg[MAX_OSPATH];

    if(cmdSystem->Argc() < 2) {
        common->Printf("playdemo <demo name>\n");
        return;
    }

    // make sure a local server is killed
    cvarSystem->Set("sv_killserver", "1");

    Q_strncpyz(arg, cmdSystem->Args(), sizeof(arg));

    clientConsoleCommandSystem->Disconnect(true, "Playing demo");

    //clientMainSystem->FlushMemory();   //----(SA)  MEM NOTE: in missionpack, this is moved to idClientDownloadSystemLocal::DownloadsComplete

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
            common->Error(ERR_DROP, "No demo selected.", name);
        } else {
            common->Error(ERR_DROP, "couldn't open %s", name);
        }

        return;
    }

    Q_strncpyz(clc.demoName, cmdSystem->Argv(1), sizeof(clc.demoName));

    clientConsoleSystem->Close();

    cls.state = CA_CONNECTED;
    clc.demoplaying = true;

    if(cvarSystem->VariableValue("cl_wavefilerecord")) {
        idClientWaveSystemLocal::WriteWaveOpen();
    }

    Q_strncpyz(cls.servername, cmdSystem->Argv(1), sizeof(cls.servername));

    // read demo messages until connected
    while(cls.state >= CA_CONNECTED && cls.state < CA_PRIMED) {
        ReadDemoMessage();
    }

    // don't get the first snapshot this frame, to prevent the long
    // time from the gamestate load from messing causing a time skip
    clc.firstDemoFrameSkipped = false;
    //if (clc.waverecording) {
    //    idClientWaveSystemLocal::WriteWaveClose();
    //    clc.waverecording = false;
    //}
}

/*
==================
idClientDemoSystemLocal::NextDemo

Called when a demo or cinematic finishes
If the "nextdemo" cvar is set, that command will be issued
==================
*/
void idClientDemoSystemLocal::NextDemo(void) {
    valueType            v[MAX_STRING_CHARS];

    Q_strncpyz(v, cvarSystem->VariableString("nextdemo"), sizeof(v));
    v[MAX_STRING_CHARS - 1] = 0;

    if(developer->integer) {
        common->Printf("CL_NextDemo: %s\n", v);
    }

    if(!v[0]) {
        clientMainSystem->FlushMemory();
        return;
    }

    cvarSystem->Set("nextdemo", "");
    cmdBufferSystem->AddText(v);
    cmdBufferSystem->AddText("\n");
    cmdBufferSystem->Execute();
}

/*
==================
idClientDemoSystemLocal::DemoName

Returns the name of the demo
==================
*/
void idClientDemoSystemLocal::DemoName(valueType *buffer, sint size) {
    if(clc.demoplaying || clc.demorecording) {
        Q_strncpyz(buffer, clc.demoName, size);
    } else if(size >= 1) {
        buffer[ 0 ] = '\0';
    }
}
