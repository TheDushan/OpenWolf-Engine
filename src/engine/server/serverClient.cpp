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
// File name:   serverClient.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: server code for dealing with clients
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.hpp>
#elif DEDICATED
#include <null/null_serverprecompiled.hpp>
#else
#include <framework/precompiled.hpp>
#endif

idServerClientSystemLocal serverClientLocal;
idServerClientSystem* serverClientSystem = &serverClientLocal;

/*
===============
idServerClientSystemLocal::idServerClientSystemLocal
===============
*/
idServerClientSystemLocal::idServerClientSystemLocal( void )
{
}

/*
===============
idServerClientSystemLocal::~idServerClientSystemLocal
===============
*/
idServerClientSystemLocal::~idServerClientSystemLocal( void )
{
}

/*
=================
idServerClientSystemLocal::GetChallenge

A "getchallenge" OOB command has been received
Returns a challenge number that can be used
in a subsequent connectResponse command.
We do this to prevent denial of service attacks that
flood the server with invalid connection IPs.  With a
challenge, they must give a valid IP address.

If we are authorizing, a challenge request will cause a packet
to be sent to the authorize server.

When an authorizeip is returned, a challenge response will be
sent to that ip.
=================
*/
void idServerClientSystemLocal::GetChallenge( netadr_t from )
{
    sint i, oldest, oldestTime, oldestClientTime, clientChallenge, getChallengeCookie;
    challenge_t* challenge;
    bool wasfound = false, gameMismatch, isBanned = false;
    valueType* guid, *gameName;
    
    // ignore if we are in single player
    if( serverGameSystem->GameIsSinglePlayer() )
    {
        return;
    }
    
    if( serverCcmdsLocal.TempBanIsBanned( from ) )
    {
        networkChainSystem->OutOfBandPrint( NS_SERVER, from, "print\n%s\n", sv_tempbanmessage->string );
        return;
    }
    
    gameName = cmdSystem->Argv( 2 );
    guid = cmdSystem->Argv( 3 );
    getChallengeCookie = ::atoi( cmdSystem->Argv( 4 ) );
    
    if( !getChallengeCookie )
    {
        networkChainSystem->OutOfBandPrint( NS_SERVER, from, "print\nIllegal request\n" );
        return;
    }
    
    oldest = 0;
    oldestClientTime = oldestTime = 0x7fffffff;
    
    // see if we already have a challenge for this ip
    challenge = &svs.challenges[0];
    clientChallenge = ::atoi( cmdSystem->Argv( 1 ) );
    
    for( i = 0; i < MAX_CHALLENGES; i++, challenge++ )
    {
        if( !challenge->connected && networkSystem->CompareAdr( from, challenge->adr ) )
        {
            wasfound = true;
            
            if( challenge->time < oldestClientTime )
            {
                oldestClientTime = challenge->time;
            }
            
            if( challenge->authServerStrict )
            {
                oldestClientTime = svs.time;
                challenge->authServerStrict = false;
            }
        }
        
        if( wasfound && i >= MAX_CHALLENGES_MULTI )
        {
            i = MAX_CHALLENGES;
            break;
        }
        
        if( challenge->time < oldestTime )
        {
            oldestTime = challenge->time;
            oldest = i;
        }
    }
    
    if( challenge->authServerStrict )
    {
        oldestTime = svs.time;
        challenge->authServerStrict = false;
    }
    
    if( i == MAX_CHALLENGES )
    {
        // this is the first time this client has asked for a challenge
        challenge = &svs.challenges[oldest];
        challenge->clientChallenge = clientChallenge;
        challenge->adr = from;
        challenge->pingTime = -1;
        challenge->firstTime = svs.time;
        challenge->firstPing = 0;
        challenge->connected = false;
        challenge->authenticated = false;
    }
    
    // always generate a new challenge number, so the client cannot circumvent sv_maxping
    challenge->challenge = ( ( rand() << 16 ) ^ rand() ) ^ svs.time;
    challenge->wasrefused = false;
    challenge->time = svs.time;
    
    Q_strncpyz( challenge->guid, guid, sizeof( challenge->guid ) );
    challenge->getChallengeCookie = getChallengeCookie;
    
    if( svs.authorizeAddress.type == NA_BAD )
    {
        Com_DPrintf( "Resolving %s\n", AUTHORIZE_SERVER_NAME );
        
        if( networkChainSystem->StringToAdr( AUTHORIZE_SERVER_NAME, &svs.authorizeAddress, NA_IP ) )
        {
            svs.authorizeAddress.port = BigShort( PORT_AUTHORIZE );
            Com_DPrintf( "%s resolved to %i.%i.%i.%i:%i\n", AUTHORIZE_SERVER_NAME,
                         svs.authorizeAddress.ip[0], svs.authorizeAddress.ip[1],
                         svs.authorizeAddress.ip[2], svs.authorizeAddress.ip[3],
                         BigShort( svs.authorizeAddress.port ) );
        }
    }
    
    if( svs.authorizeAddress.type == NA_BAD )
    {
        Com_DPrintf( "Couldn't resolve auth server address\n" );
    }
    else if( sv.time - oldestClientTime > AUTHORIZE_TIMEOUT )
    {
        Com_DPrintf( "Authorize server timed out\n" );
        
        networkChainSystem->OutOfBandPrint( NS_SERVER, challenge->adr, "authStatus %i 2 Server is configured as Strict and auth server failed to respond.", challenge->getChallengeCookie );
        
        challenge->authServerStrict = true;
        return;
    }
    else
    {
        Com_DPrintf( "Sending getIpAuthorize for %s\n", networkSystem->AdrToString( from ) );
        
        networkChainSystem->OutOfBandPrint( NS_SERVER, svs.authorizeAddress, "getIpAuthorize %i %i.%i.%i.%i %s %i %i %i",  challenge->challenge, from.ip[0], from.ip[1], from.ip[2], from.ip[3],
                                            guid, challenge->getChallengeCookie, sv_minimumAgeGuid->integer, sv_maximumAgeGuid->integer );
                                            
        return;
    }
    
    if( svs.time - challenge->firstTime > AUTHORIZE_TIMEOUT )
    {
        challenge->pingTime = svs.time;
        
        if( sv_onlyVisibleClients->integer )
        {
            networkChainSystem->OutOfBandPrint( NS_SERVER, challenge->adr, "challengeResponse %i %i %i", challenge->challenge, clientChallenge, sv_onlyVisibleClients->integer );
        }
        else
        {
            networkChainSystem->OutOfBandPrint( NS_SERVER, challenge->adr, "challengeResponse %i %i", challenge->challenge, clientChallenge );
        }
        return;
    }
}

/*
====================
idServerClientSystemLocal::AuthorizeIpPacket

A packet has been returned from the authorize server.
If we have a challenge adr for that ip, send the
challengeResponse to it
====================
*/
void idServerClientSystemLocal::AuthorizeIpPacket( netadr_t from )
{
    sint i, challenge, response;
    challenge_t* challengeptr;
    valueType* reason;
    
    if( !networkSystem->CompareBaseAdr( from, svs.authorizeAddress ) )
    {
        Com_Printf( "idServerClientSystemLocal::AuthorizeIpPacket: not from authorize server\n" );
        return;
    }
    
    challenge = ::atoi( cmdSystem->Argv( 1 ) );
    
    for( i = 0; i < MAX_CHALLENGES; i++ )
    {
        if( svs.challenges[i].challenge == challenge )
        {
            break;
        }
    }
    
    if( i == MAX_CHALLENGES )
    {
        Com_Printf( "idServerClientSystemLocal::AuthorizeIpPacket: challenge not found\n" );
        return;
    }
    
    challengeptr = &svs.challenges[i];
    challengeptr->pingTime = svs.time;
    response = ::atoi( cmdSystem->Argv( 2 ) );
    reason = cmdSystem->ArgsFrom( 3 );
    
    if( response == 0 )
    {
        challengeptr->authenticated = true;
        
        if( sv_onlyVisibleClients->integer )
        {
            networkChainSystem->OutOfBandPrint( NS_SERVER, challengeptr->adr, "challengeResponse %i %i", challengeptr->challenge, sv_onlyVisibleClients->integer );
        }
        else
        {
            networkChainSystem->OutOfBandPrint( NS_SERVER, svs.challenges[i].adr, "challengeResponse %i", challengeptr->challenge );
        }
        return;
    }
    
    if( response == 1 )
    {
        if( !reason )
        {
            networkChainSystem->OutOfBandPrint( NS_SERVER, challengeptr->adr, "authStatus %i 1", challengeptr->getChallengeCookie );
        }
        else
        {
            networkChainSystem->OutOfBandPrint( NS_SERVER, challengeptr->adr, "authStatus %i 1 %s", challengeptr->getChallengeCookie, reason );
        }
        
        ::memset( challengeptr, 0, sizeof( *challengeptr ) );
        return;
    }
    
    if( response > 1 )
    {
        if( !reason )
        {
            networkChainSystem->OutOfBandPrint( NS_SERVER, challengeptr->adr, "authStatus %i %i", challengeptr->getChallengeCookie, response );
        }
        else
        {
            networkChainSystem->OutOfBandPrint( NS_SERVER, challengeptr->adr, "authStatus %i %i %s", challengeptr->getChallengeCookie, response, reason );
        }
    }
    
    ::memset( challengeptr, 0, sizeof( *challengeptr ) );
}

