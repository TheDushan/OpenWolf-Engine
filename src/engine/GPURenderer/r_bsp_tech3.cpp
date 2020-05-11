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
// File name:   r_bsp.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2015
// Description: Loads and prepares a map file for scene rendering.
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.h>

static	world_t		s_worldData;
static	U8*		fileBase;

S32			c_subdivisions;
S32			c_gridVerts;

//===============================================================================

static void HSVtoRGB( F32 h, F32 s, F32 v, F32 rgb[3] )
{
    S32 i;
    F32 f;
    F32 p, q, t;
    
    h *= 5;
    
    i = floor( h );
    f = h - i;
    
    p = v * ( 1 - s );
    q = v * ( 1 - s * f );
    t = v * ( 1 - s * ( 1 - f ) );
    
    switch( i )
    {
        case 0:
            rgb[0] = v;
            rgb[1] = t;
            rgb[2] = p;
            break;
        case 1:
            rgb[0] = q;
            rgb[1] = v;
            rgb[2] = p;
            break;
        case 2:
            rgb[0] = p;
            rgb[1] = v;
            rgb[2] = t;
            break;
        case 3:
            rgb[0] = p;
            rgb[1] = q;
            rgb[2] = v;
            break;
        case 4:
            rgb[0] = t;
            rgb[1] = p;
            rgb[2] = v;
            break;
        case 5:
            rgb[0] = v;
            rgb[1] = p;
            rgb[2] = q;
            break;
    }
}

/*
===============
R_ColorShiftLightingBytes

===============
*/
static	void R_ColorShiftLightingBytes( U8 in[4], U8 out[4] )
{
    S32		shift, r, g, b;
    
    // shift the color data based on overbright range
    shift = r_mapOverBrightBits->integer - tr.overbrightBits;
    
    // shift the data based on overbright range
    r = in[0] << shift;
    g = in[1] << shift;
    b = in[2] << shift;
    
    // normalize by color instead of saturating to white
    if( ( r | g | b ) > 255 )
    {
        S32		max;
        
        max = r > g ? r : g;
        max = max > b ? max : b;
        r = r * 255 / max;
        g = g * 255 / max;
        b = b * 255 / max;
    }
    
    out[0] = r;
    out[1] = g;
    out[2] = b;
    out[3] = in[3];
}


/*
===============
R_ColorShiftLightingFloats

===============
*/
static void R_ColorShiftLightingFloats( F32 in[4], F32 out[4] )
{
    F32	r, g, b;
    F32   scale = ( 1 << ( r_mapOverBrightBits->integer - tr.overbrightBits ) ) / 255.0f;
    
    r = in[0] * scale;
    g = in[1] * scale;
    b = in[2] * scale;
    
    // normalize by color instead of saturating to white
    if( r > 1 || g > 1 || b > 1 )
    {
        F32	max;
        
        max = r > g ? r : g;
        max = max > b ? max : b;
        r = r / max;
        g = g / max;
        b = b / max;
    }
    
    out[0] = r;
    out[1] = g;
    out[2] = b;
    out[3] = in[3];
}

// Modified from http://graphicrants.blogspot.jp/2009/04/rgbm-color-encoding.html
void ColorToRGBM( const vec3_t color, U8 rgbm[4] )
{
    vec3_t          sample;
    F32			maxComponent;
    
    VectorCopy( color, sample );
    
    maxComponent = MAX( sample[0], sample[1] );
    maxComponent = MAX( maxComponent, sample[2] );
    maxComponent = CLAMP( maxComponent, 1.0f / 255.0f, 1.0f );
    
    rgbm[3] = ( U8 ) ceil( maxComponent * 255.0f );
    maxComponent = 255.0f / rgbm[3];
    
    VectorScale( sample, maxComponent, sample );
    
    rgbm[0] = ( U8 )( sample[0] * 255 );
    rgbm[1] = ( U8 )( sample[1] * 255 );
    rgbm[2] = ( U8 )( sample[2] * 255 );
}

void ColorToRGB16( const vec3_t color, U16 rgb16[3] )
{
    rgb16[0] = color[0] * 65535.0f + 0.5f;
    rgb16[1] = color[1] * 65535.0f + 0.5f;
    rgb16[2] = color[2] * 65535.0f + 0.5f;
}


/*
===============
R_LoadLightmaps

===============
*/
#define	DEFAULT_LIGHTMAP_SIZE	128
static	void R_LoadLightmaps( lump_t* l, lump_t* surfs )
{
    S32/*imgFlags_t*/  imgFlags = IMGFLAG_NOLIGHTSCALE | IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE;
    U8*		buf, *buf_p;
    dsurface_t*  surf;
    S32			len;
    U8*		image;
    S32			i, j, numLightmaps, textureInternalFormat = 0;
    S32			numLightmapsPerPage = 16;
    F32 maxIntensity = 0;
    F64 sumIntensity = 0;
    
    len = l->filelen;
    if( !len )
    {
        return;
    }
    buf = fileBase + l->fileofs;
    
    // we are about to upload textures
    R_IssuePendingRenderCommands();
    
    tr.lightmapSize = DEFAULT_LIGHTMAP_SIZE;
    numLightmaps = len / ( tr.lightmapSize * tr.lightmapSize * 3 );
    
    // check for deluxe mapping
    if( numLightmaps <= 1 )
    {
        tr.worldDeluxeMapping = false;
    }
    else
    {
        tr.worldDeluxeMapping = true;
        #pragma omp critical
        for( i = 0, surf = ( dsurface_t* )( fileBase + surfs->fileofs );
                i < surfs->filelen / sizeof( dsurface_t ); i++, surf++ )
        {
            S32 lightmapNum = LittleLong( surf->lightmapNum );
            
            if( lightmapNum >= 0 && ( lightmapNum & 1 ) != 0 )
            {
                tr.worldDeluxeMapping = false;
                break;
            }
        }
    }
    
    image = ( U8* )CL_RefMalloc( tr.lightmapSize * tr.lightmapSize * 4 * 2 );
    
    if( tr.worldDeluxeMapping )
        numLightmaps >>= 1;
        
    // Use fat lightmaps of an appropriate size.
    if( r_mergeLightmaps->integer )
    {
        S32 maxLightmapsPerAxis = glConfig.maxTextureSize / tr.lightmapSize;
        S32 lightmapCols = 4, lightmapRows = 4;
        
        // Increase width at first, then height.
        while( lightmapCols * lightmapRows < numLightmaps && lightmapCols != maxLightmapsPerAxis )
            lightmapCols <<= 1;
            
        while( lightmapCols * lightmapRows < numLightmaps && lightmapRows != maxLightmapsPerAxis )
            lightmapRows <<= 1;
            
        tr.fatLightmapCols  = lightmapCols;
        tr.fatLightmapRows  = lightmapRows;
        numLightmapsPerPage = lightmapCols * lightmapRows;
        
        tr.numLightmaps = ( numLightmaps + ( numLightmapsPerPage - 1 ) ) / numLightmapsPerPage;
    }
    else
    {
        tr.numLightmaps = numLightmaps;
    }
    
    tr.lightmaps = reinterpret_cast<image_t**>( Hunk_Alloc( tr.numLightmaps * sizeof( image_t* ), h_low ) );
    
    if( tr.worldDeluxeMapping )
        tr.deluxemaps = reinterpret_cast<image_t**>( Hunk_Alloc( tr.numLightmaps * sizeof( image_t* ), h_low ) );
        
    textureInternalFormat = GL_RGBA8;
    if( r_hdr->integer )
    {
        // Check for the first hdr lightmap, if it exists, use GL_RGBA16 for textures.
        UTF8 filename[MAX_QPATH];
        
        Com_sprintf( filename, sizeof( filename ), "maps/%s/lm_0000.hdr", s_worldData.baseName );
        if( fileSystem->FileExists( filename ) )
            textureInternalFormat = GL_RGBA16;
    }
    
    if( r_mergeLightmaps->integer )
    {
        S32 width  = tr.fatLightmapCols * tr.lightmapSize;
        S32 height = tr.fatLightmapRows * tr.lightmapSize;
        
        #pragma omp critical
        for( i = 0; i < tr.numLightmaps; i++ )
        {
            tr.lightmaps[i] = R_CreateImage( va( "_fatlightmap%d", i ), NULL, width, height, IMGTYPE_COLORALPHA, imgFlags, textureInternalFormat );
            
            if( tr.worldDeluxeMapping )
                tr.deluxemaps[i] = R_CreateImage( va( "_fatdeluxemap%d", i ), NULL, width, height, IMGTYPE_DELUXE, imgFlags, 0 );
        }
    }
    
    #pragma omp critical
    for( i = 0; i < numLightmaps; i++ )
    {
        S32 xoff = 0, yoff = 0;
        S32 lightmapnum = i;
        // expand the 24 bit on-disk to 32 bit
        
        if( r_mergeLightmaps->integer )
        {
            S32 lightmaponpage = i % numLightmapsPerPage;
            xoff = ( lightmaponpage % tr.fatLightmapCols ) * tr.lightmapSize;
            yoff = ( lightmaponpage / tr.fatLightmapCols ) * tr.lightmapSize;
            
            lightmapnum /= numLightmapsPerPage;
        }
        
        // if (tr.worldLightmapping)
        {
            UTF8 filename[MAX_QPATH];
            U8* hdrLightmap = NULL;
            S32 size = 0;
            
            // look for hdr lightmaps
            if( textureInternalFormat == GL_RGBA16 )
            {
                Com_sprintf( filename, sizeof( filename ), "maps/%s/lm_%04d.hdr", s_worldData.baseName, i * ( tr.worldDeluxeMapping ? 2 : 1 ) );
                //CL_RefPrintf(PRINT_ALL, "looking for %s\n", filename);
                
                size = fileSystem->ReadFile( filename, ( void** )&hdrLightmap );
            }
            
            if( hdrLightmap )
            {
                U8* p = hdrLightmap, *end = hdrLightmap + size;
                //CL_RefPrintf(PRINT_ALL, "found!\n");
                
                /* FIXME: don't just skip over this header and actually parse it */
                while( p < end && !( *p == '\n' && *( p + 1 ) == '\n' ) )
                    p++;
                    
                p += 2;
                
                while( p < end && !( *p == '\n' ) )
                    p++;
                    
                p++;
                
                if( p >= end )
                    Com_Error( ERR_DROP, "Bad header for %s!", filename );
                    
                buf_p = p;
                
#if 0 // HDRFILE_RGBE
                if( ( S32 )( end - hdrLightmap ) != tr.lightmapSize * tr.lightmapSize * 4 )
                    Com_Error( ERR_DROP, "Bad size for %s (%i)!", filename, size );
#else // HDRFILE_FLOAT
                if( ( S32 )( end - hdrLightmap ) != tr.lightmapSize * tr.lightmapSize * 12 )
                    Com_Error( ERR_DROP, "Bad size for %s (%i)!", filename, size );
#endif
            }
            else
            {
                S32 imgOffset = tr.worldDeluxeMapping ? i * 2 : i;
                buf_p = buf + imgOffset * tr.lightmapSize * tr.lightmapSize * 3;
            }
            
            #pragma omp critical
            for( j = 0 ; j < tr.lightmapSize * tr.lightmapSize; j++ )
            {
                if( hdrLightmap )
                {
                    vec4_t color;
                    
#if 0 // HDRFILE_RGBE
                    F32 exponent = exp2( buf_p[j * 4 + 3] - 128 );
                    
                    color[0] = buf_p[j * 4 + 0] * exponent;
                    color[1] = buf_p[j * 4 + 1] * exponent;
                    color[2] = buf_p[j * 4 + 2] * exponent;
#else // HDRFILE_FLOAT
                    memcpy( color, &buf_p[j * 12], 12 );
                    
                    color[0] = LittleFloat( color[0] );
                    color[1] = LittleFloat( color[1] );
                    color[2] = LittleFloat( color[2] );
#endif
                    color[3] = 1.0f;
                    
                    R_ColorShiftLightingFloats( color, color );
                    
                    ColorToRGB16( color, ( U16* )( &image[j * 8] ) );
                    ( ( U16* )( &image[j * 8] ) )[3] = 65535;
                }
                else if( textureInternalFormat == GL_RGBA16 )
                {
                    vec4_t color;
                    
                    //hack: convert LDR lightmap to HDR one
                    color[0] = MAX( buf_p[j * 3 + 0], 0.499f );
                    color[1] = MAX( buf_p[j * 3 + 1], 0.499f );
                    color[2] = MAX( buf_p[j * 3 + 2], 0.499f );
                    
                    // if under an arbitrary value (say 12) grey it out
                    // this prevents weird splotches in dimly lit areas
                    if( color[0] + color[1] + color[2] < 12.0f )
                    {
                        F32 avg = ( color[0] + color[1] + color[2] ) * 0.3333f;
                        color[0] = avg;
                        color[1] = avg;
                        color[2] = avg;
                    }
                    color[3] = 1.0f;
                    
                    R_ColorShiftLightingFloats( color, color );
                    
                    ColorToRGB16( color, ( U16* )( &image[j * 8] ) );
                    ( ( U16* )( &image[j * 8] ) )[3] = 65535;
                }
                else
                {
                    if( r_lightmap->integer == 2 )
                    {
                        // color code by intensity as development tool	(FIXME: check range)
                        F32 r = buf_p[j * 3 + 0];
                        F32 g = buf_p[j * 3 + 1];
                        F32 b = buf_p[j * 3 + 2];
                        F32 intensity;
                        F32 out[3] = {0.0, 0.0, 0.0};
                        
                        intensity = 0.33f * r + 0.685f * g + 0.063f * b;
                        
                        if( intensity > 255 )
                            intensity = 1.0f;
                        else
                            intensity /= 255.0f;
                            
                        if( intensity > maxIntensity )
                            maxIntensity = intensity;
                            
                        HSVtoRGB( intensity, 1.00, 0.50, out );
                        
                        image[j * 4 + 0] = out[0] * 255;
                        image[j * 4 + 1] = out[1] * 255;
                        image[j * 4 + 2] = out[2] * 255;
                        image[j * 4 + 3] = 255;
                        
                        sumIntensity += intensity;
                    }
                    else
                    {
                        R_ColorShiftLightingBytes( &buf_p[j * 3], &image[j * 4] );
                        image[j * 4 + 3] = 255;
                    }
                }
            }
            
            if( r_mergeLightmaps->integer )
                R_UpdateSubImage( tr.lightmaps[lightmapnum], image, xoff, yoff, tr.lightmapSize, tr.lightmapSize, textureInternalFormat );
            else
                tr.lightmaps[i] = R_CreateImage( va( "*lightmap%d", i ), image, tr.lightmapSize, tr.lightmapSize, IMGTYPE_COLORALPHA, imgFlags, textureInternalFormat );
                
            if( hdrLightmap )
                fileSystem->FreeFile( hdrLightmap );
        }
        
        if( tr.worldDeluxeMapping )
        {
            buf_p = buf + ( i * 2 + 1 ) * tr.lightmapSize * tr.lightmapSize * 3;
            
            #pragma omp critical
            for( j = 0 ; j < tr.lightmapSize * tr.lightmapSize; j++ )
            {
                image[j * 4 + 0] = buf_p[j * 3 + 0];
                image[j * 4 + 1] = buf_p[j * 3 + 1];
                image[j * 4 + 2] = buf_p[j * 3 + 2];
                
                // make 0,0,0 into 127,127,127
                if( ( image[j * 4 + 0] == 0 ) && ( image[j * 4 + 1] == 0 ) && ( image[j * 4 + 2] == 0 ) )
                {
                    image[j * 4 + 0] =
                        image[j * 4 + 1] =
                            image[j * 4 + 2] = 127;
                }
                
                image[j * 4 + 3] = 255;
            }
            
            if( r_mergeLightmaps->integer )
                R_UpdateSubImage( tr.deluxemaps[lightmapnum], image, xoff, yoff, tr.lightmapSize, tr.lightmapSize, GL_RGBA8 );
            else
                tr.deluxemaps[i] = R_CreateImage( va( "*deluxemap%d", i ), image, tr.lightmapSize, tr.lightmapSize, IMGTYPE_DELUXE, imgFlags, 0 );
        }
    }
    
    if( r_lightmap->integer == 2 )
    {
        CL_RefPrintf( PRINT_ALL, "Brightest lightmap value: %d\n", ( S32 )( maxIntensity * 255 ) );
    }
    
    Z_Free( image );
}


