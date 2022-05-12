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
// File name:   server.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SERVER_HPP__
#define __SERVER_HPP__

//#define PERS_SCORE              0 // !!! MUST NOT CHANGE, SERVER AND
// GAME BOTH REFERENCE !!!

#define MAX_BPS_WINDOW      20  // NERVE - SMF - net debugging

#define CLAN_NAME_SIZE 33
#define USER_NAME_SIZE 33

typedef struct {
    valueType name[CLAN_NAME_SIZE + 1];
} clan_t;

typedef struct user_clan_s {
    clan_t *clan;
    struct user_clan_s *next;
} user_clan_t;

typedef struct {
    valueType name[USER_NAME_SIZE + 1];
    valueType password[49]; // Tiger Hash password
    valueType uguid[33];
    valueType type;
    user_clan_t *clans;
} user_t;

#define WEAPONS 12
enum weapon_name_e {
    W_BLASTER,
    W_MACHINEGUN,
    W_PAIN_SAW,
    W_SHOTGUN,
    W_LAS_GUN,
    W_MASS_DRIVER,
    W_CHAINGUN,
    W_PULSE_RIFLE,
    W_FLAMER,
    W_LUCIFER_CANNON,
    W_GRENADE,
    W_LOCKBLOB_LAUNCHER
};

#define WEAPONS_MNEMONICS "$BL", "$MC", "$PS", "$SH", "$LG", "$MD", "$CG", "$PR", "$FL", "$LC", "$GR", "$LL", nullptr

// Must be the same long as weapon_stats_e
#define WEAPON_STATS 18

enum weapon_stats_e {
    WS_FIRED,
    WS_FIRED_HIT,
    WS_FIRED_HIT_TEAM,
    WS_FIRED_MISSED,
    WS_KILLS,
    WS_DEADS,
    WS_DAMAGE_DONE,
    WS_DAMEGE_TO_TEAM,
    WS_HIT_HEAD,
    WS_HIT_TORSO,
    WS_HIT_LEGS,
    WS_HIT_ARMS,
    WS_DAMAGE_TAKEN,
    WS_DAMAGE_TAKEN_FROM_TEAM,
    WS_DAMAGE_TAKEN_HEAD,
    WS_DAMAGE_TAKEN_TORSO,
    WS_DAMAGE_TAKEN_LEGS,
    WS_DAMAGE_TAKEN_ARMS
};

#define WEAPONS_STATS_MNEMONICS "WFD", "WFH", "WFT", "WFM", "WK", "WD", "WDD", "WTT", "WHH", "WHT", "WHL", "WHA", "WDN", "WTN", "WDH", "WDT", "WDL", "WDA", nullptr

#define MELEE_STATS 8

enum melee_stats_e {
    alevel0_kills,
    alevel1_kills,
    alevel1_upg_kills,
    alevel2_kills,
    alevel2_upg_kills,
    alevel3_kills,
    alevel3_upg_kills,
    alevel4_kills
};

#define MELEE_STATS_MNEMONICS "MKK", nullptr

#define EXPLOSIONS_STATS 10
enum explosions_stats_e {
    ES_GRANADE_KILLS
};

#define EXPLOSIONS_STATS_MNEMONICS "EGK", nullptr

#define MISC_STATS 5
enum misc_stats_e {
    MIS_ENVIRONMENTAL_DEATHS,
    MIS_SUICIDES
};

#define MISC_STATS_MNEMONICS "IED", "ISD", nullptr

enum user_type_e {
    CS_USER,
    CS_GUID
};

typedef struct user_stats_s {
    sint type;
    valueType name[USER_NAME_SIZE + 1];
    valueType game_name[USER_NAME_SIZE + 1];
    sint weapons[WEAPONS][WEAPON_STATS];
    sint melee[MELEE_STATS];
    sint explosions[EXPLOSIONS_STATS];
    sint misc[MISC_STATS];
    sint avg_ping;
    sint n_ping;
    sint time;
    sint game_time;
    sint start_time;
    sint last_time;
    sint login;
    sint score;
    sint team;
    struct user_stats_s *next;
} user_stats_t;

typedef struct {
    sint players;
    sint cpu;
    sint mem;
} server_stats_t;

typedef struct {
    user_stats_t *user_stats;
    server_stats_t server_stats;
    valueType game_id[49];
    sint game_date;
    sint mapstatus;
    valueType mapname[MAX_QPATH];
    sint start_time;
} community_stats_t;

