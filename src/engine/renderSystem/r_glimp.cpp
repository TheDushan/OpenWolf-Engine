////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2005 id Software, Inc.
// Copyright(C) 2011 - 2023 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   r_glimp.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.hpp>

sint qglMajorVersion, qglMinorVersion;
sint qglesMajorVersion, qglesMinorVersion;

enum rserr_t {
    RSERR_OK,

    RSERR_INVALID_FULLSCREEN,
    RSERR_INVALID_MODE,

    RSERR_UNKNOWN
};

SDL_Window *SDL_window = nullptr;
static SDL_GLContext SDL_glContext = nullptr;

void (APIENTRYP qglActiveTextureARB)(uint texture);
void (APIENTRYP qglClientActiveTextureARB)(uint texture);
void (APIENTRYP qglMultiTexCoord2fARB)(uint target, float32 s, float32 t);
void (APIENTRYP qglLockArraysEXT)(sint first, sint count);
void (APIENTRYP qglUnlockArraysEXT)(void);

#define GLE(ret, name, ...) name##proc * qgl##name;
QGL_1_1_PROCS;
QGL_1_1_FIXED_FUNCTION_PROCS;
QGL_DESKTOP_1_1_PROCS;
QGL_DESKTOP_1_1_FIXED_FUNCTION_PROCS;
QGL_ES_1_1_PROCS;
QGL_ES_1_1_FIXED_FUNCTION_PROCS;
QGL_1_3_PROCS;
QGL_1_5_PROCS;
QGL_2_0_PROCS;
QGL_3_0_PROCS;
QGL_4_0_PROCS;
QGL_ARB_occlusion_query_PROCS;
QGL_ARB_framebuffer_object_PROCS;
QGL_ARB_vertex_array_object_PROCS;
QGL_EXT_direct_state_access_PROCS;
#undef GLE

/*
===============
GLimp_Minimize

Minimize the game so that user is back at the desktop
===============
*/
void GLimp_Minimize(void) {
    SDL_MinimizeWindow(SDL_window);
}


/*
===============
GLimp_LogComment
===============
*/
void GLimp_LogComment(pointer comment) {
}

/*
===============
GLimp_CompareModes
===============
*/
static sint GLimp_CompareModes(const void *a, const void *b) {
    const float32 ASPECT_EPSILON = 0.001f;
    SDL_Rect *modeA = (SDL_Rect *)a;
    SDL_Rect *modeB = (SDL_Rect *)b;
    float32 aspectA = static_cast<float32>(modeA->w) / static_cast<float32>
                      (modeA->h);
    float32 aspectB = static_cast<float32>(modeB->w) / static_cast<float32>
                      (modeB->h);
    sint areaA = modeA->w * modeA->h;
    sint areaB = modeB->w * modeB->h;
    float32 aspectDiffA = fabs(aspectA - displayAspect);
    float32 aspectDiffB = fabs(aspectB - displayAspect);
    float32 aspectDiffsDiff = aspectDiffA - aspectDiffB;

    if(aspectDiffsDiff > ASPECT_EPSILON) {
        return 1;
    } else if(aspectDiffsDiff < -ASPECT_EPSILON) {
        return -1;
    } else {
        return areaA - areaB;
    }
}

/*
===============
GLimp_DetectAvailableModes
===============
*/
static void GLimp_DetectAvailableModes(void) {
    sint i, j;
    valueType buf[ MAX_STRING_CHARS ] = { 0 };
    sint numSDLModes;
    SDL_Rect *modes;
    sint numModes = 0;

    SDL_DisplayMode windowMode;
    sint display = SDL_GetWindowDisplayIndex(SDL_window);

    if(display < 0) {
        clientRendererSystem->RefPrintf(PRINT_WARNING,
                                        "Couldn't get window display index, no resolutions detected: %s\n",
                                        SDL_GetError());
        return;
    }

    numSDLModes = SDL_GetNumDisplayModes(display);

    if(SDL_GetWindowDisplayMode(SDL_window, &windowMode) < 0 ||
            numSDLModes <= 0) {
        clientRendererSystem->RefPrintf(PRINT_WARNING,
                                        "Couldn't get window display mode, no resolutions detected: %s\n",
                                        SDL_GetError());
        return;
    }

    modes = (SDL_Rect *)SDL_calloc(static_cast<uint64>(numSDLModes),
                                   sizeof(SDL_Rect));

    if(!modes) {
        common->Error(ERR_FATAL, "Out of memory");
    }

    for(i = 0; i < numSDLModes; i++) {
        SDL_DisplayMode mode;

        if(SDL_GetDisplayMode(display, i, &mode) < 0) {
            continue;
        }

        if(!mode.w || !mode.h) {
            clientRendererSystem->RefPrintf(PRINT_ALL,
                                            "Display supports any resolution\n");
            SDL_free(modes);
            return;
        }

        if(windowMode.format != mode.format) {
            continue;
        }

        // SDL can give the same resolution with different refresh rates.
        // Only list resolution once.
        for(j = 0; j < numModes; j++) {
            if(mode.w == modes[ j ].w && mode.h == modes[ j ].h) {
                break;
            }
        }

        if(j != numModes) {
            continue;
        }

        modes[ numModes ].w = mode.w;
        modes[ numModes ].h = mode.h;
        numModes++;
    }

    if(numModes > 1) {
        qsort(modes, numModes, sizeof(SDL_Rect), GLimp_CompareModes);
    }

    for(i = 0; i < numModes; i++) {
        pointer newModeString = va(nullptr, "%ux%u ", modes[ i ].w, modes[ i ].h);

        if(strlen(newModeString) < static_cast<sint>(sizeof(buf)) - strlen(buf)) {
            Q_strcat(buf, sizeof(buf), newModeString);
        } else {
            clientRendererSystem->RefPrintf(PRINT_WARNING,
                                            "Skipping mode %ux%u, buffer too small\n",
                                            modes[ i ].w, modes[ i ].h);
        }
    }

    if(*buf) {
        buf[ strlen(buf) - 1 ] = 0;
        clientRendererSystem->RefPrintf(PRINT_ALL, "Available modes: '%s'\n", buf);
        cvarSystem->Set("r_availableModes", buf);
    }

    SDL_free(modes);
}

