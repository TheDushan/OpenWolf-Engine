////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2010 Aldo Luis Aguirre
// Copyright(C) 2011 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   serverCommunity.cpp
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

idServerCommunityServer serverCommunityServer;

/*
===============
idServerCommunityServer::idServerCommunityServer
===============
*/
idServerCommunityServer::idServerCommunityServer(void) {
}

/*
===============
idServerCommunityServer::~idServerCommunityServer
===============
*/
idServerCommunityServer::~idServerCommunityServer(void) {
}

/*
===============
idServerCommunityServer::StartUp
===============
*/
void idServerCommunityServer::StartUp(void) {
    Com_Printf("Loading Community Server\n");

    LoadUserFile();
    LoadBanFile();
    LoadMsgFile();
}

/*
===============
idServerCommunityServer::StartUp
===============
*/
sint idServerCommunityServer::Login(valueType *userinfo,
                                    user_t **user_ret) {
    valueType *password = nullptr, * username = nullptr,
               * userpassword = nullptr, buffer[512], *use_private_slot;
    netadr_t from;
    user_t *user;
    user_stats_t *user_stats;

    // Check for file modifications
    CheckUserFileChange();

    networkSystem->StringToAdr(Info_ValueForKey(userinfo, "ip"), &from,
                               NA_UNSPEC);
    password = Info_ValueForKey(userinfo, "password");

    // If empty
    if(::strcmp(password, "") == 0) {
        *user_ret = nullptr;

        if(sv_cs_ServerType->integer >= 2) {
            networkChainSystem->OutOfBandPrint(NS_SERVER, from, "print\n%s",
                                               sv_cs_PrivateOnlyMSG->string);
            *user_ret = nullptr;
            return CS_ERROR;
        } else if(match_in_progress == 1) {
            networkChainSystem->OutOfBandPrint(NS_SERVER, from,
                                               "print\nThere is a match in progress.  You cannot right now.\n");
            *user_ret = nullptr;
            return CS_ERROR;
        }

        ::strncpy(buffer, Info_ValueForKey(userinfo, "cl_guid"), 32);

        buffer[32] = '\0';
        user_stats = GetPlayerStats(buffer);
        (user_stats->login)++;

        return CS_OK;
    }

    Q_strcpy_s(buffer, password);

    username = strtok(buffer, "@#\r\n\0");

    if(username != nullptr) {
        userpassword = strtok(nullptr, "\r\n\0");

        if(userpassword == nullptr) {
            networkChainSystem->OutOfBandPrint(NS_SERVER, from,
                                               "print\nPlease, check your user and password\n");
            *user_ret = nullptr;
            return CS_ERROR;
        }
    } else {
        networkChainSystem->OutOfBandPrint(NS_SERVER, from,
                                           "print\nPlease, check your user and password\n");
        *user_ret = nullptr;
        return CS_ERROR;
    }

    if(alreadyLogin(username) == 1) {
        networkChainSystem->OutOfBandPrint(NS_SERVER, from,
                                           "print\nYour username is already used! This situation was reported to admins.\n");
        Com_Printf("User %s was already logged in\n", username);
        *user_ret = nullptr;
        return CS_ERROR;

    }

    // The # character instead of @ is the mark to ask for a private slot
    use_private_slot = ::strchr(password, '#');

    user = parseUserLine(password);

    if(user == nullptr) {
        networkChainSystem->OutOfBandPrint(NS_SERVER, from,
                                           "print\nPlease, check your user and password\n");
        *user_ret = nullptr;
        return CS_ERROR;
    }

    // Check password
    if(::strcmp(user->password, Com_GetTigerHash(userpassword)) != 0) {
        networkChainSystem->OutOfBandPrint(NS_SERVER, from,
                                           "print\nPlease, check your user and password: invalid\n");
        destroyUserData(user);
        *user_ret = nullptr;
        return CS_ERROR;
    }

    // If has anough permitions to use private slot, copy private password to user password
    if(use_private_slot != nullptr && user != nullptr &&
            (user->type == CS_OWNER || user->type == CS_ADMIN)) {
        Info_SetValueForKey(userinfo, "password", sv_privatePassword->string);
    } else {
        Info_SetValueForKey(userinfo, "password", "");
    }

    if(user->type == CS_BANNED) {
        networkChainSystem->OutOfBandPrint(NS_SERVER, from,
                                           "print\nYou are not allowed to enter to this server\n");

        destroyUserData(user);
        *user_ret = nullptr;

        return CS_ERROR;
    }

    // Check if exist a match in progress and check if is invited
    if(match_in_progress == 1) {
        if(checkMatchInvited(user) != CS_OK) {
            networkChainSystem->OutOfBandPrint(NS_SERVER, from,
                                               "print\nThere is a match in progress. You cannot right now.\n");

            destroyUserData(user);
            *user_ret = nullptr;

            return CS_ERROR;
        }
    }

    Info_SetValueForKey(userinfo, "cl_guid", user->uguid);
    *user_ret = user;

    user_stats = GetPlayerStats(user->name);
    (user_stats->login)++;

    return CS_OK;
}

/*
===============
idServerCommunityServer::alreadyLogin
===============
*/
sint idServerCommunityServer::alreadyLogin(valueType *username) {
    sint i;

    for(i = 0; i < sv_maxclients->integer; i++) {
        if(svs.clients[i].state >= CS_ACTIVE &&
                svs.clients[i].cs_user != nullptr) {
            if(::strcmp(svs.clients[i].cs_user->name, username) == 0) {
                return 1;
            }
        }
    }

    return 0;
}

/*
===============
idServerCommunityServer::checkMatchInvited
===============
*/
sint idServerCommunityServer::checkMatchInvited(user_t *user) {
    sint i;
    user_clan_t *user_clan;

    if(match_in_progress != 1) {
        Com_Printf("You cannot call this function if there are not a match in progress...");
        return CS_ERROR;
    }

    // Admins and owners always may enter to game
    if(user->type == CS_ADMIN || user->type == CS_OWNER) {
        return CS_OK;
    }

    for(i = 0; i < MATCH_MAX_USERS && ::strlen(match_referees[i]) != 0 &&
            ::strcmp(user->name, match_referees[i]) != 0; i++);

    if(::strcmp(user->name, match_referees[i]) == 0) {
        user->type = CS_ADMIN;
        return CS_OK;
    }

    for(i = 0; i < MATCH_MAX_USERS && ::strlen(match_users[i]) != 0 &&
            ::strcmp(user->name, match_users[i]) != 0; i++);

    if(::strcmp(user->name, match_users[i]) == 0) {
        return CS_OK;
    }

    for(user_clan = user->clans; user_clan != nullptr;
            user_clan = user_clan->next) {
        for(i = 0; i < MATCH_MAX_CLANS && ::strlen(match_clans[i]) != 0 &&
                ::strcmp(user_clan->clan->name, match_clans[i]) != 0; i++);

        if(::strcmp(user_clan->clan->name, match_clans[i]) == 0) {
            return CS_OK;
        }
    }

    return CS_ERROR;
}

