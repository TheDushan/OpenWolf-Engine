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
// File name:   serverCcmds.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
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

idServerCcmdsSystemLocal serverCcmdsLocal;

/*
===============
idServerCcmdsSystemLocal::idServerCcmdsSystemLocal
===============
*/
idServerCcmdsSystemLocal::idServerCcmdsSystemLocal( void )
{
}

/*
===============
idServerCcmdsSystemLocal::~idServerCcmdsSystemLocal
===============
*/
idServerCcmdsSystemLocal::~idServerCcmdsSystemLocal( void )
{
}

/*
===============================================================================
OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/

/*
==================
idServerCcmdsSystemLocal::GetPlayerByName

Returns the player with name from cmdSystem->Argv(1)
==================
*/
client_t* idServerCcmdsSystemLocal::GetPlayerByName( void )
{
    sint i;
    valueType* s, cleanName[64];
    client_t* cl;
    
    // make sure server is running
    if( !com_sv_running->integer )
    {
        return nullptr;
    }
    
    if( cmdSystem->Argc() < 2 )
    {
        Com_Printf( "idServerCcmdsSystemLocal::GetPlayerByName - No player specified.\n" );
        return nullptr;
    }
    
    s = cmdSystem->Argv( 1 );
    
    // check for a name match
    for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
    {
        if( cl->state <= CS_ZOMBIE )
        {
            continue;
        }
        
        if( !Q_stricmp( cl->name, s ) )
        {
            return cl;
        }
        
        Q_strncpyz( cleanName, cl->name, sizeof( cleanName ) );
        Q_CleanStr( cleanName );
        
        if( !Q_stricmp( cleanName, s ) )
        {
            return cl;
        }
    }
    
    Com_Printf( "idServerCcmdsSystemLocal::GetPlayerByName - Player %s is not on the server\n", s );
    
    return nullptr;
}

