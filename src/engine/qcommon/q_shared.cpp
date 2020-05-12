////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2020 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   q_shared.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: stateless support routines that are included in each code dll
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.h>
#elif defined DEDICATED
#include <null/null_serverprecompiled.h>
#elif defined GUI
#include <GUI/gui_precompiled.h>
#elif defined CGAMEDLL
#include <cgame/cgame_precompiled.h>
#elif defined GAMEDLL
#include <sgame/sgame_precompiled.h>
#else
#include <framework/precompiled.h>
#endif

// os x game bundles have no standard library links, and the defines are not always defined!

#ifdef MACOS_X
S32 qmax( S32 x, S32 y )
{
    return ( ( ( x ) > ( y ) ) ? ( x ) : ( y ) );
}

S32 qmin( S32 x, S32 y )
{
    return ( ( ( x ) < ( y ) ) ? ( x ) : ( y ) );
}
#endif

memStream_t* AllocMemStream( U8* buffer, S32 bufSize )
{
    memStream_t*		s;
    
    if( buffer == nullptr || bufSize <= 0 )
        return nullptr;
        
    s = ( memStream_t* )malloc( sizeof( memStream_t ) );
    if( s == nullptr )
        return nullptr;
        
    ::memset( s, 0, sizeof( memStream_t ) );
    
    s->buffer 	= buffer;
    s->curPos 	= buffer;
    s->bufSize	= bufSize;
    s->flags	= 0;
    
    return s;
}

void FreeMemStream( memStream_t* s )
{
    free( s );
}

S32 MemStreamRead( memStream_t* s, void* buffer, S32 len )
{
    S32				ret = 1;
    
    if( s == nullptr || buffer == nullptr )
        return 0;
        
    if( s->curPos + len > s->buffer + s->bufSize )
    {
        s->flags |= MEMSTREAM_FLAGS_EOF;
        len = s->buffer + s->bufSize - s->curPos;
        ret = 0;
        
        Com_Error( ERR_FATAL, "MemStreamRead: EOF reached" );
    }
    
    ::memcpy( buffer, s->curPos, len );
    s->curPos += len;
    
    return ret;
}

S32 MemStreamGetC( memStream_t* s )
{
    S32				c = 0;
    
    if( s == nullptr )
        return -1;
        
    if( MemStreamRead( s, &c, 1 ) == 0 )
        return -1;
        
    return c;
}

S32 MemStreamGetLong( memStream_t* s )
{
    S32				c = 0;
    
    if( s == nullptr )
        return -1;
        
    if( MemStreamRead( s, &c, 4 ) == 0 )
        return -1;
        
    return LittleLong( c );
}

S32 MemStreamGetShort( memStream_t* s )
{
    S32				c = 0;
    
    if( s == nullptr )
        return -1;
        
    if( MemStreamRead( s, &c, 2 ) == 0 )
        return -1;
        
    return LittleShort( c );
}

F32 MemStreamGetFloat( memStream_t* s )
{
    floatint_t		c;
    
    if( s == nullptr )
        return -1;
        
    if( MemStreamRead( s, &c.i, 4 ) == 0 )
        return -1;
        
    return LittleFloat( c.f );
}

//=============================================================================

F32 Com_Clamp( F32 min, F32 max, F32 value )
{
    if( value < min )
    {
        return min;
    }
    if( value > max )
    {
        return max;
    }
    return value;
}


/*
COM_FixPath()
unixifies a pathname
*/

void COM_FixPath( UTF8* pathname )
{
    while( *pathname )
    {
        if( *pathname == '\\' )
        {
            *pathname = '/';
        }
        pathname++;
    }
}



/*
============
COM_SkipPath
============
*/
UTF8* COM_SkipPath( UTF8* pathname )
{
    UTF8*    last;
    
    last = pathname;
    while( *pathname )
    {
        if( *pathname == '/' )
        {
            last = pathname + 1;
        }
        pathname++;
    }
    return last;
}

/*
==================
Com_CharIsOneOfCharset
==================
*/
static bool Com_CharIsOneOfCharset( UTF8 c, UTF8* set )
{
    S32 i;
    
    for( i = 0; i < strlen( set ); i++ )
    {
        if( set[ i ] == c )
            return true;
    }
    
    return false;
}

/*
==================
Com_SkipCharset
==================
*/
UTF8* Com_SkipCharset( UTF8* s, UTF8* sep )
{
    UTF8*	p = s;
    
    while( p )
    {
        if( Com_CharIsOneOfCharset( *p, sep ) )
            p++;
        else
            break;
    }
    
    return p;
}


/*
==================
Com_SkipTokens
==================
*/
UTF8* Com_SkipTokens( UTF8* s, S32 numTokens, UTF8* sep )
{
    S32		sepCount = 0;
    UTF8*	p = s;
    
    while( sepCount < numTokens )
    {
        if( Com_CharIsOneOfCharset( *p++, sep ) )
        {
            sepCount++;
            while( Com_CharIsOneOfCharset( *p, sep ) )
                p++;
        }
        else if( *p == '\0' )
            break;
    }
    
    if( sepCount == numTokens )
        return p;
    else
        return s;
}


