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
// File name:   q_shared.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: included first by ALL program modules.
//              A user mod should never modify this file
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __Q_SHARED_H__
#define __Q_SHARED_H__

#ifndef PRE_RELEASE_DEMO
// Dushan for ET game, basegame folder was "ETMAIN"
//#define BASEGAME "etmain"
#define BASEGAME "main"
#else
#define BASEGAME "owtest"
#endif

#define CONFIG_NAME     "owconfig.cfg"

#define LOCALIZATION_SUPPORT

#define NEW_ANIMS
#define MAX_TEAMNAME    32

#define DEMOEXT "dm_"           // standard demo extension

#if defined( ppc ) || defined( __ppc ) || defined( __ppc__ ) || defined( __POWERPC__ )
#define idppc 1
#endif

#if defined(__cplusplus) && !defined(min)
template <typename T> __inline T min(T a, T b) {
    return (a < b) ? a : b;
}
template <typename T> __inline T max(T a, T b) {
    return (a > b) ? a : b;
}
#endif

/**********************************************************************
  VM Considerations

  The VM can not use the standard system headers because we aren't really
  using the compiler they were meant for. We use bg_lib.hpp which contains
  prototypes for the functions we define for our own use in bg_lib.c.

  When writing mods, please add needed headers HERE, do not start including
  stuff like <stdio.h> in the various .c files that make up each of the VMs
  since you will be including system headers files can will have issues.

  Remember, if you use a C library function that is not defined in bg_lib.c,
  you will have to add your own version for support in the VM.

 **********************************************************************/

#if defined __GNUC__ || defined __clang__
#define _attribute( x ) __attribute__( x )
#else
#define _attribute( x )
#define __attribute__( x )
#endif

//bani
//======================= GNUC DEFINES ==================================
#if (defined _MSC_VER)
#define Q_EXPORT __declspec(dllexport)
#elif ((__GNUC__ >= 3) && (!__EMX__) && (!sun))
#define Q_EXPORT __attribute__((visibility("default")))
#else
#define Q_EXPORT
#endif
//=============================================================

typedef union {
    float32 f;
    sint i;
    uint ui;
} floatint_t;

typedef sint qhandle_t;
typedef sint sfxHandle_t;
typedef sint fileHandle_t;
typedef sint clipHandle_t;

//#define   SND_NORMAL          0x000   // (default) Allow sound to be cut off only by the same sound on this channel
#define     SND_OKTOCUT         0x001   // Allow sound to be cut off by any following sounds on this channel
#define     SND_REQUESTCUT      0x002   // Allow sound to be cut off by following sounds on this channel only for sounds who request cutoff
#define     SND_CUTOFF          0x004   // Cut off sounds on this channel that are marked 'SND_REQUESTCUT'
#define     SND_CUTOFF_ALL      0x008   // Cut off all sounds on this channel
#define     SND_NOCUT           0x010   // Don't cut off.  Always let finish (overridden by SND_CUTOFF_ALL)
#define     SND_NO_ATTENUATION  0x020   // don't attenuate (even though the sound is in voice channel, for example)

#if defined(_MSC_VER)
#define ALIGN(x) __declspec(align(x));
#elif defined(__GNUC__)
#define ALIGN(x) __attribute__((aligned(x)))
#else
#define ALIGN(x)
#endif

#define lengthof( a ) (sizeof( (a) ) / sizeof( (a)[0] ))

#define PAD(x,y) (((x)+(y)-1) & ~((y)-1))
#define PADLEN(base, alignment) (PAD((base), (alignment)) - (base))
#define PADP(base, alignment)   ((void *) PAD((sint64) (base), (alignment)))

#define STRING(s)           #s
// expand constants before stringifying them
#define XSTRING(s)          STRING(s)

#define MAX_QINT            0x7fffffff
#define MIN_QINT            ( -MAX_QINT - 1 )

#ifndef BIT
#define BIT(x)              (1 << x)
#endif

// TTimo gcc: was missing, added from Q3 source
#define maxow( x, y ) ( ( ( x ) > ( y ) ) ? ( x ) : ( y ) )
#define minow( x, y ) ( ( ( x ) < ( y ) ) ? ( x ) : ( y ) )


// RF, this is just here so different elements of the engine can be aware of this setting as it changes
#define MAX_SP_CLIENTS      64      // increasing this will increase memory usage significantly

// the game guarantees that no string from the network will ever
// exceed MAX_STRING_CHARS
#define MAX_STRING_CHARS    1024    // max length of a string passed to Cmd_TokenizeString
#define MAX_STRING_TOKENS   256     // max tokens resulting from Cmd_TokenizeString
#define MAX_TOKEN_CHARS     1024    // max length of an individual token

#define MAX_INFO_STRING     4096
#define MAX_INFO_KEY        1024
#define MAX_INFO_VALUE      1024

#define BIG_INFO_STRING     8192    // used for system info key only
#define BIG_INFO_KEY        8192
#define BIG_INFO_VALUE      8192

#define MAX_QPATH           64      // max length of a quake game pathname
#define MAX_OSPATH          256     // max length of a filesystem pathname
#define MAX_CMD             1024    // max length of a command line

// rain - increased to 36 to match MAX_NETNAME, fixes #13 - UI stuff breaks
// with very long names
#define MAX_NAME_LENGTH     36      // max length of a client name
#define MAX_COLORFUL_NAME_LENGTH MAX_CVAR_VALUE_STRING

#define MAX_SAY_TEXT        800

enum messageStatus_t {
    MESSAGE_EMPTY = 0,
    MESSAGE_WAITING,        // rate/packet limited
    MESSAGE_WAITING_OVERFLOW,   // packet too large with message
};

// paramters for command buffer stuffing
enum cbufExec_t {
    EXEC_NOW,           // don't return until completed, a VM should NEVER use this,
    // because some commands might cause the VM to be unloaded...
    EXEC_INSERT,        // insert at current position, but don't run yet
    EXEC_APPEND         // add to end of the command buffer (normal case)
};


//
// these aren't needed by any of the VMs.  put in another header?
//
#define MAX_MAP_AREA_BYTES      64      // bit vector of area visibility


// print levels from renderer (FIXME: set up for game / cgame?)
enum printParm_t {
    PRINT_ALL,
    PRINT_DEVELOPER,        // only print when "developer 1"
    PRINT_WARNING,
    PRINT_ERROR
};

#ifdef  ERR_FATAL
#undef  ERR_FATAL               // this is be defined in malloc.h
#endif

// parameters to the main Error routine
enum errorParm_t {
    ERR_FATAL,                  // exit the entire game with a popup window
    ERR_VID_FATAL,              // exit the entire game with a popup window and doesn't delete profile.pid
    ERR_DROP,                   // print to console and disconnect from game
    ERR_SERVERDISCONNECT,       // don't kill server
    ERR_AUTOUPDATE
};

// font rendering values used by ui and cgame

#define PROP_GAP_WIDTH          3
#define PROP_SPACE_WIDTH        8
#define PROP_HEIGHT             27
#define PROP_SMALL_SIZE_SCALE   0.75

#define BLINK_DIVISOR           200
#define PULSE_DIVISOR           75

#define UI_LEFT         0x00000000  // default
#define UI_CENTER       0x00000001
#define UI_RIGHT        0x00000002
#define UI_FORMATMASK   0x00000007
#define UI_SMALLFONT    0x00000010
#define UI_BIGFONT      0x00000020  // default
#define UI_GIANTFONT    0x00000040
#define UI_DROPSHADOW   0x00000800
#define UI_BLINK        0x00001000
#define UI_INVERSE      0x00002000
#define UI_PULSE        0x00004000
// JOSEPH 10-24-99
#define UI_MENULEFT     0x00008000
#define UI_MENURIGHT    0x00010000
#define UI_EXSMALLFONT  0x00020000
#define UI_MENUFULL     0x00080000
// END JOSEPH

#define UI_SMALLFONT75  0x00100000

#if defined( _DEBUG ) && !defined( BSPC )
#define HUNK_DEBUG
#endif

enum ha_pref {
    h_high,
    h_low,
    h_dontcare
};

#define CIN_system  1
#define CIN_loop    2
#define CIN_hold    4
#define CIN_silent  8
#define CIN_shader  16
#define CIN_letterBox 32

/*
==============================================================

MATHLIB

==============================================================
*/

typedef float32 vec_t;
typedef vec_t vec2_t[2];

#if defined(SSEVEC3_T)
typedef vec_t   vec3_t[4];      // ALIGN(16);
typedef vec3_t  vec4_t;
#else
typedef vec_t   vec3_t[3];
typedef vec_t   vec4_t[4];
#endif

typedef vec_t   vec5_t[5];

typedef vec3_t  axis_t[3];
typedef vec_t   matrix3x3_t[9];
typedef vec_t   matrix_t[16];
typedef vec_t   quat_t[4];      // | x y z w |

typedef sint     fixed4_t;
typedef sint     fixed8_t;
typedef sint     fixed16_t;

#undef M_PI

#ifndef M_PI
#define M_PI        3.14159265358979323846f // matches value in gcc v2 math.h
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.414213562f
#endif

