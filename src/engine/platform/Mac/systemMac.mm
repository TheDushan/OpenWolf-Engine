////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2023 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   systemUnix.cpp
// Created:
// Compilers:   AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <server/serverAutoPrecompiled.hpp>
#elif DEDICATED
#include <server/serverDedPrecompiled.hpp>
#else
#include <framework/precompiled.hpp>
#endif

#if defined (__MACOSX__)
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>

/*
==================
idSystemLocal::DefaultHomePath
==================
*/
valueType *idSystemLocal::DefaultHomePath(valueType *buffer, sint size) {
    valueType *p;

    if(!*homePath) {
        if((p = getenv("HOME")) != nullptr) {
            Q_strncpyz(buffer, p, size);
#if defined (__MACOSX__)
            Q_strcat(buffer, size, "/Library/Application Support/"
                     PRODUCT_NAME_UPPPER);
#else
            Q_strcat(buffer, size, "/." PRODUCT_NAME_UPPPER);
#endif
        }
    }

    return buffer;
}

/*
================
idSystemLocal::TempPath
================
*/
pointer idSystemLocal::TempPath(void) {
    static uchar8 posixPath[MAX_OSPATH];
    FSRef ref;

    if(FSFindFolder(kOnAppropriateDisk,
                    kTemporaryFolderType, kCreateFolder, &ref) == noErr) {
        if(FSRefMakePath(&ref, posixPath,
                         sizeof(posixPath) - 1) == noErr) {
            return (pointer)posixPath;
        }
    }

    return "/tmp";
}

/*
==================
idSystemLocal::Chmod

chmod OR on a file
==================
*/
void idSystemLocal::Chmod(valueType *file, sint mode) {
    struct stat s_buf;
    sint perm;

    if(stat(file, &s_buf) != 0) {
        common->Printf("stat('%s')  failed: errno %d\n", file, errno);
        return;
    }

    perm = s_buf.st_mode | mode;

    if(chmod(file, perm) != 0) {
        common->Printf("chmod('%s', %d) failed: errno %d\n", file, perm, errno);
    }

    if(developer->integer) {
        common->Printf("chmod +%d '%s'\n", mode, file);
    }
}

/*
================
idSystemLocal::Milliseconds

current time in ms, using sys_timeBase as origin
NOTE: sys_timeBase*1000 + curtime -> ms since the Epoch
0x7fffffff ms - ~24 days
although timeval:tv_usec is an sint, I'm not sure wether it is actually used as an unsigned int
(which would affect the wrap period)
================
*/
sint idSystemLocal::Milliseconds(void) {
    struct timeval tp;

    gettimeofday(&tp, nullptr);

    return (tp.tv_sec - initial_tv_sec) * 1000 + tp.tv_usec / 1000;
}

/*
==================
idSystemLocal::RandomBytes
==================
*/
bool idSystemLocal::RandomBytes(uchar8 *string, uint64 len) {
    FILE *fp;

    fp = fopen("/dev/urandom", "r");

    if(!fp) {
        return false;
    }

    if(!fread(string, sizeof(uchar8), len, fp)) {
        fclose(fp);
        return false;
    }

    fclose(fp);
    return true;
}

/*
==================
idSystemLocal::GetCurrentUser
==================
*/
valueType *idSystemLocal::GetCurrentUser(void) {
    struct passwd *p;

    if((p = getpwuid(getuid())) == nullptr) {
        return "player";
    }

    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);

    return p->pw_name;
}

/*
==================
idSystemLocal::SysGetClipboardData
==================
*/
valueType *idSystemLocal::SysGetClipboardData(void) {
    valueType *data = nullptr;
#if !defined (DEDICATED)
    valueType *cliptext;

    if((cliptext = SDL_GetClipboardText()) != nullptr) {
        if(cliptext[0] != '\0') {
            uint64 bufsize = strlen(cliptext) + 1;

            data = (valueType *)memorySystem->Malloc(bufsize);
            Q_strncpyz(data, cliptext, bufsize);

            // find first listed valueType and set to '\0'
            strtok(data, "\n\r\b");
        }

        SDL_free(cliptext);
    }

#endif

    return data;
}

/*
==================
idSystemLocal::LowPhysicalMemory

TODO
==================
*/
bool idSystemLocal::LowPhysicalMemory(void) {
    return false;
}

/*
==================
idSystemLocal::Basename
==================
*/
pointer idSystemLocal::Basename(valueType *path) {
    return basename(path);
}

