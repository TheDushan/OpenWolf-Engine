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
// File name:   r_dsa.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.hpp>

static struct {
    uint textures[NUM_TEXTURE_BUNDLES];
    uint texunit;

    uint program;

    uint drawFramebuffer;
    uint readFramebuffer;
    uint renderbuffer;
}
glDsaState;

void GL_BindNullTextures() {
    sint i;

    if(glRefConfig.directStateAccess) {
        for(i = 0; i < NUM_TEXTURE_BUNDLES; i++) {
            qglBindMultiTextureEXT(GL_TEXTURE0 + i, GL_TEXTURE_2D, 0);
            glDsaState.textures[i] = 0;
        }
    } else {
        for(i = 0; i < NUM_TEXTURE_BUNDLES; i++) {
            qglActiveTexture(GL_TEXTURE0 + i);
            qglBindTexture(GL_TEXTURE_2D, 0);
            glDsaState.textures[i] = 0;
        }

        qglActiveTexture(GL_TEXTURE0);
        glDsaState.texunit = GL_TEXTURE0;
    }
}

sint GL_BindMultiTexture(uint texunit, uint target, uint texture) {
    uint tmu = texunit - GL_TEXTURE0;

    if(glDsaState.textures[tmu] == texture) {
        return 0;
    }

    if(target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
            target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z) {
        target = GL_TEXTURE_CUBE_MAP;
    }

    qglBindMultiTextureEXT(texunit, target, texture);
    glDsaState.textures[tmu] = texture;
    return 1;
}

void GLDSA_BindMultiTextureEXT(uint texunit, uint target, uint texture) {
    if(glDsaState.texunit != texunit) {
        qglActiveTexture(texunit);
        glDsaState.texunit = texunit;
    }

    qglBindTexture(target, texture);
}

void GLDSA_TextureParameterfEXT(uint texture, uint target, uint pname,
                                float32 param) {
    GL_BindMultiTexture(glDsaState.texunit, target, texture);
    qglTexParameterf(target, pname, param);
}

void GLDSA_TextureParameteriEXT(uint texture, uint target, uint pname,
                                sint param) {
    GL_BindMultiTexture(glDsaState.texunit, target, texture);
    qglTexParameteri(target, pname, param);
}

void GLDSA_TextureImage2DEXT(uint texture, uint target, sint level,
                             sint internalformat, sint width, sint height, sint border, uint format,
                             uint type, const void *pixels) {
    GL_BindMultiTexture(glDsaState.texunit, target, texture);
    qglTexImage2D(target, level, internalformat, width, height, border, format,
                  type, pixels);
}

void GLDSA_TextureSubImage2DEXT(uint texture, uint target, sint level,
                                sint xoffset, sint yoffset, sint width, sint height, uint format,
                                uint type, const void *pixels) {
    GL_BindMultiTexture(glDsaState.texunit, target, texture);
    qglTexSubImage2D(target, level, xoffset, yoffset, width, height, format,
                     type, pixels);
}

void GLDSA_CopyTextureSubImage2DEXT(uint texture, uint target, sint level,
                                    sint xoffset, sint yoffset, sint x, sint y, sint width, sint height) {
    GL_BindMultiTexture(glDsaState.texunit, target, texture);
    qglCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
}

void  GLDSA_CompressedTextureImage2DEXT(uint texture, uint target,
                                        sint level, uint internalformat, sint width, sint height, sint border,
                                        sint imageSize, const void *data) {
    GL_BindMultiTexture(glDsaState.texunit, target, texture);
    qglCompressedTexImage2D(target, level, internalformat, width, height,
                            border, imageSize, data);
}

void GLDSA_CompressedTextureSubImage2DEXT(uint texture, uint target,
        sint level, sint xoffset, sint yoffset, sint width, sint height,
        uint format, sint imageSize, const void *data) {
    GL_BindMultiTexture(glDsaState.texunit, target, texture);
    qglCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height,
                               format, imageSize, data);
}

void GLDSA_GenerateTextureMipmapEXT(uint texture, uint target) {
    GL_BindMultiTexture(glDsaState.texunit, target, texture);
    qglGenerateMipmap(target);
}

void GL_BindNullProgram() {
    qglUseProgram(0);
    glDsaState.program = 0;
}

sint GL_UseProgram(uint program) {
    if(glDsaState.program == program) {
        return 0;
    }

    qglUseProgram(program);
    glDsaState.program = program;
    return 1;
}

void GLDSA_ProgramUniform1iEXT(uint program, sint location, sint v0) {
    GL_UseProgram(program);
    qglUniform1i(location, v0);
}

