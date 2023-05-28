////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2023 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   clientScreen.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description: master for refresh, status bar, console, chat, notify, etc
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

// ready to draw
bool screenInitialized;

idClientScreenSystemLocal clientScreenLocal;
idClientScreenSystem *clientScreenSystem = &clientScreenLocal;

/*
===============
idClientScreenSystemLocal::idClientScreenSystemLocal
===============
*/
idClientScreenSystemLocal::idClientScreenSystemLocal(void) {
}

/*
===============
idClientScreenSystemLocal::~idClientScreenSystemLocal
===============
*/
idClientScreenSystemLocal::~idClientScreenSystemLocal(void) {
}

/*
================
idClientScreenSystemLocal::AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void idClientScreenSystemLocal::AdjustFrom640(float32 *x, float32 *y,
        float32 *w, float32 *h, scralign_t align) {
    float32 screenAspect;
    float32 xscale, lb_xscale, yscale, minscale, vertscale;
    float32 tmp_x, tmp_y, tmp_w, tmp_h, tmp_left,
            tmp_right;
    float32 xleft, xright;

    screenAspect = (float)cls.glconfig.vidWidth / (float)
                   cls.glconfig.vidHeight;

    // for eyefinity/surround setups, keep everything on the center monitor
    if(scr_surroundlayout && scr_surroundlayout->integer &&
            screenAspect >= 3.6f) {
        if(scr_surroundleft && scr_surroundleft->value > 0.0f &&
                scr_surroundleft->value < 1.0f) {
            xleft = (float)cls.glconfig.vidWidth * scr_surroundleft->value;
        } else {
            xleft = (float)cls.glconfig.vidWidth / 3.0f;
        }

        if(scr_surroundright && scr_surroundright->value > 0.0f &&
                scr_surroundright->value < 1.0f) {
            xright = (float)cls.glconfig.vidWidth * scr_surroundright->value;
        } else {
            xright = (float)cls.glconfig.vidWidth * (2.0f / 3.0f);
        }

        xscale = (xright - xleft) / SCREEN_WIDTH;
    } else {
        xleft = 0.0f;
        xright = (float)cls.glconfig.vidWidth;
        xscale = (float)cls.glconfig.vidWidth / SCREEN_WIDTH;
    }

    lb_xscale = (float)cls.glconfig.vidWidth / SCREEN_WIDTH;
    yscale = (float)cls.glconfig.vidHeight / SCREEN_HEIGHT;
    minscale = min(xscale, yscale);

    // hack for 5:4 modes
    if(!(xscale > yscale) && align != ALIGN_LETTERBOX) {
        align = ALIGN_STRETCH;
    }

    switch(align) {
        case ALIGN_CENTER:
            if(x) {
                tmp_x = *x;
                *x = (tmp_x - (0.5 * SCREEN_WIDTH)) * minscale + (0.5 *
                        cls.glconfig.vidWidth);
            }

            if(y) {
                tmp_y = *y;
                *y = (tmp_y - (0.5 * SCREEN_HEIGHT)) * minscale + (0.5 *
                        cls.glconfig.vidHeight);
            }

            if(w) {
                *w *= minscale;
            }

            if(h) {
                *h *= minscale;
            }

            break;

        case ALIGN_LETTERBOX:

            // special case: video mode (eyefinity?) is wider than object
            if(w != NULL && h != NULL &&
                    ((float)cls.glconfig.vidWidth / (float)cls.glconfig.vidHeight > *w / *h)) {
                tmp_h = *h;
                vertscale = cls.glconfig.vidHeight / tmp_h;

                if(x != NULL && w != NULL) {
                    tmp_x = *x;
                    tmp_w = *w;
                    *x = tmp_x * lb_xscale - (0.5 * (tmp_w * vertscale - tmp_w * lb_xscale));
                }

                if(y) {
                    *y = 0;
                }

                if(w) {
                    *w *= vertscale;
                }

                if(h) {
                    *h *= vertscale;
                }
            } else {
                if(x) {
                    *x *= xscale;
                }

                if(y != NULL && h != NULL) {
                    tmp_y = *y;
                    tmp_h = *h;
                    *y = tmp_y * yscale - (0.5 * (tmp_h * xscale - tmp_h * yscale));
                }

                if(w) {
                    *w *= xscale;
                }

                if(h) {
                    *h *= xscale;
                }
            }

            break;

        case ALIGN_TOP:
            if(w) {
                *w *= minscale;
            }

            if(h) {
                *h *= minscale;
            }

            if(x) {
                tmp_x = *x;
                *x = (tmp_x - (0.5 * SCREEN_WIDTH)) * minscale + (0.5 *
                        cls.glconfig.vidWidth);
            }

            if(y) {
                *y *= minscale;
            }

            break;

        case ALIGN_BOTTOM:
            if(w) {
                *w *= minscale;
            }

            if(h) {
                *h *= minscale;
            }

            if(x) {
                tmp_x = *x;
                *x = (tmp_x - (0.5 * SCREEN_WIDTH)) * minscale + (0.5 *
                        cls.glconfig.vidWidth);
            }

            if(y) {
                tmp_y = *y;
                *y = (tmp_y - SCREEN_HEIGHT) * minscale + cls.glconfig.vidHeight;
            }

            break;

        case ALIGN_RIGHT:
            if(w) {
                *w *= minscale;
            }

            if(h) {
                *h *= minscale;
            }

            if(x) {
                tmp_x = *x;
                *x = (tmp_x - SCREEN_WIDTH) * minscale + xright;
            }

            if(y) {
                tmp_y = *y;
                *y = (tmp_y - (0.5 * SCREEN_HEIGHT)) * minscale + (0.5 *
                        cls.glconfig.vidHeight);
            }

            break;

        case ALIGN_LEFT:
            if(w) {
                *w *= minscale;
            }

            if(h) {
                *h *= minscale;
            }

            if(x) {
                *x *= minscale + xleft;
            }

            if(y) {
                tmp_y = *y;
                *y = (tmp_y - (0.5 * SCREEN_HEIGHT)) * minscale + (0.5 *
                        cls.glconfig.vidHeight);
            }

            break;

        case ALIGN_TOPRIGHT:
            if(w) {
                *w *= minscale;
            }

            if(h) {
                *h *= minscale;
            }

            if(x) {
                tmp_x = *x;
                *x = (tmp_x - SCREEN_WIDTH) * minscale + xright;
            }

            if(y) {
                *y *= minscale;
            }

            break;

        case ALIGN_TOPLEFT:
            if(w) {
                *w *= minscale;
            }

            if(h) {
                *h *= minscale;
            }

            if(x) {
                tmp_x = *x;
                *x = tmp_x * minscale + xleft;
            }

            if(y) {
                *y *= minscale;
            }

            break;

        case ALIGN_BOTTOMRIGHT:
            if(w) {
                *w *= minscale;
            }

            if(h) {
                *h *= minscale;
            }

            if(x) {
                tmp_x = *x;
                *x = (tmp_x - SCREEN_WIDTH) * minscale + xright;
            }

            if(y) {
                tmp_y = *y;
                *y = (tmp_y - SCREEN_HEIGHT) * minscale + cls.glconfig.vidHeight;
            }

            break;

        case ALIGN_BOTTOMLEFT:
            if(w) {
                *w *= minscale;
            }

            if(h) {
                *h *= minscale;
            }

            if(x) {
                tmp_x = *x;
                *x = tmp_x * minscale + xleft;
            }

            if(y) {
                tmp_y = *y;
                *y = (tmp_y - SCREEN_HEIGHT) * minscale + cls.glconfig.vidHeight;
            }

            break;

        case ALIGN_TOP_STRETCH:
            if(w) {
                *w *= xscale;
            }

            if(h) {
                *h *= minscale;
            }

            if(x) {
                tmp_x = *x;
                *x = tmp_x * xscale + xleft;
            }

            if(y) {
                *y *= minscale;
            }

            break;

        case ALIGN_BOTTOM_STRETCH:
            if(w) {
                *w *= xscale;
            }

            if(h) {
                *h *= minscale;
            }

            if(x) {
                tmp_x = *x;
                *x = tmp_x * xscale + xleft;
            }

            if(y) {
                tmp_y = *y;
                *y = (tmp_y - SCREEN_HEIGHT) * minscale + cls.glconfig.vidHeight;
            }

            break;

        case ALIGN_STRETCH_ALL:
            if(x) {
                *x *= lb_xscale;
            }

            if(y) {
                *y *= yscale;
            }

            if(w) {
                *w *= lb_xscale;
            }

            if(h) {
                *h *= yscale;
            }

            break;

        case ALIGN_STRETCH_LEFT_CENTER:
            if(x && w) {
                tmp_x = *x;
                tmp_w = *w;
                tmp_left = tmp_x * xscale + xleft;
                tmp_right = (tmp_x + tmp_w - (0.5 * SCREEN_WIDTH)) * minscale + (0.5 *
                            (cls.glconfig.vidWidth));
                *x = tmp_left;
                *w = tmp_right - tmp_left;
            }

            if(y) {
                *y *= minscale;
            }

            if(h) {
                *h *= minscale;
            }

            break;

        case ALIGN_STRETCH_RIGHT_CENTER:
            if(x && w) {
                tmp_x = *x;
                tmp_w = *w;
                tmp_left = (tmp_x - (0.5 * SCREEN_WIDTH)) * minscale + (0.5 *
                           (cls.glconfig.vidWidth));
                tmp_right = (tmp_x + tmp_w - SCREEN_WIDTH) * xscale + xright;
                *x = tmp_left;
                *w = tmp_right - tmp_left;
            }

            if(y) {
                *y *= minscale;
            }

            if(h) {
                *h *= minscale;
            }

            break;

        case ALIGN_STRETCH:
        default:
            if(x) {
                tmp_x = *x;
                *x = tmp_x * xscale + xleft;
            }

            if(y) {
                *y *= yscale;
            }

            if(w) {
                *w *= xscale;
            }

            if(h) {
                *h *= yscale;
            }

            break;
    }
}

/*
================
idClientScreenSystemLocal::DrawPic

Coordinates are 640*480 virtual values
=================
*/
void idClientScreenSystemLocal::DrawPic(float32 x, float32 y,
                                        float32 width, float32 height, qhandle_t hShader) {
    AdjustFrom640(&x, &y, &width, &height, ALIGN_STRETCH);
    renderSystem->DrawStretchPic(x, y, width, height, 0, 0, 1, 1, hShader);
}

