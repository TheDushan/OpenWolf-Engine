////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 - 2019 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   gui_api.h
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __GUI_PUBLIC_H__
#define __GUI_PUBLIC_H__

#define UI_API_VERSION 1

typedef enum
{
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

typedef enum
{
    SORT_HOST,
    SORT_MAP,
    SORT_CLIENTS,
    SORT_PING,
    SORT_GAME,
    SORT_FILTERS,
    SORT_FAVOURITES
} serverSortField_t;

void trap_Cvar_CopyValue_i( StringEntry in_var, StringEntry out_var );
void trap_Error( StringEntry string );
void trap_Print( StringEntry string );
S32 trap_Milliseconds( void );
void trap_Cvar_Register( vmConvar_t* cvar, StringEntry var_name, StringEntry value, S32 flags, StringEntry description );
void trap_Cvar_Update( vmConvar_t* cvar );
void trap_Cvar_Set( StringEntry var_name, StringEntry value );
F32 trap_Cvar_VariableValue( StringEntry var_name );
void trap_Cvar_VariableStringBuffer( StringEntry var_name, UTF8* buffer, S32 bufsize );
void trap_Cvar_LatchedVariableStringBuffer( StringEntry var_name, UTF8* buffer, S32 bufsize );
void trap_Cvar_SetValue( StringEntry var_name, F32 value );
void trap_Cvar_Reset( StringEntry name );
void trap_Cvar_Create( StringEntry var_name, StringEntry var_value, S32 flags, StringEntry description );
void trap_Cvar_InfoStringBuffer( S32 bit, UTF8* buffer, S32 bufsize );
S32 trap_Argc( void );
void trap_Argv( S32 n, UTF8* buffer, S32 bufferLength );
void trap_Cmd_ExecuteText( S32 exec_when, StringEntry text );
void trap_AddCommand( StringEntry cmdName, StringEntry cmdDesc );
S32 trap_FS_FOpenFile( StringEntry qpath, fileHandle_t* f, fsMode_t mode );
void trap_FS_Read( void* buffer, S32 len, fileHandle_t f );
S32 trap_FS_Write( const void* buffer, S32 len, fileHandle_t f );
void trap_FS_FCloseFile( fileHandle_t f );
S32 trap_FS_Delete( UTF8* filename );
S32 trap_FS_GetFileList( StringEntry path, StringEntry extension, UTF8* listbuf, S32 bufsize );
S32 trap_FS_Seek( fileHandle_t f, S64 offset, S32 origin );
qhandle_t trap_R_RegisterModel( StringEntry name );
qhandle_t trap_R_RegisterSkin( StringEntry name );
qhandle_t trap_R_RegisterShaderNoMip( StringEntry name );
void trap_R_ClearScene( void );
void trap_R_AddRefEntityToScene( const refEntity_t* re );
void trap_R_AddPolyToScene( qhandle_t hShader, S32 numVerts, const polyVert_t* verts );
void trap_R_AddPolysToScene( qhandle_t hShader, S32 numVerts, const polyVert_t* verts, S32 numPolys );
void trap_R_AddLightToScene( const vec3_t org, F32 intensity, F32 r, F32 g, F32 b );
void trap_R_AddCoronaToScene( const vec3_t org, F32 r, F32 g, F32 b, F32 scale, S32 id, bool visible );
void trap_R_RenderScene( const refdef_t* fd );
void trap_R_SetColor( const F32* rgba );
void trap_R_SetClipRegion( const F32* region );
void trap_R_Add2dPolys( polyVert_t* verts, S32 numverts, qhandle_t hShader );
void trap_R_DrawStretchPic( F32 x, F32 y, F32 w, F32 h, F32 s1, F32 t1, F32 s2, F32 t2, qhandle_t hShader );
void trap_R_DrawRotatedPic( F32 x, F32 y, F32 w, F32 h, F32 s1, F32 t1, F32 s2, F32 t2, qhandle_t hShader, F32 angle );
void trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs );
void trap_UpdateScreen( void );
S32 trap_CM_LerpTag( orientation_t* tag, const refEntity_t* refent, StringEntry tagName, S32 startIndex );
sfxHandle_t trap_S_RegisterSound( StringEntry sample );
void trap_S_StartLocalSound( sfxHandle_t sfx, S32 channelNum );
void trap_Key_KeynumToStringBuf( S32 keynum, UTF8* buf, S32 buflen );
void trap_Key_GetBindingBuf( S32 keynum, UTF8* buf, S32 buflen );
void trap_Key_SetBinding( S32 keynum, StringEntry binding );
void trap_Key_KeysForBinding( StringEntry binding, S32* key1, S32* key2 );
bool trap_Key_IsDown( S32 keynum );
bool trap_Key_GetOverstrikeMode( void );
void trap_Key_SetOverstrikeMode( bool state );
void trap_Key_ClearStates( void );
S32 trap_Key_GetCatcher( void );
void trap_Key_SetCatcher( S32 catcher );
void trap_GetClipboardData( UTF8* buf, S32 bufsize );
void trap_GetClientState( uiClientState_t* state );
void trap_GetGlconfig( vidconfig_t* glconfig );
S32 trap_GetConfigString( S32 index, UTF8* buff, S32 buffsize );
void trap_LAN_LoadCachedServers( void );
void trap_LAN_SaveCachedServers( void );
S32 trap_LAN_AddServer( S32 source, StringEntry name, StringEntry addr );
void trap_LAN_RemoveServer( S32 source, StringEntry addr );
S32 trap_LAN_GetPingQueueCount( void );
void trap_LAN_ClearPing( S32 n );
void trap_LAN_GetPing( S32 n, UTF8* buf, S32 buflen, S32* pingtime );
void trap_LAN_GetPingInfo( S32 n, UTF8* buf, S32 buflen );
S32 trap_LAN_GetServerCount( S32 source );
void trap_LAN_GetServerAddressString( S32 source, S32 n, UTF8* buf, S32 buflen );
void trap_LAN_GetServerInfo( S32 source, S32 n, UTF8* buf, S32 buflen );
S32 trap_LAN_GetServerPing( S32 source, S32 n );
void trap_LAN_MarkServerVisible( S32 source, S32 n, bool visible );
S32 trap_LAN_ServerIsVisible( S32 source, S32 n );
bool trap_LAN_UpdateVisiblePings( S32 source );
void trap_LAN_ResetPings( S32 n );
S32 trap_LAN_ServerStatus( UTF8* serverAddress, UTF8* serverStatus, S32 maxLen );
bool trap_LAN_ServerIsInFavoriteList( S32 source, S32 n );
bool trap_GetNews( bool force );
S32 trap_LAN_CompareServers( S32 source, S32 sortKey, S32 sortDir, S32 s1, S32 s2 );
S32 trap_MemoryRemaining( void );
void trap_GetCDKey( UTF8* buf, S32 buflen );
void trap_SetCDKey( UTF8* buf );
void trap_R_RegisterFont( StringEntry fontName, S32 pointSize, fontInfo_t* font );
S32 trap_PC_AddGlobalDefine( UTF8* define );
void trap_PC_RemoveAllGlobalDefines( void );
S32 trap_PC_LoadSource( StringEntry filename );
S32 trap_PC_FreeSource( S32 handle );
S32 trap_PC_ReadToken( S32 handle, pc_token_t* pc_token );
S32 trap_PC_SourceFileAndLine( S32 handle, UTF8* filename, S32* line );
void trap_PC_UnReadToken( S32 handle );
void trap_S_StopBackgroundTrack( void );
void trap_S_StartBackgroundTrack( StringEntry intro, StringEntry loop );
S32 trap_RealTime( qtime_t* qtime );
S32 trap_CIN_PlayCinematic( StringEntry arg0, S32 xpos, S32 ypos, S32 width, S32 height, S32 bits );
e_status trap_CIN_StopCinematic( S32 handle );
e_status trap_CIN_RunCinematic( S32 handle );
void trap_CIN_DrawCinematic( S32 handle );
void trap_CIN_SetExtents( S32 handle, S32 x, S32 y, S32 w, S32 h );
void trap_R_RemapShader( StringEntry oldShader, StringEntry newShader, StringEntry timeOffset );
bool trap_GetLimboString( S32 index, UTF8* buf );
UTF8* trap_TranslateString( StringEntry string ) __attribute__( ( format_arg( 1 ) ) );
void trap_CheckAutoUpdate( void );
void trap_GetAutoUpdate( void );
void trap_OpenURL( StringEntry s );
S32 trap_CrosshairPlayer( void );
S32 trap_LastAttacker( void );

