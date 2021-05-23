////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   cgame_api.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CG_API_H__
#define __CG_API_H__

#define CGAME_IMPORT_API_VERSION 5
#define CAM_PRIMARY 0

#define CMD_BACKUP 512
#define CMD_MASK ( CMD_BACKUP - 1 )
// allow a lot of command backups for very fast systems
// multiple commands may be combined into a single packet, so this
// needs to be larger than PACKET_BACKUP

typedef enum cgameEvent_e {
    CGAME_EVENT_NONE,
    CGAME_EVENT_GAMEVIEW,
    CGAME_EVENT_SPEAKEREDITOR,
    CGAME_EVENT_CAMPAIGNBREIFING,
    CGAME_EVENT_DEMO,
    CGAME_EVENT_FIRETEAMMSG,
    CGAME_EVENT_MULTIVIEW
} cgameEvent_t;

struct cgameImports_t {
    void(*Print)(pointer fmt, ...);
    sint(*RealTime)(qtime_t *qtime);

    idClientGameSystem *clientGameSystem;
    idRenderSystem *renderSystem;
    idCollisionModelManager *collisionModelManager;
    idSoundSystem *soundSystem;
    idFileSystem *fileSystem;
    idCVarSystem *cvarSystem;
    idCmdBufferSystem *cmdBufferSystem;
    idCmdSystem *cmdSystem;
    idSystem *idsystem;
    idMemorySystem *memorySystem;
#ifndef DEDICATED
    idClientGUISystem *idGUISystem;
    idClientScreenSystem *clientScreenSystem;
    idParseSystem *parseSystem;
    idClientCinemaSystem *clientCinemaSystem;
    idClientLocalizationSystem *clientLocalization;
    idClientKeysSystem *clientKeysSystem;
    idClientReliableCommandsSystemAPI *clientReliableCommandsSystem;
#endif
};

class idCGame {
public:
    virtual void Init(sint serverMessageNum, sint serverCommandSequence,
                      sint clientNum, bool demoPlayback) = 0;
    // called when the level loads or when the renderer is restarted
    // all media should be registered at this time
    // cgame will display loading status by calling SCR_Update, which
    // will call CG_DrawInformation during the loading process
    // reliableCommandSequence will be 0 on fresh loads, but higher for
    // demos, tourney restarts, or vid_restarts

    virtual void Shutdown() = 0;
    // oportunity to flush and close any open files

    virtual bool ConsoleCommand() = 0;
    // a console command has been issued locally that is not recognized by the
    // main game system.
    // use Cmd_Argc() / Cmd_Argv() to read the command, return false if the
    // command is not known to the game

    virtual void DrawActiveFrame(sint serverTime, stereoFrame_t stereoView,
                                 bool demoPlayback) = 0;
    // Generates and draws a game scene and status information at the given time.
    // If demoPlayback is set, local movement prediction will not be enabled

    virtual void ConsoleText(void) = 0;
    //  pass text that has been printed to the console to cgame
    //  use Cmd_Argc() / Cmd_Argv() to read it

    virtual sint CrosshairPlayer(void) = 0;

    virtual sint LastAttacker(void) = 0;

    virtual void KeyEvent(sint key, bool down) = 0;

    virtual void MouseEvent(sint dx, sint dy) = 0;

    virtual void EventHandling(sint type, bool fForced) = 0;

    virtual bool GetTag(sint clientNum, valueType *tagname,
                        orientation_t *_or) = 0;

    virtual bool CheckExecKey(sint key) = 0;

    virtual bool WantsBindKeys(void) = 0;

    virtual void CompleteCommand(sint argNum) = 0;
    // will callback on all availible completions
    // use Cmd_Argc() / Cmd_Argv() to read the command
};

extern idCGame *cgame;

void trap_Print(pointer fmt);
void trap_Error(pointer fmt);
sint trap_Milliseconds(void);
void trap_Cvar_Register(vmConvar_t *vmCvar, pointer varName,
                        pointer defaultValue, sint flags, pointer description);
void trap_Cvar_Update(vmConvar_t *vmCvar);
void trap_Cvar_Set(pointer var_name, pointer value);
void trap_Cvar_VariableStringBuffer(pointer var_name, valueType *buffer,
                                    uint64 bufsize);
void trap_Cvar_LatchedVariableStringBuffer(pointer var_name,
        valueType *buffer, uint64 bufsize);
sint trap_Argc(void);
void trap_Argv(sint n, valueType *buffer, sint bufferLength);
void trap_Args(valueType *buffer, sint bufferLength);
void trap_LiteralArgs(valueType *buffer, sint bufferLength);
sint trap_GetDemoState(void);
sint trap_GetDemoPos(void);
sint trap_FS_FOpenFile(pointer qpath, fileHandle_t *f, fsMode_t mode);
void trap_FS_Read(void *buffer, sint len, fileHandle_t f);
sint trap_FS_Write(const void *buffer, sint len, fileHandle_t f);
void trap_FS_FCloseFile(fileHandle_t f);
sint trap_FS_GetFileList(pointer path, pointer extension,
                         valueType *listbuf, sint bufsize);
