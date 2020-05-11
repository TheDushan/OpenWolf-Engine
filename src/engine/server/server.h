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
// File name:   server.h
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SERVER_H__
#define __SERVER_H__

#ifndef __Q_SHARED_H__
#include <qcommon/q_shared.h>
#endif
#ifndef __QCOMMON_H__
#include <qcommon/qcommon.h>
#endif
#ifndef __SG_API_H__
#include <API/sgame_api.h>
#endif

//#define PERS_SCORE              0	// !!! MUST NOT CHANGE, SERVER AND
// GAME BOTH REFERENCE !!!

#define MAX_BPS_WINDOW      20	// NERVE - SMF - net debugging

typedef enum serverState_s
{
    SS_DEAD,					// no map loaded
    SS_LOADING,					// spawning level entities
    SS_GAME						// actively running
} serverState_t;

typedef struct configString_s
{
    UTF8*					s;
    
    bool			restricted; // if true, don't send to clientList
    clientList_t	clientList;
} configString_t;

typedef struct server_s
{
    serverState_t   state;
    bool            restarting;	// if true, send configstring changes during SS_LOADING
    S32             serverId;	// changes each server start
    S32             restartedServerId;	// serverId before a map_restart
    S32             checksumFeed;	// the feed key that we use to compute the pure checksum strings
    // show_bug.cgi?id=475
    // the serverId associated with the current checksumFeed (always <= serverId)
    S32             checksumFeedServerId;
    S32             snapshotCounter;	// incremented for each snapshot built
    S32             timeResidual;	// <= 1000 / sv_frame->value
    S32             nextFrameTime;	// when time > nextFrameTime, process world
    struct cmodel_s* models[MAX_MODELS];
    configString_t	configstrings[MAX_CONFIGSTRINGS];
    bool            configstringsmodified[MAX_CONFIGSTRINGS];
    svEntity_t      svEntities[MAX_GENTITIES];
    
    UTF8*           entityParsePoint;	// used during game VM init
    
    // the game virtual machine will update these on init and changes
    
    sharedEntity_t* gentities;
    S32             gentitySize;
    S32             num_entities;	// current number, <= MAX_GENTITIES
    
    // demo recording
    fileHandle_t	demoFile;
    demoState_t		demoState;
    UTF8			demoName[MAX_QPATH];
    
    // previous frame for delta compression
    sharedEntity_t	demoEntities[MAX_GENTITIES];
    playerState_t	demoPlayerStates[MAX_CLIENTS];
    
    playerState_t*  gameClients;
    S32             gameClientSize;	// will be > sizeof(playerState_t) due to game private data
    
    S32             restartTime;
    S32             time;
    
    // NERVE - SMF - net debugging
    S32             bpsWindow[MAX_BPS_WINDOW];
    S32             bpsWindowSteps;
    S32             bpsTotalBytes;
    S32             bpsMaxBytes;
    
    S32             ubpsWindow[MAX_BPS_WINDOW];
    S32             ubpsTotalBytes;
    S32             ubpsMaxBytes;
    
    F32             ucompAve;
    S32             ucompNum;
    // -NERVE - SMF
    
    md3Tag_t        tags[MAX_SERVER_TAGS];
    tagHeaderExt_t  tagHeadersExt[MAX_TAG_FILES];
    
    S32             num_tagheaders;
    S32             num_tags;
} server_t;

typedef struct clientSnapshot_s
{
    S32             areabytes;
    U8              areabits[MAX_MAP_AREA_BYTES];	// portalarea visibility bits
    playerState_t   ps;
    S32             num_entities;
    S32             first_entity;	// into the circular sv_packet_entities[]
    // the entities MUST be in increasing state number
    // order, otherwise the delta compression will fail
    S32             messageSent;	// time the message was transmitted
    S32             messageAcked;	// time the message was acked
    S32             messageSize;	// used to rate drop packets
} clientSnapshot_t;

