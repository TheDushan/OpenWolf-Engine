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
// File name:   clientInput.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: builds an intended movement command to send to the server
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

idClientInputSystemLocal clientInputLocal;
idClientInputSystemAPI *clientInputSystem = &clientInputLocal;

/*
===============
idClientInputSystemLocal::idClientInputSystemLocal
===============
*/
idClientInputSystemLocal::idClientInputSystemLocal(void) {
}

/*
===============
idClientInputSystemLocal::~idClientInputSystemLocal
===============
*/
idClientInputSystemLocal::~idClientInputSystemLocal(void) {
}

/*
===============
idClientInputSystemLocal::MLookDown
===============
*/
void idClientInputSystemLocal::MLookDown(void) {
    kb[KB_MLOOK].active = true;
}

/*
===============
idClientInputSystemLocal::MLookUp
===============
*/
void idClientInputSystemLocal::MLookUp(void) {
    kb[KB_MLOOK].active = false;

    if(!cl_freelook->integer) {
        //IN_CenterView ();
    }
}

/*
===============
idClientInputSystemLocal::KeyDown
===============
*/
void idClientInputSystemLocal::KeyDown(kbutton_t *b) {
    sint k;
    pointer c;

    c = cmdSystem->Argv(1);

    if(c[0]) {
        k = ::atoi(c);
    } else {
        // typed manually at the console for continuous down
        k = -1;
    }

    if(k == b->down[0] || k == b->down[1]) {
        // repeating key
        return;
    }

    if(!b->down[0]) {
        b->down[0] = k;
    } else if(!b->down[1]) {
        b->down[1] = k;
    } else {
        Com_Printf("Three keys down for a button!\n");
        return;
    }

    if(b->active) {
        // still down
        return;
    }

    // save timestamp for partial frame summing
    c = cmdSystem->Argv(2);
    b->downtime = atoi(c);

    b->active = true;
    b->wasPressed = true;
}

/*
===============
idClientInputSystemLocal::idClientInputSystemLocal
===============
*/
void idClientInputSystemLocal::KeyUp(kbutton_t *b) {
    sint k;
    pointer c;
    uint uptime;

    c = cmdSystem->Argv(1);

    if(c[0]) {
        k = atoi(c);
    } else {
        // typed manually at the console, assume for unsticking, so clear all
        b->down[0] = b->down[1] = 0;
        b->active = false;
        return;
    }

    if(b->down[0] == k) {
        b->down[0] = 0;
    } else if(b->down[1] == k) {
        b->down[1] = 0;
    } else {
        return;                 // key up without coresponding down (menu pass through)
    }

    if(b->down[0] || b->down[1]) {
        return;                 // some other key is still holding it down
    }

    b->active = false;

    // save timestamp for partial frame summing
    c = cmdSystem->Argv(2);
    uptime = atoi(c);

    if(uptime) {
        b->msec += uptime - b->downtime;
    } else {
        b->msec += frame_msec / 2;
    }

    b->active = false;
}

/*
===============
idClientInputSystemLocal::KeyState

Returns the fraction of the frame that the key was down
===============
*/
float32 idClientInputSystemLocal::KeyState(kbutton_t *key) {
    sint msec;
    float32 val;

    msec = key->msec;
    key->msec = 0;

    if(key->active) {
        // still down
        if(!key->downtime) {
            msec = com_frameTime;
        } else {
            msec += com_frameTime - key->downtime;
        }

        key->downtime = com_frameTime;
    }

#if 0

    if(msec) {
        Com_Printf("%i ", msec);
    }

#endif

    val = static_cast<float32>(msec) / frame_msec;

    if(val < 0) {
        val = 0;
    }

    if(val > 1) {
        val = 1;
    }

    return val;
}

/*
===============
idClientInputSystemLocal::UpDown
===============
*/
void idClientInputSystemLocal::UpDown(void) {
    KeyDown(&kb[KB_UP]);
}

/*
===============
idClientInputSystemLocal::UpUp
===============
*/
void idClientInputSystemLocal::UpUp(void) {
    KeyUp(&kb[KB_UP]);
}

/*
===============
idClientInputSystemLocal::DownDown
===============
*/
void idClientInputSystemLocal::DownDown(void) {
    KeyDown(&kb[KB_DOWN]);
}

/*
===============
idClientInputSystemLocal::DownUp
===============
*/
void idClientInputSystemLocal::DownUp(void) {
    KeyUp(&kb[KB_DOWN]);
}

/*
===============
idClientInputSystemLocal::LeftDown
===============
*/
void idClientInputSystemLocal::LeftDown(void) {
    KeyDown(&kb[KB_LEFT]);
}

/*
===============
idClientInputSystemLocal::LeftUp
===============
*/
void idClientInputSystemLocal::LeftUp(void) {
    KeyUp(&kb[KB_LEFT]);
}

/*
===============
idClientInputSystemLocal::RightDown
===============
*/
void idClientInputSystemLocal::RightDown(void) {
    KeyDown(&kb[KB_RIGHT]);
}

/*
===============
idClientInputSystemLocal::RightUp
===============
*/
void idClientInputSystemLocal::RightUp(void) {
    KeyUp(&kb[KB_RIGHT]);
}

/*
===============
idClientInputSystemLocal::ForwardDown
===============
*/
void idClientInputSystemLocal::ForwardDown(void) {
    KeyDown(&kb[KB_FORWARD]);
}

/*
===============
idClientInputSystemLocal::ForwardUp
===============
*/
void idClientInputSystemLocal::ForwardUp(void) {
    KeyUp(&kb[KB_FORWARD]);
}

/*
===============
idClientInputSystemLocal::BackDown
===============
*/
void idClientInputSystemLocal::BackDown(void) {
    KeyDown(&kb[KB_BACK]);
}

/*
===============
idClientInputSystemLocal::BackUp
===============
*/
void idClientInputSystemLocal::BackUp(void) {
    KeyUp(&kb[KB_BACK]);
}

/*
===============
idClientInputSystemLocal::LookupDown
===============
*/
void idClientInputSystemLocal::LookupDown(void) {
    KeyDown(&kb[KB_LOOKUP]);
}

/*
===============
idClientInputSystemLocal::LookupUp
===============
*/
void idClientInputSystemLocal::LookupUp(void) {
    KeyUp(&kb[KB_LOOKUP]);
}

/*
===============
idClientInputSystemLocal::LookdownDown
===============
*/
void idClientInputSystemLocal::LookdownDown(void) {
    KeyDown(&kb[KB_LOOKDOWN]);
}

/*
===============
idClientInputSystemLocal::LookdownUp
===============
*/
void idClientInputSystemLocal::LookdownUp(void) {
    KeyUp(&kb[KB_LOOKDOWN]);
}

/*
===============
idClientInputSystemLocal::MoveleftDown
===============
*/
void idClientInputSystemLocal::MoveleftDown(void) {
    KeyDown(&kb[KB_MOVELEFT]);
}

