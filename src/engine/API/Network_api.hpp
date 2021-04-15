////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2019 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   Network_api.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __NETWORK_API_H__
#define __NETWORK_API_H__

//
// idNetworkSystem
//
class idNetworkSystem {
public:
    virtual bool StringToAdr(pointer s, netadr_t *a, netadrtype_t family) = 0;
    virtual bool CompareBaseAdrMask(netadr_t a, netadr_t b, sint netmask) = 0;
    virtual bool CompareBaseAdr(netadr_t a, netadr_t b) = 0;
    virtual pointer AdrToString(netadr_t a) = 0;
    virtual pointer AdrToStringwPort(netadr_t a) = 0;
    virtual bool CompareAdr(netadr_t a, netadr_t b) = 0;
    virtual bool IsLocalAddress(netadr_t adr) = 0;
    virtual bool GetPacket(netadr_t *net_from, msg_t *net_message) = 0;
    virtual void SendPacket(sint length, const void *data, netadr_t to) = 0;
    virtual bool IsLANAddress(netadr_t adr) = 0;
    virtual void ShowIP(void) = 0;
    virtual void JoinMulticast6(void) = 0;
    virtual void LeaveMulticast6(void) = 0;
    virtual void Init(void) = 0;
    virtual void Shutdown(void) = 0;
    virtual void Sleep(sint msec) = 0;
    virtual void Restart_f(void) = 0;
    virtual sint ConnectTCP(valueType *s_host_port) = 0;
};

extern idNetworkSystem *networkSystem;

#endif //!__THREADS_API_H__
