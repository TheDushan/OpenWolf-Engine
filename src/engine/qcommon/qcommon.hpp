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
// File name:   qcommon.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
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
    uchar8*           data;
    sint             maxsize;
    sint             cursize;
    sint             uncompsize;	// NERVE - SMF - net debugging
    sint             readcount;
    sint             bit;		// for bitwise reads and writes
} msg_t;

void            MSG_Init( msg_t* buf, uchar8* data, sint length );
void            MSG_InitOOB( msg_t* buf, uchar8* data, sint length );
void            MSG_Clear( msg_t* buf );
void*           MSG_GetSpace( msg_t* buf, sint length );
void            MSG_WriteData( msg_t* buf, const void* data, sint length );
void            MSG_Bitstream( msg_t* buf );
void            MSG_Uncompressed( msg_t* buf );

// TTimo
// copy a msg_t in case we need to store it as is for a bit
// (as I needed this to keep an msg_t from a static var for later use)
// sets data buffer as MSG_Init does prior to do the copy
void            MSG_Copy( msg_t* buf, uchar8* data, sint length, msg_t* src );

struct usercmd_s;
struct entityState_s;
struct playerState_s;

void            MSG_WriteBits( msg_t* msg, sint value, sint bits );

void            MSG_WriteChar( msg_t* sb, sint c );
void            MSG_WriteByte( msg_t* sb, sint c );
void            MSG_WriteShort( msg_t* sb, sint c );
void            MSG_WriteLong( msg_t* sb, sint c );
void            MSG_WriteFloat( msg_t* sb, float32 f );
void            MSG_WriteString( msg_t* sb, pointer s );
void            MSG_WriteBigString( msg_t* sb, pointer s );
void            MSG_WriteAngle16( msg_t* sb, float32 f );

void            MSG_BeginReading( msg_t* sb );
void            MSG_BeginReadingOOB( msg_t* sb );
void            MSG_BeginReadingUncompressed( msg_t* msg );

sint             MSG_ReadBits( msg_t* msg, sint bits );

sint             MSG_ReadChar( msg_t* sb );
sint             MSG_ReadByte( msg_t* sb );
sint             MSG_ReadShort( msg_t* sb );
sint             MSG_ReadLong( msg_t* sb );
float32           MSG_ReadFloat( msg_t* sb );
valueType*           MSG_ReadString( msg_t* sb );
valueType*           MSG_ReadBigString( msg_t* sb );
valueType*           MSG_ReadStringLine( msg_t* sb );
float32           MSG_ReadAngle16( msg_t* sb );
void            MSG_ReadData( msg_t* sb, void* buffer, sint size );
sint				MSG_LookaheadByte( msg_t* msg );

void            MSG_WriteDeltaUsercmd( msg_t* msg, struct usercmd_s* from, struct usercmd_s* to );
void            MSG_ReadDeltaUsercmd( msg_t* msg, struct usercmd_s* from, struct usercmd_s* to );

void            MSG_WriteDeltaUsercmdKey( msg_t* msg, sint key, usercmd_t* from, usercmd_t* to );
void            MSG_ReadDeltaUsercmdKey( msg_t* msg, sint key, usercmd_t* from, usercmd_t* to );

void            MSG_WriteDeltaEntity( msg_t* msg, struct entityState_s* from, struct entityState_s* to, bool force );
void            MSG_ReadDeltaEntity( msg_t* msg, entityState_t* from, entityState_t* to, sint number );

void            MSG_WriteDeltaPlayerstate( msg_t* msg, struct playerState_s* from, struct playerState_s* to );
void            MSG_ReadDeltaPlayerstate( msg_t* msg, struct playerState_s* from, struct playerState_s* to );
void			MSG_WriteDeltaSharedEntity( msg_t* msg, void* from, void* to, bool force, sint number );
void			MSG_ReadDeltaSharedEntity( msg_t* msg, void* from, void* to, sint number );

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

#define MAX_PACKET_USERCMDS     512      // max number of usercmd_t in a packet

#define PORT_ANY            -1

#define MAX_MASTER_SERVERS  5
#define MAX_RCON_LIST		5