/*
===============
GLimp_GetProcAddresses

Get addresses for OpenGL functions.
===============
*/
static bool GLimp_GetProcAddresses(bool fixedFunction) {
    bool success = true;
    pointer version;

#ifdef __SDL_NOGETPROCADDR__
#define GLE( ret, name, ... ) qgl##name = gl#name;
#else
#define GLE( ret, name, ... ) qgl##name = (name##proc *) SDL_GL_GetProcAddress("gl" #name); \
    if ( qgl##name == nullptr ) { \
        clientRendererSystem->RefPrintf( PRINT_ALL, "ERROR: Missing OpenGL function %s\n", "gl" #name ); \
        success = false; \
    }
#endif

    // OpenGL 1.0 and OpenGL ES 1.0
    GLE(const uchar8 *, GetString, uint name)

    if(!qglGetString) {
        common->Error(ERR_FATAL, "glGetString is nullptr");
    }

    version = reinterpret_cast<pointer>(qglGetString(GL_VERSION));

    if(!version) {
        common->Error(ERR_FATAL, "GL_VERSION is nullptr\n");
    }

    if(Q_stricmpn("OpenGL ES", version, 9) == 0) {
        valueType profile[6]; // ES, ES-CM, or ES-CL

        sscanf(version, "OpenGL %5s %d.%d", profile, &qglesMajorVersion,
               &qglesMinorVersion);

        // common lite profile (no floating point) is not supported
        if(Q_stricmp(profile, "ES-CL") == 0) {
            qglesMajorVersion = 0;
            qglesMinorVersion = 0;
        }
    } else {
        sscanf(version, "%d.%d", &qglMajorVersion, &qglMinorVersion);
    }

    if(fixedFunction) {
        if(QGL_VERSION_ATLEAST(1, 2)) {
            QGL_1_1_PROCS;
            QGL_1_1_FIXED_FUNCTION_PROCS;
            QGL_DESKTOP_1_1_PROCS;
            QGL_DESKTOP_1_1_FIXED_FUNCTION_PROCS;
        } else if(qglesMajorVersion == 1 && qglesMinorVersion >= 1) {
            // OpenGL ES 1.1 (2.0 is not backward compatible)
            QGL_1_1_PROCS;
            QGL_1_1_FIXED_FUNCTION_PROCS;
            QGL_ES_1_1_PROCS;
            QGL_ES_1_1_FIXED_FUNCTION_PROCS;
        } else {
            common->Error(ERR_FATAL,
                          "Unsupported OpenGL Version (%s), OpenGL 1.2 is required\n", version);
        }
    } else {
        if(QGL_VERSION_ATLEAST(2, 0)) {
            QGL_1_1_PROCS;
            QGL_DESKTOP_1_1_PROCS;
            QGL_1_3_PROCS;
            QGL_1_5_PROCS;
            QGL_2_0_PROCS;
        } else if(QGLES_VERSION_ATLEAST(2, 0)) {
            QGL_1_1_PROCS;
            QGL_ES_1_1_PROCS;
            QGL_1_3_PROCS;
            QGL_1_5_PROCS;
            QGL_2_0_PROCS;
            // error so this doesn't segfault due to NULL desktop GL functions being used
            common->Error(ERR_FATAL, "Unsupported OpenGL Version: %s\n", version);
        } else {
            common->Error(ERR_FATAL,
                          "Unsupported OpenGL Version (%s), OpenGL 2.0 is required\n", version);
        }
    }

    if(QGL_VERSION_ATLEAST(3, 0) || QGLES_VERSION_ATLEAST(3, 0)) {
        QGL_1_1_PROCS;
        QGL_DESKTOP_1_1_PROCS;
        QGL_1_3_PROCS;
        QGL_1_5_PROCS;
        QGL_2_0_PROCS;
        QGL_3_0_PROCS;
        QGL_ARB_occlusion_query_PROCS;
        QGL_ARB_framebuffer_object_PROCS;
        QGL_ARB_vertex_array_object_PROCS;
        QGL_EXT_direct_state_access_PROCS;
    }

    if(QGL_VERSION_ATLEAST(4, 0) || QGLES_VERSION_ATLEAST(4, 0)) {
        QGL_1_1_PROCS;
        QGL_DESKTOP_1_1_PROCS;
        QGL_1_3_PROCS;
        QGL_1_5_PROCS;
        QGL_2_0_PROCS;
        QGL_3_0_PROCS;
        QGL_4_0_PROCS;
        QGL_ARB_occlusion_query_PROCS;
        QGL_ARB_framebuffer_object_PROCS;
        QGL_ARB_vertex_array_object_PROCS;
        QGL_EXT_direct_state_access_PROCS;
    }

#undef GLE

    return success;
}

/*
===============
GLimp_ClearProcAddresses

Clear addresses for OpenGL functions.
===============
*/
static void GLimp_ClearProcAddresses(void) {
#define GLE( ret, name, ... ) qgl##name = nullptr;

    qglMajorVersion = 0;
    qglMinorVersion = 0;
    qglesMajorVersion = 0;
    qglesMinorVersion = 0;

    QGL_1_1_PROCS;
    QGL_1_1_FIXED_FUNCTION_PROCS;
    QGL_DESKTOP_1_1_PROCS;
    QGL_DESKTOP_1_1_FIXED_FUNCTION_PROCS;
    QGL_ES_1_1_PROCS;
    QGL_ES_1_1_FIXED_FUNCTION_PROCS;
    QGL_1_3_PROCS;
    QGL_1_5_PROCS;
    QGL_2_0_PROCS;
    QGL_3_0_PROCS;
    QGL_4_0_PROCS;
    QGL_ARB_occlusion_query_PROCS;
    QGL_ARB_framebuffer_object_PROCS;
    QGL_ARB_vertex_array_object_PROCS;
    QGL_EXT_direct_state_access_PROCS;

    qglActiveTextureARB = nullptr;
    qglClientActiveTextureARB = nullptr;
    qglMultiTexCoord2fARB = nullptr;

    qglLockArraysEXT = nullptr;
    qglUnlockArraysEXT = nullptr;

#undef GLE
}

/*
===============
GLimp_GetDPIScale
===============
*/
static float32 GLimp_GetDPIScale(sint display) {
    float32 scale = 1.0f;

    pointer driver = SDL_GetCurrentVideoDriver();

    if(!::strcmp(driver, "windows")) {
        float32 ddpi;

        if(!SDL_GetDisplayDPI(display, &ddpi, nullptr, nullptr)) {
            scale = ddpi / 96.0f;
        }
    } else if(!::strcmp(driver, "x11")) {
        float32 ddpi;

        if(!SDL_GetDisplayDPI(display, &ddpi, nullptr, nullptr)) {
            scale = ::roundf(ddpi / 96.0f);
        }
    }

    return scale;
}

/*
===============
GLimp_SetMode
===============
*/
static sint GLimp_SetMode(sint mode, bool fullscreen, bool noborder,
                          bool fixedFunction) {
    pointer glstring;
    sint perChannelColorBits;
    sint colorBits, depthBits, stencilBits;
    sint i = 0;
    SDL_Surface *icon = nullptr;
    uint flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
    SDL_DisplayMode desktopMode;
    sint display = 0;
    sint x = SDL_WINDOWPOS_UNDEFINED, y = SDL_WINDOWPOS_UNDEFINED;

    clientRendererSystem->RefPrintf(PRINT_ALL,
                                    "Initializing OpenGL display\n");

    if(r_allowResize->integer) {
        flags |= SDL_WINDOW_RESIZABLE;
    }

    // If a window exists, note its display index
    if(SDL_window != nullptr) {
        display = SDL_GetWindowDisplayIndex(SDL_window);

        if(display < 0) {
            clientRendererSystem->RefPrintf(PRINT_DEVELOPER,
                                            "SDL_GetWindowDisplayIndex() failed: %s\n",
                                            SDL_GetError());
        }
    }

    if(display >= 0 && SDL_GetDesktopDisplayMode(display, &desktopMode) == 0) {
        displayAspect = static_cast<float32>(desktopMode.w) / static_cast<float32>
                        (desktopMode.h);

        clientRendererSystem->RefPrintf(PRINT_ALL, "Display aspect: %.3f\n",
                                        displayAspect);
    } else {
        ::memset(&desktopMode, 0, sizeof(SDL_DisplayMode));

        clientRendererSystem->RefPrintf(PRINT_ALL,
                                        "Cannot determine display aspect, assuming 1.333\n");
    }

    clientRendererSystem->RefPrintf(PRINT_ALL, "...setting mode %d:", mode);

    if(mode == -2) {
        // use desktop video resolution
        if(desktopMode.h > 0) {
            glConfig.vidWidth = desktopMode.w;
            glConfig.vidHeight = desktopMode.h;
        } else {
            glConfig.vidWidth = 640;
            glConfig.vidHeight = 480;
            clientRendererSystem->RefPrintf(PRINT_ALL,
                                            "Cannot determine display resolution, assuming 640x480\n");
        }

        glConfig.windowAspect = static_cast<float32>(glConfig.vidWidth) /
                                static_cast<float32>(glConfig.vidHeight);
    } else if(!R_GetModeInfo(&glConfig.vidWidth, &glConfig.vidHeight,
                             &glConfig.windowAspect, mode)) {
        clientRendererSystem->RefPrintf(PRINT_ALL, " invalid mode\n");
        return RSERR_INVALID_MODE;
    }

    clientRendererSystem->RefPrintf(PRINT_ALL, " %d %d\n", glConfig.vidWidth,
                                    glConfig.vidHeight);

    // Center window
    if(r_centerWindow->integer && !fullscreen) {
        x = (desktopMode.w / 2) - (glConfig.vidWidth / 2);
        y = (desktopMode.h / 2) - (glConfig.vidHeight / 2);
    }

    // Destroy existing state if it exists
    if(SDL_glContext != nullptr) {
        GLimp_ClearProcAddresses();
        SDL_GL_DeleteContext(SDL_glContext);
        SDL_glContext = nullptr;
    }

    if(SDL_window != nullptr) {
        SDL_GetWindowPosition(SDL_window, &x, &y);
        clientRendererSystem->RefPrintf(PRINT_DEVELOPER,
                                        "Existing window at %dx%d before being destroyed\n", x, y);
        SDL_DestroyWindow(SDL_window);
        SDL_window = nullptr;
    }

    if(fullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN;
        glConfig.isFullscreen = true;
    } else {
        if(noborder) {
            flags |= SDL_WINDOW_BORDERLESS;
        }

        glConfig.isFullscreen = false;
    }

    colorBits = r_colorbits->value;

    if((!colorBits) || (colorBits >= 32)) {
        colorBits = 24;
    }

    if(!r_depthbits->value) {
        depthBits = 24;
    } else {
        depthBits = r_depthbits->value;
    }

    stencilBits = r_stencilbits->value;

    for(i = 0; i < 16; i++) {
        sint testColorBits, testDepthBits, testStencilBits;
        sint realColorBits[3];

        // 0 - default
        // 1 - minus colorBits
        // 2 - minus depthBits
        // 3 - minus stencil
        if((i % 4) == 0 && i) {
            // one pass, reduce
            switch(i / 4) {
                case 2:
                    if(colorBits == 24) {
                        colorBits = 16;
                    }

                    break;

                case 1:
                    if(depthBits == 32) {
                        depthBits = 24;
                    } else if(depthBits == 24) {
                        depthBits = 16;
                    } else if(depthBits == 16) {
                        depthBits = 8;
                    }

                case 3:
                    if(stencilBits == 24) {
                        stencilBits = 16;
                    } else if(stencilBits == 16) {
                        stencilBits = 8;
                    }
            }
        }

        testColorBits = colorBits;
        testDepthBits = depthBits;
        testStencilBits = stencilBits;

        if((i % 4) == 3) {
            // reduce colorBits
            if(testColorBits == 24) {
                testColorBits = 16;
            }
        }

        if((i % 4) == 2) {
            // reduce depthBits
            if(testDepthBits == 24) {
                testDepthBits = 16;
            } else if(testDepthBits == 16) {
                testDepthBits = 8;
            }
        }

        if((i % 4) == 1) {
            // reduce stencilBits
            if(testStencilBits == 24) {
                testStencilBits = 16;
            } else if(testStencilBits == 16) {
                testStencilBits = 8;
            } else {
                testStencilBits = 0;
            }
        }

        if(testColorBits == 24) {
            perChannelColorBits = 8;
        } else {
            perChannelColorBits = 4;
        }

        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, perChannelColorBits);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, perChannelColorBits);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, perChannelColorBits);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, testDepthBits);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, testStencilBits);

        if(r_stereoEnabled->integer) {
            glConfig.stereoEnabled = true;
            SDL_GL_SetAttribute(SDL_GL_STEREO, 1);
        } else {
            glConfig.stereoEnabled = false;
            SDL_GL_SetAttribute(SDL_GL_STEREO, 0);
        }

        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

        SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

        if((SDL_window = SDL_CreateWindow(CLIENT_WINDOW_TITLE, x, y,
                                          glConfig.vidWidth, glConfig.vidHeight, flags)) == nullptr) {
            clientRendererSystem->RefPrintf(PRINT_DEVELOPER,
                                            "SDL_CreateWindow failed: %s\n",
                                            SDL_GetError());
            continue;
        }

        if(fullscreen) {
            SDL_DisplayMode vidMode;

            switch(testColorBits) {
                case 16:
                    vidMode.format = SDL_PIXELFORMAT_RGB565;
                    break;

                case 24:
                    vidMode.format = SDL_PIXELFORMAT_RGB24;
                    break;

                default:
                    clientRendererSystem->RefPrintf(PRINT_DEVELOPER,
                                                    "testColorBits is %d, can't fullscreen\n",
                                                    testColorBits);
                    continue;
            }

            if(mode == -1) {
                SDL_SetWindowFullscreen(SDL_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                SDL_GL_GetDrawableSize(SDL_window, &glConfig.vidWidth,
                                       &glConfig.vidHeight);
            }

            vidMode.w = glConfig.vidWidth;
            vidMode.h = glConfig.vidHeight;
            vidMode.refresh_rate = glConfig.displayFrequency =
                                       r_displayRefresh->integer;
            vidMode.driverdata = nullptr;

            if(SDL_SetWindowDisplayMode(SDL_window, &vidMode) < 0) {
                clientRendererSystem->RefPrintf(PRINT_DEVELOPER,
                                                "SDL_SetWindowDisplayMode failed: %s\n",
                                                SDL_GetError());
                continue;
            }
        }

        glConfig.displayScale = GLimp_GetDPIScale(display);

        SDL_SetWindowIcon(SDL_window, icon);

        if(!fixedFunction) {
            sint profileMask, majorVersion, minorVersion;
            SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profileMask);
            SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &majorVersion);
            SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minorVersion);

            clientRendererSystem->RefPrintf(PRINT_ALL,
                                            "Trying to get an OpenGL 3.2 core context\n");
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                                SDL_GL_CONTEXT_PROFILE_CORE);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

            if((SDL_glContext = SDL_GL_CreateContext(SDL_window)) == nullptr) {
                clientRendererSystem->RefPrintf(PRINT_ALL,
                                                "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
                clientRendererSystem->RefPrintf(PRINT_ALL,
                                                "Reverting to default context\n");

                SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profileMask);
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, majorVersion);
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minorVersion);
            } else {
                pointer renderer;

                clientRendererSystem->RefPrintf(PRINT_ALL,
                                                "SDL_GL_CreateContext succeeded.\n");

                if(GLimp_GetProcAddresses(fixedFunction)) {
                    renderer = reinterpret_cast<pointer>(qglGetString(GL_RENDERER));
                } else {
                    clientRendererSystem->RefPrintf(PRINT_ALL,
                                                    "GLimp_GetProcAddresses() failed for OpenGL 3.2 core context\n");
                    renderer = nullptr;
                }

                if(!renderer || (strstr(renderer, "Software Renderer") ||
                                 strstr(renderer, "Software Rasterizer"))) {
                    if(renderer) {
                        clientRendererSystem->RefPrintf(PRINT_ALL,
                                                        "GL_RENDERER is %s, rejecting context\n", renderer);
                    }

                    GLimp_ClearProcAddresses();
                    SDL_GL_DeleteContext(SDL_glContext);
                    SDL_glContext = nullptr;

                    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profileMask);
                    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, majorVersion);
                    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minorVersion);
                }
            }
        } else {
            SDL_glContext = nullptr;
        }

        if(!SDL_glContext) {
            if((SDL_glContext = SDL_GL_CreateContext(SDL_window)) == nullptr) {
                clientRendererSystem->RefPrintf(PRINT_ALL,
                                                "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
                continue;
            }

            if(!GLimp_GetProcAddresses(fixedFunction)) {
                clientRendererSystem->RefPrintf(PRINT_ALL,
                                                "GLimp_GetProcAddresses() failed\n");
                GLimp_ClearProcAddresses();
                SDL_GL_DeleteContext(SDL_glContext);
                SDL_glContext = nullptr;
                SDL_DestroyWindow(SDL_window);
                SDL_window = nullptr;
                continue;
            }
        }

        qglClearColor(0, 0, 0, 1);
        qglClear(GL_COLOR_BUFFER_BIT);
        SDL_GL_SwapWindow(SDL_window);

        R_IssuePendingRenderCommands();

        if(SDL_GL_SetSwapInterval(r_swapInterval->integer) == -1) {
            clientRendererSystem->RefPrintf(PRINT_DEVELOPER,
                                            "SDL_GL_SetSwapInterval failed: %s\n",
                                            SDL_GetError());
        }

        SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &realColorBits[0]);
        SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &realColorBits[1]);
        SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &realColorBits[2]);
        SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &glConfig.depthBits);
        SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &glConfig.stencilBits);

        glConfig.colorBits = realColorBits[0] + realColorBits[1] +
                             realColorBits[2];

        clientRendererSystem->RefPrintf(PRINT_ALL,
                                        "Using %d color bits, %d depth, %d stencil display.\n", glConfig.colorBits,
                                        glConfig.depthBits, glConfig.stencilBits);
        break;
    }

    if(SDL_glContext == nullptr) {
        SDL_FreeSurface(icon);
        return RSERR_UNKNOWN;
    }

    if(!SDL_window) {
        clientRendererSystem->RefPrintf(PRINT_ALL, "Couldn't get a visual\n");
        return RSERR_INVALID_MODE;
    }

    GLimp_DetectAvailableModes();

    glstring = (valueType *)qglGetString(GL_RENDERER);
    clientRendererSystem->RefPrintf(PRINT_ALL, "GL_RENDERER: %s\n", glstring);

    SDL_MinimizeWindow(SDL_window);
    SDL_RestoreWindow(SDL_window);

    return RSERR_OK;
}