/*
===============
idClientInputSystemLocal::MoveleftUp
===============
*/
void idClientInputSystemLocal::MoveleftUp(void) {
    KeyUp(&kb[KB_MOVELEFT]);
}

/*
===============
idClientInputSystemLocal::MoverightDown
===============
*/
void idClientInputSystemLocal::MoverightDown(void) {
    KeyDown(&kb[KB_MOVERIGHT]);
}

/*
===============
idClientInputSystemLocal::MoverightUp
===============
*/
void idClientInputSystemLocal::MoverightUp(void) {
    KeyUp(&kb[KB_MOVERIGHT]);
}

/*
===============
idClientInputSystemLocal::SpeedDown
===============
*/
void idClientInputSystemLocal::SpeedDown(void) {
    KeyDown(&kb[KB_SPEED]);
}

/*
===============
idClientInputSystemLocal::SpeedUp
===============
*/
void idClientInputSystemLocal::SpeedUp(void) {
    KeyUp(&kb[KB_SPEED]);
}

/*
===============
idClientInputSystemLocal::StrafeDown
===============
*/
void idClientInputSystemLocal::StrafeDown(void) {
    KeyDown(&kb[KB_STRAFE]);
}

/*
===============
idClientInputSystemLocal::StrafeUp
===============
*/
void idClientInputSystemLocal::StrafeUp(void) {
    KeyUp(&kb[KB_STRAFE]);
}

/*
===============
idClientInputSystemLocal::Button0Down
===============
*/
void idClientInputSystemLocal::Button0Down(void) {
    KeyDown(&kb[KB_BUTTONS0]);
}

/*
===============
idClientInputSystemLocal::Button0Up
===============
*/
void idClientInputSystemLocal::Button0Up(void) {
    KeyUp(&kb[KB_BUTTONS0]);
}

/*
===============
idClientInputSystemLocal::Button1Down
===============
*/
void idClientInputSystemLocal::Button1Down(void) {
    KeyDown(&kb[KB_BUTTONS1]);
}

/*
===============
idClientInputSystemLocal::Button1Up
===============
*/
void idClientInputSystemLocal::Button1Up(void) {
    KeyUp(&kb[KB_BUTTONS1]);
}

/*
===============
idClientInputSystemLocal::UseItemDown
===============
*/
void idClientInputSystemLocal::UseItemDown(void) {
    KeyDown(&kb[KB_BUTTONS2]);
}

/*
===============
idClientInputSystemLocal::UseItemUp
===============
*/
void idClientInputSystemLocal::UseItemUp(void) {
    KeyUp(&kb[KB_BUTTONS2]);
}

/*
===============
idClientInputSystemLocal::Button2Down
===============
*/
void idClientInputSystemLocal::Button2Down(void) {
    KeyDown(&kb[KB_BUTTONS2]);
}

/*
===============
idClientInputSystemLocal::Button2Up
===============
*/
void idClientInputSystemLocal::Button2Up(void) {
    KeyUp(&kb[KB_BUTTONS2]);
}

/*
===============
idClientInputSystemLocal::idClientInputSystemLocal
===============
*/
void idClientInputSystemLocal::Button3Down(void) {
    KeyDown(&kb[KB_BUTTONS3]);
}

/*
===============
idClientInputSystemLocal::Button3Up
===============
*/
void idClientInputSystemLocal::Button3Up(void) {
    KeyUp(&kb[KB_BUTTONS3]);
}

/*
===============
idClientInputSystemLocal::Button4Down
===============
*/
void idClientInputSystemLocal::Button4Down(void) {
    KeyDown(&kb[KB_BUTTONS4]);
}

/*
===============
idClientInputSystemLocal::Button4Up
===============
*/
void idClientInputSystemLocal::Button4Up(void) {
    KeyUp(&kb[KB_BUTTONS4]);
}

/*
===============
idClientInputSystemLocal::Button5Down
===============
*/
void idClientInputSystemLocal::Button5Down(void) {
    KeyDown(&kb[KB_BUTTONS5]);
}

/*
===============
idClientInputSystemLocal::Button5Up
===============
*/
void idClientInputSystemLocal::Button5Up(void) {
    KeyUp(&kb[KB_BUTTONS5]);
}

/*
===============
idClientInputSystemLocal::Button6Down
===============
*/
void idClientInputSystemLocal::Button6Down(void) {
    KeyDown(&kb[KB_BUTTONS6]);
}

/*
===============
idClientInputSystemLocal::Button6Up
===============
*/
void idClientInputSystemLocal::Button6Up(void) {
    KeyUp(&kb[KB_BUTTONS6]);
}

/*
===============
idClientInputSystemLocal::Button7Down
===============
*/
void idClientInputSystemLocal::Button7Down(void) {
    KeyDown(&kb[KB_BUTTONS7]);
}

/*
===============
idClientInputSystemLocal::Button7Up
===============
*/
void idClientInputSystemLocal::Button7Up(void) {
    KeyUp(&kb[KB_BUTTONS7]);
}

/*
===============
idClientInputSystemLocal::Button8Down
===============
*/
void idClientInputSystemLocal::Button8Down(void) {
    KeyDown(&kb[KB_BUTTONS8]);
}

/*
===============
idClientInputSystemLocal::Button8Up
===============
*/
void idClientInputSystemLocal::Button8Up(void) {
    KeyUp(&kb[KB_BUTTONS8]);
}

/*
===============
idClientInputSystemLocal::Button9Down
===============
*/
void idClientInputSystemLocal::Button9Down(void) {
    KeyDown(&kb[KB_BUTTONS9]);
}

/*
===============
idClientInputSystemLocal::Button9Up
===============
*/
void idClientInputSystemLocal::Button9Up(void) {
    KeyUp(&kb[KB_BUTTONS9]);
}

/*
===============
idClientInputSystemLocal::Button10Down
===============
*/
void idClientInputSystemLocal::Button10Down(void) {
    KeyDown(&kb[KB_BUTTONS10]);
}

/*
===============
idClientInputSystemLocal::Button10Up
===============
*/
void idClientInputSystemLocal::Button10Up(void) {
    KeyUp(&kb[KB_BUTTONS10]);
}

/*
===============
idClientInputSystemLocal::Button11Down
===============
*/
void idClientInputSystemLocal::Button11Down(void) {
    KeyDown(&kb[KB_BUTTONS11]);
}

/*
===============
idClientInputSystemLocal::Button11Up
===============
*/
void idClientInputSystemLocal::Button11Up(void) {
    KeyUp(&kb[KB_BUTTONS11]);
}

/*
===============
idClientInputSystemLocal::Button12Down
===============
*/
void idClientInputSystemLocal::Button12Down(void) {
    KeyDown(&kb[KB_BUTTONS12]);
}

/*
===============
idClientInputSystemLocal::Button12Up
===============
*/
void idClientInputSystemLocal::Button12Up(void) {
    KeyUp(&kb[KB_BUTTONS12]);
}

