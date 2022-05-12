////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2021 - 2022 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   clientInput.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTINPUT_HPP__
#define __CLIENTINPUT_HPP__

static uint frame_msec;
static sint old_com_frameTime;

/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as argv(1) so it can be matched up with the release.

argv(2) will be set to the time the event happened, which allows exact
control even at low framerates when the down and up events may both get qued
at the same time.

===============================================================================
*/

static kbutton_t kb[NUM_BUTTONS];

// Arnout: doubleTap button mapping
static kbuttons_t dtmapping[] = {
    static_cast<kbuttons_t>(-1),// DT_NONE
    KB_MOVELEFT,                // DT_MOVELEFT
    KB_MOVERIGHT,               // DT_MOVERIGHT
    KB_FORWARD,                 // DT_FORWARD
    KB_BACK,                    // DT_BACK
    KB_WBUTTONS4,               // DT_LEANLEFT
    KB_WBUTTONS5,               // DT_LEANRIGHT
    KB_UP                       // DT_UP
};

//
// idClientInputSystemLocal
//
class idClientInputSystemLocal : public idClientInputSystemAPI {
public:
    idClientInputSystemLocal();
    ~idClientInputSystemLocal();

    virtual void MouseEvent(sint dx, sint dy, sint time);
    virtual void JoystickEvent(sint axis, sint value, sint time);
    virtual void WritePacket(void);
    virtual void SendCmd(void);
    virtual void InitInput(void);
    virtual void ClearKeys(void);

    static void MLookDown(void);
    static void MLookUp(void);
    static void KeyDown(kbutton_t *b);
    static void KeyUp(kbutton_t *b);
    static float32 KeyState(kbutton_t *key);
    static void UpDown(void);
    static void UpUp(void);
    static void DownDown(void);
    static void DownUp(void);
    static void LeftDown(void);
    static void LeftUp(void);
    static void RightDown(void);
    static void RightUp(void);
    static void ForwardDown(void);
    static void ForwardUp(void);
    static void BackDown(void);
    static void BackUp(void);
    static void LookupDown(void);
    static void LookupUp(void);
    static void LookdownDown(void);
    static void LookdownUp(void);
    static void MoveleftDown(void);
    static void MoveleftUp(void);
    static void MoverightDown(void);
    static void MoverightUp(void);
    static void SpeedDown(void);
    static void SpeedUp(void);
    static void StrafeDown(void);
    static void StrafeUp(void);
    static void Button0Down(void);
    static void Button0Up(void);
    static void Button1Down(void);
    static void Button1Up(void);
    static void UseItemDown(void);
    static void UseItemUp(void);
    static void Button2Down(void);
    static void Button2Up(void);
    static void Button3Down(void);
    static void Button3Up(void);
    static void Button4Down(void);
    static void Button4Up(void);
    static void Button5Down(void);
    static void Button5Up(void);
    static void Button6Down(void);
    static void Button6Up(void);
    static void Button7Down(void);
    static void Button7Up(void);
    static void Button8Down(void);
    static void Button8Up(void);
    static void Button9Down(void);
    static void Button9Up(void);
    static void Button10Down(void);
    static void Button10Up(void);
    static void Button11Down(void);
    static void Button11Up(void);
    static void Button12Down(void);
    static void Button12Up(void);
    static void Button13Down(void);
    static void Button13Up(void);
    static void Button14Down(void);
    static void Button14Up(void);
    static void Button15Down(void);
    static void Button15Up(void);
    static void ActivateDown(void);
    static void ActivateUp(void);
    static void SprintDown(void);
    static void SprintUp(void);
    static void Wbutton0Down(void);
    static void Wbutton0Up(void);
    static void ZoomDown(void);
    static void ZoomUp(void);
    static void ReloadDown(void);
    static void ReloadUp(void);
    static void LeanLeftDown(void);
    static void LeanLeftUp(void);
    static void LeanRightDown(void);
    static void LeanRightUp(void);
    static void ProneDown(void);
    static void ProneUp(void);
    static void ButtonDown(void);
    static void ButtonUp(void);
    static void Notebook(void);
    static void Help(void);
    static void CenterView(void);
    static void AdjustAngles(void);
    static void KeyMove(usercmd_t *cmd);
    static void JoystickMove(usercmd_t *cmd);
    static void Xbox360ControllerMove(usercmd_t *cmd);
    static void MouseMove(usercmd_t *cmd);
    static void CmdButtons(usercmd_t *cmd);
    static void FinishMove(usercmd_t *cmd);
    static usercmd_t CreateCmd(void);
    static void CreateNewCommands(void);
    static bool ReadyToSendPacket(void);
};

extern idClientInputSystemLocal ClientInputLocal;

#endif // !__CLIENTINPUT_HPP__
