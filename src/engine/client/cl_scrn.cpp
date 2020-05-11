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
// File name:   cl_scrn.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: master for refresh, status bar, console, chat, notify, etc
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.h>

bool scr_initialized;	// ready to draw

convar_t*         cl_timegraph;
convar_t*         cl_debuggraph;
convar_t*         cl_graphheight;
convar_t*         cl_graphscale;
convar_t*         cl_graphshift;

/*
================
SCR_DrawNamedPic

Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawNamedPic( F32 x, F32 y, F32 width, F32 height, StringEntry picname )
{
    qhandle_t       hShader;
    
    assert( width != 0 );
    
    hShader = renderSystem->RegisterShader( picname );
    SCR_AdjustFrom640( &x, &y, &width, &height );
    renderSystem->DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}


/*
================
SCR_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void SCR_AdjustFrom640( F32* x, F32* y, F32* w, F32* h )
{
    F32           xscale;
    F32           yscale;
    
#if 0
    // adjust for wide screens
    if( cls.glconfig.vidWidth * 480 > cls.glconfig.vidHeight * 640 )
    {
        *x += 0.5 * ( cls.glconfig.vidWidth - ( cls.glconfig.vidHeight * 640 / 480 ) );
    }
#endif
    
    // scale for screen sizes
    xscale = cls.glconfig.vidWidth / 640.0;
    yscale = cls.glconfig.vidHeight / 480.0;
    if( x )
    {
        *x *= xscale;
    }
    if( y )
    {
        *y *= yscale;
    }
    if( w )
    {
        *w *= xscale;
    }
    if( h )
    {
        *h *= yscale;
    }
}

/*
================
SCR_FillRect

Coordinates are 640*480 virtual values
=================
*/
void SCR_FillRect( F32 x, F32 y, F32 width, F32 height, const F32* color )
{
    renderSystem->SetColor( color );
    
    SCR_AdjustFrom640( &x, &y, &width, &height );
    renderSystem->DrawStretchPic( x, y, width, height, 0, 0, 0, 0, cls.whiteShader );
    
    renderSystem->SetColor( NULL );
}

/*
================
SCR_DrawPic

Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawPic( F32 x, F32 y, F32 width, F32 height, qhandle_t hShader )
{
    SCR_AdjustFrom640( &x, &y, &width, &height );
    renderSystem->DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}

/*
** SCR_DrawChar
** chars are drawn at 640*480 virtual screen size
*/
static void SCR_DrawChar( S32 x, S32 y, F32 size, S32 ch )
{
    S32             row, col;
    F32           frow, fcol;
    F32           ax, ay, aw, ah;
    
    ch &= 255;
    
    if( ch == ' ' )
    {
        return;
    }
    
    if( y < -size )
    {
        return;
    }
    
    ax = x;
    ay = y;
    aw = size;
    ah = size;
    SCR_AdjustFrom640( &ax, &ay, &aw, &ah );
    
    row = ch >> 4;
    col = ch & 15;
    
    frow = row * 0.0625;
    fcol = col * 0.0625;
    size = 0.0625;
    
    renderSystem->DrawStretchPic( ax, ay, aw, ah,
                                  fcol, frow,
                                  fcol + size, frow + size,
                                  cls.charSetShader );
}

void SCR_DrawConsoleFontChar( F32 x, F32 y, S32 ch )
{
    fontInfo_t* font = &cls.consoleFont;
    glyphInfo_t* glyph = &font->glyphs[ch];
    F32 yadj = glyph->top;
    F32 xadj = ( SCR_ConsoleFontCharWidth( ch ) - glyph->xSkip ) / 2.0;
    
    if( cls.useLegacyConsoleFont )
    {
        SCR_DrawSmallChar( ( S32 ) x, ( S32 ) y, ch );
        return;
    }
    
    if( ch == ' ' ) return;
    renderSystem->DrawStretchPic( x + xadj, y - yadj, glyph->imageWidth, glyph->imageHeight,
                                  glyph->s, glyph->t,
                                  glyph->s2, glyph->t2,
                                  glyph->glyph );
}