/*
================
idClientScreenSystemLocal::FillRect

Coordinates are 640*480 virtual values
=================
*/
void idClientScreenSystemLocal::FillRect(float32 x, float32 y,
        float32 width, float32 height, const float32 *color) {
    renderSystem->SetColor(color);

    AdjustFrom640(&x, &y, &width, &height, ALIGN_STRETCH);
    renderSystem->DrawStretchPic(x, y, width, height, 0, 0, 0, 0,
                                 cls.whiteShader);

    renderSystem->SetColor(nullptr);
}

/*
==================
idClientScreenSystemLocal::DrawChar

chars are drawn at 640*480 virtual screen size
==================
*/
void idClientScreenSystemLocal::DrawChar(sint x, sint y, float32 size,
        sint ch) {
    sint row, col;
    float32 frow, fcol, ax, ay, aw, ah;

    ch &= 255;

    if(ch == ' ') {
        return;
    }

    if(y < -size) {
        return;
    }

    ax = static_cast<float32>(x);
    ay = static_cast<float32>(y);
    aw = size;
    ah = size;

    AdjustFrom640(&ax, &ay, &aw, &ah, ALIGN_STRETCH);

    row = ch >> 4;
    col = ch & 15;

    frow = row * 0.0625f;
    fcol = col * 0.0625f;
    size = 0.0625;

    renderSystem->DrawStretchPic(ax, ay, aw, ah, fcol, frow, fcol + size,
                                 frow + size, cls.charSetShader);
}

