////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2010 James Canete (use.less01@gmail.com)
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
// File name:   r_fbo.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_FBO_H__
#define __R_FBO_H__

struct image_s;
struct shaderProgram_s;

typedef struct FBO_s {
    valueType            name[MAX_QPATH];

    sint             index;

    uint        frameBuffer;

    uint        colorBuffers[16];
    sint             colorFormat;
    struct image_s  *colorImage[16];

    uint        depthBuffer;
    sint             depthFormat;

    uint        stencilBuffer;
    sint             stencilFormat;

    uint        packedDepthStencilBuffer;
    sint             packedDepthStencilFormat;

    sint             width;
    sint             height;
} FBO_t;

void FBO_AttachImage(FBO_t *fbo, image_t *image, GLenum attachment,
                     GLuint cubemapside);
void FBO_Bind(FBO_t *fbo);

void FBO_BlitFromTexture(struct image_s *src, vec4_t inSrcTexCorners,
                         vec2_t inSrcTexScale, FBO_t *dst, ivec4_t inDstBox,
                         struct shaderProgram_s *shaderProgram, vec4_t inColor, sint blend);
void FBO_Blit(FBO_t *src, ivec4_t srcBox, vec2_t srcTexScale, FBO_t *dst,
              ivec4_t dstBox, struct shaderProgram_s *shaderProgram, vec4_t color,
              sint blend);
void FBO_FastBlit(FBO_t *src, ivec4_t srcBox, FBO_t *dst, ivec4_t dstBox,
                  sint buffers, sint filter);
void FBO_FastBlitIndexed(FBO_t *src, FBO_t *dst, sint srcReadBuffer,
                         sint dstDrawBuffer, sint buffers, sint filter);

#endif //!__R_FBO_H__
