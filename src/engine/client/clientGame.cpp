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
// File name:   clientGame.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: client system interaction with client game
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

static void( *completer )( pointer s ) = nullptr;

idClientGameSystemLocal clientGameLocal;
idClientGameSystem* clientGameSystem = &clientGameLocal;

idCGame* cgame;
idCGame* ( *cgameEntry )( cgameImports_t* cgimports );
static cgameImports_t exports;

/*
===============
idClientGameSystemLocal::idClientGameSystemLocal
===============
*/
idClientGameSystemLocal::idClientGameSystemLocal( void )
{
}

/*
===============
idClientGameSystemLocal::~idClientGameSystemLocal
===============
*/
idClientGameSystemLocal::~idClientGameSystemLocal( void )
{
}

/*
====================
idClientGameSystemLocal::GetGameState
====================
*/
void idClientGameSystemLocal::GetGameState( gameState_t* gs )
{
    *gs = cl.gameState;
}

/*
====================
idClientGameSystemLocal::GetGlconfig
====================
*/
void idClientGameSystemLocal::GetGlconfig( vidconfig_t* glconfig )
{
    *glconfig = cls.glconfig;
}

/*
====================
idClientGameSystemLocal::CompleteCallback
====================
*/
sint idClientGameSystemLocal::CompleteCallback( pointer complete )
{
    if( completer )
    {
        completer( complete );
    }
    return 0;
}

/*
====================
idClientGameSystemLocal::GetUserCmd
====================
*/
bool idClientGameSystemLocal::GetUserCmd( sint cmdNumber, usercmd_t* ucmd )
{
    // cmds[cmdNumber] is the last properly generated command
    
    // can't return anything that we haven't created yet
    if( cmdNumber > cl.cmdNumber )
    {
        Com_Error( ERR_DROP, "idClientGameSystemLocal::GetUserCmd: %i >= %i", cmdNumber, cl.cmdNumber );
    }
    
    // the usercmd has been overwritten in the wrapping
    // buffer because it is too far out of date
    if( cmdNumber <= cl.cmdNumber - CMD_BACKUP )
    {
        return false;
    }
    
    *ucmd = cl.cmds[cmdNumber & CMD_MASK];
    
    return true;
}

/*
====================
idClientGameSystemLocal::GetCurrentCmdNumber
====================
*/
sint idClientGameSystemLocal::GetCurrentCmdNumber( void )
{
    return cl.cmdNumber;
}

/*
====================
idClientGameSystemLocal::GetCurrentSnapshotNumber
====================
*/
void idClientGameSystemLocal::GetCurrentSnapshotNumber( sint* snapshotNumber, sint* serverTime )
{
    *snapshotNumber = cl.snapServer.messageNum;
    *serverTime = cl.snapServer.serverTime;
}

