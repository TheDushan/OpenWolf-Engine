////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2019 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   cl_input.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: builds an intended movement command to send to the server
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.h>

U32 frame_msec;
S32 old_com_frameTime;

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
static kbuttons_t dtmapping[] =
{
    ( kbuttons_t ) - 1,				// DT_NONE
    KB_MOVELEFT,				// DT_MOVELEFT
    KB_MOVERIGHT,				// DT_MOVERIGHT
    KB_FORWARD,					// DT_FORWARD
    KB_BACK,					// DT_BACK
    KB_WBUTTONS4,				// DT_LEANLEFT
    KB_WBUTTONS5,				// DT_LEANRIGHT
    KB_UP						// DT_UP
};

void IN_MLookDown( void )
{
    kb[KB_MLOOK].active = true;
}

void IN_MLookUp( void )
{
    kb[KB_MLOOK].active = false;
    if( !cl_freelook->integer )
    {
        //IN_CenterView ();
    }
}

void IN_KeyDown( kbutton_t* b )
{
    S32             k;
    UTF8*           c;
    
    c = cmdSystem->Argv( 1 );
    if( c[0] )
    {
        k = atoi( c );
    }
    else
    {
        k = -1;					// typed manually at the console for continuous down
    }
    
    if( k == b->down[0] || k == b->down[1] )
    {
        return;					// repeating key
    }
    
    if( !b->down[0] )
    {
        b->down[0] = k;
    }
    else if( !b->down[1] )
    {
        b->down[1] = k;
    }
    else
    {
        Com_Printf( "Three keys down for a button!\n" );
        return;
    }
    
    if( b->active )
    {
        return;					// still down
    }
    
    // save timestamp for partial frame summing
    c = cmdSystem->Argv( 2 );
    b->downtime = atoi( c );
    
    b->active = true;
    b->wasPressed = true;
}

void IN_KeyUp( kbutton_t* b )
{
    S32             k;
    UTF8*           c;
    U32        uptime;
    
    c = cmdSystem->Argv( 1 );
    if( c[0] )
    {
        k = atoi( c );
    }
    else
    {
        // typed manually at the console, assume for unsticking, so clear all
        b->down[0] = b->down[1] = 0;
        b->active = false;
        return;
    }
    
    if( b->down[0] == k )
    {
        b->down[0] = 0;
    }
    else if( b->down[1] == k )
    {
        b->down[1] = 0;
    }
    else
    {
        return;					// key up without coresponding down (menu pass through)
    }
    if( b->down[0] || b->down[1] )
    {
        return;					// some other key is still holding it down
    }
    
    b->active = false;
    
    // save timestamp for partial frame summing
    c = cmdSystem->Argv( 2 );
    uptime = atoi( c );
    if( uptime )
    {
        b->msec += uptime - b->downtime;
    }
    else
    {
        b->msec += frame_msec / 2;
    }
    
    b->active = false;
}



/*
===============
CL_KeyState

Returns the fraction of the frame that the key was down
===============
*/
F32 CL_KeyState( kbutton_t* key )
{
    F32           val;
    S32             msec;
    
    msec = key->msec;
    key->msec = 0;
    
    if( key->active )
    {
        // still down
        if( !key->downtime )
        {
            msec = com_frameTime;
        }
        else
        {
            msec += com_frameTime - key->downtime;
        }
        key->downtime = com_frameTime;
    }
    
#if 0
    if( msec )
    {
        Com_Printf( "%i ", msec );
    }
#endif
    
    val = ( F32 )msec / frame_msec;
    if( val < 0 )
    {
        val = 0;
    }
    if( val > 1 )
    {
        val = 1;
    }
    
    return val;
}



void IN_UpDown( void )
{
    IN_KeyDown( &kb[KB_UP] );
}
void IN_UpUp( void )
{
    IN_KeyUp( &kb[KB_UP] );
}
void IN_DownDown( void )
{
    IN_KeyDown( &kb[KB_DOWN] );
}
void IN_DownUp( void )
{
    IN_KeyUp( &kb[KB_DOWN] );
}
void IN_LeftDown( void )
{
    IN_KeyDown( &kb[KB_LEFT] );
}
void IN_LeftUp( void )
{
    IN_KeyUp( &kb[KB_LEFT] );
}
void IN_RightDown( void )
{
    IN_KeyDown( &kb[KB_RIGHT] );
}
void IN_RightUp( void )
{
    IN_KeyUp( &kb[KB_RIGHT] );
}
void IN_ForwardDown( void )
{
    IN_KeyDown( &kb[KB_FORWARD] );
}
void IN_ForwardUp( void )
{
    IN_KeyUp( &kb[KB_FORWARD] );
}
void IN_BackDown( void )
{
    IN_KeyDown( &kb[KB_BACK] );
}
void IN_BackUp( void )
{
    IN_KeyUp( &kb[KB_BACK] );
}
void IN_LookupDown( void )
{
    IN_KeyDown( &kb[KB_LOOKUP] );
}
void IN_LookupUp( void )
{
    IN_KeyUp( &kb[KB_LOOKUP] );
}
void IN_LookdownDown( void )
{
    IN_KeyDown( &kb[KB_LOOKDOWN] );
}
void IN_LookdownUp( void )
{
    IN_KeyUp( &kb[KB_LOOKDOWN] );
}
void IN_MoveleftDown( void )
{
    IN_KeyDown( &kb[KB_MOVELEFT] );
}
void IN_MoveleftUp( void )
{
    IN_KeyUp( &kb[KB_MOVELEFT] );
}
void IN_MoverightDown( void )
{
    IN_KeyDown( &kb[KB_MOVERIGHT] );
}
void IN_MoverightUp( void )
{
    IN_KeyUp( &kb[KB_MOVERIGHT] );
}

void IN_SpeedDown( void )
{
    IN_KeyDown( &kb[KB_SPEED] );
}
void IN_SpeedUp( void )
{
    IN_KeyUp( &kb[KB_SPEED] );
}
void IN_StrafeDown( void )
{
    IN_KeyDown( &kb[KB_STRAFE] );
}
void IN_StrafeUp( void )
{
    IN_KeyUp( &kb[KB_STRAFE] );
}

