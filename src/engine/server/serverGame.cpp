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
// File name:   serverGame.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: interface to the game dll
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.h>
#elif DEDICATED
#include <null/null_serverprecompiled.h>
#else
#include <framework/precompiled.h>
#endif

idSGame* sgame;
idSGame* ( *gameDllEntry )( gameImports_t* gimports );

static gameImports_t exports;

idServerGameSystemLocal serverGameSystemLocal;
idServerGameSystem* serverGameSystem = &serverGameSystemLocal;

/*
===============
idServerGameSystemLocal::idServerGameSystemLocal
===============
*/
idServerGameSystemLocal::idServerGameSystemLocal( void )
{
}

/*
===============
idServerGameSystemLocal::~idServerGameSystemLocal
===============
*/
idServerGameSystemLocal::~idServerGameSystemLocal( void )
{
}

/*
==================
idServerGameSystemLocal::GameError
==================
*/
void idServerGameSystemLocal::GameError( pointer string )
{
    Com_Error( ERR_DROP, "%s", string );
}

/*
==================
idServerGameSystemLocal::GamePrint
==================
*/
void idServerGameSystemLocal::GamePrint( pointer string )
{
    Com_Printf( "%s", string );
}

// these functions must be used instead of pointer arithmetic, because
// the game allocates gentities with private information after the server shared part
/*
==================
idServerGameSystemLocal::NumForGentity
==================
*/
sint idServerGameSystemLocal::NumForGentity( sharedEntity_t* ent )
{
    sint num;
    
    num = ( ( uchar8* ) ent - ( uchar8* ) sv.gentities ) / sv.gentitySize;
    
    return num;
}

/*
==================
idServerGameSystemLocal::GentityNum
==================
*/
sharedEntity_t* idServerGameSystemLocal::GentityNum( sint num )
{
    sharedEntity_t* ent;
    
    ent = ( sharedEntity_t* )( ( uchar8* ) sv.gentities + sv.gentitySize * ( num ) );
    
    return ent;
}

/*
==================
idServerGameSystemLocal::GentityNum
==================
*/
playerState_t* idServerGameSystemLocal::GameClientNum( sint num )
{
    playerState_t*  ps;
    
    ps = ( playerState_t* )( ( uchar8* ) sv.gameClients + sv.gameClientSize * ( num ) );
    
    return ps;
}

/*
==================
idServerGameSystemLocal::SvEntityForGentity
==================
*/
svEntity_t* idServerGameSystemLocal::SvEntityForGentity( sharedEntity_t* gEnt )
{
    if( !gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES )
    {
        Com_Error( ERR_DROP, "idServerGameSystemLocal::SvEntityForGentity: bad gEnt" );
    }
    
    return &sv.svEntities[gEnt->s.number];
}

/*
==================
idServerGameSystemLocal::GEntityForSvEntity
==================
*/
sharedEntity_t* idServerGameSystemLocal::GEntityForSvEntity( svEntity_t* svEnt )
{
    sint num;
    
    num = svEnt - sv.svEntities;
    return serverGameSystemLocal.GentityNum( num );
}

/*
===============
idServerGameSystemLocal::GameSendServerCommand

Sends a command string to a client
===============
*/
void idServerGameSystemLocal::GameSendServerCommand( sint clientNum, pointer text )
{
    if( clientNum == -1 )
    {
        serverMainSystem->SendServerCommand( nullptr, "%s", text );
    }
    else if( clientNum == -2 )
    {
        sint j;
        client_t* client;
        
        for( j = 0, client = svs.clients; j < sv_maxclients->integer; j++, client++ )
        {
            if( client->state < CS_PRIMED )
            {
                continue;
            }
            
            if( client->netchan.remoteAddress.type == NA_LOOPBACK || client->netchan.remoteAddress.type == NA_BOT )
            {
                continue;
            }
            
            serverMainSystem->AddServerCommand( client, ( valueType* )text );
        }
    }
    else
    {
        if( clientNum < 0 || clientNum >= sv_maxclients->integer )
        {
            return;
        }
        serverMainSystem->SendServerCommand( svs.clients + clientNum, "%s", text );
    }
}


/*
===============
idServerGameSystemLocal::GameDropClient

Disconnects the client with a message
===============
*/
void idServerGameSystemLocal::GameDropClient( sint clientNum, pointer reason, sint length )
{
    if( clientNum < 0 || clientNum >= sv_maxclients->integer )
    {
        return;
    }
    
    serverClientSystem->DropClient( svs.clients + clientNum, reason );
    
    if( length )
    {
        serverCcmdsLocal.TempBanNetAddress( svs.clients[clientNum].netchan.remoteAddress, length );
    }
}

