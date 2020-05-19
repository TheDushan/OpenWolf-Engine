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
// File name:   client.h
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: primary header for client
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENT_H__
#define __CLIENT_H__

#if !defined ( DEDICATED ) && !defined ( UPDATE_SERVER ) && !defined ( BSPC )
#endif

// Dushan - create CL_GUID
// we cannot call it "qkey", "etkey" is already taken,
// so we will change it to etxreal
#define GUIDKEY_FILE "guidopenwolf"
#define GUIDKEY_SIZE 28

#define RETRANSMIT_TIMEOUT  3000	// time between connection packet retransmits

#define LIMBOCHAT_WIDTH     140	// NERVE - SMF - NOTE TTimo buffer size indicator, not related to screen bbox
#define LIMBOCHAT_HEIGHT    7	// NERVE - SMF

// snapshots are a view of the server at a given time
typedef struct
{
    bool        valid;							// cleared if delta parsing was invalid
    S32             snapFlags;						// rate delayed and dropped commands
    S32             serverTime;						// server time the message is valid for (in msec)
    S32             messageNum;						// copied from netchan->incoming_sequence
    S32             deltaNum;						// messageNum the delta is from
    S32             ping;							// time from when cmdNum-1 was sent to time packet was reeceived
    U8            areamask[MAX_MAP_AREA_BYTES];	// portalarea visibility bits
    S32             cmdNum;							// the next cmdNum the server is expecting
    playerState_t   ps;								// complete information about the current player at this time
    S32             numEntities;					// all of the entities that need to be presented
    S32             parseEntitiesNum;				// at the time of this snapshot
    S32             serverCommandNum;				// execute all commands up to this before
    // making the snapshot current
} clSnapshot_t;

// Arnout: for double tapping
typedef struct
{
    S32		pressedTime[DT_NUM];
    S32		releasedTime[DT_NUM];
    S32		lastdoubleTap;
} doubleTap_t;

/*
=============================================================================

the clientActive_t structure is wiped completely at every
new gamestate_t, potentially several times during an established connection

=============================================================================
*/

typedef struct
{
    S32             p_cmdNumber;	// cl.cmdNumber when packet was sent
    S32             p_serverTime;	// usercmd->serverTime when packet was sent
    S32             p_realtime;		// cls.realtime when packet was sent
} outPacket_t;

// the parseEntities array must be large enough to hold PACKET_BACKUP frames of
// entities, so that when a delta compressed message arives from the server
// it can be un-deltad from the original

// Dushan
// MAX_GENTITIES is defined as '1<<GENTITYNUM_BITS' which equates to 1024.
// And because of that reason we increased that 4 times (old limit was already 2k)
#define	MAX_PARSE_ENTITIES ( PACKET_BACKUP * MAX_GENTITIES * 2 )

extern S32      g_console_field_width;

