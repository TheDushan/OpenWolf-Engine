////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2006 Kirk Barnes
// Copyright(C) 2006 - 2008 Robert Beckebans <trebor_7@users.sourceforge.net>
// Copyright(C) 2011 - 2018 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   r_fbo.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2015
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.h>

/*
=============
R_CheckFBO
=============
*/
bool R_CheckFBO( const FBO_t* fbo )
{
    GLenum code = qglCheckNamedFramebufferStatusEXT( fbo->frameBuffer, GL_FRAMEBUFFER );
    
    if( code == GL_FRAMEBUFFER_COMPLETE )
        return true;
        
    // an error occured
    switch( code )
    {
        case GL_FRAMEBUFFER_UNSUPPORTED:
            CL_RefPrintf( PRINT_WARNING, "R_CheckFBO: (%s) Unsupported framebuffer format\n", fbo->name );
            break;
            
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            CL_RefPrintf( PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete attachment\n", fbo->name );
            break;
            
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            CL_RefPrintf( PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete, missing attachment\n", fbo->name );
            break;
            
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            CL_RefPrintf( PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete, missing draw buffer\n", fbo->name );
            break;
            
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            CL_RefPrintf( PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete, missing read buffer\n", fbo->name );
            break;
            
        default:
            CL_RefPrintf( PRINT_WARNING, "R_CheckFBO: (%s) unknown error 0x%X\n", fbo->name, code );
            break;
    }
    
    return false;
}

/*
============
FBO_Create
============
*/
FBO_t*          FBO_Create( pointer name, sint width, sint height )
{
    FBO_t* fbo = nullptr;
    
    if( strlen( name ) >= MAX_QPATH )
    {
        Com_Error( ERR_DROP, "FBO_Create: \"%s\" is too long", name );
    }
    
    if( width <= 0 || width > glRefConfig.maxRenderbufferSize )
    {
        Com_Error( ERR_DROP, "FBO_Create: bad width %i", width );
    }
    
    if( height <= 0 || height > glRefConfig.maxRenderbufferSize )
    {
        Com_Error( ERR_DROP, "FBO_Create: bad height %i", height );
    }
    
    if( tr.numFBOs == MAX_FBOS )
    {
        Com_Error( ERR_DROP, "FBO_Create: MAX_FBOS hit" );
    }
    
    fbo = tr.fbos[tr.numFBOs] = reinterpret_cast<FBO_t*>( Hunk_Alloc( sizeof( *fbo ), h_low ) );
    Q_strncpyz( fbo->name, name, sizeof( fbo->name ) );
    fbo->index = tr.numFBOs++;
    fbo->width = width;
    fbo->height = height;
    
    qglGenFramebuffers( 1, &fbo->frameBuffer );
    
    return fbo;
}

/*
=================
FBO_CreateBuffer
=================
*/
void FBO_CreateBuffer( FBO_t* fbo, sint format, sint index, sint multisample )
{
    uint* pRenderBuffer;
    GLenum attachment;
    bool absent;
    
    switch( format )
    {
        case GL_RGB:
        case GL_RGBA:
        case GL_RGB8:
        case GL_RGBA8:
        case GL_RGB16F:
        case GL_RGBA16F:
        case GL_RGB32F:
        case GL_RGBA32F:
            fbo->colorFormat = format;
            pRenderBuffer = &fbo->colorBuffers[index];
            attachment = GL_COLOR_ATTACHMENT0 + index;
            break;
            
        case GL_DEPTH_COMPONENT:
        case GL_DEPTH_COMPONENT16:
        case GL_DEPTH_COMPONENT24:
        case GL_DEPTH_COMPONENT32:
            fbo->depthFormat = format;
            pRenderBuffer = &fbo->depthBuffer;
            attachment = GL_DEPTH_ATTACHMENT;
            break;
            
        case GL_STENCIL_INDEX:
        case GL_STENCIL_INDEX1:
        case GL_STENCIL_INDEX4:
        case GL_STENCIL_INDEX8:
        case GL_STENCIL_INDEX16:
            fbo->stencilFormat = format;
            pRenderBuffer = &fbo->stencilBuffer;
            attachment = GL_STENCIL_ATTACHMENT;
            break;
            
        case GL_DEPTH_STENCIL:
        case GL_DEPTH24_STENCIL8:
            fbo->packedDepthStencilFormat = format;
            pRenderBuffer = &fbo->packedDepthStencilBuffer;
            attachment = 0; // special for stencil and depth
            break;
            
        default:
            CL_RefPrintf( PRINT_WARNING, "FBO_CreateBuffer: invalid format %d\n", format );
            return;
    }
    
    absent = *pRenderBuffer == 0;
    if( absent )
        qglGenRenderbuffers( 1, pRenderBuffer );
        
    if( multisample && glRefConfig.framebufferMultisample )
        qglNamedRenderbufferStorageMultisampleEXT( *pRenderBuffer, multisample, format, fbo->width, fbo->height );
    else
        qglNamedRenderbufferStorageEXT( *pRenderBuffer, format, fbo->width, fbo->height );
        
    if( absent )
    {
        if( attachment == 0 )
        {
            qglNamedFramebufferRenderbufferEXT( fbo->frameBuffer, GL_DEPTH_ATTACHMENT,   GL_RENDERBUFFER, *pRenderBuffer );
            qglNamedFramebufferRenderbufferEXT( fbo->frameBuffer, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *pRenderBuffer );
        }
        else
        {
            qglNamedFramebufferRenderbufferEXT( fbo->frameBuffer, attachment, GL_RENDERBUFFER, *pRenderBuffer );
        }
    }
}


/*
=================
FBO_AttachImage
=================
*/
void FBO_AttachImage( FBO_t* fbo, image_t* image, GLenum attachment, GLuint cubemapside )
{
    GLenum target = GL_TEXTURE_2D;
    sint index;
    
    if( image->flags & IMGFLAG_CUBEMAP )
        target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + cubemapside;
        
    qglNamedFramebufferTexture2DEXT( fbo->frameBuffer, attachment, target, image->texnum, 0 );
    index = attachment - GL_COLOR_ATTACHMENT0;
    if( index >= 0 && index <= 15 )
        fbo->colorImage[index] = image;
}

/*
============
FBO_Bind
============
*/
void FBO_Bind( FBO_t* fbo )
{
    if( !glRefConfig.framebufferObject )
    {
        CL_RefPrintf( PRINT_WARNING, "FBO_Bind() called without framebuffers enabled!\n" );
        return;
    }
    
    if( glState.currentFBO == fbo )
        return;
        
    if( r_logFile->integer )
    {
        // don't just call LogComment, or we will get a call to va() every frame!
        GLimp_LogComment( reinterpret_cast< valueType* >( va( "--- FBO_Bind( %s ) ---\n", fbo ? fbo->name : "nullptr" ) ) );
    }
    
    GL_BindFramebuffer( GL_FRAMEBUFFER, fbo ? fbo->frameBuffer : 0 );
    glState.currentFBO = fbo;
}

/*
============
idRenderSystemLocal::FBOInit
============
*/
void idRenderSystemLocal::FBOInit( void )
{
    sint             i;
    sint             hdrFormat, multisample = 0;
    
    CL_RefPrintf( PRINT_ALL, "------- idRenderSystemLocal::FBOInit -------\n" );
    
    if( !glRefConfig.framebufferObject )
        return;
        
    tr.numFBOs = 0;
    
    GL_CheckErrors();
    
    R_IssuePendingRenderCommands();
    
    hdrFormat = GL_RGBA8;
    if( r_hdr->integer && glRefConfig.textureFloat )
        hdrFormat = GL_RGBA16F;
        
    if( glRefConfig.framebufferMultisample )
        qglGetIntegerv( GL_MAX_SAMPLES, &multisample );
        
    if( r_ext_framebuffer_multisample->integer < multisample )
        multisample = r_ext_framebuffer_multisample->integer;
        
    if( multisample < 2 || !glRefConfig.framebufferBlit )
        multisample = 0;
        
    if( multisample != r_ext_framebuffer_multisample->integer )
        cvarSystem->SetValue( "r_ext_framebuffer_multisample", ( float32 )multisample );
        
    //
    // Generic FBO...
    //
    {
        tr.genericFbo = FBO_Create( "_generic", tr.genericFBOImage->width, tr.genericFBOImage->height );
        FBO_Bind( tr.genericFbo );
        FBO_AttachImage( tr.genericFbo, tr.genericFBOImage, GL_COLOR_ATTACHMENT0, 0 );
        R_CheckFBO( tr.genericFbo );
    }
    
    {
        tr.genericFbo2 = FBO_Create( "_generic2", tr.genericFBO2Image->width, tr.genericFBO2Image->height );
        FBO_Bind( tr.genericFbo2 );
        FBO_AttachImage( tr.genericFbo2, tr.genericFBO2Image, GL_COLOR_ATTACHMENT0_EXT, 0 );
        R_CheckFBO( tr.genericFbo2 );
    }
    
    //
    // Bloom VBO's...
    //
    {
        tr.bloomRenderFBO[0] = FBO_Create( "_bloom0", tr.bloomRenderFBOImage[0]->width, tr.bloomRenderFBOImage[0]->height );
        FBO_Bind( tr.bloomRenderFBO[0] );
        FBO_AttachImage( tr.bloomRenderFBO[0], tr.bloomRenderFBOImage[0], GL_COLOR_ATTACHMENT0, 0 );
        R_CheckFBO( tr.bloomRenderFBO[0] );
        
        
        tr.bloomRenderFBO[1] = FBO_Create( "_bloom1", tr.bloomRenderFBOImage[1]->width, tr.bloomRenderFBOImage[1]->height );
        FBO_Bind( tr.bloomRenderFBO[1] );
        FBO_AttachImage( tr.bloomRenderFBO[1], tr.bloomRenderFBOImage[1], GL_COLOR_ATTACHMENT0, 0 );
        R_CheckFBO( tr.bloomRenderFBO[1] );
        
        
        tr.bloomRenderFBO[2] = FBO_Create( "_bloom2", tr.bloomRenderFBOImage[2]->width, tr.bloomRenderFBOImage[2]->height );
        FBO_Bind( tr.bloomRenderFBO[2] );
        FBO_AttachImage( tr.bloomRenderFBO[2], tr.bloomRenderFBOImage[2], GL_COLOR_ATTACHMENT0, 0 );
        R_CheckFBO( tr.bloomRenderFBO[2] );
    }
    
    //
    // Anamorphic VBO's...
    //
    {
        tr.anamorphicRenderFBO[0] = FBO_Create( "_anamorphic0", tr.anamorphicRenderFBOImage[0]->width, tr.anamorphicRenderFBOImage[0]->height );
        FBO_AttachImage( tr.anamorphicRenderFBO[0], tr.anamorphicRenderFBOImage[0], GL_COLOR_ATTACHMENT0, 0 );
        R_CheckFBO( tr.anamorphicRenderFBO[0] );
        
        tr.anamorphicRenderFBO[1] = FBO_Create( "_anamorphic1", tr.anamorphicRenderFBOImage[1]->width, tr.anamorphicRenderFBOImage[1]->height );
        FBO_AttachImage( tr.anamorphicRenderFBO[1], tr.anamorphicRenderFBOImage[1], GL_COLOR_ATTACHMENT0, 0 );
        R_CheckFBO( tr.anamorphicRenderFBO[1] );
        
        tr.anamorphicRenderFBO[2] = FBO_Create( "_anamorphic2", tr.anamorphicRenderFBOImage[2]->width, tr.anamorphicRenderFBOImage[2]->height );
        FBO_AttachImage( tr.anamorphicRenderFBO[2], tr.anamorphicRenderFBOImage[2], GL_COLOR_ATTACHMENT0, 0 );
        R_CheckFBO( tr.anamorphicRenderFBO[2] );
    }
    
    {
        tr.renderFbo = FBO_Create( "_render", tr.renderDepthImage->width, tr.renderDepthImage->height );
        FBO_AttachImage( tr.renderFbo, tr.renderImage, GL_COLOR_ATTACHMENT0, 0 );
        FBO_AttachImage( tr.renderFbo, tr.renderDepthImage, GL_DEPTH_ATTACHMENT, 0 );
        FBO_AttachImage( tr.renderFbo, tr.normalDetailedImage, GL_COLOR_ATTACHMENT0_EXT, 0 );
        R_CheckFBO( tr.renderFbo );
    }
    
    // clear render buffer
    // this fixes the corrupt screen bug with r_hdr 1 on older hardware
    if( tr.renderFbo )
    {
        GL_BindFramebuffer( GL_FRAMEBUFFER, tr.renderFbo->frameBuffer );
        qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    }
    
    if( tr.screenScratchImage )
    {
        tr.screenScratchFbo = FBO_Create( "screenScratch", tr.screenScratchImage->width, tr.screenScratchImage->height );
        FBO_AttachImage( tr.screenScratchFbo, tr.screenScratchImage, GL_COLOR_ATTACHMENT0, 0 );
        FBO_AttachImage( tr.screenScratchFbo, tr.renderDepthImage, GL_DEPTH_ATTACHMENT, 0 );
        R_CheckFBO( tr.screenScratchFbo );
    }
    
    if( tr.sunRaysImage )
    {
        tr.sunRaysFbo = FBO_Create( "_sunRays", tr.renderDepthImage->width, tr.renderDepthImage->height );
        FBO_AttachImage( tr.sunRaysFbo, tr.sunRaysImage, GL_COLOR_ATTACHMENT0, 0 );
        FBO_AttachImage( tr.sunRaysFbo, tr.renderDepthImage, GL_DEPTH_ATTACHMENT, 0 );
        R_CheckFBO( tr.sunRaysFbo );
    }
    
    if( MAX_DRAWN_PSHADOWS && tr.pshadowMaps[0] )
    {
        for( i = 0; i < MAX_DRAWN_PSHADOWS; i++ )
        {
            tr.pshadowFbos[i] = FBO_Create( va( "_shadowmap%d", i ), tr.pshadowMaps[i]->width, tr.pshadowMaps[i]->height );
            // FIXME: this next line wastes 16mb with 16x512x512 sun shadow maps, skip if OpenGL 4.3+ or ARB_framebuffer_no_attachments
            FBO_CreateBuffer( tr.pshadowFbos[i], GL_RGBA8, 0, 0 );
            FBO_AttachImage( tr.pshadowFbos[i], tr.pshadowMaps[i], GL_DEPTH_ATTACHMENT, 0 );
            R_CheckFBO( tr.pshadowFbos[i] );
        }
    }
    
    if( tr.sunShadowDepthImage[0] )
    {
        for( i = 0; i < 4; i++ )
        {
            tr.sunShadowFbo[i] = FBO_Create( "_sunshadowmap", tr.sunShadowDepthImage[i]->width, tr.sunShadowDepthImage[i]->height );
            // FIXME: this next line wastes 16mb with 4x1024x1024 sun shadow maps, skip if OpenGL 4.3+ or ARB_framebuffer_no_attachments
            // This at least gets sun shadows working on older GPUs (Intel)
            FBO_CreateBuffer( tr.sunShadowFbo[i], GL_RGBA8, 0, 0 );
            FBO_AttachImage( tr.sunShadowFbo[i], tr.sunShadowDepthImage[i], GL_DEPTH_ATTACHMENT, 0 );
            R_CheckFBO( tr.sunShadowFbo[i] );
        }
    }
    
    if( tr.screenShadowImage )
    {
        tr.screenShadowFbo = FBO_Create( "_screenshadow", tr.screenShadowImage->width, tr.screenShadowImage->height );
        FBO_AttachImage( tr.screenShadowFbo, tr.screenShadowImage, GL_COLOR_ATTACHMENT0, 0 );
        R_CheckFBO( tr.screenShadowFbo );
    }
    
    if( tr.textureScratchImage[0] )
    {
        for( i = 0; i < 2; i++ )
        {
            tr.textureScratchFbo[i] = FBO_Create( va( "_texturescratch%d", i ), tr.textureScratchImage[i]->width, tr.textureScratchImage[i]->height );
            FBO_AttachImage( tr.textureScratchFbo[i], tr.textureScratchImage[i], GL_COLOR_ATTACHMENT0, 0 );
            R_CheckFBO( tr.textureScratchFbo[i] );
        }
    }
    
    if( tr.calcLevelsImage )
    {
        tr.calcLevelsFbo = FBO_Create( "_calclevels", tr.calcLevelsImage->width, tr.calcLevelsImage->height );
        FBO_AttachImage( tr.calcLevelsFbo, tr.calcLevelsImage, GL_COLOR_ATTACHMENT0, 0 );
        R_CheckFBO( tr.calcLevelsFbo );
    }
    
    if( tr.targetLevelsImage )
    {
        tr.targetLevelsFbo = FBO_Create( "_targetlevels", tr.targetLevelsImage->width, tr.targetLevelsImage->height );
        FBO_AttachImage( tr.targetLevelsFbo, tr.targetLevelsImage, GL_COLOR_ATTACHMENT0, 0 );
        R_CheckFBO( tr.targetLevelsFbo );
    }
    
    if( tr.quarterImage[0] )
    {
        for( i = 0; i < 2; i++ )
        {
            tr.quarterFbo[i] = FBO_Create( va( "_quarter%d", i ), tr.quarterImage[i]->width, tr.quarterImage[i]->height );
            FBO_AttachImage( tr.quarterFbo[i], tr.quarterImage[i], GL_COLOR_ATTACHMENT0, 0 );
            R_CheckFBO( tr.quarterFbo[i] );
        }
    }
    
    if( tr.hdrDepthImage )
    {
        tr.hdrDepthFbo = FBO_Create( "_hdrDepth", tr.hdrDepthImage->width, tr.hdrDepthImage->height );
        FBO_AttachImage( tr.hdrDepthFbo, tr.hdrDepthImage, GL_COLOR_ATTACHMENT0, 0 );
        R_CheckFBO( tr.hdrDepthFbo );
    }
    
    if( tr.screenSsaoImage )
    {
        tr.screenSsaoFbo = FBO_Create( "_screenssao", tr.screenSsaoImage->width, tr.screenSsaoImage->height );
        FBO_AttachImage( tr.screenSsaoFbo, tr.screenSsaoImage, GL_COLOR_ATTACHMENT0, 0 );
        R_CheckFBO( tr.screenSsaoFbo );
    }
    
    if( tr.renderCubeImage != nullptr )
    {
        tr.renderCubeFbo = FBO_Create( "_renderCubeFbo", tr.renderCubeImage->width, tr.renderCubeImage->height );
        FBO_AttachImage( tr.renderCubeFbo, tr.renderCubeImage, GL_COLOR_ATTACHMENT0, 0 );
        glState.currentFBO->colorImage[0] = tr.renderCubeImage;
        glState.currentFBO->colorBuffers[0] = tr.renderCubeImage->texnum;
        FBO_CreateBuffer( tr.renderCubeFbo, GL_DEPTH_COMPONENT24, 0, 0 );
        R_CheckFBO( tr.renderCubeFbo );
    }
    
    {
        for( i = 0; i < ARRAY_LEN( tr.glowImageScaled ); i++ )
        {
            tr.glowFboScaled[i] = FBO_Create( va( "*glowScaled%d", i ), tr.glowImageScaled[i]->width, tr.glowImageScaled[i]->height );
            FBO_AttachImage( tr.glowFboScaled[i], tr.glowImageScaled[i], GL_COLOR_ATTACHMENT0, 0 );
            R_CheckFBO( tr.glowFboScaled[i] );
        }
    }
    
    if( tr.renderCubeImage != nullptr )
    {
        tr.preFilterEnvMapFbo = FBO_Create( "_preFilterEnvMapFbo", tr.renderCubeImage->width, tr.renderCubeImage->height );
        FBO_AttachImage( tr.preFilterEnvMapFbo, tr.prefilterEnvMapImage, GL_COLOR_ATTACHMENT0, 0 );
        R_CheckFBO( tr.preFilterEnvMapFbo );
    }
    
    GL_CheckErrors();
    
    GL_BindFramebuffer( GL_FRAMEBUFFER, 0 );
    glState.currentFBO = nullptr;
}

/*
============
idRenderSystemLocal::FBOShutdown
============
*/
void idRenderSystemLocal::FBOShutdown( void )
{
    sint             i, j;
    FBO_t*          fbo;
    
    CL_RefPrintf( PRINT_ALL, "------- idRenderSystemLocal::FBOShutdown -------\n" );
    
    if( !glRefConfig.framebufferObject )
        return;
        
    FBO_Bind( nullptr );
    
    for( i = 0; i < tr.numFBOs; i++ )
    {
        fbo = tr.fbos[i];
        
        for( j = 0; j < glRefConfig.maxColorAttachments; j++ )
        {
            if( fbo->colorBuffers[j] )
                qglDeleteRenderbuffers( 1, &fbo->colorBuffers[j] );
        }
        
        if( fbo->depthBuffer )
            qglDeleteRenderbuffers( 1, &fbo->depthBuffer );
            
        if( fbo->stencilBuffer )
            qglDeleteRenderbuffers( 1, &fbo->stencilBuffer );
            
        if( fbo->frameBuffer )
            qglDeleteFramebuffers( 1, &fbo->frameBuffer );
    }
}

/*
============
R_FBOList_f
============
*/
void R_FBOList_f( void )
{
    sint             i;
    FBO_t*          fbo;
    
    if( !glRefConfig.framebufferObject )
    {
        CL_RefPrintf( PRINT_ALL, "GL_EXT_framebuffer_object is not available.\n" );
        return;
    }
    
    CL_RefPrintf( PRINT_ALL, "             size       name\n" );
    CL_RefPrintf( PRINT_ALL, "----------------------------------------------------------\n" );
    
    for( i = 0; i < tr.numFBOs; i++ )
    {
        fbo = tr.fbos[i];
        
        CL_RefPrintf( PRINT_ALL, "  %4i: %4i %4i %s\n", i, fbo->width, fbo->height, fbo->name );
    }
    
    CL_RefPrintf( PRINT_ALL, " %i FBOs\n", tr.numFBOs );
}

void FBO_BlitFromTexture( struct image_s* src, vec4_t inSrcTexCorners, vec2_t inSrcTexScale, FBO_t* dst, ivec4_t inDstBox, struct shaderProgram_s* shaderProgram, vec4_t inColor, sint blend )
{
    ivec4_t dstBox;
    vec4_t color;
    vec4_t quadVerts[4];
    vec2_t texCoords[4];
    vec2_t invTexRes;
    FBO_t* oldFbo = glState.currentFBO;
    mat4_t projection;
    sint width, height;
    
    if( !src )
    {
        CL_RefPrintf( PRINT_WARNING, "Tried to blit from a nullptr texture!\n" );
        return;
    }
    
    width  = dst ? dst->width  : glConfig.vidWidth;
    height = dst ? dst->height : glConfig.vidHeight;
    
    if( inSrcTexCorners )
    {
        VectorSet2( texCoords[0], inSrcTexCorners[0], inSrcTexCorners[1] );
        VectorSet2( texCoords[1], inSrcTexCorners[2], inSrcTexCorners[1] );
        VectorSet2( texCoords[2], inSrcTexCorners[2], inSrcTexCorners[3] );
        VectorSet2( texCoords[3], inSrcTexCorners[0], inSrcTexCorners[3] );
    }
    else
    {
        VectorSet2( texCoords[0], 0.0f, 1.0f );
        VectorSet2( texCoords[1], 1.0f, 1.0f );
        VectorSet2( texCoords[2], 1.0f, 0.0f );
        VectorSet2( texCoords[3], 0.0f, 0.0f );
    }
    
    // framebuffers are 0 bottom, Y up.
    if( inDstBox )
    {
        dstBox[0] = inDstBox[0];
        dstBox[1] = height - inDstBox[1] - inDstBox[3];
        dstBox[2] = inDstBox[0] + inDstBox[2];
        dstBox[3] = height - inDstBox[1];
    }
    else
    {
        VectorSet4( dstBox, 0, height, width, 0 );
    }
    
    if( inSrcTexScale )
    {
        VectorCopy2( inSrcTexScale, invTexRes );
    }
    else
    {
        VectorSet2( invTexRes, 1.0f, 1.0f );
    }
    
    if( inColor )
    {
        VectorCopy4( inColor, color );
    }
    else
    {
        VectorCopy4( colorWhite, color );
    }
    
    if( !shaderProgram )
    {
        shaderProgram = &tr.textureColorShader;
    }
    
    FBO_Bind( dst );
    
    qglViewport( 0, 0, width, height );
    qglScissor( 0, 0, width, height );
    
    Mat4Ortho( 0, width, height, 0, 0, 1, projection );
    
    GL_Cull( CT_TWO_SIDED );
    
    GL_BindToTMU( src, TB_COLORMAP );
    
    VectorSet4( quadVerts[0], dstBox[0], dstBox[1], 0.0f, 1.0f );
    VectorSet4( quadVerts[1], dstBox[2], dstBox[1], 0.0f, 1.0f );
    VectorSet4( quadVerts[2], dstBox[2], dstBox[3], 0.0f, 1.0f );
    VectorSet4( quadVerts[3], dstBox[0], dstBox[3], 0.0f, 1.0f );
    
    invTexRes[0] /= src->width;
    invTexRes[1] /= src->height;
    
    GL_State( blend );
    
    GLSL_BindProgram( shaderProgram );
    
    GLSL_SetUniformMat4( shaderProgram, UNIFORM_MODELVIEWPROJECTIONMATRIX, projection );
    GLSL_SetUniformVec4( shaderProgram, UNIFORM_COLOR, color );
    GLSL_SetUniformVec2( shaderProgram, UNIFORM_INVTEXRES, invTexRes );
    GLSL_SetUniformVec2( shaderProgram, UNIFORM_AUTOEXPOSUREMINMAX, tr.refdef.autoExposureMinMax );
    GLSL_SetUniformVec3( shaderProgram, UNIFORM_TONEMINAVGMAXLINEAR, tr.refdef.toneMinAvgMaxLinear );
    
    RB_InstantQuad2( quadVerts, texCoords );
    
    FBO_Bind( oldFbo );
}

void FBO_Blit( FBO_t* src, ivec4_t inSrcBox, vec2_t srcTexScale, FBO_t* dst, ivec4_t dstBox, struct shaderProgram_s* shaderProgram, vec4_t color, sint blend )
{
    vec4_t srcTexCorners;
    
    if( !src )
    {
        CL_RefPrintf( PRINT_WARNING, "Tried to blit from a nullptr FBO!\n" );
        return;
    }
    
    if( inSrcBox )
    {
        srcTexCorners[0] =  inSrcBox[0]                / ( float32 )src->width;
        srcTexCorners[1] = ( inSrcBox[1] + inSrcBox[3] ) / ( float32 )src->height;
        srcTexCorners[2] = ( inSrcBox[0] + inSrcBox[2] ) / ( float32 )src->width;
        srcTexCorners[3] =  inSrcBox[1]                / ( float32 )src->height;
    }
    else
    {
        VectorSet4( srcTexCorners, 0.0f, 0.0f, 1.0f, 1.0f );
    }
    
    FBO_BlitFromTexture( src->colorImage[0], srcTexCorners, srcTexScale, dst, dstBox, shaderProgram, color, blend | GLS_DEPTHTEST_DISABLE );
}

void FBO_FastBlit( FBO_t* src, ivec4_t srcBox, FBO_t* dst, ivec4_t dstBox, sint buffers, sint filter )
{
    ivec4_t srcBoxFinal, dstBoxFinal;
    GLuint srcFb, dstFb;
    
    if( !glRefConfig.framebufferBlit )
    {
        FBO_Blit( src, srcBox, nullptr, dst, dstBox, nullptr, nullptr, 0 );
        return;
    }
    
    srcFb = src ? src->frameBuffer : 0;
    dstFb = dst ? dst->frameBuffer : 0;
    
    if( !srcBox )
    {
        sint width =  src ? src->width  : glConfig.vidWidth;
        sint height = src ? src->height : glConfig.vidHeight;
        
        VectorSet4( srcBoxFinal, 0, 0, width, height );
    }
    else
    {
        VectorSet4( srcBoxFinal, srcBox[0], srcBox[1], srcBox[0] + srcBox[2], srcBox[1] + srcBox[3] );
    }
    
    if( !dstBox )
    {
        sint width  = dst ? dst->width  : glConfig.vidWidth;
        sint height = dst ? dst->height : glConfig.vidHeight;
        
        VectorSet4( dstBoxFinal, 0, 0, width, height );
    }
    else
    {
        VectorSet4( dstBoxFinal, dstBox[0], dstBox[1], dstBox[0] + dstBox[2], dstBox[1] + dstBox[3] );
    }
    
    GL_BindFramebuffer( GL_READ_FRAMEBUFFER, srcFb );
    GL_BindFramebuffer( GL_DRAW_FRAMEBUFFER, dstFb );
    qglBlitFramebuffer( srcBoxFinal[0], srcBoxFinal[1], srcBoxFinal[2], srcBoxFinal[3],
                        dstBoxFinal[0], dstBoxFinal[1], dstBoxFinal[2], dstBoxFinal[3],
                        buffers, filter );
                        
    GL_BindFramebuffer( GL_FRAMEBUFFER, 0 );
    glState.currentFBO = nullptr;
}

void FBO_FastBlitIndexed( FBO_t* src, FBO_t* dst, sint srcReadBuffer, sint dstDrawBuffer, sint buffers, sint filter )
{
    assert( src != nullptr );
    assert( dst != nullptr );
    
    qglBindFramebuffer( GL_READ_FRAMEBUFFER, src->frameBuffer );
    qglReadBuffer( GL_COLOR_ATTACHMENT0 + srcReadBuffer );
    
    qglBindFramebuffer( GL_DRAW_FRAMEBUFFER, dst->frameBuffer );
    qglDrawBuffer( GL_COLOR_ATTACHMENT0 + dstDrawBuffer );
    
    qglBlitFramebuffer( 0, 0, src->width, src->height,
                        0, 0, dst->width, dst->height,
                        buffers, filter );
                        
    qglReadBuffer( GL_COLOR_ATTACHMENT0 );
    
    glState.currentFBO = dst;
    
    qglBindFramebuffer( GL_FRAMEBUFFER, 0 );
    glState.currentFBO = nullptr;
}