/*
==================
idServerClientSystemLocal::DirectConnect

A "connect" OOB command has been received
==================
*/
void idServerClientSystemLocal::DirectConnect( netadr_t from )
{
    valueType userinfo[MAX_INFO_STRING], *denied, *ip, guid[GUIDKEY_SIZE], *password;
    sint i, clientNum, qport, challenge, startIndex, count, oldInfoLen2, newInfoLen2;
    client_t* cl, *newcl, temp;
    sharedEntity_t* ent;
#if !defined (UPDATE_SERVER)
    sint	version;
#endif
    bool reconnect = false, authed;
    user_t* user;
    
    Com_DPrintf( "idServerClientSystemLocal::DirectConnect ()\n" );
    
    Q_strncpyz( userinfo, cmdSystem->Argv( 1 ), sizeof( userinfo ) );
    oldInfoLen2 = static_cast<sint>( ::strlen( userinfo ) );
    challenge = ::atoi( Info_ValueForKey( userinfo, "challenge" ) );
    qport = ::atoi( Info_ValueForKey( userinfo, "qport" ) );
    
    // DHM - Nerve :: Update Server allows any protocol to connect
    // NOTE TTimo: but we might need to store the protocol around for potential non http/ftp clients
#if !defined (UPDATE_SERVER)
    version = ::atoi( Info_ValueForKey( userinfo, "protocol" ) );
    if( version != com_protocol->integer )
    {
        networkChainSystem->OutOfBandPrint( NS_SERVER, from, "print\nServer uses protocol version %i (yours is %i).\n", com_protocol->integer, version );
        Com_DPrintf( "    rejected connect from version %i\n", version );
        return;
    }
#endif
    
    if( serverCcmdsLocal.TempBanIsBanned( from ) )
    {
        networkChainSystem->OutOfBandPrint( NS_SERVER, from, "print\n%s\n", sv_tempbanmessage->string );
        return;
    }
    
    // quick reject
    for( i = 0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++ )
    {
        if( networkSystem->CompareBaseAdr( from, cl->netchan.remoteAddress )  && ( cl->netchan.qport == qport || from.port == cl->netchan.remoteAddress.port ) )
        {
            if( ( svs.time - cl->lastConnectTime ) < ( sv_reconnectlimit->integer * 1000 ) )
            {
                Com_DPrintf( "%s:reconnect rejected : too soon\n", networkSystem->AdrToString( from ) );
                return;
            }
            
            break;
        }
    }
    
    // See comment made in idServerClientSystemLocal::UserinfoChanged() regarding handicap.
    while( true )
    {
        // Unfortunately the string fuctions such as strlen() and Info_RemoveKey()
        // are quite expensive for large userinfo strings.  Imagine if someone
        // bombarded the server with connect packets.  That would result in very bad
        // server hitches.  We need to fix that.
        Info_RemoveKey( userinfo, "handicap" );
        newInfoLen2 = static_cast<sint>( ::strlen( userinfo ) );
        
        if( oldInfoLen2 == newInfoLen2 )
        {
            // userinfo wasn't modified.
            break;
        }
        
        oldInfoLen2 = newInfoLen2;
    }
    
    // don't let "ip" overflow userinfo string
    if( networkSystem->IsLocalAddress( from ) )
    {
        ip = "localhost";
    }
    else
    {
        ip = const_cast<valueType*>( reinterpret_cast<const valueType*>( networkSystem->AdrToString( from ) ) );
    }
    
    if( ( static_cast<sint>( ::strlen( ip ) ) + static_cast<sint>( ::strlen( userinfo ) ) + 4 ) >= MAX_INFO_STRING )
    {
        networkChainSystem->OutOfBandPrint( NS_SERVER, from,
                                            "print\nUserinfo string length exceeded.  "
                                            "Try removing setu cvars from your config.\n" );
        return;
    }
    
    Info_SetValueForKey( userinfo, "ip", ip );
    
    // see if the challenge is valid (local clients don't need to challenge)
    if( !networkSystem->IsLocalAddress( from ) )
    {
        sint ping;
        challenge_t* challengeptr;
        
        for( i = 0 ; i < MAX_CHALLENGES ; i++ )
        {
            if( networkSystem->CompareAdr( from, svs.challenges[i].adr ) )
            {
                if( challenge == svs.challenges[i].challenge )
                {
                    // good
                    break;
                }
            }
        }
        
        if( i == MAX_CHALLENGES )
        {
            networkChainSystem->OutOfBandPrint( NS_SERVER, from, "print\n[err_dialog]No or bad challenge for address.\n" );
            return;
        }
        
        challengeptr = &svs.challenges[i];
        if( challengeptr->wasrefused )
        {
            // Return silently, so that error messages written by the server keep being displayed.
            return;
        }
        
        if( svs.challenges[i].firstPing == 0 )
        {
            ping = svs.time - svs.challenges[i].pingTime;
            svs.challenges[i].firstPing = ping;
        }
        else
        {
            ping = svs.challenges[i].firstPing;
        }
        
        Com_Printf( "Client %i connecting with %i challenge ping\n", i, ping );
        svs.challenges[i].connected = true;
        
        // never reject a LAN client based on ping
        if( !networkSystem->IsLANAddress( from ) )
        {
            if( sv_minPing->value && ping < sv_minPing->value )
            {
                // don't let them keep trying until they get a big delay
                networkChainSystem->OutOfBandPrint( NS_SERVER, from, "print\nServer is for high pings only\n" );
                Com_DPrintf( "Client %i rejected on a too low ping\n", i );
                challengeptr->wasrefused = true;
                return;
            }
            if( sv_maxPing->value && ping > sv_maxPing->value )
            {
                networkChainSystem->OutOfBandPrint( NS_SERVER, from, "print\nServer is for low pings only\n" );
                Com_DPrintf( "Client %i rejected on a too high ping\n", i );
                challengeptr->wasrefused = true;
                return;
            }
        }
        
        Q_strncpyz( guid, svs.challenges[i].guid, sizeof( guid ) );
        authed = svs.challenges[i].authenticated;
        
        Com_Printf( "Client %i connecting with %i challenge ping\n", i, ping );
        challengeptr->connected = true;
    }
    else
    {
        Q_strncpyz( guid, Info_ValueForKey( userinfo, "cl_guid" ), sizeof( guid ) );
        authed = false;
    }
    
    // q3fill protection
    if( !networkSystem->IsLANAddress( from ) )
    {
        sint connectingip = 0;
        
        for( i = 0; i < sv_maxclients->integer; i++ )
        {
            if( svs.clients[i].netchan.remoteAddress.type != NA_BOT && svs.clients[i].state == CS_CONNECTED
                    && networkSystem->CompareBaseAdr( svs.clients[i].netchan.remoteAddress, from ) )
            {
                connectingip++;
            }
        }
        
        if( connectingip >= 3 )
        {
            networkChainSystem->OutOfBandPrint( NS_SERVER, from, "print\nPlease wait...\n" );
        }
    }
    
    // Community server login and ban system
    // check for privateClient password
    if( idServerCommunityServer::Login( userinfo, &user ) != CS_OK )
    {
        return;
    }
    
    // Check if guid is banned
    if( idServerCommunityServer::checkBanGUID( Info_ValueForKey( userinfo, "cl_guid" ) ) == 1 )
    {
        networkChainSystem->OutOfBandPrint( NS_SERVER, from, "print\nYou are not allowed to enter to this server!\n" );
        return;
    }
    
    newcl = &temp;
    ::memset( newcl, 0, sizeof( client_t ) );
    
    // if there is already a slot for this ip, reuse it
    for( i = 0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++ )
    {
        if( cl->state == CS_FREE )
        {
            continue;
        }
        
        if( networkSystem->CompareBaseAdr( from, cl->netchan.remoteAddress ) && ( cl->netchan.qport == qport || from.port == cl->netchan.remoteAddress.port ) )
        {
            Com_Printf( "%s:reconnect\n", networkSystem->AdrToString( from ) );
            newcl = cl;
            
            //reconnect = true;
            //disconnect the client from the game first so any flags the
            //player might have are dropped
#if !defined UPDATE_SERVER
            //sgame->ClientDisconnect( newcl - svs.clients );
#endif
            goto gotnewcl;
        }
    }
    
    // find a client slot
    // if "sv_privateClients" is set > 0, then that number
    // of client slots will be reserved for connections that
    // have "password" set to the value of "sv_privatePassword"
    // Info requests will report the maxclients as if the private
    // slots didn't exist, to prevent people from trying to connect
    // to a full server.
    // This is to allow us to reserve a couple slots here on our
    // servers so we can play without having to kick people.
    
    // check for privateClient password
    password = Info_ValueForKey( userinfo, "password" );
    if( !strcmp( password, sv_privatePassword->string ) )
    {
        startIndex = 0;
    }
    else
    {
        // skip past the reserved slots
        startIndex = sv_privateClients->integer;
    }
    
    newcl = nullptr;
    for( i = startIndex; i < sv_maxclients->integer ; i++ )
    {
        cl = &svs.clients[i];
        
        if( cl->state == CS_FREE )
        {
            newcl = cl;
            break;
        }
    }
    
    if( !newcl )
    {
        cl = nullptr;
        
        // Find a bot
        for( i = startIndex; i < sv_maxclients->integer; i++ )
        {
            if( svs.clients[i].netchan.remoteAddress.type == NA_BOT )
            {
                cl = &svs.clients[i];
                break;
            }
        }
        
        if( !cl )
        {
            networkChainSystem->OutOfBandPrint( NS_SERVER, from, "xFull\n" );
            Com_DPrintf( "Rejected a connection.\n" );
            return;
        }
        
        // Found a bot. Remove it to make room
        DropClient( cl, "bot removal" );
        newcl = cl;
    }
    
    // we got a newcl, so reset the reliableSequence and reliableAcknowledge
    cl->reliableAcknowledge = 0;
    cl->reliableSequence = 0;
    
gotnewcl:
    // build a new connection
    // accept the new client
    // this is the only place a client_t is ever initialized
    *newcl = temp;
    
    // Adding Community server Data
    if( newcl->cs_user != nullptr )
    {
        idServerCommunityServer::destroyUserData( newcl->cs_user );
    }
    newcl->cs_user = user;
    
    clientNum = ARRAY_INDEX( svs.clients, newcl );
    ent = serverGameSystem->GentityNum( clientNum );
    newcl->gentity = ent;
    
#if !defined UPDATE_SERVER
    ent->r.svFlags = 0;
#endif
    
    // save the challenge
    newcl->challenge = challenge;
    
    // save the address
    networkChainSystem->Setup( NS_SERVER, &newcl->netchan, from, qport );
    // init the netchan queue
    
    newcl->authenticated = authed;
    Info_SetValueForKey( userinfo, "authenticated", va( "%i", authed ) );
    Q_strncpyz( newcl->guid, guid, sizeof( newcl->guid ) );
    Info_SetValueForKey( userinfo, "cl_guid", guid );
    
    // save the userinfo
    Q_strncpyz( newcl->userinfo, userinfo, sizeof( newcl->userinfo ) );
    
    // get the game a chance to reject this connection or modify the userinfo
#ifndef UPDATE_SERVER
    denied = sgame->ClientConnect( clientNum, true ); // firstTime = true
    if( denied )
    {
        networkChainSystem->OutOfBandPrint( NS_SERVER, from, "print\n[err_dialog]%s\n", denied );
        Com_DPrintf( "Game rejected a connection: %s.\n", denied );
        return;
    }
#endif
    
    cvarSystem->SetValue( "sv_fps", svs.hibernation.sv_fps );
    svs.hibernation.enabled = false;
    Com_Printf( "Server restored from hibernation\n" );
    
    UserinfoChanged( newcl );
    
    // DHM - Nerve :: Clear out firstPing now that client is connected
    svs.challenges[i].firstPing = 0;
    
    // send the connect packet to the client
    networkChainSystem->OutOfBandPrint( NS_SERVER, from, "connectResponse" );
    
    Com_DPrintf( "Going from CS_FREE to CS_CONNECTED for %s\n", newcl->name );
    
    newcl->state = CS_CONNECTED;
    newcl->nextSnapshotTime = svs.time;
    newcl->lastPacketTime = svs.time;
    newcl->lastConnectTime = svs.time;
    
    // when we receive the first packet from the client, we will
    // notice that it is from a different serverid and that the
    // gamestate message was not just sent, forcing a retransmit
    newcl->gamestateMessageNum = -1;
    
    newcl->lastUserInfoChange = 0;
    newcl->lastUserInfoCount = 0;
    
    // if this was the first client on the server, or the last client
    // the server can hold, send a heartbeat to the master.
    count = 0;
    
    for( i = 0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++ )
    {
        if( svs.clients[i].state >= CS_CONNECTED )
        {
            count++;
        }
    }
    
    if( count == 1 || count == sv_maxclients->integer )
    {
        serverCcmdsLocal.Heartbeat_f();
    }
}

