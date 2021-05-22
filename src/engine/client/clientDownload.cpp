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
// File name:   clientDownload.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

idClientDownloadSystemLocal clientDownloadLocal;
idClientDownloadSystemAPI *clientDownloadSystem = &clientDownloadLocal;

/*
===============
idClientDownloadSystemLocal::idClientDownloadSystemLocal
===============
*/
idClientDownloadSystemLocal::idClientDownloadSystemLocal(void) {
}

/*
===============
idClientDownloadSystemLocal::~idClientDownloadSystemLocal
===============
*/
idClientDownloadSystemLocal::~idClientDownloadSystemLocal(void) {
}

bool autoupdateChecked;
bool autoupdateStarted;
valueType autoupdateFilename[MAX_QPATH];

// "updates" shifted from -7
#define AUTOUPDATE_DIR "ni]Zm^l"
#define AUTOUPDATE_DIR_SHIFT 7

/*
=====================
idClientDownloadSystemLocal::ClearStaticDownload
Clear download information that we keep in cls (disconnected download support)
=====================
*/
void idClientDownloadSystemLocal::ClearStaticDownload(void) {
    assert(!cls.bWWWDlDisconnected);     // reset before calling
    cls.downloadRestart = false;
    cls.downloadTempName[0] = '\0';
    cls.downloadName[0] = '\0';
    cls.originalDownloadName[0] = '\0';
}

/*
=================
idClientDownloadSystemLocal::DownloadsComplete

Called when all downloading has been completed
=================
*/
void idClientDownloadSystemLocal::DownloadsComplete(void) {
#ifndef _WIN32
    valueType *fs_write_path;
#endif
    valueType *fn;


    // DHM - Nerve :: Auto-update (not finished yet)
    if(autoupdateStarted) {

        if(autoupdateFilename[0] && (strlen(autoupdateFilename) > 4)) {
#ifdef _WIN32
            // win32's Sys_StartProcess prepends the current dir
            fn = va("%s/%s", fileSystem->ShiftStr(AUTOUPDATE_DIR,
                                                  AUTOUPDATE_DIR_SHIFT), autoupdateFilename);
#else
            fs_write_path = fs_homepath->string;
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
        clientConsoleCommandSystem->Disconnect(true, "Downloads complete");
        return;
    }

    // if we downloaded files we need to restart the file system
    if(cls.downloadRestart) {
        cls.downloadRestart = false;

        fileSystem->Restart(
            clc.checksumFeed);   // We possibly downloaded a pak, restart the file system to load it

        if(!cls.bWWWDlDisconnected) {
            // inform the server so we get new gamestate info
            clientReliableCommandsSystem->AddReliableCommand("donedl");
        }

        // we can reset that now
        cls.bWWWDlDisconnected = false;
        ClearStaticDownload();

        // by sending the donedl command we request a new gamestate
        // so we don't want to load stuff yet
        return;
    }

    if(cls.bWWWDlDisconnected) {
        cls.bWWWDlDisconnected = false;
        ClearStaticDownload();
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
    clientMainSystem->FlushMemory();

    // initialize the CGame
    cls.cgameStarted = true;
    clientGameSystem->InitCGame();

    // set pure checksums
    clientMainSystem->SendPureChecksums();

    clientInputSystem->WritePacket();
    clientInputSystem->WritePacket();
    clientInputSystem->WritePacket();
}

/*
=================
idClientDownloadSystemLocal::BeginDownload

Requests a file to download from the server.  Stores it in the current
game directory.
=================
*/
void idClientDownloadSystemLocal::BeginDownload(pointer localName,
        pointer remoteName) {

    if(developer->integer) {
        Com_Printf("***** idClientDownloadSystemLocal:BeginDownload *****\n"
                   "Localname: %s\n" "Remotename: %s\n" "****************************\n",
                   localName, remoteName);
    }

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

    clientReliableCommandsSystem->AddReliableCommand(va("download %s",
            remoteName));
}

/*
=================
idClientDownloadSystemLocal::NextDownload

A download completed or failed
=================
*/
void idClientDownloadSystemLocal::NextDownload(void) {
    valueType *s;
    valueType *remoteName, * localName;

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
            DownloadsComplete();
            return;
        }

        *s++ = 0;
        localName = s;

        if((s = strchr(s, '@')) != nullptr) {
            *s++ = 0;
        } else {
            s = localName + strlen(localName);   // point at the nul byte

        }

        BeginDownload(localName, remoteName);

        cls.downloadRestart = true;

        // move over the rest
        memmove(clc.downloadList, s, strlen(s) + 1);

        return;
    }

    DownloadsComplete();
}

