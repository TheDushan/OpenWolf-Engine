////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2000 - 2009 Darklegion Development
// Copyright(C) 2011 - 2022 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   Parse.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __PARSE_HPP__
#define __PARSE_HPP__

//script flags
#define SCFL_NOERRORS             0x0001
#define SCFL_NOWARNINGS           0x0002
#define SCFL_NOSTRINGWHITESPACES  0x0004
#define SCFL_NOSTRINGESCAPECHARS  0x0008
#define SCFL_PRIMITIVE            0x0010
#define SCFL_NOBINARYNUMBERS      0x0020
#define SCFL_NONUMBERVALUES       0x0040

//token types
#define TT_STRING           1     // string
#define TT_LITERAL          2     // literal
#define TT_NUMBER           3     // number
#define TT_NAME             4     // name
#define TT_PUNCTUATION      5     // punctuation

//string sub type
//---------------
//    the length of the string
//literal sub type
//----------------
//    the ASCII code of the literal
//number sub type
//---------------
#define TT_DECIMAL          0x0008  // decimal number
#define TT_HEX              0x0100  // hexadecimal number
#define TT_OCTAL            0x0200  // octal number
#define TT_BINARY           0x0400  // binary number
#define TT_FLOAT            0x0800  // floating point number
#define TT_INTEGER          0x1000  // integer number
#define TT_LONG             0x2000  // long number
#define TT_UNSIGNED         0x4000  // unsigned number
//punctuation sub type
//--------------------
#define P_RSHIFT_ASSIGN       1
#define P_LSHIFT_ASSIGN       2
#define P_PARMS               3
#define P_PRECOMPMERGE        4

#define P_LOGIC_AND           5
#define P_LOGIC_OR            6
#define P_LOGIC_GEQ           7
#define P_LOGIC_LEQ           8
#define P_LOGIC_EQ            9
#define P_LOGIC_UNEQ          10

#define P_MUL_ASSIGN          11
#define P_DIV_ASSIGN          12
#define P_MOD_ASSIGN          13
#define P_ADD_ASSIGN          14
#define P_SUB_ASSIGN          15
#define P_INC                 16
#define P_DEC                 17

#define P_BIN_AND_ASSIGN      18
#define P_BIN_OR_ASSIGN       19
#define P_BIN_XOR_ASSIGN      20
#define P_RSHIFT              21
#define P_LSHIFT              22

#define P_POINTERREF          23
#define P_CPP1                24
#define P_CPP2                25
#define P_MUL                 26
#define P_DIV                 27
#define P_MOD                 28
#define P_ADD                 29
#define P_SUB                 30
#define P_ASSIGN              31

#define P_BIN_AND             32
#define P_BIN_OR              33
#define P_BIN_XOR             34
#define P_BIN_NOT             35

#define P_LOGIC_NOT           36
#define P_LOGIC_GREATER       37
#define P_LOGIC_LESS          38

#define P_REF                 39
#define P_COMMA               40
#define P_SEMICOLON           41
#define P_COLON               42
#define P_QUESTIONMARK        43

#define P_PARENTHESESOPEN     44
#define P_PARENTHESESCLOSE    45
#define P_BRACEOPEN           46
#define P_BRACECLOSE          47
#define P_SQBRACKETOPEN       48
#define P_SQBRACKETCLOSE      49
#define P_BACKSLASH           50

#define P_PRECOMP             51
#define P_DOLLAR              52

//name sub type
//-------------
//    the length of the name

//punctuation
typedef struct punctuation_s {
    valueType *p;                    //punctuation character(s)
    sint n;                      //punctuation indication
    struct punctuation_s *next; //next punctuation
} punctuation_t;

//token
typedef struct owtoken_s {
    valueType string[MAX_TOKEN_CHARS]; //available token
    sint type;                     //last read token type
    sint subtype;                  //last read token sub type
    uint32 intvalue;   //integer value
    float64 floatvalue;            //floating point value
    valueType *whitespace_p;           //start of white space before token
    valueType *endwhitespace_p;        //start of white space before token
    sint line;                     //line the token was on
    sint linescrossed;             //lines crossed in white space
    struct owtoken_s *next;         //next token in chain
} owtoken_t;