/*
** SCR_DrawSmallChar
** small chars are drawn at native screen resolution
*/
void SCR_DrawSmallChar( S32 x, S32 y, S32 ch )
{
    S32             row, col;
    F32           frow, fcol;
    F32           size;
    
    ch &= 255;
    
    if( ch == ' ' )
    {
        return;
    }
    
    if( y < -SMALLCHAR_HEIGHT )
    {
        return;
    }
    
    row = ch >> 4;
    col = ch & 15;
    
    frow = row * 0.0625;
    fcol = col * 0.0625;
    size = 0.0625;
    
    renderSystem->DrawStretchPic( x, y, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, fcol, frow, fcol + size, frow + size, cls.charSetShader );
}


/*
==================
SCR_DrawBigString[Color]

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void SCR_DrawStringExt( S32 x, S32 y, F32 size, StringEntry string, F32* setColor, bool forceColor, bool noColorEscape )
{
    vec4_t          color;
    StringEntry     s;
    S32             xx;
    
    // draw the drop shadow
    color[0] = color[1] = color[2] = 0;
    color[3] = setColor[3];
    renderSystem->SetColor( color );
    s = string;
    xx = x;
    while( *s )
    {
        if( !noColorEscape && Q_IsColorString( s ) )
        {
            s += 2;
            continue;
        }
        SCR_DrawChar( xx + 2, y + 2, size, *s );
        xx += size;
        s++;
    }
    
    
    // draw the colored text
    s = string;
    xx = x;
    renderSystem->SetColor( setColor );
    while( *s )
    {
        if( !noColorEscape && Q_IsColorString( s ) )
        {
            if( !forceColor )
            {
                if( *( s + 1 ) == COLOR_NULL )
                {
                    memcpy( color, setColor, sizeof( color ) );
                }
                else
                {
                    memcpy( color, g_color_table[ColorIndex( *( s + 1 ) )], sizeof( color ) );
                    color[3] = setColor[3];
                }
                color[3] = setColor[3];
                renderSystem->SetColor( color );
            }
            s += 2;
            continue;
        }
        SCR_DrawChar( xx, y, size, *s );
        xx += size;
        s++;
    }
    renderSystem->SetColor( NULL );
}


void SCR_DrawBigString( S32 x, S32 y, StringEntry s, F32 alpha, bool noColorEscape )
{
    F32           color[4];
    
    color[0] = color[1] = color[2] = 1.0;
    color[3] = alpha;
    SCR_DrawStringExt( x, y, BIGCHAR_WIDTH, s, color, false, noColorEscape );
}

void SCR_DrawBigStringColor( S32 x, S32 y, StringEntry s, vec4_t color, bool noColorEscape )
{
    SCR_DrawStringExt( x, y, BIGCHAR_WIDTH, s, color, true, noColorEscape );
}


/*
==================
SCR_DrawSmallString[Color]

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void SCR_DrawSmallStringExt( S32 x, S32 y, StringEntry string, F32* setColor, bool forceColor, bool noColorEscape )
{
    vec4_t          color;
    StringEntry     s;
    F32             xx;
    
    // draw the colored text
    s = string;
    xx = x;
    renderSystem->SetColor( setColor );
    while( *s )
    {
        if( Q_IsColorString( s ) )
        {
            if( !forceColor )
            {
                if( *( s + 1 ) == COLOR_NULL )
                {
                    memcpy( color, setColor, sizeof( color ) );
                }
                else
                {
                    memcpy( color, g_color_table[ColorIndex( *( s + 1 ) )], sizeof( color ) );
                    color[3] = setColor[3];
                }
                renderSystem->SetColor( color );
            }
            if( !noColorEscape )
            {
                s += 2;
                continue;
            }
        }
        SCR_DrawConsoleFontChar( xx, y, *s );
        xx += SCR_ConsoleFontCharWidth( *s );
        s++;
    }
    renderSystem->SetColor( NULL );
}



/*
** SCR_Strlen -- skips color escape codes
*/
static S32 SCR_Strlen( StringEntry str )
{
    StringEntry     s = str;
    S32             count = 0;
    
    while( *s )
    {
        if( Q_IsColorString( s ) )
        {
            s += 2;
        }
        else
        {
            count++;
            s++;
        }
    }
    
    return count;
}