static F32 FatPackU( F32 input, S32 lightmapnum )
{
    if( lightmapnum < 0 )
        return input;
        
    if( tr.worldDeluxeMapping )
        lightmapnum >>= 1;
        
    if( tr.fatLightmapCols > 0 )
    {
        lightmapnum %= ( tr.fatLightmapCols * tr.fatLightmapRows );
        return ( input + ( lightmapnum % tr.fatLightmapCols ) ) / ( F32 )( tr.fatLightmapCols );
    }
    
    return input;
}

static F32 FatPackV( F32 input, S32 lightmapnum )
{
    if( lightmapnum < 0 )
        return input;
        
    if( tr.worldDeluxeMapping )
        lightmapnum >>= 1;
        
    if( tr.fatLightmapCols > 0 )
    {
        lightmapnum %= ( tr.fatLightmapCols * tr.fatLightmapRows );
        return ( input + ( lightmapnum / tr.fatLightmapCols ) ) / ( F32 )( tr.fatLightmapRows );
    }
    
    return input;
}


static S32 FatLightmap( S32 lightmapnum )
{
    if( lightmapnum < 0 )
        return lightmapnum;
        
    if( tr.worldDeluxeMapping )
        lightmapnum >>= 1;
        
    if( tr.fatLightmapCols > 0 )
        return lightmapnum / ( tr.fatLightmapCols * tr.fatLightmapRows );
        
    return lightmapnum;
}

/*
=================
idRenderSystemLocal::SetWorldVisData

This is called by the clipmodel subsystem so we can share the 1.8 megs of
space in big maps...
=================
*/
void idRenderSystemLocal::SetWorldVisData( const U8* vis )
{
    tr.externalVisData = vis;
}

/*
=================
R_LoadVisibility
=================
*/
static	void R_LoadVisibility( lump_t* l )
{
    S32		len;
    U8*	buf;
    
    len = l->filelen;
    if( !len )
    {
        return;
    }
    buf = fileBase + l->fileofs;
    
    s_worldData.numClusters = LittleLong( ( ( S32* )buf )[0] );
    s_worldData.clusterBytes = LittleLong( ( ( S32* )buf )[1] );
    
    // CM_Load should have given us the vis data to share, so
    // we don't need to allocate another copy
    if( tr.externalVisData )
    {
        s_worldData.vis = tr.externalVisData;
    }
    else
    {
        U8* dest = nullptr;
        dest = reinterpret_cast<U8*>( Hunk_Alloc( len - 8, h_low ) );
        ::memcpy( dest, buf + 8, len - 8 );
        s_worldData.vis = dest;
    }
}

//===============================================================================


/*
===============
ShaderForShaderNum
===============
*/
static shader_t* ShaderForShaderNum( S32 shaderNum, S32 lightmapNum )
{
    shader_t*	_shader;
    dshader_t*	dsh;
    
    S32 _shaderNum = LittleLong( shaderNum );
    if( _shaderNum < 0 || _shaderNum >= s_worldData.numShaders )
    {
        Com_Error( ERR_DROP, "ShaderForShaderNum: bad num %i", _shaderNum );
    }
    dsh = &s_worldData.shaders[ _shaderNum ];
    
    if( r_vertexLight->integer || glConfig.hardwareType == GLHW_PERMEDIA2 )
    {
        lightmapNum = LIGHTMAP_BY_VERTEX;
    }
    
    if( r_fullbright->integer )
    {
        lightmapNum = LIGHTMAP_WHITEIMAGE;
    }
    
    _shader = R_FindShader( dsh->shader, lightmapNum, true );
    
    // if the shader had errors, just use default shader
    if( _shader->defaultShader )
    {
        return tr.defaultShader;
    }
    
    return _shader;
}

void LoadDrawVertToSrfVert( srfVert_t* s, drawVert_t* d, S32 realLightmapNum, F32 hdrVertColors[3], vec3_t* bounds )
{
    vec4_t v;
    
    s->xyz[0] = LittleFloat( d->xyz[0] );
    s->xyz[1] = LittleFloat( d->xyz[1] );
    s->xyz[2] = LittleFloat( d->xyz[2] );
    
    if( bounds )
        AddPointToBounds( s->xyz, bounds[0], bounds[1] );
        
    s->st[0] = LittleFloat( d->st[0] );
    s->st[1] = LittleFloat( d->st[1] );
    
    if( realLightmapNum >= 0 )
    {
        s->lightmap[0] = FatPackU( LittleFloat( d->lightmap[0] ), realLightmapNum );
        s->lightmap[1] = FatPackV( LittleFloat( d->lightmap[1] ), realLightmapNum );
    }
    else
    {
        s->lightmap[0] = LittleFloat( d->lightmap[0] );
        s->lightmap[1] = LittleFloat( d->lightmap[1] );
    }
    
    v[0] = LittleFloat( d->normal[0] );
    v[1] = LittleFloat( d->normal[1] );
    v[2] = LittleFloat( d->normal[2] );
    
    R_VaoPackNormal( s->normal, v );
    
    if( hdrVertColors )
    {
        v[0] = hdrVertColors[0];
        v[1] = hdrVertColors[1];
        v[2] = hdrVertColors[2];
    }
    else
    {
        //hack: convert LDR vertex colors to HDR
        if( r_hdr->integer )
        {
            v[0] = MAX( d->color[0], 0.499f );
            v[1] = MAX( d->color[1], 0.499f );
            v[2] = MAX( d->color[2], 0.499f );
        }
        else
        {
            v[0] = d->color[0];
            v[1] = d->color[1];
            v[2] = d->color[2];
        }
        
    }
    v[3] = d->color[3] / 255.0f;
    
    R_ColorShiftLightingFloats( v, v );
    R_VaoPackColor( s->color, v );
}


/*
===============
ParseFace
===============
*/
static void ParseFace( dsurface_t* ds, drawVert_t* verts, F32* hdrVertColors, msurface_t* surf, S32* indexes )
{
    S32			i, j;
    srfBspSurface_t*	cv;
    U32*  tri;
    S32			numVerts, numIndexes, badTriangles;
    S32 realLightmapNum;
    
    realLightmapNum = LittleLong( ds->lightmapNum );
    
    // get fog volume
    surf->fogIndex = LittleLong( ds->fogNum ) + 1;
    
    // get shader value
    surf->shader = ShaderForShaderNum( ds->shaderNum, FatLightmap( realLightmapNum ) );
    if( r_singleShader->integer && !surf->shader->isSky )
    {
        surf->shader = tr.defaultShader;
    }
    
    numVerts = LittleLong( ds->numVerts );
    if( numVerts > 512 )
    {
        CL_RefPrintf( PRINT_WARNING, "WARNING: MAX_FACE_POINTS exceeded: %i\n", numVerts );
        numVerts = 512;
        surf->shader = tr.defaultShader;
    }
    
    numIndexes = LittleLong( ds->numIndexes );
    
    //cv = Hunk_Alloc(sizeof(*cv), h_low);
    cv = ( srfBspSurface_t* )surf->data;
    cv->surfaceType = SF_FACE;
    
    cv->numIndexes = numIndexes;
    cv->indexes = reinterpret_cast<U32*>( Hunk_Alloc( numIndexes * sizeof( cv->indexes[0] ), h_low ) );
    
    cv->numVerts = numVerts;
    cv->verts = reinterpret_cast<srfVert_t*>( Hunk_Alloc( numVerts * sizeof( cv->verts[0] ), h_low ) );
    
    // copy vertexes
    surf->cullinfo.type = CULLINFO_PLANE | CULLINFO_BOX;
    ClearBounds( surf->cullinfo.bounds[0], surf->cullinfo.bounds[1] );
    verts += LittleLong( ds->firstVert );
    #pragma omp critical
    for( i = 0; i < numVerts; i++ )
        LoadDrawVertToSrfVert( &cv->verts[i], &verts[i], realLightmapNum, hdrVertColors ? hdrVertColors + ( ds->firstVert + i ) * 3 : NULL, surf->cullinfo.bounds );
        
    // copy triangles
    badTriangles = 0;
    indexes += LittleLong( ds->firstIndex );
    #pragma omp critical
    for( i = 0, tri = cv->indexes; i < numIndexes; i += 3, tri += 3 )
    {
        for( j = 0; j < 3; j++ )
        {
            tri[j] = LittleLong( indexes[i + j] );
            
            if( tri[j] >= numVerts )
            {
                Com_Error( ERR_DROP, "Bad index in face surface" );
            }
        }
        
        if( ( tri[0] == tri[1] ) || ( tri[1] == tri[2] ) || ( tri[0] == tri[2] ) )
        {
            tri -= 3;
            badTriangles++;
        }
    }
    
    if( badTriangles )
    {
        CL_RefPrintf( PRINT_WARNING, "Face has bad triangles, originally shader %s %d tris %d verts, now %d tris\n", surf->shader->name, numIndexes / 3, numVerts, numIndexes / 3 - badTriangles );
        cv->numIndexes -= badTriangles * 3;
    }
    
    // take the plane information from the lightmap vector
    #pragma omp critical
    for( i = 0 ; i < 3 ; i++ )
    {
        cv->cullPlane.normal[i] = LittleFloat( ds->lightmapVecs[2][i] );
    }
    cv->cullPlane.dist = DotProduct( cv->verts[0].xyz, cv->cullPlane.normal );
    SetPlaneSignbits( &cv->cullPlane );
    cv->cullPlane.type = PlaneTypeForNormal( cv->cullPlane.normal );
    surf->cullinfo.plane = cv->cullPlane;
    
    surf->data = ( surfaceType_t* )cv;
    
    // Calculate tangent spaces
    {
        srfVert_t*      dv[3];
        
        #pragma omp critical
        for( i = 0, tri = cv->indexes; i < numIndexes; i += 3, tri += 3 )
        {
            dv[0] = &cv->verts[tri[0]];
            dv[1] = &cv->verts[tri[1]];
            dv[2] = &cv->verts[tri[2]];
            
            R_CalcTangentVectors( dv );
        }
    }
}


/*
===============
ParseMesh
===============
*/
static void ParseMesh( dsurface_t* ds, drawVert_t* verts, F32* hdrVertColors, msurface_t* surf )
{
    srfBspSurface_t*	grid = ( srfBspSurface_t* )surf->data;
    S32				i;
    S32				width, height, numPoints;
    srfVert_t points[MAX_PATCH_SIZE * MAX_PATCH_SIZE];
    vec3_t			bounds[2];
    vec3_t			tmpVec;
    static surfaceType_t	skipData = SF_SKIP;
    S32 realLightmapNum;
    
    realLightmapNum = LittleLong( ds->lightmapNum );
    
    // get fog volume
    surf->fogIndex = LittleLong( ds->fogNum ) + 1;
    
    // get shader value
    surf->shader = ShaderForShaderNum( ds->shaderNum, FatLightmap( realLightmapNum ) );
    if( r_singleShader->integer && !surf->shader->isSky )
    {
        surf->shader = tr.defaultShader;
    }
    
    // we may have a nodraw surface, because they might still need to
    // be around for movement clipping
    if( s_worldData.shaders[ LittleLong( ds->shaderNum ) ].surfaceFlags & SURF_NODRAW )
    {
        surf->data = &skipData;
        return;
    }
    
    width = LittleLong( ds->patchWidth );
    height = LittleLong( ds->patchHeight );
    
    if( width < 0 || width > MAX_PATCH_SIZE || height < 0 || height > MAX_PATCH_SIZE )
        Com_Error( ERR_DROP, "ParseMesh: bad size" );
        
    verts += LittleLong( ds->firstVert );
    numPoints = width * height;
    #pragma omp critical
    for( i = 0; i < numPoints; i++ )
        LoadDrawVertToSrfVert( &points[i], &verts[i], realLightmapNum, hdrVertColors ? hdrVertColors + ( ds->firstVert + i ) * 3 : NULL, NULL );
        
    // pre-tesseleate
    R_SubdividePatchToGrid( grid, width, height, points );
    
    // copy the level of detail origin, which is the center
    // of the group of all curves that must subdivide the same
    // to avoid cracking
    #pragma omp critical
    for( i = 0 ; i < 3 ; i++ )
    {
        bounds[0][i] = LittleFloat( ds->lightmapVecs[0][i] );
        bounds[1][i] = LittleFloat( ds->lightmapVecs[1][i] );
    }
    VectorAdd( bounds[0], bounds[1], bounds[1] );
    VectorScale( bounds[1], 0.5f, grid->lodOrigin );
    VectorSubtract( bounds[0], grid->lodOrigin, tmpVec );
    grid->lodRadius = VectorLength( tmpVec );
    
    surf->cullinfo.type = CULLINFO_BOX | CULLINFO_SPHERE;
    VectorCopy( grid->cullBounds[0], surf->cullinfo.bounds[0] );
    VectorCopy( grid->cullBounds[1], surf->cullinfo.bounds[1] );
    VectorCopy( grid->cullOrigin, surf->cullinfo.localOrigin );
    surf->cullinfo.radius = grid->cullRadius;
}

