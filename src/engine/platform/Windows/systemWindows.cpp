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
// File name:   sys_win32.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
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

#ifdef _WIN32

/*
==================
idSystemLocal::CtrlHandler

The Windows Console doesn't use signals for terminating the application
with Ctrl-C, logging off, window closing, etc.  Instead it uses a special
handler routine.  Fortunately, the values for Ctrl signals don't seem to
overlap with true signal codes that Windows provides, so calling
idSystemLocal::SigHandler() with those numbers should be safe for generating unique
shutdown messages.
==================
*/
sint idSystemLocal::CtrlHandler(uint32 sig) {
    SigHandler(sig);
    return TRUE;
}

/*
==============
idSystemLocal::SetFloatEnv
==============
*/
void idSystemLocal::SetFloatEnv(void) {
    _controlfp(FPUCW, FPUCWMASK);
}

/*
================
idSystemLocal::DefaultHomePath
================
*/
valueType *idSystemLocal::DefaultHomePath(valueType *buffer, sint size) {
    if(SHGetSpecialFolderPath(nullptr, buffer, CSIDL_PERSONAL,
                              TRUE) != NOERROR) {
        Q_strcat(buffer, size, "\\My Games\\OpenWolf");
    } else {
        Com_Error(ERR_FATAL, "couldn't find home path.\n");
        buffer[0] = 0;
    }

    return buffer;
}

/*
================
idSystemLocal::TempPath
================
*/
pointer idSystemLocal::TempPath(void) {
    static valueType path[ MAX_PATH ];
    uint length;
    valueType tmp[ MAX_OSPATH ];

    length = GetTempPath(sizeof(path), path);

    if(length > sizeof(path) || length == 0) {
        return systemLocal.DefaultHomePath(path, sizeof(tmp));
    } else {
        return path;
    }
}

/*
================
idSystemLocal::Milliseconds
================
*/

sint idSystemLocal::Milliseconds(void) {
    sint sys_curtime;
    static bool initialized = false;

    if(!initialized) {
        sys_timeBase = timeGetTime();
        initialized = true;
    }

    sys_curtime = timeGetTime() - sys_timeBase;

    return sys_curtime;
}

/*
================
idSystemLocal::RandomBytes
================
*/
bool idSystemLocal::RandomBytes(uchar8 *string, uint64 len) {
    HCRYPTPROV prov;

    if(!CryptAcquireContext(&prov, nullptr, nullptr, PROV_RSA_FULL,
                            CRYPT_VERIFYCONTEXT)) {
        return false;
    }

    if(!CryptGenRandom(prov, static_cast<DWORD>(len), (BYTE *)string)) {
        CryptReleaseContext(prov, 0);
        return false;
    }

    CryptReleaseContext(prov, 0);

    return true;
}

/*
================
idSystemLocal::GetCurrentUser
================
*/
valueType *idSystemLocal::GetCurrentUser(void) {
    static valueType s_userName[1024];
    uint32 size = sizeof(s_userName);

    if(!GetUserName(s_userName, (LPDWORD)&size)) {
        Q_strcpy_s(s_userName, "player");
    }

    if(!s_userName[0]) {
        Q_strcpy_s(s_userName, "player");
    }

    return s_userName;
}

/*
================
idSystemLocal::SysGetClipboardData
================
*/
valueType *idSystemLocal::SysGetClipboardData(void) {
    valueType *data = nullptr, *cliptext;

    if(OpenClipboard(nullptr) != 0) {
        HANDLE hClipboardData;

        if((hClipboardData = GetClipboardData(CF_TEXT)) != 0) {
            if((cliptext = reinterpret_cast<valueType *>(GlobalLock(
                               hClipboardData))) != 0) {
                data = (const_cast<valueType *>(reinterpret_cast<pointer>
                                                (memorySystem->Malloc(GlobalSize(hClipboardData)))) + 1);
                Q_strncpyz(data, cliptext, GlobalSize(hClipboardData));
                GlobalUnlock(hClipboardData);

                strtok(data, "\n\r\b");
            }
        }

        CloseClipboard();
    }

    return data;
}