#ifndef M_ROOT3
#define M_ROOT3 1.732050808f
#endif

#define ARRAY_INDEX(arr, el)    ((sint)( (el) - (arr) ))
#define ARRAY_LEN(x)            (sizeof(x) / sizeof(*(x)))

// angle indexes
#define PITCH               0   // up / down
#define YAW                 1   // left / right
#define ROLL                2   // fall over

#define NUMVERTEXNORMALS    162
extern vec3_t bytedirs[NUMVERTEXNORMALS];

// all drawing is done to a 640*480 virtual screen size
// and will be automatically scaled to the real resolution
#define SCREEN_WIDTH        640
#define SCREEN_HEIGHT       480

#define TINYCHAR_WIDTH      ( SMALLCHAR_WIDTH )
#define TINYCHAR_HEIGHT     ( SMALLCHAR_HEIGHT )

#define MINICHAR_WIDTH      8
#define MINICHAR_HEIGHT     12

#define SMALLCHAR_WIDTH     8
#define SMALLCHAR_HEIGHT    16

#define BIGCHAR_WIDTH       16
#define BIGCHAR_HEIGHT      16

#define GIANTCHAR_WIDTH     32
#define GIANTCHAR_HEIGHT    48

extern vec4_t colorBlack;
extern vec4_t colorRed;
extern vec4_t colorGreen;
extern vec4_t colorBlue;
extern vec4_t colorYellow;
extern vec4_t colorOrange;
extern vec4_t colorMagenta;
extern vec4_t colorCyan;
extern vec4_t colorWhite;
extern vec4_t colorLtGrey;
extern vec4_t colorMdGrey;
extern vec4_t colorDkGrey;
extern vec4_t colorMdRed;
extern vec4_t colorMdGreen;
extern vec4_t colorGray;
extern vec4_t colorOrange;
extern vec4_t colorRoseBud;
extern vec4_t colorPaleGreen;
extern vec4_t colorPaleGolden;
extern vec4_t colorColumbiaBlue;
extern vec4_t colorPaleTurquoise;
extern vec4_t colorPaleVioletRed;
extern vec4_t colorPalacePaleWhite;
extern vec4_t colorOlive;
extern vec4_t colorTomato;
extern vec4_t colorLime;
extern vec4_t colorLemon;
extern vec4_t colorBlueBerry;
extern vec4_t colorTurquoise;
extern vec4_t colorWildWatermelon;
extern vec4_t colorSaltpan;
extern vec4_t colorGrayChateau;
extern vec4_t colorRust;
extern vec4_t colorCopperGreen;
extern vec4_t colorGold;
extern vec4_t colorSteelBlue;
extern vec4_t colorSteelGray;
extern vec4_t colorBronze;
extern vec4_t colorSilver;
extern vec4_t colorDarkGray;
extern vec4_t colorDarkOrange;
extern vec4_t colorDarkGreen;
extern vec4_t colorRedOrange;
extern vec4_t colorForestGreen;
extern vec4_t colorBrightSun;
extern vec4_t colorMediumSlateBlue;
extern vec4_t colorCeleste;
extern vec4_t colorIronstone;
extern vec4_t colorTimberwolf;
extern vec4_t colorOnyx;
extern vec4_t colorRosewood;
extern vec4_t colorKokoda;
extern vec4_t colorPorsche;
extern vec4_t colorCloudBurst;
extern vec4_t colorBlueDiane;
extern vec4_t colorRope;
extern vec4_t colorBlonde;
extern vec4_t colorSmokeyBlack;
extern vec4_t colorAmericanRose;
extern vec4_t colorNeonGreen;
extern vec4_t colorNeonYellow;
extern vec4_t colorUltramarine;
extern vec4_t colorTurquoiseBlue;
extern vec4_t colorDarkMagenta;
extern vec4_t colorMagicMint;
extern vec4_t colorLightGray;
extern vec4_t colorLightSalmon;
extern vec4_t colorLightGreen;

#define GAME_INIT_FRAMES    6
#define FRAMETIME           100                 // msec

#define NUMBER_OF_COLORS 62
#define Q_COLOR_ESCAPE  '^'
#define Q_COLOR_HEX_ESCAPE '#'
bool Q_IsShortHexColor(pointer p);
bool Q_IsLongHexColor(pointer p);
bool Q_IsHexColor(pointer p);
bool Q_IsHardcodedColor(pointer p);
bool Q_IsColorString(pointer p);
bool Q_IsColorEscapeEscape(pointer p);
bool Q_IsColorNULLString(pointer p);
sint Q_ColorStringLength(pointer p);
sint Q_NumOfColorCodeDigits(pointer p);

#define COLOR_BLACK             '0'
#define COLOR_RED               '1'
#define COLOR_GREEN             '2'
#define COLOR_YELLOW            '3'
#define COLOR_BLUE              '4'
#define COLOR_CYAN              '5'
#define COLOR_MAGENTA           '6'
#define COLOR_WHITE             '7'
#define COLOR_GRAY              '8'
#define COLOR_ORANGE            '9'
#define COLOR_ROSE_BUD          'a'
#define COLOR_PALE_GREEN        'b'
#define COLOR_PALE_GOLDEN       'c'
#define COLOR_COLUMBIA_BLUE     'd'
#define COLOR_PALE_TURQUOISE    'e'
#define COLOR_PALE_VIOLET_RED   'f'
#define COLOR_PALACE_PALE_WHITE 'g'
#define COLOR_OLIVE             'h'
#define COLOR_TOMATO            'i'
#define COLOR_LIME              'j'
#define COLOR_LEMON             'k'
#define COLOR_BLUE_BERRY        'l'
#define COLOR_TURQUOISE         'm'
#define COLOR_WILD_WATERMELON   'n'
#define COLOR_SALTPAN           'o'
#define COLOR_GRAY_CHATEAU      'p'
#define COLOR_RUST              'q'
#define COLOR_COPPER_GREEN      'r'
#define COLOR_GOLD              's'
#define COLOR_STEEL_BLUE        't'
#define COLOR_STEEL_GRAY        'u'
#define COLOR_BRONZE            'v'
#define COLOR_SILVER            'w'
#define COLOR_DARK_GRAY         'x'
#define COLOR_DARK_ORANGE       'y'
#define COLOR_DARK_GREEN        'z'
#define COLOR_RED_ORANGE        'A'
#define COLOR_FOREST_GREEN      'B'
#define COLOR_BRIGHT_SUN        'C'
#define COLOR_MEDIUM_SLATE_BLUE 'D'
#define COLOR_CELESTE           'E'
#define COLOR_IRONSTONE         'F'
#define COLOR_TIMBERWOLF        'G'
#define COLOR_ONYX              'H'
#define COLOR_ROSEWOOD          'I'
#define COLOR_KOKODA            'J'
#define COLOR_PORSCHE           'K'
#define COLOR_CLOUD_BURST       'L'
#define COLOR_BLUE_DIANE        'M'
#define COLOR_ROPE              'N'
#define COLOR_BLONDE            'O'
#define COLOR_SMOKEY_BLACK      'P'
#define COLOR_AMERICAN_ROSE     'Q'
#define COLOR_NEON_GREEN        'R'
#define COLOR_NEON_YELLOW       'S'
#define COLOR_ULTRAMARINE       'T'
#define COLOR_TURQUOISE_BLUE    'U'
#define COLOR_DARK_MAGENTA      'V'
#define COLOR_MAGIC_MINT        'W'
#define COLOR_LIGHT_GRAY        'X'
#define COLOR_LIGHT_SALMON      'Y'
#define COLOR_LIGHT_GREEN       'Z'
#define COLOR_NULL              '*'

#define COLOR_BITS  31
#define ColorIndex(c) (((((c) >= '0') && ((c) <= '9')) ? ((c) - '0') : ((((c) >= 'a') && ((c) <= 'z')) ? ((c) - 'a' + 10) : ((((c) >= 'A') && ((c) <= 'Z')) ? ((c) - 'A' + 36) : 7))))