sint trap_FS_Delete(valueType *filename);
void trap_SendConsoleCommand(pointer text);
void trap_AddCommand(pointer cmdName, pointer cmdDesc);
void trap_RemoveCommand(pointer cmdName);
void trap_SendClientCommand(pointer s);
void trap_UpdateScreen(void);
void trap_CM_LoadMap(pointer mapname);
sint trap_CM_NumInlineModels(void);
clipHandle_t trap_CM_InlineModel(sint index);
clipHandle_t trap_CM_TempBoxModel(const vec3_t mins, const vec3_t maxs);
clipHandle_t trap_CM_TempCapsuleModel(const vec3_t mins,
                                      const vec3_t maxs);
sint trap_CM_PointContents(const vec3_t p, clipHandle_t model);
sint trap_CM_TransformedPointContents(const vec3_t p, clipHandle_t model,
                                      const vec3_t origin, const vec3_t angles);
void trap_CM_BoxTrace(trace_t *results, const vec3_t start,
                      const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model,
                      sint brushmask);
void trap_CM_TransformedBoxTrace(trace_t *results, const vec3_t start,
                                 const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model,
                                 sint brushmask, const vec3_t origin, const vec3_t angles);
void trap_CM_CapsuleTrace(trace_t *results, const vec3_t start,
                          const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model,
                          sint brushmask);
void trap_CM_TransformedCapsuleTrace(trace_t *results, const vec3_t start,
                                     const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model,
                                     sint brushmask, const vec3_t origin, const vec3_t angles);
void trap_CM_BiSphereTrace(trace_t *results, const vec3_t start,
                           const vec3_t end, float32 startRad, float32 endRad, clipHandle_t model,
                           sint mask);
void trap_CM_TransformedBiSphereTrace(trace_t *results, const vec3_t start,
                                      const vec3_t end, float32 startRad, float32 endRad, clipHandle_t model,
                                      sint mask, const vec3_t origin);
sint trap_CM_MarkFragments(sint numPoints, const vec3_t *points,
                           const vec3_t projection, sint maxPoints, vec3_t pointBuffer,
                           sint maxFragments, markFragment_t *fragmentBuffer);
void trap_R_ProjectDecal(qhandle_t hShader, sint numPoints, vec3_t *points,
                         vec4_t projection, vec4_t color, sint lifeTime, sint fadeTime);
void trap_R_ClearDecals(void);
void trap_S_StartSound(vec3_t origin, sint entityNum, sint entchannel,
                       sfxHandle_t sfx);
void trap_S_StartLocalSound(sfxHandle_t sfx, sint channelNum);
void trap_S_ClearLoopingSounds(bool killall);
void trap_S_AddLoopingSound(sint entityNum, const vec3_t origin,
                            const vec3_t velocity, sfxHandle_t sfx);
void trap_S_AddRealLoopingSound(sint entityNum, const vec3_t origin,
                                const vec3_t velocity, sfxHandle_t sfx);
void trap_S_StopLoopingSound(sint entityNum);
void trap_S_UpdateEntityPosition(sint entityNum, const vec3_t origin);
void trap_S_Respatialize(sint entityNum, const vec3_t origin,
                         vec3_t axis[3], sint inwater);
void trap_S_StartBackgroundTrack(pointer intro, pointer loop);
void trap_R_LoadWorldMap(pointer mapname);
qhandle_t trap_R_RegisterModel(pointer name);
qhandle_t trap_R_RegisterSkin(pointer name);
bool trap_R_GetSkinModel(qhandle_t skinid, pointer type, valueType *name);
qhandle_t trap_R_GetShaderFromModel(qhandle_t modelid, sint surfnum,
                                    sint withlightmap);
qhandle_t trap_R_RegisterShader(pointer name);
void trap_R_RegisterFont(pointer fontName, sint pointSize,
                         fontInfo_t *font);
qhandle_t trap_R_RegisterShaderNoMip(pointer name);
qhandle_t trap_R_RegisterShaderLightAttenuation(pointer name);
void trap_R_ClearScene(void);
void trap_R_AddRefEntityToScene(const refEntity_t *re);
void trap_R_AddPolyToScene(qhandle_t hShader, sint numVerts,
                           const polyVert_t *verts);
void trap_R_AddPolysToScene(qhandle_t hShader, sint numVerts,
                            const polyVert_t *verts, sint numPolys);
void trap_R_AddLightToScene(const vec3_t org, float32 intensity, float32 r,
                            float32 g, float32 b);
void trap_GS_FS_Seek(fileHandle_t f, sint32 offset, fsOrigin_t origin);
void trap_R_AddCoronaToScene(const vec3_t org, float32 r, float32 g,
                             float32 b, float32 scale, sint id, bool visible);
void trap_R_SetFog(sint fogvar, sint var1, sint var2, float32 r, float32 g,
                   float32 b, float32 density);
void trap_R_SetGlobalFog(bool restore, sint duration, float32 r, float32 g,
                         float32 b, float32 depthForOpaque);