/*
===============
idServerCommunityServer::LoadUserFile
===============
*/
void idServerCommunityServer::LoadUserFile(void) {
    sint32 pos, * dpos;
    valueType **reg, *filename, buffer[512], * key;
    clan_t *clan;
    hash_table_t *hash_tmp = nullptr;
    struct stat st;
    void *data;

    if(hash_users != nullptr || hash_clans != nullptr ||
            usersfile != nullptr) {
        Com_Printf("YOU CANNOT RELOAD FILE UNTIL YOU RESET HASH TABLES AND FILE DESCRIPTORS!!!\n");
        return;
    }

    hash_users = Com_CreateHashTable(Com_JenkinsHashKey, Com_StrCmp,
                                     Com_DestroyStringKey, destroyLongData, HASH_USER_SIZE);
    hash_clans = Com_CreateHashTable(Com_JenkinsHashKey, Com_StrCmp,
                                     Com_DestroyStringKey, destroyClanData, HASH_CLAN_SIZE);

    /*** Open users file ***/
    filename = fileSystem->GetFullGamePath("cs_users.txt");

    usersfile = ::fopen(filename, "rt");

    if(usersfile == nullptr) {
        usersfile = ::fopen(filename, "wt+");
        Com_Printf("File cs_users.txt does not exist... creating it.\n");

        if(usersfile == nullptr) {
            Com_Printf("Error trying to open cs_users.txt\n");
            return;
        }
    }

    stat(filename, &st);

    file_user_last_modified = st.st_mtime;

    /* Start Processing */
    pos = ::ftell(usersfile);
    ::fgets(buffer, sizeof(buffer), usersfile);

    while(!::feof(usersfile)) {
        reg = fastParseLine(buffer);

        if(reg != nullptr) {
            key = static_cast< valueType * >(::malloc(strlen(reg[SV_COMMUNITY_REGDATA])
                                             + 1));
            ::strcpy(key, reg[SV_COMMUNITY_REGDATA]);

            dpos = static_cast<sint32 *>(::malloc(sizeof(sint32)));
            *dpos = pos;

            switch(reg[SV_COMMUNITY_REGTYPE][0]) {
                case 'u':
                    hash_tmp = hash_users;
                    data = dpos;
                    break;

                case 'c':
                    hash_tmp = hash_clans;
                    clan = (clan_t *)::malloc(sizeof(clan_t));
                    ::memset(clan, 0, sizeof(clan_t));
                    ::strncpy(clan->name, reg[SV_COMMUNITY_REGDATA], sizeof(clan->name));
                    data = clan;
                    break;

                default:
                    Com_Printf("Error while parsing userline: check linetype: %s\n",
                               reg[SV_COMMUNITY_REGTYPE]);
                    hash_tmp = nullptr;
            }

            if(hash_tmp != nullptr && Com_FindHashData(hash_tmp, key) == nullptr) {
                Com_InsertIntoHash(hash_tmp, key, data);
            } else {
                Com_Printf("Error: %s:%s already exist!\n", reg[SV_COMMUNITY_REGTYPE],
                           key);
                ::free(key);
                ::free(dpos);
            }
        }

        pos = ::ftell(usersfile);
        ::fgets(buffer, sizeof(buffer), usersfile);
    }
}

/*
===============
idServerCommunityServer::fastParseLine
===============
*/
valueType **idServerCommunityServer::fastParseLine(void *userline) {
    static valueType *reg[2];

    ::memset(reg, 0, sizeof(reg));

    reg[SV_COMMUNITY_REGTYPE] = ::strtok(static_cast<valueType *>(userline),
                                         ":");

    if(reg[SV_COMMUNITY_REGTYPE] == nullptr ||
            ::strlen(reg[SV_COMMUNITY_REGTYPE]) > 1) {
        Com_Printf("Error while parsing userline: check type field\n");
        return nullptr;
    }

    reg[SV_COMMUNITY_REGDATA] = ::strtok(nullptr, ":\n\r\0");

    if(reg[SV_COMMUNITY_REGDATA] == nullptr ||
            ::strlen(reg[SV_COMMUNITY_REGDATA]) > USER_NAME_SIZE) {
        Com_Printf("Error while parsing userline: check username field\n");
        return nullptr;
    }

    return reg;
}

/*
===============
idServerCommunityServer::parseUserLine

username:t:tiger_hash_password:clan1@clan2@clan3\n
where t may be 'R', 'A' or 'B'
===============
*/
user_t *idServerCommunityServer::parseUserLine(pointer user_pass) {
    sint32 *dpos;
    valueType *username, * type, * tigerhash, * clan, userline[1024],
              buffer[512], client_user_pass[1024];
    user_t *user;
    user_clan_t *user_clan;
    clan_t *clan_p;

    Q_strcpy_s(client_user_pass, user_pass);

    username = ::strtok(client_user_pass, "@#");

    if(username == nullptr) {
        return nullptr;
    }

    if((dpos = static_cast<sint32 *>(Com_FindHashData(hash_users,
                                     username))) == nullptr) {
        Com_Printf("User %s does not exist in DB\n", username);
        return nullptr;
    }

    ::fseek(usersfile, *dpos, SEEK_SET);

    ::fgets(userline, sizeof(userline), usersfile);

    // First characters are c: (clan) or u: (user)
    username = ::strtok(&(userline[2]), ":\n\0");

    if(username == nullptr || strlen(username) > USER_NAME_SIZE) {
        Com_Printf("Error while parsing userline: check username field\n");
        return nullptr;
    }

    type = ::strtok(nullptr, ":\n\0");

    if(type == nullptr || strlen(type) != 1) {
        Com_Printf("Error while parsing userline: check user type lenght\n");
        return nullptr;
    } else if(type[0] != 'A' && type[0] != 'M' && type[0] != 'O' &&
              type[0] != 'B') {
        Com_Printf("Error while parsing userline: check user type value\n");
        return nullptr;
    }

    tigerhash = ::strtok(nullptr, "@\n\r\0");

    if(tigerhash == nullptr) {
        Com_Printf("Error while parsing userline: check tiger hash password field\n");
        return nullptr;
    } else if(::strlen(tigerhash) != 48) {
        Com_Printf("Error in tiger hash password lenght, check field\n");
        return nullptr;
    }

    user = (user_t *)::malloc(sizeof(user_t));
    ::memset(user, 0, sizeof(user_t));

    ::strncpy(user->name, username, sizeof(user->name) - 1);
    ::strncpy(user->password, tigerhash, sizeof(user->password) - 1);
    ::sprintf(buffer, "%s%s", username, sv_cs_Salt->string);
    ::strncpy(user->uguid, Com_GetTigerHash(buffer), 32);
    user->type = type[0];

    while((clan = strtok(nullptr, "@\n\r\0")) != nullptr) {
        clan_p = (clan_t *)Com_FindHashData(hash_clans, clan);

        if(clan_p != nullptr) {
            user_clan = (user_clan_t *)::malloc(sizeof(user_clan_t));
            ::memset(user_clan, 0, sizeof(user_clan_t));

            user_clan->clan = clan_p;
            user_clan->next = user->clans;
            user->clans = user_clan;
        } else {
            Com_Printf("Clan %s does not exist in Clan DB\n", clan);
        }
    }

    return user;
}

/*
===============
idServerCommunityServer::CheckUserFileChange
===============
*/
sint idServerCommunityServer::CheckUserFileChange(void) {
    struct stat st;
    valueType *filename, filesrc[1024], filedst[1024];

    filename = fileSystem->GetFullGamePath("cs_users.new");

    // Check for file real time modifications
    if(::stat(filename, &st) != -1 && ::time(nullptr) - st.st_mtime > 10) {
        if(file_user_last_modified < st.st_mtime) {
            Q_strcpy_s(filesrc, fileSystem->GetFullGamePath("cs_users.new"));
            Q_strcpy_s(filedst, fileSystem->GetFullGamePath("cs_users.txt"));

            copyFile(filesrc, filedst);

            // Free all information and reload
            Com_DestroyHash(hash_users);
            hash_users = nullptr;

            Com_DestroyHash(hash_clans);
            hash_clans = nullptr;

            ::fclose(usersfile);
            usersfile = nullptr;

            LoadUserFile();

            return 1;
        }
    }

    return 0;
}

