////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2010 Aldo Luis Aguirre
// Copyright(C) 2011 - 2022 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of OpenWolf.
//
// OpenWolf is free software; you can redistribute it
// and / or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3 of the License,
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
// File name:   serverCommunity.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SERVERCOMMUNITY_HPP__
#define __SERVERCOMMUNITY_HPP__

#define HASH_USER_SIZE 512
#define HASH_CLAN_SIZE 20
#define HASH_BAN_SIZE 512
#define SV_COMMUNITY_REGTYPE 0
#define SV_COMMUNITY_REGDATA 1
#define MATCH_MAX_CLANS 16
#define MATCH_MAX_USERS 64

typedef struct {
    valueType guid[33];
    valueType name[USER_NAME_SIZE + 1];
    sint until;
    valueType type;
} banuser_t;

#define MSG_MAX_DATA 100

static time_t  file_user_last_modified = 0;
static hash_table_t *hash_users = nullptr;
static hash_table_t *hash_clans = nullptr;
static hash_table_t *hash_bans = nullptr;

static FILE *usersfile = nullptr;
static FILE *bansfile = nullptr;
static sint match_in_progress = 0;

static valueType match_clans[MATCH_MAX_CLANS][CLAN_NAME_SIZE + 1];
static valueType match_users[MATCH_MAX_USERS][USER_NAME_SIZE + 1];
static valueType match_referees[MATCH_MAX_USERS][USER_NAME_SIZE + 1];

static community_stats_t community_stats = {0};
static sint stats_client_num = 0;
static valueType sv_community_last_match_s[1024 * 32] = {0};
static FILE *fcbbackup = nullptr;

//
// idServerCommunityServer
//
class idServerCommunityServer {
public:
    idServerCommunityServer();
    ~idServerCommunityServer();

    static void StartUp(void);
    static sint Login(valueType *userinfo, user_t **user_ret);
    static sint alreadyLogin(valueType *username);
    static sint checkMatchInvited(user_t *user);
    static void LoadUserFile(void);
    static valueType **fastParseLine(void *userline);
    static user_t *parseUserLine(pointer user_pass);
    static sint CheckUserFileChange(void);
    static sint copyFile(valueType *src, valueType *dst);
    static void destroyLongData(void *data);
    static void UserInfo(valueType *name);
    static void destroyClanData(void *data);
    static void destroyUserData(user_t *user);
    static void userInfoChanged(client_t *cl);
    static void logString(sint client_num, valueType *area,
                          valueType *message);
    static void startMatch(void);
    static void stopMatch(void);
    static void addMatchClan(valueType *clan_name);
    static void addMatchUser(valueType *user_name);
    static void addMatchReferee(valueType *user_name);
    static void matchInfo(void);
    static void LoadBanFile(void);
    static banuser_t *processBanLine(valueType *line);
    static void BanUser(client_t *cl);
    static sint checkBanGUID(valueType *guid);
    static void showBanUsers(void);
    static void unbanUser(valueType *banlist_num);
    static sint checkClientCommandPermitions(client_t *cl, pointer client_cmd);
    static sint StatsLoop(void);
    static void PlayerGameInfo(sint player);
    static sint ProcessServerCmd(valueType *message);
    static void ParseStatistic(valueType *line);
    static sint GetDescValue(valueType *line, valueType *desc, valueType *val);
    static void ProcessSplit(valueType **tokens);
    static user_stats_t *GetPlayerStats(valueType *player_name);
    static sint ProcessStats(user_stats_t *user_stats, valueType **tokens);
    static void ProcessWeapon(user_stats_t *user_stats, valueType *weapon,
                              valueType **tokens);
    static void ProcessMELEE(user_stats_t *user_stats, valueType **tokens);
    static void ProcessMISC(user_stats_t *user_stats, valueType **tokens);
    static void ProcessGeneric(pointer *l_stats, valueType **tokens,
                               sint *i_stats);
    static void SaveStatistics(void);
    static void SaveUserStats(user_stats_t *user_stats, FILE *fstats);
    static sint SaveUserStatsOptimal(user_stats_t *user_stats,
                                     valueType *fline);
    static sint PrintStat(valueType *out, sint count_stats,
                          pointer section_mnemonic, const sint *stats, pointer *stats_mnemonics);
    static void InitStatistics(void);
    static void PlayerServerInfo(sint clnum);
    static void ServerInfo(void);
    static void GetCPUProcess(void);
    static void GetMEMProcess(void);
    static void LoadMsgFile(void);
    static void NET_Loop(void);
    static void NETAddMsg(valueType *msg);
    static sint NETSendMsg(valueType *msg);
    static sint SendAll(sint s, valueType *buf, sint *len);
    static uint JenkinsHashKey_f(void *vkey);
    static sint StrCmp_f(const void *a1, const void *a2);
    static void DestroyStringKey_f(void *s);
};

extern idServerCommunityServer serverCommunityServer;

#endif //!__SERVERCOMMUNITY_HPP__