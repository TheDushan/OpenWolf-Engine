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
// File name:   serverMain.cpp
// Version:     v1.00
// Created:     12/26/2018
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

serverStatic_t svs;			// persistant server info
server_t sv;				// local server
void*  gvm = nullptr;		// game

#if defined (UPDATE_SERVER)
versionMapping_t versionMap[MAX_UPDATE_VERSIONS];
S32 numVersions = 0;
#endif

convar_t* sv_fps = nullptr;			// time rate for running non-clients
convar_t* sv_timeout;		// seconds without any message
convar_t* sv_zombietime;	// seconds to sink messages after disconnect
convar_t* sv_rconPassword;	// password for remote server commands
convar_t* sv_privatePassword;	// password for the privateClient slots
convar_t* sv_allowDownload;
convar_t* sv_maxclients;
convar_t* sv_democlients;		// number of slots reserved for playing a demo

convar_t* sv_privateClients;	// number of clients reserved for password
convar_t* sv_hostname;
convar_t* sv_master[MAX_MASTER_SERVERS];	// master server ip address
convar_t* sv_reconnectlimit;	// minimum seconds between connect messages
convar_t* sv_tempbanmessage;
convar_t* sv_showloss;	// report when usercmds are lost
convar_t* sv_padPackets;	// add nop bytes to messages
convar_t* sv_killserver;	// menu system can set to 1 to shut server down
convar_t* sv_mapname;
convar_t* sv_mapChecksum;
convar_t* sv_serverid;
convar_t* sv_maxRate;
convar_t* sv_minPing;
convar_t* sv_maxPing;

//convar_t    *sv_gametype;
convar_t* sv_pure;
convar_t* sv_newGameShlib;
convar_t* sv_floodProtect;
convar_t* sv_allowAnonymous;
convar_t* sv_lanForceRate;	// TTimo - dedicated 1 (LAN) server forces local client rates to 99999 (bug #491)
convar_t* sv_onlyVisibleClients;	// DHM - Nerve
convar_t* sv_friendlyFire;	// NERVE - SMF
convar_t* sv_maxlives;	// NERVE - SMF
convar_t* sv_needpass;

convar_t* sv_dl_maxRate;
convar_t* g_gameType;

// of characters '0' through '9' and 'A' through 'F', default 0 don't require
// Rafael gameskill
//convar_t    *sv_gameskill;
// done

convar_t* sv_reloading;

convar_t* sv_showAverageBPS;	// NERVE - SMF - net debugging

convar_t* sv_wwwDownload;	// server does a www dl redirect
convar_t* sv_wwwBaseURL;	// base URL for redirect

// tell clients to perform their downloads while disconnected from the server
// this gets you a better throughput, but you loose the ability to control the download usage
convar_t* sv_wwwDlDisconnected;
convar_t* sv_wwwFallbackURL;	// URL to send to if an http/ftp fails or is refused client side

//bani
convar_t* sv_cheats;
convar_t* sv_packetloss;
convar_t* sv_packetdelay;

// fretn
convar_t* sv_fullmsg;

convar_t* sv_hibernateTime;
convar_t* sv_authServerHost;	// hostname/port what we are using, default "" (disabled)
convar_t* sv_authServerKey;

convar_t* sv_wh_active;
convar_t* sv_wh_bbox_horz;
convar_t* sv_wh_bbox_vert;
convar_t* sv_wh_check_fov;

#define LL( x ) x = LittleLong( x )

/*
=============================================================================
EVENT MESSAGES
=============================================================================
*/

idServerMainSystemLocal serverMainSystemLocal;
idServerMainSystem* serverMainSystem = &serverMainSystemLocal;

/*
===============
idServerMainSystemLocal::ExpandNewlines

Converts newlines to "\n" so a line prints nicer
===============
*/
UTF8* idServerMainSystemLocal::ExpandNewlines( UTF8* in )
{
    S32 l;
    static UTF8 string[1024];
    
    l = 0;
    while( *in && l < sizeof( string ) - 3 )
    {
        if( *in == '\n' )
        {
            string[l++] = '\\';
            string[l++] = 'n';
        }
        else
        {
            // NERVE - SMF - HACK - strip out localization tokens before string command is displayed in syscon window
            if( !Q_strncmp( in, "[lon]", 5 ) || !Q_strncmp( in, "[lof]", 5 ) )
            {
                in += 5;
                continue;
            }
            
            string[l++] = *in;
        }
        in++;
    }
    string[l] = 0;
    
    return string;
}

/*
======================
idServerMainSystemLocal::AddServerCommand

The given command will be transmitted to the client, and is guaranteed to
not have future snapshot_t executed before it is executed
======================
*/
void idServerMainSystemLocal::AddServerCommand( client_t* client, StringEntry cmd )
{
    S32 index, i;
    
    // do not send commands until the gamestate has been sent
    if( client->state < CS_PRIMED )
    {
        return;
    }
    
    client->reliableSequence++;
    
    // if we would be losing an old command that hasn't been acknowledged,
    // we must drop the connection
    // we check == instead of >= so a broadcast print added by SV_DropClient()
    // doesn't cause a recursive drop client
    if( client->reliableSequence - client->reliableAcknowledge == MAX_RELIABLE_COMMANDS + 1 )
    {
        Com_Printf( "===== pending server commands =====\n" );
        
        for( i = client->reliableAcknowledge + 1; i <= client->reliableSequence; i++ )
        {
            Com_Printf( "cmd %5d: %s\n", i, client->reliableCommands[i & ( MAX_RELIABLE_COMMANDS - 1 )] );
        }
        
        Com_Printf( "cmd %5d: %s\n", i, cmd );
        
        serverClientSystem->DropClient( client, "Server command overflow" );
        return;
    }
    
    index = client->reliableSequence & ( MAX_RELIABLE_COMMANDS - 1 );
    Q_strncpyz( client->reliableCommands[index], cmd, sizeof( client->reliableCommands[index] ) );
}

/*
=================
idServerMainSystemLocal::SendServerCommand

Sends a reliable command string to be interpreted by
the client game module: "cp", "print", "chat", etc
A nullptr client will broadcast to all clients
=================
*/
void idServerMainSystemLocal::SendServerCommand( client_t* cl, StringEntry fmt, ... )
{
    S32 j;
    U8 message[MAX_MSGLEN];
    va_list argptr;
    client_t* client;
    
    va_start( argptr, fmt );
    Q_vsnprintf( ( UTF8* )message, sizeof( message ), fmt, argptr );
    va_end( argptr );
    
    // do not forward server command messages that would be too big to clients
    // ( q3infoboom / q3msgboom stuff )
    if( strlen( ( UTF8* )message ) > 1022 )
    {
        return;
    }
    
    if( cl != nullptr )
    {
        AddServerCommand( cl, ( UTF8* )message );
        return;
    }
    
    // hack to echo broadcast prints to console
    if( com_dedicated->integer && !strncmp( ( UTF8* )message, "print", 5 ) )
    {
        Com_Printf( "broadcast: %s\n", ExpandNewlines( ( UTF8* )message ) );
    }
    
    // save broadcasts to demo
    if( sv.demoState == DS_RECORDING )
    {
        serverDemoSystem->DemoWriteServerCommand( ( UTF8* )message );
    }
    
    // send the data to all relevent clients
    for( j = 0, client = svs.clients; j < sv_maxclients->integer; j++, client++ )
    {
        if( client->state < CS_PRIMED )
        {
            continue;
        }
        
        // Ridah, don't need to send messages to AI
        if( client->gentity && client->gentity->r.svFlags & SVF_BOT )
        {
            continue;
        }
        // done.
        
        AddServerCommand( client, ( UTF8* )message );
    }
}

