////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011-2019 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   sgame_api.h
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SG_API_H__
#define __SG_API_H__

#ifndef __SERVERMAIN_API_H__
#include <API/serverMain_api.h>
#endif

typedef void ( *xcommand_t )( void );

#define GAME_API_VERSION 1

#define SVF_NOCLIENT                  0x00000001
#define SVF_CLIENTMASK                0x00000002
#define SVF_VISDUMMY                  0x00000004
#define SVF_BOT                       0x00000008
#define SVF_POW                       0x00000010 // ignored by the engine
#define SVF_BROADCAST                 0x00000020
#define SVF_PORTAL                    0x00000040
#define SVF_BLANK                     0x00000080 // ignored by the engine
#define SVF_NOFOOTSTEPS               0x00000100 // ignored by the engine
#define SVF_CAPSULE                   0x00000200
#define SVF_VISDUMMY_MULTIPLE         0x00000400
#define SVF_SINGLECLIENT              0x00000800
#define SVF_NOSERVERINFO              0x00001000 // only meaningful for entities numbered in [0..MAX_CLIENTS)
#define SVF_NOTSINGLECLIENT           0x00002000
#define SVF_IGNOREBMODELEXTENTS       0x00004000
#define SVF_SELF_PORTAL               0x00008000
#define SVF_SELF_PORTAL_EXCLUSIVE     0x00010000
#define SVF_RIGID_BODY                0x00020000 // ignored by the engine
#define SVF_USE_CURRENT_ORIGIN        0x00040000 // ignored by the engine

#ifdef GAMEDLL
typedef struct gclient_s gclient_t;
typedef struct gentity_s gentity_t;
#endif

//
// system functions provided by the main engine
//
struct gameImports_t
{
    void( *Printf )( StringEntry fmt, ... );
    void( *Error )( S32 level, StringEntry fmt, ... );
    void( *Endgame )( void );
    S32( *RealTime )( qtime_t* qtime );
    S32( *Parse_LoadSourceHandle )( StringEntry filename );
    S32( *Parse_FreeSourceHandle )( S32 handle );
    S32( *Parse_ReadTokenHandle )( S32 handle, pc_token_t* pc_token );
    S32( *Parse_SourceFileAndLine )( S32 handle, UTF8* filename, S32* line );
    
    idSoundSystem* soundSystem;
    idCollisionModelManager* collisionModelManager;
    idFileSystem* fileSystem;
    idCVarSystem* cvarSystem;
    idServerGameSystem* serverGameSystem;
    idServerWorldSystem* serverWorldSystem;
    idServerInitSystem* serverInitSystem;
    idServerMainSystem* serverMainSystem;
    idCmdBufferSystem* cmdBufferSystem;
    idCmdSystem* cmdSystem;
    idServerDemoSystem* serverDemoSystem;
    idSystem* idsystem;
    idServerCryptoSystem* serverCryptoSystem;
};

