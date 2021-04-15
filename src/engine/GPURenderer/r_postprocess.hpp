////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 Andrei Drexler, Richard Allen, James Canete
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
// File name:   r_postprocess.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_POSTPROCESS_H__
#define __R_POSTPROCESS_H__

void RB_ToneMap(FBO_t *hdrFbo, ivec4_t hdrBox, FBO_t *ldrFbo,
                ivec4_t ldrBox, sint autoExposure);
void RB_BokehBlur(FBO_t *src, ivec4_t srcBox, FBO_t *dst, ivec4_t dstBox,
                  float32 blur);
void RB_SunRays(FBO_t *srcFbo, ivec4_t srcBox, FBO_t *dstFbo,
                ivec4_t dstBox);
void RB_GaussianBlur(FBO_t *srcFbo, FBO_t *intermediateFbo, FBO_t *dstFbo,
                     float32 spread);
void RB_HBlur(FBO_t *srcFbo, FBO_t *dstFbo, float32 strength);
void RB_VBlur(FBO_t *srcFbo, FBO_t *dstFbo, float32 strength);
void RB_TextureDetail(FBO_t *hdrFbo, ivec4_t hdrBox, FBO_t *ldrFbo,
                      ivec4_t ldrBox);
void RB_RBM(FBO_t *hdrFbo, ivec4_t hdrBox, FBO_t *ldrFbo, ivec4_t ldrBox);
void RB_Contrast(FBO_t *src, ivec4_t srcBox, FBO_t *dst, ivec4_t dstBox);
void RB_DarkExpand(FBO_t *hdrFbo, ivec4_t hdrBox, FBO_t *ldrFbo,
                   ivec4_t ldrBox);
void RB_Anamorphic(FBO_t *hdrFbo, ivec4_t hdrBox, FBO_t *ldrFbo,
                   ivec4_t ldrBox);
void RB_LensFlare(FBO_t *hdrFbo, ivec4_t hdrBox, FBO_t *ldrFbo,
                  ivec4_t ldrBox);
void RB_HDR(FBO_t *hdrFbo, ivec4_t hdrBox, FBO_t *ldrFbo, ivec4_t ldrBox);
void RB_Anaglyph(FBO_t *hdrFbo, ivec4_t hdrBox, FBO_t *ldrFbo,
                 ivec4_t ldrBox);
void RB_FXAA(FBO_t *hdrFbo, ivec4_t hdrBox, FBO_t *ldrFbo, ivec4_t ldrBox);
void RB_ESharpening(FBO_t *hdrFbo, ivec4_t hdrBox, FBO_t *ldrFbo,
                    ivec4_t ldrBox);
void RB_ESharpening2(FBO_t *hdrFbo, ivec4_t hdrBox, FBO_t *ldrFbo,
                     ivec4_t ldrBox);
void RB_TextureClean(FBO_t *hdrFbo, ivec4_t hdrBox, FBO_t *ldrFbo,
                     ivec4_t ldrBox);
void RB_DOF(FBO_t *hdrFbo, ivec4_t hdrBox, FBO_t *ldrFbo, ivec4_t ldrBox);
void RB_MultiPost(FBO_t *hdrFbo, ivec4_t hdrBox, FBO_t *ldrFbo,
                  ivec4_t ldrBox);
void RB_Vibrancy(FBO_t *hdrFbo, ivec4_t hdrBox, FBO_t *ldrFbo,
                 ivec4_t ldrBox);
void RB_Bloom(FBO_t *hdrFbo, ivec4_t hdrBox, FBO_t *ldrFbo,
              ivec4_t ldrBox);
void RB_SSGI(FBO_t *hdrFbo, ivec4_t hdrBox, FBO_t *ldrFbo, ivec4_t ldrBox);
void RB_ScreenSpaceReflections(FBO_t *hdrFbo, ivec4_t hdrBox,
                               FBO_t *ldrFbo, ivec4_t ldrBox);
void RB_Underwater(FBO_t *hdrFbo, ivec4_t hdrBox, FBO_t *ldrFbo,
                   ivec4_t ldrBox);

#endif //!__R_POSTPROCESS_H__
