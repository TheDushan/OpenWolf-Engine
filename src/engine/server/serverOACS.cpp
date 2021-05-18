////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2013 Stephen Larroque <lrq3000@gmail.com>
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
// File name:   serverOACS.cpp
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

idServerOACSSystemLocal serverOACSLocal;

/*
===============
idServerOACSSystemLocal::idServerOACSSystemLocal
===============
*/
idServerOACSSystemLocal::idServerOACSSystemLocal(void) {
}

/*
===============
idServerOACSSystemLocal::~idServerOACSSystemLocal
===============
*/
idServerOACSSystemLocal::~idServerOACSSystemLocal(void) {
}


// OACS extended recording variables (necessary for functionning)
feature_t sv_interframe[FEATURES_COUNT];
bool sv_interframeModified[MAX_CLIENTS]; // was the current interframe modified from the previous one?
playerstable_t
sv_playerstable; // extended player identification data (we only need to store one in memory at a time, since we only need it at client connection)
sint sv_oacshumanplayers; // oacs implementation of g_humanplayers (but we also count privateclients too!)
playerState_t prev_ps[MAX_CLIENTS]; // previous frame's playerstate

valueType *sv_playerstable_keys =
    "playerid,playerip,playerguid,connection_timestamp,connection_datetime,playername"; // key names, edit this if you want to add more infos in the playerstable

// names of the features, array of string keys to output in the typesfile and datafile
valueType *sv_interframe_keys[] = {
    "playerid",
    "timestamp",

    "svstime",
    "reactiontime",
    "svtime",
    "lastcommandtime",
    "commandtime_reactiontime",
    "angleinaframe",
    "lastmouseeventtime",
    "mouseeventtime_reactiontime",
    "movementdirection",

    "score",
    "scoreacc",
    "hits",
    "hitsacc",
    "death",
    "deathacc",

    "frags",
    "fragsinarow",

    "selfdamageeventcount",
    "selfdamageeventcountacc",

    "ducked",
    "midair",

    "weapon",
    "weaponstate",
    "weaponinstanthit",

    "health",
    "max_health",
    "speed",
    "speedratio",
    "selfdamagecount",

    "framerepeat",
    "cheater"
};

// types of the features, will be outputted in the typesfile
sint sv_interframe_types[] = {
    FEATURE_ID, // playerid
    FEATURE_ID, // timestamp

    FEATURE_ID, // svstime
    FEATURE_HUMAN, // reactiontime
    FEATURE_ID, // svtime
    FEATURE_METADATA, // lastcommandtime
    FEATURE_HUMAN, // commandtime_reactiontime
    FEATURE_HUMAN, // angleinaframe
    FEATURE_METADATA, // lastmouseeventtime
    FEATURE_HUMAN, // mouseeventtime_reactiontime
    FEATURE_HUMAN, // movementdirection

    FEATURE_PHYSICS, // score
    FEATURE_HUMAN, // scoreacc
    FEATURE_PHYSICS, // hits
    FEATURE_HUMAN, // hitsacc
    FEATURE_PHYSICS, // death
    FEATURE_HUMAN, // deathacc

    FEATURE_PHYSICS, // frags
    FEATURE_HUMAN, // fragsinarow

    FEATURE_PHYSICS, // damagecount
    FEATURE_HUMAN, // damageeventcountacc

    FEATURE_HUMAN, // ducked
    FEATURE_HUMAN, // midair

    FEATURE_HUMAN, // weapon
    FEATURE_HUMAN, // weaponstate
    FEATURE_HUMAN, // weaponinstanthit

    FEATURE_PHYSICS, // health
    FEATURE_PHYSICS, // max_health
    FEATURE_PHYSICS, // speed
    FEATURE_GAMESPECIFIC, // speedratio
    FEATURE_GAMESPECIFIC, // damagecount

    FEATURE_METAINTERFRAME, // framerepeat
    FEATURE_LABEL // cheater
};

//modifiers for the features, array of boolean that specifies if a feature should commit the interframe on change or not
bool sv_interframe_modifiers[] = {
    true, // playerid
    false, // timestamp

    false, // svstime
    false, // reactiontime
    false, // svtime
    false, // lastcommandtime
    false, // commandtime_reactiontime
    true, // angleinaframe
    true, // lastmouseeventtime
    false, // mouseeventtime_reactiontime
    false, // movementdirection

    true, // score
    true, // scoreacc
    true, // hits
    true, // hitsacc
    true, // death
    true, // deathacc

    true, // frags
    true, // fragsinarow

    true, // damagecount
    true, // damageeventcountacc

    true, // ducked
    true, // midair

    true, // weapon
    true, // weaponstate
    true, // weaponinstanthit


    true, // health
    false, // max_health
    false, // speed
    false, // speedratio
    false, // damagecount

    false, // framerepeat
    true // cheater
};

/*
===============
idServerOACSSystemLocal::ExtendedRecordDropClient

Initialize the interframe structure at the start of the server
Called only once, at the launching of the server
===============
*/
void idServerOACSSystemLocal::ExtendedRecordInit(void) {
    //Initializing the random seed for the random values generator
    srand(time(nullptr));

    // Initialize the features
    ExtendedRecordInterframeInit(-1);

    // Write down the types in the typesfile
    ExtendedRecordWriteStruct();
}

/*
===============
idServerOACSSystemLocal::ExtendedRecordDropClient

Update interframe values for all connected clients
Called for each server frame
===============
*/
void idServerOACSSystemLocal::ExtendedRecordUpdate(void) {
    // Update the features values
    ExtendedRecordInterframeUpdate(-1);

    // Write down the values in the datafile
    //ExtendedRecordWriteValues(-1); // this will be automatically called when an interframe needs to be committed
}

/*
===============
idServerOACSSystemLocal::ExtendedRecordDropClient

Write down the last interframe values at shutdown
Called once at engine shutdown
===============
*/
void idServerOACSSystemLocal::ExtendedRecordShutdown(void) {
    // Write down all the not yet committed values into the datafile
    ExtendedRecordWriteValues(-1);
}