/*
==================
idServerCcmdsSystemLocal::Map_f

Restart the server on a different map
==================
*/
void idServerCcmdsSystemLocal::Map_f( void )
{
    sint savegameTime = -1;
    valueType* cmd, *map, smapname[MAX_QPATH], mapname[MAX_QPATH], expanded[MAX_QPATH], *cl_profileStr = cvarSystem->VariableString( "cl_profile" );
    bool killBots, cheat, buildScript;
    
    map = cmdSystem->Argv( 1 );
    if( !map )
    {
        return;
    }
    
    if( !com_gameInfo.spEnabled )
    {
        if( !Q_stricmp( cmdSystem->Argv( 0 ), "spdevmap" ) || !Q_stricmp( cmdSystem->Argv( 0 ), "spmap" ) )
        {
            Com_Printf( "Single Player is not enabled.\n" );
            return;
        }
    }
    
    buildScript = ( bool )cvarSystem->VariableIntegerValue( "com_buildScript" );
    
    if( serverGameSystem->GameIsSinglePlayer() )
    {
        if( !buildScript && sv_reloading->integer && sv_reloading->integer != RELOAD_NEXTMAP ) // game is in 'reload' mode, don't allow starting new maps yet.
        {
            return;
        }
        
        // Trap a savegame load
        if( strstr( map, ".sav" ) )
        {
            // open the savegame, read the mapname, and copy it to the map string
            valueType savemap[MAX_QPATH], savedir[MAX_QPATH];
            uchar8* buffer;
            sint size, csize;
            
            if( com_gameInfo.usesProfiles && cl_profileStr[0] )
            {
                Com_sprintf( savedir, sizeof( savedir ), "profiles/%s/save/", cl_profileStr );
            }
            else
            {
                Q_strncpyz( savedir, "save/", sizeof( savedir ) );
            }
            
            if( !( strstr( map, savedir ) == map ) )
            {
                Com_sprintf( savemap, sizeof( savemap ), "%s%s", savedir, map );
            }
            else
            {
                strcpy( savemap, map );
            }
            
            size = fileSystem->ReadFile( savemap, nullptr );
            if( size < 0 )
            {
                Com_Printf( "Can't find savegame %s\n", savemap );
                return;
            }
            
            //buffer = Hunk_AllocateTempMemory(size);
            fileSystem->ReadFile( savemap, ( void** )&buffer );
            
            if( Q_stricmp( savemap, va( "%scurrent.sav", savedir ) ) != 0 )
            {
                // copy it to the current savegame file
                fileSystem->WriteFile( va( "%scurrent.sav", savedir ), buffer, size );
                // make sure it is the correct size
                csize = fileSystem->ReadFile( va( "%scurrent.sav", savedir ), nullptr );
                if( csize != size )
                {
                    Hunk_FreeTempMemory( buffer );
                    fileSystem->Delete( va( "%scurrent.sav", savedir ) );
// TTimo
#ifdef __linux__
                    Com_Error( ERR_DROP, "Unable to save game.\n\nPlease check that you have at least 5mb free of disk space in your home directory." );
#else
                    Com_Error( ERR_DROP, "Insufficient free disk space.\n\nPlease free at least 5mb of free space on game drive." );
#endif
                    return;
                }
            }
            
            // set the cvar, so the game knows it needs to load the savegame once the clients have connected
            cvarSystem->Set( "savegame_loading", "1" );
            
            // set the filename
            cvarSystem->Set( "savegame_filename", savemap );
            
            // the mapname is at the very start of the savegame file
            Q_strncpyz( savemap, ( valueType* )( buffer + sizeof( sint ) ), sizeof( savemap ) );	// skip the version
            Q_strncpyz( smapname, savemap, sizeof( smapname ) );
            map = smapname;
            
            savegameTime = *( sint* )( buffer + sizeof( sint ) + MAX_QPATH );
            
            if( savegameTime >= 0 )
            {
                svs.time = savegameTime;
            }
            
            Hunk_FreeTempMemory( buffer );
        }
        else
        {
            cvarSystem->Set( "savegame_loading", "0" );	// make sure it's turned off
            
            // set the filename
            cvarSystem->Set( "savegame_filename", "" );
        }
    }
    else
    {
        cvarSystem->Set( "savegame_loading", "0" );	// make sure it's turned off
        
        // set the filename
        cvarSystem->Set( "savegame_filename", "" );
    }
    
    // We need to copy cmdSystem->Argv(1) before the serverInitSystem->SpawnServer() call below.
    Q_strncpyz( mapname, map, sizeof( mapname ) );
    
    // make sure the level exists before trying to change, so that
    // a typo at the server console won't end the game
    if( strchr( map, '\\' ) )
    {
        Com_Printf( "Can't have mapnames with a \\\n" );
        return;
    }
    
    Com_sprintf( expanded, sizeof( expanded ), "maps/%s.bsp", map );
    
    if( fileSystem->ReadFile( expanded, nullptr ) == -1 )
    {
        Com_Printf( "Can't find map %s\n", expanded );
        return;
    }
    
    cvarSystem->Set( "gamestate", va( "%i", GS_INITIALIZE ) ); // NERVE - SMF - reset gamestate on map/devmap
    
    cvarSystem->Set( "g_currentRound", "0" ); // NERVE - SMF - reset the current round
    cvarSystem->Set( "g_nextTimeLimit", "0" ); // NERVE - SMF - reset the next time limit
    
    // START Mad Doctor I changes, 8/14/2002.  Need a way to force load a single player map as single player
    if( !Q_stricmp( cmdSystem->Argv( 0 ), "spdevmap" ) || !Q_stricmp( cmdSystem->Argv( 0 ), "spmap" ) )
    {
        // This is explicitly asking for a single player load of this map
        cvarSystem->Set( "g_gametype", va( "%i", com_gameInfo.defaultSPGameType ) );
        
        // force latched values to get set
        cvarSystem->Get( "g_gametype", va( "%i", com_gameInfo.defaultSPGameType ), CVAR_SERVERINFO | CVAR_USERINFO | CVAR_LATCH, "Sets the type of game being played, 2=objective, 3=stopwatch, 4=campaign, 5=LMS" );
        
        // enable bot support for AI
        cvarSystem->Set( "bot_enable", "1" );
    }
    
    cmd = cmdSystem->Argv( 0 );
    
    if( !Q_stricmp( cmd, "devmap" ) )
    {
        cheat = true;
        killBots = true;
    }
    else if( !Q_stricmp( cmdSystem->Argv( 0 ), "spdevmap" ) )
    {
        cheat = true;
        killBots = true;
    }
    else
    {
        cheat = false;
        killBots = false;
    }
    
    // stop any demos
    if( sv.demoState == DS_RECORDING )
    {
        serverDemoSystem->DemoStopRecord();
    }
    
    if( sv.demoState == DS_PLAYBACK )
    {
        serverDemoSystem->DemoStopPlayback();
    }
    
    // start up the map
    serverInitSystem->SpawnServer( mapname, killBots );
    
    // set the cheat value
    // if the level was started with "map <levelname>", then
    // cheats will not be allowed.  If started with "devmap <levelname>"
    // then cheats will be allowed
    if( cheat )
    {
        cvarSystem->Set( "sv_cheats", "1" );
    }
    else
    {
        cvarSystem->Set( "sv_cheats", "0" );
    }
}

/*
================
idServerCcmdsSystemLocal::CheckTransitionGameState

NERVE - SMF
================
*/
bool idServerCcmdsSystemLocal::CheckTransitionGameState( gamestate_t new_gs, gamestate_t old_gs )
{
    if( old_gs == new_gs && new_gs != GS_PLAYING )
    {
        return false;
    }
    
//  if ( old_gs == GS_WARMUP && new_gs != GS_WARMUP_COUNTDOWN )
//      return false;

//  if ( old_gs == GS_WARMUP_COUNTDOWN && new_gs != GS_PLAYING )
//      return false;

    if( old_gs == GS_WAITING_FOR_PLAYERS && new_gs != GS_WARMUP )
    {
        return false;
    }
    
    if( old_gs == GS_INTERMISSION && new_gs != GS_WARMUP )
    {
        return false;
    }
    
    if( old_gs == GS_RESET && ( new_gs != GS_WAITING_FOR_PLAYERS && new_gs != GS_WARMUP ) )
    {
        return false;
    }
    
    return true;
}