/*
============
COM_GetExtension
============
*/
StringEntry     COM_GetExtension( StringEntry name )
{
    S32             length, i;
    
    length = strlen( name ) - 1;
    i = length;
    
    while( name[i] != '.' )
    {
        i--;
        if( name[i] == '/' || i == 0 )
            return "";                      // no extension
    }
    
    return &name[i + 1];
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension( StringEntry in, UTF8* out )
{
    while( *in && *in != '.' )
    {
        *out++ = *in++;
    }
    *out = 0;
}

/*
============
COM_StripExtension2
a safer version
============
*/
void COM_StripExtension2( StringEntry in, UTF8* out, S32 destsize )
{
    S32 len = 0;
    while( len < destsize - 1 && *in && *in != '.' )
    {
        *out++ = *in++;
        len++;
    }
    *out = 0;
}

void COM_StripFilename( UTF8* in, UTF8* out )
{
    UTF8* end;
    Q_strncpyz( out, in, strlen( in ) + 1 );
    end = COM_SkipPath( out );
    *end = 0;
}


/*
============
COM_StripExtension3

RB: ioquake3 version
============
*/
void COM_StripExtension3( StringEntry src, UTF8* dest, S32 destsize )
{
    S32             length;
    
    Q_strncpyz( dest, src, destsize );
    
    length = strlen( dest ) - 1;
    
    while( length > 0 && dest[length] != '.' )
    {
        length--;
        
        if( dest[length] == '/' )
            return;				// no extension
    }
    
    if( length )
    {
        dest[length] = 0;
    }
}


/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension( UTF8* path, S32 maxSize, StringEntry extension )
{
    UTF8 oldPath[MAX_QPATH];
    UTF8*    src;
    
//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//
    src = path + strlen( path ) - 1;
    
    while( *src != '/' && src != path )
    {
        if( *src == '.' )
        {
            return;                 // it has an extension
        }
        src--;
    }
    
    Q_strncpyz( oldPath, path, sizeof( oldPath ) );
    Com_sprintf( path, maxSize, "%s%s", oldPath, extension );
}

//============================================================================

/*
============
Com_HashKey
============
*/
S32 Com_HashKey( UTF8* string, S32 maxlen )
{
    register S32    hash, i;
    
    hash = 0;
    for( i = 0; i < maxlen && string[i] != '\0'; i++ )
    {
        hash += string[i] * ( 119 + i );
    }
    hash = ( hash ^ ( hash >> 10 ) ^ ( hash >> 20 ) );
    return hash;
}

//============================================================================


/*
==================
COM_BitCheck

  Allows bit-wise checks on arrays with more than one item (> 32 bits)
==================
*/
bool COM_BitCheck( const S32 array[], S32 bitNum )
{
    S32 i;
    
    i = 0;
    while( bitNum > 31 )
    {
        i++;
        bitNum -= 32;
    }
    
    return ( bool )( ( array[i] & ( 1 << bitNum ) ) != 0 ); // (SA) heh, whoops. :)
}

/*
==================
COM_BitSet

  Allows bit-wise SETS on arrays with more than one item (> 32 bits)
==================
*/
void COM_BitSet( S32 array[], S32 bitNum )
{
    S32 i;
    
    i = 0;
    while( bitNum > 31 )
    {
        i++;
        bitNum -= 32;
    }
    
    array[i] |= ( 1 << bitNum );
}

/*
==================
COM_BitClear

  Allows bit-wise CLEAR on arrays with more than one item (> 32 bits)
==================
*/
void COM_BitClear( S32 array[], S32 bitNum )
{
    S32 i;
    
    i = 0;
    while( bitNum > 31 )
    {
        i++;
        bitNum -= 32;
    }
    
    array[i] &= ~( 1 << bitNum );
}
//============================================================================

/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

S16 ShortSwap( S16 l )
{
    U8 b1, b2;
    
    b1 = l & 255;
    b2 = ( l >> 8 ) & 255;
    
    return ( b1 << 8 ) + b2;
}

S16 ShortNoSwap( S16 l )
{
    return l;
}

S32    LongSwap( S32 l )
{
    U8 b1, b2, b3, b4;
    
    b1 = l & 255;
    b2 = ( l >> 8 ) & 255;
    b3 = ( l >> 16 ) & 255;
    b4 = ( l >> 24 ) & 255;
    
    return ( ( S32 )b1 << 24 ) + ( ( S32 )b2 << 16 ) + ( ( S32 )b3 << 8 ) + b4;
}

S32 LongNoSwap( S32 l )
{
    return l;
}

F32 FloatSwap( F32 f )
{
    union
    {
        F32 f;
        U8 b[4];
    } dat1, dat2;
    
    
    dat1.f = f;
    dat2.b[0] = dat1.b[3];
    dat2.b[1] = dat1.b[2];
    dat2.b[2] = dat1.b[1];
    dat2.b[3] = dat1.b[0];
    return dat2.f;
}

F32 FloatNoSwap( F32 f )
{
    return f;
}

/*
============================================================================

PARSING

============================================================================
*/

// multiple character punctuation tokens
StringEntry     punctuation[] =
{
    "+=", "-=", "*=", "/=", "&=", "|=", "++", "--",
    "&&", "||", "<=", ">=", "==", "!=",
    nullptr
};

static UTF8 com_token[MAX_TOKEN_CHARS];
static UTF8 com_parsename[MAX_TOKEN_CHARS];
static S32 com_lines;

static S32 backup_lines;
static UTF8*    backup_text;

void COM_BeginParseSession( StringEntry name )
{
    com_lines = 0;
    Com_sprintf( com_parsename, sizeof( com_parsename ), "%s", name );
}

void COM_BackupParseSession( UTF8** data_p )
{
    backup_lines = com_lines;
    backup_text = *data_p;
}

void COM_RestoreParseSession( UTF8** data_p )
{
    com_lines = backup_lines;
    *data_p = backup_text;
}

void COM_SetCurrentParseLine( S32 line )
{
    com_lines = line;
}

S32 COM_GetCurrentParseLine( void )
{
    return com_lines;
}

UTF8* COM_Parse( UTF8** data_p )
{
    return COM_ParseExt( data_p, true );
}

void COM_ParseError( UTF8* format, ... )
{
    va_list argptr;
    static UTF8 string[4096];
    
    va_start( argptr, format );
    Q_vsnprintf( string, sizeof( string ), format, argptr );
    va_end( argptr );
    
    Com_Printf( S_COLOR_RED "ERROR: %s, line %d: %s\n", com_parsename, com_lines, string );
}

void COM_ParseWarning( UTF8* format, ... )
{
    va_list argptr;
    static UTF8 string[4096];
    
    va_start( argptr, format );
    Q_vsnprintf( string, sizeof( string ), format, argptr );
    va_end( argptr );
    
    Com_Printf( "WARNING: %s, line %d: %s\n", com_parsename, com_lines, string );
}

/*
==============
COM_Parse

Parse a token out of a string
Will never return nullptr, just empty strings

If "allowLineBreaks" is true then an empty
string will be returned if the next token is
a newline.
==============
*/
static UTF8* SkipWhitespace( UTF8* data, bool* hasNewLines )
{
    S32 c;
    
    while( ( c = *data ) <= ' ' )
    {
        if( !c )
        {
            return nullptr;
        }
        if( c == '\n' )
        {
            com_lines++;
            *hasNewLines = true;
        }
        data++;
    }
    
    return data;
}

S32 COM_Compress( UTF8* data_p )
{
    UTF8* datai, *datao;
    S32 c, size;
    bool ws = false;
    
    size = 0;
    datai = datao = data_p;
    if( datai )
    {
        while( ( c = *datai ) != 0 )
        {
            if( c == 13 || c == 10 )
            {
                *datao = c;
                datao++;
                ws = false;
                datai++;
                size++;
                // skip double slash comments
            }
            else if( c == '/' && datai[1] == '/' )
            {
                while( *datai && *datai != '\n' )
                {
                    datai++;
                }
                ws = false;
                // skip /* */ comments
            }
            else if( c == '/' && datai[1] == '*' )
            {
                datai += 2; // Arnout: skip over '/*'
                while( *datai && ( *datai != '*' || datai[1] != '/' ) )
                {
                    datai++;
                }
                if( *datai )
                {
                    datai += 2;
                }
                ws = false;
            }
            else
            {
                if( ws )
                {
                    *datao = ' ';
                    datao++;
                }
                *datao = c;
                datao++;
                datai++;
                ws = false;
                size++;
            }
        }
        
        *datao = 0;
    }
    return size;
}

UTF8* COM_ParseExt( UTF8** data_p, bool allowLineBreaks )
{
    S32 c = 0, len;
    bool hasNewLines = false;
    UTF8* data;
    
    data = *data_p;
    len = 0;
    com_token[0] = 0;
    
    // make sure incoming data is valid
    if( !data )
    {
        *data_p = nullptr;
        return com_token;
    }
    
    // RF, backup the session data so we can unget easily
    COM_BackupParseSession( data_p );
    
    while( 1 )
    {
        // skip whitespace
        data = SkipWhitespace( data, &hasNewLines );
        if( !data )
        {
            *data_p = nullptr;
            return com_token;
        }
        if( hasNewLines && !allowLineBreaks )
        {
            *data_p = data;
            return com_token;
        }
        
        c = *data;
        
        // skip double slash comments
        if( c == '/' && data[1] == '/' )
        {
            data += 2;
            while( *data && *data != '\n' )
            {
                data++;
            }
//			com_lines++;
        }
        // skip /* */ comments
        else if( c == '/' && data[1] == '*' )
        {
            data += 2;
            while( *data && ( *data != '*' || data[1] != '/' ) )
            {
                data++;
                if( *data == '\n' )
                {
//					com_lines++;
                }
            }
            if( *data )
            {
                data += 2;
            }
        }
        else
        {
            break;
        }
    }
    
    // handle quoted strings
    if( c == '\"' )
    {
        data++;
        while( 1 )
        {
            c = *data++;
            if( c == '\\' && *( data ) == '\"' )
            {
                // Arnout: string-in-string
                if( len < MAX_TOKEN_CHARS )
                {
                    com_token[len] = '\"';
                    len++;
                }
                data++;
                
                while( 1 )
                {
                    c = *data++;
                    
                    if( !c )
                    {
                        com_token[len] = 0;
                        *data_p = ( UTF8* ) data;
                        break;
                    }
                    if( ( c == '\\' && *( data ) == '\"' ) )
                    {
                        if( len < MAX_TOKEN_CHARS )
                        {
                            com_token[len] = '\"';
                            len++;
                        }
                        data++;
                        c = *data++;
                        break;
                    }
                    if( len < MAX_TOKEN_CHARS )
                    {
                        com_token[len] = c;
                        len++;
                    }
                }
            }
            if( c == '\"' || !c )
            {
                com_token[len] = 0;
                *data_p = ( UTF8* ) data;
                return com_token;
            }
            if( len < MAX_TOKEN_CHARS )
            {
                com_token[len] = c;
                len++;
            }
        }
    }
    
    // parse a regular word
    do
    {
        if( len < MAX_TOKEN_CHARS )
        {
            com_token[len] = c;
            len++;
        }
        data++;
        c = *data;
        if( c == '\n' )
        {
            com_lines++;
        }
    }
    while( c > 32 );
    
    if( len == MAX_TOKEN_CHARS )
    {
//		Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
        len = 0;
    }
    com_token[len] = 0;
    
    *data_p = ( UTF8* ) data;
    return com_token;
}


UTF8*           COM_Parse2( UTF8** data_p )
{
    return COM_ParseExt2( data_p, true );
}


// *INDENT-OFF*
UTF8*           COM_ParseExt2( UTF8** data_p, bool allowLineBreaks )
{
    S32             c = 0, len;
    bool        hasNewLines = false;
    UTF8*           data;
    StringEntry*    punc;
    
    if( !data_p )
    {
        Com_Error( ERR_FATAL, "COM_ParseExt: nullptr data_p" );
    }
    
    data = *data_p;
    len = 0;
    com_token[0] = 0;
    
    // make sure incoming data is valid
    if( !data )
    {
        *data_p = nullptr;
        return com_token;
    }
    
    // RF, backup the session data so we can unget easily
    COM_BackupParseSession( data_p );
    
    // skip whitespace
    while( 1 )
    {
        data = SkipWhitespace( data, &hasNewLines );
        if( !data )
        {
            *data_p = nullptr;
            return com_token;
        }
        if( hasNewLines && !allowLineBreaks )
        {
            *data_p = data;
            return com_token;
        }
        
        c = *data;
        
        // skip double slash comments
        if( c == '/' && data[1] == '/' )
        {
            data += 2;
            while( *data && *data != '\n' )
            {
                data++;
            }
        }
        // skip /* */ comments
        else if( c == '/' && data[1] == '*' )
        {
            data += 2;
            while( *data && ( *data != '*' || data[1] != '/' ) )
            {
                data++;
            }
            if( *data )
            {
                data += 2;
            }
        }
        else
        {
            // a real token to parse
            break;
        }
    }
    
    // handle quoted strings
    if( c == '\"' )
    {
        data++;
        while( 1 )
        {
            c = *data++;
            
            if( ( c == '\\' ) && ( *data == '\"' ) )
            {
                // allow quoted strings to use \" to indicate the " character
                data++;
            }
            else if( c == '\"' || !c )
            {
                com_token[len] = 0;
                *data_p = ( UTF8* )data;
                return com_token;
            }
            else if( *data == '\n' )
            {
                com_lines++;
            }
            
            if( len < MAX_TOKEN_CHARS - 1 )
            {
                com_token[len] = c;
                len++;
            }
        }
    }
    
    // check for a number
    // is this parsing of negative numbers going to cause expression problems
    if(	( c >= '0' && c <= '9' ) ||
            ( c == '-' && data[1] >= '0' && data[1] <= '9' ) ||
            ( c == '.' && data[1] >= '0' && data[1] <= '9' ) ||
            ( c == '-' && data[1] == '.' && data[2] >= '0' && data[2] <= '9' ) )
    {
        do
        {
            if( len < MAX_TOKEN_CHARS - 1 )
            {
                com_token[len] = c;
                len++;
            }
            data++;
            
            c = *data;
        }
        while( ( c >= '0' && c <= '9' ) || c == '.' );
        
        // parse the exponent
        if( c == 'e' || c == 'E' )
        {
            if( len < MAX_TOKEN_CHARS - 1 )
            {
                com_token[len] = c;
                len++;
            }
            data++;
            c = *data;
            
            if( c == '-' || c == '+' )
            {
                if( len < MAX_TOKEN_CHARS - 1 )
                {
                    com_token[len] = c;
                    len++;
                }
                data++;
                c = *data;
            }
            
            do
            {
                if( len < MAX_TOKEN_CHARS - 1 )
                {
                    com_token[len] = c;
                    len++;
                }
                data++;
                
                c = *data;
            }
            while( c >= '0' && c <= '9' );
        }
        
        if( len == MAX_TOKEN_CHARS )
        {
            len = 0;
        }
        com_token[len] = 0;
        
        *data_p = ( UTF8* )data;
        return com_token;
    }
    
    // check for a regular word
    // we still allow forward and back slashes in name tokens for pathnames
    // and also colons for drive letters
    if(	( c >= 'a' && c <= 'z' ) ||
            ( c >= 'A' && c <= 'Z' ) ||
            ( c == '_' ) ||
            ( c == '/' ) ||
            ( c == '\\' ) ||
            ( c == '$' ) || ( c == '*' ) ) // Tr3B - for bad shader strings
    {
        do
        {
            if( len < MAX_TOKEN_CHARS - 1 )
            {
                com_token[len] = c;
                len++;
            }
            data++;
            
            c = *data;
        }
        while
        ( ( c >= 'a' && c <= 'z' ) ||
                ( c >= 'A' && c <= 'Z' ) ||
                ( c == '_' ) ||
                ( c == '-' ) ||
                ( c >= '0' && c <= '9' ) ||
                ( c == '/' ) ||
                ( c == '\\' ) ||
                ( c == ':' ) ||
                ( c == '.' ) ||
                ( c == '$' ) ||
                ( c == '*' ) ||
                ( c == '@' ) );
                
        if( len == MAX_TOKEN_CHARS )
        {
            len = 0;
        }
        com_token[len] = 0;
        
        *data_p = ( UTF8* )data;
        return com_token;
    }
    
    // check for multi-character punctuation token
    for( punc = punctuation; *punc; punc++ )
    {
        S32             l;
        S32             j;
        
        l = strlen( *punc );
        for( j = 0; j < l; j++ )
        {
            if( data[j] != ( *punc )[j] )
            {
                break;
            }
        }
        if( j == l )
        {
            // a valid multi-character punctuation
            ::memcpy( com_token, *punc, l );
            com_token[l] = 0;
            data += l;
            *data_p = ( UTF8* )data;
            return com_token;
        }
    }
    
    // single character punctuation
    com_token[0] = *data;
    com_token[1] = 0;
    data++;
    *data_p = ( UTF8* )data;
    
    return com_token;
}
// *INDENT-ON*


/*
==================
COM_MatchToken
==================
*/
void COM_MatchToken( UTF8** buf_p, UTF8* match )
{
    UTF8*    token;
    
    token = COM_Parse( buf_p );
    if( strcmp( token, match ) )
    {
        Com_Error( ERR_DROP, "MatchToken: %s != %s", token, match );
    }
}

/*
=================
SkipBracedSection_Depth

=================
*/
bool SkipBracedSection_Depth( UTF8** program, S32 depth )
{
    UTF8* token;
    
    do
    {
        token = COM_ParseExt( program, true );
        if( token[1] == 0 )
        {
            if( token[0] == '{' )
            {
                depth++;
            }
            else if( token[0] == '}' )
            {
                depth--;
            }
        }
    }
    while( depth && *program );
    
    return ( bool )( depth == 0 );
}

/*
=================
SkipBracedSection

The next token should be an open brace.
Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
bool SkipBracedSection( UTF8** program )
{
    UTF8*            token;
    S32 depth;
    
    depth = 0;
    do
    {
        token = COM_ParseExt( program, true );
        if( token[1] == 0 )
        {
            if( token[0] == '{' )
            {
                depth++;
            }
            else if( token[0] == '}' )
            {
                depth--;
            }
        }
    }
    while( depth && *program );
    
    return ( bool )( depth == 0 );
}

/*
=================
SkipRestOfLine
=================
*/
void SkipRestOfLine( UTF8** data )
{
    UTF8*    p;
    S32 c;
    
    p = *data;
    while( ( c = *p++ ) != 0 )
    {
        if( c == '\n' )
        {
            com_lines++;
            break;
        }
    }
    
    *data = p;
}

/*
===============
COM_Parse2Infos
===============
*/
S32 COM_Parse2Infos( UTF8* buf, S32 max, UTF8 infos[][MAX_INFO_STRING] )
{
    StringEntry  token;
    S32 count;
    UTF8 key[MAX_TOKEN_CHARS];
    
    count = 0;
    
    while( 1 )
    {
        token = COM_Parse( &buf );
        if( !token[0] )
        {
            break;
        }
        if( strcmp( token, "{" ) )
        {
            Com_Printf( "Missing { in info file\n" );
            break;
        }
        
        if( count == max )
        {
            Com_Printf( "Max infos exceeded\n" );
            break;
        }
        
        infos[count][0] = 0;
        while( 1 )
        {
            token = COM_Parse( &buf );
            if( !token[0] )
            {
                Com_Printf( "Unexpected end of info file\n" );
                break;
            }
            if( !strcmp( token, "}" ) )
            {
                break;
            }
            Q_strncpyz( key, token, sizeof( key ) );
            
            token = COM_ParseExt( &buf, false );
            if( !token[0] )
            {
                token = "<nullptr>";
            }
            Info_SetValueForKey( infos[count], key, token );
        }
        count++;
    }
    
    return count;
}

void COM_Parse21DMatrix( UTF8** buf_p, S32 x, F32* m, bool checkBrackets )
{
    UTF8*           token;
    S32             i;
    
    if( checkBrackets )
    {
        COM_MatchToken( buf_p, "(" );
    }
    
    for( i = 0; i < x; i++ )
    {
        token = COM_Parse2( buf_p );
        m[i] = atof( token );
    }
    
    if( checkBrackets )
    {
        COM_MatchToken( buf_p, ")" );
    }
}

void COM_Parse22DMatrix( UTF8** buf_p, S32 y, S32 x, F32* m )
{
    S32             i;
    
    COM_MatchToken( buf_p, "(" );
    
    for( i = 0; i < y; i++ )
    {
        COM_Parse21DMatrix( buf_p, x, m + i * x, true );
    }
    
    COM_MatchToken( buf_p, ")" );
}

void COM_Parse23DMatrix( UTF8** buf_p, S32 z, S32 y, S32 x, F32* m )
{
    S32             i;
    
    COM_MatchToken( buf_p, "(" );
    
    for( i = 0; i < z; i++ )
    {
        COM_Parse22DMatrix( buf_p, y, x, m + i * x * y );
    }
    
    COM_MatchToken( buf_p, ")" );
}

/*
===================
Com_HexStrToInt
===================
*/
S32 Com_HexStrToInt( StringEntry str )
{
    if( !str || !str[ 0 ] )
        return -1;
        
    // check for hex code
    if( str[ 0 ] == '0' && str[ 1 ] == 'x' )
    {
        S32 i, n = 0;
        
        for( i = 2; i < strlen( str ); i++ )
        {
            UTF8 digit;
            
            n *= 16;
            
            digit = tolower( str[ i ] );
            
            if( digit >= '0' && digit <= '9' )
                digit -= '0';
            else if( digit >= 'a' && digit <= 'f' )
                digit = digit - 'a' + 10;
            else
                return -1;
                
            n += digit;
        }
        
        return n;
    }
    
    return -1;
}

/*
===================
Com_QuoteStr
===================
*/
StringEntry Com_QuoteStr( StringEntry str )
{
    static UTF8* buf = nullptr;
    static U64 buflen = 0;
    
    U64 length;
    UTF8* ptr;
    
    // quick exit if no quoting is needed
//	if (!strpbrk (str, "\";"))
//		return str;

    length = strlen( str );
    if( buflen < 2 * length + 3 )
    {
        free( buf );
        buflen = 2 * length + 3;
        buf = ( UTF8* )malloc( buflen );
    }
    ptr = buf;
    *ptr++ = '"';
    --str;
    while( *++str )
    {
        if( *str == '"' )
            *ptr++ = '\\';
        *ptr++ = *str;
    }
    ptr[0] = '"';
    ptr[1] = 0;
    
    return buf;
}

/*
===================
Com_UnquoteStr
===================
*/
StringEntry Com_UnquoteStr( StringEntry str )
{
    static UTF8* buf = nullptr;
    
    U64 length;
    UTF8* ptr;
    StringEntry end;
    
    end = str + strlen( str );
    
    // Strip trailing spaces
    while( --end >= str )
        if( *end != ' ' )
            break;
    // end points at the last non-space character
    
    // If it doesn't begin with '"', return quickly
    if( *str != '"' )
    {
        length = end + 1 - str;
        free( buf );
        buf = ( UTF8* )malloc( length + 1 );
        strncpy( buf, str, length );
        buf[length] = 0;
        return buf;
    }
    
    // It begins with '"'; if it ends with '"', lose that '"'
    if( end > str && *end == '"' )
        --end;
        
    free( buf );
    buf = ( UTF8* )malloc( end + 1 - str );
    ptr = buf;
    
    // Copy, unquoting as we go
    // str[0] == '"', so that gets skipped
    while( ++str <= end )
    {
        if( str[0] == '\\' && str[1] == '"' && str < end ) // FIXME: \ semantics are broken
            ++str;
        *ptr++ = *str;
    }
    *ptr = 0;
    
    return buf;
}


/*
============================================================================

					LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

S32 Q_isprint( S32 c )
{
    if( c >= 0x20 && c <= 0x7E )
    {
        return ( 1 );
    }
    return ( 0 );
}

S32 Q_islower( S32 c )
{
    if( c >= 'a' && c <= 'z' )
    {
        return ( 1 );
    }
    return ( 0 );
}

S32 Q_isupper( S32 c )
{
    if( c >= 'A' && c <= 'Z' )
    {
        return ( 1 );
    }
    return ( 0 );
}

S32 Q_isalpha( S32 c )
{
    if( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) )
    {
        return ( 1 );
    }
    return ( 0 );
}

S32 Q_isnumeric( S32 c )
{
    if( c >= '0' && c <= '9' )
    {
        return ( 1 );
    }
    return ( 0 );
}

S32 Q_isalphanumeric( S32 c )
{
    if( Q_isalpha( c ) ||
            Q_isnumeric( c ) )
    {
        return( 1 );
    }
    return ( 0 );
}

S32 Q_isforfilename( S32 c )
{
    if( ( Q_isalphanumeric( c ) || c == '_' ) && c != ' ' )    // space not allowed in filename
    {
        return( 1 );
    }
    return ( 0 );
}

UTF8* Q_strrchr( StringEntry string, S32 c )
{
    UTF8 cc = c;
    UTF8* s;
    UTF8* sp = ( UTF8* )0;
    
    s = ( UTF8* )string;
    
    while( *s )
    {
        if( *s == cc )
        {
            sp = s;
        }
        s++;
    }
    if( cc == 0 )
    {
        sp = s;
    }
    
    return sp;
}

/*
=============
Q_strtoi/l

Takes a null-terminated string (which represents either a F32 or integer
conforming to strtod) and an integer to assign to (if successful).

Returns true on success and vice versa.
Demonstration of behavior of strtod and conversions: http://codepad.org/YQKxV94R
-============
*/
bool Q_strtol( StringEntry s, S64* outNum )
{
    UTF8* p;
    
    if( *s == '\0' )
    {
        return false;
    }
    
    *outNum = strtod( s, &p );
    
    return ( bool )( *p == '\0' );
}

bool Q_strtoi( StringEntry s, S32* outNum )
{
    UTF8* p;
    if( *s == '\0' )
    {
        return false;
    }
    
    *outNum = strtod( s, &p );
    
    return ( bool )( *p == '\0' );
}

/*
=============
Q_strncpyz

Safe strncpy that ensures a trailing zero
=============
*/

// Dushan
#if defined(_DEBUG)
void Q_strncpyzDebug( UTF8* dest, StringEntry src, U64 destsize, StringEntry file, S32 line )
#else
void Q_strncpyz( UTF8* dest, StringEntry src, S32 destsize )
#endif
{
    UTF8* d;
    StringEntry s;
    U64 n;
    
#ifdef _DEBUG
    if( !dest )
    {
        Com_Error( ERR_DROP, "Q_strncpyz: nullptr dest (%s, %i)", file, line );
    }
    if( !src )
    {
        Com_Error( ERR_DROP, "Q_strncpyz: nullptr src (%s, %i)", file, line );
    }
    if( destsize < 1 )
    {
        Com_Error( ERR_DROP, "Q_strncpyz: destsize < 1 (%s, %i)", file, line );
    }
#else
    
    if( !dest )
    {
        Com_Error( ERR_FATAL, "Q_strncpyz: nullptr dest" );
    }
    
    if( !src )
    {
        Com_Error( ERR_FATAL, "Q_strncpyz: nullptr src" );
    }
    if( destsize < 1 )
    {
        Com_Error( ERR_FATAL, "Q_strncpyz: destsize < 1" );
    }
#endif
    
    d = dest;
    s = src;
    n = destsize;
    
    /* Copy as many bytes as will fit */
    if( n != 0 )
    {
        while( --n != 0 )
        {
            if( ( *d++ = *s++ ) == '\0' )
                break;
        }
    }
    
    /* Not enough room in dst, add nullptr and traverse rest of src */
    if( n == 0 )
    {
        if( destsize != 0 )
            *d = '\0';
        /* NUL-terminate dst */
        while( *s++ );
    }
}

S32 Q_stricmpn( StringEntry s1, StringEntry s2, S32 n )
{
    S32 c1, c2;
    
    do
    {
        c1 = *s1++;
        c2 = *s2++;
        
        if( !n-- )
        {
            return 0;       // strings are equal until end point
        }
        
        if( c1 != c2 )
        {
            if( c1 >= 'a' && c1 <= 'z' )
            {
                c1 -= ( 'a' - 'A' );
            }
            if( c2 >= 'a' && c2 <= 'z' )
            {
                c2 -= ( 'a' - 'A' );
            }
            if( c1 != c2 )
            {
                return c1 < c2 ? -1 : 1;
            }
        }
    }
    while( c1 );
    
    return 0;       // strings are equal
}

S32 Q_strncmp( StringEntry s1, StringEntry s2, S32 n )
{
    S32 c1, c2;
    
    do
    {
        c1 = *s1++;
        c2 = *s2++;
        
        if( !n-- )
        {
            return 0;       // strings are equal until end point
        }
        
        if( c1 != c2 )
        {
            return c1 < c2 ? -1 : 1;
        }
    }
    while( c1 );
    
    return 0;       // strings are equal
}

#ifndef Q3MAP2
S32 Q_stricmp( StringEntry s1, StringEntry s2 )
{
    return ( s1 && s2 ) ? Q_stricmpn( s1, s2, 99999 ) : -1;
}
#endif

UTF8* Q_strlwr( UTF8* s1 )
{
    UTF8*   s;
    
    for( s = s1; *s; ++s )
    {
        if( ( 'A' <= *s ) && ( *s <= 'Z' ) )
        {
            *s -= 'A' - 'a';
        }
    }
    
    return s1;
}

UTF8* Q_strupr( UTF8* s1 )
{
    UTF8* cp;
    
    for( cp = s1 ; *cp ; ++cp )
    {
        if( ( 'a' <= *cp ) && ( *cp <= 'z' ) )
        {
            *cp += 'A' - 'a';
        }
    }
    
    return s1;
}


// never goes past bounds or leaves without a terminating 0
void Q_strcat( UTF8* dest, S32 size, StringEntry src )
{
    S32 l1;
    
    l1 = strlen( dest );
    if( l1 >= size )
    {
        Com_Error( ERR_FATAL, "Q_strcat: already overflowed" );
    }
    Q_strncpyz( dest + l1, src, size - l1 );
}


S32 Q_strnicmp( StringEntry string1, StringEntry string2, S32 n )
{
    S32 c1, c2;
    
    if( string1 == nullptr )
    {
        if( string2 == nullptr )
            return 0;
        else
            return -1;
    }
    else if( string2 == nullptr )
        return 1;
        
    do
    {
        c1 = *string1++;
        c2 = *string2++;
        
        if( !n-- )
            return 0;// Strings are equal until end point
            
        if( c1 != c2 )
        {
            if( c1 >= 'a' && c1 <= 'z' )
                c1 -= ( 'a' - 'A' );
            if( c2 >= 'a' && c2 <= 'z' )
                c2 -= ( 'a' - 'A' );
                
            if( c1 != c2 )
                return c1 < c2 ? -1 : 1;
        }
    }
    while( c1 );
    
    return 0;// Strings are equal
}

/*
* Find the first occurrence of find in s.
*/
StringEntry Q_stristr( StringEntry s, StringEntry find )
{
    UTF8 c, sc;
    U64 len;
    
    if( ( c = *find++ ) != 0 )
    {
        if( c >= 'a' && c <= 'z' )
        {
            c -= ( 'a' - 'A' );
        }
        len = strlen( find );
        do
        {
            do
            {
                if( ( sc = *s++ ) == 0 )
                    return nullptr;
                if( sc >= 'a' && sc <= 'z' )
                {
                    sc -= ( 'a' - 'A' );
                }
            }
            while( sc != c );
        }
        while( Q_stricmpn( s, find, len ) != 0 );
        s--;
    }
    return s;
}


/*
=============
Q_strreplace

replaces content of find by replace in dest
=============
*/
bool Q_strreplace( UTF8* dest, S32 destsize, StringEntry find, StringEntry replace )
{
    S32             lstart, lfind, lreplace, lend;
    UTF8*           s;
    UTF8            backup[32000];	// big, but small enough to fit in PPC stack
    
    lend = strlen( dest );
    if( lend >= destsize )
    {
        Com_Error( ERR_FATAL, "Q_strreplace: already overflowed" );
    }
    
    s = strstr( dest, find );
    if( !s )
    {
        return false;
    }
    else
    {
        Q_strncpyz( backup, dest, lend + 1 );
        lstart = s - dest;
        lfind = strlen( find );
        lreplace = strlen( replace );
        
        strncpy( s, replace, destsize - lstart - 1 );
        strncpy( s + lreplace, backup + lstart + lfind, destsize - lstart - lreplace - 1 );
        
        return true;
    }
}


S32 Q_PrintStrlen( StringEntry string )
{
    S32 len;
    StringEntry  p;
    
    if( !string )
    {
        return 0;
    }
    
    len = 0;
    p = string;
    while( *p )
    {
        if( Q_IsColorString( p ) )
        {
            p += 2;
            continue;
        }
        if( *p == Q_COLOR_ESCAPE && *( p + 1 ) == Q_COLOR_ESCAPE )
        {
            p++;
        }
        p++;
        len++;
    }
    
    return len;
}


UTF8* Q_CleanStr( UTF8* string )
{
    UTF8*   d;
    UTF8*   s;
    
    s = string;
    d = string;
    while( *s )
    {
        if( Q_IsColorString( s ) )
        {
            s += 2;
            continue;
        }
        
        if( *s == Q_COLOR_ESCAPE && *( s + 1 ) == Q_COLOR_ESCAPE )
        {
            s++;
        }
        
        if( *s >= 0x20 && *s <= 0x7E )
        {
            *d++ = *s;
        }
        s++;
    }
    *d = '\0';
    
    
    return string;
}

// strips whitespaces and bad characters
bool Q_isBadDirChar( UTF8 c )
{
    UTF8 badchars[] = { ';', '&', '(', ')', '|', '<', '>', '*', '?', '[', ']', '~', '+', '@', '!', '\\', '/', ' ', '\'', '\"', '\0' };
    S32 i;
    
    for( i = 0; badchars[i] != '\0'; i++ )
    {
        if( c == badchars[i] )
        {
            return true;
        }
    }
    
    return false;
}

UTF8* Q_CleanDirName( UTF8* dirname )
{
    UTF8*   d;
    UTF8*   s;
    
    s = dirname;
    d = dirname;
    
    // clear trailing .'s
    while( *s == '.' )
    {
        s++;
    }
    
    while( *s != '\0' )
    {
        if( !Q_isBadDirChar( *s ) )
        {
            *d++ = *s;
        }
        s++;
    }
    *d = '\0';
    
    return dirname;
}

S32 Q_CountChar( StringEntry string, UTF8 tocount )
{
    S32 count;
    
    for( count = 0; *string; string++ )
    {
        if( *string == tocount )
            count++;
    }
    
    return count;
}

S32 Com_sprintf( UTF8* dest, S32 size, StringEntry fmt, ... )
{
    S32 len;
    va_list argptr;
    
    va_start( argptr, fmt );
    len = Q_vsnprintf( dest, size, fmt, argptr );
    va_end( argptr );
    
    // Dushan
    if( len >= size )
    {
        Com_Printf( "Com_sprintf: Output length %d too short, require %d bytes.\n", size, len + 1 );
    }
    
    if( len == -1 )
    {
        Com_Printf( "Com_sprintf: overflow of %i bytes buffer\n", size );
    }
    
    return len;
}

/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.

Ridah, modified this into a circular list, to further prevent stepping on
previous strings
============
*/
UTF8* va( StringEntry format, ... )
{
    va_list argptr;
#define MAX_VA_STRING   32000
    static UTF8 temp_buffer[MAX_VA_STRING];
    static UTF8 string[MAX_VA_STRING];      // in case va is called by nested functions
    static S32 index = 0;
    UTF8* buf;
    S32 len;
    
    va_start( argptr, format );
#ifdef _WIN32
    vsprintf_s( temp_buffer, MAX_VA_STRING - 1, format, argptr );
#else
    vsnprintf( temp_buffer, MAX_VA_STRING - 1, format, argptr );
#endif
    va_end( argptr );
    
    if( ( len = strlen( temp_buffer ) ) >= MAX_VA_STRING )
    {
        Com_Error( ERR_DROP, "Attempted to overrun string in call to va()\n" );
    }
    
    if( len + index >= MAX_VA_STRING - 1 )
    {
        index = 0;
    }
    
    buf = &string[index];
    memcpy( buf, temp_buffer, len + 1 );
    
    index += len + 1;
    
    return buf;
}

/*
=====================================================================

  INFO STRINGS

=====================================================================
*/

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
FIXME: overflow check?
===============
*/
UTF8* Info_ValueForKey( StringEntry s, StringEntry key )
{
    UTF8 pkey[BIG_INFO_KEY];
    static UTF8 value[2][BIG_INFO_VALUE];   // use two buffers so compares
    // work without stomping on each other
    static S32 valueindex = 0;
    UTF8*    o;
    
    if( !s || !key )
    {
        return "";
    }
    
    if( strlen( s ) >= BIG_INFO_STRING )
    {
        Com_Error( ERR_DROP, "Info_ValueForKey: oversize infostring [%s] [%s]", s, key );
    }
    
    valueindex ^= 1;
    
    if( *s == '\\' )
    {
        s++;
    }
    
    while( 1 )
    {
        o = pkey;
        while( *s != '\\' )
        {
            if( !*s )
            {
                return "";
            }
            *o++ = *s++;
        }
        *o = 0;
        s++;
        
        o = value[valueindex];
        
        while( *s != '\\' && *s )
        {
            *o++ = *s++;
        }
        *o = 0;
        
        if( !Q_stricmp( key, pkey ) )
        {
            return value[valueindex];
        }
        
        if( !*s )
        {
            break;
        }
        s++;
    }
    
    return "";
}


/*
===================
Info_NextPair

Used to itterate through all the key/value pairs in an info string
===================
*/
void Info_NextPair( StringEntry* head, UTF8* key, UTF8* value )
{
    UTF8*    o;
    StringEntry  s;
    
    s = *head;
    
    if( *s == '\\' )
    {
        s++;
    }
    key[0] = 0;
    value[0] = 0;
    
    o = key;
    while( *s != '\\' )
    {
        if( !*s )
        {
            *o = 0;
            *head = s;
            return;
        }
        *o++ = *s++;
    }
    *o = 0;
    s++;
    
    o = value;
    while( *s != '\\' && *s )
    {
        *o++ = *s++;
    }
    *o = 0;
    
    *head = s;
}


/*
===================
Info_RemoveKey
===================
*/
void Info_RemoveKey( UTF8* s, StringEntry key )
{
    UTF8*    start;
    UTF8 pkey[MAX_INFO_KEY];
    UTF8 value[MAX_INFO_VALUE];
    UTF8*    o;
    
    if( strlen( s ) >= MAX_INFO_STRING )
    {
        Com_Error( ERR_DROP, "Info_RemoveKey: oversize infostring [%s] [%s]", s, key );
    }
    
    if( strchr( key, '\\' ) )
    {
        return;
    }
    
    while( 1 )
    {
        start = s;
        if( *s == '\\' )
        {
            s++;
        }
        o = pkey;
        while( *s != '\\' )
        {
            if( !*s )
            {
                return;
            }
            *o++ = *s++;
        }
        *o = 0;
        s++;
        
        o = value;
        while( *s != '\\' && *s )
        {
            if( !*s )
            {
                return;
            }
            *o++ = *s++;
        }
        *o = 0;
        
        if( !Q_stricmp( key, pkey ) )
        {
            // rain - arguments to strcpy must not overlap
            //strcpy (start, s);	// remove this part
            memmove( start, s, strlen( s ) + 1 ); // remove this part
            return;
        }
        
        if( !*s )
        {
            return;
        }
    }
    
}

/*
===================
Info_RemoveKey_Big
===================
*/
void Info_RemoveKey_Big( UTF8* s, StringEntry key )
{
    UTF8*    start;
    UTF8 pkey[BIG_INFO_KEY];
    UTF8 value[BIG_INFO_VALUE];
    UTF8*    o;
    
    if( strlen( s ) >= BIG_INFO_STRING )
    {
        Com_Error( ERR_DROP, "Info_RemoveKey_Big: oversize infostring [%s] [%s]", s, key );
    }
    
    if( strchr( key, '\\' ) )
    {
        return;
    }
    
    while( 1 )
    {
        start = s;
        if( *s == '\\' )
        {
            s++;
        }
        o = pkey;
        while( *s != '\\' )
        {
            if( !*s )
            {
                return;
            }
            *o++ = *s++;
        }
        *o = 0;
        s++;
        
        o = value;
        while( *s != '\\' && *s )
        {
            if( !*s )
            {
                return;
            }
            *o++ = *s++;
        }
        *o = 0;
        
        if( !Q_stricmp( key, pkey ) )
        {
            strcpy( start, s );  // remove this part
            return;
        }
        
        if( !*s )
        {
            return;
        }
    }
    
}




/*
==================
Info_Validate

Some characters are illegal in info strings because they
can mess up the server's parsing
==================
*/
bool Info_Validate( StringEntry s )
{
    if( strchr( s, '\"' ) )
    {
        return false;
    }
    if( strchr( s, ';' ) )
    {
        return false;
    }
    return true;
}

/*
==================
Info_SetValueForKey

Changes or adds a key/value pair
==================
*/
bool Info_SetValueForKey( UTF8* s, StringEntry key, StringEntry value )
{
    UTF8	newi[MAX_INFO_STRING], *v;
    S32		c, maxsize = MAX_INFO_STRING;
    
    if( strlen( s ) >= MAX_INFO_STRING )
    {
        Com_Error( ERR_DROP, "SetValueForKey: oversize infostring [%s] [%s] [%s]", s, key, value );
    }
    
    if( strchr( key, '\\' ) || strchr( value, '\\' ) )
    {
        Com_Printf( "SetValueForKey: Can't use keys or values with a \\\n" );
        return false;
    }
    
    if( strchr( key, ';' ) || strchr( value, ';' ) )
    {
        Com_Printf( "SetValueForKey: Can't use keys or values with a semicolon\n" );
        return false;
    }
    
    if( strlen( key ) > MAX_INFO_KEY - 1 || strlen( value ) > MAX_INFO_KEY - 1 )
    {
        Com_Error( ERR_DROP, "SetValueForKey: keys and values must be < %i characters.\n", MAX_INFO_KEY );
        return false;
    }
    
    Info_RemoveKey( s, key );
    if( !value || !strlen( value ) )
    {
        return true; // just clear variable
    }
    
    Com_sprintf( newi, sizeof( newi ), "\\%s\\%s", key, value );
    
    if( strlen( newi ) + strlen( s ) > maxsize )
    {
        Com_Printf( "SetValueForKey: Info string length exceeded: %s\n", s );
        return true;
    }
    
    // only copy ascii values
    s += strlen( s );
    v = newi;
    
    while( *v )
    {
        c = *v++;
        c &= 255;	// strip high bits
        if( c >= 32 && c <= 255 )
            *s++ = c;
    }
    *s = 0;
    
    // all done
    return true;
}

/*
==================
Info_SetValueForKey_Big

Changes or adds a key/value pair
==================
*/
void Info_SetValueForKey_Big( UTF8* s, StringEntry key, StringEntry value )
{
    UTF8 newi[BIG_INFO_STRING];
    
    if( strlen( s ) >= BIG_INFO_STRING )
    {
        Com_Error( ERR_DROP, "Info_SetValueForKey: oversize infostring [%s] [%s] [%s]", s, key, value );
    }
    
    if( strchr( key, '\\' ) || strchr( value, '\\' ) )
    {
        Com_Printf( "Can't use keys or values with a \\\n" );
        return;
    }
    
    if( strchr( key, ';' ) || strchr( value, ';' ) )
    {
        Com_Printf( "Can't use keys or values with a semicolon\n" );
        return;
    }
    
    if( strchr( key, '\"' ) || strchr( value, '\"' ) )
    {
        Com_Printf( "Can't use keys or values with a \"\n" );
        return;
    }
    
    Info_RemoveKey_Big( s, key );
    if( !value || !strlen( value ) )
    {
        return;
    }
    
    Com_sprintf( newi, sizeof( newi ), "\\%s\\%s", key, value );
    
    if( strlen( newi ) + strlen( s ) > BIG_INFO_STRING )
    {
        Com_Printf( "Info_SetValueForKey_Big: Info string length exceeded: %s\n", s );
        return;
    }
    
    strcat( s, newi );
}

/*
============
Com_ClientListContains
============
*/
bool Com_ClientListContains( const clientList_t* list, S32 clientNum )
{
    if( clientNum < 0 || clientNum >= MAX_CLIENTS || !list )
        return false;
    if( clientNum < 32 )
        return ( bool )( ( list->lo & ( 1 << clientNum ) ) != 0 );
    else
        return ( bool )( ( list->hi & ( 1 << ( clientNum - 32 ) ) ) != 0 );
}

/*
============
Com_ClientListString
============
*/
UTF8* Com_ClientListString( const clientList_t* list )
{
    static UTF8 s[ 17 ];
    
    s[ 0 ] = '\0';
    if( !list )
        return s;
    Com_sprintf( s, sizeof( s ), "%08x%08x", list->hi, list->lo );
    return s;
}

/*
============
Com_ClientListParse
============
*/
void Com_ClientListParse( clientList_t* list, StringEntry s )
{
    if( !list )
        return;
    list->lo = 0;
    list->hi = 0;
    if( !s )
        return;
    if( strlen( s ) != 16 )
        return;
    sscanf( s, "%x%x", &list->hi, &list->lo );
}

/*
================
VectorMatrixMultiply
================
*/
void VectorMatrixMultiply( const vec3_t p, vec3_t m[ 3 ], vec3_t out )
{
    out[ 0 ] = m[ 0 ][ 0 ] * p[ 0 ] + m[ 1 ][ 0 ] * p[ 1 ] + m[ 2 ][ 0 ] * p[ 2 ];
    out[ 1 ] = m[ 0 ][ 1 ] * p[ 0 ] + m[ 1 ][ 1 ] * p[ 1 ] + m[ 2 ][ 1 ] * p[ 2 ];
    out[ 2 ] = m[ 0 ][ 2 ] * p[ 0 ] + m[ 1 ][ 2 ] * p[ 1 ] + m[ 2 ][ 2 ] * p[ 2 ];
}

//====================================================================

#if defined Q3MAP2 || defined SQL
void Com_Printf( StringEntry msg, ... )
{
    va_list argptr;
    static UTF8 string[4096];
    
    va_start( argptr, msg );
    Q_vsnprintf( string, sizeof( string ), msg, argptr );
    va_end( argptr );
    
    printf( string );
}

void Com_Error( S32 level, StringEntry error, ... )
{
    va_list argptr;
    static UTF8 string[4096];
    
    va_start( argptr, error );
    Q_vsnprintf( string, sizeof( string ), error, argptr );
    va_end( argptr );
    
    printf( string );
    exit( 0 );
}
#endif

#if defined(_MSC_VER)
/*
=============
Q_vsnprintf
Special wrapper function for Microsoft's broken _vsnprintf() function.
MinGW comes with its own snprintf() which is not broken.
=============
*/

S32 Q_vsnprintf( UTF8* str, size_t size, StringEntry format, va_list ap )
{
    S32 retval;
    
    retval = _vsnprintf( str, size, format, ap );
    
    if( retval < 0 || retval == size )
    {
        // Microsoft doesn't adhere to the C99 standard of vsnprintf,
        // which states that the return value must be the number of
        // bytes written if the output string had sufficient length.
        //
        // Obviously we cannot determine that value from Microsoft's
        // implementation, so we have no choice but to return size.
        
        str[size - 1] = '\0';
        return size;
    }
    
    return retval;
}
#endif

#ifndef Q3MAP2
bool StringContainsWord( StringEntry haystack, StringEntry needle )
{
    if( !*needle )
    {
        return false;
    }
    for( ; *haystack; ++haystack )
    {
        if( toupper( *haystack ) == toupper( *needle ) )
        {
            /*
            * Matched starting char -- loop through remaining chars.
            */
            StringEntry h, n;
            for( h = haystack, n = needle; *h && *n; ++h, ++n )
            {
                if( toupper( *h ) != toupper( *n ) )
                {
                    break;
                }
            }
            if( !*n ) /* matched all of 'needle' to null termination */
            {
                return true; /* return the start of the match */
            }
        }
    }
    return false;
}
#endif


/*
============
COM_CompareExtension

string compare the end of the strings and return qtrue if strings match
============
*/
bool COM_CompareExtension( StringEntry in, StringEntry ext )
{
    S32 inlen, extlen;
    
    inlen = strlen( in );
    extlen = strlen( ext );
    
    if( extlen <= inlen )
    {
        in += inlen - extlen;
        
        if( !Q_stricmp( in, ext ) )
        {
            return true;
        }
    }
    
    return false;
}

// Returns a float min <= x < max (exclusive; will get max - 0.00001; but never max)
static U32 holdrand = 0x89abcdef;
#define QRAND_MAX 32768
F32 flrand( F32 min, F32 max )
{
    F32	result;
    
    holdrand = ( holdrand * 214013L ) + 2531011L;
    result = ( F32 )( holdrand >> 17 ); // 0 - 32767 range
    result = ( ( result * ( max - min ) ) / ( F32 )QRAND_MAX ) + min;
    
    return( result );
}

F32 Q_flrand( F32 min, F32 max )
{
    return flrand( min, max );
}

bool Q_CleanPlayerName( StringEntry in, UTF8* out, S32 outSize )
{
    S32 len, i;
    S32 numColorChanges;
    
    numColorChanges = 0;
    
    for( len = 0, i = 0; len < outSize - 1; i++ )
    {
        if( !in[i] )
            break;
            
        if( Q_IsColorString( in + i ) )
        {
            if( in[i + 1] == COLOR_BLACK )
            {
                //don't allow black
                i++;
                continue;
            }
            
            if( len >= outSize - 2 )
                //not enough room
                break;
                
            numColorChanges++;
            
            out[len++] = Q_COLOR_ESCAPE;
            out[len++] = in[i + 1];
            i++;
            
            continue;
        }
        
        if( in[i] == ' ' )
        {
            if( !len )
                //no leading spaces
                continue;
            else if( out[len - 1] == ' ' )
                //don't allow more than once consecutive space
                continue;
        }
        
        out[len++] = in[i];
    }
    
    for( ; ; )
    {
        if( len >= 1 && out[len - 1] == ' ' )
        {
            //don't allow any trailing spaces
            len -= 1;
        }
        if( len >= 2 && Q_IsColorString( out + len - 2 ) )
        {
            //don't allow any trailing color strings
            len -= 2;
            numColorChanges--;
        }
        else
            break;
    }
    
    if( numColorChanges )
    {
        //must append a return to default color at the end of the string
        
        if( len < outSize - 2 )
        {
            //good to append, no adjustments
        }
        else
        {
            if( len < outSize - 1 )
            {
                //we know that the last thing in the string isn't a color
                //sequence so we can freely truncate the last character
                len -= 1;
            }
            else
            {
                //we have to overwrite the last two characters in the string
                
                if( Q_IsColorString( out + len - 3 ) )
                {
                    //second last char is the end of a color string,
                    //replace the whole color string to avoid "^^-"
                    len -= 3;
                }
                else
                {
                    //just chop two characters
                    len -= 2;
                }
            }
        }
        
        //append the "^-" to restore the default color
        out[len++] = Q_COLOR_ESCAPE;
        out[len++] = COLOR_DEFAULT;
    }
    
    out[len] = 0;
    
    if( !len )
        //don't allow empty player names
        Q_strncpyz( out, "UnnamedPlayer", outSize );
        
    return strcmp( in, out ) != 0;
}

UTF8* Com_StringContains( UTF8* str1, UTF8* str2, S32 casesensitive )
{
    S32 len, i, j;
    
    len = strlen( str1 ) - strlen( str2 );
    for( i = 0; i <= len; i++, str1++ )
    {
        for( j = 0; str2[j]; j++ )
        {
            if( casesensitive )
            {
                if( str1[j] != str2[j] )
                {
                    break;
                }
            }
            else
            {
                if( toupper( str1[j] ) != toupper( str2[j] ) )
                {
                    break;
                }
            }
        }
        if( !str2[j] )
        {
            return str1;
        }
    }
    return nullptr;
    
}