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
// File name:   qcommon.h
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: definitions common between client and server, but not game or ref module
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __QCOMMON_H__
#define __QCOMMON_H__

//bani
#if defined __GNUC__ || defined __clang__
#define _attribute( x ) __attribute__( x )
#else
#define _attribute( x )
#endif

//#define PRE_RELEASE_DEMO
#ifdef PRE_RELEASE_DEMO
#define PRE_RELEASE_DEMO_NODEVMAP
#endif							// PRE_RELEASE_DEMO

//============================================================================

//
// msg.c
//
typedef struct msg_s
{
    bool        allowoverflow;	// if false, do a Com_Error
    bool        overflowed;	// set to true if the buffer size failed (with allowoverflow set)
    bool        oob;		// set to true if the buffer size failed (with allowoverflow set)
    U8*           data;
    S32             maxsize;
    S32             cursize;
    S32             uncompsize;	// NERVE - SMF - net debugging
    S32             readcount;
    S32             bit;		// for bitwise reads and writes
} msg_t;

void            MSG_Init( msg_t* buf, U8* data, S32 length );
void            MSG_InitOOB( msg_t* buf, U8* data, S32 length );
void            MSG_Clear( msg_t* buf );
void*           MSG_GetSpace( msg_t* buf, S32 length );
void            MSG_WriteData( msg_t* buf, const void* data, S32 length );
void            MSG_Bitstream( msg_t* buf );
void            MSG_Uncompressed( msg_t* buf );

// TTimo
// copy a msg_t in case we need to store it as is for a bit
// (as I needed this to keep an msg_t from a static var for later use)
// sets data buffer as MSG_Init does prior to do the copy
void            MSG_Copy( msg_t* buf, U8* data, S32 length, msg_t* src );

struct usercmd_s;
struct entityState_s;
struct playerState_s;

void            MSG_WriteBits( msg_t* msg, S32 value, S32 bits );

void            MSG_WriteChar( msg_t* sb, S32 c );
void            MSG_WriteByte( msg_t* sb, S32 c );
void            MSG_WriteShort( msg_t* sb, S32 c );
void            MSG_WriteLong( msg_t* sb, S32 c );
void            MSG_WriteFloat( msg_t* sb, F32 f );
void            MSG_WriteString( msg_t* sb, StringEntry s );
void            MSG_WriteBigString( msg_t* sb, StringEntry s );
void            MSG_WriteAngle16( msg_t* sb, F32 f );

void            MSG_BeginReading( msg_t* sb );
void            MSG_BeginReadingOOB( msg_t* sb );
void            MSG_BeginReadingUncompressed( msg_t* msg );

S32             MSG_ReadBits( msg_t* msg, S32 bits );

S32             MSG_ReadChar( msg_t* sb );
S32             MSG_ReadByte( msg_t* sb );
S32             MSG_ReadShort( msg_t* sb );
S32             MSG_ReadLong( msg_t* sb );
F32           MSG_ReadFloat( msg_t* sb );
UTF8*           MSG_ReadString( msg_t* sb );
UTF8*           MSG_ReadBigString( msg_t* sb );
UTF8*           MSG_ReadStringLine( msg_t* sb );
F32           MSG_ReadAngle16( msg_t* sb );
void            MSG_ReadData( msg_t* sb, void* buffer, S32 size );
S32				MSG_LookaheadByte( msg_t* msg );

void            MSG_WriteDeltaUsercmd( msg_t* msg, struct usercmd_s* from, struct usercmd_s* to );
void            MSG_ReadDeltaUsercmd( msg_t* msg, struct usercmd_s* from, struct usercmd_s* to );

void            MSG_WriteDeltaUsercmdKey( msg_t* msg, S32 key, usercmd_t* from, usercmd_t* to );
void            MSG_ReadDeltaUsercmdKey( msg_t* msg, S32 key, usercmd_t* from, usercmd_t* to );

void            MSG_WriteDeltaEntity( msg_t* msg, struct entityState_s* from, struct entityState_s* to, bool force );
void            MSG_ReadDeltaEntity( msg_t* msg, entityState_t* from, entityState_t* to, S32 number );