/*
================
idServerCcmdsSystemLocal::TransitionGameState

NERVE - SMF
================
*/
bool idServerCcmdsSystemLocal::TransitionGameState( gamestate_t new_gs, gamestate_t old_gs, sint delay )
{
    if( !serverGameSystem->GameIsSinglePlayer() && !serverGameSystem->GameIsCoop() )
    {
        // we always do a warmup before starting match
        if( old_gs == GS_INTERMISSION && new_gs == GS_PLAYING )
        {
            new_gs = GS_WARMUP;
        }
    }
    
    // check if its a valid state transition
    if( !CheckTransitionGameState( new_gs, old_gs ) )
    {
        return false;
    }
    
    if( new_gs == GS_RESET )
    {
        new_gs = GS_WARMUP;
    }
    
    cvarSystem->Set( "gamestate", va( "%i", new_gs ) );
    
    return true;
}

void MSG_PrioritiseEntitystateFields( void );
void MSG_PrioritisePlayerStateFields( void );

/*
================
idServerCcmdsSystemLocal::FieldInfo_f
================
*/
void idServerCcmdsSystemLocal::FieldInfo_f( void )
{
    MSG_PrioritiseEntitystateFields();
    MSG_PrioritisePlayerStateFields();
}

/*
================
idServerCcmdsSystemLocal::MapRestart_f

Completely restarts a level, but doesn't send a new gamestate to the clients.
This allows fair starts with variable load times.
================
*/
void idServerCcmdsSystemLocal::MapRestart_f( void )
{
    sint i, delay = 0;
    client_t* client;
    valueType* denied;
    bool isBot;
    gamestate_t new_gs, old_gs; // NERVE - SMF
    
    // make sure we aren't restarting twice in the same frame
    if( com_frameTime == sv.serverId )
    {
        return;
    }
    
    // make sure server is running
    if( !com_sv_running->integer )
    {
        Com_Printf( "Server is not running.\n" );
        return;
    }
    
    if( cmdSystem->Argc() > 1 )
    {
        delay = atoi( cmdSystem->Argv( 1 ) );
    }
    
    if( delay && !cvarSystem->VariableValue( "g_doWarmup" ) )
    {
        sv.restartTime = sv.time + delay * 1000;
        serverInitSystem->SetConfigstring( CS_WARMUP, va( "%i", sv.restartTime ) );
        return;
    }
    
    // NERVE - SMF - read in gamestate or just default to GS_PLAYING
    old_gs = ( gamestate_t )atoi( cvarSystem->VariableString( "gamestate" ) );
    
    if( serverGameSystem->GameIsSinglePlayer() || serverGameSystem->GameIsCoop() )
    {
        new_gs = GS_PLAYING;
    }
    else
    {
        if( cmdSystem->Argc() > 2 )
        {
            new_gs = ( gamestate_t )atoi( cmdSystem->Argv( 2 ) );
        }
        else
        {
            new_gs = GS_PLAYING;
        }
    }
    
    if( !TransitionGameState( new_gs, old_gs, delay ) )
    {
        return;
    }
    
    // check for changes in variables that can't just be restarted
    // check for maxclients change
    if( sv_maxclients->modified )
    {
        valueType mapname[MAX_QPATH];
        
        Com_Printf( "sv_maxclients variable change -- restarting.\n" );
        // restart the map the slow way
        Q_strncpyz( mapname, cvarSystem->VariableString( "mapname" ), sizeof( mapname ) );
        
        serverInitSystem->SpawnServer( mapname, false );
        return;
    }
    
    // Check for loading a saved game
    if( cvarSystem->VariableIntegerValue( "savegame_loading" ) )
    {
        // open the current savegame, and find out what the time is, everything else we can ignore
        valueType savemap[MAX_QPATH], *cl_profileStr = cvarSystem->VariableString( "cl_profile" );
        uchar8* buffer;
        sint size, savegameTime;
        
        if( com_gameInfo.usesProfiles )
        {
            Com_sprintf( savemap, sizeof( savemap ), "profiles/%s/save/current.sav", cl_profileStr );
        }
        else
        {
            Q_strncpyz( savemap, "save/current.sav", sizeof( savemap ) );
        }
        
        size = fileSystem->ReadFile( savemap, nullptr );
        if( size < 0 )
        {
            Com_Printf( "Can't find savegame %s\n", savemap );
            return;
        }
        
        //buffer = Hunk_AllocateTempMemory(size);
        fileSystem->ReadFile( savemap, ( void** )&buffer );
        
        // the mapname is at the very start of the savegame file
        savegameTime = *( sint* )( buffer + sizeof( sint ) + MAX_QPATH );
        
        if( savegameTime >= 0 )
        {
            svs.time = savegameTime;
        }
        
        Hunk_FreeTempMemory( buffer );
    }
    // done.
    
    // stop any demos
    if( sv.demoState == DS_RECORDING )
    {
        serverDemoSystem->DemoStopRecord();
    }
    
    if( sv.demoState == DS_PLAYBACK )
    {
        serverDemoSystem->DemoStopPlayback();
    }
    
    // toggle the server bit so clients can detect that a
    // map_restart has happened
    svs.snapFlagServerBit ^= SNAPFLAG_SERVERCOUNT;
    
    // generate a new serverid
    sv.restartedServerId = sv.serverId;
    sv.serverId = com_frameTime;
    cvarSystem->Set( "sv_serverid", va( "%i", sv.serverId ) );
    
    // if a map_restart occurs while a client is changing maps, we need
    // to give them the correct time so that when they finish loading
    // they don't violate the backwards time check in cl_cgame.cpp
    for( i = 0; i < sv_maxclients->integer; i++ )
    {
        if( svs.clients[i].state == CS_PRIMED )
        {
            svs.clients[i].oldServerTime = sv.restartTime;
        }
    }
    
    // reset all the vm data in place without changing memory allocation
    // note that we do NOT set sv.state = SS_LOADING, so configstrings that
    // had been changed from their default values will generate broadcast updates
    sv.state = SS_LOADING;
    sv.restarting = true;
    
    cvarSystem->Set( "sv_serverRestarting", "1" );
    
    serverGameSystem->RestartGameProgs();
    
    // run a few frames to allow everything to settle
    for( i = 0; i < GAME_INIT_FRAMES; i++ )
    {
        sgame->RunFrame( sv.time );
        sv.time += 100;
        svs.time += FRAMETIME;
    }
    
    // create a baseline for more efficient communications
    // Gordon: meh, this wont work here as the client doesn't know it has happened
    // CreateBaseline ();
    
    sv.state = SS_GAME;
    sv.restarting = false;
    
    // connect and begin all the clients
    for( i = 0; i < sv_maxclients->integer; i++ )
    {
        client = &svs.clients[i];
        
        // send the new gamestate to all connected clients
        if( client->state < CS_CONNECTED )
        {
            continue;
        }
        
        if( client->netchan.remoteAddress.type == NA_BOT )
        {
            if( serverGameSystem->GameIsSinglePlayer() || serverGameSystem->GameIsCoop() )
            {
                continue;		// dont carry across bots in single player
            }
            isBot = true;
        }
        else
        {
            isBot = false;
        }
        
        // add the map_restart command
        serverMainSystem->AddServerCommand( client, "map_restart\n" );
        
        // connect the client again, without the firstTime flag
        denied = ( valueType* )sgame->ClientConnect( i, false );
        if( denied )
        {
            // this generally shouldn't happen, because the client
            // was connected before the level change
            serverClientSystem->DropClient( client, denied );
            
            if( ( !serverGameSystem->GameIsSinglePlayer() ) || ( !isBot ) )
            {
                Com_Printf( "idServerBotSystemLocal::MapRestart_f(%d): dropped client %i - denied!\n", delay, i );	// bk010125
            }
            
            continue;
        }
        
        if( client->state == CS_ACTIVE )
        {
            serverClientSystem->ClientEnterWorld( client, &client->lastUsercmd );
        }
        else
        {
            // If we don't reset client->lastUsercmd and are restarting during map load,
            // the client will hang because we'll use the last Usercmd from the previous map,
            // which is wrong obviously.
            serverClientSystem->ClientEnterWorld( client, nullptr );
        }
    }
    
    // run another frame to allow things to look at all the players
    sgame->RunFrame( sv.time );
    sv.time += 100;
    svs.time += FRAMETIME;
    
    cvarSystem->Set( "sv_serverRestarting", "0" );
}

