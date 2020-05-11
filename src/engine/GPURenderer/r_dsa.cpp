////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2016 James Canete
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
// File name:   r_dsa.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2017, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.h>

static struct
{
    U32 textures[NUM_TEXTURE_BUNDLES];
    U32 texunit;
    
    U32 program;
    
    U32 drawFramebuffer;
    U32 readFramebuffer;
    U32 renderbuffer;
}
glDsaState;

void GL_BindNullTextures()
{
    S32 i;
    
    if( glRefConfig.directStateAccess )
    {
        for( i = 0; i < NUM_TEXTURE_BUNDLES; i++ )
        {
            qglBindMultiTextureEXT( GL_TEXTURE0 + i, GL_TEXTURE_2D, 0 );
            glDsaState.textures[i] = 0;
        }
    }
    else
    {
        for( i = 0; i < NUM_TEXTURE_BUNDLES; i++ )
        {
            qglActiveTexture( GL_TEXTURE0 + i );
            qglBindTexture( GL_TEXTURE_2D, 0 );
            glDsaState.textures[i] = 0;
        }
        
        qglActiveTexture( GL_TEXTURE0 );
        glDsaState.texunit = GL_TEXTURE0;
    }
}

S32 GL_BindMultiTexture( U32 texunit, U32 target, U32 texture )
{
    U32 tmu = texunit - GL_TEXTURE0;
    
    if( glDsaState.textures[tmu] == texture )
        return 0;
        
    if( target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z )
        target = GL_TEXTURE_CUBE_MAP;
        
    qglBindMultiTextureEXT( texunit, target, texture );
    glDsaState.textures[tmu] = texture;
    return 1;
}

void GLDSA_BindMultiTextureEXT( U32 texunit, U32 target, U32 texture )
{
    if( glDsaState.texunit != texunit )
    {
        qglActiveTexture( texunit );
        glDsaState.texunit = texunit;
    }
    
    qglBindTexture( target, texture );
}

void GLDSA_TextureParameterfEXT( U32 texture, U32 target, U32 pname, F32 param )
{
    GL_BindMultiTexture( glDsaState.texunit, target, texture );
    qglTexParameterf( target, pname, param );
}

void GLDSA_TextureParameteriEXT( U32 texture, U32 target, U32 pname, S32 param )
{
    GL_BindMultiTexture( glDsaState.texunit, target, texture );
    qglTexParameteri( target, pname, param );
}

void GLDSA_TextureImage2DEXT( U32 texture, U32 target, S32 level, S32 internalformat, S32 width, S32 height, S32 border, U32 format, U32 type, const void* pixels )
{
    GL_BindMultiTexture( glDsaState.texunit, target, texture );
    qglTexImage2D( target, level, internalformat, width, height, border, format, type, pixels );
}

void GLDSA_TextureSubImage2DEXT( U32 texture, U32 target, S32 level, S32 xoffset, S32 yoffset, S32 width, S32 height, U32 format, U32 type, const void* pixels )
{
    GL_BindMultiTexture( glDsaState.texunit, target, texture );
    qglTexSubImage2D( target, level, xoffset, yoffset, width, height, format, type, pixels );
}

void GLDSA_CopyTextureSubImage2DEXT( U32 texture, U32 target, S32 level, S32 xoffset, S32 yoffset, S32 x, S32 y, S32 width, S32 height )
{
    GL_BindMultiTexture( glDsaState.texunit, target, texture );
    qglCopyTexSubImage2D( target, level, xoffset, yoffset, x, y, width, height );
}

void  GLDSA_CompressedTextureImage2DEXT( U32 texture, U32 target, S32 level, U32 internalformat, S32 width, S32 height, S32 border, S32 imageSize, const void* data )
{
    GL_BindMultiTexture( glDsaState.texunit, target, texture );
    qglCompressedTexImage2D( target, level, internalformat, width, height, border, imageSize, data );
}

void GLDSA_CompressedTextureSubImage2DEXT( U32 texture, U32 target, S32 level, S32 xoffset, S32 yoffset, S32 width, S32 height, U32 format, S32 imageSize, const void* data )
{
    GL_BindMultiTexture( glDsaState.texunit, target, texture );
    qglCompressedTexSubImage2D( target, level, xoffset, yoffset, width, height, format, imageSize, data );
}

void GLDSA_GenerateTextureMipmapEXT( U32 texture, U32 target )
{
    GL_BindMultiTexture( glDsaState.texunit, target, texture );
    qglGenerateMipmap( target );
}

void GL_BindNullProgram()
{
    qglUseProgram( 0 );
    glDsaState.program = 0;
}

S32 GL_UseProgram( U32 program )
{
    if( glDsaState.program == program )
        return 0;
        
    qglUseProgram( program );
    glDsaState.program = program;
    return 1;
}

void GLDSA_ProgramUniform1iEXT( U32 program, S32 location, S32 v0 )
{
    GL_UseProgram( program );
    qglUniform1i( location, v0 );
}

void GLDSA_ProgramUniform1fEXT( U32 program, S32 location, F32 v0 )
{
    GL_UseProgram( program );
    qglUniform1f( location, v0 );
}

