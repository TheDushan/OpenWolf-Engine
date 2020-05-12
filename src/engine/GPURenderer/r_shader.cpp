////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2005 Id Software, Inc.
// Copyright(C) 2000 - 2013 Darklegion Development
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
// File name:   r_shader.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2015
// Description: this file deals with the parsing and definition of shaders
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.h>

static UTF8* s_shaderText;

// the shader is parsed into these global variables, then copied into
// dynamically allocated memory if it is valid.
static	shaderStage_t	stages[MAX_SHADER_STAGES];
static	shader_t		shader;
static	texModInfo_t	texMods[MAX_SHADER_STAGES][TR_MAX_TEXMODS];

#define FILE_HASH_SIZE		1024
static	shader_t*		hashTable[FILE_HASH_SIZE];

#define MAX_SHADERNAME_LENGTH	 127
#define MAX_SHADERTEXT_HASH		4096	// from 2048 to 4096

typedef struct shaderText_s     // 8 bytes + strlen(text)+1
{
    struct shaderText_s* next;	// linked list hashtable
    UTF8* name;					// shader name
    UTF8 text[0];				// shader text
} shaderText_t;

static shaderText_t* shaderTextHashTable[MAX_SHADERTEXT_HASH];

static S32 fileShaderCount;		// total .shader files found
static S32 shaderCount;			// total shaders parsed

// ydnar: these are here because they are only referenced while parsing a shader
static UTF8 implicitMap[MAX_QPATH];
static unsigned implicitStateBits;
static cullType_t implicitCullType;

/*
================
return a hash value for the filename
================
*/
#ifdef __GNUCC__
#warning TODO: check if S64 is ok here
#endif
static S64 generateHashValue( StringEntry fname, const S32 size )
{
    S32		i;
    S64	hash;
    UTF8	letter;
    
    hash = 0;
    i = 0;
    while( fname[i] != '\0' )
    {
        letter = tolower( fname[i] );
        if( letter == '.' ) break;				// don't include extension
        if( letter == '\\' ) letter = '/';		// damn path names
        if( letter == PATH_SEP ) letter = '/';		// damn path names
        hash += ( S64 )( letter ) * ( i + 119 );
        i++;
    }
    hash = ( hash ^ ( hash >> 10 ) ^ ( hash >> 20 ) );
    hash &= ( size - 1 );
    return hash;
}

void idRenderSystemLocal::RemapShader( StringEntry shaderName, StringEntry newShaderName, StringEntry timeOffset )
{
    UTF8		strippedName[MAX_QPATH];
    S32			hash;
    shader_t*	sh, *sh2;
    qhandle_t	h;
    
    sh = R_FindShaderByName( shaderName );
    if( sh == nullptr || sh == tr.defaultShader )
    {
        h = RE_RegisterShaderLightMap( shaderName, 0 );
        sh = R_GetShaderByHandle( h );
    }
    if( sh == nullptr || sh == tr.defaultShader )
    {
        CL_RefPrintf( PRINT_WARNING, "WARNING: idRenderSystemLocal::RemapShader: shader %s not found\n", shaderName );
        return;
    }
    
    sh2 = R_FindShaderByName( newShaderName );
    if( sh2 == nullptr || sh2 == tr.defaultShader )
    {
        h = RE_RegisterShaderLightMap( newShaderName, 0 );
        sh2 = R_GetShaderByHandle( h );
    }
    
    if( sh2 == nullptr || sh2 == tr.defaultShader )
    {
        CL_RefPrintf( PRINT_WARNING, "WARNING: idRenderSystemLocal::RemapShader: new shader %s not found\n", newShaderName );
        return;
    }
    
    // remap all the shaders with the given name
    // even tho they might have different lightmaps
    COM_StripExtension2( shaderName, strippedName, sizeof( strippedName ) );
    hash = generateHashValue( strippedName, FILE_HASH_SIZE );
    for( sh = hashTable[hash]; sh; sh = sh->next )
    {
        if( Q_stricmp( sh->name, strippedName ) == 0 )
        {
            if( sh != sh2 )
            {
                sh->remappedShader = sh2;
            }
            else
            {
                sh->remappedShader = nullptr;
            }
        }
    }
    if( timeOffset )
    {
        sh2->timeOffset = atof( timeOffset );
    }
}

/*
===============
ParseVector
===============
*/
static bool ParseVector( UTF8** text, S32 count, F32* v )
{
    UTF8*	token;
    S32		i;
    
    // FIXME: spaces are currently required after parens, should change parseext...
    token = COM_ParseExt2( text, false );
    if( strcmp( token, "(" ) )
    {
        CL_RefPrintf( PRINT_WARNING, "WARNING: missing parenthesis in shader '%s'\n", shader.name );
        return false;
    }
    
    for( i = 0 ; i < count ; i++ )
    {
        token = COM_ParseExt2( text, false );
        if( !token[0] )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing vector element in shader '%s'\n", shader.name );
            return false;
        }
        v[i] = atof( token );
    }
    
    token = COM_ParseExt2( text, false );
    if( strcmp( token, ")" ) )
    {
        CL_RefPrintf( PRINT_WARNING, "WARNING: missing parenthesis in shader '%s'\n", shader.name );
        return false;
    }
    
    return true;
}


/*
===============
NameToAFunc
===============
*/
static U32 NameToAFunc( StringEntry funcname )
{
    if( !Q_stricmp( funcname, "GT0" ) )
    {
        return GLS_ATEST_GT_0;
    }
    else if( !Q_stricmp( funcname, "LT128" ) )
    {
        return GLS_ATEST_LT_80;
    }
    else if( !Q_stricmp( funcname, "GE128" ) )
    {
        return GLS_ATEST_GE_80;
    }
    
    CL_RefPrintf( PRINT_WARNING, "WARNING: invalid alphaFunc name '%s' in shader '%s'\n", funcname, shader.name );
    return 0;
}


/*
===============
NameToSrcBlendMode
===============
*/
static S32 NameToSrcBlendMode( StringEntry name )
{
    if( !Q_stricmp( name, "GL_ONE" ) )
    {
        return GLS_SRCBLEND_ONE;
    }
    else if( !Q_stricmp( name, "GL_ZERO" ) )
    {
        return GLS_SRCBLEND_ZERO;
    }
    else if( !Q_stricmp( name, "GL_DST_COLOR" ) )
    {
        return GLS_SRCBLEND_DST_COLOR;
    }
    else if( !Q_stricmp( name, "GL_ONE_MINUS_DST_COLOR" ) )
    {
        return GLS_SRCBLEND_ONE_MINUS_DST_COLOR;
    }
    else if( !Q_stricmp( name, "GL_SRC_ALPHA" ) )
    {
        return GLS_SRCBLEND_SRC_ALPHA;
    }
    else if( !Q_stricmp( name, "GL_ONE_MINUS_SRC_ALPHA" ) )
    {
        return GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA;
    }
    else if( !Q_stricmp( name, "GL_DST_ALPHA" ) )
    {
        if( r_ignoreDstAlpha->integer )
            return GLS_SRCBLEND_ONE;
            
        return GLS_SRCBLEND_DST_ALPHA;
    }
    else if( !Q_stricmp( name, "GL_ONE_MINUS_DST_ALPHA" ) )
    {
        if( r_ignoreDstAlpha->integer )
            return GLS_SRCBLEND_ZERO;
            
        return GLS_SRCBLEND_ONE_MINUS_DST_ALPHA;
    }
    else if( !Q_stricmp( name, "GL_SRC_ALPHA_SATURATE" ) )
    {
        return GLS_SRCBLEND_ALPHA_SATURATE;
    }
    
    CL_RefPrintf( PRINT_WARNING, "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name );
    return GLS_SRCBLEND_ONE;
}

/*
===============
NameToDstBlendMode
===============
*/
static S32 NameToDstBlendMode( StringEntry name )
{
    if( !Q_stricmp( name, "GL_ONE" ) )
    {
        return GLS_DSTBLEND_ONE;
    }
    else if( !Q_stricmp( name, "GL_ZERO" ) )
    {
        return GLS_DSTBLEND_ZERO;
    }
    else if( !Q_stricmp( name, "GL_SRC_ALPHA" ) )
    {
        return GLS_DSTBLEND_SRC_ALPHA;
    }
    else if( !Q_stricmp( name, "GL_ONE_MINUS_SRC_ALPHA" ) )
    {
        return GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
    }
    else if( !Q_stricmp( name, "GL_DST_ALPHA" ) )
    {
        if( r_ignoreDstAlpha->integer )
            return GLS_DSTBLEND_ONE;
            
        return GLS_DSTBLEND_DST_ALPHA;
    }
    else if( !Q_stricmp( name, "GL_ONE_MINUS_DST_ALPHA" ) )
    {
        if( r_ignoreDstAlpha->integer )
            return GLS_DSTBLEND_ZERO;
            
        return GLS_DSTBLEND_ONE_MINUS_DST_ALPHA;
    }
    else if( !Q_stricmp( name, "GL_SRC_COLOR" ) )
    {
        return GLS_DSTBLEND_SRC_COLOR;
    }
    else if( !Q_stricmp( name, "GL_ONE_MINUS_SRC_COLOR" ) )
    {
        return GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;
    }
    
    CL_RefPrintf( PRINT_WARNING, "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name );
    return GLS_DSTBLEND_ONE;
}

/*
===============
NameToGenFunc
===============
*/
static genFunc_t NameToGenFunc( StringEntry funcname )
{
    if( !Q_stricmp( funcname, "sin" ) )
    {
        return GF_SIN;
    }
    else if( !Q_stricmp( funcname, "square" ) )
    {
        return GF_SQUARE;
    }
    else if( !Q_stricmp( funcname, "triangle" ) )
    {
        return GF_TRIANGLE;
    }
    else if( !Q_stricmp( funcname, "sawtooth" ) )
    {
        return GF_SAWTOOTH;
    }
    else if( !Q_stricmp( funcname, "inversesawtooth" ) )
    {
        return GF_INVERSE_SAWTOOTH;
    }
    else if( !Q_stricmp( funcname, "noise" ) )
    {
        return GF_NOISE;
    }
    
    CL_RefPrintf( PRINT_WARNING, "WARNING: invalid genfunc name '%s' in shader '%s'\n", funcname, shader.name );
    return GF_SIN;
}


/*
===================
ParseWaveForm
===================
*/
static void ParseWaveForm( UTF8** text, waveForm_t* wave )
{
    UTF8* token;
    
    token = COM_ParseExt2( text, false );
    if( token[0] == 0 )
    {
        CL_RefPrintf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
        return;
    }
    wave->func = NameToGenFunc( token );
    
    // BASE, AMP, PHASE, FREQ
    token = COM_ParseExt2( text, false );
    if( token[0] == 0 )
    {
        CL_RefPrintf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
        return;
    }
    wave->base = atof( token );
    
    token = COM_ParseExt2( text, false );
    if( token[0] == 0 )
    {
        CL_RefPrintf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
        return;
    }
    wave->amplitude = atof( token );
    
    token = COM_ParseExt2( text, false );
    if( token[0] == 0 )
    {
        CL_RefPrintf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
        return;
    }
    wave->phase = atof( token );
    
    token = COM_ParseExt2( text, false );
    if( token[0] == 0 )
    {
        CL_RefPrintf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
        return;
    }
    wave->frequency = atof( token );
}


/*
===================
ParseTexMod
===================
*/
static void ParseTexMod( UTF8* _text, shaderStage_t* stage )
{
    StringEntry token;
    UTF8** text = &_text;
    texModInfo_t* tmi;
    
    if( stage->bundle[0].numTexMods == TR_MAX_TEXMODS )
    {
        Com_Error( ERR_DROP, "ERROR: too many tcMod stages in shader '%s'", shader.name );
        return;
    }
    
    tmi = &stage->bundle[0].texMods[stage->bundle[0].numTexMods];
    stage->bundle[0].numTexMods++;
    
    token = COM_ParseExt2( text, false );
    
    //
    // turb
    //
    if( !Q_stricmp( token, "turb" ) )
    {
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing tcMod turb parms in shader '%s'\n", shader.name );
            return;
        }
        tmi->wave.base = atof( token );
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
            return;
        }
        tmi->wave.amplitude = atof( token );
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
            return;
        }
        tmi->wave.phase = atof( token );
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
            return;
        }
        tmi->wave.frequency = atof( token );
        
        tmi->type = TMOD_TURBULENT;
    }
    //
    // scale
    //
    else if( !Q_stricmp( token, "scale" ) )
    {
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing scale parms in shader '%s'\n", shader.name );
            return;
        }
        tmi->scale[0] = atof( token );
        
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing scale parms in shader '%s'\n", shader.name );
            return;
        }
        tmi->scale[1] = atof( token );
        tmi->type = TMOD_SCALE;
    }
    //
    // scroll
    //
    else if( !Q_stricmp( token, "scroll" ) )
    {
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing scale scroll parms in shader '%s'\n", shader.name );
            return;
        }
        tmi->scroll[0] = atof( token );
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing scale scroll parms in shader '%s'\n", shader.name );
            return;
        }
        tmi->scroll[1] = atof( token );
        tmi->type = TMOD_SCROLL;
    }
    //
    // stretch
    //
    else if( !Q_stricmp( token, "stretch" ) )
    {
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
            return;
        }
        tmi->wave.func = NameToGenFunc( token );
        
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
            return;
        }
        tmi->wave.base = atof( token );
        
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
            return;
        }
        tmi->wave.amplitude = atof( token );
        
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
            return;
        }
        tmi->wave.phase = atof( token );
        
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
            return;
        }
        tmi->wave.frequency = atof( token );
        
        tmi->type = TMOD_STRETCH;
    }
    //
    // transform
    //
    else if( !Q_stricmp( token, "transform" ) )
    {
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
            return;
        }
        tmi->matrix[0][0] = atof( token );
        
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
            return;
        }
        tmi->matrix[0][1] = atof( token );
        
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
            return;
        }
        tmi->matrix[1][0] = atof( token );
        
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
            return;
        }
        tmi->matrix[1][1] = atof( token );
        
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
            return;
        }
        tmi->translate[0] = atof( token );
        
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
            return;
        }
        tmi->translate[1] = atof( token );
        
        tmi->type = TMOD_TRANSFORM;
    }
    //
    // rotate
    //
    else if( !Q_stricmp( token, "rotate" ) )
    {
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing tcMod rotate parms in shader '%s'\n", shader.name );
            return;
        }
        tmi->rotateSpeed = atof( token );
        tmi->type = TMOD_ROTATE;
    }
    //
    // entityTranslate
    //
    else if( !Q_stricmp( token, "entityTranslate" ) )
    {
        tmi->type = TMOD_ENTITY_TRANSLATE;
    }
    else
    {
        CL_RefPrintf( PRINT_WARNING, "WARNING: unknown tcMod '%s' in shader '%s'\n", token, shader.name );
    }
}


/*
===================
ParseStage
===================
*/
static bool ParseStage( shaderStage_t* stage, UTF8** text )
{
    UTF8* token;
    S32 depthMaskBits = GLS_DEPTHMASK_TRUE, blendSrcBits = 0, blendDstBits = 0, atestBits = 0, depthFuncBits = 0;
    bool depthMaskExplicit = false;
    
    stage->active = true;
    
    while( 1 )
    {
        token = COM_ParseExt2( text, true );
        if( !token[0] )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: no matching '}' found\n" );
            return false;
        }
        
        if( token[0] == '}' )
        {
            break;
        }
        //
        // map <name>
        //
        else if( !Q_stricmp( token, "map" ) )
        {
            token = COM_ParseExt2( text, false );
            if( !token[0] )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing parameter for 'map' keyword in shader '%s'\n", shader.name );
                return false;
            }
            
            if( !Q_stricmp( token, "$whiteimage" ) )
            {
                stage->bundle[0].image[0] = tr.whiteImage;
                continue;
            }
            else if( !Q_stricmp( token, "$lightmap" ) )
            {
                stage->bundle[0].isLightmap = true;
                if( shader.lightmapIndex < 0 || !tr.lightmaps )
                {
                    stage->bundle[0].image[0] = tr.whiteImage;
                }
                else
                {
                    stage->bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex];
                }
                continue;
            }
            else if( !Q_stricmp( token, "$deluxemap" ) )
            {
                if( !tr.worldDeluxeMapping )
                {
                    CL_RefPrintf( PRINT_WARNING, "WARNING: shader '%s' wants a deluxe map in a map compiled without them\n", shader.name );
                    return false;
                }
                
                stage->bundle[0].isLightmap = true;
                if( shader.lightmapIndex < 0 )
                {
                    stage->bundle[0].image[0] = tr.whiteImage;
                }
                else
                {
                    stage->bundle[0].image[0] = tr.deluxemaps[shader.lightmapIndex];
                }
                continue;
            }
            else
            {
                imgType_t type = IMGTYPE_COLORALPHA;
                S32/*imgFlags_t*/ flags = IMGFLAG_NONE;
                
                if( !shader.noMipMaps )
                    flags |= IMGFLAG_MIPMAP;
                    
                if( !shader.noPicMip )
                    flags |= IMGFLAG_PICMIP;
                    
                if( stage->type == ST_NORMALMAP || stage->type == ST_NORMALPARALLAXMAP )
                {
                    type = IMGTYPE_NORMAL;
                    flags |= IMGFLAG_NOLIGHTSCALE;
                    
                    if( stage->type == ST_NORMALPARALLAXMAP )
                        type = IMGTYPE_NORMALHEIGHT;
                }
                else
                {
                    if( r_genNormalMaps->integer )
                        flags |= IMGFLAG_GENNORMALMAP;
                }
                
                stage->bundle[0].image[0] = R_FindImageFile( token, type, flags );
                
                if( !stage->bundle[0].image[0] )
                {
                    CL_RefPrintf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
                    return false;
                }
            }
        }
        //
        // clampmap <name>
        //
        else if( !Q_stricmp( token, "clampmap" ) )
        {
            imgType_t type = IMGTYPE_COLORALPHA;
            S32/*imgFlags_t*/ flags = IMGFLAG_CLAMPTOEDGE;
            
            token = COM_ParseExt2( text, false );
            if( !token[0] )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing parameter for 'clampmap' keyword in shader '%s'\n", shader.name );
                return false;
            }
            
            if( !shader.noMipMaps )
                flags |= IMGFLAG_MIPMAP;
                
            if( !shader.noPicMip )
                flags |= IMGFLAG_PICMIP;
                
            if( stage->type == ST_NORMALMAP || stage->type == ST_NORMALPARALLAXMAP )
            {
                type = IMGTYPE_NORMAL;
                flags |= IMGFLAG_NOLIGHTSCALE;
                
                if( stage->type == ST_NORMALPARALLAXMAP )
                    type = IMGTYPE_NORMALHEIGHT;
            }
            else
            {
                if( r_genNormalMaps->integer )
                    flags |= IMGFLAG_GENNORMALMAP;
            }
            
            
            stage->bundle[0].image[0] = R_FindImageFile( token, type, flags );
            if( !stage->bundle[0].image[0] )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
                return false;
            }
        }
        //
        // lightmap <name>
        //
        else if( !Q_stricmp( token, "lightmap" ) )
        {
            token = COM_ParseExt( text, false );
            if( !token[0] )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing parameter for 'lightmap' keyword in shader '%s'\n", shader.name );
                return false;
            }
            