/*
==================
idSystemLocal::LowPhysicalMemory
==================
*/
bool idSystemLocal::LowPhysicalMemory(void) {
    MEMORYSTATUSEX stat;
    GlobalMemoryStatusEx(&stat);

    return (stat.ullTotalPhys <= MEM_THRESHOLD) ? true : false;
}

/*
==============
idSystemLocal::Basename
==============
*/
pointer idSystemLocal::Basename(valueType *path) {
    static valueType base[ MAX_OSPATH ] = { 0 };
    uint64 length;

    length = strlen(path) - 1;

    // Skip trailing slashes
    while(length > 0 && path[ length ] == '\\') {
        length--;
    }

    while(length > 0 && path[ length - 1 ] != '\\') {
        length--;
    }

    Q_strncpyz(base, &path[ length ], sizeof(base));

    length = strlen(base) - 1;

    // Strip trailing slashes
    while(length > 0 && base[length] == '\\') {
        base[length--] = '\0';
    }

    return base;
}

/*
==============
idSystemLocal::Dirname
==============
*/
pointer idSystemLocal::Dirname(valueType *path) {
    static valueType dir[ MAX_OSPATH ] = { 0 };
    uint64 length;

    Q_strncpyz(dir, path, sizeof(dir));
    length = strlen(dir) - 1;

    while(length > 0 && dir[ length ] != '\\') {
        length--;
    }

    dir[ length ] = '\0';

    return dir;
}

/*
==============
idSystemLocal::Mkdir
==============
*/
bool idSystemLocal::Mkdir(pointer path) {
    if(!CreateDirectory(path, nullptr)) {
        if(GetLastError() != ERROR_ALREADY_EXISTS) {
            return false;
        }
    }

    return true;
}

/*
==============
idSystemLocal::Cwd
==============
*/
valueType *idSystemLocal::Cwd(void) {
    static valueType cwd[MAX_OSPATH];

    _getcwd(cwd, sizeof(cwd) - 1);
    cwd[MAX_OSPATH - 1] = 0;

    return cwd;
}


/*
==============
idSystemLocal::ListFilteredFiles
==============
*/
void idSystemLocal::ListFilteredFiles(pointer basedir, valueType *subdirs,
                                      valueType *filter, valueType **list, sint *numfiles) {
    sint64 findhandle;
    valueType search[MAX_OSPATH], newsubdirs[MAX_OSPATH], filename[MAX_OSPATH];
    struct      _finddata_t findinfo;

    if(*numfiles >= MAX_FOUND_FILES - 1) {
        return;
    }

    if(strlen(subdirs)) {
        Q_vsprintf_s(search, sizeof(search), sizeof(search), "%s\\%s\\*", basedir,
                     subdirs);
    } else {
        Q_vsprintf_s(search, sizeof(search), sizeof(search), "%s\\*", basedir);
    }

    findhandle = _findfirst(search, &findinfo);

    if(findhandle == -1) {
        return;
    }

    do {
        if(findinfo.attrib & _A_SUBDIR) {
            if(Q_stricmp(findinfo.name, ".") && Q_stricmp(findinfo.name, "..")) {
                if(strlen(subdirs)) {
                    Q_vsprintf_s(newsubdirs, sizeof(newsubdirs), sizeof(newsubdirs), "%s\\%s",
                                 subdirs, findinfo.name);
                } else {
                    Q_vsprintf_s(newsubdirs, sizeof(newsubdirs), sizeof(newsubdirs), "%s",
                                 findinfo.name);
                }

                ListFilteredFiles(basedir, newsubdirs, filter, list, numfiles);
            }
        }

        if(*numfiles >= MAX_FOUND_FILES - 1) {
            break;
        }

        Q_vsprintf_s(filename, sizeof(filename), sizeof(filename), "%s\\%s",
                     subdirs, findinfo.name);

        if(!Com_FilterPath(filter, filename, false)) {
            continue;
        }

        list[*numfiles] = memorySystem->CopyString(filename);

        (*numfiles)++;
    } while(_findnext(findhandle, &findinfo) != -1);

    _findclose(findhandle);
}

