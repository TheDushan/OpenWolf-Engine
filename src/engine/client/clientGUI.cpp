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
// File name:   clientCUI.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

void *uivm;

idUserInterfaceManager *uiManager;
idUserInterfaceManager *(*guiEntry)(guiImports_t *guimports);

static guiImports_t exports;

idClientGUISystemLocal clientGUILocal;
idClientGUISystem *clientGUISystem = &clientGUILocal;

/*
===============
idClientGUISystemLocal::idClientGUISystemLocal
===============
*/
idClientGUISystemLocal::idClientGUISystemLocal(void) {
}

/*
===============
idClientGUISystemLocal::~idClientGUISystemLocal
===============
*/
idClientGUISystemLocal::~idClientGUISystemLocal(void) {
}

/*
====================
idClientGUISystemLocal::GetClientState
====================
*/
void idClientGUISystemLocal::GetClientState(uiClientState_t *state) {
    state->connectPacketCount = clc.connectPacketCount;
    state->connState = cls.state;
    Q_strncpyz(state->servername, cls.servername, sizeof(state->servername));
    Q_strncpyz(state->updateInfoString, cls.updateInfoString,
               sizeof(state->updateInfoString));
    Q_strncpyz(state->messageString, clc.serverMessage,
               sizeof(state->messageString));
    state->clientNum = cl.snapServer.ps.clientNum;
}

/*
====================
idClientGUISystemLocal::GetNews
====================
*/
bool idClientGUISystemLocal::GetNews(bool begin) {
    bool finished = false;
    sint readSize;
    static valueType newsFile[MAX_QPATH] = "";

    if(!newsFile[0]) {
        Q_strncpyz(newsFile, fileSystem->BuildOSPath(
                       cvarSystem->VariableString("fs_homepath"), "", "news.dat"), MAX_QPATH);
        newsFile[MAX_QPATH - 1] = 0;
    }

    if(begin) {   // if not already using curl, start the download
        if(!clc.bWWWDl) {
            clc.bWWWDl = true;
            downloadSystem->BeginDownload(newsFile,
                                          "http://tremulous.net/clientnews.txt", com_developer->integer);
            cls.bWWWDlDisconnected = true;
            return false;
        }
    }

    if(fileSystem->SV_FOpenFileRead(newsFile, &clc.download)) {
        readSize = fileSystem->Read(clc.newsString, sizeof(clc.newsString),
                                    clc.download);
        clc.newsString[ readSize ] = '\0';

        if(readSize > 0) {
            finished = true;
            clc.bWWWDl = false;
            cls.bWWWDlDisconnected = false;
        }
    }

    fileSystem->FCloseFile(clc.download);

    if(!finished) {
        Q_strcpy_s(clc.newsString, "Retrieving...");
    }

    cvarSystem->Set("cl_newsString", clc.newsString);
    return finished;
}

/*
====================
idClientGUISystemLocal::GetGlConfig
====================
*/
void idClientGUISystemLocal::GetGlconfig(vidconfig_t *config) {
    *config = cls.glconfig;
}

/*
====================
idClientGUISystemLocal::GUIGetClipboarzdData
====================
*/
void idClientGUISystemLocal::GUIGetClipboardData(valueType *buf,
        uint64 buflen) {
    valueType *cbd;

    cbd = idsystem->SysGetClipboardData();

    if(!cbd) {
        *buf = 0;
        return;
    }

    Q_strncpyz(buf, cbd, buflen);

    Z_Free(cbd);
}

/*
====================
idClientGUISystemLocal::KeynumToStringBuf
====================
*/
void idClientGUISystemLocal::KeynumToStringBuf(sint keynum, valueType *buf,
        uint64 buflen) {
    Q_strncpyz(buf, Key_KeynumToString(keynum), buflen);
}

/*
====================
idClientGUISystemLocal::GetBindingBuf
====================
*/
void idClientGUISystemLocal::GetBindingBuf(sint keynum, valueType *buf,
        uint64 buflen) {
    valueType *value;

    value = Key_GetBinding(keynum);

    if(value) {
        Q_strncpyz(buf, value, buflen);
    } else {
        *buf = 0;
    }
}

/*
====================
idClientGUISystemLocal::GetCatcher
====================
*/
sint idClientGUISystemLocal::GetCatcher(void) {
    return cls.keyCatchers;
}

/*
====================
idClientGUISystemLocal::SetCatcher
====================
*/
void idClientGUISystemLocal::SetCatcher(sint catcher) {
    // console overrides everything
    if(cls.keyCatchers & KEYCATCH_CONSOLE) {
        cls.keyCatchers = catcher | KEYCATCH_CONSOLE;
    } else {
        cls.keyCatchers = catcher;
    }

}

