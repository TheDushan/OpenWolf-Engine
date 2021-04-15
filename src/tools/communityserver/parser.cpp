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
// File name:   parser.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <communityserver.hpp>
#include <json.hpp>

void parse_game(char *msg);
void parse_status(char *msg);
void parse_player(char *msg);

int level = 0;
int g_game_id = -1;
int g_server_id = -1;
int g_player_type = -1;
int g_user_id = -1;
int g_game_player_id = -1;

//parser_t *pmenu = parser_main;

typedef struct parser_s {
    char *name;
    char *sqlname;
    void (*f)(json_t *json, struct parser_s *mdata);
    struct parser_s *next;
} parser_t;

sql_data_t sql_data;

void generic(json_t *json, struct parser_s *mdata) {
    if(json != NULL && json->child != NULL && json->child->text != NULL) {
        strcpy(sql_data.col_value[sql_data.count].col, mdata->sqlname);
        sprintf(sql_data.col_value[sql_data.count].value, "\"%s\"",
                json->child->text);
        sql_data.count++;
    }
}

void genericNULL(json_t *json, struct parser_s *mdata) {
    if(json != NULL && json->child != NULL && json->child->text != NULL) {
        strcpy(sql_data.col_value[sql_data.count].col, mdata->sqlname);

        if(strlen(json->child->text) == 0) {
            sprintf(sql_data.col_value[sql_data.count].value, "NULL");
        } else {
            sprintf(sql_data.col_value[sql_data.count].value, "\"%s\"",
                    json->child->text);
        }

        sql_data.count++;
    }
}

void getServerID(json_t *json, struct parser_s *mdata) {
    char server_ip[256];
    char buffer[512];
    int server_id = 0;

    g_server_id = -1;

    if(json != NULL && json->child != NULL && json->child->text != NULL) {
        if(net_get_actual_client_ip(server_ip) == -1) {
            lprintf("Error retriving ip\n");
            return;
        }

        sprintf(buffer, "select id from server where ip='%s' and port='%s'\n",
                server_ip, json->child->text);

        if((server_id = db_get_id_from(buffer)) < 0) {
            lprintf("Server %s:%s does not exist in DB!\n", server_ip,
                    json->child->text);
            return;
        }

        g_server_id = server_id;
        strcpy(sql_data.col_value[sql_data.count].col, mdata->sqlname);
        sprintf(sql_data.col_value[sql_data.count].value, "%d", server_id);
        sql_data.count++;
    }
}

void genericDate(json_t *json, struct parser_s *mdata) {
    if(json != NULL && json->child != NULL && json->child->text != NULL) {
        strcpy(sql_data.col_value[sql_data.count].col, mdata->sqlname);
        sprintf(sql_data.col_value[sql_data.count].value, "FROM_UNIXTIME(%s)",
                json->child->text);
        sql_data.count++;
    }

}
void getGameTypeID(json_t *json, struct parser_s *mdata) {
    int game_type_id = 0;
    char buffer[512];

    if(json != NULL && json->child != NULL && json->child->text != NULL) {
        sprintf(buffer, "select id from game_type where rel=%s\n",
                json->child->text);

        if((game_type_id = db_get_id_from(buffer)) < 0) {
            return;
        }

        strcpy(sql_data.col_value[sql_data.count].col, mdata->sqlname);
        sprintf(sql_data.col_value[sql_data.count].value, "%d", game_type_id);
        sql_data.count++;
    }

}

void genericInsert(json_t *json, struct parser_s *mdata) {
    if(json != NULL && json->text != NULL) {
        strcpy(sql_data.col_value[sql_data.count].col, "insert");
        strcpy(sql_data.col_value[sql_data.count].value, mdata->sqlname);
        sql_data.count++;
    }
}