/*
===============
idServerCommunityServer::copyFile
===============
*/
sint idServerCommunityServer::copyFile(valueType *src, valueType *dst) {
    sint data;
    FILE *fsrc, *fdst;

    fsrc = ::fopen(src, "rb");
    fdst = ::fopen(dst, "wb");

    if(fsrc == nullptr || fdst == nullptr) {
        Com_Printf("Error while copy file!");
        return CS_ERROR;
    }

    data = ::fgetc(fsrc);

    while(!::feof(fsrc)) {
        ::fputc(data, fdst);
        data = ::fgetc(fsrc);
    }

    ::fclose(fsrc);
    ::fclose(fdst);
    return CS_OK;
}

/*
===============
idServerCommunityServer::destroyLongData
===============
*/
void idServerCommunityServer::destroyLongData(void *data) {
    sint32 *d;

    d = static_cast<sint32 *>(data);
    ::free(d);
}

/*
===============
idServerCommunityServer::UserInfo
===============
*/
void idServerCommunityServer::UserInfo(valueType *name) {
    sint32 *dpos;
    user_t *user;
    user_clan_t *user_clan;

    if((dpos = static_cast<sint32 *>(Com_FindHashData(hash_users,
                                     name))) == nullptr) {
        Com_Printf("User %s does not exist in DB\n", cmdSystem->Argv(1));
        return;
    }

    user = parseUserLine(cmdSystem->Argv(1));

    if(user == nullptr) {
        Com_Printf("Error parsing datafile\n");
        return;
    }

    Com_Printf("User: %s\n", user->name);
    Com_Printf("Status: %c\n", user->type);
    Com_Printf("uguid: %s\n", user->uguid);
    Com_Printf("Clans: ");

    for(user_clan = user->clans; user_clan != nullptr;
            user_clan = user_clan->next) {
        Com_Printf(" %s", user_clan->clan->name);
    }

    Com_Printf("\n");

    destroyUserData(user);
}

/*
===============
idServerCommunityServer::destroyClanData
===============
*/
void idServerCommunityServer::destroyClanData(void *data) {
    clan_t *clan;
    clan = (clan_t *)data;

    ::free(clan);
}

/*
===============
idServerCommunityServer::destroyUserData
===============
*/
void idServerCommunityServer::destroyUserData(user_t *user) {
    user_clan_t *user_clan, * user_clan_next;

    if(user == nullptr) {
        return;
    }

    for(user_clan = (user)->clans; user_clan != nullptr;
            user_clan = user_clan_next) {
        user_clan_next = user_clan->next;
        ::free(user_clan);
    }

    ::free(user);
}

/*
===============
idServerCommunityServer::userInfoChanged
===============
*/
void idServerCommunityServer::userInfoChanged(client_t *cl) {
    if(cl->cs_user != nullptr) {
        Info_SetValueForKey(cl->userinfo, "cl_guid", cl->cs_user->uguid);
        Info_SetValueForKey(cl->userinfo, "password", "");
        Info_SetValueForKey(cl->userinfo, "cg_rgb", sv_cs_MemberColor->string);
    } else {
        Info_SetValueForKey(cl->userinfo, "cg_rgb", sv_cs_UnknownColor->string);
    }
}

/*
===============
idServerCommunityServer::logString
===============
*/
void idServerCommunityServer::logString(sint client_num, valueType *area,
                                        valueType *message) {
    static sint arch = -1;
    valueType buffer[1024];
    convar_t *logFile;

    if(sv_cs_BotLog->integer != 1) {
        return;
    }

    if(arch < 0) {
        logFile = cvarSystem->Get("g_log", "bot.log", 0, "test");

        Com_Printf("Openning %s for bot log\n", logFile->string);

        arch = ::open(fileSystem->GetFullGamePath(logFile->string),
                      O_WRONLY | O_APPEND);

        if(arch < 0) {
            return;
        }
    }

    ::snprintf(buffer, sizeof(buffer), "7:14 %s: %d %s\n", area, client_num,
               message);

    if(::write(arch, buffer, ::strlen(buffer)) < 0) {
        arch = -1;
    }
}

/*
===============
idServerCommunityServer::startMatch
===============
*/
void idServerCommunityServer::startMatch(void) {
    if(match_in_progress == 1) {
        Com_Printf("Match already started, please, add clans and players\n");
        Com_Printf("tip: use addclanmatch and addusermatch commands to do that\n");
        return;
    }

    match_in_progress = 1;

    SaveStatistics();
    InitStatistics();

    ::memset(match_clans, '\0', sizeof(match_clans));
    ::memset(match_users, '\0', sizeof(match_users));
    ::memset(match_referees, '\0', sizeof(match_users));

    Com_Printf("Match started, please, add clans, players and referees\n");
    Com_Printf("tip: use addclanmatch, addusermatch and addrefereematch commands to do that\n");
}

/*
===============
idServerCommunityServer::stopMatch
===============
*/
void idServerCommunityServer::stopMatch(void) {
    if(match_in_progress == 0) {
        Com_Printf("There are NO match in progress, use startmatch to start one\n");
        return;
    }

    Com_Printf("Match closed, now you may open the server! Thanks!\n");
    match_in_progress = 0;
    return;
}

/*
===============
idServerCommunityServer::addMatchClan
===============
*/
void idServerCommunityServer::addMatchClan(valueType *clan_name) {
    sint i;
    clan_t *clan_p;

    if(match_in_progress == 0) {
        Com_Printf("There are NO match in progress, use startmatch to start one\n");
        return;
    }

    if(::strlen(clan_name) > CLAN_NAME_SIZE) {
        Com_Printf("Clan name is too long");
        return;
    }

    clan_p = (clan_t *)Com_FindHashData(hash_clans, clan_name);

    if(clan_p == nullptr) {
        Com_Printf("Clan %s doesn't exist!\n", clan_name);
        return;
    }

    for(i = 0; i < MATCH_MAX_CLANS && ::strlen(match_clans[i]) != 0 &&
            ::strcmp(match_clans[i], clan_name) != 0; i++);

    if(i >= MATCH_MAX_CLANS) {
        Com_Printf("You cannot add another clan, max reached\n");
        return;
    } else if(::strcmp(match_clans[i], clan_name) == 0) {
        Com_Printf("Clan %s already added\n", clan_name);
        return;
    }

    Q_strcpy_s(match_clans[i], clan_name);
    Com_Printf("Clan %s added to match\n", clan_name);

    return;
}

/*
===============
idServerCommunityServer::addMatchUser
===============
*/
void idServerCommunityServer::addMatchUser(valueType *user_name) {
    sint i;
    user_t *user_p;

    if(match_in_progress == 0) {
        Com_Printf("There are NO match in progress, use startmatch to start one\n");
        return;
    }

    if(strlen(user_name) > USER_NAME_SIZE) {
        Com_Printf("User name is too long\n");
        return;
    }

    user_p = static_cast< user_t * >(Com_FindHashData(hash_users, user_name));

    if(user_p == nullptr) {
        Com_Printf("User %s doesn't exist!\n", user_name);
        return;
    }

    for(i = 0; i < MATCH_MAX_USERS && ::strlen(match_users[i]) != 0 &&
            ::strcmp(match_users[i], user_name) != 0; i++);

    if(i >= MATCH_MAX_USERS) {
        Com_Printf("You cannot add another user, max reached\n");
        return;
    } else if(::strcmp(match_users[i], user_name) == 0) {
        Com_Printf("User %s already added\n", user_name);
        return;
    }

    Q_strcpy_s(match_users[i], user_name);
    Com_Printf("User %s added to match\n", user_name);

    return;
}

