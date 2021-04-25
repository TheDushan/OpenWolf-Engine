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
// File name:   downloadMain.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

/* Additional features that would be nice for this code:
    * Only display <gamepath>/<file>, i.e., etpro/etpro-3_0_1.pk3 in the UI.
    * Add server as referring URL
*/

#ifdef UPDATE_SERVER
#include <server/serverAutoPrecompiled.hpp>
#elif DEDICATED
#include <server/serverDedPrecompiled.hpp>
#else
#include <framework/precompiled.hpp>
#endif

idDownloadSystemLocal downloadLocal;
idDownloadSystem *downloadSystem = &downloadLocal;

/*
===============
idDownloadSystemLocal::idDownloadSystemLocal
===============
*/
idDownloadSystemLocal::idDownloadSystemLocal(void) {
}

/*
===============
idDownloadSystemLocal::~idDownloadSystemLocal
===============
*/
idDownloadSystemLocal::~idDownloadSystemLocal(void) {
}

/*
===============
idDownloadSystemLocal:FWriteFile

Write to file
===============
*/
uint32 idDownloadSystemLocal::FWriteFile(void *ptr, uint32 size,
        uint32 nmemb, void *stream) {
    FILE *file = (FILE *) stream;

    return fwrite(ptr, size, nmemb, file);
}

/*
===============
idDownloadSystemLocal::Progress

Print progress
===============
*/
sint idDownloadSystemLocal::Progress(void *clientp, float64 dltotal,
                                     float64 dlnow, float64 ultotal, float64 ulnow) {
    // zinx
    // cl_downloadSize and cl_downloadTime are set by the Q3 protocol...
    // and it would probably be expensive to verify them here.

    cvarSystem->SetValue("cl_downloadCount", static_cast<float32>(dlnow));
    return 0;
}

/*
===============
idDownloadSystemLocal::InitDownload
===============
*/
void idDownloadSystemLocal::InitDownload(void) {
    if(dl_initialized) {
        return;
    }

    /* Make sure curl has initialized, so the cleanup doesn't get confused */
    curl_global_init(CURL_GLOBAL_ALL);

    dl_multi = curl_multi_init();

    Com_Printf("Client download subsystem initialized\n");
    dl_initialized = 1;
}

/*
================
idDownloadSystemLocal::Shutdown
================
*/
void idDownloadSystemLocal::Shutdown(void) {
    if(!dl_initialized) {
        return;
    }

    curl_multi_cleanup(dl_multi);
    dl_multi = nullptr;

    curl_global_cleanup();

    dl_initialized = 0;
}

/*
===============
idDownloadSystemLocal::BeginDownload

inspired from http://www.w3.org/Library/Examples/LoadToFile.c
setup the download, return once we have a connection
===============
*/
sint idDownloadSystemLocal::BeginDownload(pointer localName,
        pointer remoteName, sint debug) {
    valueType referer[MAX_STRING_CHARS + 5 /*"ET://" */ ];

    if(dl_request) {
        Com_Printf("ERROR: idDownloadSystemLocal::BeginDownload called with a download request already active\n");
        return 0;
    }

    if(!localName || !remoteName) {
        Com_DPrintf("Empty download URL or empty local file name\n");
        return 0;
    }

    fileSystem->CreatePath(localName);
    dl_file = fopen(localName, "wb+");

    if(!dl_file) {
        Com_Printf("ERROR: idDownloadSystemLocal::BeginDownload unable to open '%s' for writing\n",
                   localName);
        return 0;
    }

    InitDownload();

    /* ET://ip:port */
    ::Q_strcpy_s(referer, "ET://");
    Q_strncpyz(referer + 5, cvarSystem->VariableString("cl_currentServerIP"),
               MAX_STRING_CHARS);

    dl_request = curl_easy_init();
    curl_easy_setopt(dl_request, CURLOPT_USERAGENT, va("%s %s",
                     APP_NAME "/" APP_VERSION, curl_version()));
    curl_easy_setopt(dl_request, CURLOPT_REFERER, referer);
    curl_easy_setopt(dl_request, CURLOPT_URL, remoteName);
    curl_easy_setopt(dl_request, CURLOPT_WRITEFUNCTION, FWriteFile);
    curl_easy_setopt(dl_request, CURLOPT_WRITEDATA,
                     static_cast<void *>(dl_file));
    curl_easy_setopt(dl_request, CURLOPT_PROGRESSFUNCTION, Progress);
    curl_easy_setopt(dl_request, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(dl_request, CURLOPT_FAILONERROR, 1);

    curl_multi_add_handle(dl_multi, dl_request);

    cvarSystem->Set("cl_downloadName", remoteName);

    return 1;
}

/*
===============
dlStatus_t DL_DownloadLoop( void)

(maybe this should be CL_DL_DownloadLoop)
===============
*/
dlStatus_t idDownloadSystemLocal::DownloadLoop(void) {
    CURLMcode status;
    CURLMsg *msg;
    sint dls = 0;
    pointer err = nullptr;

    if(!dl_request) {
        Com_DPrintf("idDownloadSystemLocal::DownloadLoop: unexpected call with dl_request == nullptr\n");
        return DL_DONE;
    }

    if((status = curl_multi_perform(dl_multi,
                                    &dls)) == CURLM_CALL_MULTI_PERFORM && dls) {
        return DL_CONTINUE;
    }

    while((msg = curl_multi_info_read(dl_multi, &dls)) &&
            msg->easy_handle != dl_request)
        ;

    if(!msg || msg->msg != CURLMSG_DONE) {
        return DL_CONTINUE;
    }

    if(msg->data.result != CURLE_OK) {
#ifdef __MACOS__                // ���
        err = "unknown curl error.";
#else
        err = curl_easy_strerror(msg->data.result);
#endif
    } else {
        err = nullptr;
    }

    curl_multi_remove_handle(dl_multi, dl_request);
    curl_easy_cleanup(dl_request);

    fclose(dl_file);
    dl_file = nullptr;

    dl_request = nullptr;

    cvarSystem->Set("ui_dl_running", "0");

    if(err) {
        Com_DPrintf("idDownloadSystemLocal::DownloadLoop: request terminated with failure status '%s'\n",
                    err);
        return DL_FAILED;
    }

    return DL_DONE;
}