typedef struct
{
    S32             timeoutcount;												// it requres several frames in a timeout condition
    
    // to disconnect, preventing debugging breaks from
    // causing immediate disconnects on continue
    clSnapshot_t    snap;														// latest received from server
    S32             serverTime;													// may be paused during play
    S32             oldServerTime;												// to prevent time from flowing bakcwards
    S32             oldFrameServerTime;											// to check tournament restarts
    S32             serverTimeDelta;											// cl.serverTime = cls.realtime + cl.serverTimeDelta
    // this value changes as net lag varies
    bool        extrapolatedSnapshot;										// set if any cgame frame has been forced to extrapolate
    // cleared when CL_AdjustTimeDelta looks at it
    bool        newSnapshots;												// set on parse of any valid packet
    gameState_t     gameState;													// configstrings
    UTF8            mapname[MAX_QPATH];											// extracted from CS_SERVERINFO
    S32             parseEntitiesNum;											// index (not anded off) into cl_parse_entities[]
    S32             mouseDx[2], mouseDy[2];										// added to by mouse events
    S32             mouseIndex;
    S32             joystickAxis[MAX_JOYSTICK_AXIS];							// set by joystick events
    
    // cgame communicates a few values to the client system
    S32             cgameUserCmdValue;											// current weapon to add to usercmd_t
    S32             cgameFlags;													// flags that can be set by the gamecode
    F32           cgameSensitivity;
    S32             cgameMpIdentClient;											// NERVE - SMF
    vec3_t          cgameClientLerpOrigin;										// DHM - Nerve
    
    // cmds[cmdNumber] is the predicted command, [cmdNumber-1] is the last
    // properly generated command
    usercmd_t       cmds[CMD_BACKUP];											// each mesage will send several old cmds
    S32             cmdNumber;													// incremented each frame, because multiple
    
    // frames may need to be packed into a single packet
    // Arnout: double tapping
    doubleTap_t     doubleTap;
    outPacket_t     outPackets[PACKET_BACKUP];									// information about each packet we have sent out
    
    // the client maintains its own idea of view angles, which are
    // sent to the server each frame.  It is cleared to 0 upon entering each level.
    // the server sends a delta each frame which is added to the locally
    // tracked view angles to account for standing on rotating objects,
    // and teleport direction changes
    vec3_t          viewangles;
    S32             serverId;													// included in each client message so the server
    
    // can tell if it is for a prior map_restart
    // big stuff at end of structure so most offsets are 15 bits or less
    clSnapshot_t    snapshots[PACKET_BACKUP];
    entityState_t   entityBaselines[MAX_GENTITIES];								// for delta compression when not in previous frame
    entityState_t   parseEntities[MAX_PARSE_ENTITIES];
    
    // NERVE - SMF
    // NOTE TTimo - UI uses LIMBOCHAT_WIDTH strings (140),
    // but for the processing in CL_AddToLimboChat we need some safe room
    UTF8            limboChatMsgs[LIMBOCHAT_HEIGHT][LIMBOCHAT_WIDTH * 3 + 1];
    S32             limboChatPos;
    bool        corruptedTranslationFile;
    UTF8            translationVersion[MAX_STRING_TOKENS];
    // -NERVE - SMF
    
    bool        cameraMode;
} clientActive_t;

extern clientActive_t cl;

/*
=============================================================================

the clientConnection_t structure is wiped when disconnecting from a server,
either to go to a full screen console, play a demo, or connect to a different server

A connection can be to either a server through the network layer or a
demo through a file.

=============================================================================
*/


typedef struct
{
    S32						clientNum;
    S32						lastPacketSentTime;											// for retransmits during connection
    S32						lastPacketTime;												// for timeouts
    netadr_t				serverAddress;
    S32						connectTime;												// for connection retransmits
    S32						connectPacketCount;											// for display on connection dialog
    UTF8					serverMessage[MAX_STRING_TOKENS];							// for display on connection dialog
    S32						challenge;													// from the server to use for connecting
    S32						checksumFeed;												// from the server for checksum calculations
    S32						onlyVisibleClients;											// DHM - Nerve
    // these are our reliable messages that go to the server
    S32						reliableSequence;
    S32						reliableAcknowledge;										// the last one the server has executed
    // TTimo - NOTE: incidentally, reliableCommands[0] is never used (always start at reliableAcknowledge+1)
    UTF8					reliableCommands[MAX_RELIABLE_COMMANDS][MAX_TOKEN_CHARS];
    // server message (unreliable) and command (reliable) sequence
    // numbers are NOT cleared at level changes, but continue to
    // increase as long as the connection is valid
    // message sequence is used by both the network layer and the
    // delta compression layer
    S32						serverMessageSequence;
    // reliable messages received from server
    S32						serverCommandSequence;
    S32						lastExecutedServerCommand;									// last server command grabbed or executed with idClientGameSystemLocal::GetServerCommand
    UTF8					serverCommands[MAX_RELIABLE_COMMANDS][MAX_TOKEN_CHARS];
    // file transfer from server
    fileHandle_t			download;
    S32						downloadNumber;
    S32						downloadBlock;												// block we are waiting for
    S32						downloadCount;												// how many bytes we got
    S32						downloadSize;												// how many bytes we got
    S32						downloadFlags;												// misc download behaviour flags sent by the server
    UTF8					downloadList[MAX_INFO_STRING];								// list of paks we need to download
    
    // www downloading
    bool				bWWWDl;														// we have a www download going
    bool				bWWWDlAborting;												// disable the CL_WWWDownload until server gets us a gamestate (used for aborts)
    UTF8					redirectedList[MAX_INFO_STRING];							// list of files that we downloaded through a redirect since last FS_ComparePaks
    UTF8					badChecksumList[MAX_INFO_STRING];							// list of files for which wwwdl redirect is broken (wrong checksum)
    UTF8					newsString[ MAX_NEWS_STRING ];
    
    // demo information
    UTF8					demoName[MAX_QPATH];
    bool				demorecording;
    bool				demoplaying;
    bool				demowaiting;												// don't record until a non-delta message is received
    bool				firstDemoFrameSkipped;
    fileHandle_t			demofile;
    
    bool				waverecording;
    fileHandle_t			wavefile;
    S32						wavetime;
    
    S32						timeDemoFrames;	// counter of rendered frames
    S32						timeDemoStart;	// cls.realtime before first frame
    S32						timeDemoBaseTime;	// each frame will be at this time + frameNum * 50
    
    // big stuff at end of structure so most offsets are 15 bits or less
    netchan_t				netchan;
} clientConnection_t;