/*
=================
idServerGameSystemLocal::SetBrushModel

sets mins and maxs for inline bmodels
=================
*/
void idServerGameSystemLocal::SetBrushModel( sharedEntity_t* ent, pointer name )
{
    clipHandle_t h;
    vec3_t mins, maxs;
    
    if( !name )
    {
        Com_Error( ERR_DROP, "idServerGameSystemLocal::SetBrushModel: nullptr" );
    }
    
    if( name[0] != '*' )
    {
        Com_Error( ERR_DROP, "idServerGameSystemLocal::SetBrushModel: %s isn't a brush model", name );
    }
    
    ent->s.modelindex = atoi( name + 1 );
    
    h = collisionModelManager->InlineModel( ent->s.modelindex );
    collisionModelManager->ModelBounds( h, mins, maxs );
    VectorCopy( mins, ent->r.mins );
    VectorCopy( maxs, ent->r.maxs );
    ent->r.bmodel = true;
    
    ent->r.contents = -1;		// we don't know exactly what is in the brushes
    
    //LinkEntity( ent );			// FIXME: remove
}

/*
=================
idServerGameSystemLocal::inPVS

Also checks portalareas so that doors block sight
=================
*/
bool idServerGameSystemLocal::inPVS( const vec3_t p1, const vec3_t p2 )
{
    sint leafnum, cluster, area1, area2;
    uchar8* mask;
    
    leafnum = collisionModelManager->PointLeafnum( p1 );
    cluster = collisionModelManager->LeafCluster( leafnum );
    area1 = collisionModelManager->LeafArea( leafnum );
    mask = collisionModelManager->ClusterPVS( cluster );
    
    leafnum = collisionModelManager->PointLeafnum( p2 );
    cluster = collisionModelManager->LeafCluster( leafnum );
    area2 = collisionModelManager->LeafArea( leafnum );
    
    if( mask && ( !( mask[cluster >> 3] & ( 1 << ( cluster & 7 ) ) ) ) )
    {
        return false;
    }
    
    if( !collisionModelManager->AreasConnected( area1, area2 ) )
    {
        return false; // a door blocks sight
    }
    
    return true;
}

/*
=================
idServerGameSystemLocal::inPVSIgnorePortals

Does NOT check portalareas
=================
*/
bool idServerGameSystemLocal::inPVSIgnorePortals( const vec3_t p1, const vec3_t p2 )
{
    sint leafnum, cluster, area1, area2;
    uchar8* mask;
    
    leafnum = collisionModelManager->PointLeafnum( p1 );
    cluster = collisionModelManager->LeafCluster( leafnum );
    area1 = collisionModelManager->LeafArea( leafnum );
    mask = collisionModelManager->ClusterPVS( cluster );
    
    leafnum = collisionModelManager->PointLeafnum( p2 );
    cluster = collisionModelManager->LeafCluster( leafnum );
    area2 = collisionModelManager->LeafArea( leafnum );
    
    if( mask && ( !( mask[cluster >> 3] & ( 1 << ( cluster & 7 ) ) ) ) )
    {
        return false;
    }
    
    return true;
}

/*
========================
idServerGameSystemLocal::AdjustAreaPortalState
========================
*/
void idServerGameSystemLocal::AdjustAreaPortalState( sharedEntity_t* ent, bool open )
{
    svEntity_t* svEnt;
    
    svEnt = serverGameSystemLocal.SvEntityForGentity( ent );
    
    if( svEnt->areanum2 == -1 )
    {
        return;
    }
    
    collisionModelManager->AdjustAreaPortalState( svEnt->areanum, svEnt->areanum2, open );
}

/*
==================
idServerGameSystemLocal::GameAreaEntities
==================
*/
bool idServerGameSystemLocal::EntityContact( const vec3_t mins, const vec3_t maxs, const sharedEntity_t* gEnt, traceType_t type )
{
    const float32* origin, *angles;
    clipHandle_t ch;
    trace_t trace;
    
    // check for exact collision
    origin = gEnt->r.currentOrigin;
    angles = gEnt->r.currentAngles;
    
    ch = serverWorldSystemLocal.ClipHandleForEntity( gEnt );
    collisionModelManager->TransformedBoxTrace( &trace, vec3_origin, vec3_origin, mins, maxs, ch, -1, origin, angles, type );
    
    return trace.startsolid;
}

/*
===============
idServerGameSystemLocal::GetServerinfo
===============
*/
void idServerGameSystemLocal::GetServerinfo( valueType* buffer, sint bufferSize )
{
    if( bufferSize < 1 )
    {
        Com_Error( ERR_DROP, "idServerGameSystemLocal::GetServerinfo: bufferSize == %i", bufferSize );
    }
    
    Q_strncpyz( buffer, cvarSystem->InfoString( CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE ), bufferSize );
}