/*
===============
GLimp_StartDriverAndSetMode
===============
*/
static bool GLimp_StartDriverAndSetMode(sint mode, bool fullscreen,
                                        bool noborder, bool gl3Core) {
    rserr_t err;
    SDL_DisplayMode modeSDL;

    if(!SDL_WasInit(SDL_INIT_VIDEO)) {
        pointer driverName;

        if(SDL_Init(SDL_INIT_VIDEO) != 0) {
            clientRendererSystem->RefPrintf(PRINT_DEVELOPER,
                                            "SDL_Init( SDL_INIT_VIDEO ) FAILED (%s)\n",
                                            SDL_GetError());
            return false;
        }

        driverName = SDL_GetCurrentVideoDriver();
        clientRendererSystem->RefPrintf(PRINT_DEVELOPER,
                                        "SDL using driver \"%s\"\n", driverName);
        cvarSystem->Set("r_sdlDriver", driverName);
    }

    if(fullscreen && in_nograb->integer) {
        clientRendererSystem->RefPrintf(PRINT_DEVELOPER,
                                        "Fullscreen not allowed with in_nograb 1\n");
        cvarSystem->Set("r_fullscreen", "0");
        r_fullscreen->modified = false;
        fullscreen = false;
    }

    if(r_stereoEnabled->integer) {
        glConfig.stereoEnabled = true;
    } else {
        glConfig.stereoEnabled = false;
    }

    err = (rserr_t)GLimp_SetMode(mode, fullscreen, noborder, gl3Core);

    switch(err) {
        case RSERR_INVALID_FULLSCREEN:
            clientRendererSystem->RefPrintf(PRINT_DEVELOPER,
                                            "...WARNING: fullscreen unavailable in this mode\n");
            return false;

        case RSERR_INVALID_MODE:
            clientRendererSystem->RefPrintf(PRINT_DEVELOPER,
                                            "...WARNING: could not set the given mode (%d)\n", mode);
            return false;

        default:
            break;
    }

    return true;
}