/*
==================
idClientScreenSystemLocal::DrawConsoleFontChar
==================
*/
void idClientScreenSystemLocal::DrawConsoleFontChar(float32 x, float32 y,
        sint ch) {
    fontInfo_t *font = &cls.consoleFont;
    glyphInfo_t *glyph = &font->glyphs[ch];
    float32 yadj = static_cast<float32>(glyph->top);
    float32 xadj = (ConsoleFontCharWidth(ch) - glyph->xSkip) / 2.0f;

    if(cls.useLegacyConsoleFont) {
        DrawSmallChar(static_cast<sint>(x), static_cast<sint>(y), ch);
        return;
    }

    if(ch == ' ') {
        return;
    }

    renderSystem->DrawStretchPic(x + xadj, y - yadj,
                                 static_cast<float32>(glyph->imageWidth),
                                 static_cast<float32>(glyph->imageHeight), glyph->s, glyph->t, glyph->s2,
                                 glyph->t2, glyph->glyph);
}

/*
==================
idClientScreenSystemLocal::DrawSmallChar

small chars are drawn at native screen resolution
==================
*/
void idClientScreenSystemLocal::DrawSmallChar(sint x, sint y, sint ch) {
    sint row, col;
    float32 frow, fcol, size;

    ch &= 255;

    if(ch == ' ') {
        return;
    }

    if(y < -SMALLCHAR_HEIGHT) {
        return;
    }

    row = ch >> 4;
    col = ch & 15;

    frow = row * 0.0625f;
    fcol = col * 0.0625f;
    size = 0.0625f;

    renderSystem->DrawStretchPic(static_cast<float32>(x),
                                 static_cast<float32>(y), SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, fcol, frow,
                                 fcol + size, frow + size, cls.charSetShader);
}