void IN_Button0Down( void )
{
    IN_KeyDown( &kb[KB_BUTTONS0] );
}
void IN_Button0Up( void )
{
    IN_KeyUp( &kb[KB_BUTTONS0] );
}
void IN_Button1Down( void )
{
    IN_KeyDown( &kb[KB_BUTTONS1] );
}
void IN_Button1Up( void )
{
    IN_KeyUp( &kb[KB_BUTTONS1] );
}
void IN_UseItemDown( void )
{
    IN_KeyDown( &kb[KB_BUTTONS2] );
}
void IN_UseItemUp( void )
{
    IN_KeyUp( &kb[KB_BUTTONS2] );
}
void IN_Button2Down( void )
{
    IN_KeyDown( &kb[KB_BUTTONS2] );
}
void IN_Button2Up( void )
{
    IN_KeyUp( &kb[KB_BUTTONS2] );
}
void IN_Button3Down( void )
{
    IN_KeyDown( &kb[KB_BUTTONS3] );
}
void IN_Button3Up( void )
{
    IN_KeyUp( &kb[KB_BUTTONS3] );
}
void IN_Button4Down( void )
{
    IN_KeyDown( &kb[KB_BUTTONS4] );
}
void IN_Button4Up( void )
{
    IN_KeyUp( &kb[KB_BUTTONS4] );
}

void IN_Button5Down( void )
{
    IN_KeyDown( &kb[KB_BUTTONS5] );
}

void IN_Button5Up( void )
{
    IN_KeyUp( &kb[KB_BUTTONS5] );
}

void IN_Button6Down( void )
{
    IN_KeyDown( &kb[KB_BUTTONS6] );
}

void IN_Button6Up( void )
{
    IN_KeyUp( &kb[KB_BUTTONS6] );
}

//
void IN_Button7Down( void )
{
    IN_KeyDown( &kb[KB_BUTTONS7] );
}

void IN_Button7Up( void )
{
    IN_KeyUp( &kb[KB_BUTTONS7] );
}

//
void IN_Button8Down( void )
{
    IN_KeyDown( &kb[KB_BUTTONS8] );
}

void IN_Button8Up( void )
{
    IN_KeyUp( &kb[KB_BUTTONS8] );
}

//
void IN_Button9Down( void )
{
    IN_KeyDown( &kb[KB_BUTTONS9] );
}

void IN_Button9Up( void )
{
    IN_KeyUp( &kb[KB_BUTTONS9] );
}

//
void IN_Button10Down( void )
{
    IN_KeyDown( &kb[KB_BUTTONS10] );
}

void IN_Button10Up( void )
{
    IN_KeyUp( &kb[KB_BUTTONS10] );
}

//
void IN_Button11Down( void )
{
    IN_KeyDown( &kb[KB_BUTTONS11] );
}

void IN_Button11Up( void )
{
    IN_KeyUp( &kb[KB_BUTTONS11] );
}

//
void IN_Button12Down( void )
{
    IN_KeyDown( &kb[KB_BUTTONS12] );
}

void IN_Button12Up( void )
{
    IN_KeyUp( &kb[KB_BUTTONS12] );
}

//
void IN_Button13Down( void )
{
    IN_KeyDown( &kb[KB_BUTTONS13] );
}

void IN_Button13Up( void )
{
    IN_KeyUp( &kb[KB_BUTTONS13] );
}

//
void IN_Button14Down( void )
{
    IN_KeyDown( &kb[KB_BUTTONS14] );
}

void IN_Button14Up( void )
{
    IN_KeyUp( &kb[KB_BUTTONS14] );
}

//
void IN_Button15Down( void )
{
    IN_KeyDown( &kb[KB_BUTTONS15] );
}

void IN_Button15Up( void )
{
    IN_KeyUp( &kb[KB_BUTTONS15] );
}

// Rafael activate
void IN_ActivateDown( void )
{
    IN_KeyDown( &kb[KB_BUTTONS6] );
}
void IN_ActivateUp( void )
{
    IN_KeyUp( &kb[KB_BUTTONS6] );
}

// done.

void IN_SprintDown( void )
{
    IN_KeyDown( &kb[KB_BUTTONS5] );
}
void IN_SprintUp( void )
{
    IN_KeyUp( &kb[KB_BUTTONS5] );
}


// wbuttons (wolf buttons)
void IN_Wbutton0Down( void )
{
    IN_KeyDown( &kb[KB_WBUTTONS0] );
}								//----(SA) secondary fire button
void IN_Wbutton0Up( void )
{
    IN_KeyUp( &kb[KB_WBUTTONS0] );
}
void IN_ZoomDown( void )
{
    IN_KeyDown( &kb[KB_WBUTTONS1] );
}								//----(SA)  zoom key
void IN_ZoomUp( void )
{
    IN_KeyUp( &kb[KB_WBUTTONS1] );
}
void IN_ReloadDown( void )
{
    IN_KeyDown( &kb[KB_WBUTTONS3] );
}								//----(SA)  manual weapon re-load
void IN_ReloadUp( void )
{
    IN_KeyUp( &kb[KB_WBUTTONS3] );
}
void IN_LeanLeftDown( void )
{
    IN_KeyDown( &kb[KB_WBUTTONS4] );
}								//----(SA)  lean left
void IN_LeanLeftUp( void )
{
    IN_KeyUp( &kb[KB_WBUTTONS4] );
}
void IN_LeanRightDown( void )
{
    IN_KeyDown( &kb[KB_WBUTTONS5] );
}								//----(SA)  lean right
void IN_LeanRightUp( void )
{
    IN_KeyUp( &kb[KB_WBUTTONS5] );
}

// Rafael Kick
// Arnout: now wbutton prone
void IN_ProneDown( void )
{
    IN_KeyDown( &kb[KB_WBUTTONS7] );
}
void IN_ProneUp( void )
{
    IN_KeyUp( &kb[KB_WBUTTONS7] );
}

void IN_ButtonDown( void )
{
    IN_KeyDown( &kb[KB_BUTTONS1] );
}
void IN_ButtonUp( void )
{
    IN_KeyUp( &kb[KB_BUTTONS1] );
}


/*void IN_CenterView (void) {
	cl.viewangles[PITCH] = -SHORT2ANGLE(cl.snap.ps.delta_angles[PITCH]);
}*/


