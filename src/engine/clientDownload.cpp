/*
===========================================================================

OpenWolf GPL Source Code
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2009 Cyril Gantin
Copyright (C) 2011 Dusan Jocic <dusanjocic@msn.com>

OpenWolf is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

OpenWolf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include <framework/precompiled.hpp>

static convar_t
*dl_verbose;    // 1: show http headers; 2: http headers +curl debug info
static convar_t
*dl_showprogress;   // 0: do not show; 1: show console progress; 2: show progress in one line
static convar_t *dl_showmotd;   // show server message
static convar_t
*dl_source; // url to query maps from; %m token will be replaced by mapname
static convar_t
*dl_usemainfolder;  // whether to download pk3 files in main folder (default is on)

static bool curl_initialized;
static valueType useragent[256];
static CURL *curl = NULL;
static CURLM *curlm = NULL;
static fileHandle_t f = 0;
static valueType path[MAX_OSPATH];
static valueType
dl_error[1024];    // if set, will be used in place of libcurl's error message.
static valueType motd[128];

idClientDownloadSystemLocal clientDownloadLocal;
idClientDownloadSystem *clientDownloadSystem = &clientDownloadLocal;

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

/*
===============
idClientDownloadSystemLocal::Curl_WriteCallback_f
===============
*/
sint64 idClientDownloadSystemLocal::Curl_WriteCallback_f(void *ptr,
        sint64 size, sint64 nmemb, void *stream) {
    if(!f) {
        valueType dir[MAX_OSPATH];
        valueType *c;

        // make sure Content-Type is either "application/octet-stream" or "application/zip".
        if(curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &c) != CURLE_OK
                || !c
                || (stricmp(c, "application/octet-stream")
                    && stricmp(c, "application/zip"))) {
            Q_strncpyz(dl_error,
                       "No pk3 returned - requested map is probably unknown.", sizeof(dl_error));
            return 0;
        }

        // make sure the path doesn't have directory information.
        for(c = path; *c; c++) {
            if(*c == '\\' || *c == '/' || *c == ':') {
                Q_vsprintf_s(dl_error, sizeof(dl_error), sizeof(dl_error),
                             "Destination filename \"%s\" is not valid.", path);
                return 0;
            }
        }

        // make sure the file has an appropriate extension.
        c = path + strlen(path) - 4;

        if(c <= path || strcmp(c, ".pk3")) {
            Q_vsprintf_s(dl_error, sizeof(dl_error), sizeof(dl_error),
                         "Returned file \"%s\" has wrong extension.", path);
            return 0;
        }


        // in case of a name collision, just fail - leave it to the user to sort out.
        if(fileSystem->SV_FileExists(path)) {
            Q_vsprintf_s(dl_error, sizeof(dl_error), sizeof(dl_error),
                         "Failed to download \"%s\", a pk3 by that name exists locally.", path);
            return 0;
        }

        // change the extension to .tmp - it will be changed back once the download is complete.
        c = path + strlen(path) - 4;
        strcpy(c, ".tmp");

        // FS should write the file in the appropriate gamedir and catch unsanitary paths.
        f = fileSystem->SV_FOpenFileWrite(path);

        if(!f) {
            Q_vsprintf_s(dl_error, sizeof(dl_error), sizeof(dl_error),
                         "Failed to open \"%s\" for writing.\n", path);
            return 0;
        }

        Com_Printf("Writing to: %s\n", path);
    }

    return fileSystem->Write(ptr, size * nmemb, f);
}

