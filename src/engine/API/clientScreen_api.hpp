////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2020 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// along with OpenWolf Source Code. If not, see <http://www.gnu.org/licenses/>.
//
// -------------------------------------------------------------------------------------
// File name:   clientScreen_api.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTSCREEN_API_H__
#define __CLIENTSCREEN_API_H__

//
// idClientScreenSystem
//
class idClientScreenSystem {
public:

    virtual void UpdateScreen(void) = 0;
    virtual void DrawSmallStringExt(sint x, sint y, pointer string,
                                    float32 *setColor, bool forceColor, bool noColorEscape) = 0;
    virtual void DrawBigString(sint x, sint y, pointer s, float32 alpha,
                               bool noColorEscape) = 0;
    virtual float32 ConsoleFontStringWidth(pointer s, sint len) = 0;
    virtual void DrawConsoleFontChar(float32 x, float32 y, sint ch) = 0;
    virtual void AdjustFrom640(float32 *x, float32 *y, float32 *w,
                               float32 *h) = 0;
    virtual void DrawPic(float32 x, float32 y, float32 width, float32 height,
                         qhandle_t hShader) = 0;
    virtual void FillRect(float32 x, float32 y, float32 width, float32 height,
                          const float32 *color) = 0;
    virtual void DrawChar(sint x, sint y, float32 size, sint ch) = 0;
    virtual void DrawSmallChar(sint x, sint y, sint ch) = 0;
    virtual void DrawStringExt(sint x, sint y, float32 size, pointer string,
                               float32 *setColor, bool forceColor, bool noColorEscape) = 0;
    virtual sint Strlen(pointer str) = 0;
    virtual void DrawDemoRecording(void) = 0;
    virtual void DebugGraph(float32 value, sint color) = 0;
    virtual void DrawDebugGraph(void) = 0;
    virtual void Init(void) = 0;
    virtual void DrawScreenField(stereoFrame_t stereoFrame) = 0;
    virtual float32 ConsoleFontCharWidth(sint ch) = 0;
    virtual float32 ConsoleFontCharHeight(void) = 0;
};

extern idClientScreenSystem *clientScreenSystem;

#endif // !__CLIENTSCREEN_API_H__