#define S_COLOR_BLACK             "^0"
#define S_COLOR_RED               "^1"
#define S_COLOR_GREEN             "^2"
#define S_COLOR_YELLOW            "^3"
#define S_COLOR_BLUE              "^4"
#define S_COLOR_CYAN              "^5"
#define S_COLOR_MAGENTA           "^6"
#define S_COLOR_WHITE             "^7"
#define S_COLOR_GRAY              '^8'
#define S_COLOR_ORANGE            '^9'
#define S_COLOR_ROSE_BUD          '^a'
#define S_COLOR_PALE_GREEN        '^b'
#define S_COLOR_PALE_GOLDEN       '^c'
#define S_COLOR_COLUMBIA_BLUE     '^d'
#define S_COLOR_PALE_TURQUOISE    '^e'
#define S_COLOR_PALE_VIOLET_RED   '^f'
#define S_COLOR_PALACE_PALE_WHITE '^g'
#define S_COLOR_OLIVE             '^h'
#define S_COLOR_TOMATO            '^i'
#define S_COLOR_LIME              '^j'
#define S_COLOR_LEMON             '^k'
#define S_COLOR_BLUE_BERRY        '^l'
#define S_COLOR_TURQUOISE         '^m'
#define S_COLOR_WILD_WATERMELON   '^n'
#define S_COLOR_SALTPAN           '^o'
#define S_COLOR_GRAY_CHATEAU      '^p'
#define S_COLOR_RUST              '^q'
#define S_COLOR_COPPER_GREEN      '^r'
#define S_COLOR_GOLD              '^s'
#define S_COLOR_STEEL_BLUE        '^t'
#define S_COLOR_STEEL_GRAY        '^u'
#define S_COLOR_BRONZE            '^v'
#define S_COLOR_SILVER            '^w'
#define S_COLOR_DARK_GRAY         '^x'
#define S_COLOR_DARK_ORANGE       '^y'
#define S_COLOR_DARK_GREEN        '^z'
#define S_COLOR_RED_ORANGE        '^A'
#define S_COLOR_FOREST_GREEN      '^B'
#define S_COLOR_BRIGHT_SUN        '^C'
#define S_COLOR_MEDIUM_SLATE_BLUE '^D'
#define S_COLOR_CELESTE           '^E'
#define S_COLOR_IRONSTONE         '^F'
#define S_COLOR_TIMBERWOLF        '^G'
#define S_COLOR_ONYX              '^H'
#define S_COLOR_ROSEWOOD          '^I'
#define S_COLOR_KOKODA            '^J'
#define S_COLOR_PORSCHE           '^K'
#define S_COLOR_CLOUD_BURST       '^L'
#define S_COLOR_BLUE_DIANE        '^M'
#define S_COLOR_ROPE              '^N'
#define S_COLOR_BLONDE            '^O'
#define S_COLOR_SMOKEY_BLACK      '^P'
#define S_COLOR_AMERICAN_ROSE     '^Q'
#define S_COLOR_NEON_GREEN        '^R'
#define S_COLOR_NEON_YELLOW       '^S'
#define S_COLOR_ULTRAMARINE       '^T'
#define S_COLOR_TURQUOISE_BLUE    '^U'
#define S_COLOR_DARK_MAGENTA      '^V'
#define S_COLOR_MAGIC_MINT        '^W'
#define S_COLOR_LIGHT_GRAY        '^X'
#define S_COLOR_LIGHT_SALMON      '^Y'
#define S_COLOR_LIGHT_GREEN       '^Z'
#define S_COLOR_NULL              "^*"

void Q_GetVectFromHexColor(pointer color_code, vec4_t color);
sint Q_ApproxBasicColorIndexFromVectColor(const vec4_t color);

// Dushan - Tremulous
#define INDENT_MARKER       '\v'
void Q_StripIndentMarker(valueType *string);

#define MAX_CCODES  62

extern vec4_t g_color_table[MAX_CCODES];

#define MAKERGB( v, r, g, b ) v[0] = r; v[1] = g; v[2] = b
#define MAKERGBA( v, r, g, b, a ) v[0] = r; v[1] = g; v[2] = b; v[3] = a

// Hex Color string support
#define gethex( ch ) ( ( ch ) > '9' ? ( ( ch ) >= 'a' ? ( ( ch ) - 'a' + 10 ) : ( ( ch ) - '7' ) ) : ( ( ch ) - '0' ) )
#define ishex( ch )  ( ( ch ) && ( ( ( ch ) >= '0' && ( ch ) <= '9' ) || ( ( ch ) >= 'A' && ( ch ) <= 'F' ) || ( ( ch ) >= 'a' && ( ch ) <= 'f' ) ) )
// check if it's format rrggbb r,g,b e {0..9} U {A...F}
#define Q_IsHexColorString( p ) ( ishex( *( p ) ) && ishex( *( ( p ) + 1 ) ) && ishex( *( ( p ) + 2 ) ) && ishex( *( ( p ) + 3 ) ) && ishex( *( ( p ) + 4 ) ) && ishex( *( ( p ) + 5 ) ) )
#define Q_HexColorStringHasAlpha( p ) ( ishex( *( ( p ) + 6 ) ) && ishex( *( ( p ) + 7 ) ) )

#define DEG2RAD( a ) ( ( ( a ) * M_PI ) / 180.0F )
#define RAD2DEG( a ) ( ( ( a ) * 180.0f ) / M_PI )

#define Q_max(a, b)      ((a) > (b) ? (a) : (b))
#define Q_min(a, b)      ((a) < (b) ? (a) : (b))
#define Q_bound(a, b, c) (Q_max(a, Q_min(b, c)))
#define Q_clamp(a, b, c) ((b) >= (c) ? (a)=(b) : (a) < (b) ? (a)=(b) : (a) > (c) ? (a)=(c) : (a))
#define Q_lerp(from, to, frac) (from + ((to - from) * frac))

struct cplane_s;

extern vec3_t vec3_origin;
extern vec3_t axisDefault[3];
extern matrix_t matrixIdentity;
extern quat_t   quatIdentity;

#define nanmask ( 255 << 23 )

#define IS_NAN( x ) ( ( ( *(sint *)&x ) & nanmask ) == nanmask )

static ID_INLINE float32 Q_fabs(float32 x) {
    floatint_t      tmp;

    tmp.f = x;
    tmp.i &= 0x7FFFFFFF;
    return tmp.f;
}

uchar8 ClampByte(sint i);
schar8 ClampChar(sint i);
schar16 ClampShort(sint i);

// this isn't a real cheap function to call!
sint DirToByte(vec3_t dir);
void ByteToDir(sint b, vec3_t dir);

#define DotProduct( x,y )         ( ( x )[0] * ( y )[0] + ( x )[1] * ( y )[1] + ( x )[2] * ( y )[2] )
#define VectorCopy( a,b )         ( ( b )[0] = ( a )[0],( b )[1] = ( a )[1],( b )[2] = ( a )[2] )
#define VectorSubtract( a,b,c )   ( ( c )[0] = ( a )[0] - ( b )[0],( c )[1] = ( a )[1] - ( b )[1],( c )[2] = ( a )[2] - ( b )[2] )
#define VectorAdd( a,b,c )        ( ( c )[0] = ( a )[0] + ( b )[0],( c )[1] = ( a )[1] + ( b )[1],( c )[2] = ( a )[2] + ( b )[2] )
#define VectorScale( v, s, o )    ( ( o )[0] = ( v )[0] * ( s ),( o )[1] = ( v )[1] * ( s ),( o )[2] = ( v )[2] * ( s ) )
#define VectorMA( v, s, b, o )    ( ( o )[0] = ( v )[0] + ( b )[0] * ( s ),( o )[1] = ( v )[1] + ( b )[1] * ( s ),( o )[2] = ( v )[2] + ( b )[2] * ( s ) )
#define VectorLerpTrem( f, s, e, r ) ((r)[0]=(s)[0]+(f)*((e)[0]-(s)[0]),\
                                      (r)[1]=(s)[1]+(f)*((e)[1]-(s)[1]),\
                                      (r)[2]=(s)[2]+(f)*((e)[2]-(s)[2]))

#define VectorClear( a )              ( ( a )[0] = ( a )[1] = ( a )[2] = 0 )
#define VectorNegate( a,b )           ( ( b )[0] = -( a )[0],( b )[1] = -( a )[1],( b )[2] = -( a )[2] )
#define VectorSet( v, x, y, z )       ( ( v )[0] = ( x ), ( v )[1] = ( y ), ( v )[2] = ( z ) )

#define Vector2Set( v, x, y )         ( ( v )[0] = ( x ),( v )[1] = ( y ) )
#define Vector2Copy( a,b )            ( ( b )[0] = ( a )[0],( b )[1] = ( a )[1] )
#define Vector2Subtract( a,b,c )      ( ( c )[0] = ( a )[0] - ( b )[0],( c )[1] = ( a )[1] - ( b )[1] )
#define QuatCopy(a, b) ((b)[0] = (a)[0], (b)[1] = (a)[1], (b)[2] = (a)[2], (b)[3] = (a)[3])
#define Vector4Set( v, x, y, z, n )   ( ( v )[0] = ( x ),( v )[1] = ( y ),( v )[2] = ( z ),( v )[3] = ( n ) )
#define Vector4Copy( a,b )            ( ( b )[0] = ( a )[0],( b )[1] = ( a )[1],( b )[2] = ( a )[2],( b )[3] = ( a )[3] )
#define Vector4MA( v, s, b, o )       ( ( o )[0] = ( v )[0] + ( b )[0] * ( s ),( o )[1] = ( v )[1] + ( b )[1] * ( s ),( o )[2] = ( v )[2] + ( b )[2] * ( s ),( o )[3] = ( v )[3] + ( b )[3] * ( s ) )
#define Vector4Average( v, b, s, o )  ( ( o )[0] = ( ( v )[0] * ( 1 - ( s ) ) ) + ( ( b )[0] * ( s ) ),( o )[1] = ( ( v )[1] * ( 1 - ( s ) ) ) + ( ( b )[1] * ( s ) ),( o )[2] = ( ( v )[2] * ( 1 - ( s ) ) ) + ( ( b )[2] * ( s ) ),( o )[3] = ( ( v )[3] * ( 1 - ( s ) ) ) + ( ( b )[3] * ( s ) ) )
#define Vector4Lerp( f, s, e, r ) ((r)[0]=(s)[0]+(f)*((e)[0]-(s)[0]),\
                                   (r)[1]=(s)[1]+(f)*((e)[1]-(s)[1]),\
                                   (r)[2]=(s)[2]+(f)*((e)[2]-(s)[2]),\
                                   (r)[3]=(s)[3]+(f)*((e)[3]-(s)[3]))