/*
===============
ParseTriSurf
===============
*/
static void ParseTriSurf( dsurface_t* ds, drawVert_t* verts, F32* hdrVertColors, msurface_t* surf, S32* indexes )
{
    srfBspSurface_t* cv;
    U32*  tri;
    S32             i, j;
    S32             numVerts, numIndexes, badTriangles;
    
    // get fog volume
    surf->fogIndex = LittleLong( ds->fogNum ) + 1;
    
    // get shader
    surf->shader = ShaderForShaderNum( ds->shaderNum, LIGHTMAP_BY_VERTEX );
    if( r_singleShader->integer && !surf->shader->isSky )
    {
        surf->shader = tr.defaultShader;
    }
    
    numVerts = LittleLong( ds->numVerts );
    numIndexes = LittleLong( ds->numIndexes );
    
    //cv = Hunk_Alloc(sizeof(*cv), h_low);
    cv = ( srfBspSurface_t* )surf->data;
    cv->surfaceType = SF_TRIANGLES;
    
    cv->numIndexes = numIndexes;
    cv->indexes = reinterpret_cast<U32*>( Hunk_Alloc( numIndexes * sizeof( cv->indexes[0] ), h_low ) );
    
    cv->numVerts = numVerts;
    cv->verts = reinterpret_cast<srfVert_t*>( Hunk_Alloc( numVerts * sizeof( cv->verts[0] ), h_low ) );
    
    surf->data = ( surfaceType_t* ) cv;
    
    // copy vertexes
    surf->cullinfo.type = CULLINFO_BOX;
    ClearBounds( surf->cullinfo.bounds[0], surf->cullinfo.bounds[1] );
    verts += LittleLong( ds->firstVert );
    #pragma omp critical
    for( i = 0; i < numVerts; i++ )
        LoadDrawVertToSrfVert( &cv->verts[i], &verts[i], -1, hdrVertColors ? hdrVertColors + ( ds->firstVert + i ) * 3 : NULL, surf->cullinfo.bounds );
        
    // copy triangles
    badTriangles = 0;
    indexes += LittleLong( ds->firstIndex );
    #pragma omp critical
    for( i = 0, tri = cv->indexes; i < numIndexes; i += 3, tri += 3 )
    {
        for( j = 0; j < 3; j++ )
        {
            tri[j] = LittleLong( indexes[i + j] );
            
            if( tri[j] >= numVerts )
            {
                Com_Error( ERR_DROP, "Bad index in face surface" );
            }
        }
        
        if( ( tri[0] == tri[1] ) || ( tri[1] == tri[2] ) || ( tri[0] == tri[2] ) )
        {
            tri -= 3;
            badTriangles++;
        }
    }
    
    if( badTriangles )
    {
        CL_RefPrintf( PRINT_WARNING, "Trisurf has bad triangles, originally shader %s %d tris %d verts, now %d tris\n", surf->shader->name, numIndexes / 3, numVerts, numIndexes / 3 - badTriangles );
        cv->numIndexes -= badTriangles * 3;
    }
    
    // Calculate tangent spaces
    {
        srfVert_t*      dv[3];
        
        #pragma omp critical
        for( i = 0, tri = cv->indexes; i < numIndexes; i += 3, tri += 3 )
        {
            dv[0] = &cv->verts[tri[0]];
            dv[1] = &cv->verts[tri[1]];
            dv[2] = &cv->verts[tri[2]];
            
            R_CalcTangentVectors( dv );
        }
    }
}

/*
===============
ParseFlare
===============
*/
static void ParseFlare( dsurface_t* ds, drawVert_t* verts, msurface_t* surf, S32* indexes )
{
    srfFlare_t*		flare;
    S32				i;
    
    // get fog volume
    surf->fogIndex = LittleLong( ds->fogNum ) + 1;
    
    // get shader
    surf->shader = ShaderForShaderNum( ds->shaderNum, LIGHTMAP_BY_VERTEX );
    if( r_singleShader->integer && !surf->shader->isSky )
    {
        surf->shader = tr.defaultShader;
    }
    
    //flare = Hunk_Alloc( sizeof( *flare ), h_low );
    flare = ( srfFlare_t* )surf->data;
    flare->surfaceType = SF_FLARE;
    
    surf->data = ( surfaceType_t* )flare;
    
    #pragma omp critical
    for( i = 0 ; i < 3 ; i++ )
    {
        flare->origin[i] = LittleFloat( ds->lightmapOrigin[i] );
        flare->color[i] = LittleFloat( ds->lightmapVecs[0][i] );
        flare->normal[i] = LittleFloat( ds->lightmapVecs[2][i] );
    }
    
    surf->cullinfo.type = CULLINFO_NONE;
}


/*
=================
R_MergedWidthPoints

returns true if there are grid points merged on a width edge
=================
*/
S32 R_MergedWidthPoints( srfBspSurface_t* grid, S32 offset )
{
    S32 i, j;
    
    #pragma omp critical
    for( i = 1; i < grid->width - 1; i++ )
    {
        for( j = i + 1; j < grid->width - 1; j++ )
        {
            if( fabs( grid->verts[i + offset].xyz[0] - grid->verts[j + offset].xyz[0] ) > .1 ) continue;
            if( fabs( grid->verts[i + offset].xyz[1] - grid->verts[j + offset].xyz[1] ) > .1 ) continue;
            if( fabs( grid->verts[i + offset].xyz[2] - grid->verts[j + offset].xyz[2] ) > .1 ) continue;
            return true;
        }
    }
    return false;
}

/*
=================
R_MergedHeightPoints

returns true if there are grid points merged on a height edge
=================
*/
S32 R_MergedHeightPoints( srfBspSurface_t* grid, S32 offset )
{
    S32 i, j;
    
    #pragma omp critical
    for( i = 1; i < grid->height - 1; i++ )
    {
        for( j = i + 1; j < grid->height - 1; j++ )
        {
            if( fabs( grid->verts[grid->width * i + offset].xyz[0] - grid->verts[grid->width * j + offset].xyz[0] ) > .1 ) continue;
            if( fabs( grid->verts[grid->width * i + offset].xyz[1] - grid->verts[grid->width * j + offset].xyz[1] ) > .1 ) continue;
            if( fabs( grid->verts[grid->width * i + offset].xyz[2] - grid->verts[grid->width * j + offset].xyz[2] ) > .1 ) continue;
            return true;
        }
    }
    return false;
}

/*
=================
R_FixSharedVertexLodError_r

NOTE: never sync LoD through grid edges with merged points!

FIXME: write generalized version that also avoids cracks between a patch and one that meets half way?
=================
*/
void R_FixSharedVertexLodError_r( S32 start, srfBspSurface_t* grid1 )
{
    S32 j, k, l, m, n, offset1, offset2, touch;
    srfBspSurface_t* grid2;
    
    #pragma omp critical
    for( j = start; j < s_worldData.numsurfaces; j++ )
    {
        //
        grid2 = ( srfBspSurface_t* ) s_worldData.surfaces[j].data;
        // if this surface is not a grid
        if( grid2->surfaceType != SF_GRID ) continue;
        // if the LOD errors are already fixed for this patch
        if( grid2->lodFixed == 2 ) continue;
        // grids in the same LOD group should have the exact same lod radius
        if( grid1->lodRadius != grid2->lodRadius ) continue;
        // grids in the same LOD group should have the exact same lod origin
        if( grid1->lodOrigin[0] != grid2->lodOrigin[0] ) continue;
        if( grid1->lodOrigin[1] != grid2->lodOrigin[1] ) continue;
        if( grid1->lodOrigin[2] != grid2->lodOrigin[2] ) continue;
        //
        touch = false;
        for( n = 0; n < 2; n++ )
        {
            //
            if( n ) offset1 = ( grid1->height - 1 ) * grid1->width;
            else offset1 = 0;
            if( R_MergedWidthPoints( grid1, offset1 ) ) continue;
            for( k = 1; k < grid1->width - 1; k++ )
            {
                for( m = 0; m < 2; m++ )
                {
                
                    if( m ) offset2 = ( grid2->height - 1 ) * grid2->width;
                    else offset2 = 0;
                    if( R_MergedWidthPoints( grid2, offset2 ) ) continue;
                    for( l = 1; l < grid2->width - 1; l++ )
                    {
                        //
                        if( fabs( grid1->verts[k + offset1].xyz[0] - grid2->verts[l + offset2].xyz[0] ) > .1 ) continue;
                        if( fabs( grid1->verts[k + offset1].xyz[1] - grid2->verts[l + offset2].xyz[1] ) > .1 ) continue;
                        if( fabs( grid1->verts[k + offset1].xyz[2] - grid2->verts[l + offset2].xyz[2] ) > .1 ) continue;
                        // ok the points are equal and should have the same lod error
                        grid2->widthLodError[l] = grid1->widthLodError[k];
                        touch = true;
                    }
                }
                for( m = 0; m < 2; m++ )
                {
                
                    if( m ) offset2 = grid2->width - 1;
                    else offset2 = 0;
                    if( R_MergedHeightPoints( grid2, offset2 ) ) continue;
                    for( l = 1; l < grid2->height - 1; l++ )
                    {
                        //
                        if( fabs( grid1->verts[k + offset1].xyz[0] - grid2->verts[grid2->width * l + offset2].xyz[0] ) > .1 ) continue;
                        if( fabs( grid1->verts[k + offset1].xyz[1] - grid2->verts[grid2->width * l + offset2].xyz[1] ) > .1 ) continue;
                        if( fabs( grid1->verts[k + offset1].xyz[2] - grid2->verts[grid2->width * l + offset2].xyz[2] ) > .1 ) continue;
                        // ok the points are equal and should have the same lod error
                        grid2->heightLodError[l] = grid1->widthLodError[k];
                        touch = true;
                    }
                }
            }
        }
        
        #pragma omp critical
        for( n = 0; n < 2; n++ )
        {
            //
            if( n ) offset1 = grid1->width - 1;
            else offset1 = 0;
            if( R_MergedHeightPoints( grid1, offset1 ) ) continue;
            for( k = 1; k < grid1->height - 1; k++ )
            {
                for( m = 0; m < 2; m++ )
                {
                
                    if( m ) offset2 = ( grid2->height - 1 ) * grid2->width;
                    else offset2 = 0;
                    if( R_MergedWidthPoints( grid2, offset2 ) ) continue;
                    for( l = 1; l < grid2->width - 1; l++ )
                    {
                        //
                        if( fabs( grid1->verts[grid1->width * k + offset1].xyz[0] - grid2->verts[l + offset2].xyz[0] ) > .1 ) continue;
                        if( fabs( grid1->verts[grid1->width * k + offset1].xyz[1] - grid2->verts[l + offset2].xyz[1] ) > .1 ) continue;
                        if( fabs( grid1->verts[grid1->width * k + offset1].xyz[2] - grid2->verts[l + offset2].xyz[2] ) > .1 ) continue;
                        // ok the points are equal and should have the same lod error
                        grid2->widthLodError[l] = grid1->heightLodError[k];
                        touch = true;
                    }
                }
                for( m = 0; m < 2; m++ )
                {
                
                    if( m ) offset2 = grid2->width - 1;
                    else offset2 = 0;
                    if( R_MergedHeightPoints( grid2, offset2 ) ) continue;
                    for( l = 1; l < grid2->height - 1; l++ )
                    {
                        //
                        if( fabs( grid1->verts[grid1->width * k + offset1].xyz[0] - grid2->verts[grid2->width * l + offset2].xyz[0] ) > .1 ) continue;
                        if( fabs( grid1->verts[grid1->width * k + offset1].xyz[1] - grid2->verts[grid2->width * l + offset2].xyz[1] ) > .1 ) continue;
                        if( fabs( grid1->verts[grid1->width * k + offset1].xyz[2] - grid2->verts[grid2->width * l + offset2].xyz[2] ) > .1 ) continue;
                        // ok the points are equal and should have the same lod error
                        grid2->heightLodError[l] = grid1->heightLodError[k];
                        touch = true;
                    }
                }
            }
        }
        if( touch )
        {
            grid2->lodFixed = 2;
            R_FixSharedVertexLodError_r( start, grid2 );
            //NOTE: this would be correct but makes things really slow
            //grid2->lodFixed = 1;
        }
    }
}

/*
=================
R_FixSharedVertexLodError

This function assumes that all patches in one group are nicely stitched together for the highest LoD.
If this is not the case this function will still do its job but won't fix the highest LoD cracks.
=================
*/
void R_FixSharedVertexLodError( void )
{
    S32 i;
    srfBspSurface_t* grid1;
    
    #pragma omp parallel for
    for( i = 0; i < s_worldData.numsurfaces; i++ )
    {
        //
        grid1 = ( srfBspSurface_t* ) s_worldData.surfaces[i].data;
        // if this surface is not a grid
        if( grid1->surfaceType != SF_GRID )
            continue;
        //
        if( grid1->lodFixed )
            continue;
        //
        grid1->lodFixed = 2;
        // recursively fix other patches in the same LOD group
        R_FixSharedVertexLodError_r( i + 1, grid1 );
    }
}