/*
==================
idSystemLocal::Dirname
==================
*/
pointer idSystemLocal::Dirname(valueType *path) {
    return dirname(path);
}

/*
==================
idSystemLocal::Mkdir
==================
*/
bool idSystemLocal::Mkdir(pointer path) {
    sint result = mkdir(path, 0750);

    if(result != 0) {
        return errno == EEXIST;
    }

    return true;
}

/*
==================
idSystemLocal::Cwd
==================
*/
valueType *idSystemLocal::Cwd(void) {
#if defined (__MACOSX__)
    valueType *apppath = DefaultAppPath();

    if(apppath[0] && apppath[0] != '.') {
        return apppath;
    }

#endif
    static valueType cwd[MAX_OSPATH];

    valueType *result = getcwd(cwd, sizeof(cwd) - 1);

    if(result != cwd) {
        return nullptr;
    }

    cwd[MAX_OSPATH - 1] = 0;

    return cwd;
}

/*
==================
idSystemLocal::ListFilteredFiles
==================
*/
void idSystemLocal::ListFilteredFiles(pointer basedir, valueType *subdirs,
                                      valueType *filter, valueType **list, sint *numfiles) {
    valueType          search[MAX_OSPATH], newsubdirs[MAX_OSPATH];
    valueType          filename[MAX_OSPATH];
    DIR           *fdir;
    struct dirent *d;
    struct stat   st;

    if(*numfiles >= MAX_FOUND_FILES - 1) {
        return;
    }

    if(strlen(subdirs)) {
        Q_vsprintf_s(search, sizeof(search), sizeof(search), "%s/%s", basedir,
                     subdirs);
    } else {
        Q_vsprintf_s(search, sizeof(search), sizeof(search), "%s", basedir);
    }

    if((fdir = opendir(search)) == nullptr) {
        return;
    }

    while((d = readdir(fdir)) != nullptr) {
        Q_vsprintf_s(filename, sizeof(filename), sizeof(filename), "%s/%s", search,
                     d->d_name);

        if(stat(filename, &st) == -1) {
            continue;
        }

        if(st.st_mode & S_IFDIR) {
            if(Q_stricmp(d->d_name, ".") && Q_stricmp(d->d_name, "..")) {
                if(strlen(subdirs)) {
                    Q_vsprintf_s(newsubdirs, sizeof(newsubdirs), sizeof(newsubdirs), "%s/%s",
                                 subdirs, d->d_name);
                } else {
                    Q_vsprintf_s(newsubdirs, sizeof(newsubdirs), sizeof(newsubdirs), "%s",
                                 d->d_name);
                }

                ListFilteredFiles(basedir, newsubdirs, filter, list, numfiles);
            }
        }

        if(*numfiles >= MAX_FOUND_FILES - 1) {
            break;
        }

        Q_vsprintf_s(filename, sizeof(filename), sizeof(filename), "%s/%s",
                     subdirs, d->d_name);

        if(!common->FilterPath(filter, filename, false)) {
            continue;
        }

        list[*numfiles] = memorySystem->CopyString(filename);
        (*numfiles)++;
    }

    closedir(fdir);
}

/*
==================
idSystemLocal::ListFiles
==================
*/
valueType **idSystemLocal::ListFiles(pointer directory, pointer extension,
                                     valueType *filter, sint *numfiles, bool wantsubs) {
    struct dirent *d;
    DIR *fdir;
    bool dironly = wantsubs;
    valueType search[MAX_OSPATH];
    sint nfiles;
    valueType **listCopy;
    valueType *list[MAX_FOUND_FILES];
    sint i;
    struct stat st;
    sint extLen;

    if(filter) {

        nfiles = 0;
        ListFilteredFiles(directory, "", filter, list, &nfiles);

        list[ nfiles ] = nullptr;
        *numfiles = nfiles;

        if(!nfiles) {
            return nullptr;
        }

        listCopy = (valueType **)memorySystem->Malloc((nfiles + 1) * sizeof(
                       *listCopy));

        for(i = 0 ; i < nfiles ; i++) {
            listCopy[i] = list[i];
        }

        listCopy[i] = nullptr;

        return listCopy;
    }

    if(!extension) {
        extension = "";
    }

    if(extension[0] == '/' && extension[1] == 0) {
        extension = "";
        dironly = true;
    }

    extLen = strlen(extension);

    // search
    nfiles = 0;

    if((fdir = opendir(directory)) == nullptr) {
        *numfiles = 0;
        return nullptr;
    }

    while((d = readdir(fdir)) != nullptr) {
        Q_vsprintf_s(search, sizeof(search), sizeof(search), "%s/%s", directory,
                     d->d_name);

        if(stat(search, &st) == -1) {
            continue;
        }

        if((dironly && !(st.st_mode & S_IFDIR)) ||
                (!dironly && (st.st_mode & S_IFDIR))) {
            continue;
        }

        if(*extension) {
            if(strlen(d->d_name) < strlen(extension) ||
                    Q_stricmp(
                        d->d_name + strlen(d->d_name) - strlen(extension),
                        extension)) {
                continue; // didn't match
            }
        }

        if(nfiles == MAX_FOUND_FILES - 1) {
            break;
        }

        list[ nfiles ] = memorySystem->CopyString(d->d_name);
        nfiles++;
    }

    list[ nfiles ] = nullptr;

    closedir(fdir);

    // return a copy of the list
    *numfiles = nfiles;

    if(!nfiles) {
        return nullptr;
    }

    listCopy = (valueType **)memorySystem->Malloc((nfiles + 1) * sizeof(
                   *listCopy));

    for(i = 0 ; i < nfiles ; i++) {
        listCopy[i] = list[i];
    }

    listCopy[i] = nullptr;

    return listCopy;
}

