////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2018 - 2023 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of OpenWolf.
//
// OpenWolf is free software; you can redistribute it
// and / or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the License,
// or (at your option) any later version.
//
// OpenWolf is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenWolf; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110 - 1301  USA
//
// -------------------------------------------------------------------------------------
// File name:   precompiled.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __PRECOMPILED_HPP__
#define __PRECOMPILED_HPP__

//Dushan
//FIX ALL THIS

#include <random>
#include <iostream>
#include <setjmp.h>
#include <iostream>
#include <mutex>
#include <queue>
#include <assert.h>

#ifndef _WIN32
#include <sys/ioctl.h>
#endif

#include <fcntl.h>
#include <algorithm>

#include <signal.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

#ifdef _WIN32
#include <SDL_syswm.h>
#include <io.h>
#include <shellapi.h>
#include <timeapi.h>
#include <windows.h>
#include <direct.h>
#include <lmerr.h>
#include <lmcons.h>
#include <lmwksta.h>
#include <conio.h>
#include <wincrypt.h>
#include <shlobj.h>
#include <psapi.h>
#include <float.h>
#include <Shobjidl.h>
#include <platform/windows/resource.hpp>
#pragma fenv_access (on)
#else
#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>
#include <fenv.h>
#include <pwd.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>
#include <SDL_thread.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>
#include <SDL2/SDL_thread.h>
#endif
#include <curl/curl.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2spi.h>
#else
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <ifaddrs.h>
#endif

#include <framework/appConfig.hpp>
#include <framework/types.hpp>
#include <qcommon/q_platform.hpp>
#include <qcommon/q_shared.hpp>
#include <cm/cm_bsp.hpp>
#include <qcommon/qfiles.hpp>
#include <API/MessagesToFunctions_api.hpp>
#include <framework/MessagesToFunctions.hpp>
#include <API/Common_api.hpp>
#include <framework/Common.hpp>
#include <API/cm_api.hpp>
#include <cm/cm_polylib.hpp>
#include <cm/cm_patch.hpp>
#include <API/Memory_api.hpp>
#include <framework/Memory.hpp>
#include <API/clientDownload_api.hpp>
#include <API/FileSystem_api.hpp>
#include <API/CVarSystem_api.hpp>
#include <API/download_api.hpp>
#include <framework/CommonConsoleVars.hpp>
#include <API/serverGame_api.hpp>
#include <server/server.hpp>
#include <API/serverWorld_api.hpp>
#include <API/serverInit_api.hpp>
#include <API/CmdBuffer_api.hpp>
#include <API/CmdSystem_api.hpp>
#include <API/system_api.hpp>
#include <renderSystem/r_types.hpp>
#include <API/clientScreen_api.hpp>
#include <client/clientScreen.hpp>
#include <API/clientGame_api.hpp>
#include <API/clientLAN_api.hpp>
#include <API/clientGUI_api.hpp>
#include <API/clientAVI_api.hpp>
#include <API/clientNetChan_api.hpp>
#include <client/clientNetChan.hpp>
#include <client/clientAVI.hpp>
#include <client/clientGame.hpp>
#include <API/clientCinema_api.hpp>
#include <client/clientCinema.hpp>
#include <API/clientLocalization_api.hpp>
#include <client/clientLocalization.hpp>
#include <API/clientConsole_api.hpp>
#include <client/clientConsole.hpp>
#include <API/clientInput_api.hpp>
#include <client/clientInput.hpp>
#include <API/clientMain_api.hpp>
#include <client/clientAuthorization.hpp>
#include <API/clientAutoUpdate_api.hpp>
#include <client/clientAutoUpdate.hpp>
#include <API/clientConsoleCommands_api.hpp>
#include <client/clientConsoleCommands.hpp>
#include <client/clientDemo.hpp>
#include <client/clientDownload.hpp>
#include <client/clientGUID.hpp>
#include <client/clientInput.hpp>
#include <framework/keycodes.hpp>
#include <client/clientMotd.hpp>
#include <API/clientReliableCommands_api.hpp>
#include <client/clientReliableCommands.hpp>
#include <API/clientRenderer_api.hpp>
#include <client/clientRenderer.hpp>
#include <client/clientStartUpCache.hpp>
#include <client/clientWave.hpp>
#include <client/clientMain.hpp>
#include <API/renderer_api.hpp>