/*
===============
idClientInputSystemLocal::Button13Down
===============
*/
void idClientInputSystemLocal::Button13Down(void) {
    KeyDown(&kb[KB_BUTTONS13]);
}

/*
===============
idClientInputSystemLocal::Button13Up
===============
*/
void idClientInputSystemLocal::Button13Up(void) {
    KeyUp(&kb[KB_BUTTONS13]);
}

/*
===============
idClientInputSystemLocal::Button14Down
===============
*/
void idClientInputSystemLocal::Button14Down(void) {
    KeyDown(&kb[KB_BUTTONS14]);
}

/*
===============
idClientInputSystemLocal::Button14Up
===============
*/
void idClientInputSystemLocal::Button14Up(void) {
    KeyUp(&kb[KB_BUTTONS14]);
}

/*
===============
idClientInputSystemLocal::Button15Down
===============
*/
void idClientInputSystemLocal::Button15Down(void) {
    KeyDown(&kb[KB_BUTTONS15]);
}

/*
===============
idClientInputSystemLocal::Button15Up
===============
*/
void idClientInputSystemLocal::Button15Up(void) {
    KeyUp(&kb[KB_BUTTONS15]);
}

/*
===============
idClientInputSystemLocal::ActivateDown
===============
*/
void idClientInputSystemLocal::ActivateDown(void) {
    KeyDown(&kb[KB_BUTTONS6]);
}

/*
===============
idClientInputSystemLocal::ActivateUp
===============
*/
void idClientInputSystemLocal::ActivateUp(void) {
    KeyUp(&kb[KB_BUTTONS6]);
}

/*
===============
idClientInputSystemLocal::SprintDown
===============
*/
void idClientInputSystemLocal::SprintDown(void) {
    KeyDown(&kb[KB_BUTTONS5]);
}

/*
===============
idClientInputSystemLocal::SprintUp
===============
*/
void idClientInputSystemLocal::SprintUp(void) {
    KeyUp(&kb[KB_BUTTONS5]);
}

/*
===============
idClientInputSystemLocal::Wbutton0Down
===============
*/
void idClientInputSystemLocal::Wbutton0Down(void) {
    KeyDown(&kb[KB_WBUTTONS0]);
}

/*
===============
idClientInputSystemLocal::Wbutton0Up
===============
*/
void idClientInputSystemLocal::Wbutton0Up(void) {
    KeyUp(&kb[KB_WBUTTONS0]);
}

/*
===============
idClientInputSystemLocal::ZoomDown
===============
*/
void idClientInputSystemLocal::ZoomDown(void) {
    KeyDown(&kb[KB_WBUTTONS1]);
}

/*
===============
idClientInputSystemLocal::ZoomUp
===============
*/
void idClientInputSystemLocal::ZoomUp(void) {
    KeyUp(&kb[KB_WBUTTONS1]);
}

/*
===============
idClientInputSystemLocal::ReloadDown
===============
*/
void idClientInputSystemLocal::ReloadDown(void) {
    KeyDown(&kb[KB_WBUTTONS3]);
}

/*
===============
idClientInputSystemLocal::ReloadUp
===============
*/
void idClientInputSystemLocal::ReloadUp(void) {
    KeyUp(&kb[KB_WBUTTONS3]);
}

/*
===============
idClientInputSystemLocal::LeanLeftDown
===============
*/
void idClientInputSystemLocal::LeanLeftDown(void) {
    KeyDown(&kb[KB_WBUTTONS4]);
}

/*
===============
idClientInputSystemLocal::LeanLeftUp
===============
*/
void idClientInputSystemLocal::LeanLeftUp(void) {
    KeyUp(&kb[KB_WBUTTONS4]);
}

/*
===============
idClientInputSystemLocal::LeanRightDown
===============
*/
void idClientInputSystemLocal::LeanRightDown(void) {
    KeyDown(&kb[KB_WBUTTONS5]);
}

/*
===============
idClientInputSystemLocal::LeanRightUp
===============
*/
void idClientInputSystemLocal::LeanRightUp(void) {
    KeyUp(&kb[KB_WBUTTONS5]);
}

/*
===============
idClientInputSystemLocal::ProneDown
===============
*/
void idClientInputSystemLocal::ProneDown(void) {
    KeyDown(&kb[KB_WBUTTONS7]);
}

/*
===============
idClientInputSystemLocal::ProneUp
===============
*/
void idClientInputSystemLocal::ProneUp(void) {
    KeyUp(&kb[KB_WBUTTONS7]);
}

/*
===============
idClientInputSystemLocal::ButtonDown
===============
*/
void idClientInputSystemLocal::ButtonDown(void) {
    KeyDown(&kb[KB_BUTTONS1]);
}

/*
===============
idClientInputSystemLocal::ButtonUp
===============
*/
void idClientInputSystemLocal::ButtonUp(void) {
    KeyUp(&kb[KB_BUTTONS1]);
}


/*
===============
idClientInputSystemLocal::CenterView
===============
*/
void idClientInputSystemLocal::CenterView(void) {
    cl.viewangles[PITCH] = -SHORT2ANGLE(cl.snapServer.ps.delta_angles[PITCH]);
}

/*
===============
idClientInputSystemLocal::Notebook
===============
*/
void idClientInputSystemLocal::Notebook(void) {
    //if ( cls.state == CA_ACTIVE && !clc.demoplaying ) {
    //uiManager->SetActiveMenu( UIMENU_NOTEBOOK );
    //}
}

/*
===============
idClientInputSystemLocal::Help
===============
*/
void idClientInputSystemLocal::Help(void) {
    if(cls.state == CA_ACTIVE && !clc.demoplaying) {
        uiManager->SetActiveMenu(UIMENU_HELP);
    }
}

/*
================
idClientInputSystemLocal::AdjustAngles

Moves the local angle positions
================
*/
void idClientInputSystemLocal::AdjustAngles(void) {
    float32 speed;

    if(kb[KB_SPEED].active) {
        speed = (static_cast<float32>(cls.frametime) / 1000.0f) * cls.frametime *
                cl_anglespeedkey->value;
    } else {
        speed = static_cast<float32>(cls.frametime) / 1000.0f;
    }

    if(!kb[KB_STRAFE].active) {
        cl.viewangles[YAW] -= speed * cl_yawspeed->value * KeyState(
                                  &kb[KB_RIGHT]);
        cl.viewangles[YAW] += speed * cl_yawspeed->value * KeyState(
                                  &kb[KB_LEFT]);
    }

    cl.viewangles[PITCH] -= speed * cl_pitchspeed->value * KeyState(
                                &kb[KB_LOOKUP]);
    cl.viewangles[PITCH] += speed * cl_pitchspeed->value * KeyState(
                                &kb[KB_LOOKDOWN]);
}