/*
==================
idSystemLocal::FreeFileList
==================
*/
void idSystemLocal::FreeFileList(valueType **list) {
    sint i;

    if(!list) {
        return;
    }

    for(i = 0 ; list[i] ; i++) {
        memorySystem->Free(list[i]);
    }

    memorySystem->Free(list);
}

/*
==================
idSystemLocal::Sleep

Block execution for msec or until input is recieved.
==================
*/
void idSystemLocal::Sleep(sint msec) {
    if(msec == 0) {
        return;
    }

    if(stdinIsATTY) {
        fd_set fdset;

        FD_ZERO(&fdset);
        FD_SET(STDIN_FILENO, &fdset);

        if(msec < 0) {
            select(STDIN_FILENO + 1, &fdset, nullptr, nullptr, nullptr);
        } else {
            struct timeval timeout;

            timeout.tv_sec = msec / 1000;
            timeout.tv_usec = (msec % 1000) * 1000;
            select(STDIN_FILENO + 1, &fdset, nullptr, nullptr, &timeout);
        }
    } else {
        // With nothing to select() on, we can't wait indefinitely
        if(msec < 0) {
            msec = 10;
        }

        usleep(msec * 1000);
    }
}

/*
==============
idSystemLocal::ErrorDialog

Display an error message
==============
*/
void idSystemLocal::ErrorDialog(pointer error) {
    valueType buffer[ 1024 ];
    uint size;
    sint f = -1;
    pointer homepath = fs_homepath->string;
    pointer gamedir = fs_game->string;
    pointer fileName = "crashlog.txt";
    valueType *ospath = fileSystem->BuildOSPath(homepath, gamedir, fileName);

    systemLocal.Print(va("%s\n", error));

#ifndef DEDICATED
    systemLocal.Dialog(DT_ERROR, va("%s. See \"%s\" for details.", error,
                                    ospath), "Error");
#endif

    // Make sure the write path for the crashlog exists...
    if(fileSystem->CreatePath(ospath)) {
        common->Printf("ERROR: couldn't create path '%s' for crash log.\n",
                       ospath);
        return;
    }

    // We might be crashing because we maxed out the Quake MAX_FILE_HANDLES,
    // which will come through here, so we don't want to recurse forever by
    // calling fileSystem->FOpenFileWrite()...use the Unix system APIs instead.
    f = open(ospath, O_CREAT | O_TRUNC | O_WRONLY, 0640);

    if(f == -1) {
        common->Printf("ERROR: couldn't open %s\n", fileName);
        return;
    }

    // We're crashing, so we don't care much if write() or close() fails.
    while((size = consoleLoggingSystem->LogRead(buffer, sizeof(buffer))) > 0) {
        if(write(f, buffer, size) != size) {
            common->Printf("ERROR: couldn't fully write to %s\n", fileName);
            break;
        }
    }

    close(f);
}