/*
=====================
idServerClientSystemLocal::FreeClient

Destructor for data allocated in a client structure
=====================
*/
void idServerClientSystemLocal::FreeClient( client_t* client )
{
    serverNetChanSystem->NetchanFreeQueue( client );
    CloseDownload( client );
}

/*
=====================
idServerClientSystemLocal::DropClient

Called when the player is totally leaving the server, either willingly
or unwillingly.  This is NOT called if the entire server is quiting
or crashing -- idServerInitSystemLocal::FinalCommand() will handle that
=====================
*/
void idServerClientSystemLocal::DropClient( client_t* drop, pointer reason )
{
    sint i;
    challenge_t* challenge;
    bool isBot = false;
    
    if( drop->state == CS_ZOMBIE )
    {
        // already dropped
        return;
    }
    
#if 0
    if( drop->gentity && ( drop->gentity->r.svFlags & SVF_BOT ) )
    {
        isBot = true;
    }
    else
    {
        if( drop->netchan.remoteAddress.type == NA_BOT )
        {
            isBot = true;
        }
    }
#endif
    
    if( !isBot )
    {
        // see if we already have a challenge for this ip
        challenge = &svs.challenges[0];
        
        for( i = 0; i < MAX_CHALLENGES; i++, challenge++ )
        {
            if( networkSystem->CompareAdr( drop->netchan.remoteAddress, challenge->adr ) )
            {
                ::memset( challenge, 0, sizeof( *challenge ) );
                break;
            }
        }
        
        // Kill any download
        CloseDownload( drop );
    }
    
    // Free all allocated data on the client structure
    FreeClient( drop );
    
    if( ( !serverGameSystem->GameIsSinglePlayer() ) || ( !isBot ) )
    {
        // tell everyone why they got dropped
        // Gordon: we want this displayed elsewhere now
        //serverMainSystem->SendServerCommand(nullptr, "cpm \"%s" S_COLOR_WHITE " %s\n\"", drop->name, reason);
        //serverMainSystem->SendServerCommand( nullptr, "print \"[lof]%s" S_COLOR_WHITE " [lon]%s\n\"", drop->name, reason );
    }
    
    Com_DPrintf( "Going to CS_ZOMBIE for %s\n", drop->name );
    drop->state = CS_ZOMBIE;	// become free in a few seconds
    
    if( drop->download )
    {
        fileSystem->FCloseFile( drop->download );
        drop->download = 0;
    }
    
    // call the prog function for removing a client
    // this will remove the body, among other things
#ifndef UPDATE_SERVER
    sgame->ClientDisconnect( ARRAY_INDEX( svs.clients, drop ) );
#endif
    
    // add the disconnect command
    serverMainSystem->SendServerCommand( drop, va( "disconnect \"%s\"", reason ) );
    
    // nuke user info
    serverInitSystem->SetUserinfo( ARRAY_INDEX( svs.clients, drop ), "" );
    
#if !defined (UPDATE_SERVER)
    // OACS: Commit then reset the last interframe for this client
    idServerOACSSystemLocal::ExtendedRecordDropClient( drop - svs.clients );
#endif
    
    Com_DPrintf( "Going to CS_ZOMBIE for %s\n", drop->name );
    drop->state = CS_ZOMBIE; // become free in a few seconds
    
    // if this was the last client on the server, send a heartbeat
    // to the master so it is known the server is empty
    // send a heartbeat now so the master will get up to date info
    // if there is already a slot for this ip, reuse it
    
    sint players = 0;
    
    for( i = 0; i < sv_maxclients->integer; i++ )
    {
        if( svs.clients[i].state >= CS_CONNECTED && svs.clients[i].netchan.remoteAddress.type != NA_BOT )
        {
            players++;
        }
    }
    
    for( i = 0; i < sv_maxclients->integer; i++ )
    {
        if( svs.clients[i].state >= CS_CONNECTED )
        {
            break;
        }
    }
    
    if( i == sv_maxclients->integer )
    {
        serverCcmdsLocal.Heartbeat_f();
    }
    
    if( players == 0 )
    {
        svs.hibernation.lastTimeDisconnected = idsystem->Milliseconds();
    }
}

/*
================
idServerClientSystemLocal::SendClientGameState

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each new map load.

It will be resent if the client acknowledges a later message but has
the wrong gamestate.
================
*/
void idServerClientSystemLocal::SendClientGameState( client_t* client )
{
    sint start;
    entityState_t* base, nullstate;
    msg_t msg;
    uchar8 msgBuffer[MAX_MSGLEN];
    
    while( client->state && client->netchan.unsentFragments )
    {
        Com_DPrintf( "idServerClientSystemLocal::SendClientGameState [2] for %s, writing out old fragments\n", client->name );
        networkChainSystem->TransmitNextFragment( &client->netchan );
    }
    
    Com_DPrintf( "idServerClientSystemLocal::SendClientGameState() for %s\n", client->name );
    if( client->state != CS_PRIMED )
    {
        Com_DPrintf( "Going from CS_CONNECTED to CS_PRIMED for %s\n", client->name );
    }
    
    if( client->state == CS_CONNECTED )
    {
        client->state = CS_PRIMED;
    }
    client->pureAuthentic = false;
    client->pureReceived = false;
    
    // when we receive the first packet from the client, we will
    // notice that it is from a different serverid and that the
    // gamestate message was not just sent, forcing a retransmit
    client->gamestateMessageNum = client->netchan.outgoingSequence;
    
    MSG_Init( &msg, msgBuffer, sizeof( msgBuffer ) );
    
    // NOTE, MRE: all server->client messages now acknowledge
    // let the client know which reliable clientCommands we have received
    MSG_WriteLong( &msg, client->lastClientCommand );
    
    // send any server commands waiting to be sent first.
    // we have to do this cause we send the client->reliableSequence
    // with a gamestate and it sets the clc.serverCommandSequence at
    // the client side
    serverSnapshotSystem->UpdateServerCommandsToClient( client, &msg );
    
    // send the gamestate
    MSG_WriteByte( &msg, svc_gamestate );
    MSG_WriteLong( &msg, client->reliableSequence );
    
    // write the configstrings
    for( start = 0; start < MAX_CONFIGSTRINGS; start++ )
    {
        if( sv.configstrings[start].s[0] )
        {
            MSG_WriteByte( &msg, svc_configstring );
            MSG_WriteShort( &msg, start );
            MSG_WriteBigString( &msg, sv.configstrings[start].s );
        }
    }
    
    // write the baselines
    ::memset( &nullstate, 0, sizeof( nullstate ) );
    for( start = 0; start < MAX_GENTITIES; start++ )
    {
        base = &sv.svEntities[start].baseline;
        if( !base->number )
        {
            continue;
        }
        MSG_WriteByte( &msg, svc_baseline );
        MSG_WriteDeltaEntity( &msg, &nullstate, base, true );
    }
    
    MSG_WriteByte( &msg, svc_EOF );
    
    MSG_WriteLong( &msg, ARRAY_INDEX( svs.clients, client ) );
    
    // write the checksum feed
    MSG_WriteLong( &msg, sv.checksumFeed );
    
    if( msg.overflowed )
    {
        Com_Printf( "ERROR: gamestate message buffer overflow\n" );
    }
    
    // NERVE - SMF - debug info
    Com_DPrintf( "Sending %i bytes in gamestate to client: %li\n", msg.cursize, static_cast<sint32>( client - svs.clients ) );
    
    // deliver this to the client
    serverSnapshotSystem->SendMessageToClient( &msg, client );
}

/*
==================
idServerClientSystemLocal::ClientEnterWorld
==================
*/
void idServerClientSystemLocal::ClientEnterWorld( client_t* client, usercmd_t* cmd )
{
    sint clientNum;
    sharedEntity_t*	ent;
    
    // OACS: Set up the interframe for this client
    if( sv_oacsEnable->integer == 1 )
    {
#if !defined (UPDATE_SERVER)
        idServerOACSSystemLocal::ExtendedRecordClientConnect( client - svs.clients );
#endif
    }
    
    Com_DPrintf( "Going from CS_PRIMED to CS_ACTIVE for %s\n", client->name );
    client->state = CS_ACTIVE;
    
    // resend all configstrings using the cs commands since these are
    // no longer sent when the client is CS_PRIMED
    serverInitSystem->UpdateConfigStrings();
    
    // set up the entity for the client
    clientNum = ARRAY_INDEX( svs.clients, client );
    ent = serverGameSystem->GentityNum( clientNum );
    ent->s.number = clientNum;
    client->gentity = ent;
    
    client->lastUserInfoChange = 0;
    client->lastUserInfoCount = 0;
    
    client->deltaMessage = -1;
    client->nextSnapshotTime = svs.time; // generate a snapshot immediately
    if( cmd )
    {
        ::memcpy( &client->lastUsercmd, cmd, sizeof( client->lastUsercmd ) );
    }
    else
    {
        ::memset( &client->lastUsercmd, '\0', sizeof( client->lastUsercmd ) );
    }
    
    // call the game begin function
#ifndef UPDATE_SERVER
    sgame->ClientBegin( ARRAY_INDEX( svs.clients, client ) );
#endif
}

/*
============================================================
CLIENT COMMAND EXECUTION
============================================================
*/

/*
==================
idServerClientSystemLocal::CloseDownload

clear/free any download vars
==================
*/
void idServerClientSystemLocal::CloseDownload( client_t* cl )
{
    sint i;
    
    // EOF
    if( cl->download )
    {
        fileSystem->FCloseFile( cl->download );
    }
    cl->download = 0;
    *cl->downloadName = 0;
    
    // Free the temporary buffer space
    for( i = 0; i < MAX_DOWNLOAD_WINDOW; i++ )
    {
        if( cl->downloadBlocks[i] )
        {
            Z_Free( cl->downloadBlocks[i] );
            cl->downloadBlocks[i] = nullptr;
        }
    }
    
}

/*
==================
idServerClientSystemLocal::StopDownload_f

Abort a download if in progress
==================
*/
void idServerClientSystemLocal::StopDownload_f( client_t* cl )
{
    if( *cl->downloadName )
    {
        Com_DPrintf( "clientDownload: %d : file \"%s\" aborted\n", static_cast<sint>( cl - svs.clients ), cl->downloadName );
    }
    
    serverClientLocal.CloseDownload( cl );
}

/*
==================
idServerClientSystemLocal::DoneDownload_f

Downloads are finished
==================
*/
void idServerClientSystemLocal::DoneDownload_f( client_t* cl )
{
    if( cl->state == CS_ACTIVE )
    {
        return;
    }
    
    Com_DPrintf( "clientDownload: %s Done\n", cl->name );
    
    // resend the game state to update any clients that entered during the download
    SendClientGameState( cl );
}