/*
================
idClientInputSystemLocal::KeyMove

Sets the usercmd_t based on key states
================
*/
void idClientInputSystemLocal::KeyMove(usercmd_t *cmd) {
    sint movespeed, forward, side, up;

    // adjust for speed key / running
    // the walking flag is to keep animations consistant
    // even during acceleration and develeration
    if(kb[KB_SPEED].active ^ cl_run->integer) {
        movespeed = 127;
        cmd->buttons &= ~BUTTON_WALKING;
    } else {
        cmd->buttons |= BUTTON_WALKING;
        movespeed = 64;
    }

    forward = 0;
    side = 0;
    up = 0;

    if(kb[KB_STRAFE].active) {
        side += movespeed * KeyState(&kb[KB_RIGHT]);
        side -= movespeed * KeyState(&kb[KB_LEFT]);
    }

    side += movespeed * KeyState(&kb[KB_MOVERIGHT]);
    side -= movespeed * KeyState(&kb[KB_MOVELEFT]);

    //----(SA)  added
#if 0

    if(cmd->buttons & BUTTON_ACTIVATE) {
        if(side > 0) {
            cmd->wbuttons |= WBUTTON_LEANRIGHT;
        } else if(side < 0) {
            cmd->wbuttons |= WBUTTON_LEANLEFT;
        }

        side = 0;               // disallow the strafe when holding 'activate'
    }

#endif
    //----(SA)  end

    up += movespeed * KeyState(&kb[KB_UP]);
    up -= movespeed * KeyState(&kb[KB_DOWN]);

    forward += movespeed * KeyState(&kb[KB_FORWARD]);
    forward -= movespeed * KeyState(&kb[KB_BACK]);

    // fretn - moved this to bg_pmove.c
    //if (!(cl.snapServer.ps.persistant[PERS_HWEAPON_USE]))
    //{
    cmd->forwardmove = ClampChar(forward);
    cmd->rightmove = ClampChar(side);
    cmd->upmove = ClampChar(up);
    //}

    // Arnout: double tap
    cmd->doubleTap = DT_NONE;   // reset

    if(!cl.doubleTap.lastdoubleTap ||
            com_frameTime - cl.doubleTap.lastdoubleTap > cl_doubletapdelay->integer +
            cls.frametime) {
        // frametime for low(-ish) fps situations)
        sint i;
        bool key_down;

        for(i = 1; i < DT_NUM; i++) {
            key_down = static_cast<bool>(kb[dtmapping[i]].active ||
                                         kb[dtmapping[i]].wasPressed);

            if(key_down && !cl.doubleTap.pressedTime[i]) {
                cl.doubleTap.pressedTime[i] = com_frameTime;
            } else if(!key_down && !cl.doubleTap.releasedTime[i]
                      && (com_frameTime - cl.doubleTap.pressedTime[i]) <
                      (cl_doubletapdelay->integer + cls.frametime)) {
                cl.doubleTap.releasedTime[i] = com_frameTime;
            } else if(key_down &&
                      (com_frameTime - cl.doubleTap.pressedTime[i]) < (cl_doubletapdelay->integer
                              + cls.frametime)
                      && (com_frameTime - cl.doubleTap.releasedTime[i]) <
                      (cl_doubletapdelay->integer + cls.frametime)) {
                cl.doubleTap.pressedTime[i] = cl.doubleTap.releasedTime[i] = 0;
                cmd->doubleTap = i;
                cl.doubleTap.lastdoubleTap = com_frameTime;
            } else if(!key_down && (cl.doubleTap.pressedTime[i] ||
                                    cl.doubleTap.releasedTime[i])) {
                if(com_frameTime - cl.doubleTap.pressedTime[i] >=
                        (cl_doubletapdelay->integer + cls.frametime)) {
                    cl.doubleTap.pressedTime[i] = cl.doubleTap.releasedTime[i] = 0;
                }
            }
        }
    }
}

/*
=================
idClientInputSystemLocal::MouseEvent
=================
*/
void idClientInputSystemLocal::MouseEvent(sint dx, sint dy, sint time) {
    if(cls.keyCatchers & KEYCATCH_UI) {
        // NERVE - SMF - if we just want to pass it along to game
        if(cl_bypassMouseInput->integer == 1) {
            cl.mouseDx[cl.mouseIndex] += dx;
            cl.mouseDy[cl.mouseIndex] += dy;
        } else {
            uiManager->MouseEvent(dx, dy);
        }

    } else if(cls.keyCatchers & KEYCATCH_CGAME) {
        if(cl_bypassMouseInput->integer == 1) {
            cl.mouseDx[cl.mouseIndex] += dx;
            cl.mouseDy[cl.mouseIndex] += dy;
        } else {
            cgame->MouseEvent(dx, dy);
        }
    } else {
        cl.mouseDx[cl.mouseIndex] += dx;
        cl.mouseDy[cl.mouseIndex] += dy;
    }
}

/*
=================
idClientInputSystemLocal::JoystickEvent

Joystick values stay set until changed
=================
*/
void idClientInputSystemLocal::JoystickEvent(sint axis, sint value,
        sint time) {
    if(axis < 0 || axis >= MAX_JOYSTICK_AXIS) {
        Com_Error(ERR_DROP, "idClientInputSystemLocal::JoystickEvent: bad axis %i",
                  axis);
    }

    if(axis >= 0) {
        cl.joystickAxis[axis] = value;
    }
}

/*
=================
idClientInputSystemLocal::JoystickMove
=================
*/
void idClientInputSystemLocal::JoystickMove(usercmd_t *cmd) {
    sint movespeed;
    float32 anglespeed;

    if(kb[KB_SPEED].active ^ cl_run->integer) {
        movespeed = 2;
    } else {
        movespeed = 1;
        cmd->buttons |= BUTTON_WALKING;
    }

    if(kb[KB_SPEED].active) {
        anglespeed = (static_cast<float32>(cls.frametime) / 1000.0f) *
                     cls.frametime * cl_anglespeedkey->value;
    } else {
        anglespeed = static_cast<float32>(cls.frametime) / 1000.0f;
    }

#ifdef __MACOS__
    cmd->rightmove = ClampChar(cmd->rightmove + cl.joystickAxis[AXIS_SIDE]);
#else

    if(!kb[KB_STRAFE].active) {
        cl.viewangles[YAW] += anglespeed * j_yaw->value *
                              cl.joystickAxis[j_yaw_axis->integer];
        cmd->rightmove = ClampChar(cmd->rightmove + static_cast<sint>
                                   (j_side->value * cl.joystickAxis[j_side_axis->integer]));
    } else {
        cl.viewangles[YAW] += anglespeed * j_side->value *
                              cl.joystickAxis[j_side_axis->integer];
        cmd->rightmove = ClampChar(cmd->rightmove + static_cast<sint>
                                   (j_yaw->value * cl.joystickAxis[j_yaw_axis->integer]));
    }

#endif

    if(kb[KB_MLOOK].active) {
        cl.viewangles[PITCH] += anglespeed * j_forward->value *
                                cl.joystickAxis[j_forward_axis->integer];
        cmd->forwardmove = ClampChar(cmd->forwardmove + static_cast<sint>
                                     (j_pitch->value * cl.joystickAxis[j_pitch_axis->integer]));
    } else {
        cl.viewangles[PITCH] += anglespeed * j_pitch->value *
                                cl.joystickAxis[j_pitch_axis->integer];
        cmd->forwardmove = ClampChar(cmd->forwardmove + static_cast<sint>
                                     (j_forward->value * cl.joystickAxis[j_forward_axis->integer]));
    }

    cmd->upmove = ClampChar(cmd->upmove + static_cast<sint>
                            (j_up->value * cl.joystickAxis[j_up_axis->integer]));
}