//----(SA)	fixes startup error and allows polygon shadows to work again
            if( !Q_stricmp( token, "$whiteimage" ) || !Q_stricmp( token, "*white" ) )
            {
//----(SA)	end
                stage->bundle[0].image[0] = tr.whiteImage;
                continue;
            }
//----(SA) added
            else if( !Q_stricmp( token, "$dlight" ) )
            {
                stage->bundle[0].image[0] = tr.dlightImage;
                continue;
            }
//----(SA) end
            else if( !Q_stricmp( token, "$lightmap" ) )
            {
                stage->bundle[0].isLightmap = true;
                if( shader.lightmapIndex < 0 )
                {
                    stage->bundle[0].image[0] = tr.whiteImage;
                }
                else
                {
                    stage->bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex];
                }
                continue;
            }
            else
            {
                //stage->bundle[0].image[0] = R_FindImageFile( token, false, qfalse, GL_CLAMP, true );
                stage->bundle[0].image[0] = R_FindImageFile( token, IMGTYPE_COLORALPHA, GL_CLAMP );
                if( !stage->bundle[0].image[0] )
                {
                    CL_RefPrintf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
                    return false;
                }
                stage->bundle[0].isLightmap = true;
            }
        }
        //
        // animMap <frequency> <image1> .... <imageN>
        //
        else if( !Q_stricmp( token, "animMap" ) )
        {
            S32 totalImages = 0;
            
            token = COM_ParseExt2( text, false );
            if( !token[0] )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing parameter for 'animMap' keyword in shader '%s'\n", shader.name );
                return false;
            }
            stage->bundle[0].imageAnimationSpeed = atof( token );
            
            // parse up to MAX_IMAGE_ANIMATIONS animations
            while( 1 )
            {
                S32		num;
                
                token = COM_ParseExt2( text, false );
                if( !token[0] )
                {
                    break;
                }
                num = stage->bundle[0].numImageAnimations;
                if( num < MAX_IMAGE_ANIMATIONS )
                {
                    S32/*imgFlags_t*/ flags = IMGFLAG_NONE;
                    
                    if( !shader.noMipMaps )
                        flags |= IMGFLAG_MIPMAP;
                        
                    if( !shader.noPicMip )
                        flags |= IMGFLAG_PICMIP;
                        
                    stage->bundle[0].image[num] = R_FindImageFile( token, IMGTYPE_COLORALPHA, flags );
                    if( !stage->bundle[0].image[num] )
                    {
                        CL_RefPrintf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
                        return false;
                    }
                    stage->bundle[0].numImageAnimations++;
                }
                totalImages++;
            }
            
            if( totalImages > MAX_IMAGE_ANIMATIONS )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: ignoring excess images for 'animMap' (found %d, max is %d) in shader '%s'\n",
                              totalImages, MAX_IMAGE_ANIMATIONS, shader.name );
            }
        }
        else if( !Q_stricmp( token, "videoMap" ) )
        {
            token = COM_ParseExt2( text, false );
            if( !token[0] )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing parameter for 'videoMap' keyword in shader '%s'\n", shader.name );
                return false;
            }
            stage->bundle[0].videoMapHandle = CIN_PlayCinematic( token, 0, 0, 256, 256, ( CIN_loop | CIN_silent | CIN_shader ) );
            if( stage->bundle[0].videoMapHandle != -1 )
            {
                stage->bundle[0].isVideoMap = true;
                stage->bundle[0].image[0] = tr.scratchImage[stage->bundle[0].videoMapHandle];
            }
            else
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: could not load '%s' for 'videoMap' keyword in shader '%s'\n", token, shader.name );
            }
        }
        //
        // alphafunc <func>
        //
        else if( !Q_stricmp( token, "alphaFunc" ) )
        {
            token = COM_ParseExt2( text, false );
            if( !token[0] )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing parameter for 'alphaFunc' keyword in shader '%s'\n", shader.name );
                return false;
            }
            
            atestBits = NameToAFunc( token );
        }
        //
        // depthFunc <func>
        //
        else if( !Q_stricmp( token, "depthfunc" ) )
        {
            token = COM_ParseExt2( text, false );
            
            if( !token[0] )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing parameter for 'depthfunc' keyword in shader '%s'\n", shader.name );
                return false;
            }
            
            if( !Q_stricmp( token, "lequal" ) )
            {
                depthFuncBits = 0;
            }
            else if( !Q_stricmp( token, "equal" ) )
            {
                depthFuncBits = GLS_DEPTHFUNC_EQUAL;
            }
            else
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: unknown depthfunc '%s' in shader '%s'\n", token, shader.name );
                continue;
            }
        }
        //
        // detail
        //
        else if( !Q_stricmp( token, "detail" ) )
        {
            stage->isDetail = true;
        }
        else if( !Q_stricmp( token, "fog" ) )
        {
            token = COM_ParseExt( text, false );
            if( token[0] == 0 )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing parm for fog in shader '%s'\n", shader.name );
                continue;
            }
            //if ( !Q_stricmp( token, "on" ) ) {
            //	stage->isFogged = true;
            //} else {
            //	stage->isFogged = false;
        }
        
        //
        // blendfunc <srcFactor> <dstFactor>
        // or blendfunc <add|filter|blend>
        //
        else if( !Q_stricmp( token, "blendfunc" ) )
        {
            token = COM_ParseExt2( text, false );
            if( token[0] == 0 )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name );
                continue;
            }
            // check for "simple" blends first
            if( !Q_stricmp( token, "add" ) )
            {
                blendSrcBits = GLS_SRCBLEND_ONE;
                blendDstBits = GLS_DSTBLEND_ONE;
            }
            else if( !Q_stricmp( token, "filter" ) )
            {
                blendSrcBits = GLS_SRCBLEND_DST_COLOR;
                blendDstBits = GLS_DSTBLEND_ZERO;
            }
            else if( !Q_stricmp( token, "blend" ) )
            {
                blendSrcBits = GLS_SRCBLEND_SRC_ALPHA;
                blendDstBits = GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
            }
            else
            {
                // complex double blends
                blendSrcBits = NameToSrcBlendMode( token );
                
                token = COM_ParseExt2( text, false );
                if( token[0] == 0 )
                {
                    CL_RefPrintf( PRINT_WARNING, "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name );
                    continue;
                }
                blendDstBits = NameToDstBlendMode( token );
            }
            
            // clear depth mask for blended surfaces
            if( !depthMaskExplicit )
            {
                depthMaskBits = 0;
            }
        }
        //
        // stage <type>
        //
        else if( !Q_stricmp( token, "stage" ) )
        {
            token = COM_ParseExt2( text, false );
            if( token[0] == 0 )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing parameters for stage in shader '%s'\n", shader.name );
                continue;
            }
            
            if( !Q_stricmp( token, "diffuseMap" ) )
            {
                stage->type = ST_DIFFUSEMAP;
            }
            else if( !Q_stricmp( token, "normalMap" ) || !Q_stricmp( token, "bumpMap" ) )
            {
                stage->type = ST_NORMALMAP;
                VectorSet4( stage->normalScale, r_baseNormalX->value, r_baseNormalY->value, 1.0f, r_baseParallax->value );
            }
            else if( !Q_stricmp( token, "normalParallaxMap" ) || !Q_stricmp( token, "bumpParallaxMap" ) )
            {
                if( r_parallaxMapping->integer )
                    stage->type = ST_NORMALPARALLAXMAP;
                else
                    stage->type = ST_NORMALMAP;
                VectorSet4( stage->normalScale, r_baseNormalX->value, r_baseNormalY->value, 1.0f, r_baseParallax->value );
            }
            else if( !Q_stricmp( token, "specularMap" ) )
            {
                stage->type = ST_SPECULARMAP;
                VectorSet4( stage->specularScale, 1.0f, 1.0f, 1.0f, 1.0f );
            }
            else
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: unknown stage parameter '%s' in shader '%s'\n", token, shader.name );
                continue;
            }
        }
        //
        // specularReflectance <value>
        //
        else if( !Q_stricmp( token, "specularreflectance" ) )
        {
            token = COM_ParseExt2( text, false );
            if( token[0] == 0 )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing parameter for specular reflectance in shader '%s'\n", shader.name );
                continue;
            }
            
            if( r_pbr->integer )
            {
                // interpret specularReflectance < 0.5 as nonmetal
                stage->specularScale[1] = atof( token ); // < 0.5f) ? 0.0f : 1.0f;
            }
            else
            {
                stage->specularScale[0] =
                    stage->specularScale[1] =
                        stage->specularScale[2] = atof( token );
            }
        }
        //
        // specularExponent <value>
        //
        else if( !Q_stricmp( token, "specularexponent" ) )
        {
            F32 exponent;
            
            token = COM_ParseExt2( text, false );
            if( token[0] == 0 )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing parameter for specular exponent in shader '%s'\n", shader.name );
                continue;
            }
            
            exponent = atof( token );
            
            if( r_pbr->integer )
                stage->specularScale[0] = 1.0f - powf( 2.0f / ( exponent + 2.0 ), 0.25 );
            else
            {
                // Change shininess to gloss
                // Assumes max exponent of 8190 and min of 0, must change here if altered in lightall_fp.glsl
                exponent = CLAMP( exponent, 0.0f, 8190.0f );
                stage->specularScale[3] = ( log2f( exponent + 2.0f ) - 1.0f ) / 12.0f;
            }
        }
        //
        // gloss <value>
        //
        else if( !Q_stricmp( token, "gloss" ) )
        {
            F32 gloss;
            
            token = COM_ParseExt2( text, false );
            if( token[0] == 0 )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing parameter for gloss in shader '%s'\n", shader.name );
                continue;
            }
            
            gloss = atof( token );
            
            if( r_pbr->integer )
            {
                stage->specularScale[0] = 1.0 - gloss;
            }
            else
            {
                stage->specularScale[3] = gloss;
            }
        }
        //
        // roughness <value>
        //
        else if( !Q_stricmp( token, "roughness" ) )
        {
            F32 roughness;
            
            token = COM_ParseExt2( text, false );
            if( token[0] == 0 )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing parameter for roughness in shader '%s'\n", shader.name );
                continue;
            }
            
            roughness = atof( token );
            
            if( r_pbr->integer )
                stage->specularScale[0] = roughness;
            else
            {
                if( roughness >= 0.125 )
                {
                    stage->specularScale[3] = log2f( 1.0f / roughness ) / 3.0f;
                }
                else
                {
                    stage->specularScale[3] = 1.0f;
                }
            }
        }
        //
        // parallaxDepth <value>
        //
        else if( !Q_stricmp( token, "parallaxdepth" ) )
        {
            token = COM_ParseExt2( text, false );
            if( token[0] == 0 )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing parameter for parallaxDepth in shader '%s'\n", shader.name );
                continue;
            }
            
            stage->normalScale[3] = atof( token );
        }
        //
        // normalScale <xy>
        // or normalScale <x> <y>
        // or normalScale <x> <y> <height>
        //
        else if( !Q_stricmp( token, "normalscale" ) )
        {
            token = COM_ParseExt2( text, false );
            if( token[0] == 0 )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing parameter for normalScale in shader '%s'\n", shader.name );
                continue;
            }
            
            stage->normalScale[0] = atof( token );
            
            token = COM_ParseExt2( text, false );
            if( token[0] == 0 )
            {
                // one value, applies to X/Y
                stage->normalScale[1] = stage->normalScale[0];
                continue;
            }
            
            stage->normalScale[1] = atof( token );
            
            token = COM_ParseExt2( text, false );
            if( token[0] == 0 )
            {
                // two values, no height
                continue;
            }
            
            stage->normalScale[3] = atof( token );
        }
        //
        // specularScale <rgb> <gloss>
        // or specularScale <metallic> <smoothness> with r_pbr 1
        // or specularScale <r> <g> <b>
        // or specularScale <r> <g> <b> <gloss>
        //
        else if( !Q_stricmp( token, "specularscale" ) )
        {
            token = COM_ParseExt2( text, false );
            if( token[0] == 0 )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing parameter for specularScale in shader '%s'\n", shader.name );
                continue;
            }
            
            stage->specularScale[0] = atof( token );
            
            token = COM_ParseExt2( text, false );
            if( token[0] == 0 )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing parameter for specularScale in shader '%s'\n", shader.name );
                continue;
            }
            
            stage->specularScale[1] = atof( token );
            
            token = COM_ParseExt2( text, false );
            if( token[0] == 0 )
            {
                if( r_pbr->integer )
                {
                    // two values, metallic then smoothness
                    F32 smoothness = stage->specularScale[1];
                    stage->specularScale[1] = ( stage->specularScale[0] < 0.5f ) ? 0.0f : 1.0f;
                    stage->specularScale[0] = smoothness;
                }
                else
                {
                    // two values, rgb then gloss
                    stage->specularScale[3] = stage->specularScale[1];
                    stage->specularScale[1] =
                        stage->specularScale[2] = stage->specularScale[0];
                }
                continue;
            }
            
            stage->specularScale[2] = atof( token );
            
            token = COM_ParseExt2( text, false );
            if( token[0] == 0 )
            {
                // three values, rgb
                continue;
            }
            
            stage->specularScale[3] = atof( token );
            
        }
        //
        // rgbGen
        //
        else if( !Q_stricmp( token, "rgbGen" ) )
        {
            token = COM_ParseExt2( text, false );
            if( token[0] == 0 )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing parameters for rgbGen in shader '%s'\n", shader.name );
                continue;
            }
            
            if( !Q_stricmp( token, "wave" ) )
            {
                ParseWaveForm( text, &stage->rgbWave );
                stage->rgbGen = CGEN_WAVEFORM;
            }
            else if( !Q_stricmp( token, "const" ) )
            {
                vec3_t	color;
                
                VectorClear( color );
                
                ParseVector( text, 3, color );
                stage->constantColor[0] = 255 * color[0];
                stage->constantColor[1] = 255 * color[1];
                stage->constantColor[2] = 255 * color[2];
                
                stage->rgbGen = CGEN_CONST;
            }
            else if( !Q_stricmp( token, "identity" ) )
            {
                stage->rgbGen = CGEN_IDENTITY;
            }
            else if( !Q_stricmp( token, "identityLighting" ) )
            {
                stage->rgbGen = CGEN_IDENTITY_LIGHTING;
            }
            else if( !Q_stricmp( token, "entity" ) )
            {
                stage->rgbGen = CGEN_ENTITY;
            }
            else if( !Q_stricmp( token, "oneMinusEntity" ) )
            {
                stage->rgbGen = CGEN_ONE_MINUS_ENTITY;
            }
            else if( !Q_stricmp( token, "vertex" ) )
            {
                stage->rgbGen = CGEN_VERTEX;
                if( stage->alphaGen == 0 )
                {
                    stage->alphaGen = AGEN_VERTEX;
                }
            }
            else if( !Q_stricmp( token, "exactVertex" ) )
            {
                stage->rgbGen = CGEN_EXACT_VERTEX;
            }
            else if( !Q_stricmp( token, "vertexLit" ) )
            {
                stage->rgbGen = CGEN_VERTEX_LIT;
                if( stage->alphaGen == 0 )
                {
                    stage->alphaGen = AGEN_VERTEX;
                }
            }
            else if( !Q_stricmp( token, "exactVertexLit" ) )
            {
                stage->rgbGen = CGEN_EXACT_VERTEX_LIT;
            }
            else if( !Q_stricmp( token, "lightingDiffuse" ) )
            {
                stage->rgbGen = CGEN_LIGHTING_DIFFUSE;
            }
            else if( !Q_stricmp( token, "oneMinusVertex" ) )
            {
                stage->rgbGen = CGEN_ONE_MINUS_VERTEX;
            }
            else
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: unknown rgbGen parameter '%s' in shader '%s'\n", token, shader.name );
                continue;
            }
        }
        //
        // alphaGen
        //
        else if( !Q_stricmp( token, "alphaGen" ) )
        {
            token = COM_ParseExt2( text, false );
            if( token[0] == 0 )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing parameters for alphaGen in shader '%s'\n", shader.name );
                continue;
            }
            
            if( !Q_stricmp( token, "wave" ) )
            {
                ParseWaveForm( text, &stage->alphaWave );
                stage->alphaGen = AGEN_WAVEFORM;
            }
            else if( !Q_stricmp( token, "const" ) )
            {
                token = COM_ParseExt2( text, false );
                stage->constantColor[3] = 255 * atof( token );
                stage->alphaGen = AGEN_CONST;
            }
            else if( !Q_stricmp( token, "identity" ) )
            {
                stage->alphaGen = AGEN_IDENTITY;
            }
            else if( !Q_stricmp( token, "entity" ) )
            {
                stage->alphaGen = AGEN_ENTITY;
            }
            else if( !Q_stricmp( token, "oneMinusEntity" ) )
            {
                stage->alphaGen = AGEN_ONE_MINUS_ENTITY;
            }
            else if( !Q_stricmp( token, "vertex" ) )
            {
                stage->alphaGen = AGEN_VERTEX;
            }
            else if( !Q_stricmp( token, "lightingSpecular" ) )
            {
                stage->alphaGen = AGEN_LIGHTING_SPECULAR;
            }
            else if( !Q_stricmp( token, "oneMinusVertex" ) )
            {
                stage->alphaGen = AGEN_ONE_MINUS_VERTEX;
            }
            else if( !Q_stricmp( token, "portal" ) )
            {
                stage->alphaGen = AGEN_PORTAL;
                token = COM_ParseExt2( text, false );
                if( token[0] == 0 )
                {
                    shader.portalRange = 256;
                    CL_RefPrintf( PRINT_WARNING, "WARNING: missing range parameter for alphaGen portal in shader '%s', defaulting to 256\n", shader.name );
                }
                else
                {
                    shader.portalRange = atof( token );
                }
            }
            else
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: unknown alphaGen parameter '%s' in shader '%s'\n", token, shader.name );
                continue;
            }
        }
        //
        // tcGen <function>
        //
        else if( !Q_stricmp( token, "texgen" ) || !Q_stricmp( token, "tcGen" ) )
        {
            token = COM_ParseExt2( text, false );
            if( token[0] == 0 )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing texgen parm in shader '%s'\n", shader.name );
                continue;
            }
            
            if( !Q_stricmp( token, "environment" ) )
            {
                stage->bundle[0].tcGen = TCGEN_ENVIRONMENT_MAPPED;
            }
            else if( !Q_stricmp( token, "lightmap" ) )
            {
                stage->bundle[0].tcGen = TCGEN_LIGHTMAP;
            }
            else if( !Q_stricmp( token, "texture" ) || !Q_stricmp( token, "base" ) )
            {
                stage->bundle[0].tcGen = TCGEN_TEXTURE;
            }
            else if( !Q_stricmp( token, "vector" ) )
            {
                ParseVector( text, 3, stage->bundle[0].tcGenVectors[0] );
                ParseVector( text, 3, stage->bundle[0].tcGenVectors[1] );
                
                stage->bundle[0].tcGen = TCGEN_VECTOR;
            }
            else
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: unknown texgen parm in shader '%s'\n", shader.name );
            }
        }
        //
        // tcMod <type> <...>
        //
        else if( !Q_stricmp( token, "tcMod" ) )
        {
            UTF8 buffer[1024] = "";
            
            while( 1 )
            {
                token = COM_ParseExt2( text, false );
                if( token[0] == 0 )
                    break;
                Q_strcat( buffer, sizeof( buffer ), token );
                Q_strcat( buffer, sizeof( buffer ), " " );
            }
            
            ParseTexMod( buffer, stage );
            
            continue;
        }
        //
        // depthmask
        //
        else if( !Q_stricmp( token, "depthwrite" ) )
        {
            depthMaskBits = GLS_DEPTHMASK_TRUE;
            depthMaskExplicit = true;
            
            continue;
        }
        else
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: unknown parameter '%s' in shader '%s'\n", token, shader.name );
            return false;
        }
    }
    
    //
    // if cgen isn't explicitly specified, use either identity or identitylighting
    //
    if( stage->rgbGen == CGEN_BAD )
    {
        if( blendSrcBits == 0 ||
                blendSrcBits == GLS_SRCBLEND_ONE ||
                blendSrcBits == GLS_SRCBLEND_SRC_ALPHA )
        {
            stage->rgbGen = CGEN_IDENTITY_LIGHTING;
        }
        else
        {
            stage->rgbGen = CGEN_IDENTITY;
        }
    }
    
    
    //
    // implicitly assume that a GL_ONE GL_ZERO blend mask disables blending
    //
    if( ( blendSrcBits == GLS_SRCBLEND_ONE ) &&
            ( blendDstBits == GLS_DSTBLEND_ZERO ) )
    {
        blendDstBits = blendSrcBits = 0;
        depthMaskBits = GLS_DEPTHMASK_TRUE;
    }
    
    // decide which agens we can skip
    if( stage->alphaGen == AGEN_IDENTITY )
    {
        if( stage->rgbGen == CGEN_IDENTITY
                || stage->rgbGen == CGEN_LIGHTING_DIFFUSE )
        {
            stage->alphaGen = AGEN_SKIP;
        }
    }
    
    //
    // compute state bits
    //
    stage->stateBits = depthMaskBits |
                       blendSrcBits | blendDstBits |
                       atestBits |
                       depthFuncBits;
                       
    return true;
}