#if !defined (__MACOSX__)
/*
==============
idSystemLocal::ZenityCommand
==============
*/
sint idSystemLocal::ZenityCommand(dialogType_t type, pointer message,
                                  pointer title) {
    pointer options = "";
    valueType       command[ 1024 ];

    switch(type) {
        default:
        case DT_INFO:
            options = "--info";
            break;

        case DT_WARNING:
            options = "--warning";
            break;

        case DT_ERROR:
            options = "--error";
            break;

        case DT_YES_NO:
            options = "--question --ok-label=\"Yes\" --cancel-label=\"No\"";
            break;

        case DT_OK_CANCEL:
            options = "--question --ok-label=\"OK\" --cancel-label=\"Cancel\"";
            break;
    }

    Q_vsprintf_s(command, sizeof(command), sizeof(command),
                 "zenity %s --text=\"%s\" --title=\"%s\"",
                 options, message, title);

    return system(command);
}

/*
==============
idSystemLocal::KdialogCommand
==============
*/
sint idSystemLocal::KdialogCommand(dialogType_t type, pointer message,
                                   pointer title) {
    pointer options = "";
    valueType       command[ 1024 ];

    switch(type) {
        default:
        case DT_INFO:
            options = "--msgbox";
            break;

        case DT_WARNING:
            options = "--sorry";
            break;

        case DT_ERROR:
            options = "--error";
            break;

        case DT_YES_NO:
            options = "--warningyesno";
            break;

        case DT_OK_CANCEL:
            options = "--warningcontinuecancel";
            break;
    }

    Q_vsprintf_s(command, sizeof(command), sizeof(command),
                 "kdialog %s \"%s\" --title \"%s\"",
                 options, message, title);

    return system(command);
}

/*
==============
idSystemLocal::XmessageCommand
==============
*/
sint idSystemLocal::XmessageCommand(dialogType_t type, pointer message,
                                    pointer title) {
    pointer options = "";
    valueType       command[ 1024 ];

    switch(type) {
        default:
            options = "-buttons OK";
            break;

        case DT_YES_NO:
            options = "-buttons Yes:0,No:1";
            break;

        case DT_OK_CANCEL:
            options = "-buttons OK:0,Cancel:1";
            break;
    }

    Q_vsprintf_s(command, sizeof(command), sizeof(command),
                 "xmessage -center %s \"%s\"",
                 options, message);

    return system(command);
}
#endif

/*
==============
idSystemLocal::Dialog

Display a *nix dialog box
==============
*/
dialogResult_t idSystemLocal::Dialog(dialogType_t type, pointer message,
                                     pointer title) {
#if defined (__LINUX__)
    enum dialogCommandType_t {
        NONE = 0,
        ZENITY,
        KDIALOG,
        XMESSAGE,
        NUM_DIALOG_PROGRAMS
    };

    typedef sint(*dialogCommandBuilder_t)(dialogType_t, pointer, pointer);

    pointer              session = getenv("DESKTOP_SESSION");
    bool                tried[ NUM_DIALOG_PROGRAMS ] = { false };
    dialogCommandBuilder_t  commands[ NUM_DIALOG_PROGRAMS ] = { nullptr };
    dialogCommandType_t     preferredCommandType = NONE;

    commands[ ZENITY ] = &ZenityCommand;
    commands[ KDIALOG ] = &KdialogCommand;
    commands[ XMESSAGE ] = &XmessageCommand;

    // This may not be the best way
    if(!Q_stricmp(session, "gnome")) {
        preferredCommandType = ZENITY;
    } else if(!Q_stricmp(session, "kde")) {
        preferredCommandType = KDIALOG;
    }

    while(1) {
        sint i;
        sint exitCode;

        for(i = NONE + 1; i < NUM_DIALOG_PROGRAMS; i++) {
            if(preferredCommandType != NONE && preferredCommandType != i) {
                continue;
            }

            if(!tried[ i ]) {
                exitCode = commands[ i ](type, message, title);

                if(exitCode >= 0) {
                    switch(type) {
                        case DT_YES_NO:
                            return exitCode ? DR_NO : DR_YES;

                        case DT_OK_CANCEL:
                            return exitCode ? DR_CANCEL : DR_OK;

                        default:
                            return DR_OK;
                    }
                }

                tried[ i ] = true;

                // The preference failed, so start again in order
                if(preferredCommandType != NONE) {
                    preferredCommandType = NONE;
                    break;
                }
            }
        }

        for(i = NONE + 1; i < NUM_DIALOG_PROGRAMS; i++) {
            if(!tried[ i ]) {
                continue;
            }
        }

        break;
    }

    if(developer->integer) {
        common->Printf(S_COLOR_YELLOW "WARNING: failed to show a dialog\n");
    }

    return DR_OK;
#else
    dialogResult_t result = DR_OK;
    NSAlert *alert = [NSAlert new];

    [alert setMessageText : [NSString stringWithUTF8String : title] ] ;
    [alert setInformativeText : [NSString stringWithUTF8String : message] ] ;

    if(type == DT_ERROR) {
        [alert setAlertStyle : NSCriticalAlertStyle];
    } else {
        [alert setAlertStyle : NSWarningAlertStyle];
    }

    switch(type) {
        default:
            [alert runModal] ;
            result = DR_OK;
            break;

        case DT_YES_NO:
            [alert addButtonWithTitle : @"Yes"] ;
            [alert addButtonWithTitle : @"No"] ;

            switch([alert runModal]) {
                default:
                case NSAlertFirstButtonReturn:
                    result = DR_YES;
                    break;

                case NSAlertSecondButtonReturn:
                    result = DR_NO;
                    break;
            }

            break;

        case DT_OK_CANCEL:
            [alert addButtonWithTitle : @"OK"] ;
            [alert addButtonWithTitle : @"Cancel"] ;

            switch([alert runModal]) {
                default:
                case NSAlertFirstButtonReturn:
                    result = DR_OK;
                    break;

                case NSAlertSecondButtonReturn:
                    result = DR_CANCEL;
                    break;
            }

            break;
    }

    [alert release] ;

    return result;
#endif
}