#define SnapVector( v ) {v[0] = ( (sint)( v[0] ) ); v[1] = ( (sint)( v[1] ) ); v[2] = ( (sint)( v[2] ) );}

uint ColorBytes4(float32 r, float32 g, float32 b, float32 a);

float32 NormalizeColor(const vec3_t in, vec3_t out);
void  ClampColor(vec4_t color);

float32 RadiusFromBounds(const vec3_t mins, const vec3_t maxs);
void ZeroBounds(vec3_t mins, vec3_t maxs);
void ClearBounds(vec3_t mins, vec3_t maxs);
void AddPointToBounds(const vec3_t v, vec3_t mins, vec3_t maxs);

// RB: same as BoundsIntersectPoint but kept for compatibility
bool PointInBounds(const vec3_t v, const vec3_t mins, const vec3_t maxs);

void BoundsAdd(vec3_t mins, vec3_t maxs, const vec3_t mins2,
               const vec3_t maxs2);
bool BoundsIntersect(const vec3_t mins, const vec3_t maxs,
                     const vec3_t mins2, const vec3_t maxs2);
bool BoundsIntersectSphere(const vec3_t mins, const vec3_t maxs,
                           const vec3_t origin, vec_t radius);
bool BoundsIntersectPoint(const vec3_t mins, const vec3_t maxs,
                          const vec3_t origin);

sint VectorCompare(const vec3_t v1, const vec3_t v2);

static ID_INLINE sint Vector4Compare(const vec4_t v1, const vec4_t v2) {
    if(v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2] || v1[3] != v2[3]) {
        return 0;
    }

    return 1;
}

static ID_INLINE void VectorLerp(const vec3_t from, const vec3_t to,
                                 float32 frac, vec3_t out) {
    out[0] = from[0] + ((to[0] - from[0]) * frac);
    out[1] = from[1] + ((to[1] - from[1]) * frac);
    out[2] = from[2] + ((to[2] - from[2]) * frac);
}

//Dushan - Tremulous
#define VectorLerp4( f, s, e, r ) ((r)[0]=(s)[0]+(f)*((e)[0]-(s)[0]),\
                                   (r)[1]=(s)[1]+(f)*((e)[1]-(s)[1]),\
                                   (r)[2]=(s)[2]+(f)*((e)[2]-(s)[2]))

static ID_INLINE sint VectorCompareEpsilon(
    const vec3_t v1, const vec3_t v2, float32 epsilon) {
    vec3_t d;

    VectorSubtract(v1, v2, d);
    d[0] = fabs(d[0]);
    d[1] = fabs(d[1]);
    d[2] = fabs(d[2]);

    if(d[0] > epsilon || d[1] > epsilon || d[2] > epsilon) {
        return 0;
    }

    return 1;
}

vec_t VectorLength(const vec3_t v);
vec_t VectorLengthSquared(const vec3_t v);
vec_t Distance(const vec3_t p1, const vec3_t p2);
vec_t DistanceSquared(const vec3_t p1, const vec3_t p2);
void CrossProduct(const vec3_t v1, const vec3_t v2, vec3_t cross);
vec_t VectorNormalize(vec3_t v);       // returns vector length
void VectorNormalizeFast(vec3_t
                         v);     // does NOT return vector length, uses rsqrt approximation
vec_t VectorNormalize2(const vec3_t v, vec3_t out);
void VectorInverse(vec3_t v);
void Vector4Scale(const vec4_t in, vec_t scale, vec4_t out);
void VectorRotate(vec3_t in, vec3_t matrix[3], vec3_t out);

sint NearestPowerOfTwo(sint val);
sint Q_log2(sint val);
float32 Q_acos(float32 c);
sint Q_isnan(float32 x);
float32   Q_random(sint *seed);
float32   Q_crandom(sint *seed);

#define random()     ( ( rand() & 0x7FFF ) / ( static_cast< float32>(0x8000 ) ))
#define crandom()   ( 2.0f * ( ( ( rand() & 0x7FFF ) / ( static_cast< float32>(0x7FFF )) ) - 0.5f ) )

void vectoangles(const vec3_t value1, vec3_t angles);

static ID_INLINE void VectorToAngles(const vec3_t value1, vec3_t angles) {
    vectoangles(value1, angles);
}

float32 vectoyaw(const vec3_t vec);
void AnglesToAxis(const vec3_t angles, vec3_t axis[3]);
// TTimo: const vec_t ** would require explicit casts for ANSI C conformance
// see unix/const-arg.c
void AxisToAngles(/*const*/ vec3_t axis[3], vec3_t angles);
//void AxisToAngles ( const vec3_t axis[3], vec3_t angles );
float32 VectorDistance(vec3_t v1, vec3_t v2);
float32 VectorDistanceSquared(vec3_t v1, vec3_t v2);

float32 VectorMinComponent(vec3_t v);
float32 VectorMaxComponent(vec3_t v);

void AxisClear(vec3_t axis[3]);
void AxisCopy(vec3_t in[3], vec3_t out[3]);

void SetPlaneSignbits(struct cplane_s *out);

float32   AngleMod(float32 a);
float32   LerpAngle(float32 from, float32 to, float32 frac);
void    LerpPosition(vec3_t start, vec3_t end, float32 frac, vec3_t out);
float32   AngleSubtract(float32 a1, float32 a2);
void    AnglesSubtract(vec3_t v1, vec3_t v2, vec3_t v3);

float32         AngleNormalize2Pi(float32 angle);
float32         AngleNormalize360(float32 angle);
float32         AngleNormalize180(float32 angle);
float32         AngleDelta(float32 angle1, float32 angle2);
float32           AngleBetweenVectors(const vec3_t a, const vec3_t b);
void            AngleVectors(const vec3_t angles, vec3_t forward,
                             vec3_t right, vec3_t up);

static ID_INLINE void AnglesToVector(const vec3_t angles, vec3_t out) {
    AngleVectors(angles, out, nullptr, nullptr);
}

void            VectorToAngles(const vec3_t value1, vec3_t angles);

vec_t           PlaneNormalize(vec4_t plane);    // returns normal length
/* greebo: This calculates the intersection point of three planes.
 * Returns <0,0,0> if no intersection point could be found, otherwise returns the coordinates of the intersection point
 * (this may also be 0,0,0) */
bool        PlanesGetIntersectionPoint(const vec4_t plane1,
                                       const vec4_t plane2, const vec4_t plane3, vec3_t out);
void            PlaneIntersectRay(const vec3_t rayPos, const vec3_t rayDir,
                                  const vec4_t plane, vec3_t res);

bool        PlaneFromPoints(vec4_t plane, const vec3_t a, const vec3_t b,
                            const vec3_t c, bool cw);
bool        PlaneFromPointsOrder(vec4_t plane, const vec3_t a,
                                 const vec3_t b, const vec3_t c, bool cw);
void            ProjectPointOnPlane(vec3_t dst, const vec3_t p,
                                    const vec3_t normal);
void            RotatePointAroundVector(vec3_t dst, const vec3_t dir,
                                        const vec3_t point, float32 degrees);
void            RotatePointAroundVertex(vec3_t pnt, float32 rot_x,
                                        float32 rot_y, float32 rot_z, const vec3_t origin);
void            RotateAroundDirection(vec3_t axis[3], float32 yaw);
void            MakeNormalVectors(const vec3_t forward, vec3_t right,
                                  vec3_t up);
// perpendicular vector could be replaced by this

//sint              PlaneTypeForNormal( vec3_t normal );

void            VectorMatrixMultiply(const vec3_t p, vec3_t m[3],
                                     vec3_t out);

// RB: NOTE renamed MatrixMultiply to AxisMultiply because it conflicts with most new matrix functions
// It is important for mod developers to do this change as well or they risk a memory corruption by using
// the other MatrixMultiply function.
void            AxisMultiply(float32 in1[3][3], float32 in2[3][3],
                             float32 out[3][3]);
void            AngleVectors(const vec3_t angles, vec3_t forward,
                             vec3_t right, vec3_t up);
void            PerpendicularVector(vec3_t dst, const vec3_t src);

// Ridah
void            GetPerpendicularViewVector(const vec3_t point,
        const vec3_t p1, const vec3_t p2, vec3_t up);
void            ProjectPointOntoVector(vec3_t point, vec3_t vStart,
                                       vec3_t vEnd, vec3_t vProj);
