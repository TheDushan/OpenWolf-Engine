////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 - 2023 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   types.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __TYPES_HPP__
#define __TYPES_HPP__

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//-----------------------------------------Basic Types--------------------------------------------------//
//////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef signed char
schar8;      ///< Compiler independent Signed Char
typedef unsigned char
uchar8;      ///< Compiler independent Unsigned Char

typedef signed short
schar16;     ///< Compiler independent Signed 16-bit short
typedef unsigned short
uchar16;     ///< Compiler independent Unsigned 16-bit short

typedef signed int
sint;     ///< Compiler independent Signed 32-bit integer
typedef unsigned int
uint;     ///< Compiler independent Unsigned 32-bit integer

#if defined (__MACOSX__)
typedef signed int
sint32;     ///< Compiler independent Signed 64-bit integer
typedef unsigned int
uint32;     ///< Compiler independent Unsigned 64-bit integer
#else
typedef signed long
sint32;     ///< Compiler independent Signed 64-bit integer
typedef unsigned long
uint32;     ///< Compiler independent Unsigned 64-bit integer
#endif

#ifdef _WIN32
typedef signed __int64     sint64;
typedef unsigned __int64   uint64;
#else
typedef signed long long   sint64;
typedef unsigned long long uint64;
#endif

typedef float
float32;     ///< Compiler independent 32-bit float
typedef double
float64;     ///< Compiler independent 64-bit float

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//----------------------------------------String Types--------------------------------------------------//
//////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef char
valueType;        ///< Compiler independent 8 bit Unicode encoded character
typedef const char *pointer;
typedef const char &reference;


#endif //__TYPES_HPP__
