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

#ifdef UPDATE_SERVER
#include <server/serverAutoPrecompiled.hpp>
#elif DEDICATED
#include <server/serverDedPrecompiled.hpp>
#else
#include <framework/precompiled.hpp>
#endif

idHashTableSystemLocal hashTableSystemLocal;
idHashTableSystem *hashTableSystem = &hashTableSystemLocal;

/*
===============
idHashTableSystemLocal::idHashTableSystemLocal
===============
*/
idHashTableSystemLocal::idHashTableSystemLocal(void) {
}

/*
===============
idHashTableSystemLocal::~idHashTableSystemLocal
===============
*/
idHashTableSystemLocal::~idHashTableSystemLocal(void) {
}


/*
===============
idHashTableSystemLocal::CreateHashTable
===============
*/
hash_table_t *idHashTableSystemLocal::CreateHashTable(uint(*hash_f)(
            void *k),
        sint(*compare)(const void *a, const void *b), void (*destroy_key)(void *a),
        void (*destroy_data)(void *a), sint initial_size) {
    hash_table_t *hash_table;

    if((hash_table = (hash_table_t *)malloc(sizeof(hash_table_t))) ==
            nullptr) {
        return nullptr;
    }

    ::memset(hash_table, 0, sizeof(hash_table));

    hash_table->max_colitions = 0;
    hash_table->size = initial_size;
    hash_table->used = 0;
    hash_table->hash_f = hash_f;
    hash_table->compare = compare;
    hash_table->destroy_key = destroy_key;
    hash_table->destroy_data = destroy_data;

    hash_table->table = (hash_data_t *)malloc(sizeof(hash_data_t) *
                        initial_size);

    ::memset(hash_table->table, 0, sizeof(hash_data_t)*initial_size);

    return hash_table;
}

/*
===============
idHashTableSystemLocal::InsertIntoHash
===============
*/
hash_node_t *idHashTableSystemLocal::InsertIntoHash(hash_table_t *table,
        void *key,
        void *data) {
    uint num_key;
    hash_node_t *node;

    num_key = table->hash_f(key) % table->size;

    if(FindHashNode(table, key) != nullptr) {
        return nullptr;
    }

    node = (hash_node_t *)malloc(sizeof(hash_node_t));


    node->key = key;
    node->next = table->table[num_key].list;
    node->data = data;

    table->table[num_key].list = node;
    table->table[num_key].count++;

    if(table->max_colitions < table->table[num_key].count) {
        table->max_colitions = table->table[num_key].count;
    }

    return node;
}

/*
===============
idHashTableSystemLocal::FindHashData
===============
*/
void *idHashTableSystemLocal::FindHashData(hash_table_t *table,
        void *key) {
    hash_node_t *node;

    node = FindHashNode(table, key);

    if(node == nullptr) {
        return nullptr;
    }

    return node->data;
}

/*
===============
idHashTableSystemLocal::FindHashNode
===============
*/
hash_node_t *idHashTableSystemLocal::FindHashNode(hash_table_t *table,
        void *key) {
    uint num_key;
    hash_node_t *node;

    if(table == nullptr) {
        return nullptr;
    }

    num_key = table->hash_f(key) % table->size;


    for(node = table->table[num_key].list;
            node != nullptr && table->compare(node->key, key) != 0;
            node = node->next);

    return node;
}

/*
===============
idHashTableSystemLocal::RebuildHash
===============
*/
void idHashTableSystemLocal::RebuildHash(hash_table_t *table,
        sint new_size) {
    hash_data_t *new_table;
    hash_node_t *node;
    hash_node_t *node_next;
    hash_data_t *old_table;
    sint old_size;
    sint i;

    new_table = (hash_data_t *)malloc(sizeof(hash_data_t) * new_size);

    memset(new_table, 0, sizeof(hash_data_t) * new_size);

    old_table = table->table;
    old_size = table->size;
    table->table = new_table;
    table->size = new_size;
    table->max_colitions = 0;

    for(i = 0; i < old_size; i++) {
        node = old_table[i].list;

        while(node != nullptr) {
            InsertIntoHash(table, node->key, node->data);
            node_next = node->next;
            free(node);
            node = node_next;
        }
    }

    free(old_table);
}