//script file
typedef struct script_s {
    valueType filename[1024];            //file name of the script
    valueType *buffer;                   //buffer containing the script
    valueType *script_p;                 //current pointer in the script
    valueType *end_p;                    //pointer to the end of the script
    valueType *lastscript_p;             //script pointer before reading token
    valueType *whitespace_p;             //begin of the white space
    valueType *endwhitespace_p;
    sint length;                     //length of the script in bytes
    sint line;                       //current line in script
    sint lastline;                   //line before reading token
    sint tokenavailable;             //set by UnreadLastToken
    sint flags;                      //several script flags
    punctuation_t *punctuations;    //the punctuations used in the script
    punctuation_t **punctuationtable;
    owtoken_t token;                  //available token
    struct script_s *next;          //next script in a chain
} script_t;


#define DEFINE_FIXED      0x0001

#define BUILTIN_LINE      1
#define BUILTIN_FILE      2
#define BUILTIN_DATE      3
#define BUILTIN_TIME      4
#define BUILTIN_STDC      5

#define INDENT_IF         0x0001
#define INDENT_ELSE       0x0002
#define INDENT_ELIF       0x0004
#define INDENT_IFDEF      0x0008
#define INDENT_IFNDEF     0x0010

//macro definitions
typedef struct define_s {
    valueType *name;                 //define name
    sint flags;                  //define flags
    sint builtin;                // > 0 if builtin define
    sint numparms;               //number of define parameters
    owtoken_t *parms;             //define parameters
    owtoken_t *tokens;            //macro tokens (possibly containing parm tokens)
    struct define_s *next;      //next defined macro in a list
    struct define_s *hashnext;  //next define in the hash chain
} define_t;

//indents
//used for conditional compilation directives:
//#if, #else, #elif, #ifdef, #ifndef
typedef struct indent_s {
    sint type;               //indent type
    sint skip;               //true if skipping current indent
    script_t *script;       //script the indent was in
    struct indent_s *next;  //next indent on the indent stack
} indent_t;

//source file
typedef struct source_s {
    valueType filename[MAX_QPATH];     //file name of the script
    valueType includepath[MAX_QPATH];  //path to include files
    punctuation_t *punctuations;  //punctuations to use
    script_t *scriptstack;        //stack with scripts of the source
    owtoken_t *tokens;              //tokens to read first
    define_t *defines;            //list with macro definitions
    define_t **definehash;        //hash chain with defines
    indent_t *indentstack;        //stack with indents
    sint skip;                     // > 0 if skipping conditional code
    owtoken_t token;                //last read token
} source_t;

#define MAX_DEFINEPARMS     128

//directive name with parse function
typedef struct directive_s {
    valueType *name;
    sint(*func)(source_t *source);
} directive_t;

#define DEFINEHASHSIZE    1024