void            ProjectPointOntoVectorBounded(vec3_t point, vec3_t vStart,
        vec3_t vEnd, vec3_t vProj);
float32         DistanceFromLineSquared(vec3_t p, vec3_t lp1, vec3_t lp2);
float32         DistanceFromVectorSquared(vec3_t p, vec3_t lp1,
        vec3_t lp2);
// done.

vec_t           DistanceBetweenLineSegmentsSquared(const vec3_t sP0,
        const vec3_t sP1, const vec3_t tP0, const vec3_t tP1, float32 *s,
        float32 *t);
vec_t           DistanceBetweenLineSegments(const vec3_t sP0,
        const vec3_t sP1, const vec3_t tP0, const vec3_t tP1, float32 *s,
        float32 *t);

void            MatrixFromAngles(matrix_t m, vec_t pitch, vec_t yaw,
                                 vec_t roll);
void            MatrixSetupTransformFromRotation(matrix_t m,
        const matrix_t rot, const vec3_t origin);
void            MatrixAffineInverse(const matrix_t in, matrix_t out);
void            MatrixTransformNormal(const matrix_t m, const vec3_t in,
                                      vec3_t out);
void            MatrixTransformNormal2(const matrix_t m, vec3_t inout);
void            MatrixTransformPoint(const matrix_t m, const vec3_t in,
                                     vec3_t out);

//=============================================

#ifndef MAX
#define MAX(x,y) ((x)>(y)?(x):(y))
#endif

#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif

//=============================================

//Dushan same as Com_Clamp just for integers
sint Com_Clampi(sint min, sint max, sint value);
float32 Com_Clamp(float32 min, float32 max, float32 value);

valueType *Com_SkipTokens(valueType *s, sint numTokens, valueType *sep);
valueType *Com_SkipCharset(valueType *s, valueType *sep);
pointer COM_GetExtension(pointer name);
void    COM_StripExtension(pointer in, valueType *out);
void    COM_StripExtension2(pointer in, valueType *out, sint destsize);
void    COM_StripExtension3(pointer src, valueType *dest, sint destsize);
void    COM_DefaultExtension(valueType *path, sint maxSize,
                             pointer extension);

void    COM_BeginParseSession(pointer name);
void    COM_RestoreParseSession(valueType **data_p);
void    COM_SetCurrentParseLine(sint line);
sint     COM_GetCurrentParseLine(void);
valueType *COM_Parse(valueType **data_p);

// RB: added COM_Parse2 for having a Doom 3 style tokenizer.
valueType *COM_Parse2(valueType **data_p);
valueType *COM_ParseExt2(valueType **data_p, bool allowLineBreak);

valueType *COM_ParseExt(valueType **data_p, bool allowLineBreak);
sint     COM_Compress(valueType *data_p);
void    COM_ParseError(valueType *format, ...) _attribute((format(printf,
        1, 2)));
void    COM_ParseWarning(valueType *format, ...) _attribute((format(printf,
        1, 2)));
sint COM_Parse2Infos(valueType *buf, sint max,
                     valueType infos[][MAX_INFO_STRING]);

bool COM_BitCheck(const sint array[], sint bitNum);
void COM_BitSet(sint array[], sint bitNum);
void COM_BitClear(sint array[], sint bitNum);

sint     Com_HashKey(valueType *string, sint maxlen);

#define MAX_TOKENLENGTH     1024

#ifndef TT_STRING
//token types
#define TT_STRING                   1           // string
#define TT_LITERAL                  2           // literal
#define TT_NUMBER                   3           // number
#define TT_NAME                     4           // name
#define TT_PUNCTUATION              5           // punctuation
#endif

typedef struct pc_token_s {
    sint type;
    sint subtype;
    sint intvalue;
    float32 floatvalue;
    valueType string[MAX_TOKENLENGTH];
    sint line;
    sint linescrossed;
} pc_token_t;

// data is an in/out parm, returns a parsed out token

void COM_MatchToken(valueType **buf_p, valueType *match);

void COM_Parse21DMatrix(valueType **buf_p, sint x, float32 *m,
                        bool checkBrackets);
void COM_Parse22DMatrix(valueType **buf_p, sint y, sint x, float32 *m);
void COM_Parse23DMatrix(valueType **buf_p, sint z, sint y, sint x,
                        float32 *m);

valueType *Com_StringContains(valueType *str1, valueType *str2,
                              sint casesensitive);

bool SkipBracedSection(valueType **program);
bool SkipBracedSection_Depth(valueType **program,
                             sint depth);   // start at given depth if already
void SkipRestOfLine(valueType **data);

sint Q_vsprintf_s(valueType *strDest, uint64 destMax, uint64 count,
                  pointer format, ...);
void Q_vsprintf_s(valueType *pDest, uint32 nDestSize, pointer pFmt,
                  va_list args);

template< uint32 nDestSize >
ID_INLINE void Q_vsprintf_s(valueType(&pDest)[nDestSize], pointer pFmt,
                            va_list args) {
    Q_vsprintf_s(pDest, nDestSize, pFmt, args);
}

// mode parm for FS_FOpenFile
enum fsMode_t {
    FS_READ,
    FS_WRITE,
    FS_APPEND,
    FS_APPEND_SYNC,
    FS_READ_DIRECT,
    FS_UPDATE
};

enum fsOrigin_t {
    FS_SEEK_CUR,
    FS_SEEK_END,
    FS_SEEK_SET
};

sint Com_HexStrToInt(pointer str);

pointer Com_QuoteStr(pointer str);
pointer Com_UnquoteStr(pointer str);

//=============================================

sint Q_isprint(sint c);
sint Q_islower(sint c);
sint Q_isupper(sint c);
sint Q_isalpha(sint c);
sint Q_isnumeric(sint c);
sint Q_isalphanumeric(sint c);
sint Q_isforfilename(sint c);

bool Q_isanumber(pointer s);
bool Q_isintegral(float32 f);

bool        Q_strtol(pointer s, sint32 *out);
bool        Q_strtoi(pointer s, sint *out);

[[nodiscard]]
ID_INLINE uint32 Q_strlen(pointer str) {
    return static_cast<uint32>(strlen(str));
}

// Safe strcpy that ensures null termination
// Returns bytes written
void Q_strcpy_s(valueType *pDest, uint32 nDestSize, pointer pSrc);

template< uint32 nDestSize >
ID_INLINE void Q_strcpy_s(valueType(&pDest)[nDestSize], pointer pSrc) {
    Q_strcpy_s(pDest, nDestSize, pSrc);
}

sint Q_vsprintf_s(valueType *strDest, uint64 destMax, uint64 count,
                  pointer format, ...);

template< uint32 nDestSize >
ID_INLINE void Q_vsprintf_s(valueType(&strDest)[nDestSize], uint64 destMax,
                            uint64 count, pointer format, ...) {
    Q_vsprintf_s(strDest, destMax, count, format);
}

sint     Q_stricmp(pointer s1, pointer s2);
sint     Q_strncmp(pointer s1, pointer s2, sint n);
sint     Q_stricmpn(pointer s1, pointer s2, sint n);
valueType *Q_strlwr(valueType *s1);
valueType *Q_strupr(valueType *s1);
valueType *Q_strrchr(pointer string, sint c);
pointer Q_stristr(pointer s, pointer find);

#ifdef _WIN32
#define Q_putenv _putenv
#else
#define Q_putenv putenv
#endif

// buffer size safe library replacements
// Dushan
// NOTE : had problem with loading QVM modules
#ifndef _DEBUG
void            Q_strncpyz(valueType *dest, pointer src, sint destsize);
#else
#define         Q_strncpyz(string1,string2,length) Q_strncpyzDebug( string1, string2, length, __FILE__, __LINE__ )
void            Q_strncpyzDebug(valueType *dest, pointer src,
                                uint32 destsize, pointer file, sint line) __attribute__((nonnull));
#endif
void            Q_strcat(valueType *dest, sint destsize, pointer src);
sint                Q_strnicmp(pointer string1, pointer string2, sint n);
bool        Q_strreplace(valueType *dest, sint destsize, pointer find,
                         pointer replace);
void Q_strstrip(valueType *string, pointer strip, pointer repl);

// strlen that discounts Quake color sequences
sint Q_PrintStrlen(pointer string);
// removes color sequences from string
valueType *Q_CleanStr(valueType *string);
void Q_ApproxStrHexColors(
    pointer in_string, valueType *out_string,
    const uint32 in_string_length, const uint32 out_string_length);
void Q_StringToLower(valueType *in, valueType *out, sint len);
void Q_RemoveUnusedColorStrings(valueType *in, valueType *out, sint len);
// Count the number of valueType tocount encountered in string
sint Q_CountChar(pointer string, valueType tocount);
// removes whitespaces and other bad directory characters
valueType *Q_CleanDirName(valueType *dirname);

//=============================================

valueType *va(pointer format, ...) __attribute__((format(printf, 1, 2)));

//=============================================