/*
=================
idClientInputSystemLocal::Xbox360ControllerMove
=================
*/
void idClientInputSystemLocal::Xbox360ControllerMove(usercmd_t *cmd) {
    sint movespeed;
    float32 anglespeed;

    if(kb[KB_SPEED].active ^ cl_run->integer) {
        movespeed = 2;
    } else {
        movespeed = 1;
        cmd->buttons |= BUTTON_WALKING;
    }

    if(kb[KB_SPEED].active) {
        anglespeed = 0.001 * cls.frametime * cl_anglespeedkey->value;
    } else {
        anglespeed = 0.001 * cls.frametime;
    }

    cl.viewangles[PITCH] += anglespeed * cl_pitchspeed->value *
                            (cl.joystickAxis[j_pitch_axis->integer] / 127.0f);
    cl.viewangles[YAW] += anglespeed * cl_yawspeed->value *
                          (cl.joystickAxis[j_yaw_axis->integer] / 127.0f);

    cmd->rightmove = ClampChar(cmd->rightmove +
                               cl.joystickAxis[j_side_axis->integer]);
    cmd->forwardmove = ClampChar(cmd->forwardmove +
                                 cl.joystickAxis[j_forward_axis->integer]);
    cmd->upmove = ClampChar(cmd->upmove + cl.joystickAxis[j_up_axis->integer]);
}

/*
=================
idClientInputSystemLocal::MouseMove
=================
*/
void idClientInputSystemLocal::MouseMove(usercmd_t *cmd) {
    float32 mx, my;

    // allow mouse smoothing
    if(m_filter->integer) {
        mx = (cl.mouseDx[0] + cl.mouseDx[1]) * 0.5f;
        my = (cl.mouseDy[0] + cl.mouseDy[1]) * 0.5f;
    } else {
        mx = cl.mouseDx[cl.mouseIndex];
        my = cl.mouseDy[cl.mouseIndex];
    }

    cl.mouseIndex ^= 1;
    cl.mouseDx[cl.mouseIndex] = 0;
    cl.mouseDy[cl.mouseIndex] = 0;

    if(mx == 0.0f && my == 0.0f) {
        return;
    }

    if(cl_mouseAccel->value != 0.0f) {
        if(cl_mouseAccel->value != 0.0f && cl_mouseAccelOffset->value > 0.0f) {
            float32 accelSensitivity;
            float32 rate;

            rate = sqrt(mx * mx + my * my) / static_cast<float32>(frame_msec);

            accelSensitivity = sensitivity->value + rate * cl_mouseAccel->value;
            mx *= accelSensitivity;
            my *= accelSensitivity;

            if(cl_showMouseRate->integer) {
                Com_Printf("rate: %f, accelSensitivity: %f\n", rate, accelSensitivity);
            }
        } else {
            float32 rate[2];
            float32 power[2];

            // sensitivity remains pretty much unchanged at low speeds
            // cl_mouseAccel is a power value to how the acceleration is shaped
            // cl_mouseAccelOffset is the rate for which the acceleration will have doubled the non accelerated amplification
            // NOTE: decouple the config cvars for independent acceleration setup along X and Y?

            rate[0] = fabs(mx) / static_cast<float32>(frame_msec);
            rate[1] = fabs(my) / static_cast<float32>(frame_msec);
            power[0] = powf(rate[0] / cl_mouseAccelOffset->value,
                            cl_mouseAccel->value);
            power[1] = powf(rate[1] / cl_mouseAccelOffset->value,
                            cl_mouseAccel->value);

            mx = sensitivity->value * (mx + ((mx < 0) ? -power[0] : power[0]) *
                                       cl_mouseAccelOffset->value);
            my = sensitivity->value * (my + ((my < 0) ? -power[1] : power[1]) *
                                       cl_mouseAccelOffset->value);

            /*  NERVE - SMF - this has moved to CG_CalcFov to fix zoomed-in/out transition movement bug
                if ( cl.snapServer.ps.stats[STAT_ZOOMED_VIEW] ) {
                    if(cl.snapServer.ps.weapon == WP_SNIPERRIFLE) {
                        accelSensitivity *= 0.1;
                    }
                    else if(cl.snapServer.ps.weapon == WP_SNOOPERSCOPE) {
                        accelSensitivity *= 0.2;
                    }
                }
            */
            if(cl_showMouseRate->integer) {
                Com_Printf("ratex: %f, ratey: %f, powx: %f, powy: %f\n", rate[0], rate[1],
                           power[0], power[1]);
            }
        }
    } else {
        mx *= sensitivity->value;
        my *= sensitivity->value;
    }

    // Ridah, experimenting with a slow tracking gun
#if 0

    // Rafael - mg42
    if(cl.snapServer.ps.persistant[PERS_HWEAPON_USE]) {
        mx *= 2.5;              //(accelSensitivity * 0.1);
        my *= 2;                //(accelSensitivity * 0.075);
    } else {
        mx *= sensitivity->value;
        my *= sensitivity->value;
    }

#endif

    // ingame FOV
    mx *= cl.cgameSensitivity;
    my *= cl.cgameSensitivity;

    // add mouse X/Y movement to cmd
    if(kb[KB_STRAFE].active) {
        cmd->rightmove = ClampChar(cmd->rightmove + m_side->value * mx);
    } else {
        cl.viewangles[YAW] -= m_yaw->value * mx;

        // limit yaw between -180 and 180 to avoid accumulating
        // floating-point precision errors
        while(cl.viewangles[YAW] < -180.0f) {
            cl.viewangles[YAW] += 360.0f;
        }

        while(cl.viewangles[YAW] > 180.0f) {
            cl.viewangles[YAW] -= 360.0f;
        }
    }

    if((kb[KB_MLOOK].active || cl_freelook->integer) &&
            !kb[KB_STRAFE].active) {
        cl.viewangles[PITCH] += m_pitch->value * my;

        // limit pitch between -180 and 180 to avoid accumulating
        // floating-point precision errors
        while(cl.viewangles[PITCH] < -180.0f) {
            cl.viewangles[PITCH] = -180.0f;
        }

        while(cl.viewangles[PITCH] > 180.0f) {
            cl.viewangles[PITCH] = 180.0f;
        }
    } else {
        cmd->forwardmove = ClampChar(cmd->forwardmove - m_forward->value * my);
    }
}

