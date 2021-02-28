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
// File name:   consoleLogging.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.hpp>
#elif DEDICATED
#include <null/null_serverprecompiled.hpp>
#else
#include <framework/precompiled.hpp>
#endif

idConsoleLoggingSystemLocal consoleLoggingLocal;
idConsoleLoggingSystem* consoleLoggingSystem = &consoleLoggingLocal;

/*
===============
idConsoleLoggingSystemLocal::idConsoleLoggingSystemLocal
===============
*/
idConsoleLoggingSystemLocal::idConsoleLoggingSystemLocal( void )
{
}

/*
===============
idConsoleLoggingSystemLocal::~idConsoleLoggingSystemLocal
===============
*/
idConsoleLoggingSystemLocal::~idConsoleLoggingSystemLocal( void )
{
}

/*
==================
idConsoleLoggingSystemLocal::LogSize
==================
*/
uint64 idConsoleLoggingSystemLocal::LogSize( void )
{
    if( readPos <= writePos )
    {
        return writePos - readPos;
    }
    else
    {
        return writePos + MAX_LOG - readPos;
    }
}

/*
==================
idConsoleLoggingSystemLocal::LogFree
==================
*/
uint64 idConsoleLoggingSystemLocal::LogFree( void )
{
    return MAX_LOG - LogSize( ) - 1;
}

/*
==================
idConsoleLoggingSystemLocal::LogWrite
==================
*/
uint64 idConsoleLoggingSystemLocal::LogWrite( pointer in )
{
    uint64 length = ::strlen( in );
    uint64 firstChunk, secondChunk;
    
    while( LogFree( ) < length && LogSize( ) > 0 )
    {
        // Free enough space
        while( consoleLog[readPos] != '\n' && LogSize() > 1 )
        {
            readPos = ( readPos + 1 ) % MAX_LOG;
        }
        
        // Skip past the '\n'
        readPos = ( readPos + 1 ) % MAX_LOG;
    }
    
    if( LogFree() < length )
    {
        return 0;
    }
    
    if( writePos + length > MAX_LOG )
    {
        firstChunk  = MAX_LOG - writePos;
        secondChunk = length - firstChunk;
    }
    else
    {
        firstChunk  = length;
        secondChunk = 0;
    }
    
    ::memcpy( consoleLog + writePos, in, firstChunk );
    ::memcpy( consoleLog, in + firstChunk, secondChunk );
    
    writePos = ( writePos + length ) % MAX_LOG;
    
    return length;
}

/*
==================
idConsoleLoggingSystemLocal::LogRead
==================
*/
uint64 idConsoleLoggingSystemLocal::LogRead( valueType* out, uint64 outSize )
{
    uint64 firstChunk, secondChunk;
    
    if( LogSize() < outSize )
    {
        outSize = LogSize();
    }
    
    if( readPos + outSize > MAX_LOG )
    {
        firstChunk  = MAX_LOG - readPos;
        secondChunk = outSize - firstChunk;
    }
    else
    {
        firstChunk  = outSize;
        secondChunk = 0;
    }
    
    ::memcpy( out, consoleLog + readPos, firstChunk );
    ::memcpy( out + firstChunk, consoleLog, secondChunk );
    
    readPos = ( readPos + outSize ) % MAX_LOG;
    
    return outSize;
}
