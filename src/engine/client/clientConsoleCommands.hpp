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
// File name:   clientConsoleCommands.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTCONSOLECOMMANDS_HPP__
#define __CLIENTCONSOLECOMMANDS_HPP__

//
// idClientConsoleCommandsSystemLocal
//
class idClientConsoleCommandsSystemLocal : public
    idClientConsoleCommandsSystemAPI {
public:
    idClientConsoleCommandsSystemLocal();
    ~idClientConsoleCommandsSystemLocal();

    virtual void Disconnect(bool showMainMenu, pointer reason);

    static void ForwardToServer_f(void);
    static void Configstrings_f(void);
    static void Clientinfo_f(void);
    static void Snd_Reload_f(void);
    static void Snd_Restart_f(void);
    static void Vid_Restart_f(void);
    static void UI_Restart_f(void);
    static void Connect_f(void);
    static void Reconnect_f(void);
    static void OpenUrl_f(void);
    static void Rcon_f(void);
    static void Setenv_f(void);
    static void Disconnect_f(void);
    static void OpenedPK3List_f(void);
    static void ReferencedPK3List_f(void);
    static void SetRecommended_f(void);
    static void Userinfo_f(void);
    static void Video_f(void);
    static void StopVideo_f(void);
    static void UpdateScreen(void);

};

extern idClientConsoleCommandsSystemLocal clientConsoleCommandsLocal;

#endif //!__CLIENTCONSOLECOMMANDS_HPP__