/*
===============
GLimp_HaveExtension
===============
*/
static bool GLimp_HaveExtension(pointer ext) {
    pointer ptr = Q_stristr(glConfig.extensions_string, ext);

    if(ptr == nullptr) {
        return false;
    }

    ptr += strlen(ext);
    return ((*ptr == ' ') ||
            (*ptr == '\0'));       // verify it's complete string.
}

/*
===============
GLimp_InitExtensions
===============
*/
void GLimp_InitExtensions(void) {
    if(!r_allowExtensions->integer) {
        clientRendererSystem->RefPrintf(PRINT_ALL,
                                        "* IGNORING OPENGL EXTENSIONS *\n");
        return;
    }

    if(!r_verbose) {
        clientRendererSystem->RefPrintf(PRINT_ALL,
                                        "Initializing OpenGL extensions\n");

        glConfig.textureCompression = TC_NONE;

        // GL_EXT_texture_compression_s3tc
        if(GLimp_HaveExtension("GL_ARB_texture_compression") &&
                GLimp_HaveExtension("GL_EXT_texture_compression_s3tc")) {
            if(r_ext_compressed_textures->value) {
                glConfig.textureCompression = TC_S3TC_ARB;
                clientRendererSystem->RefPrintf(PRINT_ALL,
                                                "...using GL_EXT_texture_compression_s3tc\n");
            } else {
                clientRendererSystem->RefPrintf(PRINT_ALL,
                                                "...ignoring GL_EXT_texture_compression_s3tc\n");
            }
        } else {
            clientRendererSystem->RefPrintf(PRINT_ALL,
                                            "...GL_EXT_texture_compression_s3tc not found\n");
        }

        // GL_S3_s3tc ... legacy extension before GL_EXT_texture_compression_s3tc.
        if(glConfig.textureCompression == TC_NONE) {
            if(GLimp_HaveExtension("GL_S3_s3tc")) {
                if(r_ext_compressed_textures->value) {
                    glConfig.textureCompression = TC_S3TC;
                    clientRendererSystem->RefPrintf(PRINT_ALL, "...using GL_S3_s3tc\n");
                } else {
                    clientRendererSystem->RefPrintf(PRINT_ALL, "...ignoring GL_S3_s3tc\n");
                }
            } else {
                clientRendererSystem->RefPrintf(PRINT_ALL, "...GL_S3_s3tc not found\n");
            }
        }

        // GL_EXT_texture_env_add
        glConfig.textureEnvAddAvailable = false;

        if(GLimp_HaveExtension("GL_EXT_texture_env_add")) {
            if(r_ext_texture_env_add->integer) {
                glConfig.textureEnvAddAvailable = true;
                clientRendererSystem->RefPrintf(PRINT_ALL,
                                                "...using GL_EXT_texture_env_add\n");
            } else {
                glConfig.textureEnvAddAvailable = false;
                clientRendererSystem->RefPrintf(PRINT_ALL,
                                                "...ignoring GL_EXT_texture_env_add\n");
            }
        } else {
            clientRendererSystem->RefPrintf(PRINT_ALL,
                                            "...GL_EXT_texture_env_add not found\n");
        }

        // GL_ARB_multitexture
        qglMultiTexCoord2fARB = nullptr;
        qglActiveTextureARB = nullptr;
        qglClientActiveTextureARB = nullptr;

        if(GLimp_HaveExtension("GL_ARB_multitexture")) {
            if(r_ext_multitexture->value) {
                qglMultiTexCoord2fARB = (PFNGLMULTITEXCOORD2FARBPROC)
                                        SDL_GL_GetProcAddress("glMultiTexCoord2fARB");
                qglActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)
                                      SDL_GL_GetProcAddress("glActiveTextureARB");
                qglClientActiveTextureARB = (PFNGLCLIENTACTIVETEXTUREARBPROC)
                                            SDL_GL_GetProcAddress("glClientActiveTextureARB");

                if(qglActiveTextureARB) {
                    sint glint = 16; //Dushan
                    qglGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &glint);
                    glConfig.numTextureUnits = static_cast<sint>(glint);

                    if(glConfig.numTextureUnits > 1) {
                        clientRendererSystem->RefPrintf(PRINT_ALL,
                                                        "...using GL_ARB_multitexture\n");
                    } else {
                        qglMultiTexCoord2fARB = nullptr;
                        qglActiveTextureARB = nullptr;
                        qglClientActiveTextureARB = nullptr;
                        clientRendererSystem->RefPrintf(PRINT_ALL,
                                                        "...not using GL_ARB_multitexture, < 2 texture units\n");
                    }
                }
            } else {
                clientRendererSystem->RefPrintf(PRINT_ALL,
                                                "...ignoring GL_ARB_multitexture\n");
            }
        } else {
            clientRendererSystem->RefPrintf(PRINT_ALL,
                                            "...GL_ARB_multitexture not found\n");
        }


        // GL_EXT_compiled_vertex_array
        if(GLimp_HaveExtension("GL_EXT_compiled_vertex_array")) {
            if(r_ext_compiled_vertex_array->value) {
                clientRendererSystem->RefPrintf(PRINT_ALL,
                                                "...using GL_EXT_compiled_vertex_array\n");
                qglLockArraysEXT = (void (APIENTRY *)(sint,
                                                      sint)) SDL_GL_GetProcAddress("glLockArraysEXT");
                qglUnlockArraysEXT = (void (APIENTRY *)(void))
                                     SDL_GL_GetProcAddress("glUnlockArraysEXT");

                if(!qglLockArraysEXT || !qglUnlockArraysEXT) {
                    common->Error(ERR_FATAL, "bad getprocaddress");
                }
            } else {
                clientRendererSystem->RefPrintf(PRINT_ALL,
                                                "...ignoring GL_EXT_compiled_vertex_array\n");
            }
        } else {
            clientRendererSystem->RefPrintf(PRINT_ALL,
                                            "...GL_EXT_compiled_vertex_array not found\n");
        }

        glConfig.textureFilterAnisotropic = false;

        if(GLimp_HaveExtension("GL_EXT_texture_filter_anisotropic")) {
            if(r_ext_texture_filter_anisotropic->integer) {
                qglGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT,
                               static_cast<sint *>(&glConfig.maxAnisotropy));

                if(glConfig.maxAnisotropy <= 0) {
                    clientRendererSystem->RefPrintf(PRINT_ALL,
                                                    "...GL_EXT_texture_filter_anisotropic not properly supported!\n");
                    glConfig.maxAnisotropy = 0;
                } else {
                    clientRendererSystem->RefPrintf(PRINT_ALL,
                                                    "...using GL_EXT_texture_filter_anisotropic (max: %i)\n",
                                                    glConfig.maxAnisotropy);
                    glConfig.textureFilterAnisotropic = true;
                }
            } else {
                clientRendererSystem->RefPrintf(PRINT_ALL,
                                                "...ignoring GL_EXT_texture_filter_anisotropic\n");
            }
        } else {
            clientRendererSystem->RefPrintf(PRINT_ALL,
                                            "...GL_EXT_texture_filter_anisotropic not found\n");
        }
    }
}

