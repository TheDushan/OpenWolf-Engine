////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2000 - 2009 Darklegion Development
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
// File name:   Parse.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.hpp>
#elif DEDICATED
#include <null/null_serverprecompiled.hpp>
#else
#include <framework/precompiled.hpp>
#endif

sint numtokens;

//list with global defines added to every source loaded
define_t* globaldefines;
source_t* sourceFiles[MAX_SOURCEFILES];

idParseSystemLocal parseLocal;
idParseSystem* ParseSystem = &parseLocal;

/*
===============
idParseSystemLocal::idParseSystemLocal
===============
*/
idParseSystemLocal::idParseSystemLocal( void )
{
}

/*
===============
idParseSystemLocal::~idParseSystemLocal
===============
*/
idParseSystemLocal::~idParseSystemLocal( void )
{
}

/*
===============
idParseSystemLocal::CreatePunctuationTable
===============
*/
void idParseSystemLocal::CreatePunctuationTable( script_t* script, punctuation_t* punctuations )
{
    sint i;
    punctuation_t* p, *lastp, *newp;
    
    //get memory for the table
    if( !script->punctuationtable )
    {
        script->punctuationtable = ( punctuation_t** )Z_Malloc( 256 * sizeof( punctuation_t* ) );
    }
    
    ::memset( script->punctuationtable, 0, 256 * sizeof( punctuation_t* ) );
    
    //add the punctuations in the list to the punctuation table
    for( i = 0; punctuations[i].p; i++ )
    {
        newp = &punctuations[i];
        lastp = nullptr;
        
        //sort the punctuations in this table entry on length (longer punctuations first)
        for( p = script->punctuationtable[( uint ) newp->p[0]]; p; p = p->next )
        {
            if( ::strlen( p->p ) < ::strlen( newp->p ) )
            {
                newp->next = p;
                
                if( lastp )
                {
                    lastp->next = newp;
                }
                else
                {
                    script->punctuationtable[( uint )newp->p[0]] = newp;
                }
                break;
            }
            
            lastp = p;
        }
        
        if( !p )
        {
            newp->next = nullptr;
            
            if( lastp )
            {
                lastp->next = newp;
            }
            else
            {
                script->punctuationtable[( uint )newp->p[0]] = newp;
            }
        }
    }
}

/*
===============
idParseSystemLocal::ScriptError
===============
*/
void idParseSystemLocal::ScriptError( script_t* script, valueType* str, ... )
{
    va_list argptr;
    valueType text[8192];
    
    if( script->flags & SCFL_NOERRORS )
    {
        return;
    }
    
    va_start( argptr, str );
    Q_vsprintf_s( text, sizeof( text ), str, argptr );
    va_end( argptr );
    
    Com_Printf( "file %s, line %d: %s\n", script->filename, script->line, text );
}

/*
===============
idParseSystemLocal::ScriptWarning
===============
*/
void idParseSystemLocal::ScriptWarning( script_t* script, valueType* str, ... )
{
    va_list argptr;
    valueType text[8192];
    
    if( script->flags & SCFL_NOWARNINGS )
    {
        return;
    }
    
    va_start( argptr, str );
    Q_vsprintf_s( text, sizeof( text ), str, argptr );
    va_end( argptr );
    
    Com_Printf( "file %s, line %d: %s\n", script->filename, script->line, text );
}

/*
===============
idParseSystemLocal::SetScriptPunctuations
===============
*/
void idParseSystemLocal::SetScriptPunctuations( script_t* script, punctuation_t* p )
{
    if( p )
    {
        CreatePunctuationTable( script, p );
    }
    else
    {
        CreatePunctuationTable( script, Default_Punctuations );
    }
    
    if( p )
    {
        script->punctuations = p;
    }
    else
    {
        script->punctuations = Default_Punctuations;
    }
}

/*
===============
idParseSystemLocal::ReadWhiteSpace
===============
*/
sint idParseSystemLocal::ReadWhiteSpace( script_t* script )
{
    while( 1 )
    {
        //skip white space
        while( *script->script_p <= ' ' )
        {
            if( !*script->script_p )
            {
                return 0;
            }
            
            if( *script->script_p == '\n' )
            {
                script->line++;
            }
            script->script_p++;
        }
        
        //skip comments
        if( *script->script_p == '/' )
        {
            //comments //
            if( *( script->script_p + 1 ) == '/' )
            {
                script->script_p++;
                do
                {
                    script->script_p++;
                    
                    if( !*script->script_p )
                    {
                        return 0;
                    }
                }
                while( *script->script_p != '\n' );
                
                script->line++;
                script->script_p++;
                
                if( !*script->script_p )
                {
                    return 0;
                }
                
                continue;
            }
            //comments /* */
            else if( *( script->script_p + 1 ) == '*' )
            {
                script->script_p++;
                do
                {
                    script->script_p++;
                    if( !*script->script_p )
                    {
                        return 0;
                    }
                    if( *script->script_p == '\n' )
                    {
                        script->line++;
                    }
                }
                while( !( *script->script_p == '*' && *( script->script_p + 1 ) == '/' ) );
                
                script->script_p++;
                
                if( !*script->script_p )
                {
                    return 0;
                }
                
                script->script_p++;
                
                if( !*script->script_p )
                {
                    return 0;
                }
                continue;
            }
        }
        break;
    }
    return 1;
}

/*
===============
idParseSystemLocal::ReadEscapeCharacter
===============
*/
sint idParseSystemLocal::ReadEscapeCharacter( script_t* script, valueType* ch )
{
    sint c, val, i;
    
    //step over the leading '\\'
    script->script_p++;
    
    //determine the escape character
    switch( *script->script_p )
    {
        case '\\':
            c = '\\';
            break;
        case 'n':
            c = '\n';
            break;
        case 'r':
            c = '\r';
            break;
        case 't':
            c = '\t';
            break;
        case 'v':
            c = '\v';
            break;
        case 'b':
            c = '\b';
            break;
        case 'f':
            c = '\f';
            break;
        case 'a':
            c = '\a';
            break;
        case '\'':
            c = '\'';
            break;
        case '\"':
            c = '\"';
            break;
        case '\?':
            c = '\?';
            break;
        case 'x':
        {
            script->script_p++;
            for( i = 0, val = 0; ; i++, script->script_p++ )
            {
                c = *script->script_p;
                
                if( c >= '0' && c <= '9' )
                {
                    c = c - '0';
                }
                else if( c >= 'A' && c <= 'Z' )
                {
                    c = c - 'A' + 10;
                }
                else if( c >= 'a' && c <= 'z' )
                {
                    c = c - 'a' + 10;
                }
                else
                {
                    break;
                }
                
                val = ( val << 4 ) + c;
            }
            
            script->script_p--;
            
            if( val > 0xFF )
            {
                ScriptWarning( script, "too large value in escape character" );
                val = 0xFF;
            }
            c = val;
            break;
        }
        default: //NOTE: decimal ASCII code, NOT octal
        {
            if( *script->script_p < '0' || *script->script_p > '9' )
            {
                ScriptError( script, "unknown escape valueType" );
            }
            
            for( i = 0, val = 0; ; i++, script->script_p++ )
            {
                c = *script->script_p;
                
                if( c >= '0' && c <= '9' )
                {
                    c = c - '0';
                }
                else
                {
                    break;
                }
                
                val = val * 10 + c;
            }
            
            script->script_p--;
            
            if( val > 0xFF )
            {
                ScriptWarning( script, "too large value in escape character" );
                val = 0xFF;
            }
            
            c = val;
            
            break;
        }
    }
    
    //step over the escape character or the last digit of the number
    script->script_p++;
    
    //store the escape character
    *ch = c;
    
    //succesfully read escape character
    return 1;
}

/*
===============
idParseSystemLocal::ReadString

Reads C-like string. Escape characters are interpretted.
Quotes are included with the string.
Reads two strings with a white space between them as one string.
===============
*/
sint idParseSystemLocal::ReadString( script_t* script, token_t* token, sint quote )
{
    sint len, tmpline;
    valueType* tmpscript_p;
    
    if( quote == '\"' )
    {
        token->type = TT_STRING;
    }
    else
    {
        token->type = TT_LITERAL;
    }
    
    len = 0;
    
    //leading quote
    token->string[len++] = *script->script_p++;
    
    //
    while( 1 )
    {
        //minus 2 because trailing double quote and zero have to be appended
        if( len >= MAX_TOKEN_CHARS - 2 )
        {
            ScriptError( script, "string longer than MAX_TOKEN_CHARS = %d", MAX_TOKEN_CHARS );
            return 0;
        }
        
        //if there is an escape character and
        //if escape characters inside a string are allowed
        if( *script->script_p == '\\' && !( script->flags & SCFL_NOSTRINGESCAPECHARS ) )
        {
            if( !ReadEscapeCharacter( script, &token->string[len] ) )
            {
                token->string[len] = 0;
                return 0;
            }
            
            len++;
        }
        //if a trailing quote
        else if( *script->script_p == quote )
        {
            //step over the double quote
            script->script_p++;
            
            //if white spaces in a string are not allowed
            if( script->flags & SCFL_NOSTRINGWHITESPACES )
            {
                break;
            }
            
            //
            tmpscript_p = script->script_p;
            tmpline = script->line;
            
            //read unusefull stuff between possible two following strings
            if( !ReadWhiteSpace( script ) )
            {
                script->script_p = tmpscript_p;
                script->line = tmpline;
                break;
            }
            
            //if there's no leading double qoute
            if( *script->script_p != quote )
            {
                script->script_p = tmpscript_p;
                script->line = tmpline;
                break;
            }
            
            //step over the new leading double quote
            script->script_p++;
        }
        else
        {
            if( *script->script_p == '\0' )
            {
                token->string[len] = 0;
                ScriptError( script, "missing trailing quote" );
                return 0;
            }
            if( *script->script_p == '\n' )
            {
                token->string[len] = 0;
                ScriptError( script, "newline inside string %s", token->string );
                return 0;
            }
            token->string[len++] = *script->script_p++;
        }
    }
    
    //trailing quote
    token->string[len++] = quote;
    
    //end string with a zero
    token->string[len] = '\0';
    
    //the sub type is the length of the string
    token->subtype = len;
    
    return 1;
}

/*
===============
idParseSystemLocal::ReadName
===============
*/
sint idParseSystemLocal::ReadName( script_t* script, token_t* token )
{
    sint len = 0;
    valueType c;
    
    token->type = TT_NAME;
    do
    {
        token->string[len++] = *script->script_p++;
        
        if( len >= MAX_TOKEN_CHARS )
        {
            ScriptError( script, "name longer than MAX_TOKEN_CHARS = %d", MAX_TOKEN_CHARS );
            return 0;
        }
        
        c = *script->script_p;
    }
    while( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) || c == '_' );
    
    token->string[len] = '\0';
    
    //the sub type is the length of the name
    token->subtype = len;
    
    return 1;
}

/*
===============
idParseSystemLocal::NumberValue
===============
*/
void idParseSystemLocal::NumberValue( valueType* string, sint subtype, uint32* intvalue, float64* floatvalue )
{
    uint32 dotfound = 0;
    
    *intvalue = 0;
    *floatvalue = 0;
    
    //floating point number
    if( subtype & TT_FLOAT )
    {
        while( *string )
        {
            if( *string == '.' )
            {
                if( dotfound )
                {
                    return;
                }
                
                dotfound = 10;
                
                string++;
            }
            
            if( dotfound )
            {
                *floatvalue = *floatvalue + ( float64 )( *string - '0' ) / ( float64 ) dotfound;
                dotfound *= 10;
            }
            else
            {
                *floatvalue = *floatvalue * 10.0 + ( float64 )( *string - '0' );
            }
            
            string++;
        }
        
        *intvalue = ( uint32 ) * floatvalue;
    }
    else if( subtype & TT_DECIMAL )
    {
        while( *string ) *intvalue = *intvalue * 10 + ( *string++ - '0' );
        
        *floatvalue = *intvalue;
    }
    else if( subtype & TT_HEX )
    {
        //step over the leading 0x or 0X
        string += 2;
        
        while( *string )
        {
            *intvalue <<= 4;
            
            if( *string >= 'a' && *string <= 'f' )
            {
                *intvalue += *string - 'a' + 10;
            }
            else if( *string >= 'A' && *string <= 'F' )
            {
                *intvalue += *string - 'A' + 10;
            }
            else
            {
                *intvalue += *string - '0';
            }
            string++;
        }
        
        *floatvalue = *intvalue;
    }
    else if( subtype & TT_OCTAL )
    {
        //step over the first zero
        string += 1;
        
        while( *string ) *intvalue = ( *intvalue << 3 ) + ( *string++ - '0' );
        
        *floatvalue = *intvalue;
    }
    else if( subtype & TT_BINARY )
    {
        //step over the leading 0b or 0B
        string += 2;
        
        while( *string ) *intvalue = ( *intvalue << 1 ) + ( *string++ - '0' );
        
        *floatvalue = *intvalue;
    }
}

