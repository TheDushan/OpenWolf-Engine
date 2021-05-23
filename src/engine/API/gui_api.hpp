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
// File name:   gui_api.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __GUI_PUBLIC_H__
#define __GUI_PUBLIC_H__

#define UI_API_VERSION 1

typedef enum {
    UIMENU_NONE,
    UIMENU_MAIN,
    UIMENU_INGAME,
    UIMENU_NEED_CD,
    UIMENU_BAD_CD_KEY,
    UIMENU_TEAM,
    UIMENU_POSTGAME,
    UIMENU_HELP,
    UIMENU_CLIPBOARD,

    UIMENU_WM_QUICKMESSAGE,
    UIMENU_WM_QUICKMESSAGEALT,

    UIMENU_WM_FTQUICKMESSAGE,
    UIMENU_WM_FTQUICKMESSAGEALT,

    UIMENU_WM_TAPOUT,
    UIMENU_WM_TAPOUT_LMS,

    UIMENU_WM_AUTOUPDATE,

    // ydnar: say, team say, etc
    UIMENU_INGAME_MESSAGEMODE,
    UIMENU_INGAME_OMNIBOTMENU,
} uiMenuCommand_t;

typedef enum {
    SORT_HOST,
    SORT_MAP,
    SORT_CLIENTS,
    SORT_PING,
    SORT_GAME,
    SORT_FILTERS,
    SORT_FAVOURITES
} serverSortField_t;

void trap_Cvar_CopyValue_i(pointer in_var, pointer out_var);
void trap_Error(pointer string);
void trap_Print(pointer string);
sint trap_Milliseconds(void);
void trap_Cvar_Register(vmConvar_t *cvar, pointer var_name, pointer value,
                        sint flags, pointer description);
void trap_Cvar_Update(vmConvar_t *cvar);
void trap_Cvar_Set(pointer var_name, pointer value);
float32 trap_Cvar_VariableValue(pointer var_name);
void trap_Cvar_VariableStringBuffer(pointer var_name, valueType *buffer,
                                    uint64 bufsize);
void trap_Cvar_LatchedVariableStringBuffer(pointer var_name,
        valueType *buffer, uint64 bufsize);
void trap_Cvar_SetValue(pointer var_name, float32 value);
void trap_Cvar_Reset(pointer name);
void trap_Cvar_Create(pointer var_name, pointer var_value, sint flags,
                      pointer description);
void trap_Cvar_InfoStringBuffer(sint bit, valueType *buffer, sint bufsize);
sint trap_Argc(void);
void trap_Argv(sint n, valueType *buffer, sint bufferLength);
void trap_Cmd_ExecuteText(sint exec_when, pointer text);
void trap_AddCommand(pointer cmdName, pointer cmdDesc);
sint trap_FS_FOpenFile(pointer qpath, fileHandle_t *f, fsMode_t mode);
void trap_FS_Read(void *buffer, sint len, fileHandle_t f);
sint trap_FS_Write(const void *buffer, sint len, fileHandle_t f);
void trap_FS_FCloseFile(fileHandle_t f);
sint trap_FS_Delete(valueType *filename);
sint trap_FS_GetFileList(pointer path, pointer extension,
                         valueType *listbuf, sint bufsize);
sint trap_FS_Seek(fileHandle_t f, sint32 offset, sint origin);
qhandle_t trap_R_RegisterModel(pointer name);
qhandle_t trap_R_RegisterSkin(pointer name);
qhandle_t trap_R_RegisterShaderNoMip(pointer name);
void trap_R_ClearScene(void);
void trap_R_AddRefEntityToScene(const refEntity_t *re);
void trap_R_AddPolyToScene(qhandle_t hShader, sint numVerts,
                           const polyVert_t *verts);
void trap_R_AddPolysToScene(qhandle_t hShader, sint numVerts,
                            const polyVert_t *verts, sint numPolys);
