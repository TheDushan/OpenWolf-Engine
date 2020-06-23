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
// File name:   cm_load.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.h>
#elif DEDICATED
#include <null/null_serverprecompiled.h>
#else
#include <framework/precompiled.h>
#endif

idCollisionModelManagerLocal collisionModelManagerLocal;
idCollisionModelManager* collisionModelManager = &collisionModelManagerLocal;

#ifdef BSPC

#include "../../tools/bspc/l_qfiles.h"

/*
=================
SetPlaneSignbits
=================
*/
void SetPlaneSignbits( cplane_t* out )
{
    sint bits, j;
    
    // for fast box on planeside test
    bits = 0;
    for( j = 0; j < 3; j++ )
    {
        if( out->normal[j] < 0 )
        {
            bits |= 1 << j;
        }
    }
    out->signbits = bits;
}
#endif //BSPC

// to allow boxes to be treated as brush models, we allocate
// some extra indexes along with those needed by the map
#define BOX_LEAF_BRUSHES    1	// ydnar
#define BOX_BRUSHES     1
#define BOX_SIDES       6
#define BOX_LEAFS       2
#define BOX_PLANES      12

#define LL( x ) x = LittleLong( x )


clipMap_t       cm;
sint             c_pointcontents, c_traces, c_brush_traces, c_patch_traces, c_trisoup_traces;


uchar8*           cmod_base;

#ifndef BSPC
convar_t*         cm_noAreas;
convar_t*         cm_noCurves;
convar_t*         cm_forceTriangles;
convar_t*         cm_playerCurveClip;
convar_t*         cm_optimize;
convar_t*         cm_showCurves;
convar_t*         cm_showTriangles;
#endif

cmodel_t        box_model;
cplane_t*       box_planes;
cbrush_t*       box_brush;



void            CM_InitBoxHull( void );
void            CM_FloodAreaConnections( void );


/*
===============================================================================
MAP LOADING
===============================================================================
*/

bool StringsContainWord( pointer heystack, pointer heystack2, valueType* needle )
{
    if( StringContainsWord( heystack, needle ) )
    {
        return true;
    }
    
    if( StringContainsWord( heystack2, needle ) )
    {
        return true;
    }
    
    return false;
}