//longer punctuations first
static punctuation_t Default_Punctuations[] = {
    //binary operators
    {">>=", P_RSHIFT_ASSIGN, nullptr},
    {"<<=", P_LSHIFT_ASSIGN, nullptr},
    //
    {"...", P_PARMS, nullptr},
    //define merge operator
    {"##", P_PRECOMPMERGE, nullptr},
    //logic operators
    {"&&", P_LOGIC_AND, nullptr},
    {"||", P_LOGIC_OR, nullptr},
    {">=", P_LOGIC_GEQ, nullptr},
    {"<=", P_LOGIC_LEQ, nullptr},
    {"==", P_LOGIC_EQ, nullptr},
    {"!=", P_LOGIC_UNEQ, nullptr},
    //arithmatic operators
    {"*=", P_MUL_ASSIGN, nullptr},
    {"/=", P_DIV_ASSIGN, nullptr},
    {"%=", P_MOD_ASSIGN, nullptr},
    {"+=", P_ADD_ASSIGN, nullptr},
    {"-=", P_SUB_ASSIGN, nullptr},
    {"++", P_INC, nullptr},
    {"--", P_DEC, nullptr},
    //binary operators
    {"&=", P_BIN_AND_ASSIGN, nullptr},
    {"|=", P_BIN_OR_ASSIGN, nullptr},
    {"^=", P_BIN_XOR_ASSIGN, nullptr},
    {">>", P_RSHIFT, nullptr},
    {"<<", P_LSHIFT, nullptr},
    //reference operators
    {"->", P_POINTERREF, nullptr},
    //C++
    {"::", P_CPP1, nullptr},
    {".*", P_CPP2, nullptr},
    //arithmatic operators
    {"*", P_MUL, nullptr},
    {"/", P_DIV, nullptr},
    {"%", P_MOD, nullptr},
    {"+", P_ADD, nullptr},
    {"-", P_SUB, nullptr},
    {"=", P_ASSIGN, nullptr},
    //binary operators
    {"&", P_BIN_AND, nullptr},
    {"|", P_BIN_OR, nullptr},
    {"^", P_BIN_XOR, nullptr},
    {"~", P_BIN_NOT, nullptr},
    //logic operators
    {"!", P_LOGIC_NOT, nullptr},
    {">", P_LOGIC_GREATER, nullptr},
    {"<", P_LOGIC_LESS, nullptr},
    //reference operator
    {".", P_REF, nullptr},
    //seperators
    {",", P_COMMA, nullptr},
    {";", P_SEMICOLON, nullptr},
    //label indication
    {":", P_COLON, nullptr},
    //if statement
    {"?", P_QUESTIONMARK, nullptr},
    //embracements
    {"(", P_PARENTHESESOPEN, nullptr},
    {")", P_PARENTHESESCLOSE, nullptr},
    {"{", P_BRACEOPEN, nullptr},
    {"}", P_BRACECLOSE, nullptr},
    {"[", P_SQBRACKETOPEN, nullptr},
    {"]", P_SQBRACKETCLOSE, nullptr},
    //
    {"\\", P_BACKSLASH, nullptr},
    //precompiler operator
    {"#", P_PRECOMP, nullptr},
    {"$", P_DOLLAR, nullptr},
    {nullptr, 0}
};

typedef struct operator_s {
    sint _operator;
    sint priority;
    sint parentheses;
    struct operator_s *prev, * next;
} operator_t;

typedef struct value_s {
    sint32 intvalue;
    float64 floatvalue;
    sint parentheses;
    struct value_s *prev, * next;
} value_t;

#define MAX_VALUES    64
#define MAX_OPERATORS 64
#define AllocValue(val)                 \
    if (numvalues >= MAX_VALUES) {            \
        idParseSystemLocal::SourceError(source, "out of value space\n");    \
        error = 1;                    \
        break;                      \
    }                         \
    else                        \
        val = &value_heap[numvalues++];
#define FreeValue(val)
//
#define AllocOperator(op)               \
    if (numoperators >= MAX_OPERATORS) {        \
        idParseSystemLocal::SourceError(source, "out of operator space\n"); \
        error = 1;                    \
        break;                      \
    }                         \
    else                        \
        op = &operator_heap[numoperators++];

#define FreeOperator(op)

#define MAX_SOURCEFILES 64

//
// idClientScreenSystemLocal
//
class idParseSystemLocal : public idParseSystem {
public:
    idParseSystemLocal();
    ~idParseSystemLocal();