/*
===============
idClientDownloadSystemLocal::Curl_WriteCallback_f
===============
*/
sint64 idClientDownloadSystemLocal::Curl_HeaderCallback_f(void *ptr,
        sint64 size, sint64 nmemb, void *stream) {
    valueType buf[1024], * c;

    // make a copy and remove the trailing crlf chars.
    if(size * nmemb >= sizeof(buf)) {
        Q_strncpyz(dl_error, "Curl_HeaderCallback_f() overflow.",
                   sizeof(dl_error));
        return (size_t) -1;
    }

    Q_strncpyz(buf, static_cast<pointer>(ptr), size * nmemb + 1);
    c = buf + strlen(buf) - 1;

    while(c > buf && (*c == '\r' || *c == '\n')) {
        *(c--) = 0;
    }

    // make sure it's not empty.
    if(c <= buf) {
        return size * nmemb;
    }

    // verbose output
    if(dl_verbose->integer > 0) {
        Com_Printf("< %s\n", buf);
    }

    /**
     * Check whether this is a content-disposition header.
     * Apparently RFC2183 has precise rules for the presentation of the filename attribute.
     * No one seems to respect those, though.
     * Accepted presentations:
     *  filename="somefile.pk3"
     *  filename='somefile.pk3'
     *  filename=somefile.pk3
     * Quoted strings won't support escaping (eg. "some\"file.pk3").
     * Malformed quoted strings that miss the trailing quotation mark will pass.
     * Only us-ascii chars are accepted.
     * The actual filename will be validated later, when the transfer is started.
     */
    if(!Q_strncasecmp(buf, "content-disposition:", 20)) {
        pointer c = strstr(buf, "filename=") + 9;

        if(c != (valueType *)9) {
            pointer e;
            valueType token = 0;

            if(*c == '"' || *c == '\'') {
                token = *c++;
            }

            for(e = c; *e && *e != token; e++) {
                if(*e < 32 || *e > 126) {
                    Q_strncpyz(dl_error, "Server returned an invalid filename.",
                               sizeof(dl_error));
                    return (size_t) -1;
                }
            }

            if(e == c || e - c >= sizeof(path)) {
                Q_strncpyz(dl_error, "Server returned an invalid filename.",
                           sizeof(dl_error));
                return (size_t) -1;
            }

            Q_strncpyz(path, c, e - c + 1); // +1 makes room for the trailing \0
        }
    }

    // catch x-dfengine-motd headers
    if(!Q_strncasecmp(buf, "x-dfengine-motd: ", 17)) {
        if(strlen(buf) >= 17 + sizeof(motd)) {
            if(dl_showmotd->integer) {
                Com_Printf("Warning: server motd string too large.\n");
            }
        } else {
            Q_strncpyz(motd, buf + 17, sizeof(motd));
            cvarSystem->Set("cl_downloadMotd", motd);
        }
    }

    return size * nmemb;
}

/*
===============
idClientDownloadSystemLocal::Curl_VerboseCallback_f
===============
*/
sint64 idClientDownloadSystemLocal::Curl_VerboseCallback_f(CURL *curl,
        curl_infotype type,
        valueType *data, size_t size, void *userptr) {
    valueType buf[1024];
    valueType *c, * l;

    if((type != CURLINFO_HEADER_OUT || dl_verbose->integer < 1)
            && (type != CURLINFO_TEXT || dl_verbose->integer < 2)) {
        return 0;
    }

    if(size >= sizeof(buf)) {
        Com_Printf("Curl_VerboseCallback_f() warning: overflow.\n");
        return 0;
    }

    Q_strncpyz(buf, data, size + 1);    // +1 makes room for the trailing \0

    if(type == CURLINFO_HEADER_OUT) {
        for(l = c = buf; c - buf < size; c++) {
            // header lines should have linefeeds.
            if(*c == '\n' || *c == '\r') {
                *c = 0;

                if(c > l) {
                    Com_Printf("> %s\n", l);
                }

                l = c + 1;
            }
        }

        return 0;
    }

    // CURLINFO_TEXT (has its own linefeeds)
    Com_Printf("%s",
               buf);  // Com_Printf(buf) would result in random output/segfault if buf has % chars.
    return 0;
}

/*
===============
idClientDownloadSystemLocal::Curl_ProgressCallback_f

This callback is called on regular intervals, whether data is being transferred or not.
===============
*/
sint idClientDownloadSystemLocal::Curl_ProgressCallback_f(void *clientp,
        float32 dltotal,
        float32 dlnow, float32 ultotal, float32 ulnow) {

    if(curlm) { // dont print progress in console if nonblocking download
        clientDownloadLocal.Info(false);
    } else { // print progress if blocking download
        clientDownloadLocal.Info(true);

        // pump events and refresh screen
        Com_EventLoop();
        clientScreenSystem->UpdateScreen();

        if(Key_IsDown(K_ESCAPE)) {
            Q_strncpyz(dl_error, "Download aborted.", sizeof(dl_error));
            return -1;
        }
    }

    return 0;
}