void statInsert(json_t *json, struct parser_s *mdata) {
    char *gameid;
    int i;
    char buffer[1024] = {0};

    if(json != NULL && json->text != NULL) {
        if(g_server_id == -1) {
            lprintf("ERROR! I cannot find server for stat!\n");
            return;
        }

        // Trying to add game
        g_game_id = db_add_reg(sql_data.col_value);

        // May fail due game is already inserted, so trying to get id
        if(g_game_id == -1) {
            for(i = 0; i < sql_data.count &&
                    strcmp(sql_data.col_value[i].col, "gameid") != 0; i++);

            if(i != sql_data.count) {
                gameid = sql_data.col_value[i].value;
            } else {
                lprintf("ERROR! gameid col was not found!\n");
                return;
            }

            sprintf(buffer, "select id from game where gameid=%s\n", gameid);

            if((g_game_id = db_get_id_from(buffer)) < 0) {
                lprintf("ERROR! Game is not in DB!\n");
                return;
            }

        }

        memset(&sql_data, 0, sizeof(sql_data));

        strcpy(sql_data.col_value[sql_data.count].col, "insert");
        strcpy(sql_data.col_value[sql_data.count].value, mdata->sqlname);
        sql_data.count++;

        strcpy(sql_data.col_value[sql_data.count].col, "server_id");
        sprintf(sql_data.col_value[sql_data.count].value, "%d", g_server_id);
        sql_data.count++;

        strcpy(sql_data.col_value[sql_data.count].col, "game_id");
        sprintf(sql_data.col_value[sql_data.count].value, "%d", g_game_id);
        sql_data.count++;
    }
}

parser_t parser_game[] = {
    {"PRT", "server_id", getServerID, NULL},
    {"GID", "gameid", generic, NULL},
    {"MAP", "mapname", generic, NULL},
    {"STM", "start_date", genericDate, NULL},
    {"GTY", "game_type_id", getGameTypeID, NULL},
    {"STS", "sts", generic, NULL},
    {NULL, NULL, NULL, NULL}
};

parser_t parser_stat[] = {
    {"TME", "time", genericDate, NULL},
    {"PLY", "players", generic, NULL},
    {"CPU", "cpu_perc", generic, NULL},
    {"MEM", "mem_kb", generic, NULL},
    {"PLN", "", NULL, NULL},
    {NULL, NULL, NULL, NULL}
};


void getGameID(json_t *json, struct parser_s *mdata) {
    char buffer[512];

    g_game_id = -1;

    if(json != NULL && json->child != NULL && json->child->text != NULL) {
        sprintf(buffer, "select id from game where gameid ='%s'\n",
                json->child->text);

        if((g_game_id = db_get_id_from(buffer)) < 0) {
            lprintf("Game %s does not exist!\n", json->child->text);
            return;
        }

        strcpy(sql_data.col_value[sql_data.count].col, mdata->sqlname);
        sprintf(sql_data.col_value[sql_data.count].value, "%d", g_game_id);
        sql_data.count++;
    }
}

void getPlayerType(json_t *json, struct parser_s *mdata) {
    g_player_type = -1;

    if(json != NULL && json->child != NULL && json->child->text != NULL) {
        g_player_type = atoi(json->child->text);
    }
}

void getPlayerName(json_t *json, struct parser_s *mdata) {
    char buffer[512];

    g_user_id = -1;

    if(json != NULL && json->child != NULL && json->child->text != NULL) {
        if(g_player_type == 1) { // Not registered
            strcpy(sql_data.col_value[sql_data.count].col, "guid");
            sprintf(sql_data.col_value[sql_data.count].value, "\"%s\"",
                    json->child->text);
            sql_data.count++;
        } else if(g_player_type == 0) { // registered player
            strcpy(sql_data.col_value[sql_data.count].col, "guid");
            sprintf(sql_data.col_value[sql_data.count].value, "\"%s\"",
                    json->child->text);
            sql_data.count++;

            sprintf(buffer, "select id from user where name ='%s'\n",
                    json->child->text);

            if((g_user_id = db_get_id_from(buffer)) < 0) {
                lprintf("Player %s does not exist!\n", json->child->text);
                return;
            }

            strcpy(sql_data.col_value[sql_data.count].col, "user_id");
            sprintf(sql_data.col_value[sql_data.count].value, "%d", g_user_id);
            sql_data.count++;
        } else {
            lprintf("Error at PlayerType!\n");
        }
    }
}