/*
====================
idClientGUISystemLocal::GetConfigString
====================
*/
bool idClientGUISystemLocal::GetConfigString(sint index, valueType *buf,
        uint64 size) {
    uint64 offset;

    if(index < 0 || index >= MAX_CONFIGSTRINGS) {
        return false;
    }

    offset = cl.gameState.stringOffsets[index];

    if(!offset) {
        if(size) {
            buf[0] = 0;
        }

        return false;
    }

    Q_strncpyz(buf, cl.gameState.stringData + offset, size);

    return true;
}

/*
====================
idClientGUISystemLocal::ShutdownGUI
====================
*/
void idClientGUISystemLocal::ShutdownGUI(void) {
    cls.keyCatchers &= ~KEYCATCH_UI;
    cls.uiStarted = false;

    if(uiManager == nullptr || uivm == nullptr) {
        return;
    }

    uiManager->Shutdown();
    uiManager = nullptr;

    idsystem->UnloadDll(uivm);
    uivm = nullptr;
}

/*
====================
idClientGUISystemLocal::CreateExportTable
====================
*/
void idClientGUISystemLocal::CreateExportTable(void) {
    exports.Print = Com_Printf;
    exports.Error = Com_Error;

    exports.RealTime = Com_RealTime;
    exports.CheckAutoUpdate = CL_CheckAutoUpdate;
    exports.GetAutoUpdate = CL_GetAutoUpdate;
    exports.SetBinding = Key_SetBinding;
    exports.IsDown = Key_IsDown;
    exports.GetBindingByString = Key_GetBindingByString;
    exports.GetOverstrikeMode = Key_GetOverstrikeMode;
    exports.SetOverstrikeMode = Key_SetOverstrikeMode;
    exports.ClearStates = Key_ClearStates;
    exports.Hunk_MemoryRemaining = Hunk_MemoryRemaining;
    exports.TranslateString = CL_TranslateString;
    exports.OpenURL = CL_OpenURL;
    exports.GetHunkInfo = Com_GetHunkInfo;

    exports.clientCinemaSystem = clientCinemaSystem;
    exports.renderSystem = renderSystem;
    exports.soundSystem = soundSystem;
    exports.fileSystem = fileSystem;
    exports.cvarSystem = cvarSystem;
    exports.cmdBufferSystem = cmdBufferSystem;
    exports.cmdSystem = cmdSystem;
    exports.idsystem = idsystem;
    exports.idcgame = cgame;
    exports.idLANSystem = clientLANSystem;
    exports.idGUISystem = clientGUISystem;
    exports.clientScreenSystem = clientScreenSystem;
    exports.parseSystem = ParseSystem;
}

/*
====================
idClientGUISystemLocal::InitGUI
====================
*/
void idClientGUISystemLocal::InitGUI(void) {
    sint t1, t2;

    t1 = idsystem->Milliseconds();

    // load the GUI module
    uivm = idsystem->LoadDll("gui");

    if(!uivm) {
        Com_Error(ERR_DROP, "cannot load client gui dynamic module.\n");
    }

    // Load in the entry point.
    guiEntry = (idUserInterfaceManager * (QDECL *)(guiImports_t *))
               idsystem->GetProcAddress(uivm, "guiEntry");

    if(!guiEntry) {
        Com_Error(ERR_DROP, "error loading entry point on client gui.\n");
    }

    // Create the export table.
    CreateExportTable();

    // Call the dll entry point.
    if(guiEntry) {
        uiManager = guiEntry(&exports);
    }

    uiManager->Init(cls.state >= CA_AUTHORIZING && cls.state < CA_ACTIVE);

    t2 = idsystem->Milliseconds();

    Com_Printf("idClientGUISystemLocal::InitGUI: %5.2f seconds\n",
               (t2 - t1) / 1000.0);
}

/*
====================
idClientGUISystemLocal::checkKeyExec
====================
*/
bool idClientGUISystemLocal::checkKeyExec(sint key) {
    if(uivm) {
        return uiManager->CheckExecKey(key);
    } else {
        return false;
    }
}

/*
====================
idClientGUISystemLocal::GameCommand

See if the current console command is claimed by the ui
====================
*/
bool idClientGUISystemLocal::GameCommand(void) {
    if(!cls.uiStarted) {
        return false;
    }

    return uiManager->ConsoleCommand(cls.realtime);
}
