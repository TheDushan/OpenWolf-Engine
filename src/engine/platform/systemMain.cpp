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
// File name:   systemMain.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
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

idSystemLocal systemLocal;
idSystem *idsystem = &systemLocal;

/*
===============
idSystemLocal::idSystemLocal
===============
*/
idSystemLocal::idSystemLocal(void) {
}

/*
===============
idSystemLocal::~idSystemLocal
===============
*/
idSystemLocal::~idSystemLocal(void) {
}

/*
===============
idSystemLocal::MonkeyShouldBeSpanked
===============
*/
sint idSystemLocal::MonkeyShouldBeSpanked(void) {
    return sys_monkeySpank;
}

/*
=================
idSystemLocal::SetBinaryPath
=================
*/
void idSystemLocal::SetBinaryPath(pointer path) {
    Q_strncpyz(binaryPath, path, sizeof(binaryPath));
}

/*
=================
idSystemLocal::BinaryPath
=================
*/
valueType *idSystemLocal::BinaryPath(void) {
    return binaryPath;
}

/*
=================
idSystemLocal::SetDefaultInstallPath
=================
*/
void idSystemLocal::SetDefaultInstallPath(pointer path) {
    Q_strncpyz(installPath, path, sizeof(installPath));
}

/*
=================
idSystemLocal::DefaultInstallPath
=================
*/
valueType *idSystemLocal::DefaultInstallPath(void) {
    static valueType installdir[MAX_OSPATH];

    Q_vsprintf_s(installdir, sizeof(installdir), sizeof(installdir), "%s",
                 Cwd());

    // Windows
    Q_strreplace(installdir, sizeof(installdir), "bin/windows", "");
    Q_strreplace(installdir, sizeof(installdir), "bin\\windows", "");

    // Linux
    Q_strreplace(installdir, sizeof(installdir), "bin/unix", "");

    // BSD
    Q_strreplace(installdir, sizeof(installdir), "bin/bsd", "");

    // MacOS X
    Q_strreplace(installdir, sizeof(installdir), "bin/macos", "");

    return installdir;
}

/*
=================
idSystemLocal::SetDefaultLibPath
=================
*/
void idSystemLocal::SetDefaultLibPath(pointer path) {
    Q_strncpyz(libPath, path, sizeof(libPath));
}

/*
=================
idSystemLocal::DefaultLibPath
=================
*/
valueType *idSystemLocal::DefaultLibPath(void) {
    if(*libPath) {
        return libPath;
    } else {
        return Cwd();
    }
}

/*
=================
idSystemLocal::DefaultAppPath
=================
*/
valueType *idSystemLocal::DefaultAppPath(void) {
    return BinaryPath();
}

/*
=================
idSystemLocal::Restart_f

Restart the input subsystem
=================
*/
void idSystemLocal::Restart_f(void) {
#ifndef DEDICATED

    if(!SDL_WasInit(SDL_INIT_VIDEO)) {
        common->Printf("idSystemLocal::Restart_f: Cannot restart input while video is shutdown\n");
        return;
    }

#endif
    Restart();
}

/*
=================
idSystemLocal::ConsoleInput

Handle new console input
=================
*/
valueType *idSystemLocal::ConsoleInput(void) {
    return consoleCursesSystem->Input();
}

/*
=================
idSystemLocal::PIDFileName
=================
*/
valueType *idSystemLocal::PIDFileName(void) {
    return va(nullptr, "%s/%s", TempPath(), PID_FILENAME);
}

/*
=================
idSystemLocal::WritePIDFile

Return true if there is an existing stale PID file
=================
*/
bool idSystemLocal::WritePIDFile(void) {
    valueType *pidFile = PIDFileName();
    FILE *f;
    bool stale = false;
    const sint PID_BUFFER_SIZE = 64;

    // First, check if the pid file is already there
    if((f = ::fopen(pidFile, "r")) != nullptr) {
        valueType pidBuffer[PID_BUFFER_SIZE] = { 0 };
        uint64 pid, ignored;
        valueType *endptr = nullptr;

        // Okay to ignore result, empty pid files are handled below.
        ignored = ::fread(pidBuffer, sizeof(valueType), sizeof(pidBuffer) - 1, f);
        ::fclose(f);

        // Try to convert string to process id.
        pid = static_cast<sint>(::strtol(pidBuffer, &endptr, 10));

        if(*pidBuffer != '\0' && *endptr == '\0') {
            // The pid file seems valid, so it's only stale if the
            // process it refers to is not running.
            if(!PIDIsRunning(pid)) {
                stale = true;
            }
        } else {
            // The pid file contained trash (maybe nothing at all),
            // so we consider it stale.
            stale = true;
        }
    }

    if((f = ::fopen(pidFile, "w")) != nullptr) {
        ::fprintf(f, "%d", PID());
        ::fclose(f);
    } else {
        common->Printf(S_COLOR_YELLOW "Couldn't write %s.\n", pidFile);
    }

    return stale;
}

