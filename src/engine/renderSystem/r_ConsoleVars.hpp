////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   r_ConsoleVars.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_CONSOLEVARS_HPP__
#define __R_CONSOLEVARS_HPP__

//
// cvars
//
extern convar_t *r_stencilbits;         // number of desired stencil bits
extern convar_t *r_depthbits;           // number of desired depth bits
extern convar_t
*r_colorbits;           // number of desired color bits, only relevant for fullscreen
extern convar_t *r_alphabits;           // number of desired alpha bits
extern convar_t *r_texturebits;         // number of desired texture bits
// 0 = use framebuffer depth
// 16 = use 16-bit textures
// 32 = use 32-bit textures
// all else = error

extern convar_t *r_customwidth;
extern convar_t *r_customheight;
extern convar_t *r_noborder;
extern convar_t *r_fullscreen;
extern convar_t
*r_ignorehwgamma;       // overrides hardware gamma capabilities
extern convar_t *r_drawBuffer;
extern convar_t *r_swapInterval;
extern convar_t
*r_allowExtensions;             // global enable/disable of OpenGL extensions
extern convar_t
*r_ext_compressed_textures;     // these control use of specific extensions
extern convar_t *r_ext_multitexture;
extern convar_t *r_ext_compiled_vertex_array;
extern convar_t *r_ext_texture_env_add;

extern convar_t *r_ext_texture_filter_anisotropic;
extern convar_t *r_ext_max_anisotropy;

extern convar_t *r_stereoEnabled;

extern  convar_t   *r_saveFontData;

//
// cvars
//
extern convar_t  *r_mode;
extern convar_t    *r_flareSize;
extern convar_t    *r_flareFade;
// coefficient for the flare intensity falloff function.
#define FLARE_STDCOEFF "150"
extern convar_t    *r_flareCoeff;

extern convar_t    *r_railWidth;
extern convar_t    *r_railCoreWidth;
extern convar_t    *r_railSegmentLength;

extern convar_t    *r_znear;                // near Z clip plane
extern convar_t
*r_zproj;                // z distance of projection plane
extern convar_t
*r_stereoSeparation;         // separation of cameras for stereo rendering

extern convar_t
*r_measureOverdraw;      // enables stencil buffer overdraw measurement

extern convar_t    *r_lodbias;              // push/pull LOD transitions
extern convar_t    *r_lodscale;

extern convar_t
*r_inGameVideo;              // controls whether in game video should be draw
extern convar_t
*r_fastsky;              // controls whether sky should be cleared or drawn
extern convar_t    *r_drawSun;              // controls drawing of sun quad
extern convar_t    *r_dynamiclight;     // dynamic lights enabled/disabled
extern convar_t
*r_dlightBacks;          // dlight non-facing surfaces for continuity

extern  convar_t   *r_norefresh;            // bypasses the ref rendering
extern  convar_t   *r_drawentities;     // disable/enable entity rendering
extern  convar_t
*r_drawworld;            // disable/enable world rendering
extern  convar_t
*r_drawfoliage;             // disable/enable foliage rendering
extern  convar_t
*r_speeds;               // various levels of information display
extern  convar_t
*r_detailTextures;       // enables/disables detail texturing stages
extern  convar_t   *r_novis;                // disable/enable usage of PVS
extern  convar_t   *r_nocull;
extern  convar_t
*r_facePlaneCull;        // enables culling of planar surfaces with back side test
extern  convar_t   *r_nocurves;
extern  convar_t   *r_showcluster;

extern convar_t    *r_gamma;

extern  convar_t  *r_ext_framebuffer_object;
extern  convar_t  *r_ext_texture_float;
extern  convar_t  *r_ext_framebuffer_multisample;
extern  convar_t  *r_arb_seamless_cube_map;
extern  convar_t  *r_arb_vertex_array_object;
extern  convar_t  *r_ext_direct_state_access;

extern  convar_t
*r_singleShader;             // make most world faces use default shader
extern  convar_t   *r_roundImagesDown;
extern  convar_t
*r_colorMipLevels;               // development aid to see texture mip usage
extern  convar_t
*r_picmip;                       // controls picmip values
extern convar_t *r_defaultImage;
extern  convar_t   *r_finish;
extern  convar_t   *r_textureMode;
extern  convar_t   *r_offsetFactor;
extern  convar_t   *r_offsetUnits;

extern  convar_t   *r_fullbright;                   // avoid lightmap pass
extern  convar_t   *r_lightmap;                 // render lightmaps only
extern  convar_t
*r_vertexLight;                  // vertex lighting mode for better performance
extern  convar_t   *r_uiFullScreen;             // ui is running fullscreen

extern convar_t *r_verbose;
extern  convar_t
*r_logFile;                      // number of frames to emit GL logs
extern  convar_t
*r_showtris;                 // enables wireframe rendering of the world
extern  convar_t
*r_showsky;                      // forces sky in front of all surfaces
extern  convar_t
*r_shownormals;                  // draws wireframe normals
extern  convar_t
*r_clear;                        // force screen clear every frame

extern  convar_t
*r_shadows;                      // controls shadows: 0 = none, 1 = blur, 2 = stencil, 3 = black planar projection
extern  convar_t   *r_flares;                       // light flares

extern  convar_t   *r_intensity;

extern  convar_t   *r_lockpvs;
extern  convar_t   *r_noportals;
extern  convar_t   *r_portalOnly;