void            MSG_WriteDeltaPlayerstate( msg_t* msg, struct playerState_s* from, struct playerState_s* to );
void            MSG_ReadDeltaPlayerstate( msg_t* msg, struct playerState_s* from, struct playerState_s* to );
void			MSG_WriteDeltaSharedEntity( msg_t* msg, void* from, void* to, bool force, S32 number );
void			MSG_ReadDeltaSharedEntity( msg_t* msg, void* from, void* to, S32 number );

//============================================================================

/*
==============================================================

NET

==============================================================
*/

#define NET_ENABLEV4            0x01
#define NET_ENABLEV6            0x02
// if this flag is set, always attempt ipv6 connections instead of ipv4 if a v6 address is found.
#define NET_PRIOV6              0x04
// disables ipv6 multicast support if set.
#define NET_DISABLEMCAST        0x08

#ifndef DEDICATED
#define PACKET_BACKUP	256
#else
#define	PACKET_BACKUP	32	// number of old messages that must be kept on client and server for delta comrpession and ping estimation
#endif
#define	PACKET_MASK		(PACKET_BACKUP-1)

#define MAX_PACKET_USERCMDS     32      // max number of usercmd_t in a packet

#define PORT_ANY            -1

#define MAX_MASTER_SERVERS  5
#define MAX_RCON_LIST		5

// RF, increased this, seems to keep causing problems when set to 64, especially when loading
// a savegame, which is hard to fix on that side, since we can't really spread out a loadgame
// among several frames
//#define   MAX_RELIABLE_COMMANDS   64          // max string commands buffered for restransmit
//#define   MAX_RELIABLE_COMMANDS   128         // max string commands buffered for restransmit
#define MAX_RELIABLE_COMMANDS   256	// bigger!

typedef enum
{
    NA_BAD = 0,                 // an address lookup failed
    NA_BOT,
    NA_LOOPBACK,
    NA_BROADCAST,
    NA_IP,
    NA_IP6,
    NA_MULTICAST6,
    NA_UNSPEC
} netadrtype_t;

typedef enum
{
    NS_CLIENT,
    NS_SERVER
} netsrc_t;

#define NET_ADDRSTRMAXLEN 48	// maximum length of an IPv6 address string including trailing '\0'
typedef struct
{
    netadrtype_t type;
    
    U8 ip[4];
    U8 ip6[16];
    
    U16 port;
    U64 scope_id;	// Needed for IPv6 link-local addresses
} netadr_t;

void			NET_Init( void );
void            NET_Shutdown( void );
void			NET_Restart_f( void );
void			NET_Config( bool enableNetworking );
void            NET_FlushPacketQueue( void );
void			NET_SendPacket( netsrc_t sock, S32 length, const void* data, netadr_t to );
void      NET_OutOfBandPrint( netsrc_t net_socket, netadr_t adr, StringEntry format, ... ) __attribute__( ( format( printf, 3, 4 ) ) );
void 	NET_OutOfBandData( netsrc_t sock, netadr_t adr, U8* format, S32 len );

bool		NET_CompareAdr( netadr_t a, netadr_t b );
bool		NET_CompareBaseAdr( netadr_t a, netadr_t b );
bool		NET_IsLocalAddress( netadr_t adr );
bool		NET_IsIPXAddress( StringEntry buf );
StringEntry		NET_AdrToString( netadr_t a );
StringEntry		NET_AdrToStringwPort( netadr_t a );
S32				NET_StringToAdr( StringEntry s, netadr_t* a, netadrtype_t family );
bool		NET_GetLoopPacket( netsrc_t sock, netadr_t* net_from, msg_t* net_message );
void            NET_JoinMulticast6( void );
void            NET_LeaveMulticast6( void );

void			NET_Sleep( S32 msec );

bool Net_IsLANAddress( netadr_t adr );
bool Net_GetPacket( netadr_t* net_from, msg_t* net_message );
void Net_SendPacket( S32 length, const void* data, netadr_t to );
void Net_ShowIP( void );
bool Net_StringToAdr( StringEntry s, netadr_t* a, netadrtype_t family );

//----(SA)  increased for larger submodel entity counts
#define MAX_MSGLEN					32768		// max length of a message, which may
//#define   MAX_MSGLEN              16384       // max length of a message, which may
// be fragmented into multiple packets
#define MAX_DOWNLOAD_WINDOW         8	// max of eight download frames
#define MAX_DOWNLOAD_BLKSIZE        2048	// 2048 U8 block chunks

/*
Netchan handles packet fragmentation and out of order / duplicate suppression
*/