/*
=================
idSystemLocal::Exit

Single exit point (regular exit or in case of error)
=================
*/
void idSystemLocal::Exit(sint exitCode) {
    consoleCursesSystem->Shutdown();

    if(exitCode < 2) {
        // Normal exit
        remove(PIDFileName());
    }

    sys_retcode = exitCode;
    longjmp(sys_exitframe, -1);

    // Shutdown the client
#if !defined (DEDICATED)
    clientMainSystem->Shutdown();
#endif

    // Shutdown the network stuff
    networkSystem->Shutdown();

    // Close the console
    consoleCursesSystem->Shutdown();

    PlatformExit();

    // Shutdown SDL and quit
#if !defined (DEDICATED)
    // Shutdown SDL and quit
    SDL_Quit();
#endif
    exit(exitCode);
}

/*
=================
idSystemLocal::Quit
=================
*/
void idSystemLocal::Quit(void) {
    Exit(0);
}

/*
=================
idSystemLocal::Init
=================
*/
void idSystemLocal::Init(void) {
    cmdSystem->AddCommand("in_restart", &idSystemLocal::Restart,
                          "Restarts all the input drivers, dinput, joystick, etc.");
    cvarSystem->Set("architecture", OS_STRING " " ARCH_STRING);
    cvarSystem->Set("username", GetCurrentUser());
}

/*
=================
Host_RecordError
=================
*/
void idSystemLocal::RecordError(pointer msg) {
    msg;
}

/*
=================
idSystemLocal::WriteDump
=================
*/
void idSystemLocal::WriteDump(pointer fmt, ...) {
    //this memory should live as long as the SEH is doing its thing...I hope
    static valueType msg[2048];
    va_list vargs;

    /*
        Do our own vsnprintf as using va's will change global state
        that might be pertinent to the dump.
    */

    va_start(vargs, fmt);
    Q_vsprintf_s(msg, sizeof(msg) - 1, fmt, vargs);
    va_end(vargs);

    msg[sizeof(msg) - 1] = 0;   //ensure null termination

    RecordError(msg);
}

/*
=================
idSystemLocal::Print
=================
*/
void idSystemLocal::Print(pointer msg) {
    consoleLoggingSystem->LogWrite(msg);
    consoleCursesSystem->Print(msg);
}

/*
=================
idSystemLocal::Error
=================
*/
void idSystemLocal::Error(pointer error, ...) {
    va_list argptr;
    valueType string[4096];

    va_start(argptr, error);
    Q_vsprintf_s(string, sizeof(string), error, argptr);
    va_end(argptr);

    // Print text in the console window/box
    Print(string);
    Print("\n");

#if !defined (DEDICATED)
    clientMainSystem->Shutdown();
#endif
    // Set the error text (waits for the user to quit)
    ErrorDialog(string);

    // Close the console and quit
    consoleCursesSystem->Shutdown();

    Exit(3);
}

/*
=================
idSystemLocal::UnloadDll
=================
*/
void idSystemLocal::UnloadDll(void *dllHandle) {
    if(!dllHandle) {
        common->Printf("idSystemLocal::UnloadDll(nullptr)\n");
        return;
    }

    SDL_UnloadObject(dllHandle);
}

/*
=================
idSystemLocal::GetDLLName

Used to load a development dll instead of a virtual machine
=================
*/
valueType *idSystemLocal::GetDLLName(pointer name) {
    //Dushan - I have no idea what this mess is and what I made it before
    return va(nullptr, "%s" ARCH_STRING DLL_EXT, name);
}

/*
=================
idSystemLocal::LoadDllThread
=================
*/
void idSystemLocal::LoadDllThread(valueType *filename, void **libHandle) {
    *libHandle = SDL_LoadObject(filename);
}