/*
===============
ParseDeform

deformVertexes wave <spread> <waveform> <base> <amplitude> <phase> <frequency>
deformVertexes normal <frequency> <amplitude>
deformVertexes move <vector> <waveform> <base> <amplitude> <phase> <frequency>
deformVertexes bulge <bulgeWidth> <bulgeHeight> <bulgeSpeed>
deformVertexes projectionShadow
deformVertexes autoSprite
deformVertexes autoSprite2
deformVertexes text[0-7]
===============
*/
static void ParseDeform( UTF8** text )
{
    UTF8*	token;
    deformStage_t*	ds;
    
    token = COM_ParseExt2( text, false );
    if( token[0] == 0 )
    {
        CL_RefPrintf( PRINT_WARNING, "WARNING: missing deform parm in shader '%s'\n", shader.name );
        return;
    }
    
    if( shader.numDeforms == MAX_SHADER_DEFORMS )
    {
        CL_RefPrintf( PRINT_WARNING, "WARNING: MAX_SHADER_DEFORMS in '%s'\n", shader.name );
        return;
    }
    
    ds = &shader.deforms[ shader.numDeforms ];
    shader.numDeforms++;
    
    if( !Q_stricmp( token, "projectionShadow" ) )
    {
        ds->deformation = DEFORM_PROJECTION_SHADOW;
        return;
    }
    
    if( !Q_stricmp( token, "autosprite" ) )
    {
        ds->deformation = DEFORM_AUTOSPRITE;
        return;
    }
    
    if( !Q_stricmp( token, "autosprite2" ) )
    {
        ds->deformation = DEFORM_AUTOSPRITE2;
        return;
    }
    
    if( !Q_stricmpn( token, "text", 4 ) )
    {
        S32		n;
        
        n = token[4] - '0';
        if( n < 0 || n > 7 )
        {
            n = 0;
        }
        ds->deformation = ( deform_t )( DEFORM_TEXT0 + n );
        return;
    }
    
    if( !Q_stricmp( token, "bulge" ) )
    {
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
            return;
        }
        ds->bulgeWidth = atof( token );
        
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
            return;
        }
        ds->bulgeHeight = atof( token );
        
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
            return;
        }
        ds->bulgeSpeed = atof( token );
        
        ds->deformation = DEFORM_BULGE;
        return;
    }
    
    if( !Q_stricmp( token, "wave" ) )
    {
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
            return;
        }
        
        if( atof( token ) != 0 )
        {
            ds->deformationSpread = 1.0f / atof( token );
        }
        else
        {
            ds->deformationSpread = 100.0f;
            CL_RefPrintf( PRINT_WARNING, "WARNING: illegal div value of 0 in deformVertexes command for shader '%s'\n", shader.name );
        }
        
        ParseWaveForm( text, &ds->deformationWave );
        ds->deformation = DEFORM_WAVE;
        return;
    }
    
    if( !Q_stricmp( token, "normal" ) )
    {
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
            return;
        }
        ds->deformationWave.amplitude = atof( token );
        
        token = COM_ParseExt2( text, false );
        if( token[0] == 0 )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
            return;
        }
        ds->deformationWave.frequency = atof( token );
        
        ds->deformation = DEFORM_NORMALS;
        return;
    }
    
    if( !Q_stricmp( token, "move" ) )
    {
        S32		i;
        
        for( i = 0 ; i < 3 ; i++ )
        {
            token = COM_ParseExt2( text, false );
            if( token[0] == 0 )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
                return;
            }
            ds->moveVector[i] = atof( token );
        }
        
        ParseWaveForm( text, &ds->deformationWave );
        ds->deformation = DEFORM_MOVE;
        return;
    }
    
    CL_RefPrintf( PRINT_WARNING, "WARNING: unknown deformVertexes subtype '%s' found in shader '%s'\n", token, shader.name );
}