/*
===============
idServerOACSSystemLocal::ExtendedRecordDropClient

When a client connects to the server
===============
*/
void idServerOACSSystemLocal::ExtendedRecordClientConnect(sint client) {
    // Proceed only if the player is not a bot
    if(!IsBot(client)) {
        // Recompute the number of connected human players
        sv_oacshumanplayers = CountPlayers();

        // Init the values for this client (like playerid, etc.)
        ExtendedRecordInterframeInit(client);
        ExtendedRecordInterframeInitValues(client);

        // If the admin is willing to save extended identification informations
        if(sv_oacsPlayersTableEnable->integer == 1) {
            // Init the playerstable entry of this client (extended player identification informations)
            ExtendedRecordPlayersTableInit(client);

            // Write down the entry into the players table file
            ExtendedRecordWritePlayersTable(client);
        }
    }
}

/*
===============
idServerOACSSystemLocal::ExtendedRecordDropClient

When a client gets disconnected (either willingly or unwillingly)
===============
*/
void idServerOACSSystemLocal::ExtendedRecordDropClient(sint client) {
    // Drop the client only if he isn't already dropped (so that the reset won't be called multiple times while the client goes into CS_ZOMBIE)
    if(!((sv_interframe[FEATURE_PLAYERID].value[client] == featureDefaultValue)
            |
            isnan(sv_interframe[FEATURE_PLAYERID].value[client]) | IsBot(client))) {
        // Recompute the number of connected human players
        sv_oacshumanplayers = CountPlayers();

        // Commit the last interframe for that client before he gets totally disconnected
        ExtendedRecordWriteValues(client);
        // Reinit the values for that client
        ExtendedRecordInterframeInit(client);
    }
}

/*
===============
idServerOACSSystemLocal::ExtendedRecordWriteValues

Write the types of the features in a text file, in CSV format into a file
Note: this function only needs to be called once, preferably at engine startup (Com_Init or SV_Init)
===============
*/
void idServerOACSSystemLocal::ExtendedRecordWriteStruct(void) {
    fileHandle_t file;
    valueType outheader[MAX_STRING_CSV];
    valueType out[MAX_STRING_CSV];

    if(developer->integer) {
        Com_Printf("OACS: Saving the oacs types in file %s\n",
                   sv_oacsTypesFile->string);
    }

    ExtendedRecordFeaturesToCSV(outheader, MAX_STRING_CSV, sv_interframe, 0,
                                -1);
    ExtendedRecordFeaturesToCSV(out, MAX_STRING_CSV, sv_interframe, 1, -1);

    // Output into the text file
    file = fileSystem->FOpenFileWrite(
               sv_oacsTypesFile->string);   // open in write mode
    fileSystem->Write(va("%s\n%s", outheader, out),
                      strlen(outheader) + strlen("\n") + strlen(out),
                      file);     //free(out); // write the text and free it
    fileSystem->Flush(file);   // update the content of the file
    fileSystem->FCloseFile(file);   // close the file
}

/*
===============
idServerOACSSystemLocal::ExtendedRecordWriteValues

Write the values of the current interframe's features in CSV format into a file
===============
*/
void idServerOACSSystemLocal::ExtendedRecordWriteValues(sint client) {
    fileHandle_t    file;
    valueType out[MAX_STRING_CSV];
    sint i, startclient, endclient;
    playerState_t *ps;

    if(sv_oacshumanplayers <
            sv_oacsMinPlayers->integer) { // if we are below the minimum number of human players, we just break here
        return;
    }

    // If a client id is supplied, we will only write the JSON interframe for this client
    if(client >= 0) {
        startclient = client;
        endclient = client + 1;
        // Else for every client
    } else {
        startclient = 0;
        endclient = sv_maxclients->integer;
    }

    // Open the data file
    file = fileSystem->FOpenFileAppend(
               sv_oacsDataFile->string);   // open in append mode

    // If there is no data file or it is empty, we first output the headers (features keys/names)
    if(fileSystem->FileExists(sv_oacsDataFile->string) == false ||
            (fileSystem->IsFileEmpty(sv_oacsDataFile->string) == true)) {
        valueType outheader[MAX_STRING_CSV];

        // Get the CSV string from the features keys
        ExtendedRecordFeaturesToCSV(outheader, MAX_STRING_CSV, sv_interframe, 0,
                                    -1);

        // Output the headers into the text file (only if there's at least one client connected!)
        fileSystem->Write(va("%s\n", outheader), strlen(outheader) + strlen("\n"),
                          file);  // write the text (with a line return)
        fileSystem->Flush(file);   // update the content of the file
    }

    for(i = startclient; i < endclient; i++) {
        ps = serverGameSystem->GameClientNum(i);

        if((svs.clients[i].state < CS_ACTIVE)) {
            continue;
        } else if(IsBot(i) | IsSpectator(
                      i)) {   // avoid saving empty interframes of not yet fully connected players, bots nor spectators
            continue;
        } else if(((svs.time - svs.clients[client].lastPacketTime) >
                   sv_oacsMaxLastPacketTime->integer) ||
                  (svs.clients[client].ping > sv_oacsMaxPing->integer) ||
                  // drop the last interframe(s) if the player is lagging (we don't want to save extreme values for reaction time and such stuff just because the player is flying in the air, waiting for the connection to stop lagging)
                  ((svs.clients[client].netchan.outgoingSequence -
                    svs.clients[client].deltaMessage) >= (PACKET_BACKUP - 3)))
            // client hasn't gotten a good message through in a long time
        {
            continue;
        } else if(ps->pm_type !=
                  PM_NORMAL) { // save only if the player is playing in a normal state, not in a special state where he can't play like dead or intermission FIXME: maybe add PM_DEAD too?
            continue;
        }

        ExtendedRecordFeaturesToCSV(out, MAX_STRING_CSV, sv_interframe, 2, i);

        // Output into the text file (only if there's at least one client connected!)
        if(developer->integer) {
            Com_Printf("OACS: Saving the oacs values for client %i in file %s\n",
                       client, sv_oacsDataFile->string);
        }

        fileSystem->Write(va("%s\n", out), strlen(out) + strlen("\n"),
                          file);         //free(out); // write the text (with a line return) and free it
        fileSystem->Flush(file);   // update the content of the file
    }

    // Close the data file and free variables
    fileSystem->FCloseFile(file);   // close the file
    // No variable to free, there's no malloc'd variable
}