/*
===============
idParseSystemLocal::ReadNumber
===============
*/
sint idParseSystemLocal::ReadNumber( script_t* script, token_t* token )
{
    sint len = 0, i;
    sint octal, dot;
    valueType c;
    
    token->type = TT_NUMBER;
    
    //check for a hexadecimal number
    if( *script->script_p == '0' && ( *( script->script_p + 1 ) == 'x' || *( script->script_p + 1 ) == 'X' ) )
    {
        token->string[len++] = *script->script_p++;
        token->string[len++] = *script->script_p++;
        
        c = *script->script_p;
        
        //hexadecimal
        while( ( c >= '0' && c <= '9' ) || ( c >= 'a' && c <= 'f' ) || ( c >= 'A' && c <= 'A' ) )
        {
            token->string[len++] = *script->script_p++;
            
            if( len >= MAX_TOKEN_CHARS )
            {
                ScriptError( script, "hexadecimal number longer than MAX_TOKEN_CHARS = %d", MAX_TOKEN_CHARS );
                return 0;
            }
            
            
            c = *script->script_p;
        }
        token->subtype |= TT_HEX;
    }
#ifdef BINARYNUMBERS
    //check for a binary number
    else if( *script->script_p == '0' && ( *( script->script_p + 1 ) == 'b' || *( script->script_p + 1 ) == 'B' ) )
    {
        token->string[len++] = *script->script_p++;
        token->string[len++] = *script->script_p++;
        c = *script->script_p;
        
        //binary
        while( c == '0' || c == '1' )
        {
            token->string[len++] = *script->script_p++;
            if( len >= MAX_TOKEN_CHARS )
            {
                ScriptError( script, "binary number longer than MAX_TOKEN_CHARS = %d", MAX_TOKEN_CHARS );
                return 0;
            }
            c = *script->script_p;
        }
        token->subtype |= TT_BINARY;
    }
#endif //BINARYNUMBERS
    //decimal or octal integer or floating point number
    else
    {
        octal = false;
        dot = false;
        
        if( *script->script_p == '0' )
        {
            octal = true;
        }
        
        while( 1 )
        {
            c = *script->script_p;
            if( c == '.' )
            {
                dot = true;
            }
            else if( c == '8' || c == '9' )
            {
                octal = false;
            }
            else if( c < '0' || c > '9' )
            {
                break;
            }
            
            token->string[len++] = *script->script_p++;
            
            if( len >= MAX_TOKEN_CHARS - 1 )
            {
                ScriptError( script, "number longer than MAX_TOKEN_CHARS = %d", MAX_TOKEN_CHARS );
                return 0;
            }
        }
        
        if( octal )
        {
            token->subtype |= TT_OCTAL;
        }
        else
        {
            token->subtype |= TT_DECIMAL;
        }
        
        if( dot )
        {
            token->subtype |= TT_FLOAT;
        }
    }
    
    for( i = 0; i < 2; i++ )
    {
        c = *script->script_p;
        
        //check for a LONG number
        if( ( c == 'l' || c == 'L' ) && !( token->subtype & TT_LONG ) )
        {
            script->script_p++;
            token->subtype |= TT_LONG;
        }
        //check for an UNSIGNED number
        else if( ( c == 'u' || c == 'U' ) && !( token->subtype & ( TT_UNSIGNED | TT_FLOAT ) ) )
        {
            script->script_p++;
            token->subtype |= TT_UNSIGNED;
        }
    }
    
    token->string[len] = '\0';
    
    NumberValue( token->string, token->subtype, &token->intvalue, &token->floatvalue );
    
    if( !( token->subtype & TT_FLOAT ) )
    {
        token->subtype |= TT_INTEGER;
    }
    
    return 1;
}

/*
===============
idParseSystemLocal::ReadPunctuation
===============
*/
sint idParseSystemLocal::ReadPunctuation( script_t* script, token_t* token )
{
    sint len;
    valueType* p;
    punctuation_t* punc;
    
    for( punc = script->punctuationtable[( uint ) * script->script_p]; punc; punc = punc->next )
    {
        p = punc->p;
        len = ( sint )::strlen( p );
        
        //if the script contains at least as much characters as the punctuation
        if( script->script_p + len <= script->end_p )
        {
            //if the script contains the punctuation
            if( !::strncmp( script->script_p, p, len ) )
            {
                ::strncpy( token->string, p, MAX_TOKEN_CHARS );
                script->script_p += len;
                token->type = TT_PUNCTUATION;
                
                //sub type is the number of the punctuation
                token->subtype = punc->n;
                return 1;
            }
        }
    }
    
    return 0;
}

/*
===============
idParseSystemLocal::ReadPrimitive
===============
*/
sint idParseSystemLocal::ReadPrimitive( script_t* script, token_t* token )
{
    sint len;
    
    len = 0;
    while( *script->script_p > ' ' && *script->script_p != ';' )
    {
        if( len >= MAX_TOKEN_CHARS )
        {
            ScriptError( script, "primitive token longer than MAX_TOKEN_CHARS = %d", MAX_TOKEN_CHARS );
            return 0;
        }
        
        token->string[len++] = *script->script_p++;
    }
    
    if( len >= MAX_TOKEN_CHARS )
    {
        // The last len++ made len==MAX_TOKEN_CHARS, which will overflow.
        // Bring it back down and ensure we null terminate.
        len = MAX_TOKEN_CHARS - 1;
    }
    
    token->string[len] = 0;
    
    //copy the token into the script structure
    ::memcpy( &script->token, token, sizeof( token_t ) );
    
    //primitive reading successfull
    return 1;
}

/*
===============
idParseSystemLocal::ReadScriptToken
===============
*/
sint idParseSystemLocal::ReadScriptToken( script_t* script, token_t* token )
{
    //if there is a token available (from UnreadToken)
    if( script->tokenavailable )
    {
        script->tokenavailable = 0;
        ::memcpy( token, &script->token, sizeof( token_t ) );
        return 1;
    }
    
    //save script pointer
    script->lastscript_p = script->script_p;
    
    //save line counter
    script->lastline = script->line;
    
    //clear the token stuff
    ::memset( token, 0, sizeof( token_t ) );
    
    //start of the white space
    script->whitespace_p = script->script_p;
    token->whitespace_p = script->script_p;
    
    //read unusefull stuff
    if( !ReadWhiteSpace( script ) )
    {
        return 0;
    }
    
    script->endwhitespace_p = script->script_p;
    token->endwhitespace_p = script->script_p;
    
    //line the token is on
    token->line = script->line;
    
    //number of lines crossed before token
    token->linescrossed = script->line - script->lastline;
    
    //if there is a leading double quote
    if( *script->script_p == '\"' )
    {
        if( !ReadString( script, token, '\"' ) )
        {
            return 0;
        }
    }
    //if an literal
    else if( *script->script_p == '\'' )
    {
        if( !ReadString( script, token, '\'' ) )
        {
            return 0;
        }
    }
    //if there is a number
    else if( ( *script->script_p >= '0' && *script->script_p <= '9' ) || ( *script->script_p == '.' &&
             ( *( script->script_p + 1 ) >= '0' && *( script->script_p + 1 ) <= '9' ) ) )
    {
        if( !ReadNumber( script, token ) )
        {
            return 0;
        }
    }
    //if this is a primitive script
    else if( script->flags & SCFL_PRIMITIVE )
    {
        return ReadPrimitive( script, token );
    }
    //if there is a name
    else if( ( *script->script_p >= 'a' && *script->script_p <= 'z' ) || ( *script->script_p >= 'A' && *script->script_p <= 'Z' )
             || *script->script_p == '_' )
    {
        if( !ReadName( script, token ) )
        {
            return 0;
        }
    }
    //check for punctuations
    else if( !ReadPunctuation( script, token ) )
    {
        ScriptError( script, "can't read token" );
        return 0;
    }
    
    //copy the token into the script structure
    ::memcpy( &script->token, token, sizeof( token_t ) );
    
    //succesfully read a token
    return 1;
}

/*
===============
idParseSystemLocal::StripDoubleQuotes
===============
*/
void idParseSystemLocal::StripDoubleQuotes( valueType* string )
{
    if( *string == '\"' )
    {
        ::memmove( string, string + 1, ::strlen( string ) + 1 );
    }
    
    if( string[::strlen( string ) - 1] == '\"' )
    {
        string[::strlen( string ) - 1] = '\0';
    }
}

/*
===============
idParseSystemLocal::EndOfScript
===============
*/
sint idParseSystemLocal::EndOfScript( script_t* script )
{
    return script->script_p >= script->end_p;
}

/*
===============
idParseSystemLocal::LoadScriptFile
===============
*/
script_t* idParseSystemLocal::LoadScriptFile( pointer filename )
{
    sint length;
    void* buffer;
    fileHandle_t fp;
    script_t* script;
    
    length = fileSystem->FOpenFileRead( filename, &fp, false );
    if( !fp )
    {
        return nullptr;
    }
    
    buffer = Z_Malloc( sizeof( script_t ) + length + 1 );
    ::memset( buffer, 0, sizeof( script_t ) + length + 1 );
    
    script = ( script_t* ) buffer;
    ::memset( script, 0, sizeof( script_t ) );
    Q_strcpy_s( script->filename, filename );
    script->buffer = ( valueType* ) buffer + sizeof( script_t );
    script->buffer[length] = 0;
    script->length = length;
    
    //pointer in script buffer
    script->script_p = script->buffer;
    
    //pointer in script buffer before reading token
    script->lastscript_p = script->buffer;
    
    //pointer to end of script buffer
    script->end_p = &script->buffer[length];
    
    //set if there's a token available in script->token
    script->tokenavailable = 0;
    
    //
    script->line = 1;
    script->lastline = 1;
    
    //
    SetScriptPunctuations( script, nullptr );
    
    //
    fileSystem->Read( script->buffer, length, fp );
    fileSystem->FCloseFile( fp );
    //
    
    return script;
}

/*
===============
idParseSystemLocal::LoadScriptMemory
===============
*/
script_t* idParseSystemLocal::LoadScriptMemory( valueType* ptr, sint length, valueType* name )
{
    void* buffer;
    script_t* script;
    
    buffer = Z_Malloc( sizeof( script_t ) + length + 1 );
    ::memset( buffer, 0, sizeof( script_t ) + length + 1 );
    
    script = ( script_t* ) buffer;
    ::memset( script, 0, sizeof( script_t ) );
    Q_strcpy_s( script->filename, name );
    script->buffer = ( valueType* ) buffer + sizeof( script_t );
    script->buffer[length] = 0;
    script->length = length;
    
    //pointer in script buffer
    script->script_p = script->buffer;
    
    //pointer in script buffer before reading token
    script->lastscript_p = script->buffer;
    
    //pointer to end of script buffer
    script->end_p = &script->buffer[length];
    
    //set if there's a token available in script->token
    script->tokenavailable = 0;
    
    //
    script->line = 1;
    script->lastline = 1;
    
    //
    SetScriptPunctuations( script, nullptr );
    
    //
    ::memcpy( script->buffer, ptr, length );
    
    //
    return script;
}

