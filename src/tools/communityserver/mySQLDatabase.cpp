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
// File name:   mySQLDatabase.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <communityserver.hpp>
#include <string.h>
#include <stdlib.h>

MYSQL g_mysql;

MYSQL* connect_database( char* server, char* user, char* password, char* database )
{

    mysql_init( &g_mysql );
    
    bool reconnect = 1;
    
    mysql_options( &g_mysql, MYSQL_OPT_RECONNECT, &reconnect );
    
    //mysql_options(&mysql,MYSQL_READ_DEFAULT_GROUP,"your_prog_name");
    if( !mysql_real_connect( &g_mysql, server, user, password , database, 0, NULL, 0 ) )
    {
        lprintf( "Failed to connect to database: Error: %s\n", mysql_error( &g_mysql ) );
        return NULL;
    }
    
    return &g_mysql;
}

int db_accept_ip( char* ip )
{
    char buffer[1024];
    MYSQL_RES* res;
    int ret;
    
    sprintf( buffer, "select ip from server where ip='%s'", ip );
    
    if( mysql_real_query( &g_mysql, buffer, strlen( buffer ) ) )
    {
        lprintf( "Failed to select: %s - error %s\n", buffer, mysql_error( &g_mysql ) );
        return -1;
        
    }
    
    res = mysql_store_result( &g_mysql );
    
    ret = mysql_num_rows( res );
    
    mysql_free_result( res );
    
    return ret;
}

int db_get_id_from( char* st )
{
    MYSQL_RES* res;
    MYSQL_ROW row;
    
    if( mysql_real_query( &g_mysql, st, strlen( st ) ) )
    {
        lprintf( "Failed to select: %s - error %s\n", st, mysql_error( &g_mysql ) );
        return -1;
    }
    
    res = mysql_store_result( &g_mysql );
    
    if( res == NULL )
    {
        lprintf( "Error while retriving id: %s\n", st );
        return -1;
    }
    
    row = mysql_fetch_row( res );
    
    if( row == NULL ) return -1;
    
    return atoi( row[0] );
}

int db_add_reg( col_value_t* col_value )
{
    char cols[1024] = {0};
    char vals[1024] = {0};
    char buffer[1024 * 3] = {0};
    char table[1024] = {0};
    int val_count = 0;
    
    int i;
    
    for( i = 0; col_value[i].col[0] != 0; )
    {
        if( strcmp( col_value[i].col, "insert" ) == 0 )
        {
            strcpy( table, col_value[i].value );
            i++;
            memset( cols, 0, sizeof( cols ) );
            memset( vals, 0, sizeof( vals ) );
            
            for( val_count = 0; col_value[i].col[0] != 0 && strcmp( col_value[i].col, "insert" ) != 0; i++, val_count++ )
            {
                if( val_count != 0 ) strcat( cols, ", " );
                
                strcat( cols, col_value[i].col );
                
                if( val_count != 0 ) strcat( vals, ", " );
                strcat( vals, col_value[i].value );
            }
            
            sprintf( buffer, "insert into %s (%s) value (%s)", table, cols, vals );
            lprintf( "******** %s\n", buffer );
            if( mysql_real_query( &g_mysql, buffer, strlen( buffer ) ) )
            {
                lprintf( "Failed to insert: %s - error %s\n", buffer, mysql_error( &g_mysql ) );
                return -1;
            }
        }
        
    }
    
    return mysql_insert_id( &g_mysql );
}