typedef struct statistics_tmp {
    sint32 filepos;
    valueType game_id[49];
    struct statistics_tmp *next;
} statistics_t;

enum serverState_t {
    SS_DEAD,    // no map loaded
    SS_LOADING, // spawning level entities
    SS_GAME     // actively running
};

typedef struct configString_s {
    valueType *s;

    bool restricted; // if true, don't send to clientList
    clientList_t clientList;
} configString_t;

typedef struct server_s {
    serverState_t   state;
    bool
    restarting; // if true, send configstring changes during SS_LOADING
    sint             serverId;  // changes each server start
    sint             restartedServerId; // serverId before a map_restart
    sint
    checksumFeed;  // the feed key that we use to compute the pure checksum strings
    // show_bug.cgi?id=475
    // the serverId associated with the current checksumFeed (always <= serverId)
    sint             checksumFeedServerId;
    sint             snapshotCounter;   // incremented for each snapshot built
    sint             timeResidual;  // <= 1000 / sv_frame->value
    sint             nextFrameTime; // when time > nextFrameTime, process world
    struct cmodel_s *models[MAX_MODELS];
    configString_t  configstrings[MAX_CONFIGSTRINGS];
    bool            configstringsmodified[MAX_CONFIGSTRINGS];
    svEntity_t      svEntities[MAX_GENTITIES];

    valueType           *entityParsePoint;  // used during game VM init

    // the game virtual machine will update these on init and changes

    sharedEntity_t *gentities;
    uint64             gentitySize;
    sint             num_entities;  // current number, <= MAX_GENTITIES

    // previous frame for delta compression
    sharedEntity_t  demoEntities[MAX_GENTITIES];
    playerState_t   demoPlayerStates[MAX_CLIENTS];

    playerState_t  *gameClients;
    uint64
    gameClientSize;  // will be > sizeof(playerState_t) due to game private data

    sint             restartTime;
    sint             time;

    // NERVE - SMF - net debugging
    sint             bpsWindow[MAX_BPS_WINDOW];
    sint             bpsWindowSteps;
    sint             bpsTotalBytes;
    sint             bpsMaxBytes;

    sint             ubpsWindow[MAX_BPS_WINDOW];
    sint             ubpsTotalBytes;
    sint             ubpsMaxBytes;

    float32             ucompAve;
    sint             ucompNum;
    // -NERVE - SMF

    md3Tag_t        tags[MAX_SERVER_TAGS];
    tagHeaderExt_t  tagHeadersExt[MAX_TAG_FILES];

    sint             num_tagheaders;
    sint             num_tags;
    time_t          realMapTimeStarted;
} server_t;

typedef struct clientSnapshot_s {
    sint             areabytes;
    uchar8
    areabits[MAX_MAP_AREA_BYTES];   // portalarea visibility bits
    playerState_t   ps;
    sint             num_entities;
    sint             first_entity;  // into the circular sv_packet_entities[]
    // the entities MUST be in increasing state number
    // order, otherwise the delta compression will fail
    sint             messageSent;   // time the message was transmitted
    sint             messageAcked;  // time the message was acked
    sint             messageSize;   // used to rate drop packets
} clientSnapshot_t;

enum clientState_t {
    CS_FREE,                    // can be reused for a new connection
    CS_ZOMBIE,                  // client has been disconnected, but don't reuse connection for a couple seconds
    CS_CONNECTED,               // has been assigned to a client_t, but no gamestate yet
    CS_PRIMED,                  // gamestate has been sent, but client hasn't sent a usercmd
    CS_ACTIVE                   // client is fully in game
};

typedef struct {
    valueType demoName[MAX_OSPATH];
    bool demorecording;
    bool demowaiting;
    bool isBot;
    sint minDeltaFrame;
    fileHandle_t demofile;
    sint botReliableAcknowledge;
} demoInfo_t;

typedef struct netchan_buffer_s {
    msg_t           msg;
    uchar8              msgBuffer[MAX_MSGLEN];
    valueType            lastClientCommandString[MAX_STRING_CHARS];
    struct netchan_buffer_s *next;
} netchan_buffer_t;

