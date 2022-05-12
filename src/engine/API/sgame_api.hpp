////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 - 2022 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   sgame_api.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SGAME_API_HPP__
#define __SGAME_API_HPP__

typedef void (*xcommand_t)(void);

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
struct gameImports_t {
    idSoundSystem *soundSystem;
    idCollisionModelManager *collisionModelManager;
    idFileSystem *fileSystem;
    idCVarSystem *cvarSystem;
    idServerGameSystem *serverGameSystem;
    idServerWorldSystem *serverWorldSystem;
    idServerInitSystem *serverInitSystem;
    idServerMainSystem *serverMainSystem;
    idCmdBufferSystem *cmdBufferSystem;
    idCmdSystem *cmdSystem;
    idMemorySystem *memorySystem;
    idSystem *idsystem;
    idParseSystem *parseSystem;
    idCommon *common;
};

void trap_Print(pointer fmt);
void trap_Error(pointer fmt);
sint trap_Milliseconds(void);
void trap_Cvar_Register(vmConvar_t *cvar, pointer var_name, pointer value,
                        sint flags, pointer description);
void trap_Cvar_Set(pointer var_name, pointer value);
void trap_Cvar_Update(vmConvar_t *cvar);
sint trap_Cvar_VariableIntegerValue(pointer var_name);
void trap_Cvar_VariableStringBuffer(pointer var_name, valueType *buffer,
                                    uint64 bufsize);
void trap_Cvar_LatchedVariableStringBuffer(pointer var_name,
        valueType *buffer, uint64 bufsize);
sint trap_Argc(void);
void trap_Argv(sint n, valueType *buffer, sint bufferLength);
void trap_SendConsoleCommand(sint exec_when, pointer text);
sint trap_FS_FOpenFile(pointer qpath, fileHandle_t *f, fsMode_t mode);
void trap_FS_Read(void *buffer, sint len, fileHandle_t f);
sint trap_FS_Write(const void *buffer, sint len, fileHandle_t f);
sint trap_FS_Rename(pointer from, pointer to);
void trap_FS_FCloseFile(fileHandle_t f);
sint trap_FS_GetFileList(pointer path, pointer extension,
                         valueType *listbuf, sint bufsize);
void trap_LocateGameData(gentity_t *gEnts, uint64 numGEntities,
                         sint sizeofGEntity_t, playerState_t *clients, uint64 sizeofGClient);
void trap_DropClient(sint clientNum, pointer reason, sint length);
void trap_SendServerCommand(sint clientNum, pointer text);
void trap_SetConfigstring(sint num, pointer string);
void trap_LinkEntity(gentity_t *ent);
void trap_UnlinkEntity(gentity_t *ent);
sint trap_EntitiesInBox(const vec3_t mins, const vec3_t maxs, sint *list,
                        sint maxcount);
bool trap_EntityContact(const vec3_t mins, const vec3_t maxs,
                        const gentity_t *ent);
bool trap_EntityContactCapsule(const vec3_t mins, const vec3_t maxs,
                               const gentity_t *ent);
void trap_Trace(trace_t *results, const vec3_t start, const vec3_t mins,
                const vec3_t maxs, const vec3_t end, sint passEntityNum, sint contentmask);
void trap_TraceNoEnts(trace_t *results, const vec3_t start,
                      const vec3_t mins, const vec3_t maxs, const vec3_t end, sint passEntityNum,
                      sint contentmask);
void trap_TraceCapsule(trace_t *results, const vec3_t start,
                       const vec3_t mins, const vec3_t maxs, const vec3_t end, sint passEntityNum,
                       sint contentmask);
void trap_TraceCapsuleNoEnts(trace_t *results, const vec3_t start,
                             const vec3_t mins, const vec3_t maxs, const vec3_t end, sint passEntityNum,
                             sint contentmask);
sint trap_PointContents(const vec3_t point, sint passEntityNum);
void trap_SetBrushModel(gentity_t *ent, pointer name);
bool trap_InPVS(const vec3_t p1, const vec3_t p2);
bool trap_InPVSIgnorePortals(const vec3_t p1, const vec3_t p2);
void trap_SetConfigstringRestrictions(sint num,
                                      const clientList_t *clientList);
void trap_GetConfigstring(sint num, valueType *buffer, uint64 bufferSize);
void trap_SetUserinfo(sint num, pointer buffer);
void trap_GetUserinfo(sint num, valueType *buffer, uint64 bufferSize);
void trap_GetServerinfo(valueType *buffer, uint64 bufferSize);
void trap_AdjustAreaPortalState(gentity_t *ent, bool open);
bool trap_AreasConnected(sint area1, sint area2);
void trap_UpdateSharedConfig(uint port, pointer rconpass);
void trap_GetUsercmd(sint clientNum, usercmd_t *cmd);
bool trap_GetEntityToken(valueType *buffer, uint64 bufferSize);
sint trap_RealTime(qtime_t *qtime);
void trap_SnapVector(float32 *v);
void trap_SendGameStat(pointer data);
void trap_AddCommand(pointer cmdName, pointer cmdDesc);
void trap_RemoveCommand(pointer cmdName);
bool trap_GetTag(sint clientNum, sint tagFileNumber, valueType *tagName,
                 orientation_t *ori);
bool trap_LoadTag(pointer filename);
sfxHandle_t trap_RegisterSound(pointer sample);
sint trap_GetSoundLength(sfxHandle_t sfxHandle);
sfxHandle_t trap_S_RegisterSound(pointer sample);
sint trap_S_SoundDuration(sfxHandle_t handle);

//
// idGame
//
class idSGame {
public:
    virtual void Init(sint levelTime, sint randomSeed, sint restart) = 0;
    virtual void Shutdown(sint restart) = 0;
    virtual valueType *ClientConnect(sint clientNum, bool firstTime) = 0;
    virtual void ClientBegin(sint clientNum) = 0;
    virtual void ClientThink(sint clientNum) = 0;
    virtual void ClientUserinfoChanged(sint clientNum) = 0;
    virtual void ClientDisconnect(sint clientNum) = 0;
    virtual void ClientCommand(sint clientNum) = 0;
    virtual void RunFrame(sint levelTime) = 0;
    virtual bool ConsoleCommand(void) = 0;
    virtual bool SnapshotCallback(sint entityNum, sint clientNum) = 0;
};

extern idSGame *sgame;

#endif // !__SGAME_API_HPP__
