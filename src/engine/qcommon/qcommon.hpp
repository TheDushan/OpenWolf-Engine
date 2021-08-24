////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: definitions common between client and server, but not game or ref module
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __QCOMMON_H__
#define __QCOMMON_H__

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

//
// msg.c
//
typedef struct msg_s {
    bool        allowoverflow;  // if false, do a common->Error
    bool
    overflowed; // set to true if the buffer size failed (with allowoverflow set)
    bool
    oob;        // set to true if the buffer size failed (with allowoverflow set)
    uchar8           *data;
    sint             maxsize;
    sint             cursize;
    sint             uncompsize;    // NERVE - SMF - net debugging
    sint             readcount;
    sint             bit;       // for bitwise reads and writes
} msg_t;

void            MSG_Init(msg_t *buf, uchar8 *data, sint length);
void            MSG_InitOOB(msg_t *buf, uchar8 *data, sint length);
void            MSG_Clear(msg_t *buf);
void           *MSG_GetSpace(msg_t *buf, sint length);
void            MSG_WriteData(msg_t *buf, const void *data, sint length);
void            MSG_Bitstream(msg_t *buf);
void            MSG_Uncompressed(msg_t *buf);

// TTimo
// copy a msg_t in case we need to store it as is for a bit
// (as I needed this to keep an msg_t from a static var for later use)
// sets data buffer as MSG_Init does prior to do the copy
void            MSG_Copy(msg_t *buf, uchar8 *data, sint length,
                         msg_t *src);

struct usercmd_s;
struct entityState_s;
struct playerState_s;

void            MSG_WriteBits(msg_t *msg, sint value, sint bits);

void            MSG_WriteChar(msg_t *sb, sint c);
void            MSG_WriteByte(msg_t *sb, sint c);
void            MSG_WriteShort(msg_t *sb, sint c);
void            MSG_WriteLong(msg_t *sb, sint c);
void            MSG_WriteFloat(msg_t *sb, float32 f);
void            MSG_WriteString(msg_t *sb, pointer s);
void            MSG_WriteBigString(msg_t *sb, pointer s);
void            MSG_WriteAngle16(msg_t *sb, float32 f);

void            MSG_BeginReading(msg_t *sb);
void            MSG_BeginReadingOOB(msg_t *sb);
void            MSG_BeginReadingUncompressed(msg_t *msg);

sint             MSG_ReadBits(msg_t *msg, sint bits);

sint             MSG_ReadChar(msg_t *sb);
sint             MSG_ReadByte(msg_t *sb);
sint             MSG_ReadShort(msg_t *sb);
sint             MSG_ReadLong(msg_t *sb);
float32           MSG_ReadFloat(msg_t *sb);
valueType           *MSG_ReadString(msg_t *sb);
valueType           *MSG_ReadBigString(msg_t *sb);
valueType           *MSG_ReadStringLine(msg_t *sb);
float32           MSG_ReadAngle16(msg_t *sb);
void            MSG_ReadData(msg_t *sb, void *buffer, sint size);
sint                MSG_LookaheadByte(msg_t *msg);

void            MSG_WriteDeltaUsercmd(msg_t *msg, struct usercmd_s *from,
                                      struct usercmd_s *to);
void            MSG_ReadDeltaUsercmd(msg_t *msg, struct usercmd_s *from,
                                     struct usercmd_s *to);

void            MSG_WriteDeltaUsercmdKey(msg_t *msg, sint key,
        usercmd_t *from, usercmd_t *to);
void            MSG_ReadDeltaUsercmdKey(msg_t *msg, sint key,
                                        usercmd_t *from, usercmd_t *to);

void            MSG_WriteDeltaEntity(msg_t *msg,
                                     struct entityState_s *from, struct entityState_s *to, bool force);
void            MSG_ReadDeltaEntity(msg_t *msg, entityState_t *from,
                                    entityState_t *to, sint number);

void            MSG_WriteDeltaPlayerstate(msg_t *msg,
        struct playerState_s *from, struct playerState_s *to);
void            MSG_ReadDeltaPlayerstate(msg_t *msg,
        struct playerState_s *from, struct playerState_s *to);

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

#endif //!__QCOMMON_H__