/*
=================
idSystemLocal::LoadDll
=================
*/
void *idSystemLocal::LoadDll(pointer name) {
    void *libHandle = nullptr;
    static sint lastWarning = 0;
    valueType *basepath;
    valueType *homepath;
    valueType *gamedir;
    valueType *fn;
    valueType filename[MAX_QPATH];

    assert(name);

    Q_strncpyz(filename, GetDLLName(name), sizeof(filename));

    basepath = fs_basepath->string;
    homepath = fs_homepath->string;
    gamedir = fs_game->string;

    fn = fileSystem->BuildOSPath(basepath, gamedir, filename);

#if !defined( DEDICATED )

    // if the server is pure, extract the dlls from the bin.pk3 so
    // that they can be referenced
    if(sv_pure->integer && Q_stricmp(name, "sgame")) {
        fileSystem->CL_ExtractFromPakFile(homepath, gamedir, filename);
    }

#endif

    // Load DLL in separate thread
    std::thread t(LoadDllThread, fn, &libHandle);

    if(t.joinable()) {
        t.join();
    }

    if(!libHandle) {
        if(developer->integer) {
            common->Printf("idSystemLocal::LoadDll(%s) failed: \"%s\"\n", fn,
                           SDL_GetError());
        }

        if(homepath[0]) {
            fn = fileSystem->BuildOSPath(homepath, gamedir, filename);
            t = std::thread(LoadDllThread, fn, &libHandle);

            if(t.joinable()) {
                t.join();
            }

            if(!libHandle) {
                if(developer->integer) {
                    common->Printf("idSystemLocal::LoadDll(%s) failed: \"%s\"\n", fn,
                                   SDL_GetError());
                }
            }
        }

        if(!libHandle) {
            fn = fileSystem->BuildOSPath(basepath, BASEGAME, filename);
            t = std::thread(LoadDllThread, fn, &libHandle);

            if(t.joinable()) {
                t.join();
            }

            if(!libHandle && homepath[0]) {
                fn = fileSystem->BuildOSPath(homepath, BASEGAME, filename);
                t = std::thread(LoadDllThread, fn, &libHandle);

                if(t.joinable()) {
                    t.join();
                }
            }
        }
    }

#if defined (__LINUX__)
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);
#endif

    return libHandle;
}

/*
=================
idSystemLocal::GetProcAddress
=================
*/
void *idSystemLocal::GetProcAddress(void *dllhandle, pointer name) {
    return SDL_LoadFunction(dllhandle, name);
}

/*
=================
idSystemLocal::ParseArgs
=================
*/
void idSystemLocal::ParseArgs(sint argc, valueType **argv) {
    sint i;

    if(argc == 2) {
        if(!strcmp(argv[1], "--version") || !strcmp(argv[1], "-v")) {
            pointer date = __DATE__;
#ifdef DEDICATED
            fprintf(stdout, Q3_VERSION " dedicated server (%s)\n", date);
#else
            fprintf(stdout, Q3_VERSION " client (%s)\n", date);
#endif
            Exit(0);
        }
    }

    for(i = 1; i < argc; i++) {
        if(!strcmp(argv[i], "+nocurses")) {
            break;
        }
    }
}

/*
================
idSystemLocal::SignalToString
================
*/
pointer idSystemLocal::SignalToString(sint sig) {
    switch(sig) {
        case SIGINT:
            return "Terminal interrupt signal";

        case SIGILL:
            return "Illegal instruction";

        case SIGFPE:
            return "Erroneous arithmetic operation";
#if !defined (__LINUX__)

        case SIGSEGV:
            return "Invalid memory reference";
#endif

        case SIGTERM:
            return "Termination signal";
#if defined (_WIN32)

        case SIGBREAK:
            return "Control-break";
#endif

        case SIGABRT:
            return "Process abort signal";

        default:
            return "unknown";
    }
}

/*
=================
idSystemLocal::SigHandler
=================
*/
void idSystemLocal::SigHandler(sint signal) {
    static bool signalcaught = false;

    if(signalcaught) {
        common->Printf("DOUBLE SIGNAL FAULT: Received signal %d: \"%s\", exiting...\n",
                       signal, SignalToString(signal));
        exit(1);
    } else {
        signalcaught = true;
#if !defined (DEDICATED)
        clientMainSystem->Shutdown();
        serverInitSystem->Shutdown(va(nullptr, "Received signal %d", signal));
#endif
        systemLocal.Error("Received signal %d: \"%s\", exiting...\n", signal,
                          SignalToString(signal));
    }

    if(signal == SIGTERM || signal == SIGINT) {
        Exit(1);
    } else {
        Exit(2);
    }
}

/*
================
idSystemLocal::SnapVector
================
*/
void idSystemLocal::SysSnapVector(float32 *v) {
    v[0] = round(v[0]);
    v[1] = round(v[1]);
    v[2] = round(v[2]);
}