/*
====================
idClientGameSystemLocal::GetSnapshot
====================
*/
bool idClientGameSystemLocal::GetSnapshot( sint snapshotNumber, snapshot_t* snapshot )
{
    sint i, count;
    clSnapshot_t* clSnap;
    
    if( snapshotNumber > cl.snapServer.messageNum )
    {
        Com_Error( ERR_DROP, "idClientGameSystemLocal::GetSnapshot: snapshotNumber > cl.snapshot.messageNum" );
    }
    
    // if the frame has fallen out of the circular buffer, we can't return it
    if( cl.snapServer.messageNum - snapshotNumber >= PACKET_BACKUP )
    {
        return false;
    }
    
    // if the frame is not valid, we can't return it
    clSnap = &cl.snapshots[snapshotNumber & PACKET_MASK];
    if( !clSnap->valid )
    {
        return false;
    }
    
    // if the entities in the frame have fallen out of their
    // circular buffer, we can't return it
    if( cl.parseEntitiesNum - clSnap->parseEntitiesNum >= MAX_PARSE_ENTITIES )
    {
        return false;
    }
    
    // write the snapshot
    snapshot->snapFlags = clSnap->snapFlags;
    snapshot->serverCommandSequence = clSnap->serverCommandNum;
    snapshot->ping = clSnap->ping;
    snapshot->serverTime = clSnap->serverTime;
    ::memcpy( snapshot->areamask, clSnap->areamask, sizeof( snapshot->areamask ) );
    snapshot->ps = clSnap->ps;
    
    count = clSnap->numEntities;
    if( count > MAX_ENTITIES_IN_SNAPSHOT )
    {
        Com_DPrintf( "idClientGameSystemLocal::GetSnapshot: truncated %i entities to %i\n", count, MAX_ENTITIES_IN_SNAPSHOT );
        count = MAX_ENTITIES_IN_SNAPSHOT;
    }
    
    snapshot->numEntities = count;
    
    for( i = 0; i < count; i++ )
    {
        snapshot->entities[i] = cl.parseEntities[( clSnap->parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];
    }
    
    // FIXME: configstring changes and server commands!!!
    
    return true;
}

/*
==============
idClientGameSystemLocal::SetUserCmdValue
==============
*/
void idClientGameSystemLocal::SetUserCmdValue( sint userCmdValue, sint flags, float32 sensitivityScale, sint mpIdentClient )
{
    cl.cgameUserCmdValue = userCmdValue;
    cl.cgameFlags = flags;
    cl.cgameSensitivity = sensitivityScale;
    cl.cgameMpIdentClient = mpIdentClient;	// NERVE - SMF
}

/*
==================
idClientGameSystemLocal::SetClientLerpOrigin
==================
*/
void idClientGameSystemLocal::SetClientLerpOrigin( float32 x, float32 y, float32 z )
{
    cl.cgameClientLerpOrigin[0] = x;
    cl.cgameClientLerpOrigin[1] = y;
    cl.cgameClientLerpOrigin[2] = z;
}

/*
=====================
idClientGameSystemLocal::CompleteCgameCommand
=====================
*/
void idClientGameSystemLocal::CompleteCgameCommand( valueType* args, sint argNum )
{
    cmdCompletionSystem->CompleteCgame( argNum );
}

/*
=====================
idClientGameSystemLocal::CgameCompletion
=====================
*/
void idClientGameSystemLocal::CgameCompletion( void( *callback )( pointer s ), sint argNum )
{
    completer = callback;
    cgame->CompleteCommand( argNum );
    completer = nullptr;
}

/*
==============
idClientGameSystemLocal::AddCgameCommand
==============
*/
void idClientGameSystemLocal::AddCgameCommand( pointer cmdName, pointer cmdDesc )
{
    cmdSystem->AddCommand( cmdName, nullptr, cmdDesc );
    cmdSystem->SetCommandCompletionFunc( cmdName, CompleteCgameCommand );
}

/*
==============
idClientGameSystemLocal::CgameError
==============
*/
void idClientGameSystemLocal::CgameError( pointer string )
{
    Com_Error( ERR_DROP, "%s", string );
}

/*
=====================
idClientGameSystemLocal::CGameCheckKeyExec
=====================
*/
bool idClientGameSystemLocal::CGameCheckKeyExec( sint key )
{
    if( cgvm )
    {
        return cgame->CheckExecKey( key );
    }
    else
    {
        return false;
    }
}

/*
=====================
idClientGameSystemLocal::ConfigstringModified
=====================
*/
void idClientGameSystemLocal::ConfigstringModified( void )
{
    sint i, index;
    uint64 len;
    valueType* old;
    pointer s, dup;
    gameState_t oldGs;
    
    index = atoi( cmdSystem->Argv( 1 ) );
    if( index < 0 || index >= MAX_CONFIGSTRINGS )
    {
        Com_Error( ERR_DROP, "idClientGameSystemLocal::ConfigstringModified: bad index %i", index );
    }
    
    //s = cmdSystem->Argv(2);
    
    // get everything after "cs <num>"
    s = cmdSystem->ArgsFrom( 2 );
    
    old = cl.gameState.stringData + cl.gameState.stringOffsets[index];
    if( !::strcmp( old, s ) )
    {
        // unchanged
        return;
    }
    
    // build the new gameState_t
    oldGs = cl.gameState;
    
    ::memset( &cl.gameState, 0, sizeof( cl.gameState ) );
    
    // leave the first 0 for uninitialized strings
    cl.gameState.dataCount = 1;
    
    for( i = 0; i < MAX_CONFIGSTRINGS; i++ )
    {
        if( i == index )
        {
            dup = s;
        }
        else
        {
            dup = oldGs.stringData + oldGs.stringOffsets[i];
        }
        if( !dup[0] )
        {
            // leave with the default empty string
            continue;
        }
        
        len = ::strlen( dup );
        
        if( len + 1 + cl.gameState.dataCount > MAX_GAMESTATE_CHARS )
        {
            Com_Error( ERR_DROP, "MAX_GAMESTATE_CHARS exceeded" );
        }
        
        // append it to the gameState string buffer
        cl.gameState.stringOffsets[i] = cl.gameState.dataCount;
        ::memcpy( cl.gameState.stringData + cl.gameState.dataCount, dup, len + 1 );
        cl.gameState.dataCount += len + 1;
    }
    
    if( index == CS_SYSTEMINFO )
    {
        // parse serverId and other cvars
        idClientParseSystemLocal::SystemInfoChanged();
    }
    
}

/*
=====================
idClientGameSystemLocal::UIPopup
=====================
*/
void idClientGameSystemLocal::UIPopup( pointer uiname )
{
    if( uiname == nullptr )
    {
        uiManager->SetActiveMenu( UIMENU_MAIN );
        return;
    }
    
    if( cls.state == CA_ACTIVE && !clc.demoplaying )
    {
        if( !Q_stricmp( uiname, "UIMENU_NONE" ) )
        {
            uiManager->SetActiveMenu( UIMENU_NONE );
        }
        else if( !Q_stricmp( uiname, "UIMENU_MAIN" ) )
        {
            uiManager->SetActiveMenu( UIMENU_MAIN );
        }
        else if( !Q_stricmp( uiname, "UIMENU_INGAME" ) )
        {
            uiManager->SetActiveMenu( UIMENU_INGAME );
        }
        else if( !Q_stricmp( uiname, "UIMENU_NEED_CD" ) )
        {
            uiManager->SetActiveMenu( UIMENU_NEED_CD );
        }
        else if( !Q_stricmp( uiname, "UIMENU_BAD_CD_KEY" ) )
        {
            uiManager->SetActiveMenu( UIMENU_BAD_CD_KEY );
        }
        else if( !Q_stricmp( uiname, "UIMENU_TEAM" ) )
        {
            uiManager->SetActiveMenu( UIMENU_TEAM );
        }
        else if( !Q_stricmp( uiname, "UIMENU_POSTGAME" ) )
        {
            uiManager->SetActiveMenu( UIMENU_POSTGAME );
        }
        else if( !Q_stricmp( uiname, "UIMENU_HELP" ) )
        {
            uiManager->SetActiveMenu( UIMENU_HELP );
        }
        else if( !Q_stricmp( uiname, "UIMENU_WM_QUICKMESSAGE" ) )
        {
            uiManager->SetActiveMenu( UIMENU_WM_QUICKMESSAGE );
        }
        else if( !Q_stricmp( uiname, "UIMENU_WM_QUICKMESSAGEALT" ) )
        {
            uiManager->SetActiveMenu( UIMENU_WM_QUICKMESSAGEALT );
        }
        else if( !Q_stricmp( uiname, "UIMENU_WM_FTQUICKMESSAGE" ) )
        {
            uiManager->SetActiveMenu( UIMENU_WM_FTQUICKMESSAGE );
        }
        else if( !Q_stricmp( uiname, "UIMENU_WM_TAPOUT" ) )
        {
            uiManager->SetActiveMenu( UIMENU_WM_TAPOUT );
        }
        else if( !Q_stricmp( uiname, "UIMENU_WM_TAPOUT_LMS" ) )
        {
            uiManager->SetActiveMenu( UIMENU_WM_TAPOUT_LMS );
        }
        else if( !Q_stricmp( uiname, "UIMENU_WM_AUTOUPDATE" ) )
        {
            uiManager->SetActiveMenu( UIMENU_WM_AUTOUPDATE );
        }
        else if( !Q_stricmp( uiname, "UIMENU_INGAME_MESSAGEMODE" ) )
        {
            uiManager->SetActiveMenu( UIMENU_INGAME_MESSAGEMODE );
        }
        else
        {
            uiManager->SetActiveMenu( UIMENU_MAIN );
        }
    }
}

/*
===================
idClientGameSystemLocal::GetServerCommand

Set up argc/argv for the given command
===================
*/
bool idClientGameSystemLocal::GetServerCommand( sint serverCommandNumber )
{
    sint argc, i = 0;
    pointer s, cmd;
    static valueType bigConfigString[BIG_INFO_STRING];
    
    // if we have irretrievably lost a reliable command, drop the connection
    if( serverCommandNumber <= clc.serverCommandSequence - MAX_RELIABLE_COMMANDS )
    {
        // when a demo record was started after the client got a whole bunch of
        // reliable commands then the client never got those first reliable commands
        if( clc.demoplaying )
        {
            return false;
        }
        
        // avoid spamming the console
        static sint lastTimeCrash = 0;
        sint nowHappened = idsystem->Milliseconds();
        
        if( !lastTimeCrash || nowHappened - lastTimeCrash >= 1000 )
        {
            lastTimeCrash = nowHappened;
            
            while( i < MAX_RELIABLE_COMMANDS )
            {
                if( clc.reliableCommands[i][0] )
                {
                    Com_Printf( "%i: %s\n", i, clc.reliableCommands[i] );
                }
                
                i++;
            }
            
            Com_Printf( "^idClientGameSystemLocal::GetServerCommand: a reliable command was cycled out. Auto-reconnecting.^7\n" );
            cmdBufferSystem->ExecuteText( EXEC_NOW, "reconnect\n" );
        }
        
        return false;
    }
    
    if( serverCommandNumber > clc.serverCommandSequence )
    {
        Com_Error( ERR_DROP, "idClientGameSystemLocal::GetServerCommand: requested a command not received" );
        return false;
    }
    
    s = clc.serverCommands[serverCommandNumber & ( MAX_RELIABLE_COMMANDS - 1 )];
    clc.lastExecutedServerCommand = serverCommandNumber;
    
    if( cl_showServerCommands->integer )
    {
        // NERVE - SMF
        Com_DPrintf( "serverCommand: %i : %s\n", serverCommandNumber, s );
    }
    
rescan:
    cmdSystem->TokenizeString( s );
    cmd = cmdSystem->Argv( 0 );
    argc = cmdSystem->Argc();
    
    if( !strcmp( cmd, "disconnect" ) )
    {
        // NERVE - SMF - allow server to indicate why they were disconnected
        if( argc >= 2 )
        {
            Com_Error( ERR_SERVERDISCONNECT, "Server Disconnected - %s", cmdSystem->Argv( 1 ) );
        }
        else
        {
            Com_Error( ERR_SERVERDISCONNECT, "Server disconnected\n" );
        }
    }
    
    if( !strcmp( cmd, "bcs0" ) )
    {
        Q_vsprintf_s( bigConfigString, BIG_INFO_STRING, BIG_INFO_STRING, "cs %s \"%s", cmdSystem->Argv( 1 ), cmdSystem->Argv( 2 ) );
        return false;
    }
    
    if( !strcmp( cmd, "bcs1" ) )
    {
        s = cmdSystem->Argv( 2 );
        if( strlen( bigConfigString ) + strlen( s ) >= BIG_INFO_STRING )
        {
            Com_Error( ERR_DROP, "bcs exceeded BIG_INFO_STRING" );
        }
        strcat( bigConfigString, s );
        return false;
    }
    
    if( !strcmp( cmd, "bcs2" ) )
    {
        s = cmdSystem->Argv( 2 );
        if( strlen( bigConfigString ) + strlen( s ) + 1 >= BIG_INFO_STRING )
        {
            Com_Error( ERR_DROP, "bcs exceeded BIG_INFO_STRING" );
        }
        strcat( bigConfigString, s );
        strcat( bigConfigString, "\"" );
        s = bigConfigString;
        goto rescan;
    }
    
    if( !strcmp( cmd, "cs" ) )
    {
        ConfigstringModified();
        
        // reparse the string, because ConfigstringModified may have done another cmdSystem->TokenizeString()
        cmdSystem->TokenizeString( s );
        return true;
    }
    
    if( !strcmp( cmd, "map_restart" ) )
    {
        // clear notify lines and outgoing commands before passing
        // the restart to the cgame
        Con_ClearNotify();
        // reparse the string, because Con_ClearNotify() may have done another Cmd_TokenizeString()
        cmdSystem->TokenizeString( s );
        ::memset( cl.cmds, 0, sizeof( cl.cmds ) );
        return true;
    }
    
    if( !strcmp( cmd, "popup" ) )
    {
        // direct server to client popup request, bypassing cgame
        //UIPopup(cmdSystem->Argv(1));
        //if ( cls.state == CA_ACTIVE && !clc.demoplaying )
        //{
        //    uiManager->SetActiveMenu( UIMENU_CLIPBOARD );
        //	  uiManager->Menus_ActivateByName(cmdSystem->Argv(1));
        //}
        return false;
    }
    
    // we may want to put a "connect to other server" command here
    
    // cgame can now act on the command
    return true;
}

// DHM - Nerve :: Copied from server to here
/*
====================
idClientGameSystemLocal::SetExpectedHunkUsage

Sets com_expectedhunkusage, so the client knows how to draw the percentage bar
====================
*/
void idClientGameSystemLocal::SetExpectedHunkUsage( pointer mapname )
{
    sint handle, len;
    valueType* memlistfile = "hunkusage.dat", *buf, *buftrav, *token;
    
    len = fileSystem->FOpenFileByMode( memlistfile, &handle, FS_READ );
    if( len >= 0 )
    {
        // the file exists, so read it in, strip out the current entry for this map, and save it out, so we can append the new value
        buf = static_cast<valueType*>( Z_Malloc( len + 1 ) );
        ::memset( buf, 0, len + 1 );
        
        fileSystem->Read( static_cast<void*>( buf ), len, handle );
        fileSystem->FCloseFile( handle );
        
        // now parse the file, filtering out the current map
        buftrav = buf;
        while( ( token = COM_Parse( &buftrav ) ) != nullptr && token[0] )
        {
            if( !Q_stricmp( token, ( const_cast<valueType*>( reinterpret_cast<const valueType*>( mapname ) ) ) ) )
            {
                // found a match
                token = COM_Parse( &buftrav );	// read the size
                if( token && *token )
                {
                    // this is the usage
                    com_expectedhunkusage = atoi( token );
                    Z_Free( buf );
                    return;
                }
            }
        }
        
        Z_Free( buf );
    }
    // just set it to a negative number,so the cgame knows not to draw the percent bar
    com_expectedhunkusage = -1;
}

/*
====================
idClientGameSystemLocal::LoadMap

Just adds default parameters that cgame doesn't need to know about
====================
*/
void idClientGameSystemLocal::LoadMap( pointer mapname )
{
    sint checksum;
    
    // DHM - Nerve :: If we are not running the server, then set expected usage here
    if( !com_sv_running->integer )
    {
        SetExpectedHunkUsage( mapname );
    }
    else
    {
        // TTimo
        // catch here when a local server is started to avoid outdated com_errorDiagnoseIP
        cvarSystem->Set( "com_errorDiagnoseIP", "" );
    }
    
    collisionModelManager->LoadMap( mapname, true, &checksum );
}

/*
====================
idClientGameSystemLocal::ShutdownCGame
====================
*/
void idClientGameSystemLocal::ShutdownCGame( void )
{
    clientGUISystem->SetCatcher( clientGUISystem->GetCatcher() & ~KEYCATCH_CGAME );
    cls.cgameStarted = false;
    
    if( cgame == nullptr || cgvm == nullptr )
    {
        return;
    }
    
    cgame->Shutdown();
    cgame = nullptr;
    
    idsystem->UnloadDll( cgvm );
    cgvm = nullptr;
}

/*
====================
idClientGameSystemLocal::UIClosePopup
====================
*/
void idClientGameSystemLocal::UIClosePopup( pointer uiname )
{
    uiManager->KeyEvent( K_ESCAPE, true );
}

/*
====================
idClientGameSystemLocal::KeySetCatcher
====================
*/
void idClientGameSystemLocal::KeySetCatcher( sint catcher )
{
    clientGUISystem->SetCatcher( catcher | ( clientGUISystem->GetCatcher() & KEYCATCH_CONSOLE ) );
}

/*
====================
idClientGameSystemLocal::CreateExportTable
====================
*/
void idClientGameSystemLocal::CreateExportTable( void )
{
    exports.Print = Com_Printf;
    
    exports.AddReliableCommand = CL_AddReliableCommand;
    
    exports.Hunk_MemoryRemaining = Hunk_MemoryRemaining;
    
    exports.Key_IsDown = Key_IsDown;
    exports.Key_GetKey = Key_GetKey;
    exports.Key_GetOverstrikeMode = Key_GetOverstrikeMode;
    exports.Key_SetOverstrikeMode = Key_SetOverstrikeMode;
    
    exports.RealTime = Com_RealTime;
    
    exports.CIN_PlayCinematic = CIN_PlayCinematic;
    exports.CIN_StopCinematic = CIN_StopCinematic;
    exports.CIN_RunCinematic = CIN_RunCinematic;
    exports.CIN_DrawCinematic = CIN_DrawCinematic;
    exports.CIN_SetExtents = CIN_SetExtents;
    
    exports.Key_SetBinding = Key_SetBinding;
    exports.Key_GetBindingByString = Key_GetBindingByString;
    exports.CL_TranslateString = CL_TranslateString;
    exports.Com_GetHunkInfo = Com_GetHunkInfo;
    exports.UI_LimboChat = CL_AddToLimboChat;
    
    exports.clientGameSystem = clientGameSystem;
    exports.renderSystem = renderSystem;
    exports.collisionModelManager = collisionModelManager;
    exports.soundSystem = soundSystem;
    exports.fileSystem = fileSystem;
    exports.cvarSystem = cvarSystem;
    exports.cmdBufferSystem = cmdBufferSystem;
    exports.cmdSystem = cmdSystem;
    exports.idsystem = idsystem;
    exports.idGUISystem = clientGUISystem;
    exports.clientScreenSystem = clientScreenSystem;
    exports.parseSystem = ParseSystem;
}

/*
====================
idClientGameSystemLocal::UpdateLevelHunkUsage

This updates the "hunkusage.dat" file with the current map and it's hunk usage count

This is used for level loading, so we can show a percentage bar dependant on the amount
of hunk memory allocated so far

This will be slightly inaccurate if some settings like sound quality are changed, but these
things should only account for a small variation (hopefully)
====================
*/
void idClientGameSystemLocal::UpdateLevelHunkUsage( void )
{
    sint handle, len, memusage;
    valueType* memlistfile = "hunkusage.dat", *buf, *outbuf, *buftrav, *outbuftrav, * token, outstr[256];
    
    memusage = cvarSystem->VariableIntegerValue( "com_hunkused" ) + cvarSystem->VariableIntegerValue( "hunk_soundadjust" );
    
    len = fileSystem->FOpenFileByMode( memlistfile, &handle, FS_READ );
    if( len >= 0 )
    {
        // the file exists, so read it in, strip out the current entry for this map, and save it out, so we can append the new value
        buf = static_cast< valueType*>( Z_Malloc( len + 1 ) );
        ::memset( buf, 0, len + 1 );
        
        outbuf = static_cast<valueType*>( Z_Malloc( len + 1 ) );
        ::memset( outbuf, 0, len + 1 );
        
        fileSystem->Read( static_cast<void*>( buf ), len, handle );
        fileSystem->FCloseFile( handle );
        
        // now parse the file, filtering out the current map
        buftrav = buf;
        outbuftrav = outbuf;
        outbuftrav[0] = '\0';
        
        while( ( token = COM_Parse( &buftrav ) ) != nullptr && token[0] )
        {
            if( !Q_stricmp( token, cl.mapname ) )
            {
                // found a match
                token = COM_Parse( &buftrav );	// read the size
                if( token && token[0] )
                {
                    if( atoi( token ) == memusage )
                    {
                        // if it is the same, abort this process
                        Z_Free( buf );
                        Z_Free( outbuf );
                        return;
                    }
                }
            }
            else
            {
                // send it to the outbuf
                Q_strcat( outbuftrav, len + 1, token );
                Q_strcat( outbuftrav, len + 1, " " );
                token = COM_Parse( &buftrav );	// read the size
                
                if( token && token[0] )
                {
                    Q_strcat( outbuftrav, len + 1, token );
                    Q_strcat( outbuftrav, len + 1, "\n" );
                }
                else
                {
                    Com_Error( ERR_DROP, "hunkusage.dat file is corrupt\n" );
                }
            }
        }
        
        handle = fileSystem->FOpenFileWrite( memlistfile );
        if( handle < 0 )
        {
            Com_Error( ERR_DROP, "cannot create %s\n", memlistfile );
        }
        
        // input file is parsed, now output to the new file
        len = strlen( outbuf );
        
        if( fileSystem->Write( static_cast<void*>( outbuf ), len, handle ) != len )
        {
            Com_Error( ERR_DROP, "cannot write to %s\n", memlistfile );
        }
        
        fileSystem->FCloseFile( handle );
        
        Z_Free( buf );
        Z_Free( outbuf );
    }
    
    // now append the current map to the current file
    fileSystem->FOpenFileByMode( memlistfile, &handle, FS_APPEND );
    if( handle < 0 )
    {
        Com_Error( ERR_DROP, "cannot write to hunkusage.dat, check disk full\n" );
    }
    
    Q_vsprintf_s( outstr, sizeof( outstr ), sizeof( outstr ), "%s %i\n", cl.mapname, memusage );
    
    fileSystem->Write( outstr, strlen( outstr ), handle );
    fileSystem->FCloseFile( handle );
    
    // now just open it and close it, so it gets copied to the pak dir
    len = fileSystem->FOpenFileByMode( memlistfile, &handle, FS_READ );
    if( len >= 0 )
    {
        fileSystem->FCloseFile( handle );
    }
}

/*
====================
idClientGameSystemLocal::InitCGame

Should only by called by CL_StartHunkUsers
====================
*/
void idClientGameSystemLocal::InitCGame( void )
{
    sint t1, t2;
    pointer info, mapname;
    
    t1 = idsystem->Milliseconds();
    
    // put away the console
    Con_Close();
    
    // find the current mapname
    info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO];
    mapname = Info_ValueForKey( info, "mapname" );
    Q_vsprintf_s( cl.mapname, sizeof( cl.mapname ), sizeof( cl.mapname ), "maps/%s.bsp", mapname );
    
    // load the dll
    cgvm = idsystem->LoadDll( "cgame" );
    if( !cgvm )
    {
        Com_Error( ERR_DROP, "cannot load cgame dynamic module.\n" );
    }
    
    // Load in the entry point.
    cgameEntry = ( idCGame * ( QDECL* )( cgameImports_t* ) )idsystem->GetProcAddress( cgvm, "cgameEntry" );
    if( !cgameEntry )
    {
        Com_Error( ERR_DROP, "error loading entry point on clientGame.\n" );
    }
    
    // Create the export table.
    CreateExportTable();
    
    // Call the dll entry point.
    if( cgameEntry )
    {
        cgame = cgameEntry( &exports );
    }
    
    cls.state = CA_LOADING;
    
    // init for this gamestate
    // use the lastExecutedServerCommand instead of the serverCommandSequence
    // otherwise server commands sent just before a gamestate are dropped
    //bani - added clc.demoplaying, since some mods need this at init time, and drawactiveframe is too late for them
    cgame->Init( clc.serverMessageSequence, clc.lastExecutedServerCommand, clc.clientNum, clc.demoplaying );
    
    // we will send a usercmd this frame, which
    // will cause the server to send us the first snapshot
    cls.state = CA_PRIMED;
    
    t2 = idsystem->Milliseconds();
    
    Com_Printf( "idClientGameSystemLocal::InitCGame: %5.2f seconds\n", ( t2 - t1 ) / 1000.0 );
    
    // have the renderer touch all its images, so they are present
    // on the card even if the driver does deferred loading
    renderSystem->EndRegistration();
    
    if( !idsystem->LowPhysicalMemory() )
    {
        Com_TouchMemory();
    }
    
    // clear anything that got printed
    Con_ClearNotify();
    
    // Ridah, update the memory usage file
    UpdateLevelHunkUsage();
    
    //if( cl_autorecord->integer )
    //{
    //	cvarSystem->Set( "g_synchronousClients", "1" );
    //}
}

