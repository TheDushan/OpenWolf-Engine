/*
Copyright (c) 2010-2013  p5yc0runn3r at gmail.com

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

#ifndef NO_DMAHD

#include <framework/precompiled.hpp>

void dmaHD_Update_Mix( void );
void S_UpdateBackgroundTrack( void );
void S_GetSoundtime( void );
bool S_ScanChannelStarts( void );


// used in dmaEX mixer.
#define SOUND_FULLVOLUME 80
#define SOUND_ATTENUATE 0.0007f

extern channel_t s_channels[];
extern channel_t loop_channels[];
extern sint numLoopChannels;

extern sint s_soundStarted;
extern bool s_soundMuted;

extern sint listener_number;
vec3_t g_listener_origin;
vec3_t g_listener_axis[3];

extern sint s_soundtime;
extern sint s_paintedtime;
static sint dmaHD_inwater;

// MAX_SFX may be larger than MAX_SOUNDS because of custom player sounds
#define MAX_SFX 4096 // This must be the same as the snd_dma.c
#define MAX_SOUNDBYTES (256*1024*1024) // 256MiB MAXIMUM...
extern sfx_t s_knownSfx[];
extern sint s_numSfx;

extern convar_t* s_mixahead;
convar_t* dmaHD_Enable = nullptr;
convar_t* dmaHD_Interpolation;
convar_t* dmaHD_Mixer;
convar_t* dmaEX_StereoSeparation;

extern loopSound_t loopSounds[];

#ifdef MAX_RAW_STREAMS
extern sint s_rawend[MAX_RAW_STREAMS];
extern portable_samplepair_t s_rawsamples[MAX_RAW_STREAMS][MAX_RAW_SAMPLES];
#else
extern sint s_rawend;
extern portable_samplepair_t s_rawsamples[];
#endif

#define DMAHD_PAINTBUFFER_SIZE 65536
static portable_samplepair_t dmaHD_paintbuffer[DMAHD_PAINTBUFFER_SIZE];
static sint dmaHD_snd_vol;

bool g_tablesinit = false;
float32 g_voltable[256];

#define SMPCLAMP(a) (((a) < -32768) ? -32768 : ((a) > 32767) ? 32767 : (a))
#define VOLCLAMP(a) (((a) < 0) ? 0 : ((a) > 255) ? 255 : (a))

void dmaHD_InitTables( void )
{
    if( !g_tablesinit )
    {
        sint i;
        float32 x, y;
        
        // Volume table.
        for( i = 0; i < 256; i++ )
        {
            x = ( i * ( 9.0 / 256.0 ) ) + 1.0;
            y = 1.0 - log10f( x );
            g_voltable[i] = y;
        }
        
        g_tablesinit = true;
    }
}

/*
===============================================================================
PART#01: dmaHD: dma sound EXtension : MEMORY
===============================================================================
*/

sint g_dmaHD_allocatedsoundmemory = 0;

/*
======================
dmaHD_FreeOldestSound
======================
*/

void dmaHD_FreeOldestSound( void )
{
    sint	i, oldest, used;
    sfx_t* sfx;
    schar16* buffer;
    
    oldest = Com_Milliseconds();
    used = 0;
    
    for( i = 1 ; i < s_numSfx ; i++ )
    {
        sfx = &s_knownSfx[i];
        if( sfx->inMemory && sfx->lastTimeUsed < oldest )
        {
            used = i;
            oldest = sfx->lastTimeUsed;
        }
    }
    
    sfx = &s_knownSfx[used];
    
    Com_DPrintf( "dmaHD_FreeOldestSound: freeing sound %s\n", sfx->soundName );
    
    i = ( sfx->soundLength * 2 ) * sizeof( schar16 );
    
    g_dmaHD_allocatedsoundmemory -= i;
    
    if( g_dmaHD_allocatedsoundmemory < 0 )
    {
        g_dmaHD_allocatedsoundmemory = 0;
    }
    
    if( ( buffer = reinterpret_cast< schar16* >( sfx->soundData ) ) != nullptr )
    {
        ::free( buffer );
    }
    
    sfx->inMemory = false;
    sfx->soundData = nullptr;
}

/*
======================
dmaHD_AllocateSoundBuffer
======================
*/

schar16* dmaHD_AllocateSoundBuffer( sint samples )
{
    sint bytes = samples * sizeof( schar16 );
    schar16* buffer;
    
    while( g_dmaHD_allocatedsoundmemory > 0 && ( g_dmaHD_allocatedsoundmemory + bytes ) > MAX_SOUNDBYTES )
    {
        dmaHD_FreeOldestSound();
    }
    
    if( s_numSfx >= ( MAX_SFX - 8 ) )
    {
        dmaHD_FreeOldestSound();
    }
    
    do
    {
        if( ( buffer = static_cast<schar16*>( malloc( bytes ) ) ) != nullptr )
        {
            break;
        }
        dmaHD_FreeOldestSound();
    }
    while( g_dmaHD_allocatedsoundmemory > 0 );
    
    if( buffer == nullptr )
    {
        Com_Error( ERR_FATAL, "Out of Memory" );
    }
    
    g_dmaHD_allocatedsoundmemory += bytes;
    
    return buffer;
}

// =======================================================
// DMAHD - Interpolation functions / No need to optimize a lot here since the sounds are interpolated
// once on load and not on playback. This also means that at least twice more memory is used.
// =======================================================
// x0-----x1--t--x2-----x3 / x0/2/3/4 are know samples / t = 0.0 - 1.0 between x1 and x2 / returns y value at point t
static float32 dmaHD_InterpolateCubic( float32 x0, float32 x1, float32 x2, float32 x3, float32 t )
{
    float32 a0, a1, a2, a3;
    
    a0 = x3 - x2 - x0 + x1;
    a1 = x0 - x1 - a0;
    a2 = x2 - x0;
    a3 = x1;
    
    return ( a0 * ( t * t * t ) ) + ( a1 * ( t * t ) ) + ( a2 * t ) + ( a3 );
}

static float32 dmaHD_InterpolateHermite4pt3oX( float32 x0, float32 x1, float32 x2, float32 x3, float32 t )
{
    float32 c0, c1, c2, c3;
    
    c0 = x1;
    c1 = 0.5f * ( x2 - x0 );
    c2 = x0 - ( 2.5f * x1 ) + ( 2 * x2 ) - ( 0.5f * x3 );
    c3 = ( 0.5f * ( x3 - x0 ) ) + ( 1.5f * ( x1 - x2 ) );
    
    return ( ( ( ( ( c3 * t ) + c2 ) * t ) + c1 ) * t ) + c0;
}