/*
=================
idServerCcmdsSystemLocal::LoadGame_f
=================
*/
void idServerCcmdsSystemLocal::LoadGame_f( void )
{
    sint size;
    valueType filename[MAX_QPATH], mapname[MAX_QPATH], savedir[MAX_QPATH], *cl_profileStr = cvarSystem->VariableString( "cl_profile" );
    uchar8* buffer;
    
    // dont allow command if another loadgame is pending
    if( cvarSystem->VariableIntegerValue( "savegame_loading" ) )
    {
        return;
    }
    if( sv_reloading->integer )
    {
        // (SA) disabling
        // if(sv_reloading->integer && sv_reloading->integer != RELOAD_FAILED )    // game is in 'reload' mode, don't allow starting new maps yet.
        return;
    }
    
    Q_strncpyz( filename, cmdSystem->Argv( 1 ), sizeof( filename ) );
    if( !filename[0] )
    {
        Com_Printf( "You must specify a savegame to load\n" );
        return;
    }
    
    if( com_gameInfo.usesProfiles && cl_profileStr[0] )
    {
        Com_sprintf( savedir, sizeof( savedir ), "profiles/%s/save/", cl_profileStr );
    }
    else
    {
        Q_strncpyz( savedir, "save/", sizeof( savedir ) );
    }
    
    //if ( Q_strncmp( filename, "save/", 5 ) && Q_strncmp( filename, "save\\", 5 ) )
    //{
    //	Q_strncpyz( filename, va("save/%s", filename), sizeof( filename ) );
    //}
    
    // go through a va to avoid vsnprintf call with same source and target
    Q_strncpyz( filename, va( "%s%s", savedir, filename ), sizeof( filename ) );
    
    // enforce .sav extension
    if( !strstr( filename, "." ) || Q_strncmp( strstr( filename, "." ) + 1, "sav", 3 ) )
    {
        Q_strcat( filename, sizeof( filename ), ".sav" );
    }
    
    // use '/' instead of '\\' for directories
    while( strstr( filename, "\\" ) )
    {
        *( valueType* )strstr( filename, "\\" ) = '/';
    }
    
    size = fileSystem->ReadFile( filename, nullptr );
    if( size < 0 )
    {
        Com_Printf( "Can't find savegame %s\n", filename );
        return;
    }
    
    //buffer = Hunk_AllocateTempMemory(size);
    fileSystem->ReadFile( filename, ( void** )&buffer );
    
    // read the mapname, if it is the same as the current map, then do a fast load
    Q_strncpyz( mapname, ( pointer )( buffer + sizeof( sint ) ), sizeof( mapname ) );
    
    if( com_sv_running->integer && ( com_frameTime != sv.serverId ) )
    {
        // check mapname
        if( !Q_stricmp( mapname, sv_mapname->string ) ) // same
        {
            if( Q_stricmp( filename, va( "%scurrent.sav", savedir ) ) != 0 )
            {
                // copy it to the current savegame file
                fileSystem->WriteFile( va( "%scurrent.sav", savedir ), buffer, size );
            }
            
            Hunk_FreeTempMemory( buffer );
            
            cvarSystem->Set( "savegame_loading", "2" );	// 2 means it's a restart, so stop rendering until we are loaded
            
            // set the filename
            cvarSystem->Set( "savegame_filename", filename );
            
            // quick-restart the server
            MapRestart_f();	// savegame will be loaded after restart
            
            return;
        }
    }
    
    Hunk_FreeTempMemory( buffer );
    
    // otherwise, do a slow load
    if( cvarSystem->VariableIntegerValue( "sv_cheats" ) )
    {
        cmdBufferSystem->ExecuteText( EXEC_APPEND, va( "spdevmap %s", filename ) );
    }
    else     // no cheats
    {
        cmdBufferSystem->ExecuteText( EXEC_APPEND, va( "spmap %s", filename ) );
    }
}