typedef struct
{
    netsrc_t sock;
    
    S32 dropped;	// between last packet and previous
    
    netadr_t remoteAddress;
    S32 qport;		// qport value to write when transmitting
    
    // sequencing variables
    S32 incomingSequence;
    S32 outgoingSequence;
    
    // incoming fragment assembly buffer
    S32 fragmentSequence;
    S32 fragmentLength;
    U8 fragmentBuffer[MAX_MSGLEN];
    
    // outgoing fragment buffer
    // we need to space out the sending of large fragmented messages
    bool unsentFragments;
    S32 unsentFragmentStart;
    S32 unsentLength;
    U8 unsentBuffer[MAX_MSGLEN];
    S32 lastSentTime;
    S32 lastSentSize;
} netchan_t;

void            Netchan_Init( S32 qport );
void            Netchan_Setup( netsrc_t sock, netchan_t* chan, netadr_t adr, S32 qport );

void            Netchan_Transmit( netchan_t* chan, S32 length, const U8* data );
void            Netchan_TransmitNextFragment( netchan_t* chan );

bool        Netchan_Process( netchan_t* chan, msg_t* msg );


/*
==============================================================

PROTOCOL

==============================================================
*/

// sent by the server, printed on connection screen, works for all clients
// (restrictions: does not handle \n, no more than 256 chars)
#define PROTOCOL_MISMATCH_ERROR "ERROR: Protocol Mismatch Between Client and Server.\
The server you are attempting to join is running an incompatible version of the game."

// long version used by the client in diagnostic window
#define PROTOCOL_MISMATCH_ERROR_LONG "ERROR: Protocol Mismatch Between Client and Server.\n\n\
The server you attempted to join is running an incompatible version of the game.\n\
You or the server may be running older versions of the game. Press the auto-update\
 button if it appears on the Main Menu screen."

#define GAMENAME_STRING "CelestialHarvest"
#ifndef PRE_RELEASE_DEMO
// 2.56 - protocol 83
// 2.4 - protocol 80
// 1.33 - protocol 59
// 1.4 - protocol 60
#define ETPROTOCOL_VERSION    2
#else
// the demo uses a different protocol version for independant browsing
#define ETPROTOCOL_VERSION    1
#endif

// maintain a list of compatible protocols for demo playing
// NOTE: that stuff only works with two digits protocols
extern S32 demo_protocols[];

// NERVE - SMF - wolf multiplayer master servers
#ifndef MASTER_SERVER_NAME
#define MASTER_SERVER_NAME      "74.91.120.190"
#endif
#define MOTD_SERVER_NAME        "74.91.120.190"//"etmotd.idsoftware.com" // ?.?.?.?

// TTimo: override autoupdate server for testing
#ifndef AUTOUPDATE_SERVER_NAME
#define AUTOUPDATE_SERVER_NAME "74.91.120.190"
//#define AUTOUPDATE_SERVER_NAME "au2rtcw2.activision.com"
#endif

// TTimo: allow override for easy dev/testing..
// FIXME: not planning to support more than 1 auto update server
// see cons -- update_server=myhost
#define MAX_AUTOUPDATE_SERVERS  5
#if !defined( AUTOUPDATE_SERVER_NAME )
#define AUTOUPDATE_SERVER1_NAME   "au2rtcw1.activision.com"	// DHM - Nerve
#define AUTOUPDATE_SERVER2_NAME   "au2rtcw2.activision.com"	// DHM - Nerve
#define AUTOUPDATE_SERVER3_NAME   "au2rtcw3.activision.com"	// DHM - Nerve
#define AUTOUPDATE_SERVER4_NAME   "au2rtcw4.activision.com"	// DHM - Nerve
#define AUTOUPDATE_SERVER5_NAME   "au2rtcw5.activision.com"	// DHM - Nerve
#else
#define AUTOUPDATE_SERVER1_NAME   AUTOUPDATE_SERVER_NAME
#define AUTOUPDATE_SERVER2_NAME   AUTOUPDATE_SERVER_NAME
#define AUTOUPDATE_SERVER3_NAME   AUTOUPDATE_SERVER_NAME
#define AUTOUPDATE_SERVER4_NAME   AUTOUPDATE_SERVER_NAME
#define AUTOUPDATE_SERVER5_NAME   AUTOUPDATE_SERVER_NAME
#endif