/*
** SCR_GetBigStringWidth
*/
S32 SCR_GetBigStringWidth( StringEntry str )
{
    return SCR_Strlen( str ) * 16;
}


//===============================================================================

/*
=================
SCR_DrawDemoRecording
=================
*/
void SCR_DrawDemoRecording( void )
{
    if( !clc.demorecording )
    {
        return;
    }
    
    //bani
    cvarSystem->Set( "cl_demooffset", va( "%d", fileSystem->FTell( clc.demofile ) ) );
}

/*
===============================================================================

DEBUG GRAPH

===============================================================================
*/

typedef struct
{
    F32           value;
    S32             color;
} graphsamp_t;

static S32      current;
static graphsamp_t values[1024];

/*
==============
SCR_DebugGraph
==============
*/
void SCR_DebugGraph( F32 value, S32 color )
{
    values[current & 1023].value = value;
    values[current & 1023].color = color;
    current++;
}

/*
==============
SCR_DrawDebugGraph
==============
*/
void SCR_DrawDebugGraph( void )
{
    S32             a, x, y, w, i, h;
    F32           v;
    S32             color;
    
    //
    // draw the graph
    //
    w = cls.glconfig.vidWidth;
    x = 0;
    y = cls.glconfig.vidHeight;
    renderSystem->SetColor( g_color_table[0] );
    renderSystem->DrawStretchPic( x, y - cl_graphheight->integer, w, cl_graphheight->integer, 0, 0, 0, 0, cls.whiteShader );
    renderSystem->SetColor( NULL );
    
    for( a = 0; a < w; a++ )
    {
        i = ( current - 1 - a + 1024 ) & 1023;
        v = values[i].value;
        color = values[i].color;
        v = v * cl_graphscale->integer + cl_graphshift->integer;
        
        if( v < 0 )
        {
            v += cl_graphheight->integer * ( 1 + ( S32 )( -v / cl_graphheight->integer ) );
        }
        h = ( S32 )v % cl_graphheight->integer;
        renderSystem->DrawStretchPic( x + w - 1 - a, y - h, 1, h, 0, 0, 0, 0, cls.whiteShader );
    }
}

//=============================================================================

/*
==================
SCR_Init
==================
*/
void SCR_Init( void )
{
    cl_timegraph = cvarSystem->Get( "timegraph", "0", CVAR_CHEAT, "description" );
    cl_debuggraph = cvarSystem->Get( "debuggraph", "0", CVAR_CHEAT, "description" );
    cl_graphheight = cvarSystem->Get( "graphheight", "32", CVAR_CHEAT, "description" );
    cl_graphscale = cvarSystem->Get( "graphscale", "1", CVAR_CHEAT, "description" );
    cl_graphshift = cvarSystem->Get( "graphshift", "0", CVAR_CHEAT, "description" );
    
    scr_initialized = true;
}


//=======================================================