#include <cm/cm_local.hpp>

#include <API/download_api.hpp>
#include <download/downloadLocal.hpp>

#include <API/Parse_api.hpp>
#include <framework/Parse.hpp>
#include <soundSystem/sndSystem_codec.hpp>
#include <API/soundSystem_api.hpp>
#include <soundSystem/sndSystem_local.hpp>
#include <framework/CmdSystem.hpp>
#include <API/CmdCompletion_api.hpp>
#include <framework/CmdCompletion.hpp>
#include <API/clientKeys_api.hpp>
#include <client/clientKeys.hpp>
#include <API/cgame_api.hpp>
#include <API/gui_api.hpp>
#include <client/clientPublic.hpp>
#include <client/clientLocal.hpp>
#include <client/clientParse.hpp>

#include <zlib.h>
#include <bzlib.h>

#include <framework/IOAPI.hpp>
#include <framework/Unzip.hpp>
#include <framework/Puff.hpp>
#include <framework/SurfaceFlags_Tech3.hpp>
#include <platform/systemLocal.hpp>

#ifdef __LINUX_
#include <stdint.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pwd.h>
#include <libgen.h>
#include <fcntl.h>
#include <fenv.h>
#endif

#ifdef _MSC_VER
// MSVC users must install the OpenAL SDK which doesn't use the AL/*.h scheme.
#include <al.h>
#include <alc.h>
#elif defined (__MACOSX__)
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#include <openssl/md4.h>
#include <openssl/md5.h>

#include <framework/FileSystem.hpp>
#include <API/CVarSystem_api.hpp>
#include <framework/CVarSystem.hpp>
#include <API/bgame_api.hpp>
#include <API/serverMain_api.hpp>
#include <API/sgame_api.hpp>
#include <server/serverCcmds.hpp>
#include <API/serverClient_api.hpp>
#include <API/TigerHash_api.hpp>
#include <framework/TigerHash.hpp>
#include <API/HashTable_api.hpp>
#include <framework/HashTable.hpp>
#include <server/serverCommunity.hpp>
#include <server/serverClient.hpp>
#include <server/serverGame.hpp>
#include <server/serverWorld.hpp>
#include <API/serverSnapshot_api.hpp>
#include <server/serverSnapshot.hpp>
#include <API/serverNetChan_api.hpp>
#include <server/serverNetChan.hpp>
#include <server/serverInit.hpp>
#include <API/serverMain_api.hpp>
#include <server/serverMain.hpp>
#include <framework/CmdBuffer.hpp>
#include <API/CmdDelay_api.hpp>
#include <framework/CmdDelay.hpp>
#include <API/MD4_api.hpp>
#include <framework/MD4.hpp>
#include <API/MD5_api.hpp>
#include <framework/MD5.hpp>
#include <API/NetworkChain_api.hpp>
#include <framework/NetworkChain.hpp>
#include <API/Network_api.hpp>
#include <framework/Network.hpp>
#include <server/serverOACS.hpp>
#include <server/serverWallhack.hpp>

// includes for the OGG codec
#include <errno.h>
#define OV_EXCLUDE_STATIC_CALLBACKS
#include <vorbis/vorbisfile.h>

// curses.h defines COLOR_*, which are already defined in q_shared.h
#undef COLOR_BLACK
#undef COLOR_RED
#undef COLOR_GREEN
#undef COLOR_YELLOW
#undef COLOR_BLUE
#undef COLOR_MAGENTA
#undef COLOR_CYAN
#undef COLOR_WHITE

#include <curses.h>

#include <API/consoleHistory_api.hpp>
#include <console/consoleHistory.hpp>
#include <API/consoleCurses_api.hpp>
#include <console/consoleCurses.hpp>
#include <API/consoleLogging_api.hpp>
#include <console/consoleLogging.hpp>

#include <framework/Huffman.hpp>

#include <client/clientBrowser.hpp>
#include <API/clientLAN_api.hpp>
#include <client/clientLAN.hpp>
#include <API/clientGUI_api.hpp>
#include <client/clientGUI.hpp>

#endif //!__PRECOMPILED_HPP__