#define PORT_MASTER         27950
#define PORT_MOTD           27950
#define PORT_SERVER         27960
#define NUM_SERVER_PORTS    4	// broadcast scan this many ports after
// PORT_SERVER so a single machine can
// run multiple servers

// the svc_strings[] array in cl_parse.c should mirror this
//
// server to client
//
enum svc_ops_e
{
    svc_bad,
    svc_nop,
    svc_gamestate,
    svc_configstring,			// [short] [string] only in gamestate messages
    svc_baseline,				// only in gamestate messages
    svc_serverCommand,			// [string] to be executed by client game module
    svc_download,				// [short] size [size bytes]
    svc_snapshot,
    svc_EOF,
    
    // svc_extension follows a svc_EOF, followed by another svc_* ...
    //  this keeps legacy clients compatible.
    svc_extension
};


//
// client to server
//
enum clc_ops_e
{
    clc_bad,
    clc_nop,
    clc_move,					// [[usercmd_t]
    clc_moveNoDelta,			// [[usercmd_t]
    clc_clientCommand,			// [string] message
    clc_EOF,
};

/*
==============================================================
DOWNLOAD
==============================================================
*/

#include <API/download_api.h>

/*
==============================================================

Edit fields and command line history/completion

==============================================================
*/

#define MAX_EDIT_LINE   256
typedef struct
{
    S32             cursor;
    S32             scroll;
    S32             widthInChars;
    UTF8            buffer[MAX_EDIT_LINE];
} field_t;

void Field_Clear( field_t* edit );
void Field_Set( field_t* edit, StringEntry text );
void Field_WordDelete( field_t* edit );
void Field_AutoComplete( field_t* edit, StringEntry prompt );
void Field_CompleteKeyname( void );
void Field_CompleteCgame( S32 argNum );
void Field_CompleteFilename( StringEntry dir, StringEntry ext, bool stripExt );
void Field_CompleteAlias( void );
void Field_CompleteDelay( void );
void Field_CompleteCommand( UTF8* cmd, bool doCommands, bool doCvars );

/*
==============================================================

MISC

==============================================================
*/

typedef struct gameInfo_s
{
    bool        spEnabled;
    S32         spGameTypes;
    S32         defaultSPGameType;
    S32         coopGameTypes;
    S32         defaultCoopGameType;
    S32         defaultGameType;
    bool        usesProfiles;
} gameInfo_t;

extern gameInfo_t com_gameInfo;

// TTimo
// centralized and cleaned, that's the max string you can send to a Com_Printf / Com_DPrintf (above gets truncated)
#define MAXPRINTMSG 8192

UTF8*           CopyString( StringEntry in );
void            Info_Print( StringEntry s );

void            Com_BeginRedirect( UTF8* buffer, S32 buffersize, void ( *flush )( UTF8* ) );
void            Com_EndRedirect( void );

void            Com_Quit_f( void );
S32             Com_EventLoop( void );
S32             Com_Milliseconds( void );	// will be journaled properly
UTF8*           Com_MD5File( StringEntry fn, S32 length, StringEntry prefix, S32 prefix_len );
UTF8*           Com_MD5FileOWCompat( StringEntry filename );
S32             Com_Filter( UTF8* filter, UTF8* name, S32 casesensitive );
S32             Com_FilterPath( UTF8* filter, UTF8* name, S32 casesensitive );
S32             Com_RealTime( qtime_t* qtime );
bool            Com_SafeMode( void );

void            Com_StartupVariable( StringEntry match );
void            Com_SetRecommended();

// checks for and removes command line "+set var arg" constructs
// if match is nullptr, all set commands will be executed, otherwise
// only a set with the exact name.  Only used during startup.

//bani - profile functions
void            Com_TrackProfile( UTF8* profile_path );
bool        Com_CheckProfile( UTF8* profile_path );
bool        Com_WriteProfile( UTF8* profile_path );

extern convar_t*  com_crashed;

extern convar_t*  com_ignorecrash;	//bani

extern convar_t*  com_protocol;
extern convar_t*  com_pid;		//bani

extern convar_t*  com_developer;
extern convar_t*  com_dedicated;
extern convar_t*  com_speeds;
extern convar_t*  com_timescale;
extern convar_t*  com_sv_running;
extern convar_t*  com_cl_running;
extern convar_t*  com_viewlog;	// 0 = hidden, 1 = visible, 2 = minimized
extern convar_t*  com_version;