#define R_MODE_FALLBACK 3 // 640 * 480

void GLimp_Splash(void) {
    uchar8 splashData[144000]; // width * height * bytes_per_pixel
    SDL_Surface *splashImage = nullptr;

    // decode splash image
    SPLASH_IMAGE_RUN_LENGTH_DECODE(splashData,
                                   CLIENT_WINDOW_SPLASH.rle_pixel_data,
                                   CLIENT_WINDOW_SPLASH.width * CLIENT_WINDOW_SPLASH.height,
                                   CLIENT_WINDOW_SPLASH.bytes_per_pixel);

    // get splash image
    splashImage = SDL_CreateRGBSurfaceFrom(
                      static_cast<void *>(splashData),
                      CLIENT_WINDOW_SPLASH.width,
                      CLIENT_WINDOW_SPLASH.height,
                      CLIENT_WINDOW_SPLASH.bytes_per_pixel * 8,
                      CLIENT_WINDOW_SPLASH.bytes_per_pixel * CLIENT_WINDOW_SPLASH.width,
#ifdef Q3_LITTLE_ENDIAN
                      0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
#else
                      0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF
#endif
                  );

    SDL_Rect dstRect;
    dstRect.x = glConfig.vidWidth / 2 - splashImage->w / 2;
    dstRect.y = glConfig.vidHeight / 2 - splashImage->h / 2;
    dstRect.w = splashImage->w;
    dstRect.h = splashImage->h;

    // apply image on surface
    if(SDL_BlitSurface(splashImage, nullptr, SDL_GetWindowSurface(SDL_window),
                       &dstRect) == 0) {
        SDL_UpdateWindowSurface(SDL_window);
    } else {
        common->Printf(S_COLOR_YELLOW "GLimp_Splash failed - %s\n",
                       SDL_GetError());
    }

    SDL_FreeSurface(splashImage);
}