/*
==================
idServerClientSystemLocal::NextDownload_f

The argument will be the last acknowledged block from the client, it should be
the same as cl->downloadClientBlock
==================
*/
void idServerClientSystemLocal::NextDownload_f( client_t* cl )
{
    sint block = atoi( cmdSystem->Argv( 1 ) );
    
    if( block == cl->downloadClientBlock )
    {
        Com_DPrintf( "clientDownload: %d : client acknowledge of block %d\n", static_cast<sint>( cl - svs.clients ), block );
        
        // Find out if we are done.  A zero-length block indicates EOF
        if( cl->downloadBlockSize[cl->downloadClientBlock % MAX_DOWNLOAD_WINDOW] == 0 )
        {
            Com_Printf( "clientDownload: %d : file \"%s\" completed\n", static_cast<sint>( cl - svs.clients ), cl->downloadName );
            serverClientLocal.CloseDownload( cl );
            return;
        }
        
        cl->downloadSendTime = svs.time;
        cl->downloadClientBlock++;
        
        return;
    }
    
    // We aren't getting an acknowledge for the correct block, drop the client
    // FIXME: this is bad... the client will never parse the disconnect message
    //        because the cgame isn't loaded yet
    serverClientLocal.DropClient( cl, "broken download" );
}

/*
==================
idServerClientSystemLocal::BeginDownload_f
==================
*/
void idServerClientSystemLocal::BeginDownload_f( client_t* cl )
{
    // Kill any existing download
    serverClientLocal.CloseDownload( cl );
    
    //bani - stop us from printing dupe messages
    if( strcmp( cl->downloadName, cmdSystem->Argv( 1 ) ) )
    {
        cl->downloadnotify = DLNOTIFY_ALL;
    }
    
    // cl->downloadName is non-zero now, idServerClientSystemLocal::WriteDownloadToClient will see this and open
    // the file itself
    Q_strncpyz( cl->downloadName, cmdSystem->Argv( 1 ), sizeof( cl->downloadName ) );
}

/*
==================
idServerClientSystemLocal::WWWDownload_f
==================
*/
void idServerClientSystemLocal::WWWDownload_f( client_t* cl )
{
    valueType* subcmd = cmdSystem->Argv( 1 );
    
    // only accept wwwdl commands for clients which we first flagged as wwwdl ourselves
    if( !cl->bWWWDl )
    {
        Com_Printf( "idServerClientSystemLocal::WWWDownload: unexpected wwwdl '%s' for client '%s'\n", subcmd, cl->name );
        serverClientLocal.DropClient( cl, "idServerClientSystemLocal::WWWDownload: unexpected wwwdl" );
        return;
    }
    
    if( !Q_stricmp( subcmd, "ack" ) )
    {
        if( cl->bWWWing )
        {
            Com_Printf( "WARNING: dupe wwwdl ack from client '%s'\n", cl->name );
        }
        cl->bWWWing = true;
        return;
    }
    else if( !Q_stricmp( subcmd, "bbl8r" ) )
    {
        serverClientLocal.DropClient( cl, "acking disconnected download mode" );
        return;
    }
    
    // below for messages that only happen during/after download
    if( !cl->bWWWing )
    {
        Com_Printf( "idServerClientSystemLocal::WWWDownload: unexpected wwwdl '%s' for client '%s'\n", subcmd, cl->name );
        serverClientLocal.DropClient( cl, "idServerClientSystemLocal::WWWDownload: unexpected wwwdl" );
        return;
    }
    
    if( !Q_stricmp( subcmd, "done" ) )
    {
        cl->download = 0;
        *cl->downloadName = 0;
        cl->bWWWing = false;
        return;
    }
    else if( !Q_stricmp( subcmd, "fail" ) )
    {
        cl->download = 0;
        *cl->downloadName = 0;
        cl->bWWWing = false;
        cl->bFallback = true;
        
        // send a reconnect
        SendClientGameState( cl );
        return;
    }
    else if( !Q_stricmp( subcmd, "chkfail" ) )
    {
        Com_Printf( "WARNING: client '%s' reports that the redirect download for '%s' had wrong checksum.\n", cl->name, cl->downloadName );
        Com_Printf( "         you should check your download redirect configuration.\n" );
        cl->download = 0;
        *cl->downloadName = 0;
        cl->bWWWing = false;
        cl->bFallback = true;
        
        // send a reconnect
        SendClientGameState( cl );
        return;
    }
    
    Com_Printf( "idServerClientSystemLocal::WWWDownload: unknown wwwdl subcommand '%s' for client '%s'\n", subcmd, cl->name );
    serverClientLocal.DropClient( cl, va( "idServerClientSystemLocal::WWWDownload: unknown wwwdl subcommand '%s'", subcmd ) );
}

/*
==================
idServerClientSystemLocal::CheckFallbackURL
abort an attempted download
==================
*/
void idServerClientSystemLocal::BadDownload( client_t* cl, msg_t* msg )
{
    MSG_WriteByte( msg, svc_download );
    MSG_WriteShort( msg, 0 ); // client is expecting block zero
    MSG_WriteLong( msg, -1 ); // illegal file size
    
    *cl->downloadName = 0;
}

/*
==================
idServerClientSystemLocal::CheckFallbackURL

sv_wwwFallbackURL can be used to redirect clients to a web URL in case direct ftp/http didn't work (or is disabled on client's end)
return true when a redirect URL message was filled up
when the cvar is set to something, the download server will effectively never use a legacy download strategy
==================
*/
bool idServerClientSystemLocal::CheckFallbackURL( client_t* cl, msg_t* msg )
{
    if( !sv_wwwFallbackURL->string || strlen( sv_wwwFallbackURL->string ) == 0 )
    {
        return false;
    }
    
    Com_Printf( "clientDownload: sending client '%s' to fallback URL '%s'\n", cl->name, sv_wwwFallbackURL->string );
    
    MSG_WriteByte( msg, svc_download );
    MSG_WriteShort( msg, -1 ); // block -1 means ftp/http download
    MSG_WriteString( msg, sv_wwwFallbackURL->string );
    MSG_WriteLong( msg, 0 );
    MSG_WriteLong( msg, 2 ); // DL_FLAG_URL
    
    return true;
}