/*
==============
idClientInputSystemLocal::CmdButtons
==============
*/
void idClientInputSystemLocal::CmdButtons(usercmd_t *cmd) {
    sint i;

    //
    // figure button bits
    // send a button bit even if the key was pressed and released in
    // less than a frame
    //
    for(i = 0; i < 15; i++) {
        if(kb[KB_BUTTONS0 + i].active || kb[KB_BUTTONS0 + i].wasPressed) {
            cmd->buttons |= 1 << i;
        }

        kb[KB_BUTTONS0 + i].wasPressed = false;
    }

    for(i = 0; i < 8; i++) {
        // Arnout: this was i < 7, but there are 8 wbuttons
        if(kb[KB_WBUTTONS0 + i].active || kb[KB_WBUTTONS0 + i].wasPressed) {
            cmd->wbuttons |= 1 << i;
        }

        kb[KB_WBUTTONS0 + i].wasPressed = false;
    }

    if(cls.keyCatchers && !cl_bypassMouseInput->integer) {
        cmd->buttons |= BUTTON_TALK;
    }

    // allow the game to know if any key at all is
    // currently pressed, even if it isn't bound to anything
    if(anykeydown && (!cls.keyCatchers || cl_bypassMouseInput->integer)) {
        cmd->buttons |= BUTTON_ANY;
    }

    // Arnout: clear 'waspressed' from double tap buttons
    for(i = 1; i < DT_NUM; i++) {
        kb[dtmapping[i]].wasPressed = false;
    }
}

/*
==============
idClientInputSystemLocal::FinishMove
==============
*/
void idClientInputSystemLocal::FinishMove(usercmd_t *cmd) {
    sint i;

    // copy the state that the cgame is currently sending
    cmd->weapon = static_cast<uchar8>(cl.cgameUserCmdValue);

    cmd->flags = cl.cgameFlags;

    cmd->identClient = cl.cgameMpIdentClient;   // NERVE - SMF

    // send the current server time so the amount of movement
    // can be determined without allowing cheating
    cmd->serverTime = cl.serverTime;

    for(i = 0; i < 3; i++) {
        cmd->angles[i] = ANGLE2SHORT(cl.viewangles[i]);
    }
}

/*
=================
idClientInputSystemLocal::CreateCmd
=================
*/
usercmd_t idClientInputSystemLocal::CreateCmd(void) {
    float32 recoilAdd;
    usercmd_t cmd;
    vec3_t oldAngles;

    VectorCopy(cl.viewangles, oldAngles);

    // keyboard angle adjustment
    AdjustAngles();

    ::memset(&cmd, 0, sizeof(cmd));

    CmdButtons(&cmd);

    // get basic movement from keyboard
    KeyMove(&cmd);

    // get basic movement from mouse
    MouseMove(&cmd);

    // get basic movement from mouse
    MouseMove(&cmd);

    // get basic movement from joystick or controller
    if(cl_xbox360ControllerAvailable->integer) {
        Xbox360ControllerMove(&cmd);
    } else {
        JoystickMove(&cmd);
    }

    // check to make sure the angles haven't wrapped
    if(cl.viewangles[PITCH] - oldAngles[PITCH] > 90) {
        cl.viewangles[PITCH] = oldAngles[PITCH] + 90;
    } else if(oldAngles[PITCH] - cl.viewangles[PITCH] > 90) {
        cl.viewangles[PITCH] = oldAngles[PITCH] - 90;
    }

    // RF, set the kickAngles so aiming is effected
    recoilAdd = cl_recoilPitch->value;

    if(Q_fabs(cl.viewangles[PITCH] + recoilAdd) < 40) {
        cl.viewangles[PITCH] += recoilAdd;
    }

    // the recoilPitch has been used, so clear it out
    cl_recoilPitch->value = 0;

    // store out the final values
    FinishMove(&cmd);

    // draw debug graphs of turning for mouse testing
    if(cl_debugMove->integer) {
        if(cl_debugMove->integer == 1) {
            clientScreenSystem->DebugGraph(fabs(cl.viewangles[YAW] - oldAngles[YAW]),
                                           0);
        }

        if(cl_debugMove->integer == 2) {
            clientScreenSystem->DebugGraph(fabs(cl.viewangles[PITCH] -
                                                oldAngles[PITCH]), 0);
        }
    }

    return cmd;
}

/*
=================
idClientInputSystemLocal::CreateNewCommands

Create a new usercmd_t structure for this frame
=================
*/
void idClientInputSystemLocal::CreateNewCommands(void) {
    sint cmdNum;
    usercmd_t *cmd;

    // no need to create usercmds until we have a gamestate
    if(cls.state < CA_PRIMED) {
        return;
    }

    frame_msec = com_frameTime - old_com_frameTime;

    // if running over 1000fps, act as if each frame is 1ms
    // prevents division by zero
    if(frame_msec < 1) {
        frame_msec = 1;
    }

    // if running less than 5fps, truncate the extra time to prevent
    // unexpected moves after a hitch
    if(frame_msec > 200) {
        frame_msec = 200;
    }

    old_com_frameTime = com_frameTime;


    // generate a command for this frame
    cl.cmdNumber++;
    cmdNum = cl.cmdNumber & CMD_MASK;
    cl.cmds[cmdNum] = CreateCmd();
    cmd = &cl.cmds[cmdNum];
}

/*
=================
idClientInputSystemLocal::ReadyToSendPacket

Returns false if we are over the maxpackets limit
and should choke back the bandwidth a bit by not sending
a packet this frame.  All the commands will still get
delivered in the next packet, but saving a header and
getting more delta compression will reduce total bandwidth.
=================
*/
bool idClientInputSystemLocal::ReadyToSendPacket(void) {
    sint oldPacketNum, delta;

    // don't send anything if playing back a demo
    if(clc.demoplaying) {
        return false;
    }

    // If we are downloading, we send no less than 50ms between packets
    if(*cls.downloadTempName && cls.realtime - clc.lastPacketSentTime < 50) {
        return false;
    }

    // if we don't have a valid gamestate yet, only send
    // one packet a second
    if(cls.state != CA_ACTIVE && cls.state != CA_PRIMED &&
            !*cls.downloadTempName && cls.realtime - clc.lastPacketSentTime < 1000) {
        return false;
    }

    // send every frame for loopbacks
    if(clc.netchan.remoteAddress.type == NA_LOOPBACK) {
        return true;
    }

    // send every frame for LAN
    if(networkSystem->IsLANAddress(clc.netchan.remoteAddress)) {
        return true;
    }

    // check for exceeding cl_maxpackets
    if(cl_maxpackets->integer < 1) {
        cvarSystem->Set("cl_maxpackets", "1");
    } else if(cl_maxpackets->integer > 2000) {
        cvarSystem->Set("cl_maxpackets", "2000");
    }

    oldPacketNum = (clc.netchan.outgoingSequence - 1) & PACKET_MASK;
    delta = cls.realtime - cl.outPackets[oldPacketNum].p_realtime;

    if(delta < 1000 / cl_maxpackets->integer) {
        // the accumulated commands will go out in the next packet
        return false;
    }

    return true;
}