/*
==============================================================================
MASTER SERVER FUNCTIONS
==============================================================================
*/

/*
===============
idServerMainSystemLocal::MasterHeartbeat

Send a message to the masters every few minutes to
let it know we are alive, and log information.
We will also have a heartbeat sent when a server
changes from empty to non-empty, and full to non-full,
but not on every player enter or exit.
===============
*/
void idServerMainSystemLocal::MasterHeartbeat( StringEntry hbname )
{
    static netadr_t adr[MAX_MASTER_SERVERS][2]; // [2] for v4 and v6 address for the same address string.
    S32             i;
    S32             res;
    S32             netenabled;
    UTF8* master;
    
    // Update Server doesn't send heartbeat
#if defined (UPDATE_SERVER)
    return;
#endif
    
    if( serverGameSystem->GameIsSinglePlayer() )
    {
        // no heartbeats for SP
        return;
    }
    
    netenabled = cvarSystem->VariableIntegerValue( "net_enabled" );
    
    // "dedicated 1" is for lan play, "dedicated 2" is for inet public play
    if( !com_dedicated || com_dedicated->integer != 2 || !( netenabled & ( NET_ENABLEV6 | NET_ENABLEV4 ) ) )
    {
        return;     // only dedicated servers send heartbeats
        
    }
    // if not time yet, don't send anything
    if( svs.time < svs.nextHeartbeatTime )
    {
        return;
    }
    
    svs.nextHeartbeatTime = svs.time + HEARTBEAT_MSEC;
    
    // send to group masters
    for( i = 0; i < MAX_MASTER_SERVERS; i++ )
    {
        master = cvarSystem->VariableString( va( "sv_master%i", i + 1 ) );
        if( master[0] == '\0' )
        {
            continue;
        }
        
        // see if we haven't already resolved the name
        if( netenabled & NET_ENABLEV4 )
        {
            if( adr[i][0].type == NA_BAD )
            {
                Com_Printf( "Resolving %s (IPv4)\n", master );
                res = NET_StringToAdr( master, &adr[i][0], NA_IP );
                
                if( res == 2 )
                {
                    // if no port was specified, use the default master port
                    adr[i][0].port = BigShort( PORT_MASTER );
                }
                
                if( res )
                {
                    Com_Printf( "%s resolved to %s\n", master, NET_AdrToString( adr[i][0] ) );
                }
                else
                {
                    Com_Printf( "%s has no IPv4 address.\n", master );
                }
            }
        }
        
        if( netenabled & NET_ENABLEV6 )
        {
            if( adr[i][1].type == NA_BAD )
            {
                Com_Printf( "Resolving %s (IPv6)\n", master );
                res = NET_StringToAdr( master, &adr[i][1], NA_IP6 );
                
                if( res == 2 )
                {
                    // if no port was specified, use the default master port
                    adr[i][1].port = BigShort( PORT_MASTER );
                }
                
                if( res )
                {
                    Com_Printf( "%s resolved to %s\n", master, NET_AdrToString( adr[i][1] ) );
                }
                else
                {
                    Com_Printf( "%s has no IPv6 address.\n", master );
                }
            }
        }
        
        if( ( ( ( netenabled & NET_ENABLEV4 ) && adr[i][0].type == NA_BAD ) || !( netenabled & NET_ENABLEV4 ) )
                && ( ( ( netenabled & NET_ENABLEV6 ) && adr[i][1].type == NA_BAD ) || !( netenabled & NET_ENABLEV6 ) ) )
        {
            // if the address failed to resolve, clear it
            // so we don't take repeated dns hits
            Com_Printf( "Couldn't resolve address: %s\n", master );
            cvarSystem->Set( va( "sv_master%i", i + 1 ), "" );
            continue;
        }
        
        Com_Printf( "Sending heartbeat to %s\n", master );
        
        // this command should be changed if the server info / status format
        // ever incompatably changes
        
        if( ( netenabled & NET_ENABLEV4 ) && adr[i][0].type != NA_BAD )
        {
            NET_OutOfBandPrint( NS_SERVER, adr[i][0], "heartbeat %s\n", hbname );
        }
        
        if( netenabled & NET_ENABLEV6 && adr[i][1].type != NA_BAD )
        {
            NET_OutOfBandPrint( NS_SERVER, adr[i][1], "heartbeat %s\n", hbname );
        }
        
    }
}

/*
=================
idServerMainSystemLocal::MasterGameCompleteStatus

NERVE - SMF - Sends gameCompleteStatus messages to all master servers
=================
*/
void idServerMainSystemLocal::MasterGameCompleteStatus( void )
{
    S32 i;
    static netadr_t adr[MAX_MASTER_SERVERS];
    
    // "dedicated 1" is for lan play, "dedicated 2" is for inet public play
    if( !com_dedicated || com_dedicated->integer != 2 )
    {
        // only dedicated servers send master game status
        return;
    }
    
    // send to group masters
    for( i = 0 ; i < MAX_MASTER_SERVERS ; i++ )
    {
        if( !sv_master[i]->string[0] )
        {
            continue;
        }
        
        // see if we haven't already resolved the name
        // resolving usually causes hitches on win95, so only
        // do it when needed
        if( sv_master[i]->modified )
        {
            sv_master[i]->modified = false;
            
            Com_Printf( "Resolving %s\n", sv_master[i]->string );
            
            if( !NET_StringToAdr( sv_master[i]->string, &adr[i], NA_IP ) )
            {
                // if the address failed to resolve, clear it
                // so we don't take repeated dns hits
                Com_Printf( "Couldn't resolve address: %s\n", sv_master[i]->string );
                cvarSystem->Set( sv_master[i]->name, "" );
                sv_master[i]->modified = false;
                continue;
            }
            
            if( !::strchr( sv_master[i]->string, ':' ) )
            {
                adr[i].port = BigShort( PORT_MASTER );
            }
            
            Com_Printf( "%s resolved to %i.%i.%i.%i:%i\n", sv_master[i]->string, adr[i].ip[0], adr[i].ip[1], adr[i].ip[2], adr[i].ip[3], BigShort( adr[i].port ) );
        }
        
        Com_Printf( "Sending gameCompleteStatus to %s\n", sv_master[i]->string );
        
        // this command should be changed if the server info / status format
        // ever incompatably changes
        GameCompleteStatus( adr[i] );
    }
}

/*
=================
idServerMainSystemLocal::MasterShutdown

Informs all masters that this server is going down
=================
*/
void idServerMainSystemLocal::MasterShutdown( void )
{
    // send a hearbeat right now
    svs.nextHeartbeatTime = -9999;
    MasterHeartbeat( HEARTBEAT_DEAD );	// NERVE - SMF - changed to flatline
    
    // send it again to minimize chance of drops
    //svs.nextHeartbeatTime = -9999;
    //MasterHeartbeat( HEARTBEAT_DEAD );
    
    // when the master tries to poll the server, it won't respond, so
    // it will be removed from the list
}