/*
===============
idServerOACSSystemLocal::ExtendedRecordWritePlayersTable

Write the values of the current players table entry (extended identification informations for one player) in CSV format into a file
===============
*/
void idServerOACSSystemLocal::ExtendedRecordWritePlayersTable(
    sint client) {
    fileHandle_t file;
    valueType out[MAX_STRING_CSV];

    // avoid saving empty players table entry of not yet fully connected players and bots
    if(IsBot(client)) {
        return;
    }

    // Open the data file
    if(developer->integer) {
        Com_Printf("OACS: Saving the oacs players table entry for client %i in file %s\n",
                   client, sv_oacsPlayersTable->string);
    }

    file = fileSystem->FOpenFileAppend(
               sv_oacsPlayersTable->string);   // open in append mode

    // If there is no data file or it is empty, we first output the headers (features keys/names)
    if(fileSystem->FileExists(sv_oacsPlayersTable->string) == false ||
            (fileSystem->IsFileEmpty(sv_oacsPlayersTable->string) == true)) {
        // Output the headers into the text file (only if there's at least one client connected!)
        fileSystem->Write(va("%s\n", sv_playerstable_keys),
                          strlen(sv_playerstable_keys) + strlen("\n"),
                          file);     // write the text (with a line return)
        fileSystem->Flush(file);   // update the content of the file
    }

    ExtendedRecordPlayersTableToCSV(out, MAX_STRING_CSV, sv_playerstable);

    // Output into the text file (only if there's at least one client connected!)
    fileSystem->Write(va("%s\n", out), strlen(out) + strlen("\n"),
                      file);         //free(out); // write the text (with a line return) and free it
    fileSystem->Flush(file);   // update the content of the file

    // Close the data file and free variables
    fileSystem->FCloseFile(file);   // close the file
    // No variable to free, there's no malloc'd variable
}

/*
===============
idServerOACSSystemLocal::ExtendedRecordInterframeInitValues

Will init the interframe types and values
Note: this is also used to reset the values for one specific client at disconnection
Add here the initialization settings for your own features
===============
*/
void idServerOACSSystemLocal::ExtendedRecordInterframeInit(sint client) {
    sint i, j, startclient, endclient;

    Com_Printf("OACS: Initializing the features for client %i\n", client);

    // at startup (no client), initialize the key name, type and modifier of each variable
    if(client < 0) {
        // for each variable, we set its name (key), type and modifier flag
        for(i = 0; i < FEATURES_COUNT; i++) {
            sv_interframe[i].key = sv_interframe_keys[i];
            sv_interframe[i].type = (featureType_t)sv_interframe_types[i];
            sv_interframe[i].modifier = sv_interframe_modifiers[i];
        }
    }

    // If a client id is supplied, we will only reset values for this client
    if(client >= 0) {
        startclient = client;
        endclient = client + 1;
        // Else we reset for every client
    } else {
        startclient = 0;
        endclient = MAX_CLIENTS;
    }

    // Now we will initialize the value for every feature and every client or just one client if a clientid is supplied (else we may get a weird random value from an old memory zone that was not cleaned up)
    for(i = startclient; i < endclient; i++) {
        // Generic features reset, setting default value (you can set a specific default value for a specific feature in the function SV_ExtendedRecordInterframeInitValues() )
        for(j = 0; j < FEATURES_COUNT; j++) {
            sv_interframe[j].value[i] =
                featureDefaultValue; // set the default value for features (preferable NAN - Not A Number)
        }

        // Hack to avoid the first interframe (which is null) from being committed (we don't want the first interframe to be saved, since it contains only null value - the update func will take care of fetching correct values)
        sv_interframeModified[i] = true;
    }
}

/*
===============
idServerOACSSystemLocal::ExtendedRecordInterframeInitValues

Set the initial values for some features, this function will call another one in order to set the correct value (this ease later modifications)
Note: do not use ExtendedRecordSetFeatureValue() here, just access directly sv_interframe (you don't want to commit anything here)
===============
*/
void idServerOACSSystemLocal::ExtendedRecordInterframeInitValues(
    sint client) {
    sint feature;
    playerState_t *ps;

    // Loop through all features and set a default value
    for(feature = 0; feature < FEATURES_COUNT; feature++) {
        // Proceed only if the client is not a bot
        if(!IsBot(client)) {
            sv_interframe[feature].value[client] = ExtendedRecordInterframeInitValue(
                    client, feature);
        }
    }

    // Init the previous player's state values
    ps = serverGameSystem->GameClientNum(client);   // get the player's state
    ::memset(&prev_ps, 0, sizeof(playerState_t));
    ::memcpy(&prev_ps[client], ps, sizeof(playerState_t));
}

