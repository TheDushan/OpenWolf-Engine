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
// File name:   Common_api.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description: misc functions used in client and server
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __COMMON_API_HPP__
#define __COMMON_API_HPP__

enum sysEventType_t {
    // bk001129 - make sure SE_NONE is zero
    SYSE_NONE = 0,              // evTime is still valid
    SYSE_KEY,                       // evValue is a key code, evValue2 is the down flag
    SYSE_CHAR,                  // evValue is an ascii valueType
    SYSE_MOUSE,                 // evValue and evValue2 are reletive signed x / y moves
    SYSE_JOYSTICK_AXIS,         // evValue is an axis number and evValue2 is the current state (-127 to 127)
    SYSE_CONSOLE,               // evPtr is a valueType*
    SYSE_PACKET,                 // evPtr is a netadr_t followed by data bytes to evPtrLength
    SYSE_MAX
};

enum netadrtype_t {
    NA_BAD = 0,                 // an address lookup failed
    NA_BOT,
    NA_LOOPBACK,
    NA_BROADCAST,
    NA_IP,
    NA_IP6,
    NA_MULTICAST6,
    NA_UNSPEC
};

typedef struct {
    netadrtype_t type;

    uchar8 ip[4];
    uchar8 ip6[16];

    uchar16 port;
    uint32 scope_id;    // Needed for IPv6 link-local addresses
} netadr_t;

//
// idCommon
//
class idCommon {
public:
    virtual void BeginRedirect(valueType *buffer, uint64 buffersize,
                               void (*flush)(valueType *)) = 0;
    virtual void EndRedirect(void) = 0;
    virtual void Printf(pointer fmt, ...) = 0;
    virtual void Error(errorParm_t code, pointer fmt, ...) = 0;
    virtual bool SafeMode(void) = 0;
    virtual void StartupVariable(pointer match) = 0;
    virtual void InfoPrint(pointer s) = 0;
    virtual sint Filter(valueType *filter, valueType *name,
                        sint casesensitive) = 0;
    virtual sint FilterPath(pointer filter, pointer name,
                            sint casesensitive) = 0;
    virtual sint RealTime(qtime_t *qtime) = 0;
    virtual void QueueEvent(sint evTime, sysEventType_t evType, sint value,
                            sint value2, sint ptrLength, void *ptr) = 0;
    virtual sint EventLoop(void) = 0;
    virtual sint Milliseconds(void) = 0;
    virtual void SetRecommended(void) = 0;
    virtual bool CheckProfile(valueType *profile_path) = 0;
    virtual bool WriteProfile(valueType *profile_path) = 0;
    virtual void Init(valueType *commandLine) = 0;
    virtual void Frame(void) = 0;
    virtual void RandomBytes(uchar8 *string, sint len) = 0;
    virtual void RgbToHsl(vec4_t rgb, vec4_t hsl) = 0;
    virtual void HlsToRgb(vec4_t hsl, vec4_t rgb) = 0;
};

extern idCommon *common;

#endif //!__COMMON_API_HPP__