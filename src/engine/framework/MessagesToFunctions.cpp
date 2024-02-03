////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2023 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   MessagesToFunctions.cpp
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

sint pcount[256];
sint wastedbits = 0;

sint oldsize = 0;

idMessageToFunctionsLocal msgToFuncLocalSystem;
idMessageToFunctions *msgToFuncSystem = &msgToFuncLocalSystem;

/*
===============
idMessageToFunctionsLocal::idMessageToFunctionsLocal
===============
*/
idMessageToFunctionsLocal::idMessageToFunctionsLocal(void) {
}

/*
===============
idMessageToFunctionsLocal::~idMessageToFunctionsLocal
===============
*/
idMessageToFunctionsLocal::~idMessageToFunctionsLocal(void) {
}


// static sint overflows = 0;

/*
==============================================================================

            MESSAGE IO FUNCTIONS

Handles byte ordering and avoids alignment errors
==============================================================================
*/

void idMessageToFunctionsLocal::Init(msg_t *buf, uchar8 *data,
                                     sint length) {
    ::memset(buf, 0, sizeof(*buf));
    //bani - optimization
    //  memset (data, 0, length);
    buf->data = data;
    buf->maxsize = length;
}

void idMessageToFunctionsLocal::InitOOB(msg_t *buf, uchar8 *data,
                                        sint length) {
    ::memset(buf, 0, sizeof(*buf));
    //bani - optimization
    //  memset (data, 0, length);
    buf->data = data;
    buf->maxsize = length;
    buf->oob = true;
}

void idMessageToFunctionsLocal::Clear(msg_t *buf) {
    buf->cursize = 0;
    buf->overflowed = false;
    buf->bit = 0;               //<- in bits
}


void idMessageToFunctionsLocal::Bitstream(msg_t *buf) {
    buf->oob = false;
}

void idMessageToFunctionsLocal::BeginReading(msg_t *msg) {
    msg->readcount = 0;
    msg->bit = 0;
    msg->oob = false;
}

void idMessageToFunctionsLocal::BeginReadingOOB(msg_t *msg) {
    msg->readcount = 0;
    msg->bit = 0;
    msg->oob = true;
}

void idMessageToFunctionsLocal::Copy(msg_t *buf, uchar8 *data, sint length,
                                     msg_t *src) {
    if(length < src->cursize) {
        common->Error(ERR_DROP,
                      "idMessageToFunctionsLocal::Copy: can't copy %d into a smaller %d msg_t buffer",
                      src->cursize,
                      length);
    }

    ::memcpy(buf, src, sizeof(msg_t));
    buf->data = data;
    ::memcpy(buf->data, src->data, src->cursize);
}

/*
=============================================================================

bit functions

=============================================================================
*/

sint    overflows;

// negative bit values include signs
void idMessageToFunctionsLocal::WriteBits(msg_t *msg, sint value,
        sint bits) {
    sint i, bitIndex;

    oldsize += bits;

    msg->uncompsize += bits;    // NERVE - SMF - net debugging

    // this isn't an exact overflow check, but close enough
    if(msg->maxsize - msg->cursize < 32) {
        msg->overflowed = true;
        return;
    }

    if(bits == 0 || bits < -31 || bits > 32) {
        common->Error(ERR_DROP,
                      "idMessageToFunctionsLocal::WriteBits: bad bits %i",
                      bits);
    }

    // TTimo - the overflow count is not used anywhere atm
#if 1

    // check for overflows
    if(bits != 32) {
        if(bits > 0) {
            if(value > ((1 << bits) - 1) || value < 0) {
                overflows++;
            }
        } else {
            sint             r;

            r = 1 << (bits - 1);

            if(value > r - 1 || value < -r) {
                overflows++;
            }
        }
    }

#endif

    if(bits < 0) {
        bits = -bits;
    }

    if(msg->oob) {
        if(bits == 8) {
            msg->data[msg->cursize] = value;
            msg->cursize += 1;
            msg->bit += 8;
        } else if(bits == 16) {
            uchar16 *sp = reinterpret_cast< uchar16 * >(&msg->data[msg->cursize]);

            *sp = LittleShort(value);
            msg->cursize += 2;
            msg->bit += 16;
        } else if(bits == 32) {
            uint   *ip = reinterpret_cast<uint *>(&msg->data[msg->cursize]);

            *ip = LittleLong(value);
            msg->cursize += 4;
            msg->bit += 8;
        } else {
            common->Error(ERR_DROP, "can't read %d bits\n", bits);
        }
    } else {
        //      fp = fopen("c:\\netchan.bin", "a");
        value &= (0xffffffff >> (32 - bits));

        if(bits & 7) {
            sint             nbits;

            nbits = bits & 7;
            bitIndex = msg->bit;

            for(i = 0; i < nbits; i++) {
                idHuffmanSystemLocal::WriteBit((value & 1), msg->data, bitIndex);
                value = (value >> 1);
                bitIndex++;
            }

            msg->bit = bitIndex;
            bits = bits - nbits;
        }

        if(bits) {
            bitIndex = msg->bit;

            for(i = 0; i < bits; i += 8) {
                bitIndex += idHuffmanSystemLocal::WriteSymbol((value & 0xff), msg->data,
                            bitIndex);
                value = (value >> 8);
            }

            msg->bit = bitIndex;
        }

        msg->cursize = (msg->bit >> 3) + 1;
    }
}

sint idMessageToFunctionsLocal::ReadBits(msg_t *msg, sint bits) {
    sint i, nbits, bitIndex, value, get;
    bool sgn;

    value = 0;

    if(bits < 0) {
        bits = -bits;
        sgn = true;
    } else {
        sgn = false;
    }

    if(msg->oob) {
        if(bits == 8) {
            value = msg->data[msg->readcount];
            msg->readcount += 1;
            msg->bit += 8;
        } else if(bits == 16) {
            uchar16 *sp = reinterpret_cast<uchar16 *>(&msg->data[msg->readcount]);

            value = LittleShort(*sp);
            msg->readcount += 2;
            msg->bit += 16;
        } else if(bits == 32) {
            uint *ip = reinterpret_cast<uint *>(&msg->data[msg->readcount]);

            value = LittleLong(*ip);
            msg->readcount += 4;
            msg->bit += 32;
        } else {
            common->Error(ERR_DROP, "can't read %d bits\n", bits);
        }
    } else {
        nbits = 0;

        if(bits & 7) {
            nbits = bits & 7;
            bitIndex = msg->bit;

            for(i = 0; i < nbits; i++) {
                value |= idHuffmanSystemLocal::ReadBit(msg->data, bitIndex) << i;
                bitIndex++;
            }

            msg->bit = bitIndex;
            bits = bits - nbits;
        }

        if(bits) {
            bitIndex = msg->bit;

            for(i = 0; i < bits; i += 8) {
                bitIndex += idHuffmanSystemLocal::ReadSymbol(&get, msg->data, bitIndex);
                value |= (get << (i + nbits));
            }

            msg->bit = bitIndex;
        }

        msg->readcount = (msg->bit >> 3) + 1;
    }

    if(sgn) {
        if(value & (1 << (bits - 1))) {
            value |= -1 ^ ((1 << bits) - 1);
        }
    }

    return value;
}