/*
===============
idServerOACSSystemLocal::ExtendedRecordInterframeUpdateValues

Set the initial values for some features
This is a simple switch/case in order to ease modifications, just return the value you want for this feature
set here the default values you want for a feature if you want it to be different than 0 or NaN
===============
*/
float64 idServerOACSSystemLocal::ExtendedRecordInterframeInitValue(
    sint client, sint feature) {
    playerState_t *ps;
    ps = serverGameSystem->GameClientNum(client);   // get the player's state

    switch(feature) {
        case FEATURE_PLAYERID:
            // Set unique player id (we want this id to be completely generated serverside and without any means to tamper it clientside) - we don't care that the id change for the same player when he reconnects, since anyway the id will always link to the player's ip and guid using the playerstable
            //valueType tmp[MAX_STRING_CHARS] = ""; snprintf(tmp, MAX_STRING_CHARS, "%i%lu", rand_range(1, 99999), (uint32)time(nullptr));
            return atof(va("%i%lu", rand_range(1, 99999),
                           static_cast<uint32>(::time(
                                                   nullptr))));     // FIXME: use a real UUID/GUID here (for the moment we simply use the timestamp in seconds + a random number, this should be enough for now to ensure the uniqueness of all the players) - do NOT use ioquake3 GUID since it can be spoofed (there's no centralized authorization system!)

        case FEATURE_SVSTIME:
            // Server time (serverStatic_t time, which is always strictly increasing)
            return svs.time;

        case FEATURE_SVTIME:
            // Server time (non-persistant server time, can be used to check whether a new game is starting)
            return sv.time;

        case FEATURE_LASTCOMMANDTIME:
            return ps->commandTime;

        case FEATURE_MOVEMENTDIR:
            return ps->movementDir;

        case FEATURE_SCORE:
        case FEATURE_HITS:
        case FEATURE_DEATH:
        case FEATURE_FRAGS:
        case FEATURE_DAMAGEEVENT_COUNT:
            return 0;

        case FEATURE_HEALTH:
            return ps->stats[STAT_HEALTH];

        case FEATURE_MAX_HEALTH:
            return ps->stats[STAT_MAX_HEALTH];

        case FEATURE_SPEED:
            return 0; // initial speed of the player, not ps->speed which is the theoretical maximum player's speed

        case FEATURE_SPEEDRATIO:
            return 0;

        case FEATURE_DAMAGE_COUNT:
            return 0;

        case FEATURE_FRAMEREPEAT:
            // FrameRepeat: number of times a frame was repeated (1 = one frame, it was not repeated)
            return 1;

        case LABEL_CHEATER:
            // Label: by default, the player is honest. The player is labeled as a cheater only under supervision of the admin, to grow the data file with anomalous examples.
            return 0;

        default:
            return featureDefaultValue;
    }
}

/*
===============
idServerOACSSystemLocal::ExtendedRecordInterframeUpdateValues

Update features for each server frame
===============
*/
void idServerOACSSystemLocal::ExtendedRecordInterframeUpdate(sint client) {
    sint i, startclient, endclient;

    // If a client id is supplied, we will only reset values for this client
    if(client >= 0) {
        startclient = client;
        endclient = client + 1;
        // Else we reset for every client
    } else {
        startclient = 0;
        endclient = sv_maxclients->integer;
    }

    // Now we will initialize the value for every feature and every client or just one client if a clientid is supplied (else we may get a weird random value from an old memory zone that was not cleaned up)
    for(i = startclient; i < endclient; i++) {
        // If the client was disconnected, and not already reinitialized, we commit the last interframe and reset
        if((svs.clients[i].state == CS_ZOMBIE) &&
                !((sv_interframe[FEATURE_PLAYERID].value[i] == featureDefaultValue) |
                  isnan(sv_interframe[FEATURE_PLAYERID].value[i]))) {
            ExtendedRecordDropClient(i);
            // Else if the client is not already fully connected in the game, we just skip this client
        } else if(svs.clients[i].state < CS_ACTIVE) {
            continue;
        } // Note: we do update spectators and players even when below the number of required human players, but we just don't save them (see SV_ExtendedRecordWriteValues()). This allows to always have fresh values (eg: reactiontime won't get too huge).

        // Ok this player is valid, we update the values

        // Update the features' values that are set from the victims
        // Note: this must be done BEFORE the bot check, because we want that victims bots set features of human attackers
        //ExtendedRecordInterframeUpdateValuesAttacker(i);

        // Avoid updating bots
        if(IsBot(i)) {
            continue;
        }

        // Update the features' values
        ExtendedRecordInterframeUpdateValues(i);

        // Check if the interframe is repeated or if it has changed since the previous one
        if(sv_interframeModified[i] ==
                true) {  // it changed, and the previous one was already committed, so we just have to reset FRAMEREPEAT and the modified flag
            ExtendedRecordSetFeatureValue(FEATURE_FRAMEREPEAT, 1, i);
            sv_interframeModified[i] = false;
        } else {
            // Else the interframe is repeated, not changed, thus we increment the FRAMEREPEAT
            sv_interframe[FEATURE_FRAMEREPEAT].value[i]++;
        }

    }

}