sint GetMaterialType( pointer name )
{
    //CL_RefPrintf(PRINT_WARNING, "Check material type for %s.\n", name);
    
    //
    // Special cases - where we are pretty sure we want lots of specular and reflection...
    //
    if( StringsContainWord( name, name, "plastic" ) )
        return MATERIAL_PLASTIC;
    else if( StringsContainWord( name, name, "mp/flag" ) || StringsContainWord( name, name, "lightground" ) )
        return MATERIAL_SOLIDMETAL;
    else if( !StringsContainWord( name, name, "trainer" ) && StringsContainWord( name, name, "train" ) )
        return MATERIAL_SOLIDMETAL;
    else if( StringsContainWord( name, name, "reborn" ) )
        return MATERIAL_ARMOR;
    else if( StringsContainWord( name, name, "textures/common/water" ) )
        return MATERIAL_WATER;
    else if( StringsContainWord( name, name, "grass" ) || StringsContainWord( name, name, "foliage" ) )
        return MATERIAL_SHORTGRASS;
    else if( StringsContainWord( name, name, "concrete" ) )
        return MATERIAL_CONCRETE;
    else if( StringsContainWord( name, name, "models/weapon" ) )
        return MATERIAL_HOLLOWMETAL;
    else if( StringsContainWord( name, name, "/weapon" ) || StringsContainWord( name, name, "scope" ) || StringsContainWord( name, name, "pistol" ) || StringsContainWord( name, name, "cannon" )  || StringsContainWord( name, name, "rifle" ) || StringsContainWord( name, name, "rocket" ) )
        return MATERIAL_HOLLOWMETAL;
    else if( StringsContainWord( name, name, "metal" ) || StringsContainWord( name, name, "pipe" ) || StringsContainWord( name, name, "shaft" ) || StringsContainWord( name, name, "elevator" ) || StringsContainWord( name, name, "vent" ) )
        return MATERIAL_SOLIDMETAL;
    else if( StringsContainWord( name, name, "eye" ) )
        return MATERIAL_GLASS;
    else if( StringsContainWord( name, name, "sand" ) )
        return MATERIAL_SAND;
    else if( StringsContainWord( name, name, "gravel" ) )
        return MATERIAL_GRAVEL;
    else if( StringsContainWord( name, name, "dirt" ) || StringsContainWord( name, name, "ground" ) )
        return MATERIAL_DIRT;
    else if( StringsContainWord( name, name, "snow" ) )
        return MATERIAL_SNOW;
    else if( StringsContainWord( name, name, "hood" ) || StringsContainWord( name, name, "cloth" ) || StringsContainWord( name, name, "pants" ) )
        return MATERIAL_FABRIC;
    else if( StringsContainWord( name, name, "hair" ) )
        return MATERIAL_FABRIC;//MATERIAL_CARPET; Just because it has a bit of parallax and suitable specular...
    else if( StringsContainWord( name, name, "armor" ) || StringsContainWord( name, name, "armour" ) )
        return MATERIAL_ARMOR;
    else if( StringsContainWord( name, name, "flesh" ) || StringsContainWord( name, name, "body" ) || StringsContainWord( name, name, "leg" ) || StringsContainWord( name, name, "hand" ) || StringsContainWord( name, name, "head" ) || StringsContainWord( name, name, "hips" ) || StringsContainWord( name, name, "torso" ) || StringsContainWord( name, name, "tentacles" ) || StringsContainWord( name, name, "face" ) || StringsContainWord( name, name, "arms" ) )
        return MATERIAL_FLESH;
    else if( StringsContainWord( name, name, "players" ) | StringsContainWord( name, name, "boots" ) || StringsContainWord( name, name, "accesories" ) || StringsContainWord( name, name, "accessories" ) || StringsContainWord( name, name, "holster" ) )
        return MATERIAL_FABRIC;
    else if( StringsContainWord( name, name, "canvas" ) )
        return MATERIAL_CANVAS;
    else if( StringsContainWord( name, name, "rock" ) )
        return MATERIAL_ROCK;
    else if( StringsContainWord( name, name, "rubber" ) )
        return MATERIAL_RUBBER;
    else if( StringsContainWord( name, name, "carpet" ) )
        return MATERIAL_CARPET;
    else if( StringsContainWord( name, name, "plaster" ) )
        return MATERIAL_PLASTER;
    else if( StringsContainWord( name, name, "computer" ) || StringsContainWord( name, name, "console" ) || StringsContainWord( name, name, "button" ) || StringsContainWord( name, name, "terminal" ) || StringsContainWord( name, name, "switch" ) || StringsContainWord( name, name, "panel" ) || StringsContainWord( name, name, "control" ) )
        return MATERIAL_COMPUTER;
    else if( StringsContainWord( name, name, "fabric" ) )
        return MATERIAL_FABRIC;
    else if( StringsContainWord( name, name, "leaf" ) || StringsContainWord( name, name, "leaves" ) || StringsContainWord( name, name, "fern" ) || StringsContainWord( name, name, "vine" ) )
        return MATERIAL_GREENLEAVES;
    else if( StringsContainWord( name, name, "wood" ) || ( StringsContainWord( name, name, "tree" ) && !StringsContainWord( name, name, "street" ) ) )
        return MATERIAL_SOLIDWOOD;
    else if( StringsContainWord( name, name, "mud" ) )
        return MATERIAL_MUD;
    else if( StringsContainWord( name, name, "ice" ) )
        return MATERIAL_ICE;
    else if( ( StringsContainWord( name, name, "grass" ) || StringsContainWord( name, name, "foliage" ) ) && ( StringsContainWord( name, name, "long" ) || StringsContainWord( name, name, "tall" ) || StringsContainWord( name, name, "thick" ) ) )
        return MATERIAL_LONGGRASS;
    else if( StringsContainWord( name, name, "grass" ) || StringsContainWord( name, name, "foliage" ) )
        return MATERIAL_SHORTGRASS;
    else if( StringsContainWord( name, name, "floor" ) )
        return MATERIAL_TILES;
    else if( !StringsContainWord( name, name, "_cc" ) )
        return MATERIAL_MARBLE;
    else if( !StringsContainWord( name, name, "players" ) )
        return MATERIAL_TILES;
    else if( StringsContainWord( name, name, "floor" ) )
        return MATERIAL_CONCRETE;
    else if( StringsContainWord( name, name, "frame" ) )
        return MATERIAL_SOLIDMETAL;
    else if( StringsContainWord( name, name, "wall" ) )
        return MATERIAL_SOLIDMETAL;
    else if( StringsContainWord( name, name, "wall" ) )
        return MATERIAL_CONCRETE;
    else if( StringsContainWord( name, name, "door" ) )
        return MATERIAL_SOLIDMETAL;
    else if( StringsContainWord( name, name, "door" ) )
        return MATERIAL_CONCRETE;
    else if( StringsContainWord( name, name, "ground" ) )
        return MATERIAL_TILES;
    else if( StringsContainWord( name, name, "ground" ) )
        return MATERIAL_CONCRETE;
    else if( StringsContainWord( name, name, "desert" ) )
        return MATERIAL_CONCRETE;
    else if( ( StringsContainWord( name, name, "tile" ) || StringsContainWord( name, name, "lift" ) ) )
        return MATERIAL_SOLIDMETAL;
    else if( StringsContainWord( name, name, "tile" ) || StringsContainWord( name, name, "lift" ) )
        return MATERIAL_TILES;
    else if( StringsContainWord( name, name, "glass" ) || StringsContainWord( name, name, "light" ) || StringsContainWord( name, name, "screen" ) || StringsContainWord( name, name, "lamp" ) )
        return MATERIAL_GLASS;
    else if( StringsContainWord( name, name, "flag" ) )
        return MATERIAL_FABRIC;
    else if( StringsContainWord( name, name, "column" ) || StringsContainWord( name, name, "stone" ) || StringsContainWord( name, name, "statue" ) )
        return MATERIAL_MARBLE;
    else if( StringsContainWord( name, name, "red" ) || StringsContainWord( name, name, "blue" ) || StringsContainWord( name, name, "yellow" ) || StringsContainWord( name, name, "white" ) || StringsContainWord( name, name, "monitor" ) )
        return MATERIAL_PLASTIC;
    else if( StringsContainWord( name, name, "trim" ) || StringsContainWord( name, name, "step" ) || StringsContainWord( name, name, "pad" ) )
        return MATERIAL_ROCK;
    else if( !StringsContainWord( name, name, "players" ) )
        return MATERIAL_TILES;
    else if( !StringsContainWord( name, name, "players" ) )
        return MATERIAL_MARBLE;
    else if( StringsContainWord( name, name, "outside" ) )
        return MATERIAL_CONCRETE;
    else if( StringsContainWord( name, name, "out" ) && ( StringsContainWord( name, name, "trim" ) || StringsContainWord( name, name, "step" ) || StringsContainWord( name, name, "pad" ) ) )
        return MATERIAL_CONCRETE;
    else if( StringsContainWord( name, name, "out" ) && ( StringsContainWord( name, name, "frame" ) || StringsContainWord( name, name, "wall" ) || StringsContainWord( name, name, "round" ) || StringsContainWord( name, name, "crate" ) || StringsContainWord( name, name, "trim" ) || StringsContainWord( name, name, "support" ) || StringsContainWord( name, name, "step" ) || StringsContainWord( name, name, "pad" ) || StringsContainWord( name, name, "weapon" ) || StringsContainWord( name, name, "gun" ) ) )
        return MATERIAL_CONCRETE;
    else if( StringsContainWord( name, name, "frame" ) || StringsContainWord( name, name, "wall" ) ||  StringsContainWord( name, name, "round" ) || StringsContainWord( name, name, "crate" ) ||  StringsContainWord( name, name, "trim" ) || StringsContainWord( name, name, "support" ) ||  StringsContainWord( name, name, "step" ) || StringsContainWord( name, name, "pad" ) || StringsContainWord( name, name, "weapon" ) || StringsContainWord( name, name, "gun" ) )
        return MATERIAL_CONCRETE;
    else if( StringsContainWord( name, name, "black" ) || StringsContainWord( name, name, "items" ) )
        return MATERIAL_PLASTIC;
    else if( StringsContainWord( name, name, "refract" ) || StringsContainWord( name, name, "reflect" ) )
        return MATERIAL_PLASTIC;
    else if( StringsContainWord( name, name, "map_objects" ) )
        return MATERIAL_SOLIDMETAL; // hmmm, maybe... testing...
        
    //
    // Special cases - where we are pretty sure we want lots of specular and reflection... Override!
    //
    if( StringsContainWord( name, name, "plastic" ) )
        return MATERIAL_PLASTIC;
    else if( StringsContainWord( name, name, "mp/flag" ) || StringsContainWord( name, name, "transport" ) || StringsContainWord( name, name, "crate" ) || StringsContainWord( name, name, "container" ) || StringsContainWord( name, name, "barrel" ) || StringsContainWord( name, name, "train" ) || StringsContainWord( name, name, "crane" ) || StringsContainWord( name, name, "plate" ) || StringsContainWord( name, name, "cargo" ) )
        return MATERIAL_PLASTIC;
    else if( StringsContainWord( name, name, "grass" ) || StringsContainWord( name, name, "foliage" ) )
        return MATERIAL_SHORTGRASS;
    if( StringsContainWord( name, name, "plastic" ) || StringsContainWord( name, name, "medpack" ) )
        return MATERIAL_PLASTIC;
    else if( StringsContainWord( name, name, "water" ) && !StringsContainWord( name, name, "splash" ) && !StringsContainWord( name, name, "drip" ) )
        return MATERIAL_WATER;
        
    return MATERIAL_TILES;
}