/*
===============
GLimp_Init

This routine is responsible for initializing the OS specific portions
of OpenGL
===============
*/
void GLimp_Init(bool fixedFunction) {
    clientRendererSystem->RefPrintf(PRINT_DEVELOPER, "Glimp_Init( )\n");

    if(com_abnormalExit->integer) {
        cvarSystem->Set("r_mode", va(nullptr, "%d", R_MODE_FALLBACK));
        cvarSystem->Set("r_fullscreen", "0");
        cvarSystem->Set("r_centerWindow", "0");
        cvarSystem->Set("com_abnormalExit", "0");
    }

    idsystem->GLimpInit();

    // Create the window and set up the context
    if(GLimp_StartDriverAndSetMode(r_mode->integer, r_fullscreen->integer,
                                   r_noborder->integer, fixedFunction)) {
        goto success;
    }

    // Try again, this time in a platform specific "safe mode"
    idsystem->GLimpSafeInit();

    if(GLimp_StartDriverAndSetMode(r_mode->integer, r_fullscreen->integer,
                                   false, fixedFunction)) {
        goto success;
    }

    // Finally, try the default screen resolution
    if(r_mode->integer != R_MODE_FALLBACK) {
        clientRendererSystem->RefPrintf(PRINT_ALL,
                                        "Setting r_mode %d failed, falling back on r_mode %d\n", r_mode->integer,
                                        R_MODE_FALLBACK);

        if(GLimp_StartDriverAndSetMode(R_MODE_FALLBACK, false, false,
                                       fixedFunction)) {
            goto success;
        }
    }

    // Nothing worked, give up
    common->Error(ERR_FATAL, "GLimp_Init() - could not load OpenGL subsystem");

success:
    // These values force the UI to disable driver selection
    //glConfig.driverType = GLDRV_ICD;
    //glConfig.hardwareType = GLHW_GENERIC;

    // Only using SDL_SetWindowBrightness to determine if hardware gamma is supported
    glConfig.deviceSupportsGamma = !r_ignorehwgamma->integer &&
                                   SDL_SetWindowBrightness(SDL_window, 1.0f) >= 0;

    // get our config strings
    Q_strncpyz(glConfig.vendor_string, (valueType *) qglGetString(GL_VENDOR),
               sizeof(glConfig.vendor_string));
    Q_strncpyz(glConfig.renderer_string,
               (valueType *) qglGetString(GL_RENDERER), sizeof(glConfig.renderer_string));

    if(*glConfig.renderer_string &&
            glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] == '\n') {
        glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] = 0;
    }

    Q_strncpyz(glConfig.version_string, (valueType *) qglGetString(GL_VERSION),
               sizeof(glConfig.version_string));

    // manually create extension list if using OpenGL 3
    if(qglGetStringi) {
        sint i, numExtensions, extensionLength, listLength;
        pointer extension;

        qglGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
        listLength = 0;

        for(i = 0; i < numExtensions; i++) {
            extension = (valueType *) qglGetStringi(GL_EXTENSIONS, i);
            extensionLength = strlen(extension);

            if((listLength + extensionLength + 1) >= sizeof(
                        glConfig.extensions_string)) {
                break;
            }

            if(i > 0) {
                Q_strcat(glConfig.extensions_string, sizeof(glConfig.extensions_string),
                         " ");
                listLength++;
            }

            Q_strcat(glConfig.extensions_string, sizeof(glConfig.extensions_string),
                     extension);
            listLength += extensionLength;
        }
    } else {
        Q_strncpyz(glConfig.extensions_string,
                   (valueType *) qglGetString(GL_EXTENSIONS),
                   sizeof(glConfig.extensions_string));
    }

    // initialize extensions
    GLimp_InitExtensions();

    cvarSystem->Get("r_availableModes", "", CVAR_ROM, "Available video modes");

    // Display splash screen
