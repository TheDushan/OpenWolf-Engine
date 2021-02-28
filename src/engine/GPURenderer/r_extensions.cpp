////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 James Canete (use.less01@gmail.com)
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
// File name:   r_extensions.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: extensions needed by the renderer not in r_glimp.cpp
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

void GLimp_InitExtraExtensions( void )
{
    //sint len;
    valueType* extension;
    const valueType* result[3] = { "...ignoring %s\n", "...using %s\n", "...%s not found\n" };
    bool q_gl_version_at_least_3_0;
    bool q_gl_version_at_least_3_2;
    
    // Check OpenGL version
    if( !QGL_VERSION_ATLEAST( 2, 0 ) )
        Com_Error( ERR_FATAL, "OpenGL 2.0 required!" );
    CL_RefPrintf( PRINT_ALL, "...using OpenGL %s\n", glConfig.version_string );
    
    q_gl_version_at_least_3_0 = QGL_VERSION_ATLEAST( 3, 0 );
    q_gl_version_at_least_3_2 = QGL_VERSION_ATLEAST( 3, 2 );
    
    // Check if we need Intel graphics specific fixes.
    glRefConfig.intelGraphics = false;
    if( strstr( ( valueType* )qglGetString( GL_RENDERER ), "Intel" ) )
        glRefConfig.intelGraphics = true;
        
    // set DSA fallbacks
#define GLE(ret, name, ...) qgl##name = GLDSA_##name;
    QGL_EXT_direct_state_access_PROCS;
#undef GLE
    
    // GL function loader, based on https://gist.github.com/rygorous/16796a0c876cf8a5f542caddb55bce8a
#define GLE(ret, name, ...) qgl##name = (name##proc *) SDL_GL_GetProcAddress("gl" #name);
    
    QGL_1_1_PROCS
    QGL_1_1_FIXED_FUNCTION_PROCS;
    QGL_DESKTOP_1_1_PROCS;
    QGL_DESKTOP_1_1_FIXED_FUNCTION_PROCS;
    
    // OpenGL 1.3, was GL_ARB_texture_compression
    QGL_1_3_PROCS;
    
    // OpenGL 1.5, was GL_ARB_vertex_buffer_object and GL_ARB_occlusion_query
    QGL_1_5_PROCS;
    QGL_ARB_occlusion_query_PROCS;
    glRefConfig.occlusionQuery = true;
    
    // OpenGL 2.0, was GL_ARB_shading_language_100, GL_ARB_vertex_program, GL_ARB_shader_objects, and GL_ARB_vertex_shader
    QGL_2_0_PROCS;
    QGL_3_0_PROCS;
    
    //Dushan
    QGL_4_0_PROCS;
    
    // OpenGL 3.0 - GL_ARB_framebuffer_object
    extension = "GL_ARB_framebuffer_object";
    glRefConfig.framebufferObject = false;
    glRefConfig.framebufferBlit = false;
    glRefConfig.framebufferMultisample = false;
    if( q_gl_version_at_least_3_0 || SDL_GL_ExtensionSupported( extension ) )
    {
        glRefConfig.framebufferObject = !!r_ext_framebuffer_object->integer;
        glRefConfig.framebufferBlit = true;
        glRefConfig.framebufferMultisample = true;
        
        qglGetIntegerv( GL_MAX_RENDERBUFFER_SIZE, &glRefConfig.maxRenderbufferSize );
        qglGetIntegerv( GL_MAX_COLOR_ATTACHMENTS, &glRefConfig.maxColorAttachments );
        
        QGL_ARB_framebuffer_object_PROCS;
        
        CL_RefPrintf( PRINT_ALL, result[glRefConfig.framebufferObject], extension );
    }
    else
    {
        CL_RefPrintf( PRINT_ALL, result[2], extension );
    }
    
    // OpenGL 3.0 - GL_ARB_vertex_array_object
    extension = "GL_ARB_vertex_array_object";
    glRefConfig.vertexArrayObject = false;
    if( q_gl_version_at_least_3_0 || SDL_GL_ExtensionSupported( extension ) )
    {
        if( q_gl_version_at_least_3_0 )
        {
            // force VAO, core context requires it
            glRefConfig.vertexArrayObject = true;
        }
        else
        {
            glRefConfig.vertexArrayObject = !!r_arb_vertex_array_object->integer;
        }
        
        QGL_ARB_vertex_array_object_PROCS;
        
        CL_RefPrintf( PRINT_ALL, result[glRefConfig.vertexArrayObject], extension );
    }
    else
    {
        CL_RefPrintf( PRINT_ALL, result[2], extension );
    }
    
    // OpenGL 3.0 - GL_ARB_texture_float
    extension = "GL_ARB_texture_float";
    glRefConfig.textureFloat = false;
    if( q_gl_version_at_least_3_0 || SDL_GL_ExtensionSupported( extension ) )
    {
        glRefConfig.textureFloat = !!r_ext_texture_float->integer;
        
        CL_RefPrintf( PRINT_ALL, result[glRefConfig.textureFloat], extension );
    }
    else
    {
        CL_RefPrintf( PRINT_ALL, result[2], extension );
    }
    
    // OpenGL 3.2 - GL_ARB_depth_clamp
    extension = "GL_ARB_depth_clamp";
    glRefConfig.depthClamp = false;
    if( q_gl_version_at_least_3_2 || SDL_GL_ExtensionSupported( extension ) )
    {
        glRefConfig.depthClamp = true;
        
        CL_RefPrintf( PRINT_ALL, result[glRefConfig.depthClamp], extension );
    }
    else
    {
        CL_RefPrintf( PRINT_ALL, result[2], extension );
    }
    
    // OpenGL 3.2 - GL_ARB_seamless_cube_map
    extension = "GL_ARB_seamless_cube_map";
    glRefConfig.seamlessCubeMap = false;
    if( q_gl_version_at_least_3_2 || SDL_GL_ExtensionSupported( extension ) )
    {
        glRefConfig.seamlessCubeMap = !!r_arb_seamless_cube_map->integer;
        
        CL_RefPrintf( PRINT_ALL, result[glRefConfig.seamlessCubeMap], extension );
    }
    else
    {
        CL_RefPrintf( PRINT_ALL, result[2], extension );
    }
    
    if( !qglGetString )
    {
        Com_Error( ERR_FATAL, "glGetString is nullptr" );
    }
    
    // Determine GLSL version
    if( 1 )
    {
        valueType version[256];
        
        Q_strncpyz( version, ( valueType* )qglGetString( GL_SHADING_LANGUAGE_VERSION ), sizeof( version ) );
        
        sscanf( version, "%d.%d", &glRefConfig.glslMajorVersion, &glRefConfig.glslMinorVersion );
        
        CL_RefPrintf( PRINT_ALL, "...using GLSL version %s\n", version );
    }
    
    glRefConfig.memInfo = MI_NONE;
    
    // GL_NVX_gpu_memory_info
    extension = "GL_NVX_gpu_memory_info";
    if( SDL_GL_ExtensionSupported( extension ) )
    {
        glRefConfig.memInfo = MI_NVX;
        
        CL_RefPrintf( PRINT_ALL, result[1], extension );
    }
    else
    {
        CL_RefPrintf( PRINT_ALL, result[2], extension );
    }
    
    // GL_ATI_meminfo
    extension = "GL_ATI_meminfo";
    if( SDL_GL_ExtensionSupported( extension ) )
    {
        if( glRefConfig.memInfo == MI_NONE )
        {
            glRefConfig.memInfo = MI_ATI;
            
            CL_RefPrintf( PRINT_ALL, result[1], extension );
        }
        else
        {
            CL_RefPrintf( PRINT_ALL, result[0], extension );
        }
    }
    else
    {
        CL_RefPrintf( PRINT_ALL, result[2], extension );
    }
    
    glRefConfig.textureCompression = TCR_NONE;
    
    // GL_ARB_texture_compression_rgtc
    extension = "GL_ARB_texture_compression_rgtc";
    if( SDL_GL_ExtensionSupported( extension ) )
    {
        bool useRgtc = r_ext_compressed_textures->integer >= 1;
        
        if( useRgtc )
            glRefConfig.textureCompression |= TCR_RGTC;
            
        CL_RefPrintf( PRINT_ALL, result[useRgtc], extension );
    }
    else
    {
        CL_RefPrintf( PRINT_ALL, result[2], extension );
    }
    
    glRefConfig.swizzleNormalmap = r_ext_compressed_textures->integer && !( glRefConfig.textureCompression & TCR_RGTC );
    
    // GL_ARB_texture_compression_bptc
    extension = "GL_ARB_texture_compression_bptc";
    if( SDL_GL_ExtensionSupported( extension ) )
    {
        bool useBptc = r_ext_compressed_textures->integer >= 2;
        
        if( useBptc )
            glRefConfig.textureCompression |= TCR_BPTC;
            
        CL_RefPrintf( PRINT_ALL, result[useBptc], extension );
    }
    else
    {
        CL_RefPrintf( PRINT_ALL, result[2], extension );
    }
    
    extension = "GL_EXT_shadow_samplers";
    if( SDL_GL_ExtensionSupported( extension ) )
    {
        CL_RefPrintf( PRINT_ALL, result[1], extension );
    }
    else
    {
        CL_RefPrintf( PRINT_ALL, result[2], extension );
    }
    
    // GL_EXT_direct_state_access
    extension = "GL_EXT_direct_state_access";
    glRefConfig.directStateAccess = false;
    if( SDL_GL_ExtensionSupported( extension ) )
    {
        glRefConfig.directStateAccess = !!r_ext_direct_state_access->integer;
        
        // QGL_*_PROCS becomes several functions, do not remove {}
        if( glRefConfig.directStateAccess )
        {
            QGL_EXT_direct_state_access_PROCS;
        }
        
        CL_RefPrintf( PRINT_ALL, result[glRefConfig.directStateAccess], extension );
    }
    else
    {
        CL_RefPrintf( PRINT_ALL, result[2], extension );
    }
    
#undef GLE
}