/*
===============
idParseSystemLocal::FreeScript
===============
*/
void idParseSystemLocal::FreeScript( script_t* script )
{
    if( script->punctuationtable )
    {
        Z_Free( script->punctuationtable );
    }
    
    Z_Free( script );
}

/*
===============
idParseSystemLocal::SourceError
===============
*/
void idParseSystemLocal::SourceError( source_t* source, valueType* str, ... )
{
    va_list argptr;
    valueType text[8192];
    
    va_start( argptr, str );
    Q_vsprintf_s( text, sizeof( text ), str, argptr );
    va_end( argptr );
    
    Com_Printf( "file %s, line %d: %s\n", source->scriptstack->filename, source->scriptstack->line, text );
}

/*
===============
idParseSystemLocal::SourceWarning
===============
*/
void idParseSystemLocal::SourceWarning( source_t* source, valueType* str, ... )
{
    va_list argptr;
    valueType text[8192];
    
    va_start( argptr, str );
    Q_vsprintf_s( text, sizeof( text ), str, argptr );
    va_end( argptr );
    
    Com_Printf( "file %s, line %d: %s\n", source->scriptstack->filename, source->scriptstack->line, text );
}

/*
===============
idParseSystemLocal::PushIndent
===============
*/
void idParseSystemLocal::PushIndent( source_t* source, sint type, sint skip )
{
    indent_t* indent;
    
    indent = ( indent_t* )Z_Malloc( sizeof( indent_t ) );
    indent->type = type;
    indent->script = source->scriptstack;
    indent->skip = ( skip != 0 );
    source->skip += indent->skip;
    indent->next = source->indentstack;
    source->indentstack = indent;
}

/*
===============
idParseSystemLocal::PopIndent
===============
*/
void idParseSystemLocal::PopIndent( source_t* source, sint* type, sint* skip )
{
    indent_t* indent;
    
    *type = 0;
    *skip = 0;
    
    indent = source->indentstack;
    if( !indent )
    {
        return;
    }
    
    //must be an indent from the current script
    if( source->indentstack->script != source->scriptstack )
    {
        return;
    }
    
    *type = indent->type;
    *skip = indent->skip;
    source->indentstack = source->indentstack->next;
    source->skip -= indent->skip;
    Z_Free( indent );
}

/*
===============
idParseSystemLocal::PushScript
===============
*/
void idParseSystemLocal::PushScript( source_t* source, script_t* script )
{
    script_t* s;
    
    for( s = source->scriptstack; s; s = s->next )
    {
        if( !Q_stricmp( s->filename, script->filename ) )
        {
            SourceError( source, "%s recursively included", script->filename );
            return;
        }
    }
    
    //push the script on the script stack
    script->next = source->scriptstack;
    source->scriptstack = script;
}

/*
===============
idParseSystemLocal::CopyToken
===============
*/
token_t* idParseSystemLocal::CopyToken( token_t* token )
{
    token_t* t;
    
    t = ( token_t* )Z_Malloc( sizeof( token_t ) );
    
    if( !t )
    {
        Com_Error( ERR_FATAL, "out of token space\n" );
        return nullptr;
    }
    
    ::memcpy( t, token, sizeof( token_t ) );
    t->next = nullptr;
    numtokens++;
    
    return t;
}

/*
===============
idParseSystemLocal::FreeToken
===============
*/
void idParseSystemLocal::FreeToken( token_t* token )
{
    Z_Free( token );
    numtokens--;
}

/*
===============
idParseSystemLocal::ReadSourceToken
===============
*/
sint idParseSystemLocal::ReadSourceToken( source_t* source, token_t* token )
{
    token_t* t;
    script_t* script;
    sint type, skip, lines;
    
    lines = 0;
    
    //if there's no token already available
    while( !source->tokens )
    {
        //if there's a token to read from the script
        if( ReadScriptToken( source->scriptstack, token ) )
        {
            token->linescrossed += lines;
            return true;
        }
        
        // if lines were crossed before the end of the script, count them
        lines += source->scriptstack->line - source->scriptstack->lastline;
        
        //if at the end of the script
        if( EndOfScript( source->scriptstack ) )
        {
            //remove all indents of the script
            while( source->indentstack && source->indentstack->script == source->scriptstack )
            {
                SourceWarning( source, "missing #endif" );
                PopIndent( source, &type, &skip );
            }
        }
        //if this was the initial script
        if( !source->scriptstack->next )
        {
            return false;
        }
        
        //remove the script and return to the last one
        script = source->scriptstack;
        source->scriptstack = source->scriptstack->next;
        FreeScript( script );
    }
    
    //copy the already available token
    ::memcpy( token, source->tokens, sizeof( token_t ) );
    
    //free the read token
    t = source->tokens;
    source->tokens = source->tokens->next;
    FreeToken( t );
    
    return true;
}

/*
===============
idParseSystemLocal::UnreadSourceToken
===============
*/
sint idParseSystemLocal::UnreadSourceToken( source_t* source, token_t* token )
{
    token_t* t;
    
    t = CopyToken( token );
    t->next = source->tokens;
    source->tokens = t;
    return true;
}

/*
===============
idParseSystemLocal::ReadDefineParms
===============
*/
sint idParseSystemLocal::ReadDefineParms( source_t* source, define_t* define, token_t** parms, sint maxparms )
{
    token_t token, *t, *last;
    sint i, done, lastcomma, numparms, indent;
    
    if( !ReadSourceToken( source, &token ) )
    {
        SourceError( source, "define %s missing parms", define->name );
        return false;
    }
    
    //
    if( define->numparms > maxparms )
    {
        SourceError( source, "define with more than %d parameters", maxparms );
        return false;
    }
    
    //
    for( i = 0; i < define->numparms; i++ )
    {
        parms[i] = nullptr;
    }
    
    //if no leading "("
    if( ::strcmp( token.string, "(" ) )
    {
        UnreadSourceToken( source, &token );
        SourceError( source, "define %s missing parms", define->name );
        return false;
    }
    
    //read the define parameters
    for( done = 0, numparms = 0, indent = 0; !done; )
    {
        if( numparms >= maxparms )
        {
            SourceError( source, "define %s with too many parms", define->name );
            return false;
        }
        
        if( numparms >= define->numparms )
        {
            SourceWarning( source, "define %s has too many parms", define->name );
            return false;
        }
        
        parms[numparms] = nullptr;
        lastcomma = 1;
        last = nullptr;
        
        while( !done )
        {
            //
            if( !ReadSourceToken( source, &token ) )
            {
                SourceError( source, "define %s incomplete", define->name );
                return false;
            }
            
            //
            if( !::strcmp( token.string, "," ) )
            {
                if( indent <= 0 )
                {
                    if( lastcomma )
                    {
                        SourceWarning( source, "too many comma's" );
                    }
                    
                    lastcomma = 1;
                    break;
                }
            }
            lastcomma = 0;
            
            //
            if( !::strcmp( token.string, "(" ) )
            {
                indent++;
                continue;
            }
            else if( !::strcmp( token.string, ")" ) )
            {
                if( --indent <= 0 )
                {
                    if( !parms[define->numparms - 1] )
                    {
                        SourceWarning( source, "too few define parms" );
                    }
                    
                    done = 1;
                    break;
                }
            }
            
            //
            if( numparms < define->numparms )
            {
                //
                t = CopyToken( &token );
                t->next = nullptr;
                
                if( last )
                {
                    last->next = t;
                }
                else
                {
                    parms[numparms] = t;
                }
                
                last = t;
            }
        }
        numparms++;
    }
    return true;
}

/*
===============
idParseSystemLocal::StringizeTokens
===============
*/
sint idParseSystemLocal::StringizeTokens( token_t* tokens, token_t* token )
{
    token_t* t;
    
    token->type = TT_STRING;
    token->whitespace_p = nullptr;
    token->endwhitespace_p = nullptr;
    token->string[0] = '\0';
    ::strcat( token->string, "\"" );
    
    for( t = tokens; t; t = t->next )
    {
        ::strncat( token->string, t->string, MAX_TOKEN_CHARS - ::strlen( token->string ) );
    }
    
    ::strncat( token->string, "\"", MAX_TOKEN_CHARS - ::strlen( token->string ) );
    
    return true;
}

/*
===============
idParseSystemLocal::MergeTokens
===============
*/
sint idParseSystemLocal::MergeTokens( token_t* t1, token_t* t2 )
{
    //merging of a name with a name or number
    if( t1->type == TT_NAME && ( t2->type == TT_NAME || t2->type == TT_NUMBER ) )
    {
        ::strcat( t1->string, t2->string );
        return true;
    }
    
    //merging of two strings
    if( t1->type == TT_STRING && t2->type == TT_STRING )
    {
        //remove trailing double quote
        t1->string[::strlen( t1->string ) - 1] = '\0';
        
        //concat without leading double quote
        strcat( t1->string, &t2->string[1] );
        
        return true;
    }
    
    //FIXME: merging of two number of the same sub type
    return false;
}

/*
===============
idParseSystemLocal::NameHash

//valueType primes[16] = {1, 3, 5, 7, 11, 13, 17, 19, 23, 27, 29, 31, 37, 41, 43, 47};
===============
*/
sint idParseSystemLocal::NameHash( valueType* name )
{
    sint hash, i;
    
    hash = 0;
    
    for( i = 0; name[i] != '\0'; i++ )
    {
        hash += name[i] * ( 119 + i );
    }
    
    hash = ( hash ^ ( hash >> 10 ) ^ ( hash >> 20 ) ) & ( DEFINEHASHSIZE - 1 );
    
    return hash;
}

/*
===============
idParseSystemLocal::AddDefineToHash
===============
*/
void idParseSystemLocal::AddDefineToHash( define_t* define, define_t** definehash )
{
    sint hash;
    
    hash = NameHash( define->name );
    define->hashnext = definehash[hash];
    definehash[hash] = define;
}

/*
===============
idParseSystemLocal::FindHashedDefine
===============
*/
define_t* idParseSystemLocal::FindHashedDefine( define_t** definehash, valueType* name )
{
    sint hash;
    define_t* d;
    
    hash = NameHash( name );
    
    for( d = definehash[hash]; d; d = d->hashnext )
    {
        if( !::strcmp( d->name, name ) )
        {
            return d;
        }
    }
    
    return nullptr;
}

/*
===============
idParseSystemLocal::FindDefineParm
===============
*/
sint idParseSystemLocal::FindDefineParm( define_t* define, valueType* name )
{
    sint i;
    token_t* p;
    
    i = 0;
    
    for( p = define->parms; p; p = p->next )
    {
        if( !::strcmp( p->string, name ) )
        {
            return i;
        }
        i++;
    }
    
    return -1;
}

/*
===============
idParseSystemLocal::FreeDefine
===============
*/
void idParseSystemLocal::FreeDefine( define_t* define )
{
    token_t* t, *next;
    
    //free the define parameters
    for( t = define->parms; t; t = next )
    {
        next = t->next;
        FreeToken( t );
    }
    
    //free the define tokens
    for( t = define->tokens; t; t = next )
    {
        next = t->next;
        FreeToken( t );
    }
    
    //free the define
    Z_Free( define );
}