/*
===============
ParseSkyParms

skyParms <outerbox> <cloudheight> <innerbox>
===============
*/
static void ParseSkyParms( UTF8** text )
{
    UTF8*		token;
    static StringEntry suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};
    UTF8		pathname[MAX_QPATH];
    S32			i;
    S32/*imgFlags_t*/ imgFlags = IMGFLAG_MIPMAP | IMGFLAG_PICMIP;
    
    // outerbox
    token = COM_ParseExt2( text, false );
    if( token[0] == 0 )
    {
        CL_RefPrintf( PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
        return;
    }
    if( strcmp( token, "-" ) )
    {
        for( i = 0 ; i < 6 ; i++ )
        {
            Com_sprintf( pathname, sizeof( pathname ), "%s_%s.tga"
                         , token, suf[i] );
            shader.sky.outerbox[i] = R_FindImageFile( const_cast< UTF8* >( pathname ), IMGTYPE_COLORALPHA, imgFlags | IMGFLAG_CLAMPTOEDGE );
            
            if( !shader.sky.outerbox[i] )
            {
                shader.sky.outerbox[i] = tr.defaultImage;
            }
        }
    }
    
    // cloudheight
    token = COM_ParseExt2( text, false );
    if( token[0] == 0 )
    {
        CL_RefPrintf( PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
        return;
    }
    shader.sky.cloudHeight = atof( token );
    if( !shader.sky.cloudHeight )
    {
        shader.sky.cloudHeight = 512;
    }
    R_InitSkyTexCoords( shader.sky.cloudHeight );
    
    
    // innerbox
    token = COM_ParseExt2( text, false );
    if( token[0] == 0 )
    {
        CL_RefPrintf( PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
        return;
    }
    if( strcmp( token, "-" ) )
    {
        for( i = 0 ; i < 6 ; i++ )
        {
            Com_sprintf( pathname, sizeof( pathname ), "%s_%s.tga"
                         , token, suf[i] );
            shader.sky.innerbox[i] = R_FindImageFile( const_cast< UTF8* >( pathname ), IMGTYPE_COLORALPHA, imgFlags );
            if( !shader.sky.innerbox[i] )
            {
                shader.sky.innerbox[i] = tr.defaultImage;
            }
        }
    }
    
    shader.isSky = true;
}


/*
=================
ParseSort
=================
*/
void ParseSort( UTF8** text )
{
    UTF8*	token;
    
    token = COM_ParseExt2( text, false );
    if( token[0] == 0 )
    {
        CL_RefPrintf( PRINT_WARNING, "WARNING: missing sort parameter in shader '%s'\n", shader.name );
        return;
    }
    
    if( !Q_stricmp( token, "portal" ) )
    {
        shader.sort = SS_PORTAL;
    }
    else if( !Q_stricmp( token, "sky" ) )
    {
        shader.sort = SS_ENVIRONMENT;
    }
    else if( !Q_stricmp( token, "opaque" ) )
    {
        shader.sort = SS_OPAQUE;
    }
    else if( !Q_stricmp( token, "decal" ) )
    {
        shader.sort = SS_DECAL;
    }
    else if( !Q_stricmp( token, "seeThrough" ) )
    {
        shader.sort = SS_SEE_THROUGH;
    }
    else if( !Q_stricmp( token, "banner" ) )
    {
        shader.sort = SS_BANNER;
    }
    else if( !Q_stricmp( token, "additive" ) )
    {
        shader.sort = SS_BLEND1;
    }
    else if( !Q_stricmp( token, "nearest" ) )
    {
        shader.sort = SS_NEAREST;
    }
    else if( !Q_stricmp( token, "underwater" ) )
    {
        shader.sort = SS_UNDERWATER;
    }
    else
    {
        shader.sort = atof( token );
    }
}

/*
=================
ParseMaterial
=================
*/
StringEntry materialNames[MATERIAL_LAST] =
{
    MATERIALS
};

void ParseMaterial( UTF8** text )
{
    S32 i;
    UTF8* token;
    
    token = COM_ParseExt2( text, false );
    if( token[0] == 0 )
    {
        Com_Printf( S_COLOR_YELLOW  "WARNING: missing material in shader '%s'\n", shader.name );
        return;
    }
    
    for( i = 0; i < MATERIAL_LAST; i++ )
    {
        if( !Q_stricmp( token, materialNames[i] ) )
        {
            //shader.surfaceFlags |= i;
            shader.materialType = i;
            break;
        }
    }
}


// this table is also present in q3map

typedef struct
{
    StringEntry name;
    U32 clearSolid, surfaceFlags, contents;
} infoParm_t;

infoParm_t	infoParms[] =
{
    // server relevant contents
    {"water",		1,	0,	CONTENTS_WATER },
    {"slime",		1,	0,	CONTENTS_SLIME },		// mildly damaging
    {"lava",		1,	0,	CONTENTS_LAVA },		// very damaging
    {"playerclip",	1,	0,	CONTENTS_PLAYERCLIP },
    {"monsterclip",	1,	0,	CONTENTS_MONSTERCLIP },
    {"nodrop",		1,	0,	CONTENTS_NODROP },		// don't drop items or leave bodies (death fog, lava, etc)
    {"nonsolid",	1,	SURF_NONSOLID,	0},						// clears the solid flag
    
    // utility relevant attributes
    {"origin",		1,	0,	CONTENTS_ORIGIN },		// center of rotating brushes
    {"trans",		0,	0,	CONTENTS_TRANSLUCENT },	// don't eat contained surfaces
    {"detail",		0,	0,	CONTENTS_DETAIL },		// don't include in structural bsp
    {"structural",	0,	0,	CONTENTS_STRUCTURAL },	// force into structural bsp even if trnas
    {"areaportal",	1,	0,	CONTENTS_AREAPORTAL },	// divides areas
    {"clusterportal", 1, 0,  CONTENTS_CLUSTERPORTAL },	// for bots
    {"donotenter",  1,  0,  CONTENTS_DONOTENTER },		// for bots
    
    {"fog",			1,	0,	CONTENTS_FOG},			// carves surfaces entering
    {"sky",			0,	SURF_SKY,		0 },		// emit light from an environment map
    {"lightfilter",	0,	SURF_LIGHTFILTER, 0 },		// filter light going through it
    {"alphashadow",	0,	SURF_ALPHASHADOW, 0 },		// test light on a per-pixel basis
    {"hint",		0,	SURF_HINT,		0 },		// use as a primary splitter
    
    // server attributes
    {"slick",		0,	SURF_SLICK,		0 },
    {"noimpact",	0,	SURF_NOIMPACT,	0 },		// don't make impact explosions or marks
    {"nomarks",		0,	SURF_NOMARKS,	0 },		// don't make impact marks, but still explode
    {"ladder",		0,	SURF_LADDER,	0 },
    {"nodamage",	0,	SURF_NODAMAGE,	0 },
    {"metalsteps",	0,	SURF_METALSTEPS, 0 },
    {"flesh",		0,	SURF_FLESH,		0 },
    {"nosteps",		0,	SURF_NOSTEPS,	0 },
    
    // drawsurf attributes
    {"nodraw",		0,	SURF_NODRAW,	0 },	// don't generate a drawsurface (or a lightmap)
    {"pointlight",	0,	SURF_POINTLIGHT, 0 },	// sample lighting at vertexes
    {"nolightmap",	0,	SURF_NOLIGHTMAP, 0 },	// don't generate a lightmap
    {"nodlight",	0,	SURF_NODLIGHT, 0 },		// don't ever add dynamic lights
    {"dust",		0,	SURF_DUST, 0}			// leave a dust trail when walking on this surface
};


/*
===============
ParseSurfaceParm

surfaceparm <name>
===============
*/
static void ParseSurfaceParm( UTF8** text )
{
    UTF8*	token;
    S32		numInfoParms = ARRAY_LEN( infoParms );
    S32		i;
    
    token = COM_ParseExt2( text, false );
    for( i = 0 ; i < numInfoParms ; i++ )
    {
        if( !Q_stricmp( token, infoParms[i].name ) )
        {
            shader.surfaceFlags |= infoParms[i].surfaceFlags;
            shader.contentFlags |= infoParms[i].contents;
#if 0
            if( infoParms[i].clearSolid )
            {
                si->contents &= ~CONTENTS_SOLID;
            }
#endif
            break;
        }
    }
}

static bool HaveSurfaceType( S32 surfaceFlags )
{
    switch( surfaceFlags & MATERIAL_MASK )
    {
        case MATERIAL_WATER:			// 13			// light covering of water on a surface
        case MATERIAL_SHORTGRASS:		// 5			// manicured lawn
        case MATERIAL_LONGGRASS:		// 6			// long jungle grass
        case MATERIAL_SAND:				// 8			// sandy beach
        case MATERIAL_CARPET:			// 27			// lush carpet
        case MATERIAL_GRAVEL:			// 9			// lots of small stones
        case MATERIAL_ROCK:				// 23			//
        case MATERIAL_TILES:			// 26			// tiled floor
        case MATERIAL_SOLIDWOOD:		// 1			// freshly cut timber
        case MATERIAL_HOLLOWWOOD:		// 2			// termite infested creaky wood
        case MATERIAL_SOLIDMETAL:		// 3			// solid girders
        case MATERIAL_HOLLOWMETAL:		// 4			// hollow metal machines
        case MATERIAL_DRYLEAVES:		// 19			// dried up leaves on the floor
        case MATERIAL_GREENLEAVES:		// 20			// fresh leaves still on a tree
        case MATERIAL_FABRIC:			// 21			// Cotton sheets
        case MATERIAL_CANVAS:			// 22			// tent material
        case MATERIAL_MARBLE:			// 12			// marble floors
        case MATERIAL_SNOW:				// 14			// freshly laid snow
        case MATERIAL_MUD:				// 17			// wet soil
        case MATERIAL_DIRT:				// 7			// hard mud
        case MATERIAL_CONCRETE:			// 11			// hardened concrete pavement
        case MATERIAL_FLESH:			// 16			// hung meat, corpses in the world
        case MATERIAL_RUBBER:			// 24			// hard tire like rubber
        case MATERIAL_PLASTIC:			// 25			//
        case MATERIAL_PLASTER:			// 28			// drywall style plaster
        case MATERIAL_SHATTERGLASS:		// 29			// glass with the Crisis Zone style shattering
        case MATERIAL_ARMOR:			// 30			// body armor
        case MATERIAL_ICE:				// 15			// packed snow/solid ice
        case MATERIAL_GLASS:			// 10			//
        case MATERIAL_BPGLASS:			// 18			// bulletproof glass
        case MATERIAL_COMPUTER:			// 31			// computers/electronic equipment
            return true;
            break;
        default:
            break;
    }
    
    return false;
}

static bool StringsContainWord( StringEntry heystack, StringEntry heystack2, UTF8* needle )
{
    if( StringContainsWord( heystack, needle ) ) return true;
    if( StringContainsWord( heystack2, needle ) ) return true;
    return false;
}

/*
=================
ParseShader

The current text pointer is at the explicit text definition of the
shader.  Parse it into the global shader variable.  Later functions
will optimize it.
=================
*/
static bool ParseShader( StringEntry name, UTF8** text )
{
    UTF8* token;
    S32 s;
    
    s = 0;
    
    token = COM_ParseExt2( text, true );
    if( token[0] != '{' )
    {
        CL_RefPrintf( PRINT_WARNING, "WARNING: expecting '{', found '%s' instead in shader '%s'\n", token, shader.name );
        return false;
    }
    
    while( 1 )
    {
        token = COM_ParseExt2( text, true );
        if( !token[0] )
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: no concluding '}' in shader %s\n", shader.name );
            return false;
        }
        
        // end of shader definition
        if( token[0] == '}' )
        {
            break;
        }
        // stage definition
        else if( token[0] == '{' )
        {
            if( s >= lengthof( stages ) )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: too many stages in shader %s\n", shader.name );
                return false;
            }
            
            if( s >= MAX_SHADER_STAGES )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: too many stages in shader %s (max is %i)\n", shader.name, MAX_SHADER_STAGES );
                return false;
            }
            
            if( !ParseStage( &stages[s], text ) )
            {
                return false;
            }
            stages[s].active = true;
            s++;
            
            continue;
        }
        // skip stuff that only the QuakeEdRadient needs
        else if( !Q_stricmpn( token, "qer", 3 ) )
        {
            SkipRestOfLine( text );
            continue;
        }
        // material deprecated as of 11 Jan 01
        // material undeprecated as of 7 May 01 - q3map_material deprecated
        else if( !Q_stricmp( token, "material" ) || !Q_stricmp( token, "q3map_material" ) )
        {
            ParseMaterial( text );
            continue;
        }
        
        // sun parms
        else if( !Q_stricmp( token, "q3map_sun" ) || !Q_stricmp( token, "q3map_sunExt" ) || !Q_stricmp( token, "q3gl2_sun" ) )
        {
            F32	a, b;
            bool isGL2Sun = false;
            
            if( !Q_stricmp( token, "q3gl2_sun" ) && r_sunShadows->integer )
            {
                isGL2Sun = true;
                tr.sunShadows = true;
            }
            
            token = COM_ParseExt2( text, false );
            tr.sunLight[0] = atof( token );
            token = COM_ParseExt2( text, false );
            tr.sunLight[1] = atof( token );
            token = COM_ParseExt2( text, false );
            tr.sunLight[2] = atof( token );
            
            VectorNormalize( tr.sunLight );
            
            token = COM_ParseExt2( text, false );
            a = atof( token );
            VectorScale( tr.sunLight, a, tr.sunLight );
            
            token = COM_ParseExt2( text, false );
            a = atof( token );
            a = a / 180 * M_PI;
            
            token = COM_ParseExt2( text, false );
            b = atof( token );
            b = b / 180 * M_PI;
            
            tr.sunDirection[0] = cos( a ) * cos( b );
            tr.sunDirection[1] = sin( a ) * cos( b );
            tr.sunDirection[2] = sin( b );
            
            if( isGL2Sun )
            {
                token = COM_ParseExt2( text, false );
                tr.sunShadowScale = atof( token );
                
                // parse twice, since older shaders may include mapLightScale before sunShadowScale
                token = COM_ParseExt2( text, false );
                if( token[0] )
                    tr.sunShadowScale = atof( token );
            }
            
            SkipRestOfLine( text );
            continue;
        }
        // tonemap parms
        else if( !Q_stricmp( token, "q3gl2_tonemap" ) )
        {
            token = COM_ParseExt2( text, false );
            tr.toneMinAvgMaxLevel[0] = atof( token );
            token = COM_ParseExt2( text, false );
            tr.toneMinAvgMaxLevel[1] = atof( token );
            token = COM_ParseExt2( text, false );
            tr.toneMinAvgMaxLevel[2] = atof( token );
            
            token = COM_ParseExt2( text, false );
            tr.autoExposureMinMax[0] = atof( token );
            token = COM_ParseExt2( text, false );
            tr.autoExposureMinMax[1] = atof( token );
            
            SkipRestOfLine( text );
            continue;
        }
        // q3map_surfacelight deprecated as of 16 Jul 01
        else if( !Q_stricmp( token, "surfacelight" ) || !Q_stricmp( token, "q3map_surfacelight" ) )
        {
            SkipRestOfLine( text );
            continue;
        }
        else if( !Q_stricmp( token, "lightColor" ) )
        {
            SkipRestOfLine( text );
            continue;
        }
        else if( !Q_stricmp( token, "deformVertexes" ) )
        {
            ParseDeform( text );
            continue;
        }
        else if( !Q_stricmp( token, "tesssize" ) )
        {
            SkipRestOfLine( text );
            continue;
        }
        else if( !Q_stricmp( token, "clampTime" ) )
        {
            token = COM_ParseExt2( text, false );
            if( token[0] )
            {
                shader.clampTime = atof( token );
            }
        }
        // skip stuff that only the q3map needs
        else if( !Q_stricmpn( token, "q3map", 5 ) )
        {
            SkipRestOfLine( text );
            continue;
        }
        // skip stuff that only q3map or the server needs
        else if( !Q_stricmp( token, "surfaceParm" ) )
        {
            ParseSurfaceParm( text );
            continue;
        }
        // no mip maps
        else if( !Q_stricmp( token, "nomipmaps" ) )
        {
            shader.noMipMaps = true;
            shader.noPicMip = true;
            continue;
        }
        // no picmip adjustment
        else if( !Q_stricmp( token, "nopicmip" ) )
        {
            shader.noPicMip = true;
            continue;
        }
        // polygonOffset
        else if( !Q_stricmp( token, "polygonOffset" ) )
        {
            shader.polygonOffset = true;
            continue;
        }
        // entityMergable, allowing sprite surfaces from multiple entities
        // to be merged into one batch.  This is a savings for smoke
        // puffs and blood, but can't be used for anything where the
        // shader calcs (not the surface function) reference the entity color or scroll
        else if( !Q_stricmp( token, "entityMergable" ) )
        {
            shader.entityMergable = true;
            continue;
        }
        // fogParms
        else if( !Q_stricmp( token, "fogParms" ) )
        {
            if( !ParseVector( text, 3, shader.fogParms.color ) )
            {
                return false;
            }
            
            if( r_greyscale->integer )
            {
                F32 luminance;
                
                luminance = LUMA( shader.fogParms.color[0], shader.fogParms.color[1], shader.fogParms.color[2] );
                VectorSet( shader.fogParms.color, luminance, luminance, luminance );
            }
            else if( r_greyscale->value )
            {
                F32 luminance;
                
                luminance = LUMA( shader.fogParms.color[0], shader.fogParms.color[1], shader.fogParms.color[2] );
                shader.fogParms.color[0] = LERP( shader.fogParms.color[0], luminance, r_greyscale->value );
                shader.fogParms.color[1] = LERP( shader.fogParms.color[1], luminance, r_greyscale->value );
                shader.fogParms.color[2] = LERP( shader.fogParms.color[2], luminance, r_greyscale->value );
            }
            
            token = COM_ParseExt2( text, false );
            if( !token[0] )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing parm for 'fogParms' keyword in shader '%s'\n", shader.name );
                continue;
            }
            shader.fogParms.depthForOpaque = atof( token );
            
            // skip any old gradient directions
            SkipRestOfLine( text );
            continue;
        }
        // portal
        else if( !Q_stricmp( token, "portal" ) )
        {
            shader.sort = SS_PORTAL;
            shader.isPortal = true;
            continue;
        }
        // skyparms <cloudheight> <outerbox> <innerbox>
        else if( !Q_stricmp( token, "skyparms" ) )
        {
            ParseSkyParms( text );
            continue;
        }
        else if( !Q_stricmp( token, "waterfogvars" ) )
        {
            vec3_t watercolor;
            F32 fogvar;
            UTF8 fogString[64];
            
            if( !ParseVector( text, 3, watercolor ) )
            {
                return false;
            }
            token = COM_ParseExt( text, false );
            
            if( !token[0] )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing density/distance value for water fog\n" );
                continue;
            }
            
            fogvar = atof( token );
            
            //----(SA)	right now allow one water color per map.  I'm sure this will need
            //			to change at some point, but I'm not sure how to track fog parameters
            //			on a "per-water volume" basis yet.
            
            if( fogvar == 0 )        // '0' specifies "use the map values for everything except the fog color
            {
                // TODO
            }
            else if( fogvar > 1 )  // distance "linear" fog
            {
                Com_sprintf( fogString, sizeof( fogString ), "0 %d 1.1 %f %f %f 200", ( S32 )fogvar, watercolor[0], watercolor[1], watercolor[2] );
                //				R_SetFog(FOG_WATER, 0, fogvar, watercolor[0], watercolor[1], watercolor[2], 1.1);
            }
            else                        // density "exp" fog
            {
                Com_sprintf( fogString, sizeof( fogString ), "0 5 %f %f %f %f 200", fogvar, watercolor[0], watercolor[1], watercolor[2] );
                //				R_SetFog(FOG_WATER, 0, 5, watercolor[0], watercolor[1], watercolor[2], fogvar);
            }
            
            //		near
            //		far
            //		density
            //		r,g,b
            //		time to complete
            cvarSystem->Set( "r_waterFogColor", fogString );
            
            continue;
        }
        // fogvars
        else if( !Q_stricmp( token, "fogvars" ) )
        {
            vec3_t fogColor;
            F32 fogDensity;
            S32 fogFar;
            
            if( !ParseVector( text, 3, fogColor ) )
            {
                return false;
            }
            
            token = COM_ParseExt( text, false );
            if( !token[0] )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing density value for the fog\n" );
                continue;
            }
            
            
            //----(SA)	NOTE:	fogFar > 1 means the shader is setting the farclip, < 1 means setting
            //					density (so old maps or maps that just need softening fog don't have to care about farclip)
            
            fogDensity = atof( token );
            if( fogDensity >= 1 )  // linear
            {
                fogFar = fogDensity;
            }
            else
            {
                fogFar = 5;
            }
            
            //			R_SetFog(FOG_MAP, 0, fogFar, fogColor[0], fogColor[1], fogColor[2], fogDensity);
            cvarSystem->Set( "r_mapFogColor", va( "0 %d %f %f %f %f 0", fogFar, fogDensity, fogColor[0], fogColor[1], fogColor[2] ) );
            //			R_SetFog(FOG_CMD_SWITCHFOG, FOG_MAP, 50, 0, 0, 0, 0);
            
            continue;
        }
        else if( !Q_stricmp( token, "nofog" ) )
        {
            continue;
        }
        else if( !Q_stricmp( token, "allowcompress" ) )
        {
            continue;
        }
        else if( !Q_stricmp( token, "nocompress" ) )
        {
            continue;
        }
        // light <value> determines flaring in q3map, not needed here
        else if( !Q_stricmp( token, "light" ) )
        {
            COM_ParseExt2( text, false );
            continue;
        }
        // cull <face>
        else if( !Q_stricmp( token, "cull" ) )
        {
            token = COM_ParseExt2( text, false );
            if( token[0] == 0 )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: missing cull parms in shader '%s'\n", shader.name );
                continue;
            }
            
            if( !Q_stricmp( token, "none" ) || !Q_stricmp( token, "twosided" ) || !Q_stricmp( token, "disable" ) )
            {
                shader.cullType = CT_TWO_SIDED;
            }
            else if( !Q_stricmp( token, "back" ) || !Q_stricmp( token, "backside" ) || !Q_stricmp( token, "backsided" ) )
            {
                shader.cullType = CT_BACK_SIDED;
            }
            else
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: invalid cull parm '%s' in shader '%s'\n", token, shader.name );
            }
            continue;
        }
        else if( !Q_stricmp( token, "dpglossintensitymod" ) )
        {
            COM_ParseExt2( text, false );
            continue;
        }
        else if( !Q_stricmp( token, "dpglossexponentmod" ) )
        {
            COM_ParseExt2( text, false );
            continue;
        }
        else if( !Q_stricmp( token, "dpreflectcube" ) )
        {
            COM_ParseExt2( text, false );
            continue;
        }
        else if( !Q_stricmp( token, "q3map_bouncescale" ) )
        {
            COM_ParseExt2( text, false );
            continue;
        }
        //dpoffsetmapping - .05538845958254273813 match8 91.30766800000000000000
        //dpglossintensitymod  2
        //dpglossexponentmod  4
        //dpreflectcube env / exomorph / exomorph
        //q3map_bouncescale 1.25
        else if( !Q_stricmp( token, "dpoffsetmapping" ) )
        {
            UTF8* keyword = token;
            token = COM_ParseExt2( text, false );
            
            // in common case this is not used since heightmap is detected by engine
            //shade.heightMapInNormalMap = true;
            
            if( !token[0] )
            {
                // legacy “parallax” XreaL keyword was never used
                // it had purpose to enable parallax for the
                // current shader
                //
                // the engine also relied on this to know
                // that heightMap was stored in normalMap
                // but there was no other storage options
                //
                // since parallax is now enabled by default
                // when heightMap is present, do nothing
                continue;
            }
            
            if( !Q_stricmp( token, "none" ) )
            {
                //shader.noParallax = true;
            }
            else if( !Q_stricmp( token, "disable" ) )
            {
                //shader.noParallax = true;
            }
            else if( !Q_stricmp( token, "off" ) )
            {
                //shader.noParallax = true;
            }
            else if( !Q_stricmp( token, "default" ) )
            {
                // do nothing more
            }
            else if( !Q_stricmp( token, "normal" ) )
            {
                // do nothing more (same as default)
            }
            else if( !Q_stricmp( token, "linear" ) )
            {
                // not implemented yet
                //Log::Warn("unsupported parm for '%s' keyword in shader '%s'", keyword, shader.name);
            }
            else if( !Q_stricmp( token, "relief" ) )
            {
                // the only implemented algorithm
                // hence the default
                // do nothing more
            }
            else if( !Q_stricmp( token, "-" ) )
            {
                // do nothing, that's just a filler
            }
            else
            {
                //Log::Warn("invalid parm for '%s' keyword in shader '%s'", keyword, shader.name);
                SkipRestOfLine( text );
                continue;
            }
            
            token = COM_ParseExt2( text, false );
            
            if( !token[0] )
            {
                //Log::Warn("missing parm for '%s' keyword in shader '%s'", keyword, shader.name);
                continue;
            }
            
            //shader.offsetScale = atof(token);
            
            token = COM_ParseExt2( text, false );
            
            // dpoffsetmapping - 2
            if( !token[0] )
            {
                continue;
            }
            
            // dpoffsetmapping - 2 match8 65
            F32 off;
            F32 div;
            
            if( !Q_stricmp( token, "bias" ) )
            {
                off = 0.0f;
                div = 1.0f;
            }
            else if( !Q_stricmp( token, "match" ) )
            {
                off = 1.0f;
                div = 1.0f;
            }
            else if( !Q_stricmp( token, "match8" ) )
            {
                off = 1.0f;
                div = 255.0f;
            }
            else if( !Q_stricmp( token, "match16" ) )
            {
                off = 1.0f;
                div = 65535.0f;
            }
            else
            {
                //Log::Warn("invalid parm for '%s' keyword in shader '%s'", keyword, shader.name);
                SkipRestOfLine( text );
                continue;
            }
            
            token = COM_ParseExt2( text, false );
            
            if( !token[0] )
            {
                //Log::Warn("missing parm for '%s' keyword in shader '%s'", keyword, shader.name);
                continue;
            }
            
            //F32 bias = atof(token);
            // shader.offsetBias = off - bias / div;
            continue;
        }
        // sort
        else if( !Q_stricmp( token, "sort" ) )
        {
            ParseSort( text );
            continue;
        }
        // ydnar: implicit default mapping to eliminate redundant/incorrect explicit shader stages
        else if( !Q_stricmpn( token, "implicit", 8 ) )
        {
            // set implicit mapping state
            if( !Q_stricmp( token, "implicitBlend" ) )
            {
                implicitStateBits = GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
                implicitCullType = CT_TWO_SIDED;
            }
            else if( !Q_stricmp( token, "implicitMask" ) )
            {
                implicitStateBits = GLS_DEPTHMASK_TRUE | GLS_ATEST_GE_80;
                implicitCullType = CT_TWO_SIDED;
            }
            else    // "implicitMap"
            {
                implicitStateBits = GLS_DEPTHMASK_TRUE;
                implicitCullType = CT_FRONT_SIDED;
            }
            
            // get image
            token = COM_ParseExt( text, false );
            if( token[0] != '\0' )
            {
                Q_strncpyz( implicitMap, token, sizeof( implicitMap ) );
            }
            else
            {
                implicitMap[0] = '-';
                implicitMap[1] = '\0';
            }
            
            continue;
        }
        else
        {
            CL_RefPrintf( PRINT_WARNING, "WARNING: unknown general shader parameter '%s' in '%s'\n", token, shader.name );
            return false;
        }
    }
    
    //
    // ignore shaders that don't have any stages, unless it is a sky or fog
    //
    if( s == 0 && !shader.isSky && !( shader.contentFlags & CONTENTS_FOG ) && implicitMap[0] == '\0' )
    {
        return false;
    }
    
    if( !HaveSurfaceType( shader.surfaceFlags ) )
    {
        if( StringsContainWord( name, name, "plastic" ) || StringsContainWord( name, name, "trooper" ) || StringsContainWord( name, name, "medpack" ) || StringsContainWord( name, name, "reborn" ) )
            shader.surfaceFlags |= MATERIAL_PLASTIC;
        else if( StringsContainWord( name, name, "metal" ) || StringsContainWord( name, name, "pipe" ) || StringsContainWord( name, name, "scope" ) || StringsContainWord( name, name, "blaster" ) || StringsContainWord( name, name, "pistol" ) || StringsContainWord( name, name, "thermal" ) || StringsContainWord( name, name, "bowcaster" ) || StringsContainWord( name, name, "cannon" ) || StringsContainWord( name, name, "saber" ) || StringsContainWord( name, name, "rifle" ) || StringsContainWord( name, name, "jetpack" ) )
            shader.surfaceFlags |= MATERIAL_SOLIDMETAL;
        else if( StringsContainWord( name, name, "glass" ) || StringsContainWord( name, name, "light" ) )
            shader.surfaceFlags |= MATERIAL_GLASS;
        else if( StringsContainWord( name, name, "sand" ) )
            shader.surfaceFlags |= MATERIAL_SAND;
        else if( StringsContainWord( name, name, "gravel" ) )
            shader.surfaceFlags |= MATERIAL_GRAVEL;
        else if( StringsContainWord( name, name, "dirt" ) )
            shader.surfaceFlags |= MATERIAL_DIRT;
        else if( StringsContainWord( name, name, "concrete" ) )
            shader.surfaceFlags |= MATERIAL_CONCRETE;
        else if( StringsContainWord( name, name, "marble" ) || StringsContainWord( name, name, "teeth" ) || StringsContainWord( name, name, "stucco" ) )
            shader.surfaceFlags |= MATERIAL_MARBLE;
        else if( StringsContainWord( name, name, "snow" ) )
            shader.surfaceFlags |= MATERIAL_SNOW;
        else if( StringsContainWord( name, name, "flesh" ) || StringsContainWord( name, name, "body" ) || StringsContainWord( name, name, "leg" ) || StringsContainWord( name, name, "hand" ) || StringsContainWord( name, name, "head" ) || StringsContainWord( name, name, "hips" ) || StringsContainWord( name, name, "torso" ) || StringsContainWord( name, name, "tentacles" ) || StringsContainWord( name, name, "face" ) || StringsContainWord( name, name, "arms" ) )
            shader.surfaceFlags |= MATERIAL_FLESH;
        else if( StringsContainWord( name, name, "canvas" ) )
            shader.surfaceFlags |= MATERIAL_CANVAS;
        else if( StringsContainWord( name, name, "rock" ) )
            shader.surfaceFlags |= MATERIAL_ROCK;
        else if( StringsContainWord( name, name, "rubber" ) )
            shader.surfaceFlags |= MATERIAL_RUBBER;
        else if( StringsContainWord( name, name, "carpet" ) )
            shader.surfaceFlags |= MATERIAL_CARPET;
        else if( StringsContainWord( name, name, "plaster" ) )
            shader.surfaceFlags |= MATERIAL_PLASTER;
        else if( StringsContainWord( name, name, "computer" ) || StringsContainWord( name, name, "console" ) || StringsContainWord( name, name, "button" ) || StringsContainWord( name, name, "terminal" ) || StringsContainWord( name, name, "switch" ) || StringsContainWord( name, name, "panel" ) )
            shader.surfaceFlags |= MATERIAL_COMPUTER;
        else if( StringsContainWord( name, name, "armor" ) || StringsContainWord( name, name, "armour" ) )
            shader.surfaceFlags |= MATERIAL_ARMOR;
        else if( StringsContainWord( name, name, "fabric" ) )
            shader.surfaceFlags |= MATERIAL_FABRIC;
        else if( StringsContainWord( name, name, "hood" ) || StringsContainWord( name, name, "robe" ) || StringsContainWord( name, name, "cloth" ) || StringsContainWord( name, name, "pants" ) )
            shader.surfaceFlags |= MATERIAL_FABRIC;
        else if( StringsContainWord( name, name, "tile" ) || StringsContainWord( name, name, "lift" ) )
            shader.surfaceFlags |= MATERIAL_TILES;
        else if( StringsContainWord( name, name, "leaf" ) || StringsContainWord( name, name, "leaves" ) )
            shader.surfaceFlags |= MATERIAL_GREENLEAVES;
        else if( StringsContainWord( name, name, "mud" ) )
            shader.surfaceFlags |= MATERIAL_MUD;
        else if( StringsContainWord( name, name, "ice" ) )
            shader.surfaceFlags |= MATERIAL_ICE;
        else if( StringsContainWord( name, name, "hair" ) || StringsContainWord( name, name, "chewbacca" ) ) // use carpet
            shader.surfaceFlags |= MATERIAL_CARPET;
        else
        {
            CL_RefPrintf( PRINT_DEVELOPER, "Could not work out a default surface type for shader %s. It will fallback to default parallax and specular.\n", name );
        }
    }
    
    if( StringsContainWord( name, *text, "plastic" ) || StringsContainWord( name, *text, "trooper" ) || StringsContainWord( name, *text, "medpack" ) || StringsContainWord( name, *text, "reborn" ) )
        if( !( shader.surfaceFlags & MATERIAL_PLASTIC ) ) shader.surfaceFlags |= MATERIAL_PLASTIC;
        
    shader.explicitlyDefined = true;
    
    return true;
}