void trap_R_AddLightToScene(const vec3_t org, float32 intensity, float32 r,
                            float32 g, float32 b);
void trap_R_AddCoronaToScene(const vec3_t org, float32 r, float32 g,
                             float32 b, float32 scale, sint id, bool visible);
void trap_R_RenderScene(const refdef_t *fd);
void trap_R_SetColor(const float32 *rgba);
void trap_R_SetClipRegion(const float32 *region);
void trap_R_Add2dPolys(polyVert_t *verts, sint numverts,
                       qhandle_t hShader);
void trap_R_DrawStretchPic(float32 x, float32 y, float32 w, float32 h,
                           float32 s1, float32 t1, float32 s2, float32 t2, qhandle_t hShader);
void trap_R_DrawRotatedPic(float32 x, float32 y, float32 w, float32 h,
                           float32 s1, float32 t1, float32 s2, float32 t2, qhandle_t hShader,
                           float32 angle);
void trap_R_ModelBounds(clipHandle_t model, vec3_t mins, vec3_t maxs);
void trap_UpdateScreen(void);
sint trap_CM_LerpTag(orientation_t *tag, const refEntity_t *refent,
                     pointer tagName, sint startIndex);
sfxHandle_t trap_S_RegisterSound(pointer sample);
void trap_S_StartLocalSound(sfxHandle_t sfx, sint channelNum);
void trap_Key_KeynumToStringBuf(sint keynum, valueType *buf, sint buflen);
void trap_Key_GetBindingBuf(sint keynum, valueType *buf, sint buflen);
void trap_Key_SetBinding(sint keynum, pointer binding);
void trap_Key_KeysForBinding(pointer binding, sint *key1, sint *key2);
bool trap_Key_IsDown(sint keynum);
bool trap_Key_GetOverstrikeMode(void);
void trap_Key_SetOverstrikeMode(bool state);
void trap_Key_ClearStates(void);
sint trap_Key_GetCatcher(void);
void trap_Key_SetCatcher(sint catcher);
void trap_GetClipboardData(valueType *buf, sint bufsize);
void trap_GetClientState(uiClientState_t *state);
void trap_GetGlconfig(vidconfig_t *glconfig);
sint trap_GetConfigString(sint index, valueType *buff, uint64 buffsize);
void trap_LAN_LoadCachedServers(void);
void trap_LAN_SaveCachedServers(void);
sint trap_LAN_AddServer(sint source, pointer name, pointer addr);
void trap_LAN_RemoveServer(sint source, pointer addr);
sint trap_LAN_GetPingQueueCount(void);
void trap_LAN_ClearPing(sint n);
void trap_LAN_GetPing(sint n, valueType *buf, sint buflen, sint *pingtime);
void trap_LAN_GetPingInfo(sint n, valueType *buf, sint buflen);
sint trap_LAN_GetServerCount(sint source);
void trap_LAN_GetServerAddressString(sint source, sint n, valueType *buf,
                                     sint buflen);
void trap_LAN_GetServerInfo(sint source, sint n, valueType *buf,
                            sint buflen);
sint trap_LAN_GetServerPing(sint source, sint n);
void trap_LAN_MarkServerVisible(sint source, sint n, bool visible);
sint trap_LAN_ServerIsVisible(sint source, sint n);
bool trap_LAN_UpdateVisiblePings(sint source);
void trap_LAN_ResetPings(sint n);
sint trap_LAN_ServerStatus(valueType *serverAddress,
                           valueType *serverStatus, sint maxLen);
bool trap_LAN_ServerIsInFavoriteList(sint source, sint n);
bool trap_GetNews(bool force);
sint trap_LAN_CompareServers(sint source, sint sortKey, sint sortDir,
                             sint s1, sint s2);
sint trap_MemoryRemaining(void);
void trap_GetCDKey(valueType *buf, sint buflen);
void trap_SetCDKey(valueType *buf);
void trap_R_RegisterFont(pointer fontName, sint pointSize,
                         fontInfo_t *font);