static float32 dmaHD_NormalizeSamplePosition( float32 t, sint samples )
{
    if( !samples )
    {
        return t;
    }
    
    while( t < 0.0 )
    {
        t += static_cast<float32>( samples );
    }
    
    while( t >= static_cast<float32>( samples ) )
    {
        t -= static_cast<float32>( samples );
    }
    
    return t;
}

static sint dmaHD_GetSampleRaw_8bitMono( sint index, sint samples, uchar8* data )
{
    if( index < 0 )
    {
        index += samples;
    }
    else if( index >= samples )
    {
        index -= samples;
    }
    
    return static_cast<sint>( ( static_cast< uchar8 >( data[index] ) - 128 ) << 8 );
}

static sint dmaHD_GetSampleRaw_16bitMono( sint index, sint samples, uchar8* data )
{
    if( index < 0 )
    {
        index += samples;
    }
    else if( index >= samples )
    {
        index -= samples;
    }
    
    return static_cast<sint>( LittleShort( ( reinterpret_cast<schar16*>( data ) )[index] ) );
}
static sint dmaHD_GetSampleRaw_8bitStereo( sint index, sint samples, uchar8* data )
{
    sint left, right;
    
    if( index < 0 )
    {
        index += samples;
    }
    else if( index >= samples )
    {
        index -= samples;
    }
    
    left = static_cast<sint>( ( static_cast<uchar8>( data[index * 2] ) - 128 ) << 8 );
    right = static_cast<sint>( ( static_cast<uchar8>( data[index * 2 + 1] ) - 128 ) << 8 );
    
    return ( left + right ) / 2;
}
static sint dmaHD_GetSampleRaw_16bitStereo( sint index, sint samples, uchar8* data )
{
    sint left, right;
    
    if( index < 0 )
    {
        index += samples;
    }
    else if( index >= samples )
    {
        index -= samples;
    }
    left = static_cast<sint>( LittleShort( ( reinterpret_cast<schar16*>( data ) )[index * 2] ) );
    right = static_cast<sint>( LittleShort( ( reinterpret_cast<schar16*>( data ) )[index * 2 + 1] ) );
    
    return ( left + right ) / 2;
}

// Get only decimal part (a - floor(a))
#define FLOAT_DECIMAL_PART(a) (a-(float32)((sint)a))

// t must be a float32 between 0 and samples
static sint dmaHD_GetInterpolatedSampleHermite4pt3oX( float32 t, sint samples, uchar8* data, sint( *dmaHD_GetSampleRaw )( sint, sint, uchar8* ) )
{
    sint x, val;
    
    t = dmaHD_NormalizeSamplePosition( t, samples );
    
    // Get points
    x = static_cast<sint>( t );
    
    // Interpolate
    val = static_cast<sint>( dmaHD_InterpolateHermite4pt3oX(
                                 static_cast<float32>( dmaHD_GetSampleRaw( x - 1, samples, data ) ),
                                 static_cast<float32>( dmaHD_GetSampleRaw( x, samples, data ) ),
                                 static_cast<float32>( dmaHD_GetSampleRaw( x + 1, samples, data ) ),
                                 static_cast<float32>( dmaHD_GetSampleRaw( x + 2, samples, data ) ), FLOAT_DECIMAL_PART( t ) ) );
                                 
    // Clamp
    return SMPCLAMP( val );
}

// t must be a float32 between 0 and samples
static sint dmaHD_GetInterpolatedSampleCubic( float32 t, sint samples, uchar8* data, sint( *dmaHD_GetSampleRaw )( sint, sint, uchar8* ) )
{
    sint x, val;
    
    t = dmaHD_NormalizeSamplePosition( t, samples );
    
    // Get points
    x = static_cast<sint>( t );
    
    // Interpolate
    val = static_cast<sint>( dmaHD_InterpolateCubic(
                                 static_cast<float32>( dmaHD_GetSampleRaw( x - 1, samples, data ) ),
                                 static_cast<float32>( dmaHD_GetSampleRaw( x, samples, data ) ),
                                 static_cast<float32>( dmaHD_GetSampleRaw( x + 1, samples, data ) ),
                                 static_cast<float32>( dmaHD_GetSampleRaw( x + 2, samples, data ) ), FLOAT_DECIMAL_PART( t ) ) );
                                 
    // Clamp
    return SMPCLAMP( val );
}

// t must be a float32 between 0 and samples
static sint dmaHD_GetInterpolatedSampleLinear( float32 t, sint samples, uchar8* data, sint( *dmaHD_GetSampleRaw )( sint, sint, uchar8* ) )
{
    sint x, val;
    float32 c0, c1;
    
    t = dmaHD_NormalizeSamplePosition( t, samples );
    
    // Get points
    x = static_cast<sint>( t );
    
    c0 = static_cast<float32>( dmaHD_GetSampleRaw( x, samples, data ) );
    c1 = static_cast<float32>( dmaHD_GetSampleRaw( x + 1, samples, data ) );
    
    val = static_cast<sint>( ( ( ( c1 - c0 ) * FLOAT_DECIMAL_PART( t ) ) + c0 ) );
    
    // No need to clamp for linear
    return val;
}

// t must be a float between 0 and samples
static sint dmaHD_GetNoInterpolationSample( float32 t, sint samples, uchar8* data, sint( *dmaHD_GetSampleRaw )( sint, sint, uchar8* ) )
{
    sint x;
    
    t = dmaHD_NormalizeSamplePosition( t, samples );
    
    // Get points
    x = static_cast<sint>( t );
    
    if( FLOAT_DECIMAL_PART( t ) > 0.5f )
    {
        x++;
    }
    
    return dmaHD_GetSampleRaw( x, samples, data );
}

sint( *dmaHD_GetInterpolatedSample )( float32 t, sint samples, uchar8* data, sint( *dmaHD_GetSampleRaw )( sint, sint, uchar8* ) ) = dmaHD_GetInterpolatedSampleHermite4pt3oX;