/*
===============
R_StitchPatches
===============
*/
S32 R_StitchPatches( S32 grid1num, S32 grid2num )
{
    F32* v1, *v2;
    srfBspSurface_t* grid1, *grid2;
    S32 k, l, m, n, offset1, offset2, row, column;
    
    grid1 = ( srfBspSurface_t* ) s_worldData.surfaces[grid1num].data;
    grid2 = ( srfBspSurface_t* ) s_worldData.surfaces[grid2num].data;
    
    #pragma omp parallel for
    for( n = 0; n < 2; n++ )
    {
        //
        if( n ) offset1 = ( grid1->height - 1 ) * grid1->width;
        else offset1 = 0;
        if( R_MergedWidthPoints( grid1, offset1 ) )
            continue;
        for( k = 0; k < grid1->width - 2; k += 2 )
        {
        
            for( m = 0; m < 2; m++ )
            {
            
                if( grid2->width >= MAX_GRID_SIZE )
                    break;
                if( m ) offset2 = ( grid2->height - 1 ) * grid2->width;
                else offset2 = 0;
                for( l = 0; l < grid2->width - 1; l++ )
                {
                    //
                    v1 = grid1->verts[k + offset1].xyz;
                    v2 = grid2->verts[l + offset2].xyz;
                    if( fabs( v1[0] - v2[0] ) > .1 )
                        continue;
                    if( fabs( v1[1] - v2[1] ) > .1 )
                        continue;
                    if( fabs( v1[2] - v2[2] ) > .1 )
                        continue;
                        
                    v1 = grid1->verts[k + 2 + offset1].xyz;
                    v2 = grid2->verts[l + 1 + offset2].xyz;
                    if( fabs( v1[0] - v2[0] ) > .1 )
                        continue;
                    if( fabs( v1[1] - v2[1] ) > .1 )
                        continue;
                    if( fabs( v1[2] - v2[2] ) > .1 )
                        continue;
                    //
                    v1 = grid2->verts[l + offset2].xyz;
                    v2 = grid2->verts[l + 1 + offset2].xyz;
                    if( fabs( v1[0] - v2[0] ) < .01 &&
                            fabs( v1[1] - v2[1] ) < .01 &&
                            fabs( v1[2] - v2[2] ) < .01 )
                        continue;
                    //
                    //CL_RefPrintf( PRINT_ALL, "found highest LoD crack between two patches\n" );
                    // insert column into grid2 right after after column l
                    if( m ) row = grid2->height - 1;
                    else row = 0;
                    R_GridInsertColumn( grid2, l + 1, row,
                                        grid1->verts[k + 1 + offset1].xyz, grid1->widthLodError[k + 1] );
                    grid2->lodStitched = false;
                    s_worldData.surfaces[grid2num].data = ( surfaceType_t* ) grid2;
                    return true;
                }
            }
            for( m = 0; m < 2; m++ )
            {
            
                if( grid2->height >= MAX_GRID_SIZE )
                    break;
                if( m ) offset2 = grid2->width - 1;
                else offset2 = 0;
                for( l = 0; l < grid2->height - 1; l++ )
                {
                    //
                    v1 = grid1->verts[k + offset1].xyz;
                    v2 = grid2->verts[grid2->width * l + offset2].xyz;
                    if( fabs( v1[0] - v2[0] ) > .1 )
                        continue;
                    if( fabs( v1[1] - v2[1] ) > .1 )
                        continue;
                    if( fabs( v1[2] - v2[2] ) > .1 )
                        continue;
                        
                    v1 = grid1->verts[k + 2 + offset1].xyz;
                    v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
                    if( fabs( v1[0] - v2[0] ) > .1 )
                        continue;
                    if( fabs( v1[1] - v2[1] ) > .1 )
                        continue;
                    if( fabs( v1[2] - v2[2] ) > .1 )
                        continue;
                    //
                    v1 = grid2->verts[grid2->width * l + offset2].xyz;
                    v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
                    if( fabs( v1[0] - v2[0] ) < .01 &&
                            fabs( v1[1] - v2[1] ) < .01 &&
                            fabs( v1[2] - v2[2] ) < .01 )
                        continue;
                    //
                    //CL_RefPrintf( PRINT_ALL, "found highest LoD crack between two patches\n" );
                    // insert row into grid2 right after after row l
                    if( m ) column = grid2->width - 1;
                    else column = 0;
                    R_GridInsertRow( grid2, l + 1, column,
                                     grid1->verts[k + 1 + offset1].xyz, grid1->widthLodError[k + 1] );
                    grid2->lodStitched = false;
                    s_worldData.surfaces[grid2num].data = ( surfaceType_t* ) grid2;
                    return true;
                }
            }
        }
    }
    #pragma omp parallel for
    for( n = 0; n < 2; n++ )
    {
        //
        if( n ) offset1 = grid1->width - 1;
        else offset1 = 0;
        if( R_MergedHeightPoints( grid1, offset1 ) )
            continue;
        for( k = 0; k < grid1->height - 2; k += 2 )
        {
            for( m = 0; m < 2; m++ )
            {
            
                if( grid2->width >= MAX_GRID_SIZE )
                    break;
                if( m ) offset2 = ( grid2->height - 1 ) * grid2->width;
                else offset2 = 0;
                for( l = 0; l < grid2->width - 1; l++ )
                {
                    //
                    v1 = grid1->verts[grid1->width * k + offset1].xyz;
                    v2 = grid2->verts[l + offset2].xyz;
                    if( fabs( v1[0] - v2[0] ) > .1 )
                        continue;
                    if( fabs( v1[1] - v2[1] ) > .1 )
                        continue;
                    if( fabs( v1[2] - v2[2] ) > .1 )
                        continue;
                        
                    v1 = grid1->verts[grid1->width * ( k + 2 ) + offset1].xyz;
                    v2 = grid2->verts[l + 1 + offset2].xyz;
                    if( fabs( v1[0] - v2[0] ) > .1 )
                        continue;
                    if( fabs( v1[1] - v2[1] ) > .1 )
                        continue;
                    if( fabs( v1[2] - v2[2] ) > .1 )
                        continue;
                    //
                    v1 = grid2->verts[l + offset2].xyz;
                    v2 = grid2->verts[( l + 1 ) + offset2].xyz;
                    if( fabs( v1[0] - v2[0] ) < .01 &&
                            fabs( v1[1] - v2[1] ) < .01 &&
                            fabs( v1[2] - v2[2] ) < .01 )
                        continue;
                    //
                    //CL_RefPrintf( PRINT_ALL, "found highest LoD crack between two patches\n" );
                    // insert column into grid2 right after after column l
                    if( m ) row = grid2->height - 1;
                    else row = 0;
                    R_GridInsertColumn( grid2, l + 1, row,
                                        grid1->verts[grid1->width * ( k + 1 ) + offset1].xyz, grid1->heightLodError[k + 1] );
                    grid2->lodStitched = false;
                    s_worldData.surfaces[grid2num].data = ( surfaceType_t* ) grid2;
                    return true;
                }
            }
            for( m = 0; m < 2; m++ )
            {
            
                if( grid2->height >= MAX_GRID_SIZE )
                    break;
                if( m ) offset2 = grid2->width - 1;
                else offset2 = 0;
                for( l = 0; l < grid2->height - 1; l++ )
                {
                    //
                    v1 = grid1->verts[grid1->width * k + offset1].xyz;
                    v2 = grid2->verts[grid2->width * l + offset2].xyz;
                    if( fabs( v1[0] - v2[0] ) > .1 )
                        continue;
                    if( fabs( v1[1] - v2[1] ) > .1 )
                        continue;
                    if( fabs( v1[2] - v2[2] ) > .1 )
                        continue;
                        
                    v1 = grid1->verts[grid1->width * ( k + 2 ) + offset1].xyz;
                    v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
                    if( fabs( v1[0] - v2[0] ) > .1 )
                        continue;
                    if( fabs( v1[1] - v2[1] ) > .1 )
                        continue;
                    if( fabs( v1[2] - v2[2] ) > .1 )
                        continue;
                    //
                    v1 = grid2->verts[grid2->width * l + offset2].xyz;
                    v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
                    if( fabs( v1[0] - v2[0] ) < .01 &&
                            fabs( v1[1] - v2[1] ) < .01 &&
                            fabs( v1[2] - v2[2] ) < .01 )
                        continue;
                    //
                    //CL_RefPrintf( PRINT_ALL, "found highest LoD crack between two patches\n" );
                    // insert row into grid2 right after after row l
                    if( m ) column = grid2->width - 1;
                    else column = 0;
                    R_GridInsertRow( grid2, l + 1, column,
                                     grid1->verts[grid1->width * ( k + 1 ) + offset1].xyz, grid1->heightLodError[k + 1] );
                    grid2->lodStitched = false;
                    s_worldData.surfaces[grid2num].data = ( surfaceType_t* ) grid2;
                    return true;
                }
            }
        }
    }
    for( n = 0; n < 2; n++ )
    {
        //
        if( n ) offset1 = ( grid1->height - 1 ) * grid1->width;
        else offset1 = 0;
        if( R_MergedWidthPoints( grid1, offset1 ) )
            continue;
        for( k = grid1->width - 1; k > 1; k -= 2 )
        {
        
            for( m = 0; m < 2; m++ )
            {
            
                if( !grid2 || grid2->width >= MAX_GRID_SIZE )
                    break;
                if( m ) offset2 = ( grid2->height - 1 ) * grid2->width;
                else offset2 = 0;
                for( l = 0; l < grid2->width - 1; l++ )
                {
                    //
                    v1 = grid1->verts[k + offset1].xyz;
                    v2 = grid2->verts[l + offset2].xyz;
                    if( fabs( v1[0] - v2[0] ) > .1 )
                        continue;
                    if( fabs( v1[1] - v2[1] ) > .1 )
                        continue;
                    if( fabs( v1[2] - v2[2] ) > .1 )
                        continue;
                        
                    v1 = grid1->verts[k - 2 + offset1].xyz;
                    v2 = grid2->verts[l + 1 + offset2].xyz;
                    if( fabs( v1[0] - v2[0] ) > .1 )
                        continue;
                    if( fabs( v1[1] - v2[1] ) > .1 )
                        continue;
                    if( fabs( v1[2] - v2[2] ) > .1 )
                        continue;
                    //
                    v1 = grid2->verts[l + offset2].xyz;
                    v2 = grid2->verts[( l + 1 ) + offset2].xyz;
                    if( fabs( v1[0] - v2[0] ) < .01 &&
                            fabs( v1[1] - v2[1] ) < .01 &&
                            fabs( v1[2] - v2[2] ) < .01 )
                        continue;
                    //
                    //CL_RefPrintf( PRINT_ALL, "found highest LoD crack between two patches\n" );
                    // insert column into grid2 right after after column l
                    if( m ) row = grid2->height - 1;
                    else row = 0;
                    R_GridInsertColumn( grid2, l + 1, row,
                                        grid1->verts[k - 1 + offset1].xyz, grid1->widthLodError[k + 1] );
                    grid2->lodStitched = false;
                    s_worldData.surfaces[grid2num].data = ( surfaceType_t* ) grid2;
                    return true;
                }
            }
            for( m = 0; m < 2; m++ )
            {
            
                if( !grid2 || grid2->height >= MAX_GRID_SIZE )
                    break;
                if( m ) offset2 = grid2->width - 1;
                else offset2 = 0;
                for( l = 0; l < grid2->height - 1; l++ )
                {
                    //
                    v1 = grid1->verts[k + offset1].xyz;
                    v2 = grid2->verts[grid2->width * l + offset2].xyz;
                    if( fabs( v1[0] - v2[0] ) > .1 )
                        continue;
                    if( fabs( v1[1] - v2[1] ) > .1 )
                        continue;
                    if( fabs( v1[2] - v2[2] ) > .1 )
                        continue;
                        
                    v1 = grid1->verts[k - 2 + offset1].xyz;
                    v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
                    if( fabs( v1[0] - v2[0] ) > .1 )
                        continue;
                    if( fabs( v1[1] - v2[1] ) > .1 )
                        continue;
                    if( fabs( v1[2] - v2[2] ) > .1 )
                        continue;
                    //
                    v1 = grid2->verts[grid2->width * l + offset2].xyz;
                    v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
                    if( fabs( v1[0] - v2[0] ) < .01 &&
                            fabs( v1[1] - v2[1] ) < .01 &&
                            fabs( v1[2] - v2[2] ) < .01 )
                        continue;
                    //
                    //CL_RefPrintf( PRINT_ALL, "found highest LoD crack between two patches\n" );
                    // insert row into grid2 right after after row l
                    if( m ) column = grid2->width - 1;
                    else column = 0;
                    R_GridInsertRow( grid2, l + 1, column,
                                     grid1->verts[k - 1 + offset1].xyz, grid1->widthLodError[k + 1] );
                    if( !grid2 )
                        break;
                    grid2->lodStitched = false;
                    s_worldData.surfaces[grid2num].data = ( surfaceType_t* ) grid2;
                    return true;
                }
            }
        }
    }
    for( n = 0; n < 2; n++ )
    {
        //
        if( n ) offset1 = grid1->width - 1;
        else offset1 = 0;
        if( R_MergedHeightPoints( grid1, offset1 ) )
            continue;
        for( k = grid1->height - 1; k > 1; k -= 2 )
        {
            for( m = 0; m < 2; m++ )
            {
            
                if( !grid2 || grid2->width >= MAX_GRID_SIZE )
                    break;
                if( m ) offset2 = ( grid2->height - 1 ) * grid2->width;
                else offset2 = 0;
                for( l = 0; l < grid2->width - 1; l++ )
                {
                    //
                    v1 = grid1->verts[grid1->width * k + offset1].xyz;
                    v2 = grid2->verts[l + offset2].xyz;
                    if( fabs( v1[0] - v2[0] ) > .1 )
                        continue;
                    if( fabs( v1[1] - v2[1] ) > .1 )
                        continue;
                    if( fabs( v1[2] - v2[2] ) > .1 )
                        continue;
                        
                    v1 = grid1->verts[grid1->width * ( k - 2 ) + offset1].xyz;
                    v2 = grid2->verts[l + 1 + offset2].xyz;
                    if( fabs( v1[0] - v2[0] ) > .1 )
                        continue;
                    if( fabs( v1[1] - v2[1] ) > .1 )
                        continue;
                    if( fabs( v1[2] - v2[2] ) > .1 )
                        continue;
                    //
                    v1 = grid2->verts[l + offset2].xyz;
                    v2 = grid2->verts[( l + 1 ) + offset2].xyz;
                    if( fabs( v1[0] - v2[0] ) < .01 &&
                            fabs( v1[1] - v2[1] ) < .01 &&
                            fabs( v1[2] - v2[2] ) < .01 )
                        continue;
                    //
                    //CL_RefPrintf( PRINT_ALL, "found highest LoD crack between two patches\n" );
                    // insert column into grid2 right after after column l
                    if( m ) row = grid2->height - 1;
                    else row = 0;
                    R_GridInsertColumn( grid2, l + 1, row,
                                        grid1->verts[grid1->width * ( k - 1 ) + offset1].xyz, grid1->heightLodError[k + 1] );
                    grid2->lodStitched = false;
                    s_worldData.surfaces[grid2num].data = ( surfaceType_t* ) grid2;
                    return true;
                }
            }
            for( m = 0; m < 2; m++ )
            {
            
                if( !grid2 || grid2->height >= MAX_GRID_SIZE )
                    break;
                if( m ) offset2 = grid2->width - 1;
                else offset2 = 0;
                for( l = 0; l < grid2->height - 1; l++ )
                {
                    //
                    v1 = grid1->verts[grid1->width * k + offset1].xyz;
                    v2 = grid2->verts[grid2->width * l + offset2].xyz;
                    if( fabs( v1[0] - v2[0] ) > .1 )
                        continue;
                    if( fabs( v1[1] - v2[1] ) > .1 )
                        continue;
                    if( fabs( v1[2] - v2[2] ) > .1 )
                        continue;
                        
                    v1 = grid1->verts[grid1->width * ( k - 2 ) + offset1].xyz;
                    v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
                    if( fabs( v1[0] - v2[0] ) > .1 )
                        continue;
                    if( fabs( v1[1] - v2[1] ) > .1 )
                        continue;
                    if( fabs( v1[2] - v2[2] ) > .1 )
                        continue;
                    //
                    v1 = grid2->verts[grid2->width * l + offset2].xyz;
                    v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
                    if( fabs( v1[0] - v2[0] ) < .01 &&
                            fabs( v1[1] - v2[1] ) < .01 &&
                            fabs( v1[2] - v2[2] ) < .01 )
                        continue;
                    //
                    //CL_RefPrintf( PRINT_ALL, "found highest LoD crack between two patches\n" );
                    // insert row into grid2 right after after row l
                    if( m ) column = grid2->width - 1;
                    else column = 0;
                    R_GridInsertRow( grid2, l + 1, column,
                                     grid1->verts[grid1->width * ( k - 1 ) + offset1].xyz, grid1->heightLodError[k + 1] );
                    grid2->lodStitched = false;
                    s_worldData.surfaces[grid2num].data = ( surfaceType_t* ) grid2;
                    return true;
                }
            }
        }
    }
    return false;
}