extern clientConnection_t clc;

/*
==================================================================

the clientStatic_t structure is never wiped, and is used even when
no client connection is active at all

==================================================================
*/

typedef struct
{
    netadr_t        adr;
    S32             start;
    S32             time;
    UTF8            info[MAX_INFO_STRING];
} ping_t;

typedef struct
{
    netadr_t        adr;
    UTF8            hostName[MAX_NAME_LENGTH];
    S32             load;
    UTF8            mapName[MAX_NAME_LENGTH];
    UTF8            game[MAX_NAME_LENGTH];
    S32             netType;
    S32             gameType;
    S32             clients;
    S32             maxClients;
    S32             minPing;
    S32             maxPing;
    S32             ping;
    bool        visible;
    S32             allowAnonymous;
    S32             friendlyFire;				// NERVE - SMF
    S32             maxlives;					// NERVE - SMF
    S32             needpass;
    S32             antilag;					// TTimo
    S32             weaprestrict;
    S32             balancedteams;
    UTF8            gameName[MAX_NAME_LENGTH];	// Arnout
} serverInfo_t;

typedef struct
{
    connstate_t     state;															// connection status
    S32             keyCatchers;													// bit flags
    bool            doCachePurge;													// Arnout: empty the renderer cache as soon as possible
    UTF8            servername[MAX_OSPATH];											// name of server from original connect (used by reconnect)
    // when the server clears the hunk, all of these must be restarted
    bool            rendererStarted;
    bool            soundStarted;
    bool            soundRegistered;
    bool            uiStarted;
    bool            cgameStarted;
    S32             framecount;
    S32             frametime;														// msec since last frame
    S32             realtime;														// ignores pause
    S32             realFrametime;													// ignoring pause, so console always works
    S32             numlocalservers;
    serverInfo_t    localServers[MAX_OTHER_SERVERS];
    S32             numglobalservers;
    serverInfo_t    globalServers[MAX_GLOBAL_SERVERS];
    // additional global servers
    S32             numGlobalServerAddresses;
    netadr_t		globalServerAddresses[MAX_GLOBAL_SERVERS];
    S32             numfavoriteservers;
    serverInfo_t    favoriteServers[MAX_OTHER_SERVERS];
    S32             pingUpdateSource;												// source currently pinging or updating
    S32             masterNum;
    // update server info
    netadr_t        updateServer;
    UTF8            updateChallenge[MAX_TOKEN_CHARS];
    UTF8            updateInfoString[MAX_INFO_STRING];
    netadr_t        authorizeServer;
    // DHM - Nerve :: Auto-update Info
    UTF8            autoupdateServerNames[MAX_AUTOUPDATE_SERVERS][MAX_QPATH];
    netadr_t        autoupdateServer;
    bool        autoUpdateServerChecked[MAX_AUTOUPDATE_SERVERS];
    S32             autoupdatServerFirstIndex;										// to know when we went through all of them
    S32             autoupdatServerIndex;											// to cycle through them
    // rendering info
    vidconfig_t      glconfig;
    qhandle_t       charSetShader;
    qhandle_t       whiteShader;
    qhandle_t       consoleShader;
    qhandle_t       consoleShader2;													// NERVE - SMF - merged from WolfSP
    bool        useLegacyConsoleFont;
    fontInfo_t      consoleFont;
    // www downloading
    // in the static stuff since this may have to survive server disconnects
    // if new stuff gets added, CL_ClearStaticDownload code needs to be updated for clear up
    bool        bWWWDlDisconnected;												// keep going with the download after server disconnect
    UTF8            downloadName[MAX_OSPATH];
    UTF8            downloadTempName[MAX_OSPATH];									// in wwwdl mode, this is OS path (it's a qpath otherwise)
    UTF8            originalDownloadName[MAX_QPATH];								// if we get a redirect, keep a copy of the original file path
    bool        downloadRestart;												// if true, we need to do another FS_Restart because we downloaded a pak
    S32 lastVidRestart;
} clientStatic_t;