/*
===============
idParseSystemLocal::ExpandBuiltinDefine
===============
*/
sint idParseSystemLocal::ExpandBuiltinDefine( source_t* source, token_t* deftoken, define_t* define, token_t** firsttoken, token_t** lasttoken )
{
    token_t* token;
    time_t t;
    
    valueType* curtime;
    
    token = CopyToken( deftoken );
    switch( define->builtin )
    {
        case BUILTIN_LINE:
        {
            ::sprintf( token->string, "%d", deftoken->line );
            token->intvalue = deftoken->line;
            token->floatvalue = deftoken->line;
            token->type = TT_NUMBER;
            token->subtype = TT_DECIMAL | TT_INTEGER;
            *firsttoken = token;
            *lasttoken = token;
            break;
        }
        case BUILTIN_FILE:
        {
            Q_strcpy_s( token->string, source->scriptstack->filename );
            token->type = TT_NAME;
            token->subtype = ( sint )::strlen( token->string );
            *firsttoken = token;
            *lasttoken = token;
            break;
        }
        case BUILTIN_DATE:
        {
            t = ::time( nullptr );
            curtime = ::ctime( &t );
            Q_strcpy_s( token->string, "\"" );
            ::strncat( token->string, curtime + 4, 7 );
            ::strncat( token->string + 7, curtime + 20, 4 );
            ::strcat( token->string, "\"" );
            ::free( curtime );
            token->type = TT_NAME;
            token->subtype = ( sint )::strlen( token->string );
            *firsttoken = token;
            *lasttoken = token;
            break;
        }
        case BUILTIN_TIME:
        {
            t = ::time( nullptr );
            curtime = ::ctime( &t );
            Q_strcpy_s( token->string, "\"" );
            ::strncat( token->string, curtime + 11, 8 );
            ::strcat( token->string, "\"" );
            ::free( curtime );
            token->type = TT_NAME;
            token->subtype = ( sint )::strlen( token->string );
            *firsttoken = token;
            *lasttoken = token;
            break;
        }
        case BUILTIN_STDC:
        default:
        {
            *firsttoken = nullptr;
            *lasttoken = nullptr;
            break;
        }
    }
    return true;
}

/*
===============
idParseSystemLocal::ExpandDefine
===============
*/
sint idParseSystemLocal::ExpandDefine( source_t* source, token_t* deftoken, define_t* define, token_t** firsttoken, token_t** lasttoken )
{
    sint parmnum, i;
    token_t* parms[MAX_DEFINEPARMS], *dt, *pt, *t;
    token_t* t1, *t2, *first, *last, *nextpt, token;
    
    //if it is a builtin define
    if( define->builtin )
    {
        return ExpandBuiltinDefine( source, deftoken, define, firsttoken, lasttoken );
    }
    
    //if the define has parameters
    if( define->numparms )
    {
        if( !ReadDefineParms( source, define, parms, MAX_DEFINEPARMS ) ) return false;
    }
    
    //empty list at first
    first = nullptr;
    last = nullptr;
    
    //create a list with tokens of the expanded define
    for( dt = define->tokens; dt; dt = dt->next )
    {
        parmnum = -1;
        
        //if the token is a name, it could be a define parameter
        if( dt->type == TT_NAME )
        {
            parmnum = FindDefineParm( define, dt->string );
        }
        
        //if it is a define parameter
        if( parmnum >= 0 )
        {
            for( pt = parms[parmnum]; pt; pt = pt->next )
            {
                t = CopyToken( pt );
                
                //add the token to the list
                t->next = nullptr;
                if( last )
                {
                    last->next = t;
                }
                else
                {
                    first = t;
                }
                last = t;
            }
        }
        else
        {
            //if stringizing operator
            if( dt->string[0] == '#' && dt->string[1] == '\0' )
            {
                //the stringizing operator must be followed by a define parameter
                if( dt->next )
                {
                    parmnum = FindDefineParm( define, dt->next->string );
                }
                else
                {
                    parmnum = -1;
                }
                
                //
                if( parmnum >= 0 )
                {
                    //step over the stringizing operator
                    dt = dt->next;
                    
                    //stringize the define parameter tokens
                    if( !StringizeTokens( parms[parmnum], &token ) )
                    {
                        SourceError( source, "can't stringize tokens" );
                        return false;
                    }
                    t = CopyToken( &token );
                }
                else
                {
                    SourceWarning( source, "stringizing operator without define parameter" );
                    continue;
                }
            }
            else
            {
                t = CopyToken( dt );
            }
            
            //add the token to the list
            t->next = nullptr;
            
            if( last )
            {
                last->next = t;
            }
            else
            {
                first = t;
            }
            
            last = t;
        }
    }
    //check for the merging operator
    for( t = first; t; )
    {
        if( t->next )
        {
            //if the merging operator
            if( t->next->string[0] == '#' && t->next->string[1] == '#' )
            {
                t1 = t;
                t2 = t->next->next;
                
                if( t2 )
                {
                    if( !MergeTokens( t1, t2 ) )
                    {
                        SourceError( source, "can't merge %s with %s", t1->string, t2->string );
                        return false;
                    }
                    
                    FreeToken( t1->next );
                    
                    t1->next = t2->next;
                    if( t2 == last )
                    {
                        last = t1;
                    }
                    
                    FreeToken( t2 );
                    continue;
                }
            }
        }
        
        t = t->next;
    }
    
    //store the first and last token of the list
    *firsttoken = first;
    *lasttoken = last;
    
    //free all the parameter tokens
    for( i = 0; i < define->numparms; i++ )
    {
        for( pt = parms[i]; pt; pt = nextpt )
        {
            nextpt = pt->next;
            FreeToken( pt );
        }
    }
    
    //
    return true;
}

/*
===============
idParseSystemLocal::ExpandDefineIntoSource
===============
*/
sint idParseSystemLocal::ExpandDefineIntoSource( source_t* source, token_t* deftoken, define_t* define )
{
    token_t* firsttoken, *lasttoken;
    
    if( !ExpandDefine( source, deftoken, define, &firsttoken, &lasttoken ) )
    {
        return false;
    }
    
    if( firsttoken && lasttoken )
    {
        lasttoken->next = source->tokens;
        source->tokens = firsttoken;
        return true;
    }
    
    return false;
}

/*
===============
idParseSystemLocal::ConvertPath
===============
*/
void idParseSystemLocal::ConvertPath( valueType* path )
{
    valueType* ptr;
    
    //remove double path seperators
    for( ptr = path; *ptr; )
    {
        if( ( *ptr == '\\' || *ptr == '/' ) && ( *( ptr + 1 ) == '\\' || *( ptr + 1 ) == '/' ) )
        {
            ::memmove( ptr, ptr + 1, ::strlen( ptr ) );
        }
        else
        {
            ptr++;
        }
    }
    
    //set OS dependent path seperators
    for( ptr = path; *ptr; )
    {
        if( *ptr == '/' || *ptr == '\\' )
        {
            *ptr = PATH_SEP;
        }
        ptr++;
    }
}

/*
===============
idParseSystemLocal::ReadLine

reads a token from the current line, continues reading on the next
line only if a backslash '\' is encountered.
===============
*/
sint idParseSystemLocal::ReadLine( source_t* source, token_t* token )
{
    sint crossline;
    
    crossline = 0;
    do
    {
        if( !ReadSourceToken( source, token ) )
        {
            return false;
        }
        
        if( token->linescrossed > crossline )
        {
            UnreadSourceToken( source, token );
            return false;
        }
        
        crossline = 1;
    }
    while( !::strcmp( token->string, "\\" ) );
    
    return true;
}

/*
===============
idParseSystemLocal::OperatorPriority
===============
*/
sint idParseSystemLocal::OperatorPriority( sint op )
{
    switch( op )
    {
        case P_MUL:
            return 15;
        case P_DIV:
            return 15;
        case P_MOD:
            return 15;
        case P_ADD:
            return 14;
        case P_SUB:
            return 14;
            
        case P_LOGIC_AND:
            return 7;
        case P_LOGIC_OR:
            return 6;
        case P_LOGIC_GEQ:
            return 12;
        case P_LOGIC_LEQ:
            return 12;
        case P_LOGIC_EQ:
            return 11;
        case P_LOGIC_UNEQ:
            return 11;
            
        case P_LOGIC_NOT:
            return 16;
        case P_LOGIC_GREATER:
            return 12;
        case P_LOGIC_LESS:
            return 12;
            
        case P_RSHIFT:
            return 13;
        case P_LSHIFT:
            return 13;
            
        case P_BIN_AND:
            return 10;
        case P_BIN_OR:
            return 8;
        case P_BIN_XOR:
            return 9;
        case P_BIN_NOT:
            return 16;
            
        case P_COLON:
            return 5;
        case P_QUESTIONMARK:
            return 5;
    }
    return false;
}