/*
===============
idServerOACSSystemLocal::ExtendedRecordInterframeUpdateValues

Set the updated values for all features
you can add here your code to update a feature at the end of every frame and for every player
Note: You need to update all the features! Either here in this function, or elsewhere in the ioquake3 code!
Note2: you should here use SV_ExtendedRecordSetFeatureValue() because you generally want to commit your variables when they are updated
(if not, prefer to use a modifier = false and still use the SetFeatureValue() function).
Note3: the order matters here, so that you can compute a feature only after another feature was computed.
===============
*/
void idServerOACSSystemLocal::ExtendedRecordInterframeUpdateValues(
    sint client) {
    sint attacker;
    playerState_t *ps;
    ps = serverGameSystem->GameClientNum(client);   // get the player's state
    attacker = ps->persistant[PERS_ATTACKER];

    //== Updating normal features' values

    // TIMESTAMP
    ExtendedRecordSetFeatureValue(FEATURE_TIMESTAMP, time(nullptr), client);

    // COMMANDTIME_REACTIONTIME
    // must be set before setting the new lastcommandtime
    ExtendedRecordSetFeatureValue(FEATURE_COMMANDTIME_REACTIONTIME,
                                  ps->commandTime - sv_interframe[FEATURE_LASTCOMMANDTIME].value[client],
                                  client);

    // LASTCOMMANDTIME
    ExtendedRecordSetFeatureValue(FEATURE_LASTCOMMANDTIME, ps->commandTime,
                                  client);

    // ANGLEINAFRAME
    // Compute the total angle in all directions in one frame
    ExtendedRecordSetFeatureValue(FEATURE_ANGLEINAFRAME,
                                  (abs(ps->viewangles[0] - prev_ps[client].viewangles[0]) + abs(
                                       ps->viewangles[1] - prev_ps[client].viewangles[1]) +
                                   abs(ps->viewangles[2] - prev_ps[client].viewangles[2])), client);

    // LASTMOUSEEVENTTIME and MOUSEEVENTTIME_REACTIONTIME
    if(sv_interframe[FEATURE_ANGLEINAFRAME].value[client] > 0) {
        ExtendedRecordSetFeatureValue(FEATURE_MOUSEEVENTTIME_REACTIONTIME,
                                      svs.time -
                                      sv_interframe[FEATURE_LASTMOUSEEVENTTIME].value[client],
                                      client);  // update the mouse reaction time before the last mouse event time
        ExtendedRecordSetFeatureValue(FEATURE_LASTMOUSEEVENTTIME, svs.time,
                                      client);
    }

    // MOVEMENTDIR
    ExtendedRecordSetFeatureValue(FEATURE_MOVEMENTDIR, ps->movementDir,
                                  client);

    // FRAGS and FRAGSINAROW
    // we try to heuristically tell when the current player killed someone when the number of hits
    // and the score both increased (then probably the player killed another player and got his score incremented)
    // Note: this must be done before updating both FEATURE_HITS and FEATURE_SCORE
    // Note2: this also works with bots, but may not be very reliable.
    // FIXME: find a more reliable way to count frags, and which works with bots?
    if((ps->persistant[PERS_HITS] > sv_interframe[FEATURE_HITS].value[client])
            && (ps->persistant[PERS_SCORE] >
                sv_interframe[FEATURE_SCORE].value[client])) {
        ExtendedRecordSetFeatureValue(FEATURE_FRAGS,
                                      sv_interframe[FEATURE_FRAGS].value[client] + 1, client);
        ExtendedRecordSetFeatureValue(FEATURE_FRAGSINAROW,
                                      sv_interframe[FEATURE_FRAGSINAROW].value[client] + 1, client);
    } else {
        // reset the accumulator if no frag in this frame
        ExtendedRecordSetFeatureValue(FEATURE_FRAGSINAROW, 0, client);
    }

    // SCOREACC
    // increment when the value change
    // Note: must be done before updating FEATURE_HITS
    if(ps->persistant[PERS_SCORE] >
            sv_interframe[FEATURE_SCORE].value[client]) {
        ExtendedRecordSetFeatureValue(FEATURE_SCOREACC,
                                      sv_interframe[FEATURE_SCOREACC].value[client] + (ps->persistant[PERS_SCORE]
                                              -
                                              sv_interframe[FEATURE_SCORE].value[client]), client);
    } else { // else we reset the accumulator to 0
        ExtendedRecordSetFeatureValue(FEATURE_SCOREACC, 0, client);
    }

    // SCORE
    ExtendedRecordSetFeatureValue(FEATURE_SCORE, ps->persistant[PERS_SCORE],
                                  client);

    // HITSACC
    // increment when the value change
    // Note: must be done before updating FEATURE_HITS
    if(ps->persistant[PERS_HITS] > sv_interframe[FEATURE_HITS].value[client]) {
        ExtendedRecordSetFeatureValue(FEATURE_HITSACC,
                                      sv_interframe[FEATURE_HITSACC].value[client] + (ps->persistant[PERS_HITS] -
                                              sv_interframe[FEATURE_HITS].value[client]), client);
    } else {
        // else we reset the accumulator to 0
        ExtendedRecordSetFeatureValue(FEATURE_HITSACC, 0, client);
    }

    // HITS
    ExtendedRecordSetFeatureValue(FEATURE_HITS, ps->persistant[PERS_HITS],
                                  client);

    // DEATHACC
    // increment when the value change
    // Note: must be done before updating FEATURE_DEATH
    if(ps->persistant[PERS_KILLED] >
            sv_interframe[FEATURE_DEATH].value[client]) {
        ExtendedRecordSetFeatureValue(FEATURE_DEATHACC,
                                      sv_interframe[FEATURE_DEATHACC].value[client] +
                                      (ps->persistant[PERS_KILLED] - sv_interframe[FEATURE_DEATH].value[client]),
                                      client);
    } else { // else we reset the accumulator to 0
        ExtendedRecordSetFeatureValue(FEATURE_DEATHACC, 0, client);
    }

    // DEATH
    ExtendedRecordSetFeatureValue(FEATURE_DEATH, ps->persistant[PERS_KILLED],
                                  client);

    // DAMAGEEVENT_COUNTACC
    // increment when the value change
    // Note: must be done before updating FEATURE_DAMAGEEVENT_COUNT
    if(ps->damageEvent >
            sv_interframe[FEATURE_DAMAGEEVENT_COUNT].value[client]) {
        ExtendedRecordSetFeatureValue(FEATURE_DAMAGEEVENT_COUNTACC,
                                      sv_interframe[FEATURE_DAMAGEEVENT_COUNTACC].value[client] +
                                      (ps->damageEvent - sv_interframe[FEATURE_DAMAGEEVENT_COUNT].value[client]),
                                      client);
    } else {
        // else we reset the accumulator to 0
        ExtendedRecordSetFeatureValue(FEATURE_DAMAGEEVENT_COUNTACC, 0, client);
    }

    // DAMAGEEVENT_COUNT
    ExtendedRecordSetFeatureValue(FEATURE_DAMAGEEVENT_COUNT, ps->damageEvent,
                                  client);

    // DUCKED
    ExtendedRecordSetFeatureValue(FEATURE_DUCKED,
                                  (ps->pm_flags & PMF_DUCKED) ? 1 : 0, client);

    // MIDAIR
    ExtendedRecordSetFeatureValue(FEATURE_MIDAIR,
                                  ps->groundEntityNum == ENTITYNUM_NONE, client);

    // WEAPON
    ExtendedRecordSetFeatureValue(FEATURE_WEAPON, ps->weapon, client);

    // WEAPONSTATE
    ExtendedRecordSetFeatureValue(FEATURE_WEAPONSTATE, ps->weaponstate,
                                  client);

    // WEAPONINSTANTHIT
    ExtendedRecordSetFeatureValue(FEATURE_WEAPONINSTANTHIT,
                                  IsWeaponInstantHit(ps->weapon) ? 1 : 0, client);

    // HEALTH
    ExtendedRecordSetFeatureValue(FEATURE_HEALTH, ps->stats[STAT_HEALTH],
                                  client);

    // MAX_HEALTH
    ExtendedRecordSetFeatureValue(FEATURE_MAX_HEALTH,
                                  ps->stats[STAT_MAX_HEALTH], client);

    // SPEED
    // Total amount of speed in all directions
    ExtendedRecordSetFeatureValue(FEATURE_SPEED,
                                  (abs(ps->velocity[0]) + abs(ps->velocity[1]) + abs(ps->velocity[2])),
                                  client);

    // SPEEDRATIO
    if(ps->speed > 0) {
        ExtendedRecordSetFeatureValue(FEATURE_SPEEDRATIO,
                                      static_cast<float64>(abs(ps->velocity[0]) +
                                              abs(ps->velocity[1]) + abs(ps->velocity[2])) / ps->speed, client);
    } else {
        ExtendedRecordSetFeatureValue(FEATURE_SPEEDRATIO, 0, client);
    }

    // DAMAGE_COUNT
    ExtendedRecordSetFeatureValue(FEATURE_DAMAGE_COUNT, ps->damageCount,
                                  client);

    // Update special delta features: values that need to be computed only in difference with the previous interframe when there's a change,
    // or after other features
    // Note: you should only update those features at the end, because you WANT to make sure that any change to any modifier feature already happened,
    // so that we know for sure that the interframe will be committed or not.

    // REACTIONTIME
    if(sv_interframeModified[client] == true) {
        // Update reaction time (svs.time delta) only if we have committed the last frame, because we want the difference (the delta) between the
        // last move and the new one
        ExtendedRecordSetFeatureValue(FEATURE_REACTIONTIME,
                                      svs.time - sv_interframe[FEATURE_SVSTIME].value[client], client);

        // SVSTIME
        // We have to update SVSTIME only after reaction time, and we also update it if it doesn't have any value (because we don't want that the first
        // interframe of a player is set to the start of the server/map!)
        ExtendedRecordSetFeatureValue(FEATURE_SVSTIME, svs.time, client);

        // SVTIME
        ExtendedRecordSetFeatureValue(FEATURE_SVTIME, sv.time, client);
    }

    //== Update the previous player's state with the current one (in preparation for the next iteration)
    ::memcpy(&prev_ps[client], ps, sizeof(playerState_t));
}