/*
===============
idClientDownloadSystemLocal::Curl_ShowVersion_f
===============
*/
void idClientDownloadSystemLocal::Curl_ShowVersion_f(void) {
    Com_Printf("%s\n", curl_version());
}

/*
===============
idClientDownloadSystemLocal::Curl_Download_f
===============
*/
void idClientDownloadSystemLocal::Curl_Download_f(void) {
    bool nonblocking;
    sint state;

    // interrupt download: \download -
    if(cmdSystem->Argc() >= 2 && !Q_strncmp("-", cmdSystem->Argv(1), 1)) {
        if(clientDownloadLocal.Active()) {
            clientDownloadLocal.Interrupt();
        } else {
            Com_Printf("No download in progress.\n");
        }

        return;
    }

    if(clientDownloadLocal.Active()) {
        Com_Printf("Already downloading map '%s'.\n",
                   cvarSystem->VariableString("cl_downloadName"));
        clientDownloadLocal.Info(true);
        return;
    }

    // help: \download
    if(cmdSystem->Argc() < 2) {
        Com_Printf("How to use:\n"
                   " \\download <mapname>     - blocking download ( hold ESC to abort )\n"
                   " \\download <mapname> &   - background download\n"
                   " \\download -             - abort current background download\n"
                   " \\download               - show help or background download progress\n"
                  );
        return;
    }

    // non blocking download: \download <mapname> &
    nonblocking = (cmdSystem->Argc() >= 3) &&
                  (!Q_strncmp("&", cmdSystem->Argv(2), 1));

    // initialize download
    state = clientDownloadLocal.Begin(cmdSystem->Argv(1), nonblocking);

    if(state != 1) {
        return;
    }

    // if blocking, download all file now
    if(!nonblocking) {
        clientDownloadLocal.Continue();
    }
}

/*
===============
idClientDownloadSystemLocal::Init
===============
*/
void idClientDownloadSystemLocal::Init(void) {
    if(!curl_global_init(CURL_GLOBAL_ALL)) {
        valueType *c;

        cmdSystem->AddCommand("curl_version", Curl_ShowVersion_f,
                              "Show cURL version");
        cmdSystem->AddCommand("pakdownload", Curl_Download_f, "Download pak file");
        curl_initialized = true;

        // set user-agent, something along the lines of "dfengine/1.## (libcurl/#.##.# linked libs...)"
        Q_strncpyz(useragent, Q3_VERSION, sizeof(useragent));

        for(c = useragent; *c; c++) {
            if(*c == ' ') {
                *c = '/';
            }
        }

        Q_vsprintf_s(useragent, sizeof(useragent), sizeof(useragent), "%s (%s) ",
                     useragent,
                     curl_version());
    } else {
        Com_Printf("Failed to initialize libcurl.\n");
    }

    dl_verbose = cvarSystem->Get("dl_verbose", "0", 0,
                                 "1: show http headers; 2: http headers +curl debug info");
    dl_source = cvarSystem->Get("dl_source",
                                "http://ws.q3df.org/getpk3bymapname.php/%m", CVAR_ARCHIVE,
                                "Url to query maps from; %m token will be replaced by mapname");
    dl_showprogress = cvarSystem->Get("dl_showprogress", "1", CVAR_ARCHIVE,
                                      "0: do not show; 1: show console progress; 2: show progress in one line");
    dl_showmotd = cvarSystem->Get("dl_showmotd", "1", CVAR_ARCHIVE,
                                  "show server message");
    dl_usemainfolder = cvarSystem->Get("dl_usemainfolder", "0", CVAR_ARCHIVE,
                                       "Whether to download pk3 files in main folder (default is on)");
}

/*
===============
idClientDownloadSystemLocal::Active

0 : no active,  1 : active blocking,  2 : active nonblocking
===============
*/
sint idClientDownloadSystemLocal::Active(void) {
    if(curlm && curl) {
        return 2;
    }

    if(curl) {
        return 1;
    }

    return 0;
}