/*
===============
idParseSystemLocal::EvaluateTokens
===============
*/
sint idParseSystemLocal::EvaluateTokens( source_t* source, token_t* tokens, sint32* intvalue, float64* floatvalue, sint integer )
{
    operator_t* o, *firstoperator, *lastoperator;
    value_t* v, *firstvalue, *lastvalue, *v1, *v2;
    token_t* t;
    sint brace = 0;
    sint parentheses = 0;
    sint error = 0;
    sint lastwasvalue = 0;
    sint negativevalue = 0;
    sint questmarkintvalue = 0;
    float64 questmarkfloatvalue = 0;
    sint gotquestmarkvalue = false;
    sint lastoperatortype = 0;
    //
    operator_t operator_heap[MAX_OPERATORS];
    sint numoperators = 0;
    value_t value_heap[MAX_VALUES];
    sint numvalues = 0;
    
    firstoperator = lastoperator = nullptr;
    firstvalue = lastvalue = nullptr;
    
    if( intvalue )
    {
        *intvalue = 0;
    }
    
    if( floatvalue )
    {
        *floatvalue = 0;
    }
    
    for( t = tokens; t; t = t->next )
    {
        switch( t->type )
        {
            case TT_NAME:
            {
                if( lastwasvalue || negativevalue )
                {
                    SourceError( source, "syntax error in #if/#elif" );
                    error = 1;
                    break;
                }
                
                if( ::strcmp( t->string, "defined" ) )
                {
                    SourceError( source, "undefined name %s in #if/#elif", t->string );
                    error = 1;
                    break;
                }
                
                t = t->next;
                
                if( !::strcmp( t->string, "(" ) )
                {
                    brace = true;
                    t = t->next;
                }
                
                if( !t || t->type != TT_NAME )
                {
                    SourceError( source, "defined without name in #if/#elif" );
                    error = 1;
                    break;
                }
                
                AllocValue( v );
                
                if( FindHashedDefine( source->definehash, t->string ) )
                {
                    v->intvalue = 1;
                    v->floatvalue = 1;
                }
                else
                {
                    v->intvalue = 0;
                    v->floatvalue = 0;
                }
                
                v->parentheses = parentheses;
                v->next = nullptr;
                v->prev = lastvalue;
                
                if( lastvalue )
                {
                    lastvalue->next = v;
                }
                else
                {
                    firstvalue = v;
                }
                
                lastvalue = v;
                
                if( brace )
                {
                    t = t->next;
                    
                    if( !t || ::strcmp( t->string, ")" ) )
                    {
                        SourceError( source, "defined without ) in #if/#elif" );
                        error = 1;
                        break;
                    }
                }
                
                brace = false;
                
                // defined() creates a value
                lastwasvalue = 1;
                break;
            }
            case TT_NUMBER:
            {
                if( lastwasvalue )
                {
                    SourceError( source, "syntax error in #if/#elif" );
                    error = 1;
                    break;
                }
                
                AllocValue( v );
                
                if( negativevalue )
                {
                    v->intvalue = - ( sint ) t->intvalue;
                    v->floatvalue = - t->floatvalue;
                }
                else
                {
                    v->intvalue = t->intvalue;
                    v->floatvalue = t->floatvalue;
                }
                
                v->parentheses = parentheses;
                v->next = nullptr;
                v->prev = lastvalue;
                
                if( lastvalue )
                {
                    lastvalue->next = v;
                }
                else
                {
                    firstvalue = v;
                }
                
                lastvalue = v;
                
                //last token was a value
                lastwasvalue = 1;
                
                //
                negativevalue = 0;
                break;
            }
            case TT_PUNCTUATION:
            {
                if( negativevalue )
                {
                    SourceError( source, "misplaced minus sign in #if/#elif" );
                    error = 1;
                    break;
                }
                
                if( t->subtype == P_PARENTHESESOPEN )
                {
                    parentheses++;
                    break;
                }
                else if( t->subtype == P_PARENTHESESCLOSE )
                {
                    parentheses--;
                    
                    if( parentheses < 0 )
                    {
                        SourceError( source, "too many ) in #if/#elsif" );
                        error = 1;
                    }
                    break;
                }
                
                //check for invalid operators on floating point values
                if( !integer )
                {
                    if( t->subtype == P_BIN_NOT || t->subtype == P_MOD || t->subtype == P_RSHIFT || t->subtype == P_LSHIFT ||
                            t->subtype == P_BIN_AND || t->subtype == P_BIN_OR || t->subtype == P_BIN_XOR )
                    {
                        SourceError( source, "illigal operator %s on floating point operands\n", t->string );
                        error = 1;
                        break;
                    }
                }
                switch( t->subtype )
                {
                    case P_LOGIC_NOT:
                    case P_BIN_NOT:
                    {
                        if( lastwasvalue )
                        {
                            SourceError( source, "! or ~ after value in #if/#elif" );
                            error = 1;
                            break;
                        }
                        break;
                    }
                    case P_INC:
                    case P_DEC:
                    {
                        SourceError( source, "++ or -- used in #if/#elif" );
                        break;
                    }
                    case P_SUB:
                    {
                        if( !lastwasvalue )
                        {
                            negativevalue = 1;
                            break;
                        }
                    }
                    
                    case P_MUL:
                    case P_DIV:
                    case P_MOD:
                    case P_ADD:
                    
                    case P_LOGIC_AND:
                    case P_LOGIC_OR:
                    case P_LOGIC_GEQ:
                    case P_LOGIC_LEQ:
                    case P_LOGIC_EQ:
                    case P_LOGIC_UNEQ:
                    
                    case P_LOGIC_GREATER:
                    case P_LOGIC_LESS:
                    
                    case P_RSHIFT:
                    case P_LSHIFT:
                    
                    case P_BIN_AND:
                    case P_BIN_OR:
                    case P_BIN_XOR:
                    
                    case P_COLON:
                    case P_QUESTIONMARK:
                    {
                        if( !lastwasvalue )
                        {
                            SourceError( source, "operator %s after operator in #if/#elif", t->string );
                            error = 1;
                            break;
                        }
                        break;
                    }
                    default:
                    {
                        SourceError( source, "invalid operator %s in #if/#elif", t->string );
                        error = 1;
                        break;
                    }
                }
                if( !error && !negativevalue )
                {
                    AllocOperator( o );
                    
                    o->_operator = t->subtype;
                    o->priority = OperatorPriority( t->subtype );
                    o->parentheses = parentheses;
                    o->next = nullptr;
                    o->prev = lastoperator;
                    
                    if( lastoperator )
                    {
                        lastoperator->next = o;
                    }
                    else
                    {
                        firstoperator = o;
                    }
                    
                    lastoperator = o;
                    lastwasvalue = 0;
                }
                break;
            }
            default:
            {
                SourceError( source, "unknown %s in #if/#elif", t->string );
                error = 1;
                break;
            }
        }
        
        if( error )
        {
            break;
        }
    }
    if( !error )
    {
        if( !lastwasvalue )
        {
            SourceError( source, "trailing operator in #if/#elif" );
            error = 1;
        }
        else if( parentheses )
        {
            SourceError( source, "too many ( in #if/#elif" );
            error = 1;
        }
    }
    
    //
    gotquestmarkvalue = false;
    questmarkintvalue = 0;
    questmarkfloatvalue = 0;
    
    //while there are operators
    while( !error && firstoperator )
    {
        v = firstvalue;
        for( o = firstoperator; o->next; o = o->next )
        {
            //if the current operator is nested deeper in parentheses
            //than the next operator
            if( o->parentheses > o->next->parentheses )
            {
                break;
            }
            
            //if the current and next operator are nested equally deep in parentheses
            if( o->parentheses == o->next->parentheses )
            {
                //if the priority of the current operator is equal or higher
                //than the priority of the next operator
                if( o->priority >= o->next->priority ) break;
            }
            
            //if the arity of the operator isn't equal to 1
            if( o->_operator != P_LOGIC_NOT && o->_operator != P_BIN_NOT )
            {
                v = v->next;
            }
            
            //if there's no value or no next value
            if( !v )
            {
                SourceError( source, "mising values in #if/#elif" );
                error = 1;
                break;
            }
        }
        
        if( error )
        {
            break;
        }
        
        v1 = v;
        v2 = v->next;
        
        switch( o->_operator )
        {
            case P_LOGIC_NOT:
                v1->intvalue = !v1->intvalue;
                v1->floatvalue = !v1->floatvalue;
                break;
            case P_BIN_NOT:
                v1->intvalue = ~v1->intvalue;
                break;
            case P_MUL:
                v1->intvalue *= v2->intvalue;
                v1->floatvalue *= v2->floatvalue;
                break;
            case P_DIV:
                if( !v2->intvalue || !v2->floatvalue )
                {
                    SourceError( source, "divide by zero in #if/#elif\n" );
                    error = 1;
                    break;
                }
                v1->intvalue /= v2->intvalue;
                v1->floatvalue /= v2->floatvalue;
                break;
            case P_MOD:
                if( !v2->intvalue )
                {
                    SourceError( source, "divide by zero in #if/#elif\n" );
                    error = 1;
                    break;
                }
                v1->intvalue %= v2->intvalue;
                break;
            case P_ADD:
                v1->intvalue += v2->intvalue;
                v1->floatvalue += v2->floatvalue;
                break;
            case P_SUB:
                v1->intvalue -= v2->intvalue;
                v1->floatvalue -= v2->floatvalue;
                break;
            case P_LOGIC_AND:
                v1->intvalue = v1->intvalue && v2->intvalue;
                v1->floatvalue = v1->floatvalue && v2->floatvalue;
                break;
            case P_LOGIC_OR:
                v1->intvalue = v1->intvalue || v2->intvalue;
                v1->floatvalue = v1->floatvalue || v2->floatvalue;
                break;
            case P_LOGIC_GEQ:
                v1->intvalue = v1->intvalue >= v2->intvalue;
                v1->floatvalue = v1->floatvalue >= v2->floatvalue;
                break;
            case P_LOGIC_LEQ:
                v1->intvalue = v1->intvalue <= v2->intvalue;
                v1->floatvalue = v1->floatvalue <= v2->floatvalue;
                break;
            case P_LOGIC_EQ:
                v1->intvalue = v1->intvalue == v2->intvalue;
                v1->floatvalue = v1->floatvalue == v2->floatvalue;
                break;
            case P_LOGIC_UNEQ:
                v1->intvalue = v1->intvalue != v2->intvalue;
                v1->floatvalue = v1->floatvalue != v2->floatvalue;
                break;
            case P_LOGIC_GREATER:
                v1->intvalue = v1->intvalue > v2->intvalue;
                v1->floatvalue = v1->floatvalue > v2->floatvalue;
                break;
            case P_LOGIC_LESS:
                v1->intvalue = v1->intvalue < v2->intvalue;
                v1->floatvalue = v1->floatvalue < v2->floatvalue;
                break;
            case P_RSHIFT:
                v1->intvalue >>= v2->intvalue;
                break;
            case P_LSHIFT:
                v1->intvalue <<= v2->intvalue;
                break;
            case P_BIN_AND:
                v1->intvalue &= v2->intvalue;
                break;
            case P_BIN_OR:
                v1->intvalue |= v2->intvalue;
                break;
            case P_BIN_XOR:
                v1->intvalue ^= v2->intvalue;
                break;
            case P_COLON:
            {
                if( !gotquestmarkvalue )
                {
                    SourceError( source, ": without ? in #if/#elif" );
                    error = 1;
                    break;
                }
                
                if( integer )
                {
                    if( !questmarkintvalue )
                    {
                        v1->intvalue = v2->intvalue;
                    }
                }
                else
                {
                    if( !questmarkfloatvalue )
                    {
                        v1->floatvalue = v2->floatvalue;
                    }
                }
                gotquestmarkvalue = false;
                break;
            }
            case P_QUESTIONMARK:
            {
                if( gotquestmarkvalue )
                {
                    SourceError( source, "? after ? in #if/#elif" );
                    error = 1;
                    break;
                }
                
                questmarkintvalue = v1->intvalue;
                questmarkfloatvalue = v1->floatvalue;
                gotquestmarkvalue = true;
                break;
            }
        }
        
        if( error )
        {
            break;
        }
        
        lastoperatortype = o->_operator;
        
        //if not an operator with arity 1
        if( o->_operator != P_LOGIC_NOT && o->_operator != P_BIN_NOT )
        {
            //remove the second value if not question mark operator
            if( o->_operator != P_QUESTIONMARK )
            {
                v = v->next;
            }
            
            //
            if( v->prev )
            {
                v->prev->next = v->next;
            }
            else
            {
                firstvalue = v->next;
            }
            
            if( v->next )
            {
                v->next->prev = v->prev;
            }
            else
            {
                lastvalue = v->prev;
            }
            
            FreeValue( v );
        }
        //remove the operator
        if( o->prev )
        {
            o->prev->next = o->next;
        }
        else
        {
            firstoperator = o->next;
        }
        
        if( o->next )
        {
            o->next->prev = o->prev;
        }
        else
        {
            lastoperator = o->prev;
        }
        
        FreeOperator( o );
    }
    
    if( firstvalue )
    {
        if( intvalue )
        {
            *intvalue = firstvalue->intvalue;
        }
        
        if( floatvalue )
        {
            *floatvalue = firstvalue->floatvalue;
        }
    }
    
    for( o = firstoperator; o; o = lastoperator )
    {
        lastoperator = o->next;
        FreeOperator( o );
    }
    
    for( v = firstvalue; v; v = lastvalue )
    {
        lastvalue = v->next;
        FreeValue( v );
    }
    
    if( !error )
    {
        return true;
    }
    
    if( intvalue )
    {
        *intvalue = 0;
    }
    
    if( floatvalue )
    {
        *floatvalue = 0;
    }
    
    return false;
}

/*
===============
idParseSystemLocal::Evaluate
===============
*/
sint idParseSystemLocal::Evaluate( source_t* source, sint32* intvalue, float64* floatvalue, sint integer )
{
    sint defined = false;
    token_t token, *firsttoken, *lasttoken;
    token_t* t, *nexttoken;
    define_t* define;
    
    if( intvalue )
    {
        *intvalue = 0;
    }
    
    if( floatvalue )
    {
        *floatvalue = 0;
    }
    
    //
    if( !ReadLine( source, &token ) )
    {
        SourceError( source, "no value after #if/#elif" );
        return false;
    }
    
    firsttoken = nullptr;
    lasttoken = nullptr;
    do
    {
        //if the token is a name
        if( token.type == TT_NAME )
        {
            if( defined )
            {
                defined = false;
                
                t = CopyToken( &token );
                t->next = nullptr;
                
                if( lasttoken )
                {
                    lasttoken->next = t;
                }
                else
                {
                    firsttoken = t;
                }
                
                lasttoken = t;
            }
            else if( !::strcmp( token.string, "defined" ) )
            {
                defined = true;
                t = CopyToken( &token );
                t->next = nullptr;
                
                if( lasttoken )
                {
                    lasttoken->next = t;
                }
                else
                {
                    firsttoken = t;
                }
                
                lasttoken = t;
            }
            else
            {
                //then it must be a define
                define = FindHashedDefine( source->definehash, token.string );
                if( !define )
                {
                    FreeToken( firsttoken );
                    
                    SourceError( source, "can't evaluate %s, not defined", token.string );
                    return false;
                }
                
                if( !ExpandDefineIntoSource( source, &token, define ) )
                {
                    FreeToken( firsttoken );
                    return false;
                }
            }
        }
        //if the token is a number or a punctuation
        else if( token.type == TT_NUMBER || token.type == TT_PUNCTUATION )
        {
            t = CopyToken( &token );
            t->next = nullptr;
            
            if( lasttoken )
            {
                lasttoken->next = t;
            }
            else
            {
                firsttoken = t;
            }
            
            lasttoken = t;
        }
        //can't evaluate the token
        else
        {
            FreeToken( firsttoken );
            SourceError( source, "can't evaluate %s", token.string );
            return false;
        }
    }
    while( ReadLine( source, &token ) );
    
    //
    if( !EvaluateTokens( source, firsttoken, intvalue, floatvalue, integer ) )
    {
        FreeToken( firsttoken );
        return false;
    }
    
    //
    FreeToken( firsttoken );
    
    //
    return true;
}

