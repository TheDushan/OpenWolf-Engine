////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2005 Id Software, Inc.
// Copyright(C) 2000 - 2013 Darklegion Development
// Copyright(C) 2011 - 2023 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   surfaceFlags_Tech3.h
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description: This file must be identical in the quake and utils directories
//              contents flags are seperate bits a given brush can contribute multiple
//              content bits these definitions also need to be in q_shared.h!
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SURFACEFLAGS_HPP__
#define __SURFACEFLAGS_HPP__

#define CONTENTS_SOLID          1       // an eye is never valid in a solid
#define CONTENTS_LAVA           8
#define CONTENTS_SLIME          16
#define CONTENTS_WATER          32
#define CONTENTS_FOG            64

#define CONTENTS_NOTTEAM1       0x0080
#define CONTENTS_NOTTEAM2       0x0100
#define CONTENTS_NOBOTCLIP      0x0200

#define CONTENTS_AREAPORTAL     0x8000

#define CONTENTS_PLAYERCLIP     0x10000
#define CONTENTS_MONSTERCLIP    0x20000
//bot specific contents types
#define CONTENTS_TELEPORTER     0x40000
#define CONTENTS_JUMPPAD        0x80000
#define CONTENTS_CLUSTERPORTAL  0x100000
#define CONTENTS_DONOTENTER     0x200000
#define CONTENTS_BOTCLIP        0x400000
#define CONTENTS_MOVER          0x800000

#define CONTENTS_ORIGIN         0x1000000   // removed before bsping an entity

#define CONTENTS_BODY           0x2000000   // should never be on a brush, only in game
#define CONTENTS_CORPSE         0x4000000
#define CONTENTS_DETAIL         0x8000000   // brushes not used for the bsp
#define CONTENTS_STRUCTURAL     0x10000000  // brushes used for the bsp
#define CONTENTS_TRANSLUCENT    0x20000000  // don't consume surface fragments inside
#define CONTENTS_TRIGGER        0x40000000
#define CONTENTS_NODROP         0x80000000  // don't leave bodies or items (death fog, lava)

// custominfoparms below
#define CONTENTS_NOALIENBUILD           0x1000  //disallow alien building
#define CONTENTS_NOHUMANBUILD           0x2000  //disallow human building
#define CONTENTS_NOBUILD                0x4000  //disallow building

#define SURF_NODAMAGE           0x1     // never give falling damage
#define SURF_SLICK              0x2     // effects game physics
#define SURF_SKY                0x4     // lighting from environment map
#define SURF_LADDER             0x8
#define SURF_NOIMPACT           0x10    // don't make missile explosions
#define SURF_NOMARKS            0x20    // don't leave missile marks
#define SURF_FLESH              0x40    // make flesh sounds and effects
#define SURF_NODRAW             0x80    // don't generate a drawsurface at all
#define SURF_HINT               0x100   // make a primary bsp splitter
#define SURF_SKIP               0x200   // completely ignore, allowing non-closed brushes
#define SURF_NOLIGHTMAP         0x400   // surface doesn't need a lightmap
#define SURF_POINTLIGHT         0x800   // generate lighting info at vertexes
#define SURF_METALSTEPS         0x1000  // clanking footsteps
#define SURF_NOSTEPS            0x2000  // no footstep sounds
#define SURF_NONSOLID           0x4000  // don't collide against curves with this set
#define SURF_LIGHTFILTER        0x8000  // act as a light filter during q3map -light
#define SURF_ALPHASHADOW        0x10000 // do per-pixel light shadow casting in q3map
#define SURF_NODLIGHT           0x20000 // don't dlight even if solid (solid lava, skies)
#define SURF_DUST               0x40000 // leave a dust trail when walking on this surface

// custominfoparms below
#define SURF_NOALIENBUILD   0x80000  //disallow alien building
#define SURF_NOHUMANBUILD   0x100000 //disallow human building
#define SURF_NOBUILD        0x200000 //disallow building

#define MATERIAL_BITS           5
#define MATERIAL_MASK           0x1f    // mask to get the material type

