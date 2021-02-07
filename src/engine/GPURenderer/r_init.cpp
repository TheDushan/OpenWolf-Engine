////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2005 Id Software, Inc.
// Copyright(C) 2000 - 2013 Darklegion Development
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
// File name:   r_init.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: functions that are not called every frame
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

vidconfig_t glConfig;
glRefConfig_t glRefConfig;
bool textureFilterAnisotropic = false;
sint maxAnisotropy = 0;
float32 displayAspect = 0.0f;

glstate_t glState;

static void GfxInfo_f( void );
static void GfxMemInfo_f( void );

convar_t* r_glCoreProfile;
convar_t* r_glMajorVersion;
convar_t* r_glMinorVersion;
convar_t* r_glDebugProfile;
convar_t*	r_flareSize;
convar_t*	r_flareFade;
convar_t*	r_flareCoeff;

convar_t*	r_railWidth;
convar_t*	r_railCoreWidth;
convar_t*	r_railSegmentLength;

convar_t*	r_detailTextures;

convar_t*	r_znear;
convar_t*	r_zproj;
convar_t*	r_stereoSeparation;

convar_t*	r_skipBackEnd;

convar_t*	r_stereoEnabled;
convar_t*	r_anaglyphMode;

convar_t*	r_greyscale;

convar_t*	r_ignorehwgamma;
convar_t*	r_measureOverdraw;

convar_t*	r_inGameVideo;
convar_t*	r_fastsky;
convar_t*	r_drawSun;
convar_t*	r_dynamiclight;
convar_t*	r_dlightBacks;

convar_t*	r_lodbias;
convar_t*	r_lodscale;

convar_t*	r_norefresh;
convar_t*	r_drawentities;
convar_t*	r_drawworld;
convar_t*	r_speeds;
convar_t*	r_fullbright;
convar_t*	r_novis;
convar_t*	r_nocull;
convar_t*	r_facePlaneCull;
convar_t*	r_showcluster;
convar_t*	r_nocurves;

convar_t*	r_allowExtensions;

convar_t*	r_ext_compressed_textures;
convar_t*	r_ext_multitexture;
convar_t*	r_ext_compiled_vertex_array;
convar_t*	r_ext_texture_env_add;
convar_t*	r_ext_texture_filter_anisotropic;
convar_t*	r_ext_max_anisotropy;

convar_t*  r_ext_framebuffer_object;
convar_t*  r_ext_texture_float;
convar_t*  r_ext_framebuffer_multisample;
convar_t*  r_arb_seamless_cube_map;
convar_t*  r_arb_vertex_array_object;
convar_t*  r_ext_direct_state_access;

convar_t*  r_cameraExposure;

convar_t*  r_hdr;
convar_t* r_truehdr;
convar_t*  r_postProcess;

convar_t*  r_toneMap;
convar_t*  r_forceToneMap;
convar_t*  r_forceToneMapMin;
convar_t*  r_forceToneMapAvg;
convar_t*  r_forceToneMapMax;

convar_t*  r_autoExposure;
convar_t*  r_forceAutoExposure;
convar_t*  r_forceAutoExposureMin;
convar_t*  r_forceAutoExposureMax;

convar_t*  r_depthPrepass;
convar_t*  r_ssao;

convar_t*  r_normalMapping;
convar_t*  r_specularMapping;
convar_t*  r_deluxeMapping;
convar_t*  r_parallaxMapping;
convar_t*  r_parallaxMapShadows;
convar_t*  r_cubeMapping;
convar_t*  r_horizonFade;
convar_t*  r_cubemapSize;
convar_t*  r_deluxeSpecular;
convar_t*  r_pbr;
convar_t*  r_baseNormalX;
convar_t*  r_baseNormalY;
convar_t*  r_baseParallax;
convar_t*  r_baseSpecular;
convar_t*  r_baseGloss;
convar_t*  r_mergeLightmaps;
convar_t*  r_dlightMode;
convar_t*  r_pshadowDist;
convar_t*  r_imageUpsample;
convar_t*  r_imageUpsampleMaxSize;
convar_t*  r_imageUpsampleType;
convar_t*  r_genNormalMaps;
convar_t*  r_forceSun;
convar_t*  r_forceSunLightScale;
convar_t*  r_forceSunAmbientScale;
convar_t*  r_sunlightMode;
convar_t*  r_drawSunRays;
convar_t*  r_sunShadows;
convar_t*  r_shadowFilter;
convar_t*  r_shadowBlur;
convar_t*  r_shadowMapSize;
convar_t*  r_shadowCascadeZNear;
convar_t*  r_shadowCascadeZFar;
convar_t*  r_shadowCascadeZBias;
convar_t*  r_ignoreDstAlpha;

convar_t*	r_ignoreGLErrors;
convar_t*	r_logFile;

convar_t*	r_stencilbits;
convar_t*	r_depthbits;
convar_t*	r_colorbits;
convar_t*	r_alphabits;
convar_t*	r_texturebits;
convar_t*  r_ext_multisample;

convar_t*	r_drawBuffer;
convar_t*	r_lightmap;
convar_t*	r_vertexLight;
convar_t*	r_uiFullScreen;
convar_t*	r_shadows;
convar_t*	r_flares;
convar_t*	r_mode;
convar_t*	r_singleShader;
convar_t*	r_roundImagesDown;
convar_t*	r_colorMipLevels;
convar_t* r_defaultImage;
convar_t*	r_picmip;
convar_t*	r_showtris;
convar_t*	r_showsky;
convar_t*	r_shownormals;
convar_t*	r_finish;
convar_t*	r_clear;
convar_t*	r_swapInterval;
convar_t*	r_textureMode;
convar_t*	r_offsetFactor;
convar_t*	r_offsetUnits;
convar_t*	r_intensity;
convar_t*	r_lockpvs;
convar_t*	r_noportals;
convar_t*	r_portalOnly;

convar_t*	r_subdivisions;
convar_t*	r_lodCurveError;

convar_t*	r_fullscreen;
convar_t* r_noborder;

convar_t*	r_customwidth;
convar_t*	r_customheight;

convar_t*	r_overBrightBits;
convar_t*	r_mapOverBrightBits;

convar_t*	r_debugSurface;
convar_t*	r_simpleMipMaps;

convar_t*	r_showImages;

convar_t*	r_ambientScale;
convar_t*	r_directedScale;
convar_t*	r_debugLight;
convar_t*	r_debugSort;
convar_t*	r_printShaders;
convar_t*	r_saveFontData;

convar_t*	r_marksOnTriangleMeshes;

convar_t*	r_aviMotionJpegQuality;
convar_t*	r_screenshotJpegQuality;

convar_t*	r_lensflare;
convar_t*	r_anamorphic;
convar_t*	r_anamorphicDarkenPower;
convar_t* r_ssgi;
convar_t* r_ssgiWidth;
convar_t* r_ssgiSamples;
convar_t* r_ssr;
convar_t* r_ssrStrength;
convar_t* r_sse;
convar_t* r_sseStrength;
convar_t*	r_darkexpand;
convar_t* r_dof;
convar_t* r_esharpening;
convar_t* r_esharpening2;
convar_t* r_multipost;
convar_t* r_textureClean;
convar_t* r_textureCleanSigma;
convar_t* r_textureCleanBSigma;
convar_t* r_textureCleanMSize;
convar_t* r_trueAnaglyph;
convar_t* r_trueAnaglyphSeparation;
convar_t* r_trueAnaglyphRed;
convar_t* r_trueAnaglyphGreen;
convar_t* r_trueAnaglyphBlue;
convar_t* r_vibrancy;
convar_t* r_texturedetail;
convar_t* r_texturedetailStrength;
convar_t* r_rbm;
convar_t* r_rbmStrength;
convar_t* r_screenblur;
convar_t* r_brightness;
convar_t* r_contrast;
convar_t* r_gamma;
convar_t* r_bloom;
convar_t* r_bloomPasses;
convar_t* r_bloomDarkenPower;
convar_t* r_bloomScale;
convar_t* r_fxaa;

convar_t*	r_maxpolys;
sint		max_polys;
convar_t*	r_maxpolyverts;
sint		max_polyverts;

/*
** InitOpenGL
**
** This function is responsible for initializing a valid OpenGL subsystem.  This
** is done by calling GLimp_Init (which gives us a working OGL subsystem) then
** setting variables, checking GL constants, and reporting the gfx system config
** to the user.
*/
static void InitOpenGL( void )
{
    //
    // initialize OS specific portions of the renderer
    //
    // GLimp_Init directly or indirectly references the following cvars:
    //		- r_fullscreen
    //		- r_mode
    //		- r_(color|depth|stencil)bits
    //		- r_ignorehwgamma
    //		- r_gamma
    //
    
    if( glConfig.vidWidth == 0 )
    {
        sint		temp;
        
        memset( &glConfig, 0, sizeof( glConfig ) );
        
        GLimp_Init( false );
        GLimp_InitExtraExtensions();
        
        glConfig.textureEnvAddAvailable = true;
        
        // OpenGL driver constants
        qglGetIntegerv( GL_MAX_TEXTURE_SIZE, &temp );
        glConfig.maxTextureSize = temp;
        
        // stubbed or broken drivers may have reported 0...
        if( glConfig.maxTextureSize <= 0 )
        {
            glConfig.maxTextureSize = 0;
        }
        
        qglGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS, &temp );
        glConfig.numTextureUnits = temp;
        
        // reserve 160 components for other uniforms
        qglGetIntegerv( GL_MAX_VERTEX_UNIFORM_COMPONENTS, &temp );
        glRefConfig.glslMaxAnimatedBones = Com_Clamp( 0, IQM_MAX_JOINTS, ( temp - 160 ) / 16 );
        if( glRefConfig.glslMaxAnimatedBones < 12 )
        {
            glRefConfig.glslMaxAnimatedBones = 0;
        }
        
        glConfig.smpActive = false;
        
        CL_RefPrintf( PRINT_DEVELOPER, "Trying SMP acceleration...\n" );
        
        if( GLimp_SpawnRenderThread( RB_RenderThread ) )
        {
            CL_RefPrintf( PRINT_DEVELOPER, "...succeeded.\n" );
            glConfig.smpActive = true;
        }
        else
        {
            CL_RefPrintf( PRINT_DEVELOPER, "...failed.\n" );
        }
    }
    
    // check for GLSL function textureCubeLod()
    if( r_cubeMapping->integer && !QGL_VERSION_ATLEAST( 3, 0 ) )
    {
        CL_RefPrintf( PRINT_WARNING, "WARNING: Disabled r_cubeMapping because it requires OpenGL 3.0\n" );
        cvarSystem->Set( "r_cubeMapping", "0" );
    }
    
    // set default state
    GL_SetDefaultState();
}