#ifdef _WIN32
    GLimp_Splash();
#endif

    // This depends on SDL_INIT_VIDEO, hence having it here
    idsystem->Init(SDL_window);
}

/*
===============
GLimp_EndFrame

Responsible for doing a swapbuffers
===============
*/
void GLimp_EndFrame(void) {
    // don't flip if drawing to front buffer
    if(Q_stricmp(r_drawBuffer->string, "GL_FRONT") != 0) {
        SDL_GL_SwapWindow(SDL_window);
    }
}

/*
===========================================================

SMP acceleration

===========================================================
*/

/*
 * I have no idea if this will even work...most platforms don't offer
 * thread-safe OpenGL libraries, and it looks like the original Linux
 * code counted on each thread claiming the GL context with glXMakeCurrent(),
 * which you can't currently do in SDL. We'll just have to hope for the best.
 */

static SDL_mutex *smpMutex = nullptr;
static SDL_cond *renderCommandsEvent = nullptr;
static SDL_cond *renderCompletedEvent = nullptr;
static void (*glimpRenderThread)(void) = nullptr;
static SDL_Thread *renderThread = nullptr;
static volatile void *smpData = nullptr;
static volatile bool smpDataReady;

/*
===============
GLimp_ShutdownRenderThread
===============
*/
static void GLimp_ShutdownRenderThread(void) {
    if(renderThread != nullptr) {
        GLimp_WakeRenderer(nullptr);
        SDL_WaitThread(renderThread, nullptr);
        renderThread = nullptr;
        glConfig.smpActive = false;
    }

    if(smpMutex != nullptr) {
        SDL_DestroyMutex(smpMutex);
        smpMutex = nullptr;
    }

    if(renderCommandsEvent != nullptr) {
        SDL_DestroyCond(renderCommandsEvent);
        renderCommandsEvent = nullptr;
    }

    if(renderCompletedEvent != nullptr) {
        SDL_DestroyCond(renderCompletedEvent);
        renderCompletedEvent = nullptr;
    }

    glimpRenderThread = nullptr;
}

