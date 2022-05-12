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
// File name:   CmdDelay.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description: Quake script command processing module
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CMDDELAYSYSTEM_HPP__
#define __CMDDELAYSYSTEM_HPP__

// Delay stuff
#define MAX_DELAYED_COMMANDS 64
#define CMD_DELAY_FRAME_FIRE 1
#define CMD_DELAY_UNUSED 0

enum cmdDelayType_t {
    CMD_DELAY_MSEC,
    CMD_DELAY_FRAME
};

typedef struct {
    valueType name[MAX_CMD_LINE];
    valueType text[MAX_CMD_LINE];
    sint delay;
    cmdDelayType_t  type;
} delayedCommands_s;

extern delayedCommands_s delayedCommands[MAX_DELAYED_COMMANDS];

//
// idCmdDelaySystemLocal
//
class idCmdDelaySystemLocal : public idCmdDelaySystem {
public:
    idCmdDelaySystemLocal();
    ~idCmdDelaySystemLocal();

    virtual void Frame(void);

};

extern idCmdDelaySystemLocal cmdDelayLocal;

#endif //!__CMDDELAYSYSTEM_HPP__