/*
===============
idServerGameSystemLocal::LocateGameData
===============
*/
void idServerGameSystemLocal::LocateGameData( sharedEntity_t* gEnts, sint numGEntities, sint sizeofGEntity_t, playerState_t* clients, sint sizeofGameClient )
{
    sv.gentities = gEnts;
    sv.gentitySize = sizeofGEntity_t;
    sv.num_entities = numGEntities;
    
    sv.gameClients = clients;
    sv.gameClientSize = sizeofGameClient;
}

/*
===============
idServerGameSystemLocal::GetUsercmd
===============
*/
void idServerGameSystemLocal::GetUsercmd( sint clientNum, usercmd_t* cmd )
{
    if( clientNum < 0 || clientNum >= sv_maxclients->integer )
    {
        Com_Error( ERR_DROP, "idServerGameSystemLocal::GetUsercmd: bad clientNum:%i", clientNum );
    }
    
    *cmd = svs.clients[clientNum].lastUsercmd;
    cmd->angles[ROLL] = 0;
}

/*
===============
idServerGameSystemLocal::UpdateSharedConfig
===============
*/
void idServerGameSystemLocal::UpdateSharedConfig( uint port, pointer rconpass )
{
    valueType message[MAX_RCON_MESSAGE];
    netadr_t to;
    
    message[0] = -1;
    message[1] = -1;
    message[2] = -1;
    message[3] = -1;
    message[4] = 0;
    
    Q_strcat( message, MAX_RCON_MESSAGE, "rcon " );
    
    Q_strcat( message, MAX_RCON_MESSAGE, rconpass );
    
    Q_strcat( message, MAX_RCON_MESSAGE, " !readconfig" );
    
    networkChainSystem->StringToAdr( "127.0.0.1", &to, NA_UNSPEC );
    to.port = BigShort( port );
    
    networkChainSystem->SendPacket( NS_SERVER, strlen( message ) + 1, message, to );
}

/*
===============
idServerGameSystemLocal::GetEntityToken
===============
*/
bool idServerGameSystemLocal::GetEntityToken( valueType* buffer, sint bufferSize )
{
    pointer s;
    
    s = COM_Parse( &sv.entityParsePoint );
    
    Q_strncpyz( buffer, s, bufferSize );
    
    if( !sv.entityParsePoint && !s[0] )
    {
        return false;
    }
    else
    {
        return true;
    }
}

/*
====================
idServerGameSystemLocal::DemoWriteCommand
====================
*/
sint idServerGameSystemLocal::DemoWriteCommand( sint cmd, pointer str )
{
    if( sv.demoState == DS_RECORDING )
    {
        if( cmd == -1 )
        {
            serverDemoSystem->DemoWriteServerCommand( str );
        }
        else
        {
            serverDemoSystem->DemoWriteGameCommand( cmd, str );
        }
    }
    
    return 0;
}

/*
====================
idServerGameSystemLocal::InitExportTable
====================
*/
void idServerGameSystemLocal::InitExportTable( void )
{
    exports.Printf = Com_Printf;
    exports.Error = Com_Error;
    exports.RealTime = Com_RealTime;
    
    exports.collisionModelManager = collisionModelManager;
#ifndef DEDICATED
    exports.soundSystem = soundSystem;
#endif
    exports.serverGameSystem = serverGameSystem;
    exports.serverWorldSystem = serverWorldSystem;
    exports.serverInitSystem = serverInitSystem;
    exports.serverMainSystem = serverMainSystem;
    exports.fileSystem = fileSystem;
    exports.cvarSystem = cvarSystem;
    exports.cmdBufferSystem = cmdBufferSystem;
    exports.cmdSystem = cmdSystem;
    exports.idsystem = idsystem;
    exports.serverCryptoSystem = serverCryptoSystem;
    exports.parseSystem = ParseSystem;
}

/*
===============
idServerGameSystemLocal::ShutdownGameProgs

Called every time a map changes
===============
*/
void idServerGameSystemLocal::ShutdownGameProgs( void )
{
    if( !svs.gameStarted )
    {
        return;
    }
    
    if( !gvm || sgame == nullptr )
    {
        return;
    }
    sgame->Shutdown( false );
    sgame = nullptr;
    
    idsystem->UnloadDll( gvm );
    gvm = nullptr;
    if( sv_newGameShlib->string[0] )
    {
        fileSystem->Rename( sv_newGameShlib->string, "sgameAMD64" DLL_EXT );
        cvarSystem->Set( "sv_newGameShlib", "" );
    }
}

