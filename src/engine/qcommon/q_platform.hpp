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
// File name:   q_platform.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __Q_PLATFORM_HPP__
#define __Q_PLATFORM_HPP__

//================================================================= WIN64/32 ===
#if defined(_WIN64) || defined(__WIN64__)

#define MAC_STATIC

#undef DLLEXPORT
#define DLLEXPORT __declspec(dllexport)

#if defined( _MSC_VER )
#define OS_STRING "win_msvc64"
#elif defined __MINGW64__
#define OS_STRING "win_mingw64"
#endif

//#define ID_INLINE __inline
#define ID_INLINE __forceinline /* use __forceinline (VC++ specific) */
#define PATH_SEP '\\'

#if defined( __WIN64__ )
#define ARCH_STRING "AMD64"
#endif

#define Q3_LITTLE_ENDIAN

#define DLL_DIRECTORY "libs"
#define DLL_PREFIX ""
#define DLL_EXT ".dll"

#undef QDECL
#define QDECL __cdecl
#endif
//============================================================== MAC OS X ===

#if defined (__MACOSX__)

#define MAC_STATIC

// make sure this is defined, just for sanity's sake...
#ifndef MACOS_X
#define MACOS_X
#endif

#define OS_STRING "macosx"
#define ID_INLINE __inline
#define PATH_SEP '/'

#ifdef __x86_64__
#define ARCH_STRING "x86_64"
#define Q3_LITTLE_ENDIAN
#endif

#define DLL_DIRECTORY "libs"
#define DLL_PREFIX "lib"
#define DLL_EXT ".dylib"

#undef QDECL
#define QDECL

#endif

//================================================================= LINUX ===

#if defined(__linux__) || defined(__FreeBSD_kernel__)

#define MAC_STATIC

#include <endian.h>

#if defined  (__linux__)
#define OS_STRING "linux"
#else
#define OS_STRING "kFreeBSD"
#endif

#if defined (__clang__)
#define ID_INLINE static inline
#else
#define ID_INLINE __inline
#endif

#define PATH_SEP '/'

#if defined __x86_64__
#define ARCH_STRING "x86_64"
#elif defined __arm__
#define ARCH_STRING "arm"
#endif

#if __FLOAT_WORD_ORDER == __BIG_ENDIAN
#define Q3_BIG_ENDIAN
#else
#define Q3_LITTLE_ENDIAN
#endif

#define DLL_DIRECTORY "libs"
#define DLL_PREFIX "lib"
#define DLL_EXT ".so"

#undef QDECL
#define QDECL

#endif

//=================================================================== BSD ===

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)

#include <sys/types.h>
#include <machine/endian.h>

#define MAC_STATIC

#ifndef __BSD__
#define __BSD__
#endif

#if defined(__FreeBSD__)
#define OS_STRING "freebsd"
#elif defined(__OpenBSD__)
#define OS_STRING "openbsd"
#elif defined(__NetBSD__)
#define OS_STRING "netbsd"
#elif defined(__DragonFly__)
#define OS_STRING "dragonfly"
#endif

#define ID_INLINE __inline
#define PATH_SEP '/'

#if defined __amd64__
#define ARCH_STRING "AMD64"
#elif defined __x86_64__
#define ARCH_STRING "x86_64"
#endif

#if BYTE_ORDER == BIG_ENDIAN
#define Q3_BIG_ENDIAN
#else
#define Q3_LITTLE_ENDIAN
#endif

#define DLL_DIRECTORY "libs"
#define DLL_PREFIX "lib"
#define DLL_EXT ".so"

#endif

//===========================================================================

//catch missing defines in above blocks
#if !defined( OS_STRING )
#error "Operating system not supported"
#endif

#if !defined( ARCH_STRING )
#error "Architecture not supported"
#endif

#ifndef ID_INLINE
#error "ID_INLINE not defined"
#endif

#ifndef PATH_SEP
#error "PATH_SEP not defined"
#endif

#ifndef DLL_EXT
#error "DLL_EXT not defined"
#endif

//endianness
schar16 ShortSwap(schar16 l);
sint LongSwap(sint l);
float32 FloatSwap(float32 f);

#if defined( Q3_BIG_ENDIAN ) && defined( Q3_LITTLE_ENDIAN )
#error "Endianness defined as both big and little"
#elif defined( Q3_BIG_ENDIAN )

#define LittleShort(x) ShortSwap(x)
#define LittleLong(x) LongSwap(x)
#define LittleFloat(x) FloatSwap(&x)
#define BigShort
#define BigLong
#define BigFloat

#elif defined( Q3_LITTLE_ENDIAN )

#define LittleShort
#define LittleLong
#define LittleFloat
#define BigShort(x) ShortSwap(x)
#define BigLong(x) LongSwap(x)
#define BigFloat(x) FloatSwap(&x)

#else
#error "Endianness not defined"
#endif

//platform string
#ifdef NDEBUG
#define PLATFORM_STRING OS_STRING "-" ARCH_STRING
#else
#define PLATFORM_STRING OS_STRING "-" ARCH_STRING "-debug"
#endif

#endif //!__Q_PLATFORM_HPP__