/*
=================
idServerMainSystemLocal::MasterGameStat
=================
*/
void idServerMainSystemLocal::MasterGameStat( StringEntry data )
{
    netadr_t adr;
    
    if( !com_dedicated || com_dedicated->integer != 2 )
    {
        return; // only dedicated servers send stats
    }
    
    Com_Printf( "Resolving %s\n", MASTER_SERVER_NAME );
    
    switch( NET_StringToAdr( MASTER_SERVER_NAME, &adr, NA_UNSPEC ) )
    {
        case 0:
            Com_Printf( "Couldn't resolve master address: %s\n", MASTER_SERVER_NAME );
            return;
            
        case 2:
            adr.port = BigShort( PORT_MASTER );
        default:
            break;
    }
    
    Com_Printf( "%s resolved to %s\n", MASTER_SERVER_NAME, NET_AdrToStringwPort( adr ) );
    
    Com_Printf( "Sending gamestat to %s\n", MASTER_SERVER_NAME );
    NET_OutOfBandPrint( NS_SERVER, adr, "gamestat %s", data );
}

/*
==============================================================================

CONNECTIONLESS COMMANDS

==============================================================================
*/

//bani - bugtraq 12534
//returns true if valid challenge
//returns false if m4d h4x0rz
/*
===============
idServerMainSystemLocal::VerifyChallenge
===============
*/
bool idServerMainSystemLocal::VerifyChallenge( UTF8* challenge )
{
    S32 i, j;
    
    if( !challenge )
    {
        return false;
    }
    
    j = strlen( challenge );
    
    if( j > 64 )
    {
        return false;
    }
    
    for( i = 0; i < j; i++ )
    {
        if( challenge[i] == '\\' || challenge[i] == '/' || challenge[i] == '%' || challenge[i] == ';' || challenge[i] == '"' || challenge[i] < 32 || /*// non-ascii */ challenge[i] > 126 )  // non-ascii
        {
            return false;
        }
    }
    return true;
}

/*
================
idServerMainSystemLocal::Status

Responds with all the info that qplug or qspy can see about the server
and all connected players.  Used for getting detailed information after
the simple info query.
================
*/
void idServerMainSystemLocal::Status( netadr_t from )
{
    S32 i, statusLength, playerLength;
    UTF8 player[1024], status[MAX_MSGLEN], infostring[MAX_INFO_STRING];
    client_t* cl;
    playerState_t* ps;
    
    // ignore if we are in single player
    if( serverGameSystem->GameIsSinglePlayer() )
    {
        return;
    }
    
    //bani - bugtraq 12534
    if( !VerifyChallenge( cmdSystem->Argv( 1 ) ) )
    {
        return;
    }
    
#if defined (UPDATE_SERVER)
    return;
#endif
    
    ::strcpy( infostring, cvarSystem->InfoString( CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE ) );
    
    // echo back the parameter to status. so master servers can use it as a challenge
    // to prevent timed spoofed reply packets that add ghost servers
    Info_SetValueForKey( infostring, "challenge", cmdSystem->Argv( 1 ) );
    
    // add "demo" to the sv_keywords if restricted
    if( cvarSystem->VariableValue( "fs_restrict" ) )
    {
        UTF8 keywords[MAX_INFO_STRING];
        
        Com_sprintf( keywords, sizeof( keywords ), "ettest %s", Info_ValueForKey( infostring, "sv_keywords" ) );
        Info_SetValueForKey( infostring, "sv_keywords", keywords );
    }
    else
    {
        // echo back the parameter to status. so master servers can use it as a challenge
        // to prevent timed spoofed reply packets that add ghost servers
        Info_SetValueForKey(	infostring, "challenge", cmdSystem->Argv( 1 ) );
    }
    
    status[0] = 0;
    statusLength = 0;
    
    for( i = 0; i < sv_maxclients->integer; i++ )
    {
        cl = &svs.clients[i];
        
        if( cl->state >= CS_CONNECTED )
        {
            ps = serverGameSystem->GameClientNum( i );
            Com_sprintf( player, sizeof( player ), "%i %i \"%s\"\n", ps->persistant[PERS_SCORE], cl->ping, cl->name );
            playerLength = strlen( player );
            
            if( statusLength + playerLength >= sizeof( status ) )
            {
                break; // can't hold any more
            }
            
            ::strcpy( status + statusLength, player );
            statusLength += playerLength;
        }
    }
    
    NET_OutOfBandPrint( NS_SERVER, from, "statusResponse\n%s\n%s", infostring, status );
}

/*
=================
idServerMainSystemLocal::GameCompleteStatus

NERVE - SMF - Send serverinfo cvars, etc to master servers when
game complete. Useful for tracking global player stats.
=================
*/
void idServerMainSystemLocal::GameCompleteStatus( netadr_t from )
{
    S32 i, statusLength, playerLength;
    UTF8 player[1024], status[MAX_MSGLEN], infostring[MAX_INFO_STRING];
    client_t* cl;
    playerState_t* ps;
    
    // ignore if we are in single player
    if( serverGameSystem->GameIsSinglePlayer() )
    {
        return;
    }
    
    //bani - bugtraq 12534
    if( !VerifyChallenge( cmdSystem->Argv( 1 ) ) )
    {
        return;
    }
    
    ::strcpy( infostring, cvarSystem->InfoString( CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE ) );
    
    // echo back the parameter to status. so master servers can use it as a challenge
    // to prevent timed spoofed reply packets that add ghost servers
    Info_SetValueForKey( infostring, "challenge", cmdSystem->Argv( 1 ) );
    
    // add "demo" to the sv_keywords if restricted
    if( cvarSystem->VariableValue( "fs_restrict" ) )
    {
        UTF8 keywords[MAX_INFO_STRING];
        
        Com_sprintf( keywords, sizeof( keywords ), "ettest %s", Info_ValueForKey( infostring, "sv_keywords" ) );
        Info_SetValueForKey( infostring, "sv_keywords", keywords );
    }
    
    status[0] = 0;
    statusLength = 0;
    
    for( i = 0; i < sv_maxclients->integer; i++ )
    {
        cl = &svs.clients[i];
        
        if( cl->state >= CS_CONNECTED )
        {
            ps = serverGameSystem->GameClientNum( i );
            
            Com_sprintf( player, sizeof( player ), "%i %i \"%s\"\n", ps->persistant[PERS_SCORE], cl->ping, cl->name );
            
            playerLength = strlen( player );
            
            if( statusLength + playerLength >= sizeof( status ) )
            {
                // can't hold any more
                break;
            }
            
            ::strcpy( status + statusLength, player );
            statusLength += playerLength;
        }
    }
    
    NET_OutOfBandPrint( NS_SERVER, from, "gameCompleteStatus\n%s\n%s", infostring, status );
}

