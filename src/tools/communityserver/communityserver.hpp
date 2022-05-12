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
// File name:   communityserver.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CBSERVER_H__
#define __CBSERVER_H__

#if defined(WIN32) || defined(WIN64)
#include <Windows.h>
#include <mysql.h>
#elif defined (__MACOSX__)
#include <mysql.h>
#include <stdarg.h>
#else
#include <mysql/mysql.h>
#endif

extern FILE *flog;

// Log files
#define FILE_TO_LOG "communityserver.log"

void lprintf(const char *fmt, ...);

typedef struct {
    char col[1024];
    char value[1024];
} col_value_t;

#define MAX_COL_VALUE 1000
typedef struct {
    int count;
    col_value_t col_value[MAX_COL_VALUE];
} sql_data_t;

MYSQL *connect_database(char *server, char *user, char *password,
                        char *database);
int db_get_id_from(char *st);
int db_add_reg(col_value_t *col_value);

int init_socket_server(char *ip, int port);
int accept_socket(int sock);
int db_accept_ip(char *ip);
int net_get_actual_client_ip(char *ip);

int parse_message(char *msg);
int parse_messageJSON(char *msg);

extern int actual_client_socket;

#define WEAPONS 12
typedef enum {
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
} weapon_name_e;

#define WEAPONS_MNEMONICS "$BL", "$MC", "$PS", "$SH", "$LG", "$MD", "$CG", "$PR", "$FL", "$LC", "$GR", "$LL", NULL

// Must be the same long as weapon_stats_e
#define WEAPON_STATS 18

typedef enum {
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
} weapon_stats_e;

#define WEAPONS_STATS_MNEMONICS "WFD", "WFH", "WFT", "WFM", "WK", "WD", "WDD", "WTT", "WHH", "WHT", "WHL", "WHA", "WDN", "WTN", "WDH", "WDT", "WDL", "WDA", NULL

#define MELEE_STATS 1
typedef enum {
    MS_MELLE_KILLS,
} melee_stats_e;

#define MELEE_STATS_MNEMONICS "MKK", NULL

#define EXPLOSIONS_STATS 1
typedef enum {
    ES_GRANADE_KILLS
} explosions_stats_e;

#define EXPLOSIONS_STATS_MNEMONICS "EGK", NULL

#define MISC_STATS 5
typedef enum {
    MIS_ENVIRONMENTAL_DEATHS,
    MIS_SUICIDES
} misc_stats_e;

#define MISC_STATS_MNEMONICS "IED", "ISD", NULL

typedef enum {
    CB_USER,
    CB_GUID
} user_type_e;

typedef struct user_stats_s {
    int type;
    char name[51];
    int weapons[WEAPONS][WEAPON_STATS];
    int melee[MELEE_STATS];
    int explosions[EXPLOSIONS_STATS];
    int misc[MISC_STATS];
    int avg_ping;
    int n_ping;
    int time;
    int game_time;
    int start_time;
    int last_time;
    int login;
    int score;
    int team;
    struct user_stats_s *next;
} user_stats_t;

typedef struct {
    int players;
    int cpu;
    int mem;
} server_stats_t;

typedef struct {
    user_stats_t *user_stats;
    server_stats_t server_stats;
    char game_id[49];
    int game_date;
    int mapstatus;
    char mapname[256];
} cb_stats_t;

#endif