/*
===============
idClientDownloadSystemLocal::Shutdown

the engine might be going dedicated, remove client commands
===============
*/
void idClientDownloadSystemLocal::Shutdown(void) {
    if(curl_initialized) {
        if(curlm) {
            curl_multi_cleanup(curlm);
            curlm = NULL;
        }

        if(curl) {
            curl_easy_cleanup(curl);
            curl = NULL;
        }

        if(f) {
            fileSystem->FCloseFile(f);
            f = 0;
        }

        curl_global_cleanup();
        curl_initialized = false;
        cmdSystem->RemoveCommand("curl_version");
        cmdSystem->RemoveCommand("download");
    }
}

/*
===============
idClientDownloadSystemLocal::Shutdown

-1 : error,  0 : map already exists,  1 : ok
===============
*/
sint idClientDownloadSystemLocal::Begin(pointer map, bool nonblocking) {
    valueType url[1024];
    CURLMcode resm;
    valueType *c;

    if(Active()) {
        Com_Printf("Already downloading map '%s'.\n",
                   cvarSystem->VariableString("cl_downloadName"));
        return -1;
    }

    if(fileSystem->FileIsInPAK(va("maps/%s.bsp", map), NULL) != -1) {
        Com_Printf("Map already exists locally.\n");
        return 0;
    }

    if(Q_strncasecmp(dl_source->string, "http://", 7)) {
        if(strstr(dl_source->string, "://")) {
            Com_Printf("Invalid dl_source.\n");
            return -1;
        }

        cvarSystem->Set("dl_source", va("http://%s", dl_source->string));
    }

    if((c = strstr(dl_source->string, "%m")) == 0) {
        Com_Printf("Cvar dl_source is missing a %%m token.\n");
        return -1;
    }

    if(strlen(dl_source->string) - 2 + strlen(curl_easy_escape(curl, map,
            0)) >= sizeof(url)) {
        Com_Printf("Cvar dl_source too large.\n");
        return -1;
    }

    Q_strncpyz(url, dl_source->string,
               c - dl_source->string + 1);  // +1 makes room for the trailing 0
    Q_vsprintf_s(url, sizeof(url), sizeof(url), "%s%s%s", url,
                 curl_easy_escape(curl, map,
                                  0), c + 2);

    // set a default destination filename; Content-Disposition headers will override.
    Q_vsprintf_s(path, sizeof(path), sizeof(path), "%s.pk3", map);

    curl = curl_easy_init();

    if(!curl) {
        Com_Printf("Download failed to initialize.\n");
        return -1;
    }

    *dl_error = 0;
    *motd = 0;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, useragent);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR,
                     1); // fail if http returns an error code
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Curl_WriteCallback_f);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, Curl_HeaderCallback_f);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, Curl_VerboseCallback_f);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, Curl_ProgressCallback_f);
    //curl_easy_setopt(curl, CURLOPT_MAX_RECV_SPEED_LARGE, (curl_off_t)(4*1024) ); // 4 KB/s for testing timeouts

    if(nonblocking) {
        curlm = curl_multi_init();

        if(!curlm) {
            curl_easy_cleanup(curl);
            curl = NULL;
            Com_Printf("Download failed to initialize ( nonblocking ).\n");
            return -1;
        }

        resm = curl_multi_add_handle(curlm, curl);

        if(resm != CURLM_OK) {
            curl_multi_cleanup(curlm);
            curl_easy_cleanup(curl);
            curlm = NULL;
            curl = NULL;
            Com_Printf("Download failed to initialize ( nonblocking ).\n");
            return -1;
        }
    }

    Com_Printf("Attempting download: %s\n", url);

    cvarSystem->Set("cl_downloadName",
                    map);  // show the ui download progress screen
    cvarSystem->SetValue("cl_downloadSize", 0);
    cvarSystem->SetValue("cl_downloadCount", 0);
    cvarSystem->SetValue("cl_downloadTime",
                         cls.realtime); // download start time offset

    return 1;
}