/*
==============
sidSystemLocal::trgtr
==============
*/
bool idSystemLocal::strgtr(pointer s0, pointer s1) {
    uint64 l0, l1, i;

    l0 = strlen(s0);
    l1 = strlen(s1);

    if(l1 < l0) {
        l0 = l1;
    }

    for(i = 0; i < l0; i++) {
        if(s1[i] > s0[i]) {
            return true;
        }

        if(s1[i] < s0[i]) {
            return false;
        }
    }

    return false;
}

/*
==============
idSystemLocal::ListFiles
==============
*/
valueType **idSystemLocal::ListFiles(pointer directory, pointer extension,
                                     valueType *filter, sint *numfiles, bool wantsubs) {
    valueType search[MAX_OSPATH];
    sint nfiles, flag, i;
    valueType **listCopy, *list[MAX_FOUND_FILES];
    struct _finddata_t findinfo;
    sint64 findhandle;

    if(filter) {
        nfiles = 0;
        ListFilteredFiles(directory, "", filter, list, &nfiles);

        list[ nfiles ] = 0;
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

    // passing a slash as extension will find directories
    if(extension[0] == '/' && extension[1] == 0) {
        extension = "";
        flag = 0;
    } else {
        flag = _A_SUBDIR;
    }

    Q_vsprintf_s(search, sizeof(search), sizeof(search), "%s\\*%s", directory,
                 extension);

    // search
    nfiles = 0;

    findhandle = _findfirst(search, &findinfo);

    if(findhandle == -1) {
        *numfiles = 0;
        return nullptr;
    }

    do {
        if((!wantsubs && flag ^ (findinfo.attrib & _A_SUBDIR)) || (wantsubs &&
                findinfo.attrib & _A_SUBDIR)) {
            if(nfiles == MAX_FOUND_FILES - 1) {
                break;
            }

            list[ nfiles ] = memorySystem->CopyString(findinfo.name);
            nfiles++;
        }
    } while(_findnext(findhandle, &findinfo) != -1);

    list[ nfiles ] = 0;

    _findclose(findhandle);

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

    do {
        flag = 0;

        for(i = 1; i < nfiles; i++) {
            if(strgtr(listCopy[i - 1], listCopy[i])) {
                valueType *temp = listCopy[i];

                listCopy[i] = listCopy[i - 1];
                listCopy[i - 1] = temp;
                flag = 1;
            }
        }
    } while(flag);

    return listCopy;
}

/*
==============
idSystemLocal::FreeFileList
==============
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
==============
idSystemLocal::Sleep

Block execution for msec or until input is received.
==============
*/
void idSystemLocal::Sleep(sint msec) {
    if(msec == 0) {
        return;
    }

#ifdef DEDICATED

    if(msec < 0) {
        WaitForSingleObject(GetStdHandle(STD_INPUT_HANDLE), INFINITE);
    } else {
        WaitForSingleObject(GetStdHandle(STD_INPUT_HANDLE), msec);
    }

#else

    // Client idSystemLocal::Sleep doesn't support waiting on stdin
    if(msec < 0) {
        return;
    }

    Sleep(msec);
#endif
}

/*
==============
idSystemLocal::OpenUrl
==============
*/
bool idSystemLocal::OpenUrl(pointer url) {
    return (reinterpret_cast<sint>(ShellExecute(nullptr, nullptr, url, nullptr,
                                   nullptr, SW_SHOWNORMAL)) > 32) ? true : false;
}

/*
==============
idSystemLocal::ErrorDialog

Display an error message
==============
*/
void idSystemLocal::ErrorDialog(pointer error) {
    pointer homepath = fs_homepath->string,
            gamedir = fs_game->string,
            fileName = "crashlog.txt";
    valueType buffer[ 1024 ], *ospath = fileSystem->BuildOSPath(homepath,
                                        gamedir, fileName);
    uint64 size;
    sint f = -1;

    systemLocal.Print(va("%s\n", error));

#ifndef DEDICATED
    systemLocal.Dialog(DT_ERROR, va("%s. See \"%s\" for details.", error,
                                    ospath), "Error");
#endif

    // Make sure the write path for the crashlog exists...
    if(fileSystem->CreatePath(ospath)) {
        Com_Printf("ERROR: couldn't create path '%s' for crash log.\n", ospath);
        return;
    }

    // We might be crashing because we maxed out the Quake MAX_FILE_HANDLES,
    // which will come through here, so we don't want to recurse forever by
    // calling fileSystem->FOpenFileWrite()...use the Unix system APIs instead.
    f = open(ospath, O_CREAT | O_TRUNC | O_WRONLY, 0640);

    if(f == -1) {
        Com_Printf("ERROR: couldn't open %s\n", fileName);
        return;
    }

    // We're crashing, so we don't care much if write() or close() fails.
    while((size = consoleLoggingSystem->LogRead(buffer, sizeof(buffer))) > 0) {
        if(write(f, buffer, size) != size) {
            Com_Printf("ERROR: couldn't fully write to %s\n", fileName);
            break;
        }
    }

    close(f);
}

/*
==============
idSystemLocal::Dialog

Display a win32 dialog box
==============
*/
dialogResult_t idSystemLocal::Dialog(dialogType_t type, pointer message,
                                     pointer title) {
    UINT uType;

    switch(type) {
        default:
        case DT_INFO:
            uType = MB_ICONINFORMATION | MB_OK;
            break;

        case DT_WARNING:
            uType = MB_ICONWARNING | MB_OK;
            break;

        case DT_ERROR:
            uType = MB_ICONERROR | MB_OK;
            break;

        case DT_YES_NO:
            uType = MB_ICONQUESTION | MB_YESNO;
            break;

        case DT_OK_CANCEL:
            uType = MB_ICONWARNING | MB_OKCANCEL;
            break;
    }

    switch(MessageBox(nullptr, message, title, uType)) {
        default:
        case IDOK:
            return DR_OK;

        case IDCANCEL:
            return DR_CANCEL;

        case IDYES:
            return DR_YES;

        case IDNO:
            return DR_NO;
    }
}

/*
==============
idSystemLocal::GLimpSafeInit

Windows specific "safe" GL implementation initialisation
==============
*/
void idSystemLocal::GLimpSafeInit(void) {
#ifndef DEDICATED

    if(!SDL_VIDEODRIVER_externallySet) {
        // Here, we want to let SDL decide what do to unless
        // explicitly requested otherwise
        SetEnv("SDL_VIDEODRIVER", "");
    }

#endif
}

/*
==============
idSystemLocal::GLimpInit

Windows specific GL implementation initialisation
==============
*/
void idSystemLocal::GLimpInit(void) {
#ifndef DEDICATED

    if(!SDL_VIDEODRIVER_externallySet) {
        // It's a little bit weird having in_mouse control the
        // video driver, but from ioq3's point of view they're
        // virtually the same except for the mouse input anyway
        if(in_mouse->integer == -1) {
            // Use the windib SDL backend, which is closest to
            // the behaviour of idq3 with in_mouse set to -1
            SetEnv("SDL_VIDEODRIVER", "windib");
        } else {
            // Use the DirectX SDL backend
            SetEnv("SDL_VIDEODRIVER", "windows");
        }
    }

#endif
}

/*
==============
idSystemLocal::PlatformInit

Windows specific initialisation
==============
*/
void idSystemLocal::resetTime(void) {
    timeEndPeriod(1);
}

void idSystemLocal::PlatformInit(void) {
#ifndef DEDICATED
    pointer SDL_VIDEODRIVER = getenv("SDL_VIDEODRIVER");
#endif

    SetFloatEnv();

#ifndef DEDICATED

    if(SDL_VIDEODRIVER) {
        Com_Printf("SDL_VIDEODRIVER is externally set to \"%s\", "
                   "in_mouse -1 will have no effect\n", SDL_VIDEODRIVER);
        SDL_VIDEODRIVER_externallySet = true;
    } else {
        SDL_VIDEODRIVER_externallySet = false;
    }

#endif

    // Handle Ctrl-C or other console termination
    SetConsoleCtrlHandler(CtrlHandler, TRUE);

    // Increase sleep resolution
    timeBeginPeriod(1);
    atexit(resetTime);
}

/*
==============
idSystemLocal::SetEnv

set/unset environment variables (empty value removes it)
==============
*/
void idSystemLocal::SetEnv(pointer name, pointer value) {
    _putenv(va("%s=%s", name, value));
}

/*
==============
idSystemLocal::PID
==============
*/
sint idSystemLocal::PID(void) {
    return GetCurrentProcessId();
}

/*
==============
idSystemLocal::PIDIsRunning
==============
*/
bool idSystemLocal::PIDIsRunning(sint pid) {
    uint32 processes[ 1024 ], numBytes, numProcesses;
    sint    i;

    if(!EnumProcesses(processes, sizeof(processes), &numBytes)) {
        return false; // Assume it's not running
    }

    numProcesses = numBytes / sizeof(uint32);

    // Search for the pid
    for(i = 0; i < numProcesses; i++) {
        if(processes[ i ] == pid) {
            return true;
        }
    }

    return false;
}

/*
==================
idSystemLocal::StartProcess

NERVE - SMF
//Dushan - changed all this to work with update server
==================
*/
void idSystemLocal::StartProcess(valueType *exeName, bool doexit) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    if(!CreateProcess(nullptr, exeName, nullptr, nullptr, FALSE, 0, nullptr,
                      nullptr, &si, &pi)) {
        if(doexit) {
            Com_Error(ERR_DROP, "Could not start process: '%s' ", exeName);
        } else {
            Com_Printf("Could not start process: '%s'\n", exeName);
        }

        return;
    }

    // TTimo: similar way of exiting as used in OpenURL below
    if(doexit) {
        //Dushan - quit
        Quit();
    }
}

