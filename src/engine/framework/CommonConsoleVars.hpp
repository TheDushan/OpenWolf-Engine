////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2021 - 2022 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   CommonConsoleVars.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __COMMONCONSOLEVARS_HPP__
#define __COMMONCONSOLEVARS_HPP__

//
// cvars
//
extern convar_t *cl_nodelta;
extern convar_t *cl_debugMove;
extern convar_t *cl_noprint;
extern convar_t *timegraph;
extern convar_t *cl_maxpackets;
extern convar_t *cl_packetdup;
extern convar_t *cl_shownet;
extern convar_t *cl_shownuments;           // DHM - Nerve
extern convar_t *cl_showSend;
extern convar_t *cl_showServerCommands;    // NERVE - SMF
extern convar_t *cl_timeNudge;
extern convar_t *cl_showTimeDelta;
extern convar_t *cl_freezeDemo;

extern convar_t *cl_yawspeed;
extern convar_t *cl_pitchspeed;
extern convar_t *cl_run;
extern convar_t *cl_anglespeedkey;

extern convar_t *cl_recoilPitch;   // RF

extern convar_t *cl_bypassMouseInput;  // NERVE - SMF

extern convar_t *cl_doubletapdelay;

extern convar_t *sensitivity;
extern convar_t *cl_freelook;

extern convar_t *cl_xbox360ControllerAvailable;

extern convar_t *cl_mouseAccel;
extern convar_t *cl_mouseAccelOffset;
extern convar_t *cl_mouseAccelStyle;
extern convar_t *cl_showMouseRate;

extern convar_t *m_pitch;
extern convar_t *m_yaw;
extern convar_t *m_forward;
extern convar_t *m_side;
extern convar_t *m_filter;

extern convar_t *j_pitch;
extern convar_t *j_yaw;
extern convar_t *j_forward;
extern convar_t *j_side;
extern convar_t *j_up;
extern convar_t *j_up_axis;
extern convar_t *j_pitch_axis;
extern convar_t *j_yaw_axis;
extern convar_t *j_forward_axis;
extern convar_t *j_side_axis;

extern convar_t *cl_IRC_connect_at_startup;
extern convar_t *cl_IRC_server;
extern convar_t *cl_IRC_channel;
extern convar_t *cl_IRC_port;
extern convar_t *cl_IRC_override_nickname;
extern convar_t *cl_IRC_nickname;
extern convar_t *cl_IRC_kick_rejoin;
extern convar_t *cl_IRC_reconnect_delay;

extern convar_t *timedemo;

extern convar_t *activeAction;
extern convar_t *cl_autorecord;

extern convar_t *cl_allowDownload;
extern convar_t *cl_conXOffset;
extern convar_t *cl_inGameVideo;

extern convar_t *cl_missionStats;
extern convar_t *cl_waitForFire;
extern convar_t *cl_altTab;

// NERVE - SMF - localization
extern convar_t *cl_language;
extern convar_t *cl_debugTranslation;
// -NERVE - SMF

extern convar_t *cl_profile;
extern convar_t *cl_defaultProfile;

extern convar_t *cl_consoleKeys;
extern convar_t *cl_consoleFont;
extern convar_t *cl_consoleFontSize;
extern convar_t *cl_consoleFontKerning;
extern convar_t *cl_consolePrompt;
extern convar_t *cl_aviFrameRate;
extern convar_t *cl_aviMotionJpeg;
extern convar_t *cl_guidServerUniq;

//bani
extern convar_t *sv_cheats;

extern convar_t *cl_serverStatusResendTime;
extern convar_t *cl_gamename;

extern convar_t *con_drawnotify;

#ifndef BSPC
extern convar_t *cm_noAreas;
extern convar_t *cm_noCurves;
extern convar_t *cm_playerCurveClip;
extern convar_t *cm_forceTriangles;
extern convar_t *cm_optimize;
extern convar_t *cm_showCurves;
extern convar_t *cm_showTriangles;
#endif

extern convar_t *cl_shownet;

extern convar_t *com_crashed;

extern convar_t *com_ignorecrash;  //bani

extern convar_t *com_protocol;
#ifndef DEDICATED
extern  convar_t *con_autochat;
#endif
extern convar_t *com_pid;      //bani

extern convar_t *developer;
extern convar_t *dedicated;
extern convar_t *com_speeds;
extern convar_t *timescale;
extern convar_t *sv_running;
extern convar_t *cl_running;
extern convar_t *com_version;