/*
==================
idClientScreenSystemLocal::DrawStringExt

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void idClientScreenSystemLocal::DrawStringExt(sint x, sint y, float32 size,
        pointer string, float32 *setColor, bool forceColor, bool noColorEscape) {
    sint    xx;
    vec4_t  color;
    pointer s;
    bool    skip_color_string_check = false;

    // draw the drop shadow
    color[0] = color[1] = color[2] = 0;
    color[3] = setColor[3];
    renderSystem->SetColor(color);
    s = string;
    xx = x;

    while(*s) {
        if(!noColorEscape && Q_IsColorString(s)) {
            s += Q_ColorStringLength(s);
            continue;
        } else if(!noColorEscape && Q_IsColorEscapeEscape(s)) {
            s++;
        }

        DrawChar(xx + 1, y + 1, size, *s);

        xx += static_cast<sint>(size);
        s++;
    }


    // draw the colored text
    s = string;
    xx = x;

    renderSystem->SetColor(setColor);

    while(*s) {
        if(skip_color_string_check) {
            skip_color_string_check = false;
        } else if(Q_IsColorString(s)) {
            if(!forceColor) {
                if(Q_IsColorNULLString(s + 1)) {
                    ::memcpy(color, setColor, sizeof(color));
                } else {
                    if(Q_IsHardcodedColor(s)) {
                        ::memcpy(color, g_color_table[ColorIndex(*(s + 1))],
                                 sizeof(color));
                    } else {
                        Q_GetVectFromHexColor(s, color);
                    }

                    color[3] = setColor[3];
                }

                color[3] = setColor[3];

                renderSystem->SetColor(color);
            }

            if(!noColorEscape) {
                s += Q_ColorStringLength(s);
                continue;
            }
        } else if(Q_IsColorEscapeEscape(s)) {
            if(!noColorEscape) {
                s++;
            } else if(!forceColor) {
                skip_color_string_check = true;
            }
        }

        idClientScreenSystemLocal::DrawChar(xx, y, size, *s);

        xx += static_cast<sint>(size);
        s++;
    }

    renderSystem->SetColor(nullptr);
}

/*
==================
idClientScreenSystemLocal::DrawBigString
==================
*/
void idClientScreenSystemLocal::DrawBigString(sint x, sint y, pointer s,
        float32 alpha, bool noColorEscape) {
    float32 color[4];

    color[0] = color[1] = color[2] = 1.0;
    color[3] = alpha;

    DrawStringExt(x, y, BIGCHAR_WIDTH, s, color, false, noColorEscape);
}

/*
==================
idClientScreenSystemLocal::DrawSmallString

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void idClientScreenSystemLocal::DrawSmallStringExt(sint x, sint y,
        pointer string, float32 *setColor, bool forceColor, bool noColorEscape) {
    float32 xx;
    vec4_t  color;
    pointer s;
    bool    skip_color_string_check = false;

    // draw the colored text
    s = string;
    xx = static_cast<float32>(x);
    renderSystem->SetColor(setColor);

    while(*s) {
        if(skip_color_string_check) {
            skip_color_string_check = false;
        } else if(Q_IsColorString(s)) {
            if(!forceColor) {
                if(Q_IsColorNULLString(s + 1)) {
                    ::memcpy(color, setColor, sizeof(color));
                } else {
                    if(Q_IsHardcodedColor(s)) {
                        ::memcpy(color, g_color_table[ColorIndex(*(s + 1))],
                                 sizeof(color));
                    } else {
                        Q_GetVectFromHexColor(s, color);
                    }

                    color[3] = setColor[3];
                }

                color[3] = setColor[3];

                renderSystem->SetColor(color);
            }

            if(!noColorEscape) {
                s += Q_ColorStringLength(s);
                continue;
            }
        } else if(Q_IsColorEscapeEscape(s)) {
            if(!noColorEscape) {
                s++;
            } else if(!forceColor) {
                skip_color_string_check = true;
            }
        }

        DrawConsoleFontChar(xx, static_cast<float32>(y), *s);

        xx += ConsoleFontCharWidth(*s);
        s++;
    }

    renderSystem->SetColor(nullptr);
}

/*
==================
idClientScreenSystemLocal::Strlen

skips color escape codes
==================
*/
sint idClientScreenSystemLocal::Strlen(pointer str) {
    sint count = 0;
    pointer s = str;

    while(*s) {
        if(Q_IsColorString(s)) {
            s += Q_ColorStringLength(s);
        } else {
            if(Q_IsColorEscapeEscape(s)) {
                s++;
            }

            count++;
            s++;
        }
    }

    return count;
}