// RF, increased this, seems to keep causing problems when set to 64, especially when loading
// a savegame, which is hard to fix on that side, since we can't really spread out a loadgame
// among several frames
//#define   MAX_RELIABLE_COMMANDS   64          // max string commands buffered for restransmit
//#define   MAX_RELIABLE_COMMANDS   128         // max string commands buffered for restransmit
#define MAX_RELIABLE_COMMANDS   256	// bigger!

enum netadrtype_t
{
    NA_BAD = 0,                 // an address lookup failed
    NA_BOT,
    NA_LOOPBACK,
    NA_BROADCAST,
    NA_IP,
    NA_IP6,
    NA_MULTICAST6,
    NA_UNSPEC
};

enum netsrc_t
{
    NS_CLIENT,
    NS_SERVER
};

#define NET_ADDRSTRMAXLEN 48	// maximum length of an IPv6 address string including trailing '\0'
typedef struct
{
    netadrtype_t type;
    
    uchar8 ip[4];
    uchar8 ip6[16];
    
    uchar16 port;
    uint32 scope_id;	// Needed for IPv6 link-local addresses
} netadr_t;

//----(SA)  increased for larger submodel entity counts
#define MAX_MSGLEN					32768		// max length of a message, which may
//#define   MAX_MSGLEN              16384       // max length of a message, which may
// be fragmented into multiple packets
#define MAX_DOWNLOAD_WINDOW         8	// max of eight download frames
#define MAX_DOWNLOAD_BLKSIZE        2048	// 2048 uchar8 block chunks

/*
Netchan handles packet fragmentation and out of order / duplicate suppression
*/

typedef struct
{
    netsrc_t sock;
    
    sint dropped;	// between last packet and previous
    
    netadr_t remoteAddress;
    sint qport;		// qport value to write when transmitting
    
    // sequencing variables
    sint incomingSequence;
    sint outgoingSequence;
    
    // incoming fragment assembly buffer
    sint fragmentSequence;
    sint fragmentLength;
    uchar8 fragmentBuffer[MAX_MSGLEN];
    
    // outgoing fragment buffer
    // we need to space out the sending of large fragmented messages
    bool unsentFragments;
    sint unsentFragmentStart;
    sint unsentLength;
    uchar8 unsentBuffer[MAX_MSGLEN];
    sint lastSentTime;
    sint lastSentSize;
} netchan_t;

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

//Dushan - I will soon move this in the appConfig.hpp
#define GAMENAME_STRING GAMENAME_FOR_MASTER

#ifndef PRE_RELEASE_DEMO
#define PROTOCOL_VERSION    1001
#else
// the demo uses a different protocol version for independant browsing
#define PROTOCOL_VERSION    1000
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

#define PORT_MASTER         12950
#define PORT_MOTD           12950
#define PORT_AUTHORIZE      12952
#define PORT_SERVER         12960
#define NUM_SERVER_PORTS    4	// broadcast scan this many ports after
// PORT_SERVER so a single machine can
// run multiple servers

// maintain a list of compatible protocols for demo playing
// NOTE: that stuff only works with two digits protocols
extern sint demo_protocols[];

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

/*
==============================================================

MISC

==============================================================
*/

typedef struct gameInfo_s
{
    bool        spEnabled;
    sint         spGameTypes;
    sint         defaultSPGameType;
    sint         coopGameTypes;
    sint         defaultCoopGameType;
    sint         defaultGameType;
    bool        usesProfiles;
} gameInfo_t;

extern gameInfo_t com_gameInfo;

// TTimo
// centralized and cleaned, that's the max string you can send to a Com_Printf / Com_DPrintf (above gets truncated)
#define MAXPRINTMSG 8192

valueType*           CopyString( pointer in );
void            Info_Print( pointer s );

void            Com_BeginRedirect( valueType* buffer, uint64 buffersize, void ( *flush )( valueType* ) );
void            Com_EndRedirect( void );

void            Com_Quit_f( void );
sint             Com_EventLoop( void );
sint             Com_Milliseconds( void );	// will be journaled properly
sint             Com_Filter( valueType* filter, valueType* name, sint casesensitive );
sint             Com_FilterPath( valueType* filter, valueType* name, sint casesensitive );
sint             Com_RealTime( qtime_t* qtime );
bool            Com_SafeMode( void );