/*
================
idServerMainSystemLocal::Info

Responds with a short info message that should be enough to determine
if a user is interested in a server to do a full status
================
*/
void idServerMainSystemLocal::Info( netadr_t from )
{
    S32 i, count;
    UTF8* gamedir, infostring[MAX_INFO_STRING], *antilag, *weaprestrict, *balancedteams;
    
    // ignore if we are in single player
    if( serverGameSystem->GameIsSinglePlayer() )
    {
        return;
    }
    
    //bani - bugtraq 12534
    if( !VerifyChallenge( cmdSystem->Argv( 1 ) ) )
    {
        return;
    }
    
    /*
     * Check whether cmdSystem->Argv(1) has a sane length. This was not done in the original Quake3 version which led
     * to the Infostring bug discovered by Luigi Auriemma. See http://aluigi.altervista.org/ for the advisory.
    */
    // A maximum challenge length of 128 should be more than plenty.
    if( ::strlen( cmdSystem->Argv( 1 ) ) > 128 )
    {
        return;
    }
    
#if defined (UPDATE_SERVER)
    return;
#endif
    
    // don't count privateclients
    count = 0;
    
    for( i = 0; i < sv_maxclients->integer; i++ )
    {
        if( svs.clients[i].state >= CS_CONNECTED )
        {
            count++;
        }
    }
    
    infostring[0] = 0;
    
    // echo back the parameter to status. so servers can use it as a challenge
    // to prevent timed spoofed reply packets that add ghost servers
    Info_SetValueForKey( infostring, "challenge", cmdSystem->Argv( 1 ) );
    Info_SetValueForKey( infostring, "protocol", va( "%i", com_protocol->integer ) );
    Info_SetValueForKey( infostring, "hostname", sv_hostname->string );
    Info_SetValueForKey( infostring, "serverload", va( "%i", svs.serverLoad ) );
    Info_SetValueForKey( infostring, "mapname", sv_mapname->string );
    Info_SetValueForKey( infostring, "clients", va( "%i", count ) );
    Info_SetValueForKey( infostring, "sv_maxclients", va( "%i", sv_maxclients->integer - sv_privateClients->integer ) );
    //Info_SetValueForKey( infostring, "gametype", va("%i", sv_gametype->integer ) );
    Info_SetValueForKey( infostring, "gametype", cvarSystem->VariableString( "g_gametype" ) );
    Info_SetValueForKey( infostring, "pure", va( "%i", sv_pure->integer ) );
    
    if( sv_minPing->integer )
    {
        Info_SetValueForKey( infostring, "minPing", va( "%i", sv_minPing->integer ) );
    }
    
    if( sv_maxPing->integer )
    {
        Info_SetValueForKey( infostring, "maxPing", va( "%i", sv_maxPing->integer ) );
    }
    
    gamedir = cvarSystem->VariableString( "fs_game" );
    if( *gamedir )
    {
        Info_SetValueForKey( infostring, "game", gamedir );
    }
    
    Info_SetValueForKey( infostring, "sv_allowAnonymous", va( "%i", sv_allowAnonymous->integer ) );
    
    // Rafael gameskill
//  Info_SetValueForKey (infostring, "gameskill", va ("%i", sv_gameskill->integer));
    // done
    
    Info_SetValueForKey( infostring, "friendlyFire", va( "%i", sv_friendlyFire->integer ) );	// NERVE - SMF
    Info_SetValueForKey( infostring, "maxlives", va( "%i", sv_maxlives->integer ? 1 : 0 ) );	// NERVE - SMF
    Info_SetValueForKey( infostring, "needpass", va( "%i", sv_needpass->integer ? 1 : 0 ) );
    Info_SetValueForKey( infostring, "gamename", GAMENAME_STRING );	// Arnout: to be able to filter out Quake servers
    
    // TTimo
    antilag = cvarSystem->VariableString( "g_antilag" );
    if( antilag )
    {
        Info_SetValueForKey( infostring, "g_antilag", antilag );
    }
    
    weaprestrict = cvarSystem->VariableString( "g_heavyWeaponRestriction" );
    if( weaprestrict )
    {
        Info_SetValueForKey( infostring, "weaprestrict", weaprestrict );
    }
    
    balancedteams = cvarSystem->VariableString( "g_balancedteams" );
    if( balancedteams )
    {
        Info_SetValueForKey( infostring, "balancedteams", balancedteams );
    }
    
    NET_OutOfBandPrint( NS_SERVER, from, "infoResponse\n%s", infostring );
}

/*
================
idServerMainSystemLocal::GetUpdateInfo

Responds with a short info message that tells the client if they
have an update available for their version
================
*/
void idServerMainSystemLocal::GetUpdateInfo( netadr_t from )
{
#if defined (UPDATE_SERVER)
    UTF8* version, *platform;
    S32 i;
    bool found = false;
    
    version = cmdSystem->Argv( 1 );
    platform = cmdSystem->Argv( 2 );
    
    Com_DPrintf( "idServerMainSystemLocal::GetUpdateInfo: version == %s / %s,\n", version, platform );
    
    for( i = 0; i < numVersions; i++ )
    {
        if( strcmp( versionMap[i].platform, platform ) == 0 && ( strcmp( versionMap[i].version, version ) != 0 ) )
        {
            // If the installer is set to "current", we will skip over it
            if( strcmp( versionMap[i].installer, "current" ) )
            {
                found = true;
            }
            
            break;
        }
    }
    
    if( found )
    {
        NET_OutOfBandPrint( NS_SERVER, from, "updateResponse 1 %s", versionMap[i].installer );
        Com_DPrintf( "   SENT:  updateResponse 1 %s\n", versionMap[i].installer );
    }
    else
    {
        NET_OutOfBandPrint( NS_SERVER, from, "updateResponse 0" );
        Com_DPrintf( "   SENT:  updateResponse 0\n" );
    }
#endif
}

/*
==============
idServerMainSystemLocal::FlushRedirect
==============
*/
void idServerMainSystemLocal::FlushRedirect( UTF8* outputbuf )
{
    if( *outputbuf )
    {
        NET_OutOfBandPrint( NS_SERVER, svs.redirectAddress, "print\n%s", outputbuf );
    }
}