void IN_Notebook( void )
{
    //if ( cls.state == CA_ACTIVE && !clc.demoplaying ) {
    //uiManager->SetActiveMenu( UIMENU_NOTEBOOK );
    //}
}

void IN_Help( void )
{
    if( cls.state == CA_ACTIVE && !clc.demoplaying )
    {
        uiManager->SetActiveMenu( UIMENU_HELP );
    }
}

convar_t* cl_upspeed;
convar_t* cl_forwardspeed;
convar_t* cl_sidespeed;
convar_t* cl_yawspeed;
convar_t* cl_pitchspeed;
convar_t* cl_run;
convar_t* cl_anglespeedkey;
convar_t* cl_recoilPitch;
convar_t* cl_bypassMouseInput;	// NERVE - SMF
convar_t* cl_doubletapdelay;

/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
void CL_AdjustAngles( void )
{
    F32           speed;
    
    if( kb[KB_SPEED].active )
    {
        speed = ( ( F32 )cls.frametime / 1000.0f ) * cls.frametime * cl_anglespeedkey->value;
    }
    else
    {
        speed = ( F32 )cls.frametime / 1000.0f;
    }
    
    if( !kb[KB_STRAFE].active )
    {
        cl.viewangles[YAW] -= speed * cl_yawspeed->value * CL_KeyState( &kb[KB_RIGHT] );
        cl.viewangles[YAW] += speed * cl_yawspeed->value * CL_KeyState( &kb[KB_LEFT] );
    }
    
    cl.viewangles[PITCH] -= speed * cl_pitchspeed->value * CL_KeyState( &kb[KB_LOOKUP] );
    cl.viewangles[PITCH] += speed * cl_pitchspeed->value * CL_KeyState( &kb[KB_LOOKDOWN] );
}

/*
================
CL_KeyMove

Sets the usercmd_t based on key states
================
*/
void CL_KeyMove( usercmd_t* cmd )
{
    S32             movespeed;
    S32             forward, side, up;
    
    //
    // adjust for speed key / running
    // the walking flag is to keep animations consistant
    // even during acceleration and develeration
    //
    if( kb[KB_SPEED].active ^ cl_run->integer )
    {
        movespeed = 127;
        cmd->buttons &= ~BUTTON_WALKING;
    }
    else
    {
        cmd->buttons |= BUTTON_WALKING;
        movespeed = 64;
    }
    
    forward = 0;
    side = 0;
    up = 0;
    if( kb[KB_STRAFE].active )
    {
        side += movespeed * CL_KeyState( &kb[KB_RIGHT] );
        side -= movespeed * CL_KeyState( &kb[KB_LEFT] );
    }
    
    side += movespeed * CL_KeyState( &kb[KB_MOVERIGHT] );
    side -= movespeed * CL_KeyState( &kb[KB_MOVELEFT] );
    
//----(SA)  added
#if 0
    if( cmd->buttons & BUTTON_ACTIVATE )
    {
        if( side > 0 )
        {
            cmd->wbuttons |= WBUTTON_LEANRIGHT;
        }
        else if( side < 0 )
        {
            cmd->wbuttons |= WBUTTON_LEANLEFT;
        }
        
        side = 0;				// disallow the strafe when holding 'activate'
    }
#endif
//----(SA)  end

    up += movespeed * CL_KeyState( &kb[KB_UP] );
    up -= movespeed * CL_KeyState( &kb[KB_DOWN] );
    
    forward += movespeed * CL_KeyState( &kb[KB_FORWARD] );
    forward -= movespeed * CL_KeyState( &kb[KB_BACK] );
    
    // fretn - moved this to bg_pmove.c
    //if (!(cl.snap.ps.persistant[PERS_HWEAPON_USE]))
    //{
    cmd->forwardmove = ClampChar( forward );
    cmd->rightmove = ClampChar( side );
    cmd->upmove = ClampChar( up );
    //}
    
    // Arnout: double tap
    cmd->doubleTap = DT_NONE;	// reset
    if( !cl.doubleTap.lastdoubleTap || com_frameTime - cl.doubleTap.lastdoubleTap > cl_doubletapdelay->integer + cls.frametime )
    {
        // frametime for low(-ish) fps situations)
        S32             i;
        bool        key_down;
        
        for( i = 1; i < DT_NUM; i++ )
        {
            key_down = ( bool )( kb[dtmapping[i]].active || kb[dtmapping[i]].wasPressed );
            
            if( key_down && !cl.doubleTap.pressedTime[i] )
            {
                cl.doubleTap.pressedTime[i] = com_frameTime;
            }
            else if( !key_down && !cl.doubleTap.releasedTime[i]
                     && ( com_frameTime - cl.doubleTap.pressedTime[i] ) < ( cl_doubletapdelay->integer + cls.frametime ) )
            {
                cl.doubleTap.releasedTime[i] = com_frameTime;
            }
            else if( key_down && ( com_frameTime - cl.doubleTap.pressedTime[i] ) < ( cl_doubletapdelay->integer + cls.frametime )
                     && ( com_frameTime - cl.doubleTap.releasedTime[i] ) < ( cl_doubletapdelay->integer + cls.frametime ) )
            {
                cl.doubleTap.pressedTime[i] = cl.doubleTap.releasedTime[i] = 0;
                cmd->doubleTap = i;
                cl.doubleTap.lastdoubleTap = com_frameTime;
            }
            else if( !key_down && ( cl.doubleTap.pressedTime[i] || cl.doubleTap.releasedTime[i] ) )
            {
                if( com_frameTime - cl.doubleTap.pressedTime[i] >= ( cl_doubletapdelay->integer + cls.frametime ) )
                {
                    cl.doubleTap.pressedTime[i] = cl.doubleTap.releasedTime[i] = 0;
                }
            }
        }
    }
}