/*
================
dmaHD_ResampleSfx

resample / decimate to the current source rate
================
*/
void dmaHD_ResampleSfx( sfx_t* sfx, sint channels, sint inrate, sint inwidth, uchar8* data, bool compressed )
{
    schar16* buffer;
    sint( *dmaHD_GetSampleRaw )( sint, sint, uchar8* );
    float32 stepscale, idx_smp, sample, bsample;
    float32 lp_inva, lp_a, hp_a, lp_data, lp_last, hp_data, hp_last, hp_lastsample;
    sint outcount, idx_hp, idx_lp;
    
    stepscale = static_cast<float32>( inrate ) / static_cast<float32>( dma.speed );
    outcount = static_cast<sint>( ( static_cast<float32>( sfx->soundLength ) / stepscale ) );
    
    // Create secondary buffer for bass sound while performing lowpass filter;
    buffer = dmaHD_AllocateSoundBuffer( outcount * 2 );
    
    // Check if this is a weapon sound.
    sfx->weaponsound = ( memcmp( sfx->soundName, "sound/weapons/", 14 ) == 0 ) ? true : false;
    
    if( channels == 2 )
    {
        dmaHD_GetSampleRaw = ( inwidth == 2 ) ? dmaHD_GetSampleRaw_16bitStereo : dmaHD_GetSampleRaw_8bitStereo;
    }
    else
    {
        dmaHD_GetSampleRaw = ( inwidth == 2 ) ? dmaHD_GetSampleRaw_16bitMono : dmaHD_GetSampleRaw_8bitMono;
    }
    
    // Get last sample from sound effect.
    idx_smp = -( stepscale * 4.0f );
    sample = dmaHD_GetInterpolatedSample( idx_smp, sfx->soundLength, data, dmaHD_GetSampleRaw );
    bsample = dmaHD_GetNoInterpolationSample( idx_smp, sfx->soundLength, data, dmaHD_GetSampleRaw );
    idx_smp += stepscale;
    
    // Set up high pass filter.
    idx_hp = 0;
    hp_last = sample;
    hp_lastsample = sample;
    //buffer[idx_hp++] = sample;
    hp_a = 0.95f;
    
    // Set up Low pass filter.
    idx_lp = outcount;
    lp_last = bsample;
    lp_a = 0.03f;
    lp_inva = ( 1 - lp_a );
    
    // Now do actual high/low pass on actual data.
    for( ; idx_hp < outcount; idx_hp++ )
    {
        sample = dmaHD_GetInterpolatedSample( idx_smp, sfx->soundLength, data, dmaHD_GetSampleRaw );
        bsample = dmaHD_GetNoInterpolationSample( idx_smp, sfx->soundLength, data, dmaHD_GetSampleRaw );
        idx_smp += stepscale;
        
        // High pass.
        hp_data = hp_a * ( hp_last + sample - hp_lastsample );
        buffer[idx_hp] = SMPCLAMP( hp_data );
        hp_last = hp_data;
        hp_lastsample = sample;
        
        // Low pass.
        lp_data = lp_a * static_cast<float32>( bsample ) + lp_inva * lp_last;
        buffer[idx_lp++] = SMPCLAMP( lp_data );
        lp_last = lp_data;
    }
    
    sfx->soundData = ( sndBuffer* )buffer;
    sfx->soundLength = outcount;
}

bool dmaHD_LoadSound( sfx_t* sfx )
{
    uchar8* data;
    snd_info_t info;
    valueType dmahd_soundName[MAX_QPATH];
    valueType* lpext;
    
    // Player specific sounds are never directly loaded.
    if( sfx->soundName[0] == '*' ) return false;
    
    Q_strcpy_s( dmahd_soundName, sfx->soundName );
    if( ( lpext = strrchr( sfx->soundName, '.' ) ) != nullptr )
    {
        Q_strcpy_s( dmahd_soundName, sfx->soundName );
        *( strrchr( dmahd_soundName, '.' ) ) = '\0'; // for sure there is a '.'
    }
    strcat( dmahd_soundName, "_dmahd" );
    if( lpext != nullptr ) strcat( dmahd_soundName, lpext );
    
    // Just check if file exists
    if( fileSystem->FOpenFileRead( dmahd_soundName, nullptr, true ) == true )
    {
        // Load it in.
        if( !( data = static_cast<uchar8*>( S_CodecLoad( dmahd_soundName, &info ) ) ) )
        {
            return false;
        }
    }
    else
    {
        // Load it in.
        if( !( data = static_cast<uchar8*>( S_CodecLoad( sfx->soundName, &info ) ) ) )
        {
            return false;
        }
    }
    
    // Information
#ifdef _DEBUG
    Com_DPrintf( "Loading sound: %s", sfx->soundName );
#endif
    if( info.width == 1 )
    {
        Com_DPrintf( " [8 bit -> 16 bit]" );
    }
    
    if( info.rate != dma.speed )
    {
        Com_DPrintf( " [%d Hz -> %d Hz]", info.rate, dma.speed );
    }
    
    Com_DPrintf( "\n" );
    
    sfx->lastTimeUsed = Com_Milliseconds() + 1;
    
    // Do not compress.
    sfx->soundCompressionMethod = 0;
    sfx->soundLength = info.samples;
    sfx->soundData = nullptr;
    
    dmaHD_ResampleSfx( sfx, info.channels, info.rate, info.width, data + info.dataofs, false );
    
    // Free data allocated by Codec
    Z_Free( data );
    
    return true;
}

/*
===============================================================================
PART#02: dmaHD: Mixing
===============================================================================
*/

static void dmaHD_PaintChannelFrom16_HHRTF( channel_t* ch, const sfx_t* sc, sint count, sint sampleOffset, sint bufferOffset, sint chan )
{
    sint vol, i, so;
    portable_samplepair_t* samp = &dmaHD_paintbuffer[bufferOffset];
    schar16* samples;
    schar16* tsamples;
    sint* out;
    ch_side_t* chs = ( chan == 0 ) ? &ch->l : &ch->r;
    
    if( dmaHD_snd_vol <= 0 ) return;
    
    so = sampleOffset - chs->offset;
    if( so < 0 )
    {
        // [count -= (-so)] == [count += so]
        count += so;
        so = 0;
    }
    
    if( ( so + count ) >= sc->soundLength )
    {
        count = sc->soundLength - so;
    }
    
    if( count <= 0 )
    {
        return;
    }
    
    // Process low frequency
    if( chs->bassvol > 0 )
    {
        // Select bass frequency offset (just after high frequency)
        samples = &( reinterpret_cast<schar16*>( sc->soundData ) )[sc->soundLength];
        
        // Calculate volumes.
        vol = chs->bassvol * dmaHD_snd_vol;
        tsamples = &samples[so];
        out = reinterpret_cast<sint*>( samp );
        if( chan == 1 )
        {
            out++;
        }
        
        for( i = 0; i < count; i++ )
        {
            *out += ( *tsamples * vol ) >> 8;
            ++tsamples;
            ++out;
            ++out;
        }
    }
    
    // Process high frequency
    if( chs->vol > 0 )
    {
        // Select high frequency offset.
        samples = reinterpret_cast<schar16*>( sc->soundData );
        
        // Calculate volumes.
        vol = chs->vol * dmaHD_snd_vol;
        tsamples = &samples[so];
        out = reinterpret_cast<sint*>( samp );
        
        if( chan == 1 )
        {
            out++;
        }
        
        for( i = 0; i < count; i++ )
        {
            *out += ( *tsamples * vol ) >> 8;
            ++tsamples;
            ++out;
            ++out;
        }
    }
}