/*
===============
idServerCommunityServer::addMatchReferee
===============
*/
void idServerCommunityServer::addMatchReferee(valueType *user_name) {
    sint i;
    user_t *user_p;

    if(match_in_progress == 0) {
        Com_Printf("There are NO match in progress, use startmatch to start one\n");
        return;
    }

    if(::strlen(user_name) > USER_NAME_SIZE) {
        Com_Printf("User name is too long\n");
        return;
    }

    user_p = static_cast< user_t * >(Com_FindHashData(hash_users, user_name));

    if(user_p == nullptr) {
        Com_Printf("User %s doesn't exist!\n", user_name);
        return;
    }

    for(i = 0; i < MATCH_MAX_USERS && ::strlen(match_referees[i]) != 0 &&
            ::strcmp(match_referees[i], user_name) != 0; i++);

    if(i >= MATCH_MAX_USERS) {
        Com_Printf("You cannot add another referee, max reached\n");
        return;
    } else if(::strcmp(match_users[i], user_name) == 0) {
        Com_Printf("Referee %s already added\n", user_name);
        return;
    }

    Q_strcpy_s(match_referees[i], user_name);
    Com_Printf("Referee %s added to match\n", user_name);

    return;
}

/*
===============
idServerCommunityServer::matchInfo
===============
*/
void idServerCommunityServer::matchInfo(void) {
    sint i;

    CheckUserFileChange();

    if(match_in_progress == 0) {
        Com_Printf("There are NO match in progress, use startmatch to start one\n");
        return;
    }

    Com_Printf("Match started\n");
    Com_Printf("Clans:");

    for(i = 0; i < MATCH_MAX_CLANS && strlen(match_clans[i]) != 0; i++) {
        Com_Printf(" %s", match_clans[i]);
    }

    Com_Printf("\n");

    Com_Printf("Users:");

    for(i = 0; i < MATCH_MAX_USERS && strlen(match_users[i]) != 0; i++) {
        Com_Printf(" %s", match_users[i]);
    }

    Com_Printf("\n");

    Com_Printf("Referees:");

    for(i = 0; i < MATCH_MAX_USERS && strlen(match_referees[i]) != 0; i++) {
        Com_Printf(" %s", match_referees[i]);
    }

    Com_Printf("\n");
}

/*
===============
idServerCommunityServer::LoadBanFile
===============
*/
void idServerCommunityServer::LoadBanFile(void) {
    sint32 fpos, * dfpos;
    valueType *filename, buffer[1024], * banguid;
    banuser_t *banuser;

    filename = fileSystem->GetFullGamePath("cb_bans.txt");

    bansfile = ::fopen(filename, "rt+");

    if(bansfile == nullptr) {
        bansfile = ::fopen(filename, "wt+");

        if(bansfile == nullptr) {
            Com_Printf("Error trying to open cb_bans.txt\n");
            return;
        }
    }

    hash_bans = Com_CreateHashTable(Com_JenkinsHashKey, Com_StrCmp,
                                    Com_DestroyStringKey, destroyLongData, HASH_BAN_SIZE);

    fpos = ::ftell(bansfile);
    ::fgets(buffer, sizeof(buffer), bansfile);

    while(!::feof(bansfile)) {
        banuser = processBanLine(buffer);

        if(banuser != nullptr) {
            banguid = static_cast<valueType *>(::malloc(33));

            ::memset(banguid, 0, 33);
            ::strncpy(banguid, banuser->guid, 32);

            dfpos = static_cast<sint32 *>(::malloc(sizeof(sint32)));
            *dfpos = fpos;

            Com_InsertIntoHash(hash_bans, banguid, dfpos);
        }

        fpos = ::ftell(bansfile);
        ::fgets(buffer, sizeof(buffer), bansfile);
    }
}

/*
===============
idServerCommunityServer::processBanLine
===============
*/
banuser_t *idServerCommunityServer::processBanLine(valueType *line) {
    static banuser_t banuser;
    valueType *word, buffer[1024] = {0};

    ::memset(&banuser, 0, sizeof(banuser_t));
    ::strncpy(buffer, line, sizeof(buffer) - 1);

    // Name
    word = ::strtok(buffer, ":");

    if(word == nullptr) {
        Com_Printf("Error processing name field in ban file\n");
        return nullptr;
    }

    ::strncpy(banuser.name, word, USER_NAME_SIZE);

    // Until time
    word = ::strtok(nullptr, ":");

    if(word == nullptr) {
        Com_Printf("Error processing until time field in ban file\n");
        return nullptr;
    }

    banuser.until = ::atoi(word);

    if(banuser.until < ::time(nullptr)) {
        //Com_Printf("This user is not banned anymore!\n");
        return nullptr;
    }

    // Membership
    word = ::strtok(nullptr, ":");

    if(word == nullptr) {
        Com_Printf("Error processing membership field in ban file\n");
        return nullptr;
    }

    if(word[0] != CS_MEMBER && word[0] != CS_UNKNOW) {
        Com_Printf("Error, membership %c is not valid!\n", word[0]);
        return nullptr;
    }

    banuser.type = word[0];

    // GUID
    word = ::strtok(nullptr, "\r\n\0");

    if(word == nullptr || ::strlen(word) != 32) {
        Com_Printf("Error processing GUID field in ban file\n");
        return nullptr;
    }

    Q_strcpy_s(banuser.guid, word);

    return &banuser;
}

/*
===============
idServerCommunityServer::BanUser
===============
*/
void idServerCommunityServer::BanUser(client_t *cl) {
    sint32 *dpos;
    valueType *guid;
    banuser_t banuser;
    valueType buffer[1024];

    ::memset(&banuser, 0, sizeof(banuser_t));

    dpos = static_cast<sint32 *>(::malloc(sizeof(sint32)));
    guid = static_cast<valueType *>(::malloc(33));

    ::memset(guid, 0, 33);

    // Add to Hash
    ::strncpy(guid, Info_ValueForKey(cl->userinfo, "cl_guid"), 32);
    guid[32] = '\0';

    if(Com_FindHashData(hash_bans, guid) != nullptr) {
        Com_Printf("Guid %s already added\n", guid);

        ::free(guid);
        ::free(dpos);

        return;
    }

    ::fseek(bansfile, 0, SEEK_END);

    *dpos = ::ftell(bansfile);

    Com_InsertIntoHash(hash_bans, guid, dpos);

    // Add to file
    if(cl->cs_user != nullptr) {
        Q_strcpy_s(banuser.name, cl->cs_user->name);
        banuser.type = CS_MEMBER;
    } else {
        ::strncpy(banuser.name, Info_ValueForKey(cl->userinfo, "name"),
                  USER_NAME_SIZE);
        banuser.type = CS_UNKNOW;
    }

    banuser.until = 0x7FFFFFFF;

    ::sprintf(buffer, "%s:%12d:%c:%s\n", banuser.name, banuser.until,
              banuser.type, guid);
    ::fputs(buffer, bansfile);
    ::fflush(bansfile);

    serverClientSystem->DropClient(cl, "KICKED!");

    return;
}

/*
===============
idServerCommunityServer::checkBanGUID
===============
*/
sint idServerCommunityServer::checkBanGUID(valueType *guid) {
    if(Com_FindHashData(hash_bans, guid) != nullptr) {
        return 1;
    }

    return 0;
}

/*
===============
idServerCommunityServer::showBanUsers
===============
*/
void idServerCommunityServer::showBanUsers(void) {
    sint i = 1;
    hash_table_iterator_t *iter;
    valueType buffer[1024];
    banuser_t *banuser;
    void *data;

    iter = Com_CreateHashIterator(hash_bans);

    Com_Printf("Banned users: \n");

    while((data = Com_HashIterationData(iter)) != nullptr) {
        ::fseek(bansfile, *(static_cast<sint32 *>(data)), SEEK_SET);
        ::fgets(buffer, sizeof(buffer), bansfile);

        banuser = processBanLine(buffer);

        if(banuser != nullptr) {
            Com_Printf("%d: %s - %c Banned until %s", i++, banuser->name,
                       banuser->type, ctime((const time_t *) & (banuser->until)));
        }
    }

    ::free(iter);
}

