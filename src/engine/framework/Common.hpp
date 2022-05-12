////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2022 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   common.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description: misc functions used in client and server
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __COMMON_HPP__
#define __COMMON_HPP__

typedef struct gameInfo_s {
    bool        spEnabled;
    sint         spGameTypes;
    sint         defaultSPGameType;
    sint         coopGameTypes;
    sint         defaultCoopGameType;
    sint         defaultGameType;
    bool        usesProfiles;
} gameInfo_t;

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
#define AUTOUPDATE_SERVER1_NAME   "au2rtcw1.activision.com" // DHM - Nerve
#define AUTOUPDATE_SERVER2_NAME   "au2rtcw2.activision.com" // DHM - Nerve
#define AUTOUPDATE_SERVER3_NAME   "au2rtcw3.activision.com" // DHM - Nerve
#define AUTOUPDATE_SERVER4_NAME   "au2rtcw4.activision.com" // DHM - Nerve
#define AUTOUPDATE_SERVER5_NAME   "au2rtcw5.activision.com" // DHM - Nerve
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
#define PORT_COMMUNITY      12961
#define NUM_SERVER_PORTS    5   // broadcast scan this many ports after
// PORT_SERVER so a single machine can
// run multiple servers

// the svc_strings[] array in cl_parse.c should mirror this
//
// server to client
//
enum svc_ops_e {
    svc_bad,
    svc_nop,
    svc_gamestate,
    svc_configstring,           // [short] [string] only in gamestate messages
    svc_baseline,               // only in gamestate messages
    svc_serverCommand,          // [string] to be executed by client game module
    svc_download,               // [short] size [size bytes]
    svc_snapshot,
    svc_EOF,

    // svc_extension follows a svc_EOF, followed by another svc_* ...
    //  this keeps legacy clients compatible.
    svc_extension
};


//
// client to server
//
enum clc_ops_e {
    clc_bad,
    clc_nop,
    clc_move,                   // [[usercmd_t]
    clc_moveNoDelta,            // [[usercmd_t]
    clc_clientCommand,          // [string] message
    clc_EOF,
};

// com_speeds times
extern sint      time_game;
extern sint      time_frontend;
extern sint      time_backend;  // renderer backend time

extern sint      com_frameTime;
extern sint      com_frameMsec;
extern sint      com_expectedhunkusage;
extern sint      com_hunkusedvalue;

extern bool com_errorEntered;

extern fileHandle_t com_journalFile;
extern fileHandle_t com_journalDataFile;


extern gameInfo_t com_gameInfo;


#define NET_ENABLEV4            0x01
#define NET_ENABLEV6            0x02
// if this flag is set, always attempt ipv6 connections instead of ipv4 if a v6 address is found.
#define NET_PRIOV6              0x04
// disables ipv6 multicast support if set.
#define NET_DISABLEMCAST        0x08

#ifndef DEDICATED
#define PACKET_BACKUP   256
#else
#define PACKET_BACKUP   32  // number of old messages that must be kept on client and server for delta comrpession and ping estimation
#endif
#define PACKET_MASK     (PACKET_BACKUP-1)

#define MAX_PACKET_USERCMDS     2048   // max number of usercmd_t in a packet

#define PORT_ANY            -1

#define MAX_MASTER_SERVERS  5
#define MAX_RCON_LIST       5

// RF, increased this, seems to keep causing problems when set to 64, especially when loading
// a savegame, which is hard to fix on that side, since we can't really spread out a loadgame
// among several frames
//#define   MAX_RELIABLE_COMMANDS   64          // max string commands buffered for restransmit
//#define   MAX_RELIABLE_COMMANDS   128         // max string commands buffered for restransmit
#define MAX_RELIABLE_COMMANDS   256 // bigger!

enum netsrc_t {
    NS_CLIENT,
    NS_SERVER
};

#define NET_ADDRSTRMAXLEN 48    // maximum length of an IPv6 address string including trailing '\0'