static void GLimp_SetCurrentContext(bool enable) {
    if(enable) {
        SDL_GL_MakeCurrent(SDL_window, SDL_glContext);
    } else {
        SDL_GL_MakeCurrent(SDL_window, nullptr);
    }
}

/*
===============
GLimp_RenderThreadWrapper
===============
*/
static sint GLimp_RenderThreadWrapper(void *arg) {
    common->Printf("Render thread starting\n");

    glimpRenderThread();

    GLimp_SetCurrentContext(false);

    common->Printf("Render thread terminating\n");

    return 0;
}

/*
===============
GLimp_SpawnRenderThread
===============
*/
bool GLimp_SpawnRenderThread(void (*function)(void)) {
    if(renderThread != nullptr) { // hopefully just a zombie at this point...
        clientRendererSystem->RefPrintf(PRINT_WARNING,
                                        "Already a render thread? Trying to clean it up...\n");
        GLimp_ShutdownRenderThread();
    }

    smpMutex = SDL_CreateMutex();

    if(smpMutex == nullptr) {
        clientRendererSystem->RefPrintf(PRINT_WARNING,
                                        "smpMutex creation failed: %s\n",
                                        SDL_GetError());
        GLimp_ShutdownRenderThread();
        return false;
    }

    renderCommandsEvent = SDL_CreateCond();

    if(renderCommandsEvent == nullptr) {
        clientRendererSystem->RefPrintf(PRINT_WARNING,
                                        "renderCommandsEvent creation failed: %s\n",
                                        SDL_GetError());
        GLimp_ShutdownRenderThread();
        return false;
    }

    renderCompletedEvent = SDL_CreateCond();

    if(renderCompletedEvent == nullptr) {
        clientRendererSystem->RefPrintf(PRINT_WARNING,
                                        "renderCompletedEvent creation failed: %s\n",
                                        SDL_GetError());
        GLimp_ShutdownRenderThread();
        return false;
    }

    glimpRenderThread = function;
    renderThread = SDL_CreateThread(GLimp_RenderThreadWrapper, "render thread",
                                    nullptr);

    if(renderThread == nullptr) {
        clientRendererSystem->RefPrintf(PRINT_WARNING,
                                        "SDL_CreateThread() returned %s",
                                        SDL_GetError());
        GLimp_ShutdownRenderThread();
        return false;
    }

    SDL_LockMutex(smpMutex);

    while(smpData) {
        SDL_CondWait(renderCompletedEvent, smpMutex);
    }

    SDL_UnlockMutex(smpMutex);

    return true;
}

/*
===============
GLimp_RendererSleep
===============
*/
void *GLimp_RendererSleep(void) {
    void *data = nullptr;

    GLimp_SetCurrentContext(false);

    SDL_LockMutex(smpMutex);
    {
        smpData = nullptr;
        smpDataReady = false;

        // after this, the front end can exit GLimp_FrontEndSleep
        SDL_CondSignal(renderCompletedEvent);

        while(!smpDataReady) {
            SDL_CondWait(renderCommandsEvent, smpMutex);
        }

        data = (const_cast<void *>(smpData));
    }
    SDL_UnlockMutex(smpMutex);

    GLimp_SetCurrentContext(true);

    return data;
}

/*
===============
GLimp_FrontEndSleep
===============
*/
void GLimp_FrontEndSleep(void) {
    SDL_LockMutex(smpMutex);
    {
        while(smpData) {
            SDL_CondWait(renderCompletedEvent, smpMutex);
        }
    }
    SDL_UnlockMutex(smpMutex);

    GLimp_SetCurrentContext(true);
}

/*
===============
GLimp_WakeRenderer
===============
*/
void GLimp_WakeRenderer(void *data) {
    GLimp_SetCurrentContext(false);

    SDL_LockMutex(smpMutex);
    {
        assert(smpData == nullptr);
        smpData = data;
        smpDataReady = true;

        // after this, the renderer can continue through GLimp_RendererSleep
        SDL_CondSignal(renderCommandsEvent);
    }
    SDL_UnlockMutex(smpMutex);

}

/*
===============
GLimp_Shutdown
===============
*/
void GLimp_Shutdown(void) {
    idsystem->Shutdown();

    if(renderThread != nullptr) {
        common->Printf("Destroying renderer thread...\n");
        GLimp_ShutdownRenderThread();
    }

    if(SDL_glContext) {
        SDL_GL_DeleteContext(SDL_glContext);
        SDL_glContext = nullptr;
    }

    if(SDL_window) {
        SDL_DestroyWindow(SDL_window);
        SDL_window = nullptr;
    }

    SDL_QuitSubSystem(SDL_INIT_VIDEO);

    ::memset(&glConfig, 0, sizeof(glConfig));
    ::memset(&glState, 0, sizeof(glState));
    ::memset(&glRefConfig, 0, sizeof(glRefConfig));
}

/*
===============
GLimp_SyncRenderThread
===============
*/
void GLimp_SyncRenderThread(void) {
    GLimp_FrontEndSleep();

    GLimp_SetCurrentContext(true);
}
