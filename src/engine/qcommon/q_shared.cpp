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
// File name:   q_shared.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: stateless support routines that are included in each code dll
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.hpp>
#elif defined DEDICATED
#include <null/null_serverprecompiled.hpp>
#elif defined GUI
#include <GUI/gui_precompiled.hpp>
#elif defined CGAMEDLL
#include <cgame/cgame_precompiled.hpp>
#elif defined GAMEDLL
#include <sgame/sgame_precompiled.hpp>
#elif defined (OALAUDIO)
#include <API/soundSystem_api.hpp>
#else
#include <framework/precompiled.hpp>
#endif

/*
============
Com_Clampi
============
*/
sint Com_Clampi(sint min, sint max, sint value) {
    if(value < min) {
        return min;
    }

    if(value > max) {
        return max;
    }

    return value;
}


/*
============
Com_Clamp
============
*/
float32 Com_Clamp(float32 min, float32 max, float32 value) {
    if(value < min) {
        return min;
    }

    if(value > max) {
        return max;
    }

    return value;
}

/*
==================
Com_CharIsOneOfCharset
==================
*/
static bool Com_CharIsOneOfCharset(valueType c, valueType *set) {
    sint i;

    for(i = 0; i < strlen(set); i++) {
        if(set[ i ] == c) {
            return true;
        }
    }

    return false;
}

/*
==================
Com_SkipCharset
==================
*/
valueType *Com_SkipCharset(valueType *s, valueType *sep) {
    valueType  *p = s;

    while(p) {
        if(Com_CharIsOneOfCharset(*p, sep)) {
            p++;
        } else {
            break;
        }
    }

    return p;
}


/*
==================
Com_SkipTokens
==================
*/
valueType *Com_SkipTokens(valueType *s, sint numTokens, valueType *sep) {
    sint        sepCount = 0;
    valueType  *p = s;

    while(sepCount < numTokens) {
        if(Com_CharIsOneOfCharset(*p++, sep)) {
            sepCount++;

            while(Com_CharIsOneOfCharset(*p, sep)) {
                p++;
            }
        } else if(*p == '\0') {
            break;
        }
    }

    if(sepCount == numTokens) {
        return p;
    } else {
        return s;
    }
}