/*
==================
SCR_DrawScreenField

This will be called twice if rendering in stereo mode
==================
*/
void SCR_DrawScreenField( stereoFrame_t stereoFrame )
{
    renderSystem->BeginFrame( stereoFrame );
    
    // wide aspect ratio screens need to have the sides cleared
    // unless they are displaying game renderings
    if( cls.state != CA_ACTIVE && cls.state != CA_CINEMATIC )
    {
        if( cls.glconfig.vidWidth * 480 > cls.glconfig.vidHeight * 640 )
        {
            renderSystem->SetColor( g_color_table[0] );
            renderSystem->DrawStretchPic( 0, 0, cls.glconfig.vidWidth, cls.glconfig.vidHeight, 0, 0, 0, 0, cls.whiteShader );
            renderSystem->SetColor( NULL );
        }
    }
    
    if( !uivm )
    {
        Com_DPrintf( "draw screen without UI loaded\n" );
        return;
    }
    
    // if the menu is going to cover the entire screen, we
    // don't need to render anything under it
    if( !uiManager->IsFullscreen() || ( !( cls.framecount & 7 ) && cls.state == CA_ACTIVE ) )
    {
        switch( cls.state )
        {
            default:
                Com_Error( ERR_FATAL, "SCR_DrawScreenField: bad cls.state" );
                break;
            case CA_CINEMATIC:
                SCR_DrawCinematic();
                break;
            case CA_DISCONNECTED:
                // force menu up
                soundSystem->StopAllSounds();
                uiManager->SetActiveMenu( UIMENU_MAIN );
                break;
            case CA_CONNECTING:
            case CA_CHALLENGING:
            case CA_CONNECTED:
                // connecting clients will only show the connection dialog
                // refresh to update the time
                uiManager->Refresh( cls.realtime );
                uiManager->DrawConnectScreen( false );
                break;
                // Ridah, if the cgame is valid, fall through to there
                if( !cls.cgameStarted || !com_sv_running->integer )
                {
                    // connecting clients will only show the connection dialog
                    uiManager->DrawConnectScreen( false );
                    break;
                }
            case CA_LOADING:
            case CA_PRIMED:
                // also draw the connection information, so it doesn't
                // flash away too briefly on local or lan games
                //if (!com_sv_running->value || cvarSystem->VariableIntegerValue("sv_cheats"))	// Ridah, don't draw useless text if not in dev mode
                uiManager->Refresh( cls.realtime );
                uiManager->DrawConnectScreen( true );
                
                // draw the game information screen and loading progress
                if( cgvm )
                {
                    clientGameSystem->CGameRendering( stereoFrame );
                }
                
                break;
            case CA_ACTIVE:
                clientGameSystem->CGameRendering( stereoFrame );
                SCR_DrawDemoRecording();
                break;
        }
    }
    
    // the menu draws next
    if( cls.keyCatchers & KEYCATCH_UI && uivm )
    {
        uiManager->Refresh( cls.realtime );
    }
    
    // console draws next
    Con_DrawConsole();
    
    // debug graph can be drawn on top of anything
    if( cl_debuggraph->integer || cl_timegraph->integer || cl_debugMove->integer )
    {
        SCR_DrawDebugGraph();
    }
}

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void SCR_UpdateScreen( void )
{
    static S32      recursive = 0;
    
    if( !scr_initialized )
    {
        return;					// not initialized yet
    }
    
    if( ++recursive >= 2 )
    {
        recursive = 0;
        //Com_Error( ERR_FATAL, "SCR_UpdateScreen: recursively called" );
    }
    recursive = 1;
    
    // If there is no VM, there are also no rendering commands issued. Stop the renderer in
    // that case.
    if( cls.glconfig.stereoEnabled )
    {
        SCR_DrawScreenField( STEREO_LEFT );
        SCR_DrawScreenField( STEREO_RIGHT );
    }
    else
    {
        SCR_DrawScreenField( STEREO_CENTER );
    }
    
    if( com_speeds->integer )
    {
        renderSystem->EndFrame( &time_frontend, &time_backend );
    }
    else
    {
        renderSystem->EndFrame( NULL, NULL );
    }
    
    recursive = 0;
}

F32 SCR_ConsoleFontCharWidth( S32 ch )
{
    fontInfo_t* font = &cls.consoleFont;
    glyphInfo_t* glyph = &font->glyphs[ch];
    F32 width = glyph->xSkip + cl_consoleFontKerning->value;
    
    if( cls.useLegacyConsoleFont )
    {
        return SMALLCHAR_WIDTH;
    }
    
    return ( width );
}


F32 SCR_ConsoleFontCharHeight( )
{
    fontInfo_t* font = &cls.consoleFont;
    S32 ch = 'I' & 0xff;
    glyphInfo_t* glyph = &font->glyphs[ch];
    F32 vpadding = 0.3 * cl_consoleFontSize->value;
    
    if( cls.useLegacyConsoleFont )
    {
        return SMALLCHAR_HEIGHT;
    }
    
    return ( glyph->imageHeight + vpadding );
}


F32 SCR_ConsoleFontStringWidth( StringEntry s, S32 len )
{
    S32 i;
    fontInfo_t* font = &cls.consoleFont;
    F32 width = 0;
    
    if( cls.useLegacyConsoleFont )
    {
        return len * SMALLCHAR_WIDTH;
    }
    
    for( i = 0; i < len; i++ )
    {
        S32 ch = s[i] & 0xff;
        glyphInfo_t* glyph = &font->glyphs[ch];
        width += glyph->xSkip + cl_consoleFontKerning->value;
    }
    return ( width );
}