typedef enum
{
    CS_FREE,					// can be reused for a new connection
    CS_ZOMBIE,					// client has been disconnected, but don't reuse connection for a couple seconds
    CS_CONNECTED,				// has been assigned to a client_t, but no gamestate yet
    CS_PRIMED,					// gamestate has been sent, but client hasn't sent a usercmd
    CS_ACTIVE					// client is fully in game
} clientState_t;

typedef struct netchan_buffer_s
{
    msg_t           msg;
    U8              msgBuffer[MAX_MSGLEN];
    UTF8            lastClientCommandString[MAX_STRING_CHARS];
    struct netchan_buffer_s* next;
} netchan_buffer_t;

typedef struct client_s
{
    clientState_t   state;
    UTF8            userinfo[MAX_INFO_STRING];	// name, etc
    UTF8			userinfobuffer[MAX_INFO_STRING]; //used for buffering of user info
    UTF8			userinfoPostponed[MAX_INFO_STRING];
    
    UTF8            reliableCommands[MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS];
    S32             reliableSequence;	// last added reliable message, not necesarily sent or acknowledged yet
    S32             reliableAcknowledge;	// last acknowledged reliable message
    S32             reliableSent;	// last sent reliable message, not necesarily acknowledged yet
    S32             messageAcknowledge;
    
    S32             gamestateMessageNum;	// netchan->outgoingSequence of gamestate
    S32             challenge;
    
    usercmd_t       lastUsercmd;
    S32             lastMessageNum;	// for delta compression
    S32             lastClientCommand;	// reliable client message sequence
    UTF8            lastClientCommandString[MAX_STRING_CHARS];
    sharedEntity_t* gentity;	// SV_GentityNum(clientnum)
    UTF8            name[MAX_NAME_LENGTH];	// extracted from userinfo, high bits masked
    
    // downloading
    UTF8            downloadName[MAX_QPATH];	// if not empty string, we are downloading
    fileHandle_t    download;	// file being downloaded
    S32             downloadSize;	// total bytes (can't use EOF because of paks)
    S32             downloadCount;	// bytes sent
    S32             downloadClientBlock;	// last block we sent to the client, awaiting ack
    S32             downloadCurrentBlock;	// current block number
    S32             downloadXmitBlock;	// last block we xmited
    U8*             downloadBlocks[MAX_DOWNLOAD_WINDOW];	// the buffers for the download blocks
    S32             downloadBlockSize[MAX_DOWNLOAD_WINDOW];
    bool            downloadEOF;	// We have sent the EOF block
    S32             downloadSendTime;	// time we last got an ack from the client
    
    // www downloading
    bool            bDlOK;		// passed from cl_wwwDownload CVAR_USERINFO, wether this client supports www dl
    UTF8            downloadURL[MAX_OSPATH];	// the URL we redirected the client to
    bool            bWWWDl;		// we have a www download going
    bool            bWWWing;	// the client is doing an ftp/http download
    bool            bFallback;	// last www download attempt failed, fallback to regular download
    // note: this is one-shot, multiple downloads would cause a www download to be attempted again
    
    S32             deltaMessage;	// frame last client usercmd message
    S32             nextReliableTime;	// svs.time when another reliable command will be allowed
    S32				nextReliableUserTime; // svs.time when another userinfo change will be allowed
    S32             lastPacketTime;	// svs.time when packet was last received
    S32             lastConnectTime;	// svs.time when connection started
    S32             nextSnapshotTime;	// send another snapshot when svs.time >= nextSnapshotTime
    bool            rateDelayed;	// true if nextSnapshotTime was set based on rate instead of snapshotMsec
    S32             timeoutCount;	// must timeout a few frames in a row so debugging doesn't break
    clientSnapshot_t frames[PACKET_BACKUP];	// updates can be delta'd from here
    S32             ping;
    S32             rate;		// bytes / second
    S32             snapshotMsec;	// requests a snapshot every snapshotMsec unless rate choked
    S32             pureAuthentic;
    bool            gotCP;		// TTimo - additional flag to distinguish between a bad pure checksum, and no cp command at all
    netchan_t       netchan;
    // TTimo
    // queuing outgoing fragmented messages to send them properly, without udp packet bursts
    // in case large fragmented messages are stacking up
    // buffer them into this queue, and hand them out to netchan as needed
    netchan_buffer_t* netchan_start_queue;
    //% netchan_buffer_t **netchan_end_queue;
    netchan_buffer_t* netchan_end_queue;
    
    //bani
    S32             downloadnotify;
    bool		csUpdated[MAX_CONFIGSTRINGS + 1];
    S32 lastUserInfoChange;
    S32 lastUserInfoCount;
    S32 oldServerTime;
} client_t;

