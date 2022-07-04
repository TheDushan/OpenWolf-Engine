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

#ifndef __HASHTABLE_API__
#define __HASHTABLE_API__

//
// idHashTableSystemLocal
//
class idHashTableSystemLocal : public idHashTableSystem {
public:
    idHashTableSystemLocal();
    ~idHashTableSystemLocal();

    virtual hash_table_t *CreateHashTable(uint(*hash_f)(void *k),
                                          sint(*compare)(const void *a, const void *b), void (*destroy_key)(void *a),
                                          void (*destroy_data)(void *a), sint initial_size);
    virtual hash_node_t *InsertIntoHash(hash_table_t *table, void *key,
                                        void *data);
    virtual void DeleteFromHash(hash_table_t *table, void *key);
    virtual hash_node_t *FindHashNode(hash_table_t *table, void *key);
    virtual void *FindHashData(hash_table_t *table, void *key);
    virtual void DestroyHash(hash_table_t *table);
    virtual void RebuildHash(hash_table_t *table, sint new_size);
    virtual sint StrCmp(const void *a1, const void *a2);
    virtual void DestroyStringKey(void *s);
    virtual hash_table_iterator_t *CreateHashIterator(hash_table_t *table);
    virtual void *HashIterationData(hash_table_iterator_t *iter);
    virtual uint JenkinsHashKey(void *vkey);
};

extern idHashTableSystemLocal hashTableSystemLocal;

#endif //!__HASHTABLE_API__