/*
========================================================================================

SHADER OPTIMIZATION AND FOGGING

========================================================================================
*/

/*
===================
ComputeStageIteratorFunc

See if we can use on of the simple fastpath stage functions,
otherwise set to the generic stage function
===================
*/
static void ComputeStageIteratorFunc( void )
{
    shader.optimalStageIteratorFunc = RB_StageIteratorGeneric;
    
    //
    // see if this should go into the sky path
    //
    if( shader.isSky )
    {
        shader.optimalStageIteratorFunc = RB_StageIteratorSky;
        return;
    }
}

/*
===================
ComputeVertexAttribs

Check which vertex attributes we only need, so we
don't need to submit/copy all of them.
===================
*/
static void ComputeVertexAttribs( void )
{
    S32 i, stage;
    
    // dlights always need ATTR_NORMAL
    shader.vertexAttribs = ATTR_POSITION | ATTR_NORMAL;
    
    // portals always need normals, for SurfIsOffscreen()
    if( shader.isPortal )
    {
        shader.vertexAttribs |= ATTR_NORMAL;
    }
    
    if( shader.defaultShader )
    {
        shader.vertexAttribs |= ATTR_TEXCOORD;
        return;
    }
    
    if( shader.numDeforms )
    {
        for( i = 0; i < shader.numDeforms; i++ )
        {
            deformStage_t*  ds = &shader.deforms[i];
            
            switch( ds->deformation )
            {
                case DEFORM_BULGE:
                    shader.vertexAttribs |= ATTR_NORMAL | ATTR_TEXCOORD;
                    break;
                    
                case DEFORM_AUTOSPRITE:
                    shader.vertexAttribs |= ATTR_NORMAL | ATTR_COLOR;
                    break;
                    
                case DEFORM_WAVE:
                case DEFORM_NORMALS:
                case DEFORM_TEXT0:
                case DEFORM_TEXT1:
                case DEFORM_TEXT2:
                case DEFORM_TEXT3:
                case DEFORM_TEXT4:
                case DEFORM_TEXT5:
                case DEFORM_TEXT6:
                case DEFORM_TEXT7:
                    shader.vertexAttribs |= ATTR_NORMAL;
                    break;
                    
                default:
                case DEFORM_NONE:
                case DEFORM_MOVE:
                case DEFORM_PROJECTION_SHADOW:
                case DEFORM_AUTOSPRITE2:
                    break;
            }
        }
    }
    
    for( stage = 0; stage < MAX_SHADER_STAGES; stage++ )
    {
        shaderStage_t* pStage = &stages[stage];
        
        if( !pStage->active )
        {
            break;
        }
        
        if( pStage->glslShaderGroup == tr.lightallShader )
        {
            shader.vertexAttribs |= ATTR_NORMAL;
            
            if( ( pStage->glslShaderIndex & LIGHTDEF_LIGHTTYPE_MASK ) && !( r_normalMapping->integer == 0 && r_specularMapping->integer == 0 ) )
            {
                shader.vertexAttribs |= ATTR_TANGENT;
            }
            
            switch( pStage->glslShaderIndex & LIGHTDEF_LIGHTTYPE_MASK )
            {
                case LIGHTDEF_USE_LIGHTMAP:
                case LIGHTDEF_USE_LIGHT_VERTEX:
                    shader.vertexAttribs |= ATTR_LIGHTDIRECTION;
                    break;
                default:
                    break;
            }
        }
        
        for( i = 0; i < NUM_TEXTURE_BUNDLES; i++ )
        {
            if( pStage->bundle[i].image[0] == 0 )
            {
                continue;
            }
            
            switch( pStage->bundle[i].tcGen )
            {
                case TCGEN_TEXTURE:
                    shader.vertexAttribs |= ATTR_TEXCOORD;
                    break;
                case TCGEN_LIGHTMAP:
                    shader.vertexAttribs |= ATTR_LIGHTCOORD;
                    break;
                case TCGEN_ENVIRONMENT_MAPPED:
                    shader.vertexAttribs |= ATTR_NORMAL;
                    break;
                    
                default:
                    break;
            }
        }
        
        switch( pStage->rgbGen )
        {
            case CGEN_EXACT_VERTEX:
            case CGEN_VERTEX:
            case CGEN_EXACT_VERTEX_LIT:
            case CGEN_VERTEX_LIT:
            case CGEN_ONE_MINUS_VERTEX:
                shader.vertexAttribs |= ATTR_COLOR;
                break;
                
            case CGEN_LIGHTING_DIFFUSE:
                shader.vertexAttribs |= ATTR_NORMAL;
                break;
                
            default:
                break;
        }
        
        switch( pStage->alphaGen )
        {
            case AGEN_LIGHTING_SPECULAR:
                shader.vertexAttribs |= ATTR_NORMAL;
                break;
                
            case AGEN_VERTEX:
            case AGEN_ONE_MINUS_VERTEX:
                shader.vertexAttribs |= ATTR_COLOR;
                break;
                
            default:
                break;
        }
    }
}


static void CollapseStagesToLightall( shaderStage_t* diffuse,
                                      shaderStage_t* normal, shaderStage_t* specular, shaderStage_t* lightmap,
                                      bool useLightVector, bool useLightVertex, bool parallax, bool tcgen )
{
    S32 defs = 0;
    
    //CL_RefPrintf(PRINT_ALL, "shader %s has diffuse %s", shader.name, diffuse->bundle[0].image[0]->imgName);
    
    // reuse diffuse, mark others inactive
    diffuse->type = ST_GLSL;
    
    if( lightmap )
    {
        //CL_RefPrintf(PRINT_ALL, ", lightmap");
        diffuse->bundle[TB_LIGHTMAP] = lightmap->bundle[0];
        defs |= LIGHTDEF_USE_LIGHTMAP;
    }
    else if( useLightVector )
    {
        defs |= LIGHTDEF_USE_LIGHT_VECTOR;
    }
    else if( useLightVertex )
    {
        defs |= LIGHTDEF_USE_LIGHT_VERTEX;
    }
    
    if( r_deluxeMapping->integer && tr.worldDeluxeMapping && lightmap )
    {
        //CL_RefPrintf(PRINT_ALL, ", deluxemap");
        diffuse->bundle[TB_DELUXEMAP] = lightmap->bundle[0];
        diffuse->bundle[TB_DELUXEMAP].image[0] = tr.deluxemaps[shader.lightmapIndex];
    }
    
    if( r_normalMapping->integer )
    {
        image_t* diffuseImg;
        if( normal )
        {
            //CL_RefPrintf(PRINT_ALL, ", normalmap %s", normal->bundle[0].image[0]->imgName);
            diffuse->bundle[TB_NORMALMAP] = normal->bundle[0];
            if( parallax && r_parallaxMapping->integer )
                defs |= LIGHTDEF_USE_PARALLAXMAP;
                
            VectorCopy4( normal->normalScale, diffuse->normalScale );
        }
        else if( ( lightmap || useLightVector || useLightVertex ) && ( diffuseImg = diffuse->bundle[TB_DIFFUSEMAP].image[0] ) )
        {
            UTF8 normalName[MAX_QPATH];
            image_t* normalImg;
            S32/*imgFlags_t*/ normalFlags = ( diffuseImg->flags & ~IMGFLAG_GENNORMALMAP ) | IMGFLAG_NOLIGHTSCALE;
            
            // try a normalheight image first
            COM_StripExtension2( diffuseImg->imgName, normalName, MAX_QPATH );
            Q_strcat( normalName, MAX_QPATH, "_h" );
            
            normalImg = R_FindImageFile( normalName, IMGTYPE_NORMALHEIGHT, normalFlags );
            
            if( normalImg )
            {
                parallax = true;
            }
            else
            {
                // try a normal image ("_n" suffix)
                normalName[strlen( normalName ) - 1] = '\0';
                normalImg = R_FindImageFile( normalName, IMGTYPE_NORMAL, normalFlags );
            }
            
            if( normalImg )
            {
                diffuse->bundle[TB_NORMALMAP] = diffuse->bundle[0];
                diffuse->bundle[TB_NORMALMAP].numImageAnimations = 0;
                diffuse->bundle[TB_NORMALMAP].image[0] = normalImg;
                
                if( parallax && r_parallaxMapping->integer )
                    defs |= LIGHTDEF_USE_PARALLAXMAP;
                    
                VectorSet4( diffuse->normalScale, r_baseNormalX->value, r_baseNormalY->value, 1.0f, r_baseParallax->value );
            }
        }
    }
    
    if( r_specularMapping->integer )
    {
        image_t* diffuseImg;
        if( specular )
        {
            //CL_RefPrintf(PRINT_ALL, ", specularmap %s", specular->bundle[0].image[0]->imgName);
            diffuse->bundle[TB_SPECULARMAP] = specular->bundle[0];
            VectorCopy4( specular->specularScale, diffuse->specularScale );
        }
        else if( ( lightmap || useLightVector || useLightVertex ) && ( diffuseImg = diffuse->bundle[TB_DIFFUSEMAP].image[0] ) && r_pbr->integer )
        {
            UTF8 specularName[MAX_QPATH];
            image_t* specularImg;
            S32/*imgFlags_t*/ specularFlags = ( diffuseImg->flags & ~IMGFLAG_GENNORMALMAP ) | IMGFLAG_NOLIGHTSCALE;
            
            COM_StripExtension2( diffuseImg->imgName, specularName, MAX_QPATH );
            Q_strcat( specularName, MAX_QPATH, "_s" );
            
            specularImg = R_FindImageFile( specularName, IMGTYPE_COLORALPHA, specularFlags );
            
            if( specularImg )
            {
                diffuse->bundle[TB_SPECULARMAP] = diffuse->bundle[0];
                diffuse->bundle[TB_SPECULARMAP].numImageAnimations = 0;
                diffuse->bundle[TB_SPECULARMAP].image[0] = specularImg;
                
                VectorSet4( diffuse->specularScale, 1.0f, 1.0f, 1.0f, 1.0f );
            }
        }
    }
    
    if( tcgen || diffuse->bundle[0].numTexMods )
    {
        defs |= LIGHTDEF_USE_TCGEN_AND_TCMOD;
    }
    
    //CL_RefPrintf(PRINT_ALL, ".\n");
    
    diffuse->glslShaderGroup = tr.lightallShader;
    diffuse->glslShaderIndex = defs;
}


