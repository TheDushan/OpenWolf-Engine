////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2013 Stephen Larroque <lrq3000@gmail.com>
// Copyright(C) 2018 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   serverOACS.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SERVEROACS__H__
#define __SERVEROACS__H__

#define MAX_STRING_CSV 2048

#ifdef NAN
#define featureDefaultValue NAN
#else
#define featureDefaultValue 0
#endif

// List all indexes for any feature.
// This is necessary in order to allow for a quick access to a feature (for updating purposes)
// You can add here your own features
//
// ACC (accumulators) and INARROW (actually the same thing as an accumulator) features are differential + counter features (meaning that it continues to increment as long as the difference of the feature it's based upon changes value)
enum interframeIndex_t
{
    FEATURE_PLAYERID,
    FEATURE_TIMESTAMP,
    
    // Human-specific features
    FEATURE_SVSTIME, // persistent/static server time (svs.time)
    FEATURE_REACTIONTIME, // this is svs.time delta (difference between the last interframe and this one)
    FEATURE_SVTIME, // non-persistent server time (sv.time), can be used to check when a new game starts
    FEATURE_LASTCOMMANDTIME, // ps->commandTime
    FEATURE_COMMANDTIME_REACTIONTIME,
    FEATURE_ANGLEINAFRAME, // abs(ps->viewangles[0] - prev_ps->viewangles[0]) + abs(ps->viewangles[1] - prev_ps->viewangles[1]) + abs(ps->viewangles[2] - prev_ps->viewangles[2]) // PITCH=0/YAW=1/ROLL=2 see q_shared.hpp
    FEATURE_LASTMOUSEEVENTTIME, // change when FEATURE_ANGLEINONEFRAME changes (based on svs.time)
    FEATURE_MOUSEEVENTTIME_REACTIONTIME, // FIXME: maybe add usercmd_t->button type in client_t: svs.clients[id].lastUsercmd.buttons & BUTTON_ATTACK or & BUTTON_ANY ? because for the moment only the angle change accounts for a reaction time. Maybe simply store the previous cmd and when we update, check for a change by comparing every fields of usercmd_t
    FEATURE_MOVEMENTDIR, // ps->movementDir
    
    // Semi human-specific and semi game-specific. These will be declared as human-specific.
    FEATURE_SCORE, // ps->persistant[PERS_SCORE] (it's a persEnum_t)
    FEATURE_SCOREACC,
    FEATURE_HITS,
    FEATURE_HITSACC,
    FEATURE_DEATH, // ps->persistant[PERS_KILLED]
    FEATURE_DEATHACC,
    
    FEATURE_FRAGS, // number of kills a player did, incremented in regard to when a player gets killed (we can then get the killer's id)
    FEATURE_FRAGSINAROW, // accumulator
    
    FEATURE_DAMAGEEVENT_COUNT, // damage one receive (not inflicted to the opponent). ps.damageEvent is an incremented counter
    FEATURE_DAMAGEEVENT_COUNTACC,
    
    FEATURE_DUCKED, // ps->pm_flags & PMF_DUCKED
    FEATURE_MIDAIR, // ps->groundEntityNum == ENTITYNUM_NONE
    
    FEATURE_WEAPON, // ps->weapon
    FEATURE_WEAPONSTATE, // ps->weaponstate
    FEATURE_WEAPONINSTANTHIT, // is weapon a long-range instant-hit weapon? (which can be delaggued and easier to use by aimbots?)
    
    // Game-specific features (dependent on the rules set on the game, which may change! thus you have to relearn these features for each gametype or mod you want to detect cheats on)
    FEATURE_HEALTH, // ps->stats[STAT_HEALTH] from statIndex_t enum
    FEATURE_MAX_HEALTH,
    FEATURE_SPEED, // abs(ps->velocity[0]) + abs(ps->velocity[1]) + abs(ps->velocity[2])
    FEATURE_SPEEDRATIO, // ( abs(ps->velocity[0]) + abs(ps->velocity[1]) + abs(ps->velocity[2]) ) / ps->speed; // ps->speed is the maximum speed the client should have
    FEATURE_DAMAGE_COUNT, // total amount damage one has received (not inflicted to the opponent).
    
    // Do not modify the rest of the features below
    FEATURE_FRAMEREPEAT, // Do not modify: this frame counts the number of times an interframe is repeated. This feature is necessary to keep storage space.
    LABEL_CHEATER, // Not a feature: this is used as the label Y for the features. Usually, this will be set at 0 for everyone, and set to 1 only by using the /cheater command for development purposes or to generate a dataset when you are using a cheating system.
    FEATURES_COUNT // Important: always place this at the very end! This is used to count the total number of features
};

