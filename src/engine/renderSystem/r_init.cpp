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

#include <renderSystem/r_precompiled.hpp>

vidconfig_t glConfig;
glRefConfig_t glRefConfig;
bool textureFilterAnisotropic = false;
sint maxAnisotropy = 0;
float32 displayAspect = 0.0f;

glstate_t glState;

static void GfxInfo_f(void);
static void GfxMemInfo_f(void);

sint        max_polys;
sint        max_polyverts;

/*
** InitOpenGL
**
** This function is responsible for initializing a valid OpenGL subsystem.  This
** is done by calling GLimp_Init (which gives us a working OGL subsystem) then
** setting variables, checking GL constants, and reporting the gfx system config
** to the user.
*/
static void InitOpenGL(void) {
    //
    // initialize OS specific portions of the renderer
    //
    // GLimp_Init directly or indirectly references the following cvars:
    //      - r_fullscreen
    //      - r_mode
    //      - r_(color|depth|stencil)bits
    //      - r_ignorehwgamma
    //      - r_gamma
    //

    if(glConfig.vidWidth == 0) {
        sint        temp;

        memset(&glConfig, 0, sizeof(glConfig));

        GLimp_Init(false);
        GLimp_InitExtraExtensions();

        glConfig.textureEnvAddAvailable = true;

        // OpenGL driver constants
        qglGetIntegerv(GL_MAX_TEXTURE_SIZE, &temp);
        glConfig.maxTextureSize = temp;

        // stubbed or broken drivers may have reported 0...
        if(glConfig.maxTextureSize <= 0) {
            glConfig.maxTextureSize = 0;
        }

        qglGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &temp);
        glConfig.numTextureUnits = temp;

        // reserve 160 components for other uniforms
        qglGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &temp);
        glRefConfig.glslMaxAnimatedBones = Com_Clamp(0, IQM_MAX_JOINTS,
                                           (temp - 160) / 16);

        if(glRefConfig.glslMaxAnimatedBones < 12) {
            glRefConfig.glslMaxAnimatedBones = 0;
        }

        glConfig.smpActive = false;

        clientRendererSystem->RefPrintf(PRINT_DEVELOPER,
                                        "Trying SMP acceleration...\n");

        if(GLimp_SpawnRenderThread(RB_RenderThread)) {
            clientRendererSystem->RefPrintf(PRINT_DEVELOPER, "...succeeded.\n");
            glConfig.smpActive = true;
        } else {
            clientRendererSystem->RefPrintf(PRINT_DEVELOPER, "...failed.\n");
        }
    }

    // check for GLSL function textureCubeLod()
    if(r_cubeMapping->integer && !QGL_VERSION_ATLEAST(3, 0)) {
        clientRendererSystem->RefPrintf(PRINT_WARNING,
                                        "WARNING: Disabled r_cubeMapping because it requires OpenGL 3.0\n");
        cvarSystem->Set("r_cubeMapping", "0");
    }

    // set default state
    GL_SetDefaultState();
}

/*
==================
GL_CheckErrors
==================
*/
void GL_CheckErrs(pointer file, sint line) {
    sint        err;
    valueType   s[64];

    err = qglGetError();

    if(err == GL_NO_ERROR) {
        return;
    }

    if(r_ignoreGLErrors->integer) {
        return;
    }

    switch(err) {
        case GL_INVALID_ENUM:
            Q_strcpy_s(s, "GL_INVALID_ENUM");
            break;

        case GL_INVALID_VALUE:
            Q_strcpy_s(s, "GL_INVALID_VALUE");
            break;

        case GL_INVALID_OPERATION:
            Q_strcpy_s(s, "GL_INVALID_OPERATION");
            break;

        case GL_STACK_OVERFLOW:
            Q_strcpy_s(s, "GL_STACK_OVERFLOW");
            break;

        case GL_STACK_UNDERFLOW:
            Q_strcpy_s(s, "GL_STACK_UNDERFLOW");
            break;

        case GL_OUT_OF_MEMORY:
            Q_strcpy_s(s, "GL_OUT_OF_MEMORY");
            break;

        default:
            Q_vsprintf_s(s, sizeof(s), sizeof(s), "%i", err);
            break;
    }

    common->Error(ERR_FATAL, "GL_CheckErrors: %s in %s at line %d", s, file,
                  line);
}