void GLDSA_ProgramUniform1fEXT(uint program, sint location, float32 v0) {
    GL_UseProgram(program);
    qglUniform1f(location, v0);
}

void GLDSA_ProgramUniform2fEXT(uint program, sint location,
                               float32 v0, float32 v1) {
    GL_UseProgram(program);
    qglUniform2f(location, v0, v1);
}

void GLDSA_ProgramUniform2fvEXT(uint program, sint location, sint count,
                                const float32 *value) {
    GL_UseProgram(program);
    qglUniform2fv(program, location, count, value);
}

void GLDSA_ProgramUniform3fEXT(uint program, sint location,
                               float32 v0, float32 v1, float32 v2) {
    GL_UseProgram(program);
    qglUniform3f(location, v0, v1, v2);
}

void GLDSA_ProgramUniform4fEXT(uint program, sint location,
                               float32 v0, float32 v1, float32 v2, float32 v3) {
    GL_UseProgram(program);
    qglUniform4f(location, v0, v1, v2, v3);
}

void GLDSA_ProgramUniform1fvEXT(uint program, sint location, sint count,
                                const float32 *value) {
    GL_UseProgram(program);
    qglUniform1fv(location, count, value);
}

void GLDSA_ProgramUniform3fEXT(uint program, sint location, sint count,
                               const float32 *value) {
    GL_UseProgram(program);
    qglUniform3fv(location, count, value);
}

void GLDSA_ProgramUniformMatrix4fvEXT(uint program, sint location,
                                      sint count, uchar8 transpose, const float32 *value) {
    GL_UseProgram(program);
    qglUniformMatrix4fv(location, count, transpose, value);
}

void GL_BindNullFramebuffers() {
    qglBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDsaState.drawFramebuffer = glDsaState.readFramebuffer = 0;
    qglBindRenderbuffer(GL_RENDERBUFFER, 0);
    glDsaState.renderbuffer = 0;
}

void GL_BindFramebuffer(uint target, uint framebuffer) {
    switch(target) {
        case GL_FRAMEBUFFER:
            if(framebuffer != glDsaState.drawFramebuffer ||
                    framebuffer != glDsaState.readFramebuffer) {
                qglBindFramebuffer(target, framebuffer);
                glDsaState.drawFramebuffer = glDsaState.readFramebuffer = framebuffer;
            }

            break;

        case GL_DRAW_FRAMEBUFFER:
            if(framebuffer != glDsaState.drawFramebuffer) {
                qglBindFramebuffer(target, framebuffer);
                glDsaState.drawFramebuffer = framebuffer;
            }

            break;

        case GL_READ_FRAMEBUFFER:
            if(framebuffer != glDsaState.readFramebuffer) {
                qglBindFramebuffer(target, framebuffer);
                glDsaState.readFramebuffer = framebuffer;
            }

            break;
    }
}

void GL_BindRenderbuffer(uint renderbuffer) {
    if(renderbuffer != glDsaState.renderbuffer) {
        qglBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
        glDsaState.renderbuffer = renderbuffer;
    }
}

void GL_BindFragDataLocation(uint program, uint color, pointer name) {
    qglBindFragDataLocation(program, color, name);
}

void GLDSA_NamedRenderbufferStorageEXT(uint renderbuffer,
                                       uint internalformat, sint width, sint height) {
    GL_BindRenderbuffer(renderbuffer);
    qglRenderbufferStorage(GL_RENDERBUFFER, internalformat, width, height);
}

void GLDSA_NamedRenderbufferStorageMultisampleEXT(uint renderbuffer,
        sint samples, uint internalformat, sint width, sint height) {
    GL_BindRenderbuffer(renderbuffer);
    qglRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, internalformat,
                                      width, height);
}

uint GLDSA_CheckNamedFramebufferStatusEXT(uint framebuffer, uint target) {
    GL_BindFramebuffer(target, framebuffer);
    return qglCheckFramebufferStatus(target);
}

void GLDSA_NamedFramebufferTexture2DEXT(uint framebuffer, uint attachment,
                                        uint textarget, uint texture, sint level) {
    GL_BindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    qglFramebufferTexture2D(GL_FRAMEBUFFER, attachment, textarget, texture,
                            level);
}

void GLDSA_NamedFramebufferRenderbufferEXT(uint framebuffer,
        uint attachment, uint renderbuffertarget, uint renderbuffer) {
    GL_BindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    qglFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, renderbuffertarget,
                               renderbuffer);
}

void GL_PatchParameteri(uint pname, sint value) {
    qglPatchParameteri(pname, value);
}