/*
===============
idClientDownloadSystemLocal::End
===============
*/
void idClientDownloadSystemLocal::End(CURLcode res, CURLMcode resm) {
    CURLMsg *msg;
    sint msgs;

    if(dl_verbose->integer == 0 && dl_showprogress->integer == 2 && !curlm) {
        Com_Printf("\n");
    }

    if(curlm) {
        // res = final download result
        while(msg = curl_multi_info_read(curlm, &msgs)) {
            if(msg->msg != CURLMSG_DONE) {
                if(dl_error[0] == '\0') {
                    Q_strncpyz(dl_error, "Download Interrupted.", sizeof(dl_error));
                }
            } else if(msg->easy_handle == curl) {
                if(msg->data.result != CURLE_OK);

                res = msg->data.result;
            } else {
                Com_Printf("Invalid cURL handle.\n");
            }
        }

        curl_multi_cleanup(curlm);
        curlm = NULL;
    }

    if(curl) {
        curl_easy_cleanup(curl);
        curl = NULL;
    }

    // get possible error messages
    if(!*dl_error && res != CURLE_OK) {
        Q_strncpyz(dl_error, curl_easy_strerror(res), sizeof(dl_error));
    }

    if(!*dl_error && resm != CURLM_OK) {
        Q_strncpyz(dl_error, curl_multi_strerror(resm), sizeof(dl_error));
    }

    if(!*dl_error && !f) {
        Q_strncpyz(dl_error, "File is not opened.", sizeof(dl_error));
    }

    if(f) {
        fileSystem->FCloseFile(f);
        f = 0;

        if(!*dl_error) {    // download succeeded
            valueType dest[MAX_OSPATH];
            Com_Printf("Download complete, restarting filesystem.\n");
            Q_strncpyz(dest, path, strlen(path) - 3);   // -4 +1 for the trailing \0
            Q_strcat(dest, sizeof(dest), ".pk3");

            if(!fileSystem->SV_FileExists(dest)) {
                fileSystem->SV_Rename(path, dest);
                fileSystem->Restart(clc.checksumFeed);

                if(dl_showmotd->integer && *motd) {
                    Com_Printf("Server motd: %s\n", motd);
                }
            } else {
                // normally such errors should be caught upon starting the transfer. Anyway better do
                // it here again - the filesystem might have changed, plus this may help contain some
                // bugs / exploitable flaws in the code.
                Com_Printf("Failed to copy downloaded file to its location - file already exists.\n");
                fileSystem->SV_RemoveFile(path);
            }
        } else {
            fileSystem->SV_RemoveFile(path);
        }
    }

    cvarSystem->Set("cl_downloadName", "");  // hide the ui downloading screen
    cvarSystem->SetValue("cl_downloadSize", 0);
    cvarSystem->SetValue("cl_downloadCount", 0);
    cvarSystem->SetValue("cl_downloadTime", 0);
    cvarSystem->Set("cl_downloadMotd", "");

    if(*dl_error) {
        if(cls.state == CA_CONNECTED) {
            Com_Error(ERR_DROP, "%s\n",
                      dl_error);    // download error while connecting, can not continue loading
        } else {
            Com_Printf("%s\n",
                       dl_error);    // download error while in game, do not disconnect
        }

        *dl_error = '\0';
    } else {
        // download completed, request new gamestate to check possible new map if we are not already in game
        if(cls.state == CA_CONNECTED) {
            CL_AddReliableCommand("donedl");    // get new gamestate info from server
        }
    }
}

/*
===============
idClientDownloadSystemLocal::Continue

-1 : error,  0 : done,  1 : continue
===============
*/
sint idClientDownloadSystemLocal::Continue(void) {
    CURLcode  res = CURLE_OK;
    CURLMcode resm = CURLM_OK;
    sint running = 1;
    sint state = -1;

    if(!curl) {
        return 0;
    }

    if(curlm) { // non blocking
        resm = CURLM_CALL_MULTI_PERFORM;

        while(resm == CURLM_CALL_MULTI_PERFORM) {
            resm = curl_multi_perform(curlm, &running);
        }

        if(resm == CURLM_OK) {
            state = (running ? 1 : 0);
        }
    } else { // blocking
        // NOTE:
        // blocking download has its own curl loop, so we need check events and update screen in Curl_ProgressCallback_f,
        // this loop is not updating time cvars ( com_frameMsec, cls.realFrametime, cls.frametime, cls.realtime, ... )
        // and this will cause client-server desynchronization, if download takes long time timeout can occur.
        res = curl_easy_perform(
                  curl); // returns when error or download is completed/aborted
        state = (res == CURLE_OK ? 0 : -1);
    }

    if(state != 1) { // no continue ( done or error )
        End(res, resm);
    }

    return state;
}