extern clientStatic_t cls;

//=============================================================================

extern void*    cgvm;			// interface to cgame dll or vm
extern void*    uivm;
extern void*    dbvm;
extern idCGame*	cgame;
extern idUserInterfaceManager* uiManager;

extern struct rsa_public_key public_key;
extern struct rsa_private_key private_key;

//
// cvars
//
extern convar_t*  cl_nodelta;
extern convar_t*  cl_debugMove;
extern convar_t*  cl_noprint;
extern convar_t*  cl_timegraph;
extern convar_t*  cl_maxpackets;
extern convar_t*  cl_packetdup;
extern convar_t*  cl_shownet;
extern convar_t*  cl_shownuments;			// DHM - Nerve
extern convar_t*  cl_showSend;
extern convar_t*  cl_showServerCommands;	// NERVE - SMF
extern convar_t*  cl_timeNudge;
extern convar_t*  cl_showTimeDelta;
extern convar_t*  cl_freezeDemo;

extern convar_t*  cl_yawspeed;
extern convar_t*  cl_pitchspeed;
extern convar_t*  cl_run;
extern convar_t*  cl_anglespeedkey;

extern convar_t*  cl_recoilPitch;	// RF

extern convar_t*  cl_bypassMouseInput;	// NERVE - SMF

extern convar_t*  cl_doubletapdelay;

extern convar_t*  cl_sensitivity;
extern convar_t*  cl_freelook;

extern convar_t*  cl_xbox360ControllerAvailable;

extern convar_t*  cl_mouseAccel;
extern convar_t*  cl_mouseAccelOffset;
extern convar_t*  cl_mouseAccelStyle;
extern convar_t*  cl_showMouseRate;

extern convar_t*  m_pitch;
extern convar_t*  m_yaw;
extern convar_t*  m_forward;
extern convar_t*  m_side;
extern convar_t*  m_filter;

extern convar_t*  j_pitch;
extern convar_t*  j_yaw;
extern convar_t*  j_forward;
extern convar_t*  j_side;
extern convar_t*  j_up;
extern convar_t*  j_up_axis;
extern convar_t*  j_pitch_axis;
extern convar_t*  j_yaw_axis;
extern convar_t*  j_forward_axis;
extern convar_t*  j_side_axis;

extern convar_t*  cl_IRC_connect_at_startup;
extern convar_t*  cl_IRC_server;
extern convar_t*  cl_IRC_channel;
extern convar_t*  cl_IRC_port;
extern convar_t*  cl_IRC_override_nickname;
extern convar_t*  cl_IRC_nickname;
extern convar_t*  cl_IRC_kick_rejoin;
extern convar_t*  cl_IRC_reconnect_delay;

extern convar_t*  cl_timedemo;

extern convar_t*  cl_activeAction;
extern convar_t*  cl_autorecord;

extern convar_t*  cl_allowDownload;
extern convar_t*  cl_conXOffset;
extern convar_t*  cl_inGameVideo;
extern convar_t*  cl_authserver;

extern convar_t*  cl_missionStats;
extern convar_t*  cl_waitForFire;
extern convar_t*  cl_altTab;

// NERVE - SMF - localization
extern convar_t*  cl_language;
// -NERVE - SMF

extern convar_t*  cl_profile;
extern convar_t*  cl_defaultProfile;

extern convar_t*  cl_consoleKeys;
extern convar_t*  cl_consoleFont;
extern convar_t*  cl_consoleFontSize;
extern convar_t*  cl_consoleFontKerning;
extern convar_t*  cl_consolePrompt;
extern convar_t*  cl_aviFrameRate;
extern convar_t*  cl_aviMotionJpeg;
extern convar_t*  cl_guidServerUniq;

//bani
extern convar_t* sv_cheats;

//=================================================

void            Key_GetBindingByString( StringEntry binding, S32* key1, S32* key2 );

//
// cl_main
//