/*
============
COM_GetExtension
============
*/
pointer     COM_GetExtension(pointer name) {
    sint             length, i;

    length = static_cast<sint>(::strlen(name)) - 1;
    i = length;

    while(name[i] != '.') {
        i--;

        if(name[i] == '/' || i == 0) {
            return "";    // no extension
        }
    }

    return &name[i + 1];
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension(pointer in, valueType *out) {
    while(*in && *in != '.') {
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
void COM_StripExtension2(pointer in, valueType *out, sint destsize) {
    sint len = 0;

    while(len < destsize - 1 && *in && *in != '.') {
        *out++ = *in++;
        len++;
    }

    *out = 0;
}

/*
============
COM_StripExtension3

RB: ioquake3 version
============
*/
void COM_StripExtension3(pointer src, valueType *dest, sint destsize) {
    sint             length;

    Q_strncpyz(dest, src, destsize);

    length = static_cast<sint>(::strlen(dest)) - 1;

    while(length > 0 && dest[length] != '.') {
        length--;

        if(dest[length] == '/') {
            return;    // no extension
        }
    }

    if(length) {
        dest[length] = 0;
    }
}


/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension(valueType *path, sint maxSize,
                          pointer extension) {
    valueType oldPath[MAX_QPATH];
    valueType    *src;

    //
    // if path doesn't have a .EXT, append extension
    // (extension should include the .)
    //
    src = path + strlen(path) - 1;

    while(*src != '/' && src != path) {
        if(*src == '.') {
            return;                 // it has an extension
        }

        src--;
    }

    Q_strncpyz(oldPath, path, sizeof(oldPath));
    Q_vsprintf_s(path, maxSize, maxSize, "%s%s", oldPath, extension);
}

//============================================================================

/*
============
Com_HashKey
============
*/
sint Com_HashKey(valueType *string, sint maxlen) {
    sint hash, i;

    hash = 0;

    for(i = 0; i < maxlen && string[i] != '\0'; i++) {
        hash += string[i] * (119 + i);
    }

    hash = (hash ^ (hash >> 10) ^ (hash >> 20));
    return hash;
}

//============================================================================


/*
==================
COM_BitCheck

  Allows bit-wise checks on arrays with more than one item (> 32 bits)
==================
*/
bool COM_BitCheck(const sint array[], sint bitNum) {
    sint i;

    i = 0;

    while(bitNum > 63) {
        i++;
        bitNum -= 64;
    }

    return (bool)((array[i] & (1 << bitNum)) !=
                  0);         // (SA) heh, whoops. :)
}

/*
==================
COM_BitSet

  Allows bit-wise SETS on arrays with more than one item (> 32 bits)
==================
*/
void COM_BitSet(sint array[], sint bitNum) {
    sint i;

    i = 0;

    while(bitNum > 31) {
        i++;
        bitNum -= 32;
    }

    array[i] |= (1 << bitNum);
}

/*
==================
COM_BitClear

  Allows bit-wise CLEAR on arrays with more than one item (> 32 bits)
==================
*/
void COM_BitClear(sint array[], sint bitNum) {
    sint i;

    i = 0;

    while(bitNum > 63) {
        i++;
        bitNum -= 64;
    }

    array[i] &= ~(1 << bitNum);
}
//============================================================================

/*
============================================================================

                    BYTE ORDER FUNCTIONS

============================================================================
*/

schar16 ShortSwap(schar16 l) {
    uchar8 b1, b2;

    b1 = l & 255;
    b2 = (l >> 8) & 255;

    return (b1 << 8) + b2;
}

schar16 ShortNoSwap(schar16 l) {
    return l;
}

sint    LongSwap(sint l) {
    uchar8 b1, b2, b3, b4;

    b1 = l & 255;
    b2 = (l >> 8) & 255;
    b3 = (l >> 16) & 255;
    b4 = (l >> 24) & 255;

    return (static_cast<sint>(b1) << 24) + (static_cast<sint>(b2) << 16) +
           (static_cast<sint>(b3) << 8) + b4;
}

sint LongNoSwap(sint l) {
    return l;
}

float32 FloatSwap(float32 f) {
    union {
        float32 f;
        uchar8 b[4];
    } dat1, dat2;


    dat1.f = f;
    dat2.b[0] = dat1.b[3];
    dat2.b[1] = dat1.b[2];
    dat2.b[2] = dat1.b[1];
    dat2.b[3] = dat1.b[0];
    return dat2.f;
}

float32 FloatNoSwap(float32 f) {
    return f;
}

/*
============================================================================

PARSING

============================================================================
*/

// multiple character punctuation tokens
pointer     punctuation[] = {
    "+=", "-=", "*=", "/=", "&=", "|=", "++", "--",
    "&&", "||", "<=", ">=", "==", "!=",
    nullptr
};

static valueType com_token[MAX_TOKEN_CHARS];
static valueType com_parsename[MAX_TOKEN_CHARS];
static sint com_lines;

static sint backup_lines;
static valueType    *backup_text;

void COM_BeginParseSession(pointer name) {
    com_lines = 0;
    Q_vsprintf_s(com_parsename, sizeof(com_parsename), sizeof(com_parsename),
                 "%s", name);
}

void COM_BackupParseSession(valueType **data_p) {
    backup_lines = com_lines;
    backup_text = *data_p;
}

void COM_RestoreParseSession(valueType **data_p) {
    com_lines = backup_lines;
    *data_p = backup_text;
}

void COM_SetCurrentParseLine(sint line) {
    com_lines = line;
}

sint COM_GetCurrentParseLine(void) {
    return com_lines;
}

valueType *COM_Parse(valueType **data_p) {
    return COM_ParseExt(data_p, true);
}

void COM_ParseError(valueType *format, ...) {
    va_list argptr;
    static valueType string[4096];

    va_start(argptr, format);
    Q_vsprintf_s(string, sizeof(string), format, argptr);
    va_end(argptr);

#ifndef OALAUDIO
    Com_Printf(S_COLOR_RED "ERROR: %s, line %d: %s\n", com_parsename,
               com_lines, string);
#endif
}

void COM_ParseWarning(valueType *format, ...) {
    va_list argptr;
    static valueType string[4096];

    va_start(argptr, format);
    Q_vsprintf_s(string, sizeof(string), format, argptr);
    va_end(argptr);

#ifndef OALAUDIO
    Com_Printf("WARNING: %s, line %d: %s\n", com_parsename, com_lines, string);
#endif
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
static valueType *SkipWhitespace(valueType *data, bool *hasNewLines) {
    sint c;

    while((c = *data) <= ' ') {
        if(!c) {
            return nullptr;
        }

        if(c == '\n') {
            com_lines++;
            *hasNewLines = true;
        }

        data++;
    }

    return data;
}

sint COM_Compress(valueType *data_p) {
    valueType *datai, *datao;
    sint c, size;
    bool ws = false;

    size = 0;
    datai = datao = data_p;

    if(datai) {
        while((c = *datai) != 0) {
            if(c == 13 || c == 10) {
                *datao = c;
                datao++;
                ws = false;
                datai++;
                size++;
                // skip double slash comments
            } else if(c == '/' && datai[1] == '/') {
                while(*datai && *datai != '\n') {
                    datai++;
                }

                ws = false;
                // skip /* */ comments
            } else if(c == '/' && datai[1] == '*') {
                datai += 2; // Arnout: skip over '/*'

                while(*datai && (*datai != '*' || datai[1] != '/')) {
                    datai++;
                }

                if(*datai) {
                    datai += 2;
                }

                ws = false;
            } else {
                if(ws) {
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

valueType *COM_ParseExt(valueType **data_p, bool allowLineBreaks) {
    sint c = 0, len;
    bool hasNewLines = false;
    valueType *data;

    data = *data_p;
    len = 0;
    com_token[0] = 0;

    // make sure incoming data is valid
    if(!data) {
        *data_p = nullptr;
        return com_token;
    }

    // RF, backup the session data so we can unget easily
    COM_BackupParseSession(data_p);

    while(1) {
        // skip whitespace
        data = SkipWhitespace(data, &hasNewLines);

        if(!data) {
            *data_p = nullptr;
            return com_token;
        }

        if(hasNewLines && !allowLineBreaks) {
            *data_p = data;
            return com_token;
        }

        c = *data;

        // skip double slash comments
        if(c == '/' && data[1] == '/') {
            data += 2;

            while(*data && *data != '\n') {
                data++;
            }

            //          com_lines++;
        }
        // skip /* */ comments
        else if(c == '/' && data[1] == '*') {
            data += 2;

            while(*data && (*data != '*' || data[1] != '/')) {
                data++;

                if(*data == '\n') {
                    //                  com_lines++;
                }
            }

            if(*data) {
                data += 2;
            }
        } else {
            break;
        }
    }

    // handle quoted strings
    if(c == '\"') {
        data++;

        while(1) {
            c = *data++;

            if(c == '\\' && *(data) == '\"') {
                // Arnout: string-in-string
                if(len < MAX_TOKEN_CHARS) {
                    com_token[len] = '\"';
                    len++;
                }

                data++;

                while(1) {
                    c = *data++;

                    if(!c) {
                        com_token[len] = 0;
                        *data_p = static_cast<valueType *>(data);
                        break;
                    }

                    if((c == '\\' && *(data) == '\"')) {
                        if(len < MAX_TOKEN_CHARS) {
                            com_token[len] = '\"';
                            len++;
                        }

                        data++;
                        c = *data++;
                        break;
                    }

                    if(len < MAX_TOKEN_CHARS) {
                        com_token[len] = c;
                        len++;
                    }
                }
            }

            if(c == '\"' || !c) {
                com_token[len] = 0;
                *data_p = static_cast<valueType *>(data);
                return com_token;
            }

            if(len < MAX_TOKEN_CHARS) {
                com_token[len] = c;
                len++;
            }
        }
    }

    // parse a regular word
    do {
        if(len < MAX_TOKEN_CHARS) {
            com_token[len] = c;
            len++;
        }

        data++;
        c = *data;

        if(c == '\n') {
            com_lines++;
        }
    } while(c > 32);

    if(len == MAX_TOKEN_CHARS) {
        //      Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
        len = 0;
    }

    com_token[len] = 0;

    *data_p = static_cast<valueType *>(data);
    return com_token;
}


valueType           *COM_Parse2(valueType **data_p) {
    return COM_ParseExt2(data_p, true);
}


// *INDENT-OFF*
valueType*           COM_ParseExt2( valueType** data_p, bool allowLineBreaks )
{
    sint             c = 0, len;
    bool        hasNewLines = false;
    valueType*           data;
    pointer*    punc;

    if( !data_p )
    {
#ifndef OALAUDIO
        Com_Error( ERR_FATAL, "COM_ParseExt: nullptr data_p" );
#endif
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
                *data_p = static_cast<valueType*>( data );
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

        *data_p = static_cast<valueType*>( data );
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

        *data_p = static_cast<valueType*>( data );
        return com_token;
    }

    // check for multi-character punctuation token
    for( punc = punctuation; *punc; punc++ )
    {
        sint             l;
        sint             j;

        l = static_cast<sint>( ::strlen( *punc ) );
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
            *data_p = static_cast<valueType*>( data );
            return com_token;
        }
    }

    // single character punctuation
    com_token[0] = *data;
    com_token[1] = 0;
    data++;
    *data_p = static_cast<valueType*>( data );

    return com_token;
}
// *INDENT-ON*


/*
==================
COM_MatchToken
==================
*/
void COM_MatchToken(valueType **buf_p, valueType *match) {
    valueType    *token;

    token = COM_Parse(buf_p);

    if(strcmp(token, match)) {
#ifndef OALAUDIO
        Com_Error(ERR_DROP, "MatchToken: %s != %s", token, match);
#endif
    }
}

/*
=================
SkipBracedSection_Depth

=================
*/
bool SkipBracedSection_Depth(valueType **program, sint depth) {
    valueType *token;

    do {
        token = COM_ParseExt(program, true);

        if(token[1] == 0) {
            if(token[0] == '{') {
                depth++;
            } else if(token[0] == '}') {
                depth--;
            }
        }
    } while(depth && *program);

    return (bool)(depth == 0);
}

/*
=================
SkipBracedSection

The next token should be an open brace.
Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
bool SkipBracedSection(valueType **program) {
    valueType            *token;
    sint depth;

    depth = 0;

    do {
        token = COM_ParseExt(program, true);

        if(token[1] == 0) {
            if(token[0] == '{') {
                depth++;
            } else if(token[0] == '}') {
                depth--;
            }
        }
    } while(depth && *program);

    return (bool)(depth == 0);
}

/*
=================
SkipRestOfLine
=================
*/
void SkipRestOfLine(valueType **data) {
    valueType    *p;
    sint c;

    p = *data;

    while((c = *p++) != 0) {
        if(c == '\n') {
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
sint COM_Parse2Infos(valueType *buf, sint max,
                     valueType infos[][MAX_INFO_STRING]) {
    pointer  token;
    sint count;
    valueType key[MAX_TOKEN_CHARS];

    count = 0;

    while(1) {
        token = COM_Parse(&buf);

        if(!token[0]) {
            break;
        }

        if(strcmp(token, "{")) {
#ifndef OALAUDIO
            Com_Printf("Missing { in info file\n");
#endif
            break;
        }

        if(count == max) {
#ifndef OALAUDIO
            Com_Printf("Max infos exceeded\n");
#endif
            break;
        }

        infos[count][0] = 0;

        while(1) {
            token = COM_Parse(&buf);

            if(!token[0]) {
#ifndef OALAUDIO
                Com_Printf("Unexpected end of info file\n");
#endif
                break;
            }

            if(!strcmp(token, "}")) {
                break;
            }

            Q_strncpyz(key, token, sizeof(key));

            token = COM_ParseExt(&buf, false);

            if(!token[0]) {
                token = "<nullptr>";
            }

            Info_SetValueForKey(infos[count], key, token);
        }

        count++;
    }

    return count;
}

void COM_Parse21DMatrix(valueType **buf_p, sint x, float32 *m,
                        bool checkBrackets) {
    valueType           *token;
    sint             i;

    if(checkBrackets) {
        COM_MatchToken(buf_p, "(");
    }

    for(i = 0; i < x; i++) {
        token = COM_Parse2(buf_p);
        m[i] = static_cast<float32>(::atof(token));
    }

    if(checkBrackets) {
        COM_MatchToken(buf_p, ")");
    }
}

void COM_Parse22DMatrix(valueType **buf_p, sint y, sint x, float32 *m) {
    sint             i;

    COM_MatchToken(buf_p, "(");

    for(i = 0; i < y; i++) {
        COM_Parse21DMatrix(buf_p, x, m + i * x, true);
    }

    COM_MatchToken(buf_p, ")");
}

void COM_Parse23DMatrix(valueType **buf_p, sint z, sint y, sint x,
                        float32 *m) {
    sint             i;

    COM_MatchToken(buf_p, "(");

    for(i = 0; i < z; i++) {
        COM_Parse22DMatrix(buf_p, y, x, m + i * x * y);
    }

    COM_MatchToken(buf_p, ")");
}

/*
===================
Com_HexStrToInt
===================
*/
sint Com_HexStrToInt(pointer str) {
    if(!str || !str[ 0 ]) {
        return -1;
    }

    // check for hex code
    if(str[ 0 ] == '0' && str[ 1 ] == 'x') {
        sint i, n = 0;

        for(i = 2; i < strlen(str); i++) {
            valueType digit;

            n *= 16;

            digit = tolower(str[ i ]);

            if(digit >= '0' && digit <= '9') {
                digit -= '0';
            } else if(digit >= 'a' && digit <= 'f') {
                digit = digit - 'a' + 10;
            } else {
                return -1;
            }

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
pointer Com_QuoteStr(pointer str) {
    static valueType *buf = nullptr;
    static uint64 buflen = 0;

    uint32 length;
    valueType *ptr;

    // quick exit if no quoting is needed
    //  if (!strpbrk (str, "\";"))
    //      return str;

    length = static_cast<uint64>(::strlen(str));

    if(buflen < 2 * length + 3) {
        ::free(buf);
        buflen = 2 * length + 3;
        buf = static_cast<valueType *>(::malloc(buflen));
    }

    ptr = buf;
    *ptr++ = '"';
    --str;

    while(*++str) {
        if(*str == '"') {
            *ptr++ = '\\';
        }

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
pointer Com_UnquoteStr(pointer str) {
    static valueType *buf = nullptr;

    uint32 length;
    valueType *ptr;
    pointer end;

    end = str + strlen(str);

    // Strip trailing spaces
    while(--end >= str)
        if(*end != ' ') {
            break;
        }

    // end points at the last non-space character

    // If it doesn't begin with '"', return quickly
    if(*str != '"') {
        length = static_cast<uint32>(end + 1 - str);
        free(buf);
        buf = static_cast<valueType *>(malloc(length + 1));
        strncpy(buf, str, length);
        buf[length] = 0;
        return buf;
    }

    // It begins with '"'; if it ends with '"', lose that '"'
    if(end > str && *end == '"') {
        --end;
    }

    free(buf);
    buf = static_cast<valueType *>(malloc(end + 1 - str));
    ptr = buf;

    // Copy, unquoting as we go
    // str[0] == '"', so that gets skipped
    while(++str <= end) {
        if(str[0] == '\\' && str[1] == '"' &&
                str < end) { // FIXME: \ semantics are broken
            ++str;
        }

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

sint Q_isprint(sint c) {
    if(c >= 0x20 && c <= 0x7E) {
        return (1);
    }

    return (0);
}

sint Q_islower(sint c) {
    if(c >= 'a' && c <= 'z') {
        return (1);
    }

    return (0);
}

sint Q_isupper(sint c) {
    if(c >= 'A' && c <= 'Z') {
        return (1);
    }

    return (0);
}

sint Q_isalpha(sint c) {
    if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
        return (1);
    }

    return (0);
}

sint Q_isnumeric(sint c) {
    if(c >= '0' && c <= '9') {
        return (1);
    }

    return (0);
}

sint Q_isalphanumeric(sint c) {
    if(Q_isalpha(c) ||
            Q_isnumeric(c)) {
        return(1);
    }

    return (0);
}

sint Q_isforfilename(sint c) {
    if((Q_isalphanumeric(c) || c == '_') &&
            c != ' ') {        // space not allowed in filename
        return(1);
    }

    return (0);
}

valueType *Q_strrchr(pointer string, sint c) {
    valueType cc = c;
    valueType *s;
    valueType *sp = static_cast<valueType *>(0);

    s = (const_cast<valueType *>(reinterpret_cast<const valueType *>(string)));

    while(*s) {
        if(*s == cc) {
            sp = s;
        }

        s++;
    }

    if(cc == 0) {
        sp = s;
    }

    return sp;
}

/*
=============
Q_strtoi/l

Takes a null-terminated string (which represents either a float32 or integer
conforming to strtod) and an integer to assign to (if successful).

Returns true on success and vice versa.
Demonstration of behavior of strtod and conversions: http://codepad.org/YQKxV94R
-============
*/
bool Q_strtol(pointer s, sint32 *outNum) {
    valueType *p;

    if(*s == '\0') {
        return false;
    }

    *outNum = static_cast<sint32>(::strtod(s, &p));

    return (bool)(*p == '\0');
}

bool Q_strtoi(pointer s, sint *outNum) {
    valueType *p;

    if(*s == '\0') {
        return false;
    }

    *outNum = static_cast<sint>(::strtod(s, &p));

    return (bool)(*p == '\0');
}

/*
=============
Q_strncpyz

Safe strncpy that ensures a trailing zero
=============
*/
// Dushan
#if defined(_DEBUG)
void Q_strncpyzDebug(valueType *dest, pointer src, uint32 destsize,
                     pointer file, sint line)
#else
void Q_strncpyz(valueType *dest, pointer src, sint destsize)
#endif
{
    valueType *d;
    pointer s;
    uint64 n;

#ifdef _DEBUG

    if(!dest) {
#ifndef OALAUDIO
        Com_Error(ERR_DROP, "Q_strncpyz: nullptr dest (%s, %i)", file, line);
#endif
    }

    if(!src) {
#ifndef OALAUDIO
        Com_Error(ERR_DROP, "Q_strncpyz: nullptr src (%s, %i)", file, line);
#endif
    }

    if(destsize < 1) {
#ifndef OALAUDIO
        Com_Error(ERR_DROP, "Q_strncpyz: destsize < 1 (%s, %i)", file, line);
#endif
    }

#else

    if(!dest) {
#ifndef OALAUDIO
        Com_Error(ERR_FATAL, "Q_strncpyz: nullptr dest");
#endif
    }

    if(!src) {
#ifndef OALAUDIO
        Com_Error(ERR_FATAL, "Q_strncpyz: nullptr src");
#endif
    }

    if(destsize < 1) {
#ifndef OALAUDIO
        Com_Error(ERR_FATAL, "Q_strncpyz: destsize < 1");
#endif
    }

#endif

    /*
    * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
    *
    * Permission to use, copy, modify, and distribute this software for any
    * purpose with or without fee is hereby granted, provided that the above
    * copyright notice and this permission notice appear in all copies.
    *
    * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
    * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
    * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
    */

    d = dest;
    s = src;
    n = destsize;

    /* Copy as many bytes as will fit */
    if(n != 0) {
        while(--n != 0) {
            if((*d++ = *s++) == '\0') {
                break;
            }
        }
    }

    /* Not enough room in dst, add NUL and traverse rest of src */
    if(n == 0) {
        if(destsize != 0) {
            *d = '\0';    /* NUL-terminate dst */
        }

        while(*s++)
            ;
    }
}

sint Q_stricmpn(pointer s1, pointer s2, sint n) {
    sint c1, c2;

    if(s1 == nullptr) {
        if(s2 == nullptr) {
            return 0;
        } else {
            return -1;
        }
    } else if(s2 == nullptr) {
        return 1;
    }

    do {
        c1 = *s1++;
        c2 = *s2++;

        if(!n--) {
            // strings are equal until end point
            return 0;
        }

        if(c1 != c2) {
            if(c1 >= 'a' && c1 <= 'z') {
                c1 -= ('a' - 'A');
            }

            if(c2 >= 'a' && c2 <= 'z') {
                c2 -= ('a' - 'A');
            }

            if(c1 != c2) {
                return c1 < c2 ? -1 : 1;
            }
        }
    } while(c1);

    // strings are equal
    return 0;
}

sint Q_strncmp(pointer s1, pointer s2, sint n) {
    sint c1, c2;

    do {
        c1 = *s1++;
        c2 = *s2++;

        if(!n--) {
            return 0;       // strings are equal until end point
        }

        if(c1 != c2) {
            return c1 < c2 ? -1 : 1;
        }
    } while(c1);

    return 0;       // strings are equal
}

sint Q_stricmp(pointer s1, pointer s2) {
    return (s1 && s2) ? Q_stricmpn(s1, s2, 99999) : -1;
}

valueType *Q_strlwr(valueType *s1) {
    valueType   *s;

    for(s = s1; *s; ++s) {
        if(('A' <= *s) && (*s <= 'Z')) {
            *s -= 'A' - 'a';
        }
    }

    return s1;
}

valueType *Q_strupr(valueType *s1) {
    valueType *cp;

    for(cp = s1 ; *cp ; ++cp) {
        if(('a' <= *cp) && (*cp <= 'z')) {
            *cp += 'A' - 'a';
        }
    }

    return s1;
}

// never goes past bounds or leaves without a terminating 0
void Q_strcat(valueType *dest, sint size, pointer src) {
    sint l1;

    l1 = static_cast<sint>(::strlen(dest));

    if(l1 >= size) {
#ifndef OALAUDIO
        Com_Error(ERR_FATAL, "Q_strcat: already overflowed");
#endif
    }

    Q_strncpyz(dest + l1, src,  size - l1);
}


sint Q_strnicmp(pointer string1, pointer string2, sint n) {
    sint c1, c2;

    if(string1 == nullptr) {
        if(string2 == nullptr) {
            return 0;
        } else {
            return -1;
        }
    } else if(string2 == nullptr) {
        return 1;
    }

    do {
        c1 = *string1++;
        c2 = *string2++;

        if(!n--) {
            return 0;    // Strings are equal until end point
        }

        if(c1 != c2) {
            if(c1 >= 'a' && c1 <= 'z') {
                c1 -= ('a' - 'A');
            }

            if(c2 >= 'a' && c2 <= 'z') {
                c2 -= ('a' - 'A');
            }

            if(c1 != c2) {
                return c1 < c2 ? -1 : 1;
            }
        }
    } while(c1);

    return 0;// Strings are equal
}

/*
=============
Q_strreplace

Case insensitive version of strstr
=============
*/
pointer Q_stristr(pointer s, pointer find) {
    if(!*find) {
        return s;
    }

    for(; *s; ++s) {
        if(toupper(*s) == toupper(*find)) {
            pointer h, n;

            for(h = s, n = find; *h && *n; ++h, ++n) {
                if(toupper(*h) != toupper(*n)) {
                    break;
                }
            }

            if(!*n) {
                return s;
            }
        }
    }

    return nullptr;
}


/*
=============
Q_strreplace

replaces content of find by replace in dest
=============
*/
bool Q_strreplace(valueType *dest, sint destsize, pointer find,
                  pointer replace) {
    sint             lstart, lfind, lreplace, lend;
    valueType           *s;
    valueType
    backup[32000]; // big, but small enough to fit in PPC stack

    lend = static_cast<sint>(::strlen(dest));

    if(lend >= destsize) {
#ifndef OALAUDIO
        Com_Error(ERR_FATAL, "Q_strreplace: already overflowed");
#endif
    }

    s = strstr(dest, find);

    if(!s) {
        return false;
    } else {
        Q_strncpyz(backup, dest, lend + 1);
        lstart = static_cast<sint>(s - dest);
        lfind = static_cast<sint>(::strlen(find));
        lreplace = static_cast<sint>(::strlen(replace));

        strncpy(s, replace, destsize - lstart - 1);
        strncpy(s + lreplace, backup + lstart + lfind,
                destsize - lstart - lreplace - 1);

        return true;
    }
}


sint Q_PrintStrlen(pointer string) {
    sint len;
    pointer  p;

    if(!string) {
        return 0;
    }

    len = 0;
    p = string;

    while(*p) {
        if(Q_IsColorString(p)) {
            p += 2;
            continue;
        }

        if(*p == Q_COLOR_ESCAPE && *(p + 1) == Q_COLOR_ESCAPE) {
            p++;
        }

        p++;
        len++;
    }

    return len;
}


valueType *Q_CleanStr(valueType *string) {
    valueType   *d;
    valueType   *s;

    s = string;
    d = string;

    while(*s) {
        if(Q_IsColorString(s)) {
            s += 2;
            continue;
        }

        if(*s == Q_COLOR_ESCAPE && *(s + 1) == Q_COLOR_ESCAPE) {
            s++;
        }

        if(*s >= 0x20 && *s <= 0x7E) {
            *d++ = *s;
        }

        s++;
    }

    *d = '\0';


    return string;
}

// strips whitespaces and bad characters
bool Q_isBadDirChar(valueType c) {
    valueType badchars[] = { ';', '&', '(', ')', '|', '<', '>', '*', '?', '[', ']', '~', '+', '@', '!', '\\', '/', ' ', '\'', '\"', '\0' };
    sint i;

    for(i = 0; badchars[i] != '\0'; i++) {
        if(c == badchars[i]) {
            return true;
        }
    }

    return false;
}

valueType *Q_CleanDirName(valueType *dirname) {
    valueType   *d;
    valueType   *s;

    s = dirname;
    d = dirname;

    // clear trailing .'s
    while(*s == '.') {
        s++;
    }

    while(*s != '\0') {
        if(!Q_isBadDirChar(*s)) {
            *d++ = *s;
        }

        s++;
    }

    *d = '\0';

    return dirname;
}

sint Q_CountChar(pointer string, valueType tocount) {
    sint count;

    for(count = 0; *string; string++) {
        if(*string == tocount) {
            count++;
        }
    }

    return count;
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
valueType *va(pointer format, ...) {
    va_list argptr;
    static valueType string[64][1024], *s;
    static sint stringindex = 0;

    s = string[stringindex];
    stringindex = (stringindex + 1) & 63;
    va_start(argptr, format);
    Q_vsprintf_s(s, sizeof(string[0]), format, argptr);

    va_end(argptr);

    return s;
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
valueType *Info_ValueForKey(pointer s, pointer key) {
    valueType pkey[BIG_INFO_KEY];
    static valueType value[2][BIG_INFO_VALUE];   // use two buffers so compares
    // work without stomping on each other
    static sint valueindex = 0;
    valueType    *o;

    if(!s || !key) {
        return "";
    }

    if(strlen(s) >= BIG_INFO_STRING) {
#ifndef OALAUDIO
        Com_Error(ERR_DROP, "Info_ValueForKey: oversize infostring [%s] [%s]", s,
                  key);
#endif
    }

    valueindex ^= 1;

    if(*s == '\\') {
        s++;
    }

    while(1) {
        o = pkey;

        while(*s != '\\') {
            if(!*s) {
                return "";
            }

            *o++ = *s++;
        }

        *o = 0;
        s++;

        o = value[valueindex];

        while(*s != '\\' && *s) {
            *o++ = *s++;
        }

        *o = 0;

        if(!Q_stricmp(key, pkey)) {
            return value[valueindex];
        }

        if(!*s) {
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
void Info_NextPair(pointer *head, valueType *key, valueType *value) {
    valueType    *o;
    pointer  s;

    s = *head;

    if(*s == '\\') {
        s++;
    }

    key[0] = 0;
    value[0] = 0;

    o = key;

    while(*s != '\\') {
        if(!*s) {
            *o = 0;
            *head = s;
            return;
        }

        *o++ = *s++;
    }

    *o = 0;
    s++;

    o = value;

    while(*s != '\\' && *s) {
        *o++ = *s++;
    }

    *o = 0;

    *head = s;
}

/*
=============
Q_bytestrcpy
=============
*/
static void Q_bytestrcpy(valueType *destation, pointer source) {
    while(*source) {
        *destation++ = *source++;
    }

    *destation = 0;
}

/*
===================
Info_RemoveKey
===================
*/
void Info_RemoveKey(valueType *s, pointer key) {
    valueType    *start;
    valueType pkey[MAX_INFO_KEY];
    valueType value[MAX_INFO_VALUE];
    valueType    *o;

    if(strlen(s) >= MAX_INFO_STRING) {
#ifndef OALAUDIO
        Com_Error(ERR_DROP, "Info_RemoveKey: oversize infostring [%s] [%s]", s,
                  key);
#endif
    }

    if(strchr(key, '\\')) {
        return;
    }

    while(1) {
        start = s;

        if(*s == '\\') {
            s++;
        }

        o = pkey;

        while(*s != '\\') {
            if(!*s) {
                return;
            }

            *o++ = *s++;
        }

        *o = 0;
        s++;

        o = value;

        while(*s != '\\' && *s) {
            if(!*s) {
                return;
            }

            *o++ = *s++;
        }

        *o = 0;

        if(!Q_stricmp(key, pkey)) {
            Q_bytestrcpy(start, s);
            return;
        }

        if(!*s) {
            return;
        }
    }

}

/*
===================
Info_RemoveKey_Big
===================
*/
void Info_RemoveKey_Big(valueType *s, pointer key) {
    valueType    *start;
    valueType pkey[BIG_INFO_KEY];
    valueType value[BIG_INFO_VALUE];
    valueType    *o;

    if(strlen(s) >= BIG_INFO_STRING) {
#ifndef OALAUDIO
        Com_Error(ERR_DROP, "Info_RemoveKey_Big: oversize infostring [%s] [%s]", s,
                  key);
#endif
    }

    if(strchr(key, '\\')) {
        return;
    }

    while(1) {
        start = s;

        if(*s == '\\') {
            s++;
        }

        o = pkey;

        while(*s != '\\') {
            if(!*s) {
                return;
            }

            *o++ = *s++;
        }

        *o = 0;
        s++;

        o = value;

        while(*s != '\\' && *s) {
            if(!*s) {
                return;
            }

            *o++ = *s++;
        }

        *o = 0;

        if(!Q_stricmp(key, pkey)) {
            Q_bytestrcpy(start, s);
            return;
        }

        if(!*s) {
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
bool Info_Validate(pointer s) {
    if(strchr(s, '\"')) {
        return false;
    }

    if(strchr(s, ';')) {
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
bool Info_SetValueForKey(valueType *s, pointer key, pointer value) {
    valueType   newi[MAX_INFO_STRING], *v;
    sint        c, maxsize = MAX_INFO_STRING;

    if(strlen(s) >= MAX_INFO_STRING) {
#ifndef OALAUDIO
        Com_Error(ERR_DROP, "SetValueForKey: oversize infostring [%s] [%s] [%s]",
                  s, key, value);
#endif
    }

    if(strchr(key, '\\') || strchr(value, '\\')) {
#ifndef OALAUDIO
        Com_Printf("SetValueForKey: Can't use keys or values with a \\\n");
#endif
        return false;
    }

    if(strchr(key, ';') || strchr(value, ';')) {
#ifndef OALAUDIO
        Com_Printf("SetValueForKey: Can't use keys or values with a semicolon\n");
#endif
        return false;
    }

    if(strlen(key) > MAX_INFO_KEY - 1 || strlen(value) > MAX_INFO_KEY - 1) {
#ifndef OALAUDIO
        Com_Error(ERR_DROP,
                  "SetValueForKey: keys and values must be < %i characters.\n",
                  MAX_INFO_KEY);
#endif
        return false;
    }

    Info_RemoveKey(s, key);

    if(!value || !strlen(value)) {
        return true; // just clear variable
    }

    Q_vsprintf_s(newi, sizeof(newi), sizeof(newi), "\\%s\\%s", key, value);

    if(strlen(newi) + strlen(s) > maxsize) {
#ifndef OALAUDIO
        Com_Printf("SetValueForKey: Info string length exceeded: %s\n", s);
#endif
        return true;
    }

    // only copy ascii values
    s += strlen(s);
    v = newi;

    while(*v) {
        c = *v++;
        c &= 255;   // strip high bits

        if(c >= 32 && c <= 255) {
            *s++ = c;
        }
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
void Info_SetValueForKey_Big(valueType *s, pointer key, pointer value) {
    valueType newi[BIG_INFO_STRING];

    if(strlen(s) >= BIG_INFO_STRING) {
#ifndef OALAUDIO
        Com_Error(ERR_DROP,
                  "Info_SetValueForKey: oversize infostring [%s] [%s] [%s]", s, key, value);
#endif
    }

    if(strchr(key, '\\') || strchr(value, '\\')) {
#ifndef OALAUDIO
        Com_Printf("Can't use keys or values with a \\\n");
#endif
        return;
    }

    if(strchr(key, ';') || strchr(value, ';')) {
#ifndef OALAUDIO
        Com_Printf("Can't use keys or values with a semicolon\n");
#endif
        return;
    }

    if(strchr(key, '\"') || strchr(value, '\"')) {
#ifndef OALAUDIO
        Com_Printf("Can't use keys or values with a \"\n");
#endif
        return;
    }

    Info_RemoveKey_Big(s, key);

    if(!value || !strlen(value)) {
        return;
    }

    Q_vsprintf_s(newi, sizeof(newi), sizeof(newi), "\\%s\\%s", key, value);

    if(strlen(newi) + strlen(s) > BIG_INFO_STRING) {
#ifndef OALAUDIO
        Com_Printf("Info_SetValueForKey_Big: Info string length exceeded: %s\n",
                   s);
#endif
        return;
    }

    Q_strcat(s, MAX_INFO_STRING, newi);
}

/*
============
Com_ClientListContains
============
*/
bool Com_ClientListContains(const clientList_t *list, sint clientNum) {
    if(clientNum < 0 || clientNum >= MAX_CLIENTS || !list) {
        return false;
    }

    if(clientNum < 64) {
        return (bool)((list->lo & (1 << clientNum)) != 0);
    } else {
        return (bool)((list->hi & (1 << (clientNum - 64))) != 0);
    }
}

/*
============
Com_ClientListString
============
*/
valueType *Com_ClientListString(const clientList_t *list) {
    static valueType s[ 17 ];

    s[ 0 ] = '\0';

    if(!list) {
        return s;
    }

    Q_vsprintf_s(s, sizeof(s), sizeof(s), "%08x%08x", list->hi, list->lo);
    return s;
}

/*
============
Com_ClientListParse
============
*/
void Com_ClientListParse(clientList_t *list, pointer s) {
    if(!list) {
        return;
    }

    list->lo = 0;
    list->hi = 0;

    if(!s) {
        return;
    }

    if(strlen(s) != 16) {
        return;
    }

    sscanf(s, "%x%x", &list->hi, &list->lo);
}

/*
================
VectorMatrixMultiply
================
*/
void VectorMatrixMultiply(const vec3_t p, vec3_t m[ 3 ], vec3_t out) {
    out[ 0 ] = m[ 0 ][ 0 ] * p[ 0 ] + m[ 1 ][ 0 ] * p[ 1 ] + m[ 2 ][ 0 ] *
               p[ 2 ];
    out[ 1 ] = m[ 0 ][ 1 ] * p[ 0 ] + m[ 1 ][ 1 ] * p[ 1 ] + m[ 2 ][ 1 ] *
               p[ 2 ];
    out[ 2 ] = m[ 0 ][ 2 ] * p[ 0 ] + m[ 1 ][ 2 ] * p[ 1 ] + m[ 2 ][ 2 ] *
               p[ 2 ];
}

/*
=============
Q_vsprintf_s
=============
*/
void Q_vsprintf_s(valueType *pDest, uint32 nDestSize, pointer pFmt,
                  va_list args) {
#ifdef _WIN32
    vsprintf_s(pDest, nDestSize, pFmt, args);
#else
    vsprintf(pDest, pFmt, args);
#endif
}

bool StringContainsWord(pointer haystack, pointer needle) {
    if(!*needle) {
        return false;
    }

    for(; *haystack; ++haystack) {
        if(toupper(*haystack) == toupper(*needle)) {
            /*
            * Matched starting char -- loop through remaining chars.
            */
            pointer h, n;

            for(h = haystack, n = needle; *h && *n; ++h, ++n) {
                if(toupper(*h) != toupper(*n)) {
                    break;
                }
            }

            if(!*n) { /* matched all of 'needle' to null termination */
                return true; /* return the start of the match */
            }
        }
    }

    return false;
}

/*
============
COM_CompareExtension

string compare the end of the strings and return qtrue if strings match
============
*/
bool COM_CompareExtension(pointer in, pointer ext) {
    sint inlen, extlen;

    inlen = static_cast<sint>(::strlen(in));
    extlen = static_cast<sint>(::strlen(ext));

    if(extlen <= inlen) {
        in += inlen - extlen;

        if(!Q_stricmp(in, ext)) {
            return true;
        }
    }

    return false;
}

// Returns a float min <= x < max (exclusive; will get max - 0.00001; but never max)
static uint holdrand = 0x89abcdef;
#define QRAND_MAX 32768
float32 flrand(float32 min, float32 max) {
    float32 result;

    holdrand = (holdrand * 214013L) + 2531011L;
    result = static_cast<float32>((holdrand) >> 17);     // 0 - 32767 range
    result = ((result * (max - min)) / static_cast<float32>(QRAND_MAX)) + min;

    return(result);
}

float32 Q_flrand(float32 min, float32 max) {
    return flrand(min, max);
}

bool Q_CleanPlayerName(pointer in, valueType *out, sint outSize) {
    sint len, i;
    sint numColorChanges;

    numColorChanges = 0;

    for(len = 0, i = 0; len < outSize - 1; i++) {
        if(!in[i]) {
            break;
        }

        if(Q_IsColorString(in + i)) {
            if(in[i + 1] == COLOR_BLACK) {
                //don't allow black
                i++;
                continue;
            }

            if(len >= outSize - 2)
                //not enough room
            {
                break;
            }

            numColorChanges++;

            out[len++] = Q_COLOR_ESCAPE;
            out[len++] = in[i + 1];
            i++;

            continue;
        }

        if(in[i] == ' ') {
            if(!len)
                //no leading spaces
            {
                continue;
            } else if(out[len - 1] == ' ')
                //don't allow more than once consecutive space
            {
                continue;
            }
        }

        out[len++] = in[i];
    }

    for(; ;) {
        if(len >= 1 && out[len - 1] == ' ') {
            //don't allow any trailing spaces
            len -= 1;
        }

        if(len >= 2 && Q_IsColorString(out + len - 2)) {
            //don't allow any trailing color strings
            len -= 2;
            numColorChanges--;
        } else {
            break;
        }
    }

    if(numColorChanges) {
        //must append a return to default color at the end of the string

        if(len < outSize - 2) {
            //good to append, no adjustments
        } else {
            if(len < outSize - 1) {
                //we know that the last thing in the string isn't a color
                //sequence so we can freely truncate the last character
                len -= 1;
            } else {
                //we have to overwrite the last two characters in the string

                if(Q_IsColorString(out + len - 3)) {
                    //second last char is the end of a color string,
                    //replace the whole color string to avoid "^^-"
                    len -= 3;
                } else {
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

    if(!len)
        //don't allow empty player names
    {
        Q_strncpyz(out, "UnnamedPlayer", outSize);
    }

    return strcmp(in, out) != 0;
}

valueType *Com_StringContains(valueType *str1, valueType *str2,
                              sint casesensitive) {
    uint64 len, i, j;

    len = static_cast<sint>(::strlen(str1)) - static_cast<sint>(::strlen(
                str2));

    for(i = 0; i <= len; i++, str1++) {
        for(j = 0; str2[j]; j++) {
            if(casesensitive) {
                if(str1[j] != str2[j]) {
                    break;
                }
            } else {
                if(toupper(str1[j]) != toupper(str2[j])) {
                    break;
                }
            }
        }

        if(!str2[j]) {
            return str1;
        }
    }

    return nullptr;

}

bool Q_isanumber(pointer s) {
    valueType *p;
    float64 d;

    if(*s == '\0') {
        return false;
    }

    d = strtod(s, &p);

    return *p == '\0';
}

bool Q_isintegral(float32 f) {
    return static_cast<sint>(f) == f;
}

sint COM_CompressBracedSection(valueType **data_p, valueType **name,
                               valueType **text, sint *nameLength, sint *textLength) {
    valueType *in, * out;
    sint depth, c;

    if(!*data_p) {
        return -1;
    }

    *name = nullptr;
    *text = nullptr;

    *nameLength = 0;
    *textLength = 0;

    depth = 0;
    in = out = *data_p;

    if(!*in) {
        return 0;
    }

    while((c = *in++) != '\0') {
        if(c <= '/' || c >= '{') {   // skip lot of conditions if c is regular char
            //  whitespace or newline
            if(c <= ' ') {
                if(out > * data_p && out[-1] <= ' ') {
                    out--;
                    *out = (c == '\n' ? '\n' : *out);
                } else {
                    *out = (c == '\n' ? '\n' : ' ');
                }

                while(*in && *in <= ' ') {
                    if(*in++ == '\n') {
                        com_lines++;
                        *out = '\n';
                    }
                }

                out++;
                continue;
            }

            // skip comments
            if(c == '/') {
                // double slash comments
                if(*in == '/') {
                    in++;

                    while(*in && *in != '\n') {
                        in++;    // ignore until newline
                    }

                    if(out > * data_p && out[-1] <= ' ') {
                        out--;
                    }

                    if(*in) {
                        in++;
                    }

                    com_lines++;
                    *out++ = '\n';
                }
                // multiline /* */ comments
                else if(*in == '*') {
                    in++;

                    while(*in && (*in != '*' ||
                                  in[1] != '/')) {   // ignore until comment close
                        if(*in++ == '\n') {
                            com_lines++;
                        }
                    }

                    if(*in) {
                        in += 2;
                    }
                }
                // not comment
                else {
                    *out++ = '/';
                }

                continue;
            }

            // handle quoted strings
            if(c == '"') {
                *out++ = '"';

                while(*in && *in != '"') {
                    *out++ = *in++;
                }

                *out++ = '"';
                in++;
                continue;
            }

            // brace matching
            if(c == '{' || c == '}') {
                if(c == '{' && !*name) {
                    *name = *data_p;

                    if(*(*name) <= ' ') {
                        (*name)++;
                    }

                    *nameLength = out - *name;

                    if((*name)[*nameLength - 1] <= ' ') {
                        (*nameLength)--;
                    }

                    *text = out;
                }

                if(out > * data_p && out[-1] > ' ' && out + 1 < in) {
                    *out++ = ' ';
                }

                *out++ = c;

                if(out + 1 < in) {
                    *out++ = ' ';
                }

                depth += (c == '{' ? +1 : -1);

                if(depth <= 0) {
                    break;
                }

                continue;
            }
        }

        // parse a regular word
        while(c) {
            *out++ = c;
            c = *in;

            // end of regular chars ?
            if(c <= '/') {
                break;
            }

            if(c >= '{') {
                break;
            }

            in++;
        }
    }

    if(depth) {
        COM_ParseWarning("Unmatched braces in shader text");
    }

    if(!c) {
        in--;
    }

    if(*text && *(*text) <= ' ') {
        (*text)++;    // remove begining white char
    }

    if(out > * data_p && out[-1] <= ' ') {
        out--;    // remove ending white char
    }

    if(*text) {
        *textLength = out - *text;    // compressed text length
    }

    c = out - *data_p;                      // uncompressed chars parsed

    *data_p = in;

    return c;
}

void Q_strstrip(valueType *string, pointer strip, pointer repl) {
    sint replaceLen = repl ? strlen(repl) : 0, offset = 0;
    valueType *out = string, * p = string, c;
    pointer s = strip;
    bool recordChar = true;

    while((c = *p++) != '\0') {
        recordChar = true;

        for(s = strip; *s; s++) {
            offset = s - strip;

            if(c == *s) {
                if(!repl || offset >= replaceLen) {
                    recordChar = false;
                } else {
                    c = repl[offset];
                }

                break;
            }
        }

        if(recordChar) {
            *out++ = c;
        }
    }

    *out = '\0';
}

bool Q_IsColorString(pointer p) {
    if(!p) {
        return false;
    }

    if(p[0] != Q_COLOR_ESCAPE) {
        return false;
    }

    if(p[1] == 0) {
        return false;
    }

    // isalnum expects a signed integer in the range -1 (EOF) to 255, or it might assert on undefined behaviour
    // a dereferenced char pointer has the range -128 to 127, so we just need to rangecheck the negative part
    if(p[1] < 0) {
        return false;
    }

    if(isalnum(p[1]) == 0) {
        return false;
    }

    return true;
}

void Q_strcpy_s(valueType *pDest, uint32 nDestSize, pointer pSrc) {
    assert(pDest && pSrc);

    valueType *pLast = pDest + nDestSize - 1;

    while((pDest < pLast) && (*pSrc != 0)) {
        *pDest = *pSrc;
        ++pDest;
        ++pSrc;
    }

    *pDest = 0;
}

sint Q_vsprintf_s(valueType *strDest, uint64 destMax, uint64 count,
                  pointer format, ...) {
    sint ret = 0;
    va_list arglist;

    va_start(arglist, format);

#if defined (_WIN32)
    ret = vsnprintf_s(strDest, destMax, count, format, arglist);
#else
    ret = vsnprintf(strDest, destMax, format, arglist);
#endif
    va_end(arglist);

    return ret;
}