/*
===============
idClientDownloadSystemLocal::Interrupt
===============
*/
void idClientDownloadSystemLocal::Interrupt(void) {
    if(!curl) {
        return;
    }

    Q_strncpyz(dl_error, "Download Interrupted.", sizeof(dl_error));
    End(CURLE_OK, CURLM_OK);
}

/*
===============
idClientDownloadSystemLocal::Info
===============
*/
void idClientDownloadSystemLocal::Info(bool console) {
#if 1
    static float32 lastTime = 0.0;
    float32 dltotal, dlnow, speed, time;
    sint timeleft;
    CURLcode res;

    if(!Active()) {
        return;
    }

    if(cls.state != CA_CONNECTED && !console) {
        return;
    }

    res = curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME,
                            &time);                  // total downloading time

    if(res != CURLE_OK) {
        time = -1.0;
    }

    res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD,
                            &dltotal);  // file size bytes

    if(res != CURLE_OK || dltotal < 0.0) {
        dltotal = 0.0;
    }

    res = curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD,
                            &dlnow);              // current bytes

    if(res != CURLE_OK || dlnow < 0.0) {
        dlnow = 0.0;
    }

    if(dltotal > 0.0 && dlnow > dltotal) {
        dlnow = 0.0;
    }

    res = curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD,
                            &speed);             // download rate bytes/sec

    if(res != CURLE_OK) {
        speed = -1.0;
    }

    // update ui download progress screen cvars
    if(cls.state == CA_CONNECTED) {
        cvarSystem->SetValue("cl_downloadSize", (float32)dltotal);
        cvarSystem->SetValue("cl_downloadCount", (float32)dlnow);
    }

    // print download progress in console
    if(console && dl_showprogress->integer && dlnow > 0.0) {
        // 8 times per second is enough
        if(time - lastTime > 1.0 / 8.0 || time - lastTime < 0.0 ||
                dlnow == dltotal) {
            lastTime = time;

            if(dl_verbose->integer == 0 && dl_showprogress->integer == 2 && !curlm) {
                Com_Printf("\r");    // overwrite old progress line
            }

            if(dltotal != 0.0   // content-size is known
                    && dlnow <= dltotal) {  // and appropriate
                if(dltotal > 1024.0 * 1024.0) {     // MB range
                    Com_Printf("%.1f/%.1fMB", dlnow / 1024.0 / 1024.0,
                               dltotal / 1024.0 / 1024.0);
                } else if(dltotal > 10240.0) {      // KB range (>10KB)
                    Com_Printf("%.1f/%.1fKB", dlnow / 1024.0, dltotal / 1024.0);
                } else {                        // byte range
                    Com_Printf("%.0f/%.0fB", dlnow, dltotal);
                }
            } else { // unknown content-size
                if(dlnow > 1024.0 * 1024.0) {       // MB range
                    Com_Printf("%.1fMB", dlnow / 1024.0 / 1024.0);
                } else if(dlnow > 10240.0) {    // KB range (>10KB)
                    Com_Printf("%.1fKB", dlnow / 1024.0);
                } else {                        // byte range
                    Com_Printf("%.0fB", dlnow);
                }
            }

            if(speed >= 0.0) {
                Com_Printf(" @%.1fKB/s", speed / 1024.0);
            }

            if(dltotal != 0.0 && dlnow <= dltotal) {
                Com_Printf(" (%2.1f%%)", 100.0 * dlnow / dltotal);

                if(time > 0.0 && dlnow > 0.0) {
                    timeleft = (sint)((dltotal - dlnow) * time / dlnow);
                    Com_Printf(" time left: %d:%0.2d", timeleft / 60, timeleft % 60);
                }
            }

            if(dl_verbose->integer == 0 && dl_showprogress->integer == 2 && !curlm) {
                Com_Printf("      ");    // make sure line is totally overwriten
            } else {
                Com_Printf("\n");    // or start a new line
            }
        }
    }

#endif
}
