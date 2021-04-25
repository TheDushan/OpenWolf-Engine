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
// File name:   serverWallhack.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <server/serverAutoPrecompiled.hpp>
#elif DEDICATED
#include <server/serverDedPrecompiled.hpp>
#else
#include <framework/precompiled.hpp>
#endif

idServerWallhackSystemLocal serverWallhackLocal;

/*
===============
idServerWallhackSystemLocal::idServerWallhackSystemLocal
===============
*/
idServerWallhackSystemLocal::idServerWallhackSystemLocal(void) {
}

/*
===============
idServerWallhackSystemLocal::~idServerWallhackSystemLocal
===============
*/
idServerWallhackSystemLocal::~idServerWallhackSystemLocal(void) {
}

/*
===============
idServerWallhackSystemLocal::zero_vector
===============
*/
sint idServerWallhackSystemLocal::zero_vector(vec3_t v) {
    if(v[0] > POS_LIM || v[0] < NEG_LIM) {
        return 0;
    }

    if(v[1] > POS_LIM || v[1] < NEG_LIM) {
        return 0;
    }

    if(v[2] > POS_LIM || v[2] < NEG_LIM) {
        return 0;
    }

    return 1;
}

/*
===============
idServerWallhackSystemLocal::predict_clip_velocity

@note The following functions for predicting player positions
have been adopted from 'g_unlagged.c' which is part of the
unlagged' system created by Neil "haste" Toronto.
WEB site: http://www.ra.is/unlagged
===============
*/
void idServerWallhackSystemLocal::predict_clip_velocity(vec3_t in,
        vec3_t normal, vec3_t out) {
    float32 backoff;

    // find the magnitude of the vector "in" along "normal"
    backoff = DotProduct(in, normal);

    // tilt the plane a bit to avoid floating-point error issues
    if(backoff < 0) {
        backoff *= 1.001f;
    } else {
        backoff /= 1.001f;
    }

    // slide along
    VectorMA(in, -backoff, normal, out);
}

/*
===============
idServerWallhackSystemLocal::predict_slide_move
===============
*/
sint idServerWallhackSystemLocal::predict_slide_move(sharedEntity_t *ent,
        float32 frametime, trajectory_t *tr, vec3_t result) {
    sint count, numplanes = 0, i, j, k;
    float32 d, time_left = frametime, into;
    vec3_t planes[MAX_CLIP_PLANES], velocity, origin, clipVelocity,
           endVelocity, endClipVelocity, dir, end;
    trace_t trace;

    VectorCopy(tr->trBase, origin);
    origin[2] += Z_ADJUST;  // move it off the floor

    VectorCopy(tr->trDelta, velocity);
    VectorCopy(tr->trDelta, endVelocity);

    for(count = 0; count < NUMBUMPS; count++) {
        // calculate position we are trying to move to
        VectorMA(origin, time_left, velocity, end);

        // see if we can make it there
        serverWorldSystem->Trace(&trace, origin, ent->r.mins, ent->r.maxs, end,
                                 ent->s.number, CONTENTS_SOLID, TT_AABB);

        if(trace.allsolid) {
            // entity is completely trapped in another solid
            VectorCopy(origin, result);
            return 0;
        }

        if(trace.fraction > 0.99f) { // moved the entire distance
            VectorCopy(trace.endpos, result);
            return 1;
        }

        if(trace.fraction > 0) { // covered some distance
            VectorCopy(trace.endpos, origin);
        }

        time_left -= time_left * trace.fraction;

        if(numplanes >= MAX_CLIP_PLANES) {
            // this shouldn't really happen
            VectorCopy(origin, result);
            return 0;
        }

        // if this is the same plane we hit before, nudge velocity
        // out along it, which fixes some epsilon issues with
        // non-axial planes
        for(i = 0; i < numplanes; i++) {
            if(DotProduct(trace.plane.normal, planes[i]) > 0.99f) {
                VectorAdd(trace.plane.normal, velocity, velocity);
                break;
            }
        }

        if(i < numplanes) {
            continue;
        }

        VectorCopy(trace.plane.normal, planes[numplanes]);
        numplanes++;

        // modify velocity so it parallels all of the clip planes
        // find a plane that it enters
        for(i = 0; i < numplanes; i++) {
            into = DotProduct(velocity, planes[i]);

            if(into >= 0.1f) { // move doesn't interact with the plane
                continue;
            }

            // slide along the plane
            predict_clip_velocity(velocity, planes[i], clipVelocity);

            // slide along the plane
            predict_clip_velocity(endVelocity, planes[i], endClipVelocity);

            // see if there is a second plane that the new move enters
            for(j = 0; j < numplanes; j++) {
                if(j == i) {
                    continue;
                }

                if(DotProduct(clipVelocity,
                              planes[j]) >= 0.1f) {   // move doesn't interact with the plane
                    continue;
                }

                // try clipping the move to the plane
                predict_clip_velocity(clipVelocity, planes[j], clipVelocity);
                predict_clip_velocity(endClipVelocity, planes[j], endClipVelocity);

                // see if it goes back into the first clip plane
                if(DotProduct(clipVelocity, planes[i]) >= 0) {
                    continue;
                }

                // slide the original velocity along the crease
                CrossProduct(planes[i], planes[j], dir);
                VectorNormalize(dir);
                d = DotProduct(dir, velocity);
                VectorScale(dir, d, clipVelocity);

                CrossProduct(planes[i], planes[j], dir);
                VectorNormalize(dir);
                d = DotProduct(dir, endVelocity);
                VectorScale(dir, d, endClipVelocity);

                // see if there is a third plane the new move enters
                for(k = 0; k < numplanes; k++) {
                    if(k == i || k == j) {
                        continue;
                    }

                    if(DotProduct(clipVelocity,
                                  planes[k]) >= 0.1f) {   // move doesn't interact with the plane
                        continue;
                    }

                    // stop dead at a tripple plane interaction
                    VectorCopy(origin, result);
                    return 1;
                }
            }

            // if we have fixed all interactions, try another move
            VectorCopy(clipVelocity, velocity);
            VectorCopy(endClipVelocity, endVelocity);
            break;
        }
    }

    VectorCopy(origin, result);

    if(count == 0) {
        return 1;
    }

    return 0;
}