/*
=================
CL_MouseEvent
=================
*/
void CL_MouseEvent( S32 dx, S32 dy, S32 time )
{
    if( cls.keyCatchers & KEYCATCH_UI )
    {
        // NERVE - SMF - if we just want to pass it along to game
        if( cl_bypassMouseInput->integer == 1 )
        {
            cl.mouseDx[cl.mouseIndex] += dx;
            cl.mouseDy[cl.mouseIndex] += dy;
        }
        else
        {
            uiManager->MouseEvent( dx, dy );
        }
        
    }
    else if( cls.keyCatchers & KEYCATCH_CGAME )
    {
        if( cl_bypassMouseInput->integer == 1 )
        {
            cl.mouseDx[cl.mouseIndex] += dx;
            cl.mouseDy[cl.mouseIndex] += dy;
        }
        else
        {
            cgame->MouseEvent( dx, dy );
        }
    }
    else
    {
        cl.mouseDx[cl.mouseIndex] += dx;
        cl.mouseDy[cl.mouseIndex] += dy;
    }
}

/*
=================
CL_JoystickEvent

Joystick values stay set until changed
=================
*/
void CL_JoystickEvent( S32 axis, S32 value, S32 time )
{
    if( axis < 0 || axis >= MAX_JOYSTICK_AXIS )
    {
        Com_Error( ERR_DROP, "CL_JoystickEvent: bad axis %i", axis );
    }
    
    if( axis >= 0 )
    {
        cl.joystickAxis[axis] = value;
    }
}

/*
=================
CL_JoystickMove
=================
*/
void CL_JoystickMove( usercmd_t* cmd )
{
    S32             movespeed;
    F32           anglespeed;
    
    if( kb[KB_SPEED].active ^ cl_run->integer )
    {
        movespeed = 2;
    }
    else
    {
        movespeed = 1;
        cmd->buttons |= BUTTON_WALKING;
    }
    
    if( kb[KB_SPEED].active )
    {
        anglespeed = ( ( F32 )cls.frametime / 1000.0f ) * cls.frametime * cl_anglespeedkey->value;
    }
    else
    {
        anglespeed = ( F32 )cls.frametime / 1000.0f;
    }
    
#ifdef __MACOS__
    cmd->rightmove = ClampChar( cmd->rightmove + cl.joystickAxis[AXIS_SIDE] );
#else
    if( !kb[KB_STRAFE].active )
    {
        cl.viewangles[YAW] += anglespeed * j_yaw->value * cl.joystickAxis[j_yaw_axis->integer];
        cmd->rightmove = ClampChar( cmd->rightmove + ( S32 )( j_side->value * cl.joystickAxis[j_side_axis->integer] ) );
    }
    else
    {
        cl.viewangles[YAW] += anglespeed * j_side->value * cl.joystickAxis[j_side_axis->integer];
        cmd->rightmove = ClampChar( cmd->rightmove + ( S32 )( j_yaw->value * cl.joystickAxis[j_yaw_axis->integer] ) );
    }
#endif
    if( kb[KB_MLOOK].active )
    {
        cl.viewangles[PITCH] += anglespeed * j_forward->value * cl.joystickAxis[j_forward_axis->integer];
        cmd->forwardmove = ClampChar( cmd->forwardmove + ( S32 )( j_pitch->value * cl.joystickAxis[j_pitch_axis->integer] ) );
    }
    else
    {
        cl.viewangles[PITCH] += anglespeed * j_pitch->value * cl.joystickAxis[j_pitch_axis->integer];
        cmd->forwardmove = ClampChar( cmd->forwardmove + ( S32 )( j_forward->value * cl.joystickAxis[j_forward_axis->integer] ) );
    }
    
    cmd->upmove = ClampChar( cmd->upmove + ( S32 )( j_up->value * cl.joystickAxis[j_up_axis->integer] ) );
}

/*
=================
CL_Xbox360ControllerMove
=================
*/

void CL_Xbox360ControllerMove( usercmd_t* cmd )
{
    S32     movespeed;
    F32   anglespeed;
    
    if( kb[KB_SPEED].active ^ cl_run->integer )
    {
        movespeed = 2;
    }
    else
    {
        movespeed = 1;
        cmd->buttons |= BUTTON_WALKING;
    }
    
    if( kb[KB_SPEED].active )
    {
        anglespeed = 0.001 * cls.frametime * cl_anglespeedkey->value;
    }
    else
    {
        anglespeed = 0.001 * cls.frametime;
    }
    
    cl.viewangles[PITCH] += anglespeed * cl_pitchspeed->value * ( cl.joystickAxis[j_pitch_axis->integer] / 127.0f );
    cl.viewangles[YAW] += anglespeed * cl_yawspeed->value * ( cl.joystickAxis[j_yaw_axis->integer] / 127.0f );
    
    cmd->rightmove = ClampChar( cmd->rightmove + cl.joystickAxis[j_side_axis->integer] );
    cmd->forwardmove = ClampChar( cmd->forwardmove + cl.joystickAxis[j_forward_axis->integer] );
    cmd->upmove = ClampChar( cmd->upmove + cl.joystickAxis[j_up_axis->integer] );
}



