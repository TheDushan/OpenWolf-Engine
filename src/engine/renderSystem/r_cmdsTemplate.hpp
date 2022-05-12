////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2020 - 2022 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   r_cmdTemplate.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_CMDSTEMPLATE_HPP__
#define __R_CMDSTEMPLATE_HPP__

/*
=============
idRenderSystemCmdsLocal::GetCommandBuffer

returns nullptr if there is not enough space for important commands
=============
*/
template<typename B, typename T>
T GetCommandBuffer(B bytes, T type) {
    renderCommandList_t *cmdList;

    cmdList = &backEndData->commands[tr.smpFrame];
    bytes = PAD(bytes, sizeof(void *));

    // always leave room for the end of list command
    if(cmdList->used + bytes + (sizeof(swapBuffersCommand_t) + sizeof(
                                    sint)) > MAX_RENDER_COMMANDS) {
        if(bytes > MAX_RENDER_COMMANDS - (sizeof(swapBuffersCommand_t) + sizeof(
                                              sint))) {
            common->Error(ERR_FATAL, "GetCommandBuffer: bad size %i", bytes);
        }

        clientRendererSystem->RefPrintf(PRINT_ALL,
                                        "GetCommandBuffer (%i, %i): out of room, dropping command\n",
                                        bytes, type);

        // if we run out of room, just start dropping commands
        return nullptr;
    }

    cmdList->used += bytes;

    return reinterpret_cast<T>(cmdList->cmds + cmdList->used - bytes);
}

#endif //!__R_CMDSTEMPLATE_HPP__