bool HaveSurfaceType( sint surfaceFlags )
{
    switch( surfaceFlags & MATERIAL_MASK )
    {
        case MATERIAL_WATER:			// 13			// light covering of water on a surface
        case MATERIAL_SHORTGRASS:		// 5			// manicured lawn
        case MATERIAL_LONGGRASS:		// 6			// long jungle grass
        case MATERIAL_SAND:				// 8			// sandy beach
        case MATERIAL_CARPET:			// 27			// lush carpet
        case MATERIAL_GRAVEL:			// 9			// lots of small stones
        case MATERIAL_ROCK:				// 23			//
        case MATERIAL_TILES:			// 26			// tiled floor
        case MATERIAL_SOLIDWOOD:		// 1			// freshly cut timber
        case MATERIAL_HOLLOWWOOD:		// 2			// termite infested creaky wood
        case MATERIAL_SOLIDMETAL:		// 3			// solid girders
        case MATERIAL_HOLLOWMETAL:		// 4			// hollow metal machines
        case MATERIAL_DRYLEAVES:		// 19			// dried up leaves on the floor
        case MATERIAL_GREENLEAVES:		// 20			// fresh leaves still on a tree
        case MATERIAL_FABRIC:			// 21			// Cotton sheets
        case MATERIAL_CANVAS:			// 22			// tent material
        case MATERIAL_MARBLE:			// 12			// marble floors
        case MATERIAL_SNOW:				// 14			// freshly laid snow
        case MATERIAL_MUD:				// 17			// wet soil
        case MATERIAL_DIRT:				// 7			// hard mud
        case MATERIAL_CONCRETE:			// 11			// hardened concrete pavement
        case MATERIAL_FLESH:			// 16			// hung meat, corpses in the world
        case MATERIAL_RUBBER:			// 24			// hard tire like rubber
        case MATERIAL_PLASTIC:			// 25			//
        case MATERIAL_PLASTER:			// 28			// drywall style plaster
        case MATERIAL_SHATTERGLASS:		// 29			// glass with the Crisis Zone style shattering
        case MATERIAL_ARMOR:			// 30			// body armor
        case MATERIAL_ICE:				// 15			// packed snow/solid ice
        case MATERIAL_GLASS:			// 10			//
        case MATERIAL_BPGLASS:			// 18			// bulletproof glass
        case MATERIAL_COMPUTER:			// 31			// computers/electronic equipment
            return true;
            break;
        default:
            break;
    }
    
    return false;
}

/*
=================
CMod_LoadShaders
=================
*/
void CMod_LoadShaders( lump_t* l )
{
    dshader_t*      in, *out;
    sint             i, count;
    
    in = ( dshader_t* )( cmod_base + l->fileofs );
    if( l->filelen % sizeof( *in ) )
    {
        Com_Error( ERR_DROP, "CMod_LoadShaders: funny lump size" );
    }
    count = l->filelen / sizeof( *in );
    
    if( count < 1 )
    {
        Com_Error( ERR_DROP, "Map with no shaders" );
    }
    cm.shaders = ( dshader_t* )Hunk_Alloc( count * sizeof( *cm.shaders ), h_high );
    cm.numShaders = count;
    
    ::memcpy( cm.shaders, in, count * sizeof( *cm.shaders ) );
    
    if( LittleLong( 1 ) != 1 )
    {
        out = cm.shaders;
        for( i = 0; i < count; i++, in++, out++ )
        {
            Q_strncpyz( out->shader, in->shader, MAX_QPATH );
            out->contentFlags = LittleLong( out->contentFlags );
            out->surfaceFlags = LittleLong( out->surfaceFlags );
            
            if( !HaveSurfaceType( out->surfaceFlags ) )
            {
                out->surfaceFlags = LittleLong( GetMaterialType( in->shader ) );
            }
        }
    }
}


/*
=================
CMod_LoadSubmodels
=================
*/
void CMod_LoadSubmodels( lump_t* l )
{
    dmodel_t*       in;
    cmodel_t*       out;
    sint             i, j, count, *indexes;
    
    in = ( dmodel_t* )( cmod_base + l->fileofs );
    if( l->filelen % sizeof( *in ) )
    {
        Com_Error( ERR_DROP, "CMod_LoadSubmodels: funny lump size" );
    }
    count = l->filelen / sizeof( *in );
    
    if( count < 1 )
    {
        Com_Error( ERR_DROP, "Map with no models" );
    }
    cm.cmodels = ( cmodel_t* )Hunk_Alloc( count * sizeof( *cm.cmodels ), h_high );
    cm.numSubModels = count;
    
    if( count > MAX_SUBMODELS )
    {
        Com_Error( ERR_DROP, "MAX_SUBMODELS exceeded" );
    }
    
    for( i = 0; i < count; i++, in++, out++ )
    {
        out = &cm.cmodels[i];
        
        for( j = 0; j < 3; j++ ) // spread the mins / maxs by a pixel
        {
            out->mins[j] = LittleFloat( in->mins[j] ) - 1;
            out->maxs[j] = LittleFloat( in->maxs[j] ) + 1;
        }
        
        if( i == 0 )
        {
            continue; // world model doesn't need other info
        }
        
        // make a "leaf" just to hold the model's brushes and surfaces
        out->leaf.numLeafBrushes = LittleLong( in->numBrushes );
        indexes = ( sint* )Hunk_Alloc( out->leaf.numLeafBrushes * 4, h_high );
        out->leaf.firstLeafBrush = indexes - cm.leafbrushes;
        for( j = 0; j < out->leaf.numLeafBrushes; j++ )
        {
            indexes[j] = LittleLong( in->firstBrush ) + j;
        }
        
        out->leaf.numLeafSurfaces = LittleLong( in->numSurfaces );
        indexes = ( sint* )Hunk_Alloc( out->leaf.numLeafSurfaces * 4, h_high );
        out->leaf.firstLeafSurface = indexes - cm.leafsurfaces;
        for( j = 0; j < out->leaf.numLeafSurfaces; j++ )
        {
            indexes[j] = LittleLong( in->firstSurface ) + j;
        }
    }
}


/*
=================
CMod_LoadNodes
=================
*/
void CMod_LoadNodes( lump_t* l )
{
    dnode_t*        in;
    sint             child, i, j, count;
    cNode_t*        out;
    
    in = ( dnode_t* )( cmod_base + l->fileofs );
    if( l->filelen % sizeof( *in ) )
    {
        Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
    }
    count = l->filelen / sizeof( *in );
    
    if( count < 1 )
    {
        Com_Error( ERR_DROP, "Map has no nodes" );
    }
    cm.nodes = ( cNode_t* )Hunk_Alloc( count * sizeof( *cm.nodes ), h_high );
    cm.numNodes = count;
    
    out = cm.nodes;
    
    for( i = 0; i < count; i++, out++, in++ )
    {
        out->plane = cm.planes + LittleLong( in->planeNum );
        for( j = 0; j < 2; j++ )
        {
            child = LittleLong( in->children[j] );
            out->children[j] = child;
        }
    }
    
}

/*
=================
CM_BoundBrush
=================
*/
void CM_BoundBrush( cbrush_t* b )
{
    b->bounds[0][0] = -b->sides[0].plane->dist;
    b->bounds[1][0] = b->sides[1].plane->dist;
    
    b->bounds[0][1] = -b->sides[2].plane->dist;
    b->bounds[1][1] = b->sides[3].plane->dist;
    
    b->bounds[0][2] = -b->sides[4].plane->dist;
    b->bounds[1][2] = b->sides[5].plane->dist;
}