static void dmaHD_PaintChannelFrom16_dmaEX2( channel_t* ch, const sfx_t* sc, sint count, sint sampleOffset, sint bufferOffset )
{
    sint data, rvol, lvol, i, so;
    portable_samplepair_t* samp = &dmaHD_paintbuffer[bufferOffset];
    schar16* samples;
    schar16* tsamples;
    sint* out;
    
    if( dmaHD_snd_vol <= 0 )
    {
        return;
    }
    
    so = sampleOffset - ch->l.offset;
    
    if( so < 0 )
    {
        // [count -= (-so)] == [count += so]
        count += so;
        so = 0;
    }
    
    if( ( so + count ) > sc->soundLength )
    {
        count = sc->soundLength - so;
    }
    if( count <= 0 )
    {
        return;
    }
    
    // Process low frequency.
    if( ch->l.bassvol > 0 )
    {
        samples = &( reinterpret_cast<schar16*>( sc->soundData ) )[sc->soundLength]; // Select bass frequency offset (just after high frequency)
        
        // Calculate volumes.
        lvol = ch->l.bassvol * dmaHD_snd_vol;
        tsamples = &samples[so];
        out = reinterpret_cast<sint*>( samp );
        for( i = 0; i < count; i++ )
        {
            data = ( *tsamples * lvol ) >> 8;
            ++tsamples;
            *out += data;
            ++out; // L
            *out += data;
            ++out; // R
        }
    }
    
    // Process high frequency.
    if( ch->l.vol > 0 || ch->r.vol > 0 )
    {
        // Select high frequency offset.
        samples = reinterpret_cast<schar16*>( sc->soundData );
        
        // Calculate volumes.
        lvol = ch->l.vol * dmaHD_snd_vol;
        rvol = ch->r.vol * dmaHD_snd_vol;
        
        // Behind viewer?
        if( ch->fixed_origin && ch->sodrot[0] < 0 )
        {
            if( ch->r.vol > ch->l.vol )
            {
                lvol = -lvol;
            }
            else
            {
                rvol = -rvol;
            }
        }
        
        tsamples = &samples[so];
        out = reinterpret_cast<sint*>( samp );
        for( i = 0; i < count; i++ )
        {
            *out += ( *tsamples * lvol ) >> 8;
            ++out; // L
            *out += ( *tsamples * rvol ) >> 8;
            ++out; // R
            ++tsamples;
        }
    }
    
    // Process high frequency reverb.
    if( ch->l.reverbvol > 0 || ch->r.reverbvol > 0 )
    {
        // Select high frequency offset.
        samples = reinterpret_cast<schar16*>( sc->soundData );
        so = sampleOffset - ch->l.reverboffset;
        if( so < 0 )
        {
            // [count -= (-so)] == [count += so]
            count += so;
            so = 0;
        }
        
        if( ( so + count ) > sc->soundLength )
        {
            count = sc->soundLength - so;
        }
        
        // Calculate volumes for reverb.
        lvol = ch->l.reverbvol * dmaHD_snd_vol;
        rvol = ch->r.reverbvol * dmaHD_snd_vol;
        tsamples = &samples[so];
        out = reinterpret_cast<sint*>( samp );
        
        for( i = 0; i < count; i++ )
        {
            *out += ( *tsamples * lvol ) >> 8;
            ++out; // L
            *out += ( *tsamples * rvol ) >> 8;
            ++out; // R
            ++tsamples;
        }
    }
}

static void dmaHD_PaintChannelFrom16_dmaEX( channel_t* ch, const sfx_t* sc, sint count, sint sampleOffset, sint bufferOffset )
{
    sint rvol, lvol, i, so;
    portable_samplepair_t* samp = &dmaHD_paintbuffer[bufferOffset];
    schar16* samples, *bsamples;
    sint* out;
    
    if( dmaHD_snd_vol <= 0 )
    {
        return;
    }
    
    so = sampleOffset - ch->l.offset;
    if( so < 0 )
    {
        count += so;    // [count -= (-so)] == [count += so]
        so = 0;
    }
    if( ( so + count ) > sc->soundLength )
    {
        count = sc->soundLength - so;
    }
    if( count <= 0 )
    {
        return;
    }
    if( ch->l.vol <= 0 && ch->r.vol <= 0 )
    {
        return;
    }
    
    samples = &( reinterpret_cast<schar16*>( sc->soundData ) )[so]; // Select high frequency offset.
    bsamples = &( reinterpret_cast<schar16*>( sc->soundData ) )[sc->soundLength + so]; // Select bass frequency offset (just after high frequency)
    
    // Calculate volumes.
    lvol = ch->l.vol * dmaHD_snd_vol;
    rvol = ch->r.vol * dmaHD_snd_vol;
    
    // Behind viewer?
    if( ch->fixed_origin && ch->sodrot[0] < 0 )
    {
        if( lvol < rvol )
        {
            lvol = -lvol;
        }
        else
        {
            rvol = -rvol;
        }
    }
    out = reinterpret_cast<sint*>( samp );
    for( i = 0; i < count; i++ )
    {
        *out += ( ( *samples * lvol ) >> 8 ) + ( ( *bsamples * lvol ) >> 8 );
        ++out; // L
        *out += ( ( *samples * rvol ) >> 8 ) + ( ( *bsamples * rvol ) >> 8 );
        ++out; // R
        ++samples;
        ++bsamples;
    }
}

static void dmaHD_PaintChannelFrom16( channel_t* ch, const sfx_t* sc, sint count, sint sampleOffset, sint bufferOffset )
{
    switch( dmaHD_Mixer->integer )
    {
            // Hybrid-HRTF
        case 10:
        case 11:
            dmaHD_PaintChannelFrom16_HHRTF( ch, sc, count, sampleOffset, bufferOffset, 0 ); // LEFT
            dmaHD_PaintChannelFrom16_HHRTF( ch, sc, count, sampleOffset, bufferOffset, 1 ); // RIGHT
            break;
            // dmaEX2
        case 20:
            dmaHD_PaintChannelFrom16_dmaEX2( ch, sc, count, sampleOffset, bufferOffset );
            break;
        case 21:
            // No reverb.
            ch->l.reverbvol = ch->r.reverbvol = 0;
            dmaHD_PaintChannelFrom16_dmaEX2( ch, sc, count, sampleOffset, bufferOffset );
            break;
            // dmaEX
        case 30:
            dmaHD_PaintChannelFrom16_dmaEX( ch, sc, count, sampleOffset, bufferOffset );
            break;
    }
}

extern sint* snd_p;
extern sint snd_linear_count;
extern schar16* snd_out;
extern void S_TransferPaintBuffer( sint endtime, portable_samplepair_t paintbuffer[] );