void trap_Print( StringEntry fmt );
void trap_Error( StringEntry fmt );
S32 trap_Milliseconds( void );
void trap_Cvar_Register( vmConvar_t* cvar, StringEntry var_name, StringEntry value, S32 flags, StringEntry description );
void trap_Cvar_Set( StringEntry var_name, StringEntry value );
void trap_Cvar_Update( vmConvar_t* cvar );
S32 trap_Cvar_VariableIntegerValue( StringEntry var_name );
void trap_Cvar_VariableStringBuffer( StringEntry var_name, UTF8* buffer, S32 bufsize );
void trap_Cvar_LatchedVariableStringBuffer( StringEntry var_name, UTF8* buffer, S32 bufsize );
S32 trap_Argc( void );
void trap_Argv( S32 n, UTF8* buffer, S32 bufferLength );
void trap_SendConsoleCommand( S32 exec_when, StringEntry text );
S32 trap_FS_FOpenFile( StringEntry qpath, fileHandle_t* f, fsMode_t mode );
void trap_FS_Read( void* buffer, S32 len, fileHandle_t f );
S32 trap_FS_Write( const void* buffer, S32 len, fileHandle_t f );
S32 trap_FS_Rename( StringEntry from, StringEntry to );
void trap_FS_FCloseFile( fileHandle_t f );
S32 trap_FS_GetFileList( StringEntry path, StringEntry extension, UTF8* listbuf, S32 bufsize );
void trap_LocateGameData( gentity_t* gEnts, S32 numGEntities, S32 sizeofGEntity_t, playerState_t* clients, S32 sizeofGClient );
void trap_DropClient( S32 clientNum, StringEntry reason, S32 length );
void trap_SendServerCommand( S32 clientNum, StringEntry text );
void trap_SetConfigstring( S32 num, StringEntry string );
void trap_LinkEntity( gentity_t* ent );
void trap_UnlinkEntity( gentity_t* ent );
S32 trap_EntitiesInBox( const vec3_t mins, const vec3_t maxs, S32* list, S32 maxcount );
bool trap_EntityContact( const vec3_t mins, const vec3_t maxs, const gentity_t* ent );
bool trap_EntityContactCapsule( const vec3_t mins, const vec3_t maxs, const gentity_t* ent );
void trap_Trace( trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, S32 passEntityNum, S32 contentmask );
void trap_TraceNoEnts( trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, S32 passEntityNum, S32 contentmask );
void trap_TraceCapsule( trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, S32 passEntityNum, S32 contentmask );
void trap_TraceCapsuleNoEnts( trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, S32 passEntityNum, S32 contentmask );
S32 trap_PointContents( const vec3_t point, S32 passEntityNum );
void trap_SetBrushModel( gentity_t* ent, StringEntry name );
bool trap_InPVS( const vec3_t p1, const vec3_t p2 );
bool trap_InPVSIgnorePortals( const vec3_t p1, const vec3_t p2 );
void trap_SetConfigstringRestrictions( S32 num, const clientList_t* clientList );
void trap_GetConfigstring( S32 num, UTF8* buffer, S32 bufferSize );
void trap_SetUserinfo( S32 num, StringEntry buffer );
void trap_GetUserinfo( S32 num, UTF8* buffer, S32 bufferSize );
void trap_GetServerinfo( UTF8* buffer, S32 bufferSize );
void trap_AdjustAreaPortalState( gentity_t* ent, bool open );
bool trap_AreasConnected( S32 area1, S32 area2 );
void trap_UpdateSharedConfig( U32 port, StringEntry rconpass );
void trap_GetUsercmd( S32 clientNum, usercmd_t* cmd );
bool trap_GetEntityToken( UTF8* buffer, S32 bufferSize );
S32 trap_RealTime( qtime_t* qtime );
void trap_SnapVector( F32* v );
void trap_SendGameStat( StringEntry data );
void trap_AddCommand( StringEntry cmdName, StringEntry cmdDesc );
void trap_RemoveCommand( StringEntry cmdName );
bool trap_GetTag( S32 clientNum, S32 tagFileNumber, UTF8* tagName, orientation_t* ori );
bool trap_LoadTag( StringEntry filename );
sfxHandle_t trap_RegisterSound( StringEntry sample );
S32 trap_GetSoundLength( sfxHandle_t sfxHandle );
sfxHandle_t trap_S_RegisterSound( StringEntry sample );
S32 trap_S_SoundDuration( sfxHandle_t handle );
void trap_DemoCommand( demoCommand_t cmd, StringEntry string );
bool trap_Crypto_GenerateKeys( publicKey_t* pk, secretKey_t* sk );
bool trap_Crypto_LoadKeysFromFS( publicKey_t* pk, StringEntry pkFilename, secretKey_t* sk, StringEntry skFilename );
bool trap_Crypto_SaveKeysToFS( publicKey_t* pk, StringEntry pkFilename, secretKey_t* sk, StringEntry skFilename );
bool trap_Crypto_EncryptString( publicKey_t* pk, StringEntry inRaw, UTF8* outHex, size_t outHexSize );
bool trap_Crypto_DecryptString( publicKey_t* pk, secretKey_t* sk, StringEntry inHex, UTF8* outRaw, size_t outRawSize );
bool trap_Crypto_Hash( StringEntry inRaw, UTF8* outHex, size_t outHexSize );

//
// idGame
//
class idSGame
{
public:
    virtual void Init( S32 levelTime, S32 randomSeed, S32 restart ) = 0;
    virtual void Shutdown( S32 restart ) = 0;
    virtual UTF8* ClientConnect( S32 clientNum, bool firstTime ) = 0;
    virtual void ClientBegin( S32 clientNum ) = 0;
    virtual void ClientThink( S32 clientNum ) = 0;
    virtual void ClientUserinfoChanged( S32 clientNum ) = 0;
    virtual void ClientDisconnect( S32 clientNum ) = 0;
    virtual void ClientCommand( S32 clientNum ) = 0;
    virtual void RunFrame( S32 levelTime ) = 0;
    virtual bool ConsoleCommand( void ) = 0;
    virtual bool SnapshotCallback( S32 entityNum, S32 clientNum ) = 0;
    virtual void GameDemoCommand( S32 arg0 ) = 0;
};

extern idSGame* sgame;

#endif // !__SG_API_H__