/*
==================
GL_CheckErrors
==================
*/
void GL_CheckErrs( pointer file, sint line )
{
    sint		err;
    valueType	s[64];
    
    err = qglGetError();
    if( err == GL_NO_ERROR )
    {
        return;
    }
    if( r_ignoreGLErrors->integer )
    {
        return;
    }
    switch( err )
    {
        case GL_INVALID_ENUM:
            Q_strcpy_s( s, "GL_INVALID_ENUM" );
            break;
        case GL_INVALID_VALUE:
            Q_strcpy_s( s, "GL_INVALID_VALUE" );
            break;
        case GL_INVALID_OPERATION:
            Q_strcpy_s( s, "GL_INVALID_OPERATION" );
            break;
        case GL_STACK_OVERFLOW:
            Q_strcpy_s( s, "GL_STACK_OVERFLOW" );
            break;
        case GL_STACK_UNDERFLOW:
            Q_strcpy_s( s, "GL_STACK_UNDERFLOW" );
            break;
        case GL_OUT_OF_MEMORY:
            Q_strcpy_s( s, "GL_OUT_OF_MEMORY" );
            break;
        default:
            Q_vsprintf_s( s, sizeof( s ), sizeof( s ), "%i", err );
            break;
    }
    
    Com_Error( ERR_FATAL, "GL_CheckErrors: %s in %s at line %d", s, file, line );
}

/*
** R_GetModeInfo
*/
typedef struct vidmode_s
{
    pointer description;
    sint        width, height;
    float32      pixelAspect; // pixel width / height
} vidmode_t;

//Dushan
static const vidmode_t r_vidModes[] =
{
    { "Mode  0:  320x240  (4:3)",   320,  240, 1 },
    { "Mode  1:  320x200  (16:10)", 320,  200, 1 },
    { "Mode  2:  640x480  (4:3)",   640,  480, 1 },
    { "Mode  3:  640x400  (16:10)", 640,  400, 1 },
    { "Mode  4:  720x480  (3:2)",   720,  480, 1 },
    { "Mode  5:  800x480  (5:3)",   800,  480, 1 },
    { "Mode  6:  800x600  (4:3)",   800,  600, 1 },
    { "Mode  7:  852x480  (16:9)",  853,  480, 1 },
    { "Mode  8: 1024x768  (4:3)",   1024, 768, 1 },
    { "Mode  9: 1152x768  (3:2)",   1152, 758, 1 },
    { "Mode 10: 1152x864  (4:3)",   1152, 864, 1 },
    { "Mode 11: 1280x720  (16:9)",  1280, 720, 1 },
    { "Mode 12: 1280x768  (5:4)",   1280, 768, 1 },
    { "Mode 13: 1280x800  (16:10)", 1280, 800, 1 },
    { "Mode 14: 1280x854  (3:2)",   1280, 854, 1 },
    { "Mode 15: 1280x1024 (5:4) ",  1280, 1024, 1 },
    { "Mode 16: 1366x768  (16:9)",  1366, 768, 1 },
    { "Mode 17: 1440x900  (16:10)", 1440, 900, 1 },
    { "Mode 18: 1440x960  (3:2)",   1440, 960, 1 },
    { "Mode 19: 1600x900  (16:9)",  1600, 900, 1 },
    { "Mode 20: 1680x1050 (16:10)", 1680, 1050, 1 },
    { "Mode 21: 1600x1200 (4:3)",   1600, 1200, 1 },
    { "Mode 22: 1920x1080 (16:9)",  1920, 1080, 1 },
    { "Mode 23: 1920x1200 (16:10)", 1920, 1200, 1 },
    { "Mode 24: 2048x1080 (17:9)",  2048, 1080, 1 },
    { "Mode 25: 2048x1536 (4:3)",   2048, 1536, 1 },
    { "Mode 26: 2560x1080 (21:9)",  2560, 1080, 1 },
    { "Mode 27: 2560x1600 (16:10)", 2560, 1600, 1 },
    { "Mode 28: 2560x2048 (5:4)",   2560, 2048, 1 },
    { "Mode 29: 2880x1920 (3:2)",   2880, 1920, 1 },
    { "Mode 30: 3440x1440 (21:9)",  3440, 1440, 1 },
    { "Mode 31: 3840x2400 (16:10)", 3840, 2400, 1 },
    { "Mode 32: 5120x4096 (5:4)",   5120, 4096, 1 },
    { "Mode 33: 7680x4800 (16:10)", 7680, 4800, 1 },
};
static const sint s_numVidModes = ARRAY_LEN( r_vidModes );

bool R_GetModeInfo( sint* width, sint* height, float32* windowAspect, sint mode )
{
    const vidmode_t* vm;
    
    if( mode < -2 )
    {
        return false;
    }
    
    if( mode >= s_numVidModes )
    {
        return false;
    }
    
    if( mode == -2 )
    {
        // Must set width and height to display size before calling this function!
        *windowAspect = ( float32 ) * width / *height;
    }
    else if( mode == -1 )
    {
        *width = r_customwidth->integer;
        *height = r_customheight->integer;
    }
    else
    {
        vm = &r_vidModes[ mode ];
        
        *width = vm->width;
        *height = vm->height;
        *windowAspect = ( float32 ) vm->width / ( vm->height * vm->pixelAspect );
    }
    
    return true;
}

/*
** R_ModeList_f
*/
static void R_ModeList_f( void )
{
    sint i;
    
    CL_RefPrintf( PRINT_ALL, "\n" );
    
    for( i = 0; i < s_numVidModes; i++ )
    {
        CL_RefPrintf( PRINT_ALL, "Mode %-2d: %s\n", i, r_vidModes[ i ].description );
    }
    
    CL_RefPrintf( PRINT_ALL, "\n" );
}


/*
==============================================================================

						SCREEN SHOTS

NOTE TTimo
some thoughts about the screenshots system:
screenshots get written in fs_homepath + fs_gamedir
vanilla q3 .. baseq3/screenshots/ *.tga
team arena .. missionpack/screenshots/ *.tga

two commands: "screenshot" and "screenshotJPEG"
we use statics to store a count and start writing the first screenshot/screenshot????.tga (.jpg) available
(with fileSystem->FileExists / fileSystem->FOpenFileWrite calls)
FIXME: the statics don't get a reinit between fs_game changes

==============================================================================
*/

/*
==================
RB_ReadPixels

Reads an image but takes care of alignment issues for reading RGB images.

Reads a minimum offset for where the RGB data starts in the image from
integer stored at pointer offset. When the function has returned the actual
offset was written back to address offset. This address will always have an
alignment of packAlign to ensure efficient copying.

Stores the length of padding after a line of pixels to address padlen

Return value must be freed with Hunk_FreeTempMemory()
==================
*/

uchar8* RB_ReadPixels( sint x, sint y, sint width, sint height, uint32* offset, sint* padlen )
{
    uchar8* buffer = nullptr, *bufstart;
    sint padwidth, linelen;
    sint packAlign;
    
    qglGetIntegerv( GL_PACK_ALIGNMENT, &packAlign );
    
    linelen = width * 3;
    padwidth = PAD( linelen, packAlign );
    
    // Allocate a few more bytes so that we can choose an alignment we like
    buffer = ( uchar8* )Hunk_AllocateTempMemory( padwidth * height + *offset + packAlign - 1 );
    
    bufstart = ( uchar8* )PADP( ( intptr_t ) buffer + *offset, packAlign );
    
    qglReadPixels( x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, bufstart );
    
    *offset = bufstart - buffer;
    *padlen = padwidth - linelen;
    
    return buffer;
}

/*
==================
RB_TakeScreenshot
==================
*/
void RB_TakeScreenshot( sint x, sint y, sint width, sint height, valueType* fileName )
{
    uchar8* allbuf, *buffer;
    uchar8* srcptr, *destptr;
    uchar8* endline, *endmem;
    uchar8 temp;
    
    sint linelen, padlen;
    uint32 offset = 18, memcount;
    
    allbuf = RB_ReadPixels( x, y, width, height, &offset, &padlen );
    buffer = allbuf + offset - 18;
    
    ::memset( buffer, 0, 18 );
    buffer[2] = 2;		// uncompressed type
    buffer[12] = width & 255;
    buffer[13] = width >> 8;
    buffer[14] = height & 255;
    buffer[15] = height >> 8;
    buffer[16] = 24;	// pixel size
    
    // swap rgb to bgr and remove padding from line endings
    linelen = width * 3;
    
    srcptr = destptr = allbuf + offset;
    endmem = srcptr + ( linelen + padlen ) * height;
    
    while( srcptr < endmem )
    {
        endline = srcptr + linelen;
        
        while( srcptr < endline )
        {
            temp = srcptr[0];
            *destptr++ = srcptr[2];
            *destptr++ = srcptr[1];
            *destptr++ = temp;
            
            srcptr += 3;
        }
        
        // Skip the pad
        srcptr += padlen;
    }
    
    memcount = linelen * height;
    
    fileSystem->WriteFile( fileName, buffer, memcount + 18 );
    
    Hunk_FreeTempMemory( allbuf );
}

/*
==================
RB_TakeScreenshotJPEG
==================
*/

void RB_TakeScreenshotJPEG( sint x, sint y, sint width, sint height, valueType* fileName )
{
    uchar8* buffer;
    uint32 offset = 0, memcount;
    sint padlen;
    
    buffer = RB_ReadPixels( x, y, width, height, &offset, &padlen );
    memcount = ( width * 3 + padlen ) * height;
    
    RE_SaveJPG( fileName, r_screenshotJpegQuality->integer, width, height, buffer + offset, padlen );
    Hunk_FreeTempMemory( buffer );
}

/*
==================
RB_TakeScreenshotCmd
==================
*/
const void* RB_TakeScreenshotCmd( const void* data )
{
    const screenshotCommand_t*	cmd;
    
    cmd = ( const screenshotCommand_t* )data;
    
    // finish any 2D drawing if needed
    if( tess.numIndexes )
        RB_EndSurface();
        
    if( cmd->jpeg )
        RB_TakeScreenshotJPEG( cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName );
    else
        RB_TakeScreenshot( cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName );
        
    return ( const void* )( cmd + 1 );
}