/*
=================
CMod_LoadBrushes
=================
*/
void CMod_LoadBrushes( lump_t* l )
{
    dbrush_t*       in;
    cbrush_t*       out;
    sint             i, count;
    
    in = ( dbrush_t* )( cmod_base + l->fileofs );
    if( l->filelen % sizeof( *in ) )
    {
        Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
    }
    count = l->filelen / sizeof( *in );
    
    cm.brushes = ( cbrush_t* )Hunk_Alloc( ( BOX_BRUSHES + count ) * sizeof( *cm.brushes ), h_high );
    cm.numBrushes = count;
    
    out = cm.brushes;
    
    for( i = 0; i < count; i++, out++, in++ )
    {
        out->sides = cm.brushsides + LittleLong( in->firstSide );
        out->numsides = LittleLong( in->numSides );
        
        out->shaderNum = LittleLong( in->shaderNum );
        if( out->shaderNum < 0 || out->shaderNum >= cm.numShaders )
        {
            Com_Error( ERR_DROP, "CMod_LoadBrushes: bad shaderNum: %i", out->shaderNum );
        }
        out->contents = cm.shaders[out->shaderNum].contentFlags;
        
        CM_BoundBrush( out );
    }
    
}

/*
=================
CMod_LoadLeafs
=================
*/
void CMod_LoadLeafs( lump_t* l )
{
    sint             i, count;
    cLeaf_t*        out;
    dleaf_t*        in;
    
    in = ( dleaf_t* )( cmod_base + l->fileofs );
    if( l->filelen % sizeof( *in ) )
    {
        Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
    }
    count = l->filelen / sizeof( *in );
    
    if( count < 1 )
    {
        Com_Error( ERR_DROP, "Map with no leafs" );
    }
    
    cm.leafs = ( cLeaf_t* )Hunk_Alloc( ( BOX_LEAFS + count ) * sizeof( *cm.leafs ), h_high );
    cm.numLeafs = count;
    
    out = cm.leafs;
    for( i = 0; i < count; i++, in++, out++ )
    {
        out->cluster = LittleLong( in->cluster );
        out->area = LittleLong( in->area );
        out->firstLeafBrush = LittleLong( in->firstLeafBrush );
        out->numLeafBrushes = LittleLong( in->numLeafBrushes );
        out->firstLeafSurface = LittleLong( in->firstLeafSurface );
        out->numLeafSurfaces = LittleLong( in->numLeafSurfaces );
        
        if( out->cluster >= cm.numClusters )
        {
            cm.numClusters = out->cluster + 1;
        }
        if( out->area >= cm.numAreas )
        {
            cm.numAreas = out->area + 1;
        }
    }
    
    cm.areas = ( cArea_t* )Hunk_Alloc( cm.numAreas * sizeof( *cm.areas ), h_high );
    cm.areaPortals = ( sint* )Hunk_Alloc( cm.numAreas * cm.numAreas * sizeof( *cm.areaPortals ), h_high );
}

/*
=================
CMod_LoadPlanes
=================
*/
void CMod_LoadPlanes( lump_t* l )
{
    sint             i, j, count, bits;
    cplane_t*       out;
    dplane_t*       in;
    
    in = ( dplane_t* )( cmod_base + l->fileofs );
    if( l->filelen % sizeof( *in ) )
    {
        Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
    }
    count = l->filelen / sizeof( *in );
    
    if( count < 1 )
    {
        Com_Error( ERR_DROP, "Map with no planes" );
    }
    cm.planes = ( cplane_t* )Hunk_Alloc( ( BOX_PLANES + count ) * sizeof( *cm.planes ), h_high );
    cm.numPlanes = count;
    
    out = cm.planes;
    
    for( i = 0; i < count; i++, in++, out++ )
    {
        bits = 0;
        for( j = 0; j < 3; j++ )
        {
            out->normal[j] = LittleFloat( in->normal[j] );
            if( out->normal[j] < 0 )
            {
                bits |= 1 << j;
            }
        }
        
        out->dist = LittleFloat( in->dist );
        out->type = PlaneTypeForNormal( out->normal );
        out->signbits = bits;
    }
}

/*
=================
CMod_LoadLeafBrushes
=================
*/
void CMod_LoadLeafBrushes( lump_t* l )
{
    sint i, *out, *in, count;
    
    in = ( sint* )( cmod_base + l->fileofs );
    if( l->filelen % sizeof( *in ) )
    {
        Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
    }
    count = l->filelen / sizeof( *in );
    
    // ydnar: more than <count> brushes are stored in leafbrushes...
    cm.leafbrushes = ( sint* )Hunk_Alloc( ( BOX_LEAF_BRUSHES + count ) * sizeof( *cm.leafbrushes ), h_high );
    cm.numLeafBrushes = count;
    
    out = cm.leafbrushes;
    
    for( i = 0; i < count; i++, in++, out++ )
    {
        *out = LittleLong( *in );
    }
}

/*
=================
CMod_LoadLeafSurfaces
=================
*/
void CMod_LoadLeafSurfaces( lump_t* l )
{
    sint i, *out, *in, count;
    
    in = ( sint* )( cmod_base + l->fileofs );
    if( l->filelen % sizeof( *in ) )
    {
        Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
    }
    count = l->filelen / sizeof( *in );
    
    cm.leafsurfaces = ( sint* )Hunk_Alloc( count * sizeof( *cm.leafsurfaces ), h_high );
    cm.numLeafSurfaces = count;
    
    out = cm.leafsurfaces;
    
    for( i = 0; i < count; i++, in++, out++ )
    {
        *out = LittleLong( *in );
    }
}

/*
=================
CMod_LoadBrushSides
=================
*/
void CMod_LoadBrushSides( lump_t* l )
{
    sint             i, count, num;
    cbrushside_t*   out;
    dbrushside_t*   in;
    
    in = ( dbrushside_t* )( cmod_base + l->fileofs );
    if( l->filelen % sizeof( *in ) )
    {
        Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
    }
    count = l->filelen / sizeof( *in );
    
    cm.brushsides = ( cbrushside_t* )Hunk_Alloc( ( BOX_SIDES + count ) * sizeof( *cm.brushsides ), h_high );
    cm.numBrushSides = count;
    
    out = cm.brushsides;
    
    for( i = 0; i < count; i++, in++, out++ )
    {
        num = LittleLong( in->planeNum );
        out->planeNum = num;
        out->plane = &cm.planes[num];
        out->shaderNum = LittleLong( in->shaderNum );
        if( out->shaderNum < 0 || out->shaderNum >= cm.numShaders )
        {
            Com_Error( ERR_DROP, "CMod_LoadBrushSides: bad shaderNum: %i", out->shaderNum );
        }
        out->surfaceFlags = cm.shaders[out->shaderNum].surfaceFlags;
    }
}

#define CM_EDGE_VERTEX_EPSILON 0.1f