/*
===============
idServerMainSystemLocal::CheckDRDoS

DRDoS stands for "Distributed Reflected Denial of Service".
See here: http://www.lemuria.org/security/application-drdos.html

Returns false if we're good.  true return value means we need to block.
If the address isn't NA_IP, it's automatically denied.
===============
*/
bool idServerMainSystemLocal::CheckDRDoS( netadr_t from )
{
    S32 i, oldestBan, oldestBanTime, globalCount, specificCount, oldest, oldestTime, lastGlobalLogTime = 0;
    receipt_t* receipt;
    netadr_t exactFrom;
    floodBan_t* ban;
    
    // Usually the network is smart enough to not allow incoming UDP packets
    // with a source address being a spoofed LAN address.  Even if that's not
    // the case, sending packets to other hosts in the LAN is not a big deal.
    // NA_LOOPBACK qualifies as a LAN address.
    if( Net_IsLANAddress( from ) )
    {
        return false;
    }
    
    exactFrom = from;
    
    if( from.type == NA_IP )
    {
        // xx.xx.xx.0
        from.ip[3] = 0;
    }
    else
    {
        from.ip6[15] = 0;
    }
    
    // This quick exit strategy while we're being bombarded by getinfo/getstatus requests
    // directed at a specific IP address doesn't really impact server performance.
    // The code below does its duty very quickly if we're handling a flood packet.
    ban = &svs.infoFloodBans[0];
    oldestBan = 0;
    oldestBanTime = 0x7fffffff;
    for( i = 0; i < MAX_INFO_FLOOD_BANS; i++, ban++ )
    {
        if( svs.time - ban->time < 120000 && // Two minute ban.
                NET_CompareBaseAdr( from, ban->adr ) )
        {
            ban->count++;
            
            if( !ban->flood && ( ( svs.time - ban->time ) >= 3000 ) && ban->count <= 5 )
            {
                Com_DPrintf( "Unban info flood protect for address %s, they're not flooding\n", NET_AdrToString( exactFrom ) );
                ::memset( ban, 0, sizeof( floodBan_t ) );
                oldestBan = i;
                break;
            }
            
            if( ban->count >= 180 )
            {
                Com_DPrintf( "Renewing info flood ban for address %s, received %i getinfo/getstatus requests in %i milliseconds\n", NET_AdrToString( exactFrom ), ban->count, svs.time - ban->time );
                ban->time = svs.time;
                ban->count = 0;
                ban->flood = true;
            }
            
            return true;
        }
        
        if( ban->time < oldestBanTime )
        {
            oldestBanTime = ban->time;
            oldestBan = i;
        }
    }
    
    // Count receipts in last 2 seconds.
    globalCount = 0;
    specificCount = 0;
    receipt = &svs.infoReceipts[0];
    oldest = 0;
    oldestTime = 0x7fffffff;
    
    for( i = 0; i < MAX_INFO_RECEIPTS; i++, receipt++ )
    {
        if( receipt->time + 2000 > svs.time )
        {
            if( receipt->time )
            {
                // When the server starts, all receipt times are at zero.  Furthermore,
                // svs.time is close to zero.  We check that the receipt time is already
                // set so that during the first two seconds after server starts, queries
                // from the master servers don't get ignored.  As a consequence a potentially
                // unlimited number of getinfo+getstatus responses may be sent during the
                // first frame of a server's life.
                globalCount++;
            }
            
            if( NET_CompareBaseAdr( from, receipt->adr ) )
            {
                specificCount++;
            }
        }
        
        if( receipt->time < oldestTime )
        {
            oldestTime = receipt->time;
            oldest = i;
        }
    }
    
    if( specificCount >= 3 )  // Already sent 3 to this IP in last 2 seconds.
    {
        Com_Printf( "Possible DRDoS attack to address %s, putting into temporary getinfo/getstatus ban list\n", NET_AdrToString( exactFrom ) );
        ban = &svs.infoFloodBans[oldestBan];
        ban->adr = from;
        ban->time = svs.time;
        ban->count = 0;
        ban->flood = false;
        return true;
    }
    
    if( globalCount == MAX_INFO_RECEIPTS )  // All receipts happened in last 2 seconds.
    {
        // Detect time wrap where the server sets time back to zero.  Problem
        // is that we're using a static variable here that doesn't get zeroed out when
        // the time wraps.  TTimo's way of doing this is casting everything including
        // the difference to U32, but I think that's confusing to the programmer.
        if( svs.time < lastGlobalLogTime )
        {
            lastGlobalLogTime = 0;
        }
        
        if( lastGlobalLogTime + 1000 <= svs.time )  // Limit one log every second.
        {
            Com_Printf( "Detected flood of arbitrary getinfo/getstatus connectionless packets\n" );
            lastGlobalLogTime = svs.time;
        }
        return true;
    }
    
    receipt = &svs.infoReceipts[oldest];
    receipt->adr = from;
    receipt->time = svs.time;
    return false;
}

/*
===============
idServerMainSystemLocal::RemoteCommand

An rcon packet arrived from the network.
Shift down the remaining args
Redirect all printfs
===============
*/
void idServerMainSystemLocal::RemoteCommand( netadr_t from, msg_t* msg )
{
    bool valid;
    U32 i, time;
    UTF8 remaining[1024];
    
    // show_bug.cgi?id=376
    // if we send an OOB print message this size, 1.31 clients die in a Com_Printf buffer overflow
    // the buffer overflow will be fixed in > 1.31 clients
    // but we want a server side fix
    // we must NEVER send an OOB message that will be > 1.31 MAXPRINTMSG (4096)
    UTF8 sv_outputbuf[MAX_MSGLEN - 16];
    static U32 lasttime = 0;
    
    // TTimo - show_bug.cgi?id=534
    time = Com_Milliseconds();
    if( !::strlen( sv_rconPassword->string ) || strcmp( cmdSystem->Argv( 1 ), sv_rconPassword->string ) )
    {
        // MaJ - If the rconpassword is bad and one just happned recently, don't spam the log file, just die.
        if( ( U32 )( time - lasttime ) < 500u )
        {
            return;
        }
        
        valid = false;
        
        Com_Printf( "Bad rcon from %s:\n%s\n", NET_AdrToString( from ), cmdSystem->Argv( 2 ) );
    }
    else
    {
        // MaJ - If the rconpassword is good, allow it much sooner than a bad one.
        if( ( U32 )( time - lasttime ) < 200u )
        {
            return;
        }
        
        valid = true;
        
        Com_Printf( "Rcon from %s:\n%s\n", NET_AdrToString( from ), cmdSystem->Argv( 2 ) );
    }
    
    lasttime = time;
    
    // start redirecting all print outputs to the packet
    svs.redirectAddress = from;
    // FIXME TTimo our rcon redirection could be improved
    //   big rcon commands such as status lead to sending
    //   out of band packets on every single call to Com_Printf
    //   which leads to client overflows
    //   see show_bug.cgi?id=51
    //     (also a Q3 issue)
    Com_BeginRedirect( sv_outputbuf, sizeof( sv_outputbuf ), FlushRedirect );
    
    if( !::strlen( sv_rconPassword->string ) )
    {
        Com_Printf( "No rconpassword set on the server.\n" );
    }
    else if( !valid )
    {
        Com_Printf( "Bad rconpassword.\n" );
    }
    else
    {
        remaining[0] = 0;
        
        for( i = 2; i < cmdSystem->Argc(); i++ )
        {
            strcat( remaining, cmdSystem->Argv( i ) );
            strcat( remaining, " " );
        }
        
        cmdSystem->ExecuteString( remaining );
        
    }
    
    Com_EndRedirect();
}