/*
===============
R_TryStitchPatch

This function will try to stitch patches in the same LoD group together for the highest LoD.

Only single missing vertice cracks will be fixed.

Vertices will be joined at the patch side a crack is first found, at the other side
of the patch (on the same row or column) the vertices will not be joined and cracks
might still appear at that side.
===============
*/
S32 R_TryStitchingPatch( S32 grid1num )
{
    S32 j, numstitches;
    srfBspSurface_t* grid1, *grid2;
    
    numstitches = 0;
    grid1 = ( srfBspSurface_t* ) s_worldData.surfaces[grid1num].data;
    for( j = 0; j < s_worldData.numsurfaces; j++ )
    {
        //
        grid2 = ( srfBspSurface_t* ) s_worldData.surfaces[j].data;
        // if this surface is not a grid
        if( grid2->surfaceType != SF_GRID ) continue;
        // grids in the same LOD group should have the exact same lod radius
        if( grid1->lodRadius != grid2->lodRadius ) continue;
        // grids in the same LOD group should have the exact same lod origin
        if( grid1->lodOrigin[0] != grid2->lodOrigin[0] ) continue;
        if( grid1->lodOrigin[1] != grid2->lodOrigin[1] ) continue;
        if( grid1->lodOrigin[2] != grid2->lodOrigin[2] ) continue;
        //
        while( R_StitchPatches( grid1num, j ) )
        {
            numstitches++;
        }
    }
    return numstitches;
}

/*
===============
R_StitchAllPatches
===============
*/
void R_StitchAllPatches( void )
{
    S32 i, stitched, numstitches;
    srfBspSurface_t* grid1;
    
    numstitches = 0;
    do
    {
        stitched = false;
        for( i = 0; i < s_worldData.numsurfaces; i++ )
        {
            //
            grid1 = ( srfBspSurface_t* ) s_worldData.surfaces[i].data;
            // if this surface is not a grid
            if( grid1->surfaceType != SF_GRID )
                continue;
            //
            if( grid1->lodStitched )
                continue;
            //
            grid1->lodStitched = true;
            stitched = true;
            //
            numstitches += R_TryStitchingPatch( i );
        }
    }
    while( stitched );
    CL_RefPrintf( PRINT_ALL, "stitched %d LoD cracks\n", numstitches );
}

/*
===============
R_MovePatchSurfacesToHunk
===============
*/
void R_MovePatchSurfacesToHunk( void )
{
    S32 i;
    srfBspSurface_t* grid;
    
    for( i = 0; i < s_worldData.numsurfaces; i++ )
    {
        void* copyFrom;
        //
        grid = ( srfBspSurface_t* ) s_worldData.surfaces[i].data;
        // if this surface is not a grid
        if( grid->surfaceType != SF_GRID )
            continue;
        //
        
        copyFrom = grid->widthLodError;
        grid->widthLodError = reinterpret_cast<F32*>( Hunk_Alloc( grid->width * 4, h_low ) );
        ::memcpy( grid->widthLodError, copyFrom, grid->width * 4 );
        Z_Free( copyFrom );
        
        copyFrom = grid->heightLodError;
        grid->heightLodError = reinterpret_cast<F32*>( Hunk_Alloc( grid->height * 4, h_low ) );
        ::memcpy( grid->heightLodError, copyFrom, grid->height * 4 );
        Z_Free( copyFrom );
        
        copyFrom = grid->indexes;
        grid->indexes = reinterpret_cast<U32*>( Hunk_Alloc( grid->numIndexes * sizeof( U32 ), h_low ) );
        ::memcpy( grid->indexes, copyFrom, grid->numIndexes * sizeof( U32 ) );
        Z_Free( copyFrom );
        
        copyFrom = grid->verts;
        grid->verts = reinterpret_cast<srfVert_t*>( Hunk_Alloc( grid->numVerts * sizeof( srfVert_t ), h_low ) );
        ::memcpy( grid->verts, copyFrom, grid->numVerts * sizeof( srfVert_t ) );
        Z_Free( copyFrom );
    }
}


/*
===============
R_LoadSurfaces
===============
*/
static	void R_LoadSurfaces( lump_t* surfs, lump_t* verts, lump_t* indexLump )
{
    dsurface_t*	in;
    msurface_t*	out = nullptr;
    drawVert_t*	dv;
    S32*			indexes;
    S32			count;
    S32			numFaces, numMeshes, numTriSurfs, numFlares;
    S32			i;
    F32* hdrVertColors = NULL;
    
    numFaces = 0;
    numMeshes = 0;
    numTriSurfs = 0;
    numFlares = 0;
    
    if( surfs->filelen % sizeof( *in ) )
        Com_Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
    count = surfs->filelen / sizeof( *in );
    
    dv = ( drawVert_t* )( fileBase + verts->fileofs );
    if( verts->filelen % sizeof( *dv ) )
        Com_Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
        
    indexes = ( S32* )( fileBase + indexLump->fileofs );
    if( indexLump->filelen % sizeof( *indexes ) )
        Com_Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
        
    out = reinterpret_cast<msurface_t*>( Hunk_Alloc( count * sizeof( *out ), h_low ) );
    
    s_worldData.surfaces = out;
    s_worldData.numsurfaces = count;
    s_worldData.surfacesViewCount = reinterpret_cast<S32*>( Hunk_Alloc( count * sizeof( *s_worldData.surfacesViewCount ), h_low ) );
    s_worldData.surfacesDlightBits = reinterpret_cast<S32*>( Hunk_Alloc( count * sizeof( *s_worldData.surfacesDlightBits ), h_low ) );
    s_worldData.surfacesPshadowBits = reinterpret_cast<S32*>( Hunk_Alloc( count * sizeof( *s_worldData.surfacesPshadowBits ), h_low ) );
    
    // load hdr vertex colors
    if( r_hdr->integer )
    {
        UTF8 filename[MAX_QPATH];
        S32 size;
        
        Com_sprintf( filename, sizeof( filename ), "maps/%s/vertlight.raw", s_worldData.baseName );
        //CL_RefPrintf(PRINT_ALL, "looking for %s\n", filename);
        
        size = fileSystem->ReadFile( filename, ( void** )&hdrVertColors );
        
        if( hdrVertColors )
        {
            //CL_RefPrintf(PRINT_ALL, "Found!\n");
            if( size != sizeof( F32 ) * 3 * ( verts->filelen / sizeof( *dv ) ) )
                Com_Error( ERR_DROP, "Bad size for %s (%i, expected %i)!", filename, size, ( S32 )( ( sizeof( F32 ) ) * 3 * ( verts->filelen / sizeof( *dv ) ) ) );
        }
    }
    
    
    // Two passes, allocate surfaces first, then load them full of data
    // This ensures surfaces are close together to reduce L2 cache misses when using VAOs,
    // which don't actually use the verts and indexes
    in = ( dsurface_t* )( fileBase + surfs->fileofs );
    out = s_worldData.surfaces;
    for( i = 0 ; i < count ; i++, in++, out++ )
    {
        switch( LittleLong( in->surfaceType ) )
        {
            case MST_PATCH:
                out->data = reinterpret_cast<surfaceType_t*>( Hunk_Alloc( sizeof( srfBspSurface_t ), h_low ) );
                break;
            case MST_TRIANGLE_SOUP:
                out->data = reinterpret_cast<surfaceType_t*>( Hunk_Alloc( sizeof( srfBspSurface_t ), h_low ) );
                break;
            case MST_PLANAR:
                out->data = reinterpret_cast<surfaceType_t*>( Hunk_Alloc( sizeof( srfBspSurface_t ), h_low ) );
                break;
            case MST_FLARE:
                out->data = reinterpret_cast<surfaceType_t*>( Hunk_Alloc( sizeof( srfFlare_t ), h_low ) );
                break;
            default:
                break;
        }
    }
    
    in = ( dsurface_t* )( fileBase + surfs->fileofs );
    out = s_worldData.surfaces;
    for( i = 0 ; i < count ; i++, in++, out++ )
    {
        switch( LittleLong( in->surfaceType ) )
        {
            case MST_PATCH:
                ParseMesh( in, dv, hdrVertColors, out );
                numMeshes++;
                break;
            case MST_TRIANGLE_SOUP:
                ParseTriSurf( in, dv, hdrVertColors, out, indexes );
                numTriSurfs++;
                break;
            case MST_PLANAR:
                ParseFace( in, dv, hdrVertColors, out, indexes );
                numFaces++;
                break;
            case MST_FLARE:
                ParseFlare( in, dv, out, indexes );
                numFlares++;
                break;
            default:
                Com_Error( ERR_DROP, "Bad surfaceType" );
        }
    }
    
    if( hdrVertColors )
    {
        fileSystem->FreeFile( hdrVertColors );
    }
    
#ifdef PATCH_STITCHING
    R_StitchAllPatches();
#endif
    
    R_FixSharedVertexLodError();
    
#ifdef PATCH_STITCHING
    R_MovePatchSurfacesToHunk();
#endif
    
    CL_RefPrintf( PRINT_ALL, "...loaded %d faces, %i meshes, %i trisurfs, %i flares\n",
                  numFaces, numMeshes, numTriSurfs, numFlares );
}



/*
=================
R_LoadSubmodels
=================
*/
static	void R_LoadSubmodels( lump_t* l )
{
    dmodel_t*	in;
    bmodel_t*	out;
    S32			i, j, count;
    
    in = ( dmodel_t* )( fileBase + l->fileofs );
    if( l->filelen % sizeof( *in ) )
        Com_Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
    count = l->filelen / sizeof( *in );
    
    s_worldData.numBModels = count;
    s_worldData.bmodels = out = reinterpret_cast<bmodel_t*>( Hunk_Alloc( count * sizeof( *out ), h_low ) );
    
    for( i = 0 ; i < count ; i++, in++, out++ )
    {
        model_t* model;
        
        model = R_AllocModel();
        
        assert( model != NULL );			// this should never happen
        if( model == NULL )
        {
            Com_Error( ERR_DROP, "R_LoadSubmodels: R_AllocModel() failed" );
        }
        
        model->type = MOD_BRUSH;
        model->bmodel = out;
        Com_sprintf( model->name, sizeof( model->name ), "*%d", i );
        
        for( j = 0 ; j < 3 ; j++ )
        {
            out->bounds[0][j] = LittleFloat( in->mins[j] );
            out->bounds[1][j] = LittleFloat( in->maxs[j] );
        }
        
        out->firstSurface = LittleLong( in->firstSurface );
        out->numSurfaces = LittleLong( in->numSurfaces );
        
        if( i == 0 )
        {
            // Add this for limiting VAO surface creation
            s_worldData.numWorldSurfaces = out->numSurfaces;
        }
    }
}



//==================================================================

/*
=================
R_SetParent
=================
*/
static	void R_SetParent( mnode_t* node, mnode_t* parent )
{
    node->parent = parent;
    if( node->contents != -1 )
        return;
    R_SetParent( node->children[0], node );
    R_SetParent( node->children[1], node );
}