void dmaHD_PaintChannels( sint endtime )
{
    sint i;
    sint end;
    channel_t* ch;
    sfx_t* sc;
    sint ltime, count;
    sint sampleOffset;
#ifdef MAX_RAW_STREAMS
    sint stream;
#endif
    
    dmaHD_snd_vol =
#ifdef MAX_RAW_STREAMS
        ( s_muted->integer ) ? 0 :
#endif
        s_volume->value * 256;
        
    while( s_paintedtime < endtime )
    {
        // if paintbuffer is smaller than DMA buffer we may need to fill it multiple times
        end = endtime;
        if( ( endtime - s_paintedtime ) >= DMAHD_PAINTBUFFER_SIZE )
        {
            end = s_paintedtime + DMAHD_PAINTBUFFER_SIZE;
        }
        
#ifdef MAX_RAW_STREAMS
        // clear the paint buffer and mix any raw samples...
        Com_Memset( dmaHD_paintbuffer, 0, sizeof( dmaHD_paintbuffer ) );
        for( stream = 0; stream < MAX_RAW_STREAMS; stream++ )
        {
            if( s_rawend[stream] >= s_paintedtime )
            {
                // copy from the streaming sound source
                const portable_samplepair_t* rawsamples = s_rawsamples[stream];
                const sint stop = ( end < s_rawend[stream] ) ? end : s_rawend[stream];
                for( i = s_paintedtime ; i < stop ; i++ )
                {
                    const sint s = i & ( MAX_RAW_SAMPLES - 1 );
                    dmaHD_paintbuffer[i - s_paintedtime].left += rawsamples[s].left;
                    dmaHD_paintbuffer[i - s_paintedtime].right += rawsamples[s].right;
                }
            }
        }
#else
        // clear the paint buffer to either music or zeros
        if( s_rawend < s_paintedtime )
        {
            ::memset( dmaHD_paintbuffer, 0, ( end - s_paintedtime ) * sizeof( portable_samplepair_t ) );
        }
        else
        {
            // copy from the streaming sound source
            sint s;
            sint stop;
        
            stop = ( end < s_rawend ) ? end : s_rawend;
        
            for( i = s_paintedtime ; i < stop ; i++ )
            {
                s = i & ( MAX_RAW_SAMPLES - 1 );
                dmaHD_paintbuffer[i - s_paintedtime].left = s_rawsamples[s].left;
                dmaHD_paintbuffer[i - s_paintedtime].right = s_rawsamples[s].right;
            }
            for( ; i < end ; i++ )
            {
                dmaHD_paintbuffer[i - s_paintedtime].left = 0;
                dmaHD_paintbuffer[i - s_paintedtime].right = 0;
            }
        }
#endif
        
        // paint in the channels.
        ch = s_channels;
        for( i = 0; i < MAX_CHANNELS ; i++, ch++ )
        {
            if( !ch->thesfx )
            {
                continue;
            }
            
            ltime = s_paintedtime;
            sc = ch->thesfx;
            sampleOffset = ltime - ch->startSample;
            count = end - ltime;
            
            if( sampleOffset + count >= sc->soundLength )
            {
                count = sc->soundLength - sampleOffset;
            }
            
            if( count > 0 )
            {
                dmaHD_PaintChannelFrom16( ch, sc, count, sampleOffset, 0 );
            }
        }
        
        // paint in the looped channels.
        ch = loop_channels;
        for( i = 0; i < numLoopChannels ; i++, ch++ )
        {
            if( !ch->thesfx )
            {
                continue;
            }
            
            ltime = s_paintedtime;
            sc = ch->thesfx;
            
            if( sc->soundData == nullptr || sc->soundLength == 0 )
            {
                continue;
            }
            // we might have to make two passes if it is a looping sound effect and the end of the sample is hit
            do
            {
                sampleOffset = ( ltime % sc->soundLength );
                count = end - ltime;
                if( sampleOffset + count >= sc->soundLength )
                {
                    count = sc->soundLength - sampleOffset;
                }
                if( count > 0 )
                {
                    dmaHD_PaintChannelFrom16( ch, sc, count, sampleOffset, ltime - s_paintedtime );
                    ltime += count;
                }
            }
            while( ltime < end );
        }
        
        // transfer out according to DMA format
        S_TransferPaintBuffer( end, dmaHD_paintbuffer );
        s_paintedtime = end;
    }
}

/*
===============================================================================
PART#03: dmaHD: main
===============================================================================
*/


/*
=================
dmaHD_SpatializeReset

Reset/Prepares channel before calling dmaHD_SpatializeOrigin
=================
*/
void dmaHD_SpatializeReset( channel_t* ch )
{
    VectorClear( ch->sodrot );
    ::memset( &ch->l, 0, sizeof( ch_side_t ) );
    ::memset( &ch->r, 0, sizeof( ch_side_t ) );
}

/*
=================
dmaHD_SpatializeOrigin

Used for spatializing s_channels
=================
*/

#define CALCVOL(dist) (((tmp = (sint)((float32)ch->master_vol * g_voltable[ \
(((idx = (dist / iattenuation)) > 255) ? 255 : idx)])) < 0) ? 0 : tmp)
#define CALCSMPOFF(dist) (dist * dma.speed) >> ismpshift