/*
=================
idServerMainSystemLocal::ConnectionlessPacket

A connectionless packet has four leading 0xff
characters to distinguish it from a game channel.
Clients that are in the game can still send
connectionless packets.
=================
*/
void idServerMainSystemLocal::ConnectionlessPacket( netadr_t from, msg_t* msg )
{
    UTF8* s, *c;
    
    MSG_BeginReadingOOB( msg );
    MSG_ReadLong( msg );			// skip the -1 marker
    
    if( !Q_strncmp( "connect", ( UTF8* )&msg->data[4], 7 ) )
    {
        idHuffmanSystemLocal::DynDecompress( msg, 12 );
    }
    
    s = MSG_ReadStringLine( msg );
    
    cmdSystem->TokenizeString( s );
    
    c = cmdSystem->Argv( 0 );
    Com_DPrintf( "SV packet %s : %s\n", NET_AdrToString( from ), c );
    
    if( !Q_stricmp( c, "getstatus" ) )
    {
        if( CheckDRDoS( from ) )
        {
            return;
        }
        
        Status( from );
    }
    else if( !Q_stricmp( c, "getinfo" ) )
    {
        if( CheckDRDoS( from ) )
        {
            return;
        }
        Info( from );
    }
    else if( !Q_stricmp( c, "getchallenge" ) )
    {
        serverClientSystem->GetChallenge( from );
    }
    else if( !Q_stricmp( c, "connect" ) )
    {
        serverClientSystem->DirectConnect( from );
    }
    else if( !Q_stricmp( c, "rcon" ) )
    {
        RemoteCommand( from, msg );
    }
#if defined (UPDATE_SERVER)
    else if( !Q_stricmp( c, "getUpdateInfo" ) )
    {
        GetUpdateInfo( from );
    }
#endif
    else if( !Q_stricmp( c, "disconnect" ) )
    {
        // if a client starts up a local server, we may see some spurious
        // server disconnect messages when their new server sees our final
        // sequenced messages to the old client
    }
    else
    {
        Com_DPrintf( "bad connectionless packet from %s:\n%s\n", NET_AdrToString( from ), s );
    }
}

/*
=================
idServerMainSystemLocal::ReadPackets
=================
*/
void idServerMainSystemLocal::PacketEvent( netadr_t from, msg_t* msg )
{
    S32 i, qport;
    client_t* cl;
    
    // check for connectionless packet (0xffffffff) first
    if( msg->cursize >= 4 && *( S32* )msg->data == -1 )
    {
        ConnectionlessPacket( from, msg );
        return;
    }
    
    if( sv.state == SS_DEAD )
    {
        return;
    }
    
    // read the qport out of the message so we can fix up
    // stupid address translating routers
    MSG_BeginReadingOOB( msg );
    MSG_ReadLong( msg );			// sequence number
    qport = MSG_ReadShort( msg ) & 0xffff;
    
    // find which client the message is from
    for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
    {
        if( cl->state <= CS_FREE )
        {
            continue;
        }
        
        if( !NET_CompareBaseAdr( from, cl->netchan.remoteAddress ) )
        {
            continue;
        }
        
        // it is possible to have multiple clients from a single IP
        // address, so they are differentiated by the qport variable
        if( cl->netchan.qport != qport )
        {
            continue;
        }
        
        // the IP port can't be used to differentiate them, because
        // some address translating routers periodically change UDP
        // port assignments
        if( cl->netchan.remoteAddress.port != from.port )
        {
            Com_Printf( "idServerMainSystemLocal::PacketEvent: fixing up a translated port\n" );
            cl->netchan.remoteAddress.port = from.port;
        }
        
        // make sure it is a valid, in sequence packet
        if( serverNetChanSystem->NetchanProcess( cl, msg ) )
        {
            // zombie clients still need to do the Netchan_Process
            // to make sure they don't need to retransmit the final
            // reliable message, but they don't do any other processing
            if( cl->state != CS_ZOMBIE )
            {
                cl->lastPacketTime = svs.time;	// don't timeout
                serverClientSystem->ExecuteClientMessage( cl, msg );
            }
        }
        return;
    }
    
    // if we received a sequenced packet from an address we don't recognize,
    // send an out of band disconnect packet to it
    NET_OutOfBandPrint( NS_SERVER, from, "disconnect" );
}

/*
===================
idServerMainSystemLocal::CalcPings

Updates the cl->ping variables
===================
*/
void idServerMainSystemLocal::CalcPings( void )
{
    S32 i, j, total, count, delta;
    client_t* cl;
    playerState_t* ps;
    
    for( i = 0; i < sv_maxclients->integer; i++ )
    {
        cl = &svs.clients[i];
        
#if defined (UPDATE_SERVER)
        if( !cl )
        {
            continue;
        }
#endif
        
        if( cl->state != CS_ACTIVE )
        {
            cl->ping = 999;
            continue;
        }
        
        if( !cl->gentity )
        {
            cl->ping = 999;
            continue;
        }
        
        if( cl->gentity->r.svFlags & SVF_BOT )
        {
            cl->ping = 0;
            continue;
        }
        
        total = 0;
        count = 0;
        
        for( j = 0; j < PACKET_BACKUP; j++ )
        {
            if( cl->frames[j].messageAcked <= 0 )
            {
                continue;
            }
            
            delta = cl->frames[j].messageAcked - cl->frames[j].messageSent;
            count++;
            total += delta;
        }
        
        if( !count )
        {
            cl->ping = 999;
        }
        else
        {
            cl->ping = total / count;
            if( cl->ping > 999 )
            {
                cl->ping = 999;
            }
        }
        
        // let the game dll know about the ping
        ps = serverGameSystem->GameClientNum( i );
        ps->ping = cl->ping;
    }
}

/*
==================
idServerMainSystemLocal::CheckTimeouts

If a packet has not been received from a client for timeout->integer
seconds, drop the conneciton.  Server time is used instead of
realtime to avoid dropping the local client while debugging.

When a client is normally dropped, the client_t goes into a zombie state
for a few seconds to make sure any final reliable message gets resent
if necessary
==================
*/
void idServerMainSystemLocal::CheckTimeouts( void )
{
    S32 i, droppoint, zombiepoint;
    client_t* cl;
    
    droppoint = svs.time - 1000 * sv_timeout->integer;
    zombiepoint = svs.time - 1000 * sv_zombietime->integer;
    
    for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
    {
        // message times may be wrong across a changelevel
        if( cl->lastPacketTime > svs.time )
        {
            cl->lastPacketTime = svs.time;
        }
        
        if( cl->state == CS_ZOMBIE && cl->lastPacketTime < zombiepoint )
        {
            // using the client id cause the cl->name is empty at this point
            Com_DPrintf( "Going from CS_ZOMBIE to CS_FREE for client %d\n", i );
            cl->state = CS_FREE;	// can now be reused
            
            continue;
        }
        
        if( cl->state >= CS_CONNECTED && cl->lastPacketTime < droppoint )
        {
            // wait several frames so a debugger session doesn't
            // cause a timeout
            if( ++cl->timeoutCount > 5 )
            {
                serverClientSystem->DropClient( cl, "timed out" );
                cl->state = CS_FREE;	// don't bother with zombie state
            }
        }
        else
        {
            cl->timeoutCount = 0;
        }
    }
}

/*
==================
idServerMainSystemLocal::CheckPaused
==================
*/
bool idServerMainSystemLocal::CheckPaused( void )
{
    S32 count, i;
    client_t* cl;
    
    if( !cl_paused->integer )
    {
        return false;
    }
    
    // only pause if there is just a single client connected
    count = 0;
    
    for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
    {
        if( cl->state >= CS_ZOMBIE && cl->netchan.remoteAddress.type != NA_BOT )
        {
            count++;
        }
    }
    
    if( count > 1 )
    {
        // don't pause
        if( sv_paused->integer )
        {
            cvarSystem->Set( "sv_paused", "0" );
        }
        
        return false;
    }
    
    if( !sv_paused->integer )
    {
        cvarSystem->Set( "sv_paused", "1" );
    }
    
    return true;
}