/*
=================
idClientScreenSystemLocal::DrawDemoRecording
=================
*/
void idClientScreenSystemLocal::DrawDemoRecording(void) {
    if(!clc.demorecording) {
        return;
    }

    cvarSystem->Set("cl_demooffset", va(nullptr, "%d",
                                        fileSystem->FTell(clc.demofile)));
}

/*
==============
idClientScreenSystemLocal::DebugGraph
==============
*/
void idClientScreenSystemLocal::DebugGraph(float32 value, sint color) {
    values[current & 1023].value = value;
    values[current & 1023].color = color;
    current++;
}

/*
==============
idClientScreenSystemLocal::DrawDebugGraph
==============
*/
void idClientScreenSystemLocal::DrawDebugGraph(void) {
    sint a, x, y, w, i, h, color;
    float32 v;

    // draw the graph
    w = cls.glconfig.vidWidth;
    x = 0;
    y = cls.glconfig.vidHeight;

    renderSystem->SetColor(g_color_table[ColorIndex(COLOR_BLACK)]);
    renderSystem->DrawStretchPic(static_cast<float32>(x),
                                 static_cast<float32>(y - graphheight->integer), static_cast<float32>(w),
                                 graphheight->value, 0, 0, 0, 0, cls.whiteShader);
    renderSystem->SetColor(nullptr);

    for(a = 0; a < w; a++) {
        i = (current - 1 - a + 1024) & 1023;
        v = values[i].value;
        color = values[i].color;
        v = v * graphscale->integer + graphshift->integer;

        if(v < 0) {
            v += graphheight->integer * (1 + static_cast<sint>
                                         (-v / graphheight->integer));
        }

        h = static_cast<sint>(v) % graphheight->integer;

        renderSystem->DrawStretchPic(x + static_cast<float32>(w - 1 - a),
                                     static_cast<float32>(y - h), 1, static_cast<float32>(h), 0, 0, 0, 0,
                                     cls.whiteShader);
    }
}

/*
==================
idClientScreenSystemLocal::Init
==================
*/
void idClientScreenSystemLocal::Init(void) {
    screenInitialized = true;
}