/*
===============
idServerOACSSystemLocal::ExtendedRecordPlayersTableInit

Update the features of the attacker from the victim
WARNING: only put here features that are set on the attacker, never the victim! Else you may cause a crash (because the victim can be a bot,
but bots don't have interframes)
===============
*/
void idServerOACSSystemLocal::ExtendedRecordInterframeUpdateValuesAttacker(
    sint client) {
    sint attacker;
    playerState_t *ps;
    ps = serverGameSystem->GameClientNum(client);   // get the player's state
    attacker = ps->persistant[PERS_ATTACKER];

    // Check that the attacker is human, else we abort
    if(IsBot(attacker)) {
        return;
    }
}

/*
===============
idServerOACSSystemLocal::ExtendedRecordPlayersTableInit

Setup the values for the players table entry of one client (these are extended identification informations,
useful at prediction to do a post action like kick or ban, or just to report with the proper name and ip)
Edit here if you want to add more identification informations
===============
*/
void idServerOACSSystemLocal::ExtendedRecordPlayersTableInit(sint client) {
    client_t *cl;

    // Get the client object
    cl = &svs.clients[client];

    // Set playerid (the random server-side uuid we set for each player, it should be computed prior calling this function, when initializing the sv_interframe)
    sv_playerstable.playerid = sv_interframe[FEATURE_PLAYERID].value[client];

    // Set IP
    //sv_playerstable.ip = Info_ValueForKey( cl->userinfo, "ip" ); // alternative way to get the ip, from the userinfo string
    Q_strncpyz(sv_playerstable.ip,
               networkSystem->AdrToString(cl->netchan.remoteAddress),
               MAX_STRING_CHARS);   // reliable way to get the client's ip adress
    //snprintf(sv_playerstable.ip, MAX_STRING_CHARS, "%i.%i.%i.%i", cl->netchan.remoteAddress.ip[0], cl->netchan.remoteAddress.ip[1], cl->netchan.remoteAddress.ip[2], cl->netchan.remoteAddress.ip[3]);

    // Set GUID
    sv_playerstable.guid = Info_ValueForKey(cl->userinfo, "cl_guid");

    // Set timestamp
    sv_playerstable.timestamp = time(nullptr);

    // Human readable date time (only for human operators when reviewing the playerstable, else it's totally useless for the cheat detection)
    // Note: this is UTC time
    time_t  utcnow = time(nullptr);
    struct tm tnow = *gmtime(&utcnow);
    strftime(sv_playerstable.datetime, MAX_STRING_CSV, "%Y-%m-%d %H:%M:%S",
             &tnow);

    // Set nickname
    sv_playerstable.nickname = cl->name;
}