parser_t parser_general_stat[] = {
    {"GID", "game_id", getGameID, NULL},
    {"PTY", "", getPlayerType, NULL},
    {"NME", "", getPlayerName, NULL},
    {"GNM", "name", genericNULL, NULL},
    {"AVP", "avg_ping", generic, NULL},
    {"TME", "server_time", generic, NULL},
    {"GME", "game_time", generic, NULL},
    {"LIN", "login", generic, NULL},
    {"SRE", "score", generic, NULL},
    {"TEM", "team", generic, NULL},
    {NULL, NULL, NULL, NULL}
};

parser_t parser_weapon_stat[] = {
    {"WFD", "fired", generic, NULL},
    {"WFH", "fired_hit", generic, NULL},
    {"WFT", "fired_hit_team", generic, NULL},
    {"WFM", "fired_missed", generic, NULL},
    {"WK", "kills", generic, NULL},
    {"WD", "deads", generic, NULL},
    {"WDD", "damage_done", generic, NULL},
    {"WTT", "damage_to_team", generic, NULL},
    {"WHH", "hit_head", generic, NULL},
    {"WHT", "hit_torso", generic, NULL},
    {"WHL", "hit_legs", generic, NULL},
    {"WHA", "hit_arms", generic, NULL},
    {"WDN", "damage_taken", generic, NULL},
    {"WTN", "damage_taken_from_team", generic, NULL},
    {"WDH", "damage_taken_head", generic, NULL},
    {"WDT", "damage_taken_torso", generic, NULL},
    {"WDL", "damage_taken_legs", generic, NULL},
    {"WDA", "damage_taken_arms", generic, NULL},
    {NULL, NULL, NULL, NULL}
};

void genericWeaponInsert(json_t *json, struct parser_s *mdata) {
    char buffer[1024];
    int weapon_id;

    strcpy(sql_data.col_value[sql_data.count].col, "insert");
    strcpy(sql_data.col_value[sql_data.count].value, "weapon_stat");
    sql_data.count++;

    strcpy(sql_data.col_value[sql_data.count].col, "game_player_id");
    sprintf(sql_data.col_value[sql_data.count].value, "%d", g_game_player_id);
    sql_data.count++;

    /*strcpy(sql_data.col_value[sql_data.count].col, "weapon_id");
    strcpy(sql_data.col_value[sql_data.count].value, json->child->text);
    sql_data.count++;*/

    sprintf(buffer, "select id from weapon where mnemonic='%s'\n",
            mdata->sqlname);

    if((weapon_id = db_get_id_from(buffer)) < 0) {
        lprintf("Weapon does not exist! %s!\n", mdata->sqlname);
        return;
    }

    strcpy(sql_data.col_value[sql_data.count].col, "weapon_id");
    sprintf(sql_data.col_value[sql_data.count].value, "%d", weapon_id);
    sql_data.count++;
}


parser_t parser_weapon [] = {
    {"PB", "PB", genericWeaponInsert, parser_weapon_stat},
    {"PD", "PD", genericWeaponInsert, parser_weapon_stat},
    {"PP", "PP", genericWeaponInsert, parser_weapon_stat},
    {"PS", "PS", genericWeaponInsert, parser_weapon_stat},
    {"PU", "PU", genericWeaponInsert, parser_weapon_stat},
    {"PM", "PM", genericWeaponInsert, parser_weapon_stat},
    {"PA", "PA", genericWeaponInsert, parser_weapon_stat},
    {"P4", "P4", genericWeaponInsert, parser_weapon_stat},
    {"PL", "PL", genericWeaponInsert, parser_weapon_stat},
    {"PK", "PK", genericWeaponInsert, parser_weapon_stat},
    {"PG", "PG", genericWeaponInsert, parser_weapon_stat},
    {"PN", "PN", genericWeaponInsert, parser_weapon_stat},
    {NULL, NULL, NULL, NULL}
};

void weaponInsert(json_t *json, struct parser_s *mdata) {
    // Warning! This add game_player to DB take care if you modify Server msg!

    g_game_player_id = db_add_reg(sql_data.col_value);

    if(g_game_player_id == -1) {
        lprintf("Error adding Player->General!\n");
        return;
    }

    memset(&sql_data, 0, sizeof(sql_data));
}