void trap_R_RenderScene(const refdef_t *fd);
void trap_R_RestoreViewParms();
void trap_R_SetColor(const float32 *rgba);
void trap_R_SetClipRegion(const float32 *region);
void trap_R_DrawStretchPic(float32 x, float32 y, float32 w, float32 h,
                           float32 s1, float32 t1, float32 s2, float32 t2, qhandle_t hShader);
void trap_R_DrawRotatedPic(float32 x, float32 y, float32 w, float32 h,
                           float32 s1, float32 t1, float32 s2, float32 t2, qhandle_t hShader,
                           float32 angle);
void trap_R_DrawStretchPicGradient(float32 x, float32 y, float32 w,
                                   float32 h, float32 s1, float32 t1, float32 s2, float32 t2,
                                   qhandle_t hShader, const float32 *gradientColor, sint gradientType);
void trap_R_Add2dPolys(polyVert_t *verts, sint numverts,
                       qhandle_t hShader);
void trap_R_ModelBounds(clipHandle_t model, vec3_t mins, vec3_t maxs);
sint trap_R_LerpTag(orientation_t *tag, clipHandle_t mod, sint startFrame,
                    sint endFrame, float32 frac, pointer tagName);
void trap_GetGlconfig(vidconfig_t *glconfig);
void trap_GetGameState(gameState_t *gamestate);
void trap_GetCurrentSnapshotNumber(sint *snapshotNumber, sint *serverTime);
bool trap_GetSnapshot(sint snapshotNumber, snapshot_t *snapshot);
bool trap_GetServerCommand(sint serverCommandNumber);
sint trap_GetCurrentCmdNumber(void);
bool trap_GetUserCmd(sint cmdNumber, usercmd_t *ucmd);
void trap_SetUserCmdValue(sint stateValue, sint flags,
                          float32 sensitivityScale, sint mpIdentClient);
void trap_SetClientLerpOrigin(float32 x, float32 y, float32 z);
sint trap_MemoryRemaining(void);
bool trap_Key_IsDown(sint keynum);
sint trap_Key_GetCatcher(void);
void trap_Key_SetCatcher(sint catcher);
sint trap_Key_GetKey(pointer binding);
bool trap_Key_GetOverstrikeMode(void);
void trap_Key_SetOverstrikeMode(bool state);
sint trap_PC_AddGlobalDefine(valueType *define);
sint trap_PC_LoadSource(pointer filename);
sint trap_PC_FreeSource(sint handle);
sint trap_PC_ReadToken(sint handle, pc_token_t *pc_token);
sint trap_PC_SourceFileAndLine(sint handle, valueType *filename,
                               sint *line);
void trap_PC_UnReadToken(sint handle);
void trap_S_StopBackgroundTrack(void);
sint trap_RealTime(qtime_t *qtime);
void trap_SnapVector(float32 *v);
sint trap_CIN_PlayCinematic(pointer arg0, sint xpos, sint ypos, sint width,
                            sint height, sint bits);
e_status trap_CIN_StopCinematic(sint handle);
e_status trap_CIN_RunCinematic(sint handle);
void trap_CIN_DrawCinematic(sint handle);
void trap_CIN_SetExtents(sint handle, sint x, sint y, sint w, sint h);
void trap_R_RemapShader(pointer oldShader, pointer newShader,
                        pointer timeOffset);
bool trap_loadCamera(sint camNum, pointer name);
void trap_startCamera(sint camNum, sint time);
void trap_stopCamera(sint camNum);
bool trap_getCameraInfo(sint camNum, sint time, vec3_t *origin,
                        vec3_t *angles, float32 *fov);
bool trap_GetEntityToken(valueType *buffer, uint64 bufferSize);
void trap_UI_Popup(pointer arg0);
void trap_UI_ClosePopup(pointer arg0);
void trap_Key_GetBindingBuf(sint keynum, valueType *buf, sint buflen);
void trap_Key_SetBinding(sint keynum, pointer binding);
void trap_Key_KeynumToStringBuf(sint keynum, valueType *buf, sint buflen);
void trap_Key_KeysForBinding(pointer binding, sint *key1, sint *key2);
void trap_CG_TranslateString(pointer string, valueType *buf);
bool trap_R_inPVS(const vec3_t p1, const vec3_t p2);
void trap_PumpEventLoop(void);
bool trap_R_LoadDynamicShader(pointer shadername, pointer shadertext);
void trap_R_RenderToTexture(sint textureid, sint x, sint y, sint w,
                            sint h);
sint trap_R_GetTextureId(pointer name);
void trap_R_Finish(void);
void trap_GetDemoName(valueType *buffer, sint size);
sint trap_R_LightForPoint(vec3_t point, vec3_t ambientLight,
                          vec3_t directedLight, vec3_t lightDir);
qhandle_t trap_R_RegisterAnimation(pointer name);
void trap_TranslateString(pointer string, valueType *buf);
sint trap_S_SoundDuration(sfxHandle_t handle);
void trap_Cvar_SetValue(pointer var_name, float32 value);

#endif //!__CG_API_H__