/*
=================
R_LoadNodesAndLeafs
=================
*/
static	void R_LoadNodesAndLeafs( lump_t* nodeLump, lump_t* leafLump )
{
    S32			i, j, p;
    dnode_t*		in;
    dleaf_t*		inLeaf;
    mnode_t*	 out = nullptr;
    S32			numNodes, numLeafs;
    
    in = ( dnode_t* )( fileBase + nodeLump->fileofs );
    if( nodeLump->filelen % sizeof( dnode_t ) ||
            leafLump->filelen % sizeof( dleaf_t ) )
    {
        Com_Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
    }
    numNodes = nodeLump->filelen / sizeof( dnode_t );
    numLeafs = leafLump->filelen / sizeof( dleaf_t );
    
    out = reinterpret_cast<mnode_t*>( Hunk_Alloc( ( numNodes + numLeafs ) * sizeof( *out ), h_low ) );
    
    s_worldData.nodes = out;
    s_worldData.numnodes = numNodes + numLeafs;
    s_worldData.numDecisionNodes = numNodes;
    
    // load nodes
    for( i = 0 ; i < numNodes; i++, in++, out++ )
    {
        for( j = 0 ; j < 3 ; j++ )
        {
            out->mins[j] = LittleLong( in->mins[j] );
            out->maxs[j] = LittleLong( in->maxs[j] );
        }
        
        p = LittleLong( in->planeNum );
        out->plane = s_worldData.planes + p;
        
        out->contents = CONTENTS_NODE;	// differentiate from leafs
        
        for( j = 0 ; j < 2 ; j++ )
        {
            p = LittleLong( in->children[j] );
            if( p >= 0 )
                out->children[j] = s_worldData.nodes + p;
            else
                out->children[j] = s_worldData.nodes + numNodes + ( -1 - p );
        }
    }
    
    // load leafs
    inLeaf = ( dleaf_t* )( fileBase + leafLump->fileofs );
    for( i = 0 ; i < numLeafs ; i++, inLeaf++, out++ )
    {
        for( j = 0 ; j < 3 ; j++ )
        {
            out->mins[j] = LittleLong( inLeaf->mins[j] );
            out->maxs[j] = LittleLong( inLeaf->maxs[j] );
        }
        
        out->cluster = LittleLong( inLeaf->cluster );
        out->area = LittleLong( inLeaf->area );
        
        if( out->cluster >= s_worldData.numClusters )
        {
            s_worldData.numClusters = out->cluster + 1;
        }
        
        out->firstmarksurface = LittleLong( inLeaf->firstLeafSurface );
        out->nummarksurfaces = LittleLong( inLeaf->numLeafSurfaces );
    }
    
    // chain decendants
    R_SetParent( s_worldData.nodes, NULL );
}

//=============================================================================

/*
=================
R_LoadShaders
=================
*/
static	void R_LoadShaders( lump_t* l )
{
    S32		i, count;
    dshader_t*	in, *out = nullptr;
    
    in = ( dshader_t* )( fileBase + l->fileofs );
    if( l->filelen % sizeof( *in ) )
        Com_Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
    count = l->filelen / sizeof( *in );
    out = reinterpret_cast<dshader_t*>( Hunk_Alloc( count * sizeof( *out ), h_low ) );
    
    s_worldData.shaders = out;
    s_worldData.numShaders = count;
    
    ::memcpy( out, in, count * sizeof( *out ) );
    
    for( i = 0 ; i < count ; i++ )
    {
        out[i].surfaceFlags = LittleLong( out[i].surfaceFlags );
        out[i].contentFlags = LittleLong( out[i].contentFlags );
    }
}


/*
=================
R_LoadMarksurfaces
=================
*/
static	void R_LoadMarksurfaces( lump_t* l )
{
    S32		i, j, count;
    S32*		in;
    S32*     out = nullptr;
    
    in = ( S32* )( fileBase + l->fileofs );
    if( l->filelen % sizeof( *in ) )
        Com_Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
    count = l->filelen / sizeof( *in );
    out = reinterpret_cast<S32*>( Hunk_Alloc( count * sizeof( *out ), h_low ) );
    
    s_worldData.marksurfaces = out;
    s_worldData.nummarksurfaces = count;
    
    for( i = 0 ; i < count ; i++ )
    {
        j = LittleLong( in[i] );
        out[i] = j;
    }
}


/*
=================
R_LoadPlanes
=================
*/
static	void R_LoadPlanes( lump_t* l )
{
    S32			i, j;
    cplane_t*	out = nullptr;
    dplane_t*	 in;
    S32			count;
    S32			bits;
    
    in = ( dplane_t* )( fileBase + l->fileofs );
    if( l->filelen % sizeof( *in ) )
        Com_Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
    count = l->filelen / sizeof( *in );
    out = reinterpret_cast<cplane_t*>( Hunk_Alloc( count * 2 * sizeof( *out ), h_low ) );
    
    s_worldData.planes = out;
    s_worldData.numplanes = count;
    
    for( i = 0 ; i < count ; i++, in++, out++ )
    {
        bits = 0;
        for( j = 0 ; j < 3 ; j++ )
        {
            out->normal[j] = LittleFloat( in->normal[j] );
            if( out->normal[j] < 0 )
            {
                bits |= 1 << j;
            }
        }
        
        out->dist = LittleFloat( in->dist );
        out->type = PlaneTypeForNormal( out->normal );
        out->signbits = bits;
    }
}

/*
=================
R_LoadFogs

=================
*/
static	void R_LoadFogs( lump_t* l, lump_t* brushesLump, lump_t* sidesLump )
{
    S32			i;
    fog_t*		out;
    dfog_t*		fogs;
    dbrush_t*	 brushes, *brush;
    dbrushside_t*	sides;
    S32			count, brushesCount, sidesCount;
    S32			sideNum;
    S32			planeNum;
    shader_t*	shader;
    F32		d;
    S32			firstSide;
    
    fogs = ( dfog_t* )( fileBase + l->fileofs );
    if( l->filelen % sizeof( *fogs ) )
    {
        Com_Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
    }
    count = l->filelen / sizeof( *fogs );
    
    // create fog strucutres for them
    s_worldData.numfogs = count + 1;
    s_worldData.fogs = reinterpret_cast<fog_t*>( Hunk_Alloc( s_worldData.numfogs * sizeof( *out ), h_low ) );
    out = s_worldData.fogs + 1;
    
    if( !count )
    {
        return;
    }
    
    brushes = ( dbrush_t* )( fileBase + brushesLump->fileofs );
    if( brushesLump->filelen % sizeof( *brushes ) )
    {
        Com_Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
    }
    brushesCount = brushesLump->filelen / sizeof( *brushes );
    
    sides = ( dbrushside_t* )( fileBase + sidesLump->fileofs );
    if( sidesLump->filelen % sizeof( *sides ) )
    {
        Com_Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
    }
    sidesCount = sidesLump->filelen / sizeof( *sides );
    
    for( i = 0 ; i < count ; i++, fogs++ )
    {
        out->originalBrushNumber = LittleLong( fogs->brushNum );
        
        if( ( U32 )out->originalBrushNumber >= brushesCount )
        {
            Com_Error( ERR_DROP, "fog brushNumber out of range" );
        }
        brush = brushes + out->originalBrushNumber;
        
        firstSide = LittleLong( brush->firstSide );
        
        if( ( U32 )firstSide > sidesCount - 6 )
        {
            Com_Error( ERR_DROP, "fog brush sideNumber out of range" );
        }
        
        // brushes are always sorted with the axial sides first
        sideNum = firstSide + 0;
        planeNum = LittleLong( sides[ sideNum ].planeNum );
        out->bounds[0][0] = -s_worldData.planes[ planeNum ].dist;
        
        sideNum = firstSide + 1;
        planeNum = LittleLong( sides[ sideNum ].planeNum );
        out->bounds[1][0] = s_worldData.planes[ planeNum ].dist;
        
        sideNum = firstSide + 2;
        planeNum = LittleLong( sides[ sideNum ].planeNum );
        out->bounds[0][1] = -s_worldData.planes[ planeNum ].dist;
        
        sideNum = firstSide + 3;
        planeNum = LittleLong( sides[ sideNum ].planeNum );
        out->bounds[1][1] = s_worldData.planes[ planeNum ].dist;
        
        sideNum = firstSide + 4;
        planeNum = LittleLong( sides[ sideNum ].planeNum );
        out->bounds[0][2] = -s_worldData.planes[ planeNum ].dist;
        
        sideNum = firstSide + 5;
        planeNum = LittleLong( sides[ sideNum ].planeNum );
        out->bounds[1][2] = s_worldData.planes[ planeNum ].dist;
        
        // get information from the shader for fog parameters
        shader = R_FindShader( fogs->shader, LIGHTMAP_NONE, true );
        
        out->parms = shader->fogParms;
        
        out->colorInt = ColorBytes4( shader->fogParms.color[0],
                                     shader->fogParms.color[1],
                                     shader->fogParms.color[2], 1.0 );
                                     
        d = shader->fogParms.depthForOpaque < 1 ? 1 : shader->fogParms.depthForOpaque;
        out->tcScale = 1.0f / ( d * 8 );
        
        // set the gradient vector
        sideNum = LittleLong( fogs->visibleSide );
        
        if( sideNum == -1 )
        {
            out->hasSurface = false;
        }
        else
        {
            out->hasSurface = true;
            planeNum = LittleLong( sides[ firstSide + sideNum ].planeNum );
            VectorSubtract( vec3_origin, s_worldData.planes[ planeNum ].normal, out->surface );
            out->surface[3] = -s_worldData.planes[ planeNum ].dist;
        }
        
        out++;
    }
    
}


/*
================
R_LoadLightGrid
================
*/
void R_LoadLightGrid( lump_t* l )
{
    S32		i;
    vec3_t	maxs;
    S32		numGridPoints;
    world_t*	w;
    F32*	wMins, *wMaxs;
    
    w = &s_worldData;
    
    w->lightGridInverseSize[0] = 1.0f / w->lightGridSize[0];
    w->lightGridInverseSize[1] = 1.0f / w->lightGridSize[1];
    w->lightGridInverseSize[2] = 1.0f / w->lightGridSize[2];
    
    wMins = w->bmodels[0].bounds[0];
    wMaxs = w->bmodels[0].bounds[1];
    
    for( i = 0 ; i < 3 ; i++ )
    {
        w->lightGridOrigin[i] = w->lightGridSize[i] * ceil( wMins[i] / w->lightGridSize[i] );
        maxs[i] = w->lightGridSize[i] * floor( wMaxs[i] / w->lightGridSize[i] );
        w->lightGridBounds[i] = ( maxs[i] - w->lightGridOrigin[i] ) / w->lightGridSize[i] + 1;
    }
    
    numGridPoints = w->lightGridBounds[0] * w->lightGridBounds[1] * w->lightGridBounds[2];
    
    if( l->filelen != numGridPoints * 8 )
    {
        CL_RefPrintf( PRINT_WARNING, "WARNING: light grid mismatch\n" );
        w->lightGridData = NULL;
        return;
    }
    
    w->lightGridData = reinterpret_cast<U8*>( Hunk_Alloc( l->filelen, h_low ) );
    ::memcpy( w->lightGridData, ( void* )( fileBase + l->fileofs ), l->filelen );
    
    // deal with overbright bits
    for( i = 0 ; i < numGridPoints ; i++ )
    {
        R_ColorShiftLightingBytes( &w->lightGridData[i * 8], &w->lightGridData[i * 8] );
        R_ColorShiftLightingBytes( &w->lightGridData[i * 8 + 3], &w->lightGridData[i * 8 + 3] );
    }
    
    // load hdr lightgrid
    if( r_hdr->integer )
    {
        UTF8 filename[MAX_QPATH];
        F32* hdrLightGrid;
        S32 size;
        
        Com_sprintf( filename, sizeof( filename ), "maps/%s/lightgrid.raw", s_worldData.baseName );
        //CL_RefPrintf(PRINT_ALL, "looking for %s\n", filename);
        
        size = fileSystem->ReadFile( filename, ( void** )&hdrLightGrid );
        
        if( hdrLightGrid )
        {
            //CL_RefPrintf(PRINT_ALL, "found!\n");
            
            if( size != sizeof( F32 ) * 6 * numGridPoints )
                Com_Error( ERR_DROP, "Bad size for %s (%i, expected %i)!", filename, size, ( S32 )( sizeof( F32 ) ) * 6 * numGridPoints );
                
            w->lightGrid16 = reinterpret_cast<U16*>( Hunk_Alloc( sizeof( w->lightGrid16 ) * 6 * numGridPoints, h_low ) );
            
            for( i = 0; i < numGridPoints ; i++ )
            {
                vec4_t c;
                
                c[0] = hdrLightGrid[i * 6];
                c[1] = hdrLightGrid[i * 6 + 1];
                c[2] = hdrLightGrid[i * 6 + 2];
                c[3] = 1.0f;
                
                R_ColorShiftLightingFloats( c, c );
                ColorToRGB16( c, &w->lightGrid16[i * 6] );
                
                c[0] = hdrLightGrid[i * 6 + 3];
                c[1] = hdrLightGrid[i * 6 + 4];
                c[2] = hdrLightGrid[i * 6 + 5];
                c[3] = 1.0f;
                
                R_ColorShiftLightingFloats( c, c );
                ColorToRGB16( c, &w->lightGrid16[i * 6 + 3] );
            }
        }
        else if( 0 )
        {
            // promote 8-bit lightgrid to 16-bit
            w->lightGrid16 = reinterpret_cast<U16*>( Hunk_Alloc( sizeof( w->lightGrid16 ) * 6 * numGridPoints, h_low ) );
            
            for( i = 0; i < numGridPoints; i++ )
            {
                w->lightGrid16[i * 6]     = w->lightGridData[i * 8] * 257;
                w->lightGrid16[i * 6 + 1] = w->lightGridData[i * 8 + 1] * 257;
                w->lightGrid16[i * 6 + 2] = w->lightGridData[i * 8 + 2] * 257;
                w->lightGrid16[i * 6 + 3] = w->lightGridData[i * 8 + 3] * 257;
                w->lightGrid16[i * 6 + 4] = w->lightGridData[i * 8 + 4] * 257;
                w->lightGrid16[i * 6 + 5] = w->lightGridData[i * 8 + 5] * 257;
            }
        }
        
        if( hdrLightGrid )
            fileSystem->FreeFile( hdrLightGrid );
    }
}

