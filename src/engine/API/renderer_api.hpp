////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   renderer_api.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_PUBLIC_H__
#define __R_PUBLIC_H__

// AVI files have the start of pixel lines 4 byte-aligned
#define AVI_LINE_PADDING 4

// font support
#define GLYPH_START 0
#define GLYPH_END 255
#define GLYPH_CHARSTART 32
#define GLYPH_CHAREND 127
#define GLYPHS_PER_FONT (GLYPH_END - GLYPH_START + 1)

typedef struct
{
    sint height;       // number of scan lines
    sint top;          // top of glyph in buffer
    sint bottom;       // bottom of glyph in buffer
    sint pitch;        // width for copying
    sint xSkip;        // x adjustment
    sint imageWidth;   // width of actual image
    sint imageHeight;  // height of actual image
    float32 s;          // x offset in image where glyph starts
    float32 t;          // y offset in image where glyph starts
    float32 s2;
    float32 t2;
    qhandle_t glyph;  // handle to the shader with the glyph
    valueType shaderName[32];
} glyphInfo_t;

typedef struct
{
    glyphInfo_t glyphs [GLYPHS_PER_FONT];
    float32 glyphScale;
    valueType name[MAX_QPATH];
} fontInfo_t;

//
// idRenderSystem
//
class idRenderSystem
{
public:
    virtual void Shutdown( bool destroyWindow ) = 0;
    virtual void Init( vidconfig_t* config ) = 0;
    virtual qhandle_t RegisterModel( pointer name ) = 0;
    virtual qhandle_t RegisterSkin( pointer name ) = 0;
    virtual qhandle_t RegisterShader( pointer name ) = 0;
    virtual qhandle_t RegisterShaderNoMip( pointer name ) = 0;
    virtual void LoadWorld( pointer name ) = 0;
    virtual void SetWorldVisData( const uchar8* vis ) = 0;
    virtual void EndRegistration( void ) = 0;
    virtual void ClearScene( void ) = 0;
    virtual void AddRefEntityToScene( const refEntity_t* re ) = 0;
    virtual void AddPolyToScene( qhandle_t hShader, sint numVerts, const polyVert_t* verts, sint num ) = 0;
    virtual bool LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir ) = 0;
    virtual void AddLightToScene( const vec3_t org, float32 intensity, float32 r, float32 g, float32 b ) = 0;
    virtual void AddAdditiveLightToScene( const vec3_t org, float32 intensity, float32 r, float32 g, float32 b ) = 0;
    virtual void RenderScene( const refdef_t* fd ) = 0;
    virtual void SetColor( const float32* rgba ) = 0;
    virtual void SetClipRegion( const float32* region ) = 0;
    virtual void DrawStretchPic( float32 x, float32 y, float32 w, float32 h, float32 s1, float32 t1, float32 s2, float32 t2, qhandle_t hShader ) = 0;
    virtual void DrawStretchRaw( sint x, sint y, sint w, sint h, sint cols, sint rows, const uchar8* data, sint client, bool dirty ) = 0;
    virtual void UploadCinematic( sint w, sint h, sint cols, sint rows, const uchar8* data, sint client, bool dirty ) = 0;
    virtual void BeginFrame( stereoFrame_t stereoFrame ) = 0;
    virtual void EndFrame( sint* frontEndMsec, sint* backEndMsec ) = 0;
    virtual sint MarkFragments( sint numPoints, const vec3_t* points, const vec3_t projection, sint maxPoints, vec3_t pointBuffer, sint maxFragments, markFragment_t* fragmentBuffer ) = 0;
    virtual sint	LerpTag( orientation_t* tag,  qhandle_t model, sint startFrame, sint endFrame, float32 frac, pointer tagName ) = 0;
    virtual void ModelBounds( qhandle_t model, vec3_t mins, vec3_t maxs ) = 0;
    virtual void RegisterFont( pointer fontName, sint pointSize, fontInfo_t* font ) = 0;
    virtual void RemapShader( pointer oldShader, pointer newShader, pointer offsetTime ) = 0;
    virtual bool GetEntityToken( valueType* buffer, uint64 size ) = 0;
    virtual bool inPVS( const vec3_t p1, const vec3_t p2 ) = 0;
    virtual void TakeVideoFrame( sint h, sint w, uchar8* captureBuffer, uchar8* encodeBuffer, bool motionJpeg ) = 0;
};

extern idRenderSystem* renderSystem;

#endif	//!__R_PUBLIC_H__