/*
==================
idServerCcmdsSystemLocal::TempBanNetAddress
==================
*/
void idServerCcmdsSystemLocal::TempBanNetAddress( netadr_t address, sint length )
{
    sint i, oldesttime = 0, oldest = -1;
    
    for( i = 0; i < MAX_TEMPBAN_ADDRESSES; i++ )
    {
        if( !svs.tempBanAddresses[i].endtime || svs.tempBanAddresses[i].endtime < svs.time )
        {
            // found a free slot
            svs.tempBanAddresses[i].adr = address;
            svs.tempBanAddresses[i].endtime = svs.time + ( length * 1000 );
            return;
        }
        else
        {
            if( oldest == -1 || oldesttime > svs.tempBanAddresses[i].endtime )
            {
                oldesttime = svs.tempBanAddresses[i].endtime;
                oldest = i;
            }
        }
    }
    
    svs.tempBanAddresses[oldest].adr = address;
    svs.tempBanAddresses[oldest].endtime = svs.time + length;
}

/*
==================
idServerCcmdsSystemLocal::TempBanIsBanned
==================
*/
bool idServerCcmdsSystemLocal::TempBanIsBanned( netadr_t address )
{
    sint i;
    
    for( i = 0; i < MAX_TEMPBAN_ADDRESSES; i++ )
    {
        if( svs.tempBanAddresses[i].endtime && svs.tempBanAddresses[i].endtime > svs.time )
        {
            if( networkSystem->CompareAdr( address, svs.tempBanAddresses[i].adr ) )
            {
                return true;
            }
        }
    }
    
    return false;
}