/*
==================
idServerMainSystemLocal::CheckCvars
==================
*/
void idServerMainSystemLocal::CheckCvars( void )
{
    static S32 lastMod = -1;
    bool changed = false;
    
    if( sv_hostname->modificationCount != lastMod )
    {
        UTF8 hostname[MAX_INFO_STRING];
        UTF8* c = hostname;
        lastMod = sv_hostname->modificationCount;
        
        strcpy( hostname, sv_hostname->string );
        
        while( *c )
        {
            if( ( *c == '\\' ) || ( *c == ';' ) || ( *c == '"' ) )
            {
                *c = '.';
                changed = true;
            }
            c++;
        }
        if( changed )
        {
            cvarSystem->Set( "sv_hostname", hostname );
        }
    }
}

void idServerMainSystemLocal::IntegerOverflowShutDown( UTF8* msg )
{
    // save the map name in case it gets cleared during the shut down
    UTF8 mapName[MAX_QPATH];
    Q_strncpyz( mapName, cvarSystem->VariableString( "mapname" ), sizeof( mapName ) );
    
    serverInitSystem->Shutdown( msg );
    cmdBufferSystem->AddText( va( "map %s\n", mapName ) );
}


/*
==================
idServerMainSystemLocal::Frame

Player movement occurs as a result of packet events, which
happen before idServerMainSystemLocal::Frame is called
==================
*/
void idServerMainSystemLocal::Frame( S32 msec )
{
    S32 frameMsec, startTime, frameStartTime = 0, frameEndTime;
    static S32 start, end;
    UTF8 mapname[MAX_QPATH];
    
    start = idsystem->Milliseconds();
    svs.stats.idle += ( F64 )( start - end ) / 1000;
    
    // the menu kills the server with this cvar
    if( sv_killserver->integer )
    {
        serverInitSystem->Shutdown( "Server was killed.\n" );
        cvarSystem->Set( "sv_killserver", "0" );
        return;
    }
    
    if( svs.initialized && svs.gameStarted )
    {
        S32 i = 0, players = 0;
        for( i = 0; i < sv_maxclients->integer; i++ )
        {
            if( svs.clients[i].state >= CS_CONNECTED && svs.clients[i].netchan.remoteAddress.type != NA_BOT )
            {
                players++;
            }
        }
        
        //Check for hibernation mode
        if( sv_hibernateTime->integer && !svs.hibernation.enabled && !players )
        {
            S32 elapsed_time = idsystem->Milliseconds() - svs.hibernation.lastTimeDisconnected;
            
            if( elapsed_time >= sv_hibernateTime->integer )
            {
                cvarSystem->Set( "sv_fps", "1" );
                sv_fps->value = svs.hibernation.sv_fps;
                svs.hibernation.enabled = true;
                Com_Printf( "Server switched to hibernation mode\n" );
            }
        }
    }
    
    if( !com_sv_running->integer )
    {
        // Running as a server, but no map loaded
#if defined (DEDICATED)
        // Block until something interesting happens
        idsystem->Sleep( -1 );
#endif
        return;
    }
    
    // allow pause if only the local client is connected
    if( CheckPaused() )
    {
        return;
    }
    
    if( com_dedicated->integer )
    {
        frameStartTime = idsystem->Milliseconds();
    }
    
    // if it isn't time for the next frame, do nothing
    if( sv_fps->integer < 1 )
    {
        cvarSystem->Set( "sv_fps", "10" );
    }
    
    frameMsec = 1000 / sv_fps->integer * com_timescale->value;
    
    // don't let it scale below 1ms
    if( frameMsec < 1 )
    {
        cvarSystem->Set( "timescale", va( "%f", sv_fps->integer / 1000.0f ) );
        frameMsec = 1;
    }
    
    sv.timeResidual += msec;
    
    if( com_dedicated->integer && sv.timeResidual < frameMsec )
    {
        // NET_Sleep will give the OS time slices until either get a packet
        // or time enough for a server frame has gone by
        NET_Sleep( frameMsec - sv.timeResidual );
        return;
    }
    
    bool hasHuman = false;
    for( S32 i = 0; i < sv_maxclients->integer; ++i )
    {
        client_t* cl = &svs.clients[i];
        if( cl->state >= CS_CONNECTED )
        {
            const bool isBot = ( cl->netchan.remoteAddress.type == NA_BOT ) || ( cl->gentity && ( cl->gentity->r.svFlags & SVF_BOT ) );
            if( !isBot )
            {
                hasHuman = true;
                break;
            }
        }
    }
    
    // The shader time is stored as a floating-point number.
    // Some mods may still have code like "sin(cg.time / 1000.0f)".
    // IEEE 754 floats have a 23-bit mantissa.
    // Rounding errors will start after roughly ((1<<23) / (60*1000)) ~ 139.8 minutes.
    const S32 minRebootTimeCvar = 60 * 1000 * cvarSystem->Get( "sv_minRebootDelayMins", "1440", 0, "description" )->integer;
    const S32 minRebootTimeConst = 60 * 60 * 1000;	// absolute min. time: 1 hour
    const S32 maxRebootTime = 0x7FFFFFFF;			// absolute max. time: ~ 24.86 days
    const S32 minRebootTime = max( minRebootTimeCvar, minRebootTimeConst );
    if( svs.time >= minRebootTime && !hasHuman )
    {
        IntegerOverflowShutDown( "Restarting server early to avoid time wrapping and/or precision issues" );
        return;
    }
    
    if( minRebootTimeCvar < minRebootTimeConst )
    {
        cvarSystem->Set( "sv_minRebootDelayMins", va( "%d", minRebootTimeConst / ( 60 * 1000 ) ) );
    }
    
    // If the time is close to hitting the 32nd bit, kick all clients and clear svs.time
    // rather than checking for negative time wraparound everywhere.
    // No, resetting the time on map change like ioq3 does is not on the cards. It breaks stuff.
    if( svs.time >= maxRebootTime )
    {
        IntegerOverflowShutDown( "Restarting server due to time wrapping" );
        return;
    }
    
    // this can happen considerably earlier when lots of clients play and the map doesn't change
    if( svs.nextSnapshotEntities >= 0x7FFFFFFE - svs.numSnapshotEntities )
    {
        Q_strncpyz( mapname, sv_mapname->string, MAX_QPATH );
        IntegerOverflowShutDown( "Restarting server due to numSnapshotEntities wrapping" );
        return;
    }
    
    if( sv.restartTime && svs.time >= sv.restartTime )
    {
        sv.restartTime = 0;
        cmdBufferSystem->AddText( "map_restart 0\n" );
        return;
    }
    
    // update infostrings if anything has been changed
    if( cvar_modifiedFlags & CVAR_SERVERINFO )
    {
        serverInitSystem->SetConfigstring( CS_SERVERINFO, cvarSystem->InfoString( CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE ) );
        cvar_modifiedFlags &= ~CVAR_SERVERINFO;
    }
    
    if( cvar_modifiedFlags & CVAR_SERVERINFO_NOUPDATE )
    {
        serverInitSystem->SetConfigstringNoUpdate( CS_SERVERINFO, cvarSystem->InfoString( CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE ) );
        cvar_modifiedFlags &= ~CVAR_SERVERINFO_NOUPDATE;
    }
    
    if( cvar_modifiedFlags & CVAR_SYSTEMINFO )
    {
        serverInitSystem->SetConfigstring( CS_SYSTEMINFO, cvarSystem->InfoString_Big( CVAR_SYSTEMINFO ) );
        cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
    }
    
    // NERVE - SMF
    if( cvar_modifiedFlags & CVAR_WOLFINFO )
    {
        serverInitSystem->SetConfigstring( CS_WOLFINFO, cvarSystem->InfoString( CVAR_WOLFINFO ) );
        cvar_modifiedFlags &= ~CVAR_WOLFINFO;
    }
    
    if( com_speeds->integer )
    {
        startTime = idsystem->Milliseconds();
    }
    else
    {
        // quite a compiler warning
        startTime = 0;
    }
    
    // update ping based on the all received frames
    CalcPings();
    
    // run the game simulation in chunks
    while( sv.timeResidual >= frameMsec )
    {
        sv.timeResidual -= frameMsec;
        svs.time += frameMsec;
        sv.time += frameMsec;
        
        // let everything in the world think and move
#if !defined (UPDATE_SERVER)
        sgame->RunFrame( sv.time );
        if( sv.demoState == DS_RECORDING )
        {
            serverDemoSystem->DemoWriteFrame();
        }
        else if( sv.demoState == DS_PLAYBACK )
        {
            serverDemoSystem->DemoReadFrame();
        }
#endif
        if( sv_oacsEnable->integer == 1 )
        {
            idServerOACSSystemLocal::ExtendedRecordUpdate();
        }
    }
    
    if( com_speeds->integer )
    {
        time_game = idsystem->Milliseconds() - startTime;
    }
    
    // check timeouts
    CheckTimeouts();
    
    // check user info buffer thingy
    serverSnapshotSystem->CheckClientUserinfoTimer();
    
    // send messages back to the clients
    serverSnapshotSystem->SendClientMessages();
    
    CheckCvars();
    
    // send a heartbeat to the master if needed
    MasterHeartbeat( HEARTBEAT_GAME );
    
    if( com_dedicated->integer )
    {
        frameEndTime = idsystem->Milliseconds();
        
        svs.totalFrameTime += ( frameEndTime - frameStartTime );
        svs.currentFrameIndex++;
        
        //if( svs.currentFrameIndex % 50 == 0 )
        //  Com_Printf( "currentFrameIndex: %i\n", svs.currentFrameIndex );
        
        if( svs.currentFrameIndex == SERVER_PERFORMANCECOUNTER_FRAMES )
        {
            S32 averageFrameTime;
            
            averageFrameTime = svs.totalFrameTime / SERVER_PERFORMANCECOUNTER_FRAMES;
            
            svs.sampleTimes[svs.currentSampleIndex % SERVER_PERFORMANCECOUNTER_SAMPLES] = averageFrameTime;
            svs.currentSampleIndex++;
            
            if( svs.currentSampleIndex > SERVER_PERFORMANCECOUNTER_SAMPLES )
            {
                S32 totalTime, i;
                
                totalTime = 0;
                
                for( i = 0; i < SERVER_PERFORMANCECOUNTER_SAMPLES; i++ )
                {
                    totalTime += svs.sampleTimes[i];
                }
                
                if( !totalTime )
                {
                    totalTime = 1;
                }
                
                averageFrameTime = totalTime / SERVER_PERFORMANCECOUNTER_SAMPLES;
                
                svs.serverLoad = ( averageFrameTime / ( F32 )frameMsec ) * 100;
            }
            
            //Com_Printf( "serverload: %i (%i/%i)\n", svs.serverLoad, averageFrameTime, frameMsec );
            
            svs.totalFrameTime = 0;
            svs.currentFrameIndex = 0;
        }
    }
    else
    {
        svs.serverLoad = -1;
    }
    
    // collect timing statistics
    end = idsystem->Milliseconds();
    svs.stats.active += ( ( F64 )( end - start ) ) / 1000;
    
    if( ++svs.stats.count == STATFRAMES )
    {
        svs.stats.latched_active = svs.stats.active;
        svs.stats.latched_idle = svs.stats.idle;
        svs.stats.latched_packets = svs.stats.packets;
        svs.stats.active = 0;
        svs.stats.idle = 0;
        svs.stats.packets = 0;
        svs.stats.count = 0;
    }
}