void            CL_Init( void );
void            CL_FlushMemory( void );
void            CL_ShutdownAll( void );
void            CL_AddReliableCommand( StringEntry cmd );

void            CL_StartHunkUsers( void );

#if !defined(UPDATE_SERVER)
void            CL_CheckAutoUpdate( void );
bool            CL_NextUpdateServer( void );
void            CL_GetAutoUpdate( void );
#endif

void            CL_Disconnect_f( void );
void            CL_GetChallengePacket( void );
void            CL_Vid_Restart_f( void );
void            CL_Snd_Restart_f( void );
void            CL_NextDemo( void );
void            CL_ReadDemoMessage( void );
void            CL_StartDemoLoop( void );
demoState_t     CL_DemoState( void );
S32             CL_DemoPos( void );
void            CL_DemoName( UTF8* buffer, S32 size );

void            CL_WriteWaveFilePacket();

void            CL_InitDownloads( void );
void            CL_NextDownload( void );

void            CL_GetPing( S32 n, UTF8* buf, S32 buflen, S32* pingtime );
void            CL_GetPingInfo( S32 n, UTF8* buf, S32 buflen );
void            CL_ClearPing( S32 n );
S32             CL_GetPingQueueCount( void );

void            CL_ShutdownRef( void );
void            CL_InitRef( void );

S32             CL_ServerStatus( UTF8* serverAddress, UTF8* serverStatusString, S32 maxLen );

void            CL_AddToLimboChat( StringEntry str );	// NERVE - SMF
bool        CL_GetLimboString( S32 index, UTF8* buf );	// NERVE - SMF

// NERVE - SMF - localization
void            CL_InitTranslation();
void            CL_SaveTransTable( StringEntry fileName, bool newOnly );
void            CL_ReloadTranslation();
void            CL_TranslateString( StringEntry string, UTF8* dest_buffer );
StringEntry     CL_TranslateStringBuf( StringEntry string ) __attribute__( ( format_arg( 1 ) ) ); // TTimo
// -NERVE - SMF

void            CL_OpenURL( StringEntry url );	// TTimo
void            CL_Record( StringEntry name );

//
// cl_input
//
typedef struct
{
    S32             down[2];	// key nums holding it down
    U32        downtime;	// msec timestamp
    U32        msec;		// msec down this frame if both a down and up happened
    bool        active;		// current state
    bool        wasPressed;	// set when down, not cleared when up
} kbutton_t;

typedef enum
{
    KB_LEFT,
    KB_RIGHT,
    KB_FORWARD,
    KB_BACK,
    KB_LOOKUP,
    KB_LOOKDOWN,
    KB_MOVELEFT,
    KB_MOVERIGHT,
    KB_STRAFE,
    KB_SPEED,
    KB_UP,
    KB_DOWN,
    KB_BUTTONS0,
    KB_BUTTONS1,
    KB_BUTTONS2,
    KB_BUTTONS3,
    KB_BUTTONS4,
    KB_BUTTONS5,
    KB_BUTTONS6,
    KB_BUTTONS7,
    KB_BUTTONS8,
    KB_BUTTONS9,
    KB_BUTTONS10,
    KB_BUTTONS11,
    KB_BUTTONS12,
    KB_BUTTONS13,
    KB_BUTTONS14,
    KB_BUTTONS15,
    KB_WBUTTONS0,
    KB_WBUTTONS1,
    KB_WBUTTONS2,
    KB_WBUTTONS3,
    KB_WBUTTONS4,
    KB_WBUTTONS5,
    KB_WBUTTONS6,
    KB_WBUTTONS7,
    KB_MLOOK,
    // Dushan
    NUM_BUTTONS
} kbuttons_t;


void            CL_ClearKeys( void );
void            CL_InitInput( void );
void            CL_SendCmd( void );
void            CL_ClearState( void );
void            CL_ReadPackets( void );
void            CL_WritePacket( void );
//void			IN_CenterView (void);
void            IN_Notebook( void );
void            IN_Help( void );
//----(SA) salute
void            IN_Salute( void );
//----(SA)
F32           CL_KeyState( kbutton_t* key );
S32             Key_StringToKeynum( UTF8* str );
UTF8*           Key_KeynumToString( S32 keynum );

//
// cl_parse.c
//
extern S32      cl_connectedToPureServer;
void            CL_SystemInfoChanged( void );
void            CL_ParseServerMessage( msg_t* msg );

