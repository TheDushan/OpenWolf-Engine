////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2020 - 2021 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of OpenWolf Engine.
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
// File name:   clientScreen.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTSCREEN_LOCAL_H__
#define __CLIENTSCREEN_LOCAL_H__

typedef struct
{
    sint color;
    float32 value;
} graphsamp_t;

static sint current;
static graphsamp_t values[1024];

//
// idClientScreenSystemLocal
//
class idClientScreenSystemLocal : public idClientScreenSystem
{
public:
    idClientScreenSystemLocal();
    ~idClientScreenSystemLocal();
    
    virtual void UpdateScreen( void );
    
    static void AdjustFrom640( float32* x, float32* y, float32* w, float32* h );
    static void FillRect( float32 x, float32 y, float32 width, float32 height, const float32* color );
    static void DrawChar( sint x, sint y, float32 size, sint ch );
    static void DrawConsoleFontChar( float32 x, float32 y, sint ch );
    static void DrawSmallChar( sint x, sint y, sint ch );
    static void DrawStringExt( sint x, sint y, float32 size, pointer string, float32* setColor, bool forceColor, bool noColorEscape );
    static void DrawBigString( sint x, sint y, pointer s, float32 alpha, bool noColorEscape );
    static void DrawSmallStringExt( sint x, sint y, pointer string, float32* setColor, bool forceColor, bool noColorEscape );
    static sint Strlen( pointer str );
    static void DrawDemoRecording( void );
    static void DebugGraph( float32 value, sint color );
    static void DrawDebugGraph( void );
    static void Init( void );
    static void DrawScreenField( stereoFrame_t stereoFrame );
    static float32 ConsoleFontCharWidth( sint ch );
    static float32 ConsoleFontCharHeight( void );
    static float32 ConsoleFontStringWidth( pointer s, sint len );
};

extern idClientScreenSystemLocal clientScreenLocal;

#endif // !__CLIENTSCREEN_LOCAL_H__