void dmaHD_SpatializeOrigin_HHRTF( vec3_t so, channel_t* ch )
{
    // so = sound origin/[d]irection/[n]ormalized/[rot]ated/[d]irection [l]eft/[d]irection [r]ight
    vec3_t sod, sodl, sodr;
    // lo = listener origin/[l]eft/[r]ight
    vec3_t lol, lor;
    // distance to ears/[l]eft/[r]ight
    sint distl, distr; // using sint since calculations are integer based.
    // temp, index
    sint tmp, idx;
    float32 t;
    
    sint iattenuation = ( dmaHD_inwater ) ? 2 : 6;
    sint ismpshift = ( dmaHD_inwater ) ? 19 : 17;
    
    // Increase attenuation for weapon sounds since they would be very loud!
    if( ch->thesfx && ch->thesfx->weaponsound )
    {
        iattenuation *= 2;
    }
    
    // Calculate sound direction.
    VectorSubtract( so, g_listener_origin, sod );
    // Rotate sound origin to listener axis
    VectorRotate( sod, g_listener_axis, ch->sodrot );
    
    // Origin for ears (~20cm apart)
    lol[0] = 0.0;
    lol[1] = 40;
    lol[2] = 0.0; // left
    lor[0] = 0.0;
    lor[1] = -40;
    lor[2] = 0.0; // right
    
    // Calculate sound direction.
    VectorSubtract( ch->sodrot, lol, sodl ); // left
    VectorSubtract( ch->sodrot, lor, sodr ); // right
    
    VectorNormalize( ch->sodrot );
    // Calculate length of sound origin direction vector.
    distl = static_cast<sint>( VectorNormalize( sodl ) ); // left
    distr = static_cast<sint>( VectorNormalize( sodr ) ); // right
    
    // Close enough to be at full volume?
    if( distl < 80 )
    {
        distl = 0; // left
    }
    if( distr < 80 )
    {
        distr = 0; // right
    }
    
    // Distance 384units = 1m
    // 340.29m/s (speed of sound at sea level)
    // Do surround effect with doppler.
    // 384.0 * 340.29 = 130671.36
    // Most similar is 2 ^ 17 = 131072; so shift right by 17 to divide by 131072
    
    // 1484m/s in water
    // 384.0 * 1484 = 569856
    // Most similar is 2 ^ 19 = 524288; so shift right by 19 to divide by 524288
    
    ch->l.offset = CALCSMPOFF( distl ); // left
    ch->r.offset = CALCSMPOFF( distr ); // right
    
    // Calculate volume at ears
    ch->l.bassvol = ch->l.vol = CALCVOL( distl ); // left
    ch->r.bassvol = ch->r.vol = CALCVOL( distr ); // right
    
    if( distl != 0 || distr != 0 )
    {
        // Sound originating from inside head of left ear (i.e. from right)
        if( ch->sodrot[1] < 0 )
        {
            ch->l.vol *= ( 1.0f + ( ch->sodrot[1] * 0.7f ) );
        }
        
        // Sound originating from inside head of right ear (i.e. from left)
        if( ch->sodrot[1] > 0 )
        {
            ch->r.vol *= ( 1.0f - ( ch->sodrot[1] * 0.7f ) );
        }
        
        // Calculate HRTF function (lowpass filter) parameters
        //if (ch->fixed_origin)
        {
            // Sound originating from behind viewer
            if( ch->sodrot[0] < 0 )
            {
                ch->l.vol *= ( 1.0f + ( ch->sodrot[0] * 0.05f ) );
                ch->r.vol *= ( 1.0f + ( ch->sodrot[0] * 0.05f ) );
                
                // 2ms max
                //t = -ch->sodrot[0] * 0.04f; if (t > 0.005f) t = 0.005f;
                t = ( dma.speed * 0.001f );
                ch->l.offset -= t;
                ch->r.offset += t;
            }
        }
        
        if( dmaHD_Mixer->integer == 10 )
        {
            // Sound originating from above viewer (decrease bass)
            // Sound originating from below viewer (increase bass)
            ch->l.bassvol *= ( ( 1 - ch->sodrot[2] ) * 0.5f );
            ch->r.bassvol *= ( ( 1 - ch->sodrot[2] ) * 0.5f );
        }
    }
    
    // Normalize volume
    ch->l.vol *= 0.5f;
    ch->r.vol *= 0.5f;
    
    if( dmaHD_inwater )
    {
        // Keep bass in water.
        ch->l.vol *= 0.2f;
        ch->r.vol *= 0.2f;
    }
}

void dmaHD_SpatializeOrigin_dmaEX2( vec3_t so, channel_t* ch )
{
    // so = sound origin/[d]irection/[n]ormalized/[rot]ated
    vec3_t sod;
    // distance to head
    sint dist; // using sint since calculations are integer based.
    // temp, index
    sint tmp, idx, vol;
    vec_t dot;
    
    sint iattenuation = ( dmaHD_inwater ) ? 2 : 6;
    sint ismpshift = ( dmaHD_inwater ) ? 19 : 17;
    
    // Increase attenuation for weapon sounds since they would be very loud!
    if( ch->thesfx && ch->thesfx->weaponsound )
    {
        iattenuation *= 2;
    }
    
    // Calculate sound direction.
    VectorSubtract( so, g_listener_origin, sod );
    // Rotate sound origin to listener axis
    VectorRotate( sod, g_listener_axis, ch->sodrot );
    
    VectorNormalize( ch->sodrot );
    // Calculate length of sound origin direction vector.
    dist = static_cast<sint>( VectorNormalize( sod ) ); // left
    
    // Close enough to be at full volume?
    if( dist < 0 )
    {
        dist = 0; // left
    }
    
    // Distance 384units = 1m
    // 340.29m/s (speed of sound at sea level)
    // Do surround effect with doppler.
    // 384.0 * 340.29 = 130671.36
    // Most similar is 2 ^ 17 = 131072; so shift right by 17 to divide by 131072
    
    // 1484m/s in water
    // 384.0 * 1484 = 569856
    // Most similar is 2 ^ 19 = 524288; so shift right by 19 to divide by 524288
    
    ch->l.offset = CALCSMPOFF( dist );
    
    // Calculate volume at ears
    vol = CALCVOL( dist );
    ch->l.vol = vol;
    ch->r.vol = vol;
    ch->l.bassvol = vol;
    
    dot = -ch->sodrot[1];
    ch->l.vol *= 0.5f * ( 1.0f - dot );
    ch->r.vol *= 0.5f * ( 1.0f + dot );
    
    // Calculate HRTF function (lowpass filter) parameters
    if( ch->fixed_origin )
    {
        // Reverberation
        dist += 768;
        ch->l.reverboffset = CALCSMPOFF( dist );
        vol = CALCVOL( dist );
        ch->l.reverbvol = vol;
        ch->r.reverbvol = vol;
        ch->l.reverbvol *= 0.5f * ( 1.0f + dot );
        ch->r.reverbvol *= 0.5f * ( 1.0f - dot );
        
        // Sound originating from behind viewer: decrease treble + reverb
        if( ch->sodrot[0] < 0 )
        {
            ch->l.vol *= ( 1.0f + ( ch->sodrot[0] * 0.5f ) );
            ch->r.vol *= ( 1.0f + ( ch->sodrot[0] * 0.5f ) );
        }
        else // from front...
        {
            // adjust reverb for each ear.
            if( ch->sodrot[1] < 0 )
            {
                ch->r.reverbvol = 0;
            }
            else if( ch->sodrot[1] > 0 )
            {
                ch->l.reverbvol = 0;
            }
        }
        
        // Sound originating from above viewer (decrease bass)
        // Sound originating from below viewer (increase bass)
        ch->l.bassvol *= ( ( 1 - ch->sodrot[2] ) * 0.5f );
    }
    else
    {
        // Reduce base volume by half to keep overall valume.
        ch->l.bassvol *= 0.5f;
    }
    
    if( dmaHD_inwater )
    {
        // Keep bass in water.
        ch->l.vol *= 0.2f;
        ch->r.vol *= 0.2f;
    }
}