/*
=================
CMod_BrushEdgesAreTheSame
=================
*/
static bool CMod_BrushEdgesAreTheSame( const vec3_t p0, const vec3_t p1, const vec3_t q0, const vec3_t q1 )
{
    if( VectorCompareEpsilon( p0, q0, CM_EDGE_VERTEX_EPSILON ) && VectorCompareEpsilon( p1, q1, CM_EDGE_VERTEX_EPSILON ) )
    {
        return true;
    }
    
    if( VectorCompareEpsilon( p1, q0, CM_EDGE_VERTEX_EPSILON ) && VectorCompareEpsilon( p0, q1, CM_EDGE_VERTEX_EPSILON ) )
    {
        return true;
    }
    
    return false;
}

/*
=================
CMod_AddEdgeToBrush
=================
*/
static bool CMod_AddEdgeToBrush( const vec3_t p0, const vec3_t p1, cbrushedge_t* edges, sint* numEdges )
{
    sint i;
    
    if( !edges || !numEdges )
    {
        return false;
    }
    
    for( i = 0; i < *numEdges; i++ )
    {
        if( CMod_BrushEdgesAreTheSame( p0, p1, edges[i].p0, edges[i].p1 ) )
        {
            return false;
        }
    }
    
    VectorCopy( p0, edges[*numEdges].p0 );
    VectorCopy( p1, edges[*numEdges].p1 );
    ( *numEdges )++;
    
    return true;
}

/*
=================
CMod_CreateBrushSideWindings
=================
*/
static void CMod_CreateBrushSideWindings( void )
{
    sint             i, j, k, numEdges, edgesAlloc, totalEdgesAlloc = 0, totalEdges = 0;
    winding_t*      w;
    cbrushside_t*   side, *chopSide;
    cplane_t*       plane;
    cbrush_t*       brush;
    cbrushedge_t*   tempEdges;
    
    for( i = 0; i < cm.numBrushes; i++ )
    {
        brush = &cm.brushes[i];
        numEdges = 0;
        
        // walk the list of brush sides
        for( j = 0; j < brush->numsides; j++ )
        {
            // get side and plane
            side = &brush->sides[j];
            plane = side->plane;
            
            w = BaseWindingForPlane( plane->normal, plane->dist );
            
            // walk the list of brush sides
            for( k = 0; k < brush->numsides && w != nullptr; k++ )
            {
                chopSide = &brush->sides[k];
                
                if( chopSide == side )
                {
                    continue;
                }
                
                if( chopSide->planeNum == ( side->planeNum ^ 1 ) )
                {
                    continue; // back side clipaway
                }
                
                plane = &cm.planes[chopSide->planeNum ^ 1];
                ChopWindingInPlace( &w, plane->normal, plane->dist, 0 );
            }
            
            if( w )
            {
                numEdges += w->numpoints;
            }
            
            // set side winding
            side->winding = w;
        }
        
        // Allocate a temporary buffer of the maximal size
        tempEdges = ( cbrushedge_t* ) Z_Malloc( sizeof( cbrushedge_t ) * numEdges );
        brush->numEdges = 0;
        
        // compose the points into edges
        for( j = 0; j < brush->numsides; j++ )
        {
            side = &brush->sides[j];
            
            if( side->winding )
            {
                for( k = 0; k < side->winding->numpoints - 1; k++ )
                {
                    if( brush->numEdges == numEdges )
                    {
                        Com_Error( ERR_FATAL, "Insufficient memory allocated for collision map edges" );
                    }
                    
                    CMod_AddEdgeToBrush( side->winding->p[k], side->winding->p[k + 1], tempEdges, &brush->numEdges );
                }
                
                FreeWinding( side->winding );
                side->winding = nullptr;
            }
        }
        
        // Allocate a buffer of the actual size
        edgesAlloc = sizeof( cbrushedge_t ) * brush->numEdges;
        totalEdgesAlloc += edgesAlloc;
        brush->edges = ( cbrushedge_t* ) Hunk_Alloc( edgesAlloc, h_low );
        
        // Copy temporary buffer to permanent buffer
        ::memcpy( brush->edges, tempEdges, edgesAlloc );
        
        // Free temporary buffer
        Z_Free( tempEdges );
        
        totalEdges += brush->numEdges;
    }
    
    Com_DPrintf( "Allocated %d bytes for %d collision map edges...\n", totalEdgesAlloc, totalEdges );
}

/*
=================
CMod_LoadEntityString
=================
*/
void CMod_LoadEntityString( lump_t* l )
{
    valueType* p, *token, keyname[MAX_TOKEN_CHARS], value[MAX_TOKEN_CHARS];
    
    cm.entityString = ( valueType* )Hunk_Alloc( l->filelen, h_high );
    cm.numEntityChars = l->filelen;
    ::memcpy( cm.entityString, cmod_base + l->fileofs, l->filelen );
    
    p = cm.entityString;
    
    // only parse the world spawn
    while( 1 )
    {
        // parse key
        token = COM_ParseExt2( &p, true );
        
        if( !*token )
        {
            Com_Printf( S_COLOR_YELLOW "WARNING: unexpected end of entities string while parsing worldspawn\n" );
            break;
        }
        
        if( *token == '{' )
        {
            continue;
        }
        
        if( *token == '}' )
        {
            break;
        }
        
        Q_strncpyz( keyname, token, sizeof( keyname ) );
        
        // parse value
        token = COM_ParseExt2( &p, false );
        
        if( !*token )
        {
            continue;
        }
        
        Q_strncpyz( value, token, sizeof( value ) );
        
        // check for per-poly collision support
        if( !Q_stricmp( keyname, "perPolyCollision" ) && !Q_stricmp( value, "1" ) )
        {
            Com_Printf( "map features per poly collision detection\n" );
            cm.perPolyCollision = true;
            continue;
        }
        
        if( !Q_stricmp( keyname, "classname" ) && Q_stricmp( value, "worldspawn" ) )
        {
            Com_Printf( S_COLOR_YELLOW "WARNING: expected worldspawn found '%s'\n", value );
            break;
        }
    }
}

/*
=================
CMod_LoadVisibility
=================
*/
#define VIS_HEADER  8
void CMod_LoadVisibility( lump_t* l )
{
    sint             len;
    uchar8*           buf;
    
    len = l->filelen;
    if( !len )
    {
        cm.clusterBytes = ( cm.numClusters + 31 ) & ~31;
        cm.visibility = ( uchar8* )Hunk_Alloc( cm.clusterBytes, h_high );
        ::memset( cm.visibility, 255, cm.clusterBytes );
        return;
    }
    buf = cmod_base + l->fileofs;
    
    cm.vised = true;
    cm.visibility = ( uchar8* )Hunk_Alloc( len, h_high );
    cm.numClusters = LittleLong( ( ( sint* )buf )[0] );
    cm.clusterBytes = LittleLong( ( ( sint* )buf )[1] );
    ::memcpy( cm.visibility, buf + VIS_HEADER, len - VIS_HEADER );
}

//==================================================================


