////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2021 - 2023 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   clientInput_api.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTINPUT_API_HPP__
#define __CLIENTINPUT_API_HPP__

//
// cl_input
//
typedef struct {
    sint down[2];    // key nums holding it down
    uint downtime;   // msec timestamp
    uint msec;       // msec down this frame if both a down and up happened
    bool active;     // current state
    bool wasPressed; // set when down, not cleared when up
} kbutton_t;

enum kbuttons_t {
    KB_LEFT,
    KB_RIGHT,
    KB_FORWARD,
    KB_BACK,
    KB_LOOKUP,
    KB_LOOKDOWN,
    KB_MOVELEFT,
    KB_MOVERIGHT,
    KB_STRAFE,
    KB_SPEED,
    KB_UP,
    KB_DOWN,
    KB_BUTTONS0,
    KB_BUTTONS1,
    KB_BUTTONS2,
    KB_BUTTONS3,
    KB_BUTTONS4,
    KB_BUTTONS5,
    KB_BUTTONS6,
    KB_BUTTONS7,
    KB_BUTTONS8,
    KB_BUTTONS9,
    KB_BUTTONS10,
    KB_BUTTONS11,
    KB_BUTTONS12,
    KB_BUTTONS13,
    KB_BUTTONS14,
    KB_BUTTONS15,
    KB_WBUTTONS0,
    KB_WBUTTONS1,
    KB_WBUTTONS2,
    KB_WBUTTONS3,
    KB_WBUTTONS4,
    KB_WBUTTONS5,
    KB_WBUTTONS6,
    KB_WBUTTONS7,
    KB_MLOOK,
    // Dushan
    NUM_BUTTONS
};

//
// idClientInputSystemAPI
//
class idClientInputSystemAPI {
public:
    virtual void MouseEvent(sint dx, sint dy, sint time) = 0;
    virtual void JoystickEvent(sint axis, sint value, sint time) = 0;
    virtual void WritePacket(void) = 0;
    virtual void SendCmd(void) = 0;
    virtual void InitInput(void) = 0;
    virtual void ClearKeys(void) = 0;
};

extern idClientInputSystemAPI *clientInputSystem;

#endif // !__CLIENTINPUT_API_HPP__