/*
===============
idHashTableSystemLocal::DeleteFromHash
===============
*/
void idHashTableSystemLocal::DeleteFromHash(hash_table_t *table,
        void *key) {
    uint num_key;
    hash_node_t *node;
    hash_node_t *prev_node;

    num_key = table->hash_f(key) % table->size;

    for(prev_node = node = table->table[num_key].list;
            node != nullptr && table->compare(node->key, key) != 0;
            prev_node = node, node = node->next);

    if(node == nullptr) {
        return;
    } else if(prev_node == node) {
        table->table[num_key].list = node->next;
    } else {
        prev_node->next = node->next;
    }

    table->destroy_key(node->key);
    table->destroy_data(node->data);
    free(node);
}

/*
===============
idHashTableSystemLocal::DestroyHash
===============
*/
void idHashTableSystemLocal::DestroyHash(hash_table_t *table) {
    hash_node_t *node;
    hash_node_t *next_node;
    sint i;

    for(i = 0; i < table->size; i++) {
        node = table->table[i].list;

        while(node != nullptr) {
            next_node = node->next;
            table->destroy_key(node->key);
            table->destroy_data(node->data);
            free(node);
            node = next_node;
        }
    }

    free(table->table);
    free(table);
}

/*
===============
idHashTableSystemLocal::JenkinsHashKey
===============
*/
uint idHashTableSystemLocal::JenkinsHashKey(void *vkey) {
    uint hash = 0;
    uint64 i;
    uchar8 *key;
    sint key_len;

    key = static_cast< uchar8 *>(vkey);

    key_len = ::strlen(reinterpret_cast< pointer >(key));

    for(i = 0; i < key_len; i++) {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

/*
===============
idHashTableSystemLocal::
===============
*/
sint idHashTableSystemLocal::StrCmp(const void *a1, const void *a2) {
    valueType *s1, *s2;
    s1 = (const_cast<valueType *>(reinterpret_cast<pointer>(a1)));
    s2 = (const_cast<valueType *>(reinterpret_cast<pointer>(a2)));

    if(strcmp(s1, s2) < 0) {
        return -1;
    }

    if(strcmp(s1, s2) == 0) {
        return 0;
    }

    return 1;
}

/*
===============
idHashTableSystemLocal::DestroyStringKey
===============
*/
void idHashTableSystemLocal::DestroyStringKey(void *s) {
    free(static_cast< valueType *>(s));
}

/*
===============
idHashTableSystemLocal::CreateHashIterator
===============
*/
hash_table_iterator_t *idHashTableSystemLocal::CreateHashIterator(
    hash_table_t *table) {
    hash_table_iterator_t *iter;
    iter = (hash_table_iterator_t *)::malloc(sizeof(hash_table_iterator_t));
    memset(iter, 0, sizeof(hash_table_iterator_t));
    iter->table = table;
    iter->node = nullptr;
    iter->index = -1;
    return iter;
}

/*
===============
idHashTableSystemLocal::HashIterationData
===============
*/
void *idHashTableSystemLocal::HashIterationData(hash_table_iterator_t
        *iter) {
    void *data;

    // start point
    if(iter->index == -1) {
        iter->index = 0;
        iter->node = nullptr;
    }

    // If there are not more nodes in the table position goes for the next one
    if(iter->node == nullptr) {
        iter->index++;

        for(;   iter->index < iter->table->size &&
                iter->table->table[iter->index].list == nullptr;
                iter->index++);

        // End of table
        if(iter->table->size <= iter->index) {
            iter->index = -1;
            return nullptr;
        }

        iter->node = iter->table->table[iter->index].list;
    }

    data = iter->node->data;
    iter->node = iter->node->next;

    return data;
}