static S32 CollapseStagesToGLSL( void )
{
    S32 i, j, numStages;
    bool skip = false;
    
    // skip shaders with deforms
    if( shader.numDeforms != 0 )
    {
        skip = true;
    }
    
    if( !skip )
    {
        // if 2+ stages and first stage is lightmap, switch them
        // this makes it easier for the later bits to process
        if( stages[0].active && stages[0].bundle[0].tcGen == TCGEN_LIGHTMAP && stages[1].active )
        {
            S32 blendBits = stages[1].stateBits & ( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );
            
            if( blendBits == ( GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO )
                    || blendBits == ( GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR ) )
            {
                S32 stateBits0 = stages[0].stateBits;
                S32 stateBits1 = stages[1].stateBits;
                shaderStage_t swapStage;
                
                swapStage = stages[0];
                stages[0] = stages[1];
                stages[1] = swapStage;
                
                stages[0].stateBits = stateBits0;
                stages[1].stateBits = stateBits1;
            }
        }
    }
    
    if( !skip )
    {
        // scan for shaders that aren't supported
        for( i = 0; i < MAX_SHADER_STAGES; i++ )
        {
            shaderStage_t* pStage = &stages[i];
            
            if( !pStage->active )
                continue;
                
            if( pStage->adjustColorsForFog )
            {
                skip = true;
                break;
            }
            
            if( pStage->bundle[0].tcGen == TCGEN_LIGHTMAP )
            {
                S32 blendBits = pStage->stateBits & ( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );
                
                if( blendBits != ( GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO )
                        && blendBits != ( GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR ) )
                {
                    skip = true;
                    break;
                }
            }
            
            switch( pStage->bundle[0].tcGen )
            {
                case TCGEN_TEXTURE:
                case TCGEN_LIGHTMAP:
                case TCGEN_ENVIRONMENT_MAPPED:
                case TCGEN_VECTOR:
                    break;
                default:
                    skip = true;
                    break;
            }
            
            switch( pStage->alphaGen )
            {
                case AGEN_LIGHTING_SPECULAR:
                case AGEN_PORTAL:
                    skip = true;
                    break;
                default:
                    break;
            }
        }
    }
    
    if( !skip )
    {
        for( i = 0; i < MAX_SHADER_STAGES; i++ )
        {
            shaderStage_t* pStage = &stages[i];
            shaderStage_t* diffuse, *normal, *specular, *lightmap;
            bool parallax, tcgen, diffuselit, vertexlit;
            
            if( !pStage->active )
                continue;
                
            // skip normal and specular maps
            if( pStage->type != ST_COLORMAP )
                continue;
                
            // skip lightmaps
            if( pStage->bundle[0].tcGen == TCGEN_LIGHTMAP )
                continue;
                
            diffuse  = pStage;
            normal   = nullptr;
            parallax = false;
            specular = nullptr;
            lightmap = nullptr;
            
            bool usedLightmap = false;
            // we have a diffuse map, find matching normal, specular, and lightmap
            for( j = i + 1; j < MAX_SHADER_STAGES; j++ )
            {
                shaderStage_t* pStage2 = &stages[j];
                
                if( !pStage2->active )
                    continue;
                    
                switch( pStage2->type )
                {
                    case ST_NORMALMAP:
                        if( !normal )
                        {
                            normal = pStage2;
                        }
                        break;
                        
                    case ST_NORMALPARALLAXMAP:
                        if( !normal )
                        {
                            normal = pStage2;
                            parallax = true;
                        }
                        break;
                        
                    case ST_SPECULARMAP:
                        if( !specular )
                        {
                            specular = pStage2;
                        }
                        break;
                        
                    case ST_COLORMAP:
                        if( pStage2->bundle[0].tcGen == TCGEN_LIGHTMAP )
                        {
                            lightmap = pStage2;
                            S32 blendBits = pStage->stateBits & ( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );
                            
                            // Only add lightmap to blendfunc filter stage if it's the first time lightmap is used
                            // otherwise it will cause the shader to be darkened by the lightmap multiple times.
                            if( !usedLightmap || ( blendBits != ( GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO )
                                                   && blendBits != ( GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR ) ) )
                            {
                                lightmap = pStage2;
                                usedLightmap = true;
                            }
                        }
                        break;
                        
                    default:
                        break;
                }
            }
            
            tcgen = false;
            if( diffuse->bundle[0].tcGen == TCGEN_ENVIRONMENT_MAPPED
                    || diffuse->bundle[0].tcGen == TCGEN_LIGHTMAP
                    || diffuse->bundle[0].tcGen == TCGEN_VECTOR )
            {
                tcgen = true;
            }
            
            diffuselit = false;
            if( diffuse->rgbGen == CGEN_LIGHTING_DIFFUSE )
            {
                diffuselit = true;
            }
            
            vertexlit = false;
            if( diffuse->rgbGen == CGEN_VERTEX_LIT || diffuse->rgbGen == CGEN_EXACT_VERTEX_LIT )
            {
                vertexlit = true;
            }
            
            CollapseStagesToLightall( diffuse, normal, specular, lightmap, diffuselit, vertexlit, parallax, tcgen );
        }
        
        // deactivate lightmap stages
        for( i = 0; i < MAX_SHADER_STAGES; i++ )
        {
            shaderStage_t* pStage = &stages[i];
            
            if( !pStage->active )
                continue;
                
            if( pStage->bundle[0].tcGen == TCGEN_LIGHTMAP )
            {
                pStage->active = false;
            }
        }
    }
    
    // deactivate normal and specular stages
    for( i = 0; i < MAX_SHADER_STAGES; i++ )
    {
        shaderStage_t* pStage = &stages[i];
        
        if( !pStage->active )
            continue;
            
        if( pStage->type == ST_NORMALMAP )
        {
            pStage->active = false;
        }
        
        if( pStage->type == ST_NORMALPARALLAXMAP )
        {
            pStage->active = false;
        }
        
        if( pStage->type == ST_SPECULARMAP )
        {
            pStage->active = false;
        }
    }
    
    // remove inactive stages
    numStages = 0;
    for( i = 0; i < MAX_SHADER_STAGES; i++ )
    {
        if( !stages[i].active )
            continue;
            
        if( i == numStages )
        {
            numStages++;
            continue;
        }
        
        stages[numStages] = stages[i];
        stages[i].active = false;
        numStages++;
    }
    
    // convert any remaining lightmap stages to a lighting pass with a white texture
    // only do this with r_sunlightMode non-zero, as it's only for correct shadows.
    if( r_sunlightMode->integer && shader.numDeforms == 0 )
    {
        for( i = 0; i < MAX_SHADER_STAGES; i++ )
        {
            shaderStage_t* pStage = &stages[i];
            
            if( !pStage->active )
                continue;
                
            if( pStage->adjustColorsForFog )
                continue;
                
            if( pStage->bundle[TB_DIFFUSEMAP].tcGen == TCGEN_LIGHTMAP )
            {
                pStage->glslShaderGroup = tr.lightallShader;
                pStage->glslShaderIndex = LIGHTDEF_USE_LIGHTMAP;
                pStage->bundle[TB_LIGHTMAP] = pStage->bundle[TB_DIFFUSEMAP];
                pStage->bundle[TB_DIFFUSEMAP].image[0] = tr.whiteImage;
                pStage->bundle[TB_DIFFUSEMAP].isLightmap = false;
                pStage->bundle[TB_DIFFUSEMAP].tcGen = TCGEN_TEXTURE;
            }
        }
    }
    
    // convert any remaining lightingdiffuse stages to a lighting pass
    if( shader.numDeforms == 0 )
    {
        for( i = 0; i < MAX_SHADER_STAGES; i++ )
        {
            shaderStage_t* pStage = &stages[i];
            
            if( !pStage->active )
                continue;
                
            if( pStage->adjustColorsForFog )
                continue;
                
            if( pStage->rgbGen == CGEN_LIGHTING_DIFFUSE )
            {
                pStage->glslShaderGroup = tr.lightallShader;
                pStage->glslShaderIndex = LIGHTDEF_USE_LIGHT_VECTOR;
                
                if( pStage->bundle[0].tcGen != TCGEN_TEXTURE || pStage->bundle[0].numTexMods != 0 )
                    pStage->glslShaderIndex |= LIGHTDEF_USE_TCGEN_AND_TCMOD;
            }
        }
    }
    
    return numStages;
}

/*
=============

FixRenderCommandList
https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=493
Arnout: this is a nasty issue. Shaders can be registered after drawsurfaces are generated
but before the frame is rendered. This will, for the duration of one frame, cause drawsurfaces
to be rendered with bad shaders. To fix this, need to go through all render commands and fix
sortedIndex.
==============
*/
static void FixRenderCommandList( S32 newShader )
{
    renderCommandList_t* cmdList = &backEndData[tr.smpFrame]->commands;
    
    if( cmdList )
    {
        const void* curCmd = cmdList->cmds;
        
        while( 1 )
        {
            curCmd = PADP( curCmd, sizeof( void* ) );
            
            switch( *( const S32* )curCmd )
            {
                case RC_SET_COLOR:
                {
                    const setColorCommand_t* sc_cmd = ( const setColorCommand_t* )curCmd;
                    curCmd = ( const void* )( sc_cmd + 1 );
                    break;
                }
                case RC_STRETCH_PIC:
                {
                    const stretchPicCommand_t* sp_cmd = ( const stretchPicCommand_t* )curCmd;
                    curCmd = ( const void* )( sp_cmd + 1 );
                    break;
                }
                case RC_DRAW_SURFS:
                {
                    S32 i;
                    drawSurf_t*	drawSurf;
                    shader_t*	shader;
                    S32			fogNum;
                    S32			entityNum;
                    S32			dlightMap;
                    S32         pshadowMap;
                    S32			sortedIndex;
                    const drawSurfsCommand_t* ds_cmd = ( const drawSurfsCommand_t* )curCmd;
                    
                    for( i = 0, drawSurf = ds_cmd->drawSurfs; i < ds_cmd->numDrawSurfs; i++, drawSurf++ )
                    {
                        R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &dlightMap, &pshadowMap );
                        sortedIndex = ( ( drawSurf->sort >> QSORT_SHADERNUM_SHIFT ) & ( MAX_SHADERS - 1 ) );
                        if( sortedIndex >= newShader )
                        {
                            sortedIndex++;
                            drawSurf->sort = ( sortedIndex << QSORT_SHADERNUM_SHIFT ) | entityNum | ( fogNum << QSORT_FOGNUM_SHIFT ) | ( ( S32 )pshadowMap << QSORT_PSHADOW_SHIFT ) | ( S32 )dlightMap;
                        }
                    }
                    curCmd = ( const void* )( ds_cmd + 1 );
                    break;
                }
                case RC_DRAW_BUFFER:
                {
                    const drawBufferCommand_t* db_cmd = ( const drawBufferCommand_t* )curCmd;
                    curCmd = ( const void* )( db_cmd + 1 );
                    break;
                }
                case RC_SWAP_BUFFERS:
                {
                    const swapBuffersCommand_t* sb_cmd = ( const swapBuffersCommand_t* )curCmd;
                    curCmd = ( const void* )( sb_cmd + 1 );
                    break;
                }
                case RC_END_OF_LIST:
                default:
                    return;
            }
        }
    }
}

/*
==============
SortNewShader

Positions the most recently created shader in the tr.sortedShaders[]
array so that the shader->sort key is sorted reletive to the other
shaders.

Sets shader->sortedIndex
==============
*/
static void SortNewShader( void )
{
    S32		i;
    F32	sort;
    shader_t*	newShader;
    
    newShader = tr.shaders[ tr.numShaders - 1 ];
    sort = newShader->sort;
    
    for( i = tr.numShaders - 2 ; i >= 0 ; i-- )
    {
        if( tr.sortedShaders[ i ]->sort <= sort )
        {
            break;
        }
        tr.sortedShaders[i + 1] = tr.sortedShaders[i];
        tr.sortedShaders[i + 1]->sortedIndex++;
    }
    
    // Arnout: fix rendercommandlist
    // https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=493
    FixRenderCommandList( i + 1 );
    
    newShader->sortedIndex = i + 1;
    tr.sortedShaders[i + 1] = newShader;
}


/*
====================
GeneratePermanentShader
====================
*/
static shader_t* GeneratePermanentShader( void )
{
    shader_t*	newShader = nullptr;
    S32			i, b;
    S32			size, hash;
    
    if( tr.numShaders == MAX_SHADERS )
    {
        CL_RefPrintf( PRINT_WARNING, "WARNING: GeneratePermanentShader - MAX_SHADERS hit\n" );
        return tr.defaultShader;
    }
    
    newShader = reinterpret_cast<shader_t*>( Hunk_Alloc( sizeof( shader_t ), h_low ) );
    
    *newShader = shader;
    
    if( shader.sort <= SS_OPAQUE )
    {
        newShader->fogPass = FP_EQUAL;
    }
    else if( shader.contentFlags & CONTENTS_FOG )
    {
        newShader->fogPass = FP_LE;
    }
    
    tr.shaders[ tr.numShaders ] = newShader;
    newShader->index = tr.numShaders;
    
    tr.sortedShaders[ tr.numShaders ] = newShader;
    newShader->sortedIndex = tr.numShaders;
    
    tr.numShaders++;
    
    for( i = 0 ; i < newShader->numUnfoggedPasses ; i++ )
    {
        if( !stages[i].active )
        {
            break;
        }
        newShader->stages[i] = reinterpret_cast<shaderStage_t*>( Hunk_Alloc( sizeof( stages[i] ), h_low ) );
        *newShader->stages[i] = stages[i];
        
        for( b = 0 ; b < NUM_TEXTURE_BUNDLES ; b++ )
        {
            size = newShader->stages[i]->bundle[b].numTexMods * sizeof( texModInfo_t );
            newShader->stages[i]->bundle[b].texMods = reinterpret_cast<texModInfo_t*>( Hunk_Alloc( size, h_low ) );
            ::memcpy( newShader->stages[i]->bundle[b].texMods, stages[i].bundle[b].texMods, size );
        }
    }
    
    SortNewShader();
    
    hash = generateHashValue( newShader->name, FILE_HASH_SIZE );
    newShader->next = hashTable[hash];
    hashTable[hash] = newShader;
    
    return newShader;
}

/*
====================
FindLightingStages
Find proper stage for dlight pass
====================
*/
#define GLS_BLEND_BITS (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)
static void FindLightingStages( void )
{
    S32 i;
    shader.lightingStage = -1;
    
    if( shader.isSky || ( shader.surfaceFlags & ( SURF_NODLIGHT | SURF_SKY ) ) || shader.sort > SS_OPAQUE )
        return;
        
    for( i = 0; i < shader.numUnfoggedPasses; i++ )
    {
        if( !stages[i].bundle[0].isLightmap )
        {
            if( stages[i].bundle[0].tcGen != TCGEN_TEXTURE )
                continue;
            if( ( stages[i].stateBits & GLS_BLEND_BITS ) == ( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE ) )
                continue;
            if( stages[i].rgbGen == CGEN_IDENTITY && ( stages[i].stateBits & GLS_BLEND_BITS ) == ( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO ) )
            {
                if( shader.lightingStage >= 0 )
                {
                    continue;
                }
            }
            shader.lightingStage = i;
        }
    }
}