// List of features types
// You should not modify this unless you know what you do (you'll have to code a new input parser inside OACS)
enum featureType_t
{
    FEATURE_ID,			// Identifier features
    FEATURE_HUMAN,			// Human-specific features
    FEATURE_GAMESPECIFIC,				// Game-specific features (game rules)
    FEATURE_PHYSICS,                     // Physics limitation features (to avoid!!!)
    FEATURE_METADATA,                   // Feature containing meta data that are useful for other features or just for post-analysis but are not to be used for the learning nor detection process unless post-processed into higher order features (such as svtime)
    FEATURE_METAINTERFRAME,           // Feature containing meta data about the interframe (like the framerepeat, which should be used as a ponderation factor for all the others features)
    FEATURE_LABEL // Not a feature, this is a label for the data
};

// Structure of one feature
typedef struct feature_s
{
    valueType* key;
    featureType_t type;
    bool modifier; // if true, change the modify flag (means that this feature modifies meaningfully the content of this interframe, and thus write a new separate interframe). Else if false, it will concatenate consequent interframes into only one but increments FEATURE_FRAMEREPEAT (this avoids storing multiple same frames taking up a lot of storage space uselessly).
    float64 value[MAX_CLIENTS]; // one value for each player (the key and type do not change, they are the same for every player. This saves some memory space.)
} feature_t;

// Structure of one player entry in the players' table
typedef struct playerstable_s
{
    float64 playerid;
    valueType ip[NET_ADDRSTRMAXLEN];
    valueType* guid;
    float64 timestamp;
    valueType datetime[MAX_STRING_CSV];
    valueType* nickname;
} playerstable_t;

// Declare the sv.interframe global variable, which will contain the array of all features
//extern feature_t interframe[FEATURES_COUNT];

// Cvars to configure OACS behavior
extern convar_t* sv_oacsEnable; // enable the extended logging facility?
extern convar_t* sv_oacsPlayersTableEnable; // enable the extended player identification logging?
extern convar_t* sv_oacsTypesFile; // where to save the features types
extern convar_t* sv_oacsDataFile; // where to save the features data
extern convar_t* sv_oacsPlayersTable; // where to save the players table (if enabled)
extern convar_t* sv_oacsMinPlayers; // minimum number of human players required to begin logging data
extern convar_t* sv_oacsLabelPassword; // password necessary for a player to label himself
extern convar_t* sv_oacsMaxPing; // max ping to accept interframes (above, the interframe will be dropped until the ping goes down)
extern convar_t* sv_oacsMaxLastPacketTime; // max last packet time to accept interframes (above, the interframe will be dropped until the LastPacketTime goes down)

//
// idServerOACSSystemLocal
//
class idServerOACSSystemLocal
{
public:
    idServerOACSSystemLocal();
    ~idServerOACSSystemLocal();
    
    // Functions
    static void ExtendedRecordInit( void );
    static void ExtendedRecordUpdate( void );
    static void ExtendedRecordShutdown( void );
    static void ExtendedRecordClientConnect( sint client );
    static void ExtendedRecordDropClient( sint client );
    static void ExtendedRecordWriteStruct( void );
    static void ExtendedRecordWriteValues( sint client );
    static void ExtendedRecordWritePlayersTable( sint client );
    static void ExtendedRecordInterframeInit( sint client );
    static void ExtendedRecordInterframeInitValues( sint client );
    static float64 ExtendedRecordInterframeInitValue( sint client, sint feature );
    static void ExtendedRecordInterframeUpdate( sint client );
    static void ExtendedRecordInterframeUpdateValues( sint client );
    static void ExtendedRecordInterframeUpdateValuesAttacker( sint client );
    static void ExtendedRecordPlayersTableInit( sint client );
    static valueType* ExtendedRecordFeaturesToCSV( valueType* csv_string, sint max_string_size, feature_t* interframe, sint savewhat, sint client );
    static valueType* ExtendedRecordPlayersTableToCSV( valueType* csv_string, sint max_string_size, playerstable_t playerstable );
    static void ExtendedRecordSetFeatureValue( interframeIndex_t feature, float64 value, sint client );
    static bool IsBot( sint client );
    static bool IsSpectator( sint client );
    static bool IsWeaponInstantHit( sint weapon );
    static sint CountPlayers( void );
    static sint rand_range( sint min, sint max );
    static void ExtendedRecordSetCheater( sint client, sint label );
    static void ExtendedRecordSetCheaterFromClient_f( client_t* cl );
    static void ExtendedRecordSetHonestFromClient_f( client_t* cl );
    static void ExtendedRecordSetCheater_f( void );
};

extern idServerOACSSystemLocal serverOACSLocal;

#endif //!__SERVERCCMDS_H__
