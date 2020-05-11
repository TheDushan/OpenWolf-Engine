////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2018 - 2019 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   serverCcmds.h
// Version:     v1.02
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SERVERWALLHACK_H__
#define __SERVERWALLHACK_H__

static vec3_t pred_ppos, pred_opos;
static trajectory_t traject;
static vec3_t old_origin[MAX_CLIENTS];
static S32 origin_changed[MAX_CLIENTS];
static F32 delta_sign[8][3] =
{
    { 1,  1,  1  },
    { 1,  1,  1  },
    { 1,  1,  -1 },
    { 1,  1,  -1 },
    { -1, 1,  1  },
    { 1,  -1, 1  },
    { -1, 1,  -1 },
    { 1,  -1, -1 }
};

static vec3_t delta[8];

static S32 bbox_horz;
static S32 bbox_vert;

//======================================================================
// local functions
//======================================================================

#define POS_LIM     1.0f
#define NEG_LIM    -1.0f

//======================================================================

#define MAX_CLIP_PLANES   5
#define Z_ADJUST          1

#define NUMBUMPS          4

//======================================================================

#define STEPSIZE 18

//======================================================================

#define PREDICT_TIME      0.1f
#define VOFS              6


//
// idServerWallhackSystemLocal
//
class idServerWallhackSystemLocal
{
public:
    idServerWallhackSystemLocal();
    ~idServerWallhackSystemLocal();
    
    static S32 zero_vector( vec3_t v );
    static void predict_clip_velocity( vec3_t in, vec3_t normal, vec3_t out );
    static S32 predict_slide_move( sharedEntity_t* ent, F32 frametime, trajectory_t* tr, vec3_t result );
    static void predict_move( sharedEntity_t* ent, F32 frametime, trajectory_t* tr, vec3_t result );
    static void calc_viewpoint( playerState_t* ps, vec3_t org, vec3_t vp );
    static S32 player_in_fov( vec3_t viewangle, vec3_t ppos, vec3_t opos );
    static void copy_trajectory( trajectory_t* src, trajectory_t* dst );
    static S32 is_visible( vec3_t start, vec3_t end );
    static void init_horz_delta( void );
    static void init_vert_delta( void );
    static void InitWallhack( void );
    static S32 CanSee( S32 player, S32 other );
    static void RandomizePos( S32 player, S32 other );
    static void RestorePos( S32 cli );
    static S32 PositionChanged( S32 cli );
};

extern idServerWallhackSystemLocal serverWallhackLocal;

#endif //!__SERVERWALLHACK_H__