/*
================
R_LoadEntities
================
*/
void R_LoadEntities( lump_t* l )
{
    UTF8* p, *token;
    UTF8* s;
    UTF8 keyname[MAX_TOKEN_CHARS];
    UTF8 value[MAX_TOKEN_CHARS];
    world_t*	w;
    
    w = &s_worldData;
    w->lightGridSize[0] = 64;
    w->lightGridSize[1] = 64;
    w->lightGridSize[2] = 128;
    
    p = reinterpret_cast< UTF8* >( ( fileBase + l->fileofs ) );
    
    // store for reference by the cgame
    w->entityString = reinterpret_cast<UTF8*>( Hunk_Alloc( l->filelen + 1, h_low ) );
    strcpy( w->entityString, p );
    w->entityParsePoint = w->entityString;
    
    token = COM_ParseExt( &p, true );
    if( !*token || *token != '{' )
    {
        return;
    }
    
    // only parse the world spawn
    while( 1 )
    {
        // parse key
        token = COM_ParseExt( &p, true );
        
        if( !*token || *token == '}' )
        {
            break;
        }
        Q_strncpyz( keyname, token, sizeof( keyname ) );
        
        // parse value
        token = COM_ParseExt( &p, true );
        
        if( !*token || *token == '}' )
        {
            break;
        }
        Q_strncpyz( value, token, sizeof( value ) );
        
        // check for remapping of shaders for vertex lighting
        if( !Q_strncmp( keyname, "vertexremapshader", strlen( "vertexremapshader" ) ) )
        {
            s = strchr( value, ';' );
            if( !s )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: no semi colon in vertexshaderremap '%s'\n", value );
                break;
            }
            *s++ = 0;
            if( r_vertexLight->integer )
            {
                renderSystemLocal.RemapShader( value, s, "0" );
            }
            continue;
        }
        // check for remapping of shaders
        if( !Q_strncmp( keyname, "remapshader", strlen( "remapshader" ) ) )
        {
            s = strchr( value, ';' );
            if( !s )
            {
                CL_RefPrintf( PRINT_WARNING, "WARNING: no semi colon in shaderremap '%s'\n", value );
                break;
            }
            *s++ = 0;
            renderSystemLocal.RemapShader( value, s, "0" );
            continue;
        }
        // check for a different grid size
        if( !Q_stricmp( keyname, "gridsize" ) )
        {
            sscanf( value, "%f %f %f", &w->lightGridSize[0], &w->lightGridSize[1], &w->lightGridSize[2] );
            continue;
        }
        
        // check for auto exposure
        if( !Q_stricmp( keyname, "autoExposureMinMax" ) )
        {
            sscanf( value, "%f %f", &tr.autoExposureMinMax[0], &tr.autoExposureMinMax[1] );
            continue;
        }
    }
}

/*
=================
idRenderSystemLocal::GetEntityToken
=================
*/
bool idRenderSystemLocal::GetEntityToken( UTF8* buffer, S32 size )
{
    StringEntry	s;
    
    s = COM_Parse( &s_worldData.entityParsePoint );
    Q_strncpyz( buffer, s, size );
    if( !s_worldData.entityParsePoint || !s[0] )
    {
        s_worldData.entityParsePoint = s_worldData.entityString;
        return false;
    }
    else
    {
        return true;
    }
}

#ifndef MAX_SPAWN_VARS
#define MAX_SPAWN_VARS 64
#endif

// derived from G_ParseSpawnVars() in g_spawn.c
bool R_ParseSpawnVars( UTF8* spawnVarChars, S32 maxSpawnVarChars, S32* numSpawnVars, UTF8* spawnVars[MAX_SPAWN_VARS][2] )
{
    UTF8		keyname[MAX_TOKEN_CHARS];
    UTF8		com_token[MAX_TOKEN_CHARS];
    S32			numSpawnVarChars = 0;
    
    *numSpawnVars = 0;
    
    // parse the opening brace
    if( !renderSystemLocal.GetEntityToken( com_token, sizeof( com_token ) ) )
    {
        // end of spawn string
        return false;
    }
    if( com_token[0] != '{' )
    {
        CL_RefPrintf( PRINT_ALL, "R_ParseSpawnVars: found %s when expecting {\n", com_token );
        return false;
    }
    
    // go through all the key / value pairs
    while( 1 )
    {
        S32 keyLength, tokenLength;
        
        // parse key
        if( !renderSystemLocal.GetEntityToken( keyname, sizeof( keyname ) ) )
        {
            CL_RefPrintf( PRINT_ALL, "R_ParseSpawnVars: EOF without closing brace\n" );
            return false;
        }
        
        if( keyname[0] == '}' )
        {
            break;
        }
        
        // parse value
        if( !renderSystemLocal.GetEntityToken( com_token, sizeof( com_token ) ) )
        {
            CL_RefPrintf( PRINT_ALL, "R_ParseSpawnVars: EOF without closing brace\n" );
            return false;
        }
        
        if( com_token[0] == '}' )
        {
            CL_RefPrintf( PRINT_ALL, "R_ParseSpawnVars: closing brace without data\n" );
            return false;
        }
        
        if( *numSpawnVars == MAX_SPAWN_VARS )
        {
            CL_RefPrintf( PRINT_ALL, "R_ParseSpawnVars: MAX_SPAWN_VARS\n" );
            return false;
        }
        
        keyLength = strlen( keyname ) + 1;
        tokenLength = strlen( com_token ) + 1;
        
        if( numSpawnVarChars + keyLength + tokenLength > maxSpawnVarChars )
        {
            CL_RefPrintf( PRINT_ALL, "R_ParseSpawnVars: MAX_SPAWN_VAR_CHARS\n" );
            return false;
        }
        
        strcpy( spawnVarChars + numSpawnVarChars, keyname );
        spawnVars[ *numSpawnVars ][0] = spawnVarChars + numSpawnVarChars;
        numSpawnVarChars += keyLength;
        
        strcpy( spawnVarChars + numSpawnVarChars, com_token );
        spawnVars[ *numSpawnVars ][1] = spawnVarChars + numSpawnVarChars;
        numSpawnVarChars += tokenLength;
        
        ( *numSpawnVars )++;
    }
    
    return true;
}

void R_LoadCubemapEntities( StringEntry cubemapEntityName )
{
    UTF8 spawnVarChars[2048];
    S32 numSpawnVars;
    UTF8* spawnVars[MAX_SPAWN_VARS][2];
    S32 numCubemaps = 0;
    
    // count cubemaps
    numCubemaps = 0;
    while( R_ParseSpawnVars( spawnVarChars, sizeof( spawnVarChars ), &numSpawnVars, spawnVars ) )
    {
        S32 i;
        
        for( i = 0; i < numSpawnVars; i++ )
        {
            if( !Q_stricmp( spawnVars[i][0], "classname" ) && !Q_stricmp( spawnVars[i][1], cubemapEntityName ) )
                numCubemaps++;
        }
    }
    
    if( !numCubemaps )
        return;
        
    tr.numCubemaps = numCubemaps;
    tr.cubemaps = reinterpret_cast<cubemap_t*>( Hunk_Alloc( tr.numCubemaps * sizeof( *tr.cubemaps ), h_low ) );
    memset( tr.cubemaps, 0, tr.numCubemaps * sizeof( *tr.cubemaps ) );
    
    numCubemaps = 0;
    while( R_ParseSpawnVars( spawnVarChars, sizeof( spawnVarChars ), &numSpawnVars, spawnVars ) )
    {
        S32 i;
        UTF8 name[MAX_QPATH];
        bool isCubemap = false;
        bool originSet = false;
        vec3_t origin;
        F32 parallaxRadius = 1000.0f;
        
        name[0] = '\0';
        for( i = 0; i < numSpawnVars; i++ )
        {
            if( !Q_stricmp( spawnVars[i][0], "classname" ) && !Q_stricmp( spawnVars[i][1], cubemapEntityName ) )
                isCubemap = true;
                
            if( !Q_stricmp( spawnVars[i][0], "name" ) )
                Q_strncpyz( name, spawnVars[i][1], MAX_QPATH );
                
            if( !Q_stricmp( spawnVars[i][0], "origin" ) )
            {
                sscanf( spawnVars[i][1], "%f %f %f", &origin[0], &origin[1], &origin[2] );
                originSet = true;
            }
            else if( !Q_stricmp( spawnVars[i][0], "radius" ) )
            {
                sscanf( spawnVars[i][1], "%f", &parallaxRadius );
            }
        }
        
        if( isCubemap && originSet )
        {
            cubemap_t* cubemap = &tr.cubemaps[numCubemaps];
            Q_strncpyz( cubemap->name, name, MAX_QPATH );
            VectorCopy( origin, cubemap->origin );
            cubemap->parallaxRadius = parallaxRadius;
            numCubemaps++;
        }
    }
}

bool R_MaterialUsesCubemap( S32 surfaceFlags )
{
    switch( surfaceFlags & MATERIAL_MASK )
    {
        case MATERIAL_WATER:			// 13			// light covering of water on a surface
            return true;
            break;
        case MATERIAL_SHORTGRASS:		// 5			// manicured lawn
            return false;
            break;
        case MATERIAL_LONGGRASS:		// 6			// long jungle grass
            return false;
            break;
        case MATERIAL_SAND:				// 8			// sandy beach
            return false;
            break;
        case MATERIAL_CARPET:			// 27			// lush carpet
            return false;
            break;
        case MATERIAL_GRAVEL:			// 9			// lots of small stones
            return false;
            break;
        case MATERIAL_ROCK:				// 23			//
            return false;
            break;
        case MATERIAL_TILES:			// 26			// tiled floor
            return true;
            break;
        case MATERIAL_SOLIDWOOD:		// 1			// freshly cut timber
            return false;
            break;
        case MATERIAL_HOLLOWWOOD:		// 2			// termite infested creaky wood
            return false;
            break;
        case MATERIAL_SOLIDMETAL:		// 3			// solid girders
            return true;
            break;
        case MATERIAL_HOLLOWMETAL:		// 4			// hollow metal machines -- Used for weapons to force lower parallax and high reflection...
            return true;
            break;
        case MATERIAL_DRYLEAVES:		// 19			// dried up leaves on the floor
            return false;
            break;
        case MATERIAL_GREENLEAVES:		// 20			// fresh leaves still on a tree
            return false;
            break;
        case MATERIAL_FABRIC:			// 21			// Cotton sheets
            return false;
            break;
        case MATERIAL_CANVAS:			// 22			// tent material
            return false;
            break;
        case MATERIAL_MARBLE:			// 12			// marble floors
            return true;
            break;
        case MATERIAL_SNOW:				// 14			// freshly laid snow
            return false;
            break;
        case MATERIAL_MUD:				// 17			// wet soil
            return false;
            break;
        case MATERIAL_DIRT:				// 7			// hard mud
            return false;
            break;
        case MATERIAL_CONCRETE:			// 11			// hardened concrete pavement
            return false;
            break;
        case MATERIAL_FLESH:			// 16			// hung meat, corpses in the world
            return false;
            break;
        case MATERIAL_RUBBER:			// 24			// hard tire like rubber
            return false;
            break;
        case MATERIAL_PLASTIC:			// 25			//
            return true;
            break;
        case MATERIAL_PLASTER:			// 28			// drywall style plaster
            return false;
            break;
        case MATERIAL_SHATTERGLASS:		// 29			// glass with the Crisis Zone style shattering
            return true;
            break;
        case MATERIAL_ARMOR:			// 30			// body armor
            return true;
            break;
        case MATERIAL_ICE:				// 15			// packed snow/solid ice
            return true;
            break;
        case MATERIAL_GLASS:			// 10			//
            return true;
            break;
        case MATERIAL_BPGLASS:			// 18			// bulletproof glass
            return true;
            break;
        case MATERIAL_COMPUTER:			// 31			// computers/electronic equipment
            return true;
            break;
        default:
            return false;
            break;
    }
    
    return false;
}

void R_AssignCubemapsToWorldSurfaces( void )
{
    world_t*	w;
    S32 i;
    
    w = &s_worldData;
    
    #pragma omp parallel for
    for( i = 0; i < w->numsurfaces; i++ )
    {
        msurface_t* surf = &w->surfaces[i];
        vec3_t surfOrigin;
        
        if( !R_MaterialUsesCubemap( surf->shader->surfaceFlags ) )
        {
            surf->cubemapIndex = 0;
        }
        
        if( surf->cullinfo.type & CULLINFO_SPHERE )
        {
            VectorCopy( surf->cullinfo.localOrigin, surfOrigin );
        }
        else if( surf->cullinfo.type & CULLINFO_BOX )
        {
            surfOrigin[0] = ( surf->cullinfo.bounds[0][0] + surf->cullinfo.bounds[1][0] ) * 0.5f;
            surfOrigin[1] = ( surf->cullinfo.bounds[0][1] + surf->cullinfo.bounds[1][1] ) * 0.5f;
            surfOrigin[2] = ( surf->cullinfo.bounds[0][2] + surf->cullinfo.bounds[1][2] ) * 0.5f;
        }
        else
        {
            //CL_RefPrintf(PRINT_ALL, "surface %d has no cubemap\n", i);
            continue;
        }
        
        surf->cubemapIndex = R_CubemapForPoint( surfOrigin );
        //CL_RefPrintf(PRINT_ALL, "surface %d has cubemap %d\n", i, surf->cubemapIndex);
    }
}


void R_LoadCubemaps( void )
{
    S32 i;
    S32/*imgFlags_t*/ flags = IMGFLAG_CLAMPTOEDGE | IMGFLAG_MIPMAP | IMGFLAG_NOLIGHTSCALE | IMGFLAG_CUBEMAP;
    
    #pragma omp parallel for
    for( i = 0; i < tr.numCubemaps; i++ )
    {
        UTF8 filename[MAX_QPATH];
        cubemap_t* cubemap = &tr.cubemaps[i];
        
        Com_sprintf( filename, MAX_QPATH, "cubemaps/%s/%03d.dds", tr.world->baseName, i );
        
        cubemap->image = R_FindImageFile( filename, IMGTYPE_COLORALPHA, flags );
        cubemap->mipmapped = 0;
    }
}


void R_RenderMissingCubemaps( void )
{
    S32 i, j;
    S32/*imgFlags_t*/ flags = IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE | IMGFLAG_MIPMAP | IMGFLAG_NOLIGHTSCALE | IMGFLAG_CUBEMAP;
    U32 cubemapFormat = GL_RGBA8;
    
    if( r_hdr->integer )
    {
        cubemapFormat = GL_RGBA16;
    }
    
    #pragma omp parallel for
    for( i = 0; i < tr.numCubemaps; i++ )
    {
        if( !tr.cubemaps[i].image )
        {
            tr.cubemaps[i].image = R_CreateImage( va( "*cubeMap%d", i ), NULL, r_cubemapSize->integer, r_cubemapSize->integer, IMGTYPE_COLORALPHA, flags, cubemapFormat );
            tr.cubemaps[i].mipmapped = 0;
            
            for( j = 0; j < 6; j++ )
            {
                renderSystemLocal.ClearScene();
                R_RenderCubemapSide( i, j, false );
                R_IssuePendingRenderCommands();
                R_InitNextFrame();
            }
        }
    }
}


