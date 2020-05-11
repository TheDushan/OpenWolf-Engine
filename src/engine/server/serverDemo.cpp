////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2018 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   sv_bot.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2017, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.h>
#elif DEDICATED
#include <null/null_serverprecompiled.h>
#else
#include <framework/precompiled.h>
#endif

idServerDemoSystemLocal serverDemoSystemLocal;
idServerDemoSystem* serverDemoSystem = &serverDemoSystemLocal;

/*
===============
idServerDemoSystemLocal::idServerDemoSystemLocal
===============
*/
idServerDemoSystemLocal::idServerDemoSystemLocal( void )
{
}

/*
===============
idServerDemoSystemLocal::~idServerDemoSystemLocal
===============
*/
idServerDemoSystemLocal::~idServerDemoSystemLocal( void )
{
}

/*
====================
idServerDemoSystemLocal::DemoWriteMessage

Write a message to the demo file
====================
*/
void idServerDemoSystemLocal::DemoWriteMessage( msg_t* msg )
{
    S32 len;
    
    // Write the entire message to the file, prefixed by the length
    MSG_WriteByte( msg, demo_EOF );
    len = LittleLong( msg->cursize );
    fileSystem->Write( &len, 4, sv.demoFile );
    fileSystem->Write( msg->data, msg->cursize, sv.demoFile );
    MSG_Clear( msg );
}

/*
====================
idServerDemoSystemLocal::DemoWriteServerCommand

Write a server command to the demo file
====================
*/
void idServerDemoSystemLocal::DemoWriteServerCommand( StringEntry str )
{
    msg_t msg;
    
    MSG_Init( &msg, buf, sizeof( buf ) );
    MSG_WriteByte( &msg, demo_serverCommand );
    MSG_WriteString( &msg, str );
    DemoWriteMessage( &msg );
}

/*
====================
SV_DemoWriteGameCommand

Write a game command to the demo file
====================
*/
void idServerDemoSystemLocal::DemoWriteGameCommand( S32 cmd, StringEntry str )
{
    msg_t msg;
    
    MSG_Init( &msg, buf, sizeof( buf ) );
    MSG_WriteByte( &msg, demo_gameCommand );
    MSG_WriteByte( &msg, cmd );
    MSG_WriteString( &msg, str );
    DemoWriteMessage( &msg );
}

/*
====================
SV_DemoWriteFrame

Record all the entities and players at the end the frame
====================
*/
void idServerDemoSystemLocal::DemoWriteFrame( void )
{
    msg_t msg;
    playerState_t* player;
    sharedEntity_t* entity;
    S32 i;
    
    MSG_Init( &msg, buf, sizeof( buf ) );
    
    // Write entities
    MSG_WriteByte( &msg, demo_entityState );
    for( i = 0; i < sv.num_entities; i++ )
    {
        if( i >= sv_maxclients->integer && i < MAX_CLIENTS )
            continue;
        entity = serverGameSystem->GentityNum( i );
        entity->s.number = i;
        MSG_WriteDeltaEntity( &msg, &sv.demoEntities[i].s, &entity->s, false );
        sv.demoEntities[i].s = entity->s;
    }
    MSG_WriteBits( &msg, ENTITYNUM_NONE, GENTITYNUM_BITS );
    MSG_WriteByte( &msg, demo_entityShared );
    for( i = 0; i < sv.num_entities; i++ )
    {
        if( i >= sv_maxclients->integer && i < MAX_CLIENTS )
        {
            continue;
        }
        entity = serverGameSystem->GentityNum( i );
        MSG_WriteDeltaSharedEntity( &msg, &sv.demoEntities[i].r, &entity->r, false, i );
        sv.demoEntities[i].r = entity->r;
    }
    MSG_WriteBits( &msg, ENTITYNUM_NONE, GENTITYNUM_BITS );
    DemoWriteMessage( &msg );
    
    // Write clients
    for( i = 0; i < sv_maxclients->integer; i++ )
    {
        if( svs.clients[i].state < CS_ACTIVE )
        {
            continue;
        }
        
        player = serverGameSystem->GameClientNum( i );
        MSG_WriteByte( &msg, demo_playerState );
        MSG_WriteBits( &msg, i, CLIENTNUM_BITS );
        MSG_WriteDeltaPlayerstate( &msg, &sv.demoPlayerStates[i], player );
        sv.demoPlayerStates[i] = *player;
    }
    MSG_WriteByte( &msg, demo_endFrame );
    MSG_WriteLong( &msg, svs.time );
    DemoWriteMessage( &msg );
}