//extern    convar_t  *com_blood;
extern convar_t *com_buildScript;  // for building release pak files
extern convar_t *journal;
extern convar_t *com_cameraMode;
extern convar_t *com_ansiColor;

extern convar_t *com_unfocused;
extern convar_t *com_minimized;


// watchdog
extern convar_t *com_watchdog;
extern convar_t *com_watchdog_cmd;

// both client and server must agree to pause
extern convar_t *cl_paused;
extern convar_t *sv_paused;

extern convar_t *cl_packetdelay;
extern convar_t *sv_packetdelay;

extern convar_t *sv_fps;
extern convar_t *sv_timeout;
extern convar_t *sv_zombietime;
extern convar_t *sv_rconPassword;
extern convar_t *sv_privatePassword;
extern convar_t *sv_allowDownload;
extern convar_t *sv_friendlyFire;  // NERVE - SMF
extern convar_t *sv_maxlives;  // NERVE - SMF
extern convar_t *sv_maxclients;
extern convar_t *sv_threads;
extern convar_t *sv_needpass;

extern convar_t *sv_privateClients;
extern convar_t *sv_hostname;
extern convar_t *sv_master[MAX_MASTER_SERVERS];
extern convar_t *sv_reconnectlimit;
extern convar_t *sv_tempbanmessage;
extern convar_t *sv_showloss;
extern convar_t *sv_padPackets;
extern convar_t *sv_killserver;
extern convar_t *sv_mapname;
extern convar_t *sv_mapChecksum;
extern convar_t *sv_serverid;
extern convar_t *sv_maxRate;
extern convar_t *sv_dlRate;
extern convar_t *sv_minPing;
extern convar_t *sv_maxPing;

//extern    convar_t  *sv_gametype;

extern convar_t *sv_newGameShlib;

extern convar_t *sv_pure;
extern convar_t *sv_floodProtect;
extern convar_t *sv_allowAnonymous;
extern convar_t *sv_lanForceRate;
extern convar_t *sv_onlyVisibleClients;
extern convar_t *sv_showAverageBPS;    // NERVE - SMF - net debugging

extern convar_t *sv_requireValidGuid;

extern convar_t *sv_ircchannel;

extern convar_t *g_gameType;

// Rafael gameskill
//extern    convar_t  *sv_gameskill;
// done

extern convar_t *g_reloading;

// TTimo - autodl
extern convar_t *sv_dl_maxRate;

// TTimo
extern convar_t
*sv_wwwDownload;    // general flag to enable/disable www download redirects
extern convar_t *sv_wwwBaseURL;    // the base URL of all the files

// tell clients to perform their downloads while disconnected from the server
// this gets you a better throughput, but you loose the ability to control the download usage
extern convar_t *sv_wwwDlDisconnected;
extern convar_t *sv_wwwFallbackURL;

//bani
extern convar_t *sv_cheats;
extern convar_t *sv_packetloss;
extern convar_t *sv_packetdelay;

//fretn
extern convar_t *sv_fullmsg;

extern convar_t *sv_IPmaxGetstatusPerSecond;

extern convar_t *sv_hibernateTime;

extern convar_t *sv_wh_active;
extern convar_t *sv_wh_bbox_horz;
extern convar_t *sv_wh_bbox_vert;
extern convar_t *sv_wh_check_fov;

extern convar_t *sv_minimumAgeGuid; // Min guid age to enter a server
extern convar_t *sv_maximumAgeGuid; // Max guid age to enter a server

extern convar_t
*sv_cs_ServerType;  // 0: public, 1: public-registered, 2: private
extern convar_t *sv_cs_Salt;
extern convar_t *sv_cs_BotLog;
extern convar_t *sv_cs_MemberColor;
extern convar_t *sv_cs_UnknownColor;
extern convar_t *sv_cs_PrivateOnlyMSG;
extern convar_t *sv_cs_stats;
extern convar_t *sv_cs_ServerPort;

extern convar_t *sv_autoRecDemo;
extern convar_t *sv_autoRecDemoBots;
extern convar_t *sv_autoRecDemoMaxMaps;