void R_CalcVertexLightDirs( void )
{
    S32 i, k;
    msurface_t* surface;
    
    for( k = 0, surface = &s_worldData.surfaces[0]; k < s_worldData.numsurfaces /* s_worldData.numWorldSurfaces */; k++, surface++ )
    {
        srfBspSurface_t* bspSurf = ( srfBspSurface_t* ) surface->data;
        
        switch( bspSurf->surfaceType )
        {
            case SF_FACE:
            case SF_GRID:
            case SF_TRIANGLES:
                for( i = 0; i < bspSurf->numVerts; i++ )
                {
                    vec3_t lightDir;
                    vec3_t normal;
                    
                    R_VaoUnpackNormal( normal, bspSurf->verts[i].normal );
                    R_LightDirForPoint( bspSurf->verts[i].xyz, lightDir, normal, &s_worldData );
                    R_VaoPackNormal( bspSurf->verts[i].lightdir, lightDir );
                }
                
                break;
                
            default:
                break;
        }
    }
}


/*
=================
idRenderSystemLocal::LoadWorld

Called directly from cgame
=================
*/
void idRenderSystemLocal::LoadWorld( StringEntry name )
{
    S32			i;
    dheader_t*	header;
    union
    {
        U8* b;
        void* v;
    } buffer;
    S32	startMarker = 0;
    
    if( tr.worldMapLoaded )
    {
        Com_Error( ERR_DROP, "ERROR: attempted to redundantly load world map" );
    }
    
    // set default map light scale
    tr.sunShadowScale = 0.5f;
    
    // set default sun direction to be used if it isn't
    // overridden by a shader
    tr.sunDirection[0] = 0.45f;
    tr.sunDirection[1] = 0.3f;
    tr.sunDirection[2] = 0.9f;
    
    tr.sunShader = nullptr;
    
    VectorNormalize( tr.sunDirection );
    
    // set default autoexposure settings
    tr.autoExposureMinMax[0] = -2.0f;
    tr.autoExposureMinMax[1] = 2.0f;
    
    // set default tone mapping settings
    tr.toneMinAvgMaxLevel[0] = -8.0f;
    tr.toneMinAvgMaxLevel[1] = -2.0f;
    tr.toneMinAvgMaxLevel[2] = 0.0f;
    
    // reset last cascade sun direction so last shadow cascade is rerendered
    VectorClear( tr.lastCascadeSunDirection );
    
    tr.worldMapLoaded = true;
    
    // load it
    fileSystem->ReadFile( name, &buffer.v );
    if( !buffer.b )
    {
        Com_Error( ERR_DROP, "idRenderSystemLocal::LoadWorldMap: %s not found", name );
    }
    
    // clear tr.world so if the level fails to load, the next
    // try will not look at the partially loaded version
    tr.world = NULL;
    
    ::memset( &s_worldData, 0, sizeof( s_worldData ) );
    Q_strncpyz( s_worldData.name, name, sizeof( s_worldData.name ) );
    
    Q_strncpyz( s_worldData.baseName, COM_SkipPath( s_worldData.name ), sizeof( s_worldData.name ) );
    COM_StripExtension2( s_worldData.baseName, s_worldData.baseName, sizeof( s_worldData.baseName ) );
    
    startMarker = ( S32 )Hunk_Alloc( 0, h_low );
    c_gridVerts = 0;
    
    header = ( dheader_t* )buffer.b;
    fileBase = ( U8* )header;
    
    i = LittleLong( header->version );
#if 0
    if( i != BSP_VERSION )
    {
        Com_Error( ERR_DROP, "idRenderSystemLocal::LoadWorldMap: %s has wrong version number (%i should be %i)",
                   name, i, BSP_VERSION );
    }
#endif
    
    // swap all the lumps
    for( i = 0 ; i < sizeof( dheader_t ) / 4 ; i++ )
    {
        ( ( S32* )header )[i] = LittleLong( ( ( S32* )header )[i] );
    }
    
    // load into heap
    R_LoadEntities( &header->lumps[LUMP_ENTITIES] );
    R_LoadShaders( &header->lumps[LUMP_SHADERS] );
    R_LoadLightmaps( &header->lumps[LUMP_LIGHTMAPS], &header->lumps[LUMP_SURFACES] );
    R_LoadPlanes( &header->lumps[LUMP_PLANES] );
    R_LoadFogs( &header->lumps[LUMP_FOGS], &header->lumps[LUMP_BRUSHES], &header->lumps[LUMP_BRUSHSIDES] );
    R_LoadSurfaces( &header->lumps[LUMP_SURFACES], &header->lumps[LUMP_DRAWVERTS], &header->lumps[LUMP_DRAWINDEXES] );
    R_LoadMarksurfaces( &header->lumps[LUMP_LEAFSURFACES] );
    R_LoadNodesAndLeafs( &header->lumps[LUMP_NODES], &header->lumps[LUMP_LEAFS] );
    R_LoadSubmodels( &header->lumps[LUMP_MODELS] );
    R_LoadVisibility( &header->lumps[LUMP_VISIBILITY] );
    R_LoadLightGrid( &header->lumps[LUMP_LIGHTGRID] );
    
    // determine vertex light directions
    R_CalcVertexLightDirs();
    
    // determine which parts of the map are in sunlight
    if( 0 )
    {
        world_t*	w;
        U8* primaryLightGrid, *data;
        S32 lightGridSize;
        S32 i;
        
        w = &s_worldData;
        
        lightGridSize = w->lightGridBounds[0] * w->lightGridBounds[1] * w->lightGridBounds[2];
        primaryLightGrid = ( U8* )CL_RefMalloc( lightGridSize * sizeof( *primaryLightGrid ) );
        
        memset( primaryLightGrid, 0, lightGridSize * sizeof( *primaryLightGrid ) );
        
        data = w->lightGridData;
        for( i = 0; i < lightGridSize; i++, data += 8 )
        {
            S32 lat, lng;
            vec3_t gridLightDir, gridLightCol;
            
            // skip samples in wall
            if( !( data[0] + data[1] + data[2] + data[3] + data[4] + data[5] ) )
                continue;
                
            gridLightCol[0] = ByteToFloat( data[3] );
            gridLightCol[1] = ByteToFloat( data[4] );
            gridLightCol[2] = ByteToFloat( data[5] );
            ( void )gridLightCol; // Suppress unused-but-set-variable warning
            
            lat = data[7];
            lng = data[6];
            lat *= ( FUNCTABLE_SIZE / 256 );
            lng *= ( FUNCTABLE_SIZE / 256 );
            
            // decode X as cos( lat ) * sin( long )
            // decode Y as sin( lat ) * sin( long )
            // decode Z as cos( long )
            
            gridLightDir[0] = tr.sinTable[( lat + ( FUNCTABLE_SIZE / 4 ) )&FUNCTABLE_MASK] * tr.sinTable[lng];
            gridLightDir[1] = tr.sinTable[lat] * tr.sinTable[lng];
            gridLightDir[2] = tr.sinTable[( lng + ( FUNCTABLE_SIZE / 4 ) )&FUNCTABLE_MASK];
            
            // FIXME: magic number for determining if light direction is close enough to sunlight
            if( DotProduct( gridLightDir, tr.sunDirection ) > 0.75f )
            {
                primaryLightGrid[i] = 1;
            }
            else
            {
                primaryLightGrid[i] = 255;
            }
        }
        
        if( 0 )
        {
            S32 i;
            U8* buffer = ( U8* )CL_RefMalloc( w->lightGridBounds[0] * w->lightGridBounds[1] * 3 + 18 );
            U8* out;
            U8* in;
            UTF8 fileName[MAX_QPATH];
            
            ::memset( buffer, 0, 18 );
            buffer[2] = 2;		// uncompressed type
            buffer[12] = w->lightGridBounds[0] & 255;
            buffer[13] = w->lightGridBounds[0] >> 8;
            buffer[14] = w->lightGridBounds[1] & 255;
            buffer[15] = w->lightGridBounds[1] >> 8;
            buffer[16] = 24;	// pixel size
            
            in = primaryLightGrid;
            for( i = 0; i < w->lightGridBounds[2]; i++ )
            {
                S32 j;
                
                Com_sprintf( fileName, sizeof( fileName ), "primarylg%d.tga", i );
                
                out = buffer + 18;
                for( j = 0; j < w->lightGridBounds[0] * w->lightGridBounds[1]; j++ )
                {
                    if( *in == 1 )
                    {
                        *out++ = 255;
                        *out++ = 255;
                        *out++ = 255;
                    }
                    else if( *in == 255 )
                    {
                        *out++ = 64;
                        *out++ = 64;
                        *out++ = 64;
                    }
                    else
                    {
                        *out++ = 0;
                        *out++ = 0;
                        *out++ = 0;
                    }
                    in++;
                }
                
                fileSystem->WriteFile( fileName, buffer, w->lightGridBounds[0] * w->lightGridBounds[1] * 3 + 18 );
            }
            
            Z_Free( buffer );
        }
        
        for( i = 0; i < w->numWorldSurfaces; i++ )
        {
            msurface_t* surf = w->surfaces + i;
            cullinfo_t* ci = &surf->cullinfo;
            
            if( ci->type & CULLINFO_PLANE )
            {
                if( DotProduct( ci->plane.normal, tr.sunDirection ) <= 0.0f )
                {
                    //CL_RefPrintf(PRINT_ALL, "surface %d is not oriented towards sunlight\n", i);
                    continue;
                }
            }
            
            if( ci->type & CULLINFO_BOX )
            {
                S32 ibounds[2][3], x, y, z, goodSamples, numSamples;
                vec3_t lightOrigin;
                
                VectorSubtract( ci->bounds[0], w->lightGridOrigin, lightOrigin );
                
                ibounds[0][0] = floor( lightOrigin[0] * w->lightGridInverseSize[0] );
                ibounds[0][1] = floor( lightOrigin[1] * w->lightGridInverseSize[1] );
                ibounds[0][2] = floor( lightOrigin[2] * w->lightGridInverseSize[2] );
                
                VectorSubtract( ci->bounds[1], w->lightGridOrigin, lightOrigin );
                
                ibounds[1][0] = ceil( lightOrigin[0] * w->lightGridInverseSize[0] );
                ibounds[1][1] = ceil( lightOrigin[1] * w->lightGridInverseSize[1] );
                ibounds[1][2] = ceil( lightOrigin[2] * w->lightGridInverseSize[2] );
                
                ibounds[0][0] = CLAMP( ibounds[0][0], 0, w->lightGridSize[0] );
                ibounds[0][1] = CLAMP( ibounds[0][1], 0, w->lightGridSize[1] );
                ibounds[0][2] = CLAMP( ibounds[0][2], 0, w->lightGridSize[2] );
                
                ibounds[1][0] = CLAMP( ibounds[1][0], 0, w->lightGridSize[0] );
                ibounds[1][1] = CLAMP( ibounds[1][1], 0, w->lightGridSize[1] );
                ibounds[1][2] = CLAMP( ibounds[1][2], 0, w->lightGridSize[2] );
                
                /*
                CL_RefPrintf(PRINT_ALL, "surf %d bounds (%f %f %f)-(%f %f %f) ibounds (%d %d %d)-(%d %d %d)\n", i,
                	ci->bounds[0][0], ci->bounds[0][1], ci->bounds[0][2],
                	ci->bounds[1][0], ci->bounds[1][1], ci->bounds[1][2],
                	ibounds[0][0], ibounds[0][1], ibounds[0][2],
                	ibounds[1][0], ibounds[1][1], ibounds[1][2]);
                */
                
                goodSamples = 0;
                numSamples = 0;
                for( x = ibounds[0][0]; x <= ibounds[1][0]; x++ )
                {
                    for( y = ibounds[0][1]; y <= ibounds[1][1]; y++ )
                    {
                        for( z = ibounds[0][2]; z <= ibounds[1][2]; z++ )
                        {
                            U8 primaryLight = primaryLightGrid[x * 8 + y * 8 * w->lightGridBounds[0] + z * 8 * w->lightGridBounds[0] * w->lightGridBounds[2]];
                            
                            if( primaryLight == 0 )
                                continue;
                                
                            numSamples++;
                            
                            if( primaryLight == 1 )
                                goodSamples++;
                        }
                    }
                }
                
                // FIXME: magic number for determining whether object is mostly in sunlight
                if( goodSamples > numSamples * 0.75f )
                {
                    //CL_RefPrintf(PRINT_ALL, "surface %d is in sunlight\n", i);
                    //surf->primaryLight = 1;
                }
            }
        }
        
        Z_Free( primaryLightGrid );
    }
    
    // load cubemaps
    if( r_cubeMapping->integer )
    {
        R_LoadCubemapEntities( "misc_cubemap" );
        if( !tr.numCubemaps )
        {
            // use deathmatch spawn points as cubemaps
            R_LoadCubemapEntities( "target_location" );
        }
        
        if( tr.numCubemaps )
        {
            R_AssignCubemapsToWorldSurfaces();
        }
    }
    
    s_worldData.dataSize = ( S32 )Hunk_Alloc( 0, h_low ) - startMarker;
    
    CL_RefPrintf( PRINT_ALL, "total world data size: %d.%02d MB\n", s_worldData.dataSize / ( 1024 * 1024 ),
                  ( s_worldData.dataSize % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ) );
                  
    // only set tr.world now that we know the entire level has loaded properly
    tr.world = &s_worldData;
    
    // make sure the VAO glState entry is safe
    R_BindNullVao();
    
    // Render or load all cubemaps
    if( r_cubeMapping->integer && tr.numCubemaps && glRefConfig.framebufferObject )
    {
        R_LoadCubemaps();
        R_RenderMissingCubemaps();
    }
    
    
    fileSystem->FreeFile( buffer.v );
}