typedef struct client_s {
    clientState_t   state;
    valueType            userinfo[MAX_INFO_STRING]; // name, etc
    valueType
    userinfobuffer[MAX_INFO_STRING]; //used for buffering of user info
    valueType           userinfoPostponed[MAX_INFO_STRING];

    valueType
    reliableCommands[MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS];
    sint
    reliableSequence;  // last added reliable message, not necesarily sent or acknowledged yet
    sint
    reliableAcknowledge;   // last acknowledged reliable message
    sint
    reliableSent;  // last sent reliable message, not necesarily acknowledged yet
    sint             messageAcknowledge;

    sint
    gamestateMessageNum;   // netchan->outgoingSequence of gamestate
    sint             challenge;

    usercmd_t       lastUsercmd;
    sint             lastMessageNum;    // for delta compression
    sint             lastClientCommand; // reliable client message sequence
    valueType            lastClientCommandString[MAX_STRING_CHARS];
    sharedEntity_t *gentity;    // SV_GentityNum(clientnum)
    valueType
    name[MAX_COLORFUL_NAME_LENGTH]; // extracted from userinfo, high bits masked

    // downloading
    valueType
    downloadName[MAX_QPATH];   // if not empty string, we are downloading
    fileHandle_t    download;   // file being downloaded
    sint
    downloadSize;  // total bytes (can't use EOF because of paks)
    sint             downloadCount; // bytes sent
    sint
    downloadClientBlock;   // last block we sent to the client, awaiting ack
    sint             downloadCurrentBlock;  // current block number
    sint             downloadXmitBlock; // last block we xmited
    uchar8
    *downloadBlocks[MAX_DOWNLOAD_WINDOW];    // the buffers for the download blocks
    sint             downloadBlockSize[MAX_DOWNLOAD_WINDOW];
    bool            downloadEOF;    // We have sent the EOF block
    sint
    downloadSendTime;  // time we last got an ack from the client

    // www downloading
    bool
    bDlOK;      // passed from cl_wwwDownload CVAR_USERINFO, wether this client supports www dl
    valueType
    downloadURL[MAX_OSPATH];   // the URL we redirected the client to
    bool            bWWWDl;     // we have a www download going
    bool            bWWWing;    // the client is doing an ftp/http download
    bool
    bFallback;  // last www download attempt failed, fallback to regular download
    // note: this is one-shot, multiple downloads would cause a www download to be attempted again

    sint             deltaMessage;  // frame last client usercmd message
    sint
    nextReliableTime;  // svs.time when another reliable command will be allowed
    sint
    nextReliableUserTime; // svs.time when another userinfo change will be allowed
    sint
    lastPacketTime;    // svs.time when packet was last received
    sint             lastConnectTime;   // svs.time when connection started
    sint
    nextSnapshotTime;  // send another snapshot when svs.time >= nextSnapshotTime
    bool
    rateDelayed;    // true if nextSnapshotTime was set based on rate instead of snapshotMsec
    sint
    timeoutCount;  // must timeout a few frames in a row so debugging doesn't break
    clientSnapshot_t frames[PACKET_BACKUP]; // updates can be delta'd from here
    sint             ping;
    sint             rate;      // bytes / second
    sint
    snapshotMsec;  // requests a snapshot every snapshotMsec unless rate choked
    bool             pureAuthentic;
    bool
    pureReceived;       // TTimo - additional flag to distinguish between a bad pure checksum, and no cp command at all
    netchan_t       netchan;
    // TTimo
    // queuing outgoing fragmented messages to send them properly, without udp packet bursts
    // in case large fragmented messages are stacking up
    // buffer them into this queue, and hand them out to netchan as needed
    netchan_buffer_t *netchan_start_queue;
    //% netchan_buffer_t **netchan_end_queue;
    netchan_buffer_t *netchan_end_queue;

    valueType guid[GUIDKEY_SIZE];
    bool authenticated;

    //bani
    sint             downloadnotify;
    bool        csUpdated[MAX_CONFIGSTRINGS + 1];
    sint lastUserInfoChange;
    sint lastUserInfoCount;
    sint oldServerTime;
    user_t *cs_user;
    demoInfo_t demo;
} client_t;

//=============================================================================

// Dushan
#define STATFRAMES  100
typedef struct svstats_s {
    float64 active;
    float64 idle;
    sint    count;
    sint    packets;

    float64 latched_active;
    float64 latched_idle;
    sint    latched_packets;
} svstats_t;

// MAX_CHALLENGES is made large to prevent a denial
// of service attack that could cycle all of them
// out before legitimate users connected
#define MAX_CHALLENGES  2048

// Allow a certain amount of challenges to have the same IP address
// to make it a bit harder to DOS one single IP address from connecting
// while not allowing a single ip to grab all challenge resources
#define MAX_CHALLENGES_MULTI (MAX_CHALLENGES / 2)