#define MATERIAL_NONE                           0           // for when the artist hasn't set anything up =)
#define MATERIAL_SOLIDWOOD                      1           // freshly cut timber
#define MATERIAL_HOLLOWWOOD                     2           // termite infested creaky wood
#define MATERIAL_SOLIDMETAL                     3           // solid girders
#define MATERIAL_HOLLOWMETAL                    4           // hollow metal machines
#define MATERIAL_SHORTGRASS                     5           // manicured lawn
#define MATERIAL_LONGGRASS                      6           // long jungle grass
#define MATERIAL_DIRT                           7           // hard mud
#define MATERIAL_SAND                           8           // sandy beach
#define MATERIAL_GRAVEL                         9           // lots of small stones
#define MATERIAL_GLASS                          10          //
#define MATERIAL_CONCRETE                       11          // hardened concrete pavement
#define MATERIAL_MARBLE                         12          // marble floors
#define MATERIAL_WATER                          13          // light covering of water on a surface
#define MATERIAL_SNOW                           14          // freshly laid snow
#define MATERIAL_ICE                            15          // packed snow/solid ice
#define MATERIAL_FLESH                          16          // hung meat, corpses in the world
#define MATERIAL_MUD                            17          // wet soil
#define MATERIAL_BPGLASS                        18          // bulletproof glass
#define MATERIAL_DRYLEAVES                      19          // dried up leaves on the floor
#define MATERIAL_GREENLEAVES                    20          // fresh leaves still on a tree
#define MATERIAL_FABRIC                         21          // Cotton sheets
#define MATERIAL_CANVAS                         22          // tent material
#define MATERIAL_ROCK                           23          //
#define MATERIAL_RUBBER                         24          // hard tire like rubber
#define MATERIAL_PLASTIC                        25          //
#define MATERIAL_TILES                          26          // tiled floor
#define MATERIAL_CARPET                         27          // lush carpet
#define MATERIAL_PLASTER                        28          // drywall style plaster
#define MATERIAL_SHATTERGLASS                   29          // glass with the Crisis Zone style shattering
#define MATERIAL_ARMOR                          30          // body armor
#define MATERIAL_COMPUTER                       31          // computers/electronic equipment
#define MATERIAL_PUDDLE                         32          // shallow puddle on floor surface, or water that should not use GLSL.
#define MATERIAL_POLISHEDWOOD                   33          // shiny polished wood
#define MATERIAL_LAVA                           34          // lava
#define MATERIAL_EFX                            35          // any efx surfaces
#define MATERIAL_BLASTERBOLT                    36          // blaster bolts etc
#define MATERIAL_FIRE                           37          // fire
#define MATERIAL_SMOKE                          38          // smoke
#define MATERIAL_FIREFLIES                      39          // fire flies fx
#define MATERIAL_MAGIC_PARTICLES_TREE           40          // magic tree particle fx
#define MATERIAL_MAGIC_PARTICLES                41          // magic particles (small particle fx)
#define MATERIAL_LAST                           42          // number of materials

// Defined as a macro here so one change will affect all the relevant files

#define MATERIALS   \
    "none",         \
    "solidwood",    \
    "hollowwood",   \
    "SolidMetal",   \
    "hollowmetal",  \
    "shortgrass",   \
    "longgrass",    \
    "dirt",         \
    "sand",         \
    "gravel",       \
    "glass",        \
    "concrete",     \
    "marble",       \
    "water",        \
    "snow",         \
    "ice",          \
    "flesh",        \
    "mud",          \
    "bpglass",      \
    "dryleaves",    \
    "greenleaves",  \
    "fabric",       \
    "canvas",       \
    "rock",         \
    "rubber",       \
    "plastic",      \
    "tiles",        \
    "carpet",       \
    "plaster",      \
    "shatterglass", \
    "armor",        \
    "lava",         \
    "computer"/* this was missing, see enums above, plus ShaderEd2 pulldown options */

#endif //!__SURFACEFLAGS_HPP__