/*
==================
R_TakeScreenshot
==================
*/
void R_TakeScreenshot( sint x, sint y, sint width, sint height, valueType* name, bool jpeg )
{
    static valueType	fileName[MAX_OSPATH]; // bad things if two screenshots per frame?
    screenshotCommand_t*	cmd;
    
    cmd = ( screenshotCommand_t* )R_GetCommandBuffer( sizeof( *cmd ) );
    if( !cmd )
    {
        return;
    }
    cmd->commandId = RC_SCREENSHOT;
    
    cmd->x = x;
    cmd->y = y;
    cmd->width = width;
    cmd->height = height;
    Q_strncpyz( fileName, name, sizeof( fileName ) );
    cmd->fileName = fileName;
    cmd->jpeg = jpeg;
}

/*
==================
R_ScreenshotFilename
==================
*/
void R_ScreenshotFilename( sint lastNumber, valueType* fileName )
{
    sint		a, b, c, d;
    
    if( lastNumber < 0 || lastNumber > 9999 )
    {
        Q_vsprintf_s( fileName, MAX_OSPATH, MAX_OSPATH, "screenshots/shot9999.tga" );
        return;
    }
    
    a = lastNumber / 1000;
    lastNumber -= a * 1000;
    b = lastNumber / 100;
    lastNumber -= b * 100;
    c = lastNumber / 10;
    lastNumber -= c * 10;
    d = lastNumber;
    
    Q_vsprintf_s( fileName, MAX_OSPATH, MAX_OSPATH, "screenshots/shot%i%i%i%i.tga" , a, b, c, d );
}

/*
==================
R_ScreenshotFilename
==================
*/
void R_ScreenshotFilenameJPEG( sint lastNumber, valueType* fileName )
{
    sint		a, b, c, d;
    
    if( lastNumber < 0 || lastNumber > 9999 )
    {
        Q_vsprintf_s( fileName, MAX_OSPATH, MAX_OSPATH, "screenshots/shot9999.jpg" );
        return;
    }
    
    a = lastNumber / 1000;
    lastNumber -= a * 1000;
    b = lastNumber / 100;
    lastNumber -= b * 100;
    c = lastNumber / 10;
    lastNumber -= c * 10;
    d = lastNumber;
    
    Q_vsprintf_s( fileName, MAX_OSPATH, MAX_OSPATH, "screenshots/shot%i%i%i%i.jpg" , a, b, c, d );
}

/*
====================
R_LevelShot

levelshots are specialized 128*128 thumbnails for
the menu system, sampled down from full screen distorted images
====================
*/
void R_LevelShot( void )
{
    valueType		checkname[MAX_OSPATH];
    uchar8*		buffer = nullptr;
    uchar8*		source, *allsource;
    uchar8*		src, *dst;
    uint32			offset = 0;
    sint			padlen;
    sint			x, y;
    sint			r, g, b;
    float32		xScale, yScale;
    sint			xx, yy;
    
    Q_vsprintf_s( checkname, sizeof( checkname ), sizeof( checkname ), "levelshots/%s.tga", tr.world->baseName );
    
    allsource = RB_ReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, &offset, &padlen );
    source = allsource + offset;
    
    buffer = ( uchar8* )Hunk_AllocateTempMemory( 128 * 128 * 3 + 18 );
    ::memset( buffer, 0, 18 );
    buffer[2] = 2;		// uncompressed type
    buffer[12] = 128;
    buffer[14] = 128;
    buffer[16] = 24;	// pixel size
    
    // resample from source
    xScale = glConfig.vidWidth / 512.0f;
    yScale = glConfig.vidHeight / 384.0f;
    for( y = 0 ; y < 128 ; y++ )
    {
        for( x = 0 ; x < 128 ; x++ )
        {
            r = g = b = 0;
            for( yy = 0 ; yy < 3 ; yy++ )
            {
                for( xx = 0 ; xx < 4 ; xx++ )
                {
                    src = source + ( 3 * glConfig.vidWidth + padlen ) * ( sint )( ( y * 3 + yy ) * yScale ) +
                          3 * ( sint )( ( x * 4 + xx ) * xScale );
                    r += src[0];
                    g += src[1];
                    b += src[2];
                }
            }
            dst = buffer + 18 + 3 * ( y * 128 + x );
            dst[0] = b / 12;
            dst[1] = g / 12;
            dst[2] = r / 12;
        }
    }
    
    fileSystem->WriteFile( checkname, buffer, 128 * 128 * 3 + 18 );
    
    Hunk_FreeTempMemory( buffer );
    Hunk_FreeTempMemory( allsource );
    
    CL_RefPrintf( PRINT_ALL, "Wrote %s\n", checkname );
}

/*
==================
R_ScreenShot_f

screenshot
screenshot [silent]
screenshot [levelshot]
screenshot [filename]

Doesn't print the pacifier message if there is a second arg
==================
*/
void R_ScreenShot_f( void )
{
    valueType	checkname[MAX_OSPATH];
    static	sint	lastNumber = -1;
    bool	silent;
    
    if( !strcmp( cmdSystem->Argv( 1 ), "levelshot" ) )
    {
        R_LevelShot();
        return;
    }
    
    if( !strcmp( cmdSystem->Argv( 1 ), "silent" ) )
    {
        silent = true;
    }
    else
    {
        silent = false;
    }
    
    if( cmdSystem->Argc() == 2 && !silent )
    {
        // explicit filename
        Q_vsprintf_s( checkname, MAX_OSPATH, MAX_OSPATH, "screenshots/%s.tga", cmdSystem->Argv( 1 ) );
    }
    else
    {
        // scan for a free filename
        
        // if we have saved a previous screenshot, don't scan
        // again, because recording demo avis can involve
        // thousands of shots
        if( lastNumber == -1 )
        {
            lastNumber = 0;
        }
        // scan for a free number
        for( ; lastNumber <= 9999 ; lastNumber++ )
        {
            R_ScreenshotFilename( lastNumber, checkname );
            
            if( !fileSystem->FileExists( checkname ) )
            {
                break; // file doesn't exist
            }
        }
        
        if( lastNumber >= 9999 )
        {
            CL_RefPrintf( PRINT_ALL, "ScreenShot: Couldn't create a file\n" );
            return;
        }
        
        lastNumber++;
    }
    
    R_TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, false );
    
    if( !silent )
    {
        CL_RefPrintf( PRINT_ALL, "Wrote %s\n", checkname );
    }
}

void R_ScreenShotJPEG_f( void )
{
    valueType		checkname[MAX_OSPATH];
    static	sint	lastNumber = -1;
    bool	silent;
    
    if( !strcmp( cmdSystem->Argv( 1 ), "levelshot" ) )
    {
        R_LevelShot();
        return;
    }
    
    if( !strcmp( cmdSystem->Argv( 1 ), "silent" ) )
    {
        silent = true;
    }
    else
    {
        silent = false;
    }
    
    if( cmdSystem->Argc() == 2 && !silent )
    {
        // explicit filename
        Q_vsprintf_s( checkname, MAX_OSPATH, MAX_OSPATH, "screenshots/%s.jpg", cmdSystem->Argv( 1 ) );
    }
    else
    {
        // scan for a free filename
        
        // if we have saved a previous screenshot, don't scan
        // again, because recording demo avis can involve
        // thousands of shots
        if( lastNumber == -1 )
        {
            lastNumber = 0;
        }
        // scan for a free number
        for( ; lastNumber <= 9999 ; lastNumber++ )
        {
            R_ScreenshotFilenameJPEG( lastNumber, checkname );
            
            if( !fileSystem->FileExists( checkname ) )
            {
                break; // file doesn't exist
            }
        }
        
        if( lastNumber == 10000 )
        {
            CL_RefPrintf( PRINT_ALL, "ScreenShot: Couldn't create a file\n" );
            return;
        }
        
        lastNumber++;
    }
    
    R_TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, true );
    
    if( !silent )
    {
        CL_RefPrintf( PRINT_ALL, "Wrote %s\n", checkname );
    }
}

//============================================================================

/*
==================
R_ExportCubemaps
==================
*/
void R_ExportCubemaps( void )
{
    exportCubemapsCommand_t*	cmd;
    
    cmd = ( exportCubemapsCommand_t* )R_GetCommandBuffer( sizeof( *cmd ) );
    if( !cmd )
    {
        return;
    }
    cmd->commandId = RC_EXPORT_CUBEMAPS;
}


/*
==================
R_ExportCubemaps_f
==================
*/
void R_ExportCubemaps_f( void )
{
    R_ExportCubemaps();
}

//============================================================================

/*
==================
RB_TakeVideoFrameCmd
==================
*/
const void* RB_TakeVideoFrameCmd( const void* data )
{
    const videoFrameCommand_t*	cmd;
    uchar8*				cBuf;
    uint32				memcount, linelen;
    sint				padwidth, avipadwidth, padlen, avipadlen;
    sint packAlign;
    
    // finish any 2D drawing if needed
    if( tess.numIndexes )
        RB_EndSurface();
        
    cmd = ( const videoFrameCommand_t* )data;
    
    qglGetIntegerv( GL_PACK_ALIGNMENT, &packAlign );
    
    linelen = cmd->width * 3;
    
    // Alignment stuff for qglReadPixels
    padwidth = PAD( linelen, packAlign );
    padlen = padwidth - linelen;
    // AVI line padding
    avipadwidth = PAD( linelen, AVI_LINE_PADDING );
    avipadlen = avipadwidth - linelen;
    
    cBuf = ( uchar8* )PADP( cmd->captureBuffer, packAlign );
    
    qglReadPixels( 0, 0, cmd->width, cmd->height, GL_RGB,
                   GL_UNSIGNED_BYTE, cBuf );
                   
    memcount = padwidth * cmd->height;
    
    if( cmd->motionJpeg )
    {
        memcount = RE_SaveJPGToBuffer( cmd->encodeBuffer, linelen * cmd->height,
                                       r_aviMotionJpegQuality->integer,
                                       cmd->width, cmd->height, cBuf, padlen );
        clientAVISystem->WriteAVIVideoFrame( cmd->encodeBuffer, memcount );
    }
    else
    {
        uchar8* lineend, *memend;
        uchar8* srcptr, *destptr;
        
        srcptr = cBuf;
        destptr = cmd->encodeBuffer;
        memend = srcptr + memcount;
        
        // swap R and B and remove line paddings
        while( srcptr < memend )
        {
            lineend = srcptr + linelen;
            while( srcptr < lineend )
            {
                *destptr++ = srcptr[2];
                *destptr++ = srcptr[1];
                *destptr++ = srcptr[0];
                srcptr += 3;
            }
            
            ::memset( destptr, '\0', avipadlen );
            destptr += avipadlen;
            
            srcptr += padlen;
        }
        
        clientAVISystem->WriteAVIVideoFrame( cmd->encodeBuffer, avipadwidth * cmd->height );
    }
    
    return ( const void* )( cmd + 1 );
}