/*
================
idServerCcmdsSystemLocal::Status_f
================
*/
void idServerCcmdsSystemLocal::Status_f( void )
{
    sint i, j, l, ping;
    client_t* cl;
    playerState_t* ps;
    pointer s;
    uchar8 cpu, avg; //Dushan
    
    // make sure server is running
    if( !com_sv_running->integer )
    {
        Com_Printf( "Server is not running.\n" );
        return;
    }
    
    // Dushan
    cpu = ( uchar8 )( svs.stats.latched_active + svs.stats.latched_idle );
    
    if( cpu )
    {
        cpu = ( uchar8 )( 100 * svs.stats.latched_active / cpu );
    }
    
    avg = ( uchar8 )( 1000 * svs.stats.latched_active / STATFRAMES );
    
    Com_Printf( "cpu utilization  : %3i%%\n", ( sint )cpu );
    Com_Printf( "avg response time: %i ms\n", ( sint )avg );
    
    Com_Printf( "map: %s\n", sv_mapname->string );
    
    Com_Printf( "num score ping name            lastmsg address               qport rate\n" );
    Com_Printf( "--- ----- ---- --------------- ------- --------------------- ----- -----\n" );
    
    for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
    {
        if( !cl->state )
        {
            continue;
        }
        Com_Printf( "%3i ", i );
        
        ps = serverGameSystem->GameClientNum( i );
        
        Com_Printf( "%5i ", ps->persistant[PERS_SCORE] );
        
        if( cl->state == CS_CONNECTED )
        {
            Com_Printf( "CNCT " );
        }
        else if( cl->state == CS_ZOMBIE )
        {
            Com_Printf( "ZMBI " );
        }
        else
        {
            ping = cl->ping < 9999 ? cl->ping : 9999;
            Com_Printf( "%4i ", ping );
        }
        
        Com_Printf( "%s", cl->name );
        l = 16 - ( sint )::strlen( cl->name );
        for( j = 0; j < l; j++ )
        {
            Com_Printf( " " );
        }
        
        Com_Printf( "%7i ", svs.time - cl->lastPacketTime );
        
        s = networkSystem->AdrToString( cl->netchan.remoteAddress );
        Com_Printf( "%s", s );
        l = 22 - ( sint )::strlen( s );
        for( j = 0; j < l; j++ )
        {
            Com_Printf( " " );
        }
        
        Com_Printf( "%5i", cl->netchan.qport );
        
        Com_Printf( " %5i", cl->rate );
        
        Com_Printf( "\n" );
    }
    Com_Printf( "\n" );
}

/*
==================
idServerCcmdsSystemLocal::ConSay_f
==================
*/
void idServerCcmdsSystemLocal::ConSay_f( void )
{
    valueType* p;
    valueType text[1024];
    
    if( !com_dedicated->integer )
    {
        Com_Printf( "Server is not dedicated.\n" );
        return;
    }
    
    // make sure server is running
    if( !com_sv_running->integer )
    {
        Com_Printf( "Server is not running.\n" );
        return;
    }
    
    if( cmdSystem->Argc() < 2 )
    {
        return;
    }
    
    ::strcpy( text, "Server: " );
    p = cmdSystemLocal.Args();
    
    if( *p == '"' )
    {
        p++;
        p[strlen( p ) - 1] = 0;
    }
    
    strcat( text, p );
    
    serverMainSystem->SendServerCommand( nullptr, "chat \"%s\"", text );
}


/*
==================
idServerCcmdsSystemLocal::Heartbeat_f

Also called by idServerClientSystemLocal::DropClient, idServerClientSystemLocal::DirectConnect, and idServerInitSystemLocal::SpawnServer
==================
*/
void idServerCcmdsSystemLocal::Heartbeat_f( void )
{
    svs.nextHeartbeatTime = -9999999;
}

/*
===========
idServerCcmdsSystemLocal::Serverinfo_f

Examine the serverinfo string
===========
*/
void idServerCcmdsSystemLocal::Serverinfo_f( void )
{
    Com_Printf( "Server info settings:\n" );
    
    Info_Print( cvarSystem->InfoString( CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE ) );
    if( !com_sv_running->integer )
    {
        Com_Printf( "Server is not running.\n" );
    }
}

/*
===========
SV_Systeminfo_f

Examine or change the serverinfo string
===========
*/
void idServerCcmdsSystemLocal::Systeminfo_f( void )
{
    // make sure server is running
    if( !com_sv_running->integer )
    {
        Com_Printf( "Server is not running.\n" );
        return;
    }
    
    Com_Printf( "System info settings:\n" );
    
    Info_Print( cvarSystem->InfoString_Big( CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE ) );
}

/*
===========
idServerCcmdsSystemLocal::DumpUser_f

Examine all a users info strings FIXME: move to game
===========
*/
void idServerCcmdsSystemLocal::DumpUser_f( void )
{
    client_t* cl;
    
    // make sure server is running
    if( !com_sv_running->integer )
    {
        Com_Printf( "Server is not running.\n" );
        return;
    }
    
    if( cmdSystem->Argc() != 2 )
    {
        Com_Printf( "Usage: info <userid>\n" );
        return;
    }
    
    cl = GetPlayerByName();
    if( !cl )
    {
        return;
    }
    
    Com_Printf( "userinfo\n" );
    Com_Printf( "--------\n" );
    Info_Print( cl->userinfo );
}


/*
=================
idServerCcmdsSystemLocal::KillServer
=================
*/
void idServerCcmdsSystemLocal::KillServer_f( void )
{
    serverInitSystem->Shutdown( "killserver" );
}

/*
=================
idServerCcmdsSystemLocal::GameCompleteStatus_f

NERVE - SMF
=================
*/
void idServerCcmdsSystemLocal::GameCompleteStatus_f( void )
{
    serverMainSystem->MasterGameCompleteStatus();
}

//===========================================================

/*
==================
idServerCcmdsSystemLocal::CompleteMapName
==================
*/
void idServerCcmdsSystemLocal::CompleteMapName( valueType* args, sint argNum )
{
    if( argNum == 2 )
    {
        Field_CompleteFilename( "maps", "bsp", true );
    }
}