/*
==================
idServerClientSystemLocal::WWriteDownloadToClient

Check to see if the client wants a file, open it if needed and start pumping the client
Fill up msg with data
==================
*/
void idServerClientSystemLocal::WriteDownloadToClient( client_t* cl, msg_t* msg )
{
    sint curindex, rate, blockspersnap, idPack, download_flag;
    valueType errorMessage[1024];
#if defined (UPDATE_SERVER)
    sint i;
    valueType testname[MAX_QPATH];
#endif
    
    bool bTellRate = false; // verbosity
    
    if( !*cl->downloadName )
    {
        // Nothing being downloaded
        return;
    }
    
    if( cl->bWWWing )
    {
        // The client acked and is downloading with ftp/http
        return;
    }
    
#ifndef UPDATE_SERVER
    // CVE-2006-2082
    // validate the download against the list of pak files
    if( !fileSystem->VerifyPak( cl->downloadName ) )
    {
        // will drop the client and leave it hanging on the other side. good for him
        DropClient( cl, "illegal download request" );
        return;
    }
#endif
    
    if( !cl->download )
    {
        cl->download = 0;
        
        // We open the file here
        // Update server only allows files that are in versionmap.cfg to download
#if defined (UPDATE_SERVER)
        for( i = 0; i < numVersions; i++ )
        {
            strcpy( testname, "updates/" );
            Q_strcat( testname, MAX_QPATH, versionMap[i].installer );
            
            if( !Q_stricmp( cl->downloadName, testname ) )
            {
                break;
            }
        }
        
        if( i == numVersions )
        {
            MSG_WriteByte( msg, svc_download );
            MSG_WriteShort( msg, 0 ); // client is expecting block zero
            MSG_WriteLong( msg, -1 ); // illegal file size
            
            Q_vsprintf_s( errorMessage, sizeof( errorMessage ), sizeof( errorMessage ), "Invalid download from update server" );
            MSG_WriteString( msg, errorMessage );
            
            *cl->downloadName = 0;
            
            DropClient( cl, "Invalid download from update server" );
            return;
        }
#endif
        
        //bani - prevent duplicate download notifications
        if( cl->downloadnotify & DLNOTIFY_BEGIN )
        {
            cl->downloadnotify &= ~DLNOTIFY_BEGIN;
            Com_Printf( "clientDownload: %d : beginning \"%s\"\n", static_cast<sint>( cl - svs.clients ), cl->downloadName );
        }
        
        idPack = fileSystem->idPak( cl->downloadName, BASEGAME );
        
        // sv_allowDownload and idPack checks
        if( !sv_allowDownload->integer || idPack )
        {
            // cannot auto-download file
            if( !::strstr( cl->downloadName, ".pk3" ) )
            {
                Com_Printf( "Not a pk3 %s\n", cl->downloadName );
                Q_vsprintf_s( errorMessage, sizeof( errorMessage ), sizeof( errorMessage ), "Cannot download non-pk3 \"%s\"\n", cl->downloadName );
            }
            else if( ::strstr( cl->downloadName, ".." ) )
            {
                Com_Printf( "Folder hack? %s\n", cl->downloadName );
                Q_vsprintf_s( errorMessage, sizeof( errorMessage ), sizeof( errorMessage ), "Cannot download bad folder \"%s\"\n", cl->downloadName );
            }
            else if( idPack )
            {
                Com_Printf( "clientDownload: %d : \"%s\" cannot download id pk3 files\n", static_cast<sint>( cl - svs.clients ), cl->downloadName );
                Q_vsprintf_s( errorMessage, sizeof( errorMessage ), sizeof( errorMessage ), "Cannot autodownload official pk3 file \"%s\"", cl->downloadName );
            }
            else
            {
                Com_Printf( "clientDownload: %d : \"%s\" download disabled", static_cast<sint>( cl - svs.clients ), cl->downloadName );
                
                if( sv_pure->integer )
                {
                    Q_vsprintf_s( errorMessage, sizeof( errorMessage ), sizeof( errorMessage ),
                                  "Could not download \"%s\" because autodownloading is disabled on the server.\n\n"
                                  "You will need to get this file elsewhere before you " "can connect to this pure server.\n",
                                  cl->downloadName );
                }
                else
                {
                    Q_vsprintf_s( errorMessage, sizeof( errorMessage ), sizeof( errorMessage ),
                                  "Could not download \"%s\" because autodownloading is disabled on the server.\n\n"
                                  "Set autodownload to No in your settings and you might be "
                                  "able to connect even if you don't have the file.\n", cl->downloadName );
                }
            }
            
            BadDownload( cl, msg );
            MSG_WriteString( msg, errorMessage );	// (could SV_DropClient isntead?)
            
            return;
        }
        
        // www download redirect protocol
        // NOTE: this is called repeatedly while a client connects. Maybe we should sort of cache the message or something
        // FIXME: we need to abstract this to an independant module for maximum configuration/usability by server admins
        // FIXME: I could rework that, it's crappy
        if( sv_wwwDownload->integer )
        {
            if( cl->bDlOK )
            {
                if( !cl->bFallback )
                {
                    fileHandle_t handle;
                    sint downloadSize = fileSystem->SV_FOpenFileRead( cl->downloadName, &handle );
                    
                    if( downloadSize )
                    {
                        fileSystem->FCloseFile( handle );	// don't keep open, we only care about the size
                        
                        Q_strncpyz( cl->downloadURL, va( "%s/%s", sv_wwwBaseURL->string, cl->downloadName ), sizeof( cl->downloadURL ) );
                        
                        //bani - prevent multiple download notifications
                        if( cl->downloadnotify & DLNOTIFY_REDIRECT )
                        {
                            cl->downloadnotify &= ~DLNOTIFY_REDIRECT;
                            Com_Printf( "Redirecting client '%s' to %s\n", cl->name, cl->downloadURL );
                        }
                        
                        // once cl->downloadName is set (and possibly we have our listening socket), let the client know
                        cl->bWWWDl = true;
                        MSG_WriteByte( msg, svc_download );
                        MSG_WriteShort( msg, -1 ); // block -1 means ftp/http download
                        
                        // compatible with legacy svc_download protocol: [size] [size bytes]
                        // download URL, size of the download file, download flags
                        MSG_WriteString( msg, cl->downloadURL );
                        MSG_WriteLong( msg, downloadSize );
                        download_flag = 0;
                        
                        if( sv_wwwDlDisconnected->integer )
                        {
                            download_flag |= ( 1 << DL_FLAG_DISCON );
                        }
                        
                        MSG_WriteLong( msg, download_flag ); // flags
                        
                        return;
                    }
                    else
                    {
                        // that should NOT happen - even regular download would fail then anyway
                        Com_Printf( "ERROR: Client '%s': couldn't extract file size for %s\n", cl->name, cl->downloadName );
                    }
                }
                else
                {
                    cl->bFallback = false;
                    
                    if( CheckFallbackURL( cl, msg ) )
                    {
                        return;
                    }
                    
                    Com_Printf( "Client '%s': falling back to regular downloading for failed file %s\n", cl->name, cl->downloadName );
                }
            }
            else
            {
                if( CheckFallbackURL( cl, msg ) )
                {
                    return;
                }
                Com_Printf( "Client '%s' is not configured for www download\n", cl->name );
            }
        }
        
        // find file
        cl->bWWWDl = false;
        cl->downloadSize = fileSystem->SV_FOpenFileRead( cl->downloadName, &cl->download );
        
        if( cl->downloadSize <= 0 )
        {
            Com_Printf( "clientDownload: %d : \"%s\" file not found on server\n", static_cast<sint>( cl - svs.clients ), cl->downloadName );
            Q_vsprintf_s( errorMessage, sizeof( errorMessage ), sizeof( errorMessage ), "File \"%s\" not found on server for autodownloading.\n", cl->downloadName );
            
            BadDownload( cl, msg );
            
            MSG_WriteString( msg, errorMessage ); // (could SV_DropClient isntead?)
            return;
        }
        
        // is valid source, init
        cl->downloadCurrentBlock = cl->downloadClientBlock = cl->downloadXmitBlock = 0;
        cl->downloadCount = 0;
        cl->downloadEOF = false;
        
        bTellRate = true;
    }
    
    // Perform any reads that we need to
    while( cl->downloadCurrentBlock - cl->downloadClientBlock < MAX_DOWNLOAD_WINDOW && cl->downloadSize != cl->downloadCount )
    {
        curindex = ( cl->downloadCurrentBlock % MAX_DOWNLOAD_WINDOW );
        
        if( !cl->downloadBlocks[curindex] )
        {
            cl->downloadBlocks[curindex] = static_cast<uchar8*>( Z_Malloc( MAX_DOWNLOAD_BLKSIZE ) );
        }
        
        cl->downloadBlockSize[curindex] = fileSystem->Read( cl->downloadBlocks[curindex], MAX_DOWNLOAD_BLKSIZE, cl->download );
        
        if( cl->downloadBlockSize[curindex] < 0 )
        {
            // EOF right now
            cl->downloadCount = cl->downloadSize;
            break;
        }
        
        cl->downloadCount += cl->downloadBlockSize[curindex];
        
        // Load in next block
        cl->downloadCurrentBlock++;
    }
    
    // Check to see if we have eof condition and add the EOF block
    if( cl->downloadCount == cl->downloadSize && !cl->downloadEOF && cl->downloadCurrentBlock - cl->downloadClientBlock < MAX_DOWNLOAD_WINDOW )
    {
        cl->downloadBlockSize[cl->downloadCurrentBlock % MAX_DOWNLOAD_WINDOW] = 0;
        cl->downloadCurrentBlock++;
        
        cl->downloadEOF = true;	// We have added the EOF block
    }
    
    // Loop up to window size times based on how many blocks we can fit in the
    // client snapMsec and rate
    
    // based on the rate, how many bytes can we fit in the snapMsec time of the client
    // normal rate / snapshotMsec calculation
    rate = cl->rate;
    
    // show_bug.cgi?id=509
    // for autodownload, we use a seperate max rate value
    // we do this everytime because the client might change it's rate during the download
    if( sv_dl_maxRate->integer < rate )
    {
        rate = sv_dl_maxRate->integer;
        
        if( bTellRate )
        {
            Com_Printf( "'%s' downloading at sv_dl_maxrate (%d)\n", cl->name, sv_dl_maxRate->integer );
        }
    }
    else if( bTellRate )
    {
        Com_Printf( "'%s' downloading at rate %d\n", cl->name, rate );
    }
    
    if( !rate )
    {
        blockspersnap = 1;
    }
    else
    {
        blockspersnap = ( ( rate * cl->snapshotMsec ) / 1000 + MAX_DOWNLOAD_BLKSIZE ) / MAX_DOWNLOAD_BLKSIZE;
    }
    
    if( blockspersnap < 0 )
    {
        blockspersnap = 1;
    }
    
    while( blockspersnap-- )
    {
        // Write out the next section of the file, if we have already reached our window,
        // automatically start retransmitting
        if( cl->downloadClientBlock == cl->downloadCurrentBlock )
        {
            return; // Nothing to transmit
            
        }
        if( cl->downloadXmitBlock == cl->downloadCurrentBlock )
        {
            // We have transmitted the complete window, should we start resending?
            
            //FIXME:  This uses a hardcoded one second timeout for lost blocks
            //the timeout should be based on client rate somehow
            if( svs.time - cl->downloadSendTime > 1000 )
            {
                cl->downloadXmitBlock = cl->downloadClientBlock;
            }
            else
            {
                return;
            }
        }
        
        // Send current block
        curindex = ( cl->downloadXmitBlock % MAX_DOWNLOAD_WINDOW );
        
        MSG_WriteByte( msg, svc_download );
        MSG_WriteShort( msg, cl->downloadXmitBlock );
        
        // block zero is special, contains file size
        if( cl->downloadXmitBlock == 0 )
        {
            MSG_WriteLong( msg, cl->downloadSize );
        }
        
        MSG_WriteShort( msg, cl->downloadBlockSize[curindex] );
        
        // Write the block
        if( cl->downloadBlockSize[curindex] )
        {
            MSG_WriteData( msg, cl->downloadBlocks[curindex], cl->downloadBlockSize[curindex] );
        }
        
        Com_DPrintf( "clientDownload: %d : writing block %d\n", static_cast<sint>( cl - svs.clients ), cl->downloadXmitBlock );
        
        // Move on to the next block
        // It will get sent with next snap shot.  The rate will keep us in line.
        cl->downloadXmitBlock++;
        
        cl->downloadSendTime = svs.time;
    }
}

/*
=================
idServerClientSystemLocal::Disconnect_f

The client is going to disconnect, so remove the connection immediately  FIXME: move to game?
=================
*/
void idServerClientSystemLocal::Disconnect_f( client_t* cl )
{
    if( cmdSystem->Argc() > 1 )
    {
        valueType reason[MAX_STRING_CHARS] = { 0 };
        Q_strncpyz( reason, cmdSystem->Argv( 1 ), sizeof( reason ) );
        Q_strstrip( reason, "\r\n;\"", nullptr );
        serverClientLocal.DropClient( cl, "disconnected" );
        ( cl, va( "disconnected: %s", reason ) );
    }
    else
    {
        serverClientLocal.DropClient( cl, "disconnected" );
    }
}

