////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2020 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   clientAVI.h
// Version:     v1.01
// Created:
// Compilers:   Microsoft Visual C++ 2019, gcc (Ubuntu 8.3.0-6ubuntu1) 8.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTSCREEN_LOCAL_H__
#define __CLIENTSCREEN_LOCAL_H__

typedef struct
{
    S32 color;
    F32 value;
} graphsamp_t;

static S32 current;
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
    
    static void AdjustFrom640( F32* x, F32* y, F32* w, F32* h );
    static void FillRect( F32 x, F32 y, F32 width, F32 height, const F32* color );
    static void DrawChar( S32 x, S32 y, F32 size, S32 ch );
    static void DrawConsoleFontChar( F32 x, F32 y, S32 ch );
    static void DrawSmallChar( S32 x, S32 y, S32 ch );
    static void DrawStringExt( S32 x, S32 y, F32 size, StringEntry string, F32* setColor, bool forceColor, bool noColorEscape );
    static void DrawBigString( S32 x, S32 y, StringEntry s, F32 alpha, bool noColorEscape );
    static void DrawSmallStringExt( S32 x, S32 y, StringEntry string, F32* setColor, bool forceColor, bool noColorEscape );
    static S32 Strlen( StringEntry str );
    static void DrawDemoRecording( void );
    static void DebugGraph( F32 value, S32 color );
    static void DrawDebugGraph( void );
    static void Init( void );
    static void DrawScreenField( stereoFrame_t stereoFrame );
    static F32 ConsoleFontCharWidth( S32 ch );
    static F32 ConsoleFontCharHeight( void );
    static F32 ConsoleFontStringWidth( StringEntry s, S32 len );
};

extern idClientScreenSystemLocal clientScreenLocal;

#endif // !__CLIENTSCREEN_LOCAL_H__