void genericStatInsert(json_t *json, struct parser_s *mdata) {
    strcpy(sql_data.col_value[sql_data.count].col, "insert");
    strcpy(sql_data.col_value[sql_data.count].value, mdata->sqlname);
    sql_data.count++;

    strcpy(sql_data.col_value[sql_data.count].col, "game_player_id");
    sprintf(sql_data.col_value[sql_data.count].value, "%d", g_game_player_id);
    sql_data.count++;
}

parser_t parser_melee[] = {
    {"MKK", "alevel0_kills", generic, NULL},
    {"MTK", "alevel1_kills", generic, NULL},
    {"MBK", "alevel1_upg_kills", generic, NULL},
    {"MOK", "alevel2_kills", generic, NULL},
    {"MGK", "alevel2_upg_kills", generic, NULL},
    {"MKD", "alevel3_kills", generic, NULL},
    {"MTD", "alevel3_upg_kills", generic, NULL},
    {"MBD", "alevel4_kills", generic, NULL},
    {NULL, NULL, NULL, NULL}
};

parser_t parser_explosions[] = {
    {"EGK", "granade_kills", generic, NULL},
    {NULL, NULL, NULL, NULL}
};

parser_t parser_misc[] = {
    {"IED", "environmental_deaths", generic, NULL},
    {"ISD", "suicides", generic, NULL},
    {NULL, NULL, NULL, NULL}
};

parser_t parser_player[] = {
    {"GENERAL", "game_player", genericInsert, parser_general_stat},
    {"W", "weapon_stat", weaponInsert, parser_weapon},
    {"M", "melee_stat", genericStatInsert, parser_melee},
    {"E", "explosions_stat", genericStatInsert, parser_explosions},
    {"I", "misc_stat", genericStatInsert, parser_misc},
    {NULL, NULL, NULL, NULL}
};

parser_t parser_main [] = {
    {"ST", "game_stat", NULL, NULL},
    {"GAME", "game", genericInsert, parser_game},
    {"STAT", "game_stat", statInsert, parser_stat},
    {"PLAYER", "", NULL, parser_player},
    {NULL, NULL, NULL, NULL}
};

void viewParserJSON(json_t *json, parser_t *pmenu) {
    json_t *next;
    parser_t *next_menu;

    int i = 0;

    level ++;

    if(json == NULL || pmenu == NULL) {
        level --;
        return;
    }

    for(i = 0; i < level; i++) {
        printf("*");
    }

    printf(" Tipo %d Valor %s\n", json->type, json->text);

    if(json->type == 2) {
        next_menu = pmenu;
    } else {
        for(i = 0; pmenu[i].name != NULL &&
                strcmp(pmenu[i].name, json->text) != 0; i++);

        if(pmenu[i].name != NULL) {
            next_menu = pmenu[i].next;

            if(pmenu[i].f != NULL) {
                pmenu[i].f(json, &(pmenu[i]));
            }
        } else {
            level--;
            return;
        }
    }

    for(next = json->child; next != NULL; next = next->next) {
        viewParserJSON(next, next_menu);
    }

    level--;
}

void viewJSON(json_t *json) {
    json_t *next;
    int i = 0;

    level++;

    if(json == NULL) {
        return;
    }

    for(i = 0; i < level * 2; i++) {
        printf("*");
    }

    printf(" Tipo %d Valor %s\n", json->type, json->text);

    for(next = json->child; next != NULL; next = next->next) {
        viewJSON(next);
    }

    level--;
}