//
// key / value info strings
//
valueType *Info_ValueForKey(pointer s, pointer key);
void Info_RemoveKey(valueType *s, pointer key);
void Info_RemoveKey_big(valueType *s, pointer key);
bool Info_SetValueForKey(valueType *s, pointer key, pointer value);
void Info_SetValueForKey_Big(valueType *s, pointer key, pointer value);
bool Info_Validate(pointer s);
void Info_NextPair(pointer *s, valueType *key, valueType *value);

// this is only here so the functions in q_shared.c and bg_*.c can link
void Com_Error(sint level, pointer error, ...) _attribute((format(printf,
        2, 3), noreturn));
void Com_FatalError(pointer error, ...);
void Com_DropError(pointer error, ...);
void Com_Warning(pointer error, ...);
void Com_Printf(pointer msg, ...) _attribute((format(printf, 1, 2)));

/*
==========================================================

  RELOAD STATES

==========================================================
*/

#define RELOAD_SAVEGAME         0x01
#define RELOAD_NEXTMAP          0x02
#define RELOAD_NEXTMAP_WAITING  0x04
#define RELOAD_FAILED           0x08
#define RELOAD_ENDGAME          0x10


//=====================================================================


// in order from highest priority to lowest
// if none of the catchers are active, bound key strings will be executed
#define KEYCATCH_CONSOLE        0x0001
#define KEYCATCH_UI             0x0002
#define KEYCATCH_MESSAGE        0x0004
#define KEYCATCH_CGAME          0x0008
#define KEYCATCH_BUG            0x0010


// sound channels
// channel 0 never willingly overrides
// other channels will allways override a playing sound on that channel
enum soundChannel_t {
    CHAN_AUTO,
    CHAN_LOCAL,     // menu sounds, etc
    CHAN_WEAPON,
    CHAN_VOICE,
    CHAN_ITEM,
    CHAN_BODY,
    CHAN_LOCAL_SOUND,   // chat messages, etc
    CHAN_ANNOUNCER,     // announcer voices, etc
    CHAN_VOICE_BG,  // xkan - background sound for voice (radio static, etc.)
};


/*
========================================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

========================================================================
*/
#define ANIM_BITS       10

#define ANGLE2SHORT( x )  ( (sint)( ( x ) * 65536 / 360 ) & 65535 )
#define SHORT2ANGLE( x )  ( ( x ) * ( 360.0f / 65536 ) )

#define SNAPFLAG_RATE_DELAYED   1
#define SNAPFLAG_NOT_ACTIVE     2   // snapshot used during connection and for zombies
#define SNAPFLAG_SERVERCOUNT    4   // toggled every map_restart so transitions can be detected

//
// per-level limits
//
#define CLIENTNUM_BITS      7
#define MAX_CLIENTS         (1<<CLIENTNUM_BITS)     // absolute limit

#define GENTITYNUM_BITS     11  // JPW NERVE put q3ta default back for testing  // don't need to send any more

#define MAX_GENTITIES       ( 1 << GENTITYNUM_BITS )

// tjw: used for limiting weapons that may overflow gentities[]
#define MIN_SPARE_GENTITIES 64

// entitynums are communicated with GENTITY_BITS, so any reserved
// values thatare going to be communcated over the net need to
// also be in this range
#define ENTITYNUM_NONE      ( MAX_GENTITIES - 1 )
#define ENTITYNUM_WORLD     ( MAX_GENTITIES - 2 )
#define ENTITYNUM_MAX_NORMAL    ( MAX_GENTITIES - 2 )

#define MAX_MODELS          256     // these are sent over the net as 8 bits (Gordon: upped to 9 bits, erm actually it was already at 9 bits, wtf? NEVAR TRUST GAMECODE COMMENTS, comments are evil :E, lets hope it doesnt horribly break anything....)
#define MAX_SOUNDS          256     // so they cannot be blindly increased
#define MAX_CS_SKINS        64
#define MAX_CSSTRINGS       32
#define MAX_EFFECTS         256
#define MAX_FX              64
#define MAX_CS_SHADERS      32
#define MAX_SERVER_TAGS     256
#define MAX_TAG_FILES       64

#define MAX_MULTI_SPAWNTARGETS  16 // JPW NERVE

#define MAX_CONFIGSTRINGS   20480

#define MAX_DLIGHT_CONFIGSTRINGS    16
#define MAX_SPLINE_CONFIGSTRINGS    8

#define PARTICLE_SNOW128    1
#define PARTICLE_SNOW64     2
#define PARTICLE_SNOW32     3
#define PARTICLE_SNOW256    0

#define PARTICLE_BUBBLE8    4
#define PARTICLE_BUBBLE16   5
#define PARTICLE_BUBBLE32   6
#define PARTICLE_BUBBLE64   7

// these are the only configstrings that the system reserves, all the
// other ones are strictly for servergame to clientgame communication
#define CS_SERVERINFO       0       // an info string with all the serverinfo cvars
#define CS_SYSTEMINFO       1       // an info string for server system to client system configuration (timescale, etc)

#define RESERVED_CONFIGSTRINGS  2   // game can't modify below this, only the system can

#define MAX_GAMESTATE_CHARS 16000
typedef struct {
    uint64 stringOffsets[MAX_CONFIGSTRINGS];
    valueType stringData[MAX_GAMESTATE_CHARS];
    uint64 dataCount;
} gameState_t;

// xkan, 1/10/2003 - adapted from original SP
enum aistateEnum_t {
    AISTATE_RELAXED,
    AISTATE_QUERY,
    AISTATE_ALERT,
    AISTATE_COMBAT,

    MAX_AISTATES
};

#define REF_FORCE_DLIGHT    ( 1 << 31 ) // RF, passed in through overdraw parameter, force this dlight under all conditions
#define REF_JUNIOR_DLIGHT   ( 1 << 30 ) // (SA) this dlight does not light surfaces.  it only affects dynamic light grid
#define REF_DIRECTED_DLIGHT ( 1 << 29 ) // ydnar: global directional light, origin should be interpreted as a normal vector

// bit field limits
#define MAX_STATS               16
#define MAX_PERSISTANT          16
#define MAX_MISC                16  // Dushan - Tremulous
#define MAX_POWERUPS            16
#define MAX_WEAPONS             64  // (SA) and yet more!

#define MAX_EVENTS              4   // max events per frame before we drop events

#define PS_PMOVEFRAMECOUNTBITS  6

// plane_t structure
// !!! if this is changed, it must be changed in asm code too !!!
typedef struct cplane_s {
    vec3_t normal;
    float32 dist;
    uchar8 type;              // for fast side tests: 0,1,2 = axial, 3 = nonaxial
    uchar8 signbits;          // signx + (signy<<1) + (signz<<2), used as lookup during collision
    uchar8 pad[2];
} cplane_t;

// a trace is returned when a box is swept through the world
typedef struct {
    bool allsolid;      // if true, plane is not valid
    bool startsolid;    // if true, the initial point was in a solid area
    float32 fraction;         // time completed, 1.0 = didn't hit anything
    vec3_t endpos;          // final position
    cplane_t plane;         // surface normal at impact, transformed to world space
    sint surfaceFlags;           // surface hit
    sint contents;           // contents on other side of surface hit
    sint entityNum;          // entity the contacted sirface is a part of
    float32 lateralFraction;  // fraction of collision tangetially to the trace direction
} trace_t;

// playerState_t is the information needed by both the client and server
// to predict player motion and actions
// nothing outside of pmove should modify these, or some degree of prediction error
// will occur

// you can't add anything to this without modifying the code in msg.c
// (Gordon: unless it doesnt need transmitted over the network, in which case it should prolly go in the new pmext struct anyway)