/*
===================
idClientInputSystemLocal::WritePacket

Create and send the command packet to the server
Including both the reliable commands and the usercmds

During normal gameplay, a client packet will contain something like:

4   sequence number
2   qport
4   serverid
4   acknowledged sequence number
4   clc.serverCommandSequence
<optional reliable commands>
1   clc_move or clc_moveNoDelta
1   command count
<count * usercmds>

===================
*/
void idClientInputSystemLocal::WritePacket(void) {
    sint i, j, packetNum, oldPacketNum, count, key;
    usercmd_t *cmd, *oldcmd, nullcmd;
    msg_t buf;
    uchar8 data[MAX_MSGLEN];

    // don't send anything if playing back a demo
    if(clc.demoplaying) {
        return;
    }

    ::memset(&nullcmd, 0, sizeof(nullcmd));
    oldcmd = &nullcmd;

    MSG_Init(&buf, data, sizeof(data));

    MSG_Bitstream(&buf);
    // write the current serverId so the server
    // can tell if this is from the current gameState
    MSG_WriteLong(&buf, cl.serverId);

    // write the last message we received, which can
    // be used for delta compression, and is also used
    // to tell if we dropped a gamestate
    MSG_WriteLong(&buf, clc.serverMessageSequence);

    // write the last reliable message we received
    MSG_WriteLong(&buf, clc.serverCommandSequence);

    // write any unacknowledged clientCommands
    // NOTE TTimo: if you verbose this, you will see that there are quite a few duplicates
    // typically several unacknowledged cp or userinfo commands stacked up
    for(i = clc.reliableAcknowledge + 1; i <= clc.reliableSequence; i++) {
        MSG_WriteByte(&buf, clc_clientCommand);
        MSG_WriteLong(&buf, i);
        MSG_WriteString(&buf, clc.reliableCommands[i & (MAX_RELIABLE_COMMANDS -
                          1)]);
    }

    // we want to send all the usercmds that were generated in the last
    // few packet, so even if a couple packets are dropped in a row,
    // all the cmds will make it to the server
    if(cl_packetdup->integer < 0) {
        cvarSystem->Set("cl_packetdup", "0");
    } else if(cl_packetdup->integer > 1000) {
        cvarSystem->Set("cl_packetdup", "1000");
    }

    oldPacketNum = (clc.netchan.outgoingSequence - 1 - cl_packetdup->integer) &
                   PACKET_MASK;
    count = cl.cmdNumber - cl.outPackets[oldPacketNum].p_cmdNumber;

    if(count > MAX_PACKET_USERCMDS) {
        count = MAX_PACKET_USERCMDS;
        Com_Printf("MAX_PACKET_USERCMDS\n");
    }

    if(count >= 1) {
        if(cl_showSend->integer) {
            Com_Printf("(%i)", count);
        }

        // begin a client move command
        if(cl_nodelta->integer || !cl.snapServer.valid || clc.demowaiting ||
                clc.serverMessageSequence != cl.snapServer.messageNum) {
            MSG_WriteByte(&buf, clc_moveNoDelta);
        } else {
            MSG_WriteByte(&buf, clc_move);
        }

        // write the command count
        MSG_WriteByte(&buf, count);

        // use the checksum feed in the key
        key = clc.checksumFeed;
        // also use the message acknowledge
        key ^= clc.serverMessageSequence;
        // also use the last acknowledged server command in the key
        key ^= Com_HashKey(clc.serverCommands[clc.serverCommandSequence &
                                                                        (MAX_RELIABLE_COMMANDS - 1)], 32);

        // write all the commands, including the predicted command
        for(i = 0; i < count; i++) {
            j = (cl.cmdNumber - count + i + 1) & CMD_MASK;
            cmd = &cl.cmds[j];
            MSG_WriteDeltaUsercmdKey(&buf, key, oldcmd, cmd);
            oldcmd = cmd;
        }
    }

    //
    // deliver the message
    //
    packetNum = clc.netchan.outgoingSequence & PACKET_MASK;
    cl.outPackets[packetNum].p_realtime = cls.realtime;
    cl.outPackets[packetNum].p_serverTime = oldcmd->serverTime;
    cl.outPackets[packetNum].p_cmdNumber = cl.cmdNumber;
    clc.lastPacketSentTime = cls.realtime;

    if(cl_showSend->integer) {
        Com_Printf("%i ", buf.cursize);
    }

    clientNetChanSystem->Netchan_Transmit(&clc.netchan, &buf);

    // clients never really should have messages large enough
    // to fragment, but in case they do, fire them all off
    // at once
    // TTimo: this causes a packet burst, which is bad karma for winsock
    // added a WARNING message, we'll see if there are legit situations where this happens
    while(clc.netchan.unsentFragments) {
        if(cl_showSend->integer) {
            Com_Printf("WARNING: unsent fragments (not supposed to happen!)\n");
        }

        clientNetChanSystem->Netchan_TransmitNextFragment(&clc.netchan);
    }
}

/*
=================
idClientInputSystemLocal::SendCmd

Called every frame to builds and sends a command packet to the server.
=================
*/
void idClientInputSystemLocal::SendCmd(void) {
    // don't send any message if not connected
    if(cls.state < CA_CONNECTED) {
        return;
    }

    // don't send commands if paused
    if(sv_running->integer && sv_paused->integer && cl_paused->integer) {
        return;
    }

    // we create commands even if a demo is playing,
    CreateNewCommands();

    // don't send a packet if the last packet was sent too recently
    if(!ReadyToSendPacket()) {
        if(cl_showSend->integer) {
            Com_Printf(". ");
        }

        return;
    }

    WritePacket();
}