/*
=================
VertexLightingCollapse

If vertex lighting is enabled, only render a single
pass, trying to guess which is the correct one to best aproximate
what it is supposed to look like.
=================
*/
static void VertexLightingCollapse( void )
{
    S32		stage;
    shaderStage_t*	bestStage;
    S32		bestImageRank;
    S32		rank;
    
    // if we aren't opaque, just use the first pass
    if( shader.sort == SS_OPAQUE )
    {
    
        // pick the best texture for the single pass
        bestStage = &stages[0];
        bestImageRank = -999999;
        
        for( stage = 0; stage < MAX_SHADER_STAGES; stage++ )
        {
            shaderStage_t* pStage = &stages[stage];
            
            if( !pStage->active )
            {
                break;
            }
            rank = 0;
            
            if( pStage->bundle[0].isLightmap )
            {
                rank -= 100;
            }
            if( pStage->bundle[0].tcGen != TCGEN_TEXTURE )
            {
                rank -= 5;
            }
            if( pStage->bundle[0].numTexMods )
            {
                rank -= 5;
            }
            if( pStage->rgbGen != CGEN_IDENTITY && pStage->rgbGen != CGEN_IDENTITY_LIGHTING )
            {
                rank -= 3;
            }
            
            if( rank > bestImageRank )
            {
                bestImageRank = rank;
                bestStage = pStage;
            }
        }
        
        stages[0].bundle[0] = bestStage->bundle[0];
        stages[0].stateBits &= ~( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );
        stages[0].stateBits |= GLS_DEPTHMASK_TRUE;
        if( shader.lightmapIndex == LIGHTMAP_NONE )
        {
            stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
        }
        else
        {
            stages[0].rgbGen = CGEN_EXACT_VERTEX;
        }
        stages[0].alphaGen = AGEN_SKIP;
    }
    else
    {
        // don't use a lightmap (tesla coils)
        if( stages[0].bundle[0].isLightmap )
        {
            stages[0] = stages[1];
        }
        
        // if we were in a cross-fade cgen, hack it to normal
        if( stages[0].rgbGen == CGEN_ONE_MINUS_ENTITY || stages[1].rgbGen == CGEN_ONE_MINUS_ENTITY )
        {
            stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
        }
        if( ( stages[0].rgbGen == CGEN_WAVEFORM && stages[0].rgbWave.func == GF_SAWTOOTH )
                && ( stages[1].rgbGen == CGEN_WAVEFORM && stages[1].rgbWave.func == GF_INVERSE_SAWTOOTH ) )
        {
            stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
        }
        if( ( stages[0].rgbGen == CGEN_WAVEFORM && stages[0].rgbWave.func == GF_INVERSE_SAWTOOTH )
                && ( stages[1].rgbGen == CGEN_WAVEFORM && stages[1].rgbWave.func == GF_SAWTOOTH ) )
        {
            stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
        }
    }
    
    for( stage = 1; stage < MAX_SHADER_STAGES; stage++ )
    {
        shaderStage_t* pStage = &stages[stage];
        
        if( !pStage->active )
        {
            break;
        }
        
        ::memset( pStage, 0, sizeof( *pStage ) );
    }
}

/*
===============
InitShader
===============
*/
static void InitShader( StringEntry name, S32 lightmapIndex )
{
    S32 i;
    
    // make sure the render thread is stopped, because we are probably
// going to have to upload an image
    if( r_smp->integer )
    {
        R_IssuePendingRenderCommands();
    }
    
    // clear the global shader
    ::memset( &shader, 0, sizeof( shader ) );
    ::memset( &stages, 0, sizeof( stages ) );
    
    Q_strncpyz( shader.name, name, sizeof( shader.name ) );
    shader.lightmapIndex = lightmapIndex;
    
    for( i = 0 ; i < MAX_SHADER_STAGES ; i++ )
    {
        stages[i].bundle[0].texMods = texMods[i];
        
        // ydnar: default to no implicit mappings
        implicitMap[0] = '\0';
        implicitStateBits = GLS_DEFAULT;
        implicitCullType = CT_FRONT_SIDED;
        
        // default normal/specular
        VectorSet4( stages[i].normalScale, 0.0f, 0.0f, 0.0f, 0.0f );
        if( r_pbr->integer )
        {
            stages[i].specularScale[0] = r_baseGloss->value;
            stages[i].specularScale[2] = 1.0;
        }
        else
        {
            stages[i].specularScale[0] =
                stages[i].specularScale[1] =
                    stages[i].specularScale[2] = r_baseSpecular->value;
            stages[i].specularScale[3] = r_baseGloss->value;
        }
    }
}

/*
=========================
FinishShader

Returns a freshly allocated shader with all the needed info
from the current global working shader
=========================
*/
static shader_t* FinishShader( void )
{
    S32 stage;
    bool		hasLightmapStage;
    bool		vertexLightmap;
    
    hasLightmapStage = false;
    vertexLightmap = false;
    
    //
    // set sky stuff appropriate
    //
    if( shader.isSky )
    {
        shader.sort = SS_ENVIRONMENT;
    }
    
    //
    // set polygon offset
    //
    if( shader.polygonOffset && !shader.sort )
    {
        shader.sort = SS_DECAL;
    }
    
    //
    // set appropriate stage information
    //
    for( stage = 0; stage < MAX_SHADER_STAGES; )
    {
        shaderStage_t* pStage = &stages[stage];
        
        if( !pStage->active )
        {
            break;
        }
        
        // check for a missing texture
        if( !pStage->bundle[0].image[0] )
        {
            CL_RefPrintf( PRINT_WARNING, "Shader %s has a stage with no image\n", shader.name );
            pStage->active = false;
            stage++;
            continue;
        }
        
        //
        // ditch this stage if it's detail and detail textures are disabled
        //
        if( pStage->isDetail && !r_detailTextures->integer )
        {
            S32 index;
            
            for( index = stage + 1; index < MAX_SHADER_STAGES; index++ )
            {
                if( !stages[index].active )
                    break;
            }
            
            if( index < MAX_SHADER_STAGES )
                memmove( pStage, pStage + 1, sizeof( *pStage ) * ( index - stage ) );
            else
            {
                if( stage + 1 < MAX_SHADER_STAGES )
                    memmove( pStage, pStage + 1, sizeof( *pStage ) * ( index - stage - 1 ) );
                    
                ::memset( &stages[index - 1], 0, sizeof( *stages ) );
            }
            
            continue;
        }
        
        //
        // default texture coordinate generation
        //
        if( pStage->bundle[0].isLightmap )
        {
            if( pStage->bundle[0].tcGen == TCGEN_BAD )
            {
                pStage->bundle[0].tcGen = TCGEN_LIGHTMAP;
            }
            hasLightmapStage = true;
        }
        else
        {
            if( pStage->bundle[0].tcGen == TCGEN_BAD )
            {
                pStage->bundle[0].tcGen = TCGEN_TEXTURE;
            }
        }
        
        
        // not a true lightmap but we want to leave existing
        // behaviour in place and not print out a warning
        //if (pStage->rgbGen == CGEN_VERTEX) {
        //  vertexLightmap = true;
        //}
        
        
        
        //
        // determine sort order and fog color adjustment
        //
        if( ( pStage->stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) &&
                ( stages[0].stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) )
        {
            S32 blendSrcBits = pStage->stateBits & GLS_SRCBLEND_BITS;
            S32 blendDstBits = pStage->stateBits & GLS_DSTBLEND_BITS;
            
            // fog color adjustment only works for blend modes that have a contribution
            // that aproaches 0 as the modulate values aproach 0 --
            // GL_ONE, GL_ONE
            // GL_ZERO, GL_ONE_MINUS_SRC_COLOR
            // GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
            
            // modulate, additive
            if( ( ( blendSrcBits == GLS_SRCBLEND_ONE ) && ( blendDstBits == GLS_DSTBLEND_ONE ) ) ||
                    ( ( blendSrcBits == GLS_SRCBLEND_ZERO ) && ( blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR ) ) )
            {
                pStage->adjustColorsForFog = ACFF_MODULATE_RGB;
            }
            // strict blend
            else if( ( blendSrcBits == GLS_SRCBLEND_SRC_ALPHA ) && ( blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA ) )
            {
                pStage->adjustColorsForFog = ACFF_MODULATE_ALPHA;
            }
            // premultiplied alpha
            else if( ( blendSrcBits == GLS_SRCBLEND_ONE ) && ( blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA ) )
            {
                pStage->adjustColorsForFog = ACFF_MODULATE_RGBA;
            }
            else
            {
                // we can't adjust this one correctly, so it won't be exactly correct in fog
            }
            
            // don't screw with sort order if this is a portal or environment
            if( !shader.sort )
            {
                // see through item, like a grill or grate
                if( pStage->stateBits & GLS_DEPTHMASK_TRUE )
                {
                    shader.sort = SS_SEE_THROUGH;
                }
                else
                {
                    shader.sort = SS_BLEND0;
                }
            }
        }
        
        stage++;
    }
    
    // there are times when you will need to manually apply a sort to
    // opaque alpha tested shaders that have later blend passes
    if( !shader.sort )
    {
        shader.sort = SS_OPAQUE;
    }
    
    //
    // if we are in r_vertexLight mode, never use a lightmap texture
    //
    if( stage > 1 && ( ( r_vertexLight->integer && !r_uiFullScreen->integer ) || glConfig.hardwareType == GLHW_PERMEDIA2 ) )
    {
        VertexLightingCollapse();
        hasLightmapStage = false;
    }
    
    //
    // look for multitexture potential
    //
    stage = CollapseStagesToGLSL();
    
    if( shader.lightmapIndex >= 0 && !hasLightmapStage )
    {
        if( vertexLightmap )
        {
            CL_RefPrintf( PRINT_DEVELOPER, "WARNING: shader '%s' has VERTEX forced lightmap!\n", shader.name );
        }
        else
        {
            CL_RefPrintf( PRINT_DEVELOPER, "WARNING: shader '%s' has lightmap but no lightmap stage!\n", shader.name );
            // Don't set this, it will just add duplicate shaders to the hash
            //shader.lightmapIndex = LIGHTMAP_NONE;
        }
    }
    
    
    //
    // compute number of passes
    //
    shader.numUnfoggedPasses = stage;
    
    FindLightingStages();
    
    // fogonly shaders don't have any normal passes
    if( stage == 0 && !shader.isSky )
        shader.sort = SS_FOG;
        
    if( shader.contentFlags & CONTENTS_LAVA )
    {
        // Override missing basejka material type.. lava.
        shader.materialType = MATERIAL_LAVA;
    }
    
    // determine which stage iterator function is appropriate
    ComputeStageIteratorFunc();
    
    // determine which vertex attributes this shader needs
    ComputeVertexAttribs();
    
    return GeneratePermanentShader();
}

//========================================================================================

/*
====================
FindShaderInShaderText

Scans the combined text description of all the shader files for
the given shader name.

return nullptr if not found

If found, it will return a valid shader
=====================
*/
static UTF8* FindShaderInShaderText( StringEntry shadername )
{
    shaderText_t* st;
    S32 hash;
    
    hash = generateHashValue( shadername, MAX_SHADERTEXT_HASH );
    
    for( st = shaderTextHashTable[hash]; st; st = st->next )
    {
        if( !Q_stricmp( st->name, shadername ) )
        {
            return st->text;
        }
    }
    
    return nullptr;
}


/*
==================
R_FindShaderByName

Will always return a valid shader, but it might be the
default shader if the real one can't be found.
==================
*/
shader_t* R_FindShaderByName( StringEntry name )
{
    UTF8		strippedName[MAX_QPATH];
    S32			hash;
    shader_t*	sh;
    
    if( ( name == nullptr ) || ( name[0] == 0 ) )
    {
        return tr.defaultShader;
    }
    
    COM_StripExtension2( name, strippedName, sizeof( strippedName ) );
    
    hash = generateHashValue( strippedName, FILE_HASH_SIZE );
    
    //
    // see if the shader is already loaded
    //
    for( sh = hashTable[hash]; sh; sh = sh->next )
    {
        // NOTE: if there was no shader or image available with the name strippedName
        // then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
        // have to check all default shaders otherwise for every call to R_FindShader
        // with that same strippedName a new default shader is created.
        if( Q_stricmp( sh->name, strippedName ) == 0 )
        {
            // match found
            return sh;
        }
    }
    
    return tr.defaultShader;
}


/*
===============
R_FindShader

Will always return a valid shader, but it might be the
default shader if the real one can't be found.

In the interest of not requiring an explicit shader text entry to
be defined for every single image used in the game, three default
shader behaviors can be auto-created for any image:

If lightmapIndex == LIGHTMAP_NONE, then the image will have
dynamic diffuse lighting applied to it, as apropriate for most
entity skin surfaces.

If lightmapIndex == LIGHTMAP_2D, then the image will be used
for 2D rendering unless an explicit shader is found

If lightmapIndex == LIGHTMAP_BY_VERTEX, then the image will use
the vertex rgba modulate values, as apropriate for misc_model
pre-lit surfaces.

Other lightmapIndex values will have a lightmap stage created
and src*dest blending applied with the texture, as apropriate for
most world construction surfaces.

===============
*/
shader_t* R_FindShader( StringEntry name, S32 lightmapIndex, bool mipRawImage )
{
    UTF8		strippedName[MAX_QPATH];
    S32			hash;
    UTF8*		shaderText;
    image_t*		image;
    shader_t*	sh;
    UTF8 fileName[MAX_QPATH];
    
    if( name[0] == 0 )
    {
        return tr.defaultShader;
    }
    
    // use (fullbright) vertex lighting if the bsp file doesn't have
    // lightmaps
    if( lightmapIndex >= 0 && lightmapIndex >= tr.numLightmaps )
    {
        lightmapIndex = LIGHTMAP_BY_VERTEX;
    }
    else if( lightmapIndex < LIGHTMAP_2D )
    {
        // negative lightmap indexes cause stray pointers (think tr.lightmaps[lightmapIndex])
        CL_RefPrintf( PRINT_WARNING, "WARNING: shader '%s' has invalid lightmap index of %d\n", name, lightmapIndex );
        lightmapIndex = LIGHTMAP_BY_VERTEX;
    }
    
    COM_StripExtension2( name, strippedName, sizeof( strippedName ) );
    
    hash = generateHashValue( strippedName, FILE_HASH_SIZE );
    
    //
    // see if the shader is already loaded
    //
    for( sh = hashTable[hash]; sh; sh = sh->next )
    {
        // NOTE: if there was no shader or image available with the name strippedName
        // then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
        // have to check all default shaders otherwise for every call to R_FindShader
        // with that same strippedName a new default shader is created.
        if( ( sh->lightmapIndex == lightmapIndex || sh->defaultShader ) &&
                !Q_stricmp( sh->name, strippedName ) )
        {
            // match found
            return sh;
        }
    }
    
    InitShader( strippedName, lightmapIndex );
    
    //
    // attempt to define shader from an explicit parameter file
    //
    shaderText = FindShaderInShaderText( strippedName );
    if( shaderText )
    {
        // enable this when building a pak file to get a global list
        // of all explicit shaders
        if( r_printShaders->integer )
        {
            CL_RefPrintf( PRINT_ALL, "*SHADER* %s\n", name );
        }
        
        if( !ParseShader( name, &shaderText ) )
        {
            // had errors, so use default shader
            shader.defaultShader = true;
            sh = FinishShader();
            return sh;
        }
        
        // ydnar: allow implicit mappings
        if( implicitMap[0] == '\0' )
        {
            sh = FinishShader();
            return sh;
        }
    }
    
    // ydnar: allow implicit mapping ('-' = use shader name)
    if( implicitMap[0] == '\0' || implicitMap[0] == '-' )
    {
        Q_strncpyz( fileName, name, sizeof( fileName ) );
    }
    else
    {
        Q_strncpyz( fileName, implicitMap, sizeof( fileName ) );
    }
    COM_DefaultExtension( fileName, sizeof( fileName ), ".tga" );
    
    // ydnar: implicit shaders were breaking nopicmip/nomipmaps
    if( !mipRawImage )
    {
        shader.noMipMaps = true;
        shader.noPicMip = true;
    }
    
    
    //
    // if not defined in the in-memory shader descriptions,
    // look for a single supported image file
    //
    {
        S32/*imgFlags_t*/ flags;
        
        flags = IMGFLAG_NONE;
        
        if( mipRawImage )
        {
            flags |= IMGFLAG_MIPMAP | IMGFLAG_PICMIP;
            
            if( r_genNormalMaps->integer )
                flags |= IMGFLAG_GENNORMALMAP;
        }
        else
        {
            flags |= IMGFLAG_CLAMPTOEDGE;
        }
        
        image = R_FindImageFile( fileName, IMGTYPE_COLORALPHA, flags );
        if( !image )
        {
            CL_RefPrintf( PRINT_DEVELOPER, "Couldn't find image file for shader %s\n", name );
            shader.defaultShader = true;
            return FinishShader();
        }
    }
    
    // set implicit cull type
    if( implicitCullType && !shader.cullType )
    {
        shader.cullType = implicitCullType;
    }
    
    //
    // create the default shading commands
    //
    if( shader.lightmapIndex == LIGHTMAP_NONE )
    {
        // dynamic colors at vertexes
        stages[0].bundle[0].image[0] = image;
        stages[0].active = true;
        stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
        stages[0].stateBits = implicitStateBits;
    }
    else if( shader.lightmapIndex == LIGHTMAP_BY_VERTEX )
    {
        // explicit colors at vertexes
        stages[0].bundle[0].image[0] = image;
        stages[0].active = true;
        stages[0].rgbGen = CGEN_EXACT_VERTEX;
        stages[0].alphaGen = AGEN_SKIP;
        stages[0].stateBits = implicitStateBits;
    }
    else if( shader.lightmapIndex == LIGHTMAP_2D )
    {
        // GUI elements
        stages[0].bundle[0].image[0] = image;
        stages[0].active = true;
        stages[0].rgbGen = CGEN_VERTEX;
        stages[0].alphaGen = AGEN_VERTEX;
        stages[0].stateBits = GLS_DEPTHTEST_DISABLE |
                              GLS_SRCBLEND_SRC_ALPHA |
                              GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
    }
    else if( shader.lightmapIndex == LIGHTMAP_WHITEIMAGE )
    {
        // fullbright level
        stages[0].bundle[0].image[0] = tr.whiteImage;
        stages[0].active = true;
        stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
        stages[0].stateBits = GLS_DEFAULT;
        
        stages[1].bundle[0].image[0] = image;
        stages[1].active = true;
        stages[1].rgbGen = CGEN_IDENTITY;
        stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
    }
    else
    {
        // unmmasked or blended implicit shaders need texture first
        if( implicitStateBits & ( GLS_ATEST_BITS | GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) )
        {
            stages[0].bundle[0].image[0] = image;
            stages[0].active = true;
            stages[0].rgbGen = CGEN_IDENTITY;
            stages[0].stateBits = implicitStateBits;
            
            stages[1].bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex];
            stages[1].bundle[0].isLightmap = true;
            stages[1].active = true;
            stages[1].rgbGen = CGEN_IDENTITY;
            stages[1].stateBits = GLS_DEFAULT | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO | GLS_DEPTHFUNC_EQUAL;
        }
        else
        {
            // otherwise do standard lightmap + texture
            // two pass lightmap
            stages[0].bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex];
            stages[0].bundle[0].isLightmap = true;
            stages[0].active = true;
            stages[0].rgbGen = CGEN_IDENTITY;	// lightmaps are scaled on creation
            // for identitylight
            stages[0].stateBits = GLS_DEFAULT;
            
            stages[1].bundle[0].image[0] = image;
            stages[1].active = true;
            stages[1].rgbGen = CGEN_IDENTITY;
            stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
        }
    }
    
    return FinishShader();
}