/*
====================
SV_DemoReadFrame

Play a frame from the demo file
====================
*/
void idServerDemoSystemLocal::DemoReadFrame( void )
{
    msg_t msg;
    S32 cmd, r, num, i;
    playerState_t* player;
    sharedEntity_t* entity;
    
    MSG_Init( &msg, buf, sizeof( buf ) );
    
    while( 1 )
    {
exit_loop:
        // Get a message
        r = fileSystem->Read( &msg.cursize, 4, sv.demoFile );
        if( r != 4 )
        {
            DemoStopPlayback();
            return;
        }
        msg.cursize = LittleLong( msg.cursize );
        if( msg.cursize > msg.maxsize )
        {
            Com_Error( ERR_DROP, "SV_DemoReadFrame: demo message too long" );
        }
        r = fileSystem->Read( msg.data, msg.cursize, sv.demoFile );
        if( r != msg.cursize )
        {
            Com_Printf( "Demo file was truncated.\n" );
            DemoStopPlayback();
            return;
        }
        
        // Parse the message
        while( 1 )
        {
            cmd = MSG_ReadByte( &msg );
            switch( cmd )
            {
                default:
                    Com_Error( ERR_DROP, "SV_DemoReadFrame: Illegible demo message\n" );
                    return;
                case demo_EOF:
                    MSG_Clear( &msg );
                    goto exit_loop;
                case demo_endDemo:
                    DemoStopPlayback();
                    return;
                case demo_endFrame:
                    // Overwrite anything the game may have changed
                    for( i = 0; i < sv.num_entities; i++ )
                    {
                        if( i >= sv_democlients->integer && i < MAX_CLIENTS )
                        {
                            continue;
                        }
                        *serverGameSystem->GentityNum( i ) = sv.demoEntities[i];
                    }
                    for( i = 0; i < sv_democlients->integer; i++ )
                        *serverGameSystem->GameClientNum( i ) = sv.demoPlayerStates[i];
                    // Set the server time
                    svs.time = MSG_ReadLong( &msg );
                    return;
                case demo_serverCommand:
                    cmdSystem->SaveCmdContext();
                    cmdSystem->TokenizeString( MSG_ReadString( &msg ) );
                    serverMainSystem->SendServerCommand( NULL, "%s \"^3[DEMO] ^7%s\"", cmdSystem->Argv( 0 ), cmdSystem->ArgsFrom( 1 ) );
                    cmdSystem->RestoreCmdContext();
                    break;
                case demo_gameCommand:
                    num = MSG_ReadByte( &msg );
                    cmdSystem->SaveCmdContext();
                    cmdSystem->TokenizeString( MSG_ReadString( &msg ) );
#if !defined (DEDICATED) || !defined (UPDATE_SERVER)
                    sgame->GameDemoCommand( num );
#endif
                    cmdSystem->RestoreCmdContext();
                    break;
                case demo_playerState:
                    num = MSG_ReadBits( &msg, CLIENTNUM_BITS );
                    player = serverGameSystem->GameClientNum( num );
                    MSG_ReadDeltaPlayerstate( &msg, &sv.demoPlayerStates[num], player );
                    sv.demoPlayerStates[num] = *player;
                    break;
                case demo_entityState:
                    while( 1 )
                    {
                        num = MSG_ReadBits( &msg, GENTITYNUM_BITS );
                        if( num == ENTITYNUM_NONE )
                            break;
                        entity = serverGameSystem->GentityNum( num );
                        MSG_ReadDeltaEntity( &msg, &sv.demoEntities[num].s, &entity->s, num );
                        sv.demoEntities[num].s = entity->s;
                    }
                    break;
                case demo_entityShared:
                    while( 1 )
                    {
                        num = MSG_ReadBits( &msg, GENTITYNUM_BITS );
                        if( num == ENTITYNUM_NONE )
                        {
                            break;
                        }
                        entity = serverGameSystem->GentityNum( num );
                        MSG_ReadDeltaSharedEntity( &msg, &sv.demoEntities[num].r, &entity->r, num );
                        
                        // Link/unlink the entity
                        if( entity->r.linked && ( !sv.demoEntities[num].r.linked || entity->r.linkcount != sv.demoEntities[num].r.linkcount ) )
                        {
                            serverWorldSystem->LinkEntity( entity );
                        }
                        else if( !entity->r.linked && sv.demoEntities[num].r.linked )
                        {
                            serverWorldSystem->UnlinkEntity( entity );
                        }
                        
                        sv.demoEntities[num].r = entity->r;
                        if( num > sv.num_entities )
                        {
                            sv.num_entities = num;
                        }
                    }
                    break;
            }
        }
    }
}