extern  convar_t   *r_subdivisions;
extern  convar_t   *r_lodCurveError;
extern  convar_t   *r_skipBackEnd;

extern  convar_t   *r_anaglyphMode;
extern  convar_t  *r_hdr;
extern  convar_t *r_truehdr;
extern  convar_t  *r_postProcess;

extern  convar_t  *r_toneMap;
extern  convar_t  *r_forceToneMap;
extern  convar_t  *r_forceToneMapMin;
extern  convar_t  *r_forceToneMapAvg;
extern  convar_t  *r_forceToneMapMax;

extern  convar_t  *r_autoExposure;
extern  convar_t  *r_forceAutoExposure;
extern  convar_t  *r_forceAutoExposureMin;
extern  convar_t  *r_forceAutoExposureMax;

extern  convar_t  *r_cameraExposure;

extern  convar_t  *r_depthPrepass;
extern  convar_t  *r_ssao;

extern  convar_t  *r_normalMapping;
extern  convar_t  *r_specularMapping;
extern  convar_t  *r_deluxeMapping;
extern  convar_t  *r_parallaxMapping;
extern  convar_t  *r_parallaxMapOffset;
extern  convar_t  *r_parallaxMapShadows;
extern  convar_t  *r_cubeMapping;
extern  convar_t  *r_horizonFade;
extern  convar_t  *r_cubemapSize;
extern  convar_t  *r_deluxeSpecular;
extern  convar_t  *r_pbr;
extern  convar_t  *r_baseNormalX;
extern  convar_t  *r_baseNormalY;
extern  convar_t  *r_baseParallax;
extern  convar_t  *r_baseSpecular;
extern  convar_t  *r_baseGloss;
extern  convar_t  *r_dlightMode;
extern  convar_t  *r_pshadowDist;
extern  convar_t  *r_mergeLightmaps;
extern  convar_t  *r_imageUpsample;
extern  convar_t  *r_imageUpsampleMaxSize;
extern  convar_t  *r_imageUpsampleType;
extern  convar_t  *r_genNormalMaps;
extern  convar_t  *r_forceSun;
extern  convar_t  *r_forceSunLightScale;
extern  convar_t  *r_forceSunAmbientScale;
extern  convar_t  *r_sunlightMode;
extern  convar_t  *r_drawSunRays;
extern  convar_t  *r_sunShadows;
extern  convar_t  *r_shadowFilter;
extern  convar_t  *r_shadowBlur;
extern  convar_t  *r_shadowMapSize;
extern  convar_t  *r_shadowCascadeZNear;
extern  convar_t  *r_shadowCascadeZFar;
extern  convar_t  *r_shadowCascadeZBias;
extern  convar_t  *r_ignoreDstAlpha;

extern  convar_t   *r_greyscale;

extern  convar_t   *r_ignoreGLErrors;

extern  convar_t   *r_overBrightBits;
extern  convar_t   *r_mapOverBrightBits;

extern  convar_t   *r_debugSurface;
extern  convar_t   *r_simpleMipMaps;

extern  convar_t   *r_showImages;
extern  convar_t   *r_debugSort;

extern  convar_t   *r_printShaders;

extern convar_t    *r_marksOnTriangleMeshes;
extern convar_t *r_maxpolys;
extern convar_t *r_maxpolyverts;

extern convar_t *r_screenshotJpegQuality;
extern convar_t *r_aviMotionJpegQuality;

extern convar_t *r_lensflare;
extern convar_t *r_anamorphic;
extern convar_t *r_anamorphicDarkenPower;
extern convar_t *r_ssgi;
extern convar_t *r_ssgiWidth;
extern convar_t *r_ssgiSamples;
extern convar_t *r_darkexpand;
extern convar_t *r_dof;
extern convar_t *r_esharpening;
extern convar_t *r_esharpening2;
extern convar_t *r_multipost;
extern convar_t *r_textureClean;
extern convar_t *r_textureCleanSigma;
extern convar_t *r_textureCleanBSigma;
extern convar_t *r_textureCleanMSize;
extern convar_t *r_trueAnaglyph;
extern convar_t *r_trueAnaglyphSeparation;
extern convar_t *r_trueAnaglyphRed;
extern convar_t *r_trueAnaglyphGreen;
extern convar_t *r_trueAnaglyphBlue;
extern convar_t *r_vibrancy;
extern convar_t *r_texturedetail;
extern convar_t *r_texturedetailStrength;
extern convar_t *r_rbm;
extern convar_t *r_rbmStrength;
extern convar_t *r_screenblur;
extern convar_t *r_brightness;
extern convar_t *r_contrast;
extern convar_t *r_gamma;
extern convar_t *r_fxaa;
extern convar_t *r_bloom;
extern convar_t *r_bloomPasses;
extern convar_t *r_bloomDarkenPower;
extern convar_t *r_bloomScale;
extern convar_t *r_ssr;
extern convar_t *r_ssrStrength;
extern convar_t *r_sse;
extern convar_t *r_sseStrength;
extern convar_t *r_ambientScale;
extern convar_t *r_directedScale;
extern convar_t *r_debugLight;
extern convar_t *r_allowResize;
extern convar_t *r_centerWindow;
extern convar_t *r_displayRefresh;
extern convar_t *com_abnormalExit;
extern convar_t *com_minimized;
extern convar_t *in_nograb;

#endif //!__R_CONSOLEVARS_HPP__