/*
===============
idServerWallhackSystemLocal::predict_move
===============
*/
void idServerWallhackSystemLocal::predict_move(sharedEntity_t *ent,
        float32 frametime, trajectory_t *tr, vec3_t result) {
    float32 stepSize;
    vec3_t start_o, start_v, down, up;
    trace_t trace;

    VectorCopy(tr->trBase, result);   // assume the move fails

    if(zero_vector(tr->trDelta)) {   // not moving
        return;
    }

    if(predict_slide_move(ent, frametime, tr, result)) {   // move completed
        return;
    }

    VectorCopy(tr->trBase, start_o);
    VectorCopy(tr->trDelta, start_v);

    VectorCopy(start_o, up);
    up[2] += STEPSIZE;

    // test the player position if they were a stepheight higher
    serverWorldSystem->Trace(&trace, start_o, ent->r.mins, ent->r.maxs, up,
                             ent->s.number, CONTENTS_SOLID, TT_AABB);

    if(trace.allsolid) {    // can't step up
        return;
    }

    stepSize = trace.endpos[2] - start_o[2];

    // try slidemove from this position
    VectorCopy(trace.endpos, tr->trBase);
    VectorCopy(start_v, tr->trDelta);

    predict_slide_move(ent, frametime, tr, result);

    // push down the final amount
    VectorCopy(tr->trBase, down);
    down[2] -= stepSize;
    serverWorldSystem->Trace(&trace, tr->trBase, ent->r.mins, ent->r.maxs,
                             down, ent->s.number, CONTENTS_SOLID, TT_AABB);

    if(!trace.allsolid) {
        VectorCopy(trace.endpos, result);
    }
}