//=============================================================================

// Dushan
#define	STATFRAMES	100
typedef struct svstats_s
{
    F64	active;
    F64	idle;
    S32	count;
    S32	packets;
    
    F64	latched_active;
    F64	latched_idle;
    S32	latched_packets;
} svstats_t;

// MAX_CHALLENGES is made large to prevent a denial
// of service attack that could cycle all of them
// out before legitimate users connected
#define	MAX_CHALLENGES	2048

// Allow a certain amount of challenges to have the same IP address
// to make it a bit harder to DOS one single IP address from connecting
// while not allowing a single ip to grab all challenge resources
#define MAX_CHALLENGES_MULTI (MAX_CHALLENGES / 2)

#define AUTHORIZE_TIMEOUT   5000

typedef struct challenge_s
{
    netadr_t	adr;
    S32 challenge;
    S32 clientChallenge;		// challenge number coming from the client
    S32 time;		// time the last packet was sent to the autherize server
    S32 pingTime;	// time the challenge response was sent to client
    S32 firstTime;	// time the adr was first used, for authorize timeout checks
    S32 firstPing;	// Used for min and max ping checks
    bool connected;
    bool wasrefused;
} challenge_t;

typedef struct
{
    netadr_t  adr;
    S32       time;
} receipt_t;

typedef struct
{
    netadr_t	adr;
    S32			time;
    S32			count;
    bool	    flood;
} floodBan_t;

// MAX_INFO_RECEIPTS is the maximum number of getstatus+getinfo responses that we send
// in a two second time period.
#define MAX_INFO_RECEIPTS  48

typedef struct tempBan_s
{
    netadr_t        adr;
    S32             endtime;
} tempBan_t;

#define MAX_INFO_FLOOD_BANS 36

#define MAX_MASTERS                         8	// max recipients for heartbeat packets
#define MAX_TEMPBAN_ADDRESSES               MAX_CLIENTS

#define SERVER_PERFORMANCECOUNTER_FRAMES    600
#define SERVER_PERFORMANCECOUNTER_SAMPLES   6

// this structure will be cleared only when the game dll changes
typedef struct serverStatic_s
{
    bool        initialized;	// sv_init has completed
    
    S32             time;		// will be strictly increasing across level changes
    
    S32             snapFlagServerBit;	// ^= SNAPFLAG_SERVERCOUNT every SV_SpawnServer()
    
    client_t*       clients;	// [sv_maxclients->integer];
    S32             numSnapshotEntities;	// sv_maxclients->integer*PACKET_BACKUP*MAX_PACKET_ENTITIES
    S32             nextSnapshotEntities;	// next snapshotEntities to use
    entityState_t*  snapshotEntities;	// [numSnapshotEntities]
    S32             nextHeartbeatTime;
    challenge_t     challenges[MAX_CHALLENGES];	// to prevent invalid IPs from connecting
    receipt_t       infoReceipts[MAX_INFO_RECEIPTS];
    floodBan_t		infoFloodBans[MAX_INFO_FLOOD_BANS];
    netadr_t        redirectAddress;	// for rcon return messages
    netadr_t	    authorizeAddress;	// address of server we're using
    tempBan_t       tempBanAddresses[MAX_TEMPBAN_ADDRESSES];
    S32             sampleTimes[SERVER_PERFORMANCECOUNTER_SAMPLES];
    S32             currentSampleIndex;
    S32             totalFrameTime;
    S32             currentFrameIndex;
    S32             serverLoad;
    // Dushan
    svstats_t		stats;
    S32				queryDone;
    
    struct
    {
        bool enabled;
        S32 lastTimeDisconnected;
        F32 sv_fps;
    } hibernation;
    
    bool            gameStarted;
} serverStatic_t;