/*
=================
idClientDownloadSystemLocal::InitDownloads

After receiving a valid game state, we valid the cgame and local zip files here
and determine if we need to download them
=================
*/
void idClientDownloadSystemLocal::InitDownloads(void) {
#ifndef PRE_RELEASE_DEMO
    valueType            missingfiles[1024];
    valueType *dir = fileSystem->ShiftStr(AUTOUPDATE_DIR,
                                          AUTOUPDATE_DIR_SHIFT);

    // TTimo
    // init some of the www dl data
    clc.bWWWDl = false;
    clc.bWWWDlAborting = false;
    cls.bWWWDlDisconnected = false;
    ClearStaticDownload();

    if(autoupdateStarted &&
            networkSystem->CompareAdr(cls.autoupdateServer, clc.serverAddress)) {
        if(strlen(cl_updatefiles->string) > 4) {
            Q_strncpyz(autoupdateFilename, cl_updatefiles->string,
                       sizeof(autoupdateFilename));
            Q_strncpyz(clc.downloadList, va("@%s/%s@%s/%s", dir,
                                            cl_updatefiles->string, dir, cl_updatefiles->string),
                       MAX_INFO_STRING);
            cls.state = CA_CONNECTED;
            NextDownload();
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
                NextDownload();
                return;
            }
        }
    }

#endif

    DownloadsComplete();
}

/*
==================
idClientDownloadSystemLocal::WWWDownload
==================
*/
void idClientDownloadSystemLocal::WWWDownload(void) {
    valueType *to_ospath;
    dlStatus_t      ret;
    static bool bAbort = false;

    if(clc.bWWWDlAborting) {
        if(!bAbort) {
            if(developer->integer) {
                Com_Printf("idClientDownloadSystemLocal::WWWDownload: WWWDlAborting\n");
            }

            bAbort = true;
        }

        return;
    }

    if(bAbort) {
        if(developer->integer) {
            Com_Printf("idClientDownloadSystemLocal::WWWDownload: WWWDlAborting done\n");
        }

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
                        fs_homepath->string, cls.originalDownloadName, "");
        to_ospath[strlen(to_ospath) - 1] = '\0';

        if(rename(cls.downloadTempName, to_ospath)) {
            fileSystem->FSCopyFile(cls.downloadTempName, to_ospath);
            remove(cls.downloadTempName);
        }

        *cls.downloadTempName = *cls.downloadName = 0;
        cvarSystem->Set("cl_downloadName", "");

        if(cls.bWWWDlDisconnected) {
            // for an auto-update in disconnected mode, we'll be spawning the setup in idClientDownloadSystemLocal::DownloadsComplete
            if(!autoupdateStarted) {
                // reconnect to the server, which might send us to a new disconnected download
                cmdBufferSystem->ExecuteText(EXEC_APPEND, "reconnect\n");
            }
        } else {
            clientReliableCommandsSystem->AddReliableCommand("wwwdl done");

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
            ClearStaticDownload();
            Com_Error(ERR_DROP, "%s", error);
        } else {
            // see CL_ParseDownload, same abort strategy
            Com_Printf("Download failure while getting '%s'\n", cls.downloadName);
            clientReliableCommandsSystem->AddReliableCommand("wwwdl fail");
            clc.bWWWDlAborting = true;
        }

        return;
    }

    clc.bWWWDl = false;
    NextDownload();
}

/*
==================
idClientDownloadSystemLocal::WWWBadChecksum

FS code calls this when doing fileSystem->ComparePaks
we can detect files that we got from a www dl redirect with a wrong checksum
this indicates that the redirect setup is broken, and next dl attempt should NOT redirect
==================
*/
bool idClientDownloadSystemLocal::WWWBadChecksum(pointer pakname) {
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

        if(developer->integer) {
            Com_Printf("bad checksums: %s\n", clc.badChecksumList);
        }

        return true;
    }

    return false;
}