/*
===============
idServerOACSSystemLocal::IsSpectator

Loop through an array of feature_t and convert to a CSV row
===============
*/
valueType *idServerOACSSystemLocal::ExtendedRecordFeaturesToCSV(
    valueType *csv_string, sint max_string_size, feature_t *interframe,
    sint savewhat, sint client) {
    sint i;
    // will store the current cursor position at the end of the string, so that we can quickly append without doing a strlen()
    // everytime which would be O(n^2) instead of O(n)
    sint length = 0;

    for(i = 0; i < FEATURES_COUNT; i++) {
        // Add CSV separator between fields
        if(i > 0) {
            length += snprintf(csv_string + length, max_string_size, "%s", ",");
        }

        // Save the fields
        if(savewhat == 0) {  // type 0: save the key (name strings)
            length += snprintf(csv_string + length, max_string_size, "%s",
                               interframe[i].key);
        } else if(savewhat ==
                  1) { // type 1: save the type (int coming from an enum)
            //s = strncat_lin( s, va("%i", interframe[i].type), (sizeof(csv_string) - sizeof(s)) );
            length += snprintf(csv_string + length, max_string_size, "%i",
                               interframe[i].type);
        } else { // type 2: save the values (the data type depends on the content of the field)
            // If possible, print the value as a long int, else as a float
            if(round(interframe[i].value[client]) == interframe[i].value[client]) {
                length += snprintf(csv_string + length, max_string_size, "%.0f",
                                   interframe[i].value[client]);
            } else {
                length += snprintf(csv_string + length, max_string_size, "%f",
                                   interframe[i].value[client]);
            }
        }

    }

    return csv_string;
}

/*
===============
idServerOACSSystemLocal::IsSpectator

Convert a playerstable_t entry into a CSV string
===============
*/
valueType *idServerOACSSystemLocal::ExtendedRecordPlayersTableToCSV(
    valueType *csv_string, sint max_string_size, playerstable_t playerstable) {
    // willl store the current cursor position at the end of the string, so that we can quickly append without doing a strlen()
    // everytime which would be O(n^2) instead of O(n)
    sint length = 0;

    // append playerid
    length += snprintf(csv_string + length, max_string_size, "%.0f",
                       playerstable.playerid);

    // append ip
    length += snprintf(csv_string + length, max_string_size, "%s",
                       ",");   // append a comma
    length += snprintf(csv_string + length, max_string_size, "\"%s\"",
                       playerstable.ip);

    // append guid
    length += snprintf(csv_string + length, max_string_size, "%s", ",");
    length += snprintf(csv_string + length, max_string_size, "\"%s\"",
                       playerstable.guid);

    // append timestamp
    length += snprintf(csv_string + length, max_string_size, "%s", ",");
    length += snprintf(csv_string + length, max_string_size, "%0.f",
                       playerstable.timestamp);

    // append human-readable date time
    length += snprintf(csv_string + length, max_string_size, "%s", ",");
    length += snprintf(csv_string + length, max_string_size, "\"%s\"",
                       playerstable.datetime);

    // append nick name
    length += snprintf(csv_string + length, max_string_size, "%s", ",");

    // wrap inside quotes to avoid breaking if there are spaces or special characters in the nickname (quotes are filtered by the engine anyway,
    // since the engine also use quotes to wrap variables content)
    length += snprintf(csv_string + length, max_string_size, "\"%s\"",
                       playerstable.nickname);

    return csv_string;
}

/*
===============
idServerOACSSystemLocal::IsSpectator

Set the value of a feature in the current interframe for a given player, and commit the previous interframe for this client if the feature is a modifier
Note: you should use this function whenever you want to modify the value of a feature for a client, because it will take care of writing down the
interframes values whenever needed (else you may lose the data of your interframes!), because the function to commit the features values is only called from here.
===============
*/
void idServerOACSSystemLocal::ExtendedRecordSetFeatureValue(
    interframeIndex_t feature, float64 value, sint client) {
    // If the value has changed (or the old one is NaN), we do something, else we just keep it like that
    // FIXME: maybe compare the delta (diff) below a certain threshold for floats, eg: if (fabs(a - b) < SOME_DELTA)
    if((sv_interframe[feature].value[client] != value) | isnan(
                sv_interframe[feature].value[client])) {
        // If this feature is a modifier, and the interframe wasn't already modified in the current frame, we switch the modified flag and
        // commit the previous interframe (else if it was already modified, we wait until all features modifications take place and we'll see at the next frame)
        if(sv_interframe[feature].modifier == true &&
                sv_interframeModified[client] != true) {
            // Set the modified flag to true
            sv_interframeModified[client] = true;

            // Flush/Commit/Write down the previous interframe since it will be modified
            ExtendedRecordWriteValues(client);
        }

        // Modify the value (after having committed the previous interframe if this feature is a modifier)
        sv_interframe[feature].value[client] = value;
    }
}

/*
===============
idServerOACSSystemLocal::IsSpectator

Check if a client is a bot
===============
*/
bool idServerOACSSystemLocal::IsBot(sint client) {
    sharedEntity_t *entity;
    entity = serverGameSystem->GentityNum(
                 client);   // Get entity (of this player) object

    // Proceed only if the client is not a bot
    if((entity->r.svFlags & SVF_BOT) |
            (svs.clients[client].netchan.remoteAddress.type == NA_BOT)) {
        return true;
    } else {
        return false;
    }
}

/*
===============
idServerOACSSystemLocal::IsSpectator

Check if a player is spectating
===============
*/
bool idServerOACSSystemLocal::IsSpectator(sint client) {
    // WARNING: sv.configstrings[CS_PLAYERS + client] is NOT the same as cl->userinfo, they don't contain the same infos!

    sint team;
    valueType team_s[MAX_STRING_CHARS];
    client_t *cl;
    cl = &svs.clients[client]; // Get client object

    team = atoi(Info_ValueForKey(sv.configstrings[CS_PLAYERS + client].s,
                                 "t"));   // Get the team
    Q_strncpyz(team_s, Info_ValueForKey(cl->userinfo, "team"),
               MAX_STRING_CHARS);  // Get the team from another cvar which is formatted differently

    if(!Q_stricmp(team_s, "spectator") | !Q_stricmp(team_s, "s")) {
        return true;
    } else {
        return false;
    }
}