// playerState_t is a full superset of entityState_t as it is used by players,
// so if a playerState_t is transmitted, the entityState_t can be fully derived
// from it.
//
// NOTE: all fields in here must be 32 bits (or those within sub-structures)
typedef struct playerState_s {
    sint commandTime;            // cmd->serverTime of last executed command
    sint pm_type;
    sint bobCycle;               // for view bobbing and footstep generation
    sint pm_flags;               // ducked, jump_held, etc
    sint pm_time;

    vec3_t origin;
    vec3_t velocity;
    sint weaponTime;
    sint weaponDelay;            // for weapons that don't fire immediately when 'fire' is hit (grenades, venom, ...)
    sint grenadeTimeLeft;            // for delayed grenade throwing.  this is set to a #define for grenade
    // lifetime when the attack button goes down, then when attack is released
    // this is the amount of time left before the grenade goes off (or if it
    // gets to 0 while in players hand, it explodes)


    sint gravity;
    float32 leanf;                // amount of 'lean' when player is looking around corner //----(SA)   added

    sint speed;
    sint delta_angles[3];            // add to command angles to get view direction
    // changed by spawns, rotating objects, and teleporters

    sint groundEntityNum;        // ENTITYNUM_NONE = in air

    sint legsTimer;              // don't change low priority animations until this runs out
    sint legsAnim;               // mask off ANIM_TOGGLEBIT

    sint torsoTimer;             // don't change low priority animations until this runs out
    sint torsoAnim;              // mask off ANIM_TOGGLEBIT

    sint movementDir;            // a number 0 to 7 that represents the reletive angle
    // of movement to the view angle (axial and diagonals)
    // when at rest, the value will remain unchanged
    // used to twist the legs during strafing



    sint eFlags;                 // copied to entityState_t->eFlags

    sint eventSequence;          // pmove generated events
    sint events[MAX_EVENTS];
    sint eventParms[MAX_EVENTS];
    sint oldEventSequence;           // so we can see which events have been added since we last converted to entityState_t

    sint externalEvent;          // events set on player from another source
    sint externalEventParm;
    sint externalEventTime;

    sint clientNum;              // ranges from 0 to MAX_CLIENTS-1

    // weapon info
    sint weapon;                 // copied to entityState_t->weapon
    sint weaponstate;

    // item info
    sint item;

    vec3_t viewangles;          // for fixed views
    sint viewheight;

    // damage feedback
    sint damageEvent;            // when it changes, latch the other parms
    sint damageYaw;
    sint damagePitch;
    sint damageCount;

    sint stats[MAX_STATS];
    sint persistant[MAX_PERSISTANT];         // stats that aren't cleared on death
    sint powerups[MAX_POWERUPS];         // level.time that the powerup runs out
    sint ammo;              // total amount of ammo
    sint ammoclip;          // ammo in clip
    sint holdable[16];
    sint holding;                        // the current item in holdable[] that is selected (held)
    sint weapons[MAX_WEAPONS / (sizeof(sint) *
                                8)];     // 64 bits for weapons held

    // Ridah, allow for individual bounding boxes
    vec3_t mins, maxs;
    float32 crouchMaxZ;
    float32 crouchViewHeight, standViewHeight, deadViewHeight;
    // variable movement speed
    float32 runSpeedScale, sprintSpeedScale, crouchSpeedScale;
    // done.

    // Ridah, view locking for mg42
    sint viewlocked;
    sint viewlocked_entNum;

    float32 friction;

    sint nextWeapon;
    sint teamNum;                        // Arnout: doesn't seem to be communicated over the net

    // Rafael
    //sint          gunfx;

    // RF, burning effect is required for view blending effect
    sint onFireStart;

    sint serverCursorHint;               // what type of cursor hint the server is dictating
    sint serverCursorHintVal;            // a value (0-255) associated with the above

    trace_t serverCursorHintTrace;      // not communicated over net, but used to store the current server-side cursorhint trace

    // ----------------------------------------------------------------------
    // So to use persistent variables here, which don't need to come from the server,
    // we could use a marker variable, and use that to store everything after it
    // before we read in the new values for the predictedPlayerState, then restore them
    // after copying the structure recieved from the server.

    // Arnout: use the pmoveExt_t structure in bg_public.hpp to store this kind of data now (presistant on client, not network transmitted)

    sint ping;                   // server to game info for scoreboard
    sint pmove_framecount;
    sint entityEventSequence;

    sint sprintExertTime;

    // JPW NERVE -- value for all multiplayer classes with regenerating "class weapons" -- ie LT artillery, medic medpack, engineer build points, etc
    sint classWeaponTime;                // Arnout : DOES get send over the network
    sint jumpTime;                   // used in MP to prevent jump accel
    // jpw

    sint weapAnim;                   // mask off ANIM_TOGGLEBIT                                     //----(SA)  added       // Arnout : DOES get send over the network

    bool releasedFire;

    float32 aimSpreadScaleFloat;          // (SA) the server-side aimspreadscale that lets it track finer changes but still only
    // transmit the 8bit sint to the client
    sint aimSpreadScale;                 // 0 - 255 increases with angular movement     // Arnout : DOES get send over the network
    sint lastFireTime;                   // used by server to hold last firing frame briefly when randomly releasing trigger (AI)

    sint quickGrenTime;

    sint leanStopDebounceTime;

    //----(SA)  added

    // seems like heat and aimspread could be tied together somehow, however, they (appear to) change at different rates and
    // I can't currently see how to optimize this to one server->client transmission "weapstatus" value.
    sint weapHeat[MAX_WEAPONS];          // some weapons can overheat.  this tracks (server-side) how hot each weapon currently is.
    sint curWeapHeat;                    // value for the currently selected weapon (for transmission to client)        // Arnout : DOES get send over the network
    sint identifyClient;                 // NERVE - SMF
    sint identifyClientHealth;

    aistateEnum_t aiState;          // xkan, 1/10/2003

    // Dushan - Tremulous
    sint    generic1;
    sint    loopSound;
    sint    otherEntityNum;
    vec3_t grapplePoint;    // location of grapple to pull towards if PMF_GRAPPLE_PULL
    sint    weaponAnim;         // mask off ANIM_TOGGLEBIT
    sint    clips;              // clips held
    sint    tauntTimer;         // don't allow another taunt until this runs out
    sint    misc[MAX_MISC];     // misc data
    sint    jumppad_frame;
    sint    jumppad_ent;    // jumppad entity hit this frame
} playerState_t;


//====================================================================


//
// usercmd_t->button bits, many of which are generated by the client system,
// so they aren't game/cgame only definitions
//
#define BUTTON_ATTACK       1
#define BUTTON_TALK         2           // displays talk balloon and disables actions
#define BUTTON_USE_HOLDABLE 4           // activate upgrade
#define BUTTON_GESTURE      8
#define BUTTON_WALKING      16          // walking can't just be infered from MOVE_RUN
// because a key pressed late in the frame will
// only generate a small move value for that frame
// walking will use different animations and
// won't generate footsteps
#define BUTTON_ATTACK2  32
#define BUTTON_DODGE        64          // start a dodge or sprint motion
#define BUTTON_USE_EVOLVE   128         // use target or open evolve menu
#define BUTTON_SPRINT   256

#define BUTTON_ANY          2048            // any key whatsoever

#define MOVE_RUN            120         // if forwardmove or rightmove are >= MOVE_RUN,
// then BUTTON_WALKING should be set

// Arnout: doubleTap buttons - DT_NUM can be max 8
enum dtType_t {
    DT_NONE,
    DT_MOVELEFT,
    DT_MOVERIGHT,
    DT_FORWARD,
    DT_BACK,
    DT_LEANLEFT,
    DT_LEANRIGHT,
    DT_UP,
    DT_NUM
};

// usercmd_t is sent to the server each client frame
typedef struct usercmd_s {
    sint serverTime;
    uchar8 buttons;
    uchar8 wbuttons;
    uchar8 weapon;
    uchar8 flags;
    sint angles[3];

    schar8 forwardmove, rightmove, upmove;
    uchar8 doubleTap;             // Arnout: only 3 bits used

    // rain - in ET, this can be any entity, and it's used as an array
    // index, so make sure it's unsigned
    uchar8 identClient;           // NERVE - SMF
} usercmd_t;

//===================================================================

// if entityState->solid == SOLID_BMODEL, modelindex is an inline model number
#define SOLID_BMODEL    0xffffff

enum trType_t {
    TR_STATIONARY,
    TR_INTERPOLATE,             // non-parametric, but interpolate between snapshots
    TR_LINEAR,
    TR_LINEAR_STOP,
    TR_NONLINEAR_STOP,
    TR_SINE,                    // value = base + sin( time / duration ) * delta
    TR_GRAVITY,
    TR_BUOYANCY
};

typedef struct {
    trType_t trType;
    sint trTime;
    sint trDuration;             // if non 0, trTime + trDuration = stop time
    //----(SA)  removed
    vec3_t trBase;
    vec3_t trDelta;             // velocity, etc
    //----(SA)  removed
} trajectory_t;

// entityState_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
// Different eTypes may use the information in different ways
// The messages are delta compressed, so it doesn't really matter if
// the structure size is fairly large
//
// NOTE: all fields in here must be 32 bits (or those within sub-structures)


//
// entityState_t->eType
//
enum entityType_t {
    ET_GENERAL,
    ET_PLAYER,
    ET_ITEM,

    ET_BUILDABLE,       // buildable type

    ET_LOCATION,

    ET_MISSILE,
    ET_MOVER,
    ET_BEAM,
    ET_PORTAL,
    ET_SPEAKER,
    ET_PUSH_TRIGGER,
    ET_TELEPORT_TRIGGER,
    ET_INVISIBLE,
    ET_GRAPPLE,       // grapple hooked on wall

    ET_CORPSE,
    ET_PARTICLE_SYSTEM,
    ET_ANIMMAPOBJ,
    ET_MODELDOOR,
    ET_LIGHTFLARE,
    ET_LEV2_ZAP_CHAIN,

    ET_EVENTS       // any of the EV_* events can be added freestanding
    // by setting eType to ET_EVENTS + eventNum
    // this avoids having to set eFlags and eventNum
};