//================================================================================

//
// writing functions
//

#if defined(_DEBUG)
#define PARANOID 1
#endif

#if defined(_DEBUG)
void idMessageToFunctionsLocal::WriteByteDebug(msg_t *sb, sint c,
        pointer file,
        sint line) {
#else
void idMessageToFunctionsLocal::WriteByte(msg_t *sb, sint c) {
#endif
#ifdef PARANOID

    if(c < 0 || c > 255) {
        common->Error(ERR_FATAL, "Interface call from '%s' line %i\n", file, line);
    }

#endif

    WriteBits(sb, c, 8);
}

void idMessageToFunctionsLocal::WriteData(msg_t *buf, const void *data,
        sint length) {
    sint             i;

    for(i = 0; i < length; i++) {
        WriteByte(buf, (const_cast<uchar8 *>(reinterpret_cast<const uchar8 *>
                                             (data)))[i]);
    }
}

#if defined(_DEBUG)
void idMessageToFunctionsLocal::WriteShortDebug(msg_t *sb, sint c,
        pointer file,
        sint line) {
#else
void idMessageToFunctionsLocal::WriteShort(msg_t *sb, sint c) {
#endif
#ifdef PARANOID

    if(c < ((schar16)0x8000) || c > (schar16)0x7fff) {
        common->Error(ERR_FATAL,
                      "idMessageToFunctionsLocal::WriteShort: range error",
                      file, line);;
    }

#endif

    WriteBits(sb, c, 16);
}

void idMessageToFunctionsLocal::WriteLong(msg_t *sb, sint c) {
    WriteBits(sb, c, 32);
}

void idMessageToFunctionsLocal::WriteString(msg_t *sb, pointer s) {
    if(!s) {
        WriteData(sb, "", 1);
    } else {
        uint64 l, i;
        valueType            string[MAX_STRING_CHARS];

        l = strlen(s);

        if(l >= MAX_STRING_CHARS) {
            common->Printf("idMessageToFunctionsLocal::WriteString: MAX_STRING_CHARS");
            WriteData(sb, "", 1);
            return;
        }

        Q_strncpyz(string, s, sizeof(string));

        // get rid of 0xff chars, because old clients don't like them
        for(i = 0; i < l; i++) {
            if((reinterpret_cast<uchar8 *>(string))[i] > 127) {
                string[i] = '.';
            }
        }

        WriteData(sb, string, l + 1);
    }
}

void idMessageToFunctionsLocal::WriteBigString(msg_t *sb, pointer s) {
    if(!s) {
        WriteData(sb, "", 1);
    } else {
        uint64 l, i;
        valueType            string[BIG_INFO_STRING];

        l = strlen(s);

        if(l >= BIG_INFO_STRING) {
            common->Printf("idMessageToFunctionsLocal::WriteString: BIG_INFO_STRING");
            WriteData(sb, "", 1);
            return;
        }

        Q_strncpyz(string, s, sizeof(string));

        // get rid of 0xff chars, because old clients don't like them
        for(i = 0; i < l; i++) {
            if((reinterpret_cast<uchar8 *>(string))[i] > 127) {
                string[i] = '.';
            }
        }

        WriteData(sb, string, l + 1);
    }
}

//
// reading functions
//

sint idMessageToFunctionsLocal::LookaheadByte(msg_t *msg) {
    const sint bloc = idHuffmanSystemLocal::getBloc();
    const sint readcount = msg->readcount;
    const sint bit = msg->bit;
    sint c = ReadByte(msg);
    idHuffmanSystemLocal::setBloc(bloc);
    msg->readcount = readcount;
    msg->bit = bit;
    return c;
}

sint idMessageToFunctionsLocal::ReadByte(msg_t *msg) {
    sint             c;

    c = static_cast<uchar8>(ReadBits(msg, 8));

    if(msg->readcount > msg->cursize) {
        c = -1;
    }

    return c;
}

sint idMessageToFunctionsLocal::ReadShort(msg_t *msg) {
    sint             c;

    c = (schar16)ReadBits(msg, 16);

    if(msg->readcount > msg->cursize) {
        c = -1;
    }

    return c;
}

sint idMessageToFunctionsLocal::ReadLong(msg_t *msg) {
    sint             c;

    c = ReadBits(msg, 32);

    if(msg->readcount > msg->cursize) {
        c = -1;
    }

    return c;
}

valueType *idMessageToFunctionsLocal::ReadString(msg_t *msg) {
    static valueType string[MAX_STRING_CHARS];
    sint l, c;

    l = 0;

    do {
        // use ReadByte so -1 is out of bounds
        c = ReadByte(msg);

        // skip these to avoid security problems
        if(c == 255) {
            continue;
        }

        if(c == -1 || c == 0) {
            break;
        }

        // translate all format specs to avoid crash bugs
        // don't allow higher ascii values
        if(c == '%' || c > 127) {
            c = '.';
        }

        // break only after reading all expected data from bitstream
        if(l >= sizeof(string) - 1) {
            break;
        }

        string[l++] = c;
    } while(true);

    string[l] = 0;

    return string;
}

valueType *idMessageToFunctionsLocal::ReadBigString(msg_t *msg) {
    static valueType string[BIG_INFO_STRING];
    sint l, c;

    l = 0;

    do {
        // use ReadByte so -1 is out of bounds
        c = ReadByte(msg);

        // skip these to avoid security problems
        if(c == 255) {
            continue;
        }

        if(c == -1 || c == 0) {
            break;
        }

        // translate all format specs to avoid crash bugs
        // don't allow higher ascii values
        if(c == '%' || c > 127) {
            c = '.';
        }

        // break only after reading all expected data from bitstream
        if(l >= sizeof(string) - 1) {
            break;
        }

        string[l++] = c;
    } while(true);

    string[l] = '\0';

    return string;
}

valueType *idMessageToFunctionsLocal::ReadStringLine(msg_t *msg) {
    static valueType string[MAX_STRING_CHARS];
    sint l, c;

    l = 0;

    do {
        // use ReadByte so -1 is out of bounds
        c = ReadByte(msg);

        // skip these to avoid security problems
        if(c == 255) {
            continue;
        }

        if(c == -1 || c == 0 || c == '\n') {
            break;
        }

        // translate all format specs to avoid crash bugs
        // don't allow higher ascii values
        if(c == '%' || c > 127) {
            c = '.';
        }

        // break only after reading all expected data from bitstream
        if(l >= sizeof(string) - 1) {
            break;
        }

        string[l++] = c;
    } while(true);

    string[l] = '\0';

    return string;
}

void idMessageToFunctionsLocal::ReadData(msg_t *msg, void *data,
        sint len) {
    sint             i;

    for(i = 0; i < len; i++) {
        (reinterpret_cast<uchar8 *>(data))[i] = ReadByte(msg);
    }
}

// a string hasher which gives the same hash value even if the
// string is later modified via the legacy MSG read/write code
sint idMessageToFunctionsLocal::HashKey(pointer string, sint maxlen) {
    sint hash, i;

    hash = 0;

    for(i = 0; i < maxlen && string[i] != '\0'; i++) {
        if(string[i] & 0x80 || string[i] == '%') {
            hash += '.' * (119 + i);
        } else {
            hash += string[i] * (119 + i);
        }
    }

    hash = (hash ^ (hash >> 10) ^ (hash >> 20));
    return hash;
}


/*
=============================================================================

delta functions

=============================================================================
*/


#define LOG( x ) if ( cl_shownet && cl_shownet->integer == 4 ) { common->Printf( "%s ", x ); };

void idMessageToFunctionsLocal::WriteDelta(msg_t *msg, sint oldV,
        sint newV,
        sint bits) {
    if(oldV == newV) {
        WriteBits(msg, 0, 1);
        return;
    }

    WriteBits(msg, 1, 1);
    WriteBits(msg, newV, bits);
}

sint idMessageToFunctionsLocal::ReadDelta(msg_t *msg, sint oldV,
        sint bits) {
    if(ReadBits(msg, 1)) {
        return ReadBits(msg, bits);
    }

    return oldV;
}

void idMessageToFunctionsLocal::WriteDeltaFloat(msg_t *msg, float32 oldV,
        float32 newV) {
    if(oldV == newV) {
        msgToFuncLocalSystem.WriteBits(msg, 0, 1);
        return;
    }

    msgToFuncLocalSystem.WriteBits(msg, 1, 1);
    msgToFuncLocalSystem.WriteBits(msg, *reinterpret_cast<sint *>(&newV), 32);
}

float32 idMessageToFunctionsLocal::ReadDeltaFloat(msg_t *msg,
        float32 oldV) {
    if(msgToFuncLocalSystem.ReadBits(msg, 1)) {
        float32           newV;

        *reinterpret_cast<sint *>(&newV) = msgToFuncLocalSystem.ReadBits(msg, 32);
        return newV;
    }

    return oldV;
}

/*
=============================================================================

delta functions with keys

=============================================================================
*/

uint kbitmask[32] = {
    0x00000001, 0x00000003, 0x00000007, 0x0000000F,
    0x0000001F, 0x0000003F, 0x0000007F, 0x000000FF,
    0x000001FF, 0x000003FF, 0x000007FF, 0x00000FFF,
    0x00001FFF, 0x00003FFF, 0x00007FFF, 0x0000FFFF,
    0x0001FFFF, 0x0003FFFF, 0x0007FFFF, 0x000FFFFF,
    0x001FFFFf, 0x003FFFFF, 0x007FFFFF, 0x00FFFFFF,
    0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF,
    0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF,
};

void idMessageToFunctionsLocal::WriteDeltaKey(msg_t *msg, sint key,
        sint oldV,
        sint newV,
        sint bits) {
    if(oldV == newV) {
        msgToFuncLocalSystem.WriteBits(msg, 0, 1);
        return;
    }

    msgToFuncLocalSystem.WriteBits(msg, 1, 1);
    msgToFuncLocalSystem.WriteBits(msg, newV ^ key, bits);
}

sint idMessageToFunctionsLocal::ReadDeltaKey(msg_t *msg, sint key,
        sint oldV,
        sint bits) {
    if(msgToFuncLocalSystem.ReadBits(msg, 1)) {
        return msgToFuncLocalSystem.ReadBits(msg,
                                             bits) ^ (key & kbitmask[bits - 1]);
    }

    return oldV;
}

void idMessageToFunctionsLocal::WriteDeltaKeyFloat(msg_t *msg, sint key,
        float32 oldV,
        float32 newV) {
    if(oldV == newV) {
        msgToFuncLocalSystem.WriteBits(msg, 0, 1);
        return;
    }

    msgToFuncLocalSystem.WriteBits(msg, 1, 1);
    msgToFuncLocalSystem.WriteBits(msg,
                                   (*reinterpret_cast<sint *>(&newV)) ^ key, 32);
}

float32 idMessageToFunctionsLocal::ReadDeltaKeyFloat(msg_t *msg, sint key,
        float32 oldV) {
    if(msgToFuncLocalSystem.ReadBits(msg, 1)) {
        float32 newV;

        *reinterpret_cast<sint *>(&newV) = msgToFuncLocalSystem.ReadBits(msg,
                                           32) ^ key;
        return newV;
    }

    return oldV;
}


/*
============================================================================

usercmd_t communication

============================================================================
*/

// ms is allways sent, the others are optional
#define CM_ANGLE1   ( 1 << 0 )
#define CM_ANGLE2   ( 1 << 1 )
#define CM_ANGLE3   ( 1 << 2 )
#define CM_FORWARD  ( 1 << 3 )
#define CM_SIDE     ( 1 << 4 )
#define CM_UP       ( 1 << 5 )
#define CM_BUTTONS  ( 1 << 6 )
#define CM_WEAPON   ( 1 << 7 )

/*
=====================
idMessageToFunctionsLocal::WriteDeltaUsercmd
=====================
*/
void idMessageToFunctionsLocal::WriteDeltaUsercmd(msg_t *msg,
        usercmd_t *from,
        usercmd_t *to) {
    if(to->serverTime - from->serverTime < 256) {
        WriteBits(msg, 1, 1);
        WriteBits(msg, to->serverTime - from->serverTime, 8);
    } else {
        WriteBits(msg, 0, 1);
        WriteBits(msg, to->serverTime, 32);
    }

    WriteDelta(msg, from->angles[0], to->angles[0], 16);
    WriteDelta(msg, from->angles[1], to->angles[1], 16);
    WriteDelta(msg, from->angles[2], to->angles[2], 16);
    WriteDelta(msg, from->forwardmove, to->forwardmove, 8);
    WriteDelta(msg, from->rightmove, to->rightmove, 8);
    WriteDelta(msg, from->upmove, to->upmove, 8);
    WriteDelta(msg, from->buttons, to->buttons, 8);
    WriteDelta(msg, from->wbuttons, to->wbuttons, 8);
    WriteDelta(msg, from->weapon, to->weapon, 8);
    WriteDelta(msg, from->flags, to->flags, 8);
    WriteDelta(msg, from->doubleTap, to->doubleTap, 3);
    WriteDelta(msg, from->identClient, to->identClient,
               8);      // NERVE - SMF
}


/*
=====================
idMessageToFunctionsLocal::ReadDeltaUsercmd
=====================
*/
void idMessageToFunctionsLocal::ReadDeltaUsercmd(msg_t *msg,
        usercmd_t *from, usercmd_t *to) {
    if(ReadBits(msg, 1)) {
        to->serverTime = from->serverTime + ReadBits(msg, 8);
    } else {
        to->serverTime = ReadBits(msg, 32);
    }

    to->angles[0] = ReadDelta(msg, from->angles[0], 16);
    to->angles[1] = ReadDelta(msg, from->angles[1], 16);
    to->angles[2] = ReadDelta(msg, from->angles[2], 16);
    to->forwardmove = ReadDelta(msg, from->forwardmove, 8);
    to->rightmove = ReadDelta(msg, from->rightmove, 8);
    to->upmove = ReadDelta(msg, from->upmove, 8);
    to->buttons = ReadDelta(msg, from->buttons, 8);
    to->wbuttons = ReadDelta(msg, from->wbuttons, 8);
    to->weapon = ReadDelta(msg, from->weapon, 8);
    to->flags = ReadDelta(msg, from->flags, 8);
    to->doubleTap = ReadDelta(msg, from->doubleTap, 3) & 0x7;
    to->identClient = ReadDelta(msg, from->identClient,
                                8);      // NERVE - SMF
}

/*
=====================
idMessageToFunctionsLocal::WriteDeltaUsercmd
=====================
*/
void idMessageToFunctionsLocal::WriteDeltaUsercmdKey(msg_t *msg, sint key,
        usercmd_t *from,
        usercmd_t *to) {
    if(to->serverTime - from->serverTime < 256) {
        WriteBits(msg, 1, 1);
        WriteBits(msg, to->serverTime - from->serverTime, 8);
    } else {
        WriteBits(msg, 0, 1);
        WriteBits(msg, to->serverTime, 32);
    }

    if(from->angles[0] == to->angles[0] &&
            from->angles[1] == to->angles[1] &&
            from->angles[2] == to->angles[2] &&
            from->forwardmove == to->forwardmove &&
            from->rightmove == to->rightmove &&
            from->upmove == to->upmove &&
            from->buttons == to->buttons &&
            from->wbuttons == to->wbuttons &&
            from->weapon == to->weapon &&
            from->flags == to->flags && from->doubleTap == to->doubleTap &&
            from->identClient == to->identClient) {
        // NERVE - SMF
        WriteBits(msg, 0, 1);    // no change
        oldsize += 7;
        return;
    }

    key ^= to->serverTime;
    WriteBits(msg, 1, 1);
    WriteDeltaKey(msg, key, from->angles[0], to->angles[0], 16);
    WriteDeltaKey(msg, key, from->angles[1], to->angles[1], 16);
    WriteDeltaKey(msg, key, from->angles[2], to->angles[2], 16);
    WriteDeltaKey(msg, key, from->forwardmove, to->forwardmove, 8);
    WriteDeltaKey(msg, key, from->rightmove, to->rightmove, 8);
    WriteDeltaKey(msg, key, from->upmove, to->upmove, 8);
    WriteDeltaKey(msg, key, from->buttons, to->buttons, 8);
    WriteDeltaKey(msg, key, from->wbuttons, to->wbuttons, 8);
    WriteDeltaKey(msg, key, from->weapon, to->weapon, 8);
    WriteDeltaKey(msg, key, from->flags, to->flags, 8);
    WriteDeltaKey(msg, key, from->doubleTap, to->doubleTap, 3);
    WriteDeltaKey(msg, key, from->identClient, to->identClient,
                  8);      // NERVE - SMF
}


/*
=====================
idMessageToFunctionsLocal::ReadDeltaUsercmd
=====================
*/
void idMessageToFunctionsLocal::ReadDeltaUsercmdKey(msg_t *msg, sint key,
        usercmd_t *from,
        usercmd_t *to) {
    if(ReadBits(msg, 1)) {
        to->serverTime = from->serverTime + ReadBits(msg, 8);
    } else {
        to->serverTime = ReadBits(msg, 32);
    }

    if(ReadBits(msg, 1)) {
        key ^= to->serverTime;
        to->angles[0] = ReadDeltaKey(msg, key, from->angles[0], 16);
        to->angles[1] = ReadDeltaKey(msg, key, from->angles[1], 16);
        to->angles[2] = ReadDeltaKey(msg, key, from->angles[2], 16);
        to->forwardmove = ReadDeltaKey(msg, key, from->forwardmove, 8);
        to->rightmove = ReadDeltaKey(msg, key, from->rightmove, 8);
        to->upmove = ReadDeltaKey(msg, key, from->upmove, 8);
        to->buttons = ReadDeltaKey(msg, key, from->buttons, 8);
        to->wbuttons = ReadDeltaKey(msg, key, from->wbuttons, 8);
        to->weapon = ReadDeltaKey(msg, key, from->weapon, 8);
        to->flags = ReadDeltaKey(msg, key, from->flags, 8);
        to->doubleTap = ReadDeltaKey(msg, key, from->doubleTap, 3) & 0x7;
        to->identClient = ReadDeltaKey(msg, key, from->identClient,
                                       8);      // NERVE - SMF
    } else {
        to->angles[0] = from->angles[0];
        to->angles[1] = from->angles[1];
        to->angles[2] = from->angles[2];
        to->forwardmove = from->forwardmove;
        to->rightmove = from->rightmove;
        to->upmove = from->upmove;
        to->buttons = from->buttons;
        to->wbuttons = from->wbuttons;
        to->weapon = from->weapon;
        to->flags = from->flags;
        to->doubleTap = from->doubleTap;
        to->identClient = from->identClient;    // NERVE - SMF
    }
}


/*
=============================================================================

entityState_t communication

=============================================================================
*/

/*
=================
idMessageToFunctionsLocal::ReportChangeVectors_f

Prints out a table from the current statistics for copying to code
=================
*/
void idMessageToFunctionsLocal::ReportChangeVectors_f(void) {
    sint             i;

    for(i = 0; i < 256; i++) {
        if(pcount[i]) {
            common->Printf("%d used %d\n", i, pcount[i]);
        }
    }
}

typedef struct {
    pointer     name;
    uint64 offset;
    sint             bits;      // 0 = float32
    sint             used;
} netField_t;

// using the stringizing operator to save typing...
#define NETF(x) #x,offsetof(entityState_t, x)

netField_t entityStateFields[] = {
    { NETF(pos.trTime), 32 },
    { NETF(pos.trBase[0]), 0 },
    { NETF(pos.trBase[1]), 0 },
    { NETF(pos.trDelta[0]), 0 },
    { NETF(pos.trDelta[1]), 0 },
    { NETF(pos.trBase[2]), 0 },
    { NETF(apos.trBase[1]), 0 },
    { NETF(pos.trDelta[2]), 0 },
    { NETF(apos.trBase[0]), 0 },
    { NETF(_event), 10 },
    { NETF(angles2[1]), 0 },
    { NETF(eType), 8 },
    { NETF(torsoAnim), 8 },
    { NETF(eventParm), 8 },
    { NETF(legsAnim), 8 },
    { NETF(groundEntityNum), GENTITYNUM_BITS },
    { NETF(pos.trType), 8 },
    { NETF(eFlags), 19 },
    { NETF(otherEntityNum), GENTITYNUM_BITS },
    { NETF(weapon), 8 },
    { NETF(clientNum), 8 },
    { NETF(angles[1]), 0 },
    { NETF(pos.trDuration), 32 },
    { NETF(apos.trType), 8 },
    { NETF(origin[0]), 0 },
    { NETF(origin[1]), 0 },
    { NETF(origin[2]), 0 },
    { NETF(solid), 24 },
    { NETF(misc), MAX_MISC },
    { NETF(modelindex), 8 },
    { NETF(otherEntityNum2), GENTITYNUM_BITS },
    { NETF(loopSound), 8 },
    { NETF(generic1), 8 },
    { NETF(origin2[2]), 0 },
    { NETF(origin2[0]), 0 },
    { NETF(origin2[1]), 0 },
    { NETF(modelindex2), 8 },
    { NETF(angles[0]), 0 },
    { NETF(time), 32 },
    { NETF(apos.trTime), 32 },
    { NETF(apos.trDuration), 32 },
    { NETF(apos.trBase[2]), 0 },
    { NETF(apos.trDelta[0]), 0 },
    { NETF(apos.trDelta[1]), 0 },
    { NETF(apos.trDelta[2]), 0 },
    { NETF(time2), 32 },
    { NETF(angles[2]), 0 },
    { NETF(angles2[0]), 0 },
    { NETF(angles2[2]), 0 },
    { NETF(constantLight), 32 },
    { NETF(frame), 16 },
    { NETF(weaponAnim), 8 }
};


static sint qsort_entitystatefields(const void *a, const void *b) {
    sint             aa, bb;

    aa = *const_cast<sint *>(reinterpret_cast<const sint *>(a));
    bb = *const_cast<sint *>(reinterpret_cast<const sint *>(b));

    if(entityStateFields[aa].used > entityStateFields[bb].used) {
        return -1;
    }

    if(entityStateFields[bb].used > entityStateFields[aa].used) {
        return 1;
    }

    return 0;
}

void idMessageToFunctionsLocal::PrioritiseEntitystateFields(void) {
    sint             fieldorders[sizeof(entityStateFields) / sizeof(
                                                               entityStateFields[0])];
    sint             numfields = sizeof(entityStateFields) / sizeof(
                                     entityStateFields[0]);
    sint             i;

    for(i = 0; i < numfields; i++) {
        fieldorders[i] = i;
    }

    qsort(fieldorders, numfields, sizeof(sint), qsort_entitystatefields);

    common->Printf("Entitystate fields in order of priority\n");
    common->Printf("netField_t entityStateFields[] = {\n");

    for(i = 0; i < numfields; i++) {
        common->Printf("{ NETF (%s), %i },\n",
                       entityStateFields[fieldorders[i]].name,
                       entityStateFields[fieldorders[i]].bits);
    }

    common->Printf("};\n");
}

// if (sint)f == f and (sint)f + ( 1<<(FLOAT_INT_BITS-1) ) < ( 1 << FLOAT_INT_BITS )
// the float32 will be sent with FLOAT_INT_BITS, otherwise all 32 bits will be sent
#define FLOAT_INT_BITS  13
#define FLOAT_INT_BIAS  ( 1 << ( FLOAT_INT_BITS - 1 ) )

/*
==================
idMessageToFunctionsLocal::WriteDeltaEntity

Writes part of a packetentities message, including the entity number.
Can delta from either a baseline or a previous packet_entity
If to is nullptr, a remove entity update will be sent
If force is not set, then nothing at all will be generated if the entity is
identical, under the assumption that the in-order delta code will catch it.
==================
*/
void idMessageToFunctionsLocal::WriteDeltaEntity(msg_t *msg,
        struct entityState_s *from,
        struct entityState_s *to, bool force) {
    sint             i, lc;
    sint             numFields;
    netField_t     *field;
    sint             trunc;
    float32           fullFloat;
    sint            *fromF, *toF;

    numFields = sizeof(entityStateFields) / sizeof(entityStateFields[0]);

    // all fields should be 32 bits to avoid any compiler packing issues
    // the "number" field is not part of the field list
    // if this assert fails, someone added a field to the entityState_t
    // struct without updating the message fields
    assert(numFields + 1 == sizeof(*from) / 4);

    // a nullptr to is a delta remove message
    if(to == nullptr) {
        if(from == nullptr) {
            return;
        }

        if(cl_shownet && (cl_shownet->integer >= 2 || cl_shownet->integer == -1)) {
            common->Printf("W|%3i: #%-3i remove\n", msg->cursize, from->number);
        }

        WriteBits(msg, from->number, GENTITYNUM_BITS);
        WriteBits(msg, 1, 1);
        return;
    }

    if(to->number < 0 || to->number >= MAX_GENTITIES) {
        common->Error(ERR_FATAL,
                      "idMessageToFunctionsLocal::WriteDeltaEntity: Bad entity number: %i",
                      to->number);
    }

    lc = 0;

    // build the change vector as bytes so it is endien independent
    for(i = 0, field = entityStateFields; i < numFields; i++, field++) {
        fromF = reinterpret_cast<sint *>(reinterpret_cast<uchar8 *>
                                         (from) + field->offset);
        toF = reinterpret_cast<sint *>(reinterpret_cast<uchar8 *>
                                       (to) + field->offset);

        if(*fromF != *toF) {
            lc = i + 1;

            field->used++;
        }
    }

    if(lc == 0) {
        // nothing at all changed
        if(!force) {
            return;             // nothing at all
        }

        // write two bits for no change
        WriteBits(msg, to->number, GENTITYNUM_BITS);
        WriteBits(msg, 0, 1);    // not removed
        WriteBits(msg, 0, 1);    // no delta
        return;
    }

    WriteBits(msg, to->number, GENTITYNUM_BITS);
    WriteBits(msg, 0, 1);    // not removed
    WriteBits(msg, 1, 1);    // we have a delta

    WriteByte(msg, lc);          // # of changes

    oldsize += numFields;

    //  common->Printf( "Delta for ent %i: ", to->number );

    for(i = 0, field = entityStateFields; i < lc; i++, field++) {
        fromF = reinterpret_cast<sint *>(reinterpret_cast<uchar8 *>
                                         (from) + field->offset);
        toF = reinterpret_cast<sint *>(reinterpret_cast<uchar8 *>
                                       (to) + field->offset);

        if(*fromF == *toF) {
            WriteBits(msg, 0, 1);    // no change

            wastedbits++;

            continue;
        }

        WriteBits(msg, 1, 1);    // changed

        if(field->bits == 0) {
            // float32
            fullFloat = *reinterpret_cast<float32 *>(toF);
            trunc = static_cast<sint>(fullFloat);

            if(fullFloat == 0.0f) {
                WriteBits(msg, 0, 1);
                oldsize += FLOAT_INT_BITS;
            } else {
                WriteBits(msg, 1, 1);

                if(trunc == fullFloat && trunc + FLOAT_INT_BIAS >= 0 &&
                        trunc + FLOAT_INT_BIAS < (1 << FLOAT_INT_BITS)) {
                    // send as small integer
                    WriteBits(msg, 0, 1);
                    WriteBits(msg, trunc + FLOAT_INT_BIAS, FLOAT_INT_BITS);
                    //                  if ( print ) {
                    //                      common->Printf( "%s:%i ", field->name, trunc );
                    //                  }
                } else {
                    // send as full floating point value
                    WriteBits(msg, 1, 1);
                    WriteBits(msg, *toF, 32);
                    //                  if ( print ) {
                    //                      common->Printf( "%s:%f ", field->name, *(float32 *)toF );
                    //                  }
                }
            }
        } else {
            if(*toF == 0) {
                WriteBits(msg, 0, 1);
            } else {
                WriteBits(msg, 1, 1);
                // integer
                WriteBits(msg, *toF, field->bits);
                //              if ( print ) {
                //                  common->Printf( "%s:%i ", field->name, *toF );
                //              }
            }
        }
    }

    //  common->Printf( "\n" );

    /*
        c = msg->cursize - c;

        if ( print ) {
            if ( msg->bit == 0 ) {
                endBit = msg->cursize * 8 - GENTITYNUM_BITS;
            } else {
                endBit = ( msg->cursize - 1 ) * 8 + msg->bit - GENTITYNUM_BITS;
            }
            common->Printf( " (%i bits)\n", endBit - startBit  );
        }
    */
}

/*
==================
idMessageToFunctionsLocal::ReadDeltaEntity

The entity number has already been read from the message, which
is how the from state is identified.

If the delta removes the entity, entityState_t->number will be set to MAX_GENTITIES-1

Can go from either a baseline or a previous packet_entity
==================
*/
void idMessageToFunctionsLocal::ReadDeltaEntity(msg_t *msg,
        entityState_t *from,
        entityState_t *to, sint number) {
    sint             i, lc;
    sint             numFields;
    netField_t     *field;
    sint            *fromF, *toF;
    sint             print;
    sint             trunc;
    sint             startBit, endBit;

    if(number < 0 || number >= MAX_GENTITIES) {
        common->Error(ERR_DROP, "Bad delta entity number: %i", number);
    }

    if(msg->bit == 0) {
        startBit = msg->readcount * 8 - GENTITYNUM_BITS;
    } else {
        startBit = (msg->readcount - 1) * 8 + msg->bit - GENTITYNUM_BITS;
    }

    // check for a remove
    if(ReadBits(msg, 1) == 1) {
        memset(to, 0, sizeof(*to));
        to->number = MAX_GENTITIES - 1;

        if(cl_shownet && (cl_shownet->integer >= 2 || cl_shownet->integer == -1)) {
            common->Printf("%3i: #%-3i remove\n", msg->readcount, number);
        }

        return;
    }

    // check for no delta
    if(ReadBits(msg, 1) == 0) {
        *to = *from;
        to->number = number;
        return;
    }

    numFields = sizeof(entityStateFields) / sizeof(entityStateFields[0]);
    lc = ReadByte(msg);

    if(lc > numFields || lc < 0) {
        common->Error(ERR_DROP, "invalid entityState field count");
    }

    // shownet 2/3 will interleave with other printed info, -1 will
    // just print the delta records`
    if(cl_shownet && (cl_shownet->integer >= 2 || cl_shownet->integer == -1)) {
        print = 1;
        common->Printf("%3i: #%-3i ", msg->readcount, to->number);
    } else {
        print = 0;
    }

    to->number = number;

    for(i = 0, field = entityStateFields; i < lc; i++, field++) {
        fromF = reinterpret_cast<sint *>(reinterpret_cast<uchar8 *>
                                         (from) + field->offset);
        toF = reinterpret_cast<sint *>(reinterpret_cast<uchar8 *>
                                       (to) + field->offset);

        if(!ReadBits(msg, 1)) {
            // no change
            *toF = *fromF;
        } else {
            if(field->bits == 0) {
                // float32
                if(ReadBits(msg, 1) == 0) {
                    *reinterpret_cast<float32 *>(toF) = 0.0f;
                } else {
                    if(ReadBits(msg, 1) == 0) {
                        // integral float32
                        trunc = ReadBits(msg, FLOAT_INT_BITS);
                        // bias to allow equal parts positive and negative
                        trunc -= FLOAT_INT_BIAS;
                        *reinterpret_cast<float32 *>(toF) = trunc;

                        if(print) {
                            common->Printf("%s:%i ", field->name, trunc);
                        }
                    } else {
                        // full floating point value
                        *toF = ReadBits(msg, 32);

                        if(print) {
                            common->Printf("%s:%f ", field->name, *reinterpret_cast<float32 *>(toF));
                        }
                    }
                }
            } else {
                if(ReadBits(msg, 1) == 0) {
                    *toF = 0;
                } else {
                    // integer
                    *toF = ReadBits(msg, field->bits);

                    if(print) {
                        common->Printf("%s:%i ", field->name, *toF);
                    }
                }
            }

            //          pcount[i]++;
        }
    }

    for(i = lc, field = &entityStateFields[lc]; i < numFields; i++, field++) {
        fromF = reinterpret_cast<sint *>(reinterpret_cast<uchar8 *>
                                         (from) + field->offset);
        toF = reinterpret_cast<sint *>(reinterpret_cast<uchar8 *>
                                       (to) + field->offset);
        // no change
        *toF = *fromF;
    }

    if(print) {
        if(msg->bit == 0) {
            endBit = msg->readcount * 8 - GENTITYNUM_BITS;
        } else {
            endBit = (msg->readcount - 1) * 8 + msg->bit - GENTITYNUM_BITS;
        }

        common->Printf(" (%i bits)\n", endBit - startBit);
    }
}


/*
============================================================================

player_state_t communication

============================================================================
*/

// using the stringizing operator to save typing...
#define PSF(x) #x,offsetof(playerState_t, x)

netField_t      playerStateFields[] = {
    { PSF(commandTime), 32 },
    { PSF(origin[0]), 0 },
    { PSF(origin[1]), 0 },
    { PSF(bobCycle), 8 },
    { PSF(velocity[0]), 0 },
    { PSF(velocity[1]), 0 },
    { PSF(viewangles[1]), 0 },
    { PSF(viewangles[0]), 0 },
    { PSF(weaponTime), -16 },
    { PSF(origin[2]), 0 },
    { PSF(velocity[2]), 0 },
    { PSF(legsTimer), 8 },
    { PSF(pm_time), -16 },
    { PSF(eventSequence), 16 },
    { PSF(torsoAnim), 8 },
    { PSF(movementDir), 4 },
    { PSF(events[0]), 8 },
    { PSF(legsAnim), 8 },
    { PSF(events[1]), 8 },
    { PSF(pm_flags), 16 },
    { PSF(groundEntityNum), GENTITYNUM_BITS },
    { PSF(weaponstate), 4 },
    { PSF(eFlags), 16 },
    { PSF(externalEvent), 10 },
    { PSF(gravity), -16 },
    { PSF(speed), -16 },
    { PSF(delta_angles[1]), 16 },
    { PSF(externalEventParm), 8 },
    { PSF(viewheight), -8 },
    { PSF(damageEvent), 8 },
    { PSF(damageYaw), 8 },
    { PSF(damagePitch), 8 },
    { PSF(damageCount), 8 },
    { PSF(generic1), 8 },
    { PSF(pm_type), 8 },
    { PSF(delta_angles[0]), 16 },
    { PSF(delta_angles[2]), 16 },
    { PSF(torsoTimer), 12 },
    { PSF(eventParms[0]), 8 },
    { PSF(eventParms[1]), 8 },
    { PSF(clientNum), 8 },
    { PSF(weapon), 5 },
    { PSF(viewangles[2]), 0 },
    { PSF(grapplePoint[0]), 0 },
    { PSF(grapplePoint[1]), 0 },
    { PSF(grapplePoint[2]), 0 },
    { PSF(otherEntityNum), 10 },
    { PSF(loopSound), 16 },
    { PSF(identifyClient),  8},
    { PSF(tauntTimer), 12 },
    { PSF(weaponAnim), 8 }
};

static sint qsort_playerstatefields(const void *a, const void *b) {
    sint             aa, bb;

    aa = *const_cast<sint *>(reinterpret_cast<const sint *>(a));
    bb = *const_cast<sint *>(reinterpret_cast<const sint *>(b));

    if(playerStateFields[aa].used > playerStateFields[bb].used) {
        return -1;
    }

    if(playerStateFields[bb].used > playerStateFields[aa].used) {
        return 1;
    }

    return 0;
}

void idMessageToFunctionsLocal::PrioritisePlayerStateFields(void) {
    sint             fieldorders[sizeof(playerStateFields) / sizeof(
                                                               playerStateFields[0])];
    sint             numfields = sizeof(playerStateFields) / sizeof(
                                     playerStateFields[0]);
    sint             i;

    for(i = 0; i < numfields; i++) {
        fieldorders[i] = i;
    }

    qsort(fieldorders, numfields, sizeof(sint), qsort_playerstatefields);

    common->Printf("Playerstate fields in order of priority\n");
    common->Printf("netField_t playerStateFields[] = {\n");

    for(i = 0; i < numfields; i++) {
        common->Printf("{ PSF(%s), %i },\n",
                       playerStateFields[fieldorders[i]].name,
                       playerStateFields[fieldorders[i]].bits);
    }

    common->Printf("};\n");
}

/*
=============
idMessageToFunctionsLocal::WriteDeltaPlayerstate
=============
*/
void idMessageToFunctionsLocal::WriteDeltaPlayerstate(msg_t *msg,
        struct playerState_s *from,
        struct playerState_s *to) {
    sint i;
    playerState_t   dummy;
    sint statsbits;
    sint persistantbits;
    sint ammobits;
    sint miscbits;
    sint numFields;
    sint c;
    netField_t *field;
    sint *fromF, * toF;
    float32 fullFloat;
    sint trunc, lc;

    if(!from) {
        from = &dummy;
        ::memset(&dummy, 0, sizeof(dummy));
    }

    c = msg->cursize;

    numFields = sizeof(playerStateFields) / sizeof(playerStateFields[0]);

    lc = 0;

    for(i = 0, field = playerStateFields; i < numFields; i++, field++) {
        fromF = reinterpret_cast<sint *>((uchar8 *)from + field->offset);
        toF = reinterpret_cast<sint *>((uchar8 *)to + field->offset);

        if(*fromF != *toF) {
            lc = i + 1;
        }
    }

    WriteByte(msg, lc); // # of changes

    oldsize += numFields - lc;

    for(i = 0, field = playerStateFields; i < lc; i++, field++) {
        fromF = reinterpret_cast<sint *>((uchar8 *)from + field->offset);
        toF = reinterpret_cast<sint *>((uchar8 *)to + field->offset);

        if(*fromF == *toF) {
            WriteBits(msg, 0, 1);   // no change
            continue;
        }

        WriteBits(msg, 1, 1);   // changed
        //      pcount[i]++;

        if(field->bits == 0) {
            // float
            fullFloat = *reinterpret_cast<float32 *>(toF);
            trunc = (int)fullFloat;

            if(trunc == fullFloat && trunc + FLOAT_INT_BIAS >= 0 &&
                    trunc + FLOAT_INT_BIAS < (1 << FLOAT_INT_BITS)) {
                // send as small integer
                WriteBits(msg, 0, 1);
                WriteBits(msg, trunc + FLOAT_INT_BIAS, FLOAT_INT_BITS);
            } else {
                // send as full floating point value
                WriteBits(msg, 1, 1);
                WriteBits(msg, *toF, 32);
            }
        } else {
            // integer
            WriteBits(msg, *toF, field->bits);
        }
    }

    c = msg->cursize - c;


    //
    // send the arrays
    //
    statsbits = 0;

    for(i = 0; i < MAX_STATS; i++) {
        if(to->stats[i] != from->stats[i]) {
            statsbits |= 1 << i;
        }
    }

    persistantbits = 0;

    for(i = 0; i < MAX_PERSISTANT; i++) {
        if(to->persistant[i] != from->persistant[i]) {
            persistantbits |= 1 << i;
        }
    }

    // backporting: 1.1 expects an array here
    ammobits = (to->ammo != from->ammo) | (to->clips != from->clips) << 1;

    for(i = 0; i < 14; i++) {
        if(to->ammo_extra[i] != from->ammo_extra[i]) {
            ammobits |= 4 << i;
        }
    }

    miscbits = 0;

    for(i = 0; i < MAX_MISC; i++) {
        if(to->misc[i] != from->misc[i]) {
            miscbits |= 1 << i;
        }
    }

    if(!statsbits && !persistantbits && !ammobits && !miscbits) {
        WriteBits(msg, 0, 1);   // no change
        oldsize += 4;
        return;
    }

    WriteBits(msg, 1, 1);   // changed

    if(statsbits) {
        WriteBits(msg, 1, 1);   // changed
        WriteBits(msg, statsbits, MAX_STATS);

        for(i = 0; i < MAX_STATS; i++)
            if(statsbits & (1 << i)) {
                WriteShort(msg, to->stats[i]);
            }
    } else {
        WriteBits(msg, 0, 1);   // no change
    }


    if(persistantbits) {
        WriteBits(msg, 1, 1);   // changed
        WriteBits(msg, persistantbits, MAX_PERSISTANT);

        for(i = 0; i < MAX_PERSISTANT; i++)
            if(persistantbits & (1 << i)) {
                WriteShort(msg, to->persistant[i]);
            }
    } else {
        WriteBits(msg, 0, 1);   // no change
    }


    if(ammobits) {
        WriteBits(msg, 1, 1);   // changed
        WriteBits(msg, ammobits, 16);

        if(ammobits & 1) {
            WriteShort(msg, to->ammo);
        }

        if(ammobits & 2) {
            WriteShort(msg, to->clips);
        }

        for(i = 0; i < 14; i++)
            if(ammobits & (4 << i)) {
                WriteShort(msg, to->ammo_extra[i]);
            }
    } else {
        WriteBits(msg, 0, 1);   // no change
    }



    if(miscbits) {
        WriteBits(msg, 1, 1);   // changed
        WriteBits(msg, miscbits, MAX_MISC);

        for(i = 0; i < MAX_MISC; i++)
            if(miscbits & (1 << i)) {
                WriteLong(msg, to->misc[i]);
            }
    } else {
        WriteBits(msg, 0, 1);   // no change
    }

}


/*
===================
idMessageToFunctionsLocal::ReadDeltaPlayerstate
===================
*/
void idMessageToFunctionsLocal::ReadDeltaPlayerstate(msg_t *msg,
        playerState_t *from,
        playerState_t *to) {
    sint i, lc;
    sint bits;
    netField_t *field;
    sint numFields;
    sint startBit, endBit;
    sint print;
    sint *fromF, * toF;
    sint trunc;
    playerState_t dummy;

    if(!from) {
        from = &dummy;
        ::memset(&dummy, 0, sizeof(dummy));
    }

    *to = *from;

    if(msg->bit == 0) {
        startBit = msg->readcount * 8 - GENTITYNUM_BITS;
    } else {
        startBit = (msg->readcount - 1) * 8 + msg->bit - GENTITYNUM_BITS;
    }

    // shownet 2/3 will interleave with other printed info, -2 will
    // just print the delta records
    if(cl_shownet && (cl_shownet->integer >= 2 || cl_shownet->integer == -2)) {
        print = 1;
        common->Printf("%3i: playerstate ", msg->readcount);
    } else {
        print = 0;
    }

    numFields = sizeof(playerStateFields) / sizeof(playerStateFields[0]);
    lc = ReadByte(msg);

    for(i = 0, field = playerStateFields; i < lc; i++, field++) {
        fromF = reinterpret_cast<sint *>((uchar8 *)from + field->offset);
        toF = reinterpret_cast<sint *>((uchar8 *)to + field->offset);

        if(!ReadBits(msg, 1)) {
            // no change
            *toF = *fromF;
        } else {
            if(field->bits == 0) {
                // float
                if(ReadBits(msg, 1) == 0) {
                    // integral float
                    trunc = ReadBits(msg, FLOAT_INT_BITS);
                    // bias to allow equal parts positive and negative
                    trunc -= FLOAT_INT_BIAS;
                    *(float *)toF = trunc;

                    if(print) {
                        common->Printf("%s:%i ", field->name, trunc);
                    }
                } else {
                    // full floating point value
                    *toF = ReadBits(msg, 32);

                    if(print) {
                        common->Printf("%s:%f ", field->name, *(float *)toF);
                    }
                }
            } else {
                // integer
                *toF = ReadBits(msg, field->bits);

                if(print) {
                    common->Printf("%s:%i ", field->name, *toF);
                }
            }
        }
    }

    for(i = lc, field = &playerStateFields[lc]; i < numFields; i++, field++) {
        fromF = reinterpret_cast<sint *>((uchar8 *)from + field->offset);
        toF = reinterpret_cast<sint *>((uchar8 *)to + field->offset);
        // no change
        *toF = *fromF;
    }


    // read the arrays
    if(ReadBits(msg, 1)) {
        // parse stats
        if(ReadBits(msg, 1)) {
            LOG("PS_STATS");
            bits = ReadBits(msg, MAX_STATS);

            for(i = 0; i < MAX_STATS; i++) {
                if(bits & (1 << i)) {
                    to->stats[i] = ReadShort(msg);
                }
            }
        }

        // parse persistant stats
        if(ReadBits(msg, 1)) {
            LOG("PS_PERSISTANT");
            bits = ReadBits(msg, MAX_PERSISTANT);

            for(i = 0; i < MAX_PERSISTANT; i++) {
                if(bits & (1 << i)) {
                    to->persistant[i] = ReadShort(msg);
                }
            }
        }

        // parse ammo data
        if(ReadBits(msg, 1)) {
            LOG("PS_AMMO");
            bits = ReadBits(msg, 16);

            if(bits & 1) {
                to->ammo = ReadShort(msg);
            }

            if(bits & 2) {
                to->clips = ReadShort(msg);
            }

            for(i = 0; i < 14; i++) {
                if(bits & (4 << i)) {
                    to->ammo_extra[i] = ReadShort(msg);
                }
            }
        }

        // parse misc data
        if(ReadBits(msg, 1)) {
            LOG("PS_MISC");
            bits = ReadBits(msg, MAX_MISC);

            for(i = 0; i < MAX_MISC; i++) {
                if(bits & (1 << i)) {
                    to->misc[i] = ReadLong(msg);
                }
            }
        }
    }

    if(print) {
        if(msg->bit == 0) {
            endBit = msg->readcount * 8 - GENTITYNUM_BITS;
        } else {
            endBit = (msg->readcount - 1) * 8 + msg->bit - GENTITYNUM_BITS;
        }

        common->Printf(" (%i bits)\n", endBit - startBit);
    }
}

//===========================================================================