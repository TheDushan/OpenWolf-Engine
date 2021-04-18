////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2021 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of OpenWolf.
//
// OpenWolf is free software; you can redistribute it
// and / or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the License,
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
// File name:   clientNetChan.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTNETCHAN_HPP__
#define __CLIENTNETCHAN_HPP__

//
// idClientLANSystemLocal
//
class idClientNetChanSystemLocal : public idClientNetChanSystem {
public:
    idClientNetChanSystemLocal();
    ~idClientNetChanSystemLocal();

    virtual void Netchan_TransmitNextFragment(netchan_t *chan);
    virtual void Netchan_Transmit(netchan_t *chan, msg_t *msg);
    virtual bool Netchan_Process(netchan_t *chan, msg_t *msg);

    static void Netchan_Encode(msg_t *msg);
    static void Netchan_Decode(msg_t *msg);


};

extern idClientNetChanSystemLocal clientNetChanLocal;

#endif // !__CLIENTNETCHAN_HPP__