/*
===============
idParseSystemLocal::DollarEvaluate
===============
*/
sint idParseSystemLocal::DollarEvaluate( source_t* source, sint32* intvalue, float64* floatvalue, sint integer )
{
    sint indent, defined = false;
    token_t token, *firsttoken, *lasttoken;
    token_t* t, *nexttoken;
    define_t* define;
    
    if( intvalue )
    {
        *intvalue = 0;
    }
    
    if( floatvalue )
    {
        *floatvalue = 0;
    }
    
    //
    if( !ReadSourceToken( source, &token ) )
    {
        SourceError( source, "no leading ( after $evalint/$evalfloat" );
        return false;
    }
    
    if( !ReadSourceToken( source, &token ) )
    {
        SourceError( source, "nothing to evaluate" );
        return false;
    }
    
    indent = 1;
    firsttoken = nullptr;
    lasttoken = nullptr;
    do
    {
        //if the token is a name
        if( token.type == TT_NAME )
        {
            if( defined )
            {
                defined = false;
                t = CopyToken( &token );
                t->next = nullptr;
                
                if( lasttoken )
                {
                    lasttoken->next = t;
                }
                else
                {
                    firsttoken = t;
                }
                
                lasttoken = t;
            }
            else if( !::strcmp( token.string, "defined" ) )
            {
                defined = true;
                
                t = CopyToken( &token );
                t->next = nullptr;
                
                if( lasttoken )
                {
                    lasttoken->next = t;
                }
                else
                {
                    firsttoken = t;
                }
                
                lasttoken = t;
            }
            else
            {
                //then it must be a define
                define = FindHashedDefine( source->definehash, token.string );
                
                if( !define )
                {
                    SourceError( source, "can't evaluate %s, not defined", token.string );
                    return false;
                }
                
                if( !ExpandDefineIntoSource( source, &token, define ) )
                {
                    return false;
                }
            }
        }
        //if the token is a number or a punctuation
        else if( token.type == TT_NUMBER || token.type == TT_PUNCTUATION )
        {
            if( *token.string == '(' )
            {
                indent++;
            }
            else if( *token.string == ')' )
            {
                indent--;
            }
            
            if( indent <= 0 )
            {
                break;
            }
            
            t = CopyToken( &token );
            t->next = nullptr;
            
            if( lasttoken )
            {
                lasttoken->next = t;
            }
            else
            {
                firsttoken = t;
            }
            
            lasttoken = t;
        }
        //can't evaluate the token
        else
        {
            SourceError( source, "can't evaluate %s", token.string );
            return false;
        }
    }
    while( ReadSourceToken( source, &token ) );
    
    //
    if( !EvaluateTokens( source, firsttoken, intvalue, floatvalue, integer ) )
    {
        return false;
    }
    
    //
    for( t = firsttoken; t; t = nexttoken )
    {
        nexttoken = t->next;
        FreeToken( t );
    }
    
    //
    return true;
}

/*
===============
idParseSystemLocal::Directive_include
===============
*/
sint idParseSystemLocal::Directive_include( source_t* source )
{
    valueType path[MAX_QPATH];
    script_t* script;
    token_t token;
    
    if( source->skip > 0 )
    {
        return true;
    }
    
    //
    if( !ReadSourceToken( source, &token ) )
    {
        SourceError( source, "#include without file name" );
        return false;
    }
    
    if( token.linescrossed > 0 )
    {
        SourceError( source, "#include without file name" );
        return false;
    }
    
    if( token.type == TT_STRING )
    {
        StripDoubleQuotes( token.string );
        ConvertPath( token.string );
        
        script = LoadScriptFile( token.string );
        if( !script )
        {
            Q_strcpy_s( path, source->includepath );
            ::strcat( path, token.string );
            script = LoadScriptFile( path );
        }
    }
    else if( token.type == TT_PUNCTUATION && *token.string == '<' )
    {
        Q_strcpy_s( path, source->includepath );
        
        while( ReadSourceToken( source, &token ) )
        {
            if( token.linescrossed > 0 )
            {
                UnreadSourceToken( source, &token );
                break;
            }
            
            if( token.type == TT_PUNCTUATION && *token.string == '>' )
            {
                break;
            }
            
            ::strncat( path, token.string, MAX_QPATH - 1 );
        }
        
        if( *token.string != '>' )
        {
            SourceWarning( source, "#include missing trailing >" );
        }
        
        if( !::strlen( path ) )
        {
            SourceError( source, "#include without file name between < >" );
            return false;
        }
        
        ConvertPath( path );
        script = LoadScriptFile( path );
    }
    else
    {
        SourceError( source, "#include without file name" );
        return false;
    }
    
    if( !script )
    {
        SourceError( source, "file %s not found", path );
        return false;
    }
    
    PushScript( source, script );
    return true;
}

/*
===============
idParseSystemLocal::WhiteSpaceBeforeToken
===============
*/
sint idParseSystemLocal::WhiteSpaceBeforeToken( token_t* token )
{
    return token->endwhitespace_p - token->whitespace_p > 0;
}

/*
===============
idParseSystemLocal::ClearTokenWhiteSpace
===============
*/
void idParseSystemLocal::ClearTokenWhiteSpace( token_t* token )
{
    token->whitespace_p = nullptr;
    token->endwhitespace_p = nullptr;
    token->linescrossed = 0;
}

/*
===============
idParseSystemLocal::Directive_undef
===============
*/
sint idParseSystemLocal::Directive_undef( source_t* source )
{
    sint hash;
    token_t token;
    define_t* define, *lastdefine;
    
    if( source->skip > 0 )
    {
        return true;
    }
    
    //
    if( !ReadLine( source, &token ) )
    {
        SourceError( source, "undef without name" );
        return false;
    }
    
    if( token.type != TT_NAME )
    {
        UnreadSourceToken( source, &token );
        SourceError( source, "expected name, found %s", token.string );
        return false;
    }
    
    hash = NameHash( token.string );
    for( lastdefine = nullptr, define = source->definehash[hash]; define; define = define->hashnext )
    {
        if( !::strcmp( define->name, token.string ) )
        {
            if( define->flags & DEFINE_FIXED )
            {
                SourceWarning( source, "can't undef %s", token.string );
            }
            else
            {
                if( lastdefine )
                {
                    lastdefine->hashnext = define->hashnext;
                }
                else
                {
                    source->definehash[hash] = define->hashnext;
                }
                
                FreeDefine( define );
            }
            break;
        }
        
        lastdefine = define;
    }
    
    return true;
}

/*
===============
idParseSystemLocal::Directive_elif
===============
*/
sint idParseSystemLocal::Directive_elif( source_t* source )
{
    sint type, skip;
    sint32 value;
    
    PopIndent( source, &type, &skip );
    
    if( !type || type == INDENT_ELSE )
    {
        SourceError( source, "misplaced #elif" );
        return false;
    }
    
    if( !Evaluate( source, &value, nullptr, true ) )
    {
        return false;
    }
    
    skip = ( value == 0 );
    PushIndent( source, INDENT_ELIF, skip );
    return true;
}

/*
===============
idParseSystemLocal::Directive_if
===============
*/
sint idParseSystemLocal::Directive_if( source_t* source )
{
    sint skip;
    sint32 value;
    
    if( !Evaluate( source, &value, nullptr, true ) )
    {
        return false;
    }
    
    skip = ( value == 0 );
    PushIndent( source, INDENT_IF, skip );
    return true;
}

/*
===============
idParseSystemLocal::Directive_line
===============
*/
sint idParseSystemLocal::Directive_line( source_t* source )
{
    SourceError( source, "#line directive not supported" );
    return false;
}

/*
===============
idParseSystemLocal::Directive_error
===============
*/
sint idParseSystemLocal::Directive_error( source_t* source )
{
    token_t token;
    
    Q_strcpy_s( token.string, "" );
    ReadSourceToken( source, &token );
    SourceError( source, "#error directive: %s", token.string );
    return false;
}

/*
===============
idParseSystemLocal::Directive_pragma
===============
*/
sint idParseSystemLocal::Directive_pragma( source_t* source )
{
    token_t token;
    
    SourceWarning( source, "#pragma directive not supported" );
    
    while( ReadLine( source, &token ) ) ;
    
    return true;
}

/*
===============
idParseSystemLocal::UnreadSignToken
===============
*/
void idParseSystemLocal::UnreadSignToken( source_t* source )
{
    token_t token;
    
    token.line = source->scriptstack->line;
    token.whitespace_p = source->scriptstack->script_p;
    token.endwhitespace_p = source->scriptstack->script_p;
    token.linescrossed = 0;
    Q_strcpy_s( token.string, "-" );
    token.type = TT_PUNCTUATION;
    token.subtype = P_SUB;
    UnreadSourceToken( source, &token );
}

/*
===============
idParseSystemLocal::Directive_eval
===============
*/
sint idParseSystemLocal::Directive_eval( source_t* source )
{
    sint32 value;
    token_t token;
    
    if( !Evaluate( source, &value, nullptr, true ) )
    {
        return false;
    }
    
    //
    token.line = source->scriptstack->line;
    token.whitespace_p = source->scriptstack->script_p;
    token.endwhitespace_p = source->scriptstack->script_p;
    token.linescrossed = 0;
    ::sprintf( token.string, "%d", abs( value ) );
    token.type = TT_NUMBER;
    token.subtype = TT_INTEGER | TT_LONG | TT_DECIMAL;
    UnreadSourceToken( source, &token );
    
    if( value < 0 )
    {
        UnreadSignToken( source );
    }
    
    return true;
}

/*
===============
idParseSystemLocal::Directive_evalfloat
===============
*/
sint idParseSystemLocal::Directive_evalfloat( source_t* source )
{
    float64 value;
    token_t token;
    
    if( !Evaluate( source, nullptr, &value, false ) )
    {
        return false;
    }
    
    token.line = source->scriptstack->line;
    token.whitespace_p = source->scriptstack->script_p;
    token.endwhitespace_p = source->scriptstack->script_p;
    token.linescrossed = 0;
    ::sprintf( token.string, "%1.2f", fabs( value ) );
    token.type = TT_NUMBER;
    token.subtype = TT_FLOAT | TT_LONG | TT_DECIMAL;
    UnreadSourceToken( source, &token );
    
    if( value < 0 )
    {
        UnreadSignToken( source );
    }
    
    return true;
}

/*
===============
idParseSystemLocal::DollarDirective_evalint
===============
*/
sint idParseSystemLocal::DollarDirective_evalint( source_t* source )
{
    sint32 value;
    token_t token;
    
    if( !DollarEvaluate( source, &value, nullptr, true ) )
    {
        return false;
    }
    
    //
    token.line = source->scriptstack->line;
    token.whitespace_p = source->scriptstack->script_p;
    token.endwhitespace_p = source->scriptstack->script_p;
    token.linescrossed = 0;
    ::sprintf( token.string, "%d", abs( value ) );
    token.type = TT_NUMBER;
    token.subtype = TT_INTEGER | TT_LONG | TT_DECIMAL;
    token.intvalue = value;
    token.floatvalue = value;
    UnreadSourceToken( source, &token );
    
    if( value < 0 )
    {
        UnreadSignToken( source );
    }
    
    return true;
}

/*
===============
idParseSystemLocal::DollarDirective_evalfloat
===============
*/
sint idParseSystemLocal::DollarDirective_evalfloat( source_t* source )
{
    float64 value;
    token_t token;
    
    if( !DollarEvaluate( source, nullptr, &value, false ) )
    {
        return false;
    }
    
    token.line = source->scriptstack->line;
    token.whitespace_p = source->scriptstack->script_p;
    token.endwhitespace_p = source->scriptstack->script_p;
    token.linescrossed = 0;
    ::sprintf( token.string, "%1.2f", fabs( value ) );
    token.type = TT_NUMBER;
    token.subtype = TT_FLOAT | TT_LONG | TT_DECIMAL;
    token.intvalue = ( uint32 ) value;
    token.floatvalue = value;
    
    UnreadSourceToken( source, &token );
    
    if( value < 0 )
    {
        UnreadSignToken( source );
    }
    
    return true;
}

