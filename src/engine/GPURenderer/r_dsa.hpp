////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2016 James Canete
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
// File name:   r_dsa.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_DSA_H__
#define __R_DSA_H__

void GL_BindNullTextures(void);
sint GL_BindMultiTexture(uint texunit, uint target, uint texture);

void GLDSA_BindMultiTextureEXT(uint texunit, uint target, uint texture);
void GLDSA_TextureParameterfEXT(uint texture, uint target, uint pname,
                                float32 param);
void GLDSA_TextureParameteriEXT(uint texture, uint target, uint pname,
                                sint param);
void GLDSA_TextureImage2DEXT(uint texture, uint target, sint level,
                             sint internalformat, sint width, sint height, sint border, uint format,
                             uint type, const void *pixels);
void GLDSA_TextureSubImage2DEXT(uint texture, uint target, sint level,
                                sint xoffset, sint yoffset, sint width, sint height, uint format,
                                uint type, const void *pixels);
void GLDSA_CopyTextureSubImage2DEXT(uint texture, uint target, sint level,
                                    sint xoffset, sint yoffset, sint x, sint y, sint width, sint height);
void GLDSA_CompressedTextureImage2DEXT(uint texture, uint target,
                                       sint level, uint internalformat, sint width, sint height, sint border,
                                       sint imageSize, const void *data);
void GLDSA_CompressedTextureSubImage2DEXT(uint texture, uint target,
        sint level, sint xoffset, sint yoffset, sint width, sint height,
        uint format, sint imageSize, const void *data);
void GLDSA_GenerateTextureMipmapEXT(uint texture, uint target);

void GL_BindNullProgram(void);
sint GL_UseProgram(uint program);

void GLDSA_ProgramUniform1iEXT(uint program, sint location, sint v0);
void GLDSA_ProgramUniform1fEXT(uint program, sint location, float32 v0);
void GLDSA_ProgramUniform2fEXT(uint program, sint location, float32 v0,
                               float32 v1);
void GLDSA_ProgramUniform2fvEXT(uint program, sint location, sint count,
                                const float32 *value);
void GLDSA_ProgramUniform3fEXT(uint program, sint location, float32 v0,
                               float32 v1, float32 v2);
void GLDSA_ProgramUniform4fEXT(uint program, sint location, float32 v0,
                               float32 v1, float32 v2, float32 v3);
void GLDSA_ProgramUniform1fvEXT(uint program, sint location, sint count,
                                const float32 *value);
void GLDSA_ProgramUniformMatrix4fvEXT(uint program, sint location,
                                      sint count, uchar8 transpose, const float32 *value);
void GLDSA_ProgramUniform3fEXT(uint program, sint location, sint count,
                               const float32 *value);
void GL_BindNullFramebuffers(void);
void GL_BindFramebuffer(uint target, uint framebuffer);
void GL_BindRenderbuffer(uint renderbuffer);
void GL_BindFragDataLocation(uint program, uint color, pointer name);

void GLDSA_NamedRenderbufferStorageEXT(uint renderbuffer,
                                       uint internalformat, sint width, sint height);
void GLDSA_NamedRenderbufferStorageMultisampleEXT(uint renderbuffer,
        sint samples, uint internalformat, sint width, sint height);
uint GLDSA_CheckNamedFramebufferStatusEXT(uint framebuffer, uint target);
void GLDSA_NamedFramebufferTexture2DEXT(uint framebuffer, uint attachment,
                                        uint textarget, uint texture, sint level);
void GLDSA_NamedFramebufferRenderbufferEXT(uint framebuffer,
        uint attachment, uint renderbuffertarget, uint renderbuffer);
void GLDSA_CopyTexImage2DEXT(uint framebuffer, uint target, sint level,
                             uint internalformat, sint x, sint y, sint width, sint height, sint border);

void GL_PatchParameteri(uint pname, sint value);

#endif