    virtual sint AddGlobalDefine(valueType *string);
    virtual sint LoadSourceHandle(pointer filename);
    virtual sint FreeSourceHandle(sint handle);
    virtual sint ReadTokenHandle(sint handle, pc_token_t *pc_token);
    virtual sint SourceFileAndLine(sint handle, valueType *filename,
                                   sint *line);

public:
    static void CreatePunctuationTable(script_t *script,
                                       punctuation_t *punctuations);
    static void ScriptError(script_t *script, valueType *str, ...);
    static void ScriptWarning(script_t *script, valueType *str, ...);
    static void SetScriptPunctuations(script_t *script, punctuation_t *p);
    static sint ReadWhiteSpace(script_t *script);
    static sint ReadEscapeCharacter(script_t *script, valueType *ch);
    static sint ReadString(script_t *script, owtoken_t *token, sint quote);
    static sint ReadName(script_t *script, owtoken_t *token);
    static void NumberValue(valueType *string, sint subtype, uint32 *intvalue,
                            float64 *floatvalue);
    static sint ReadNumber(script_t *script, owtoken_t *token);
    static sint ReadPunctuation(script_t *script, owtoken_t *token);
    static sint ReadPrimitive(script_t *script, owtoken_t *token);
    static sint ReadScriptToken(script_t *script, owtoken_t *token);
    static void StripDoubleQuotes(valueType *string);
    static sint EndOfScript(script_t *script);
    static script_t *LoadScriptFile(pointer filename);
    static script_t *LoadScriptMemory(valueType *ptr, sint length,
                                      valueType *name);
    static void FreeScript(script_t *script);
    static void SourceError(source_t *source, valueType *str, ...);
    static void SourceWarning(source_t *source, valueType *str, ...);
    static void PushIndent(source_t *source, sint type, sint skip);
    static void PopIndent(source_t *source, sint *type, sint *skip);
    static void PushScript(source_t *source, script_t *script);
    static owtoken_t *CopyToken(owtoken_t *token);
    static void FreeToken(owtoken_t *token);
    static sint ReadSourceToken(source_t *source, owtoken_t *token);
    static sint UnreadSourceToken(source_t *source, owtoken_t *token);
    static sint ReadDefineParms(source_t *source, define_t *define,
                                owtoken_t **parms, sint maxparms);
    static sint StringizeTokens(owtoken_t *tokens, owtoken_t *token);
    static sint MergeTokens(owtoken_t *t1, owtoken_t *t2);
    static sint NameHash(valueType *name);
    static void AddDefineToHash(define_t *define, define_t **definehash);
    static define_t *FindHashedDefine(define_t **definehash, valueType *name);
    static sint FindDefineParm(define_t *define, valueType *name);
    static void FreeDefine(define_t *define);
    static sint ExpandBuiltinDefine(source_t *source, owtoken_t *deftoken,
                                    define_t *define, owtoken_t **firsttoken, owtoken_t **lasttoken);
    static sint ExpandDefine(source_t *source, owtoken_t *deftoken,
                             define_t *define, owtoken_t **firsttoken, owtoken_t **lasttoken);
    static sint ExpandDefineIntoSource(source_t *source, owtoken_t *deftoken,
                                       define_t *define);
    static void ConvertPath(valueType *path);
    static sint ReadLine(source_t *source, owtoken_t *token);
    static sint OperatorPriority(sint op);
    static sint EvaluateTokens(source_t *source, owtoken_t *tokens,
                               sint32 *intvalue, float64 *floatvalue, sint integer);
    static sint Evaluate(source_t *source, sint32 *intvalue,
                         float64 *floatvalue, sint integer);
    static sint DollarEvaluate(source_t *source, sint32 *intvalue,
                               float64 *floatvalue, sint integer);
    static sint Directive_include(source_t *source);
    static sint WhiteSpaceBeforeToken(owtoken_t *token);
    static void ClearTokenWhiteSpace(owtoken_t *token);
    static sint Directive_undef(source_t *source);
    static sint Directive_elif(source_t *source);
    static sint Directive_if(source_t *source);
    static sint Directive_line(source_t *source);
    static sint Directive_error(source_t *source);
    static sint Directive_pragma(source_t *source);
    static void UnreadSignToken(source_t *source);
    static sint Directive_eval(source_t *source);
    static sint Directive_evalfloat(source_t *source);
    static sint DollarDirective_evalint(source_t *source);
    static sint DollarDirective_evalfloat(source_t *source);
    static sint ReadDollarDirective(source_t *source);
    static sint Directive_if_def(source_t *source, sint type);
    static sint Directive_ifdef(source_t *source);
    static sint Directive_ifndef(source_t *source);
    static sint Directive_else(source_t *source);
    static sint Directive_endif(source_t *source);
    static sint CheckTokenString(source_t *source, valueType *string);
    static sint Directive_define(source_t *source);
    static sint ReadDirective(source_t *source);
    static void UnreadToken(source_t *source, owtoken_t *token);
    static bool ReadEnumeration(source_t *source);
    static sint ReadToken(source_t *source, owtoken_t *token);
    static define_t *DefineFromString(valueType *string);
    static define_t *CopyDefine(source_t *source, define_t *define);
    static bool AddDefineToSourceFromString(source_t *source,
                                            valueType *string);
    static void AddGlobalDefinesToSource(source_t *source);
    static source_t *LoadSourceFile(pointer filename);
    static void FreeSource(source_t *source);
};

extern idParseSystemLocal parseLocal;

#endif //!__PARSE_HPP__