/*
=================
idServerClientSystemLocal::VerifyPaks_f

If we are pure, disconnect the client if they do no meet the following conditions:

1. the first two checksums match our view of cgame and gui DLLs
   Wolf specific: the checksum is the checksum of the pk3 we found the DLL in
2. there are no any additional checksums that we do not have

This routine would be a bit simpler with a goto but I abstained
=================
*/
void idServerClientSystemLocal::VerifyPaks_f( client_t* cl )
{
    sint nChkSum1, nChkSum2, nClientPaks, nServerPaks, i, j, nClientChkSum[1024], nServerChkSum[1024];
    pointer pPaks, pArg;
    
    // if we are pure, we "expect" the client to load certain things from
    // certain pk3 files, namely we want the client to have loaded the
    // ui and cgame that we think should be loaded based on the pure setting
    if( sv_pure->integer != 0 )
    {
        bool bGood = true;
        nChkSum1 = nChkSum2 = 0;
        
        bGood = ( bool )( fileSystem->FileIsInPAK( idsystem->GetDLLName( "cgame" ), &nChkSum1 ) == 1 );
        if( bGood )
        {
            bGood = ( bool )( fileSystem->FileIsInPAK( idsystem->GetDLLName( "gui" ), &nChkSum2 ) == 1 );
            
        }
        
        nClientPaks = cmdSystem->Argc();
        
        if( nClientPaks > ARRAY_LEN( nClientChkSum ) )
        {
            nClientPaks = ARRAY_LEN( nClientChkSum );
        }
        
        // start at arg 2 ( skip serverId cl_paks )
        sint nCurArg = 1;
        
        pArg = cmdSystem->Argv( nCurArg++ );
        
        if( !pArg )
        {
            bGood = false;
        }
        else
        {
            // show_bug.cgi?id=475
            // we may get incoming cp sequences from a previous checksumFeed, which we need to ignore
            // since serverId is a frame count, it always goes up
            if( ::atoi( pArg ) < sv.checksumFeedServerId )
            {
                Com_DPrintf( "ignoring outdated cp command from client %s\n", cl->name );
                return;
            }
        }
        
        // we basically use this while loop to avoid using 'goto' :)
        while( bGood )
        {
            // must be at least 6: "cl_paks cgame gui @ firstref ... numChecksums"
            // numChecksums is encoded
            if( nClientPaks < 6 )
            {
                bGood = false;
                break;
            }
            
            // verify first to be the cgame checksum
            pArg = cmdSystem->Argv( nCurArg++ );
            if( !pArg || *pArg == '@' || ::atoi( pArg ) != nChkSum1 )
            {
                Com_Printf( "nChkSum1 %d == %d\n", ::atoi( pArg ), nChkSum1 );
                bGood = false;
                break;
            }
            
            // verify the second to be the gui checksum
            pArg = cmdSystem->Argv( nCurArg++ );
            if( !pArg || *pArg == '@' || ::atoi( pArg ) != nChkSum2 )
            {
                Com_Printf( "nChkSum2 %d == %d\n", ::atoi( pArg ), nChkSum2 );
                bGood = false;
                break;
            }
            
            // should be sitting at the delimeter now
            pArg = cmdSystem->Argv( nCurArg++ );
            if( *pArg != '@' )
            {
                bGood = false;
                break;
            }
            
            // store checksums since tokenization is not re-entrant
            for( i = 0; nCurArg < nClientPaks; i++ )
            {
                nClientChkSum[i] = ::atoi( cmdSystem->Argv( nCurArg++ ) );
            }
            
            // store number to compare against (minus one cause the last is the number of checksums)
            nClientPaks = i - 1;
            
            // make sure none of the client check sums are the same
            // so the client can't send 5 the same checksums
            for( i = 0; i < nClientPaks; i++ )
            {
                for( j = 0; j < nClientPaks; j++ )
                {
                    if( i == j )
                    {
                        continue;
                    }
                    
                    if( nClientChkSum[i] == nClientChkSum[j] )
                    {
                        bGood = false;
                        break;
                    }
                }
                
                if( bGood == false )
                {
                    break;
                }
            }
            
            if( bGood == false )
            {
                break;
            }
            
            // get the pure checksums of the pk3 files loaded by the server
            pPaks = fileSystem->LoadedPakPureChecksums();
            cmdSystem->TokenizeString( pPaks );
            nServerPaks = cmdSystem->Argc();
            
            if( nServerPaks > 1024 )
            {
                nServerPaks = 1024;
            }
            
            for( i = 0; i < nServerPaks; i++ )
            {
                nServerChkSum[i] = ::atoi( cmdSystem->Argv( i ) );
            }
            
            // check if the client has provided any pure checksums of pk3 files not loaded by the server
            for( i = 0; i < nClientPaks; i++ )
            {
                for( j = 0; j < nServerPaks; j++ )
                {
                    if( nClientChkSum[i] == nServerChkSum[j] )
                    {
                        break;
                    }
                }
                
                if( j > nServerPaks )
                {
                    bGood = false;
                    break;
                }
            }
            
            if( bGood == false )
            {
                break;
            }
            
            // check if the number of checksums was correct
            nChkSum1 = sv.checksumFeed;
            
            for( i = 0; i < nClientPaks; i++ )
            {
                nChkSum1 ^= nClientChkSum[i];
            }
            
            nChkSum1 ^= nClientPaks;
            
            if( nChkSum1 != nClientChkSum[nClientPaks] )
            {
                bGood = false;
                break;
            }
            
            // break out
            break;
        }
        
        cl->pureReceived = true;
        
        if( bGood )
        {
            cl->pureAuthentic = true;
        }
        else
        {
            cl->pureAuthentic = false;
            cl->nextSnapshotTime = -1;
            cl->state = CS_ACTIVE;
            
            serverSnapshotSystemLocal.SendClientSnapshot( cl );
            serverMainSystem->SendServerCommand( cl, "disconnect \"%s\"", "This is a pure server. This is caused by corrupted or missing files. Try turning on AutoDownload." );
            serverMainSystem->SendServerCommand( cl, "Unpure client detected. Invalid .PK3 files referenced!" );
        }
    }
}

/*
=================
idServerClientSystemLocal::ResetPureClient_f
=================
*/
void idServerClientSystemLocal::ResetPureClient_f( client_t* cl )
{
    cl->pureAuthentic = false;
    cl->pureReceived = false;
}

/*
==================
idServerClientSystemLocal::CheckFunstuffExploit

Makes sure each comma-separated token of the specified userinfo key
is at most 13 characters to protect the game against buffer overflow.
==================
*/
bool idServerClientSystemLocal::CheckFunstuffExploit( valueType* userinfo, valueType* key )
{
    valueType* token = Info_ValueForKey( userinfo, key );
    
    if( !token )
    {
        return false;
    }
    
    while( token && *token )
    {
        sint len;
        valueType* next = strchr( token, ',' );
        
        if( next == nullptr )
        {
            len = strlen( token );
            token = nullptr;
        }
        else
        {
            len = next - token;
            token = next + 1;
        }
        
        if( len > 13 )
        {
            Info_SetValueForKey( userinfo, key, "" );
            return true;
        }
    }
    
    return false;
}

/*
=================
idServerClientSystemLocal::UserinfoChanged

Pull specific info from a newly changed userinfo string
into a more C friendly form.
=================
*/
void idServerClientSystemLocal::UserinfoChanged( client_t* cl )
{
    valueType* val;
    sint i;
    
    // In the ugly [commented out] code below, handicap is supposed to be
    // either missing or a valid sint between 1 and 100.
    // It's safe therefore to stick with that policy and just remove it.
    // ET never uses handicap anyways.  Unfortunately it's possible
    // to have a key such as handicap appear more than once in the userinfo.
    // So we remove every instance of it.
    sint oldInfoLen = strlen( cl->userinfo );
    sint newInfoLen;
    
    while( true )
    {
        Info_RemoveKey( cl->userinfo, "handicap" );
        newInfoLen = strlen( cl->userinfo );
        if( oldInfoLen == newInfoLen )
        {
            break;    // userinfo wasn't modified.
        }
        oldInfoLen = newInfoLen;
    }
    
    idServerCommunityServer::userInfoChanged( cl );
    
    // name for C code
    Q_strncpyz( cl->name, Info_ValueForKey( cl->userinfo, "name" ), sizeof( cl->name ) );
    
    // rate command
    // if the client is on the same subnet as the server and we aren't running an
    // internet public server, assume they don't need a rate choke
    if( networkSystem->IsLANAddress( cl->netchan.remoteAddress ) && com_dedicated->integer != 2 && sv_lanForceRate->integer == 1 )
    {
        cl->rate = 99999; // lans should not rate limit
    }
    else
    {
        val = Info_ValueForKey( cl->userinfo, "rate" );
        if( strlen( val ) )
        {
            i = ::atoi( val );
            cl->rate = i;
            
            if( cl->rate < 1000 )
            {
                cl->rate = 1000;
            }
            else if( cl->rate > 90000 )
            {
                cl->rate = 90000;
            }
        }
        else
        {
            cl->rate = 5000;
        }
    }
    
    val = Info_ValueForKey( cl->userinfo, "handicap" );
    if( strlen( val ) )
    {
        i = ::atoi( val );
        if( i <= -100 || i > 100 || strlen( val ) > 4 )
        {
            Info_SetValueForKey( cl->userinfo, "handicap", "0" );
        }
    }
    
    // snaps command
    val = Info_ValueForKey( cl->userinfo, "snaps" );
    
    if( val[0] && !networkSystem->IsLocalAddress( cl->netchan.remoteAddress ) )
    {
        i = ::atoi( val );
    }
    else
    {
        i = sv_fps->integer; // sync with server
    }
    
    // range check
    if( i < 1 )
    {
        i = 1;
    }
    else if( i > sv_fps->integer )
    {
        i = sv_fps->integer;
    }
    
    i = 1000 / i; // from FPS to milliseconds
    
    if( i != cl->snapshotMsec )
    {
        // Reset next snapshot so we avoid desync between server frame time and snapshot send time
        cl->nextSnapshotTime = -1;
        cl->snapshotMsec = i;
    }
    
    // TTimo
    // maintain the IP information
    // this is set in idServerClientSystemLocal::DirectConnect (directly on the server, not transmitted), may be lost when client updates it's userinfo
    // the banning code relies on this being consistently present
    // zinx - modified to always keep this consistent, instead of only
    // when "ip" is 0-length, so users can't supply their own IP
    
    //Com_DPrintf("Maintain IP in userinfo for '%s'\n", cl->name);
    
    if( !networkSystem->IsLocalAddress( cl->netchan.remoteAddress ) )
    {
        Info_SetValueForKey( cl->userinfo, "ip", networkSystem->AdrToString( cl->netchan.remoteAddress ) );
    }
    else
    {
        // force the "ip" info key to "localhost" for local clients
        Info_SetValueForKey( cl->userinfo, "ip", "localhost" );
    }
    
    Info_SetValueForKey( cl->userinfo, "authenticated", va( "%i", cl->authenticated ) );
    Info_SetValueForKey( cl->userinfo, "cl_guid", cl->guid );
    
    // TTimo
    // download prefs of the client
    val = Info_ValueForKey( cl->userinfo, "cl_wwwDownload" );
    cl->bDlOK = false;
    
    if( strlen( val ) )
    {
        i = ::atoi( val );
        if( i != 0 )
        {
            cl->bDlOK = true;
        }
    }
    
    val = Info_ValueForKey( cl->userinfo, "cl_guid" );
    
    for( i = 0; i < strlen( val ); i++ )
    {
        if( !isalnum( val[i] ) )
        {
            Info_SetValueForKey( cl->userinfo, "cl_guid", "" );
            Com_Printf( "Cleared malformed cl_guid from %s.\n", networkSystem->AdrToString( cl->netchan.remoteAddress ) );
            break;
        }
    }
    
    if( CheckFunstuffExploit( cl->userinfo, "funfree" ) ||
            CheckFunstuffExploit( cl->userinfo, "funred" ) ||
            CheckFunstuffExploit( cl->userinfo, "funblue" ) )
    {
        Com_Printf( "funstuff exploit attempt from %s\n", networkSystem->AdrToString( cl->netchan.remoteAddress ) );
    }
}

/*
==================
idServerClientSystemLocal::UpdateUserinfo_f
==================
*/
#define INFO_CHANGE_MIN_INTERVAL	6000 //6 seconds is reasonable I suppose
#define INFO_CHANGE_MAX_COUNT		3 //only allow 3 changes within the 6 seconds
void idServerClientSystemLocal::UpdateUserinfo_f( client_t* cl )
{
    valueType* arg = cmdSystem->Argv( 1 ), info[MAX_INFO_STRING];;
    
    // Stop random empty /userinfo calls without hurting anything
    if( !arg || !*arg )
    {
        return;
    }
    
    if( ( sv_floodProtect->integer ) && ( cl->state >= CS_ACTIVE ) && ( svs.time < cl->nextReliableUserTime ) )
    {
        Q_strncpyz( cl->userinfobuffer, arg, sizeof( cl->userinfobuffer ) );
        serverMainSystem->SendServerCommand( cl, "print \"^7Command ^1delayed^7 due to sv_floodprotect.\"" );
        return;
    }
    cl->userinfobuffer[0] = 0;
    cl->nextReliableUserTime = svs.time + 5000;
    
    Q_strncpyz( cl->userinfo, arg, sizeof( cl->userinfo ) );
    if( cl->lastUserInfoChange > svs.time )
    {
        cl->lastUserInfoCount++;
        
        if( cl->lastUserInfoCount >= INFO_CHANGE_MAX_COUNT )
        {
            Q_strncpyz( cl->userinfoPostponed, arg, sizeof( cl->userinfoPostponed ) );
            serverMainSystem->SendServerCommand( cl, "print \"Warning: Too many info changes, last info postponed\n\"\n" );
            return;
        }
    }
    else
    {
        cl->userinfoPostponed[0] = 0;
        cl->lastUserInfoCount = 0;
        cl->lastUserInfoChange = svs.time + INFO_CHANGE_MIN_INTERVAL;
    }
    
    serverClientLocal.UserinfoChanged( cl );
    
    // call prog code to allow overrides
#ifndef UPDATE_SERVER
    sgame->ClientUserinfoChanged( cl - svs.clients );
#endif
    
    // get the name out of the game and set it in the engine
    serverInitSystem->GetConfigstring( CS_PLAYERS + ( cl - svs.clients ), info, sizeof( info ) );
    Info_SetValueForKey( cl->userinfo, "name", Info_ValueForKey( info, "n" ) );
    Q_strncpyz( cl->name, Info_ValueForKey( info, "n" ), sizeof( cl->name ) );
}