sint trap_PC_AddGlobalDefine(valueType *define);
void trap_PC_RemoveAllGlobalDefines(void);
sint trap_PC_LoadSource(pointer filename);
sint trap_PC_FreeSource(sint handle);
sint trap_PC_ReadToken(sint handle, pc_token_t *pc_token);
sint trap_PC_SourceFileAndLine(sint handle, valueType *filename,
                               sint *line);
void trap_PC_UnReadToken(sint handle);
void trap_S_StopBackgroundTrack(void);
void trap_S_StartBackgroundTrack(pointer intro, pointer loop);
sint trap_RealTime(qtime_t *qtime);
sint trap_CIN_PlayCinematic(pointer arg0, sint xpos, sint ypos, sint width,
                            sint height, sint bits);
e_status trap_CIN_StopCinematic(sint handle);
e_status trap_CIN_RunCinematic(sint handle);
void trap_CIN_DrawCinematic(sint handle);
void trap_CIN_SetExtents(sint handle, sint x, sint y, sint w, sint h);
void trap_R_RemapShader(pointer oldShader, pointer newShader,
                        pointer timeOffset);
bool trap_GetLimboString(sint index, valueType *buf);
valueType *trap_TranslateString(pointer string) __attribute__((format_arg(
            1)));
void trap_CheckAutoUpdate(void);
void trap_GetAutoUpdate(void);
void trap_OpenURL(pointer s);
sint trap_CrosshairPlayer(void);
sint trap_LastAttacker(void);

struct guiImports_t {
    void(*Print)(pointer fmt, ...);
    void(*Error)(sint level, pointer fmt, ...);
    sint(*RealTime)(qtime_t *qtime);

    idRenderSystem *renderSystem;
    idSoundSystem *soundSystem;
    idFileSystem *fileSystem;
    idCVarSystem *cvarSystem;
    idCmdBufferSystem *cmdBufferSystem;
    idCmdSystem *cmdSystem;
    idSystem *idsystem;
    idMemorySystem *memorySystem;
#ifndef DEDICATED
    idCGame *idcgame;
    idClientLANSystem *idLANSystem;
    idClientGUISystem *idGUISystem;
    idClientScreenSystem *clientScreenSystem;
    idParseSystem *parseSystem;
    idClientCinemaSystem *clientCinemaSystem;
    idClientLocalizationSystem *clientLocalization;
    idClientKeysSystem *clientKeysSystem;
    idClientReliableCommandsSystemAPI *clientReliableCommandsSystem;
    idClientAutoUpdateSystemAPI *clientAutoUpdateSystem;
    idClientMainSystemAPI *clientMainSystem;
#endif
};

//
// idUserInterfaceManager
//
class idUserInterfaceManager {
public:
    virtual void Init(bool inGameLoad) = 0;
    virtual void Shutdown(void) = 0;

    virtual void KeyEvent(sint key, bool down) = 0;
    virtual void MouseEvent(sint dx, sint dy) = 0;
    virtual void Refresh(sint time) = 0;

    virtual bool IsFullscreen(void) = 0;

    virtual sint MousePosition(void) = 0;
    virtual void SetMousePosition(sint x, sint y) = 0;

    virtual void SetActiveMenu(uiMenuCommand_t menu) = 0;

    virtual uiMenuCommand_t GetActiveMenu(void) = 0;
    virtual bool ConsoleCommand(sint realTime) = 0;

    // if !overlay, the background will be drawn, otherwise it will be
    // overlayed over whatever the cgame has drawn.
    // a GetClientState syscall will be made to get the current strings
    virtual void DrawConnectScreen(bool overlay) = 0;

    virtual bool CheckExecKey(sint key) = 0;
    virtual bool WantsBindKeys(void) = 0;
};

extern idUserInterfaceManager *uiManager;

#endif
