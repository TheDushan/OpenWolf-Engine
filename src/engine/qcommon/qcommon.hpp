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
// File name:   qcommon.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description: definitions common between client and server, but not game or ref module
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __QCOMMON_HPP__
#define __QCOMMON_HPP__

//bani
#if defined __GNUC__ || defined __clang__
#define _attribute( x ) __attribute__( x )
#else
#define _attribute( x )
#endif

//#define PRE_RELEASE_DEMO
#ifdef PRE_RELEASE_DEMO
#define PRE_RELEASE_DEMO_NODEVMAP
#endif                          // PRE_RELEASE_DEMO

//============================================================================

valueType *Com_GetTigerHash(valueType *str);

// HASH TABLE
typedef struct hash_node_s {
    void *key;
    void *data;
    struct hash_node_s *next;
} hash_node_t;

typedef struct {
    hash_node_t *list;
    schar16 count;
} hash_data_t;

typedef struct {
    hash_data_t *table;
    sint max_colitions;
    sint size;
    sint used;
    uint(*hash_f)(void *k);
    sint(*compare)(const void *a, const void *b);
    void (*destroy_key)(void *a);
    void (*destroy_data)(void *a);
} hash_table_t;

typedef struct {
    hash_table_t *table;
    hash_node_t *node;
    sint index;
} hash_table_iterator_t;

// HASH FUNCTIONS
hash_table_t *Com_CreateHashTable(uint(*hash_f)(void *k),
                                  sint(*compare)(const void *a, const void *b), void (*destroy_key)(void *a),
                                  void (*destroy_data)(void *a), sint initial_size);
hash_node_t *Com_InsertIntoHash(hash_table_t *table, void *key,
                                void *data);
void Com_DeleteFromHash(hash_table_t *table, void *key);
hash_node_t *Com_FindHashNode(hash_table_t *table, void *key);
void *Com_FindHashData(hash_table_t *table, void *key);
void Com_DestroyHash(hash_table_t *table);
void Com_RebuildHash(hash_table_t *table, sint new_size);
uint Com_JenkinsHashKey(void *vkey);
sint Com_StrCmp(const void *a1, const void *a2);
void Com_DestroyStringKey(void *s);
hash_table_iterator_t *Com_CreateHashIterator(hash_table_t *table);
void *Com_HashIterationData(hash_table_iterator_t *iter);

extern fileHandle_t com_logfile;
extern fileHandle_t com_journalFile;    // events are written here
extern fileHandle_t
com_journalDataFile;    // config files are written here

#endif //!__QCOMMON_HPP__