//============================================================================

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState( void )
{
    qglClearDepth( 1.0f );
    
    qglCullFace( GL_FRONT );
    
    GL_BindNullTextures();
    
    if( glRefConfig.framebufferObject )
        GL_BindNullFramebuffers();
        
    GL_TextureMode( r_textureMode->string );
    
    //qglShadeModel( GL_SMOOTH );
    qglDepthFunc( GL_LEQUAL );
    
    //
    // make sure our GL state vector is set correctly
    //
    glState.glStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;
    glState.storedGlState = 0;
    glState.faceCulling = CT_TWO_SIDED;
    glState.faceCullFront = true;
    
    GL_BindNullProgram();
    
    if( glRefConfig.vertexArrayObject )
        qglBindVertexArray( 0 );
        
    qglBindBuffer( GL_ARRAY_BUFFER, 0 );
    qglBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glState.currentVao = nullptr;
    glState.vertexAttribsEnabled = 0;
    
    qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    qglDepthMask( GL_TRUE );
    qglDisable( GL_DEPTH_TEST );
    qglEnable( GL_SCISSOR_TEST );
    qglDisable( GL_CULL_FACE );
    qglDisable( GL_BLEND );
    
    if( glRefConfig.seamlessCubeMap )
        qglEnable( GL_TEXTURE_CUBE_MAP_SEAMLESS );
        
    // GL_POLYGON_OFFSET_FILL will be glEnable()d when this is used
    qglPolygonOffset( r_offsetFactor->value, r_offsetUnits->value );
    
    qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );	// FIXME: get color of sky
}

/*
================
R_PrintLongString

Workaround for CL_RefPrintf's 1024 characters buffer limit.
================
*/
void R_PrintLongString( pointer string )
{
    valueType buffer[1024];
    pointer p;
    sint size = strlen( string );
    
    p = string;
    while( size > 0 )
    {
        Q_strncpyz( buffer, p, sizeof( buffer ) );
        CL_RefPrintf( PRINT_ALL, "%s", buffer );
        p += 1023;
        size -= 1023;
    }
}