struct guiImports_t
{
    void( *Print )( StringEntry fmt, ... );
    void( *Error )( S32 level, StringEntry fmt, ... );
    void( *CheckAutoUpdate )( void );
    void( *GetAutoUpdate )( void );
    S32( *Parse_LoadSourceHandle )( StringEntry filename );
    S32( *Parse_FreeSourceHandle )( S32 handle );
    S32( *Parse_ReadTokenHandle )( S32 handle, pc_token_t* pc_token );
    S32( *Parse_SourceFileAndLine )( S32 handle, UTF8* filename, S32* line );
    
    void( *SetBinding )( S32 keynum, StringEntry binding );
    void ( *GetBindingByString )( StringEntry binding, S32* key1, S32* key2 );
    bool( *IsDown )( S32 keynum );
    bool ( *GetOverstrikeMode )( void );
    void ( *SetOverstrikeMode )( bool state );
    void ( *ClearStates )( void );
    S32( *Hunk_MemoryRemaining )( void );
    S32( *RealTime )( qtime_t* qtime );
    S32( *PlayCinematic )( StringEntry arg0, S32 xpos, S32 ypos, S32 width, S32 height, S32 bits );
    e_status( *StopCinematic )( S32 handle );
    e_status( *RunCinematic )( S32 handle );
    void( *DrawCinematic )( S32 handle );
    void( *SetExtents )( S32 handle, S32 x, S32 y, S32 w, S32 h );
    void ( *TranslateString )( StringEntry string, UTF8* dest_buffer );
    void ( *OpenURL )( StringEntry s );
    void ( *GetHunkInfo )( S32* hunkused, S32* hunkexpected );
    void( *AddCommand )( StringEntry cmdName, StringEntry cmdDesc );
    
