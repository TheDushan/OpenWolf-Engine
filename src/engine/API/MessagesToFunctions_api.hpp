////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2021 - 2022 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   MessagesToFunctions_api.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __MESSAGESTOFUNCTIONS_API_HPP__
#define __MESSAGESTOFUNCTIONS_API_HPP__

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

struct usercmd_s;
struct entityState_s;
struct playerState_s;

//
// idMessageToFunctions
//
class idMessageToFunctions {
public:

    virtual void Init(msg_t *buf, uchar8 *data, sint length) = 0;
    virtual void InitOOB(msg_t *buf, uchar8 *data, sint length) = 0;
    virtual void Clear(msg_t *buf) = 0;
    virtual void WriteData(msg_t *buf, const void *data, sint length) = 0;
    virtual void Bitstream(msg_t *buf) = 0;

    // TTimo
    // copy a msg_t in case we need to store it as is for a bit
    // (as I needed this to keep an msg_t from a static var for later use)
    // sets data buffer as MSG_Init does prior to do the copy
    virtual void Copy(msg_t *buf, uchar8 *data, sint length, msg_t *src) = 0;
    virtual void WriteBits(msg_t *msg, sint value, sint bits) = 0;
#if !defined(_DEBUG)
    virtual void WriteByte(msg_t *sb, sint c) = 0;
    virtual void WriteShort(msg_t *sb, sint c) = 0;
#else
#define WriteByte(sb,c) WriteByteDebug( sb, c, __FILE__, __LINE__ )
    virtual void WriteByteDebug(msg_t *sb, sint c, pointer file,
                                sint line) __attribute__((nonnull)) = 0;
#define WriteShort(sb,c) WriteShortDebug( sb, c, __FILE__, __LINE__ )
    virtual void WriteShortDebug(msg_t *sb, sint c, pointer file,
                                 sint line) __attribute__((nonnull)) = 0;
#endif
    virtual void WriteLong(msg_t *sb, sint c) = 0;
    virtual void WriteString(msg_t *sb, pointer s) = 0;
    virtual void WriteBigString(msg_t *sb, pointer s) = 0;
    virtual void BeginReading(msg_t *sb) = 0;
    virtual void BeginReadingOOB(msg_t *sb) = 0;
    virtual sint ReadBits(msg_t *msg, sint bits) = 0;
    virtual sint ReadByte(msg_t *sb) = 0;
    virtual sint ReadShort(msg_t *sb) = 0;
    virtual sint ReadLong(msg_t *sb) = 0;
    virtual valueType *ReadString(msg_t *sb) = 0;
    virtual valueType *ReadBigString(msg_t *sb) = 0;
    virtual valueType *ReadStringLine(msg_t *sb) = 0;
    virtual void ReadData(msg_t *sb, void *buffer, sint size) = 0;
    virtual sint LookaheadByte(msg_t *msg) = 0;
    virtual void WriteDeltaUsercmd(msg_t *msg,
                                   usercmd_t *from,
                                   usercmd_t *to) = 0;
    virtual void  ReadDeltaUsercmd(msg_t *msg,
                                   usercmd_t *from, usercmd_t *to) = 0;
    virtual void WriteDeltaUsercmdKey(msg_t *msg, sint key,
                                      usercmd_t *from, usercmd_t *to) = 0;
    virtual void ReadDeltaUsercmdKey(msg_t *msg, sint key,
                                     usercmd_t *from, usercmd_t *to) = 0;
    virtual void WriteDeltaEntity(msg_t *msg,
                                  struct entityState_s *from, struct entityState_s *to, bool force) = 0;
    virtual void ReadDeltaEntity(msg_t *msg, entityState_t *from,
                                 entityState_t *to, sint number) = 0;
    virtual void WriteDeltaPlayerstate(msg_t *msg,
                                       struct playerState_s *from, struct playerState_s *to) = 0;
    virtual void ReadDeltaPlayerstate(msg_t *msg,
                                      playerState_t *from,
                                      playerState_t *to) = 0;

    virtual sint HashKey(pointer string, sint maxlen) = 0;
    virtual void WriteDelta(msg_t *msg, sint oldV, sint newV,
                            sint bits) = 0;
    virtual sint ReadDelta(msg_t *msg, sint oldV, sint bits) = 0;
    virtual void PrioritiseEntitystateFields(void) = 0;
    virtual void PrioritisePlayerStateFields(void) = 0;
};

extern idMessageToFunctions *msgToFuncSystem;

#endif //!__MESSAGESTOFUNCTIONS_API_HPP__