/*
============
idClientInputSystemLocal::InitInput
============
*/
void idClientInputSystemLocal::InitInput(void) {
    Com_Printf("----- idClientInputSystemLocal::InitInput -------\n");

    //cmdSystem->AddCommand ("centerview", CenterView, "Centers view on screen");

    cmdSystem->AddCommand("+moveup", UpDown, "Move up, i.e. Jump");
    cmdSystem->AddCommand("-moveup", UpUp, "Stop issuing command to jump");
    cmdSystem->AddCommand("+movedown", DownDown, "Move downwards, crouch");
    cmdSystem->AddCommand("-movedown", DownUp,
                          "Stop issing command to crouch");
    cmdSystem->AddCommand("+left", LeftDown, "Look left");
    cmdSystem->AddCommand("-left", LeftUp,
                          "Stop issuing command to look further to the left");
    cmdSystem->AddCommand("+right", RightDown, "Rotate camera right");
    cmdSystem->AddCommand("-right", RightUp,
                          "Stops issuing look right command");
    cmdSystem->AddCommand("+forward", ForwardDown, "Move forward");
    cmdSystem->AddCommand("-forward", ForwardUp,
                          "Stop issuing command to move forwards");
    cmdSystem->AddCommand("+back", BackDown, "Move backward");
    cmdSystem->AddCommand("-back", BackUp,
                          "Stop issuing command to move backwards");
    cmdSystem->AddCommand("+lookup", LookupDown, "Tilt camera up");
    cmdSystem->AddCommand("-lookup", LookupUp,
                          "Stop issuing command to look further upwards");
    cmdSystem->AddCommand("+lookdown", LookdownDown, "Tilt camera down");
    cmdSystem->AddCommand("-lookdown", LookdownUp,
                          "Stop issuing command to look further downwards");
    cmdSystem->AddCommand("+strafe", StrafeDown, "Hold to strafe");
    cmdSystem->AddCommand("-strafe", StrafeUp,
                          "Stops issuing strafe command");
    cmdSystem->AddCommand("+moveleft", MoveleftDown,
                          "Strafe/sidestep to the left");
    cmdSystem->AddCommand("-moveleft", MoveleftUp,
                          "Stop issuing command to strafe left");
    cmdSystem->AddCommand("+moveright", MoverightDown,
                          "Strafe/sidestep to the right");
    cmdSystem->AddCommand("-moveright", MoverightUp,
                          "Stop issuing command to strafe right");
    cmdSystem->AddCommand("+speed", SpeedDown, "Walk or run");
    cmdSystem->AddCommand("-speed", SpeedUp,
                          "Stops issuing walk/run command");

    cmdSystem->AddCommand("+attack", Button0Down,
                          "Fires weapon, or uses the weaponbank object currently selected");     // ---- id   (primary firing)
    cmdSystem->AddCommand("-attack", Button0Up,
                          "Stops issuing command to attack");

    cmdSystem->AddCommand("+button0", Button0Down, "Button0");
    cmdSystem->AddCommand("-button0", Button0Up,
                          "Stop issuing command button0");

    cmdSystem->AddCommand("+button1", Button1Down, "Button1");
    cmdSystem->AddCommand("-button1", Button1Up,
                          "Stop issuing command button1");

    cmdSystem->AddCommand("+button2", Button2Down, "Button2");
    cmdSystem->AddCommand("-button2", Button2Up,
                          "Stop issuing command button2");

    cmdSystem->AddCommand("+useitem", UseItemDown, "Use selected item");
    cmdSystem->AddCommand("-useitem", UseItemUp,
                          "Stop issuing command for selected item");

    cmdSystem->AddCommand("+salute", Button3Down, "Salute");
    cmdSystem->AddCommand("-salute", Button3Up,
                          "Stop issuing salute command");

    cmdSystem->AddCommand("+button3", Button3Down, "Button3");
    cmdSystem->AddCommand("-button3", Button3Up,
                          "Stop issuing command button3");

    cmdSystem->AddCommand("+button4", Button4Down, "Button4");
    cmdSystem->AddCommand("-button4", Button4Up,
                          "Stop issuing command button4");

    cmdSystem->AddCommand("+button5", Button5Down, "Button5");
    cmdSystem->AddCommand("-button5", Button5Up,
                          "Stop issuing command button5");

    cmdSystem->AddCommand("+button6", Button6Down, "Button6");
    cmdSystem->AddCommand("-button6", Button6Up,
                          "Stop issuing command button6");

    cmdSystem->AddCommand("+button7", Button7Down, "Button7");
    cmdSystem->AddCommand("-button7", Button7Up,
                          "Stop issuing command button7");

    cmdSystem->AddCommand("+button8", Button8Down, "Button8");
    cmdSystem->AddCommand("-button8", Button8Up,
                          "Stop issuing command button9");

    cmdSystem->AddCommand("+button9", Button9Down, "Button9");
    cmdSystem->AddCommand("-button9", Button9Up,
                          "Stop issuing command button9");

    cmdSystem->AddCommand("+button10", Button10Down, "Button10");
    cmdSystem->AddCommand("-button10", Button10Up,
                          "Stop issuing command button10");

    cmdSystem->AddCommand("+button11", Button11Down, "Button11");
    cmdSystem->AddCommand("-button11", Button11Up,
                          "Stop issuing command button11");

    cmdSystem->AddCommand("+button12", Button12Down, "Button12");
    cmdSystem->AddCommand("-button12", Button12Up,
                          "Stop issuing command button12");

    cmdSystem->AddCommand("+button13", Button13Down, "Button13");
    cmdSystem->AddCommand("-button13", Button13Up,
                          "Stop issuing command button13");

    cmdSystem->AddCommand("+button14", Button14Down, "Button14");
    cmdSystem->AddCommand("-button14", Button14Up,
                          "Stop issuing command button14");

    // Rafael Activate
    cmdSystem->AddCommand("+activate", ActivateDown,
                          "Performs various actions like opening doors, picking up weapons.");
    cmdSystem->AddCommand("-activate", ActivateUp,
                          "Stops issuing +activate command, stop opening doors etc");
    // done.

    // Rafael Kick
    // Arnout: now prone
    cmdSystem->AddCommand("+prone", ProneDown, "Go prone, lie down.");
    cmdSystem->AddCommand("-prone", ProneUp,
                          "Stop issuing command to go prone");
    // done

    cmdSystem->AddCommand("+dodge", ProneDown, "dodge");
    cmdSystem->AddCommand("-dodge", ProneUp, "dodge");

    cmdSystem->AddCommand("+sprint", SprintDown,
                          "Sprint, run fast draining stanima bar");
    cmdSystem->AddCommand("-sprint", SprintUp,
                          "Stops issuing sprint command");


    // wolf buttons
    cmdSystem->AddCommand("+attack2", Wbutton0Down,
                          "Secondary firing mode");   //----(SA) secondary firing
    cmdSystem->AddCommand("-attack2", Wbutton0Up,
                          "Stop issuing command to perform secondary attack");
    cmdSystem->AddCommand("+zoom", ZoomDown, "Zoom command");     //
    cmdSystem->AddCommand("-zoom", ZoomUp, "Stops issuing zoom command");
    cmdSystem->AddCommand("+reload", ReloadDown, "Reload weapon");    //
    cmdSystem->AddCommand("-reload", ReloadUp,
                          "Stops issuing reload command");
    cmdSystem->AddCommand("+leanleft", LeanLeftDown, "Leans to the left");
    cmdSystem->AddCommand("-leanleft", LeanLeftUp,
                          "Stop issuing command to lean to the left");
    cmdSystem->AddCommand("+leanright", LeanRightDown,
                          "Leans to the right");
    cmdSystem->AddCommand("-leanright", LeanRightUp,
                          "Stop issuing command to lean to the right");


    cmdSystem->AddCommand("+mlook", MLookDown, "Toggles mouselook");
    cmdSystem->AddCommand("-mlook", MLookUp,
                          "Stop +mlook (mouselook), go back to mouse-movement");

    //cmdSystem->AddCommand ("notebook",Notebook);
    cmdSystem->AddCommand("help", Help, "Toggles help");
}

/*
============
idClientInputSystemLocal::ClearKeys
============
*/
void idClientInputSystemLocal::ClearKeys(void) {
    ::memset(kb, 0, sizeof(kb));
}