/*
=================
CMod_LoadSurfaces
=================
*/
#ifdef DEDICATED
#define	MAX_PATCH_SIZE		64
#endif
#define	MAX_PATCH_VERTS		(MAX_PATCH_SIZE * MAX_PATCH_SIZE)
void CMod_LoadSurfaces( lump_t* surfs, lump_t* verts, lump_t* indexesLump )
{
    drawVert_t*     dv, *dv_p;
    dsurface_t*     in;
    sint             count, i, j, numVertexes, width, height, shaderNum, numIndexes, *index, *index_p;
    cSurface_t*     surface;
    static vec3_t   vertexes[SHADER_MAX_VERTEXES];
    static sint      indexes[SHADER_MAX_INDEXES];
    
    in = ( dsurface_t* )( cmod_base + surfs->fileofs );
    if( surfs->filelen % sizeof( *in ) )
    {
        Com_Error( ERR_DROP, "CMod_LoadSurfaces: funny lump size" );
    }
    cm.numSurfaces = count = surfs->filelen / sizeof( *in );
    cm.surfaces = ( cSurface_t** )Hunk_Alloc( cm.numSurfaces * sizeof( cm.surfaces[0] ), h_high );
    
    dv = ( drawVert_t* )( cmod_base + verts->fileofs );
    if( verts->filelen % sizeof( *dv ) )
    {
        Com_Error( ERR_DROP, "CMod_LoadSurfaces: funny lump size" );
    }
    
    index = ( sint* )( cmod_base + indexesLump->fileofs );
    if( indexesLump->filelen % sizeof( *index ) )
    {
        Com_Error( ERR_DROP, "CMod_LoadSurfaces: funny lump size" );
    }
    
    // scan through all the surfaces
    for( i = 0; i < count; i++, in++ )
    {
        if( LittleLong( in->surfaceType ) == MST_PATCH )
        {
            sint j = 0, k = 0, rowLimit = in->patchHeight - 1, colLimit = in->patchWidth - 1;
            
            // FIXME: check for non-colliding patches
            cm.surfaces[i] = surface = ( cSurface_t* )Hunk_Alloc( sizeof( *surface ), h_high );
            surface->type = MST_PATCH;
            
            // load the full drawverts onto the stack
            width = LittleLong( in->patchWidth );
            height = LittleLong( in->patchHeight );
            numVertexes = width * height;
            if( numVertexes > MAX_PATCH_VERTS )
            {
                Com_Error( ERR_DROP, "CMod_LoadSurfaces: MAX_PATCH_VERTS" );
            }
            
            dv_p = dv + LittleLong( in->firstVert );
            for( j = 0; j < numVertexes; j++, dv_p++ )
            {
                vertexes[j][0] = LittleFloat( dv_p->xyz[0] );
                vertexes[j][1] = LittleFloat( dv_p->xyz[1] );
                vertexes[j][2] = LittleFloat( dv_p->xyz[2] );
            }
            
            shaderNum = LittleLong( in->shaderNum );
            surface->contents = cm.shaders[shaderNum].contentFlags;
            surface->surfaceFlags = cm.shaders[shaderNum].surfaceFlags;
            
            // create the internal facet structure
            surface->sc = CM_GeneratePatchCollide( width, height, vertexes );
        }
        else if( LittleLong( in->surfaceType ) == MST_TRIANGLE_SOUP && ( cm.perPolyCollision
#ifndef BSPC
                 || cm_forceTriangles->integer
#endif
                                                                       ) )
        {
            // FIXME: check for non-colliding triangle soups
            
            cm.surfaces[i] = surface = ( cSurface_t* )Hunk_Alloc( sizeof( *surface ), h_high );
            surface->type = MST_TRIANGLE_SOUP;
            
            // load the full drawverts onto the stack
            numVertexes = LittleLong( in->numVerts );
            if( numVertexes > SHADER_MAX_VERTEXES )
            {
                Com_Error( ERR_DROP, "CMod_LoadSurfaces: SHADER_MAX_VERTEXES" );
            }
            
            dv_p = dv + LittleLong( in->firstVert );
            for( j = 0; j < numVertexes; j++, dv_p++ )
            {
                vertexes[j][0] = LittleFloat( dv_p->xyz[0] );
                vertexes[j][1] = LittleFloat( dv_p->xyz[1] );
                vertexes[j][2] = LittleFloat( dv_p->xyz[2] );
            }
            
            numIndexes = LittleLong( in->numIndexes );
            if( numIndexes > SHADER_MAX_INDEXES )
            {
                Com_Error( ERR_DROP, "CMod_LoadSurfaces: SHADER_MAX_INDEXES" );
            }
            
            index_p = index + LittleLong( in->firstIndex );
            for( j = 0; j < numIndexes; j++, index_p++ )
            {
                indexes[j] = LittleLong( *index_p );
                
                if( indexes[j] < 0 || indexes[j] >= numVertexes )
                {
                    Com_Error( ERR_DROP, "CMod_LoadSurfaces: Bad index in trisoup surface" );
                }
            }
            
            shaderNum = LittleLong( in->shaderNum );
            surface->contents = cm.shaders[shaderNum].contentFlags;
            surface->surfaceFlags = cm.shaders[shaderNum].surfaceFlags;
            
            // create the internal facet structure
            surface->sc = CM_GenerateTriangleSoupCollide( numVertexes, vertexes, numIndexes, indexes );
        }
    }
}

//==================================================================

/*
==================
CM_LumpChecksum
==================
*/
uint CM_LumpChecksum( lump_t* lump )
{
    return LittleLong( MD4System->BlockChecksum( cmod_base + lump->fileofs, lump->filelen ) );
}

/*
==================
CM_Checksum
==================
*/
uint CM_Checksum( dheader_t* header )
{
    uint checksums[16];
    
    checksums[0] = CM_LumpChecksum( &header->lumps[LUMP_SHADERS] );
    checksums[1] = CM_LumpChecksum( &header->lumps[LUMP_LEAFS] );
    checksums[2] = CM_LumpChecksum( &header->lumps[LUMP_LEAFBRUSHES] );
    checksums[3] = CM_LumpChecksum( &header->lumps[LUMP_LEAFSURFACES] );
    checksums[4] = CM_LumpChecksum( &header->lumps[LUMP_PLANES] );
    checksums[5] = CM_LumpChecksum( &header->lumps[LUMP_BRUSHSIDES] );
    checksums[6] = CM_LumpChecksum( &header->lumps[LUMP_BRUSHES] );
    checksums[7] = CM_LumpChecksum( &header->lumps[LUMP_MODELS] );
    checksums[8] = CM_LumpChecksum( &header->lumps[LUMP_NODES] );
    checksums[9] = CM_LumpChecksum( &header->lumps[LUMP_SURFACES] );
    checksums[10] = CM_LumpChecksum( &header->lumps[LUMP_DRAWVERTS] );
    
    return LittleLong( MD4System->BlockChecksum( checksums, 11 * 4 ) );
}