int parse_messageJSON(char *msg) {
    json_t *document = NULL;
    int ret;
    int i;
    char *line;
    char *sptr;

    // clear last message
    memset(&sql_data, 0, sizeof(sql_data_t));

#if defined (_WIN32)
    line = strtok_s(msg, "\n\r\0", &sptr);
#else
    line = strtok_r(msg, "\n\r\0", &sptr);
#endif

    while(line != NULL) {
        g_game_id = -1;
        g_server_id = -1;
        g_player_type = -1;
        g_user_id = -1;
        g_game_player_id = -1;

        if((ret = json_parse_document(&document, line)) != JSON_OK) {
            lprintf("JSON ERROR: %d: %s\n", ret, line);
            return -1;
        }

        printf("-------------------------\n");
        viewJSON(document);
        printf("-------------------------\n");
        viewParserJSON(document, parser_main);
        printf("-------------------------\n");

        for(i = 0; i < sql_data.count; i++) {
            printf("Col %s val %s\n", sql_data.col_value[i].col,
                   sql_data.col_value[i].value);
        }

        json_free_value(&document);

        db_add_reg(sql_data.col_value);

#if defined (_WIN32)
        line = strtok_s(NULL, "\n\r\0", &sptr);
#else
        line = strtok_r(NULL, "\n\r\0", &sptr);
#endif
        // clear last message
        memset(&sql_data, 0, sizeof(sql_data_t));
    }

    return 0;
}


int parse_message(char *msg) {
    char *line;
    char *saveptr;


    if(strncmp(msg, "ST:", 3) != 0) {
        lprintf("There is not a valid massage: %s\n", msg);
        return -1;
    }

#if defined (_WIN32)
    line = strtok_s(msg, "\n\r\0", &saveptr);
#else
    line = strtok_r(msg, "\n\r\0", &saveptr);
#endif

    while(line != NULL) {
        lprintf("PARSING: (%s)\n", line);

        if(strlen(line) < 4) {
            lprintf("Error, there is not a valid line! %s\n", line);
        } else {
            switch(line[3]) {
                case 'G':
                    parse_game(line);
                    break;

                case 'S':
                    parse_status(line);
                    break;

                case 'P':
                    parse_player(line);
                    break;

                default:
                    lprintf("There is not a valid option: %c\n", line[3]);
            }
        }

#if defined (_WIN32)
        line = strtok_s(msg, "\n\r\0", &saveptr);
#else
        line = strtok_r(msg, "\n\r\0", &saveptr);
#endif
    }

    return 0;
}

void parse_game(char *msg) {
    char *saveptr;
    char *token;
    col_value_t col_value[100];
    int game_type_id;
    char buffer[1024] = {0};

    lprintf("Parsing game...\n");

    memset(col_value, 0, sizeof(col_value));
#if defined (_WIN32)
    token = strtok_s(msg, "|\n\0", &saveptr);
#else
    token = strtok_r(msg, "|\n\0", &saveptr);
#endif

#if defined (_WIN32)
    token = strtok_s(NULL, "|\n\0", &saveptr);
#else
    token = strtok_r(NULL, "|\n\0", &saveptr);
#endif

    if(token == NULL) {
        lprintf("Error parsing GID:\n");
        return;
    }

    if(strncmp(token, "GID:", 4) == 0) {
        strcpy(col_value[0].col, "gameid");
        strcpy(col_value[0].value, &(token[4]));
    }

#if defined (_WIN32)
    token = strtok_s(NULL, "|\n\0", &saveptr);
#else
    token = strtok_r(NULL, "|\n\0", &saveptr);
#endif

    if(token == NULL) {
        lprintf("Error parsing GID:\n");
        return;
    }

    if(strncmp(token, "MAP:", 4) == 0) {
        strcpy(col_value[1].col, "mapname");
        strcpy(col_value[1].value, &(token[4]));
    }

#if defined (_WIN32)
    token = strtok_s(NULL, "|\n\0", &saveptr);
#else
    token = strtok_r(NULL, "|\n\0", &saveptr);
#endif

    if(token == NULL) {
        lprintf("Error parsing GID:\n");
        return;
    }

    if(strncmp(token, "TME:", 4) == 0) {
        strcpy(col_value[2].col, "start_date");
        sprintf(col_value[2].value, "FROM_UNIXTIME(%s)", &(token[4]));
    }

#if defined (_WIN32)
    token = strtok_s(NULL, "|\n\0", &saveptr);
#else
    token = strtok_r(NULL, "|\n\0", &saveptr);
#endif

    if(token == NULL) {
        lprintf("Error parsing GID:\n");
        return;
    }

    if(strncmp(token, "GTY:", 4) == 0) {
        strcpy(col_value[3].col, "game_type_id");
        sprintf(buffer, "select id from game_type where rel=%c\n", token[4]);

        if((game_type_id = db_get_id_from(buffer)) < 0) {
            return;
        }

        sprintf(col_value[3].value, "%d", game_type_id);
    }

#if defined (_WIN32)
    token = strtok_s(NULL, "|\n\0", &saveptr);
#else
    token = strtok_r(NULL, "|\n\0", &saveptr);
#endif

    if(token == NULL) {
        lprintf("Error parsing GID:\n");
        return;
    }

    if(strncmp(token, "STS:", 4) == 0) {
        strcpy(col_value[4].col, "sts");
        sprintf(col_value[4].value, "FROM_UNIXTIME(%s)", &(token[4]));
    }

    lprintf("End ok\n");
    //db_insert_data("
}