    idRenderSystem* renderSystem;
    idSoundSystem* soundSystem;
    idFileSystem* fileSystem;
    idCVarSystem* cvarSystem;
    idCmdBufferSystem* cmdBufferSystem;
    idCmdSystem* cmdSystem;
    idSystem* idsystem;
    idCGame* idcgame;
    idClientLANSystem* idLANSystem;
    idClientGUISystem* idGUISystem;
    idClientScreenSystem* clientScreenSystem;
};

//
// idUserInterfaceManager
//
class idUserInterfaceManager
{
public:
    virtual void Init( bool inGameLoad ) = 0;
    virtual void Shutdown( void ) = 0;
    
    virtual	void KeyEvent( S32 key, bool down ) = 0;
    virtual void MouseEvent( S32 dx, S32 dy ) = 0;
    virtual void Refresh( S32 time ) = 0;
    
    virtual bool IsFullscreen( void ) = 0;
    
    virtual S32 MousePosition( void ) = 0;
    virtual void SetMousePosition( S32 x, S32 y ) = 0;
    
    virtual void SetActiveMenu( uiMenuCommand_t menu ) = 0;
    
    virtual uiMenuCommand_t GetActiveMenu( void ) = 0;
    virtual bool ConsoleCommand( S32 realTime ) = 0;
    
    // if !overlay, the background will be drawn, otherwise it will be
    // overlayed over whatever the cgame has drawn.
    // a GetClientState syscall will be made to get the current strings
    virtual void DrawConnectScreen( bool overlay ) = 0;
    
    virtual bool CheckExecKey( S32 key ) = 0;
    virtual bool WantsBindKeys( void ) = 0;
};

extern idUserInterfaceManager* uiManager;

#endif