#if defined (UPDATE_SERVER)

typedef struct
{
    UTF8 version[MAX_QPATH];
    UTF8 platform[MAX_QPATH];
    UTF8 installer[MAX_QPATH];
} versionMapping_t;

#define MAX_UPDATE_VERSIONS 128
extern versionMapping_t versionMap[MAX_UPDATE_VERSIONS];
extern S32 numVersions;
// Maps client version to appropriate installer

#endif

//=============================================================================

extern serverStatic_t svs;		// persistant server info across maps
extern server_t sv;				// cleared each map
extern void*    gvm;

extern convar_t*  sv_fps;
extern convar_t*  sv_timeout;
extern convar_t*  sv_zombietime;
extern convar_t*  sv_rconPassword;
extern convar_t*  sv_privatePassword;
extern convar_t*  sv_allowDownload;
extern convar_t*  sv_friendlyFire;	// NERVE - SMF
extern convar_t*  sv_maxlives;	// NERVE - SMF
extern convar_t*  sv_maxclients;
extern convar_t* sv_democlients;
extern convar_t*  sv_needpass;

extern convar_t*  sv_privateClients;
extern convar_t*  sv_hostname;
extern convar_t*  sv_master[MAX_MASTER_SERVERS];
extern convar_t*  sv_reconnectlimit;
extern convar_t*  sv_tempbanmessage;
extern convar_t*  sv_showloss;
extern convar_t*  sv_padPackets;
extern convar_t*  sv_killserver;
extern convar_t*  sv_mapname;
extern convar_t*  sv_mapChecksum;
extern convar_t*  sv_serverid;
extern convar_t*  sv_maxRate;
extern convar_t*  sv_minPing;
extern convar_t*  sv_maxPing;

//extern    convar_t  *sv_gametype;

extern convar_t*  sv_newGameShlib;

extern convar_t*  sv_pure;
extern convar_t*  sv_floodProtect;
extern convar_t*  sv_allowAnonymous;
extern convar_t*  sv_lanForceRate;
extern convar_t*  sv_onlyVisibleClients;
extern convar_t*  sv_showAverageBPS;	// NERVE - SMF - net debugging

extern convar_t*  sv_requireValidGuid;

extern convar_t*  sv_ircchannel;

extern convar_t*  g_gameType;

// Rafael gameskill
//extern    convar_t  *sv_gameskill;
// done

extern convar_t*  sv_reloading;

// TTimo - autodl
extern convar_t*  sv_dl_maxRate;

// TTimo
extern convar_t*  sv_wwwDownload;	// general flag to enable/disable www download redirects
extern convar_t*  sv_wwwBaseURL;	// the base URL of all the files

// tell clients to perform their downloads while disconnected from the server
// this gets you a better throughput, but you loose the ability to control the download usage
extern convar_t*  sv_wwwDlDisconnected;
extern convar_t*  sv_wwwFallbackURL;

//bani
extern convar_t*  sv_cheats;
extern convar_t*  sv_packetloss;
extern convar_t*  sv_packetdelay;

//fretn
extern convar_t*  sv_fullmsg;

extern convar_t*  sv_IPmaxGetstatusPerSecond;

extern convar_t*  sv_hibernateTime;

extern convar_t* sv_authServerHost;
extern convar_t* sv_authServerKey;

extern convar_t* sv_wh_active;
extern convar_t* sv_wh_bbox_horz;
extern convar_t* sv_wh_bbox_vert;
extern convar_t* sv_wh_check_fov;

//bani - cl->downloadnotify
#define DLNOTIFY_REDIRECT   0x00000001	// "Redirecting client ..."
#define DLNOTIFY_BEGIN      0x00000002	// "clientDownload: 4 : beginning ..."
#define DLNOTIFY_ALL        ( DLNOTIFY_REDIRECT | DLNOTIFY_BEGIN )

#endif //!__SERVER_H__