void            Com_StartupVariable( pointer match );
void            Com_SetRecommended();

// checks for and removes command line "+set var arg" constructs
// if match is nullptr, all set commands will be executed, otherwise
// only a set with the exact name.  Only used during startup.

//bani - profile functions
void            Com_TrackProfile( valueType* profile_path );
bool        Com_CheckProfile( valueType* profile_path );
bool        Com_WriteProfile( valueType* profile_path );

extern convar_t*  com_crashed;

extern convar_t*  com_ignorecrash;	//bani

extern convar_t*  com_protocol;
#ifndef DEDICATED
extern  convar_t* con_autochat;
#endif
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

extern convar_t*  com_unfocused;
extern convar_t*  com_minimized;


// watchdog
extern convar_t*  com_watchdog;
extern convar_t*  com_watchdog_cmd;

// both client and server must agree to pause
extern convar_t*  cl_paused;
extern convar_t*  sv_paused;

extern convar_t*  cl_packetdelay;
extern convar_t*  sv_packetdelay;

// com_speeds times
extern sint      time_game;
extern sint      time_frontend;
extern sint      time_backend;	// renderer backend time

extern sint      com_frameTime;
extern sint      com_frameMsec;
extern sint      com_expectedhunkusage;
extern sint      com_hunkusedvalue;

extern bool com_errorEntered;

extern fileHandle_t com_journalFile;
extern fileHandle_t com_journalDataFile;

enum memtag_t
{
    TAG_FREE,
    TAG_GENERAL,
    TAG_BOTLIB,
    TAG_RENDERER,
    TAG_SMALL,
    TAG_STATIC
};

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
void*           Z_TagMallocDebug( uint64 size, memtag_t tag, valueType* label, valueType* file, sint line );	// NOT 0 filled memory
void*           Z_MallocDebug( uint64 size, valueType* label, valueType* file, sint line );	// returns 0 filled memory
void*           S_MallocDebug( uint64 size, valueType* label, valueType* file, sint line );	// returns 0 filled memory
#else
void*           Z_TagMalloc( uint64 size, memtag_t tag );	// NOT 0 filled memory
void*           Z_Malloc( uint64 size );	// returns 0 filled memory
void*           S_Malloc( uint64 size );	// NOT 0 filled memory only for small allocations
#endif
void            Z_Free( void* ptr );
void            Z_FreeTags( memtag_t tag );
uint64 Z_AvailableMemory( void );
void            Z_LogHeap( void );

void            Hunk_Clear( void );
void            Hunk_ClearToMark( void );
void            Hunk_SetMark( void );
bool        Hunk_CheckMark( void );

//void *Hunk_Alloc( sint size );
// void *Hunk_Alloc( sint size, ha_pref preference );
void            Hunk_ClearTempMemory( void );
void*           Hunk_AllocateTempMemory( uint64 size );
void            Hunk_FreeTempMemory( void* buf );
uint64             Hunk_MemoryRemaining( void );
void            Hunk_SmallLog( void );
void            Hunk_Log( void );

void            Com_TouchMemory( void );
void            Com_ReleaseMemory( void );

// commandLine should not include the executable name (argv[0])
void            Com_Init( valueType* commandLine );
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
void CL_Disconnect( bool showMainMenu, pointer reason );
void            CL_Shutdown( void );
void            CL_Frame( sint msec );
void            CL_KeyEvent( sint key, sint down, sint time );
void       CL_RefPrintf( sint print_level, pointer fmt, ... );
void            CL_CharEvent( sint key );

// valueType events are for field typing, not game control

void            CL_MouseEvent( sint dx, sint dy, sint time );

void            CL_JoystickEvent( sint axis, sint value, sint time );

void            CL_PacketEvent( netadr_t from, msg_t* msg );

void            CL_ConsolePrint( valueType* text );

void            CL_MapLoading( void );

// do a screen update before starting to load a map
// when the server is going to load a new map, the entire hunk
// will be cleared, so the client must shutdown cgame, ui, and
// the renderer

void            CL_ForwardCommandToServer( pointer string );

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


