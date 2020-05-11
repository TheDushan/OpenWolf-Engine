////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 - 2019 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   serverGame_api.h
// Version:     v1.01
// Created:     11/24/2018
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SERVERGAME_API_H__
#define __SERVERGAME_API_H__

typedef struct
{
    bool linked;         // false if not in any good cluster
    S32 linkcount;
    
    S32 svFlags;        // SVF_NOCLIENT, SVF_BROADCAST, etc.
    S32 singleClient;   // only send to this client when SVF_SINGLECLIENT is set
    S32 hiMask, loMask; // if SVF_CLIENTMASK is set, then only send to the
    //  clients specified by the following 64-bit bitmask:
    //  hiMask: high-order bits (32..63)
    //  loMask: low-order bits (0..31)
    
    bool bmodel;         // if false, assume an explicit mins/maxs bounding box
    // only set by trap_SetBrushModel
    vec3_t mins, maxs;
    S32 contents;       // CONTENTS_TRIGGER, CONTENTS_SOLID, CONTENTS_BODY, etc.
    // a non-solid entity should have this set to 0
    
    vec3_t absmin, absmax; // derived from mins/maxs and origin + rotation
    
    // currentOrigin will be used for all collision detection and world linking.
    // it will not necessarily be the same as the trajectory evaluation for the current
    // time, because each entity must be moved one at a time after time is advanced
    // to avoid simultanious collision issues
    vec3_t currentOrigin;
    vec3_t currentAngles;
    
    // when a trace call is made and the specified pass entity isn't none,
    //  then a given entity will be excluded from testing if:
    // - the given entity is the pass entity (use case: don't interact with self),
    // - the owner of the given entity is the pass entity (use case: don't interact with your own missiles), or
    // - the given entity and the pass entity have the same owner entity (that is not none)
    //    (use case: don't interact with other missiles from owner).
    // that is, ent will be excluded if
    // ( passEntityNum != ENTITYNUM_NONE &&
    //   ( ent->s.number == passEntityNum || ent->r.ownerNum == passEntityNum ||
    //     ( ent->r.ownerNum != ENTITYNUM_NONE && ent->r.ownerNum == entities[passEntityNum].r.ownerNum ) ) )
    S32 ownerNum;
    S32 eventTime;
    
    S32 worldflags;
    
    bool snapshotCallback;
} entityShared_t;

// the server looks at a sharedEntity_t structure, which must be at the start of a gentity_t structure
typedef struct
{
    entityState_t  s; // communicated by the server to clients
    entityShared_t r; // shared by both the server and game module
} sharedEntity_t;

#ifdef GAMEDLL
typedef struct gclient_s gclient_t;
typedef struct gentity_s gentity_t;
#define sharedEntity_t gentity_t
#else
#define gentity_t sharedEntity_t
#endif

#define MAX_ENT_CLUSTERS    16

typedef struct svEntity_s
{
    struct worldSector_s* worldSector;
    struct svEntity_s* nextEntityInWorldSector;
    
    entityState_t   baseline;	// for delta compression of initial sighting
    S32             numClusters;	// if -1, use headnode instead
    S32             clusternums[MAX_ENT_CLUSTERS];
    S32             lastCluster;	// if all the clusters don't fit in clusternums
    S32             areanum, areanum2;
    S32             snapshotCounter;	// used to prevent double adding from portal views
    S32             originCluster;	// Gordon: calced upon linking, for origin only bmodel vis checks
} svEntity_t;

//
// idServerGameSystem
//
class idServerGameSystem
{
public:
    virtual void ShutdownGameProgs( void ) = 0;
    virtual bool GameCommand( void ) = 0;
    virtual void LocateGameData( sharedEntity_t* gEnts, S32 numGEntities, S32 sizeofGEntity_t, playerState_t* clients, S32 sizeofGameClient ) = 0;
    virtual void GameDropClient( S32 clientNum, StringEntry reason, S32 length ) = 0;
    virtual void GameSendServerCommand( S32 clientNum, StringEntry text ) = 0;
    virtual bool EntityContact( const vec3_t mins, const vec3_t maxs, const sharedEntity_t* gEnt, traceType_t type ) = 0;
    virtual void SetBrushModel( sharedEntity_t* ent, StringEntry name ) = 0;
    virtual bool inPVS( const vec3_t p1, const vec3_t p2 ) = 0;
    virtual bool inPVSIgnorePortals( const vec3_t p1, const vec3_t p2 ) = 0;
    virtual void GetServerinfo( UTF8* buffer, S32 bufferSize ) = 0;
    virtual void AdjustAreaPortalState( sharedEntity_t* ent, bool open ) = 0;
    virtual void UpdateSharedConfig( U32 port, StringEntry rconpass ) = 0;
    virtual void GetUsercmd( S32 clientNum, usercmd_t* cmd ) = 0;
    virtual bool GetTag( S32 clientNum, S32 tagFileNumber, UTF8* tagname, orientation_t* _or ) = 0;
    virtual bool GetEntityToken( UTF8* buffer, S32 bufferSize ) = 0;
    virtual bool GameIsSinglePlayer( void ) = 0;
    virtual void InitGameProgs( void ) = 0;
    virtual sharedEntity_t* GentityNum( S32 num ) = 0;
    virtual svEntity_t* SvEntityForGentity( sharedEntity_t* gEnt ) = 0;
    virtual bool GameIsCoop( void ) = 0;
    virtual sharedEntity_t* GEntityForSvEntity( svEntity_t* svEnt ) = 0;
    virtual void RestartGameProgs( void ) = 0;
    virtual playerState_t* GameClientNum( S32 num ) = 0;
    virtual S32 DemoWriteCommand( S32 cmd, StringEntry str ) = 0;
};

extern idServerGameSystem* serverGameSystem;

#endif //!__SERVERGAME_API_H__
