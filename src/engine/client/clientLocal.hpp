////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   clientLocal.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: primary header for client
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTLOCAL_HPP__
#define __CLIENTLOCAL_HPP__

#if !defined ( DEDICATED ) && !defined ( UPDATE_SERVER ) && !defined ( BSPC )
#endif


//=============================================================================

extern void    *cgvm;           // interface to cgame dll or vm
extern void    *uivm;
extern void    *dbvm;
extern idCGame *cgame;
extern idUserInterfaceManager *uiManager;

//=================================================

void            Key_GetBindingByString(pointer binding, sint *key1,
                                       sint *key2);

//
// cl_main
//
void CL_PurgeCache(void);
void CL_Init(void);
void CL_FlushMemory(void);
void CL_ShutdownAll(bool shutdownRen);
void CL_AddReliableCommand(pointer cmd);

void            CL_StartHunkUsers(bool rendererOnly);

#if !defined(UPDATE_SERVER)
void            CL_CheckAutoUpdate(void);
bool            CL_NextUpdateServer(void);
void            CL_GetAutoUpdate(void);
#endif

void            CL_Disconnect_f(void);
void            CL_Vid_Restart_f(void);
void            CL_Snd_Restart_f(void);
void            CL_NextDemo(void);
void            CL_ReadDemoMessage(void);
void            CL_StartDemoLoop(void);

void            CL_InitDownloads(void);
void            CL_NextDownload(void);


void            CL_ShutdownRef(void);
void            CL_InitRef(void);

void            CL_AddToLimboChat(pointer str);      // NERVE - SMF

void            CL_OpenURL(pointer url);     // TTimo
void            CL_Record(pointer name);

//
// cl_input
//
typedef struct {
    sint             down[2];   // key nums holding it down
    uint        downtime;   // msec timestamp
    uint
    msec;       // msec down this frame if both a down and up happened
    bool        active;     // current state
    bool        wasPressed; // set when down, not cleared when up
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


void            CL_ClearKeys(void);
void            CL_InitInput(void);
void            CL_SendCmd(void);
void            CL_ClearState(void);
void            CL_WritePacket(void);
void            IN_Notebook(void);
void            IN_Help(void);
float32           CL_KeyState(kbutton_t *key);
sint             Key_StringToKeynum(pointer str);
valueType           *Key_KeynumToString(sint keynum);

extern sint      cl_connectedToPureServer;

//====================================================================

void            CL_UpdateInfoPacket(netadr_t from);      // DHM - Nerve

//
// console
//

void Con_ConsoleSwitch(sint n);
void Con_ConsoleNext(sint n);
void Con_LineAccept(void);
void            Con_Init(void);
void            Con_Clear_f(void);
void            Con_ToggleConsole_f(void);
void            Con_OpenConsole_f(void);
void            Con_DrawNotify(void);
void            Con_ClearNotify(void);
void            Con_RunConsole(void);
void            Con_DrawConsole(void);
void            Con_PageUp(void);
void            Con_PageDown(void);
void            Con_Top(void);
void            Con_Bottom(void);
void            Con_Close(void);

//
// cl_main.c
//
void            CL_WriteDemoMessage(msg_t *msg, sint headerBytes);
void            CL_RequestMotd(void);

#define NUMBER_TABS 4
#define CON_ALL 0
#define CON_SYS 1
#define CON_CHAT 2
#define CON_TCHAT 3

// check if this is a chat console
#define CON_ISCHAT(conNum) (conNum >= CON_CHAT)

#define NUM_CON_TIMES 4
#define CON_TEXTSIZE 65536
typedef struct {
    bool        initialized;

    schar16         text[CON_TEXTSIZE];
    sint         current;   // line where next message will be printed
    sint         x;         // offset in current line for next print
    sint         display;   // bottom of console displays this line

    sint         linewidth; // characters across screen
    sint         totallines;    // total lines in console scrollback

    float32         xadjust;    // for wide aspect screens

    float32         displayFrac;    // aproaches finalFrac at scr_conspeed
    float32         finalFrac;  // 0.0 to 1.0 lines of console to display
    float32         desiredFrac;    // ydnar: for variable console heights

    sint         vislines;  // in scanlines

    sint
    times[NUM_CON_TIMES];  // cls.realtime time the line was generated
    // for transparent notify lines
    vec4_t      color;

    sint          acLength; // Arnout: autocomplete buffer length
} console_t;

extern console_t    con[NUMBER_TABS];
extern console_t *activeCon;

#endif //!__CLIENTLOCAL_HPP__