/*
====================
idServerDemoSystemLocal::DemoStartRecord

sv.demo* have already been set and the demo file opened, start writing gamestate info
====================
*/
void idServerDemoSystemLocal::DemoStartRecord( void )
{
    msg_t msg;
    
    MSG_Init( &msg, buf, sizeof( buf ) );
    
    // Write current time
    MSG_WriteLong( &msg, svs.time );
    // Write map name
    MSG_WriteString( &msg, sv_mapname->string );
    // Write number of clients (sv_maxclients < MAX_CLIENTS or else we can't playback)
    MSG_WriteBits( &msg, sv_maxclients->integer, CLIENTNUM_BITS );
    DemoWriteMessage( &msg );
    
    // Write entities and players
    ::memset( sv.demoEntities, 0, sizeof( sv.demoEntities ) );
    ::memset( sv.demoPlayerStates, 0, sizeof( sv.demoPlayerStates ) );
    DemoWriteFrame();
    Com_Printf( "Recording demo %s.\n", sv.demoName );
    sv.demoState = DS_RECORDING;
    cvarSystem->SetValue( "sv_demoState", DS_RECORDING );
}

/*
====================
SV_DemoStopRecord

Write end of file and close the demo file
====================
*/
void idServerDemoSystemLocal::DemoStopRecord( void )
{
    msg_t msg;
    
    // End the demo
    MSG_Init( &msg, buf, sizeof( buf ) );
    MSG_WriteByte( &msg, demo_endDemo );
    DemoWriteMessage( &msg );
    
    fileSystem->FCloseFile( sv.demoFile );
    sv.demoState = DS_NONE;
    cvarSystem->SetValue( "sv_demoState", DS_NONE );
    Com_Printf( "Stopped recording demo %s.\n", sv.demoName );
}