qhandle_t RE_RegisterShaderFromImage( StringEntry name, S32 lightmapIndex, image_t* image, bool mipRawImage )
{
    S32			hash;
    shader_t*	sh;
    
    hash = generateHashValue( name, FILE_HASH_SIZE );
    
    // probably not necessary since this function
    // only gets called from tr_font.c with lightmapIndex == LIGHTMAP_2D
    // but better safe than sorry.
    if( lightmapIndex >= tr.numLightmaps )
    {
        lightmapIndex = LIGHTMAP_WHITEIMAGE;
    }
    
    //
    // see if the shader is already loaded
    //
    for( sh = hashTable[hash]; sh; sh = sh->next )
    {
        // NOTE: if there was no shader or image available with the name strippedName
        // then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
        // have to check all default shaders otherwise for every call to R_FindShader
        // with that same strippedName a new default shader is created.
        if( ( sh->lightmapIndex == lightmapIndex || sh->defaultShader ) &&
                // index by name
                !Q_stricmp( sh->name, name ) )
        {
            // match found
            return sh->index;
        }
    }
    
    InitShader( name, lightmapIndex );
    
    //
    // create the default shading commands
    //
    if( shader.lightmapIndex == LIGHTMAP_NONE )
    {
        // dynamic colors at vertexes
        stages[0].bundle[0].image[0] = image;
        stages[0].active = true;
        stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
        stages[0].stateBits = GLS_DEFAULT;
    }
    else if( shader.lightmapIndex == LIGHTMAP_BY_VERTEX )
    {
        // explicit colors at vertexes
        stages[0].bundle[0].image[0] = image;
        stages[0].active = true;
        stages[0].rgbGen = CGEN_EXACT_VERTEX;
        stages[0].alphaGen = AGEN_SKIP;
        stages[0].stateBits = GLS_DEFAULT;
    }
    else if( shader.lightmapIndex == LIGHTMAP_2D )
    {
        // GUI elements
        stages[0].bundle[0].image[0] = image;
        stages[0].active = true;
        stages[0].rgbGen = CGEN_VERTEX;
        stages[0].alphaGen = AGEN_VERTEX;
        stages[0].stateBits = GLS_DEPTHTEST_DISABLE |
                              GLS_SRCBLEND_SRC_ALPHA |
                              GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
    }
    else if( shader.lightmapIndex == LIGHTMAP_WHITEIMAGE )
    {
        // fullbright level
        stages[0].bundle[0].image[0] = tr.whiteImage;
        stages[0].active = true;
        stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
        stages[0].stateBits = GLS_DEFAULT;
        
        stages[1].bundle[0].image[0] = image;
        stages[1].active = true;
        stages[1].rgbGen = CGEN_IDENTITY;
        stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
    }
    else
    {
        // two pass lightmap
        stages[0].bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex];
        stages[0].bundle[0].isLightmap = true;
        stages[0].active = true;
        stages[0].rgbGen = CGEN_IDENTITY;	// lightmaps are scaled on creation
        // for identitylight
        stages[0].stateBits = GLS_DEFAULT;
        
        stages[1].bundle[0].image[0] = image;
        stages[1].active = true;
        stages[1].rgbGen = CGEN_IDENTITY;
        stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
    }
    
    sh = FinishShader();
    return sh->index;
}


/*
====================
RE_RegisterShader

This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t RE_RegisterShaderLightMap( StringEntry name, S32 lightmapIndex )
{
    shader_t*	sh;
    
    if( strlen( name ) >= MAX_QPATH )
    {
        CL_RefPrintf( PRINT_ALL, "Shader name exceeds MAX_QPATH\n" );
        return 0;
    }
    
    sh = R_FindShader( name, lightmapIndex, true );
    
    // we want to return 0 if the shader failed to
    // load for some reason, but R_FindShader should
    // still keep a name allocated for it, so if
    // something calls RE_RegisterShader again with
    // the same name, we don't try looking for it again
    if( sh->defaultShader )
    {
        return 0;
    }
    
    return sh->index;
}


/*
====================
idRenderSystemLocal::RegisterShader

This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t idRenderSystemLocal::RegisterShader( StringEntry name )
{
    shader_t*	sh;
    
    if( strlen( name ) >= MAX_QPATH )
    {
        CL_RefPrintf( PRINT_ALL, "Shader name exceeds MAX_QPATH\n" );
        return 0;
    }
    
    sh = R_FindShader( name, LIGHTMAP_2D, true );
    
    // we want to return 0 if the shader failed to
    // load for some reason, but R_FindShader should
    // still keep a name allocated for it, so if
    // something calls RE_RegisterShader again with
    // the same name, we don't try looking for it again
    if( sh->defaultShader )
    {
        return 0;
    }
    
    return sh->index;
}


/*
====================
idRenderSystemLocal::RegisterShaderNoMip

For menu graphics that should never be picmiped
====================
*/
qhandle_t idRenderSystemLocal::RegisterShaderNoMip( StringEntry name )
{
    shader_t*	sh;
    
    if( strlen( name ) >= MAX_QPATH )
    {
        CL_RefPrintf( PRINT_ALL, "Shader name exceeds MAX_QPATH\n" );
        return 0;
    }
    
    sh = R_FindShader( name, LIGHTMAP_2D, false );
    
    // we want to return 0 if the shader failed to
    // load for some reason, but R_FindShader should
    // still keep a name allocated for it, so if
    // something calls RE_RegisterShader again with
    // the same name, we don't try looking for it again
    if( sh->defaultShader )
    {
        return 0;
    }
    
    return sh->index;
}

/*
====================
R_GetShaderByHandle

When a handle is passed in by another module, this range checks
it and returns a valid (possibly default) shader_t to be used internally.
====================
*/
shader_t* R_GetShaderByHandle( qhandle_t hShader )
{
    if( hShader < 0 )
    {
        CL_RefPrintf( PRINT_WARNING, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader );
        return tr.defaultShader;
    }
    if( hShader >= tr.numShaders )
    {
        CL_RefPrintf( PRINT_WARNING, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader );
        return tr.defaultShader;
    }
    return tr.shaders[hShader];
}

void RE_ShaderNameFromIndex( UTF8* name, qhandle_t hShader )
{
    if( name )
    {
        shader_t* sh = R_GetShaderByHandle( hShader );
        
        if( sh && sh != tr.defaultShader )
        {
            name = sh->name;
        }
        else
        {
            name = "";
        }
    }
}

/*
===============
R_ShaderList_f

Dump information on all valid shaders to the console
A second parameter will cause it to print in sorted order
===============
*/
void	R_ShaderList_f( void )
{
    S32			i;
    S32			count;
    shader_t*	shader;
    
    CL_RefPrintf( PRINT_ALL, "-----------------------\n" );
    
    count = 0;
    for( i = 0 ; i < tr.numShaders ; i++ )
    {
        if( cmdSystem->Argc() > 1 )
        {
            shader = tr.sortedShaders[i];
        }
        else
        {
            shader = tr.shaders[i];
        }
        
        CL_RefPrintf( PRINT_ALL, "%i ", shader->numUnfoggedPasses );
        
        if( shader->lightmapIndex >= 0 )
        {
            CL_RefPrintf( PRINT_ALL, "L " );
        }
        else
        {
            CL_RefPrintf( PRINT_ALL, "  " );
        }
        if( shader->explicitlyDefined )
        {
            CL_RefPrintf( PRINT_ALL, "E " );
        }
        else
        {
            CL_RefPrintf( PRINT_ALL, "  " );
        }
        
        if( shader->optimalStageIteratorFunc == RB_StageIteratorGeneric )
        {
            CL_RefPrintf( PRINT_ALL, "gen " );
        }
        else if( shader->optimalStageIteratorFunc == RB_StageIteratorSky )
        {
            CL_RefPrintf( PRINT_ALL, "sky " );
        }
        else
        {
            CL_RefPrintf( PRINT_ALL, "    " );
        }
        
        if( shader->defaultShader )
        {
            CL_RefPrintf( PRINT_ALL,  ": %s (DEFAULTED)\n", shader->name );
        }
        else
        {
            CL_RefPrintf( PRINT_ALL,  ": %s\n", shader->name );
        }
        count++;
    }
    CL_RefPrintf( PRINT_ALL, "%i total shaders\n", count );
    CL_RefPrintf( PRINT_ALL, "------------------\n" );
}

/*
====================
ScanAndLoadShaderFiles

Finds and loads all .shader files, combining them into
a single large text block that can be scanned for shader names
=====================
*/
static void LoadShaderFromBuffer( UTF8* buff )
{
    UTF8 shadername[MAX_SHADERNAME_LENGTH + 1];
    shaderText_t* st;
    UTF8* p, * name, * text;
    S32 nameLength, textLength;
    S64 size, hash;
    S32 shaderBug = 1;
    
    if( !buff )
    {
        return;
    }
    
    p = buff;
    while( *p )
    {
        COM_CompressBracedSection( &p, &name, &text, &nameLength, &textLength );
        if( !nameLength || !textLength ) continue;
        
        if( nameLength >= MAX_SHADERNAME_LENGTH )
        {
            strncpy( shadername, name, MAX_SHADERNAME_LENGTH );
            shadername[MAX_SHADERNAME_LENGTH] = '\0';
            CL_RefPrintf( PRINT_DEVELOPER, "Warning: Shader name too long '%s'...\n", shadername );
            continue;
        }
        
        ::strncpy( shadername, name, nameLength );
        shadername[nameLength] = '\0';
        name = shadername;
        
        // if shader already exists ignore the new shader text
        hash = generateHashValue( name, MAX_SHADERTEXT_HASH );
        for( st = shaderTextHashTable[hash]; st; st = st->next )
        {
            if( !Q_stricmp( name, st->name ) )
            {
                if( shaderBug )
                {
                    st->name[0] = '\0';
                    st = nullptr;
                }
                break;
            }
        }
        if( st )
        {
            continue;
        }
        
        shaderBug = 0;
        
        size = sizeof( shaderText_t ) + ( textLength + 1 ) + ( nameLength + 1 );
        st = ( shaderText_t* )Hunk_Alloc( size, h_low );
        
        ::memcpy( st->text, text, textLength );
        st->text[textLength] = '\0';
        st->name = st->text + ( textLength + 1 );
        strncpy( st->name, name, nameLength );
        st->name[nameLength] = '\0';
        
        st->next = shaderTextHashTable[hash];
        shaderTextHashTable[hash] = st;
        
        shaderCount++;
    }
}

static void ScanAndLoadShaderFiles( void )
{
    S32 i, numShaderFiles;
    UTF8 filename[MAX_QPATH];
    UTF8* buffer;
    UTF8** shaderFiles;
    
    // scan for shader files
    shaderFiles = fileSystem->ListFiles( "scripts", ".shader", &numShaderFiles );
    
    if( !shaderFiles || !numShaderFiles )
    {
        CL_RefPrintf( PRINT_WARNING, "WARNING: no shader files found\n" );
        return;
    }
    
    for( i = numShaderFiles - 1; i >= 0; i-- )
    {
        Com_sprintf( filename, sizeof( filename ), "scripts/%s", shaderFiles[i] );
        CL_RefPrintf( PRINT_DEVELOPER, "...loading '%s'\n", filename );
        
        fileSystem->ReadFile( filename, ( void** )&buffer );
        
        if( !buffer )
        {
            Com_Error( ERR_DROP, "Couldn't load %s", filename );
        }
        
        LoadShaderFromBuffer( buffer );
        
        fileSystem->FreeFile( buffer );
        
        fileShaderCount++;
    }
    
    // free up memory
    fileSystem->FreeFileList( shaderFiles );
    
    return;
}

/*
====================
CreateInternalShaders
====================
*/
static void CreateInternalShaders( void )
{
    tr.numShaders = 0;
    
    // init the default shader
    InitShader( "<default>", LIGHTMAP_NONE );
    stages[0].bundle[0].image[0] = tr.defaultImage;
    stages[0].active = true;
    stages[0].stateBits = GLS_DEFAULT;
    tr.defaultShader = FinishShader();
    
    // shadow shader is just a marker
    Q_strncpyz( shader.name, "<stencil shadow>", sizeof( shader.name ) );
    shader.sort = SS_STENCIL_SHADOW;
    tr.shadowShader = FinishShader();
}

static void CreateExternalShaders( void )
{
    tr.projectionShadowShader = R_FindShader( "projectionShadow", LIGHTMAP_NONE, true );
    tr.flareShader = R_FindShader( "flareShader", LIGHTMAP_NONE, true );
    
    // Hack to make fogging work correctly on flares. Fog colors are calculated
    // in tr_flare.c already.
    if( !tr.flareShader->defaultShader )
    {
        S32 index;
        
        for( index = 0; index < tr.flareShader->numUnfoggedPasses; index++ )
        {
            tr.flareShader->stages[index]->adjustColorsForFog = ACFF_NONE;
            tr.flareShader->stages[index]->stateBits |= GLS_DEPTHTEST_DISABLE;
        }
    }
    
    tr.sunShader = R_FindShader( "sun", LIGHTMAP_NONE, true );
    
    tr.sunFlareShader = R_FindShader( "gfx/2d/sunflare", LIGHTMAP_NONE, true );
    
    // HACK: if sunflare is missing, make one using the flare image or dlight image
    if( tr.sunFlareShader->defaultShader )
    {
        image_t* image;
        
        if( !tr.flareShader->defaultShader && tr.flareShader->stages[0] && tr.flareShader->stages[0]->bundle[0].image[0] )
            image = tr.flareShader->stages[0]->bundle[0].image[0];
        else
            image = tr.dlightImage;
            
        InitShader( "gfx/2d/sunflare", LIGHTMAP_NONE );
        stages[0].bundle[0].image[0] = image;
        stages[0].active = true;
        stages[0].stateBits = GLS_DEFAULT;
        tr.sunFlareShader = FinishShader();
    }
    
}

/*
==================
R_InitShaders
==================
*/
void R_InitShaders( void )
{
    S32 time, mem;
    
    CL_RefPrintf( PRINT_ALL, "Initializing Shaders\n" );
    
    COM_BeginParseSession( "R_InitShaders" );
    time = idsystem->Milliseconds();
    mem = Hunk_MemoryRemaining();
    fileShaderCount = 0;
    shaderCount = 0;
    
    ::memset( hashTable, 0, sizeof( hashTable ) );
    ::memset( shaderTextHashTable, 0, sizeof( shaderTextHashTable ) ); // drakkar - clear shader hashtable
    
    CreateInternalShaders();
    
    ScanAndLoadShaderFiles();
    
    CreateExternalShaders();
    
    time = idsystem->Milliseconds() - time;
    mem = mem - Hunk_MemoryRemaining();
    
    CL_RefPrintf( PRINT_ALL, "-------------------------\n" );
    CL_RefPrintf( PRINT_ALL, "%d shader files read \n", fileShaderCount );
    CL_RefPrintf( PRINT_ALL, "%d shaders found\n", shaderCount );
    CL_RefPrintf( PRINT_ALL, "%d code lines\n", COM_GetCurrentParseLine() );
    CL_RefPrintf( PRINT_ALL, "%.2f MB shader data\n", mem / 1024.0f / 1024.0f );
    CL_RefPrintf( PRINT_ALL, "%.3f seconds\n", time / 1000.0f );
    CL_RefPrintf( PRINT_ALL, "-------------------------\n" );
    
    COM_BeginParseSession( "" );
}