/*
** R_GetModeInfo
*/
typedef struct vidmode_s {
    pointer description;
    sint        width, height;
    float32      pixelAspect; // pixel width / height
} vidmode_t;

//Dushan
static constexpr vidmode_t r_vidModes[] = {
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
static const sint s_numVidModes = ARRAY_LEN(r_vidModes);

bool R_GetModeInfo(sint *width, sint *height, float32 *windowAspect,
                   sint mode) {
    const vidmode_t *vm;

    if(mode < -2) {
        return false;
    }

    if(mode >= s_numVidModes) {
        return false;
    }

    if(mode == -2) {
        // Must set width and height to display size before calling this function!
        *windowAspect = static_cast<float32>(*width) / static_cast<float32>
                        (*height);
    } else if(mode == -1) {
        *width = r_customwidth->integer;
        *height = r_customheight->integer;
    } else {
        vm = &r_vidModes[ mode ];

        *width = vm->width;
        *height = vm->height;
        *windowAspect = static_cast<float32>(vm->width) / (vm->height *
                        vm->pixelAspect);
    }

    return true;
}

/*
** R_ModeList_f
*/
static void R_ModeList_f(void) {
    sint i;

    clientRendererSystem->RefPrintf(PRINT_ALL, "\n");

    for(i = 0; i < s_numVidModes; i++) {
        clientRendererSystem->RefPrintf(PRINT_ALL, "Mode %-2d: %s\n", i,
                                        r_vidModes[ i ].description);
    }

    clientRendererSystem->RefPrintf(PRINT_ALL, "\n");
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

Return value must be freed with memorySystem->FreeTempMemory()
==================
*/

uchar8 *RB_ReadPixels(sint x, sint y, sint width, sint height,
                      uint32 *offset, sint *padlen) {
    uchar8 *buffer = nullptr, *bufstart;
    sint padwidth, linelen;
    sint packAlign;

    qglGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);

    linelen = width * 3;
    padwidth = PAD(linelen, packAlign);

    // Allocate a few more bytes so that we can choose an alignment we like
    buffer = static_cast<uchar8 *>(memorySystem->AllocateTempMemory(
                                       padwidth * height + *offset + packAlign - 1));

    bufstart = static_cast<uchar8 *>(PADP(reinterpret_cast< sint64 >
                                          (buffer) + *offset, packAlign));

    qglReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, bufstart);

    *offset = bufstart - buffer;
    *padlen = padwidth - linelen;

    return buffer;
}