/*
===============
ReadDollarDirective
===============
*/
directive_t DollarDirectives[20] =
{
    {"evalint", idParseSystemLocal::DollarDirective_evalint},
    {"evalfloat", idParseSystemLocal::DollarDirective_evalfloat},
    {nullptr, nullptr}
};

sint idParseSystemLocal::ReadDollarDirective( source_t* source )
{
    sint i;
    token_t token;
    
    //read the directive name
    if( !ReadSourceToken( source, &token ) )
    {
        SourceError( source, "found $ without name" );
        return false;
    }
    
    //directive name must be on the same line
    if( token.linescrossed > 0 )
    {
        UnreadSourceToken( source, &token );
        SourceError( source, "found $ at end of line" );
        return false;
    }
    
    //if if is a name
    if( token.type == TT_NAME )
    {
        //find the precompiler directive
        for( i = 0; DollarDirectives[i].name; i++ )
        {
            if( !::strcmp( DollarDirectives[i].name, token.string ) )
            {
                return DollarDirectives[i].func( source );
            }
        }
    }
    
    UnreadSourceToken( source, &token );
    SourceError( source, "unknown precompiler directive %s", token.string );
    
    return false;
}

/*
===============
idParseSystemLocal::Directive_if_def
===============
*/
sint idParseSystemLocal::Directive_if_def( source_t* source, sint type )
{
    sint skip;
    token_t token;
    define_t* d;
    
    if( !ReadLine( source, &token ) )
    {
        SourceError( source, "#ifdef without name" );
        return false;
    }
    
    if( token.type != TT_NAME )
    {
        UnreadSourceToken( source, &token );
        SourceError( source, "expected name after #ifdef, found %s", token.string );
        return false;
    }
    
    d = FindHashedDefine( source->definehash, token.string );
    
    skip = ( type == INDENT_IFDEF ) == ( d == nullptr );
    PushIndent( source, type, skip );
    
    return true;
}

/*
===============
idParseSystemLocal::Directive_ifdef
===============
*/
sint idParseSystemLocal::Directive_ifdef( source_t* source )
{
    return Directive_if_def( source, INDENT_IFDEF );
}

/*
===============
idParseSystemLocal::Directive_ifndef
===============
*/
sint idParseSystemLocal::Directive_ifndef( source_t* source )
{
    return Directive_if_def( source, INDENT_IFNDEF );
}

/*
===============
idParseSystemLocal::Directive_else
===============
*/
sint idParseSystemLocal::Directive_else( source_t* source )
{
    sint type, skip;
    
    PopIndent( source, &type, &skip );
    
    if( !type )
    {
        SourceError( source, "misplaced #else" );
        return false;
    }
    
    if( type == INDENT_ELSE )
    {
        SourceError( source, "#else after #else" );
        return false;
    }
    
    PushIndent( source, INDENT_ELSE, !skip );
    
    return true;
}

/*
===============
idParseSystemLocal::Directive_endif
===============
*/
sint idParseSystemLocal::Directive_endif( source_t* source )
{
    sint type, skip;
    
    PopIndent( source, &type, &skip );
    
    if( !type )
    {
        SourceError( source, "misplaced #endif" );
        return false;
    }
    
    return true;
}

/*
===============
idParseSystemLocal::CheckTokenString
===============
*/
sint idParseSystemLocal::CheckTokenString( source_t* source, valueType* string )
{
    token_t tok;
    
    if( !ReadToken( source, &tok ) )
    {
        return false;
    }
    
    //if the token is available
    if( !::strcmp( tok.string, string ) )
    {
        return true;
    }
    
    //
    UnreadSourceToken( source, &tok );
    
    return false;
}

/*
===============
idParseSystemLocal::Directive_define
===============
*/
sint idParseSystemLocal::Directive_define( source_t* source )
{
    token_t token, *t, *last;
    define_t* define;
    
    if( source->skip > 0 )
    {
        return true;
    }
    
    //
    if( !ReadLine( source, &token ) )
    {
        SourceError( source, "#define without name" );
        return false;
    }
    
    if( token.type != TT_NAME )
    {
        UnreadSourceToken( source, &token );
        SourceError( source, "expected name after #define, found %s", token.string );
        return false;
    }
    
    //check if the define already exists
    define = FindHashedDefine( source->definehash, token.string );
    if( define )
    {
        if( define->flags & DEFINE_FIXED )
        {
            SourceError( source, "can't redefine %s", token.string );
            return false;
        }
        
        SourceWarning( source, "redefinition of %s", token.string );
        
        //unread the define name before executing the #undef directive
        UnreadSourceToken( source, &token );
        
        if( !Directive_undef( source ) )
        {
            return false;
        }
    }
    
    //allocate define
    define = ( define_t* )Z_Malloc( sizeof( define_t ) + ::strlen( token.string ) + 1 );
    ::memset( define, 0, sizeof( define_t ) );
    
    define->name = ( valueType* ) define + sizeof( define_t );
    ::strcpy( define->name, token.string );
    
    //add the define to the source
    AddDefineToHash( define, source->definehash );
    
    //if nothing is defined, just return
    if( !ReadLine( source, &token ) )
    {
        return true;
    }
    
    //if it is a define with parameters
    if( !WhiteSpaceBeforeToken( &token ) && !strcmp( token.string, "(" ) )
    {
        //read the define parameters
        last = nullptr;
        
        if( !CheckTokenString( source, ")" ) )
        {
            while( 1 )
            {
                if( !ReadLine( source, &token ) )
                {
                    SourceError( source, "expected define parameter" );
                    return false;
                }
                
                //if it isn't a name
                if( token.type != TT_NAME )
                {
                    SourceError( source, "invalid define parameter" );
                    return false;
                }
                //
                if( FindDefineParm( define, token.string ) >= 0 )
                {
                    SourceError( source, "two the same define parameters" );
                    return false;
                }
                
                //add the define parm
                t = CopyToken( &token );
                ClearTokenWhiteSpace( t );
                t->next = nullptr;
                
                if( last )
                {
                    last->next = t;
                }
                else
                {
                    define->parms = t;
                }
                
                last = t;
                define->numparms++;
                
                //read next token
                if( !ReadLine( source, &token ) )
                {
                    SourceError( source, "define parameters not terminated" );
                    return false;
                }
                
                //
                if( !::strcmp( token.string, ")" ) ) break;
                
                //then it must be a comma
                if( ::strcmp( token.string, "," ) )
                {
                    SourceError( source, "define not terminated" );
                    return false;
                }
            }
        }
        
        if( !ReadLine( source, &token ) )
        {
            return true;
        }
    }
    
    //read the defined stuff
    last = nullptr;
    do
    {
        t = CopyToken( &token );
        
        if( t->type == TT_NAME && !strcmp( t->string, define->name ) )
        {
            SourceError( source, "recursive define (removed recursion)" );
            continue;
        }
        
        ClearTokenWhiteSpace( t );
        
        t->next = nullptr;
        if( last )
        {
            last->next = t;
        }
        else
        {
            define->tokens = t;
        }
        
        last = t;
    }
    while( ReadLine( source, &token ) );
    
    //
    if( last )
    {
        //check for merge operators at the beginning or end
        if( !::strcmp( define->tokens->string, "##" ) || !::strcmp( last->string, "##" ) )
        {
            SourceError( source, "define with misplaced ##" );
            return false;
        }
    }
    
    return true;
}

directive_t Directives[20] =
{
    {"if", idParseSystemLocal::Directive_if},
    {"ifdef", idParseSystemLocal::Directive_ifdef},
    {"ifndef", idParseSystemLocal::Directive_ifndef},
    {"elif", idParseSystemLocal::Directive_elif},
    {"else", idParseSystemLocal::Directive_else},
    {"endif", idParseSystemLocal::Directive_endif},
    {"include", idParseSystemLocal::Directive_include},
    {"define", idParseSystemLocal::Directive_define},
    {"undef", idParseSystemLocal::Directive_undef},
    {"line", idParseSystemLocal::Directive_line},
    {"error", idParseSystemLocal::Directive_error},
    {"pragma", idParseSystemLocal::Directive_pragma},
    {"eval", idParseSystemLocal::Directive_eval},
    {"evalfloat", idParseSystemLocal::Directive_evalfloat},
    {nullptr, nullptr}
    
};

/*
===============
idParseSystemLocal::ReadDirective
===============
*/
sint idParseSystemLocal::ReadDirective( source_t* source )
{
    sint i;
    token_t token;
    
    //read the directive name
    if( !ReadSourceToken( source, &token ) )
    {
        SourceError( source, "found # without name" );
        return false;
    }
    
    //directive name must be on the same line
    if( token.linescrossed > 0 )
    {
        UnreadSourceToken( source, &token );
        SourceError( source, "found # at end of line" );
        return false;
    }
    
    //if if is a name
    if( token.type == TT_NAME )
    {
        //find the precompiler directive
        for( i = 0; Directives[i].name; i++ )
        {
            if( !::strcmp( Directives[i].name, token.string ) )
            {
                return Directives[i].func( source );
            }
        }
    }
    
    SourceError( source, "unknown precompiler directive %s", token.string );
    return false;
}

/*
===============
idParseSystemLocal::UnreadToken
===============
*/
void idParseSystemLocal::UnreadToken( source_t* source, token_t* token )
{
    UnreadSourceToken( source, token );
}

/*
===============
idParseSystemLocal::ReadEnumeration

It is assumed that the 'enum' token has already been consumed
This is fairly basic: it doesn't catch some fairly obvious errors like nested
enums, and enumerated names conflict with #define parameters
===============
*/
bool idParseSystemLocal::ReadEnumeration( source_t* source )
{
    token_t newtoken;
    sint value;
    
    if( !ReadToken( source, &newtoken ) )
    {
        return false;
    }
    
    if( newtoken.type != TT_PUNCTUATION || newtoken.subtype != P_BRACEOPEN )
    {
        SourceError( source, "Found %s when expecting {\n", newtoken.string );
        return false;
    }
    
    for( value = 0;; value++ )
    {
        token_t name;
        
        // read the name
        if( !ReadToken( source, &name ) )
        {
            break;
        }
        
        // it's ok for the enum to end immediately
        if( name.type == TT_PUNCTUATION && name.subtype == P_BRACECLOSE )
        {
            if( !ReadToken( source, &name ) )
            {
                break;
            }
            
            // ignore trailing semicolon
            if( name.type != TT_PUNCTUATION || name.subtype != P_SEMICOLON )
            {
                UnreadToken( source, &name );
            }
            
            return true;
        }
        
        // ... but not for it to do anything else
        if( name.type != TT_NAME )
        {
            SourceError( source, "Found %s when expecting identifier\n", name.string );
            return false;
        }
        
        if( !ReadToken( source, &newtoken ) )
        {
            break;
        }
        
        if( newtoken.type != TT_PUNCTUATION )
        {
            SourceError( source, "Found %s when expecting , or = or }\n", newtoken.string );
            return false;
        }
        
        if( newtoken.subtype == P_ASSIGN )
        {
            sint neg = 1;
            
            if( !ReadToken( source, &newtoken ) )
            {
                break;
            }
            
            // ReadToken doesn't seem to read negative numbers, so we do it
            // ourselves
            if( newtoken.type == TT_PUNCTUATION && newtoken.subtype == P_SUB )
            {
                neg = -1;
                
                // the next token should be the number
                if( !ReadToken( source, &newtoken ) )
                {
                    break;
                }
            }
            
            if( newtoken.type != TT_NUMBER || !( newtoken.subtype & TT_INTEGER ) )
            {
                SourceError( source, "Found %s when expecting integer\n", newtoken.string );
                return false;
            }
            
            // this is somewhat silly, but cheap to check
            if( neg == -1 && ( newtoken.subtype & TT_UNSIGNED ) )
            {
                SourceWarning( source, "Value in enumeration is negative and unsigned\n" );
            }
            
            // set the new define value
            value = newtoken.intvalue * neg;
            
            if( !ReadToken( source, &newtoken ) )
            {
                break;
            }
        }
        
        if( newtoken.type != TT_PUNCTUATION || ( newtoken.subtype != P_COMMA && newtoken.subtype != P_BRACECLOSE ) )
        {
            SourceError( source, "Found %s when expecting , or }\n", newtoken.string );
            return false;
        }
        
        if( !AddDefineToSourceFromString( source, va( "%s %d\n", name.string, value ) ) )
        {
            SourceWarning( source, "Couldn't add define to source: %s = %d\n", name.string, value );
            return false;
        }
        
        if( newtoken.subtype == P_BRACECLOSE )
        {
            if( !ReadToken( source, &name ) )
            {
                break;
            }
            
            // ignore trailing semicolon
            if( name.type != TT_PUNCTUATION || name.subtype != P_SEMICOLON )
            {
                UnreadToken( source, &name );
            }
            
            return true;
        }
    }
    
    // got here if a ReadToken returned false
    return false;
}