/*
=================
CL_MouseMove
=================
*/
void CL_MouseMove( usercmd_t* cmd )
{
    F32 mx, my;
    
    // allow mouse smoothing
    if( m_filter->integer )
    {
        mx = ( cl.mouseDx[0] + cl.mouseDx[1] ) * 0.5f;
        my = ( cl.mouseDy[0] + cl.mouseDy[1] ) * 0.5f;
    }
    else
    {
        mx = cl.mouseDx[cl.mouseIndex];
        my = cl.mouseDy[cl.mouseIndex];
    }
    cl.mouseIndex ^= 1;
    cl.mouseDx[cl.mouseIndex] = 0;
    cl.mouseDy[cl.mouseIndex] = 0;
    
    if( mx == 0.0f && my == 0.0f )
        return;
        
    if( cl_mouseAccel->value != 0.0f )
    {
        if( cl_mouseAccel->value != 0.0f && cl_mouseAccelOffset->value > 0.0f )
        {
            F32 accelSensitivity;
            F32 rate;
            
            rate = sqrt( mx * mx + my * my ) / ( F32 ) frame_msec;
            
            accelSensitivity = cl_sensitivity->value + rate * cl_mouseAccel->value;
            mx *= accelSensitivity;
            my *= accelSensitivity;
            
            if( cl_showMouseRate->integer )
                Com_Printf( "rate: %f, accelSensitivity: %f\n", rate, accelSensitivity );
        }
        else
        {
            F32 rate[2];
            F32 power[2];
            
            // sensitivity remains pretty much unchanged at low speeds
            // cl_mouseAccel is a power value to how the acceleration is shaped
            // cl_mouseAccelOffset is the rate for which the acceleration will have doubled the non accelerated amplification
            // NOTE: decouple the config cvars for independent acceleration setup along X and Y?
            
            rate[0] = fabs( mx ) / ( F32 ) frame_msec;
            rate[1] = fabs( my ) / ( F32 ) frame_msec;
            power[0] = powf( rate[0] / cl_mouseAccelOffset->value, cl_mouseAccel->value );
            power[1] = powf( rate[1] / cl_mouseAccelOffset->value, cl_mouseAccel->value );
            
            mx = cl_sensitivity->value * ( mx + ( ( mx < 0 ) ? -power[0] : power[0] ) * cl_mouseAccelOffset->value );
            my = cl_sensitivity->value * ( my + ( ( my < 0 ) ? -power[1] : power[1] ) * cl_mouseAccelOffset->value );
            
            /*	NERVE - SMF - this has moved to CG_CalcFov to fix zoomed-in/out transition movement bug
            	if ( cl.snap.ps.stats[STAT_ZOOMED_VIEW] ) {
            		if(cl.snap.ps.weapon == WP_SNIPERRIFLE) {
            			accelSensitivity *= 0.1;
            		}
            		else if(cl.snap.ps.weapon == WP_SNOOPERSCOPE) {
            			accelSensitivity *= 0.2;
            		}
            	}
            */
            if( cl_showMouseRate->integer )
                Com_Printf( "ratex: %f, ratey: %f, powx: %f, powy: %f\n", rate[0], rate[1], power[0], power[1] );
        }
    }
    
// Ridah, experimenting with a slow tracking gun
#if 0
    // Rafael - mg42
    if( cl.snap.ps.persistant[PERS_HWEAPON_USE] )
    {
        mx *= 2.5;				//(accelSensitivity * 0.1);
        my *= 2;				//(accelSensitivity * 0.075);
    }
    else
    {
        mx *= cl_sensitivity->value;
        my *= cl_sensitivity->value;
    }
#endif
    
    // ingame FOV
    mx *= cl.cgameSensitivity;
    my *= cl.cgameSensitivity;
    
    // add mouse X/Y movement to cmd
    if( kb[KB_STRAFE].active )
        cmd->rightmove = ClampChar( cmd->rightmove + m_side->value * mx );
    else
        cl.viewangles[YAW] -= m_yaw->value * mx;
        
    if( ( kb[KB_MLOOK].active || cl_freelook->integer ) && !kb[KB_STRAFE].active )
        cl.viewangles[PITCH] += m_pitch->value * my;
    else
        cmd->forwardmove = ClampChar( cmd->forwardmove - m_forward->value * my );
}


/*
==============
CL_CmdButtons
==============
*/
void CL_CmdButtons( usercmd_t* cmd )
{
    S32             i;
    
    //
    // figure button bits
    // send a button bit even if the key was pressed and released in
    // less than a frame
    //
    for( i = 0; i < 15; i++ )
    {
        if( kb[KB_BUTTONS0 + i].active || kb[KB_BUTTONS0 + i].wasPressed )
        {
            cmd->buttons |= 1 << i;
        }
        kb[KB_BUTTONS0 + i].wasPressed = false;
    }
    
    for( i = 0; i < 16; i++ )
    {
        // Arnout: this was i < 7, but there are 8 wbuttons
        if( kb[KB_WBUTTONS0 + i].active || kb[KB_WBUTTONS0 + i].wasPressed )
        {
            cmd->wbuttons |= 1 << i;
        }
        kb[KB_WBUTTONS0 + i].wasPressed = false;
    }
    
    if( cls.keyCatchers && !cl_bypassMouseInput->integer )
    {
        cmd->buttons |= BUTTON_TALK;
    }
    
    // allow the game to know if any key at all is
    // currently pressed, even if it isn't bound to anything
    if( anykeydown && ( !cls.keyCatchers || cl_bypassMouseInput->integer ) )
    {
        cmd->buttons |= BUTTON_ANY;
    }
    
    // Arnout: clear 'waspressed' from double tap buttons
    for( i = 1; i < DT_NUM; i++ )
    {
        kb[dtmapping[i]].wasPressed = false;
    }
}


/*
==============
CL_FinishMove
==============
*/
void CL_FinishMove( usercmd_t* cmd )
{
    S32             i;
    
    // copy the state that the cgame is currently sending
    cmd->weapon = cl.cgameUserCmdValue;
    
    cmd->flags = cl.cgameFlags;
    
    cmd->identClient = cl.cgameMpIdentClient;	// NERVE - SMF
    
    // send the current server time so the amount of movement
    // can be determined without allowing cheating
    cmd->serverTime = cl.serverTime;
    
    for( i = 0; i < 3; i++ )
    {
        cmd->angles[i] = ANGLE2SHORT( cl.viewangles[i] );
    }
}

/*
=================
CL_CreateCmd
=================
*/
usercmd_t CL_CreateCmd( void )
{
    usercmd_t       cmd;
    vec3_t          oldAngles;
    F32           recoilAdd;
    
    VectorCopy( cl.viewangles, oldAngles );
    
    // keyboard angle adjustment
    CL_AdjustAngles();
    
    memset( &cmd, 0, sizeof( cmd ) );
    
    CL_CmdButtons( &cmd );
    
    // get basic movement from keyboard
    CL_KeyMove( &cmd );
    
    // get basic movement from mouse
    CL_MouseMove( &cmd );
    
    // get basic movement from mouse
    CL_MouseMove( &cmd );
    
    // get basic movement from joystick or controller
    if( cl_xbox360ControllerAvailable->integer )
    {
        CL_Xbox360ControllerMove( &cmd );
    }
    else
    {
        CL_JoystickMove( &cmd );
    }
    
    // check to make sure the angles haven't wrapped
    if( cl.viewangles[PITCH] - oldAngles[PITCH] > 90 )
    {
        cl.viewangles[PITCH] = oldAngles[PITCH] + 90;
    }
    else if( oldAngles[PITCH] - cl.viewangles[PITCH] > 90 )
    {
        cl.viewangles[PITCH] = oldAngles[PITCH] - 90;
    }
    
    // RF, set the kickAngles so aiming is effected
    recoilAdd = cl_recoilPitch->value;
    if( Q_fabs( cl.viewangles[PITCH] + recoilAdd ) < 40 )
    {
        cl.viewangles[PITCH] += recoilAdd;
    }
    // the recoilPitch has been used, so clear it out
    cl_recoilPitch->value = 0;
    
    // store out the final values
    CL_FinishMove( &cmd );
    
    // draw debug graphs of turning for mouse testing
    if( cl_debugMove->integer )
    {
        if( cl_debugMove->integer == 1 )
        {
            idClientScreenSystemLocal::DebugGraph( fabs( cl.viewangles[YAW] - oldAngles[YAW] ), 0 );
        }
        if( cl_debugMove->integer == 2 )
        {
            idClientScreenSystemLocal::DebugGraph( fabs( cl.viewangles[PITCH] - oldAngles[PITCH] ), 0 );
        }
    }
    
    return cmd;
}