/*
=================
SV_StartRedirect_f

Redirect console output to a client
=================
*/
static client_t* redirect_client = nullptr;

void idServerCcmdsSystemLocal::ClientRedirect( valueType* outputbuf )
{
    serverMainSystem->SendServerCommand( redirect_client, "%s", outputbuf );
}

void idServerCcmdsSystemLocal::StartRedirect_f( void )
{
#define SV_OUTPUTBUF_LENGTH (1024 - 16)
    sint clientNum;
    static valueType sv_outputbuf[SV_OUTPUTBUF_LENGTH];
    
    clientNum = atoi( cmdSystem->Argv( 1 ) );
    
    if( clientNum < 0 || clientNum >= sv_maxclients->integer )
    {
        return;
    }
    
    redirect_client = svs.clients + clientNum;
    
    Com_EndRedirect();
    Com_BeginRedirect( sv_outputbuf, SV_OUTPUTBUF_LENGTH, ClientRedirect );
}


/*
=================
idServerCcmdsSystemLocal::Demo_Record_f
=================
*/
void idServerCcmdsSystemLocal::Demo_Record_f( void )
{
    // make sure server is running
    if( !com_sv_running->integer )
    {
        Com_Printf( "Server is not running.\n" );
        return;
    }
    
    if( cmdSystem->Argc() > 2 )
    {
        Com_Printf( "Usage: demo_record <demoname>\n" );
        return;
    }
    
    if( sv.demoState != DS_NONE )
    {
        Com_Printf( "A demo is already being recorded/played.\n" );
        return;
    }
    
    if( sv_maxclients->integer == MAX_CLIENTS )
    {
        Com_Printf( "Too many slots, reduce sv_maxclients.\n" );
        return;
    }
    
    if( cmdSystem->Argc() == 2 )
    {
        Com_sprintf( sv.demoName, sizeof( sv.demoName ), "svdemos/%s.svdm_%d", cmdSystem->Argv( 1 ), ETPROTOCOL_VERSION );
    }
    else
    {
        sint	number;
        // scan for a free demo name
        for( number = 0; number >= 0; number++ )
        {
            Com_sprintf( sv.demoName, sizeof( sv.demoName ), "svdemos/%d.svdm_%d", number, ETPROTOCOL_VERSION );
            if( !fileSystem->FileExists( sv.demoName ) )
                break;	// file doesn't exist
        }
        if( number < 0 )
        {
            Com_Printf( "Couldn't generate a filename for the demo, try deleting some old ones.\n" );
            return;
        }
    }
    
    sv.demoFile = fileSystem->FOpenFileWrite( sv.demoName );
    if( !sv.demoFile )
    {
        Com_Printf( "ERROR: Couldn't open %s for writing.\n", sv.demoName );
        return;
    }
    serverDemoSystem->DemoStartRecord();
}


/*
=================
SV_Demo_Play_f
=================
*/
void idServerCcmdsSystemLocal::Demo_Play_f( void )
{
    valueType* arg;
    
    if( cmdSystem->Argc() != 2 )
    {
        Com_Printf( "Usage: demo_play <demoname>\n" );
        return;
    }
    
    if( sv.demoState != DS_NONE )
    {
        Com_Printf( "A demo is already being recorded/played.\n" );
        return;
    }
    
    //if( sv_democlients->integer <= 0 )
    //{
    //    Com_Printf( "You need to set sv_democlients to a value greater than 0.\n" );
    //    return;
    //}
    
    // check for an extension .svdm_?? (?? is protocol)
    arg = cmdSystem->Argv( 1 );
    if( !strcmp( arg + strlen( arg ) - 6, va( ".svdm_%d", ETPROTOCOL_VERSION ) ) )
        Com_sprintf( sv.demoName, sizeof( sv.demoName ), "svdemos/%s", arg );
    else
        Com_sprintf( sv.demoName, sizeof( sv.demoName ), "svdemos/%s.svdm_%d", arg, ETPROTOCOL_VERSION );
        
    fileSystem->FOpenFileRead( sv.demoName, &sv.demoFile, true );
    if( !sv.demoFile )
    {
        Com_Printf( "ERROR: Couldn't open %s for reading.\n", sv.demoName );
        return;
    }
    
    serverDemoSystem->DemoStartPlayback();
}


/*
=================
SV_Demo_Stop_f
=================
*/
void idServerCcmdsSystemLocal::Demo_Stop_f( void )
{
    if( sv.demoState == DS_NONE )
    {
        Com_Printf( "No demo is currently being recorded or played.\n" );
        return;
    }
    
    // Close the demo file
    if( sv.demoState == DS_PLAYBACK )
    {
        serverDemoSystem->DemoStopPlayback();
    }
    else
    {
        serverDemoSystem->DemoStopRecord();
    }
}

