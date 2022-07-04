////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2018 - 2022 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   precompiled.h
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_PRECOMPILED_HPP__
#define __R_PRECOMPILED_HPP__

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
#include <iostream>
#include <vector>

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


#ifdef _WIN32
#include <freetype/ft2build.h>
#else
#include <freetype2/ft2build.h>
#endif
#undef getch

#if defined (__LINUX__) || defined (__MACOSX__)
//hack
typedef int boolean;
#endif
extern "C"
{
#include <jpeglib.h>
}

#include <framework/appConfig.hpp>
#include <renderSystem/qgl.hpp>
#include <framework/types.hpp>
#include <framework/SurfaceFlags_Tech3.hpp>
#include <qcommon/q_platform.hpp>
#include <qcommon/q_shared.hpp>
#include <qcommon/qfiles.hpp>
#include <API/Common_api.hpp>
#include <API/Memory_api.hpp>
#include <API/FileSystem_api.hpp>
#include <API/CVarSystem_api.hpp>
#include <API/CmdBuffer_api.hpp>
#include <API/CmdSystem_api.hpp>
#include <API/system_api.hpp>
#include <API/clientAVI_api.hpp>
#include <API/clientMain_api.hpp>
#include <API/clientCinema_api.hpp>
#include <API/clientRenderer_api.hpp>
#include <renderSystem/r_types.hpp>
#include <API/cm_api.hpp>
#include <API/renderer_api.hpp>
#include <renderSystem/r_ConsoleVars.hpp>

#ifdef _WIN32
#include <freetype/ft2build.h>
#include <freetype/freetype.h>
#include <freetype/fterrors.h>
#include <freetype/ftsystem.h>
#include <freetype/ftimage.h>
#include <freetype/ftoutln.h>
#else
#include <freetype2/ft2build.h>
#include <freetype2/freetype/freetype.h>
#include <freetype2/freetype/fterrors.h>
#include <freetype2/freetype/ftsystem.h>
#include <freetype2/freetype/ftimage.h>
#include <freetype2/freetype/ftoutln.h>
#endif
#undef getch

#include <renderSystem/qgl.hpp>
#include <API/renderer_api.hpp>
#include <renderSystem/iqm.hpp>
#include <renderSystem/r_splash.hpp>
#include <renderSystem/r_common.hpp>
#include <renderSystem/r_extratypes.hpp>
#include <renderSystem/r_extramath.hpp>
#include <renderSystem/r_fbo.hpp>
#include <renderSystem/r_postprocess.hpp>
#include <renderSystem/r_dsa.hpp>
#include <renderSystem/r_local.hpp>
#include <renderSystem/r_cmdsTemplate.hpp>

#include <framework/Puff.hpp>
#include <framework/SurfaceFlags_Tech3.hpp>
#include <API/system_api.hpp>

#endif //!__R_PRECOMPILED_HPP__