/*
================
GfxInfo_f
================
*/
void GfxInfo_f( void )
{
    pointer enablestrings[] =
    {
        "disabled",
        "enabled"
    };
    pointer fsstrings[] =
    {
        "windowed",
        "fullscreen"
    };
    
    CL_RefPrintf( PRINT_ALL, "GL_VENDOR: %s\n", glConfig.vendor_string );
    CL_RefPrintf( PRINT_ALL, "GL_RENDERER: %s\n", glConfig.renderer_string );
    CL_RefPrintf( PRINT_ALL, "GL_VERSION: %s\n", glConfig.version_string );
#ifdef _DEBUG
    if( qglGetStringi )
    {
        GLint numExtensions;
        sint i;
        
        qglGetIntegerv( GL_NUM_EXTENSIONS, &numExtensions );
        for( i = 0; i < numExtensions; i++ )
        {
            CL_RefPrintf( PRINT_ALL, "%s ", qglGetStringi( GL_EXTENSIONS, i ) );
        }
    }
    else
    {
        R_PrintLongString( glConfig.extensions_string );
    }
#endif
    CL_RefPrintf( PRINT_ALL, "GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize );
    CL_RefPrintf( PRINT_ALL, "GL_MAX_TEXTURE_IMAGE_UNITS: %d\n", glConfig.numTextureUnits );
    CL_RefPrintf( PRINT_ALL, "PIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
    CL_RefPrintf( PRINT_ALL, "MODE: %d, %d x %d %s hz:", r_mode->integer, glConfig.vidWidth, glConfig.vidHeight, fsstrings[r_fullscreen->integer == 1] );
    if( glConfig.displayFrequency )
    {
        CL_RefPrintf( PRINT_ALL, "%d\n", glConfig.displayFrequency );
    }
    else
    {
        CL_RefPrintf( PRINT_ALL, "N/A\n" );
    }
    if( glConfig.deviceSupportsGamma )
    {
        CL_RefPrintf( PRINT_ALL, "GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits );
    }
    else
    {
        CL_RefPrintf( PRINT_ALL, "GAMMA: software w/ %d overbright bits\n", tr.overbrightBits );
    }
    
    CL_RefPrintf( PRINT_ALL, "texturemode: %s\n", r_textureMode->string );
    CL_RefPrintf( PRINT_ALL, "picmip: %d\n", r_picmip->integer );
    CL_RefPrintf( PRINT_ALL, "texture bits: %d\n", r_texturebits->integer );
    CL_RefPrintf( PRINT_ALL, "compiled vertex arrays: %s\n", enablestrings[qglLockArraysEXT != 0 ] );
    CL_RefPrintf( PRINT_ALL, "texenv add: %s\n", enablestrings[glConfig.textureEnvAddAvailable != 0] );
    CL_RefPrintf( PRINT_ALL, "compressed textures: %s\n", enablestrings[glConfig.textureCompression != TC_NONE] );
    if( r_vertexLight->integer || glConfig.hardwareType == GLHW_PERMEDIA2 )
    {
        CL_RefPrintf( PRINT_ALL, "HACK: using vertex lightmap approximation\n" );
    }
    if( glConfig.hardwareType == GLHW_RAGEPRO )
    {
        CL_RefPrintf( PRINT_ALL, "HACK: ragePro approximations\n" );
    }
    if( glConfig.hardwareType == GLHW_RIVA128 )
    {
        CL_RefPrintf( PRINT_ALL, "HACK: riva128 approximations\n" );
    }
    if( r_finish->integer )
    {
        CL_RefPrintf( PRINT_ALL, "Forcing glFinish\n" );
    }
}

/*
===============
R_Register
===============
*/
void R_Register( void )
{
    //
    // latched and archived variables
    //
    r_allowExtensions = cvarSystem->Get( "r_allowExtensions", "1", CVAR_ARCHIVE | CVAR_LATCH, "Toggles the functions of various OpenGL extensions. 0=disables;1=enables. " );
    r_ext_compressed_textures = cvarSystem->Get( "r_ext_compressed_textures", "0", CVAR_ARCHIVE | CVAR_LATCH, "Compress textures" );
    r_ext_multitexture = cvarSystem->Get( "r_ext_multitexture", "1", CVAR_ARCHIVE | CVAR_LATCH, "Toggles hardware mutitexturing if available. 0=disables; 1=enables. " );
    r_ext_compiled_vertex_array = cvarSystem->Get( "r_ext_compiled_vertex_array", "1", CVAR_ARCHIVE | CVAR_LATCH, "Toggles hardware compiled vertex array rendering method if available. 0=disables;1=enables." );
    r_ext_texture_env_add = cvarSystem->Get( "r_ext_texture_env_add", "1", CVAR_ARCHIVE | CVAR_LATCH, "Enables additive blending when using multi-texturing" );
    
    r_ext_framebuffer_object = cvarSystem->Get( "r_ext_framebuffer_object", "1", CVAR_ARCHIVE | CVAR_LATCH, "Toggles hardware compiled framebuffer objects rendering method if available. 0=disables;1=enables." );
    r_ext_texture_float = cvarSystem->Get( "r_ext_texture_float", "0", CVAR_ARCHIVE | CVAR_LATCH, "Toggles hardware compiled texture float rendering method if available. 0=disables;1=enables." );
    r_ext_framebuffer_multisample = cvarSystem->Get( "r_ext_framebuffer_multisample", "0", CVAR_ARCHIVE | CVAR_LATCH, "Toggles hardware compiled framebuffer multisample rendering method if available. 0=disables;1=enables." );
    r_arb_seamless_cube_map = cvarSystem->Get( "r_arb_seamless_cube_map", "0", CVAR_ARCHIVE | CVAR_LATCH, "Toggles hardware compiled seamless cubemap rendering method if available. 0=disables;1=enables." );
    r_arb_vertex_array_object = cvarSystem->Get( "r_arb_vertex_array_object", "1", CVAR_ARCHIVE | CVAR_LATCH, "Toggles hardware compiled vertex arry object rendering method if available. 0=disables;1=enables." );
    r_ext_direct_state_access = cvarSystem->Get( "r_ext_direct_state_access", "1", CVAR_ARCHIVE | CVAR_LATCH, "Toggles hardware compiled direct state access rendering method if available. 0=disables;1=enables." );
    
    r_ext_texture_filter_anisotropic = cvarSystem->Get( "r_ext_texture_filter_anisotropic", "0", CVAR_ARCHIVE | CVAR_LATCH, "When it is set to 1 image quality can be improved - elements of the scene will appear smoother when viewed at near edge angles. Your video card must support this however. Set x to 1 to disable anisotropic filtering for improved performance, or if your video card doesn't support it. If not supported it will automatically be disabled." );
    r_ext_max_anisotropy = cvarSystem->Get( "r_ext_max_anisotropy", "0", CVAR_ARCHIVE | CVAR_LATCH, "Sets the maximum level of anisotropic filtering" );
    
    r_defaultImage = cvarSystem->Get( "r_defaultImage", "", CVAR_ARCHIVE | CVAR_LATCH, "Replace the default (missing texture) images" );
    r_picmip = cvarSystem->Get( "r_picmip", "1", CVAR_ARCHIVE | CVAR_LATCH, "Sets maximum texture size. 0=highest detail;8=unbelievably ugly." );
    r_roundImagesDown = cvarSystem->Get( "r_roundImagesDown", "1", CVAR_ARCHIVE | CVAR_LATCH, "Set rounding down factor (larger = faster, lower quality)" );
    r_colorMipLevels = cvarSystem->Get( "r_colorMipLevels", "0", CVAR_LATCH, "A developer aid to see texture mip usage" );
    cvarSystem->CheckRange( r_picmip, 0, 16, true );
    r_detailTextures = cvarSystem->Get( "r_detailtextures", "0", CVAR_ARCHIVE | CVAR_LATCH, "Enables the usage of detail texturing stages" );
    r_texturebits = cvarSystem->Get( "r_texturebits", "0", CVAR_ARCHIVE | CVAR_LATCH, "Sets the texture quality level" );
    r_colorbits = cvarSystem->Get( "r_colorbits", "0", CVAR_ARCHIVE | CVAR_LATCH, "Sets video color depth" );
    r_alphabits = cvarSystem->Get( "r_alphabits", "1", CVAR_ARCHIVE | CVAR_LATCH, "Sets ovideo alpha bits" );
    r_stencilbits = cvarSystem->Get( "r_stencilbits", "8", CVAR_ARCHIVE | CVAR_LATCH, "Adjusts rendering of hardware's stencil buffer depth." );
    r_depthbits = cvarSystem->Get( "r_depthbits", "0", CVAR_ARCHIVE | CVAR_LATCH, "Sets depth bits-per-pixel. Mutually exclusive with r_colorbits." );
    r_ext_multisample = cvarSystem->Get( "r_ext_multisample", "0", CVAR_ARCHIVE | CVAR_LATCH, "Activate OpenGL texture multisampling" );
    cvarSystem->CheckRange( r_ext_multisample, 0, 4, true );
    r_overBrightBits = cvarSystem->Get( "r_overBrightBits", "0", CVAR_ARCHIVE | CVAR_LATCH, "Ambient lighting applied to in-game entities or objects" );
    r_ignorehwgamma = cvarSystem->Get( "r_ignorehwgamma", "0", CVAR_ARCHIVE | CVAR_LATCH, "Enables ignoreing of hardware gamma settings" );
    r_mode = cvarSystem->Get( "r_mode", "-2", CVAR_ARCHIVE | CVAR_LATCH, "Screen resolution setting. -1 enables r_customWidth and r_customHeight" );
    r_fullscreen = cvarSystem->Get( "r_fullscreen", "1", CVAR_ARCHIVE | CVAR_LATCH, "Enables fullscreen view" );
    r_noborder = cvarSystem->Get( "r_noborder", "0", CVAR_ARCHIVE | CVAR_LATCH, "Remove window decoration from window managers, like borders and titlebar" );
    r_customwidth = cvarSystem->Get( "r_customwidth", "1680", CVAR_ARCHIVE | CVAR_LATCH, "Sets the custom horizontal resolution when r_mode -1" );
    r_customheight = cvarSystem->Get( "r_customheight", "1050", CVAR_ARCHIVE | CVAR_LATCH, "Sets the custom vertical resolution when r_mode -1" );
    r_simpleMipMaps = cvarSystem->Get( "r_simpleMipMaps", "1", CVAR_ARCHIVE | CVAR_LATCH, "Toggle the use of simple mip mapping. used to dumb - down resoluiton displays for slower machines" );
    r_vertexLight = cvarSystem->Get( "r_vertexLight", "0", CVAR_ARCHIVE | CVAR_LATCH, "Enable vertex lighting (faster, lower quality than lightmap) removes lightmaps, forces every shader to only use a single rendering pass, no layered transparancy, environment mapping, world lighting is completely static, and there is no dynamic lighting when in vertex lighting mode. (recommend dynamiclight 0 and this 1) direct FPS benefit" );
    r_uiFullScreen = cvarSystem->Get( "r_uifullscreen", "0", 0, "Sets the User Interface(UI) running fullscreen." );
    r_subdivisions = cvarSystem->Get( "r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH, "Patch mesh/curve sub divisions, sets number of subdivisions of curves, increasing makes curves into straights." );
    r_stereoEnabled = cvarSystem->Get( "r_stereoEnabled", "0", CVAR_CHEAT, "Enables stereo separation, for 3D effects" );
    r_greyscale = cvarSystem->Get( "r_greyscale", "0", CVAR_ARCHIVE | CVAR_LATCH, "Enables greyscaling of everything" );
    cvarSystem->CheckRange( r_greyscale, 0, 1, false );
    
    r_hdr = cvarSystem->Get( "r_hdr", "1", CVAR_ARCHIVE | CVAR_LATCH, "Do scene rendering in a framebuffer with high dynamic range. (Less banding, and exposure changes look much better)" );
    r_truehdr = cvarSystem->Get( "r_truehdr", "0", CVAR_ARCHIVE, "Do scene rendering in a framebuffer with high dynamic range with GLSL shaders" );
    r_postProcess = cvarSystem->Get( "r_postProcess", "1", CVAR_ARCHIVE, "Enable post-processing" );
    
    r_toneMap = cvarSystem->Get( "r_toneMap", "1", CVAR_ARCHIVE, "Enable tone mapping. Requires r_hdr and r_postProcess." );
    r_forceToneMap = cvarSystem->Get( "r_forceToneMap", "0", CVAR_CHEAT, "Override built-in and map tonemap settings and use cvars r_forceToneMapAvg" );
    r_forceToneMapMin = cvarSystem->Get( "r_forceToneMapMin", "-8.0", CVAR_CHEAT, "After mapping average, luminance below this level is mapped to black. Requires r_forceToneMap. -5 - Not noticeable. -3.25 - Normal. (default) 0.0 - Too dar" );
    r_forceToneMapAvg = cvarSystem->Get( "r_forceToneMapAvg", "-2.0", CVAR_CHEAT, "Map average scene luminance to this value, in powers of two. Requires r_forceToneMap. -2.0 - Dark. -1.0 - Kinda dark. (default). 2.0 - Too bright." );
    r_forceToneMapMax = cvarSystem->Get( "r_forceToneMapMax", "0.0", CVAR_CHEAT, "After mapping average, luminance above this level is mapped to white. Requires r_forceToneMap. 0.0 - Too bright. 1.0 - Normal. (default). 2.0 - Washed out" );
    
    r_autoExposure = cvarSystem->Get( "r_autoExposure", "1", CVAR_ARCHIVE, "Do automatic exposure based on scene brightness. Hardcoded to -2 to 2 on maps that don't specify otherwise. Requires r_hdr, r_postprocess, and r_toneMap. 0 - No. 1 - Yes. (default)" );
    r_forceAutoExposure = cvarSystem->Get( "r_forceAutoExposure", "0", CVAR_CHEAT, "Override built-in and map auto exposure settings and use cvars r_forceAutoExposureMin and r_forceAutoExposureMax. 0 - No. (default) 1 - Yes." );
    r_forceAutoExposureMin = cvarSystem->Get( "r_forceAutoExposureMin", "-2.0", CVAR_CHEAT, "Set minimum exposure to this value, in powers of two. Requires r_forceAutoExpsure. -3.0 - Dimmer. -2.0 - Normal. (default) -1.0 - Brighter." );
    r_forceAutoExposureMax = cvarSystem->Get( "r_forceAutoExposureMax", "2.0", CVAR_CHEAT, "Set maximum exposure to this value, in powers of two. Requires r_forceAutoExpsure. 1.0 - Dimmer. 2.0 - Normal. (default) 3.0 - Brighter" );
    
    r_cameraExposure = cvarSystem->Get( "r_cameraExposurre", "4", CVAR_CHEAT, "Alter brightness, in powers of two. -2 - 4x as dark. 0 - Normal. (default) 0.5 - Sqrt(2)x as bright. 2 - 4x as bright." );
    
    r_depthPrepass = cvarSystem->Get( "r_depthPrepass", "1", CVAR_ARCHIVE, "Do a depth-only pass before rendering. Speeds up rendering in cases where advanced features are used. Required for r_sunShadows. 0 - No. 1 - Yes. (default)" );
    r_ssao = cvarSystem->Get( "r_ssao", "0", CVAR_LATCH | CVAR_ARCHIVE, "Enable screen-space ambient occlusion. Currently eats framerate and has some visible artifacts. 0 - No. (default) 1 - Yes." );
    
    r_normalMapping = cvarSystem->Get( "r_normalMapping", "1", CVAR_ARCHIVE | CVAR_LATCH, "Enable normal maps for materials that support it. 0 - No. 1 - Yes. (default)" );
    r_specularMapping = cvarSystem->Get( "r_specularMapping", "1", CVAR_ARCHIVE | CVAR_LATCH, "Enable specular maps for materials that support it. 0 - No. 1 - Yes. (default)" );
    r_deluxeMapping = cvarSystem->Get( "r_deluxeMapping", "1", CVAR_ARCHIVE | CVAR_LATCH, "Enable deluxe mapping. (Map is compiled with light directions.) Even if the map doesn't have deluxe mapping compiled in, an approximation based on the lightgrid will be used. 0 - No. 1 - Yes. (default)" );
    r_parallaxMapping = cvarSystem->Get( "r_parallaxMapping", "1", CVAR_ARCHIVE | CVAR_LATCH, "Enable parallax mapping for materials that support it. 0 - No. (default) 1 - Use parallax occlusion mapping. 2 - Use relief mapping. (slower)" );
    r_parallaxMapShadows = cvarSystem->Get( "r_parallaxMapShadows", "0", CVAR_ARCHIVE | CVAR_LATCH, "Enable self-shadowing on parallax map supported materials. 0 - No. (default) 1 - Yes." );
    r_cubeMapping = cvarSystem->Get( "r_cubeMapping", "1", CVAR_ARCHIVE | CVAR_LATCH, "Enable cubemap environment mapping" );
    r_horizonFade = cvarSystem->Get( "r_horizonFade", "3", CVAR_ARCHIVE | CVAR_LATCH, "Enable horizon fade cubemap environment mapping" );
    r_cubemapSize = cvarSystem->Get( "r_cubemapSize", "128", CVAR_ARCHIVE | CVAR_LATCH, "Cubemap size" );
    r_deluxeSpecular = cvarSystem->Get( "r_deluxeSpecular", "0.3", CVAR_ARCHIVE | CVAR_LATCH, "Enable deluxe specular for materials that support it" );
    r_pbr = cvarSystem->Get( "r_pbr", "1", CVAR_ARCHIVE | CVAR_LATCH, "Enable physically based rendering." );
    r_baseNormalX = cvarSystem->Get( "r_baseNormalX", "1.0", CVAR_ARCHIVE | CVAR_LATCH, "Set the scale of the X values from normal maps when the normalScale keyword is not used. -1 - Flip X. 0 - Ignore X. 1 - Normal X. (default) 2 - Double X." );
    r_baseNormalY = cvarSystem->Get( "r_baseNormalY", "1.0", CVAR_ARCHIVE | CVAR_LATCH, "Set the scale of the Y values from normal maps when the normalScale keyword is not used. -1 - Flip Y. 0 - Ignore Y. 1 - Normal Y. (default) 2 - Double Y" );
    r_baseParallax = cvarSystem->Get( "r_baseParallax", "0.001", CVAR_ARCHIVE | CVAR_LATCH, "Sets the scale of the parallax effect for materials when the parallaxDepth keyword is not used. 0 - No depth. 0.01 - Pretty smooth. 0.05 - Standard depth. (default) 0.001" );
    r_baseSpecular = cvarSystem->Get( "r_baseSpecular", "0.04", CVAR_ARCHIVE | CVAR_LATCH, "Set the specular reflectance of materials which don't include a specular map or use the specularReflectance keyword. 0 - No. 0.04 - Realistic. (default) 1.0 - Ack." );
    r_baseGloss = cvarSystem->Get( "r_baseGloss", "0.3", CVAR_ARCHIVE | CVAR_LATCH, "Set the glossiness of materials which don't include a specular map or use the specularExponent keyword. 0 - Rough. 0.3 - Default. 1.0 - Shiny." );
    r_dlightMode = cvarSystem->Get( "r_dlightMode", "0", CVAR_ARCHIVE | CVAR_LATCH, "Change how dynamic lights look. 0 - Quake 3 style dlights, fake brightening. (default) 1 - Actual lighting, no shadows. 2 - Light and shadows." );
    r_pshadowDist = cvarSystem->Get( "r_pshadowDist", "128", CVAR_ARCHIVE, "Virtual camera distance when creating shadowmaps for projected shadows" );
    r_mergeLightmaps = cvarSystem->Get( "r_mergeLightmaps", "1", CVAR_ARCHIVE | CVAR_LATCH, "Merge the small (128x128) lightmaps into 2 or fewer giant (4096x4096) lightmaps. Easy speedup. 0 - Don't. 1 - Do. (default)" );
    r_imageUpsample = cvarSystem->Get( "r_imageUpsample", "0", CVAR_ARCHIVE | CVAR_LATCH, "Use interpolation to artificially increase the resolution of all textures. Looks good in certain circumstances. 0 - No. (default) 1 - 2x size. 2 - 4x size. 3 - 8x size, etc" );
    r_imageUpsampleMaxSize = cvarSystem->Get( "r_imageUpsampleMaxSize", "1024", CVAR_ARCHIVE | CVAR_LATCH, "Maximum texture size when upsampling textures. 1024 - Default. 2048 - Really nice. 4096 - Really slow." );
    r_imageUpsampleType = cvarSystem->Get( "r_imageUpsampleType", "1", CVAR_ARCHIVE | CVAR_LATCH, "Type of interpolation when upsampling textures. 0 - None. (probably broken) 1 - Bad but fast (default, FCBI without second derivatives) 2 - Okay but slow (normal FCBI)" );
    r_genNormalMaps = cvarSystem->Get( "r_genNormalMaps", "0", CVAR_ARCHIVE | CVAR_LATCH, "Naively generate normal maps for all textures. 0 - Don't. (default) 1 - Do." );
    
    r_forceSun = cvarSystem->Get( "r_forceSun", "1", CVAR_ARCHIVE | CVAR_LATCH, "Force sunlight and shadows, using sun position from sky material. 0 - Don't. (default) 1 - Do. 2 - Sunrise, sunset." );
    r_forceSunLightScale = cvarSystem->Get( "r_forceSunLightScale", "1.0", CVAR_CHEAT, "Scale sun brightness by this factor when r_forceSun 1. 1.0 - Default" );
    r_forceSunAmbientScale = cvarSystem->Get( "r_forceSunAmbientScale", "0.5", CVAR_CHEAT, "Scale sun ambient brightness by this factor when r_forceSun 1. 0.5 - Default" );
    r_drawSunRays = cvarSystem->Get( "r_drawSunRays", "1", CVAR_ARCHIVE | CVAR_LATCH, "Draw sun rays" );
    r_sunlightMode = cvarSystem->Get( "r_sunlightMode", "2", CVAR_ARCHIVE | CVAR_LATCH, "Specify the method used to add sunlight to the scene. 0 - No. 1 - Multiply lit areas by light scale, and shadowed areas by ambient scale. (default) 2" );
    
    r_sunShadows = cvarSystem->Get( "r_sunShadows", "1", CVAR_ARCHIVE | CVAR_LATCH, "Enable sunlight and cascaded shadow maps for it on maps that support it. 0 - No. 1 - Yes. (default)" );
    r_shadowFilter = cvarSystem->Get( "r_shadowFilter", "0", CVAR_ARCHIVE | CVAR_LATCH, "Enable filtering shadows for a smoother look. 0 - No. 1 - Some. (default) 2 - Much." );
    r_shadowBlur = cvarSystem->Get( "r_shadowBlur", "1", CVAR_ARCHIVE | CVAR_LATCH, "Enable shadow blur." );
    r_shadowMapSize = cvarSystem->Get( "r_shadowMapSize", "1024", CVAR_ARCHIVE | CVAR_LATCH, "Size of each cascaded shadow map. 256 - 256x256, ugly, probably shouldn't go below this. 512 - 512x512, passable. 1024 - 1024x1024, good. (default) 2048 - 2048x2048, extreme. 4096 - 4096x4096, indistinguishable from 2048." );
    r_shadowCascadeZNear = cvarSystem->Get( "r_shadowCascadeZNear", "4", CVAR_ARCHIVE | CVAR_LATCH, "Size of each cascaded shadow map. 256 - 256x256, ugly, probably shouldn't go below this. 512 - 512x512, passable. 1024 - 1024x1024, good. (default) 2048 - 2048x2048, extreme. 4096 - 4096x4096, indistinguishable from 2048." );
    r_shadowCascadeZFar = cvarSystem->Get( "r_shadowCascadeZFar", "1024", CVAR_ARCHIVE | CVAR_LATCH, "Far plane for shadow cascade frustums. 3072 - Default." );
    r_shadowCascadeZBias = cvarSystem->Get( "r_shadowCascadeZBias", "0", CVAR_ARCHIVE | CVAR_LATCH, "Z-bias for shadow cascade frustums. -256 - Default." );
    r_ignoreDstAlpha = cvarSystem->Get( "r_ignoreDstAlpha", "1", CVAR_ARCHIVE | CVAR_LATCH, "Ignoring DST Alpha" );
    
    r_lensflare = cvarSystem->Get( "r_lensflare", "1", CVAR_ARCHIVE, "Enabled lens flare effects" );
    r_anamorphic = cvarSystem->Get( "r_anamorphic", "0", CVAR_ARCHIVE, "Enabled anamorphic effects" );
    r_anamorphicDarkenPower = cvarSystem->Get( "r_anamorphicDarkenPower", "256.0", CVAR_ARCHIVE, "Setting anamorphic darken power" );
    r_ssgi = cvarSystem->Get( "r_ssgi", "0", CVAR_ARCHIVE, "Enabled Screen Space Global Illumination effects" );
    r_ssgiWidth = cvarSystem->Get( "r_ssgiWidth", "8.0", CVAR_ARCHIVE, "Setting Screen Space Global Illumination width" );
    r_ssgiSamples = cvarSystem->Get( "r_ssgiSamples", "2", CVAR_ARCHIVE, "Setting Screen Space Global Illumination samples" );
    r_ssr = cvarSystem->Get( "r_ssr", "0", CVAR_ARCHIVE, "Enabled Screen Space Reflections effects" );
    r_ssrStrength = cvarSystem->Get( "r_ssrStrength", "0.02", CVAR_ARCHIVE, "Setting Screen Space Reflections strength" );
    r_sse = cvarSystem->Get( "r_sse", "0", CVAR_ARCHIVE, "Enabled Screen Space Emission effects" );
    r_sseStrength = cvarSystem->Get( "r_sseStrength", "0.02", CVAR_ARCHIVE, "Setting Screen Space Emission strength" );
    r_darkexpand = cvarSystem->Get( "r_darkexpand", "1", CVAR_ARCHIVE, "Enabled Dark Expand effects" );
    r_dof = cvarSystem->Get( "r_dof", "0", CVAR_ARCHIVE, "Enabled Depth of Field effects" );
    r_esharpening = cvarSystem->Get( "r_esharpening", "0", CVAR_ARCHIVE, "Enabled Sharpering effects" );
    r_esharpening2 = cvarSystem->Get( "r_esharpening2", "0", CVAR_ARCHIVE, "Enabled Sharpering effects" );
    r_multipost = cvarSystem->Get( "r_multipost", "0", CVAR_ARCHIVE, "Enabled Multipost effects" );
    r_textureClean = cvarSystem->Get( "r_textureClean", "0", CVAR_ARCHIVE, "Enabled Texture Clen effects" );
    r_textureCleanSigma = cvarSystem->Get( "r_textureCleanSigma", "1.2", CVAR_ARCHIVE, "Setting Texture Clean sigma" );
    r_textureCleanBSigma = cvarSystem->Get( "r_textureCleanBSigma", "0.1", CVAR_ARCHIVE, "Setting Texture Clean B sigma" );
    r_textureCleanMSize = cvarSystem->Get( "r_textureCleanMSize", "6.0", CVAR_ARCHIVE, "Setting Texture Clean size" );
    r_trueAnaglyph = cvarSystem->Get( "r_trueAnaglyph", "0", CVAR_ARCHIVE, "Enabled Anaglyph effects" );
    r_trueAnaglyphSeparation = cvarSystem->Get( "r_trueAnaglyphSeparation", "8.0", CVAR_ARCHIVE, "Setting Anaglyph separation" );
    r_trueAnaglyphRed = cvarSystem->Get( "r_trueAnaglyphRed", "0.0", CVAR_ARCHIVE, "Setting Anaglyph red color" );
    r_trueAnaglyphGreen = cvarSystem->Get( "r_trueAnaglyphGreen", "0.0", CVAR_ARCHIVE, "Setting Anaglyph green color" );
    r_trueAnaglyphBlue = cvarSystem->Get( "r_trueAnaglyphBlue", "0.0", CVAR_ARCHIVE, "Setting Anaglyph blue color" );
    r_vibrancy = cvarSystem->Get( "r_vibrancy", "0.4", CVAR_ARCHIVE, "Enabled vibrancy effects" );
    r_bloom = cvarSystem->Get( "r_bloom", "0", CVAR_ARCHIVE, "Enabled Bloom effects" );
    r_bloomPasses = cvarSystem->Get( "r_bloomPasses", "1", CVAR_ARCHIVE, "Bloom Passes" );
    r_bloomDarkenPower = cvarSystem->Get( "r_bloomDarkenPower", "5.0", CVAR_ARCHIVE, "Darken power for bloom" );
    r_bloomScale = cvarSystem->Get( "r_bloomScale", "1.5", CVAR_ARCHIVE, "Scaling bloom effects" );
    r_fxaa = cvarSystem->Get( "r_fxaa", "0", CVAR_ARCHIVE, "Enabled Full Screen Anti Aliasing effects" );
    
    r_texturedetail = cvarSystem->Get( "r_textureDetail", "0", CVAR_ARCHIVE, "Enabled texture details effects" );
    r_texturedetailStrength = cvarSystem->Get( "r_texturedetailStrength", "0.004", CVAR_ARCHIVE, "Toggles texture details strength" );
    r_rbm = cvarSystem->Get( "r_rbm", "0", CVAR_ARCHIVE, "Enabled Reflective Bump Mapping effects" );
    r_rbmStrength = cvarSystem->Get( "r_rbmStrength", "0.015", CVAR_ARCHIVE, "Toggles Reflective Bump Mapping strength" );
    r_screenblur = cvarSystem->Get( "r_screenBlur", "0", CVAR_ARCHIVE, "Enabled screen blur" );
    r_brightness = cvarSystem->Get( "r_brightness", "0.0", CVAR_ARCHIVE, "Sets brightness level, gamma correction." );
    r_contrast = cvarSystem->Get( "r_contrast", "1.0", CVAR_ARCHIVE, "Sets contrast level, gamma correction." );
    r_gamma = cvarSystem->Get( "r_gamma", "1.0", CVAR_ARCHIVE, "Sets gamma level, gamma correction." );
    
    //
    // temporary latched variables that can only change over a restart
    //
    r_fullbright = cvarSystem->Get( "r_fullbright", "0", CVAR_LATCH | CVAR_CHEAT, "Toggles lightmaps when r_ext_multitexture is 0. Textures will be rendered at full brightness when enabled. See also r_lightmap. 0=disables;1=enables." );
    r_mapOverBrightBits = cvarSystem->Get( "r_mapOverBrightBits", "0", CVAR_LATCH, "Sets intensity of bounced light from textures (adjusts lighting saturation) when r_vertexlight is 0. Higher values=brighter lights reflected from textures." );
    r_intensity = cvarSystem->Get( "r_intensity", "1", CVAR_LATCH, "Adjusts the overall strength of colors, with emphasis on the color White. Higher values result in higher apparent overall brightness. An alternative to adjusting gamma values, with the side effect of loss of texture details at too high a value." );
    r_singleShader = cvarSystem->Get( "r_singleShader", "0", CVAR_CHEAT | CVAR_LATCH, "Possibly toggles use of 1 shader for objects that have multiple shaders" );
    
    //
    // archived variables that can change at any time
    //
    r_lodCurveError = cvarSystem->Get( "r_lodCurveError", "250", CVAR_ARCHIVE | CVAR_CHEAT, "Another level of detail setting if set to 10000" );
    r_lodbias = cvarSystem->Get( "r_lodbias", "0", CVAR_ARCHIVE, "Change the geometric level of detail (0 - 2)" );
    r_flares = cvarSystem->Get( "r_flares", "0", CVAR_ARCHIVE, "Toggle projectile flare and lighting effect. the flare effect is a translucent disk that is used to alter the colors around lights with a corona effect" );
    r_znear = cvarSystem->Get( "r_znear", "4", CVAR_CHEAT, "Set how close objects can be to the player before they're clipped out of the scene" );
    cvarSystem->CheckRange( r_znear, 0.001f, 200, false );
    r_zproj = cvarSystem->Get( "r_zproj", "64", CVAR_ARCHIVE, "Z distance of projection plane" );
    r_stereoSeparation = cvarSystem->Get( "r_stereoSeparation", "64", CVAR_ARCHIVE, "Control eye separation. Resulting separation is r_zProj divided by this value in quake3 standard units." );
    r_ignoreGLErrors = cvarSystem->Get( "r_ignoreGLErrors", "1", CVAR_ARCHIVE, "Toggles option to ignore OpenGL errors and to attempt to continue rendering. 0=disables;1=enables." );
    r_fastsky = cvarSystem->Get( "r_fastsky", "0", CVAR_ARCHIVE, "Toggles rendering of sky. Setting to 1 will also disable the view through portals. 0=enables;1=disables. " );
    r_inGameVideo = cvarSystem->Get( "r_inGameVideo", "1", CVAR_ARCHIVE, "Toggle use of video clips in game (limbo menu)" );
    r_drawSun = cvarSystem->Get( "r_drawSun", "0", CVAR_ARCHIVE, "Set to zero if you do not want to render sunlight into the equation of lighting effects" );
    r_dynamiclight = cvarSystem->Get( "r_dynamiclight", "1", CVAR_ARCHIVE, "Toggle dynamic lighting (different dynamic method of rendering lights)" );
    r_dlightBacks = cvarSystem->Get( "r_dlightBacks", "1", CVAR_ARCHIVE, "Brighter areas are changed more by dlights than dark areas." );
    r_finish = cvarSystem->Get( "r_finish", "0", CVAR_ARCHIVE, "Toggle synchronization of rendered frames (engine will wait for GL calls to finish)" );
    r_textureMode = cvarSystem->Get( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE, "Texture rendering quality. GL_NEAREST is the worst with pixellated textures and no mipmapping, GL_NEAREST_MIPMAP_NEAREST is the lowest quality you an access via in-game UI and uses low-quality textures and mipmaps, GL_LINEAR_MIPMAP_NEAREST uses good filters for textures and simple mipmaps, GL_LINEAR_MIPMAP_LINEAR corresponds to trilinear filtering in the in-game UI and is the best quality setting." );
    r_swapInterval = cvarSystem->Get( "r_swapInterval", "0", CVAR_ARCHIVE | CVAR_LATCH, "Toggle frame swapping." );
    r_facePlaneCull = cvarSystem->Get( "r_facePlaneCull", "1", CVAR_ARCHIVE, "Toggle culling of brush faces not in view (0 will slow FPS)" );
    
    r_railWidth = cvarSystem->Get( "r_railWidth", "16", CVAR_ARCHIVE, "Set width of the rail trail" );
    r_railCoreWidth = cvarSystem->Get( "r_railCoreWidth", "6", CVAR_ARCHIVE, "Set size of the rail trail's core" );
    r_railSegmentLength = cvarSystem->Get( "r_railSegmentLength", "32", CVAR_ARCHIVE, "Set distance between rail sun bursts" );
    
    r_ambientScale = cvarSystem->Get( "r_ambientScale", "0.6", CVAR_CHEAT, "Set the scale or intensity of ambient light" );
    r_directedScale = cvarSystem->Get( "r_directedScale", "1", CVAR_CHEAT, "Set scale/intensity of light shinning directly upon objects" );
    
    r_anaglyphMode = cvarSystem->Get( "r_anaglyphMode", "0", CVAR_ARCHIVE, " Enable rendering of anaglyph images red - cyan glasses : 1 red - blue : 2 red - green : 3 green - magenta : 4 To swap the colors for leftand right eye just add 4 to the value for the wanted color combination.For red - blue and red - green you probably want to enable" );
    
    //
    // temporary variables that can change at any time
    //
    r_showImages = cvarSystem->Get( "r_showImages", "0", CVAR_CHEAT | CVAR_TEMP, "Show images" );
    
    r_debugLight = cvarSystem->Get( "r_debuglight", "0", CVAR_TEMP, "Toggles the display of data representing the intensity of ambient light and intensity of direct light. 0=disables;1=enables." );
    r_debugSort = cvarSystem->Get( "r_debugSort", "0", CVAR_CHEAT, "For debugging of sort order issues, stop rendering after a given sort value" );
    r_printShaders = cvarSystem->Get( "r_printShaders", "0", 0, "Enable this when building a pak file to get a global list of all explicit shaders" );
    r_saveFontData = cvarSystem->Get( "r_saveFontData", "0", 0, "Saving font data" );
    
    r_nocurves = cvarSystem->Get( "r_nocurves", "0", CVAR_CHEAT, "Presumably enables/disables rendering of curves." );
    r_drawworld = cvarSystem->Get( "r_drawworld", "1", CVAR_CHEAT, "Toggles rendering of world brushes (map architecture). Entities will still be drawn. 0=disables;1=enables. " );
    r_lightmap = cvarSystem->Get( "r_lightmap", "0", 0, "Toggles rendering of lightmaps without rendering textures. 0=disables;1=enables" );
    r_portalOnly = cvarSystem->Get( "r_portalOnly", "0", CVAR_CHEAT, "Toggles seeing what the game engine can see through a portal. When enabled, if the game engine can see a portal, nothing else will be drawn except for the area the portal can see. 0=disables;1=enables." );
    
    r_flareSize = cvarSystem->Get( "r_flareSize", "40", CVAR_CHEAT, "Adjusts the size of flares when r_flares is 1." );
    r_flareFade = cvarSystem->Get( "r_flareFade", "7", CVAR_CHEAT, "Sets scale of fading of flares in relation to distance when r_flares is 1." );
    r_flareCoeff = cvarSystem->Get( "r_flareCoeff", FLARE_STDCOEFF, CVAR_CHEAT, "Adjusts the flares coefficient when r_flares is 1." );
    
    r_skipBackEnd = cvarSystem->Get( "r_skipBackEnd", "0", CVAR_CHEAT, "Toggle to skip renderer BackEnd" );
    
    r_measureOverdraw = cvarSystem->Get( "r_measureOverdraw", "0", CVAR_CHEAT, "Presumably implements sun's stenciling method of measuring overdraw." );
    r_lodscale = cvarSystem->Get( "r_lodscale", "5", CVAR_CHEAT, "Sets scale with which to adjust Level-Of-Detail." );
    r_norefresh = cvarSystem->Get( "r_norefresh", "0", CVAR_CHEAT, "Toggles clearing of screen prior to re-rendering. 0=allow refresh;1=disable refresh. " );
    r_drawentities = cvarSystem->Get( "r_drawentities", "1", CVAR_CHEAT, "Toggles rendering of entities, including brush models. 0=disables;1=enables. " );
    r_nocull = cvarSystem->Get( "r_nocull", "0", CVAR_CHEAT, "Toggles rendering of items/objects normally not seen from player's point-of-view. 0=disables;1=enables. " );
    r_novis = cvarSystem->Get( "r_novis", "0", CVAR_CHEAT, "Toggles the option to ignore vis data when drawing the map. 0=disables;1=enables" );
    r_showcluster = cvarSystem->Get( "r_showcluster", "0", CVAR_CHEAT, "Toggles display of clusters by number as the player enters them on the currently loaded map. 0=disables;1=enables." );
    r_speeds = cvarSystem->Get( "r_speeds", "0", CVAR_CHEAT, "Toggles the display of map geometry data. 0=disables,1=enables." );
    r_logFile = cvarSystem->Get( "r_logFile", "0", CVAR_CHEAT, "Toggles the writing of GL.LOG in the same binary directory which records all OpenGL commands used in a game session. 0=disables;1=enable" );
    r_debugSurface = cvarSystem->Get( "r_debugSurface", "0", CVAR_CHEAT, "Sets the size of the debug surface grid for curved surfaces when r_debugsurface is 1." );
    r_showtris = cvarSystem->Get( "r_showtris", "0", CVAR_CHEAT, "Toggles the drawing of polygon triangles. See also r_shownormals. 0=disables,1=enables." );
    r_showsky = cvarSystem->Get( "r_showsky", "0", CVAR_CHEAT, "Toggles the drawing of the sky in front of other brushes. 0=disables;1=enables. " );
    r_shownormals = cvarSystem->Get( "r_shownormals", "0", CVAR_CHEAT, "Toggles the drawing of short lines indicating brush and entity polygon vertices. See also r_showtris. 0=disables;1=enables." );
    r_clear = cvarSystem->Get( "r_clear", "0", CVAR_CHEAT, "Toggles the clearing of unrefreshed images (clears hall of mirrors effect). 0=disables;1=enables. " );
    r_offsetFactor = cvarSystem->Get( "r_offsetfactor", "-1", CVAR_CHEAT, "Toggles to set polygon offset factor" );
    r_offsetUnits = cvarSystem->Get( "r_offsetunits", "-2", CVAR_CHEAT, "Toggles to set polygon offset units factor" );
    r_drawBuffer = cvarSystem->Get( "r_drawBuffer", "GL_BACK", CVAR_CHEAT, "Sets which buffer is drawn to." );
    r_lockpvs = cvarSystem->Get( "r_lockpvs", "0", CVAR_CHEAT, "oggles the option of preventing the update of the PVS table as the player moves through the map. When set to 1 new areas entered will not be rendered. A mapper's tool. 0=disables;1=enables. " );
    r_noportals = cvarSystem->Get( "r_noportals", "0", CVAR_CHEAT, "Toggles player view through portals. 0=enables view;1=disables view." );
    r_shadows = cvarSystem->Get( "cg_shadows", "0", 0, "Sets type of shadows rendering when cg_marks is set to 1" );
    
    r_marksOnTriangleMeshes = cvarSystem->Get( "r_marksOnTriangleMeshes", "0", CVAR_ARCHIVE, "Toggles the marks on triangle meshes" );
    
    r_aviMotionJpegQuality = cvarSystem->Get( "r_aviMotionJpegQuality", "90", CVAR_ARCHIVE, "Controls quality of video capture when cl_aviMotionJpeg is enabled" );
    r_screenshotJpegQuality = cvarSystem->Get( "r_screenshotJpegQuality", "90", CVAR_ARCHIVE, "Controls quality of jpeg screenshots captured using screenshotJPEG" );
    
    r_maxpolys = cvarSystem->Get( "r_maxpolys", va( "%d", MAX_POLYS ), 0, "Maximum number of polygons on screen" );
    r_maxpolyverts = cvarSystem->Get( "r_maxpolyverts", va( "%d", MAX_POLYVERTS ), 0, "Maximum number of vertices from polygons on screen" );
    
    // make sure all the commands added here are also
    // removed in R_Shutdown
    cmdSystem->AddCommand( "imagelist", R_ImageList_f, "Lists currently open images/textures used by the current map, also displays the amount of texture memory the map is using which is the last number displayed." );
    cmdSystem->AddCommand( "shaderlist", R_ShaderList_f, "List of currently open shaders (light effects)." );
    cmdSystem->AddCommand( "skinlist", R_SkinList_f, "List of currently open skins." );
    cmdSystem->AddCommand( "modellist", R_Modellist_f, "Lists all models in console" );
    cmdSystem->AddCommand( "modelist", R_ModeList_f, "Gives a list of the r_mode resolution numbers" );
    cmdSystem->AddCommand( "screenshot", R_ScreenShot_f, "Takes a screenshot, in high quality lossless TGA format" );
    cmdSystem->AddCommand( "screenshotJPEG", R_ScreenShotJPEG_f, "Takes a screenshot, in lossy-compression JPEG format" );
    cmdSystem->AddCommand( "gfxinfo", GfxInfo_f, "Reports current graphics rendering info, including: OpenGL extensions loaded, color depth, resolution, status of multithreaded support and the state of some rendering options." );
    //cmdSystem->AddCommand( "minimize", GLimp_Minimize , "Game window is minimized when set to non-zero");
    cmdSystem->AddCommand( "exportCubemaps", R_ExportCubemaps_f, "Exports maps Cubemaps as DDS files" );
}

void R_InitQueries( void )
{
    if( !glRefConfig.occlusionQuery )
        return;
        
    if( r_drawSunRays->integer )
        qglGenQueries( ARRAY_LEN( tr.sunFlareQuery ), tr.sunFlareQuery );
}

void R_ShutDownQueries( void )
{
    if( !glRefConfig.occlusionQuery )
        return;
        
    if( r_drawSunRays->integer )
        qglDeleteQueries( ARRAY_LEN( tr.sunFlareQuery ), tr.sunFlareQuery );
}

/*
===============
R_Init
===============
*/
void R_Init( void )
{
    sint	err;
    sint i;
    uchar8* ptr = nullptr;
    
    CL_RefPrintf( PRINT_ALL, "----- R_Init -----\n" );
    
    // clear all our internal state
    ::memset( &tr, 0, sizeof( tr ) );
    ::memset( &backEnd, 0, sizeof( backEnd ) );
    ::memset( &tess, 0, sizeof( tess ) );
    
//	Swap_Init();

    if( ( intptr_t )tess.xyz & 15 )
    {
        CL_RefPrintf( PRINT_WARNING, "tess.xyz not 16 uchar8 aligned\n" );
    }
    //::memset( tess.constantColor255, 255, sizeof( tess.constantColor255 ) );
    
    R_NoiseInit();
    
    //
    // init function tables
    //
    for( i = 0; i < FUNCTABLE_SIZE; i++ )
    {
        tr.sinTable[i]		= sin( DEG2RAD( i * 360.0f / ( ( float32 )( FUNCTABLE_SIZE - 1 ) ) ) );
        tr.squareTable[i]	= ( i < FUNCTABLE_SIZE / 2 ) ? 1.0f : -1.0f;
        tr.sawToothTable[i] = ( float32 )i / FUNCTABLE_SIZE;
        tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];
        
        if( i < FUNCTABLE_SIZE / 2 )
        {
            if( i < FUNCTABLE_SIZE / 4 )
            {
                tr.triangleTable[i] = ( float32 ) i / ( FUNCTABLE_SIZE / 4 );
            }
            else
            {
                tr.triangleTable[i] = 1.0f - tr.triangleTable[i - FUNCTABLE_SIZE / 4];
            }
        }
        else
        {
            tr.triangleTable[i] = -tr.triangleTable[i - FUNCTABLE_SIZE / 2];
        }
    }
    
    R_InitFogTable();
    
    R_Register();
    
    max_polys = r_maxpolys->integer;
    if( max_polys < MAX_POLYS )
        max_polys = MAX_POLYS;
        
    max_polyverts = r_maxpolyverts->integer;
    if( max_polyverts < MAX_POLYVERTS )
        max_polyverts = MAX_POLYVERTS;
        
    ptr = ( uchar8* )( Hunk_Alloc( sizeof( *backEndData ) + sizeof( srfPoly_t ) * max_polys + sizeof( polyVert_t ) * max_polyverts, h_low ) );
    backEndData = ( backEndData_t* )ptr;
    backEndData->polys = ( srfPoly_t* )( ( valueType* )( ptr ) + sizeof( *backEndData ) );
    backEndData->polyVerts = ( polyVert_t* )( ( valueType* )( ptr ) + sizeof( *backEndData ) + sizeof( srfPoly_t ) * max_polys );
    
    R_InitNextFrame();
    
    InitOpenGL();
    
    R_InitImages();
    
    if( glRefConfig.framebufferObject )
    {
        renderSystemLocal.FBOInit();
    }
    
    //Init GLSL
    renderSystemLocal.InitGPUShaders();
    
    R_InitVaos();
    
    R_InitShaders();
    
    R_InitSkins();
    
    R_ModelInit();
    
    R_InitFreeType();
    
    R_InitQueries();
    
    err = qglGetError();
    if( err != GL_NO_ERROR )
        CL_RefPrintf( PRINT_ALL, "glGetError() = 0x%x\n", err );
        
    // print info
    GfxInfo_f();
    CL_RefPrintf( PRINT_ALL, "----- finished R_Init -----\n" );
}

/*
===============
idRenderSystemLocal::Shutdown
===============
*/
void idRenderSystemLocal::Shutdown( bool destroyWindow )
{

    CL_RefPrintf( PRINT_ALL, "idRenderSystemLocal::Shutdown( %i )\n", destroyWindow );
    
    cmdSystem->RemoveCommand( "exportCubemaps" );
    cmdSystem->RemoveCommand( "gfxinfo" );
    cmdSystem->RemoveCommand( "gfxmeminfo" );
    cmdSystem->RemoveCommand( "imagelist" );
    cmdSystem->RemoveCommand( "minimize" );
    cmdSystem->RemoveCommand( "modellist" );
    cmdSystem->RemoveCommand( "screenshot" );
    cmdSystem->RemoveCommand( "screenshotJPEG" );
    cmdSystem->RemoveCommand( "shaderlist" );
    cmdSystem->RemoveCommand( "skinlist" );
    cmdSystem->RemoveCommand( "modelist" );
    
    if( tr.registered )
    {
        R_IssuePendingRenderCommands();
        R_ShutDownQueries();
        
        if( glRefConfig.framebufferObject )
        {
            FBOShutdown();
        }
        R_DeleteTextures();
        R_ShutdownVaos();
        //
        ShutdownGPUShaders();
    }
    
    R_DoneFreeType();
    
    // shut down platform specific OpenGL stuff
    if( destroyWindow )
    {
        GLimp_Shutdown();
        
        ::memset( &glConfig, 0, sizeof( glConfig ) );
        ::memset( &glState, 0, sizeof( glState ) );
        ::memset( &glRefConfig, 0, sizeof( glRefConfig ) );
        textureFilterAnisotropic = false;
        maxAnisotropy = 0;
        displayAspect = 0.0f;
    }
    
    tr.registered = false;
}


/*
=============
idRenderSystemLocal::EndRegistration

Touch all images to make sure they are resident
=============
*/
void idRenderSystemLocal::EndRegistration( void )
{
    R_IssuePendingRenderCommands();
    if( !idsystem->LowPhysicalMemory() )
    {
        RB_ShowImages();
    }
}
