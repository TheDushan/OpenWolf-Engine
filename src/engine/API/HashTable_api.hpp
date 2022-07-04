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
// File name:   HashTable.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __HASHTABLE_HPP_API__
#define __HASHTABLE_HPP_API__

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

//
// idHashTableSystem
//
class idHashTableSystem {
public:
    virtual hash_table_t *CreateHashTable(uint(*hash_f)(void *k),
                                          sint(*compare)(const void *a, const void *b), void (*destroy_key)(void *a),
                                          void (*destroy_data)(void *a), sint initial_size) = 0;
    virtual hash_node_t *InsertIntoHash(hash_table_t *table, void *key,
                                        void *data) = 0;
    virtual void DeleteFromHash(hash_table_t *table, void *key) = 0;
    virtual hash_node_t *FindHashNode(hash_table_t *table, void *key) = 0;
    virtual void *FindHashData(hash_table_t *table, void *key) = 0;
    virtual void DestroyHash(hash_table_t *table) = 0;
    virtual void RebuildHash(hash_table_t *table, sint new_size) = 0;
    virtual sint StrCmp(const void *a1, const void *a2) = 0;
    virtual void DestroyStringKey(void *s) = 0;
    virtual hash_table_iterator_t *CreateHashIterator(hash_table_t *table) = 0;
    virtual void *HashIterationData(hash_table_iterator_t *iter) = 0;
    virtual uint JenkinsHashKey(void *vkey) = 0;
};

extern idHashTableSystem *hashTableSystem;

#endif //!__HASHTABLE_HPP_API__