/*
====================
idServerDemoSystemLocal::DemoStartPlayback

sv.demo* have already been set and the demo file opened, start reading gamestate info
====================
*/
void idServerDemoSystemLocal::DemoStartPlayback( void )
{
    msg_t msg;
    S32 r, i, clients;
    UTF8* s;
    
    MSG_Init( &msg, buf, sizeof( buf ) );
    
    // Get the demo header
    r = fileSystem->Read( &msg.cursize, 4, sv.demoFile );
    if( r != 4 )
    {
        DemoStopPlayback();
        return;
    }
    msg.cursize = LittleLong( msg.cursize );
    if( msg.cursize == -1 )
    {
        DemoStopPlayback();
        return;
    }
    if( msg.cursize > msg.maxsize )
    {
        Com_Error( ERR_DROP, "SV_DemoReadFrame: demo message too long" );
    }
    
    r = fileSystem->Read( msg.data, msg.cursize, sv.demoFile );
    if( r != msg.cursize )
    {
        Com_Printf( "Demo file was truncated.\n" );
        DemoStopPlayback();
        return;
    }
    
    // Check slots, time and map
    savedMaxClients = sv_maxclients->integer;
    savedDemoClients = sv_democlients->integer;
    r = MSG_ReadLong( &msg );
    if( r < 400 )
    {
        Com_Printf( "Demo time too small: %d.\n", r );
        DemoStopPlayback();
        return;
    }
    s = MSG_ReadString( &msg );
    if( !fileSystem->FOpenFileRead( va( "maps/%s.bsp", s ), NULL, false ) )
    {
        Com_Printf( "Map does not exist: %s.\n", s );
        DemoStopPlayback();
        return;
    }
    clients = MSG_ReadBits( &msg, CLIENTNUM_BITS );
    if( sv_democlients->integer < clients )
    {
        S32 count = 0;
        // get the number of clients in use
        for( i = 0 ; i < sv_maxclients->integer ; i++ )
        {
            if( svs.clients[i].state >= CS_CONNECTED )
            {
                count++;
            }
        }
        if( clients + count > MAX_CLIENTS )
        {
            Com_Printf( "Not enough slots to fit all connected clients and all demo clients." \
                        "%d clients needs to disconnect.\n", clients + count - MAX_CLIENTS );
            DemoStopPlayback();
            return;
        }
        cvarSystem->SetValue( "sv_democlients", clients );
        cvarSystem->SetValue( "sv_maxclients", clients + count );
    }
    if( !com_sv_running->integer || strcmp( sv_mapname->string, s ) ||
            !cvarSystem->VariableIntegerValue( "sv_cheats" ) || r < svs.time ||
            sv_maxclients->modified || sv_democlients->modified )
    {
        // Change to the right map and start the demo with a g_warmup second delay
        cmdBufferSystem->AddText( va( "devmap %s\ndelay %d %s\n", s, cvarSystem->VariableIntegerValue( "g_warmup" ) * 1000, cmdSystem->Cmd() ) );
        DemoStopPlayback();
        return;
    }
    
    // Initialize our stuff
    ::memset( sv.demoEntities, 0, sizeof( sv.demoEntities ) );
    ::memset( sv.demoPlayerStates, 0, sizeof( sv.demoPlayerStates ) );
    cvarSystem->SetValue( "sv_democlients", clients );
    DemoReadFrame();
    
    Com_Printf( "Playing demo %s.\n", sv.demoName );
    
    sv.demoState = DS_PLAYBACK;
    cvarSystem->SetValue( "sv_demoState", DS_PLAYBACK );
}

/*
====================
idServerDemoSystemLocal::DemoStopPlayback

Close the demo file and restart the map
====================
*/
void idServerDemoSystemLocal::DemoStopPlayback( void )
{
    fileSystem->FCloseFile( sv.demoFile );
    sv.demoState = DS_NONE;
    cvarSystem->SetValue( "sv_demoState", DS_NONE );
    Com_Printf( "Stopped playing demo %s.\n", sv.demoName );
    
    // restore maxclients and democlients
    cvarSystem->SetValue( "sv_maxclients", savedMaxClients );
    cvarSystem->SetValue( "sv_democlients", savedDemoClients );
    
    // demo hasn't actually started yet
    if( sv.demoState != DS_PLAYBACK )
    {
#ifdef DEDICATED
        cmdBufferSystem->AddText( "map_restart 0\n" );
#else
        cmdBufferSystem->AddText( "killserver\n" );
#endif
    }
}