/*
==================
RB_TakeScreenshot
==================
*/
void RB_TakeScreenshot(sint x, sint y, sint width, sint height,
                       valueType *fileName) {
    uchar8 *allbuf, *buffer;
    uchar8 *srcptr, *destptr;
    uchar8 *endline, *endmem;
    uchar8 temp;

    sint linelen, padlen;
    uint32 offset = 18, memcount;

    allbuf = RB_ReadPixels(x, y, width, height, &offset, &padlen);
    buffer = allbuf + offset - 18;

    ::memset(buffer, 0, 18);
    buffer[2] = 2;      // uncompressed type
    buffer[12] = width & 255;
    buffer[13] = width >> 8;
    buffer[14] = height & 255;
    buffer[15] = height >> 8;
    buffer[16] = 24;    // pixel size

    // swap rgb to bgr and remove padding from line endings
    linelen = width * 3;

    srcptr = destptr = allbuf + offset;
    endmem = srcptr + (linelen + padlen) * height;

    while(srcptr < endmem) {
        endline = srcptr + linelen;

        while(srcptr < endline) {
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

    fileSystem->WriteFile(fileName, buffer, memcount + 18);

    memorySystem->FreeTempMemory(allbuf);
}

/*
==================
RB_TakeScreenshotJPEG
==================
*/

void RB_TakeScreenshotJPEG(sint x, sint y, sint width, sint height,
                           valueType *fileName) {
    uchar8 *buffer;
    uint32 offset = 0, memcount;
    sint padlen;

    buffer = RB_ReadPixels(x, y, width, height, &offset, &padlen);
    memcount = (width * 3 + padlen) * height;

    RE_SaveJPG(fileName, r_screenshotJpegQuality->integer, width, height,
               buffer + offset, padlen);
    memorySystem->FreeTempMemory(buffer);
}

/*
==================
RB_TakeScreenshotCmd
==================
*/
const void *RB_TakeScreenshotCmd(const void *data) {
    const screenshotCommand_t  *cmd;

    cmd = (const screenshotCommand_t *)data;

    // finish any 2D drawing if needed
    if(tess.numIndexes) {
        RB_EndSurface();
    }

    if(cmd->jpeg) {
        RB_TakeScreenshotJPEG(cmd->x, cmd->y, cmd->width, cmd->height,
                              cmd->fileName);
    } else {
        RB_TakeScreenshot(cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);
    }

    return (const void *)(cmd + 1);
}

/*
==================
R_TakeScreenshot
==================
*/
void R_TakeScreenshot(sint x, sint y, sint width, sint height,
                      valueType *name, bool jpeg) {
    static valueType
    fileName[MAX_OSPATH]; // bad things if two screenshots per frame?
    screenshotCommand_t *cmd = nullptr;

    cmd = GetCommandBuffer(sizeof * cmd, cmd);

    if(!cmd) {
        return;
    }

    cmd->commandId = RC_SCREENSHOT;

    cmd->x = x;
    cmd->y = y;
    cmd->width = width;
    cmd->height = height;
    Q_strncpyz(fileName, name, sizeof(fileName));
    cmd->fileName = fileName;
    cmd->jpeg = jpeg;
}

/*
==================
R_ScreenshotFilename
==================
*/
void R_ScreenshotFilename(sint lastNumber, valueType *fileName) {
    sint        a, b, c, d;

    if(lastNumber < 0 || lastNumber > 9999) {
        Q_vsprintf_s(fileName, MAX_OSPATH, MAX_OSPATH, "screenshots/shot9999.tga");
        return;
    }

    a = lastNumber / 1000;
    lastNumber -= a * 1000;
    b = lastNumber / 100;
    lastNumber -= b * 100;
    c = lastNumber / 10;
    lastNumber -= c * 10;
    d = lastNumber;

    Q_vsprintf_s(fileName, MAX_OSPATH, MAX_OSPATH,
                 "screenshots/shot%i%i%i%i.tga", a, b, c, d);
}

/*
==================
R_ScreenshotFilename
==================
*/
void R_ScreenshotFilenameJPEG(sint lastNumber, valueType *fileName) {
    sint        a, b, c, d;

    if(lastNumber < 0 || lastNumber > 9999) {
        Q_vsprintf_s(fileName, MAX_OSPATH, MAX_OSPATH, "screenshots/shot9999.jpg");
        return;
    }

    a = lastNumber / 1000;
    lastNumber -= a * 1000;
    b = lastNumber / 100;
    lastNumber -= b * 100;
    c = lastNumber / 10;
    lastNumber -= c * 10;
    d = lastNumber;

    Q_vsprintf_s(fileName, MAX_OSPATH, MAX_OSPATH,
                 "screenshots/shot%i%i%i%i.jpg", a, b, c, d);
}

/*
====================
R_LevelShot

levelshots are specialized 128*128 thumbnails for
the menu system, sampled down from full screen distorted images
====================
*/
void R_LevelShot(void) {
    valueType       checkname[MAX_OSPATH];
    uchar8     *buffer = nullptr;
    uchar8     *source, *allsource;
    uchar8     *src, *dst;
    uint32          offset = 0;
    sint            padlen;
    sint            x, y;
    sint            r, g, b;
    float32     xScale, yScale;
    sint            xx, yy;

    Q_vsprintf_s(checkname, sizeof(checkname), sizeof(checkname),
                 "levelshots/%s.tga", tr.world->baseName);

    allsource = RB_ReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight,
                              &offset, &padlen);
    source = allsource + offset;

    buffer = static_cast<uchar8 *>(memorySystem->AllocateTempMemory(
                                       128 * 128 * 3 + 18));
    ::memset(buffer, 0, 18);
    buffer[2] = 2;      // uncompressed type
    buffer[12] = 128;
    buffer[14] = 128;
    buffer[16] = 24;    // pixel size

    // resample from source
    xScale = glConfig.vidWidth / 512.0f;
    yScale = glConfig.vidHeight / 384.0f;

    for(y = 0 ; y < 128 ; y++) {
        for(x = 0 ; x < 128 ; x++) {
            r = g = b = 0;

            for(yy = 0 ; yy < 3 ; yy++) {
                for(xx = 0 ; xx < 4 ; xx++) {
                    src = source + (3 * glConfig.vidWidth + padlen) * static_cast<sint>((
                                y * 3 + yy) * yScale) +
                          3 * static_cast<sint>((x * 4 + xx) * xScale);
                    r += src[0];
                    g += src[1];
                    b += src[2];
                }
            }

            dst = buffer + 18 + 3 * (y * 128 + x);
            dst[0] = b / 12;
            dst[1] = g / 12;
            dst[2] = r / 12;
        }
    }

    fileSystem->WriteFile(checkname, buffer, 128 * 128 * 3 + 18);

    memorySystem->FreeTempMemory(buffer);
    memorySystem->FreeTempMemory(allsource);

    clientRendererSystem->RefPrintf(PRINT_ALL, "Wrote %s\n", checkname);
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
void R_ScreenShot_f(void) {
    valueType   checkname[MAX_OSPATH];
    static  sint    lastNumber = -1;
    bool    silent;

    if(!strcmp(cmdSystem->Argv(1), "levelshot")) {
        R_LevelShot();
        return;
    }

    if(!strcmp(cmdSystem->Argv(1), "silent")) {
        silent = true;
    } else {
        silent = false;
    }

    if(cmdSystem->Argc() == 2 && !silent) {
        // explicit filename
        Q_vsprintf_s(checkname, MAX_OSPATH, MAX_OSPATH, "screenshots/%s.tga",
                     cmdSystem->Argv(1));
    } else {
        // scan for a free filename

        // if we have saved a previous screenshot, don't scan
        // again, because recording demo avis can involve
        // thousands of shots
        if(lastNumber == -1) {
            lastNumber = 0;
        }

        // scan for a free number
        for(; lastNumber <= 9999 ; lastNumber++) {
            R_ScreenshotFilename(lastNumber, checkname);

            if(!fileSystem->FileExists(checkname)) {
                break; // file doesn't exist
            }
        }

        if(lastNumber >= 9999) {
            clientRendererSystem->RefPrintf(PRINT_ALL,
                                            "ScreenShot: Couldn't create a file\n");
            return;
        }

        lastNumber++;
    }

    R_TakeScreenshot(0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname,
                     false);

    if(!silent) {
        clientRendererSystem->RefPrintf(PRINT_ALL, "Wrote %s\n", checkname);
    }
}

void R_ScreenShotJPEG_f(void) {
    valueType       checkname[MAX_OSPATH];
    static  sint    lastNumber = -1;
    bool    silent;

    if(!strcmp(cmdSystem->Argv(1), "levelshot")) {
        R_LevelShot();
        return;
    }

    if(!strcmp(cmdSystem->Argv(1), "silent")) {
        silent = true;
    } else {
        silent = false;
    }

    if(cmdSystem->Argc() == 2 && !silent) {
        // explicit filename
        Q_vsprintf_s(checkname, MAX_OSPATH, MAX_OSPATH, "screenshots/%s.jpg",
                     cmdSystem->Argv(1));
    } else {
        // scan for a free filename

        // if we have saved a previous screenshot, don't scan
        // again, because recording demo avis can involve
        // thousands of shots
        if(lastNumber == -1) {
            lastNumber = 0;
        }

        // scan for a free number
        for(; lastNumber <= 9999 ; lastNumber++) {
            R_ScreenshotFilenameJPEG(lastNumber, checkname);

            if(!fileSystem->FileExists(checkname)) {
                break; // file doesn't exist
            }
        }

        if(lastNumber == 10000) {
            clientRendererSystem->RefPrintf(PRINT_ALL,
                                            "ScreenShot: Couldn't create a file\n");
            return;
        }

        lastNumber++;
    }

    R_TakeScreenshot(0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname,
                     true);

    if(!silent) {
        clientRendererSystem->RefPrintf(PRINT_ALL, "Wrote %s\n", checkname);
    }
}

//============================================================================

/*
==================
R_ExportCubemaps
==================
*/
void R_ExportCubemaps(void) {
    exportCubemapsCommand_t *cmd = nullptr;

    cmd = GetCommandBuffer(sizeof * cmd, cmd);

    if(!cmd) {
        return;
    }

    cmd->commandId = RC_EXPORT_CUBEMAPS;
}


/*
==================
R_ExportCubemaps_f
==================
*/
void R_ExportCubemaps_f(void) {
    R_ExportCubemaps();
}

//============================================================================

/*
==================
RB_TakeVideoFrameCmd
==================
*/
const void *RB_TakeVideoFrameCmd(const void *data) {
    const videoFrameCommand_t  *cmd;
    uchar8             *cBuf;
    uint64              memcount, linelen;
    uint64              padwidth, avipadwidth, padlen, avipadlen;
    sint packAlign;

    // finish any 2D drawing if needed
    if(tess.numIndexes) {
        RB_EndSurface();
    }

    cmd = (const videoFrameCommand_t *)data;

    qglGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);

    linelen = cmd->width * 3;

    // Alignment stuff for qglReadPixels
    padwidth = PAD(linelen, packAlign);
    padlen = padwidth - linelen;
    // AVI line padding
    avipadwidth = PAD(linelen, AVI_LINE_PADDING);
    avipadlen = avipadwidth - linelen;

    cBuf = static_cast<uchar8 *>(PADP(cmd->captureBuffer, packAlign));

    qglReadPixels(0, 0, cmd->width, cmd->height, GL_RGB,
                  GL_UNSIGNED_BYTE, cBuf);

    memcount = padwidth * cmd->height;

    if(cmd->motionJpeg) {
        memcount = RE_SaveJPGToBuffer(cmd->encodeBuffer, linelen * cmd->height,
                                      r_aviMotionJpegQuality->integer,
                                      cmd->width, cmd->height, cBuf, padlen);
        clientAVISystem->WriteAVIVideoFrame(cmd->encodeBuffer, memcount);
    } else {
        uchar8 *lineend, *memend;
        uchar8 *srcptr, *destptr;

        srcptr = cBuf;
        destptr = cmd->encodeBuffer;
        memend = srcptr + memcount;

        // swap R and B and remove line paddings
        while(srcptr < memend) {
            lineend = srcptr + linelen;

            while(srcptr < lineend) {
                *destptr++ = srcptr[2];
                *destptr++ = srcptr[1];
                *destptr++ = srcptr[0];
                srcptr += 3;
            }

            ::memset(destptr, '\0', avipadlen);
            destptr += avipadlen;

            srcptr += padlen;
        }

        clientAVISystem->WriteAVIVideoFrame(cmd->encodeBuffer,
                                            avipadwidth * cmd->height);
    }

    return (const void *)(cmd + 1);
}

//============================================================================

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState(void) {
    qglClearDepth(1.0f);

    qglCullFace(GL_FRONT);

    GL_BindNullTextures();

    if(glRefConfig.framebufferObject) {
        GL_BindNullFramebuffers();
    }

    GL_TextureMode(r_textureMode->string);

    //qglShadeModel( GL_SMOOTH );
    qglDepthFunc(GL_LEQUAL);

    //
    // make sure our GL state vector is set correctly
    //
    glState.glStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;
    glState.storedGlState = 0;
    glState.faceCulling = CT_TWO_SIDED;
    glState.faceCullFront = true;

    GL_BindNullProgram();

    if(glRefConfig.vertexArrayObject) {
        qglBindVertexArray(0);
    }

    qglBindBuffer(GL_ARRAY_BUFFER, 0);
    qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glState.currentVao = nullptr;
    glState.vertexAttribsEnabled = 0;

    qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    qglDepthMask(GL_TRUE);
    qglDisable(GL_DEPTH_TEST);
    qglEnable(GL_SCISSOR_TEST);
    qglDisable(GL_CULL_FACE);
    qglDisable(GL_BLEND);

    if(glRefConfig.seamlessCubeMap) {
        qglEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    }

    // GL_POLYGON_OFFSET_FILL will be glEnable()d when this is used
    qglPolygonOffset(r_offsetFactor->value, r_offsetUnits->value);

    qglClearColor(0.0f, 0.0f, 0.0f, 1.0f);   // FIXME: get color of sky
}

/*
================
R_PrintLongString

Workaround for clientRendererSystem->RefPrintf's 1024 characters buffer limit.
================
*/
void R_PrintLongString(pointer string) {
    valueType buffer[1024];
    pointer p;
    sint size = strlen(string);

    p = string;

    while(p < &string[size]) {
        Q_strncpyz(buffer, p, sizeof(buffer));
        clientRendererSystem->RefPrintf(PRINT_ALL, "%s", buffer);
        p += 1023;
        size -= 1023;
    }
}

/*
================
GfxInfo_f
================
*/
void GfxInfo_f(void) {
    pointer enablestrings[] = {
        "disabled",
        "enabled"
    };
    pointer fsstrings[] = {
        "windowed",
        "fullscreen"
    };

    clientRendererSystem->RefPrintf(PRINT_ALL, "GL_VENDOR: %s\n",
                                    glConfig.vendor_string);
    clientRendererSystem->RefPrintf(PRINT_ALL, "GL_RENDERER: %s\n",
                                    glConfig.renderer_string);
    clientRendererSystem->RefPrintf(PRINT_ALL, "GL_VERSION: %s\n",
                                    glConfig.version_string);

#ifdef _DEBUG

    if(qglGetStringi) {
        GLint numExtensions;
        sint i;

        qglGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

        for(i = 0; i < numExtensions; i++) {
            clientRendererSystem->RefPrintf(PRINT_ALL, "%s ",
                                            qglGetStringi(GL_EXTENSIONS, i));
        }
    } else {
        R_PrintLongString(glConfig.extensions_string);
    }

#endif
    clientRendererSystem->RefPrintf(PRINT_ALL, "GL_MAX_TEXTURE_SIZE: %d\n",
                                    glConfig.maxTextureSize);
    clientRendererSystem->RefPrintf(PRINT_ALL,
                                    "GL_MAX_TEXTURE_IMAGE_UNITS: %d\n",
                                    glConfig.numTextureUnits);
    clientRendererSystem->RefPrintf(PRINT_ALL,
                                    "PIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n",
                                    glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits);
    clientRendererSystem->RefPrintf(PRINT_ALL, "MODE: %d, %d x %d %s hz:",
                                    r_mode->integer,
                                    glConfig.vidWidth, glConfig.vidHeight,
                                    fsstrings[r_fullscreen->integer == 1]);

    if(glConfig.displayFrequency) {
        clientRendererSystem->RefPrintf(PRINT_ALL, "%d\n",
                                        glConfig.displayFrequency);
    } else {
        clientRendererSystem->RefPrintf(PRINT_ALL, "N/A\n");
    }

    clientRendererSystem->RefPrintf(PRINT_ALL, "Display Scale: %d%%\n",
                                    (sint)glConfig.displayScale * 100);

    if(glConfig.deviceSupportsGamma) {
        clientRendererSystem->RefPrintf(PRINT_ALL,
                                        "GAMMA: hardware w/ %d overbright bits\n",
                                        tr.overbrightBits);
    } else {
        clientRendererSystem->RefPrintf(PRINT_ALL,
                                        "GAMMA: software w/ %d overbright bits\n",
                                        tr.overbrightBits);
    }

    clientRendererSystem->RefPrintf(PRINT_ALL, "texturemode: %s\n",
                                    r_textureMode->string);
    clientRendererSystem->RefPrintf(PRINT_ALL, "picmip: %d\n",
                                    r_picmip->integer);
    clientRendererSystem->RefPrintf(PRINT_ALL, "texture bits: %d\n",
                                    r_texturebits->integer);
    clientRendererSystem->RefPrintf(PRINT_ALL, "compiled vertex arrays: %s\n",
                                    enablestrings[qglLockArraysEXT != 0 ]);
    clientRendererSystem->RefPrintf(PRINT_ALL, "texenv add: %s\n",
                                    enablestrings[glConfig.textureEnvAddAvailable != 0]);
    clientRendererSystem->RefPrintf(PRINT_ALL, "compressed textures: %s\n",
                                    enablestrings[glConfig.textureCompression != TC_NONE]);

    if(r_vertexLight->integer) {
        clientRendererSystem->RefPrintf(PRINT_ALL,
                                        "HACK: using vertex lightmap approximation\n");
    }

    if(r_finish->integer) {
        clientRendererSystem->RefPrintf(PRINT_ALL, "Forcing glFinish\n");
    }
}

/*
===============
R_Register
===============
*/
void R_Register(void) {

    R_InitConsoleVars();

    // make sure all the commands added here are also
    // removed in R_Shutdown
    cmdSystem->AddCommand("imagelist", R_ImageList_f,
                          "Lists currently open images/textures used by the current map, also displays the amount of texture memory the map is using which is the last number displayed.");
    cmdSystem->AddCommand("shaderlist", R_ShaderList_f,
                          "List of currently open shaders (light effects).");
    cmdSystem->AddCommand("skinlist", R_SkinList_f,
                          "List of currently open skins.");
    cmdSystem->AddCommand("modellist", R_Modellist_f,
                          "Lists all models in console");
    cmdSystem->AddCommand("modelist", R_ModeList_f,
                          "Gives a list of the r_mode resolution numbers");
    cmdSystem->AddCommand("screenshot", R_ScreenShot_f,
                          "Takes a screenshot, in high quality lossless TGA format");
    cmdSystem->AddCommand("screenshotJPEG", R_ScreenShotJPEG_f,
                          "Takes a screenshot, in lossy-compression JPEG format");
    cmdSystem->AddCommand("gfxinfo", GfxInfo_f,
                          "Reports current graphics rendering info, including: OpenGL extensions loaded, color depth, resolution, status of multithreaded support and the state of some rendering options.");
    //cmdSystem->AddCommand( "minimize", GLimp_Minimize , "Game window is minimized when set to non-zero");
    cmdSystem->AddCommand("exportCubemaps", R_ExportCubemaps_f,
                          "Exports maps Cubemaps as DDS files");
}

void R_InitQueries(void) {
    if(!glRefConfig.occlusionQuery) {
        return;
    }

    if(r_drawSunRays->integer) {
        qglGenQueries(ARRAY_LEN(tr.sunFlareQuery), tr.sunFlareQuery);
    }
}

void R_ShutDownQueries(void) {
    if(!glRefConfig.occlusionQuery) {
        return;
    }

    if(r_drawSunRays->integer) {
        qglDeleteQueries(ARRAY_LEN(tr.sunFlareQuery), tr.sunFlareQuery);
    }
}

/*
===============
R_Init
===============
*/
void R_Init(void) {
    sint    err;
    sint i;
    uchar8 *ptr = nullptr;

    clientRendererSystem->RefPrintf(PRINT_ALL, "----- R_Init -----\n");

    // clear all our internal state
    ::memset(&tr, 0, sizeof(tr));
    ::memset(&backEnd, 0, sizeof(backEnd));
    ::memset(&tess, 0, sizeof(tess));

    //  Swap_Init();

    if(reinterpret_cast<sint64>(tess.xyz) & 15) {
        clientRendererSystem->RefPrintf(PRINT_WARNING,
                                        "tess.xyz not 16 byte aligned\n");
    }

    //::memset( tess.constantColor255, 255, sizeof( tess.constantColor255 ) );

    R_NoiseInit();

    //
    // init function tables
    //
    for(i = 0; i < FUNCTABLE_SIZE; i++) {
        tr.sinTable[i]      = sin(DEG2RAD(i * 360.0f / (static_cast<float32>
                                          (FUNCTABLE_SIZE - 1))));
        tr.squareTable[i]   = (i < FUNCTABLE_SIZE / 2) ? 1.0f : -1.0f;
        tr.sawToothTable[i] = static_cast<float32>(i) / FUNCTABLE_SIZE;
        tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];

        if(i < FUNCTABLE_SIZE / 2) {
            if(i < FUNCTABLE_SIZE / 4) {
                tr.triangleTable[i] = static_cast<float32>(i) / (FUNCTABLE_SIZE / 4);
            } else {
                tr.triangleTable[i] = 1.0f - tr.triangleTable[i - FUNCTABLE_SIZE / 4];
            }
        } else {
            tr.triangleTable[i] = -tr.triangleTable[i - FUNCTABLE_SIZE / 2];
        }
    }

    R_InitFogTable();

    R_Register();

    max_polys = r_maxpolys->integer;

    if(max_polys < MAX_POLYS) {
        max_polys = MAX_POLYS;
    }

    max_polyverts = r_maxpolyverts->integer;

    if(max_polyverts < MAX_POLYVERTS) {
        max_polyverts = MAX_POLYVERTS;
    }

    ptr = static_cast<uchar8 *>(memorySystem->Alloc(sizeof(
                                    *backEndData) + sizeof(
                                    srfPoly_t) * max_polys + sizeof(polyVert_t) * max_polyverts, h_low));
    backEndData = (backEndData_t *)ptr;
    backEndData->polys = (srfPoly_t *)(reinterpret_cast< valueType *>
                                       (ptr) + sizeof(*backEndData));
    backEndData->polyVerts = (polyVert_t *)(reinterpret_cast<valueType *>
                                            (ptr) + sizeof(*backEndData) + sizeof(srfPoly_t) * max_polys);

    R_InitNextFrame();

    InitOpenGL();

    R_InitImages();

    if(glRefConfig.framebufferObject) {
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

    if(err != GL_NO_ERROR) {
        clientRendererSystem->RefPrintf(PRINT_ALL, "glGetError() = 0x%x\n", err);
    }

    // print info
    if(!r_verbose) {
        GfxInfo_f();
    }

    clientRendererSystem->RefPrintf(PRINT_ALL,
                                    "----- finished R_Init -----\n");
}

/*
===============
idRenderSystemLocal::Shutdown
===============
*/
void idRenderSystemLocal::Shutdown(bool destroyWindow) {

    clientRendererSystem->RefPrintf(PRINT_ALL,
                                    "idRenderSystemLocal::Shutdown( %i )\n",
                                    destroyWindow);

    cmdSystem->RemoveCommand("exportCubemaps");
    cmdSystem->RemoveCommand("gfxinfo");
    cmdSystem->RemoveCommand("gfxmeminfo");
    cmdSystem->RemoveCommand("imagelist");
    cmdSystem->RemoveCommand("minimize");
    cmdSystem->RemoveCommand("modellist");
    cmdSystem->RemoveCommand("screenshot");
    cmdSystem->RemoveCommand("screenshotJPEG");
    cmdSystem->RemoveCommand("shaderlist");
    cmdSystem->RemoveCommand("skinlist");
    cmdSystem->RemoveCommand("modelist");

    if(tr.registered) {
        R_IssuePendingRenderCommands();
        R_ShutDownQueries();

        if(glRefConfig.framebufferObject) {
            FBOShutdown();
        }

        R_DeleteTextures();
        R_ShutdownVaos();
        //
        ShutdownGPUShaders();
    }

    R_DoneFreeType();

    // shut down platform specific OpenGL stuff
    if(destroyWindow) {
        R_IssuePendingRenderCommands();
        R_ShutdownCommandBuffers();
        GLimp_Shutdown();

        ::memset(&glConfig, 0, sizeof(glConfig));
        ::memset(&glState, 0, sizeof(glState));
        ::memset(&glRefConfig, 0, sizeof(glRefConfig));
        textureFilterAnisotropic = false;
        maxAnisotropy = 0;
        displayAspect = 0.0f;
    }

    clientRendererSystem->RefTagFree();

    tr.registered = false;
}


/*
=============
idRenderSystemLocal::EndRegistration

Touch all images to make sure they are resident
=============
*/
void idRenderSystemLocal::EndRegistration(void) {
    R_IssuePendingRenderCommands();

    if(!idsystem->LowPhysicalMemory()) {
        RB_ShowImages();
    }
}