//====================================================================

void            CL_UpdateInfoPacket( netadr_t from );	// DHM - Nerve
void            CL_ServerInfoPacket( netadr_t from, msg_t* msg );
void            CL_LocalServers_f( void );
void            CL_GlobalServers_f( void );
void            CL_FavoriteServers_f( void );
void            CL_Ping_f( void );
bool        CL_UpdateVisiblePings_f( S32 source );
void			CL_GenGuid( msg_t* msg );

//
// console
//
void            Con_DrawCharacter( S32 cx, S32 line, S32 num );
void            Con_CheckResize( void );
void            Con_Init( void );
void            Con_Clear_f( void );
void            Con_ToggleConsole_f( void );
void            Con_OpenConsole_f( void );
void            Con_DrawNotify( void );
void            Con_ClearNotify( void );
void            Con_RunConsole( void );
void            Con_DrawConsole( void );
void            Con_PageUp( void );
void            Con_PageDown( void );
void            Con_Top( void );
void            Con_Bottom( void );
void            Con_Close( void );
void            CL_LoadConsoleHistory( void );
void            CL_SaveConsoleHistory( void );
StringEntry     Con_GetText( S32 console );

//
// cl_scrn.c
//
void            SCR_Init( void );
void            SCR_UpdateScreen( void );
void            SCR_DebugGraph( F32 value, S32 color );
S32             SCR_GetBigStringWidth( StringEntry str );	// returns in virtual 640x480 coordinates
void            SCR_AdjustFrom640( F32* x, F32* y, F32* w, F32* h );
void            SCR_FillRect( F32 x, F32 y, F32 width, F32 height, const F32* color );
void            SCR_DrawPic( F32 x, F32 y, F32 width, F32 height, qhandle_t hShader );
void            SCR_DrawNamedPic( F32 x, F32 y, F32 width, F32 height, StringEntry picname );
void            SCR_DrawBigString( S32 x, S32 y, StringEntry s, F32 alpha, bool noColorEscape );	// draws a string with embedded color control characters with fade
void            SCR_DrawBigStringColor( S32 x, S32 y, StringEntry s, vec4_t color, bool noColorEscape );	// ignores embedded color control characters
void            SCR_DrawSmallStringExt( S32 x, S32 y, StringEntry string, F32* setColor, bool forceColor, bool noColorEscape );
void            SCR_DrawSmallChar( S32 x, S32 y, S32 ch );
void            SCR_DrawConsoleFontChar( F32 x, F32 y, S32 ch );
F32           SCR_ConsoleFontCharWidth( S32 ch );
F32           SCR_ConsoleFontCharHeight( void );
F32           SCR_ConsoleFontStringWidth( StringEntry s, S32 len );

//
// cl_cin.c
//

void            CL_PlayCinematic_f( void );
void            SCR_DrawCinematic( void );
void            SCR_RunCinematic( void );
void            SCR_StopCinematic( void );
S32             CIN_PlayCinematic( StringEntry arg0, S32 xpos, S32 ypos, S32 width, S32 height, S32 bits );
e_status        CIN_StopCinematic( S32 handle );
e_status        CIN_RunCinematic( S32 handle );
void            CIN_DrawCinematic( S32 handle );
void            CIN_SetExtents( S32 handle, S32 x, S32 y, S32 w, S32 h );
void            CIN_SetLooping( S32 handle, bool loop );
void            CIN_UploadCinematic( S32 handle );
void            CIN_CloseAllVideos( void );

// yuv->rgb will be used for Theora(ogm)
void			ROQ_GenYUVTables( void );
void			Frame_yuv_to_rgb24( const U8* y, const U8* u, const U8* v, S32 width, S32 height, S32 y_stride, S32 uv_stride, S32 yWShift, S32 uvWShift, S32 yHShift, S32 uvHShift, U32* output );

//
// cl_net_chan.c
//
void            CL_Netchan_Transmit( netchan_t* chan, msg_t* msg );
void            CL_Netchan_TransmitNextFragment( netchan_t* chan );
bool            CL_Netchan_Process( netchan_t* chan, msg_t* msg );

//
// cl_main.c
//
void            CL_WriteDemoMessage( msg_t* msg, S32 headerBytes );
void            CL_RequestMotd( void );

#endif //!__CLIENT_H__