//----(SA)  increased for larger submodel entity counts
#define MAX_MSGLEN                  32768       // max length of a message, which may
//#define   MAX_MSGLEN              16384       // max length of a message, which may
// be fragmented into multiple packets
#define MAX_DOWNLOAD_WINDOW         8   // max of eight download frames
#define MAX_DOWNLOAD_BLKSIZE        2048    // 2048 uchar8 block chunks

/*
Netchan handles packet fragmentation and out of order / duplicate suppression
*/

typedef struct {
    netsrc_t sock;

    sint dropped;   // between last packet and previous

    netadr_t remoteAddress;
    sint qport;     // qport value to write when transmitting

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

// AVI files have the start of pixel lines 4 uchar8-aligned
#define AVI_LINE_PADDING 4

#define SV_ENCODE_START     4
#define SV_DECODE_START     12
#define CL_ENCODE_START     12
#define CL_DECODE_START     4

// Dushan - create CL_GUID
// we cannot call it "qkey", "etkey" is already taken,
// so we will change it to etxreal
#define GUIDKEY_FILE "guidopenwolf"
#define GUIDKEY_SIZE 28

#define MAX_NUM_ARGVS   50

#define MIN_DEDICATED_COMHUNKMEGS 64
#define MIN_COMHUNKMEGS 64              // JPW NERVE changed this to 42 for MP, was 56 for team arena and 75 for wolfSP
#define DEF_COMHUNKMEGS 512         // RF, increased this, some maps are exceeding 56mb 
// JPW NERVE changed this for multiplayer back to 42, 56 for depot/mp_cpdepot, 42 for everything else
#define DEF_COMZONEMEGS 64              // RF, increased this from 16, to account for botlib/AAS
#define DEF_COMHUNKMEGS_S   XSTRING(DEF_COMHUNKMEGS)
#define DEF_COMZONEMEGS_S   XSTRING(DEF_COMZONEMEGS)

#define MAX_CONSOLE_LINES   32

// bk001129 - here we go again: upped from 64
// Dushan, 512
#define MAX_PUSHED_EVENTS               512
// bk001129 - init, also static
static sint com_pushedEventsHead = 0;
static sint com_pushedEventsTail = 0;

#define MAX_QUEUED_EVENTS  256
#define MASK_QUEUED_EVENTS ( MAX_QUEUED_EVENTS - 1 )

#define MAX_JOYSTICK_AXIS 16


typedef struct sysEvent_s {
    sint             evTime;
    sysEventType_t  evType;
    sint             evValue, evValue2;
    sint
    evPtrLength;   // bytes of data pointed to by evPtr, for journaling
    void *evPtr;      // this must be manually freed if not nullptr
} sysEvent_t;

// centralized and cleaned, that's the max string you can send to a Com_Printf (above gets truncated)
#define MAXPRINTMSG 8192

static sysEvent_t  eventQueue[MAX_QUEUED_EVENTS];
static sint         eventHead = 0;
static sint         eventTail = 0;
static uchar8        sys_packetReceived[MAX_MSGLEN];

// bk001129 - static
static sysEvent_t com_pushedEvents[MAX_PUSHED_EVENTS];

typedef struct com_color_defs_s {
    valueType const *name;
    valueType const *code;
} com_color_defs_t;

static const com_color_defs_t color_definitions[] = {
    {"Black", "0"},
    {"Red", "1"},
    {"Green", "2"},
    {"Yellow", "3"},
    {"Blue", "4"},
    {"Cyan", "5"},
    {"Magenta", "6"},
    {"White", "7"},
    {"Gray", "8"},
    {"Orange", "9"},
    {"Rose Bud", "a"},
    {"Pale Green", "b"},
    {"Pale Golden", "c"},
    {"Columbia Blue", "d"},
    {"Pale Turquoise", "e"},
    {"Pale Violet Red", "f"},
    {"Palace Pale White", "g"},
    {"Olive", "h"},
    {"Tomato", "i"},
    {"Lime", "j"},
    {"Lemon", "k"},
    {"Blue Berry", "l"},
    {"Turquoise", "m"},
    {"Wild Watermelon", "n"},
    {"Saltpan", "o"},
    {"Gray Chateau", "p"},
    {"Rust", "q"},
    {"Copper Green", "r"},
    {"Gold", "s"},
    {"Steel Blue", "t"},
    {"Steel Gray", "u"},
    {"Bronze", "v"},
    {"Silver", "w"},
    {"Dark Gray", "x"},
    {"Dark Orange", "y"},
    {"Dark Green", "z"},
    {"Red Orange", "A"},
    {"Forest Green", "B"},
    {"Bright Sun", "C"},
    {"Medium Slate Blue", "D"},
    {"Celeste", "E"},
    {"Ironstone", "F"},
    {"Timberwolf", "G"},
    {"Onyx", "H"},
    {"Rosewood", "I"},
    {"Kokoda", "J"},
    {"Porsche", "K"},
    {"Cloud Burst", "L"},
    {"Blue Diane", "M"},
    {"Rope", "N"},
    {"Blonde", "O"},
    {"Smokey Black", "P"},
    {"American Rose", "Q"},
    {"Neon Green", "R"},
    {"Neon Yellow", "S"},
    {"Ultramarine", "T"},
    {"Turquoise Blue", "U"},
    {"Dark Magenta", "V"},
    {"Magic Mint", "W"},
    {"Light Gray", "X"},
    {"Light Salmon", "Y"},
    {"Light Green", "Z"},
};

//bani - from files.c
extern valueType     fs_gamedir[MAX_OSPATH];

static sint color_definitions_length = ARRAY_LEN(color_definitions);

#ifndef DEDICATED
extern bool consoleButtonWasPressed;
#endif

class idCommonLocal : public idCommon {
public:
    idCommonLocal(void);
    ~idCommonLocal(void);

    virtual void BeginRedirect(valueType *buffer, uint64 buffersize,
                               void (*flush)(valueType *));
    virtual void EndRedirect(void);
    virtual void Printf(pointer fmt, ...);
    virtual void Error(errorParm_t code, pointer fmt, ...);
    virtual bool SafeMode(void);
    virtual void StartupVariable(pointer match);
    virtual void InfoPrint(pointer s);
    virtual sint Filter(valueType *filter, valueType *name,
                        sint casesensitive);
    virtual sint FilterPath(pointer filter, pointer name, sint casesensitive);
    virtual sint RealTime(qtime_t *qtime);
    virtual void QueueEvent(sint evTime, sysEventType_t evType, sint value,
                            sint value2, sint ptrLength, void *ptr);
    virtual sint EventLoop(void);
    virtual sint Milliseconds(void);
    virtual void SetRecommended(void);
    virtual bool CheckProfile(valueType *profile_path);
    virtual bool WriteProfile(valueType *profile_path);
    virtual void Init(valueType *commandLine);
    virtual void Frame(void);
    virtual void RandomBytes(uchar8 *string, sint len);
    virtual void RgbToHsl(vec4_t rgb, vec4_t hsl);
    virtual void HlsToRgb(vec4_t hsl, vec4_t rgb);

    static void Colors_f(void);
    static void Quit_f(void);
    static void ParseCommandLine(valueType *commandLine);
    static bool AddStartupCommands(void);
    static void InitJournaling(void);
    static pointer ShowEventName(sysEventType_t eventType);
    static sysEvent_t GetSystemEvent(void);
    static sysEvent_t GetRealEvent(void);
    static void InitPushEvent(void);
    static void PushEvent(sysEvent_t *_event);
    static sysEvent_t GetEvent(void);
    static void RunAndTimeServerPacket(netadr_t *evFrom, msg_t *buf);
    static void Freeze_f(void);
    static void Crash_f(void);
    static void GetGameInfo(void);
    static void TrackProfile(valueType *profile_path);
    static void WriteConfigToFile(pointer filename);
    static void WriteConfiguration(void);
    static void WriteConfig_f(void);
    static sint ModifyMsec(sint msec);
    static void Shutdown(bool badProfile);
    static void EndRedirect_f(void);
    static void Error_f(void);
    static void InitCommonConsoleVars(void);
};

extern idCommonLocal commonLocal;

#endif // !__COMMON_HPP__