void dmaHD_SpatializeOrigin_dmaEX( vec3_t origin, channel_t* ch )
{
    vec_t dot;
    vec_t dist;
    vec_t lscale, rscale, scale;
    vec3_t source_vec;
    sint tmp;
    
    const float32 dist_mult = SOUND_ATTENUATE;
    
    // calculate stereo seperation and distance attenuation
    VectorSubtract( origin, g_listener_origin, source_vec );
    
    // VectorNormalize returns original length of vector and normalizes vector.
    dist = VectorNormalize( source_vec );
    dist -= SOUND_FULLVOLUME;
    
    if( dist < 0 )
    {
        // close enough to be at full volume
        dist = 0;
    }
    
    // different attenuation levels
    dist *= dist_mult;
    
    VectorRotate( source_vec, g_listener_axis, ch->sodrot );
    
    dot = -ch->sodrot[1];
    
    // DMAEX - Multiply by the stereo separation CVAR.
    dot *= dmaEX_StereoSeparation->value;
    
    rscale = 0.5f * ( 1.0f + dot );
    lscale = 0.5f * ( 1.0f - dot );
    if( rscale < 0 )
    {
        rscale = 0;
    }
    
    if( lscale < 0 )
    {
        lscale = 0;
    }
    
    // add in distance effect
    scale = ( 1.0f - dist ) * rscale;
    tmp = ( ch->master_vol * scale );
    if( tmp < 0 )
    {
        tmp = 0;
    }
    ch->r.vol = tmp;
    
    scale = ( 1.0f - dist ) * lscale;
    tmp = ( ch->master_vol * scale );
    if( tmp < 0 ) tmp = 0;
    ch->l.vol = tmp;
}

void dmaHD_SpatializeOrigin( vec3_t so, channel_t* ch )
{
    switch( dmaHD_Mixer->integer )
    {
            // HHRTF
        case 10:
        case 11:
            dmaHD_SpatializeOrigin_HHRTF( so, ch );
            break;
            // dmaEX2
        case 20:
        case 21:
            dmaHD_SpatializeOrigin_dmaEX2( so, ch );
            break;
            // dmaEX
        case 30:
            dmaHD_SpatializeOrigin_dmaEX( so, ch );
            break;
    }
}

/*
==============================================================
continuous looping sounds are added each frame
==============================================================
*/

/*
==================
dmaHD_AddLoopSounds

Spatialize all of the looping sounds.
All sounds are on the same cycle, so any duplicates can just
sum up the channel multipliers.
==================
*/
void dmaHD_AddLoopSounds( void )
{
    sint i, time;
    channel_t* ch;
    loopSound_t* loop;
    static sint loopFrame;
    
    numLoopChannels = 0;
    
    time = Com_Milliseconds();
    
    loopFrame++;
    
    //#pragma omp parallel for private(loop, ch)
    for( i = 0 ; i < MAX_GENTITIES; i++ )
    {
        if( numLoopChannels >= MAX_CHANNELS )
        {
            continue;
        }
        
        loop = &loopSounds[i];
        // already merged into an earlier sound
        if( !loop->active || loop->mergeFrame == loopFrame )
        {
            continue;
        }
        
        // allocate a channel
        ch = &loop_channels[numLoopChannels];
        
        dmaHD_SpatializeReset( ch );
        ch->fixed_origin = true;
        ch->master_vol = ( loop->kill ) ? 127 : 90; // 3D / Sphere
        dmaHD_SpatializeOrigin( loop->origin, ch );
        
        loop->sfx->lastTimeUsed = time;
        
        ch->master_vol = 127;
        
        // Clip volumes.
        ch->l.vol = VOLCLAMP( ch->l.vol );
        ch->r.vol = VOLCLAMP( ch->r.vol );
        ch->l.bassvol = VOLCLAMP( ch->l.bassvol );
        ch->r.bassvol = VOLCLAMP( ch->r.bassvol );
        ch->thesfx = loop->sfx;
        ch->doppler = false;
        
        //#pragma omp critical
        {
            numLoopChannels++;
        }
    }
}

//=============================================================================

/*
============
dmaHD_Respatialize

Change the volumes of all the playing sounds for changes in their positions
============
*/
void dmaHD_Respatialize( sint entityNum, const vec3_t head, vec3_t axis[3], sint inwater )
{
    sint i;
    channel_t* ch;
    vec3_t origin;
    
    if( !s_soundStarted || s_soundMuted )
    {
        return;
    }
    
    dmaHD_inwater = inwater;
    
    listener_number = entityNum;
    VectorCopy( head, g_listener_origin );
    VectorCopy( axis[0], g_listener_axis[0] );
    VectorCopy( axis[1], g_listener_axis[1] );
    VectorCopy( axis[2], g_listener_axis[2] );
    
    // update spatialization for dynamic sounds
    //#pragma omp parallel for private(ch)
    for( i = 0 ; i < MAX_CHANNELS; i++ )
    {
        ch = &s_channels[i];
        if( !ch->thesfx )
        {
            continue;
        }
        
        dmaHD_SpatializeReset( ch );
        // Anything coming from the view entity will always be full volume
        if( ch->entnum == listener_number )
        {
            ch->l.vol = ch->master_vol;
            ch->r.vol = ch->master_vol;
            ch->l.bassvol = ch->master_vol;
            ch->r.bassvol = ch->master_vol;
            
            switch( dmaHD_Mixer->integer )
            {
                case 10:
                case 11:
                case 20:
                case 21:
                    if( dmaHD_inwater )
                    {
                        ch->l.vol *= 0.2f;
                        ch->r.vol *= 0.2f;
                    }
                    break;
            }
        }
        else
        {
            if( ch->fixed_origin )
            {
                VectorCopy( ch->origin, origin );
            }
            else
            {
                VectorCopy( loopSounds[ ch->entnum ].origin, origin );
            }
            
            dmaHD_SpatializeOrigin( origin, ch );
        }
    }
    
    // add loopsounds
    dmaHD_AddLoopSounds();
}

/*
============
dmaHD_Update

Called once each time through the main loop
============
*/
void dmaHD_Update( void )
{
    if( !s_soundStarted || s_soundMuted )
    {
        return;
    }
    
    // add raw data from streamed samples
    S_UpdateBackgroundTrack();
    
    // mix some sound
    dmaHD_Update_Mix();
}

void dmaHD_Update_Mix( void )
{
    uint endtime;
    static sint lastTime = 0.0f;
    sint mixahead, op, thisTime, sane;
    static sint lastsoundtime = -1;
    
    if( !s_soundStarted || s_soundMuted ) return;
    
    thisTime = Com_Milliseconds();
    
    // Updates s_soundtime
    S_GetSoundtime();
    
    if( s_soundtime <= lastsoundtime ) return;
    lastsoundtime = s_soundtime;
    
    // clear any sound effects that end before the current time,
    // and start any new sounds
    S_ScanChannelStarts();
    
    if( ( sane = thisTime - lastTime ) < 8 )
    {
        // ms since last mix (cap to 8ms @ 125fps)
        sane = 8;
    }
    op = static_cast<sint>( ( static_cast<float32>( ( dma.speed ) * sane ) * 0.001f ) ); // samples to mix based on last mix time
    mixahead = static_cast<sint>( ( static_cast<float32>( dma.speed ) * s_mixahead->value ) );
    
    if( mixahead < op )
    {
        mixahead = op;
    }
    
    // mix ahead of current position
    endtime = s_soundtime + mixahead;
    
    // never mix more than the complete buffer
    if( endtime - s_soundtime > dma.fullsamples )
    {
        endtime = s_soundtime + dma.fullsamples;
    }
    
    SNDDMA_BeginPainting();
    
    dmaHD_PaintChannels( endtime );
    
    SNDDMA_Submit();
    
    lastTime = thisTime;
}