/*
==================
idSystemLocal::DoStartProcess
actually forks and starts a process

UGLY HACK:
  idSystemLocal::StartProcess works with a command line only
  if this command line is actually a binary with command line parameters,
  the only way to do this on unix is to use a system() call
  but system doesn't replace the current process, which leads to a situation like:
  wolf.x86--spawned_process.x86
  in the case of auto-update for instance, this will cause write access denied on wolf.x86:
  wolf-beta/2002-March/000079.html
  we hack the implementation here, if there are no space in the command line, assume it's a straight process and use execl
  otherwise, use system ..
  The clean solution would be idSystemLocal::StartProcess and idSystemLocal::StartProcess_Args..
==================
*/
void idSystemLocal::DoStartProcess(valueType *cmdline) {
    switch(fork()) {
        case - 1:
            // main thread
            break;

        case 0:
            if(strchr(cmdline, ' ')) {
                system(cmdline);
            } else {
                execl(cmdline, cmdline, NULL);
                printf("execl failed: %s\n", strerror(errno));
            }

            _exit(0);
            break;
    }
}

/*
==================
idSystemLocal::StartProcess
if !doexit, start the process asap
otherwise, push it for execution at exit
(i.e. let complete shutdown of the game and freeing of resources happen)
NOTE: might even want to add a small delay?
==================
*/
void idSystemLocal::StartProcess(valueType *cmdline, bool doexit) {
    if(doexit) {
        if(developer->integer) {
            //Dushan review all this and see is this working for the autoupdate and Unix builds
            common->Printf("idSystemLocal::StartProcess %s (delaying to final exit)\n",
                           cmdline);
        }

        Q_strncpyz(exit_cmdline, cmdline, MAX_CMD);
        cmdBufferSystem->ExecuteText(EXEC_APPEND, "quit\n");
        return;
    }

    if(developer->integer) {
        common->Printf("idSystemLocal::StartProcess %s\n", cmdline);
    }

    DoStartProcess(cmdline);
}

/*
=================
idSystemLocal::OpenURL
=================
*/
void idSystemLocal::OpenURL(pointer url, bool doexit) {
    valueType *basepath, *homepath, *pwdpath;
    valueType fname[20];
    valueType fn[MAX_OSPATH];
    valueType cmdline[MAX_CMD];

    static bool doexit_spamguard = false;

    if(doexit_spamguard) {
        if(developer->integer) {
            common->Printf("idSystemLocal::OpenURL: already in a doexit sequence, ignoring %s\n",
                           url);
        }

        return;
    }

    common->Printf("Open URL: %s\n", url);
    // opening an URL on *nix can mean a lot of things ..
    // just spawn a script instead of deciding for the user :-)

    // do the setup before we fork
    // search for an openurl.sh script
    // search procedure taken from idSystemLocal::LoadDll
    Q_strncpyz(fname, "openurl.sh", 20);

    pwdpath = Cwd();
    Q_vsprintf_s(fn, MAX_OSPATH, MAX_OSPATH, "%s/%s", pwdpath, fname);

    if(access(fn, X_OK) == -1) {
        if(developer->integer) {
            common->Printf("%s not found\n", fn);
        }

        // try in home path
        homepath = fs_homepath->string;
        Q_vsprintf_s(fn, MAX_OSPATH, MAX_OSPATH, "%s/%s", homepath, fname);

        if(access(fn, X_OK) == -1) {
            if(developer->integer) {
                common->Printf("%s not found\n", fn);
            }

            // basepath, last resort
            basepath = fs_basepath->string;
            Q_vsprintf_s(fn, MAX_OSPATH, MAX_OSPATH, "%s/%s", basepath, fname);

            if(access(fn, X_OK) == -1) {
                if(developer->integer) {
                    common->Printf("%s not found\n", fn);
                }

                common->Printf("Can't find script '%s' to open requested URL (use +set developer 1 for more verbosity)\n",
                               fname);
                // we won't quit
                return;
            }
        }
    }

    // show_bug.cgi?id=612
    if(doexit) {
        doexit_spamguard = true;
    }

    if(developer->integer) {
        common->Printf("URL script: %s\n", fn);
    }

    // build the command line
    Q_vsprintf_s(cmdline, MAX_CMD, MAX_CMD, "%s '%s' &", fn, url);

    StartProcess(cmdline, doexit);

}