/*
===============
idServerCommunityServer::unbanUser
===============
*/
void idServerCommunityServer::unbanUser(valueType *banlist_num) {
    sint i, unban_num;
    hash_table_iterator_t *iter;
    valueType buffer[1024];
    banuser_t *banuser;
    void *data;

    unban_num = ::atoi(banlist_num);

    iter = Com_CreateHashIterator(hash_bans);

    for(i = 1; (data = Com_HashIterationData(iter)) != nullptr; i++) {
        if(unban_num == i) {
            ::fseek(bansfile, *(static_cast<sint32 *>(data)), SEEK_SET);
            ::fgets(buffer, sizeof(buffer), bansfile);

            banuser = processBanLine(buffer);

            if(banuser != nullptr) {
                Com_Printf("Unbanned: %s - %c Banned until %s", banuser->name,
                           banuser->type, ::ctime((const time_t *) & (banuser->until)));

                ::fseek(bansfile, *(static_cast<sint32 *>(data)), SEEK_SET);
                ::sprintf(buffer, "%s:%12d", banuser->name, 0);
                ::fwrite(buffer, strlen(buffer), 1, bansfile);
                ::fflush(bansfile);

                Com_DeleteFromHash(hash_bans, banuser->guid);
            }

            return;
        }
    }

    Com_Printf("Banuser %s does not exist! Please check using banlist command\n",
               banlist_num);

    ::free(iter);
}

/*
===============
idServerCommunityServer::PlayerGameInfo
===============
*/
sint idServerCommunityServer::checkClientCommandPermitions(client_t *cl,
        pointer client_cmd) {
    sint i;
    valueType buffer[1024] = {0}, * cmd;
    valueType *only_members[] = {"callvote", "vote", nullptr };

    if(cl->cs_user != nullptr || sv_cs_ServerType->integer == 0) {
        return CS_OK;
    }

    ::strncpy(buffer, client_cmd, sizeof(buffer) - 1);

    cmd = ::strtok(buffer, " \0\n\r");

    for(i = 0; only_members[i] != nullptr &&
#ifdef _WIN32
            ::_stricmp(cmd, only_members[i]) != 0; i++);

#else
            ::strcasecmp(cmd, only_members[i]) != 0;
            i++);
#endif

    if(only_members[i] != nullptr) {
        serverMainSystem->SendServerCommand(cl,
                                            "chat \"^9YOU MUST BE MEMBER TO SEND %s COMMAND!\"", cmd);
        return CS_ERROR;
    }

    return CS_OK;

}

/*
===============
idServerCommunityServer::StatsLoop
===============
*/
sint idServerCommunityServer::StatsLoop(void) {
    static sint last_update = 0, client_num = 0;
    sint actual_update;

    actual_update = idsystem->Milliseconds();

    // TODO: poner el timeout como variable seteable
    if(((actual_update - last_update) >= 1000 &&
            community_stats.mapstatus == 2) ||
            ((actual_update - last_update) >= 3000 &&
             community_stats.mapstatus != 2)) {
        last_update = actual_update;
        client_num = (client_num + 1) % sv_maxclients->integer;
        stats_client_num = client_num;

        PlayerGameInfo(stats_client_num);
        PlayerServerInfo(stats_client_num);
        ServerInfo();
    }

    NET_Loop();

    return 0;
}