/*
=================
CL_CreateNewCommands

Create a new usercmd_t structure for this frame
=================
*/
void CL_CreateNewCommands( void )
{
    usercmd_t*      cmd;
    S32             cmdNum;
    
    // no need to create usercmds until we have a gamestate
    if( cls.state < CA_PRIMED )
    {
        return;
    }
    
    frame_msec = com_frameTime - old_com_frameTime;
    
    // if running over 1000fps, act as if each frame is 1ms
    // prevents division by zero
    if( frame_msec < 1 )
    {
        frame_msec = 1;
    }
    
    // if running less than 5fps, truncate the extra time to prevent
    // unexpected moves after a hitch
    if( frame_msec > 200 )
    {
        frame_msec = 200;
    }
    old_com_frameTime = com_frameTime;
    
    
    // generate a command for this frame
    cl.cmdNumber++;
    cmdNum = cl.cmdNumber & CMD_MASK;
    cl.cmds[cmdNum] = CL_CreateCmd();
    cmd = &cl.cmds[cmdNum];
}

/*
=================
CL_ReadyToSendPacket

Returns false if we are over the maxpackets limit
and should choke back the bandwidth a bit by not sending
a packet this frame.  All the commands will still get
delivered in the next packet, but saving a header and
getting more delta compression will reduce total bandwidth.
=================
*/
bool CL_ReadyToSendPacket( void )
{
    S32             oldPacketNum;
    S32             delta;
    
    // don't send anything if playing back a demo
    if( clc.demoplaying || cls.state == CA_CINEMATIC )
    {
        return false;
    }
    
    // If we are downloading, we send no less than 50ms between packets
    if( *cls.downloadTempName && cls.realtime - clc.lastPacketSentTime < 50 )
    {
        return false;
    }
    
    // if we don't have a valid gamestate yet, only send
    // one packet a second
    if( cls.state != CA_ACTIVE && cls.state != CA_PRIMED && !*cls.downloadTempName && cls.realtime - clc.lastPacketSentTime < 1000 )
    {
        return false;
    }
    
    // send every frame for loopbacks
    if( clc.netchan.remoteAddress.type == NA_LOOPBACK )
    {
        return true;
    }
    
    // send every frame for LAN
    if( Net_IsLANAddress( clc.netchan.remoteAddress ) )
    {
        return true;
    }
    
    // check for exceeding cl_maxpackets
    if( cl_maxpackets->integer < 1 )
    {
        cvarSystem->Set( "cl_maxpackets", "1" );
    }
    else if( cl_maxpackets->integer > 2000 )
    {
        cvarSystem->Set( "cl_maxpackets", "2000" );
    }
    oldPacketNum = ( clc.netchan.outgoingSequence - 1 ) & PACKET_MASK;
    delta = cls.realtime - cl.outPackets[oldPacketNum].p_realtime;
    if( delta < 1000 / cl_maxpackets->integer )
    {
        // the accumulated commands will go out in the next packet
        return false;
    }
    
    return true;
}