/*
===============
idServerWallhackSystemLocal::calc_viewpoint

@brief Calculates the view point of a player model at position 'org' using
information in the player state 'ps' of its client, and stores the
viewpoint coordinates in 'vp'.
===============
*/
void idServerWallhackSystemLocal::calc_viewpoint(playerState_t *ps,
        vec3_t org, vec3_t vp) {
    VectorCopy(org, vp);

    if(ps->leanf != 0.f) {
        vec3_t right, v3ViewAngles;

        VectorCopy(ps->viewangles, v3ViewAngles);
        v3ViewAngles[2] += ps->leanf / 2.0f;
        AngleVectors(v3ViewAngles, nullptr, right, nullptr);
        VectorMA(org, ps->leanf, right, org);
    }

    if(ps->pm_flags & PMF_DUCKED) {
        vp[2] += CROUCH_VIEWHEIGHT;
    } else {
        vp[2] += DEFAULT_VIEWHEIGHT;
    }
}

/*
===============
idServerWallhackSystemLocal::player_in_fov
===============
*/
sint idServerWallhackSystemLocal::player_in_fov(vec3_t viewangle,
        vec3_t ppos, vec3_t opos) {
    float32 yaw, pitch, cos_angle;
    vec3_t dir, los;

    VectorSubtract(opos, ppos, los);

    // Check if the two players are roughly on the same X/Y plane
    // and skip the test if not. We only want to eliminate info that
    // would reveal the position of opponents behind the player on
    // the same X/Y plane (e.g. on the same floor in a room).
    if(VectorLength(los) < (5.f * Q_fabs((opos[2] - ppos[2])))) {
        return 1;
    }

    // calculate unit vector of the direction the player looks at
    yaw = viewangle[YAW] * (M_PI * 2 / 360);
    pitch = viewangle[PITCH] * (M_PI * 2 / 360);
    dir[0] = cos(yaw) * cos(pitch);
    dir[1] = sin(yaw);
    dir[2] = cos(yaw) * sin(pitch);

    // calculate unit vector corresponding to line of sight to opponent
    VectorNormalize(los);

    // calculate and test the angle between the two vectors
    cos_angle = DotProduct(dir, los);

    if(cos_angle > 0) {     // +/- 90 degrees (fov = 180)
        return 1;
    }

    return 0;
}

/*
===============
idServerWallhackSystemLocal::copy_trajectory
===============
*/
void idServerWallhackSystemLocal::copy_trajectory(trajectory_t *src,
        trajectory_t *dst) {
    dst->trType = src->trType;
    dst->trTime = src->trTime;
    dst->trDuration = src->trDuration;
    VectorCopy(src->trBase, dst->trBase);
    VectorCopy(src->trDelta, dst->trDelta);
}

/*
===============
idServerWallhackSystemLocal::is_visible
===============
*/
sint idServerWallhackSystemLocal::is_visible(vec3_t start, vec3_t end) {
    trace_t trace;

    collisionModelManager->BoxTrace(&trace, start, end, nullptr, nullptr, 0,
                                    CONTENTS_SOLID, TT_NONE);

    if(trace.contents & CONTENTS_SOLID) {
        return 0;
    }

    return 1;
}

/*
===============
idServerWallhackSystemLocal::init_horz_delta
===============
*/
void idServerWallhackSystemLocal::init_horz_delta(void) {
    sint i;

    bbox_horz = sv_wh_bbox_horz->integer;

    for(i = 0; i < 8; i++) {
        delta[i][0] = (bbox_horz * delta_sign[i][0]) / 2.0f;
        delta[i][1] = (bbox_horz * delta_sign[i][1]) / 2.0f;
    }
}

/*
===============
idServerWallhackSystemLocal::init_vert_delta
===============
*/
void idServerWallhackSystemLocal::init_vert_delta(void) {
    sint i;

    bbox_vert = sv_wh_bbox_vert->integer;

    for(i = 0; i < 8; i++) {
        delta[i][2] = (bbox_vert * delta_sign[i][2]) / 2.0f;
    }
}

/*
===============
idServerWallhackSystemLocal::InitWallhack
===============
*/
void idServerWallhackSystemLocal::InitWallhack(void) {
    init_horz_delta();
    init_vert_delta();
}