/*
====================
idClientGameSystemLocal::GameCommand

See if the current console command is claimed by the cgame
====================
*/
bool idClientGameSystemLocal::GameCommand( void )
{
    if( !cgvm )
    {
        return false;
    }
    
    return cgame->ConsoleCommand();
}

/*
====================
idClientGameSystemLocal::GameCommand

See if the current console command is claimed by the cgame
====================
*/
void idClientGameSystemLocal::GameConsoleText( void )
{
    if( !cgvm )
    {
        return;
    }
    
    return cgame->ConsoleText();
}


/*
=====================
idClientGameSystemLocal::CGameRendering
=====================
*/
void idClientGameSystemLocal::CGameRendering( stereoFrame_t stereo )
{
    /*	static sint x = 0;
    	if(!((++x) % 20)) {
    		Com_Printf( "numtraces: %i\n", numtraces / 20 );
    		numtraces = 0;
    	} else {
    	}*/
    
    cgame->DrawActiveFrame( cl.serverTime, stereo, clc.demoplaying );
}

/*
=================
idClientGameSystemLocal::AdjustTimeDelta

Adjust the clients view of server time.

We attempt to have cl.serverTime exactly equal the server's view
of time plus the timeNudge, but with variable latencies over
the internet it will often need to drift a bit to match conditions.

Our ideal time would be to have the adjusted time approach, but not pass,
the very latest snapshot.

Adjustments are only made when a new snapshot arrives with a rational
latency, which keeps the adjustment process framerate independent and
prevents massive overadjustment during times of significant packet loss
or bursted delayed packets.
=================
*/
void idClientGameSystemLocal::AdjustTimeDelta( void )
{
    sint newDelta, deltaDelta;
    
    cl.newSnapshots = false;
    
    // the delta never drifts when replaying a demo
    if( clc.demoplaying )
    {
        return;
    }
    
    newDelta = cl.snapServer.serverTime - cls.realtime;
    deltaDelta = abs( newDelta - cl.serverTimeDelta );
    
    if( deltaDelta > RESET_TIME )
    {
        cl.serverTimeDelta = newDelta;
        cl.oldServerTime = cl.snapServer.serverTime;	// FIXME: is this a problem for cgame?
        cl.serverTime = cl.snapServer.serverTime;
        
        if( cl_showTimeDelta->integer )
        {
            Com_Printf( "<RESET> " );
        }
    }
    else if( deltaDelta > 100 )
    {
        // fast adjust, cut the difference in half
        if( cl_showTimeDelta->integer )
        {
            Com_Printf( "<FAST> " );
        }
        cl.serverTimeDelta = ( cl.serverTimeDelta + newDelta ) >> 1;
    }
    else
    {
        // slow drift adjust, only move 1 or 2 msec
        // if any of the frames between this and the previous snapshot
        // had to be extrapolated, nudge our sense of time back a little
        // the granularity of +1 / -2 is too high for timescale modified frametimes
        if( com_timescale->value == 0 || com_timescale->value == 1 )
        {
            if( cl.extrapolatedSnapshot )
            {
                cl.extrapolatedSnapshot = false;
                cl.serverTimeDelta -= 2;
            }
            else
            {
                // otherwise, move our sense of time forward to minimize total latency
                cl.serverTimeDelta++;
            }
        }
    }
    
    if( cl_showTimeDelta->integer )
    {
        Com_Printf( "%i ", cl.serverTimeDelta );
    }
}