/*
===============
idServerCommunityServer::PlayerGameInfo
===============
*/
void idServerCommunityServer::PlayerGameInfo(sint player) {
    valueType bigbuffer[ MAX_INFO_STRING * 2];

    // make sure server is running
    if(!com_sv_running->integer) {
        Com_Printf("Server is not running.\n");
        return;
    }

    if(svs.clients[player].state >= CS_ACTIVE) {
        ::sprintf(bigbuffer, "stats %i MELEE", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i BLASTER", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i MACHINEGUNE", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i PAIN_SAW", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i SHOTGUN", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i LAS_GUN", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i MASS_DRIVER", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i CHAINGUN", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i PULSE_RIFLE", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i FLAMER", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i LUCIFER_CANNON", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i GRENADE", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i LOCKBLOB_LAUNCHER", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i MISC", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);

        ::sprintf(bigbuffer, "stats %i EXPLOSIONS", player);
        cmdSystem->TokenizeString(bigbuffer);
        sgame->ClientCommand(player);
    }
}

/*
===============
idServerCommunityServer::ProcessServerCmd
===============
*/
sint idServerCommunityServer::ProcessServerCmd(valueType *message) {
    static valueType statsbufferold [MAX_INFO_STRING * 4];
    valueType line[1024 * 15];

    if(::strstr(static_cast<valueType *>(message), "stats \"")) {
        ::sprintf(line, "%s", &(message[7]));
        line[::strlen(line) - 1] = '\0';
        ::sprintf(line, "%s%s", line, statsbufferold);

        ParseStatistic(line);

        ::memset(line, 0, sizeof(line));
        ::memset(statsbufferold, 0, sizeof(statsbufferold));
        return 1;
    }

    if(::strstr(static_cast<valueType *>(message), "stats1 \"")) {
        ::sprintf(statsbufferold, "%s", &(message[8]));
        statsbufferold[::strlen(statsbufferold) - 2] = '\0';
        return 1;
    }

    return 0;
}

/*
===============
idServerCommunityServer::ParseStatistic
===============
*/
void idServerCommunityServer::ParseStatistic(valueType *line) {
#define MAX_TOKENS 100
    valueType *tokens[MAX_TOKENS + 1] = { nullptr };
    sint itoken = 0;
    valueType *tk;

    tk = ::strtok(line, "|\n\0");

    for(itoken = 0; itoken < MAX_TOKENS && tk != nullptr; itoken++) {
        tokens[itoken] = tk;
        tk = ::strtok(nullptr, "|\n\0");
    }

    ProcessSplit(tokens);
}

/*
===============
idServerCommunityServer::GetDescValue
===============
*/
sint idServerCommunityServer::GetDescValue(valueType *line,
        valueType *desc, valueType *val) {
    valueType *tmp;
    sint i;

    if(line == nullptr || desc == nullptr || val == nullptr) {
        return 0;
    }

    tmp = strstr(line, desc);

    if(tmp == nullptr) {
        return 0;
    }

    ::sprintf(val, "%s", &(tmp[::strlen(desc)]));

    for(tmp = val; *tmp != '\0' && *tmp == ' '; tmp++);

    for(i = 0; tmp[i] != '\0'; i++) {
        val[i] = tmp[i];
    }

    val[i] = '\0';

    for(i = ::strlen(val) - 1; i > 0 && val[i] == ' '; i--) {
        val[i] = '\0';
    }

    return 1;
}

/*
===============
idServerCommunityServer::ProcessSplit
===============
*/
void idServerCommunityServer::ProcessSplit(valueType **tokens) {
    valueType username[USER_NAME_SIZE + 1] = {0};
    user_stats_t *user_stats;
    sint type;

    if(svs.clients[stats_client_num].cs_user == nullptr) {
        ::strncpy(username, Info_ValueForKey(
                      svs.clients[stats_client_num].userinfo, "cl_guid"), 32);
        username[32] = '\0';
        type = CS_GUID;
    } else {
        Q_strcpy_s(username, svs.clients[stats_client_num].cs_user->name);
        type = CS_USER;
    }

    if((user_stats = GetPlayerStats(username)) != nullptr) {
        user_stats->type = type;

        if(ProcessStats(user_stats, tokens)) {
        }
    }
}

/*
===============
idServerCommunityServer::GetPlayerStats
===============
*/
user_stats_t *idServerCommunityServer::GetPlayerStats(
    valueType *player_name) {
    valueType username[USER_NAME_SIZE + 1] = {0};
    user_stats_t **paux;

    Q_strcpy_s(username, player_name);

    for(paux = &(community_stats.user_stats); *paux != nullptr &&
            ::strcmp((*paux)->name, username) != 0; paux = &((*paux)->next));

    if(*paux == nullptr) {
        *paux = (user_stats_t *)::malloc(sizeof(user_stats_t));

        ::memset(*paux, 0, sizeof(user_stats_t));
        Q_strcpy_s((*paux)->name, username);

        (*paux)->start_time = idsystem->Milliseconds();
    }

    return *paux;
}

/*
===============
idServerCommunityServer::ProcessWeapon
===============
*/
sint idServerCommunityServer::ProcessStats(user_stats_t *user_stats,
        valueType **tokens) {
    valueType val[512];

    if(GetDescValue(tokens[1], "Weapon:", val)) {
        ProcessWeapon(user_stats, val, tokens);
    } else if(GetDescValue(tokens[1], "Category:", val)) {
        if(::strcmp(val, "MELEE") == 0) {
            ProcessMELEE(user_stats, tokens);
        }

        if(::strcmp(val, "MISC") == 0) {
            ProcessMISC(user_stats, tokens);
        }
    }

    return 1;
}

/*
===============
idServerCommunityServer::ProcessWeapon
===============
*/
void idServerCommunityServer::ProcessWeapon(user_stats_t *user_stats,
        valueType *weapon, valueType **tokens) {
    // MUST MATCH WITH weapon_name_e !
    valueType *l_weapons[] = {"BLASTER", "MACHINEGUNE", "PAIN_SAW", "SHOTGUN", "LAS_GUN", "MASS_DRIVER", "CHAINGUN", "PULSE_RIFLE", "FLAMER", "LUCIFER_CANNON", "GRENADE", "LOCKBLOB_LAUNCHER", nullptr };
    sint iweapon;
    valueType **l_stats = nullptr;
    pointer l_stats_weapons[WEAPON_STATS + 1] = {"Fired:", "Hit:", "Hit Team:", "Missed:", "Kills:", "Deaths:",
                                                 "Damage Done:", "Damage To Team:", "Head:", "Torso:", "Legs:", "Arms:",
                                                 "Damage Taken:", "From Team:", "Head:",  "Torso:", "Legs:", "Arms:", nullptr
                                                };

    for(iweapon = 0; l_weapons[iweapon] != nullptr &&
            ::strcmp(l_weapons[iweapon], weapon) != 0; iweapon++);

    if(l_weapons[iweapon] == nullptr) {
        Com_Printf("Weapon %s does not exist!\n", weapon);

        return;
    }

    ProcessGeneric(l_stats_weapons, &(tokens[3]),
                   user_stats->weapons[iweapon]);
}

/*
===============
idServerCommunityServer::ProcessMELEE
===============
*/
void idServerCommunityServer::ProcessMELEE(user_stats_t *user_stats,
        valueType **tokens) {
    pointer l_stats[] = {"Alien Level 0 Kills:", "Alien Level 1 Kills:", "Alien Level 1 Upgrade Kills:", "Alien Level 2 Kills:", "Alien Level 2 Upgrade Kills:",
                         "Alien Level 3 Kills: ", "Alien Level 3 Upgrade Kills: ", "Alien Level 4 Upgrade Kills: ",
                         "Alien Level 0 Deaths: ", "Alien Level 1 Upgrade Deaths:", "Alien Level 2 Deaths:", "Alien Level 2 Upgrade Deaths:", "Alien Level 3 Deaths:", "Alien Level 3 Upgrade Kills:",
                         "Alien Level 4 Death:", nullptr
                        };

    ProcessGeneric(l_stats, &(tokens[2]), user_stats->melee);

}

/*
===============
idServerCommunityServer::ProcessMISC
===============
*/
void idServerCommunityServer::ProcessMISC(user_stats_t *user_stats,
        valueType **tokens) {
    pointer l_stats[] = {"Environmental Deaths:", "Suicides:", nullptr };

    ProcessGeneric(l_stats, &(tokens[2]), user_stats->misc);
}

/*
===============
idServerCommunityServer::ProcessGeneric
===============
*/
void idServerCommunityServer::ProcessGeneric(pointer *l_stats,
        valueType **tokens, sint *i_stats) {
    sint i_stat = 0, i_token = 0;
    valueType val[1024];

    while(l_stats[i_stat] != nullptr && tokens[i_token] != nullptr) {
        if(GetDescValue(tokens[i_token],
                        const_cast<valueType *>(reinterpret_cast<const valueType *>
                                                (l_stats[i_stat])), val) == 1) {
            i_stats[i_stat] = atoi(val);
            i_stat++;
        }

        while(l_stats[i_stat] != nullptr && l_stats[i_stat][0] == '0') {
            i_stat++;
        }

        i_token++;
    }
}

/*
===============
idServerCommunityServer::SaveStatistics
===============
*/
void idServerCommunityServer::SaveStatistics(void) {
    sint offset = 0, old_offset = 0;
    valueType *filename;
    user_stats_t *pu_iter, *pu_del;
    static FILE *fstats = nullptr;

    filename = fileSystem->GetFullGamePath("community_stats.txt");

    if(fstats == nullptr) {
        fstats = fopen(filename, "a+");

        if(fstats == nullptr) {
            Com_Printf("CBStats Error: Cannot open file %s to save stats!\n",
                       filename);
        }
    }

    for(pu_del = nullptr, pu_iter = community_stats.user_stats;
            pu_iter != nullptr; pu_del = pu_iter, pu_iter = pu_iter->next) {
        old_offset = offset;
        offset += SaveUserStatsOptimal(pu_iter,
                                       &(sv_community_last_match_s[offset]));

        NETAddMsg(&(sv_community_last_match_s[old_offset]));

        ::free(pu_del);
    }

    ::fprintf(fstats, "%s", sv_community_last_match_s);
    ::fflush(fstats);

    community_stats.user_stats = nullptr;
}

/*
===============
idServerCommunityServer::SaveUserStats
===============
*/
void idServerCommunityServer::SaveUserStats(user_stats_t *user_stats,
        FILE *fstats) {
    sint i, j, offset = 0;
    valueType fline[1024 * 5] = {0};
    pointer weapons_mnemonics[WEAPONS + 1] = { WEAPONS_MNEMONICS };
    pointer weapons_stats_mnemonics[WEAPON_STATS + 1] = { WEAPONS_STATS_MNEMONICS };
    pointer melee_stats_mnemonics[MELEE_STATS + 1] = { MELEE_STATS_MNEMONICS };
    pointer explosions_stats_mnemonics[EXPLOSIONS_STATS + 1] = { EXPLOSIONS_STATS_MNEMONICS };
    pointer misc_stats_mnemonics[MISC_STATS + 1] = { MISC_STATS_MNEMONICS };

    ::fprintf(fstats, "H|%s|%s", community_stats.game_id, user_stats->name);
    offset += ::sprintf(&(fline[offset]), "M|%s|%s", community_stats.game_id,
                        user_stats->name);

    // Start sections
    ::fprintf(fstats, "*WEAPONS");
    offset += ::sprintf(&(fline[offset]), "*W");

    for(i = 0; i < WEAPONS; i++) {
        ::fprintf(fstats, "$%s", weapons_mnemonics[i]);
        offset += ::sprintf(&(fline[offset]), "$%d", i);

        for(j = 0; j < WEAPON_STATS; j++) {
            ::fprintf(fstats, "|%s:%d", weapons_stats_mnemonics[j],
                      user_stats->weapons[i][j]);
            offset += ::sprintf(&(fline[offset]), "|%d", user_stats->weapons[i][j]);
        }
    }

    // Start sections
    ::fprintf(fstats, "*MELEE");
    offset += ::sprintf(&(fline[offset]), "*M");

    for(i = 0; i < MELEE_STATS; i++) {
        ::fprintf(fstats, "|%s:%d", melee_stats_mnemonics[i],
                  user_stats->melee[i]);
        offset += ::sprintf(&(fline[offset]), "|%d", user_stats->melee[i]);
    }

    // Start sections
    ::fprintf(fstats, "*EXPLOSIONS");
    offset += ::sprintf(&(fline[offset]), "*E");

    for(i = 0; i < EXPLOSIONS_STATS; i++) {
        ::fprintf(fstats, "|%s:%d", explosions_stats_mnemonics[i],
                  user_stats->explosions[i]);
        offset += ::sprintf(&(fline[offset]), "|%d", user_stats->explosions[i]);
    }

    // Start sections
    ::fprintf(fstats, "*MISC");
    offset += ::sprintf(&(fline[offset]), "*I");

    for(i = 0; i < MISC_STATS; i++) {
        ::fprintf(fstats, "|%s:%d", misc_stats_mnemonics[i], user_stats->misc[i]);
        offset += ::sprintf(&(fline[offset]), "|%d", user_stats->misc[i]);
    }

    ::fprintf(fstats, "\n");
    ::fprintf(fstats, "%s\n", fline);
}

/*
===============
idServerCommunityServer::SaveUserStatsOptimal
===============
*/
sint idServerCommunityServer::SaveUserStatsOptimal(user_stats_t
        *user_stats, valueType *fline) {
    pointer weapons_mnemonics[WEAPONS + 1] = { WEAPONS_MNEMONICS };
    pointer weapons_stats_mnemonics[WEAPON_STATS + 1] = { WEAPONS_STATS_MNEMONICS };
    pointer melee_stats_mnemonics[MELEE_STATS + 1] = { MELEE_STATS_MNEMONICS };
    pointer misc_stats_mnemonics[MISC_STATS + 1] = { MISC_STATS_MNEMONICS };

    //valueType fline[1024*5] = {0};
    sint offset = 0;
    sint oldoffset = 0;
    sint first_weapon = 1;
    sint i;

    offset += ::sprintf(&(fline[offset]),
                        "{\"ST\":\"P\",\"PLAYER\":{\"GENERAL\":{\"GID\":\"%s\",\"PTY\":\"%d\",\"NME\":\"%s\",\"GNM\":\"%s\",\"AVP\":\"%d\",\"TME\":\"%d\",\"GME\":\"%d\",\"LIN\":\"%d\",\"SRE\":\"%d\",\"TEM\":\"%d\"}",
                        community_stats.game_id, user_stats->type, user_stats->name,
                        user_stats->game_name,
                        user_stats->avg_ping, user_stats->time / 1000,
                        user_stats->game_time / 1000,
                        user_stats->login, user_stats->score, user_stats->team);

    offset += ::sprintf(&(fline[offset]), ",\"W\":{");

    oldoffset = offset;

    for(i = 0; i < WEAPONS; i++) {
        offset += PrintStat(&(fline[offset]), WEAPON_STATS, weapons_mnemonics[i],
                            user_stats->weapons[i], weapons_stats_mnemonics);

        if(oldoffset != offset) {
            if(first_weapon == 1) {
                first_weapon = 0;
            } else {
                fline[oldoffset] = ',';
            }
        }

        oldoffset = offset;
    }

    offset += ::sprintf(&(fline[offset]), "}");

    oldoffset = offset;
    offset += PrintStat(&(fline[offset]), MELEE_STATS, "M", user_stats->melee,
                        melee_stats_mnemonics);

    if(oldoffset != offset) {
        fline[oldoffset] = ',';
    }

    oldoffset = offset;
    offset += PrintStat(&(fline[offset]), MISC_STATS, "I", user_stats->misc,
                        misc_stats_mnemonics);

    if(oldoffset != offset) {
        fline[oldoffset] = ',';
    }

    offset += ::sprintf(&(fline[offset]), "}}");
    offset += ::sprintf(&(fline[offset]), "\n");

    return offset;
}

/*
===============
idServerCommunityServer::PrintStat
===============
*/
sint idServerCommunityServer::PrintStat(valueType *out, sint count_stats,
                                        pointer section_mnemonic, const sint *stats, pointer *stats_mnemonics) {
    sint boffset = 0;
    valueType buffer[1024 * 5] = {0};
    sint ret = 0;
    sint i = 0;
    sint j = 0;

    for(i = 0; i < count_stats; i++) {
        if(stats[i] != 0) {
            if(j != 0) {
                boffset += ::sprintf(&(buffer[boffset]), ",");
            }

            j++;

            boffset += ::sprintf(&(buffer[boffset]), "\"%s\":\"%d\"",
                                 stats_mnemonics[i], stats[i]);
        }
    }

    if(boffset != 0) {
        // must be a space in front of the section to add ',' later!
        ret = ::sprintf(out, " \"%s\":{%s}", section_mnemonic, buffer);
    }

    return ret;
}

/*
===============
idServerCommunityServer::InitStatistics
===============
*/
void idServerCommunityServer::InitStatistics(void) {
    valueType buffer[1024];

    LoadMsgFile();

    // Cleaning community_stats structure
    if(community_stats.user_stats != nullptr) {
        Com_Printf("ERROR! all Community Manager Stats must be cleaned at init stage! Memory will be lost!\n");
    }

    memset(&community_stats, 0, sizeof(community_stats_t));

    srand(time(nullptr));
    sprintf(buffer, "%d%d", idsystem->Milliseconds(), rand());
    sprintf(community_stats.game_id, "%s",  Com_GetTigerHash(buffer));
    community_stats.game_date = time(nullptr);
    Com_Printf("Game ID: %s\n", community_stats.game_id);

    sprintf(buffer,
            "{\"ST\":\"G\",\"GAME\":{\"PRT\":\"%d\",\"GID\":\"%s\",\"MAP\":\"%s\",\"STM\":\"%d\",\"GTY\":\"%d\",\"STS\":\"%d\"}}\n",
            sv_cs_ServerPort->integer,
            community_stats.game_id,
            sv_mapname->string,
            static_cast<sint>(time(nullptr)),
            g_gameType->integer,
            match_in_progress);
    NETAddMsg(buffer);

}

/*
===============
idServerCommunityServer::PlayerServerInfo
===============
*/
void idServerCommunityServer::PlayerServerInfo(sint clnum) {
    sint n, actual_time, type;
    float32 aux_ping, actual_ping;
    user_stats_t *user_stats = nullptr;
    playerState_t   *ps = nullptr;
    valueType username[1024];

    if(svs.clients[clnum].state >= CS_ACTIVE) {
        if(svs.clients[clnum].cs_user == nullptr) {
            ::strncpy(username, Info_ValueForKey(svs.clients[clnum].userinfo,
                                                 "cl_guid"), 32);
            username[32] = '\0';
            type = CS_GUID;
        } else {
            Q_strcpy_s(username, svs.clients[stats_client_num].cs_user->name);
            type = CS_USER;
        }

        if((user_stats = GetPlayerStats(username)) != nullptr) {
            user_stats->type = type;
            ::strncpy(user_stats->game_name, svs.clients[stats_client_num].name,
                      sizeof(user_stats->game_name));
            user_stats->game_name[sizeof(user_stats->game_name) - 1] = '\0';

            // Ping Average
            user_stats->n_ping++;

            aux_ping = svs.clients[clnum].ping;
            actual_ping = user_stats->avg_ping;
            n = user_stats->n_ping;
            user_stats->avg_ping = actual_ping / n * (n - 1) + aux_ping / n;

            actual_time = idsystem->Milliseconds();
            ps = serverGameSystem->GameClientNum(clnum);

            if(user_stats->last_time == 0) {
                user_stats->last_time = actual_time;
            }

            // Team
#if 0

            //Dushan - make this work with Tremulous
            if(ps->persistant[PERS_TEAM] == 1 || ps->persistant[PERS_TEAM] == 2) {
                user_stats->team = ps->persistant[PERS_TEAM];
                user_stats->game_time += actual_time - user_stats->last_time;
            }

#endif

            // Gametime
            user_stats->time = actual_time - user_stats->start_time;
            user_stats->last_time = actual_time;
            user_stats->score = ps->persistant[PERS_SCORE];
        }
    }
}

/*
===============
idServerCommunityServer::ServerInfo
===============
*/
void idServerCommunityServer::ServerInfo(void) {
    sint i, players = 0;

    for(i = 0; i < sv_maxclients->integer; i++) {
        if(svs.clients[i].state >= CS_ACTIVE) {
            players++;
        }
    }

    community_stats.server_stats.players = players;

    GetCPUProcess();
    GetMEMProcess();
}

/*
===============
idServerCommunityServer::GetCPUProcess
===============
*/
void idServerCommunityServer::GetCPUProcess(void) {
#ifdef __LINUX___
    static struct timeval last = {0};
    static struct timeval last_cpu = {0};
    struct timeval actual = {0};
    struct timeval actual_cpu = {0};
    struct timeval aux_res = {0};
    struct timeval aux_cpu_res = {0};
    struct rusage usage;
    uint itime;
    uint icputime;

    if(last.tv_sec == 0) {
        gettimeofday(&last, nullptr);
        getrusage(RUSAGE_SELF, &usage);
        timeradd(&usage.ru_utime, &usage.ru_stime, &last_cpu);
        memcpy(&actual, &last, sizeof(struct timeval));
        memcpy(&actual_cpu, &last_cpu, sizeof(struct timeval));
        return;
    }

    gettimeofday(&actual, nullptr);
    timersub(&actual, &last, &aux_res);


    getrusage(RUSAGE_SELF, &usage);

    // CPU Calcs
    timeradd(&usage.ru_utime, &usage.ru_stime, &actual_cpu);
    timersub(&actual_cpu, &last_cpu, &aux_cpu_res);

    itime = aux_res.tv_sec * 1000000 + aux_res.tv_usec;
    icputime = aux_cpu_res.tv_sec * 1000000 + aux_cpu_res.tv_usec;

    community_stats.server_stats.cpu = (icputime * 100 / itime);
#else
    return;
#endif

}

/*
===============
idServerCommunityServer::GetMEMProcess
===============
*/
void idServerCommunityServer::GetMEMProcess(void) {
#ifdef __LINUX___
    static FILE *pipe = nullptr;
    valueType cmd[1024];
    valueType buffer[1024];

    if(pipe == nullptr) {
        sprintf(cmd, "/proc/%d/status", getpid());
        pipe = fopen(cmd, "rt");
    }

    fseek(pipe, 0, SEEK_SET);
    fgets(buffer, sizeof(buffer), pipe);

    while(!feof(pipe)) {
        if(strncmp("VmRSS:", buffer, 6) == 0) {
            community_stats.server_stats.mem = atoi(&(buffer[6]));
            return;
        }

        ::fgets(buffer, sizeof(buffer), pipe);
    }

#else
    return;
#endif
}

/*
===============
idServerCommunityServer::LoadMsgFile
===============
*/
void idServerCommunityServer::LoadMsgFile(void) {
    valueType *filename = nullptr;

    if(fcbbackup == nullptr) {
        filename = fileSystem->GetFullGamePath("cmss.dat");
        fcbbackup = fopen(filename, "ab");

        if(fcbbackup == nullptr) {
            Com_Printf("Error! I cannot write cmss.dat!\n");
        }
    }
}

/*
===============
idServerCommunityServer::NET_Loop
===============
*/
void idServerCommunityServer::NET_Loop(void) {
    static sint last_update = 0;
    sint actual_date, i, offset = 0;
    valueType buffer[1024 * 4] = {0};

    actual_date = idsystem->Milliseconds();

    if((actual_date - last_update) > (60000 * 3) &&
            community_stats.mapstatus != 2) {

        offset += ::sprintf(&(buffer[offset]),
                            "{\"ST\":\"S\",\"GAME\":{\"PRT\":\"%d\",\"GID\":\"%s\",\"MAP\":\"%s\",\"STM\":\"%d\",\"GTY\":\"%d\",\"STS\":\"%d\"},\"STAT\":{\"TME\":\"%d\",\"PLY\":\"%d\",\"CPU\":\"%d\",\"MEM\":\"%d\"",
                            sv_cs_ServerPort->integer,
                            community_stats.game_id,
                            sv_mapname->string,
                            community_stats.game_date,
                            g_gameType->integer,
                            match_in_progress,
                            static_cast<sint>(time(nullptr)),
                            community_stats.server_stats.players,
                            community_stats.server_stats.cpu,
                            community_stats.server_stats.mem);

        for(i = 0; i < sv_maxclients->integer; i++) {
            if(svs.clients[i].state >= CS_ACTIVE &&
                    svs.clients[i].cs_user != nullptr) {
                offset += ::sprintf(&(buffer[offset]), ",\"PLN\":\"%s\"",
                                    svs.clients[i].cs_user->name);
            }
        }

        offset += ::sprintf(&(buffer[offset]), "}}\n");

        NETAddMsg(buffer);

        last_update = actual_date;
    }
}

/*
===============
idServerCommunityServer::NETAddMsg
===============
*/
void idServerCommunityServer::NETAddMsg(valueType *msg) {
    valueType buffer[1024 * 3], * line;

    line = ::strtok(msg, "\n\r\0");

    while(line != nullptr) {
        ::sprintf(buffer, "%s\n", line);

        NETSendMsg(buffer);

        line = ::strtok(nullptr, "\n\r\0");
    }
}

/*
===============
idServerCommunityServer::NETSendMsg
===============
*/
sint idServerCommunityServer::NETSendMsg(valueType *msg) {
    sint sret = 0, len;
    static sint socket = -1;

    ::fprintf(fcbbackup, "%s", msg);

    if(socket == -1) {
        socket = networkSystem->ConnectTCP(sv_cs_stats->string);

        if(socket == -1) {
            return -1;
        }
    }

    len = ::strlen(msg) + 1;

    sret = SendAll(socket, msg, &len);

    if(sret == -1) {
        Com_Printf("Closed connection to community server...\n");
        socket = -1;
    }

    return 0;

}

/*
===============
idServerCommunityServer::SendAll
===============
*/
sint idServerCommunityServer::SendAll(sint s, valueType *buf, sint *len) {
    sint total = 0; // how many bytes we've sent
    sint bytesleft = *len; // how many we have left to send
    sint n = 0;

    while(total < *len) {
        n = ::send(s, buf + total, bytesleft, 0);

        if(n == -1) {
            break;
        }

        total += n;
        bytesleft -= n;
    }

    // return number actually sent here
    *len = total;

    // return -1 on failure, 0 on success
    return n == -1 ? -1 : 0;
}