/*
===============
idServerWallhackSystemLocal::CanSee

@brief Checks if 'player' can see 'other' or not.

@details First a check is made if 'other' is in the maximum allowed fov
of 'player'. If not, then zero is returned w/o any further checks.
Next traces are carried out from the present viewpoint of 'player'
to the corners of the bounding box of 'other'. If any of these
traces are successful (i.e. nothing solid is between the start
and end positions) then non-zero is returned.

Otherwise the expected positions of the two players are calculated,
by extrapolating their movements for PREDICT_TIME seconds and the above
tests are carried out again. The result is reported by returning non-zero
(expected to become visible) or zero (not expected to become visible
in the next frame).
===============
*/
sint idServerWallhackSystemLocal::CanSee(sint player, sint other) {
    sharedEntity_t *pent, * oent;
    playerState_t *ps;
    vec3_t viewpoint, tmp;
    sint i;

    // check if bounding box has been changed
    if(sv_wh_bbox_horz->integer != bbox_horz) {
        init_horz_delta();
    }

    if(sv_wh_bbox_vert->integer != bbox_vert) {
        init_vert_delta();
    }

    ps = serverGameSystem->GameClientNum(player);
    pent = serverGameSystem->GentityNum(player);
    oent = serverGameSystem->GentityNum(other);

    // check if 'other' is in the maximum fov allowed
    if(sv_wh_check_fov->integer > 0) {
        if(!player_in_fov(pent->s.apos.trBase, pent->s.pos.trBase,
                          oent->s.pos.trBase)) {
            return 0;
        }
    }

    // check if visible in this frame
    calc_viewpoint(ps, pent->s.pos.trBase, viewpoint);

    for(i = 0; i < 8; i++) {
        VectorCopy(oent->s.pos.trBase, tmp);
        tmp[0] += delta[i][0];
        tmp[1] += delta[i][1];
        tmp[2] += delta[i][2] + VOFS;

        if(is_visible(viewpoint, tmp)) {
            return 1;
        }
    }

    // predict player positions
    copy_trajectory(&pent->s.pos, &traject);
    predict_move(pent, PREDICT_TIME, &traject, pred_ppos);

    copy_trajectory(&oent->s.pos, &traject);
    predict_move(oent, PREDICT_TIME, &traject, pred_opos);

    // Check again if 'other' is in the maximum fov allowed.
    // FIXME: We use the original viewangle that may have
    // changed during the move. This could introduce some
    // errors.
    if(sv_wh_check_fov->integer > 0) {
        if(!player_in_fov(pent->s.apos.trBase, pred_ppos, pred_opos)) {
            return 0;
        }
    }

    // check if expected to be visible in the next frame
    calc_viewpoint(ps, pred_ppos, viewpoint);

    for(i = 0; i < 8; i++) {
        VectorCopy(pred_opos, tmp);
        tmp[0] += delta[i][0];
        tmp[1] += delta[i][1];
        tmp[2] += delta[i][2] + VOFS;

        if(is_visible(viewpoint, tmp)) {
            return 1;
        }
    }

    return 0;
}

/*
===============
idServerWallhackSystemLocal::RandomizePos

@brief Changes the position of client 'other' so that it is directly
below 'player'. The distance is maintained so that sound scaling
will work correctly.
===============
*/
void idServerWallhackSystemLocal::RandomizePos(sint player, sint other) {
    sharedEntity_t *pent, * oent;
    vec3_t los;
    float32 dist;

    pent = serverGameSystem->GentityNum(player);
    oent = serverGameSystem->GentityNum(other);

    VectorCopy(oent->s.pos.trBase, old_origin[other]);
    origin_changed[other] = 1;

    // get distance (we need it for correct sound scaling)
    VectorSubtract(oent->s.pos.trBase, pent->s.pos.trBase, los);
    dist = VectorLength(los);

    // set the opponent's position directly below the player
    VectorCopy(pent->s.pos.trBase, oent->s.pos.trBase);
    oent->s.pos.trBase[2] -= dist;
}

/*
===============
idServerWallhackSystemLocal::RestorePos
===============
*/
void idServerWallhackSystemLocal::RestorePos(sint cli) {
    sharedEntity_t *ent;

    ent = serverGameSystem->GentityNum(cli);
    VectorCopy(old_origin[cli], ent->s.pos.trBase);
    origin_changed[cli] = 0;
}

/*
===============
idServerWallhackSystemLocal::PositionChanged
===============
*/
sint idServerWallhackSystemLocal::PositionChanged(sint cli) {
    return origin_changed[cli];
}
