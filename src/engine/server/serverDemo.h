////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2018 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   serverBot.h
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2017, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SERVERDEMO_H__
#define __SERVERDEMO_H__


// Headers for demo messages
typedef enum
{
    demo_endFrame,
    demo_serverCommand,
    demo_gameCommand,
    demo_entityState,
    demo_entityShared,
    demo_playerState,
    demo_endDemo,
    demo_EOF
} demo_ops_e;

// Big fat buffer to store all our stuff
static U8 buf[0x400000];

// Save maxclients and democlients and restore them after the demo
static S32 savedMaxClients, savedDemoClients;

//
// idServerBotSystemLocal
//
class idServerDemoSystemLocal : public idServerDemoSystem
{
public:
    idServerDemoSystemLocal();
    ~idServerDemoSystemLocal();
    
    virtual void DemoWriteServerCommand( StringEntry str );
    virtual void DemoWriteGameCommand( S32 cmd, StringEntry str );
    virtual void DemoWriteFrame( void );
    virtual void DemoReadFrame( void );
    virtual void DemoStartRecord( void );
    virtual void DemoStopRecord( void );
    virtual void DemoStartPlayback( void );
    virtual void DemoStopPlayback( void );
    
    static void DemoWriteMessage( msg_t* msg );
    
};

extern idServerDemoSystemLocal serverDemoLocal;

#endif //!__SERVERDEMO_H__