/*
===============
idServerOACSSystemLocal::IsWeaponInstantHit

Check whether a given weapon (from weapon_t enum type) is an instant-hit long range weapon
===============
*/
bool idServerOACSSystemLocal::IsWeaponInstantHit(sint weapon) {
    switch(weapon) {
        case WP_NONE:
            return false;
            break;

        case WP_BLASTER:
            return false;
            break;

        case WP_MACHINEGUN:
            return true;
            break;

        case WP_PAIN_SAW:
            return false;
            break;

        case WP_SHOTGUN:
            return true;
            break;

        case WP_LAS_GUN:
            return false;
            break;

        case WP_MASS_DRIVER:
            return true;
            break;

        case WP_CHAINGUN:
            return true;
            break;

        case WP_PULSE_RIFLE:
            return false;
            break;

        case WP_FLAMER:
            return true;
            break;

        case WP_LUCIFER_CANNON:
            return false;
            break;

        case WP_GRENADE:
            return true;
            break;

        //Dushan - I have no idea what is this
        case WP_LOCKBLOB_LAUNCHER:
            return false;
            break;

        default:
            return false;
            break;
    }
}

/*
===============
idServerOACSSystemLocal::CountPlayers

Count the number of connected players
===============
*/
sint idServerOACSSystemLocal::CountPlayers(void) {
    sint i, count;

    count = 0;

    for(i = 0; i < sv_maxclients->integer; i++) {
        // If the slot is empty, we skip
        if(svs.clients[i].state < CS_CONNECTED) {
            continue;
            // or if it's a bot, we also skip
        } else if(IsBot(i)) {
            continue;
        }

        // else, it's a human player, we add him to the count
        count++;
    }

    return count;
}


/*
===============
idServerOACSSystemLocal::rand_range

Return a random number between min and max, with more variability than a simple (rand() % (max-min)) + min
===============
*/
sint idServerOACSSystemLocal::rand_range(sint min, sint max) {
    sint retval;

    retval = (static_cast<float64>(rand()) / static_cast<float64>(RAND_MAX) *
              (max - min + 1)) + min;

    return retval;
}

/*
===============
idServerOACSSystemLocal::ExtendedRecordSetCheater_f

Set the cheater label for a client
===============
*/
void idServerOACSSystemLocal::ExtendedRecordSetCheater(sint client,
        sint label) {
    // Checking for sane values
    if((client < 0) || (client > sv_maxclients->integer)) {
        Com_Printf("OACS: idServerOACSSystemLocal::ExtendedRecordSetCheater() Invalid arguments\n");
        return;
    }

    if(label < 0) {
        Com_Printf("Label for player %i: %.0f\n", client,
                   sv_interframe[LABEL_CHEATER].value[client]);
    } else {
        // Set the label for this client
        ExtendedRecordSetFeatureValue(LABEL_CHEATER, label, client);
    }
}

/*
===============
idServerOACSSystemLocal::ExtendedRecordSetCheater_f

Server-side command to set a client's cheater label
cheater <client> <label> where label is 0 for honest players, and >= 1 for cheaters
===============
*/
void idServerOACSSystemLocal::ExtendedRecordSetCheater_f(void) {
    // Need at least one argument to proceed (the first argument is the command, so min = 2)
    if(cmdSystem->Argc() < 2) {
        Com_Printf("Invalid arguments. Usage: cheater <client> [label] where label is 0 for honest players, and >= 1 for cheaters. If no label, will show the current value for the player.\n");
        return;
    }

    // If only one argument is given
    if(cmdSystem->Argc() == 2) {
        // Show the cheater label for the specified client
        ExtendedRecordSetCheater(atoi(cmdSystem->Argv(1)), -1);
    } else {
        // Set cheater label
        ExtendedRecordSetCheater(atoi(cmdSystem->Argv(1)),
                                 atoi(cmdSystem->Argv(2)));
    }

}

/*
===============
idServerOACSSystemLocal::ExtendedRecordSetHonestFromClient_f

Client-side command to declare being a cheater

Note: client needs a password to use this command
Note2: the client can optionally set the value of the label >= 1, this will then represent a kind of cheat
===============
*/
void idServerOACSSystemLocal::ExtendedRecordSetCheaterFromClient_f(
    client_t *cl) {
    sint client;

    // Get the client id
    client = cl - svs.clients;

    // need at least the password
    if(cmdSystem->Argc() < 2) {
        return;
    }

    if(!strlen(sv_oacsLabelPassword->string) ||
            Q_stricmp(cmdSystem->Argv(1), sv_oacsLabelPassword->string)) {
        Com_Printf("OACS: Client tried to declare being cheater: Bad label password from %s\n",
                   networkSystem->AdrToString(cl->netchan.remoteAddress));
        return;
    }

    // type of cheat was given
    if(cmdSystem->Argc() >= 3) {
        if(atoi(cmdSystem->Argv(2)) >
                0) {     // accept only if it's a cheat type (0 == honest, >= 1 == cheat)
            Com_Printf("OACS: Client %i declares being cheater with label %i\n",
                       client, atoi(cmdSystem->Argv(2)));
            ExtendedRecordSetCheater(client, atoi(cmdSystem->Argv(2)));
        } else {
            Com_Printf("OACS: Client tried to declare being cheater: Bad cheat label from %s: %i\n",
                       networkSystem->AdrToString(cl->netchan.remoteAddress),
                       atoi(cmdSystem->Argv(2)));
        }
    } else {
        Com_Printf("OACS: Client %i declares being cheater with label 1\n",
                   client);
        ExtendedRecordSetCheater(client, 1);
    }
}

/*
===============
idServerOACSSystemLocal::ExtendedRecordSetHonestFromClient_f

Client-side command to declare being a honest player
Note: client needs a password to use this command
===============
*/
void idServerOACSSystemLocal::ExtendedRecordSetHonestFromClient_f(
    client_t *cl) {
    sint client;

    // Get the client id
    client = cl - svs.clients;

    // need at least the password
    if(cmdSystem->Argc() < 2) {
        return;
    }

    if(!strlen(sv_oacsLabelPassword->string) ||
            Q_stricmp(cmdSystem->Argv(1), sv_oacsLabelPassword->string)) {
        Com_Printf("OACS: Client tried to declare being honest: Bad label password from %s\n",
                   networkSystem->AdrToString(cl->netchan.remoteAddress));
        return;
    }

    Com_Printf("OACS: Client %i declares being honest\n", client);
    ExtendedRecordSetCheater(client, 0);
}