//extern    convar_t  *com_blood;
extern convar_t*  com_buildScript;	// for building release pak files
extern convar_t*  com_journal;
extern convar_t*  com_cameraMode;
extern convar_t*  com_ansiColor;
extern convar_t*  com_logosPlaying;

extern convar_t*  com_unfocused;
extern convar_t*  com_minimized;


// watchdog
extern convar_t*  com_watchdog;
extern convar_t*  com_watchdog_cmd;

extern	convar_t*	com_affinity;

// both client and server must agree to pause
extern convar_t*  cl_paused;
extern convar_t*  sv_paused;

extern convar_t*  cl_packetdelay;
extern convar_t*  sv_packetdelay;

// com_speeds times
extern S32      time_game;
extern S32      time_frontend;
extern S32      time_backend;	// renderer backend time

extern S32      com_frameTime;
extern S32      com_frameMsec;
extern S32      com_expectedhunkusage;
extern S32      com_hunkusedvalue;

extern bool com_errorEntered;

extern fileHandle_t com_journalFile;
extern fileHandle_t com_journalDataFile;

typedef enum
{
    TAG_FREE,
    TAG_GENERAL,
    TAG_BOTLIB,
    TAG_RENDERER,
    TAG_SMALL,
    TAG_CRYPTO,
    TAG_STATIC
} memtag_t;

/*

--- low memory ----
server vm
server clipmap
---mark---
renderer initialization (shaders, etc)
UI vm
cgame vm
renderer map
renderer models

---free---

temp file loading
--- high memory ---

*/

#if defined( _DEBUG ) && !defined( BSPC )
#define ZONE_DEBUG
#endif

#ifdef ZONE_DEBUG
#define Z_TagMalloc( size, tag )          Z_TagMallocDebug( size, tag, # size, __FILE__, __LINE__ )
#define Z_Malloc( size )                  Z_MallocDebug( size, # size, __FILE__, __LINE__ )
#define S_Malloc( size )                  S_MallocDebug( size, # size, __FILE__, __LINE__ )
void*           Z_TagMallocDebug( size_t size, memtag_t tag, UTF8* label, UTF8* file, S32 line );	// NOT 0 filled memory
void*           Z_MallocDebug( size_t size, UTF8* label, UTF8* file, S32 line );	// returns 0 filled memory
void*           S_MallocDebug( size_t size, UTF8* label, UTF8* file, S32 line );	// returns 0 filled memory
#else
void*           Z_TagMalloc( size_t size, memtag_t tag );	// NOT 0 filled memory
void*           Z_Malloc( size_t size );	// returns 0 filled memory
void*           S_Malloc( size_t size );	// NOT 0 filled memory only for small allocations
#endif
void            Z_Free( void* ptr );
void            Z_FreeTags( memtag_t tag );
S32             Z_AvailableMemory( void );
void            Z_LogHeap( void );

void            Hunk_Clear( void );
void            Hunk_ClearToMark( void );
void            Hunk_SetMark( void );
bool        Hunk_CheckMark( void );

//void *Hunk_Alloc( S32 size );
// void *Hunk_Alloc( S32 size, ha_pref preference );
void            Hunk_ClearTempMemory( void );
void*           Hunk_AllocateTempMemory( size_t size );
void            Hunk_FreeTempMemory( void* buf );
S32             Hunk_MemoryRemaining( void );
void            Hunk_SmallLog( void );
void            Hunk_Log( void );

void            Com_TouchMemory( void );
void            Com_ReleaseMemory( void );

// commandLine should not include the executable name (argv[0])
void            Com_Init( UTF8* commandLine );
void            Com_Frame( void );
void            Com_Shutdown( bool badProfile );

/*
==============================================================

CLIENT / SERVER SYSTEMS

==============================================================
*/

//
// client interface
//
void            CL_InitKeyCommands( void );

// the keyboard binding interface must be setup before execing
// config files, but the rest of client startup will happen later

void            CL_Init( void );
void            CL_ClearStaticDownload( void );
void            CL_Disconnect( bool showMainMenu );
void            CL_Shutdown( void );
void            CL_Frame( S32 msec );
void            CL_KeyEvent( S32 key, S32 down, S32 time );
void       CL_RefPrintf( S32 print_level, StringEntry fmt, ... );
void            CL_CharEvent( S32 key );