void parse_status(char *msg) {
    char *saveptr;
    char *token;
    col_value_t stat_col_value[100];
    col_value_t game_col_value[100];
    int game_id = -1;

    int game_type_id;
    char buffer[1024] = {0};
    char server_ip[128] = {0};
    int server_id = 0;

    lprintf("Parsing status...\n");

    memset(stat_col_value, 0, sizeof(stat_col_value));

    memset(game_col_value, 0, sizeof(game_col_value));

#if defined (_WIN32)
    token = strtok_s(msg, "|\n\0", &saveptr);
#else
    token = strtok_r(msg, "|\n\0", &saveptr);
#endif

#if defined (_WIN32)
    token = strtok_s(NULL, "|\n\0", &saveptr);
#else
    token = strtok_r(NULL, "|\n\0", &saveptr);
#endif

    if(token == NULL) {
        lprintf("Error parsing PRT:\n");
        return;
    }

    // Serching server ID
    if(strncmp(token, "PRT:", 4) == 0) {
        if(net_get_actual_client_ip(server_ip) == -1) {
            lprintf("Error retriving ip\n");
            return;
        }

        sprintf(buffer, "select id from server where ip='%s' and port='%s'\n",
                server_ip, &(token[4]));

        if((server_id = db_get_id_from(buffer)) < 0) {
            lprintf("Server %s:%s does not exist in DB!\n", server_ip, &(token[4]));
            return;
        }

        lprintf("Server id=%d\n", server_id);

    }

#if defined (_WIN32)
    token = strtok_s(NULL, "|\n\0", &saveptr);
#else
    token = strtok_r(NULL, "|\n\0", &saveptr);
#endif

    if(token == NULL) {
        lprintf("Error parsing GID:\n");
        return;
    }

    if(strncmp(token, "GID:", 4) == 0) {
        sprintf(buffer, "select id from game where gameid='%s'", &(token[4]));

        if((game_id = db_get_id_from(buffer)) < 0) {
            lprintf("New game: %s\n", &(token[4]));
            strcpy(game_col_value[0].col, "gameid");
            strcpy(game_col_value[0].value, &(token[4]));
        }
    }

#if defined (_WIN32)
    token = strtok_s(NULL, "|\n\0", &saveptr);
#else
    token = strtok_r(NULL, "|\n\0", &saveptr);
#endif

    if(token == NULL) {
        lprintf("Error parsing MAP:\n");
        return;
    }

    if(strncmp(token, "MAP:", 4) == 0 && game_id == -1) {
        strcpy(game_col_value[1].col, "mapname");
        strcpy(game_col_value[1].value, &(token[4]));
    }

#if defined (_WIN32)
    token = strtok_s(NULL, "|\n\0", &saveptr);
#else
    token = strtok_r(NULL, "|\n\0", &saveptr);
#endif

    if(token == NULL) {
        lprintf("Error parsing STM:\n");
        return;
    }

    if(strncmp(token, "STM:", 4) == 0 && game_id == -1) {
        strcpy(game_col_value[2].col, "start_date");
        sprintf(game_col_value[2].value, "FROM_UNIXTIME(%s)", &(token[4]));
    }

#if defined (_WIN32)
    token = strtok_s(NULL, "|\n\0", &saveptr);
#else
    token = strtok_r(NULL, "|\n\0", &saveptr);
#endif

    if(token == NULL) {
        lprintf("Error parsing GTY:\n");
        return;
    }

    if(strncmp(token, "GTY:", 4) == 0 && game_id == -1) {
        strcpy(game_col_value[3].col, "game_type_id");
        sprintf(buffer, "select id from game_type where rel=%c\n", token[4]);

        if((game_type_id = db_get_id_from(buffer)) < 0) {
            printf("No se pudo obtener el game_type_id!\n");
            return;
        }

        sprintf(game_col_value[3].value, "%d", game_type_id);
    }

#if defined (_WIN32)
    token = strtok_s(NULL, "|\n\0", &saveptr);
#else
    token = strtok_r(NULL, "|\n\0", &saveptr);
#endif

    if(token == NULL) {
        lprintf("Error parsing STS:\n");
        return;
    }

    if(strncmp(token, "STS:", 4) == 0 && game_id == -1) {
        strcpy(game_col_value[4].col, "sts");
        strcpy(game_col_value[4].value, &(token[4]));
    }

    if(game_id == -1) {
        strcpy(game_col_value[5].col, "server_id");
        sprintf(game_col_value[5].value, "%d", server_id);
        game_col_value[6].col[0] = 0;

        //game_id = db_add_reg("game", game_col_value);
        if(game_id == -1) {
            lprintf("Error! Cannot insert new game!\n");
            return;
        }
    }

    /***************************************
     *   Status parse
     ***************************************/

#if defined (_WIN32)
    token = strtok_s(NULL, "|\n\0", &saveptr);
#else
    token = strtok_r(NULL, "|\n\0", &saveptr);
#endif

    if(token == NULL) {
        lprintf("Error parsing TME:\n");
        return;
    }

    if(strncmp(token, "TME:", 4) == 0) {
        strcpy(stat_col_value[0].col, "time");
        sprintf(stat_col_value[0].value, "FROM_UNIXTIME(%s)", &(token[4]));
    }

#if defined (_WIN32)
    token = strtok_s(NULL, "|\n\0", &saveptr);
#else
    token = strtok_r(NULL, "|\n\0", &saveptr);
#endif

    if(token == NULL) {
        lprintf("Error parsing PLY:\n");
        return;
    }

    if(strncmp(token, "PLY:", 4) == 0) {
        strcpy(stat_col_value[1].col, "players");
        strcpy(stat_col_value[1].value, &(token[4]));
    }

#if defined (_WIN32)
    token = strtok_s(NULL, "|\n\0", &saveptr);
#else
    token = strtok_r(NULL, "|\n\0", &saveptr);
#endif

    if(token == NULL) {
        lprintf("Error parsing CPU:\n");
        return;
    }

    if(strncmp(token, "CPU:", 4) == 0) {
        strcpy(stat_col_value[2].col, "cpu_perc");
        strcpy(stat_col_value[2].value, &(token[4]));
    }

#if defined (_WIN32)
    token = strtok_s(NULL, "|\n\0", &saveptr);
#else
    token = strtok_r(NULL, "|\n\0", &saveptr);
#endif

    if(token == NULL) {
        lprintf("Error parsing MEM:\n");
        return;
    }

    if(strncmp(token, "MEM:", 4) == 0) {
        strcpy(stat_col_value[3].col, "mem_kb");
        strcpy(stat_col_value[3].value, &(token[4]));
    }

    strcpy(stat_col_value[4].col, "game_id");
    sprintf(stat_col_value[4].value, "%d", game_id);

    strcpy(stat_col_value[5].col, "server_id");
    sprintf(stat_col_value[5].value, "%d", server_id);

    stat_col_value[6].col[0] = 0;

    //db_add_reg("game_stat", stat_col_value);

    lprintf("End ok\n");
    //db_insert_data("
}

void parse_player(char *msg) {
    lprintf("Parsing player...\n");
}

int parse_weapons(char *out, char *in) {
    return 0;
}
