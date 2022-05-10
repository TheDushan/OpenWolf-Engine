////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   CommonConsoleVars.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <server/serverAutoPrecompiled.hpp>
#elif DEDICATED
#include <server/serverDedPrecompiled.hpp>
#else
#include <framework/precompiled.hpp>
#endif

convar_t *timegraph;
convar_t *debuggraph;
convar_t *graphheight;
convar_t *graphscale;
convar_t *graphshift;
convar_t *con_conspeed;
convar_t *con_notifytime;
convar_t *con_autoclear;

// Color and alpha for console
convar_t *scr_conColorAlpha;
convar_t *scr_conColorRed;
convar_t *scr_conColorBlue;
convar_t *scr_conColorGreen;

// Color and alpha for bar under console
convar_t *scr_conBarHeight;

convar_t *scr_conBarColorAlpha;
convar_t *scr_conBarColorRed;
convar_t *scr_conBarColorBlue;
convar_t *scr_conBarColorGreen;

convar_t *scr_conBarSize;
convar_t *scr_conHeight;

// DHM - Nerve :: Must hold CTRL + SHIFT + ~ to get console
convar_t *con_restricted;
convar_t *cl_upspeed;
convar_t *cl_forwardspeed;
convar_t *cl_sidespeed;
convar_t *cl_yawspeed;
convar_t *cl_pitchspeed;
convar_t *cl_run;
convar_t *cl_anglespeedkey;
convar_t *cl_recoilPitch;
convar_t *cl_bypassMouseInput;  // NERVE - SMF
convar_t *cl_doubletapdelay;
convar_t *cl_wavefilerecord;
convar_t *cl_nodelta;
convar_t *cl_debugMove;

convar_t *cl_noprint;
convar_t *cl_motd;
convar_t *cl_autoupdate;    // DHM - Nerve

convar_t *rcon_client_password;
convar_t *rconAddress;

convar_t *cl_timeout;
convar_t *cl_maxpackets;
convar_t *cl_packetdup;
convar_t *cl_timeNudge;
convar_t *cl_showTimeDelta;
convar_t *cl_freezeDemo;

convar_t *cl_shownet =
    nullptr; // NERVE - SMF - This is referenced in msg.c and we need to make sure it is nullptr
convar_t *cl_shownuments;   // DHM - Nerve
convar_t *cl_showSend;
convar_t *cl_showServerCommands;    // NERVE - SMF
convar_t *timedemo;

convar_t *cl_aviFrameRate;
convar_t *cl_forceavidemo;

convar_t *cl_freelook;
convar_t *sensitivity;
convar_t *cl_xbox360ControllerAvailable;

convar_t *cl_mouseAccelOffset;
convar_t *cl_mouseAccel;
convar_t *cl_mouseAccelStyle;
convar_t *cl_showMouseRate;

convar_t *m_pitch;
convar_t *m_yaw;
convar_t *m_forward;
convar_t *m_side;
convar_t *m_filter;

convar_t *j_pitch;
convar_t *j_yaw;
convar_t *j_forward;
convar_t *j_side;
convar_t *j_up;
convar_t *j_pitch_axis;
convar_t *j_yaw_axis;
convar_t *j_forward_axis;
convar_t *j_side_axis;
convar_t *j_up_axis;

convar_t *activeAction;

convar_t *cl_autorecord;

convar_t *cl_motdString;

convar_t *cl_allowDownload;
convar_t *cl_wwwDownload;
convar_t *cl_conXOffset;
convar_t *cl_inGameVideo;

convar_t *cl_serverStatusResendTime;
convar_t *cl_trn;
convar_t *cl_missionStats;
convar_t *cl_waitForFire;

// NERVE - SMF - localization
convar_t *cl_language;
convar_t *cl_debugTranslation;

// -NERVE - SMF
// DHM - Nerve :: Auto-Update
convar_t *cl_updateavailable;
convar_t *cl_updatefiles;

// DHM - Nerve

convar_t *cl_profile;
convar_t *cl_defaultProfile;

convar_t *cl_demorecording; // fretn
convar_t *cl_demofilename;  // bani
convar_t *cl_demooffset;    // bani

convar_t *cl_waverecording; //bani
convar_t *cl_wavefilename;  //bani
convar_t *cl_waveoffset;    //bani

convar_t *cl_packetloss;    //bani
convar_t *cl_packetdelay;   //bani

convar_t *cl_consoleKeys;
convar_t *cl_consoleFont;
convar_t *cl_consoleFontSize;
convar_t *cl_consoleFontKerning;
convar_t *cl_consolePrompt;

convar_t *cl_gamename;
convar_t *cl_altTab;

convar_t *cl_aviMotionJpeg;
convar_t *cl_guidServerUniq;

convar_t *cl_guid;

#ifndef BSPC
convar_t *cm_noAreas;
convar_t *cm_noCurves;
convar_t *cm_forceTriangles;
convar_t *cm_playerCurveClip;
convar_t *cm_optimize;
convar_t *cm_showCurves;
convar_t *cm_showTriangles;
#endif

convar_t *fs_debug;
convar_t *fs_homepath;
convar_t *fs_basepath;
convar_t *fs_libpath;

#if defined (__MACOSX__)
// Also search the .app bundle for .pk3 files
convar_t *fs_apppath;
#endif

convar_t *fs_buildpath;
convar_t *fs_buildgame;
convar_t *fs_basegame;
convar_t *fs_copyfiles;
convar_t *fs_game;
convar_t *fs_texturesfolder;
convar_t *fs_soundsfolder;
convar_t *fs_modelsfolder;
convar_t *fs_missing;
convar_t *fs_restrict;

convar_t *net_enabled;

convar_t *net_socksEnabled;
convar_t *net_socksServer;
convar_t *net_socksPort;
convar_t *net_socksUsername;
convar_t *net_socksPassword;

convar_t *net_ip;
convar_t *net_ip6;
convar_t *net_port;
convar_t *net_port6;
convar_t *net_mcast6addr;
convar_t *net_mcast6iface;

convar_t *showpackets;
convar_t *showdrop;
convar_t *net_qport;
convar_t *in_keyboardDebug = nullptr;
convar_t *in_mouse = nullptr;
convar_t *in_nograb;

convar_t *in_joystick = nullptr;
convar_t *in_joystickDebug = nullptr;
convar_t *in_joystickThreshold = nullptr;
convar_t *in_joystickNo = nullptr;
convar_t *in_joystickUseAnalog = nullptr;

convar_t *s_sdlBits;
convar_t *s_sdlChannels;
convar_t *s_sdlDevSamps;
convar_t *s_sdlMixSamps;
convar_t *com_crashed =
    nullptr;    // ydnar: set in case of a crash, prevents CVAR_UNSAFE variables from being set from a cfg

//bani - explicit nullptr to make win32 teh happy

convar_t *com_ignorecrash =
    nullptr;    // bani - let experienced users ignore crashes, explicit nullptr to make win32 teh happy
convar_t *com_pid;      // bani - process id

convar_t *com_speeds;
convar_t *developer;
convar_t *dedicated;
convar_t *timescale;
convar_t *fixedtime;
convar_t *com_dropsim;  // 0.0 to 1.0, simulated packet drops
convar_t *journal;
convar_t *com_timedemo;
convar_t *sv_running;
convar_t *cl_running;
convar_t *com_showtrace;
convar_t *com_version;
convar_t *logfile;      // 1 = buffer log, 2 = flush after each print
convar_t *com_buildScript;  // for automated data building scripts
convar_t *con_drawnotify;
convar_t *com_ansiColor;

convar_t *com_unfocused;
convar_t *com_minimized;

convar_t *com_introPlayed;
convar_t *cl_paused;
convar_t *sv_paused;
//convar_t         *sv_packetdelay;
convar_t *com_cameraMode;
convar_t *com_maxfpsUnfocused;
convar_t *com_maxfpsMinimized;
convar_t *com_abnormalExit;

convar_t *com_watchdog;
convar_t *com_watchdog_cmd;

// Rafael Notebook
convar_t *cl_notebook;

convar_t *com_hunkused; // Ridah
convar_t *com_protocol;

#ifndef DEDICATED
convar_t *con_autochat;
#endif

convar_t *sv_fps = nullptr;         // time rate for running non-clients
convar_t *sv_timeout;       // seconds without any message
convar_t *sv_zombietime;    // seconds to sink messages after disconnect
convar_t *sv_rconPassword;  // password for remote server commands
convar_t *sv_privatePassword;   // password for the privateClient slots
convar_t *sv_allowDownload;
convar_t *sv_maxclients;