/*
==================
idSystemLocal::OpenURL

NERVE - SMF
==================
*/
void idSystemLocal::OpenURL(pointer url, bool doexit) {
    HWND wnd;

    static bool doexit_spamguard = false;

    if(doexit_spamguard) {
        if(developer->integer) {
            Com_Printf("idSystemLocal::OpenURL: already in a doexit sequence, ignoring %s\n",
                       url);
        }

        return;
    }

    Com_Printf("Open URL: %s\n", url);

    if(!ShellExecute(nullptr, "open", url, nullptr, nullptr, SW_RESTORE)) {
        // couldn't start it, popup error box
        Com_Error(ERR_DROP, "Could not open url: '%s' ", url);
        return;
    }

    wnd = GetForegroundWindow();

    if(wnd) {
        ShowWindow(wnd, SW_MAXIMIZE);
    }

    if(doexit) {
        // show_bug.cgi?id=612
        doexit_spamguard = true;
        cmdBufferSystem->ExecuteText(EXEC_APPEND, "quit\n");
    }
}

/*
==============
idSystemLocal::IsNumLockDown
==============
*/
bool idSystemLocal::IsNumLockDown(void) {
    SHORT state = GetKeyState(VK_NUMLOCK);

    if(state & 0x01) {
        return true;
    }

    return false;
}

/*
================
idSystemLocal::Chmod
================
*/
void idSystemLocal::Chmod(valueType *file, sint mode) {
}

/*
================
idSystemLocal::DoStartProcess
================
*/
void idSystemLocal::DoStartProcess(valueType *cmdline) {
}

/*
================
idSystemLocal::Fork
================
*/
bool idSystemLocal::Fork(pointer path, pointer cmdLine) {
    return true;
}

/*
================
idSystemLocal::KdialogCommand
================
*/
sint idSystemLocal::KdialogCommand(dialogType_t type, pointer message,
                                   pointer title) {
    return 0;
}

/*
================
idSystemLocal::ZenityCommand
================
*/
sint idSystemLocal::ZenityCommand(dialogType_t type, pointer message,
                                  pointer title) {
    return 0;
}

/*
================
idSystemLocal::XmessageCommand
================
*/
sint idSystemLocal::XmessageCommand(dialogType_t type, pointer message,
                                    pointer title) {
    return 0;
}

#endif