/*
==================
idCollisionModelManagerLocal::LoadMap

Loads in the map and all submodels
==================
*/
void idCollisionModelManagerLocal::LoadMap( pointer name, bool clientload, sint* checksum )
{
    sint* buf, i, length;
    dheader_t header;
    static uint last_checksum;
    
    if( !name || !name[0] )
    {
        Com_Error( ERR_DROP, "idCollisionModelManagerLocal::LoadMap: nullptr name" );
    }
    
#ifndef BSPC
    cm_noAreas = cvarSystem->Get( "cm_noAreas", "0", CVAR_CHEAT, "Toggle the ability of the player bounding box to clip through areas." );
    cm_noCurves = cvarSystem->Get( "cm_noCurves", "0", CVAR_CHEAT, "Toggle the ability of the player bounding box to clip through curved surfaces." );
    cm_forceTriangles = cvarSystem->Get( "cm_forceTriangles", "0", CVAR_CHEAT | CVAR_LATCH, "Convert all patches into triangles." );
    cm_playerCurveClip = cvarSystem->Get( "cm_playerCurveClip", "1", CVAR_ARCHIVE | CVAR_CHEAT, "toggles the ability of the player bounding box to respect curved surfaces." );
    cm_optimize = cvarSystem->Get( "cm_optimize", "1", CVAR_CHEAT, "Collision model optimization" );
    cm_showCurves = cvarSystem->Get( "cm_showCurves", "0", CVAR_CHEAT, "Showing curved surfaces" );
    cm_showTriangles = cvarSystem->Get( "cm_showTriangles", "0", CVAR_CHEAT, "Showing triangles in the surfaces" );
#endif
    Com_DPrintf( "idCollisionModelManagerLocal::LoadMap( %s, %i )\n", name, clientload );
    
    if( !strcmp( cm.name, name ) && clientload )
    {
        *checksum = last_checksum;
        return;
    }
    
    // free old stuff
    ::memset( &cm, 0, sizeof( cm ) );
    CM_ClearLevelPatches();
    
    if( !name[0] )
    {
        cm.numLeafs = 1;
        cm.numClusters = 1;
        cm.numAreas = 1;
        cm.cmodels = ( cmodel_t* )Hunk_Alloc( sizeof( *cm.cmodels ), h_high );
        *checksum = 0;
        return;
    }
    
    //
    // load the file
    //
#ifndef BSPC
    length = fileSystem->ReadFile( name, ( void** )&buf );
#else
    length = LoadQuakeFile( ( quakefile_t* ) name, ( void** )&buf );
#endif
    
    if( !buf )
    {
        Com_Error( ERR_DROP, "Couldn't load %s", name );
    }
    
    last_checksum = LittleLong( MD4System->BlockChecksum( buf, length ) );
    *checksum = last_checksum;
    
    header = *( dheader_t* ) buf;
    for( i = 0; i < sizeof( dheader_t ) / 4; i++ )
    {
        ( ( sint* )&header )[i] = LittleLong( ( ( sint* )&header )[i] );
    }
    
#if 0
    if( header.version != BSP_VERSION )
    {
        Com_Error( ERR_DROP, "idCollisionModelManagerLocal::LoadMap: %s has wrong version number (%i should be %i)", name, header.version, BSP_VERSION );
    }
#endif
    cmod_base = ( uchar8* ) buf;
    
    // load into heap
    CMod_LoadShaders( &header.lumps[LUMP_SHADERS] );
    CMod_LoadLeafs( &header.lumps[LUMP_LEAFS] );
    CMod_LoadLeafBrushes( &header.lumps[LUMP_LEAFBRUSHES] );
    CMod_LoadLeafSurfaces( &header.lumps[LUMP_LEAFSURFACES] );
    CMod_LoadPlanes( &header.lumps[LUMP_PLANES] );
    CMod_LoadBrushSides( &header.lumps[LUMP_BRUSHSIDES] );
    CMod_LoadBrushes( &header.lumps[LUMP_BRUSHES] );
    CMod_LoadSubmodels( &header.lumps[LUMP_MODELS] );
    CMod_LoadNodes( &header.lumps[LUMP_NODES] );
    CMod_LoadEntityString( &header.lumps[LUMP_ENTITIES] );
    CMod_LoadVisibility( &header.lumps[LUMP_VISIBILITY] );
    CMod_LoadSurfaces( &header.lumps[LUMP_SURFACES], &header.lumps[LUMP_DRAWVERTS], &header.lumps[LUMP_DRAWINDEXES] );
    
    CMod_CreateBrushSideWindings();
    
    // we are NOT freeing the file, because it is cached for the ref
    fileSystem->FreeFile( buf );
    
    CM_InitBoxHull();
    
    CM_FloodAreaConnections();
    
    // allow this to be cached if it is loaded by the server
    if( !clientload )
    {
        Q_strncpyz( cm.name, name, sizeof( cm.name ) );
    }
}

/*
==================
idCollisionModelManagerLocal::ClearMap
==================
*/
void idCollisionModelManagerLocal::ClearMap( void )
{
    ::memset( &cm, 0, sizeof( cm ) );
    CM_ClearLevelPatches();
}

/*
==================
CM_ClipHandleToModel
==================
*/
cmodel_t* CM_ClipHandleToModel( clipHandle_t handle )
{
    if( handle < 0 )
    {
        Com_Error( ERR_DROP, "CM_ClipHandleToModel: bad handle %i", handle );
    }
    if( handle < cm.numSubModels )
    {
        return &cm.cmodels[handle];
    }
    if( handle == BOX_MODEL_HANDLE || handle == CAPSULE_MODEL_HANDLE )
    {
        return &box_model;
    }
    if( handle < MAX_SUBMODELS )
    {
        Com_Error( ERR_DROP, "CM_ClipHandleToModel: bad handle %i < %i < %i", cm.numSubModels, handle, MAX_SUBMODELS );
    }
    Com_Error( ERR_DROP, "CM_ClipHandleToModel: bad handle %i", handle + MAX_SUBMODELS );
    
    return nullptr;
    
}

/*
==================
idCollisionModelManagerLocal::InlineModel
==================
*/
clipHandle_t idCollisionModelManagerLocal::InlineModel( sint index )
{
    if( index < 0 || index >= cm.numSubModels )
    {
        Com_Error( ERR_DROP, "CM_InlineModel: bad number" );
    }
    return index;
}

/*
===================
idCollisionModelManagerLocal::NumClusters
===================
*/
sint idCollisionModelManagerLocal::NumClusters( void )
{
    return cm.numClusters;
}

/*
===================
idCollisionModelManagerLocal::NumInlineModels
===================
*/
sint idCollisionModelManagerLocal::NumInlineModels( void )
{
    return cm.numSubModels;
}

/*
===================
idCollisionModelManagerLocal::EntityString
===================
*/
valueType* idCollisionModelManagerLocal::EntityString( void )
{
    return cm.entityString;
}

/*
===================
idCollisionModelManagerLocal::LeafCluster
===================
*/
sint idCollisionModelManagerLocal::LeafCluster( sint leafnum )
{
    if( leafnum < 0 || leafnum >= cm.numLeafs )
    {
        Com_Error( ERR_DROP, "CM_LeafCluster: bad leaf number %i", leafnum );
    }
    return cm.leafs[leafnum].cluster;
}

/*
===================
idCollisionModelManagerLocal::LeafArea
===================
*/
sint idCollisionModelManagerLocal::LeafArea( sint leafnum )
{
    if( leafnum < 0 || leafnum >= cm.numLeafs )
    {
        Com_Error( ERR_DROP, "CM_LeafArea: bad leaf number %i", leafnum );
    }
    return cm.leafs[leafnum].area;
}