#if defined (DEDICATED) || defined (UPDATE_SERVER)
/*
===============
idSystemLocal::Init
===============
*/
void idSystemLocal::Init(void *windowData) {
}

/*
===============
idSystemLocal::Shutdown
===============
*/
void idSystemLocal::Shutdown(void) {
}

/*
===============
idSystemLocal::Restart
===============
*/
void idSystemLocal::Restart(void) {
}

/*
===============
idSystemLocal::Frame
===============
*/
void idSystemLocal::Frame(void) {
}
#endif

/*
=================
main
=================
*/
// Main function
#if defined (DEDICATED)
sint main(sint argc, valueType **argv)
#elif defined (__MACOSX__)
extern "C" sint engineMain(sint argc, valueType * *argv)
#elif defined (__LINUX__)
extern "C" sint engineMain(sint argc, valueType * *argv)
#else
Q_EXPORT sint engineMain(sint argc, valueType * *argv)
#endif
{
    sint i;
    valueType commandLine[ MAX_STRING_CHARS ] = { 0 };

#ifndef DEDICATED
    // SDL version check

    // Compile time
#   if !SDL_VERSION_ATLEAST(MINSDL_MAJOR,MINSDL_MINOR,MINSDL_PATCH)
#       error A more recent version of SDL is required
#   endif

    // Run time
    SDL_version ver;
    SDL_GetVersion(&ver);

#define MINSDL_VERSION \
    XSTRING(MINSDL_MAJOR) "." \
    XSTRING(MINSDL_MINOR) "." \
    XSTRING(MINSDL_PATCH)

    if(SDL_VERSIONNUM(ver.major, ver.minor, ver.patch) <
            SDL_VERSIONNUM(MINSDL_MAJOR, MINSDL_MINOR, MINSDL_PATCH)) {
        systemLocal.Dialog(DT_ERROR,
                           va(nullptr, "SDL version " MINSDL_VERSION " or greater is required, "
                              "but only version %d.%d.%d was found. You may be able to obtain a more recent copy "
                              "from http://www.libsdl.org/.", ver.major, ver.minor, ver.patch),
                           "SDL Library Too Old");

        systemLocal.Exit(1);
    }

#endif

    if(_setjmp(sys_exitframe)) {
        try {
#if !defined (DEDICATED) && !defined (UPDATE_SERVER)
            soundSystem->Shutdown();
            clientRendererSystem->ShutdownRef();
#endif
        }

        catch(...) {
            memorySystem->ReleaseMemory();
        }

        memorySystem->ReleaseMemory();
        return sys_retcode;
    }


    idSystemLocal::PlatformInit();

    // Set the initial time base
    systemLocal.Milliseconds();

    idSystemLocal::ParseArgs(argc, argv);

    idSystemLocal::SetBinaryPath(idSystemLocal::Dirname(argv[ 0 ]));
    idSystemLocal::SetDefaultInstallPath(DEFAULT_BASEDIR);
    idSystemLocal::SetDefaultLibPath(DEFAULT_BASEDIR);

    // Concatenate the command line for passing to Com_Init
    for(i = 1; i < argc; i++) {
        const bool containsSpaces = (bool)(strchr(argv[i], ' ') != nullptr);

        if(!::strcmp(argv[ i ], "+nocurses")) {
            continue;
        }

        if(!::strcmp(argv[ i ], "+showconsole")) {
            continue;
        }

        if(containsSpaces) {
            Q_strcat(commandLine, sizeof(commandLine), "\"");
        }

        Q_strcat(commandLine, sizeof(commandLine), argv[ i ]);

        if(containsSpaces) {
            Q_strcat(commandLine, sizeof(commandLine), "\"");
        }

        Q_strcat(commandLine, sizeof(commandLine), " ");
    }

    consoleCursesSystem->Init();

    common->Init(commandLine);
    networkSystem->Init();

    systemLocal.PDInit();

    while(1) {
        systemLocal.Frame();
        common->Frame();
    }

    return 0;
}

/*
========================================================================

Platform-dependent startup and shutdown

========================================================================
*/
void idSystemLocal::PDInit(void) {
    signal(SIGILL, systemLocal.SigHandler);
    signal(SIGFPE, systemLocal.SigHandler);
#if !defined (__LINUX__)
    signal(SIGSEGV, systemLocal.SigHandler);
#endif
    signal(SIGTERM, systemLocal.SigHandler);
    signal(SIGINT, systemLocal.SigHandler);
}

/*
===============
idSystemLocal::SetClipboardData
===============
*/
void idSystemLocal::SetClipboardData(pointer cbText) {
    SDL_SetClipboardText(cbText);
}