typedef struct
{
    valueType* name;
    void ( *func )( client_t* cl );
    bool allowedpostmapchange;
} ucmd_t;

static ucmd_t ucmds[] =
{
    {"userinfo", &idServerClientSystemLocal::UpdateUserinfo_f, false},
    {"disconnect", &idServerClientSystemLocal::Disconnect_f, true},
    {"cp", &idServerClientSystemLocal::VerifyPaks_f, false},
    {"vdr", &idServerClientSystemLocal::ResetPureClient_f, false},
    {"download", &idServerClientSystemLocal::BeginDownload_f, false},
    {"nextdl", &idServerClientSystemLocal::NextDownload_f, false},
    {"stopdl", &idServerClientSystemLocal::StopDownload_f, false},
    {"donedl", &idServerClientSystemLocal::DoneDownload_f, false},
    {"wwwdl", &idServerClientSystemLocal::WWWDownload_f, false},
    {"cheater", &idServerOACSSystemLocal::ExtendedRecordSetCheaterFromClient_f, false},
    {"honest", &idServerOACSSystemLocal::ExtendedRecordSetHonestFromClient_f, false},
    {nullptr, nullptr}
};

/*
==================
idServerClientSystemLocal::ExecuteClientCommand

Also called by bot code
==================
*/
void idServerClientSystemLocal::ExecuteClientCommand( client_t* cl, pointer s, bool clientOK, bool premaprestart )
{
    ucmd_t* u;
    bool bProcessed = false;
    sint argsFromOneMaxlen, charCount, dollarCount, i;
    valueType* arg;
    bool exploitDetected;
    bool foundCommand = false;
    
    Com_DPrintf( "idServerClientSystemLocal::ExecuteClientCommand: %s\n", s );
    
    cmdSystem->TokenizeString( s );
    
    if( idServerCommunityServer::checkClientCommandPermitions( cl, s ) != CS_OK )
    {
        clientOK = false;
    }
    
    // see if it is a server level command
    for( u = ucmds; u->name; u++ )
    {
        if( !strcmp( cmdSystem->Argv( 0 ), u->name ) )
        {
            if( premaprestart && !u->allowedpostmapchange )
            {
                continue;
            }
            foundCommand = true;
            u->func( cl );
            bProcessed = true;
            break;
        }
    }
    
    if( clientOK )
    {
        // pass unknown strings to the	game
        if( !u->name && sv.state == SS_GAME )
        {
            argsFromOneMaxlen = -1;
            
            // q3cbufexec fix
            if( ( !Q_stricmp( cmdSystem->Argv( 0 ), "callvote" ) || !Q_stricmp( cmdSystem->Argv( 0 ), "callteamvote" ) ) )
            {
                if( strpbrk( cmdSystem->Argv( 1 ), ";\n\r" ) || strpbrk( cmdSystem->Argv( 2 ), ";\n\r" ) )
                {
                    Com_Printf( "Callvote from %s (client #%i, %s): %s\n", cl->name, static_cast<sint>( cl - svs.clients ),
                                networkSystem->AdrToString( cl->netchan.remoteAddress ), cmdSystem->Args() );
                    return;
                }
            }
            
            // teamcmd crash fix
            if( !Q_stricmp( cmdSystem->Argv( 0 ), "team" ) && ( !Q_stricmp( cmdSystem->Argv( 1 ), "follow1" ) || !Q_stricmp( cmdSystem->Argv( 1 ), "follow2" ) ) )
            {
                return;
            }
            
            if( !Q_stricmp( "stats", cmdSystem->Argv( 0 ) ) )
            {
                if( cl - svs.clients != serverGameSystem->GameClientNum( cl - svs.clients )->clientNum )
                {
                    Com_Printf( "Stats command exploit attempt from %s\n", networkSystem->AdrToString( cl->netchan.remoteAddress ) );
                    serverMainSystem->SendServerCommand( cl, "print \"^7Stats command exploit attempt detected. This has been ^1reported^7.\n\"" );
                    return;
                }
            }
            
            // q3cbufexec fix
            if( Q_stricmp( "say", cmdSystem->Argv( 0 ) ) == 0 || Q_stricmp( "say_team",
                    cmdSystem->Argv( 0 ) ) == 0 || Q_stricmp( "say_fireteam", cmdSystem->Argv( 0 ) ) == 0 )
            {
                argsFromOneMaxlen = MAX_SAY_STRLEN;
            }
            else if( Q_stricmp( "tell", cmdSystem->Argv( 0 ) ) == 0 )
            {
                // A command will look like "tell 12 hi" or "tell foo hi".  The "12"
                // and "foo" in the examples will be counted towards MAX_SAY_STRLEN,
                // plus the space.
                argsFromOneMaxlen = MAX_SAY_STRLEN;
            }
            
            if( argsFromOneMaxlen >= 0 )
            {
                exploitDetected = false;
                charCount = 0;
                dollarCount = 0;
                
                for( i = cmdSystem->Argc() - 1; i >= 1; i-- )
                {
                    arg = cmdSystem->Argv( i );
                    
                    while( *arg )
                    {
                        if( ++charCount > argsFromOneMaxlen )
                        {
                            exploitDetected = true;
                            break;
                        }
                        
                        if( *arg == '$' )
                        {
                            if( ++dollarCount > MAX_DOLLAR_VARS )
                            {
                                exploitDetected = true;
                                break;
                            }
                            
                            charCount += STRLEN_INCREMENT_PER_DOLLAR_VAR;
                            
                            if( charCount > argsFromOneMaxlen )
                            {
                                exploitDetected = true;
                                break;
                            }
                        }
                        
                        arg++;
                    }
                    
                    if( exploitDetected )
                    {
                        break;
                    }
                    
                    // cmdSystem->ArgsFrom() will add space
                    if( i != 1 )
                    {
                        if( ++charCount > argsFromOneMaxlen )
                        {
                            exploitDetected = true;
                            break;
                        }
                    }
                }
                
                if( exploitDetected )
                {
                    Com_Printf( "Buffer overflow exploit radio/say, possible attempt from %s\n", networkSystem->AdrToString( cl->netchan.remoteAddress ) );
                    serverMainSystem->SendServerCommand( cl, "print \"Chat dropped due to message length constraints.\n\"" );
                    return;
                }
                
                if( !Q_stricmp( "stats", cmdSystem->Argv( 0 ) ) )
                {
                    if( cl - svs.clients != serverGameSystem->GameClientNum( cl - svs.clients )->clientNum )
                    {
                        Com_Printf( "Stats command exploit attempt from %s\n", networkSystem->AdrToString( cl->netchan.remoteAddress ) );
                        serverMainSystem->SendServerCommand( cl, "print \"^7Stats command exploit attempt detected. This has been ^1reported^7.\n\"" );
                        return;
                    }
                }
            }
            
#ifndef UPDATE_SERVER
            sgame->ClientCommand( ARRAY_INDEX( svs.clients, cl ) );
#endif
        }
    }
    else if( !bProcessed )
    {
        Com_DPrintf( "client text ignored for %s: %s\n", cl->name, cmdSystem->Argv( 0 ) );
    }
    else if( !foundCommand )
    {
        Com_DPrintf( "client text ignored for %s\n", cl->name );
    }
}

/*
===============
idServerClientSystemLocal::ClientCommand
===============
*/
bool idServerClientSystemLocal::ClientCommand( client_t* cl, msg_t* msg, bool premaprestart )
{
    sint seq;
    pointer s;
    bool clientOk = true, floodprotect = true;
    
    seq = MSG_ReadLong( msg );
    s = MSG_ReadString( msg );
    
    // see if we have already executed it
    if( cl->lastClientCommand >= seq )
    {
        return true;
    }
    
    Com_DPrintf( "clientCommand: %s : %i : %s\n", cl->name, seq, s );
    
    // drop the connection if we have somehow lost commands
    if( seq > cl->lastClientCommand + 1 )
    {
        Com_Printf( "Client %s lost %i clientCommands\n", cl->name, seq - cl->lastClientCommand + 1 );
        serverClientLocal.DropClient( cl, "Lost reliable commands" );
        return false;
    }
    
    //Fix the "ws 9999..." crash.
    //Most mods are already safe, but who knows... This might be useful
    if( !Q_strncmp( "ws", s, 2 ) )
    {
        sint idx = 0;
        
        if( strlen( s ) > 2 )
        {
            idx = ::atoi( &s[3] );
            
            if( idx < 0 || idx > 128 ) //Using 128 because i doubt this may be higher than 128, no matter which mod is used
            {
                cl->lastClientCommand = seq;
                Q_vsprintf_s( cl->lastClientCommandString, sizeof( cl->lastClientCommandString ), sizeof( cl->lastClientCommandString ), "%s", s );
                return false;
            }
        }
    }
    
    // Gordon: AHA! Need to steal this for some other stuff BOOKMARK
    // NERVE - SMF - some server game-only commands we cannot have flood protect
    if( !Q_strncmp( "team", s, 4 ) || !Q_strncmp( "setspawnpt", s, 10 ) || !Q_strncmp( "score", s, 5 ) || !Q_stricmp( "forcetapout", s ) )
    {
        //Com_DPrintf( "Skipping flood protection for: %s\n", s );
        floodprotect = false;
    }
    
    // malicious users may try using too many string commands
    // to lag other players.  If we decide that we want to stall
    // the command, we will stop processing the rest of the packet,
    // including the usercmd.  This causes flooders to lag themselves
    // but not other people
    // We don't do this when the client hasn't been active yet since its
    // normal to spam a lot of commands when downloading
    // (SA) this was commented out in Wolf.  Did we do that?
    // Applying floodprotect only to "CS_ACTIVE" clients leaves too much room for abuse.
    // Extending floodprotect to clients pre CS_ACTIVE shouldn't cause any issues,
    // as the download-commands are handled within the engine and floodprotect only filters calls to the VM.
    if( !com_cl_running->integer && cl->state >= CS_ACTIVE && sv_floodProtect->integer && svs.time < cl->nextReliableTime && floodprotect )
    {
        // ignore any other text messages from this client but let them keep playing
        // TTimo - moved the ignored verbose to the actual processing in SV_ExecuteClientCommand, only printing if the core doesn't intercept
        clientOk = false;
    }
    
    // don't allow another command for 800 msec
    if( floodprotect && svs.time >= cl->nextReliableTime )
    {
        cl->nextReliableTime = svs.time + 800;
    }
    
    serverClientLocal.ExecuteClientCommand( cl, s, clientOk, premaprestart );
    
    cl->lastClientCommand = seq;
    Q_vsprintf_s( cl->lastClientCommandString, sizeof( cl->lastClientCommandString ), sizeof( cl->lastClientCommandString ), "%s", s );
    
    return true; // continue procesing
}