convar_t *sv_privateClients;    // number of clients reserved for password
convar_t *sv_hostname;
convar_t *sv_master[MAX_MASTER_SERVERS];    // master server ip address
convar_t *sv_reconnectlimit;    // minimum seconds between connect messages
convar_t *sv_tempbanmessage;
convar_t *sv_showloss;  // report when usercmds are lost
convar_t *sv_padPackets;    // add nop bytes to messages
convar_t *sv_killserver;    // menu system can set to 1 to shut server down
convar_t *sv_mapname;
convar_t *sv_mapChecksum;
convar_t *sv_serverid;
convar_t *sv_maxRate;
convar_t *sv_minPing;
convar_t *sv_maxPing;
convar_t *sv_dlRate;

//convar_t    *sv_gametype;
convar_t *sv_pure;
convar_t *sv_newGameShlib;
convar_t *sv_floodProtect;
convar_t *sv_allowAnonymous;
convar_t *sv_lanForceRate;  // TTimo - dedicated 1 (LAN) server forces local client rates to 99999 (bug #491)
convar_t *sv_onlyVisibleClients;    // DHM - Nerve
convar_t *sv_friendlyFire;  // NERVE - SMF
convar_t *sv_maxlives;  // NERVE - SMF
convar_t *sv_needpass;
convar_t *sv_lagAbuse;
convar_t *sv_lagAbuseFPS;

convar_t *sv_dl_maxRate;
convar_t *g_gameType;

// of characters '0' through '9' and 'A' through 'F', default 0 don't require
// Rafael gameskill
//convar_t    *sv_gameskill;
// done

convar_t *g_reloading;

convar_t *sv_showAverageBPS;    // NERVE - SMF - net debugging

convar_t *sv_wwwDownload;   // server does a www dl redirect
convar_t *sv_wwwBaseURL;    // base URL for redirect

// tell clients to perform their downloads while disconnected from the server
// this gets you a better throughput, but you loose the ability to control the download usage
convar_t *sv_wwwDlDisconnected;
convar_t *sv_wwwFallbackURL;    // URL to send to if an http/ftp fails or is refused client side

//bani
convar_t *sv_cheats;
convar_t *sv_packetloss;
convar_t *sv_packetdelay;

// fretn
convar_t *sv_fullmsg;

convar_t *sv_hibernateTime;

convar_t *sv_wh_active;
convar_t *sv_wh_bbox_horz;
convar_t *sv_wh_bbox_vert;
convar_t *sv_wh_check_fov;

convar_t *sv_minimumAgeGuid;
convar_t *sv_maximumAgeGuid;

convar_t *sv_cs_ServerType;
convar_t *sv_cs_Salt;
convar_t *sv_cs_BotLog;
convar_t *sv_cs_MemberColor;
convar_t *sv_cs_UnknownColor;
convar_t *sv_cs_PrivateOnlyMSG;
convar_t *sv_cs_stats;
convar_t *sv_cs_ServerPort;

convar_t *sv_autoRecDemo;
convar_t *sv_autoRecDemoBots;
convar_t *sv_autoRecDemoMaxMaps;

// Cvars to configure OACS behavior
convar_t *sv_oacsEnable;
convar_t *sv_oacsPlayersTableEnable;
convar_t *sv_oacsTypesFile;
convar_t *sv_oacsDataFile;
convar_t *sv_oacsPlayersTable;
convar_t *sv_oacsMinPlayers;
convar_t *sv_oacsLabelPassword;
convar_t *sv_oacsMaxPing;
convar_t *sv_oacsMaxLastPacketTime;

convar_t *s_volume;
convar_t *s_testsound;
convar_t *s_khz;
convar_t *s_show;
convar_t *s_mixahead;
convar_t *s_mixPreStep; //Dushan - not used
convar_t *s_musicVolume;
convar_t *s_separation; //Dushan - not used
convar_t *s_doppler;
convar_t *s_module;
convar_t *com_recommended;
convar_t *com_highSettings;
convar_t *savegame_loading;
convar_t *s_initsound;
convar_t *hunk_soundadjust;
convar_t *cl_maxPing;
convar_t *r_fullscreen;

convar_t *cl_logChat;

