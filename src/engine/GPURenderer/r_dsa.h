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
// File name:   r_dsa.h
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2017, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_DSA_H__
#define __R_DSA_H__

void GL_BindNullTextures( void );
S32 GL_BindMultiTexture( U32 texunit, U32 target, U32 texture );

void GLDSA_BindMultiTextureEXT( U32 texunit, U32 target, U32 texture );
void GLDSA_TextureParameterfEXT( U32 texture, U32 target, U32 pname, F32 param );
void GLDSA_TextureParameteriEXT( U32 texture, U32 target, U32 pname, S32 param );
void GLDSA_TextureImage2DEXT( U32 texture, U32 target, S32 level, S32 internalformat, S32 width, S32 height, S32 border, U32 format, U32 type, const void* pixels );
void GLDSA_TextureSubImage2DEXT( U32 texture, U32 target, S32 level, S32 xoffset, S32 yoffset, S32 width, S32 height, U32 format, U32 type, const void* pixels );
void GLDSA_CopyTextureSubImage2DEXT( U32 texture, U32 target, S32 level, S32 xoffset, S32 yoffset, S32 x, S32 y, S32 width, S32 height );
void GLDSA_CompressedTextureImage2DEXT( U32 texture, U32 target, S32 level, U32 internalformat, S32 width, S32 height, S32 border, S32 imageSize, const void* data );
void GLDSA_CompressedTextureSubImage2DEXT( U32 texture, U32 target, S32 level, S32 xoffset, S32 yoffset, S32 width, S32 height, U32 format, S32 imageSize, const void* data );
void GLDSA_GenerateTextureMipmapEXT( U32 texture, U32 target );

void GL_BindNullProgram( void );
S32 GL_UseProgram( U32 program );

void GLDSA_ProgramUniform1iEXT( U32 program, S32 location, S32 v0 );
void GLDSA_ProgramUniform1fEXT( U32 program, S32 location, F32 v0 );
void GLDSA_ProgramUniform2fEXT( U32 program, S32 location, F32 v0, F32 v1 );
void GLDSA_ProgramUniform2fvEXT( U32 program, S32 location, S32 count, const F32* value );
void GLDSA_ProgramUniform3fEXT( U32 program, S32 location, F32 v0, F32 v1, F32 v2 );
void GLDSA_ProgramUniform4fEXT( U32 program, S32 location, F32 v0, F32 v1, F32 v2, F32 v3 );
void GLDSA_ProgramUniform1fvEXT( U32 program, S32 location, S32 count, const F32* value );
void GLDSA_ProgramUniformMatrix4fvEXT( U32 program, S32 location, S32 count, U8 transpose, const F32* value );
void GLDSA_ProgramUniform3fEXT( U32 program, S32 location, S32 count, const F32* value );
void GL_BindNullFramebuffers( void );
void GL_BindFramebuffer( U32 target, U32 framebuffer );
void GL_BindRenderbuffer( U32 renderbuffer );
void GL_BindFragDataLocation( U32 program, U32 color, StringEntry name );

void GLDSA_NamedRenderbufferStorageEXT( U32 renderbuffer, U32 internalformat, S32 width, S32 height );
void GLDSA_NamedRenderbufferStorageMultisampleEXT( U32 renderbuffer, S32 samples, U32 internalformat, S32 width, S32 height );
U32 GLDSA_CheckNamedFramebufferStatusEXT( U32 framebuffer, U32 target );
void GLDSA_NamedFramebufferTexture2DEXT( U32 framebuffer, U32 attachment, U32 textarget, U32 texture, S32 level );
void GLDSA_NamedFramebufferRenderbufferEXT( U32 framebuffer, U32 attachment, U32 renderbuffertarget, U32 renderbuffer );
void GLDSA_CopyTexImage2DEXT( U32 framebuffer, U32 target, S32 level, U32 internalformat, S32 x, S32 y, S32 width, S32 height, S32 border );

void GL_PatchParameteri( U32 pname, S32 value );

#endif