/*
==================
idClientGameSystemLocal::FirstSnapshot
==================
*/
void idClientGameSystemLocal::FirstSnapshot( void )
{
    // ignore snapshots that don't have entities
    if( cl.snapServer.snapFlags & SNAPFLAG_NOT_ACTIVE )
    {
        return;
    }
    
    cls.state = CA_ACTIVE;
    
    // set the timedelta so we are exactly on this first frame
    cl.serverTimeDelta = cl.snapServer.serverTime - cls.realtime;
    cl.oldServerTime = cl.snapServer.serverTime;
    
    clc.timeDemoBaseTime = cl.snapServer.serverTime;
    
    // if this is the first frame of active play,
    // execute the contents of activeAction now
    // this is to allow scripting a timedemo to start right
    // after loading
    if( cl_activeAction->string[0] )
    {
        cmdBufferSystem->AddText( cl_activeAction->string );
        cmdBufferSystem->AddText( "\n" );
        cvarSystem->Set( "activeAction", "" );
    }
}

/*
==================
idClientGameSystemLocal::SetCGameTime
==================
*/
void idClientGameSystemLocal::SetCGameTime( void )
{
    // getting a valid frame message ends the connection process
    if( cls.state != CA_ACTIVE )
    {
        if( cls.state != CA_PRIMED )
        {
            return;
        }
        
        if( clc.demoplaying )
        {
            // we shouldn't get the first snapshot on the same frame
            // as the gamestate, because it causes a bad time skip
            if( !clc.firstDemoFrameSkipped )
            {
                clc.firstDemoFrameSkipped = true;
                return;
            }
            
            CL_ReadDemoMessage();
        }
        
        if( cl.newSnapshots )
        {
            cl.newSnapshots = false;
            FirstSnapshot();
        }
        
        if( cls.state != CA_ACTIVE )
        {
            return;
        }
    }
    
    // if we have gotten to this point, cl.snap is guaranteed to be valid
    if( !cl.snapServer.valid )
    {
        Com_Error( ERR_DROP, "idClientGameSystemLocal::SetCGameTime: !cl.snapServer.valid" );
    }
    
    // allow pause in single player
    if( sv_paused->integer && cl_paused->integer && com_sv_running->integer )
    {
        // paused
        return;
    }
    
    if( cl.snapServer.serverTime < cl.oldFrameServerTime )
    {
        // Ridah, if this is a localhost, then we are probably loading a savegame
        if( !Q_stricmp( cls.servername, "localhost" ) )
        {
            // do nothing?
            FirstSnapshot();
        }
        else
        {
            Com_Error( ERR_DROP, "cl.snapServer.serverTime < cl.oldFrameServerTime" );
        }
    }
    
    cl.oldFrameServerTime = cl.snapServer.serverTime;
    
    // get our current view of time
    if( clc.demoplaying && cl_freezeDemo->integer )
    {
        // cl_freezeDemo is used to lock a demo in place for single frame advances
        cl.serverTimeDelta -= cls.frametime;
    }
    else
    {
        // cl_timeNudge is a user adjustable convar that allows more
        // or less latency to be added in the interest of better
        // smoothness or better responsiveness.
        sint timeNudge;
        
        timeNudge = cl_timeNudge->integer;
        if( timeNudge < 0 && ( cl.snapServer.ps.pm_type == PM_SPECTATOR || cl.snapServer.ps.pm_flags & PMF_FOLLOW || clc.demoplaying ) )
        {
            // disable negative timeNudge when spectating
            timeNudge = 0;
        }
        
#ifdef _DEBUG
        timeNudge = Com_Clampi( -900, 900, timeNudge );
#else
        timeNudge = Com_Clampi( -30, 30, timeNudge );
#endif
        
        cl.serverTime = cls.realtime + cl.serverTimeDelta - timeNudge;
        
        // guarantee that time will never flow backwards, even if
        // serverTimeDelta made an adjustment or cl_timeNudge was changed
        if( cl.serverTime < cl.oldServerTime )
        {
            cl.serverTime = cl.oldServerTime;
        }
        cl.oldServerTime = cl.serverTime;
        
        // note if we are almost past the latest frame (without timeNudge),
        // so we will try and adjust back a bit when the next snapshot arrives
        if( cls.realtime + cl.serverTimeDelta >= cl.snapServer.serverTime - 5 )
        {
            cl.extrapolatedSnapshot = true;
        }
    }
    
    // if we have gotten new snapshots, drift serverTimeDelta
    // don't do this every frame, or a period of packet loss would
    // make a huge adjustment
    if( cl.newSnapshots )
    {
        AdjustTimeDelta();
    }
    
    if( !clc.demoplaying )
    {
        return;
    }
    
    // if we are playing a demo back, we can just keep reading
    // messages from the demo file until the cgame definately
    // has valid snapshots to interpolate between
    
    // a timedemo will always use a deterministic set of time samples
    // no matter what speed machine it is run on,
    // while a normal demo may have different time samples
    // each time it is played back
    if( cl_timedemo->integer )
    {
        if( !clc.timeDemoStart )
        {
            clc.timeDemoStart = idsystem->Milliseconds();
        }
        clc.timeDemoFrames++;
        cl.serverTime = clc.timeDemoBaseTime + clc.timeDemoFrames * 50;
    }
    
    while( cl.serverTime >= cl.snapServer.serverTime )
    {
        // feed another messag, which should change
        // the contents of cl.snap
        CL_ReadDemoMessage();
        if( cls.state != CA_ACTIVE )
        {
            cvarSystem->Set( "timescale", "1" );
            // end of demo
            return;
        }
    }
    
}

/*
====================
idClientGameSystemLocal::GetTag
====================
*/
bool idClientGameSystemLocal::GetTag( sint clientNum, valueType* tagname, orientation_t* _or )
{
    if( !cgvm )
    {
        return false;
    }
    
    return cgame->GetTag( clientNum, tagname, _or );
}

