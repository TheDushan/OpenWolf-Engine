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
// File name:   cmd.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: Quake script command processing module
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.h>
#elif DEDICATED
#include <null/null_serverprecompiled.h>
#else
#include <framework/precompiled.h>
#endif

idCmdBufferSystemLocal cmdBufferLocal;
idCmdBufferSystem* cmdBufferSystem = &cmdBufferLocal;

/*
===============
idCmdBufferSystemLocal::idCmdBufferSystemLocal
===============
*/
idCmdBufferSystemLocal::idCmdBufferSystemLocal( void )
{
}

/*
===============
idCmdSystemLocal::~idCmdSystemLocal
===============
*/
idCmdBufferSystemLocal::~idCmdBufferSystemLocal( void )
{
}

/*
============
idCmdBufferSystemLocal::Init
============
*/
void idCmdBufferSystemLocal::Init( void )
{
    cmd_text.data = cmd_text_buf;
    cmd_text.maxsize = MAX_CMD_BUFFER;
    cmd_text.cursize = 0;
}

/*
============
idCmdBufferSystemLocal::AddText

Adds command text at the end of the buffer, does NOT add a final \n
============
*/
void idCmdBufferSystemLocal::AddText( pointer text )
{
    sint l;
    
    l = strlen( text );
    
    if( cmd_text.cursize + l >= cmd_text.maxsize )
    {
        Com_Printf( "idCmdBufferSystemLocal::AddText: overflow\n" );
        return;
    }
    
    ::memcpy( &cmd_text.data[cmd_text.cursize], text, l );
    cmd_text.cursize += l;
}


/*
============
idCmdBufferSystemLocal::InsertText

Adds command text immediately after the current command
Adds a \n to the text
============
*/
void idCmdBufferSystemLocal::InsertText( pointer text )
{
    sint i, len;
    
    len = strlen( text ) + 1;
    
    if( len + cmd_text.cursize > cmd_text.maxsize )
    {
        Com_Printf( "dCmdBufferSystemLocal::InsertText overflowed\n" );
        return;
    }
    
    // move the existing command text
    for( i = cmd_text.cursize - 1; i >= 0; i-- )
    {
        cmd_text.data[i + len] = cmd_text.data[i];
    }
    
    // copy the new text in
    ::memcpy( cmd_text.data, text, len - 1 );
    
    // add a \n
    cmd_text.data[len - 1] = '\n';
    cmd_text.cursize += len;
}


/*
============
idCmdBufferSystemLocal::ExecuteText
============
*/
void idCmdBufferSystemLocal::ExecuteText( sint exec_when, pointer text )
{
    switch( exec_when )
    {
    
        case EXEC_NOW:
            if( text && strlen( text ) > 0 )
            {
                Com_DPrintf( S_COLOR_YELLOW "EXEC_NOW %s\n", text );
                cmdSystemLocal.ExecuteString( text );
            }
            else
            {
                Execute();
                Com_DPrintf( S_COLOR_YELLOW "EXEC_NOW %s\n", cmd_text.data );
            }
            break;
            
        case EXEC_INSERT:
            InsertText( text );
            break;
            
        case EXEC_APPEND:
            AddText( text );
            break;
            
        default:
            Com_Error( ERR_FATAL, "idCmdBufferSystemLocal::ExecuteText: bad exec_when" );
    }
}

/*
============
idCmdBufferSystemLocal::Execute
============
*/
void idCmdBufferSystemLocal::Execute( void )
{
    sint i, quotes;
    valueType* text;
    valueType line[MAX_CMD_LINE];
    
    // This will keep // style comments all on one line by not breaking on
    // a semicolon.  It will keep /* ... */ style comments all on one line by not
    // breaking it for semicolon or newline.
    bool in_star_comment = false;
    bool in_slash_comment = false;
    while( cmd_text.cursize > 0 )
    {
        if( cmd_wait  > 0 )
        {
            // skip out while text still remains in buffer, leaving it
            // for next frame
            cmd_wait--;
            break;
        }
        
        // find a \n or ; line break or comment: // or /* */
        text = reinterpret_cast<valueType*>( cmd_text.data );
        
        quotes = 0;
        for( i = 0; i < cmd_text.cursize; i++ )
        {
            if( text[i] == '\\' && text[i + 1] == '"' )
            {
                i++;
                continue;
            }
            
            if( text[i] == '"' )
            {
                quotes++;
            }
            
            if( !( quotes & 1 ) )
            {
                if( i < cmd_text.cursize - 1 )
                {
                    if( !in_star_comment && text[i] == '/' && text[i + 1] == '/' )
                    {
                        in_slash_comment = true;
                    }
                    else if( !in_slash_comment && text[i] == '/' && text[i + 1] == '*' )
                    {
                        in_star_comment = true;
                    }
                    else if( in_star_comment && text[i] == '*' && text[i + 1] == '/' )
                    {
                        in_star_comment = false;
                        // If we are in a star comment, then the part after it is valid
                        // Note: This will cause it to NUL out the terminating '/'
                        // but ExecuteString doesn't require it anyway.
                        i++;
                        break;
                    }
                }
                
                if( !in_slash_comment && !in_star_comment && text[i] == ';' )
                {
                    break;
                }
            }
            
            if( !in_star_comment && ( text[i] == '\n' || text[i] == '\r' ) )
            {
                in_slash_comment = false;
                break;
            }
        }
        
        if( i >= ( MAX_CMD_LINE - 1 ) )
        {
            i = MAX_CMD_LINE - 1;
        }
        
        ::memcpy( line, text, i );
        line[i] = 0;
        
        // delete the text from the command buffer and move remaining commands down
        // this is necessary because commands (exec) can insert data at the
        // beginning of the text buffer
        
        if( i == cmd_text.cursize )
        {
            cmd_text.cursize = 0;
        }
        else
        {
            i++;
            cmd_text.cursize -= i;
            // skip all repeating newlines/semicolons
            while( ( text[i] == '\n' || text[i] == '\r' || text[i] == ';' ) && cmd_text.cursize > 0 )
            {
                cmd_text.cursize--;
                i++;
            }
            memmove( text, text + i, cmd_text.cursize );
        }
        
        // execute the command line
        cmdSystemLocal.ExecuteString( line );
    }
}