/*
====================
SV_CompleteDemoName
====================
*/
void idServerCcmdsSystemLocal::CompleteDemoName( valueType* args, sint argNum )
{
    if( argNum == 2 )
    {
        valueType demoExt[16];
        
        Com_sprintf( demoExt, sizeof( demoExt ), ".svdm_%d", ETPROTOCOL_VERSION );
        Field_CompleteFilename( "svdemos", demoExt, true );
    }
}


/*
==================
idServerCcmdsSystemLocal::AddOperatorCommands
==================
*/
void idServerCcmdsSystemLocal::AddOperatorCommands( void )
{
    static bool initialized;
    
    if( initialized )
    {
        return;
    }
    
    initialized = true;
    
    cmdSystem->AddCommand( "heartbeat", &idServerCcmdsSystemLocal::Heartbeat_f, "Sends an update from the server to the master server with the result of updating server info." );
    cmdSystem->AddCommand( "status", &idServerCcmdsSystemLocal::Status_f, "Reports map loaded, and information on all connected players." );
    cmdSystem->AddCommand( "serverinfo", &idServerCcmdsSystemLocal::Serverinfo_f, "Shows server cvars on the local machine, including user created variables set with the sets command." );
    cmdSystem->AddCommand( "systeminfo", &idServerCcmdsSystemLocal::Systeminfo_f, "eports settings for: g_syncronousclients sv_serverid" );
    cmdSystem->AddCommand( "dumpuser", &idServerCcmdsSystemLocal::DumpUser_f, "Reports info on the specified user. Info includes: name, handicap, color, snd, model, snaps, rate" );
    cmdSystem->AddCommand( "map_restart", &idServerCcmdsSystemLocal::MapRestart_f, "Restarts the game on the current map. Replaces restart." );
    cmdSystem->AddCommand( "fieldinfo", &idServerCcmdsSystemLocal::FieldInfo_f, "Lists entitystate fields and playerstate fields and other data in the console, useful to developers." );
    cmdSystem->AddCommand( "sectorlist", &idServerWorldSystemLocal::SectorList_f, "Lists sectors and number of entities in each on the currently loaded map. " );
    cmdSystem->AddCommand( "map", &idServerCcmdsSystemLocal::Map_f, "oads a map file, specifying cheats disabled. The .BSP file extension is not required. See also devmap." );
    cmdSystem->SetCommandCompletionFunc( "map", &idServerCcmdsSystemLocal::CompleteMapName );
    cmdSystem->AddCommand( "gameCompleteStatus", &idServerCcmdsSystemLocal::GameCompleteStatus_f, "Sends game complete status to master server." ); // NERVE - SMF
#ifndef PRE_RELEASE_DEMO_NODEVMAP
    cmdSystem->AddCommand( "devmap", &idServerCcmdsSystemLocal::Map_f, "Loads a map file specifying cheats 1. The .BSP file extension is not required. See also map" );
    cmdSystem->SetCommandCompletionFunc( "devmap", &idServerCcmdsSystemLocal::CompleteMapName );
    cmdSystem->AddCommand( "spmap", &idServerCcmdsSystemLocal::Map_f, "Loads a single player map file" );
    cmdSystem->SetCommandCompletionFunc( "devmap", &idServerCcmdsSystemLocal::CompleteMapName );
    cmdSystem->AddCommand( "spdevmap", &idServerCcmdsSystemLocal::Map_f, "Loads a single player map file specifying cheats 1. The .BSP file extension is not required. See also map" );
    cmdSystem->SetCommandCompletionFunc( "devmap", &idServerCcmdsSystemLocal::CompleteMapName );
#endif
    cmdSystem->AddCommand( "loadgame", &idServerCcmdsSystemLocal::LoadGame_f, "Loading a saved game file" );
    cmdSystem->AddCommand( "killserver", &idServerCcmdsSystemLocal::KillServer_f, "Command for terminating the server, but leaving application running." );
    cmdSystem->AddCommand( "startRedirect", &idServerCcmdsSystemLocal::StartRedirect_f, "Redirecting clients" );
    cmdSystem->AddCommand( "endRedirect", Com_EndRedirect, "End of the cient redirection" );
    cmdSystem->AddCommand( "demo_record", &idServerCcmdsSystemLocal::Demo_Record_f, "Recording a demo" );
    cmdSystem->AddCommand( "demo_play", &idServerCcmdsSystemLocal::Demo_Play_f, "Playing a demo file" );
    cmdSystem->SetCommandCompletionFunc( "demo_play", &idServerCcmdsSystemLocal::CompleteDemoName );
    cmdSystem->AddCommand( "demo_stop", &idServerCcmdsSystemLocal::Demo_Stop_f, "Stopping a demo file" );
    
    cmdSystem->AddCommand( "cheater", &idServerOACSSystemLocal::ExtendedRecordSetCheater_f, "Server-side command to set a client's cheater label cheater <client> <label> where label is 0 for honest players, and >= 1 for cheaters" );
    
    if( com_dedicated->integer )
    {
        cmdSystem->AddCommand( "say", &idServerCcmdsSystemLocal::ConSay_f, "Used by the server. The text in the string is sent to all players as a message." );
    }
}