typedef struct entityState_s {
    sint        number;         // entity index
    entityType_t eType;     // entityType_t
    sint        eFlags;

    trajectory_t pos;       // for calculating position
    trajectory_t apos;      // for calculating angles

    sint time;
    sint time2;

    vec3_t origin;
    vec3_t origin2;

    vec3_t angles;
    vec3_t angles2;

    sint otherEntityNum;     // shotgun sources, etc
    sint otherEntityNum2;

    sint groundEntityNum;        // -1 = in air

    sint constantLight;      // r + (g<<8) + (b<<16) + (intensity<<24)
    sint dl_intensity;       // used for coronas
    sint loopSound;          // constantly loop this sound

    sint modelindex;
    sint modelindex2;
    sint clientNum;          // 0 to (MAX_CLIENTS - 1), for players and corpses
    sint frame;

    sint solid;              // for client side prediction, trap_linkentity sets this properly

    // old style events, in for compatibility only
    sint _event;                // impulse events -- muzzle flashes, footsteps, etc
    sint eventParm;

    sint eventSequence;      // pmove generated events
    sint events[MAX_EVENTS];
    sint eventParms[MAX_EVENTS];

    // for players
    sint powerups;           // bit flags   // Arnout: used to store entState_t for non-player entities (so we know to draw them translucent clientsided)
    sint weapon;             // determines weapon and flash model, etc
    sint legsAnim;           // mask off ANIM_TOGGLEBIT
    sint torsoAnim;          // mask off ANIM_TOGGLEBIT
    //  sint        weapAnim;       // mask off ANIM_TOGGLEBIT  //----(SA)  removed (weap anims will be client-side only)

    sint density;            // for particle effects

    sint dmgFlags;           // to pass along additional information for damage effects for players/ Also used for cursorhints for non-player entities

    // Ridah
    sint onFireStart, onFireEnd;

    sint nextWeapon;
    sint teamNum;

    sint effect1Time, effect2Time, effect3Time;

    aistateEnum_t aiState;      // xkan, 1/10/2003
    sint animMovetype;       // clients can't derive movetype of other clients for anim scripting system

    // Dushan - Tremulous
    sint    misc;           // bit flags
    sint    generic1;
    sint    weaponAnim;     // mask off ANIM_TOGGLEBIT
} entityState_t;

enum connstate_t {
    CA_UNINITIALIZED,
    CA_DISCONNECTED,    // not talking to a server
    CA_AUTHORIZING,     // not used any more, was checking cd key
    CA_CONNECTING,      // sending request packets to the server
    CA_CHALLENGING,     // sending challenge packets to the server
    CA_CONNECTED,       // netchan_t established, getting gamestate
    CA_LOADING,         // only during cgame initialization, never during main loop
    CA_PRIMED,          // got gamestate, waiting for first frame
    CA_ACTIVE,          // game views should be displayed
    CA_CINEMATIC        // playing a cinematic or a static pic, not connected to a server
};

typedef struct lineInfo_t {
    pointer text;   // text
    sint            count;  // number of characters
    float32     sa;     // offset per white space
    float32     ox;     // ofset from left bounds
    float32     width;  // width of line
    float32     height; // height of line

    float32     startColor[4];
    float32     endColor[4];
    float32     defaultColor[4];
} lineInfo_t;

enum textAlign_e {
    TEXT_ALIGN_LEFT = 0,
    TEXT_ALIGN_CENTER = 1,
    TEXT_ALIGN_RIGHT = 2,
    TEXT_ALIGN_JUSTIFY = 3,

    TEXT_ALIGN_NOCLIP = 0x0080,
};

enum textStyle_e {
    TEXT_STYLE_SHADOWED = 2,
    TEXT_STYLE_OUTLINED = 4,
    TEXT_STYLE_BLINK = 8,
    TEXT_STYLE_ITALIC = 16,
};

#define Square( x ) ( ( x ) * ( x ) )

// real time
//=============================================


typedef struct qtime_s {
    sint tm_sec;     /* seconds after the minute - [0,59] */
    sint tm_min;     /* minutes after the hour - [0,59] */
    sint tm_hour;    /* hours since midnight - [0,23] */
    sint tm_mday;    /* day of the month - [1,31] */
    sint tm_mon;     /* months since January - [0,11] */
    sint tm_year;    /* years since 1900 */
    sint tm_wday;    /* days since Sunday - [0,6] */
    sint tm_yday;    /* days since January 1 - [0,365] */
    sint tm_isdst;   /* daylight savings time flag */
} qtime_t;


// server browser sources
#define AS_LOCAL        0
#define AS_GLOBAL       1           // NERVE - SMF - modified
#define AS_FAVORITES    2


// cinematic states
enum e_status {
    FMV_IDLE,
    FMV_PLAY,       // play
    FMV_EOF,        // all other conditions, i.e. stop/EOF/abort
    FMV_ID_BLT,
    FMV_ID_IDLE,
    FMV_LOOPED,
    FMV_ID_WAIT
};

enum flagStatus_t {
    FLAG_ATBASE = 0,
    FLAG_TAKEN,         // CTF
    FLAG_TAKEN_RED,     // One Flag CTF
    FLAG_TAKEN_BLUE,    // One Flag CTF
    FLAG_DROPPED
};

#define MAX_GLOBAL_SERVERS          4096
#define MAX_OTHER_SERVERS           128
#define MAX_PINGREQUESTS            16
#define MAX_SERVERSTATUSREQUESTS    16

// NERVE - SMF - localization
enum languages_t {
    LANGUAGE_FRENCH = 0,
    LANGUAGE_GERMAN,
    LANGUAGE_ITALIAN,
    LANGUAGE_SPANISH,
    MAX_LANGUAGES
};

// NERVE - SMF - wolf server/game states
enum gamestate_t {
    GS_INITIALIZE = -1,
    GS_PLAYING,
    GS_WARMUP_COUNTDOWN,
    GS_WARMUP,
    GS_INTERMISSION,
    GS_WAITING_FOR_PLAYERS,
    GS_RESET
};

// Dushan - Tremulous
#define GENTITYNUM_MASK     (MAX_GENTITIES - 1)

#define MAX_EMOTICON_NAME_LEN       16
#define MAX_EMOTICONS               64

#define MAX_LOCATIONS               64
#define MAX_MODELS                  256     // these are sent over the net as 8 bits
#define MAX_SOUNDS                  256     // so they cannot be blindly increased
#define MAX_GAME_SHADERS            64
#define MAX_GAME_PARTICLE_SYSTEMS   64
#define MAX_HOSTNAME_LENGTH         80      // max length of a host name
#define MAX_NEWS_STRING             10000

typedef struct {
    valueType      name[MAX_EMOTICON_NAME_LEN];
#ifndef GAMEDLL
    sint       width;
    qhandle_t shader;
#endif
} emoticon_t;

typedef struct {
    uint hi;
    uint lo;
} clientList_t;

bool Com_ClientListContains(const clientList_t *list, sint clientNum);
valueType *Com_ClientListString(const clientList_t *list);
void Com_ClientListParse(clientList_t *list, pointer s);

#define SQR( a ) ( ( a ) * ( a ) )

#ifndef BSPC
enum cullType_t {
    CT_FRONT_SIDED,
    CT_BACK_SIDED,
    CT_TWO_SIDED
};
#endif

#define LERP( a, b, w ) ( ( a ) * ( 1.0f - ( w ) ) + ( b ) * ( w ) )
#define LUMA( red, green, blue ) ( 0.2126f * ( red ) + 0.7152f * ( green ) + 0.0722f * ( blue ) )

#define SAY_ALL     0
#define SAY_TEAM    1
#define SAY_TELL    2
#define SAY_ACTION      3
#define SAY_ACTION_T    4
#define SAY_ADMINS    5

void Com_MatchToken(pointer(*buf_p), pointer match, bool warning = false);
pointer Com_Parse(pointer(*data_p));
void Com_UngetToken(void);
pointer Com_ParseOnLine(pointer(*data_p));
pointer Com_ParseRestOfLine(pointer(*data_p));
float32 Com_ParseFloat(pointer(*buf_p));
void Com_Parse1DMatrix(pointer(*buf_p), sint x, float32 *m);
void Com_Parse2DMatrix(pointer(*buf_p), sint y, sint x, float32 *m);
void Com_Parse3DMatrix(pointer(*buf_p), sint z, sint y, sint x,
                       float32 *m);

// handy stuff when tracking isnan problems
#ifndef NDEBUG
#define CHECK_NAN( x ) assert( !IS_NAN( x ) )
#define CHECK_NAN_VEC( v ) assert( !IS_NAN( v[0] ) && !IS_NAN( v[1] ) && !IS_NAN( v[2] ) )
#else
#define CHECK_NAN
#define CHECK_NAN_VEC
#endif

void Com_BeginParseSession(pointer filename);
void Com_EndParseSession(void);

bool StringContainsWord(pointer haystack, pointer needle);
bool COM_CompareExtension(pointer in, pointer ext);
#define CL_TARGET_OPENCL_VERSION 120
#define VALIDSTRING( a ) ( ( a != nullptr ) && ( a[0] != '\0' ) )
float32 Q_flrand(float32 min, float32 max);
bool Q_CleanPlayerName(pointer in, valueType *out, sint outSize);
sint COM_CompressBracedSection(valueType **data_p, valueType **name,
                               valueType **text, sint *nameLength, sint *textLength);

#define KEYBOARDCTRL(a) ((a)-'a'+1)

#endif //!__Q_SHARED_H__