#define AUTHORIZE_TIMEOUT 8000

typedef struct challenge_s {
    netadr_t    adr;
    sint challenge;
    sint clientChallenge;       // challenge number coming from the client
    sint time;      // time the last packet was sent to the autherize server
    sint pingTime;  // time the challenge response was sent to client
    sint firstTime; // time the adr was first used, for authorize timeout checks
    sint firstPing; // Used for min and max ping checks
    bool connected;
    bool wasrefused;
    valueType guid[GUIDKEY_SIZE];
    sint getChallengeCookie;
    bool authenticated;
    bool authServerStrict;
} challenge_t;

typedef struct {
    netadr_t  adr;
    sint       time;
} receipt_t;

typedef struct {
    netadr_t    adr;
    sint            time;
    sint            count;
    bool        flood;
} floodBan_t;

// MAX_INFO_RECEIPTS is the maximum number of getstatus+getinfo responses that we send
// in a two second time period.
#define MAX_INFO_RECEIPTS  48

typedef struct tempBan_s {
    netadr_t        adr;
    sint             endtime;
} tempBan_t;

#define MAX_INFO_FLOOD_BANS 36

#define MAX_MASTERS                         8   // max recipients for heartbeat packets
#define MAX_TEMPBAN_ADDRESSES               MAX_CLIENTS

#define SERVER_PERFORMANCECOUNTER_FRAMES    600
#define SERVER_PERFORMANCECOUNTER_SAMPLES   6

// this structure will be cleared only when the game dll changes
typedef struct serverStatic_s {
    bool        initialized;    // sv_init has completed

    sint
    time;      // will be strictly increasing across level changes

    sint
    snapFlagServerBit; // ^= SNAPFLAG_SERVERCOUNT every idServerInitSystemLocal::SpawnServer()

    client_t       *clients;    // [sv_maxclients->integer];
    sint
    numSnapshotEntities;   // sv_maxclients->integer*PACKET_BACKUP*MAX_PACKET_ENTITIES
    sint             nextSnapshotEntities;  // next snapshotEntities to use
    entityState_t  *snapshotEntities;   // [numSnapshotEntities]
    sint             nextHeartbeatTime;
    challenge_t
    challenges[MAX_CHALLENGES]; // to prevent invalid IPs from connecting
    receipt_t       infoReceipts[MAX_INFO_RECEIPTS];
    floodBan_t      infoFloodBans[MAX_INFO_FLOOD_BANS];
    netadr_t        redirectAddress;    // for rcon return messages
    tempBan_t       tempBanAddresses[MAX_TEMPBAN_ADDRESSES];
    sint             sampleTimes[SERVER_PERFORMANCECOUNTER_SAMPLES];
    sint             currentSampleIndex;
    sint             totalFrameTime;
    sint             currentFrameIndex;
    sint             serverLoad;
    // Dushan
    svstats_t       stats;
    sint                queryDone;

    netadr_t authorizeAddress;

    struct {
        bool enabled;
        sint lastTimeDisconnected;
        float32 sv_fps;
    } hibernation;

    bool            gameStarted;
} serverStatic_t;

#if defined (UPDATE_SERVER)

typedef struct {
    valueType version[MAX_QPATH];
    valueType platform[MAX_QPATH];
    valueType installer[MAX_QPATH];
} versionMapping_t;

#define MAX_UPDATE_VERSIONS 128
extern versionMapping_t versionMap[MAX_UPDATE_VERSIONS];
extern sint numVersions;
// Maps client version to appropriate installer

#endif

//=============================================================================

extern serverStatic_t svs;      // persistant server info across maps
extern server_t sv;             // cleared each map
extern void    *gvm;

#define CS_OK 1
#define CS_ERROR 0
#define CS_CHECK 2
#define CS_ADMIN 'A'
#define CS_MEMBER 'M'
#define CS_BANNED 'B'
#define CS_OWNER  'O'
#define CS_UNKNOW 'U'

//bani - cl->downloadnotify
#define DLNOTIFY_REDIRECT   0x00000001  // "Redirecting client ..."
#define DLNOTIFY_BEGIN      0x00000002  // "clientDownload: 4 : beginning ..."
#define DLNOTIFY_ALL        ( DLNOTIFY_REDIRECT | DLNOTIFY_BEGIN )

#endif //!__SERVER_HPP__