/*
===============
idCommonLocal::InitCommonConsoleVars
===============
*/
void idCommonLocal::InitCommonConsoleVars(void) {
    sint index, pid;
    valueType *s;

    developer = cvarSystem->Get("developer", "0", CVAR_TEMP,
                                "Enable/disable (1/0) developer mode, allows cheats and so on.");

    timegraph = cvarSystem->Get("timegraph", "0", CVAR_CHEAT,
                                "description");
    debuggraph = cvarSystem->Get("debuggraph", "0", CVAR_CHEAT,
                                 "description");
    graphheight = cvarSystem->Get("graphheight", "32", CVAR_CHEAT,
                                  "description");
    graphscale = cvarSystem->Get("graphscale", "1", CVAR_CHEAT,
                                 "description");
    graphshift = cvarSystem->Get("graphshift", "0", CVAR_CHEAT,
                                 "description");
    con_notifytime = cvarSystem->Get("con_notifytime", "7", 0,
                                     "^1Defines how long messages (from players or the system) are on the screen.");
    con_conspeed = cvarSystem->Get("scr_conspeed", "3", 0,
                                   "^1Set how fast the console goes up and down.");
    con_autoclear = cvarSystem->Get("con_autoclear", "1", CVAR_ARCHIVE,
                                    "^1Toggles clearing of unfinished text after closing console.");
    con_restricted = cvarSystem->Get("con_restricted", "0", CVAR_INIT,
                                     "^1Toggles clearing of unfinished text after closing console.");
    scr_conColorAlpha = cvarSystem->Get("scr_conColorAlpha", "0.5",
                                        CVAR_ARCHIVE, "^1Defines the backgroud Alpha color of the console.");
    scr_conColorRed = cvarSystem->Get("scr_conColorRed", "0", CVAR_ARCHIVE,
                                      "^1Defines the backgroud Red color of the console.");
    scr_conColorBlue = cvarSystem->Get("scr_conColorBlue", "0.3", CVAR_ARCHIVE,
                                       "^1Defines the backgroud Blue color of the console.");
    scr_conColorGreen = cvarSystem->Get("scr_conColorGreen", "0.23",
                                        CVAR_ARCHIVE, "^1Defines the backgroud Green color of the console.");

    scr_conBarHeight = cvarSystem->Get("scr_conBarHeight", "2", CVAR_ARCHIVE,
                                       "^1Defines the bar height of the console.");

    scr_conBarColorAlpha = cvarSystem->Get("scr_conBarColorAlpha", "0.3",
                                           CVAR_ARCHIVE, "^1Defines the bar Alpha color of the console.");
    scr_conBarColorRed = cvarSystem->Get("scr_conBarColorRed", "1",
                                         CVAR_ARCHIVE, "^1Defines the bar Red color of the console.");
    scr_conBarColorBlue = cvarSystem->Get("scr_conBarColorBlue", "1",
                                          CVAR_ARCHIVE, "^1Defines the bar Blue color of the console.");
    scr_conBarColorGreen = cvarSystem->Get("scr_conBarColorGreen", "1",
                                           CVAR_ARCHIVE, "^1Defines the bar Green color of the console.");

    scr_conHeight = cvarSystem->Get("scr_conHeight", "52", CVAR_ARCHIVE,
                                    "^1Console height size.");

    scr_conBarSize = cvarSystem->Get("scr_conBarSize", "2", CVAR_ARCHIVE,
                                     "^1Console bar size.");

    cl_nodelta = cvarSystem->Get("cl_nodelta", "0", 0,
                                 "Wether to disable delta compression for networking stuff.");
    cl_debugMove = cvarSystem->Get("cl_debugMove", "0", 0,
                                   "Draws a chart at the bottom displaying something to do with how much you look around.");

    cl_noprint = cvarSystem->Get("cl_noprint", "0", 0,
                                 "At 1, it doesnt print to the console. Doesnt affect ingame-console messages.");
    cl_motd = cvarSystem->Get("cl_motd", "1", 0,
                              "Wether to get the motd string from tmaphe master server");
    cl_autoupdate = cvarSystem->Get("cl_autoupdate", "1", CVAR_ARCHIVE,
                                    "Automatic game update checks on launch.");

    cl_timeout = cvarSystem->Get("cl_timeout", "200", 0,
                                 "Set the inactivity time before a client is disconnected (timed out)");

    cl_wavefilerecord = cvarSystem->Get("cl_wavefilerecord", "0", CVAR_TEMP,
                                        "Toggle recording a .wav audio file upon loading a demo. Suggest setting to 0 in autoexec.cfg");

    cl_timeNudge = cvarSystem->Get("cl_timeNudge", "0", CVAR_ARCHIVE,
                                   "Supposed to be for adjusting prediction for your ping.");
    cl_shownet = cvarSystem->Get("cl_shownet", "0", CVAR_TEMP,
                                 "Display network quality graph");
    cl_shownuments = cvarSystem->Get("cl_shownuments", "0", CVAR_TEMP,
                                     "Display the number of entities in each packet");
    cl_showServerCommands = cvarSystem->Get("cl_showServerCommands", "0", 0,
                                            "Show server commands");
    cl_showSend = cvarSystem->Get("cl_showSend", "0", CVAR_TEMP,
                                  "Shows each packet");
    cl_showTimeDelta = cvarSystem->Get("cl_showTimeDelta", "0", CVAR_TEMP,
                                       "Shows time difference between each packet");
    cl_freezeDemo = cvarSystem->Get("cl_freezeDemo", "0", CVAR_TEMP,
                                    "Pauses demo playback");
    rcon_client_password = cvarSystem->Get("rconPassword", "", CVAR_TEMP,
                                           "Password for remote console access");
    activeAction = cvarSystem->Get("activeAction", "", CVAR_TEMP,
                                   "Perform the specified when joining server");
    cl_autorecord = cvarSystem->Get("cl_autorecord", "0", CVAR_TEMP,
                                    "At 1, then it will start/stop recording a demo at the start/end of each match.");

    timedemo = cvarSystem->Get("timedemo", "0", 0,
                               "Set to 1 to enable timedemo mode,for benchmarking purposes");
    cl_forceavidemo = cvarSystem->Get("cl_forceavidemo", "0", 0,
                                      "Forces all demo recording into a sequence of screenshots in TGA format.");
    cl_aviFrameRate = cvarSystem->Get("cl_aviFrameRate", "25", CVAR_ARCHIVE,
                                      "Framerate to use when capturing video");

    cl_aviMotionJpeg = cvarSystem->Get("cl_aviMotionJpeg", "1", CVAR_ARCHIVE,
                                       "Use the MJPEG codec when capturing video");

    rconAddress = cvarSystem->Get("rconAddress", "", 0,
                                  "Alternate server address to remotely access via rcon protocol");

    cl_yawspeed = cvarSystem->Get("cl_yawspeed", "140", CVAR_ARCHIVE,
                                  "Sets turn speed when using +left and +right.");
    cl_pitchspeed = cvarSystem->Get("cl_pitchspeed", "140", CVAR_ARCHIVE,
                                    "Turn speed when using keyboard to look up/down");
    cl_anglespeedkey = cvarSystem->Get("cl_anglespeedkey", "1.5", 0,
                                       "When pressing +left or +right keys, this sets the speed that the view angle turns");

    cl_maxpackets = cvarSystem->Get("cl_maxpackets", "30", CVAR_ARCHIVE,
                                    "Cap for data packet transmissions (upstream)");
    cl_packetdup = cvarSystem->Get("cl_packetdup", "1", CVAR_ARCHIVE,
                                   "Number of duplicates for every data packet sent upstream, minimised packetloss");

    cl_run = cvarSystem->Get("cl_run", "1", CVAR_ARCHIVE,
                             "Toggle 'always run' setting");
    sensitivity = cvarSystem->Get("sensitivity", "5", CVAR_ARCHIVE,
                                  "Used for setting the mouse sensitivity");
    cl_mouseAccel = cvarSystem->Get("cl_mouseAccel", "0", CVAR_ARCHIVE,
                                    "Toggles mouse accelleration");
    cl_freelook = cvarSystem->Get("cl_freelook", "1", CVAR_ARCHIVE,
                                  "Look around using the mouse");

    cl_xbox360ControllerAvailable =
        cvarSystem->Get("in_xbox360ControllerAvailable", "0", CVAR_ARCHIVE,
                        "Use Xbox 360 controller if it is avaliable");

    // 0: legacy mouse acceleration
    // 1: new implementation

    cl_mouseAccelStyle = cvarSystem->Get("cl_mouseAccelStyle", "0",
                                         CVAR_ARCHIVE, "Sets mouse acceleration style.");
    // offset for the power function (for style 1, ignored otherwise)
    // this should be set to the max rate value
    cl_mouseAccelOffset = cvarSystem->Get("cl_mouseAccelOffset", "5",
                                          CVAR_ARCHIVE, "Sets mouse acceleration sensitivity offset.");

    cl_showMouseRate = cvarSystem->Get("cl_showmouserate", "0", 0,
                                       "Show how fast you move the mouse in ratio to sensitivity setting.");

    cl_allowDownload = cvarSystem->Get("cl_allowDownload", "1", CVAR_ARCHIVE,
                                       "Toggles downloading missing files from the server");
    cl_wwwDownload = cvarSystem->Get("cl_wwwDownload", "1",
                                     CVAR_USERINFO | CVAR_ARCHIVE,
                                     "Toggles downloading missing files from a www file server");

    cl_profile = cvarSystem->Get("cl_profile", "", CVAR_ROM,
                                 "Stores which player profile is being used");
    cl_defaultProfile = cvarSystem->Get("cl_defaultProfile", "", CVAR_ROM,
                                        "Sets what player profile is to be used by default when loading the game");

    cl_conXOffset = cvarSystem->Get("cl_conXOffset", "3", 0,
                                    "Supposed to move the on-screen console text up/down");
    cl_inGameVideo = cvarSystem->Get("r_inGameVideo", "1", CVAR_ARCHIVE,
                                     "Toggle use of video clips in game");

    cl_serverStatusResendTime = cvarSystem->Get("cl_serverStatusResendTime",
                                "750", 0, "Sets the amount of time (in milliseconds) between heartbeats sent to the master server");

    cl_recoilPitch = cvarSystem->Get("cg_recoilPitch", "0", CVAR_ROM,
                                     "Sets the amount of the recoil pitch");

    cl_bypassMouseInput = cvarSystem->Get("cl_bypassMouseInput", "0", 0,
                                          "Automatically toggled when there are items (e.g. Vchat menu) on screen, to maintain mouse focus being in game, instead of in menu.");

    cl_doubletapdelay = cvarSystem->Get("cl_doubletapdelay", "100",
                                        CVAR_ARCHIVE,
                                        "Sets the delay between keypresses required to be a double-tap");

    m_pitch = cvarSystem->Get("m_pitch", "0.022", CVAR_ARCHIVE,
                              "Sets the mouse pitch (up/down)");
    m_yaw = cvarSystem->Get("m_yaw", "0.022", CVAR_ARCHIVE,
                            "Sets the mouse yaw (left/right)");
    m_forward = cvarSystem->Get("m_forward", "0.25", CVAR_ARCHIVE,
                                "Sets the mouse yaw (forward)");
    m_side = cvarSystem->Get("m_side", "0.25", CVAR_ARCHIVE,
                             "Sets the mouse yaw (side)");
    m_filter = cvarSystem->Get("m_filter", "0", CVAR_ARCHIVE,
                               "Toggles mouse filter (mouse smoothing)");

    j_pitch = cvarSystem->Get("j_pitch", "0.022", CVAR_ARCHIVE,
                              "Sets joystics pitch");
    j_yaw = cvarSystem->Get("j_yaw", "-0.022", CVAR_ARCHIVE,
                            "Sets joystics yaw");
    j_forward = cvarSystem->Get("j_forward", "-0.25", CVAR_ARCHIVE,
                                "Sets joystics forward");
    j_side = cvarSystem->Get("j_side", "0.25", CVAR_ARCHIVE,
                             "Sets joystics side");
    j_up = cvarSystem->Get("j_up", "1", CVAR_ARCHIVE, "Sets joystics up");
    j_pitch_axis = cvarSystem->Get("j_pitch_axis", "3", CVAR_ARCHIVE,
                                   "Sets joystics axis pitch");
    j_yaw_axis = cvarSystem->Get("j_yaw_axis", "4", CVAR_ARCHIVE,
                                 "Sets joystics axis yaw");
    j_forward_axis = cvarSystem->Get("j_forward_axis", "1", CVAR_ARCHIVE,
                                     "Sets joystics axis forward");
    j_side_axis = cvarSystem->Get("j_side_axis", "0", CVAR_ARCHIVE,
                                  "Sets joystics axis side");
    j_up_axis = cvarSystem->Get("j_up_axis", "2", CVAR_ARCHIVE,
                                "Sets joystics axis up");

    cvarSystem->CheckRange(j_pitch_axis, 0, MAX_JOYSTICK_AXIS - 1, true);
    cvarSystem->CheckRange(j_yaw_axis, 0, MAX_JOYSTICK_AXIS - 1, true);
    cvarSystem->CheckRange(j_forward_axis, 0, MAX_JOYSTICK_AXIS - 1, true);
    cvarSystem->CheckRange(j_side_axis, 0, MAX_JOYSTICK_AXIS - 1, true);
    cvarSystem->CheckRange(j_up_axis, 0, MAX_JOYSTICK_AXIS - 1, true);

    cl_motdString = cvarSystem->Get("cl_motdString", "", CVAR_ROM,
                                    "Message of the day string.");

    // ~ and `, as keys and characters
    cl_consoleKeys = cvarSystem->Get("cl_consoleKeys", "~ ` 0x7e 0x60",
                                     CVAR_ARCHIVE, "Toggle button for opening in-game console");

    cl_consoleFont = cvarSystem->Get("cl_consoleFont", "",
                                     CVAR_ARCHIVE | CVAR_LATCH,
                                     "The outline font used for the in-game console.	");
    cl_consoleFontSize = cvarSystem->Get("cl_consoleFontSize", "16",
                                         CVAR_ARCHIVE | CVAR_LATCH,
                                         "The size of the console font (if using an outline font).");
    cl_consoleFontKerning = cvarSystem->Get("cl_consoleFontKerning", "0",
                                            CVAR_ARCHIVE, "The width of the console font char.");
    cl_consolePrompt = cvarSystem->Get("cl_consolePrompt", "^3->",
                                       CVAR_ARCHIVE, "The console prompt line.");

    cl_gamename = cvarSystem->Get("cl_gamename", GAMENAME_FOR_MASTER,
                                  CVAR_TEMP,
                                  "Gamename sent to master server in getservers[Ext] query and infoResponse, infostring value.Also used for filtering local network games.");
    cl_altTab = cvarSystem->Get("cl_altTab", "1", CVAR_ARCHIVE,
                                "Allow ⎇ Alt + Tab ↹ out of application (⌘ Cmd + Tab ↹ on a Mac)");

    //bani - make these cvars visible to cgame
    cl_demorecording = cvarSystem->Get("cl_demorecording", "0", CVAR_ROM,
                                       "This CVAR is Read Only, you can use it in-game though, basically if you have turned recording on in-game, you can use this CVAR to make sure it is recording in the console, if it says 1 then its recording.");
    cl_demofilename = cvarSystem->Get("cl_demofilename", "", CVAR_ROM,
                                      "This CVAR is Read Only, you can use it in-game though, basically if you have turned recording on in-game, you can use this CVAR to see what the name of the demo is in the console.");
    cl_demooffset = cvarSystem->Get("cl_demooffset", "0", CVAR_ROM,
                                    "This CVAR is Read Only, you can use it in-game though, basically if you have turned recording on in-game, you can use this CVAR to see what the current camera offset is in the console, it changes based on seconds, but you cant view its offset realtime, you have to continually type the CVAR in the console..");
    cl_waverecording = cvarSystem->Get("cl_waverecording", "0", CVAR_ROM,
                                       "This is not changeable, it just tells you when you type cl_waverecording in console if its recording or not, if it is it will say 1 if it isnt it will say 0.");
    cl_wavefilename = cvarSystem->Get("cl_wavefilename", "", CVAR_ROM,
                                      "If you type 1, i assume it shows the name of the current .wav audio file being recorded, EDIT: tested yes it does this.");
    cl_waveoffset = cvarSystem->Get("cl_waveoffset", "0", CVAR_ROM,
                                    "When 1, if you are recording a .wav audio file it just prints to the console information about the wave offset.");

    //bani
    cl_packetloss = cvarSystem->Get("cl_packetloss", "0", CVAR_CHEAT,
                                    "Mimic packet loss.");
    cl_packetdelay = cvarSystem->Get("cl_packetdelay", "0", CVAR_CHEAT,
                                     "Mimic packet dealy.");

    cvarSystem->Get("cl_maxPing", "800", CVAR_ARCHIVE,
                    "This will NOT show servers with a ping higher than the chosen setting in the server browser.");

    cl_guidServerUniq = cvarSystem->Get("cl_guidServerUniq", "1", CVAR_ARCHIVE,
                                        "Use a unique guid value per server.");

    // userinfo
    cvarSystem->Get("name", idsystem->GetCurrentUser(),
                    CVAR_USERINFO | CVAR_ARCHIVE, "Sets the name of the player");
    cvarSystem->Get("rate", "25000", CVAR_USERINFO | CVAR_ARCHIVE,
                    "Cap on the connection bandwidth to use, 1000=~1KB/s. For 56k use about 4000, broadband 25000");     // Dushan - changed from 5000
    cvarSystem->Get("snaps", "20", CVAR_USERINFO | CVAR_ARCHIVE,
                    "snapshots for server to send you, leave at 20.");
    cvarSystem->Get("cg_version", PRODUCT_NAME, CVAR_ROM | CVAR_USERINFO,
                    "Displays client game version.");
    cvarSystem->Get("password", "", CVAR_USERINFO,
                    "Used for setting password required for some servers");
    cvarSystem->Get("cg_predictItems", "1", CVAR_ARCHIVE,
                    "Toggle use of prediction for picking up items.");

#if 1
    cvarSystem->Get("p_hp", "", CVAR_ROM, "");
    cvarSystem->Get("p_team", "", CVAR_ROM, "");
    cvarSystem->Get("p_class", "", CVAR_ROM, "");
    cvarSystem->Get("p_credits", "", CVAR_ROM, "");
    cvarSystem->Get("p_attacker", "", CVAR_ROM, "");
    cvarSystem->Get("p_killed", "", CVAR_ROM, "");
    cvarSystem->Get("p_score", "", CVAR_ROM, "");
#endif

    // cgame might not be initialized before menu is used
    cvarSystem->Get("cg_viewsize", "100", CVAR_ARCHIVE,
                    "Supposed to be for setting the % of screen actually displaying rendered game. Might have been useful for using a lower-res ET while using a native resolution on TFT screens");

    cl_waitForFire = cvarSystem->Get("cl_waitForFire", "0", CVAR_ROM,
                                     "Wait for the fire.");

    cl_language = cvarSystem->Get("cl_language", "0", CVAR_ARCHIVE,
                                  "Stores the language. English is 0");
    cl_debugTranslation = cvarSystem->Get("cl_debugTranslation", "0", 0,
                                          "Debug translations");
    cl_updateavailable = cvarSystem->Get("cl_updateavailable", "0", CVAR_ROM,
                                         "Show if update is available");
    cl_updatefiles = cvarSystem->Get("cl_updatefiles", "", CVAR_ROM,
                                     "Sset when there is a download an update patch");

#ifndef BSPC
    cm_noAreas = cvarSystem->Get("cm_noAreas", "0", CVAR_CHEAT,
                                 "Toggle the ability of the player bounding box to clip through areas.");
    cm_noCurves = cvarSystem->Get("cm_noCurves", "0", CVAR_CHEAT,
                                  "Toggle the ability of the player bounding box to clip through curved surfaces.");
    cm_forceTriangles = cvarSystem->Get("cm_forceTriangles", "0",
                                        CVAR_CHEAT | CVAR_LATCH, "Convert all patches into triangles.");
    cm_playerCurveClip = cvarSystem->Get("cm_playerCurveClip", "1",
                                         CVAR_ARCHIVE | CVAR_CHEAT,
                                         "toggles the ability of the player bounding box to respect curved surfaces.");
    cm_optimize = cvarSystem->Get("cm_optimize", "1", CVAR_CHEAT,
                                  "Collision model optimization");
    cm_showCurves = cvarSystem->Get("cm_showCurves", "0", CVAR_CHEAT,
                                    "Showing curved surfaces");
    cm_showTriangles = cvarSystem->Get("cm_showTriangles", "0", CVAR_CHEAT,
                                       "Showing triangles in the surfaces");
#endif

    fs_debug = cvarSystem->Get("fs_debug", "0", 0,
                               "enables the display of file system messages to the console.");
    fs_copyfiles = cvarSystem->Get("fs_copyfiles", "0", CVAR_INIT,
                                   "Relic/obsolete.!");
    fs_basepath = cvarSystem->Get("fs_basepath",
                                  idsystem->DefaultInstallPath(), CVAR_INIT,
                                  "Holds the logical path to install folder.");
    fs_buildpath = cvarSystem->Get("fs_buildpath", "", CVAR_INIT,
                                   "Holds the build path to install folder");
    fs_buildgame = cvarSystem->Get("fs_buildgame", BASEGAME, CVAR_INIT,
                                   "Relic/obsolete.!");
    fs_basegame = cvarSystem->Get("fs_basegame", "", CVAR_INIT,
                                  "Allows people to base mods upon mods syntax to follow.");
    fs_libpath = cvarSystem->Get("fs_libpath", idsystem->DefaultLibPath(),
                                 CVAR_INIT, "Default binary directory.");
#if defined (__MACOSX__)
    fs_apppath = cvarSystem->Get("fs_apppath", idsystem->DefaultAppPath(),
                                 CVAR_INIT, "Default app directory.");
#endif

    fs_homepath = cvarSystem->Get("fs_homepath", "", CVAR_INIT,
                                  "The default is the path to the game executable.");
    fs_game = cvarSystem->Get("fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO,
                              "Set Game path. Set the game folder/dir. ");

    fs_texturesfolder = cvarSystem->Get("fs_texturesFolder", "", CVAR_INIT,
                                        "fs_texturesFolder 'main/pk3Dir' will load from main/pk3Dir folder so its independent from fs_game");
    fs_soundsfolder = cvarSystem->Get("fs_soundsFolder", "", CVAR_INIT,
                                      "fs_soundsfolder 'main/pk3Dir' will load from main/pk3Dir folder so its independent from fs_game");
    fs_modelsfolder = cvarSystem->Get("fs_modelsFolder", "", CVAR_INIT,
                                      "fs_modelsfolder 'main/pk3Dir' will load from main/pk3Dir folder so its independent from fs_game");
    fs_restrict = cvarSystem->Get("fs_restrict", "", CVAR_INIT,
                                  "Demoversion if set to 1 restricts game to some number of maps.");
    fs_missing = cvarSystem->Get("fs_missing", "", CVAR_INIT, "Missing files");


#ifdef DEDICATED
    // I want server owners to explicitly turn on ipv6 support.
    net_enabled = cvarSystem->Get("net_enabled", "1",
                                  CVAR_LATCH | CVAR_ARCHIVE,
                                  "Enable networking, bitmask. Add up number for option to enable it : enable ipv4 networking : 1 enable ipv6 networking : 2 prioritise ipv6 over ipv4 : 4 disable multicast support : 8");
#else
    /* End users have it enabled so they can connect to ipv6-only hosts, but ipv4 will be
     * used if available due to ping */
    net_enabled = cvarSystem->Get("net_enabled", "3",
                                  CVAR_LATCH | CVAR_ARCHIVE,
                                  "Enable networking, bitmask. Add up number for option to enable it : enable ipv4 networking : 1 enable ipv6 networking : 2 prioritise ipv6 over ipv4 : 4 disable multicast support : 8");
#endif

    net_ip = cvarSystem->Get("net_ip", "0.0.0.0", CVAR_LATCH,
                             "IPv4 address to bind to");

    net_ip6 = cvarSystem->Get("net_ip6", "::", CVAR_LATCH,
                              "IPv6 address to bind to");

    net_port = cvarSystem->Get("net_port", va("%i", PORT_SERVER), CVAR_LATCH,
                               "Port to bind to using the ipv4 address");

    net_port6 = cvarSystem->Get("net_port6", va("%i", PORT_SERVER), CVAR_LATCH,
                                "Port to bind to using the ipv6 address");

    // Some cvars for configuring multicast options which facilitates scanning for servers on local subnets.
    net_mcast6addr = cvarSystem->Get("net_mcast6addr", NET_MULTICAST_IP6,
                                     CVAR_LATCH | CVAR_ARCHIVE,
                                     "Multicast address to use for scanning for ipv6 servers on the local network");

#ifdef _WIN32
    net_mcast6iface = cvarSystem->Get("net_mcast6iface", "0",
                                      CVAR_LATCH | CVAR_ARCHIVE,
                                      "Enables the outgoing interface used for IPv6 multicast scanning on LAN.");
#else
    net_mcast6iface = cvarSystem->Get("net_mcast6iface", "",
                                      CVAR_LATCH | CVAR_ARCHIVE,
                                      "Enables the outgoing interface used for IPv6 multicast scanning on LAN.");
#endif

    net_socksEnabled = cvarSystem->Get("net_socksEnabled", "0",
                                       CVAR_LATCH | CVAR_ARCHIVE, "Enables socks 5 network protocol.");

    net_socksServer = cvarSystem->Get("net_socksServer", "",
                                      CVAR_LATCH | CVAR_ARCHIVE,
                                      "Sets the name or IP address of the socks server.");
    net_socksPort = cvarSystem->Get("net_socksPort", "1080",
                                    CVAR_LATCH | CVAR_ARCHIVE, "Sets proxy and firewall port.");
    net_socksUsername = cvarSystem->Get("net_socksUsername", "",
                                        CVAR_LATCH | CVAR_ARCHIVE,
                                        "Sets the username for socks firewall supports. It does not support GSS-API authentication.");
    net_socksPassword = cvarSystem->Get("net_socksPassword", "",
                                        CVAR_LATCH | CVAR_ARCHIVE,
                                        "Sets password for socks network/firewall access.");

    showpackets = cvarSystem->Get("showpackets", "0", CVAR_TEMP,
                                  "Toggles the running display of all packets sent and received. 0=disables;1=enables. ");
    showdrop = cvarSystem->Get("showdrop", "0", CVAR_TEMP,
                               "When enabled, reports dropped packets should they occur. 0=disables;1=enables. ");
    net_port = cvarSystem->Get("net_port", XSTRING(PORT_SERVER), CVAR_LATCH,
                               "");
    net_qport = cvarSystem->Get("net_qport", "0", CVAR_INIT, "");

    in_joystickNo = cvarSystem->Get("in_joystickNo", "0", CVAR_ARCHIVE,
                                    "Check whether a user has changed the joystick number");

    in_joystickUseAnalog = cvarSystem->Get("in_joystickUseAnalog", "0",
                                           CVAR_ARCHIVE,
                                           "Do not translate joystick axis events to keyboard commands");

    in_keyboardDebug = cvarSystem->Get("in_keyboardDebug", "0", CVAR_ARCHIVE,
                                       "Print keyboard debug info");

    // mouse variables
    in_mouse = cvarSystem->Get("in_mouse", "1", CVAR_ARCHIVE,
                               "Toggles polling the port used for mouse input. 0=disables;1=enables. ");
    in_nograb = cvarSystem->Get("in_nograb", "0", CVAR_ARCHIVE,
                                "Dont capture mouse in window mode");

    in_joystick = cvarSystem->Get("in_joystick", "0",
                                  CVAR_ARCHIVE | CVAR_LATCH,
                                  "Toggle the initialization of the joystick  (command line)");
    in_joystickDebug = cvarSystem->Get("in_joystickDebug", "0", CVAR_TEMP,
                                       "A debugging tool that joystick keypress input data to the console.");
    in_joystickThreshold = cvarSystem->Get("joy_threshold", "0.15",
                                           CVAR_ARCHIVE, "Sets joystick threshold sensitivity");

    if(!s_sdlBits) {
        s_sdlBits = cvarSystem->Get("s_sdlBits", "16", CVAR_ARCHIVE,
                                    "SDL sound bit resolution");
        s_sdlChannels = cvarSystem->Get("s_sdlChannels", "2", CVAR_ARCHIVE,
                                        "SDL sound number of channels");
        s_sdlDevSamps = cvarSystem->Get("s_sdlDevSamps", "0", CVAR_ARCHIVE,
                                        "SDL sound DMA buffer size override");
        s_sdlMixSamps = cvarSystem->Get("s_sdlMixSamps", "0", CVAR_ARCHIVE,
                                        "SDL sound mix buffer size override.");
    }

    journal = cvarSystem->Get("journal", "0", CVAR_INIT,
                              "Use in command line to record 'demo' of everything you do in application. '+set journal 1' to record; 2 for playback. journaldata.dat & journal.dat are the files it creates, they get very large quickly. Files will also store cfgs loaded.");
    com_highSettings = cvarSystem->Get("com_highSettings", "1", CVAR_ARCHIVE,
                                       "Setting high quality settings");
    com_recommended = cvarSystem->Get("com_recommended", "-1", CVAR_ARCHIVE,
                                      "Setting the recommended settings");

    com_ignorecrash = cvarSystem->Get("com_ignorecrash", "0", 0,
                                      "Tells the client override unsafe cvars that result from crash (often also from running modifications as ET didnt delete pid file)");

    // ydnar: init crashed variable as early as possible
    com_crashed = cvarSystem->Get("com_crashed", "0", CVAR_TEMP,
                                  "Force a error for development reasons but never really trialed this.");

    //
    // init commands and vars
    //

    logfile = cvarSystem->Get("logfile", "0", CVAR_TEMP,
                              "Toggles saving a logfile");

    // Gordon: no need to latch this in ET, our recoil is framerate independant
    //  com_blood = cvarSystem->Get ("com_blood", "1", CVAR_ARCHIVE, "Enable blood mist effects."); // Gordon: no longer used?

    timescale = cvarSystem->Get("timescale", "1",
                                CVAR_CHEAT | CVAR_SYSTEMINFO,
                                "Increase to fast forward through demos, or decrease for slow motion(fraction of 1).");
    fixedtime = cvarSystem->Get("fixedtime", "0", CVAR_CHEAT,
                                "Toggle the rendering of every frame. The game will wait until each frame is completely rendered before sending the next frame.");
    com_showtrace = cvarSystem->Get("com_showtrace", "0", CVAR_CHEAT,
                                    "Toggle display of packet traces. 0=disables,1=toggles.");
    com_dropsim = cvarSystem->Get("com_dropsim", "0", CVAR_CHEAT,
                                  "For testing simulates packet loss during communication drops");
    com_speeds = cvarSystem->Get("com_speeds", "0", 0,
                                 "Toggle display of frame counter, all, sv, cl, gm, rf, and bk whatever they are");
    com_timedemo = cvarSystem->Get("timedemo", "0", CVAR_CHEAT,
                                   "When set to 1 times a demo and returns frames per second like a benchmark");
    com_cameraMode = cvarSystem->Get("com_cameraMode", "0", CVAR_CHEAT,
                                     "Seems to toggle the view of your player model off and on when in 3D camera view ");

    com_watchdog = cvarSystem->Get("com_watchdog", "60", CVAR_ARCHIVE,
                                   "Watchdog checks for a map not being loaded");
    com_watchdog_cmd = cvarSystem->Get("com_watchdog_cmd", "", CVAR_ARCHIVE,
                                       "Sets what to do when watchdog finds a map is not loaded");

    cl_paused = cvarSystem->Get("cl_paused", "0", CVAR_ROM,
                                "Variable holds the status of the paused flag on the client side");
    sv_paused = cvarSystem->Get("sv_paused", "0", CVAR_ROM,
                                "Allow the game to be paused from the server console?");
    sv_running = cvarSystem->Get("sv_running", "0", CVAR_ROM,
                                 "Variable flag tells the console whether or not a local server is running");
    cl_running = cvarSystem->Get("cl_running", "0", CVAR_ROM,
                                 "Variable which shows whether or not a client game is running or whether we are in server/client mode (read only)");
    com_buildScript = cvarSystem->Get("com_buildScript", "0", 0,
                                      "Automated data building scripts");

    con_drawnotify = cvarSystem->Get("con_drawnotify", "0", CVAR_CHEAT,
                                     "Draws the last few lines of output transparently over the game top.");

    com_introPlayed = cvarSystem->Get("com_introplayed", "0", CVAR_ARCHIVE,
                                      "Whether or not the intro for the game has been played.");
    com_ansiColor = cvarSystem->Get("com_ansiColor", "0", CVAR_ARCHIVE,
                                    "Enable use of ANSI escape codes in the tty");

    com_unfocused = cvarSystem->Get("com_unfocused", "0", CVAR_ROM,
                                    "Automatically toggled when the game window is unfocused.");
    com_minimized = cvarSystem->Get("com_minimized", "0", CVAR_ROM,
                                    "Automatically toggled when the game window is minimized.");
    com_maxfpsUnfocused = cvarSystem->Get("com_maxfpsUnfocused", "0",
                                          CVAR_ARCHIVE, "Maximum frames per second when unfocused");
    com_maxfpsMinimized = cvarSystem->Get("com_maxfpsMinimized", "0",
                                          CVAR_ARCHIVE, "Maximum frames per second when minimized");
    com_abnormalExit = cvarSystem->Get("com_abnormalExit", "0", CVAR_ROM,
                                       "As Application crashed it will start with safe video settings");

    com_hunkused = cvarSystem->Get("com_hunkused", "0", 0,
                                   "Tells you the amound of hunk currently being used.");

#ifdef UPDATE_SERVER
    dedicated = cvarSystem->Get("dedicated", "1", CVAR_LATCH,
                                "Sets server type: 1 dedicated LAN, 2 dedicated internet, 0 listen (play & serve)");
#elif DEDICATED
    // TTimo: default to internet dedicated, not LAN dedicated
    dedicated = cvarSystem->Get("dedicated", "2", CVAR_ROM,
                                "Sets server type: 1 dedicated LAN, 2 dedicated internet, 0 listen (play & serve)");
    cvarSystem->CheckRange(dedicated, 1, 2, true);
#else
    dedicated = cvarSystem->Get("dedicated", "0", CVAR_LATCH,
                                "Sets server type: 1 dedicated LAN, 2 dedicated internet, 0 listen (play & serve)");
    cvarSystem->CheckRange(dedicated, 0, 2, true);
#endif

    // bani: init pid
#ifdef _WIN32
    pid = GetCurrentProcessId();
#else
    pid = getpid();
#endif
    s = va("%d", pid);
    com_pid = cvarSystem->Get("com_pid", s, CVAR_ROM, "Process id");

    s = va("%s %s %s %s", PRODUCT_NAME, OS_STRING, OS_STRING, __DATE__);
    com_version = cvarSystem->Get("version", s, CVAR_ROM | CVAR_SERVERINFO,
                                  "Records all info about the application version: build number, build date, win/linux etc");
    com_protocol = cvarSystem->Get("protocol", va("%i", PROTOCOL_VERSION),
                                   CVAR_SERVERINFO | CVAR_ARCHIVE,
                                   "Returns the current protocol (changes with patches).");

#ifndef DEDICATED
    con_autochat = cvarSystem->Get("con_autochat", "1", CVAR_ARCHIVE,
                                   "Set to 0 to disable sending console input text as chat when there is not a slash at the beginning.");
#endif
    // serverinfo vars
    cvarSystem->Get("dmflags", "0", /*CVAR_SERVERINFO */ 0,
                    "Sets the game options for deathmatch play.");
    cvarSystem->Get("fraglimit", "0", /*CVAR_SERVERINFO */ 0,
                    "Sets the number of frags required for the game to be won if timelimit has been not reached or set. Setting to 0 disables fraglimit. ");
    cvarSystem->Get("timelimit", "0", CVAR_SERVERINFO,
                    "Sets the amount of time before a game will end if fraglimit is not reached or set. Setting to 0 disables timelimit. ");

    cvarSystem->Get("sv_keywords", "", CVAR_SERVERINFO,
                    "Variable holds the search string entered in the internet connection menu");
    cvarSystem->Get("protocol", va("%i", PROTOCOL_VERSION),
                    CVAR_SERVERINFO | CVAR_ARCHIVE,
                    "Display network protocol version. Useful for backward compatibility with servers with otherwise incompatible versions.");
    sv_mapname = cvarSystem->Get("mapname", "nomap",
                                 CVAR_SERVERINFO | CVAR_ROM,
                                 "Display the name of the current map being used");
    sv_privateClients = cvarSystem->Get("sv_privateClients", "0",
                                        CVAR_SERVERINFO,
                                        "The number of spots, out of sv_maxclients, reserved for players with the server password (sv_privatePassword)");
    sv_hostname = cvarSystem->Get("sv_hostname", "OpenWolf Host",
                                  CVAR_SERVERINFO | CVAR_ARCHIVE,
                                  "The name of the server in server browsers.");
    //
    sv_maxclients = cvarSystem->Get("sv_maxclients", "20",
                                    CVAR_SERVERINFO | CVAR_LATCH,
                                    "Number of players allowed to connect wen running a server. 16 is a 'soft' limit—it is the maximum recommended. Many servers run 32 or more players. You could set this lower than 2, but then you only have one player on a server; better to use /devmap if you want to play alone.");   // NERVE - SMF - changed to 20 from 8

    sv_maxRate = cvarSystem->Get("sv_maxRate", "0",
                                 CVAR_ARCHIVE | CVAR_SERVERINFO,
                                 "Option to force all clients to play with a max rate. This can be used to limit the advantage of low pings, or to cap bandwidth utilization for a server. Note that rate is ignored for clients that are on the same LAN.");
    sv_minPing = cvarSystem->Get("sv_minPing", "0",
                                 CVAR_ARCHIVE | CVAR_SERVERINFO,
                                 "Set the minimum ping aloud on the server to keep low pings out");
    sv_maxPing = cvarSystem->Get("sv_maxPing", "0",
                                 CVAR_ARCHIVE | CVAR_SERVERINFO,
                                 "Set the maximum ping allowed on the server to keep high pings out");
    sv_dlRate = cvarSystem->Get("sv_dlRate", "100",
                                CVAR_ARCHIVE | CVAR_SERVERINFO,
                                "Bandwidth allotted to PK3 file downloads via UDP, in kbyte / s");
    sv_floodProtect = cvarSystem->Get("sv_floodProtect", "1",
                                      CVAR_ARCHIVE | CVAR_SERVERINFO,
                                      "Whether or not to use flood protection, preventing clients from sending numerous consecutive commands to the server.");
    sv_allowAnonymous = cvarSystem->Get("sv_allowAnonymous", "0",
                                        CVAR_SERVERINFO, "Allow anonymous connections in the server");
    sv_friendlyFire = cvarSystem->Get("g_friendlyFire", "1",
                                      CVAR_SERVERINFO | CVAR_ARCHIVE,
                                      "Toggles wether players can damage their teammates");  // NERVE - SMF
    sv_maxlives = cvarSystem->Get("g_maxlives", "0",
                                  CVAR_ARCHIVE | CVAR_LATCH | CVAR_SERVERINFO,
                                  "Number of lives (respawns) all players have. 0 = unlimited");    // NERVE - SMF
    sv_needpass = cvarSystem->Get("g_needpass", "0",
                                  CVAR_SERVERINFO | CVAR_ROM,
                                  "Toggles requiring a password for players to join");

    // systeminfo
    //bani - added convar_t for sv_cheats so server engine can reference it
    sv_cheats = cvarSystem->Get("sv_cheats", "0", CVAR_SYSTEMINFO | CVAR_ROM,
                                "Enable cheats (serverside only)");
    sv_serverid = cvarSystem->Get("sv_serverid", "0",
                                  CVAR_SYSTEMINFO | CVAR_ROM,
                                  "The identification number of the local server. ");
    sv_pure = cvarSystem->Get("sv_pure", "1", CVAR_SYSTEMINFO,
                              "Toggles check that client's files are the same as the servers (basic anticheat).");

    cvarSystem->Get("sv_paks", "", CVAR_SYSTEMINFO | CVAR_ROM,
                    "Variable holds the checksum of all pk3 files");
    cvarSystem->Get("sv_pakNames", "", CVAR_SYSTEMINFO | CVAR_ROM,
                    "Variable holds a list of all the pk3 files the server found");
    cvarSystem->Get("sv_referencedPaks", "", CVAR_SYSTEMINFO | CVAR_ROM,
                    "Variable holds the checksum of the referenced pk3 files");
    cvarSystem->Get("sv_referencedPakNames", "", CVAR_SYSTEMINFO | CVAR_ROM,
                    "Variable holds a list of all the pk3 files the server loaded data from. these pk3 files will be autodownloaded by a client if the client does not have them.");

    // server vars
    sv_rconPassword = cvarSystem->Get("rconPassword", "", CVAR_TEMP,
                                      "RCon password, for server admins to use Remote Console commands.");
    sv_privatePassword = cvarSystem->Get("sv_privatePassword", "", CVAR_TEMP,
                                         "Set password for private clients to login with");
    sv_fps = cvarSystem->Get("sv_fps", "20", CVAR_TEMP,
                             "Set the max frames per second the server sends the client");
    sv_timeout = cvarSystem->Get("sv_timeout", "240", CVAR_TEMP,
                                 "Sets the amount of time for the server to wait for a client packet before assuming a disconnected state.");
    sv_zombietime = cvarSystem->Get("sv_zombietime", "2", CVAR_TEMP,
                                    "The amount of time in minutes before a frozen character is removed from the map.");
    cvarSystem->Get("nextmap", "", CVAR_TEMP,
                    "Start the next map in the rotation");

    sv_allowDownload = cvarSystem->Get("sv_allowDownload", "1", CVAR_ARCHIVE,
                                       "Whether the server will allow data to be downloaded from it (default 0) .");

    sv_master[0] = cvarSystem->Get("sv_master1", MASTER_SERVER_NAME, 0,
                                   "Set URL or address to master server");

    sv_reconnectlimit = cvarSystem->Get("sv_reconnectlimit", "3", 0,
                                        "Number of times a disconnected client can come back and reconnect");
    sv_tempbanmessage = cvarSystem->Get("sv_tempbanmessage",
                                        "You have been kicked and are temporarily banned from joining this server.",
                                        0, "What player's get after being kicked - “you have been kicked and are temporarily banned...");
    sv_showloss = cvarSystem->Get("sv_showloss", "0", 0,
                                  "Toggle sever packet loss display");
    sv_padPackets = cvarSystem->Get("sv_padPackets", "0", 0,
                                    "Toggles the padding of network packets on the server PAD - Packet Assembler/Disassembler");
    sv_killserver = cvarSystem->Get("sv_killserver", "0", 0,
                                    "When set to 1, kills the server (shuts down map or demo) without shutting down the application. Value then returns to 0.");
    sv_mapChecksum = cvarSystem->Get("sv_mapChecksum", "", CVAR_ROM,
                                     "Reports the file size of the currently loaded map. Used to prevent cheating by ensuring all clients are not using hacked maps.");

    sv_lanForceRate = cvarSystem->Get("sv_lanForceRate", "1", CVAR_ARCHIVE,
                                      "Toggle for forcing very high rate setting for clients detected (sometimes wrongly) as connecting via LAN.");

    sv_onlyVisibleClients = cvarSystem->Get("sv_onlyVisibleClients", "0", 0,
                                            "Showing only visible clients while generating a new challenge");  // DHM - Nerve

    sv_showAverageBPS = cvarSystem->Get("sv_showAverageBPS", "0", 0,
                                        "BSP Network debugging");  // NERVE - SMF - net debugging

    sv_cs_ServerType = cvarSystem->Get("sv_cs_ServerType", "0", 0,
                                       "Setup server type for the community server. 0: public, 1: public-registered, 2: private.");
    sv_cs_Salt = cvarSystem->Get("sv_cs_Salt", "12345", 0,
                                 "Community server hash password field.");
    sv_cs_BotLog = cvarSystem->Get("sv_cs_BotLog", "0", 0,
                                   "Log bots in the community server.");
    sv_cs_MemberColor = cvarSystem->Get("sv_cs_MemberColor", "0 255 0", 0,
                                        "Color of the registered user in the community server.");
    sv_cs_UnknownColor = cvarSystem->Get("sv_cs_UnknownColor", "255 0 0", 0,
                                         "Color of the unknown player in the community server.");
    sv_cs_PrivateOnlyMSG = cvarSystem->Get("sv_cs_PrivateOnlyMSG",
                                           "This server is for registered users only. Register at " PRODUCT_NAME
                                           ".com", 0, "Send private message only to the registered user.");
    sv_cs_stats = cvarSystem->Get("sv_cs_stats", COMMUNITY_SERVER_NAME, 0,
                                  "Enter the address of the community server. Example: xxx.xxx.xxx.xxx:port");
    sv_cs_ServerPort = cvarSystem->Get("sv_cs_ServerPort", va("%i",
                                       PORT_COMMUNITY), 0,
                                       "Enter the port address of the community server.");

    g_gameType = cvarSystem->Get("g_gametype", va("%i",
                                 com_gameInfo.defaultGameType), CVAR_SERVERINFO | CVAR_LATCH,
                                 "Sets the type of game being played, 2=objective, 3=stopwatch, 4=campaign, 5=LMS");

#if !defined (UPDATE_SERVER)
    // the download netcode tops at 18/20 kb/s, no need to make you think you can go above
    sv_dl_maxRate = cvarSystem->Get("sv_dl_maxRate", "42000", CVAR_ARCHIVE,
                                    "Sets the maximum speed clients can download files from the server");
#else
    // the update server is on steroids, sv_fps 60 and no snapshotMsec limitation, it can go up to 30 kb/s
    sv_dl_maxRate = cvarSystem->Get("sv_dl_maxRate", "60000", CVAR_ARCHIVE,
                                    "Sets the maximum speed clients can download files from the server");
#endif

    sv_minimumAgeGuid = cvarSystem->Get("sv_minimumAgeGuid", "0",
                                        CVAR_ARCHIVE | CVAR_SERVERINFO, "Min guid age to enter a server");
    sv_maximumAgeGuid = cvarSystem->Get("sv_maximumAgeGuid", "0",
                                        CVAR_ARCHIVE | CVAR_SERVERINFO, "Max guid age to enter a server");

    sv_wwwDownload = cvarSystem->Get("sv_wwwDownload", "0", CVAR_ARCHIVE,
                                     "Toggles enabling www download redirect");
    sv_wwwBaseURL = cvarSystem->Get("sv_wwwBaseURL", "", CVAR_ARCHIVE,
                                    "Sets the location of www download redirect");
    sv_wwwDlDisconnected = cvarSystem->Get("sv_wwwDlDisconnected", "0",
                                           CVAR_ARCHIVE,
                                           "Wether to disconnect players from gameserver while they download via www");
    sv_wwwFallbackURL = cvarSystem->Get("sv_wwwFallbackURL", "", CVAR_ARCHIVE,
                                        "Alternative URL to download the files from");

    //bani
    sv_packetloss = cvarSystem->Get("sv_packetloss", "0", CVAR_CHEAT,
                                    "Mimic packet loss.");
    sv_packetdelay = cvarSystem->Get("sv_packetdelay", "0", CVAR_CHEAT,
                                     "Mimic packet dealy.");

    // fretn - note: redirecting of clients to other servers relies on this,
    // ET://someserver.com
    sv_fullmsg = cvarSystem->Get("sv_fullmsg", "Server is full.", CVAR_ARCHIVE,
                                 "Customise the server full message, or redirect to another server : sv_fullmsg OW://host.to.redirect.to:port");

    sv_hibernateTime = cvarSystem->Get("sv_hibernateTime", "0", CVAR_ARCHIVE,
                                       "Switches the server to a hibernation mode in which it uses less CPU power when no player is connected. The value is the time in milliseconds after which it automatically switches to the said state when the last player disconnected from the server. The value zero disables hibernation mode.");
    svs.hibernation.sv_fps = sv_fps->value;

    // oacs extended recording variables
    sv_oacsEnable = cvarSystem->Get("sv_oacsEnable", "0", CVAR_ARCHIVE,
                                    "Enable the extended logging facility");
    sv_oacsPlayersTableEnable = cvarSystem->Get("sv_oacsPlayersTableEnable",
                                "1", CVAR_ARCHIVE, "Enable the extended player identification logging");
    sv_oacsTypesFile = cvarSystem->Get("sv_oacsTypesFile", "oacs/types.txt",
                                       CVAR_ARCHIVE, "Where to save the features types");
    sv_oacsDataFile = cvarSystem->Get("sv_oacsDataFile", "oacs/data.txt",
                                      CVAR_ARCHIVE, "Where to save the features data");
    sv_oacsPlayersTable = cvarSystem->Get("sv_oacsPlayersTable",
                                          "oacs/playerstable.txt", CVAR_ARCHIVE,
                                          "Where to save the players table (if enabled)");
    sv_oacsMinPlayers = cvarSystem->Get("sv_oacsMinPlayers", "1", CVAR_ARCHIVE,
                                        "Minimum number of human players required to begin logging data");
    sv_oacsLabelPassword = cvarSystem->Get("sv_oacsLabelPassword", "",
                                           CVAR_TEMP, "Password necessary for a player to label himself");
    sv_oacsMaxPing = cvarSystem->Get("sv_oacsMaxPing", "700", CVAR_ARCHIVE,
                                     "Max ping to accept interframes (above, the interframe will be dropped until the ping goes down)");
    sv_oacsMaxLastPacketTime = cvarSystem->Get("sv_oacsMaxLastPacketTime",
                               "10000", CVAR_ARCHIVE,
                               "Max last packet time to accept interframes (above, the interframe will be dropped until the LastPacketTime goes down)");

    sv_wh_active = cvarSystem->Get("sv_wh_active", "0", CVAR_ARCHIVE,
                                   "Enable wallhack protection on the server.");
    sv_wh_bbox_horz = cvarSystem->Get("sv_wh_bbox_horz", "60", CVAR_ARCHIVE,
                                      "These is the horizontal dimension (in Quake units) of the players' bounding boxes used for performing line-of-sight traces.");

    if(sv_wh_bbox_horz->integer < 20) {
        cvarSystem->Set("sv_wh_bbox_horz", "10");
    }

    if(sv_wh_bbox_horz->integer > 50) {
        cvarSystem->Set("sv_wh_bbox_horz", "50");
    }

    sv_wh_bbox_vert = cvarSystem->Get("sv_wh_bbox_vert", "60", CVAR_ARCHIVE,
                                      "These is the vertical dimension (in Quake units) of the players' bounding boxes used for performing line-of-sight traces");

    if(sv_wh_bbox_vert->integer < 10) {
        cvarSystem->Set("sv_wh_bbox_vert", "30");
    }

    if(sv_wh_bbox_vert->integer > 50) {
        cvarSystem->Set("sv_wh_bbox_vert", "80");
    }

    sv_wh_check_fov = cvarSystem->Get("sv_wh_check_fov", "0", CVAR_ARCHIVE,
                                      "Enable wallhack protection only when players are in their respective FOV.");

    sv_autoRecDemo = cvarSystem->Get("sv_autoRecDemo", "0", CVAR_ARCHIVE,
                                     "Toggle (default off) auto demo recording of human players.");
    sv_autoRecDemoBots = cvarSystem->Get("sv_autoRecDemoBots", "0",
                                         CVAR_ARCHIVE, "If above is turned on, also record bots.");
    sv_autoRecDemoMaxMaps = cvarSystem->Get("sv_autoRecDemoMaxMaps", "0",
                                            CVAR_ARCHIVE,
                                            " Adjust how many maps, demos will be kept (default 0, probably should set if turning auto record on).");
    s_volume = cvarSystem->Get("s_volume", "0.8", CVAR_ARCHIVE,
                               "Sets volume of the game sounds, multiplier value (0.0 to 1.0)");
    s_musicVolume = cvarSystem->Get("s_musicvolume", "0.25", CVAR_ARCHIVE,
                                    "Sets volume of the music, multiplier value (0.0 to 1.0)");
    s_separation = cvarSystem->Get("s_separation", "0.5", CVAR_ARCHIVE,
                                   "Set separation between left and right sound channels (this one is it.)");
    s_doppler = cvarSystem->Get("s_doppler", "1", CVAR_ARCHIVE,
                                "Toggle doppler effect");
    s_khz = cvarSystem->Get("s_khz", "22", CVAR_ARCHIVE,
                            "Set the sampling frequency of sounds lower=performance higher=quality");
    s_mixahead = cvarSystem->Get("s_mixahead", "0.2", CVAR_ARCHIVE,
                                 "Set delay before mixing sound samples.");

    s_mixPreStep = cvarSystem->Get("s_mixPreStep", "0.05", CVAR_ARCHIVE,
                                   "Set the prefetching of sound on sound cards that have that power");
    s_show = cvarSystem->Get("s_show", "0", CVAR_CHEAT,
                             "Toggle display of paths and filenames of all sound files as they are played.");
    s_testsound = cvarSystem->Get("s_testsound", "0", CVAR_CHEAT,
                                  "Toggle a test tone to test sound system. 0=disables,1=toggles.");

    s_module = cvarSystem->Get("s_module", "AL", CVAR_ARCHIVE,
                               "Name of the sound system module.");

    savegame_loading = cvarSystem->Get("savegame_loading", "", CVAR_ARCHIVE,
                                       "Loading saved game");

    s_initsound = cvarSystem->Get("s_initsound", "1", CVAR_ARCHIVE,
                                  "Toggle weather sound is initialized or not (on next game)");

    hunk_soundadjust = cvarSystem->Get("hunk_soundadjust", "", CVAR_ARCHIVE,
                                       "");
    g_reloading = cvarSystem->Get("g_reloading", "", CVAR_ARCHIVE, "");

    r_fullscreen = cvarSystem->Get("r_fullscreen", "0",
                                   CVAR_ARCHIVE | CVAR_LATCH, "Enables fullscreen view");

    cl_logChat = cvarSystem->Get("cl_logChat", "0", CVAR_ARCHIVE,
                                 "Enables console logging in the file.");
}