/*
=================
idServerMainSystemLocal::LoadTag
=================
*/
S32 idServerMainSystemLocal::LoadTag( StringEntry mod_name )
{
    S32 i, j, version;
    U8* buffer;
    tagHeader_t* pinmodel;
    md3Tag_t* tag, *readTag;
    
    for( i = 0; i < sv.num_tagheaders; i++ )
    {
        if( !Q_stricmp( mod_name, sv.tagHeadersExt[i].filename ) )
        {
            return i + 1;
        }
    }
    
    fileSystem->ReadFile( mod_name, ( void** )&buffer );
    
    if( !buffer )
    {
        return false;
    }
    
    pinmodel = ( tagHeader_t* ) buffer;
    
    version = LittleLong( pinmodel->version );
    if( version != TAG_VERSION )
    {
        Com_Printf( S_COLOR_YELLOW "WARNING: idServerMainSystemLocal::LoadTag: %s has wrong version (%i should be %i)\n", mod_name, version, TAG_VERSION );
        return 0;
    }
    
    if( sv.num_tagheaders >= MAX_TAG_FILES )
    {
        Com_Error( ERR_DROP, "MAX_TAG_FILES reached\n" );
        
        fileSystem->FreeFile( buffer );
        return 0;
    }
    
    LL( pinmodel->ident );
    LL( pinmodel->numTags );
    LL( pinmodel->ofsEnd );
    LL( pinmodel->version );
    
    Q_strncpyz( sv.tagHeadersExt[sv.num_tagheaders].filename, mod_name, MAX_QPATH );
    sv.tagHeadersExt[sv.num_tagheaders].start = sv.num_tags;
    sv.tagHeadersExt[sv.num_tagheaders].count = pinmodel->numTags;
    
    if( sv.num_tags + pinmodel->numTags >= MAX_SERVER_TAGS )
    {
        Com_Error( ERR_DROP, "MAX_SERVER_TAGS reached\n" );
        
        fileSystem->FreeFile( buffer );
        return false;
    }
    
    // swap all the tags
    tag = &sv.tags[sv.num_tags];
    sv.num_tags += pinmodel->numTags;
    readTag = ( md3Tag_t* )( buffer + sizeof( tagHeader_t ) );
    
    for( i = 0; i < pinmodel->numTags; i++, tag++, readTag++ )
    {
        for( j = 0; j < 3; j++ )
        {
            tag->origin[j] = LittleFloat( readTag->origin[j] );
            tag->axis[0][j] = LittleFloat( readTag->axis[0][j] );
            tag->axis[1][j] = LittleFloat( readTag->axis[1][j] );
            tag->axis[2][j] = LittleFloat( readTag->axis[2][j] );
        }
        
        Q_strncpyz( tag->name, readTag->name, 64 );
    }
    
    fileSystem->FreeFile( buffer );
    return ++sv.num_tagheaders;
}