/*
==================
idServerClientSystemLocal::ClientThink

Also called by bot code
==================
*/
void idServerClientSystemLocal::ClientThink( sint client, usercmd_t* cmd )
{
    if( client < 0 || sv_maxclients->integer <= client )
    {
        Com_DPrintf( S_COLOR_YELLOW "idServerClientSystemLocal::ClientThink: bad clientNum %i\n", client );
        return;
    }
    
    svs.clients[client].lastUsercmd = *cmd;
    
    if( svs.clients[client].state != CS_ACTIVE )
    {
        // may have been kicked during the last usercmd
        return;
    }
    
    if( svs.clients[client].lastUserInfoCount >= INFO_CHANGE_MAX_COUNT && svs.clients[client].lastUserInfoChange < svs.time && svs.clients[client].userinfoPostponed[0] )
    {
        // Update postponed userinfo changes now
        client_t* cl = &svs.clients[client];
        valueType info[MAX_INFO_STRING];
        
        Q_strncpyz( cl->userinfo, cl->userinfoPostponed, sizeof( cl->userinfo ) );
        UserinfoChanged( cl );
        
        // call prog code to allow overrides
        sgame->ClientUserinfoChanged( ARRAY_INDEX( svs.clients, cl ) );
        
        // get the name out of the game and set it in the engine
        serverInitSystem->GetConfigstring( CS_PLAYERS + ( cl - svs.clients ), info, sizeof( info ) );
        Info_SetValueForKey( cl->userinfo, "name", Info_ValueForKey( info, "n" ) );
        Q_strncpyz( cl->name, Info_ValueForKey( info, "n" ), sizeof( cl->name ) );
        
        // clear it
        cl->userinfoPostponed[0] = 0;
        cl->lastUserInfoCount = 0;
        cl->lastUserInfoChange = svs.time + INFO_CHANGE_MIN_INTERVAL;
    }
    
    sgame->ClientThink( client );
}

/*
==================
idServerClientSystemLocal::UserMove

The message usually contains all the movement commands
that were in the last three packets, so that the information
in dropped packets can be recovered.

On very fast clients, there may be multiple usercmd packed into
each of the backup packets.
==================
*/
void idServerClientSystemLocal::UserMove( client_t* cl, msg_t* msg, bool delta )
{
    sint i, key, cmdCount;
    usercmd_t nullcmd, cmds[MAX_PACKET_USERCMDS], *cmd, *oldcmd;
    
    if( delta )
    {
        cl->deltaMessage = cl->messageAcknowledge;
    }
    else
    {
        cl->deltaMessage = -1;
    }
    
    cmdCount = MSG_ReadByte( msg );
    
    if( cmdCount < 1 )
    {
        Com_Printf( "cmdCount < 1\n" );
        return;
    }
    
    if( cmdCount > MAX_PACKET_USERCMDS )
    {
        Com_Printf( "cmdCount > MAX_PACKET_USERCMDS\n" );
        return;
    }
    
    // use the checksum feed in the key
    key = sv.checksumFeed;
    
    // also use the message acknowledge
    key ^= cl->messageAcknowledge;
    
    // also use the last acknowledged server command in the key
    key ^= Com_HashKey( cl->reliableCommands[cl->reliableAcknowledge & ( MAX_RELIABLE_COMMANDS - 1 )], 32 );
    
    ::memset( &nullcmd, 0, sizeof( nullcmd ) );
    oldcmd = &nullcmd;
    
    for( i = 0; i < cmdCount; i++ )
    {
        cmd = &cmds[i];
        MSG_ReadDeltaUsercmdKey( msg, key, oldcmd, cmd );
        //MSG_ReadDeltaUsercmd( msg, oldcmd, cmd );
        oldcmd = cmd;
    }
    
    // save time for ping calculation
    if( cl->frames[cl->messageAcknowledge & PACKET_MASK].messageAcked <= 0 )
    {
        cl->frames[cl->messageAcknowledge & PACKET_MASK].messageAcked = svs.time;
    }
    
    
    // TTimo
    // catch the no-cp-yet situation before SV_ClientEnterWorld
    // if CS_ACTIVE, then it's time to trigger a new gamestate emission
    // if not, then we are getting remaining parasite usermove commands, which we should ignore
    if( sv_pure->integer != 0 && cl->pureAuthentic == false && !cl->pureReceived )
    {
        if( cl->state == CS_ACTIVE )
        {
            // we didn't get a cp yet, don't assume anything and just send the gamestate all over again
            Com_DPrintf( "%s: didn't get cp command, resending gamestate\n", cl->name );
            
            SendClientGameState( cl );
        }
        return;
    }
    
    // if this is the first usercmd we have received
    // this gamestate, put the client into the world
    if( cl->state == CS_PRIMED )
    {
        serverClientLocal.ClientEnterWorld( cl, &cmds[0] );
        // the moves can be processed normaly
    }
    
    // a bad cp command was sent, drop the client
    if( sv_pure->integer != 0 && cl->pureAuthentic == false )
    {
        serverClientLocal.DropClient( cl, "Cannot validate pure client!" );
        return;
    }
    
    if( cl->state != CS_ACTIVE )
    {
        cl->deltaMessage = -1;
        return;
    }
    
    // usually, the first couple commands will be duplicates
    // of ones we have previously received, but the servertimes
    // in the commands will cause them to be immediately discarded
    for( i = 0; i < cmdCount; i++ )
    {
        // if this is a cmd from before a map_restart ignore it
        if( cmds[i].serverTime > cmds[cmdCount - 1].serverTime )
        {
            continue;
        }
        
        // extremely lagged or cmd from before a map_restart
        //if ( cmds[i].serverTime > svs.time + 3000 )
        //{
        //  continue;
        //}
        
        if( !serverGameSystem->GameIsSinglePlayer() ) // We need to allow this in single player, where loadgame's can cause the player to freeze after reloading if we do this check
        {
            // don't execute if this is an old cmd which is already executed
            // these old cmds are included when cl_packetdup > 0
            if( cmds[i].serverTime <= cl->lastUsercmd.serverTime ) // Q3_MISSIONPACK
            {
                //if ( cmds[i].serverTime > cmds[cmdCount-1].serverTime )
                // from just before a map_restart
                continue;
            }
        }
        serverClientLocal.ClientThink( cl - svs.clients, &cmds[i] );
    }
}

/*
===========================================================================
USER CMD EXECUTION
===========================================================================
*/

/*
===================
idServerClientSystemLocal::ExecuteClientMessage
Parse a client packet
===================
*/
void idServerClientSystemLocal::ExecuteClientMessage( client_t* cl, msg_t* msg )
{
    sint c, serverId;
    
    MSG_Bitstream( msg );
    
    serverId = MSG_ReadLong( msg );
    cl->messageAcknowledge = MSG_ReadLong( msg );
    
    if( cl->messageAcknowledge < 0 )
    {
        // usually only hackers create messages like this
        // it is more annoying for them to let them hanging
#ifndef NDEBUG
        DropClient( cl, "DEBUG: illegible client message" );
#endif
        return;
    }
    
    cl->reliableAcknowledge = MSG_ReadLong( msg );
    
    // NOTE: when the client message is fux0red the acknowledgement numbers
    // can be out of range, this could cause the server to send thousands of server
    // commands which the server thinks are not yet acknowledged in serverSnapshotSystemLocal::UpdateServerCommandsToClient
    if( cl->reliableAcknowledge < cl->reliableSequence - MAX_RELIABLE_COMMANDS )
    {
        // usually only hackers create messages like this
        // it is more annoying for them to let them hanging
#ifndef NDEBUG
        DropClient( cl, "DEBUG: illegible client message" );
#endif
        cl->reliableAcknowledge = cl->reliableSequence;
        return;
    }
    
    // if this is a usercmd from a previous gamestate,
    // ignore it or retransmit the current gamestate
    //
    // if the client was downloading, let it stay at whatever serverId and
    // gamestate it was at.  This allows it to keep downloading even when
    // the gamestate changes.  After the download is finished, we'll
    // notice and send it a new game state
    //
    // show_bug.cgi?id=536
    // don't drop as long as previous command was a nextdl, after a dl is done, downloadName is set back to ""
    // but we still need to read the next message to move to next download or send gamestate
    // I don't like this hack though, it must have been working fine at some point, suspecting the fix is somewhere else
    if( serverId != sv.serverId && !*cl->downloadName && !strstr( cl->lastClientCommandString, "nextdl" ) )
    {
        if( serverId >= sv.restartedServerId && serverId < sv.serverId ) // TTimo - use a comparison here to catch multiple map_restart
        {
            // they just haven't caught the map_restart yet
            Com_DPrintf( "%s : ignoring pre map_restart / outdated client message\n", cl->name );
            return;
        }
        // if we can tell that the client has dropped the last
        // gamestate we sent them, resend it
        if( cl->state != CS_ACTIVE && cl->messageAcknowledge > cl->gamestateMessageNum )
        {
            Com_DPrintf( "%s : dropped gamestate, resending\n", cl->name );
            SendClientGameState( cl );
        }
        
        // this client has acknowledged the new gamestate so it's
        // safe to start sending it the real time again
        if( cl->oldServerTime && serverId == sv.serverId )
        {
            Com_DPrintf( "%s acknowledged gamestate\n", cl->name );
            cl->oldServerTime = 0;
        }
        
        // read optional clientCommand strings
        do
        {
            c = MSG_ReadByte( msg );
            
            if( c == clc_EOF )
            {
                break;
            }
            
            if( c != clc_clientCommand )
            {
                break;
            }
            
            if( !ClientCommand( cl, msg, true ) )
            {
                // we couldn't execute it because of the flood protection
                return;
            }
            
            if( cl->state == CS_ZOMBIE )
            {
                // disconnect command
                return;
            }
        }
        while( 1 );
        
        return;
    }
    
    // read optional clientCommand strings
    do
    {
        c = MSG_ReadByte( msg );
        
        if( c == clc_EOF )
        {
            break;
        }
        
        if( c != clc_clientCommand )
        {
            break;
        }
        
        if( !ClientCommand( cl, msg, false ) )
        {
            // we couldn't execute it because of the flood protection
            return;
        }
        
        if( cl->state == CS_ZOMBIE )
        {
            // disconnect command
            return;
        }
    }
    while( 1 );
    
    // read the usercmd_t
    if( c == clc_move )
    {
        UserMove( cl, msg, true );
    }
    else if( c == clc_moveNoDelta )
    {
        UserMove( cl, msg, false );
    }
    else if( c != clc_EOF )
    {
        Com_Printf( "WARNING: bad command byte for client %i\n", static_cast<sint>( cl - svs.clients ) );
    }
    
    //if ( msg->readcount != msg->cursize )
    //{
    //    Com_Printf( "WARNING: Junk at end of packet for client %i\n", cl - svs.clients );
    //}
}