/*
===============
idParseSystemLocal::ReadToken
===============
*/
sint idParseSystemLocal::ReadToken( source_t* source, token_t* token )
{
    define_t* define;
    
    while( 1 )
    {
        if( !ReadSourceToken( source, token ) )
        {
            return false;
        }
        
        //check for precompiler directives
        if( token->type == TT_PUNCTUATION && *token->string == '#' )
        {
            {
                //read the precompiler directive
                if( !ReadDirective( source ) )
                {
                    return false;
                }
                
                continue;
            }
        }
        if( token->type == TT_PUNCTUATION && *token->string == '$' )
        {
            {
                //read the precompiler directive
                if( !ReadDollarDirective( source ) )
                {
                    return false;
                }
                
                continue;
            }
        }
        if( token->type == TT_NAME && !Q_stricmp( token->string, "enum" ) )
        {
            if( !ReadEnumeration( source ) )
            {
                return false;
            }
            
            continue;
        }
        // recursively concatenate strings that are behind each other still resolving defines
        if( token->type == TT_STRING )
        {
            token_t newtoken;
            if( ReadToken( source, &newtoken ) )
            {
                if( newtoken.type == TT_STRING )
                {
                    token->string[::strlen( token->string ) - 1] = '\0';
                    
                    if( ::strlen( token->string ) + ::strlen( newtoken.string + 1 ) + 1 >= MAX_TOKEN_CHARS )
                    {
                        SourceError( source, "string longer than MAX_TOKEN_CHARS %d\n", MAX_TOKEN_CHARS );
                        return false;
                    }
                    
                    ::strcat( token->string, newtoken.string + 1 );
                }
                else
                {
                    UnreadToken( source, &newtoken );
                }
            }
        }
        //if skipping source because of conditional compilation
        if( source->skip )
        {
            continue;
        }
        
        //if the token is a name
        if( token->type == TT_NAME )
        {
            //check if the name is a define macro
            define = FindHashedDefine( source->definehash, token->string );
            
            //if it is a define macro
            if( define )
            {
                //expand the defined macro
                if( !ExpandDefineIntoSource( source, token, define ) )
                {
                    return false;
                }
                
                continue;
            }
        }
        
        //copy token for unreading
        ::memcpy( &source->token, token, sizeof( token_t ) );
        
        //found a token
        return true;
    }
}

/*
===============
idParseSystemLocal::DefineFromString
===============
*/
define_t* idParseSystemLocal::DefineFromString( valueType* string )
{
    sint res, i;
    script_t* script;
    source_t src;
    token_t* t;
    define_t* def;
    
    script = LoadScriptMemory( string, ( sint )::strlen( string ), "*extern" );
    
    //create a new source
    ::memset( &src, 0, sizeof( source_t ) );
    ::strncpy( src.filename, "*extern", MAX_QPATH );
    
    src.scriptstack = script;
    src.definehash = ( define_t** )Z_Malloc( DEFINEHASHSIZE * sizeof( define_t* ) );
    ::memset( src.definehash, 0, DEFINEHASHSIZE * sizeof( define_t* ) );
    
    //create a define from the source
    res = Directive_define( &src );
    
    //free any tokens if left
    for( t = src.tokens; t; t = src.tokens )
    {
        src.tokens = src.tokens->next;
        FreeToken( t );
    }
    
    def = nullptr;
    
    for( i = 0; i < DEFINEHASHSIZE; i++ )
    {
        if( src.definehash[i] )
        {
            def = src.definehash[i];
            break;
        }
    }
    
    //
    Z_Free( src.definehash );
    
    //
    FreeScript( script );
    
    //if the define was created succesfully
    if( res > 0 )
    {
        return def;
    }
    
    //free the define is created
    if( src.defines )
    {
        FreeDefine( def );
    }
    
    //
    return nullptr;
}

/*
===============
idParseSystemLocal::AddDefineToSourceFromString
===============
*/
bool idParseSystemLocal::AddDefineToSourceFromString( source_t* source, valueType* string )
{
    PushScript( source, LoadScriptMemory( string, ( sint )::strlen( string ), "*extern" ) );
    
    return ( bool )Directive_define( source );
}

/*
===============
idParseSystemLocal::AddGlobalDefine

adds or overrides a global define that will be added to all opened sources
===============
*/
sint idParseSystemLocal::AddGlobalDefine( valueType* string )
{
    define_t* define, *prev, *curr;
    
    define = DefineFromString( string );
    if( !define )
    {
        return false;
    }
    
    prev = nullptr;
    for( curr = globaldefines; curr; curr = curr->next )
    {
        if( !::strcmp( curr->name, define->name ) )
        {
            define->next = curr->next;
            FreeDefine( curr );
            
            if( prev )
            {
                prev->next = define;
            }
            else
            {
                globaldefines = define;
            }
            
            break;
        }
        
        prev = curr;
    }
    
    if( !curr )
    {
        define->next = globaldefines;
        globaldefines = define;
    }
    
    return true;
}

/*
===============
idParseSystemLocal::CopyDefine
===============
*/
define_t* idParseSystemLocal::CopyDefine( source_t* source, define_t* define )
{
    define_t* newdefine;
    token_t* token, *newtoken, *lasttoken;
    
    newdefine = ( define_t* )Z_Malloc( sizeof( define_t ) + ::strlen( define->name ) + 1 );
    
    //copy the define name
    newdefine->name = ( valueType* ) newdefine + sizeof( define_t );
    ::strcpy( newdefine->name, define->name );
    newdefine->flags = define->flags;
    newdefine->builtin = define->builtin;
    newdefine->numparms = define->numparms;
    
    //the define is not linked
    newdefine->next = nullptr;
    newdefine->hashnext = nullptr;
    
    //copy the define tokens
    newdefine->tokens = nullptr;
    
    for( lasttoken = nullptr, token = define->tokens; token; token = token->next )
    {
        newtoken = CopyToken( token );
        newtoken->next = nullptr;
        if( lasttoken )
        {
            lasttoken->next = newtoken;
        }
        else
        {
            newdefine->tokens = newtoken;
        }
        
        lasttoken = newtoken;
    }
    
    //copy the define parameters
    newdefine->parms = nullptr;
    
    for( lasttoken = nullptr, token = define->parms; token; token = token->next )
    {
        newtoken = CopyToken( token );
        newtoken->next = nullptr;
        if( lasttoken )
        {
            lasttoken->next = newtoken;
        }
        else
        {
            newdefine->parms = newtoken;
        }
        
        lasttoken = newtoken;
    }
    return newdefine;
}

/*
===============
idParseSystemLocal::AddGlobalDefinesToSource
===============
*/
void idParseSystemLocal::AddGlobalDefinesToSource( source_t* source )
{
    define_t* define, *newdefine;
    
    for( define = globaldefines; define; define = define->next )
    {
        newdefine = CopyDefine( source, define );
        AddDefineToHash( newdefine, source->definehash );
    }
}

/*
===============
idParseSystemLocal::LoadSourceFile
===============
*/
source_t* idParseSystemLocal::LoadSourceFile( pointer filename )
{
    source_t* source;
    script_t* script;
    
    script = LoadScriptFile( filename );
    if( !script )
    {
        return nullptr;
    }
    
    script->next = nullptr;
    
    source = ( source_t* )Z_Malloc( sizeof( source_t ) );
    ::memset( source, 0, sizeof( source_t ) );
    
    ::strncpy( source->filename, filename, MAX_QPATH );
    source->scriptstack = script;
    source->tokens = nullptr;
    source->defines = nullptr;
    source->indentstack = nullptr;
    source->skip = 0;
    
    source->definehash = ( define_t** )Z_Malloc( DEFINEHASHSIZE * sizeof( define_t* ) );
    ::memset( source->definehash, 0, DEFINEHASHSIZE * sizeof( define_t* ) );
    
    AddGlobalDefinesToSource( source );
    return source;
}

/*
===============
idParseSystemLocal::FreeSource
===============
*/
void idParseSystemLocal::FreeSource( source_t* source )
{
    sint i;
    script_t* script;
    token_t* token;
    define_t* define;
    indent_t* indent;
    
    //free all the scripts
    while( source->scriptstack )
    {
        script = source->scriptstack;
        source->scriptstack = source->scriptstack->next;
        FreeScript( script );
    }
    
    //free all the tokens
    while( source->tokens )
    {
        token = source->tokens;
        source->tokens = source->tokens->next;
        FreeToken( token );
    }
    
    for( i = 0; i < DEFINEHASHSIZE; i++ )
    {
        while( source->definehash[i] )
        {
            define = source->definehash[i];
            source->definehash[i] = source->definehash[i]->hashnext;
            FreeDefine( define );
        }
    }
    
    //free all indents
    while( source->indentstack )
    {
        indent = source->indentstack;
        source->indentstack = source->indentstack->next;
        Z_Free( indent );
    }
    //
    if( source->definehash )
    {
        Z_Free( source->definehash );
    }
    
    //free the source itself
    Z_Free( source );
}

/*
===============
idParseSystemLocal::LoadSourceHandle
===============
*/
sint idParseSystemLocal::LoadSourceHandle( pointer filename )
{
    sint i;
    source_t* source;
    
    for( i = 1; i < MAX_SOURCEFILES; i++ )
    {
        if( !sourceFiles[i] )
        {
            break;
        }
    }
    
    if( i >= MAX_SOURCEFILES )
    {
        return 0;
    }
    
    source = LoadSourceFile( filename );
    if( !source )
    {
        return 0;
    }
    
    sourceFiles[i] = source;
    
    return i;
}

/*
===============
idParseSystemLocal::FreeSourceHandle
===============
*/
sint idParseSystemLocal::FreeSourceHandle( sint handle )
{
    if( handle < 1 || handle >= MAX_SOURCEFILES )
    {
        return false;
    }
    
    if( !sourceFiles[handle] )
    {
        return false;
    }
    
    FreeSource( sourceFiles[handle] );
    sourceFiles[handle] = nullptr;
    
    return true;
}

/*
===============
idParseSystemLocal::ReadTokenHandle
===============
*/
sint idParseSystemLocal::ReadTokenHandle( sint handle, pc_token_t* pc_token )
{
    token_t token;
    sint ret;
    
    if( handle < 1 || handle >= MAX_SOURCEFILES )
    {
        return 0;
    }
    
    if( !sourceFiles[handle] )
    {
        return 0;
    }
    
    ret = ReadToken( sourceFiles[handle], &token );
    Q_strcpy_s( pc_token->string, token.string );
    pc_token->type = token.type;
    pc_token->subtype = token.subtype;
    pc_token->intvalue = token.intvalue;
    pc_token->floatvalue = token.floatvalue;
    
    if( pc_token->type == TT_STRING )
    {
        StripDoubleQuotes( pc_token->string );
    }
    
    return ret;
}

/*
===============
idParseSystemLocal::SourceFileAndLine
===============
*/
sint idParseSystemLocal::SourceFileAndLine( sint handle, valueType* filename, sint* line )
{
    if( handle < 1 || handle >= MAX_SOURCEFILES )
    {
        return false;
    }
    
    if( !sourceFiles[handle] )
    {
        return false;
    }
    
    ::strcpy( filename, sourceFiles[handle]->filename );
    
    if( sourceFiles[handle]->scriptstack )
    {
        *line = sourceFiles[handle]->scriptstack->line;
    }
    else
    {
        *line = 0;
    }
    
    return true;
}
