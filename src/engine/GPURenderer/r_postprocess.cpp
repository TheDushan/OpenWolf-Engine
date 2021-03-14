////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011 Andrei Drexler, Richard Allen, James Canete
// Copyright(C) 2011 - 2021 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of OpenWolf.
//
// OpenWolf is free software; you can redistribute it
// and / or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3 of the License,
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
// File name:   r_postprocess.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

void RB_ToneMap( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox, sint autoExposure )
{
    ivec4_t srcBox, dstBox;
    vec4_t color;
    static sint lastFrameCount = 0;
    
    if( autoExposure )
    {
        if( lastFrameCount == 0 || tr.frameCount < lastFrameCount || tr.frameCount - lastFrameCount > 5 )
        {
            // determine average log luminance
            FBO_t* srcFbo, *dstFbo, *tmp;
            sint size = 256;
            
            lastFrameCount = tr.frameCount;
            
            VectorSet4( dstBox, 0, 0, size, size );
            
            FBO_Blit( hdrFbo, hdrBox, nullptr, tr.textureScratchFbo[0], dstBox, &tr.calclevels4xShader[0], nullptr, 0 );
            
            srcFbo = tr.textureScratchFbo[0];
            dstFbo = tr.textureScratchFbo[1];
            
            // downscale to 1x1 texture
            while( size > 1 )
            {
                VectorSet4( srcBox, 0, 0, size, size );
                //size >>= 2;
                size >>= 1;
                VectorSet4( dstBox, 0, 0, size, size );
                
                if( size == 1 )
                    dstFbo = tr.targetLevelsFbo;
                    
                //FBO_Blit(targetFbo, srcBox, nullptr, tr.textureScratchFbo[nextScratch], dstBox, &tr.calclevels4xShader[1], nullptr, 0);
                FBO_FastBlit( srcFbo, srcBox, dstFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_LINEAR );
                
                tmp = srcFbo;
                srcFbo = dstFbo;
                dstFbo = tmp;
            }
        }
        
        // blend with old log luminance for gradual change
        VectorSet4( srcBox, 0, 0, 0, 0 );
        
        color[0] =
            color[1] =
                color[2] = 1.0f;
        if( glRefConfig.textureFloat )
            color[3] = 0.03f;
        else
            color[3] = 0.1f;
            
        FBO_Blit( tr.targetLevelsFbo, srcBox, nullptr, tr.calcLevelsFbo, nullptr,  nullptr, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
    }
    
    // tonemap
    color[0] =
        color[1] =
            color[2] = pow( 2, r_cameraExposure->value ); //exp2(r_cameraExposure->value);
    color[3] = 1.0f;
    
    if( autoExposure )
        GL_BindToTMU( tr.calcLevelsImage,  TB_LEVELSMAP );
    else
        GL_BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
        
    FBO_Blit( hdrFbo, hdrBox, nullptr, ldrFbo, ldrBox, &tr.tonemapShader, color, 0 );
}

/*
=============
RB_BokehBlur


Blurs a part of one framebuffer to another.

Framebuffers can be identical.
=============
*/
void RB_BokehBlur( FBO_t* src, ivec4_t srcBox, FBO_t* dst, ivec4_t dstBox, float32 blur )
{
//	ivec4_t srcBox, dstBox;
    vec4_t color;
    
    blur *= 10.0f;
    
    if( blur < 0.004f )
        return;
        
    if( glRefConfig.framebufferObject )
    {
        // bokeh blur
        if( blur > 0.0f )
        {
            ivec4_t quarterBox;
            
            quarterBox[0] = 0;
            quarterBox[1] = tr.quarterFbo[0]->height;
            quarterBox[2] = tr.quarterFbo[0]->width;
            quarterBox[3] = -tr.quarterFbo[0]->height;
            
            // create a quarter texture
            //FBO_Blit(nullptr, nullptr, nullptr, tr.quarterFbo[0], nullptr, nullptr, nullptr, 0);
            FBO_FastBlit( src, srcBox, tr.quarterFbo[0], quarterBox, GL_COLOR_BUFFER_BIT, GL_LINEAR );
        }
        
#ifndef HQ_BLUR
        if( blur > 1.0f )
        {
            // create a 1/16th texture
            //FBO_Blit(tr.quarterFbo[0], nullptr, nullptr, tr.textureScratchFbo[0], nullptr, nullptr, nullptr, 0);
            FBO_FastBlit( tr.quarterFbo[0], nullptr, tr.textureScratchFbo[0], nullptr, GL_COLOR_BUFFER_BIT, GL_LINEAR );
        }
#endif
        
        if( blur > 0.0f && blur <= 1.0f )
        {
            // Crossfade original with quarter texture
            VectorSet4( color, 1, 1, 1, blur );
            
            FBO_Blit( tr.quarterFbo[0], nullptr, nullptr, dst, dstBox, nullptr, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
        }
#ifndef HQ_BLUR
        // ok blur, but can see some pixelization
        else if( blur > 1.0f && blur <= 2.0f )
        {
            // crossfade quarter texture with 1/16th texture
            FBO_Blit( tr.quarterFbo[0], nullptr, nullptr, dst, dstBox, nullptr, nullptr, 0 );
            
            VectorSet4( color, 1, 1, 1, blur - 1.0f );
            
            FBO_Blit( tr.textureScratchFbo[0], nullptr, nullptr, dst, dstBox, nullptr, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
        }
        else if( blur > 2.0f )
        {
            // blur 1/16th texture then replace
            sint i;
            
            for( i = 0; i < 2; i++ )
            {
                vec2_t blurTexScale;
                float32 subblur;
                
                subblur = ( ( blur - 2.0f ) / 2.0f ) / 3.0f * static_cast<float32>( i + 1 );
                
                blurTexScale[0] =
                    blurTexScale[1] = subblur;
                    
                color[0] =
                    color[1] =
                        color[2] = 0.5f;
                color[3] = 1.0f;
                
                if( i != 0 )
                    FBO_Blit( tr.textureScratchFbo[0], nullptr, blurTexScale, tr.textureScratchFbo[1], nullptr, &tr.bokehShader, color, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
                else
                    FBO_Blit( tr.textureScratchFbo[0], nullptr, blurTexScale, tr.textureScratchFbo[1], nullptr, &tr.bokehShader, color, 0 );
            }
            
            FBO_Blit( tr.textureScratchFbo[1], nullptr, nullptr, dst, dstBox, nullptr, nullptr, 0 );
        }
#else // higher quality blur, but slower
        else if( blur > 1.0f )
        {
            // blur quarter texture then replace
            sint i;
        
            src = tr.quarterFbo[0];
            dst = tr.quarterFbo[1];
        
            VectorSet4( color, 0.5f, 0.5f, 0.5f, 1 );
        
            for( i = 0; i < 2; i++ )
            {
                vec2_t blurTexScale;
                float32 subblur;
        
                subblur = ( blur - 1.0f ) / 2.0f * static_cast<float32>( i + 1 );
        
                blurTexScale[0] =
                    blurTexScale[1] = subblur;
        
                color[0] =
                    color[1] =
                        color[2] = 1.0f;
                if( i != 0 )
                    color[3] = 1.0f;
                else
                    color[3] = 0.5f;
        
                FBO_Blit( tr.quarterFbo[0], nullptr, blurTexScale, tr.quarterFbo[1], nullptr, &tr.bokehShader, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
            }
        
            FBO_Blit( tr.quarterFbo[1], nullptr, nullptr, dst, dstBox, nullptr, nullptr, 0 );
        }
#endif
    }
}


static void RB_RadialBlur( FBO_t* srcFbo, FBO_t* dstFbo, sint passes, float32 stretch, float32 x, float32 y, float32 w, float32 h, float32 xcenter, float32 ycenter, float32 alpha )
{
    ivec4_t srcBox, dstBox;
    sint srcWidth, srcHeight;
    vec4_t color;
    const float32 inc = 1.f / passes;
    const float32 mul = powf( stretch, inc );
    float32 scale;
    
    alpha *= inc;
    VectorSet4( color, alpha, alpha, alpha, 1.0f );
    
    srcWidth  = srcFbo ? srcFbo->width  : glConfig.vidWidth;
    srcHeight = srcFbo ? srcFbo->height : glConfig.vidHeight;
    
    VectorSet4( srcBox, 0, 0, srcWidth, srcHeight );
    
    VectorSet4( dstBox, x, y, w, h );
    FBO_Blit( srcFbo, srcBox, nullptr, dstFbo, dstBox, nullptr, color, 0 );
    
    --passes;
    scale = mul;
    while( passes > 0 )
    {
        float32 iscale = 1.f / scale;
        float32 s0 = xcenter * ( 1.f - iscale );
        float32 t0 = ( 1.0f - ycenter ) * ( 1.f - iscale );
        
        srcBox[0] = s0 * srcWidth;
        srcBox[1] = t0 * srcHeight;
        srcBox[2] = iscale * srcWidth;
        srcBox[3] = iscale * srcHeight;
        
        FBO_Blit( srcFbo, srcBox, nullptr, dstFbo, dstBox, nullptr, color, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
        
        scale *= mul;
        --passes;
    }
}


static bool RB_UpdateSunFlareVis( void )
{
    uint sampleCount = 0;
    if( !glRefConfig.occlusionQuery )
        return true;
        
    tr.sunFlareQueryIndex ^= 1;
    if( !tr.sunFlareQueryActive[tr.sunFlareQueryIndex] )
        return true;
        
    /* debug code */
    if( 0 )
    {
        sint iter;
        for( iter = 0 ; ; ++iter )
        {
            sint available = 0;
            qglGetQueryObjectiv( tr.sunFlareQuery[tr.sunFlareQueryIndex], GL_QUERY_RESULT_AVAILABLE, &available );
            if( available )
                break;
        }
        
        CL_RefPrintf( PRINT_DEVELOPER, "Waited %d iterations\n", iter );
    }
    
    qglGetQueryObjectuiv( tr.sunFlareQuery[tr.sunFlareQueryIndex], GL_QUERY_RESULT, &sampleCount );
    return sampleCount > 0;
}

void RB_SunRays( FBO_t* srcFbo, ivec4_t srcBox, FBO_t* dstFbo, ivec4_t dstBox )
{
    vec4_t color;
    float32 dot;
    const float32 cutoff = 0.25f;
    bool colorize = true;
    
//	float32 w, h, w2, h2;
    mat4_t mvp, trans, model;
    vec4_t pos, hpos;
    
    if( tr.sunShaderScale <= 0 )
        return;
        
    dot = DotProduct( tr.sunDirection, backEnd.viewParms.orientation.axis[0] );
    if( dot < cutoff )
        return;
        
    if( !RB_UpdateSunFlareVis() )
        return;
        
    float32 dist;
    
    dist = backEnd.viewParms.zFar / 1.75f;		// div sqrt(3)
    
    VectorScale( tr.sunDirection, dist, pos );
    
    // project sun point
    Mat4Translation( backEnd.viewParms.orientation.origin, trans );
    Mat4Multiply( backEnd.viewParms.world.modelMatrix, trans, model );
    Mat4Multiply( backEnd.viewParms.projectionMatrix, model, mvp );
    Mat4Transform( mvp, pos, hpos );
    
    // transform to UV coords
    hpos[3] = 0.5f / hpos[3];
    
    pos[0] = 0.5f + hpos[0] * hpos[3];
    pos[1] = 0.5f + hpos[1] * hpos[3];
    
    // initialize quarter buffers
    {
        float32 mul = 1.f;
        ivec4_t rayBox, quarterBox;
        sint srcWidth  = srcFbo ? srcFbo->width  : glConfig.vidWidth;
        sint srcHeight = srcFbo ? srcFbo->height : glConfig.vidHeight;
        
        VectorSet4( color, mul, mul, mul, 1 );
        
        rayBox[0] = srcBox[0] * tr.sunRaysFbo->width  / srcWidth;
        rayBox[1] = srcBox[1] * tr.sunRaysFbo->height / srcHeight;
        rayBox[2] = srcBox[2] * tr.sunRaysFbo->width  / srcWidth;
        rayBox[3] = srcBox[3] * tr.sunRaysFbo->height / srcHeight;
        
        quarterBox[0] = 0;
        quarterBox[1] = tr.quarterFbo[0]->height;
        quarterBox[2] = tr.quarterFbo[0]->width;
        quarterBox[3] = -tr.quarterFbo[0]->height;
        
        // first, downsample the framebuffer
        if( colorize )
        {
            FBO_FastBlit( srcFbo, srcBox, tr.quarterFbo[0], quarterBox, GL_COLOR_BUFFER_BIT, GL_LINEAR );
            FBO_Blit( tr.sunRaysFbo, rayBox, nullptr, tr.quarterFbo[0], quarterBox, nullptr, color, GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO );
        }
        else
        {
            FBO_FastBlit( tr.sunRaysFbo, rayBox, tr.quarterFbo[0], quarterBox, GL_COLOR_BUFFER_BIT, GL_LINEAR );
        }
    }
    
    // radial blur passes, ping-ponging between the two quarter-size buffers
    {
        const float32 stretch_add = 2.f / 3.f;
        float32 stretch = 1.f + stretch_add;
        sint i;
        for( i = 0; i < 2; ++i )
        {
            RB_RadialBlur( tr.quarterFbo[i & 1], tr.quarterFbo[( ~i ) & 1], 5, stretch, 0.f, 0.f, tr.quarterFbo[0]->width, tr.quarterFbo[0]->height, pos[0], pos[1], 1.125f );
            stretch += stretch_add;
        }
    }
    
    // add result back on top of the main buffer
    {
        float32 mul = 1.f;
        
        VectorSet4( color, mul, mul, mul, 1 );
        
        FBO_Blit( tr.quarterFbo[0], nullptr, nullptr, dstFbo, dstBox, nullptr, color, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
    }
}

static void RB_BlurAxis( FBO_t* srcFbo, FBO_t* dstFbo, float32 strength, bool horizontal )
{
    float32 dx, dy;
    float32 xmul, ymul;
    float32 weights[3] =
    {
        0.227027027f,
        0.316216216f,
        0.070270270f,
    };
    float32 offsets[3] =
    {
        0.f,
        1.3846153846f,
        3.2307692308f,
    };
    
    xmul = horizontal;
    ymul = 1.f - xmul;
    
    xmul *= strength;
    ymul *= strength;
    
    {
        ivec4_t srcBox, dstBox;
        vec4_t color;
        
        VectorSet4( color, weights[0], weights[0], weights[0], 1.0f );
        VectorSet4( srcBox, 0, 0, srcFbo->width, srcFbo->height );
        VectorSet4( dstBox, 0, 0, dstFbo->width, dstFbo->height );
        FBO_Blit( srcFbo, srcBox, nullptr, dstFbo, dstBox, nullptr, color, 0 );
        
        VectorSet4( color, weights[1], weights[1], weights[1], 1.0f );
        dx = offsets[1] * xmul;
        dy = offsets[1] * ymul;
        VectorSet4( srcBox, dx, dy, srcFbo->width, srcFbo->height );
        FBO_Blit( srcFbo, srcBox, nullptr, dstFbo, dstBox, nullptr, color, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
        VectorSet4( srcBox, -dx, -dy, srcFbo->width, srcFbo->height );
        FBO_Blit( srcFbo, srcBox, nullptr, dstFbo, dstBox, nullptr, color, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
        
        VectorSet4( color, weights[2], weights[2], weights[2], 1.0f );
        dx = offsets[2] * xmul;
        dy = offsets[2] * ymul;
        VectorSet4( srcBox, dx, dy, srcFbo->width, srcFbo->height );
        FBO_Blit( srcFbo, srcBox, nullptr, dstFbo, dstBox, nullptr, color, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
        VectorSet4( srcBox, -dx, -dy, srcFbo->width, srcFbo->height );
        FBO_Blit( srcFbo, srcBox, nullptr, dstFbo, dstBox, nullptr, color, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
    }
}

void RB_HBlur( FBO_t* srcFbo, FBO_t* dstFbo, float32 strength )
{
    RB_BlurAxis( srcFbo, dstFbo, strength, true );
}

void RB_VBlur( FBO_t* srcFbo, FBO_t* dstFbo, float32 strength )
{
    RB_BlurAxis( srcFbo, dstFbo, strength, false );
}

void RB_GaussianBlur( float32 blur )
{
    //float32 mul = 1.f;
    float32 factor = Com_Clamp( 0.f, 1.f, blur );
    
    if( factor <= 0.f )
        return;
        
    {
        ivec4_t srcBox, dstBox;
        vec4_t color;
        
        VectorSet4( color, 1, 1, 1, 1 );
        
        // first, downsample the framebuffer
        FBO_FastBlit( nullptr, nullptr, tr.quarterFbo[0], nullptr, GL_COLOR_BUFFER_BIT, GL_LINEAR );
        FBO_FastBlit( tr.quarterFbo[0], nullptr, tr.textureScratchFbo[0], nullptr, GL_COLOR_BUFFER_BIT, GL_LINEAR );
        
        // set the alpha channel
        qglColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE );
        FBO_BlitFromTexture( tr.whiteImage, nullptr, nullptr, tr.textureScratchFbo[0], nullptr, nullptr, color, GLS_DEPTHTEST_DISABLE );
        qglColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
        
        // blur the tiny buffer horizontally and vertically
        RB_HBlur( tr.textureScratchFbo[0], tr.textureScratchFbo[1], factor );
        RB_VBlur( tr.textureScratchFbo[1], tr.textureScratchFbo[0], factor );
        
        // finally, merge back to framebuffer
        VectorSet4( srcBox, 0, 0, tr.textureScratchFbo[0]->width, tr.textureScratchFbo[0]->height );
        VectorSet4( dstBox, 0, 0, glConfig.vidWidth, glConfig.vidHeight );
        color[3] = factor;
        FBO_Blit( tr.textureScratchFbo[0], srcBox, nullptr, nullptr, dstBox, nullptr, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
    }
}

void RB_GaussianBlur( FBO_t* srcFbo, FBO_t* intermediateFbo, FBO_t* dstFbo, float32 spread )
{
    // Blur X
    vec2_t scale;
    VectorSet2( scale, spread, spread );
    
    FBO_Blit( srcFbo, nullptr, scale, intermediateFbo, nullptr, &tr.gaussianBlurShader[0], nullptr, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
    
    // Blur Y
    FBO_Blit( intermediateFbo, nullptr, scale, dstFbo, nullptr, &tr.gaussianBlurShader[1], nullptr, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
}

void RB_DarkExpand( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = pow( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    GLSL_BindProgram( &tr.darkexpandShader );
    
    GL_BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    {
        vec2_t screensize;
        screensize[0] = glConfig.vidWidth;
        screensize[1] = glConfig.vidHeight;
        
        GLSL_SetUniformVec2( &tr.darkexpandShader, UNIFORM_DIMENSIONS, screensize );
    }
    
    FBO_Blit( hdrFbo, hdrBox, nullptr, ldrFbo, ldrBox, &tr.darkexpandShader, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
}

void RB_Anamorphic( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t	color;
    ivec4_t halfBox;
    vec2_t	texScale, texHalfScale, texDoubleScale;
    
    texScale[0] = texScale[1] = 1.0f;
    texHalfScale[0] = texHalfScale[1] = texScale[0] / 8.0;
    texDoubleScale[0] = texDoubleScale[1] = texScale[0] * 8.0;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = pow( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    halfBox[0] = backEnd.viewParms.viewportX      * tr.anamorphicRenderFBOImage[0]->width / static_cast<float32>( glConfig.vidWidth );
    halfBox[1] = backEnd.viewParms.viewportY      * tr.anamorphicRenderFBOImage[0]->height / static_cast<float32>( glConfig.vidHeight );
    halfBox[2] = backEnd.viewParms.viewportWidth  * tr.anamorphicRenderFBOImage[0]->width / static_cast<float32>( glConfig.vidWidth );
    halfBox[3] = backEnd.viewParms.viewportHeight * tr.anamorphicRenderFBOImage[0]->height / static_cast<float32>( glConfig.vidHeight );
    
    //
    // Darken to VBO...
    //
    
    GLSL_BindProgram( &tr.anamorphicDarkenShader );
    
    GL_BindToTMU( tr.fixedLevelsImage, TB_DIFFUSEMAP );
    
    {
        vec2_t screensize;
        screensize[0] = glConfig.vidWidth;
        screensize[1] = glConfig.vidHeight;
        
        GLSL_SetUniformVec2( &tr.anamorphicDarkenShader, UNIFORM_DIMENSIONS, screensize );
    }
    
    {
        vec4_t local0;
        VectorSet4( local0, r_anamorphicDarkenPower->value, 0.0, 0.0, 0.0 );
        GLSL_SetUniformVec4( &tr.anamorphicDarkenShader, UNIFORM_LOCAL0, local0 );
    }
    
    FBO_Blit( hdrFbo, nullptr, texHalfScale, tr.anamorphicRenderFBO[1], nullptr, &tr.anamorphicDarkenShader, color, 0 );
    FBO_FastBlit( tr.anamorphicRenderFBO[1], nullptr, tr.anamorphicRenderFBO[0], nullptr, GL_COLOR_BUFFER_BIT, GL_LINEAR );
    
    //
    // Blur the new darken'ed VBO...
    //
    
    for( sint i = 0; i < r_bloomPasses->integer; i++ )
    {
        //
        // Bloom X axis... (to VBO 1)
        //
        
        //for (sint width = 1; width < 12 ; width++)
        {
            GLSL_BindProgram( &tr.ssgiBlurShader );
            
            GL_BindToTMU( tr.anamorphicRenderFBOImage[0], TB_DIFFUSEMAP );
            
            {
                vec2_t screensize;
                screensize[0] = tr.anamorphicRenderFBOImage[0]->width;
                screensize[1] = tr.anamorphicRenderFBOImage[0]->height;
                
                GLSL_SetUniformVec2( &tr.ssgiBlurShader, UNIFORM_DIMENSIONS, screensize );
            }
            
            {
                vec4_t local0;
                VectorSet4( local0, 1.0, 0.0, 16.0, 0.0 );
                GLSL_SetUniformVec4( &tr.ssgiBlurShader, UNIFORM_LOCAL0, local0 );
            }
            
            FBO_Blit( tr.anamorphicRenderFBO[0], nullptr, nullptr, tr.anamorphicRenderFBO[1], nullptr, &tr.ssgiBlurShader, color, 0 );
            FBO_FastBlit( tr.anamorphicRenderFBO[1], nullptr, tr.anamorphicRenderFBO[0], nullptr, GL_COLOR_BUFFER_BIT, GL_LINEAR );
        }
    }
    
    //
    // Copy (and upscale) the bloom image to our full screen image...
    //
    
    FBO_Blit( tr.anamorphicRenderFBO[0], nullptr, texDoubleScale, tr.anamorphicRenderFBO[2], nullptr, &tr.ssgiBlurShader, color, 0 );
    
    //
    // Combine the screen with the bloom'ed VBO...
    //
    
    
    GLSL_BindProgram( &tr.anamorphicCombineShader );
    
    GL_BindToTMU( tr.fixedLevelsImage, TB_DIFFUSEMAP );
    
    GLSL_SetUniformInt( &tr.anamorphicCombineShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
    GLSL_SetUniformInt( &tr.anamorphicCombineShader, UNIFORM_NORMALMAP, TB_NORMALMAP );
    
    GL_BindToTMU( tr.anamorphicRenderFBOImage[2], TB_NORMALMAP );
    
    {
        vec4_t local0;
        VectorSet4( local0, 0.6, 0.0, 0.0, 0.0 );
        
        VectorSet4( local0, 1.0, 0.0, 0.0, 0.0 ); // Account for already added glow...
        GLSL_SetUniformVec4( &tr.anamorphicCombineShader, UNIFORM_LOCAL0, local0 );
    }
    
    FBO_Blit( hdrFbo, hdrBox, nullptr, ldrFbo, ldrBox, &tr.anamorphicCombineShader, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
    
    //
    // Render the results now...
    //
    
    FBO_FastBlit( ldrFbo, nullptr, hdrFbo, nullptr, GL_COLOR_BUFFER_BIT, GL_LINEAR );
}


void RB_LensFlare( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = pow( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    GLSL_BindProgram( &tr.lensflareShader );
    
    GL_BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    {
        vec2_t screensize;
        screensize[0] = glConfig.vidWidth;
        screensize[1] = glConfig.vidHeight;
        
        GLSL_SetUniformVec2( &tr.lensflareShader, UNIFORM_DIMENSIONS, screensize );
    }
    
    FBO_Blit( hdrFbo, hdrBox, nullptr, ldrFbo, ldrBox, &tr.lensflareShader, color, 0 );
}


void RB_MultiPost( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = pow( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    GLSL_BindProgram( &tr.multipostShader );
    
    GL_BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    {
        vec2_t screensize;
        screensize[0] = glConfig.vidWidth;
        screensize[1] = glConfig.vidHeight;
        
        GLSL_SetUniformVec2( &tr.multipostShader, UNIFORM_DIMENSIONS, screensize );
    }
    
    FBO_Blit( hdrFbo, hdrBox, nullptr, ldrFbo, ldrBox, &tr.multipostShader, color, 0 );
}

void RB_HDR( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = pow( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    GLSL_BindProgram( &tr.hdrShader );
    
    GL_BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    {
        vec4_t viewInfo;
        
        float32 zmax = backEnd.viewParms.zFar;
        float32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmax / zmin, zmax, 0.0, 0.0 );
        //VectorSet4(viewInfo, zmin, zmax, 0.0, 0.0);
        
        GLSL_SetUniformVec4( &tr.hdrShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    {
        vec2_t screensize;
        screensize[0] = glConfig.vidWidth;
        screensize[1] = glConfig.vidHeight;
        
        GLSL_SetUniformVec2( &tr.hdrShader, UNIFORM_DIMENSIONS, screensize );
        
        //CL_RefPrintf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
    }
    
    FBO_Blit( hdrFbo, hdrBox, nullptr, ldrFbo, ldrBox, &tr.hdrShader, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
}

void RB_Anaglyph( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = pow( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    GLSL_BindProgram( &tr.anaglyphShader );
    
    GL_BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    //qglUseProgramObjectARB(tr.fakedepthShader.program);
    
    {
        vec4_t viewInfo;
        
        float32 zmax = backEnd.viewParms.zFar;
        float32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmax / zmin, zmax, 0.0, 0.0 );
        //VectorSet4(viewInfo, zmin, zmax, 0.0, 0.0);
        
        GLSL_SetUniformVec4( &tr.anaglyphShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    {
        vec2_t screensize;
        screensize[0] = glConfig.vidWidth;
        screensize[1] = glConfig.vidHeight;
        
        GLSL_SetUniformVec2( &tr.anaglyphShader, UNIFORM_DIMENSIONS, screensize );
        
        //CL_RefPrintf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
    }
    
    {
        vec4_t local0;
        VectorSet4( local0, r_trueAnaglyphSeparation->value, r_trueAnaglyphRed->value, r_trueAnaglyphGreen->value, r_trueAnaglyphBlue->value );
        GLSL_SetUniformVec4( &tr.anaglyphShader, UNIFORM_LOCAL0, local0 );
    }
    
    FBO_Blit( hdrFbo, hdrBox, nullptr, ldrFbo, ldrBox, &tr.anaglyphShader, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
}

void RB_TextureClean( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    color[0] =
        color[1] =
            color[2] = pow( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    GLSL_BindProgram( &tr.texturecleanShader );
    
    GL_BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    {
        vec4_t viewInfo;
        
        float32 zmax = backEnd.viewParms.zFar;
        float32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmax / zmin, zmax, 0.0, 0.0 );
        //VectorSet4(viewInfo, zmin, zmax, 0.0, 0.0);
        
        GLSL_SetUniformVec4( &tr.texturecleanShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    {
        vec2_t screensize;
        screensize[0] = glConfig.vidWidth;
        screensize[1] = glConfig.vidHeight;
        
        GLSL_SetUniformVec2( &tr.texturecleanShader, UNIFORM_DIMENSIONS, screensize );
        
        //CL_RefPrintf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
    }
    
    {
        vec4_t local0;
        VectorSet4( local0, r_textureCleanSigma->value, r_textureCleanBSigma->value, r_textureCleanMSize->value, 0 );
        GLSL_SetUniformVec4( &tr.texturecleanShader, UNIFORM_LOCAL0, local0 );
    }
    
    //FBO_Blit(hdrFbo, hdrBox, nullptr, ldrFbo, ldrBox, &tr.texturecleanShader, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
    FBO_Blit( hdrFbo, hdrBox, nullptr, ldrFbo, ldrBox, &tr.texturecleanShader, color, 0 );
}

void RB_ESharpening( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = pow( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    GLSL_BindProgram( &tr.esharpeningShader );
    
    GL_BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    //qglUseProgramObjectARB(tr.esharpeningShader.program);
    
    {
        vec4_t viewInfo;
        
        float32 zmax = backEnd.viewParms.zFar;
        float32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmax / zmin, zmax, 0.0, 0.0 );
        //VectorSet4(viewInfo, zmin, zmax, 0.0, 0.0);
        
        GLSL_SetUniformVec4( &tr.esharpeningShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    {
        vec2_t screensize;
        screensize[0] = glConfig.vidWidth;
        screensize[1] = glConfig.vidHeight;
        
        GLSL_SetUniformVec2( &tr.esharpeningShader, UNIFORM_DIMENSIONS, screensize );
        
        //CL_RefPrintf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
    }
    
    //{
    //	vec4_t local0;
    //	VectorSet4(local0, r_textureCleanSigma->value, r_textureCleanBSigma->value, 0, 0);
    //	GLSL_SetUniformVec4(&tr.texturecleanShader, UNIFORM_LOCAL0, local0);
    //}
    
    //FBO_Blit(hdrFbo, hdrBox, nullptr, ldrFbo, ldrBox, &tr.esharpeningShader, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
    FBO_Blit( hdrFbo, hdrBox, nullptr, ldrFbo, ldrBox, &tr.esharpeningShader, color, 0 );
}


void RB_ESharpening2( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = pow( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    GLSL_BindProgram( &tr.esharpening2Shader );
    
    GL_BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    {
        vec4_t viewInfo;
        
        float32 zmax = backEnd.viewParms.zFar;
        float32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmax / zmin, zmax, 0.0, 0.0 );
        
        GLSL_SetUniformVec4( &tr.esharpening2Shader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    {
        vec2_t screensize;
        screensize[0] = glConfig.vidWidth;
        screensize[1] = glConfig.vidHeight;
        
        GLSL_SetUniformVec2( &tr.esharpening2Shader, UNIFORM_DIMENSIONS, screensize );
    }
    
    FBO_Blit( hdrFbo, hdrBox, nullptr, ldrFbo, ldrBox, &tr.esharpeningShader, color, 0 );
}


void RB_DOF( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = pow( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    GLSL_BindProgram( &tr.dofShader );
    
    GL_BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    GLSL_SetUniformInt( &tr.dofShader, UNIFORM_LEVELSMAP, TB_LEVELSMAP );
    GLSL_SetUniformInt( &tr.dofShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP );
    GL_BindToTMU( tr.renderDepthImage, TB_LIGHTMAP );
    
    {
        vec2_t screensize;
        screensize[0] = glConfig.vidWidth;
        screensize[1] = glConfig.vidHeight;
        
        GLSL_SetUniformVec2( &tr.dofShader, UNIFORM_DIMENSIONS, screensize );
    }
    
    {
        vec4_t viewInfo;
        
        float32 zmax = backEnd.viewParms.zFar;
        float32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmin, zmax, zmax / zmin, 0.0 );
        
        GLSL_SetUniformVec4( &tr.dofShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    FBO_Blit( hdrFbo, hdrBox, nullptr, ldrFbo, ldrBox, &tr.dofShader, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
}

void RB_Vibrancy( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = pow( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    GLSL_BindProgram( &tr.vibrancyShader );
    
    GL_BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    GLSL_SetUniformInt( &tr.vibrancyShader, UNIFORM_LEVELSMAP, TB_LEVELSMAP );
    
    {
        vec4_t info;
        
        info[0] = r_vibrancy->value;
        info[1] = 0.0;
        info[2] = 0.0;
        info[3] = 0.0;
        
        VectorSet4( info, info[0], info[1], info[2], info[3] );
        
        GLSL_SetUniformVec4( &tr.vibrancyShader, UNIFORM_LOCAL0, info );
    }
    
    FBO_Blit( hdrFbo, hdrBox, nullptr, ldrFbo, ldrBox, &tr.vibrancyShader, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
}

void RB_TextureDetail( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = pow( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    GLSL_BindProgram( &tr.texturedetailShader );
    
    GL_BindToTMU( hdrFbo->colorImage[0], TB_LEVELSMAP );
    
    GLSL_SetUniformMat4( &tr.texturedetailShader, UNIFORM_INVEYEPROJECTIONMATRIX, glState.invEyeProjection );
    GLSL_SetUniformInt( &tr.texturedetailShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP );
    GL_BindToTMU( tr.renderDepthImage, TB_LIGHTMAP );
    {
        vec2_t screensize;
        screensize[0] = glConfig.vidWidth;
        screensize[1] = glConfig.vidHeight;
        
        GLSL_SetUniformVec2( &tr.texturedetailShader, UNIFORM_DIMENSIONS, screensize );
    }
    
    {
        vec4_t viewInfo;
        
        float32 zmax = backEnd.viewParms.zFar;
        float32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmax / zmin, zmax, zmin, zmax );
        
        GLSL_SetUniformVec4( &tr.texturedetailShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    {
        vec4_t local0;
        VectorSet4( local0, r_texturedetailStrength->value, 0.0, 0.0, 0.0 ); // non-flicker version
        GLSL_SetUniformVec4( &tr.texturedetailShader, UNIFORM_LOCAL0, local0 );
    }
    
    FBO_Blit( hdrFbo, hdrBox, nullptr, ldrFbo, ldrBox, &tr.texturedetailShader, color, 0 ); //GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
}

void RB_RBM( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t		color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = pow( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    GLSL_BindProgram( &tr.rbmShader );
    
    GLSL_SetUniformMat4( &tr.rbmShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection );
    GLSL_SetUniformMat4( &tr.rbmShader, UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix );
    
    GLSL_SetUniformVec3( &tr.rbmShader, UNIFORM_VIEWORIGIN, backEnd.refdef.vieworg );
    
    GLSL_SetUniformInt( &tr.rbmShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
    GL_BindToTMU( hdrFbo->colorImage[0], TB_DIFFUSEMAP );
    GLSL_SetUniformInt( &tr.rbmShader, UNIFORM_NORMALMAP, TB_NORMALMAP );
    GL_BindToTMU( tr.normalDetailedImage, TB_NORMALMAP );
    GLSL_SetUniformInt( &tr.rbmShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP );
    GL_BindToTMU( tr.renderDepthImage, TB_LIGHTMAP );
    
    {
        vec4_t local0;
        VectorSet4( local0, r_rbmStrength->value, 0.0, 0.0, 0.0 );
        GLSL_SetUniformVec4( &tr.rbmShader, UNIFORM_LOCAL0, local0 );
    }
    
    {
        vec2_t screensize;
        screensize[0] = glConfig.vidWidth;
        screensize[1] = glConfig.vidHeight;
        
        GLSL_SetUniformVec2( &tr.rbmShader, UNIFORM_DIMENSIONS, screensize );
    }
    
    {
        vec4_t viewInfo;
        float32 zmax = backEnd.viewParms.zFar;
        float32 ymax = zmax * tan( backEnd.viewParms.fovY * M_PI / 360.0f );
        float32 xmax = zmax * tan( backEnd.viewParms.fovX * M_PI / 360.0f );
        float32 zmin = r_znear->value;
        VectorSet4( viewInfo, zmin, zmax, zmax / zmin, 0.0 );
        GLSL_SetUniformVec4( &tr.rbmShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    FBO_Blit( hdrFbo, hdrBox, nullptr, ldrFbo, ldrBox, &tr.rbmShader, color, 0 );
}

void RB_Contrast( FBO_t* src, ivec4_t srcBox, FBO_t* dst, ivec4_t dstBox )
{
    float32 brightness = 2;
    
    if( !glRefConfig.framebufferObject )
    {
        return;
    }
    
    GLSL_SetUniformFloat( &tr.contrastShader, UNIFORM_BRIGHTNESS, r_brightness->value );
    GLSL_SetUniformFloat( &tr.contrastShader, UNIFORM_CONTRAST, r_contrast->value );
    GLSL_SetUniformFloat( &tr.contrastShader, UNIFORM_GAMMA, r_gamma->value );
    
    FBO_FastBlit( src, srcBox, tr.screenScratchFbo, srcBox, GL_COLOR_BUFFER_BIT, GL_LINEAR );
    FBO_Blit( tr.screenScratchFbo, srcBox, nullptr, dst, dstBox, &tr.contrastShader, nullptr, 0 );
}


void RB_FXAA( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = pow( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    GLSL_BindProgram( &tr.fxaaShader );
    GL_BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    GLSL_SetUniformMat4( &tr.fxaaShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection );
    
    {
        vec2_t screensize;
        screensize[0] = glConfig.vidWidth;
        screensize[1] = glConfig.vidHeight;
        
        GLSL_SetUniformVec2( &tr.fxaaShader, UNIFORM_DIMENSIONS, screensize );
    }
    
    FBO_Blit( hdrFbo, hdrBox, nullptr, ldrFbo, ldrBox, &tr.fxaaShader, color, 0 );
}

void RB_Bloom( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t	color;
    ivec4_t halfBox;
    vec2_t	texScale, texHalfScale, texDoubleScale;
    
    texScale[0] = texScale[1] = 1.0f;
    texHalfScale[0] = texHalfScale[1] = texScale[0] / 2.0;
    texDoubleScale[0] = texDoubleScale[1] = texScale[0] * 2.0;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = pow( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    halfBox[0] = backEnd.viewParms.viewportX      * tr.bloomRenderFBOImage[0]->width / static_cast<float32>( glConfig.vidWidth );
    halfBox[1] = backEnd.viewParms.viewportY      * tr.bloomRenderFBOImage[0]->height / static_cast<float32>( glConfig.vidHeight );
    halfBox[2] = backEnd.viewParms.viewportWidth  * tr.bloomRenderFBOImage[0]->width / static_cast<float32>( glConfig.vidWidth );
    halfBox[3] = backEnd.viewParms.viewportHeight * tr.bloomRenderFBOImage[0]->height / static_cast<float32>( glConfig.vidHeight );
    
    //
    // Darken to VBO...
    //
    
    GLSL_BindProgram( &tr.bloomDarkenShader );
    
    GL_BindToTMU( tr.fixedLevelsImage, TB_DIFFUSEMAP );
    
    {
        vec2_t screensize;
        screensize[0] = glConfig.vidWidth;
        screensize[1] = glConfig.vidHeight;
        
        GLSL_SetUniformVec2( &tr.bloomDarkenShader, UNIFORM_DIMENSIONS, screensize );
    }
    
    {
        vec4_t local0;
        VectorSet4( local0, r_bloomDarkenPower->value, 0.0, 0.0, 0.0 );
        GLSL_SetUniformVec4( &tr.bloomDarkenShader, UNIFORM_LOCAL0, local0 );
    }
    
    FBO_Blit( hdrFbo, nullptr, texHalfScale, tr.bloomRenderFBO[1], nullptr, &tr.bloomDarkenShader, color, 0 );
    FBO_FastBlit( tr.bloomRenderFBO[1], nullptr, tr.bloomRenderFBO[0], nullptr, GL_COLOR_BUFFER_BIT, GL_LINEAR );
    
    //
    // Blur the new darken'ed VBO...
    //
    
    for( sint i = 0; i < r_bloomPasses->integer; i++ )
    {
#ifdef ___BLOOM_AXIS_UNCOMBINED_SHADER___
        //
        // Bloom X axis... (to VBO 1)
        //
        
        GLSL_BindProgram( &tr.bloomBlurShader );
        
        GL_BindToTMU( tr.bloomRenderFBOImage[0], TB_DIFFUSEMAP );
        
        {
            vec2_t screensize;
            screensize[0] = tr.bloomRenderFBOImage[0]->width;
            screensize[1] = tr.bloomRenderFBOImage[0]->height;
            
            GLSL_SetUniformVec2( &tr.bloomBlurShader, UNIFORM_DIMENSIONS, screensize );
        }
        
        {
            vec4_t local0;
            VectorSet4( local0, 1.0, 0.0, 0.0, 0.0 );
            GLSL_SetUniformVec4( &tr.bloomBlurShader, UNIFORM_LOCAL0, local0 );
        }
        
        FBO_Blit( tr.bloomRenderFBO[0], nullptr, nullptr, tr.bloomRenderFBO[1], nullptr, &tr.bloomBlurShader, color, 0 );
        FBO_FastBlit( tr.bloomRenderFBO[1], nullptr, tr.bloomRenderFBO[0], nullptr, GL_COLOR_BUFFER_BIT, GL_LINEAR );
        
        //
        // Bloom Y axis... (back to VBO 0)
        //
        
        GLSL_BindProgram( &tr.bloomBlurShader );
        
        GL_BindToTMU( tr.bloomRenderFBOImage[1], TB_DIFFUSEMAP );
        
        {
            vec2_t screensize;
            screensize[0] = tr.bloomRenderFBOImage[1]->width;
            screensize[1] = tr.bloomRenderFBOImage[1]->height;
            
            GLSL_SetUniformVec2( &tr.bloomBlurShader, UNIFORM_DIMENSIONS, screensize );
        }
        
        {
            vec4_t local0;
            VectorSet4( local0, 0.0, 1.0, 0.0, 0.0 );
            GLSL_SetUniformVec4( &tr.bloomBlurShader, UNIFORM_LOCAL0, local0 );
        }
        
        FBO_Blit( tr.bloomRenderFBO[0], nullptr, nullptr, tr.bloomRenderFBO[1], nullptr, &tr.bloomBlurShader, color, 0 );
        FBO_FastBlit( tr.bloomRenderFBO[1], nullptr, tr.bloomRenderFBO[0], nullptr, GL_COLOR_BUFFER_BIT, GL_LINEAR );
#else //___BLOOM_AXIS_UNCOMBINED_SHADER___
        
        //
        // Bloom X and Y axis... (to VBO 1)
        //
        
        GLSL_BindProgram( &tr.bloomBlurShader );
        
        GL_BindToTMU( tr.bloomRenderFBOImage[0], TB_DIFFUSEMAP );
        
        {
            vec2_t screensize;
            screensize[0] = tr.bloomRenderFBOImage[0]->width;
            screensize[1] = tr.bloomRenderFBOImage[0]->height;
        
            GLSL_SetUniformVec2( &tr.bloomBlurShader, UNIFORM_DIMENSIONS, screensize );
        }
        
        {
            vec4_t local0;
            VectorSet4( local0, 0.0, 0.0, /* bloom width */ 3.0, 0.0 );
            GLSL_SetUniformVec4( &tr.bloomBlurShader, UNIFORM_LOCAL0, local0 );
        }
        
        FBO_Blit( tr.bloomRenderFBO[0], nullptr, nullptr, tr.bloomRenderFBO[1], nullptr, &tr.bloomBlurShader, color, 0 );
        FBO_FastBlit( tr.bloomRenderFBO[1], nullptr, tr.bloomRenderFBO[0], nullptr, GL_COLOR_BUFFER_BIT, GL_LINEAR );
        
#endif //___BLOOM_AXIS_UNCOMBINED_SHADER___
    }
    
    //
    // Copy (and upscale) the bloom image to our full screen image...
    //
    
    FBO_Blit( tr.bloomRenderFBO[0], nullptr, texDoubleScale, tr.bloomRenderFBO[2], nullptr, &tr.bloomBlurShader, color, 0 );
    
    //
    // Combine the screen with the bloom'ed VBO...
    //
    GLSL_BindProgram( &tr.bloomCombineShader );
    
    GL_BindToTMU( tr.fixedLevelsImage, TB_DIFFUSEMAP );
    
    GLSL_SetUniformInt( &tr.bloomCombineShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
    GLSL_SetUniformInt( &tr.bloomCombineShader, UNIFORM_NORMALMAP, TB_NORMALMAP );
    
    GL_BindToTMU( tr.bloomRenderFBOImage[2], TB_NORMALMAP );
    
    {
        vec4_t local0;
        VectorSet4( local0, r_bloomScale->value, 0.0, 0.0, 0.0 );
        VectorSet4( local0, 0.5 * r_bloomScale->value, 0.0, 0.0, 0.0 );
        GLSL_SetUniformVec4( &tr.bloomCombineShader, UNIFORM_LOCAL0, local0 );
    }
    
    FBO_Blit( hdrFbo, hdrBox, nullptr, ldrFbo, ldrBox, &tr.bloomCombineShader, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
    
    //
    // Render the results now...
    //
    
    FBO_FastBlit( ldrFbo, nullptr, hdrFbo, nullptr, GL_COLOR_BUFFER_BIT, GL_LINEAR );
}

void RB_SSGI( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t	color;
    // bloom
    color[0] =
        color[1] =
            color[2] = pow( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    {
        ivec4_t halfBox;
        vec2_t	texScale, texHalfScale, texDoubleScale;
        
        texScale[0] = texScale[1] = 1.0f;
        texHalfScale[0] = texHalfScale[1] = texScale[0] / 8.0;
        texDoubleScale[0] = texDoubleScale[1] = texScale[0] * 8.0;
        
        halfBox[0] = backEnd.viewParms.viewportX * tr.anamorphicRenderFBOImage[0]->width / static_cast<float32>( glConfig.vidWidth );
        halfBox[1] = backEnd.viewParms.viewportY * tr.anamorphicRenderFBOImage[0]->height / static_cast<float32>( glConfig.vidHeight );
        halfBox[2] = backEnd.viewParms.viewportWidth * tr.anamorphicRenderFBOImage[0]->width / static_cast<float32>( glConfig.vidWidth );
        halfBox[3] = backEnd.viewParms.viewportHeight * tr.anamorphicRenderFBOImage[0]->height / static_cast<float32>( glConfig.vidHeight );
        
        //
        // Darken to VBO...
        //
        
        //if (r_dynamicGlow->integer)
        //{
        //	FBO_BlitFromTexture(tr.glowFboScaled[0]->colorImage[0], nullptr, nullptr, tr.anamorphicRenderFBO[0], nullptr, nullptr, color, 0);
        //}
        //else
        {
            GLSL_BindProgram( &tr.anamorphicDarkenShader );
            
            GL_BindToTMU( tr.fixedLevelsImage, TB_DIFFUSEMAP );
            
            {
                vec2_t screensize;
                screensize[0] = glConfig.vidWidth;
                screensize[1] = glConfig.vidHeight;
                
                GLSL_SetUniformVec2( &tr.anamorphicDarkenShader, UNIFORM_DIMENSIONS, screensize );
            }
            
            {
                vec4_t local0;
                VectorSet4( local0, r_anamorphicDarkenPower->value, 0.0, 0.0, 0.0 );
                GLSL_SetUniformVec4( &tr.anamorphicDarkenShader, UNIFORM_LOCAL0, local0 );
            }
            
            FBO_Blit( hdrFbo, nullptr, texHalfScale, tr.anamorphicRenderFBO[1], nullptr, &tr.anamorphicDarkenShader, color, 0 );
            FBO_FastBlit( tr.anamorphicRenderFBO[1], nullptr, tr.anamorphicRenderFBO[0], nullptr, GL_COLOR_BUFFER_BIT, GL_LINEAR );
        }
        
        //
        // Blur the new darken'ed VBO...
        //
        
//#define __NEW_SATURATION_MAP_METHOD__ // grr slower and crappier...

#ifdef __NEW_SATURATION_MAP_METHOD__
        float32 SCAN_WIDTH = 16.0;
        
        //for (sint i = 0; i < 2; i++)
        {
            // Initial blur...
            GLSL_BindProgram( &tr.anamorphicBlurShader );
            
            GL_BindToTMU( tr.anamorphicRenderFBOImage[0], TB_DIFFUSEMAP );
            
            {
                vec2_t screensize;
                screensize[0] = tr.anamorphicRenderFBOImage[0]->width;
                screensize[1] = tr.anamorphicRenderFBOImage[0]->height;
                
                GLSL_SetUniformVec2( &tr.anamorphicBlurShader, UNIFORM_DIMENSIONS, screensize );
            }
            
            {
                vec4_t local0;
                VectorSet4( local0, 0.0, 0.0, SCAN_WIDTH, 1.0 );
                GLSL_SetUniformVec4( &tr.anamorphicBlurShader, UNIFORM_LOCAL0, local0 );
            }
            
            FBO_Blit( tr.anamorphicRenderFBO[0], nullptr, nullptr, tr.anamorphicRenderFBO[1], nullptr, &tr.anamorphicBlurShader, color, 0 );
            FBO_FastBlit( tr.anamorphicRenderFBO[1], nullptr, tr.anamorphicRenderFBO[0], nullptr, GL_COLOR_BUFFER_BIT, GL_LINEAR );
        }
        
        {
            // Final blur...
            GLSL_BindProgram( &tr.anamorphicBlurShader );
            
            GL_BindToTMU( tr.anamorphicRenderFBOImage[0], TB_DIFFUSEMAP );
            
            {
                vec2_t screensize;
                screensize[0] = tr.anamorphicRenderFBOImage[0]->width;
                screensize[1] = tr.anamorphicRenderFBOImage[0]->height;
                
                GLSL_SetUniformVec2( &tr.anamorphicBlurShader, UNIFORM_DIMENSIONS, screensize );
            }
            
            {
                vec4_t local0;
                VectorSet4( local0, 0.0, 0.0, SCAN_WIDTH, 2.0 );
                GLSL_SetUniformVec4( &tr.anamorphicBlurShader, UNIFORM_LOCAL0, local0 );
            }
            
            FBO_Blit( tr.anamorphicRenderFBO[0], nullptr, nullptr, tr.anamorphicRenderFBO[1], nullptr, &tr.anamorphicBlurShader, color, 0 );
            FBO_FastBlit( tr.anamorphicRenderFBO[1], nullptr, tr.anamorphicRenderFBO[0], nullptr, GL_COLOR_BUFFER_BIT, GL_LINEAR );
        }
#else //!__NEW_SATURATION_MAP_METHOD__
        //float32 SCAN_WIDTH = 16.0;
        float32 SCAN_WIDTH = r_ssgiWidth->value;//8.0;
        
        {
            //
            // Bloom +-X axis... (to VBO 1)
            //
        
            {
                GLSL_BindProgram( &tr.ssgiBlurShader );
        
                GL_BindToTMU( tr.anamorphicRenderFBOImage[0], TB_DIFFUSEMAP );
        
                {
                    vec2_t screensize;
                    screensize[0] = tr.anamorphicRenderFBOImage[0]->width;
                    screensize[1] = tr.anamorphicRenderFBOImage[0]->height;
        
                    GLSL_SetUniformVec2( &tr.ssgiBlurShader, UNIFORM_DIMENSIONS, screensize );
                }
        
                {
                    vec4_t local0;
                    //VectorSet4(local0, (float32)width, 0.0, 0.0, 0.0);
                    VectorSet4( local0, 1.0, 0.0, SCAN_WIDTH, 3.0 );
                    GLSL_SetUniformVec4( &tr.ssgiBlurShader, UNIFORM_LOCAL0, local0 );
                }
        
                FBO_Blit( tr.anamorphicRenderFBO[0], nullptr, nullptr, tr.anamorphicRenderFBO[1], nullptr, &tr.ssgiBlurShader, color, 0 );
                FBO_FastBlit( tr.anamorphicRenderFBO[1], nullptr, tr.anamorphicRenderFBO[0], nullptr, GL_COLOR_BUFFER_BIT, GL_LINEAR );
            }
        
            //
            // Bloom +-Y axis... (to VBO 1)
            //
        
            {
                GLSL_BindProgram( &tr.ssgiBlurShader );
        
                GL_BindToTMU( tr.anamorphicRenderFBOImage[0], TB_DIFFUSEMAP );
        
                {
                    vec2_t screensize;
                    screensize[0] = tr.anamorphicRenderFBOImage[0]->width;
                    screensize[1] = tr.anamorphicRenderFBOImage[0]->height;
        
                    GLSL_SetUniformVec2( &tr.ssgiBlurShader, UNIFORM_DIMENSIONS, screensize );
                }
        
                {
                    vec4_t local0;
                    //VectorSet4(local0, (float32)width, 0.0, 0.0, 0.0);
                    VectorSet4( local0, 0.0, 1.0, SCAN_WIDTH, 3.0 );
                    GLSL_SetUniformVec4( &tr.ssgiBlurShader, UNIFORM_LOCAL0, local0 );
                }
        
                FBO_Blit( tr.anamorphicRenderFBO[0], nullptr, nullptr, tr.anamorphicRenderFBO[1], nullptr, &tr.ssgiBlurShader, color, 0 );
                FBO_FastBlit( tr.anamorphicRenderFBO[1], nullptr, tr.anamorphicRenderFBO[0], nullptr, GL_COLOR_BUFFER_BIT, GL_LINEAR );
            }
        
            //
            // Bloom XY & -X-Y axis... (to VBO 1)
            //
        
            {
                GLSL_BindProgram( &tr.ssgiBlurShader );
        
                GL_BindToTMU( tr.anamorphicRenderFBOImage[0], TB_DIFFUSEMAP );
        
                {
                    vec2_t screensize;
                    screensize[0] = tr.anamorphicRenderFBOImage[0]->width;
                    screensize[1] = tr.anamorphicRenderFBOImage[0]->height;
        
                    GLSL_SetUniformVec2( &tr.ssgiBlurShader, UNIFORM_DIMENSIONS, screensize );
                }
        
                {
                    vec4_t local0;
                    //VectorSet4(local0, (float32)width, 0.0, 0.0, 0.0);
                    VectorSet4( local0, 1.0, 1.0, SCAN_WIDTH, 3.0 );
                    GLSL_SetUniformVec4( &tr.ssgiBlurShader, UNIFORM_LOCAL0, local0 );
                }
        
                FBO_Blit( tr.anamorphicRenderFBO[0], nullptr, nullptr, tr.anamorphicRenderFBO[1], nullptr, &tr.ssgiBlurShader, color, 0 );
                FBO_FastBlit( tr.anamorphicRenderFBO[1], nullptr, tr.anamorphicRenderFBO[0], nullptr, GL_COLOR_BUFFER_BIT, GL_LINEAR );
            }
        
            //
            // Bloom -X+Y & +X-Y axis... (to VBO 1)
            //
        
            {
                GLSL_BindProgram( &tr.ssgiBlurShader );
        
                GL_BindToTMU( tr.anamorphicRenderFBOImage[0], TB_DIFFUSEMAP );
        
                {
                    vec2_t screensize;
                    screensize[0] = tr.anamorphicRenderFBOImage[0]->width;
                    screensize[1] = tr.anamorphicRenderFBOImage[0]->height;
        
                    GLSL_SetUniformVec2( &tr.ssgiBlurShader, UNIFORM_DIMENSIONS, screensize );
                }
        
                {
                    vec4_t local0;
                    //VectorSet4(local0, (float32)width, 0.0, 0.0, 0.0);
                    VectorSet4( local0, -1.0, 1.0, SCAN_WIDTH, 3.0 );
                    GLSL_SetUniformVec4( &tr.ssgiBlurShader, UNIFORM_LOCAL0, local0 );
                }
        
                FBO_Blit( tr.anamorphicRenderFBO[0], nullptr, nullptr, tr.anamorphicRenderFBO[1], nullptr, &tr.ssgiBlurShader, color, 0 );
                FBO_FastBlit( tr.anamorphicRenderFBO[1], nullptr, tr.anamorphicRenderFBO[0], nullptr, GL_COLOR_BUFFER_BIT, GL_LINEAR );
            }
        }
        
        //
        // Do a final blur pass - but this time don't mark it as a ssgi one - so that it uses darkness as well...
        //
        
        {
            //
            // Bloom +-X axis... (to VBO 1)
            //
        
            {
                GLSL_BindProgram( &tr.ssgiBlurShader );
        
                GL_BindToTMU( tr.anamorphicRenderFBOImage[0], TB_DIFFUSEMAP );
        
                {
                    vec2_t screensize;
                    screensize[0] = tr.anamorphicRenderFBOImage[0]->width;
                    screensize[1] = tr.anamorphicRenderFBOImage[0]->height;
        
                    GLSL_SetUniformVec2( &tr.ssgiBlurShader, UNIFORM_DIMENSIONS, screensize );
                }
        
                {
                    vec4_t local0;
                    //VectorSet4(local0, (float32)width, 0.0, 0.0, 0.0);
                    VectorSet4( local0, 1.0, 0.0, SCAN_WIDTH, 0.0 );
                    GLSL_SetUniformVec4( &tr.ssgiBlurShader, UNIFORM_LOCAL0, local0 );
                }
        
                FBO_Blit( tr.anamorphicRenderFBO[0], nullptr, nullptr, tr.anamorphicRenderFBO[1], nullptr, &tr.ssgiBlurShader, color, 0 );
                FBO_FastBlit( tr.anamorphicRenderFBO[1], nullptr, tr.anamorphicRenderFBO[0], nullptr, GL_COLOR_BUFFER_BIT, GL_LINEAR );
            }
        
            //
            // Bloom +-Y axis... (to VBO 1)
            //
        
            {
                GLSL_BindProgram( &tr.ssgiBlurShader );
        
                GL_BindToTMU( tr.anamorphicRenderFBOImage[0], TB_DIFFUSEMAP );
        
                {
                    vec2_t screensize;
                    screensize[0] = tr.anamorphicRenderFBOImage[0]->width;
                    screensize[1] = tr.anamorphicRenderFBOImage[0]->height;
        
                    GLSL_SetUniformVec2( &tr.ssgiBlurShader, UNIFORM_DIMENSIONS, screensize );
                }
        
                {
                    vec4_t local0;
                    //VectorSet4(local0, (float32)width, 0.0, 0.0, 0.0);
                    VectorSet4( local0, 0.0, 1.0, SCAN_WIDTH, 0.0 );
                    GLSL_SetUniformVec4( &tr.ssgiBlurShader, UNIFORM_LOCAL0, local0 );
                }
        
                FBO_Blit( tr.anamorphicRenderFBO[0], nullptr, nullptr, tr.anamorphicRenderFBO[1], nullptr, &tr.ssgiBlurShader, color, 0 );
                FBO_FastBlit( tr.anamorphicRenderFBO[1], nullptr, tr.anamorphicRenderFBO[0], nullptr, GL_COLOR_BUFFER_BIT, GL_LINEAR );
            }
        
            //
            // Bloom XY & -X-Y axis... (to VBO 1)
            //
        
            {
                GLSL_BindProgram( &tr.ssgiBlurShader );
        
                GL_BindToTMU( tr.anamorphicRenderFBOImage[0], TB_DIFFUSEMAP );
        
                {
                    vec2_t screensize;
                    screensize[0] = tr.anamorphicRenderFBOImage[0]->width;
                    screensize[1] = tr.anamorphicRenderFBOImage[0]->height;
        
                    GLSL_SetUniformVec2( &tr.ssgiBlurShader, UNIFORM_DIMENSIONS, screensize );
                }
        
                {
                    vec4_t local0;
                    //VectorSet4(local0, (float32)width, 0.0, 0.0, 0.0);
                    VectorSet4( local0, 1.0, 1.0, SCAN_WIDTH, 0.0 );
                    GLSL_SetUniformVec4( &tr.ssgiBlurShader, UNIFORM_LOCAL0, local0 );
                }
        
                FBO_Blit( tr.anamorphicRenderFBO[0], nullptr, nullptr, tr.anamorphicRenderFBO[1], nullptr, &tr.ssgiBlurShader, color, 0 );
                FBO_FastBlit( tr.anamorphicRenderFBO[1], nullptr, tr.anamorphicRenderFBO[0], nullptr, GL_COLOR_BUFFER_BIT, GL_LINEAR );
            }
        
            //
            // Bloom -X+Y & +X-Y axis... (to VBO 1)
            //
        
            {
                GLSL_BindProgram( &tr.ssgiBlurShader );
        
                GL_BindToTMU( tr.anamorphicRenderFBOImage[0], TB_DIFFUSEMAP );
        
                {
                    vec2_t screensize;
                    screensize[0] = tr.anamorphicRenderFBOImage[0]->width;
                    screensize[1] = tr.anamorphicRenderFBOImage[0]->height;
        
                    GLSL_SetUniformVec2( &tr.ssgiBlurShader, UNIFORM_DIMENSIONS, screensize );
                }
        
                {
                    vec4_t local0;
                    //VectorSet4(local0, (float32)width, 0.0, 0.0, 0.0);
                    VectorSet4( local0, -1.0, 1.0, SCAN_WIDTH, 0.0 );
                    GLSL_SetUniformVec4( &tr.ssgiBlurShader, UNIFORM_LOCAL0, local0 );
                }
        
                FBO_Blit( tr.anamorphicRenderFBO[0], nullptr, nullptr, tr.anamorphicRenderFBO[1], nullptr, &tr.ssgiBlurShader, color, 0 );
                FBO_FastBlit( tr.anamorphicRenderFBO[1], nullptr, tr.anamorphicRenderFBO[0], nullptr, GL_COLOR_BUFFER_BIT, GL_LINEAR );
            }
        }
#endif //__NEW_SATURATION_MAP_METHOD__
        
        //
        // Copy (and upscale) the bloom image to our full screen image...
        //
        
        FBO_Blit( tr.anamorphicRenderFBO[0], nullptr, texDoubleScale, tr.anamorphicRenderFBO[2], nullptr, &tr.ssgiBlurShader, color, 0 );
    }
    
    //
    // Do the SSAO/SSGI...
    //
    
    GLSL_BindProgram( &tr.ssgiShader );
    
    GL_BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    GLSL_SetUniformInt( &tr.ssgiShader, UNIFORM_LEVELSMAP, TB_LEVELSMAP );
    GLSL_SetUniformInt( &tr.ssgiShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP );
    GL_BindToTMU( tr.renderDepthImage, TB_LIGHTMAP );
    GLSL_SetUniformInt( &tr.ssgiShader, UNIFORM_NORMALMAP, TB_NORMALMAP ); // really scaled down dynamic glow map
    GL_BindToTMU( tr.anamorphicRenderFBOImage[2], TB_NORMALMAP ); // really scaled down dynamic glow map
    
    {
        vec2_t screensize;
        screensize[0] = glConfig.vidWidth;
        screensize[1] = glConfig.vidHeight;
        
        GLSL_SetUniformVec2( &tr.ssgiShader, UNIFORM_DIMENSIONS, screensize );
        
        //CL_RefPrintf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
    }
    
    {
        vec4_t viewInfo;
        
        float32 zmax = backEnd.viewParms.zFar;
        float32 zmin = r_znear->value;
        //float32 zmin = backEnd.viewParms.zNear;
        
        VectorSet4( viewInfo, zmin, zmax, zmax / zmin, 0.0 );
        
        //CL_RefPrintf(PRINT_WARNING, "Sent zmin %f, zmax %f, zmax/zmin %f.\n", zmin, zmax, zmax / zmin);
        
        GLSL_SetUniformVec4( &tr.ssgiShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    {
        vec4_t local0;
        local0[0] = r_ssgi->value;
        local0[1] = r_ssgiSamples->value;
        local0[2] = 0.0;
        local0[3] = 0.0;
        
        GLSL_SetUniformVec4( &tr.ssgiShader, UNIFORM_LOCAL0, local0 );
        
        //CL_RefPrintf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
    }
    
    FBO_Blit( hdrFbo, hdrBox, nullptr, ldrFbo, ldrBox, &tr.ssgiShader, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
}

void RB_ScreenSpaceReflections( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = pow( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    GLSL_BindProgram( &tr.ssrShader );
    
    GLSL_SetUniformInt( &tr.ssrShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
    GL_BindToTMU( hdrFbo->colorImage[0], TB_DIFFUSEMAP );
    
    GLSL_SetUniformInt( &tr.ssrShader, UNIFORM_GLOWMAP, TB_GLOWMAP );
    
    // Use the most blurred version of glow...
    if( r_anamorphic->integer )
    {
        GL_BindToTMU( tr.anamorphicRenderFBOImage[0], TB_GLOWMAP );
    }
    else if( r_bloom->integer )
    {
        GL_BindToTMU( tr.bloomRenderFBOImage[0], TB_GLOWMAP );
    }
    else
    {
        GL_BindToTMU( tr.glowFboScaled[0]->colorImage[0], TB_GLOWMAP );
    }
    
    GLSL_SetUniformInt( &tr.ssrShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP );
    GL_BindToTMU( tr.renderDepthImage, TB_LIGHTMAP );
    
    
    GLSL_SetUniformMat4( &tr.ssrShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection );
    
    vec4_t local1;
    VectorSet4( local1, r_ssr->integer, r_sse->integer, r_ssrStrength->value, r_sseStrength->value );
    GLSL_SetUniformVec4( &tr.ssrShader, UNIFORM_LOCAL1, local1 );
    
    vec4_t local3;
    VectorSet4( local3, 1.0, 0.0, 0.0, 1.0 );
    GLSL_SetUniformVec4( &tr.ssrShader, UNIFORM_LOCAL3, local3 );
    
    {
        vec2_t screensize;
        screensize[0] = glConfig.vidWidth;
        screensize[1] = glConfig.vidHeight;
        
        GLSL_SetUniformVec2( &tr.ssrShader, UNIFORM_DIMENSIONS, screensize );
    }
    
    {
        vec4_t viewInfo;
        //float32 zmax = 2048.0;
        float32 zmax = backEnd.viewParms.zFar;
        float32 ymax = zmax * tan( backEnd.viewParms.fovY * M_PI / 360.0f );
        float32 xmax = zmax * tan( backEnd.viewParms.fovX * M_PI / 360.0f );
        float32 zmin = r_znear->value;
        VectorSet4( viewInfo, zmin, zmax, zmax / zmin, backEnd.viewParms.fovX );
        GLSL_SetUniformVec4( &tr.ssrShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    //FBO_Blit(hdrFbo, hdrBox, nullptr, ldrFbo, ldrBox, &tr.ssrShader, color, 0);
    
    FBO_Blit( hdrFbo, nullptr, nullptr, tr.genericFbo2, nullptr, &tr.ssrShader, color, 0 );
    
    // Combine render and hbao...
    GLSL_BindProgram( &tr.ssrCombineShader );
    
    GLSL_SetUniformMat4( &tr.ssrCombineShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection );
    GLSL_SetUniformMat4( &tr.ssrCombineShader, UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix );
    
    GLSL_SetUniformInt( &tr.ssrCombineShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
    GL_BindToTMU( hdrFbo->colorImage[0], TB_DIFFUSEMAP );
    
    GLSL_SetUniformInt( &tr.ssrCombineShader, UNIFORM_NORMALMAP, TB_NORMALMAP );
    GL_BindToTMU( tr.genericFBO2Image, TB_NORMALMAP );
    
    vec2_t screensize;
    screensize[0] = tr.genericFBO2Image->width;
    screensize[1] = tr.genericFBO2Image->height;
    GLSL_SetUniformVec2( &tr.ssrCombineShader, UNIFORM_DIMENSIONS, screensize );
    
    FBO_Blit( hdrFbo, nullptr, nullptr, ldrFbo, nullptr, &tr.ssrCombineShader, color, 0 );
}

void RB_Underwater( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = powf( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    GLSL_BindProgram( &tr.underWaterShader );
    GL_BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    GLSL_SetUniformMat4( &tr.underWaterShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection );
    
    GLSL_SetUniformFloat( &tr.underWaterShader, UNIFORM_TIME, static_cast<float32>( backEnd.refdef.floatTime ) * 5.0f/*tr.refdef.floatTime*/ );
    
    {
        vec2_t screensize;
        screensize[0] = static_cast<float32>( glConfig.vidWidth );
        screensize[1] = static_cast<float32>( glConfig.vidHeight );
        
        GLSL_SetUniformVec2( &tr.underWaterShader, UNIFORM_DIMENSIONS, screensize );
    }
    
    FBO_Blit( hdrFbo, hdrBox, nullptr, ldrFbo, ldrBox, &tr.underWaterShader, color, 0 );
}