// Cvars to configure OACS behavior
extern convar_t *sv_oacsEnable; // enable the extended logging facility?
extern convar_t
*sv_oacsPlayersTableEnable;  // enable the extended player identification logging?
extern convar_t *sv_oacsTypesFile; // where to save the features types
extern convar_t *sv_oacsDataFile; // where to save the features data
extern convar_t
*sv_oacsPlayersTable;  // where to save the players table (if enabled)
extern convar_t
*sv_oacsMinPlayers;  // minimum number of human players required to begin logging data
extern convar_t
*sv_oacsLabelPassword;  // password necessary for a player to label himself
extern convar_t
*sv_oacsMaxPing;  // max ping to accept interframes (above, the interframe will be dropped until the ping goes down)
extern convar_t
*sv_oacsMaxLastPacketTime;  // max last packet time to accept interframes (above, the interframe will be dropped until the LastPacketTime goes down)

extern convar_t *s_volume;
extern convar_t *s_nosound;
extern convar_t *s_khz;
extern convar_t *s_show;
extern convar_t *s_mixahead;

extern convar_t *s_testsound;
extern convar_t *s_separation;

#if defined (__MACOSX__)
extern convar_t *fs_apppath;
#endif

extern convar_t *fs_basepath;

extern convar_t *net_enabled;
extern convar_t *s_module;
extern convar_t *com_recommended;
extern convar_t *com_highSettings;
extern convar_t *net_qport;
extern convar_t *fixedtime;
extern convar_t *s_mixPreStep;
extern convar_t *in_nograb;
extern convar_t *savegame_loading;
extern convar_t *com_abnormalExit;
extern convar_t *logfile;
extern convar_t *s_initsound;
extern convar_t *s_musicVolume;
extern convar_t *s_doppler;
extern convar_t *s_sdlMixSamps;
extern convar_t *s_sdlChannels;
extern convar_t *com_dropsim;
extern convar_t *com_timedemo;
extern convar_t *com_maxfpsMinimized;
extern convar_t *com_maxfpsUnfocused;
extern convar_t *com_showtrace;
extern convar_t *com_hunkused;
extern convar_t *con_restricted;
extern convar_t *con_autoclear;
extern convar_t *con_notifytime;
extern convar_t *scr_conColorRed;
extern convar_t *scr_conColorGreen;
extern convar_t *scr_conColorBlue;
extern convar_t *scr_conHeight;
extern convar_t *scr_conBarColorRed;
extern convar_t *scr_conBarColorGreen;
extern convar_t *scr_conBarColorBlue;
extern convar_t *scr_conBarColorAlpha;
extern convar_t *scr_conColorAlpha;
extern convar_t *con_conspeed;
extern convar_t *cl_motd;
extern convar_t *cl_motdString;
extern convar_t *rcon_client_password;
extern convar_t *rconAddres;
extern convar_t *cl_updatefiles;
extern convar_t *cl_guid;
extern convar_t *cl_updateavailable;
extern convar_t *cl_timeout;
extern convar_t *cl_forceavidemo;
extern convar_t *cl_autoupdate;
extern convar_t *fs_homepath;
extern convar_t *fs_debug;
extern convar_t *fs_restrict;
extern convar_t *fs_basegame;
extern convar_t *fs_game;
extern convar_t *fs_texturesfolder;
extern convar_t *fs_soundsfolder;
extern convar_t *fs_modelsfolder;
extern convar_t *net_mcast6addr;
extern convar_t *net_mcast6iface;
extern convar_t *net_socksServer;
extern convar_t *net_socksPort;
extern convar_t *net_socksUsername;
extern convar_t *net_socksPassword;
extern convar_t *net_port;
extern convar_t *net_port6;
extern convar_t *net_ip6;
extern convar_t *net_ip;
extern convar_t *showpackets;
extern convar_t *showdrop;
extern convar_t *s_sdlBits;
extern convar_t *s_sdlDevSamps;
extern convar_t *in_keyboardDebug;
extern convar_t *in_joystick;
extern convar_t *in_joystickNo;
extern convar_t *in_joystickUseAnalog;
extern convar_t *in_joystickThreshold;
extern convar_t *in_mouse;
extern convar_t *fs_missing;
extern convar_t *net_socksEnabled;
extern convar_t *cl_maxPing;
extern convar_t *rconAddress;
extern convar_t *debuggraph;
extern convar_t *graphheight;
extern convar_t *graphscale;
extern convar_t *graphshift;
extern convar_t *r_fullscreen;
extern convar_t *cl_logChat;

#endif //!__COMMONCONSOLEVARS_HPP__
