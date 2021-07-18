////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2019 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   systemMacOS.cpp
// Created:
// Compilers:   
// Description: 
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

//Dushan - broken

#if !defined (_WIN32) && !defined (__LINUX__)

#ifndef MACOS_X
#error This file is for Mac OS X only. You probably should not compile it.
#endif

// Please note that this file is just some Mac-specific bits. Most of the
// Mac OS X code is shared with other Unix platforms in sys_unix.c ...

#ifdef UPDATE_SERVER
#include <server/serverAutoPrecompiled.hpp>
#elif DEDICATED
#include <server/serverDedPrecompiled.hpp>
#else
#include <framework/precompiled.hpp>
#endif

/*
================
idSystemLocal::TempPath
================
*/
pointer idSystemLocal::TempPath( void )
{
    static UInt8 posixPath[ MAX_OSPATH ];
    FSRef ref;
    if( FSFindFolder( kOnAppropriateDisk,
                      kTemporaryFolderType, kCreateFolder, &ref ) == noErr )
    {
        if( FSRefMakePath( &ref, posixPath,
                           sizeof( posixPath ) - 1 ) == noErr )
        {
            return ( pointer )posixPath;
        }
    }
    
    return "/tmp";
}

/*
==================
IdSystemLocal::SysGetClipboardData
==================
*/
valueType* idSystemLocal::SysGetClipboardData( void )
{
    FILE* pipe = popen( "pbpaste", "r" );
    valueType buffer[MAX_EDIT_LINE];
    valueType* data = NULL;
    
    if( !pipe )
    {
        return NULL;
    }
    
    fgets( buffer, sizeof( buffer ), pipe );
    pclose( pipe );
    
    data = Z_Malloc( sizeof( buffer ) );
    Q_strncpyz( data, buffer, sizeof( buffer ) );
    
    return data;
}

/*
==============
idSystemLocal::Dialog

Display an OS X dialog box
==============
*/
dialogResult_t idSystemLocal::Dialog( dialogType_t type, pointer message, pointer title )
{
    dialogResult_t result = DR_OK;
    NSAlert* alert = [NSAlert new];
    
[alert setMessageText: [NSString stringWithUTF8String: title]];
[alert setInformativeText: [NSString stringWithUTF8String: message]];

    if( type == DT_ERROR )
[alert setAlertStyle: NSCriticalAlertStyle];
    else
[alert setAlertStyle: NSWarningAlertStyle];

    switch( type )
    {
        default:
            [alert runModal];
            result = DR_OK;
            break;
            
        case DT_YES_NO:
[alert addButtonWithTitle: @"Yes"];
[alert addButtonWithTitle: @"No"];
            switch( [alert runModal] )
            {
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
[alert addButtonWithTitle: @"OK"];
[alert addButtonWithTitle: @"Cancel"];

            switch( [alert runModal] )
            {
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
    
    [alert release];
    
    return result;
}

valueType* idSystemLocal::StripAppBundle( valueType* dir )
{
    static valueType cwd[MAX_OSPATH];
    
    Q_strncpyz( cwd, dir, sizeof( cwd ) );
    if( ::strcmp( Basename( cwd ), "MacOS" ) )
        return dir;
    Q_strncpyz( cwd, Dirname( cwd ), sizeof( cwd ) );
    if( ::strcmp( Basename( cwd ), "Contents" ) )
        return dir;
    Q_strncpyz( cwd, Dirname( cwd ), sizeof( cwd ) );
    if( !::strstr( Basename( cwd ), ".app" ) )
        return dir;
    Q_strncpyz( cwd, Dirname( cwd ), sizeof( cwd ) );
    return cwd;
}

#endif //