/*
==================
idClientScreenSystemLocal::DrawScreenField

This will be called twice if rendering in stereo mode
==================
*/
void idClientScreenSystemLocal::DrawScreenField(stereoFrame_t
        stereoFrame) {
    bool uiFullscreen;

    if(!cls.rendererStarted) {
        return;
    }

    renderSystem->BeginFrame(stereoFrame);

    uiFullscreen = (uivm && uiManager->IsFullscreen());

    // wide aspect ratio screens need to have the sides cleared
    // unless they are displaying game renderings
    if(cls.glconfig.vidWidth * 480 != cls.glconfig.vidHeight * 640) {
        sint clw;
        clw = 0.5f * (cls.glconfig.vidWidth - 640 * cls.glconfig.vidHeight /
                      480.0);
        renderSystem->SetColor(g_color_table[ColorIndex(COLOR_BLACK)]);
        renderSystem->DrawStretchPic(0, 0,
                                     static_cast<float32>(clw),
                                     static_cast<float32>(cls.glconfig.vidHeight), 0, 0, 0, 0, cls.whiteShader);
        renderSystem->DrawStretchPic(cls.glconfig.vidWidth - clw, 0, clw,
                                     cls.glconfig.vidHeight, 0, 0, 0, 0, cls.whiteShader);
        renderSystem->SetColor(nullptr);
    }

    // if the menu is going to cover the entire screen, we
    // don't need to render anything under it
    if(uivm && !uiFullscreen) {
        switch(cls.state) {
            default:
                common->Error(ERR_FATAL,
                              "idClientScreenSystemLocal::DrawScreenField: bad cls.state");
                break;

            case CA_CINEMATIC:
                clientCinemaSystem->DrawCinematic();
                break;

            case CA_DISCONNECTED:
                // force menu up
                soundSystem->StopAllSounds();

                if(uivm) {
                    uiManager->SetActiveMenu(UIMENU_MAIN);
                }

                break;

            case CA_CONNECTING:
            case CA_CHALLENGING:
            case CA_CONNECTED:
                // connecting clients will only show the connection dialog
                // refresh to update the time
                uiManager->Refresh(cls.realtime);
                uiManager->DrawConnectScreen(false);
                break;

            case CA_LOADING:
            case CA_PRIMED:
                // draw the game information screen and loading progress
                clientGameSystem->CGameRendering(stereoFrame);

                // also draw the connection information, so it doesn't
                // flash away too briefly on local or lan games
                uiManager->Refresh(cls.realtime);
                uiManager->DrawConnectScreen(true);
                break;

            case CA_ACTIVE:
                clientGameSystem->CGameRendering(stereoFrame);

                DrawDemoRecording();
                break;
        }
    }

    // the menu draws next
    if(cls.keyCatchers & KEYCATCH_UI && uivm) {
        uiManager->Refresh(cls.realtime);
    }

    // console draws next
    clientConsoleSystem->DrawConsole();

    // debug graph can be drawn on top of anything
    if(debuggraph->integer || timegraph->integer ||
            cl_debugMove->integer) {
        DrawDebugGraph();
    }
}

/*
==================
idClientScreenSystemLocal::UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void idClientScreenSystemLocal::UpdateScreen(void) {
    static sint recursive = 0;

    if(!screenInitialized) {
        // not initialized yet
        return;
    }

    if(++recursive >= 2) {
        recursive = 0;
        return;
    }

    recursive = 1;

    // If there is no VM, there are also no rendering commands issued. Stop the renderer in that case.
    if(cls.glconfig.stereoEnabled) {
        DrawScreenField(STEREO_LEFT);
        DrawScreenField(STEREO_RIGHT);
    } else {
        DrawScreenField(STEREO_CENTER);
    }

    if(com_speeds->integer) {
        renderSystem->EndFrame(&time_frontend, &time_backend);
    } else {
        renderSystem->EndFrame(nullptr, nullptr);
    }

    recursive = 0;
}

/*
==================
idClientScreenSystemLocal::ConsoleFontCharWidth
==================
*/
float32 idClientScreenSystemLocal::ConsoleFontCharWidth(sint ch) {
    fontInfo_t *font = &cls.consoleFont;
    glyphInfo_t *glyph = &font->glyphs[ch];
    float32 width = glyph->xSkip + cl_consoleFontKerning->value;

    if(cls.useLegacyConsoleFont) {
        return SMALLCHAR_WIDTH;
    }

    return (width);
}

/*
==================
idClientScreenSystemLocal::ConsoleFontCharHeight
==================
*/
float32 idClientScreenSystemLocal::ConsoleFontCharHeight(void) {
    sint ch = 'I' & 0xff;
    float32 vpadding = 0.3f * cl_consoleFontSize->value;
    fontInfo_t *font = &cls.consoleFont;
    glyphInfo_t *glyph = &font->glyphs[ch];

    if(cls.useLegacyConsoleFont) {
        return SMALLCHAR_HEIGHT;
    }

    return (glyph->imageHeight + vpadding);
}

/*
==================
idClientScreenSystemLocal::ConsoleFontStringWidth
==================
*/
float32 idClientScreenSystemLocal::ConsoleFontStringWidth(pointer s,
        sint len) {
    sint i;
    float32 width = 0;
    fontInfo_t *font = &cls.consoleFont;

    if(cls.useLegacyConsoleFont) {
        return static_cast<float32>(len) * SMALLCHAR_WIDTH;
    }

    for(i = 0; i < len; i++) {
        sint ch = s[i] & 0xff;
        glyphInfo_t *glyph = &font->glyphs[ch];
        width += glyph->xSkip + cl_consoleFontKerning->value;
    }

    return (width);
}