void			Key_KeynameCompletion( void( *callback )( pointer s ) );
// for keyname autocompletion

void            Key_WriteBindings( fileHandle_t f );

void            SCR_DebugGraph( float32 value, sint color );	// FIXME: move logging to common?


// AVI files have the start of pixel lines 4 uchar8-aligned
#define AVI_LINE_PADDING 4


/*
==============================================================

NON-PORTABLE SYSTEM SERVICES

==============================================================
*/

#define MAX_JOYSTICK_AXIS 16
#if !defined ( BSPC )
enum sysEventType_t
{
    // bk001129 - make sure SE_NONE is zero
    SYSE_NONE = 0,				// evTime is still valid
    SYSE_KEY,						// evValue is a key code, evValue2 is the down flag
    SYSE_CHAR,					// evValue is an ascii valueType
    SYSE_MOUSE,					// evValue and evValue2 are reletive signed x / y moves
    SYSE_JOYSTICK_AXIS,			// evValue is an axis number and evValue2 is the current state (-127 to 127)
    SYSE_CONSOLE,				// evPtr is a valueType*
    SYSE_PACKET					// evPtr is a netadr_t followed by data bytes to evPtrLength
};

typedef struct sysEvent_s
{
    sint             evTime;
    sysEventType_t  evType;
    sint             evValue, evValue2;
    sint             evPtrLength;	// bytes of data pointed to by evPtr, for journaling
    void*           evPtr;		// this must be manually freed if not nullptr
} sysEvent_t;

void			Com_QueueEvent( sint time, sysEventType_t type, sint value, sint value2, sint ptrLength, void* ptr );
sint				Com_EventLoop( void );
sysEvent_t		Com_GetSystemEvent( void );
#endif

#define SV_ENCODE_START     4
#define SV_DECODE_START     12
#define CL_ENCODE_START     12
#define CL_DECODE_START     4

void            Com_GetHunkInfo( sint* hunkused, sint* hunkexpected );
void            Com_RandomBytes( uchar8* string, sint len );

#if !defined ( BSPC )
void            Com_QueueEvent( sint time, sysEventType_t type, sint value, sint value2, sint ptrLength, void* ptr );
#endif

// Dushan - create CL_GUID
// we cannot call it "qkey", "etkey" is already taken,
// so we will change it to etxreal
#define GUIDKEY_FILE "guidopenwolf"
#define GUIDKEY_SIZE 28

valueType* Com_GetTigerHash( valueType* str );

// HASH TABLE
typedef struct hash_node_s
{
    void* key;
    void* data;
    struct hash_node_s* next;
} hash_node_t;

typedef struct
{
    hash_node_t* list;
    schar16 count;
} hash_data_t;

typedef struct
{
    hash_data_t* table;
    sint max_colitions;
    sint size;
    sint used;
    uint( *hash_f )( void* k );
    sint( *compare )( const void* a, const void* b );
    void ( *destroy_key )( void* a );
    void ( *destroy_data )( void* a );
} hash_table_t;

typedef struct
{
    hash_table_t* table;
    hash_node_t* node;
    sint index;
} hash_table_iterator_t;

// HASH FUNCTIONS
hash_table_t* Com_CreateHashTable( uint( *hash_f )( void* k ), sint( *compare )( const void* a, const void* b ), void ( *destroy_key )( void* a ), void ( *destroy_data )( void* a ), sint initial_size );
hash_node_t* Com_InsertIntoHash( hash_table_t* table, void* key, void* data );
void Com_DeleteFromHash( hash_table_t* table, void* key );
hash_node_t* Com_FindHashNode( hash_table_t* table, void* key );
void* Com_FindHashData( hash_table_t* table, void* key );
void Com_DestroyHash( hash_table_t* table );
void Com_RebuildHash( hash_table_t* table, sint new_size );
uint Com_JenkinsHashKey( void* vkey );
sint Com_StrCmp( const void* a1, const void* a2 );
void Com_DestroyStringKey( void* s );
hash_table_iterator_t* Com_CreateHashIterator( hash_table_t* table );
void* Com_HashIterationData( hash_table_iterator_t* iter );

#endif //!__QCOMMON_H__
