////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2022 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   MessagesToFunctions.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __MESSAGESTOFUNCTIONS_HPP__
#define __MESSAGESTOFUNCTIONS_HPP__

//
// idMessageToFunctions
//
class idMessageToFunctionsLocal : public idMessageToFunctions {
public:

    idMessageToFunctionsLocal();
    ~idMessageToFunctionsLocal();

    virtual void Init(msg_t *buf, uchar8 *data, sint length);
    virtual void InitOOB(msg_t *buf, uchar8 *data, sint length);
    virtual void Clear(msg_t *buf);
    virtual void WriteData(msg_t *buf, const void *data, sint length);
    virtual void Bitstream(msg_t *buf);

    // TTimo
    // copy a msg_t in case we need to store it as is for a bit
    // (as I needed this to keep an msg_t from a static var for later use)
    // sets data buffer as MSG_Init does prior to do the copy
    virtual void Copy(msg_t *buf, uchar8 *data, sint length, msg_t *src);
    virtual void WriteBits(msg_t *msg, sint value, sint bits);
#if !defined(_DEBUG)
    virtual void WriteByte(msg_t *sb, sint c);
    virtual void WriteShort(msg_t *sb, sint c);
#else
#define WriteByte(sb,c) WriteByteDebug( sb, c, __FILE__, __LINE__ )
    virtual void WriteByteDebug(msg_t *sb, sint c, pointer file,
                                sint line) __attribute__((nonnull));
#define WriteShort(sb,c) WriteShortDebug( sb, c, __FILE__, __LINE__ )
    virtual void WriteShortDebug(msg_t *sb, sint c, pointer file,
                                 sint line) __attribute__((nonnull));
#endif
    virtual void WriteLong(msg_t *sb, sint c);
    virtual void WriteString(msg_t *sb, pointer s);
    virtual void WriteBigString(msg_t *sb, pointer s);
    virtual void BeginReading(msg_t *sb);
    virtual void BeginReadingOOB(msg_t *sb);
    virtual sint ReadBits(msg_t *msg, sint bits);
    virtual sint ReadByte(msg_t *sb);
    virtual sint ReadShort(msg_t *sb);
    virtual sint ReadLong(msg_t *sb);
    virtual valueType *ReadString(msg_t *sb);
    virtual valueType *ReadBigString(msg_t *sb);
    virtual valueType *ReadStringLine(msg_t *sb);
    virtual void ReadData(msg_t *sb, void *buffer, sint size);
    virtual sint LookaheadByte(msg_t *msg);
    virtual void WriteDeltaUsercmd(msg_t *msg,
                                   usercmd_t *from,
                                   usercmd_t *to);
    virtual void  ReadDeltaUsercmd(msg_t *msg,
                                   usercmd_t *from, usercmd_t *to);
    virtual void WriteDeltaUsercmdKey(msg_t *msg, sint key,
                                      usercmd_t *from, usercmd_t *to);
    virtual void ReadDeltaUsercmdKey(msg_t *msg, sint key,
                                     usercmd_t *from, usercmd_t *to);
    virtual void WriteDeltaEntity(msg_t *msg,
                                  struct entityState_s *from, struct entityState_s *to, bool force);
    virtual void ReadDeltaEntity(msg_t *msg, entityState_t *from,
                                 entityState_t *to, sint number);
    virtual void WriteDeltaPlayerstate(msg_t *msg,
                                       struct playerState_s *from, struct playerState_s *to);
    virtual void ReadDeltaPlayerstate(msg_t *msg,
                                      playerState_t *from,
                                      playerState_t *to);

    virtual sint HashKey(pointer string, sint maxlen);
    virtual void WriteDelta(msg_t *msg, sint oldV, sint newV,
                            sint bits);
    virtual sint ReadDelta(msg_t *msg, sint oldV, sint bits);
    virtual void PrioritiseEntitystateFields(void);
    virtual void PrioritisePlayerStateFields(void);

    static void WriteDeltaFloat(msg_t *msg, float32 oldV, float32 newV);
    static float32 ReadDeltaFloat(msg_t *msg, float32 oldV);
    static void WriteDeltaKey(msg_t *msg, sint key, sint oldV,
                              sint newV,
                              sint bits);
    static sint ReadDeltaKey(msg_t *msg, sint key, sint oldV,
                             sint bits);
    static void WriteDeltaKeyFloat(msg_t *msg, sint key,
                                   float32 oldV,
                                   float32 newV);
    static float32 ReadDeltaKeyFloat(msg_t *msg, sint key,
                                     float32 oldV);
    static void ReportChangeVectors_f(void);
};

extern idMessageToFunctionsLocal msgToFuncLocalSystem;

#endif //!__MESSAGESTOFUNCTIONS_HPP__