// Dushan
bool idSystemLocal::OpenUrl(pointer url) {
    valueType *browser = getenv("BROWSER");
    valueType *kde_session = getenv("KDE_FULL_SESSION");
    valueType *gnome_session = getenv("GNOME_DESKTOP_SESSION_ID");

    //Try to use xdg-open, if not, try default, then kde, gnome
    if(browser) {
        Fork(browser, url);
        return true;
    } else if(kde_session && Q_stricmp("true", kde_session) == 0) {
        Fork("konqueror", url);
        return true;
    } else if(gnome_session) {
        Fork("gnome-open", url);
        return true;
    } else {
        Fork("/usr/bin/firefox", url);
    }

    // open url somehow
    return true;
}

bool idSystemLocal::Fork(pointer path, pointer cmdLine) {
    sint pid;

    pid = fork();

    if(pid == 0) {
        struct stat filestat;

        //Try to set the executable bit
        if(stat(path, &filestat) == 0) {
            chmod(path, filestat.st_mode | S_IXUSR);
        }

        execlp(path, path, cmdLine, NULL);
        printf("Exec Failed for: %s\n", path);
        _exit(255);
    } else if(pid == -1) {
        return false;
    }

    return true;
}

/*
==============
idSystemLocal::GLimpSafeInit

Unix specific "safe" GL implementation initialisation
==============
*/
void idSystemLocal::GLimpSafeInit(void) {
    // NOP
}

/*
==============
idSystemLocal::GLimpInit

Unix specific GL implementation initialisation
==============
*/
void idSystemLocal::GLimpInit(void) {
    // NOP
}

void idSystemLocal::SetFloatEnv(void) {
    // rounding towards 0
    fesetround(FE_TOWARDZERO);
}

/*
==============
idSystemLocal::PlatformInit

Unix specific initialisation
==============
*/
void idSystemLocal::PlatformInit(void) {
    pointer term = getenv("TERM");

    signal(SIGHUP, SigHandler);
    signal(SIGQUIT, SigHandler);
    signal(SIGTRAP, SigHandler);
    signal(SIGIOT, SigHandler);
    signal(SIGBUS, SigHandler);

    stdinIsATTY = isatty(STDIN_FILENO) &&
                  !(term && (!strcmp(term, "raw") || !strcmp(term, "dumb")));
}

/*
==============
idSystemLocal::SetEnv

set/unset environment variables (empty value removes it)
==============
*/
sint idSystemLocal::SetEnv(pointer name, pointer value) {
    if(value && *value) {
        setenv(name, value, 1);
    } else {
        unsetenv(name);
    }
}

/*
==============
idSystemLocal::PID
==============
*/
sint idSystemLocal::PID(void) {
    return getpid();
}

/*
==============
idSystemLocal::PIDIsRunning
==============
*/
bool idSystemLocal::PIDIsRunning(sint pid) {
	// For kill(), pid <= 0 has special meanings so
	// we can't really use it to check if something
	// is actually running.
    if(pid <= 0) {
        return false;
    } else {
        return kill(pid, 0) == 0;
    }
}

/*
==============
idSystemLocal::IsNumLockDown
==============
*/
bool idSystemLocal::IsNumLockDown(void) {
    return (SDL_GetModState() & KMOD_NUM) == KMOD_NUM;
}

#endif