/*
===================
CL_WritePacket

Create and send the command packet to the server
Including both the reliable commands and the usercmds

During normal gameplay, a client packet will contain something like:

4	sequence number
2	qport
4	serverid
4	acknowledged sequence number
4	clc.serverCommandSequence
<optional reliable commands>
1	clc_move or clc_moveNoDelta
1	command count
<count * usercmds>

===================
*/
void CL_WritePacket( void )
{
    msg_t           buf;
    U8            data[MAX_MSGLEN];
    S32             i, j;
    usercmd_t*      cmd, *oldcmd;
    usercmd_t       nullcmd;
    S32             packetNum;
    S32             oldPacketNum;
    S32             count, key;
    
    // don't send anything if playing back a demo
    if( clc.demoplaying || cls.state == CA_CINEMATIC )
    {
        return;
    }
    
    memset( &nullcmd, 0, sizeof( nullcmd ) );
    oldcmd = &nullcmd;
    
    MSG_Init( &buf, data, sizeof( data ) );
    
    MSG_Bitstream( &buf );
    // write the current serverId so the server
    // can tell if this is from the current gameState
    MSG_WriteLong( &buf, cl.serverId );
    
    // write the last message we received, which can
    // be used for delta compression, and is also used
    // to tell if we dropped a gamestate
    MSG_WriteLong( &buf, clc.serverMessageSequence );
    
    // write the last reliable message we received
    MSG_WriteLong( &buf, clc.serverCommandSequence );
    
    // write any unacknowledged clientCommands
    // NOTE TTimo: if you verbose this, you will see that there are quite a few duplicates
    // typically several unacknowledged cp or userinfo commands stacked up
    for( i = clc.reliableAcknowledge + 1; i <= clc.reliableSequence; i++ )
    {
        MSG_WriteByte( &buf, clc_clientCommand );
        MSG_WriteLong( &buf, i );
        MSG_WriteString( &buf, clc.reliableCommands[i & ( MAX_RELIABLE_COMMANDS - 1 )] );
    }
    
    // we want to send all the usercmds that were generated in the last
    // few packet, so even if a couple packets are dropped in a row,
    // all the cmds will make it to the server
    if( cl_packetdup->integer < 0 )
    {
        cvarSystem->Set( "cl_packetdup", "0" );
    }
    else if( cl_packetdup->integer > 5 )
    {
        cvarSystem->Set( "cl_packetdup", "5" );
    }
    oldPacketNum = ( clc.netchan.outgoingSequence - 1 - cl_packetdup->integer ) & PACKET_MASK;
    count = cl.cmdNumber - cl.outPackets[oldPacketNum].p_cmdNumber;
    if( count > MAX_PACKET_USERCMDS )
    {
        count = MAX_PACKET_USERCMDS;
        Com_Printf( "MAX_PACKET_USERCMDS\n" );
    }
    
    if( count >= 1 )
    {
        if( cl_showSend->integer )
        {
            Com_Printf( "(%i)", count );
        }
        
        // begin a client move command
        if( cl_nodelta->integer || !cl.snap.valid || clc.demowaiting || clc.serverMessageSequence != cl.snap.messageNum )
        {
            MSG_WriteByte( &buf, clc_moveNoDelta );
        }
        else
        {
            MSG_WriteByte( &buf, clc_move );
        }
        
        // write the command count
        MSG_WriteByte( &buf, count );
        
        // use the checksum feed in the key
        key = clc.checksumFeed;
        // also use the message acknowledge
        key ^= clc.serverMessageSequence;
        // also use the last acknowledged server command in the key
        key ^= Com_HashKey( clc.serverCommands[clc.serverCommandSequence & ( MAX_RELIABLE_COMMANDS - 1 )], 32 );
        
        // write all the commands, including the predicted command
        for( i = 0; i < count; i++ )
        {
            j = ( cl.cmdNumber - count + i + 1 ) & CMD_MASK;
            cmd = &cl.cmds[j];
            MSG_WriteDeltaUsercmdKey( &buf, key, oldcmd, cmd );
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
    
    if( cl_showSend->integer )
    {
        Com_Printf( "%i ", buf.cursize );
    }
    CL_Netchan_Transmit( &clc.netchan, &buf );
    
    // clients never really should have messages large enough
    // to fragment, but in case they do, fire them all off
    // at once
    // TTimo: this causes a packet burst, which is bad karma for winsock
    // added a WARNING message, we'll see if there are legit situations where this happens
    while( clc.netchan.unsentFragments )
    {
        if( cl_showSend->integer )
        {
            Com_Printf( "WARNING: unsent fragments (not supposed to happen!)\n" );
        }
        CL_Netchan_TransmitNextFragment( &clc.netchan );
    }
}

/*
=================
CL_SendCmd

Called every frame to builds and sends a command packet to the server.
=================
*/
void CL_SendCmd( void )
{
    // don't send any message if not connected
    if( cls.state < CA_CONNECTED )
    {
        return;
    }
    
    // don't send commands if paused
    if( com_sv_running->integer && sv_paused->integer && cl_paused->integer )
    {
        return;
    }
    
    // we create commands even if a demo is playing,
    CL_CreateNewCommands();
    
    // don't send a packet if the last packet was sent too recently
    if( !CL_ReadyToSendPacket() )
    {
        if( cl_showSend->integer )
        {
            Com_Printf( ". " );
        }
        return;
    }
    
    CL_WritePacket();
}

/*
============
CL_InitInput
============
*/
void CL_InitInput( void )
{
    //cmdSystem->AddCommand ("centerview", IN_CenterView, "Centers view on screen");
    
    cmdSystem->AddCommand( "+moveup", IN_UpDown, "Move up, i.e. Jump" );
    cmdSystem->AddCommand( "-moveup", IN_UpUp, "Stop issuing command to jump" );
    cmdSystem->AddCommand( "+movedown", IN_DownDown, "Move downwards, crouch" );
    cmdSystem->AddCommand( "-movedown", IN_DownUp, "Stop issing command to crouch" );
    cmdSystem->AddCommand( "+left", IN_LeftDown, "Look left" );
    cmdSystem->AddCommand( "-left", IN_LeftUp, "Stop issuing command to look further to the left" );
    cmdSystem->AddCommand( "+right", IN_RightDown, "Rotate camera right" );
    cmdSystem->AddCommand( "-right", IN_RightUp, "Stops issuing look right command" );
    cmdSystem->AddCommand( "+forward", IN_ForwardDown, "Move forward" );
    cmdSystem->AddCommand( "-forward", IN_ForwardUp, "Stop issuing command to move forwards" );
    cmdSystem->AddCommand( "+back", IN_BackDown, "Move backward" );
    cmdSystem->AddCommand( "-back", IN_BackUp, "Stop issuing command to move backwards" );
    cmdSystem->AddCommand( "+lookup", IN_LookupDown, "Tilt camera up" );
    cmdSystem->AddCommand( "-lookup", IN_LookupUp, "Stop issuing command to look further upwards" );
    cmdSystem->AddCommand( "+lookdown", IN_LookdownDown, "Tilt camera down" );
    cmdSystem->AddCommand( "-lookdown", IN_LookdownUp, "Stop issuing command to look further downwards" );
    cmdSystem->AddCommand( "+strafe", IN_StrafeDown, "Hold to strafe" );
    cmdSystem->AddCommand( "-strafe", IN_StrafeUp, "Stops issuing strafe command" );
    cmdSystem->AddCommand( "+moveleft", IN_MoveleftDown, "Strafe/sidestep to the left" );
    cmdSystem->AddCommand( "-moveleft", IN_MoveleftUp, "Stop issuing command to strafe left" );
    cmdSystem->AddCommand( "+moveright", IN_MoverightDown, "Strafe/sidestep to the right" );
    cmdSystem->AddCommand( "-moveright", IN_MoverightUp, "Stop issuing command to strafe right" );
    cmdSystem->AddCommand( "+speed", IN_SpeedDown, "Walk or run" );
    cmdSystem->AddCommand( "-speed", IN_SpeedUp, "Stops issuing walk/run command" );
    
    cmdSystem->AddCommand( "+attack", IN_Button0Down, "Fires weapon, or uses the weaponbank object currently selected" );	// ---- id   (primary firing)
    cmdSystem->AddCommand( "-attack", IN_Button0Up, "Stops issuing command to attack" );
    
    cmdSystem->AddCommand( "+button0", IN_Button0Down, "Button0" );
    cmdSystem->AddCommand( "-button0", IN_Button0Up, "Stop issuing command button0" );
    
    cmdSystem->AddCommand( "+button1", IN_Button1Down, "Button1" );
    cmdSystem->AddCommand( "-button1", IN_Button1Up, "Stop issuing command button1" );
    
    cmdSystem->AddCommand( "+button2", IN_Button2Down, "Button2" );
    cmdSystem->AddCommand( "-button2", IN_Button2Up, "Stop issuing command button2" );
    
    cmdSystem->AddCommand( "+useitem", IN_UseItemDown, "Use selected item" );
    cmdSystem->AddCommand( "-useitem", IN_UseItemUp, "Stop issuing command for selected item" );
    
    cmdSystem->AddCommand( "+salute", IN_Button3Down, "Salute" );
    cmdSystem->AddCommand( "-salute", IN_Button3Up, "Stop issuing salute command" );
    
    cmdSystem->AddCommand( "+button3", IN_Button3Down, "Button3" );
    cmdSystem->AddCommand( "-button3", IN_Button3Up, "Stop issuing command button3" );
    
    cmdSystem->AddCommand( "+button4", IN_Button4Down, "Button4" );
    cmdSystem->AddCommand( "-button4", IN_Button4Up, "Stop issuing command button4" );
    
    cmdSystem->AddCommand( "+button5", IN_Button5Down, "Button5" );
    cmdSystem->AddCommand( "-button5", IN_Button5Up, "Stop issuing command button5" );
    
    cmdSystem->AddCommand( "+button6", IN_Button6Down, "Button6" );
    cmdSystem->AddCommand( "-button6", IN_Button6Up, "Stop issuing command button6" );
    
    cmdSystem->AddCommand( "+button7", IN_Button7Down, "Button7" );
    cmdSystem->AddCommand( "-button7", IN_Button7Up, "Stop issuing command button7" );
    
    cmdSystem->AddCommand( "+button8", IN_Button8Down, "Button8" );
    cmdSystem->AddCommand( "-button8", IN_Button8Up, "Stop issuing command button9" );
    
    cmdSystem->AddCommand( "+button9", IN_Button9Down, "Button9" );
    cmdSystem->AddCommand( "-button9", IN_Button9Up, "Stop issuing command button9" );
    
    cmdSystem->AddCommand( "+button10", IN_Button10Down, "Button10" );
    cmdSystem->AddCommand( "-button10", IN_Button10Up, "Stop issuing command button10" );
    
    cmdSystem->AddCommand( "+button11", IN_Button11Down, "Button11" );
    cmdSystem->AddCommand( "-button11", IN_Button11Up, "Stop issuing command button11" );
    
    cmdSystem->AddCommand( "+button12", IN_Button12Down, "Button12" );
    cmdSystem->AddCommand( "-button12", IN_Button12Up, "Stop issuing command button12" );
    
    cmdSystem->AddCommand( "+button13", IN_Button13Down, "Button13" );
    cmdSystem->AddCommand( "-button13", IN_Button13Up, "Stop issuing command button13" );
    
    cmdSystem->AddCommand( "+button14", IN_Button14Down, "Button14" );
    cmdSystem->AddCommand( "-button14", IN_Button14Up, "Stop issuing command button14" );
    
    // Rafael Activate
    cmdSystem->AddCommand( "+activate", IN_ActivateDown, "Performs various actions like opening doors, picking up weapons." );
    cmdSystem->AddCommand( "-activate", IN_ActivateUp, "Stops issuing +activate command, stop opening doors etc" );
    // done.
    
    // Rafael Kick
    // Arnout: now prone
    cmdSystem->AddCommand( "+prone", IN_ProneDown, "Go prone, lie down." );
    cmdSystem->AddCommand( "-prone", IN_ProneUp, "Stop issuing command to go prone" );
    // done
    
    cmdSystem->AddCommand( "+dodge", IN_ProneDown, "dodge" );
    cmdSystem->AddCommand( "-dodge", IN_ProneUp, "dodge" );
    
    cmdSystem->AddCommand( "+sprint", IN_SprintDown, "Sprint, run fast draining stanima bar" );
    cmdSystem->AddCommand( "-sprint", IN_SprintUp, "Stops issuing sprint command" );
    
    
    // wolf buttons
    cmdSystem->AddCommand( "+attack2", IN_Wbutton0Down, "Secondary firing mode" );	//----(SA) secondary firing
    cmdSystem->AddCommand( "-attack2", IN_Wbutton0Up, "Stop issuing command to perform secondary attack" );
    cmdSystem->AddCommand( "+zoom", IN_ZoomDown, "Zoom command" );	//
    cmdSystem->AddCommand( "-zoom", IN_ZoomUp, "Stops issuing zoom command" );
    cmdSystem->AddCommand( "+reload", IN_ReloadDown, "Reload weapon" );	//
    cmdSystem->AddCommand( "-reload", IN_ReloadUp, "Stops issuing reload command" );
    cmdSystem->AddCommand( "+leanleft", IN_LeanLeftDown, "Leans to the left" );
    cmdSystem->AddCommand( "-leanleft", IN_LeanLeftUp, "Stop issuing command to lean to the left" );
    cmdSystem->AddCommand( "+leanright", IN_LeanRightDown, "Leans to the right" );
    cmdSystem->AddCommand( "-leanright", IN_LeanRightUp, "Stop issuing command to lean to the right" );
    
    
    cmdSystem->AddCommand( "+mlook", IN_MLookDown, "Toggles mouselook" );
    cmdSystem->AddCommand( "-mlook", IN_MLookUp, "Stop +mlook (mouselook), go back to mouse-movement" );
    
    //cmdSystem->AddCommand ("notebook",IN_Notebook);
    cmdSystem->AddCommand( "help", IN_Help, "Toggles help" );
    
    cl_nodelta = cvarSystem->Get( "cl_nodelta", "0", 0, "Wether to disable delta compression for networking stuff." );
    cl_debugMove = cvarSystem->Get( "cl_debugMove", "0", 0, "Draws a chart at the bottom displaying something to do with how much you look around." );
}


/*
============
CL_ClearKeys
============
*/
void CL_ClearKeys( void )
{
    memset( kb, 0, sizeof( kb ) );
}