/*
==================
idServerGameSystemLocal::InitGameModule

Called for both a full init and a restart
==================
*/
void idServerGameSystemLocal::InitGameModule( bool restart )
{
    sint i;
    
    // clear physics interaction links
    serverWorldSystemLocal.ClearWorld();
    
    // start the entity parsing at the beginning
    sv.entityParsePoint = collisionModelManager->EntityString();
    
    // clear all gentity pointers that might still be set from
    // a previous level
    for( i = 0; i < sv_maxclients->integer; i++ )
    {
        svs.clients[i].gentity = nullptr;
    }
    
    // use the current msec count for a random seed
    // init for this gamestate
    sgame->Init( sv.time, Com_Milliseconds(), restart );
    
}

/*
===================
idServerGameSystemLocal::RestartGameProgs

Called on a map_restart, but not on a normal map change
===================
*/
void idServerGameSystemLocal::RestartGameProgs( void )
{
    InitExportTable();
    
    if( !gvm )
    {
        svs.gameStarted = false;
        Com_Error( ERR_DROP, "idServerGameSystemLocal::RestartGameProgs on game failed" );
        return;
    }
    
    sgame->Shutdown( true );
    
    InitGameModule( true );
}


/*
===============
idServerGameSystemLocal::InitGameProgs

Called on a normal map change, not on a map_restart
===============
*/
void idServerGameSystemLocal::InitGameProgs( void )
{
    sv.num_tagheaders = 0;
    sv.num_tags = 0;
    
    convar_t* var = cvarSystem->Get( "bot_enable", "1", CVAR_LATCH, "Whether or not the server allows bots." );
    //bot_enable = var ? var->integer : 0;
    
    // load the dll or bytecode
    gvm = idsystem->LoadDll( "sgame" );
    if( !gvm )
    {
        Com_Error( ERR_FATAL, "idServerGameSystemLocal::InitGameProgs on game failed" );
    }
    
    // Get the entry point.
    gameDllEntry = ( idSGame * ( QDECL* )( gameImports_t* ) )idsystem->GetProcAddress( gvm, "dllEntry" );
    if( !gameDllEntry )
    {
        Com_Error( ERR_FATAL, "gameDllEntry on game failed.\n" );
    }
    
    svs.gameStarted = true;
    
    // Init the export table.
    InitExportTable();
    
    sgame = gameDllEntry( &exports );
    
    InitGameModule( false );
}

/*
====================
idServerGameSystemLocal::GameCommand

See if the current console command is claimed by the game
====================
*/
bool idServerGameSystemLocal::GameCommand( void )
{
    if( sv.state != SS_GAME )
    {
        return false;
    }
    
    return sgame->ConsoleCommand();
}

/*
====================
idServerGameSystemLocal::GameIsSinglePlayer
====================
*/
bool idServerGameSystemLocal::GameIsSinglePlayer( void )
{
    return ( bool )( com_gameInfo.spGameTypes & ( 1 << g_gameType->integer ) );
}

/*
====================
idServerGameSystemLocal::GameIsCoop

This is a modified SinglePlayer, no savegame capability for example
====================
*/
bool idServerGameSystemLocal::GameIsCoop( void )
{
    return ( bool )( com_gameInfo.coopGameTypes & ( 1 << g_gameType->integer ) );
}

/*
====================
idServerGameSystemLocal::GetTag

return false if unable to retrieve tag information for this client
Dushan - I have no idea if this ever worked in Wolfenstein: Enemy Territory
====================
*/
bool idServerGameSystemLocal::GetTag( sint clientNum, sint tagFileNumber, valueType* tagname, orientation_t* _or )
{
    sint i;
    
    if( tagFileNumber > 0 && tagFileNumber <= sv.num_tagheaders )
    {
        for( i = sv.tagHeadersExt[tagFileNumber - 1].start; i < sv.tagHeadersExt[tagFileNumber - 1].start + sv.tagHeadersExt[tagFileNumber - 1].count; i++ )
        {
            if( !Q_stricmp( sv.tags[i].name, tagname ) )
            {
                VectorCopy( sv.tags[i].origin, _or->origin );
                VectorCopy( sv.tags[i].axis[0], _or->axis[0] );
                VectorCopy( sv.tags[i].axis[1], _or->axis[1] );
                VectorCopy( sv.tags[i].axis[2], _or->axis[2] );
                return true;
            }
        }
    }
    
    // Gordon: lets try and remove the inconsitancy between ded/non-ded servers...
    // Gordon: bleh, some code in clientthink_real really relies on this working on player models...
#ifndef DEDICATED				// TTimo: dedicated only binary defines DEDICATED
    if( com_dedicated->integer )
    {
        return false;
    }
    
    return clientGameSystem->GetTag( clientNum, tagname, _or );
#else
    return false;
#endif
}