//=======================================================================


/*
===================
CM_InitBoxHull

Set up the planes and nodes so that the six floats of a bounding box
can just be stored out and get a proper clipping hull structure.
===================
*/
void CM_InitBoxHull( void )
{
    sint             i, side;
    cplane_t*       p;
    cbrushside_t*   s;
    
    box_planes = &cm.planes[cm.numPlanes];
    
    box_brush = &cm.brushes[cm.numBrushes];
    box_brush->numsides = 6;
    box_brush->sides = cm.brushsides + cm.numBrushSides;
    box_brush->contents = CONTENTS_BODY;
    box_brush->edges = ( cbrushedge_t* ) Hunk_Alloc( sizeof( cbrushedge_t ) * 12, h_low );
    box_brush->numEdges = 12;
    
    box_model.leaf.numLeafBrushes = 1;
//  box_model.leaf.firstLeafBrush = cm.numBrushes;
    box_model.leaf.firstLeafBrush = cm.numLeafBrushes;
    cm.leafbrushes[cm.numLeafBrushes] = cm.numBrushes;
    
    for( i = 0; i < 6; i++ )
    {
        side = i & 1;
        
        // brush sides
        s = &cm.brushsides[cm.numBrushSides + i];
        s->plane = cm.planes + ( cm.numPlanes + i * 2 + side );
        s->surfaceFlags = 0;
        
        // planes
        p = &box_planes[i * 2];
        p->type = i >> 1;
        p->signbits = 0;
        VectorClear( p->normal );
        p->normal[i >> 1] = 1;
        
        p = &box_planes[i * 2 + 1];
        p->type = 3 + ( i >> 1 );
        p->signbits = 0;
        VectorClear( p->normal );
        p->normal[i >> 1] = -1;
        
        SetPlaneSignbits( p );
    }
}

/*
===================
idCollisionModelManagerLocal::TempBoxModel

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
Capsules are handled differently though.
===================
*/
clipHandle_t idCollisionModelManagerLocal::TempBoxModel( const vec3_t mins, const vec3_t maxs, sint capsule )
{

    VectorCopy( mins, box_model.mins );
    VectorCopy( maxs, box_model.maxs );
    
    if( capsule )
    {
        return CAPSULE_MODEL_HANDLE;
    }
    
    box_planes[0].dist = maxs[0];
    box_planes[1].dist = -maxs[0];
    box_planes[2].dist = mins[0];
    box_planes[3].dist = -mins[0];
    box_planes[4].dist = maxs[1];
    box_planes[5].dist = -maxs[1];
    box_planes[6].dist = mins[1];
    box_planes[7].dist = -mins[1];
    box_planes[8].dist = maxs[2];
    box_planes[9].dist = -maxs[2];
    box_planes[10].dist = mins[2];
    box_planes[11].dist = -mins[2];
    
    // First side
    VectorSet( box_brush->edges[0].p0, mins[0], mins[1], mins[2] );
    VectorSet( box_brush->edges[0].p1, mins[0], maxs[1], mins[2] );
    VectorSet( box_brush->edges[1].p0, mins[0], maxs[1], mins[2] );
    VectorSet( box_brush->edges[1].p1, mins[0], maxs[1], maxs[2] );
    VectorSet( box_brush->edges[2].p0, mins[0], maxs[1], maxs[2] );
    VectorSet( box_brush->edges[2].p1, mins[0], mins[1], maxs[2] );
    VectorSet( box_brush->edges[3].p0, mins[0], mins[1], maxs[2] );
    VectorSet( box_brush->edges[3].p1, mins[0], mins[1], mins[2] );
    
    // Opposite side
    VectorSet( box_brush->edges[4].p0, maxs[0], mins[1], mins[2] );
    VectorSet( box_brush->edges[4].p1, maxs[0], maxs[1], mins[2] );
    VectorSet( box_brush->edges[5].p0, maxs[0], maxs[1], mins[2] );
    VectorSet( box_brush->edges[5].p1, maxs[0], maxs[1], maxs[2] );
    VectorSet( box_brush->edges[6].p0, maxs[0], maxs[1], maxs[2] );
    VectorSet( box_brush->edges[6].p1, maxs[0], mins[1], maxs[2] );
    VectorSet( box_brush->edges[7].p0, maxs[0], mins[1], maxs[2] );
    VectorSet( box_brush->edges[7].p1, maxs[0], mins[1], mins[2] );
    
    // Connecting edges
    VectorSet( box_brush->edges[8].p0, mins[0], mins[1], mins[2] );
    VectorSet( box_brush->edges[8].p1, maxs[0], mins[1], mins[2] );
    VectorSet( box_brush->edges[9].p0, mins[0], maxs[1], mins[2] );
    VectorSet( box_brush->edges[9].p1, maxs[0], maxs[1], mins[2] );
    VectorSet( box_brush->edges[10].p0, mins[0], maxs[1], maxs[2] );
    VectorSet( box_brush->edges[10].p1, maxs[0], maxs[1], maxs[2] );
    VectorSet( box_brush->edges[11].p0, mins[0], mins[1], maxs[2] );
    VectorSet( box_brush->edges[11].p1, maxs[0], mins[1], maxs[2] );
    
    VectorCopy( mins, box_brush->bounds[0] );
    VectorCopy( maxs, box_brush->bounds[1] );
    
    return BOX_MODEL_HANDLE;
}

void idCollisionModelManagerLocal::SetTempBoxModelContents( sint contents )
{
    box_brush->contents = contents;
}

/*
===================
idCollisionModelManagerLocal::ModelBounds
===================
*/
void idCollisionModelManagerLocal::ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs )
{
    cmodel_t* cmod;
    
    cmod = CM_ClipHandleToModel( model );
    VectorCopy( cmod->mins, mins );
    VectorCopy( cmod->maxs, maxs );
}

/*
===================
idCollisionModelManagerLocal::ModelBounds
===================
*/
sint idCollisionModelManagerLocal::BoxOnPlaneSide( vec3_t emins, vec3_t emaxs, cplane_t* p )
{
    float32           dist1, dist2;
    sint             sides;
    
    // fast axial cases
    if( p->type < 3 )
    {
        if( p->dist <= emins[p->type] )
            return 1;
        if( p->dist >= emaxs[p->type] )
            return 2;
        return 3;
    }
    
    // general case
    switch( p->signbits )
    {
        case 0:
            dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
            dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
            break;
        case 1:
            dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
            dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
            break;
        case 2:
            dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
            dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
            break;
        case 3:
            dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
            dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
            break;
        case 4:
            dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
            dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
            break;
        case 5:
            dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
            dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
            break;
        case 6:
            dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
            dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
            break;
        case 7:
            dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
            dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
            break;
        default:
            dist1 = dist2 = 0;	// shut up compiler
            break;
    }
    
    sides = 0;
    if( dist1 >= p->dist )
        sides = 1;
    if( dist2 < p->dist )
        sides |= 2;
        
    return sides;
}