// UTF8 events are for field typing, not game control

void            CL_MouseEvent( S32 dx, S32 dy, S32 time );

void            CL_JoystickEvent( S32 axis, S32 value, S32 time );

void            CL_PacketEvent( netadr_t from, msg_t* msg );

void            CL_ConsolePrint( UTF8* text );

void            CL_MapLoading( void );

// do a screen update before starting to load a map
// when the server is going to load a new map, the entire hunk
// will be cleared, so the client must shutdown cgame, ui, and
// the renderer

void            CL_ForwardCommandToServer( StringEntry string );

// adds the current command line as a clc_clientCommand to the client message.
// things like godmode, noclip, etc, are commands directed to the server,
// so when they are typed in at the console, they will need to be forwarded.

void            CL_CDDialog( void );

// bring up the "need a cd to play" dialog

void            CL_ShutdownAll( void );

// shutdown all the client stuff

void            CL_FlushMemory( void );

// dump all memory on an error

void            CL_StartHunkUsers( void );

// start all the client stuff using the hunk

#if !defined(UPDATE_SERVER)
void            CL_CheckAutoUpdate( void );
bool            CL_NextUpdateServer( void );
void            CL_GetAutoUpdate( void );
#endif


void			Key_KeynameCompletion( void( *callback )( StringEntry s ) );
// for keyname autocompletion

void            Key_WriteBindings( fileHandle_t f );

void            SCR_DebugGraph( F32 value, S32 color );	// FIXME: move logging to common?


// AVI files have the start of pixel lines 4 U8-aligned
#define AVI_LINE_PADDING 4


/*
==============================================================

NON-PORTABLE SYSTEM SERVICES

==============================================================
*/

#define MAX_JOYSTICK_AXIS 16
#if !defined ( BSPC )
typedef enum
{
    // bk001129 - make sure SE_NONE is zero
    SYSE_NONE = 0,				// evTime is still valid
    SYSE_KEY,						// evValue is a key code, evValue2 is the down flag
    SYSE_CHAR,					// evValue is an ascii UTF8
    SYSE_MOUSE,					// evValue and evValue2 are reletive signed x / y moves
    SYSE_JOYSTICK_AXIS,			// evValue is an axis number and evValue2 is the current state (-127 to 127)
    SYSE_CONSOLE,				// evPtr is a UTF8*
    SYSE_PACKET					// evPtr is a netadr_t followed by data bytes to evPtrLength
} sysEventType_t;

typedef struct sysEvent_s
{
    S32             evTime;
    sysEventType_t  evType;
    S32             evValue, evValue2;
    S32             evPtrLength;	// bytes of data pointed to by evPtr, for journaling
    void*           evPtr;		// this must be manually freed if not nullptr
} sysEvent_t;

void			Com_QueueEvent( S32 time, sysEventType_t type, S32 value, S32 value2, S32 ptrLength, void* ptr );
S32				Com_EventLoop( void );
sysEvent_t		Com_GetSystemEvent( void );
#endif

void Hist_Load( void );
void Hist_Add( StringEntry field );
StringEntry Hist_Next( StringEntry field );
StringEntry Hist_Prev( void );

#define SV_ENCODE_START     4
#define SV_DECODE_START     12
#define CL_ENCODE_START     12
#define CL_DECODE_START     4

S32				Parse_AddGlobalDefine( UTF8* string );
S32				Parse_LoadSourceHandle( StringEntry filename );
S32				Parse_FreeSourceHandle( S32 handle );
S32				Parse_ReadTokenHandle( S32 handle, pc_token_t* pc_token );
S32				Parse_SourceFileAndLine( S32 handle, UTF8* filename, S32* line );

void            Com_GetHunkInfo( S32* hunkused, S32* hunkexpected );
void            Com_RandomBytes( U8* string, S32 len );

#if !defined ( BSPC )
void            Com_QueueEvent( S32 time, sysEventType_t type, S32 value, S32 value2, S32 ptrLength, void* ptr );
#endif

S32 Parse_LoadSourceHandle( StringEntry filename );
S32 Parse_FreeSourceHandle( S32 handle );
S32 Parse_ReadTokenHandle( S32 handle, pc_token_t* pc_token );
S32 Parse_SourceFileAndLine( S32 handle, UTF8* filename, S32* line );

#endif //!__QCOMMON_H__