void GLDSA_ProgramUniform2fEXT( U32 program, S32 location,
                                F32 v0, F32 v1 )
{
    GL_UseProgram( program );
    qglUniform2f( location, v0, v1 );
}

void GLDSA_ProgramUniform2fvEXT( U32 program, S32 location, S32 count, const F32* value )
{
    GL_UseProgram( program );
    qglUniform2fv( program, location, count, value );
}

void GLDSA_ProgramUniform3fEXT( U32 program, S32 location,
                                F32 v0, F32 v1, F32 v2 )
{
    GL_UseProgram( program );
    qglUniform3f( location, v0, v1, v2 );
}

void GLDSA_ProgramUniform4fEXT( U32 program, S32 location,
                                F32 v0, F32 v1, F32 v2, F32 v3 )
{
    GL_UseProgram( program );
    qglUniform4f( location, v0, v1, v2, v3 );
}

void GLDSA_ProgramUniform1fvEXT( U32 program, S32 location, S32 count, const F32* value )
{
    GL_UseProgram( program );
    qglUniform1fv( location, count, value );
}

void GLDSA_ProgramUniform3fEXT( U32 program, S32 location, S32 count, const F32* value )
{
    GL_UseProgram( program );
    qglUniform3fv( location, count, value );
}

void GLDSA_ProgramUniformMatrix4fvEXT( U32 program, S32 location, S32 count, U8 transpose, const F32* value )
{
    GL_UseProgram( program );
    qglUniformMatrix4fv( location, count, transpose, value );
}

void GL_BindNullFramebuffers()
{
    qglBindFramebuffer( GL_FRAMEBUFFER, 0 );
    glDsaState.drawFramebuffer = glDsaState.readFramebuffer = 0;
    qglBindRenderbuffer( GL_RENDERBUFFER, 0 );
    glDsaState.renderbuffer = 0;
}

void GL_BindFramebuffer( U32 target, U32 framebuffer )
{
    switch( target )
    {
        case GL_FRAMEBUFFER:
            if( framebuffer != glDsaState.drawFramebuffer || framebuffer != glDsaState.readFramebuffer )
            {
                qglBindFramebuffer( target, framebuffer );
                glDsaState.drawFramebuffer = glDsaState.readFramebuffer = framebuffer;
            }
            break;
            
        case GL_DRAW_FRAMEBUFFER:
            if( framebuffer != glDsaState.drawFramebuffer )
            {
                qglBindFramebuffer( target, framebuffer );
                glDsaState.drawFramebuffer = framebuffer;
            }
            break;
            
        case GL_READ_FRAMEBUFFER:
            if( framebuffer != glDsaState.readFramebuffer )
            {
                qglBindFramebuffer( target, framebuffer );
                glDsaState.readFramebuffer = framebuffer;
            }
            break;
    }
}

void GL_BindRenderbuffer( U32 renderbuffer )
{
    if( renderbuffer != glDsaState.renderbuffer )
    {
        qglBindRenderbuffer( GL_RENDERBUFFER, renderbuffer );
        glDsaState.renderbuffer = renderbuffer;
    }
}

void GL_BindFragDataLocation( U32 program, U32 color, StringEntry name )
{
    qglBindFragDataLocation( program, color, name );
}

void GLDSA_NamedRenderbufferStorageEXT( U32 renderbuffer, U32 internalformat, S32 width, S32 height )
{
    GL_BindRenderbuffer( renderbuffer );
    qglRenderbufferStorage( GL_RENDERBUFFER, internalformat, width, height );
}

void GLDSA_NamedRenderbufferStorageMultisampleEXT( U32 renderbuffer, S32 samples, U32 internalformat, S32 width, S32 height )
{
    GL_BindRenderbuffer( renderbuffer );
    qglRenderbufferStorageMultisample( GL_RENDERBUFFER, samples, internalformat, width, height );
}

U32 GLDSA_CheckNamedFramebufferStatusEXT( U32 framebuffer, U32 target )
{
    GL_BindFramebuffer( target, framebuffer );
    return qglCheckFramebufferStatus( target );
}

void GLDSA_NamedFramebufferTexture2DEXT( U32 framebuffer, U32 attachment, U32 textarget, U32 texture, S32 level )
{
    GL_BindFramebuffer( GL_FRAMEBUFFER, framebuffer );
    qglFramebufferTexture2D( GL_FRAMEBUFFER, attachment, textarget, texture, level );
}

void GLDSA_NamedFramebufferRenderbufferEXT( U32 framebuffer, U32 attachment, U32 renderbuffertarget, U32 renderbuffer )
{
    GL_BindFramebuffer( GL_FRAMEBUFFER, framebuffer );
    qglFramebufferRenderbuffer( GL_FRAMEBUFFER, attachment, renderbuffertarget, renderbuffer );
}

void GLDSA_CopyTexImage2DEXT( U32 framebuffer, U32 target, S32 level, U32 internalformat, S32 x, S32 y, S32 width, S32 height, S32 border )
{
    GL_BindRenderbuffer( framebuffer );
    qglCopyTexImage2DEXT( target, level, internalformat, x, y, width, height, border );
}

void GL_PatchParameteri( U32 pname, S32 value )
{
    qglPatchParameteri( pname, value );
}