/*
================
dmaHD_Enabled
================
*/
bool dmaHD_Enabled( void )
{
    if( dmaHD_Enable == nullptr )
    {
        dmaHD_Enable = cvarSystem->Get( "dmaHD_enable", "1", CVAR_ARCHIVE, "Enable HD sound system" );
    }
    
    return ( dmaHD_Enable->integer );
}

// ====================================================================
// User-setable variables
// ====================================================================
void dmaHD_SoundInfo( void )
{
    Com_Printf( "\n" );
    Com_Printf( "dmaHD 3D software sound engine by p5yc0runn3r\n" );
    
    if( !s_soundStarted )
    {
        Com_Printf( " Engine not started.\n" );
    }
    else
    {
        switch( dmaHD_Mixer->integer )
        {
            case 10:
                Com_Printf( " dmaHD full 3D sound mixer [10]\n" );
                break;
            case 11:
                Com_Printf( " dmaHD planar 3D sound mixer [11]\n" );
                break;
            case 20:
                Com_Printf( " dmaEX2 sound mixer [20]\n" );
                break;
            case 21:
                Com_Printf( " dmaEX2 sound mixer with no reverb [21]\n" );
                break;
            case 30:
                Com_Printf( " dmaEX sound mixer [30]\n" );
                break;
        }
        Com_Printf( " %d ch / %d Hz / %d bps\n", dma.channels, dma.speed, dma.samplebits );
        if( s_numSfx > 0 || g_dmaHD_allocatedsoundmemory > 0 )
        {
            Com_Printf( " %d sounds in %.2f MiB\n", s_numSfx, static_cast<float32>( g_dmaHD_allocatedsoundmemory ) / 1048576.0f );
        }
        else
        {
            Com_Printf( " No sounds loaded yet.\n" );
        }
    }
    Com_Printf( "\n" );
}

void dmaHD_SoundList( void )
{
    sint i;
    sfx_t* sfx;
    
    Com_Printf( "\n" );
    Com_Printf( "dmaHD HRTF sound engine by p5yc0runn3r\n" );
    
    if( s_numSfx > 0 || g_dmaHD_allocatedsoundmemory > 0 )
    {
        for( sfx = s_knownSfx, i = 0; i < s_numSfx; i++, sfx++ )
        {
            Com_Printf( " %s %.2f KiB %s\n", sfx->soundName, static_cast<float32>( sfx->soundLength ) / 1024.0f, ( sfx->inMemory ? "" : "!" ) );
        }
        Com_Printf( " %d sounds in %.2f MiB\n", s_numSfx, static_cast<float32>( g_dmaHD_allocatedsoundmemory ) / 1048576.0f );
    }
    else
    {
        Com_Printf( " No sounds loaded yet.\n" );
    }
    Com_Printf( "\n" );
}


/*
================
dmaHD_Init
================
*/
bool dmaHD_Init( soundInterface_t* si )
{
    if( !si )
    {
        return false;
    }
    
    // Return if not enabled
    if( !dmaHD_Enabled() )
    {
        return true;
    }
    
    dmaHD_Mixer = cvarSystem->Get( "dmaHD_mixer", "10", CVAR_ARCHIVE, "Active mixer [10=hHRTF-3D,11=hHRTF-2D,20=dmaEX2,21=dmaEX2-noverb|30=dmaEX]" );
    if( dmaHD_Mixer->integer != 10 && dmaHD_Mixer->integer != 11 && dmaHD_Mixer->integer != 20 && dmaHD_Mixer->integer != 21 && dmaHD_Mixer->integer != 30 )
    {
        cvarSystem->Set( "dmaHD_Mixer", "10" );
        dmaHD_Mixer = cvarSystem->Get( "dmaHD_mixer", "10", CVAR_ARCHIVE, "Active mixer [10=hHRTF-3D,11=hHRTF-2D,20=dmaEX2,21=dmaEX2-noverb|30=dmaEX]" );
    }
    
    dmaEX_StereoSeparation = cvarSystem->Get( "dmaEX_StereoSeparation", "0.9", CVAR_ARCHIVE, "dmaHD stereo separation amount [0.1,2.0|def=0.9]" );
    if( dmaEX_StereoSeparation->value < 0.1 )
    {
        cvarSystem->Set( "dmaEX_StereoSeparation", "0.1" );
        dmaEX_StereoSeparation = cvarSystem->Get( "dmaEX_StereoSeparation", "0.9", CVAR_ARCHIVE, "dmaHD stereo separation amount [0.1,2.0|def=0.9]" );
    }
    else if( dmaEX_StereoSeparation->value > 2.0 )
    {
        cvarSystem->Set( "dmaEX_StereoSeparation", "2.0" );
        dmaEX_StereoSeparation = cvarSystem->Get( "dmaEX_StereoSeparation", "0.9", CVAR_ARCHIVE, "dmaHD stereo separation amount [0.1,2.0|def=0.9]" );
    }
    
    dmaHD_Interpolation = cvarSystem->Get( "dmaHD_interpolation", "3", CVAR_ARCHIVE, "This will set the type of sound re-sampling interpolation used [1,2,3,4]" );
    if( dmaHD_Interpolation->integer == 0 )
    {
        dmaHD_GetInterpolatedSample = dmaHD_GetNoInterpolationSample;
    }
    else if( dmaHD_Interpolation->integer == 1 )
    {
        dmaHD_GetInterpolatedSample = dmaHD_GetInterpolatedSampleLinear;
    }
    else if( dmaHD_Interpolation->integer == 2 )
    {
        dmaHD_GetInterpolatedSample = dmaHD_GetInterpolatedSampleCubic;
    }
    else //if (dmaHD_Interpolation->integer == 3) // DEFAULT
    {
        dmaHD_GetInterpolatedSample = dmaHD_GetInterpolatedSampleHermite4pt3oX;
    }
    
    dmaHD_InitTables();
    
    // Override function pointers to dmaHD version, the rest keep base.
    si->SoundInfo = dmaHD_SoundInfo;
    si->Respatialize = dmaHD_Respatialize;
    si->Update = dmaHD_Update;
    si->SoundList = dmaHD_SoundList;
    
    return true;
}

#endif//NO_DMAHD
