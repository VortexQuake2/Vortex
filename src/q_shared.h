// q_shared.h -- included first by ALL program modules

#ifndef Q_SHARED
#define Q_SHARED

#define __STDC_LIB_EXT1__ 1
#include <stdint.h>


#ifdef _MSC_VER
// unknown pragmas are SUPPOSED to be ignored, but....
#pragma warning(disable : 4018)     // signed/unsigned mismatch
#pragma warning(disable : 4305)		// truncation from const double to float
#pragma warning(disable : 4996)		// deprecation
#endif

//K03 Begin
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
//K03 End

typedef unsigned char byte;

typedef int32_t qboolean;
#define DEG2RAD( a ) ( a * M_PI ) / 180.0F
//K03 End


// for things we need abi compatibility for with the old version of the api
#ifndef VRX_REPRO
typedef qboolean _rebool;
#else
typedef bool _rebool;
#endif

// angle indexes
#define	PITCH				0		// up / down
#define	YAW					1		// left / right
#define	ROLL				2		// fall over

#define	MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString
#define	MAX_STRING_TOKENS	80		// max tokens resulting from Cmd_TokenizeString
#define	MAX_TOKEN_CHARS		512		// max length of an individual token

#define	MAX_QPATH			64		// max length of a quake game pathname
#define	MAX_OSPATH			128		// max length of a filesystem pathname
#define MAX_SPLIT_PLAYERS   4

//
// per-level limits
//
#define	MAX_CLIENTS			256		// absolute limit
#ifndef VRX_REPRO
#define	MAX_EDICTS			1024	// must change protocol to increase more
#define	MAX_MODELS			256		// these are sent over the net as bytes
#define	MAX_SOUNDS			256		// so they cannot be blindly increased
#define	MAX_IMAGES			256
#else
#define MAX_EDICTS			8192
#define	MAX_MODELS			8192
#define	MAX_SOUNDS			2048		// so they cannot be blindly increased
#define	MAX_IMAGES			512

#endif
#define	MAX_LIGHTSTYLES		256
#define	MAX_ITEMS			256
#define MAX_GENERAL			(MAX_CLIENTS*2)	// general config strings

#define MAX_SHADOW_LIGHTS 256

#define U32BIT(x) ((uint32_t)1 << (uint32_t)x)
#define U64BIT(x) ((uint64_t)1 << (uint64_t)x)

// game print flags
enum print_type_t {
    PRINT_LOW = 0, // pickup messages
    PRINT_MEDIUM = 1, // death messages
    PRINT_HIGH = 2, // critical messages
    PRINT_CHAT = 3, // chat messages
#ifdef VRX_REPRO
    PRINT_TYPEWRITER = 4, // centerpritnt but one char at a time
    PRINT_CENTER = 5, // centerprint without a separate function
    PRINT_TTS = 6, // PRINT_HIGH but will speak for players with narration on
    PRINT_BROADCAST = 1 << 3, // bitflag, add message to broadcast print to all clients
    PRINT_NO_NOTIFY = 1 << 4 // bitflag, don't put on notify
#endif
};


#define	ERR_FATAL			0		// exit the entire game with a popup window
#define	ERR_DROP			1		// print to console and disconnect from game
#define	ERR_DISCONNECT		2		// don't kill server

#define	PRINT_ALL			0
#define PRINT_DEVELOPER		1		// only print when "developer 1"
#define PRINT_ALERT			2

// [Paril-KEX] max number of arguments (not including the base) for
// localization prints
constexpr size_t MAX_LOCALIZATION_ARGS = 8;

// remaps old configstring IDs to new ones
// for old DLL & demo support
struct configstring_remap_t {
    // start position in the configstring list
    // to write into
    size_t start;
    // max length to write into; [start+length-1] should always
    // be set to '\0'
    size_t length;
};

// destination class for gi.multicast()
typedef enum {
    MULTICAST_ALL,
    MULTICAST_PHS,
    MULTICAST_PVS,
    MULTICAST_ALL_R,
    MULTICAST_PHS_R,
    MULTICAST_PVS_R
} multicast_t;

// player_state_t->refdef flags
enum refdef_flags_t : uint8_t {
    RDF_NONE = 0,
    RDF_UNDERWATER = 1, // warp the screen as apropriate
    RDF_NOWORLDMODEL = 2, // used for player configuration screen
    //ROGUE
    RDF_IRGOGGLES = 4,
    RDF_UVGOGGLES = 8,
    //ROGUE
    RDF_NO_WEAPON_LERP = 1 << 4
};

/*
==============================================================

MATHLIB

==============================================================
*/

#ifndef min
#define min(a,b) ((a) > (b) ? (b) : (a))
#endif
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

typedef float vec_t;
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef vec_t vec5_t[5];

typedef union {
    uint32_t u32;
    uint8_t u8[4];

    struct {
        uint8_t r, g, b, a;
    };
} color_t;

typedef union {
    struct {
        vec_t x, y;
    };

    vec_t v[2];
} vec2_t;

typedef color_t rgba_t;
constexpr rgba_t rgba_white = {.r = 255, .g = 255, .b = 255, .a = 255};
constexpr rgba_t rgba_black = {.r = 0, .g = 0, .b = 0, .a = 255};
constexpr rgba_t rgba_red = {.r = 255, .g = 0, .b = 0, .a = 255};
constexpr rgba_t rgba_green = {.r = 0, .g = 255, .b = 0, .a = 255};
constexpr rgba_t rgba_blue = {.r = 0, .g = 0, .b = 255, .a = 255};

struct cplane_s;

extern vec3_t vec3_origin;

#define	nanmask (255<<23)

#define	IS_NAN(x) (((*(int *)&x)&nanmask)==nanmask)

#if !defined C_ONLY
extern long Q_ftol(float f);
#else
#define Q_ftol( f ) ( long ) (f)
#endif

#if (defined _WIN32) && (defined ASMOPT)
double __fastcall sqrt14(double n);

#undef sqrt
#define sqrt(x) sqrt14(x)
#endif

#define DotProduct(x,y)			(x[0]*y[0]+x[1]*y[1]+x[2]*y[2])
#define VectorSubtract(a,b,c)	(c[0]=a[0]-b[0],c[1]=a[1]-b[1],c[2]=a[2]-b[2])
#define VectorAdd(a,b,c)		(c[0]=a[0]+b[0],c[1]=a[1]+b[1],c[2]=a[2]+b[2])
#define VectorCopy(a,b)			(b[0]=a[0],b[1]=a[1],b[2]=a[2])
#define VectorClear(a)			(a[0]=a[1]=a[2]=0)
#define VectorNegate(a,b)		(b[0]=-a[0],b[1]=-a[1],b[2]=-a[2])
#define VectorSet(v, x, y, z)	(v[0]=(x), v[1]=(y), v[2]=(z))

void VectorMA(vec3_t veca, float scale, vec3_t vecb, vec3_t vecc);

// just in case you do't want to use the macros
vec_t _DotProduct(vec3_t v1, vec3_t v2);

void _VectorSubtract(vec3_t veca, vec3_t vecb, vec3_t out);

void _VectorAdd(vec3_t veca, vec3_t vecb, vec3_t out);

void _VectorCopy(vec3_t in, vec3_t out);

void ClearBounds(vec3_t mins, vec3_t maxs);

void AddPointToBounds(vec3_t v, vec3_t mins, vec3_t maxs);

int VectorCompare(vec3_t v1, vec3_t v2);

vec_t VectorLength(vec3_t v);

#if (defined WIN32) && (defined ASMOPT)

__inline vec_t VectorLength(vec3_t v) {
    float len;
    __asm {
            fldz
            push eax

            mov eax,dword ptr[v]
            fld dword ptr[eax]
            fmul dword ptr[eax]

            fld dword ptr[eax+4]
            fmul dword ptr[eax+4]
            faddp st(1),st(0)
            fstp st(1)

            fld dword ptr[eax+8]
            fmul dword ptr[eax+8]
            faddp st(1),st(0)
            fstp st(1)
            fsqrt
            pop eax
            fstp dword ptr[len]
            }
    return len;
}
#else
vec_t VectorLength(vec3_t v);
#endif

vec_t VectorLengthSqr(vec3_t v);

void CrossProduct(vec3_t v1, vec3_t v2, vec3_t cross);

vec_t VectorNormalize(vec3_t v); // returns vector length
vec_t VectorNormalize2(vec3_t v, vec3_t out);

void VectorInverse(vec3_t v);

void VectorScale(vec3_t in, vec_t scale, vec3_t out);

int Q_log2(int val);

void R_ConcatRotations(float in1[3][3], float in2[3][3], float out[3][3]);

void R_ConcatTransforms(float in1[3][4], float in2[3][4], float out[3][4]);

void AngleVectors(vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);

int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s *plane);

float anglemod(float a);

float LerpAngle(float a1, float a2, float frac);

#define BOX_ON_PLANE_SIDE(emins, emaxs, p)	\
	(((p)->type < 3)?						\
	(										\
		((p)->dist <= (emins)[(p)->type])?	\
			1								\
		:									\
		(									\
			((p)->dist >= (emaxs)[(p)->type])?\
				2							\
			:								\
				3							\
		)									\
	)										\
	:										\
		BoxOnPlaneSide( (emins), (emaxs), (p)))

void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal);

void PerpendicularVector(vec3_t dst, const vec3_t src);

void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees);


//=============================================

char *COM_SkipPath(char *pathname);

void COM_StripExtension(char *in, char *out);

void COM_FileBase(char *in, char *out);

void COM_FilePath(char *in, char *out);

void COM_DefaultExtension(char *path, char *extension);

char *COM_Parse(const char **data_p);

// data is an in/out parm, returns a parsed out token

void Com_sprintf(char *dest, int size, char *fmt, ...);

void Com_PageInMemory(const byte *buffer, int size);

//=============================================

// portable case insensitive compare
int Q_stricmp(char *s1, char *s2);

int Q_strcasecmp(const char *s1, const char *s2);

int Q_strncasecmp(const char *s1, const char *s2, int n);

size_t Q_strlcpy(char *dst, const char *src, size_t siz);

//=============================================

short BigShort(short l);

short LittleShort(short l);

int BigLong(int l);

int LittleLong(int l);

float BigFloat(float l);

float LittleFloat(float l);

void Swap_Init(void);

char *va(const char *format, ...);

//=============================================

//
// key / value info strings
//
#define	MAX_INFO_KEY		64

#ifndef VRX_REPRO
#define	MAX_INFO_VALUE		64
#define	MAX_INFO_STRING		512
#else
#define	MAX_INFO_VALUE		256
#define	MAX_INFO_STRING		2048
#endif

#define MAX_WHEEL_ITEMS 32

char *Info_ValueForKey(char *s, char *key);

void Info_RemoveKey(char *s, const char *key);

void Info_SetValueForKey(char *s, char *key, char *value);

qboolean Info_Validate(char *s);

/*
==============================================================

SYSTEM SPECIFIC

==============================================================
*/

extern int curtime; // time returned by last Sys_Milliseconds

int Sys_Milliseconds(void);

void Sys_Mkdir(char *path);

// large block stack allocation routines
void *Hunk_Begin(int maxsize);

void *Hunk_Alloc(int size);

void Hunk_Free(void *buf);

int Hunk_End(void);

// directory searching
#define SFF_ARCH    0x01
#define SFF_HIDDEN  0x02
#define SFF_RDONLY  0x04
#define SFF_SUBDIR  0x08
#define SFF_SYSTEM  0x10

/*
** pass in an attribute mask of things you wish to REJECT
*/
char *Sys_FindFirst(char *path, unsigned musthave, unsigned canthave);

char *Sys_FindNext(unsigned musthave, unsigned canthave);

void Sys_FindClose(void);


void Com_Printf(const char *msg, ...);


/*
==========================================================

CVARS (console variables)

==========================================================
*/

#ifndef CVAR
#define	CVAR

enum cvar_flags_t {
    CVAR_NOFLAGS = 0,
    CVAR_ARCHIVE = 1, // set to cause it to be saved to vars.rc
    CVAR_USERINFO = 2, // added to userinfo  when changed
    CVAR_SERVERINFO = 4, // added to serverinfo when changed
    CVAR_NOSET = 8, // don't allow change from console at all,
    // but can be set from the command line
    CVAR_LATCH = 16, // save changes until server restart
#ifdef VRX_REPRO
    CVAR_USER_PROFILE = 32 // like cvar_userinfo but not sent to server
#endif
};

struct edict_t;

// nothing outside the Cvar_*() functions should modify these fields!
typedef struct cvar_s {
    char *name;
    char *string;
    char *latched_string; // for CVAR_LATCH vars
    int flags;
#if VRX_REPRO
    int32_t modified_count; // set each time the cvar is changed but never zero
#else
    qboolean modified; // set each time the cvar is changed
#endif
    float value;
    struct cvar_s *next;
#if VRX_REPRO
    int32_t integer; // integral value
#endif
} cvar_t;

#endif		// CVAR

/*
==============================================================

COLLISION DETECTION

==============================================================
*/

// lower bits are stronger, and will eat weaker brushes completely
enum contents_t : uint32_t {
    CONTENTS_NONE = 0,
    CONTENTS_SOLID = 1, // an eye is never valid in a solid
    CONTENTS_WINDOW = 2, // translucent, but not watery
    CONTENTS_AUX = 4,
    CONTENTS_LAVA = 8,
    CONTENTS_SLIME = 16,
    CONTENTS_WATER = 32,
    CONTENTS_MIST = 64,

#ifdef VRX_REPRO
    CONTENTS_NO_WATERJUMP = U64BIT(13),
    CONTENTS_PROJECTILECLIP = U64BIT(14),
    CONTENTS_AREAPROTAL = U64BIT(15),
    CONTENTS_PLAYERCLIP = U64BIT(16),
    CONTENTS_MONSTERCLIP = U64BIT(17),

    CONTENTS_CURRENT_0 = U64BIT(18),
    CONTENTS_CURRENT_90 = U64BIT(19),
    CONTENTS_CURRENT_180 = U64BIT(20),
    CONTENTS_CURRENT_270 = U64BIT(21),
    CONTENT_CURRENTS_UP = U64BIT(22),
    CONTENT_CURRENTS_DOWN = U64BIT(23),

    CONTENTS_ORIGIN = U64BIT(24), // removed before bsping an entity

    CONTENTS_MONSTER = U64BIT(25),
    CONTENTS_DEADMONSTER = U64BIT(26),
    CONTENTS_DETAIL = U64BIT(27),
    CONTENTS_TRANSLUCENT = U64BIT(28),
    CONTENTS_LADDER = U64BIT(29),
    CONTENTS_PLAYER = U64BIT(30),
    CONTENTS_PROJECTILE = U64BIT(31)
#endif
};

enum surfflags_t : uint32_t {
    SURF_LIGHT = 1 << 0, // value will hold the light strength
    SURF_SLICK = 1 << 1, // effects game physics
    SURF_SKY = 1 << 2, // don't draw, but add to skybox
    SURF_WARP = 1 << 3, // turbulent water warp
    SURF_TRANS33 = 1 << 4,
    SURF_TRANS66 = 1 << 5,
    SURF_FLOWING = 1 << 6, // scroll towards angle
    SURF_NODRAW = 1 << 7, // don't bother referencing the texture

#ifdef VRX_REPRO
    SURF_ALPHATEST = 1 << 25, // [Paril-KEX] alpha test using widely supported flag
    SURF_N64_UV = U32BIT(28), // [Sam-KEX] Stretches texture UVs
    SURF_N64_SCROLL_X = U32BIT(29), // [Sam-KEX] Texture scroll X-axis
    SURF_N64_SCROLL_Y = U32BIT(30), // [Sam-KEX] Texture scroll Y-axis
    SURF_N64_SCROLL_FLIP = U32BIT(31) // [Sam-KEX] Flip direction of texture scroll
#endif
};


// content masks
#define	MASK_ALL				(-1)
#define	MASK_SOLID				(CONTENTS_SOLID|CONTENTS_WINDOW)
#define	MASK_PLAYERSOLID		(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define	MASK_DEADSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW)
#define	MASK_MONSTERSOLID		(CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define	MASK_WATER				(CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define	MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define	MASK_SHOT				(CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEADMONSTER)
#define MASK_CURRENT			(CONTENTS_CURRENT_0|CONTENTS_CURRENT_90|CONTENTS_CURRENT_180|CONTENTS_CURRENT_270|CONTENTS_CURRENT_UP|CONTENTS_CURRENT_DOWN)
#define	MASK_BOTSOLID			(CONTENTS_SOLID|CONTENTS_LADDER/*CONTENTS_PLAYERCLIP*/|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define	MASK_BOTSOLIDX			(CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_PLAYERCLIP|CONTENTS_MONSTER|CONTENTS_MONSTERCLIP)
#define	MASK_GROUND				(CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_MONSTER)

// gi.BoxEdicts() can return a list of either solid or trigger entities
// FIXME: eliminate AREA_ distinction?
enum solidity_area_t {
    AREA_SOLID = 1,
    AREA_TRIGGERS = 2
};

// plane_t structure
// !!! if this is changed, it must be changed in asm code too !!!
typedef struct cplane_s {
    vec3_t normal;
    float dist;
    byte type; // for fast side tests
    byte signbits; // signx + (signy<<1) + (signz<<1)
    byte pad[2];
} cplane_t;

// structure offset for asm code
#define CPLANE_NORMAL_X			0
#define CPLANE_NORMAL_Y			4
#define CPLANE_NORMAL_Z			8
#define CPLANE_DIST				12
#define CPLANE_TYPE				16
#define CPLANE_SIGNBITS			17
#define CPLANE_PAD0				18
#define CPLANE_PAD1				19

typedef struct cmodel_s {
    vec3_t mins, maxs;
    vec3_t origin; // for sounds or lights
    int headnode;
} cmodel_t;

// [Paril-KEX]
#define MAX_MATERIAL_NAME 16

typedef struct csurface_s {
#ifndef VRX_REPRO
    char name[16];
#else
    char name[32];
#endif
    int32_t flags; // surfflags_t
    int32_t value;

#ifdef VRX_REPRO
    // [Paril-KEX]
    uint32_t id; // unique texinfo ID, offset by 1 (0 is 'null')
    char material[MAX_MATERIAL_NAME];
#endif
} csurface_t;

// a trace is returned when a box is swept through the world
typedef struct {
#ifndef VRX_REPRO
    qboolean allsolid; // if true, plane is not valid
    qboolean startsolid; // if true, the initial point was in a solid area
    float fraction; // time completed, 1.0 = didn't hit anything
    vec3_t endpos; // final position
    cplane_t plane; // surface normal at impact
    csurface_t *surface; // surface hit
    int contents; // contents on other side of surface hit
    struct edict_s *ent; // not set by CM_*() functions
#else
    byte allsolid; // if true, plane is not valid
    byte startsolid; // if true, the initial point was in a solid area
    float fraction; // time completed, 1.0 = didn't hit anything
    vec3_t endpos; // final position
    cplane_t plane; // surface normal at impact
    csurface_t *surface; // surface hit
    enum contents_t contents; // contents on other side of surface hit (contents_t)
    struct edict_s *ent; // not set by CM_*() functions

    // [Paril-KEX] the second-best surface hit from a trace
    cplane_t plane2; // second surface normal at impact
    csurface_t *surface2; // second surface hit
#endif
} trace_t;


// pmove_state_t is the information necessary for client side movement
// prediction
typedef enum {
    // can accelerate and turn
    PM_NORMAL,
#ifdef VRX_REPRO
    PM_GRAPPLE,
    PM_NOCLIP,
#endif
    PM_SPECTATOR,
    // no acceleration or turning
    PM_DEAD,
    PM_GIB, // different bounding box
    PM_FREEZE
} pmtype_t;

// pmove->pm_flags
enum pmflags_t : uint16_t {
    PMF_NONE = 0,
#ifndef VRX_REPRO
    PMF_DUCKED = 1,
            PMF_JUMP_HELD = 2,
            PMF_ON_GROUND = 4,
            PMF_TIME_WATERJUMP = 8, // pm_time is waterjump
            PMF_TIME_LAND = 16, // pm_time is time before rejump
            PMF_TIME_TELEPORT = 32, // pm_time is non-moving time
            PMF_NO_PREDICTION = 64,  // temporarily disables prediction (used for grappling hook)
#else
    PMF_DUCKED = 1 << 0,
    PMF_JUMP_HELD = 1 << 1,
    PMF_ON_GROUND = 1 << 2,
    PMF_TIME_WATERJUMP = 1 << 3, // pm_time is waterjump
    PMF_TIME_LAND = 1 << 4, // pm_time is time before rejump
    PMF_TIME_TELEPORT = 1 << 5, // pm_time is non-moving time
    PMF_NO_POSITIONAL_PREDICTION = 1 << 6, // temporarily disables positional prediction (used for grappling hook)
    PMF_ON_LADDER = 1 << 7, // signal to game that we are on a ladder
    PMF_NO_ANGULAR_PREDICTION = 1 << 8, // temporary disables angular prediction
    PMF_IGNORE_PLAYER_COLLISION = 1 << 9, // don't collide with other players
    PMF_TIME_TRICK = 1 << 10, // pm_time is trick jump time
    PMF_NO_PREDICTION = PMF_NO_ANGULAR_PREDICTION | PMF_NO_POSITIONAL_PREDICTION
#endif
};

// this structure needs to be communicated bit-accurate
// from the server to the client to guarantee that
// prediction stays in sync, so no floats are used.
// if any part of the game code modifies this struct, it
// will result in a prediction error of some degree.

#ifndef VRX_REPRO
typedef struct {
    pmtype_t pm_type;

    short origin[3]; // 12.3
    short velocity[3]; // 12.3
    byte pm_flags; // ducked, jump_held, etc
    byte pm_time; // each unit = 8 ms
    short gravity;
    short delta_angles[3]; // add to command angles to get view direction
    // changed by spawns, rotating objects, and teleporters
} pmove_state_t;
#else
typedef struct pmove_state_s {
    pmtype_t pm_type;

    vec3_t origin;
    vec3_t velocity;
    enum pmflags_t pm_flags;

    uint16_t pm_time;
    int16_t gravity;
    vec3_t delta_angles;

    int8_t viewheight;
} pmove_state_t;
#endif

//
// button bits
//
enum button_t : uint8_t {
    BUTTON_NONE = 0,
    BUTTON_ATTACK = 1 << 0,
    BUTTON_USE = 1 << 1,
    BUTTON_HOLSTER = 1 << 2, // [Paril-KEX]
    BUTTON_JUMP = 1 << 3,
    BUTTON_CROUCH = 1 << 4,
    BUTTON_ANY = 1 << 7 // any key whatsoever
};


// usercmd_t is sent to the server each client frame
typedef struct usercmd_s {
    byte msec;
    enum button_t buttons;
#ifndef VRX_REPRO
    short angles[3];
    short forwardmove, sidemove, upmove;
    byte impulse; // remove?
    byte lightlevel; // light level the player is standing on
#else
    vec3_t angles;
    float forwardmove, sidemove, upmove;
    uint32_t server_frame;
#endif
} usercmd_t;


#define	MAXTOUCH	32

// [Paril-KEX] generic touch list; used for contact entities
typedef struct touch_list_s
{
    size_t	num;
    trace_t traces[MAXTOUCH];
} touch_list_t;

typedef struct {
    // state (in / out)
    pmove_state_t s;

    // command (in)
    usercmd_t cmd;
    _rebool snapinitial; // if s has been changed outside pmove

    // results (out)
#ifndef VRX_REPRO
    int numtouch;
    struct edict_s *touchents[MAXTOUCH];
#else
    struct touch_list_s touch;
#endif
    vec3_t viewangles; // clamped
#ifndef VRX_REPRO
    float viewheight;
#endif

    vec3_t mins, maxs; // bounding box size

    struct edict_s *groundentity;
#ifdef VRX_REPRO
    cplane_t groundplane;
#endif

    enum contents_t watertype;
    int waterlevel;

#ifndef VRX_REPRO
    // callbacks to test the world
    trace_t (*trace)(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);
    int (*pointcontents)(vec3_t point);
#else
    struct edict_s *player; // edict_t

    // clip against world & entities
    trace_t (*trace)(
        vec3_t start,
        const vec3_t *mins,
        const vec3_t *maxs,
        vec3_t end,
        const struct edict_s *passent,
        enum contents_t contentmask);

    // [Paril-KEX] clip against world only
    trace_t (*clip)(
        vec3_t start,
        vec3_t *mins,
        vec3_t *maxs,
        vec3_t end,
        enum contents_t contentmask);

    int (*pointcontents)(vec3_t point);
    vec3_t viewoffset;

    vec4_t screen_blend;
    enum refdef_flags_t rdflags;
    _rebool jump_sound;
    _rebool step_clip;
    float impact_delta;
#endif
} pmove_t;


// entity_state_t->effects
// Effects are things handled on the client side (lights, particles, frame animations)
// that happen constantly on the given entity.
// An entity that has effects will be sent to the client
// even if it has a zero index model.
enum effects_t :
#ifndef VRX_REPRO
        int
#else
        uint64_t
#endif
{
    EF_ROTATE = 0x00000001, // rotate (bonus items)
    EF_GIB = 0x00000002, // leave a trail
#ifdef VRX_REPRO
    EF_BOB = 1 << 2, // bob
#endif
    EF_BLASTER = 0x00000008, // redlight + trail
    EF_ROCKET = 0x00000010, // redlight + trail
    EF_GRENADE = 0x00000020,
    EF_HYPERBLASTER = 0x00000040,
    EF_BFG = 0x00000080,
    EF_COLOR_SHELL = 0x00000100,
    EF_POWERSCREEN = 0x00000200,
    EF_ANIM01 = 0x00000400, // automatically cycle between frames 0 and 1 at 2 hz
    EF_ANIM23 = 0x00000800, // automatically cycle between frames 2 and 3 at 2 hz
    EF_ANIM_ALL = 0x00001000, // automatically cycle through all frames at 2hz
    EF_ANIM_ALLFAST = 0x00002000, // automatically cycle through all frames at 10hz
    EF_FLIES = 0x00004000,
    EF_QUAD = 0x00008000,
    EF_PENT = 0x00010000,
    EF_TELEPORTER = 0x00020000, // particle fountain
    EF_FLAG1 = 0x00040000,
    EF_FLAG2 = 0x00080000,
    // RAFAEL
    EF_IONRIPPER = 0x00100000,
    EF_GREENGIB = 0x00200000,
    EF_BLUEHYPERBLASTER = 0x00400000,
    EF_SPINNINGLIGHTS = 0x00800000,
    EF_PLASMA = 0x01000000,
    EF_TRAP = 0x02000000,

    //ROGUE
    EF_TRACKER = 0x04000000,
    EF_DOUBLE = 0x08000000,
    EF_SPHERETRANS = 0x10000000,
    EF_TAGTRAIL = 0x20000000,
    EF_HALF_DAMAGE = 0x40000000,
    EF_TRACKERTRAIL = 0x80000000,
    //ROGUE

#ifdef VRX_REPRO
    EF_DUALFIRE = U64BIT(32), // [KEX] dualfire damage color shell
    EF_HOLOGRAM = U64BIT(33), // [Paril-KEX] N64 hologram
    EF_FLASHLIGHT = U64BIT(34), // [Paril-KEX] project flashlight, only for players
    EF_BARREL_EXPLODING = U64BIT(35),
    EF_TELEPORTER2 = U64BIT(36), // [Paril-KEX] n64 teleporter
    EF_GRENADE_LIGHT = U64BIT(37)
#endif
};

// entity_state_t->renderfx flags
enum renderfx_t : uint32_t {
    RF_NONE = 0,
    RF_MINLIGHT = 1, // allways have some light (viewmodel)
    RF_VIEWERMODEL = U32BIT(1), // don't draw through eyes, only mirrors
    RF_WEAPONMODEL = U32BIT(2), // only draw through eyes
    RF_FULLBRIGHT = U32BIT(3), // allways draw full intensity
    RF_DEPTHHACK = U32BIT(4), // for view weapon Z crunching
    RF_TRANSLUCENT = U32BIT(5),
    RF_FRAMELERP = U32BIT(6), // "No interpolation for origins"
    RF_BEAM = U32BIT(7),
    RF_CUSTOMSKIN = U32BIT(8), // skin is an index in image_precache
    RF_GLOW = U32BIT(9), // pulse lighting for bonus items
    RF_SHELL_RED = U32BIT(10),
    RF_SHELL_GREEN = U32BIT(11),
    RF_SHELL_BLUE = U32BIT(12),
#ifdef VRX_REPRO
    RF_NOSHADOW = U32BIT(13),
    RF_CASTSHADOW = U32BIT(14),

    // ROGUE
    RF_IR_VISIBLE = U32BIT(15), // [Paril-KEX] IR visible
    RF_SHELL_DOUBLE = U32BIT(16),
    RF_SHELL_HALF_DAM = U32BIT(17),
    RF_USE_DISGUISE = U32BIT(18),
    // ROGUE

    RF_SHELL_LITE_GREEN = U32BIT(19),
    RF_CUSTOM_LIGHT = U32BIT(20),
    // [Paril-KEX] custom point dlight that is designed to strobe/be turned off; s.frame is radius, s.skinnum is color
    RF_FLARE = U32BIT(21),
    RF_OLD_FRAME_LERP = U32BIT(22),
    // [Paril-KEX] force model to lerp from oldframe in entity state; otherwise it uses last frame client received
    RF_DOT_SHADOW = U32BIT(23), // [Paril-KEX] draw blobby shadow
    RF_LOW_PRIORITY = U32BIT(24),
    // low primority object; if we can't be added to the scene, don't bother replacing entities,
    // and we can be replaced if anything non-low-priority needs room
    RF_NO_LOD = U32BIT(25),
    RF_NO_STEREO = RF_WEAPONMODEL, // this is a bit dumb, but, for looping noises if this is set there's no stereo
    RF_STAIR_STEP = U32BIT(26), // [Paril-KEX] re-tuned, now used to handle stair steps for monsters

    RF_FLARE_LOCK_ANGLE = RF_MINLIGHT,
#endif
    RF_SHELL_YELLOW = RF_SHELL_DOUBLE, //K03
};

#define RF_SHELL_CYAN (RF_SHELL_GREEN | RF_SHELL_BLUE) /* cyan shell */

// [Paril-KEX] make a lightning bolt instead of a laser
#define RF_BEAM_LIGHTNING (RF_BEAM | RF_GLOW)

//
// muzzle flashes / player effects
//
enum player_muzzle_t : uint8_t {
    MZ_BLASTER = 0,
    MZ_MACHINEGUN = 1,
    MZ_SHOTGUN = 2,
    MZ_CHAINGUN1 = 3,
    MZ_CHAINGUN2 = 4,
    MZ_CHAINGUN3 = 5,
    MZ_RAILGUN = 6,
    MZ_ROCKET = 7,
    MZ_GRENADE = 8,
    MZ_LOGIN = 9,
    MZ_LOGOUT = 10,
    MZ_RESPAWN = 11,
    MZ_BFG = 12,
    MZ_SSHOTGUN = 13,
    MZ_HYPERBLASTER = 14,
    MZ_ITEMRESPAWN = 15,
    // RAFAEL
    MZ_IONRIPPER = 16,
    MZ_BLUEHYPERBLASTER = 17,
    MZ_PHALANX = 18,
    MZ_BFG2 = 19,
    MZ_PHALANX2 = 20,

    //ROGUE
    MZ_ETF_RIFLE = 30,
    MZ_UNUSED = 31,
    MZ_SHOTGUN2 = 32,
    MZ_HEATBEAM = 33,
    MZ_BLASTER2 = 34,
    MZ_TRACKER = 35,
    MZ_NUKE1 = 36,
    MZ_NUKE2 = 37,
    MZ_NUKE4 = 38,
    MZ_NUKE8 = 39,
    //ROGUE

    MZ_SILENCED = 128 // bit flag ORed with one of the above numbers
};

//
// monster muzzle flashes
//
enum monster_muzzleflash_id_t : uint16_t {
    MZ2_TANK_BLASTER_1,
    MZ2_TANK_BLASTER_2,
    MZ2_TANK_BLASTER_3,
    MZ2_TANK_MACHINEGUN_1,
    MZ2_TANK_MACHINEGUN_2,
    MZ2_TANK_MACHINEGUN_3,
    MZ2_TANK_MACHINEGUN_4,
    MZ2_TANK_MACHINEGUN_5,
    MZ2_TANK_MACHINEGUN_6,
    MZ2_TANK_MACHINEGUN_7,
    MZ2_TANK_MACHINEGUN_8,
    MZ2_TANK_MACHINEGUN_9,
    MZ2_TANK_MACHINEGUN_10,
    MZ2_TANK_MACHINEGUN_11,
    MZ2_TANK_MACHINEGUN_12,
    MZ2_TANK_MACHINEGUN_13,
    MZ2_TANK_MACHINEGUN_14,
    MZ2_TANK_MACHINEGUN_15,
    MZ2_TANK_MACHINEGUN_16,
    MZ2_TANK_MACHINEGUN_17,
    MZ2_TANK_MACHINEGUN_18,
    MZ2_TANK_MACHINEGUN_19,
    MZ2_TANK_ROCKET_1,
    MZ2_TANK_ROCKET_2,
    MZ2_TANK_ROCKET_3,

    MZ2_INFANTRY_MACHINEGUN_1,
    MZ2_INFANTRY_MACHINEGUN_2,
    MZ2_INFANTRY_MACHINEGUN_3,
    MZ2_INFANTRY_MACHINEGUN_4,
    MZ2_INFANTRY_MACHINEGUN_5,
    MZ2_INFANTRY_MACHINEGUN_6,
    MZ2_INFANTRY_MACHINEGUN_7,
    MZ2_INFANTRY_MACHINEGUN_8,
    MZ2_INFANTRY_MACHINEGUN_9,
    MZ2_INFANTRY_MACHINEGUN_10,
    MZ2_INFANTRY_MACHINEGUN_11,
    MZ2_INFANTRY_MACHINEGUN_12,
    MZ2_INFANTRY_MACHINEGUN_13,

    MZ2_SOLDIER_BLASTER_1,
    MZ2_SOLDIER_BLASTER_2,
    MZ2_SOLDIER_SHOTGUN_1,
    MZ2_SOLDIER_SHOTGUN_2,
    MZ2_SOLDIER_MACHINEGUN_1,
    MZ2_SOLDIER_MACHINEGUN_2,

    MZ2_GUNNER_MACHINEGUN_1,
    MZ2_GUNNER_MACHINEGUN_2,
    MZ2_GUNNER_MACHINEGUN_3,
    MZ2_GUNNER_MACHINEGUN_4,
    MZ2_GUNNER_MACHINEGUN_5,
    MZ2_GUNNER_MACHINEGUN_6,
    MZ2_GUNNER_MACHINEGUN_7,
    MZ2_GUNNER_MACHINEGUN_8,
    MZ2_GUNNER_GRENADE_1,
    MZ2_GUNNER_GRENADE_2,
    MZ2_GUNNER_GRENADE_3,
    MZ2_GUNNER_GRENADE_4,

    MZ2_CHICK_ROCKET_1,

    MZ2_FLYER_BLASTER_1,
    MZ2_FLYER_BLASTER_2,

    MZ2_MEDIC_BLASTER_1,

    MZ2_GLADIATOR_RAILGUN_1,

    MZ2_HOVER_BLASTER_1,

    MZ2_ACTOR_MACHINEGUN_1,

    MZ2_SUPERTANK_MACHINEGUN_1,
    MZ2_SUPERTANK_MACHINEGUN_2,
    MZ2_SUPERTANK_MACHINEGUN_3,
    MZ2_SUPERTANK_MACHINEGUN_4,
    MZ2_SUPERTANK_MACHINEGUN_5,
    MZ2_SUPERTANK_MACHINEGUN_6,
    MZ2_SUPERTANK_ROCKET_1,
    MZ2_SUPERTANK_ROCKET_2,
    MZ2_SUPERTANK_ROCKET_3,

    MZ2_BOSS2_MACHINEGUN_L1,
    MZ2_BOSS2_MACHINEGUN_L2,
    MZ2_BOSS2_MACHINEGUN_L3,
    MZ2_BOSS2_MACHINEGUN_L4,
    MZ2_BOSS2_MACHINEGUN_L5,
    MZ2_BOSS2_ROCKET_1,
    MZ2_BOSS2_ROCKET_2,
    MZ2_BOSS2_ROCKET_3,
    MZ2_BOSS2_ROCKET_4,

    MZ2_FLOAT_BLASTER_1,

    MZ2_SOLDIER_BLASTER_3,
    MZ2_SOLDIER_SHOTGUN_3,
    MZ2_SOLDIER_MACHINEGUN_3,
    MZ2_SOLDIER_BLASTER_4,
    MZ2_SOLDIER_SHOTGUN_4,
    MZ2_SOLDIER_MACHINEGUN_4,
    MZ2_SOLDIER_BLASTER_5,
    MZ2_SOLDIER_SHOTGUN_5,
    MZ2_SOLDIER_MACHINEGUN_5,
    MZ2_SOLDIER_BLASTER_6,
    MZ2_SOLDIER_SHOTGUN_6,
    MZ2_SOLDIER_MACHINEGUN_6,
    MZ2_SOLDIER_BLASTER_7,
    MZ2_SOLDIER_SHOTGUN_7,
    MZ2_SOLDIER_MACHINEGUN_7,
    MZ2_SOLDIER_BLASTER_8,
    MZ2_SOLDIER_SHOTGUN_8,
    MZ2_SOLDIER_MACHINEGUN_8,

    // --- Xian shit below ---
    MZ2_MAKRON_BFG,
    MZ2_MAKRON_BLASTER_1,
    MZ2_MAKRON_BLASTER_2,
    MZ2_MAKRON_BLASTER_3,
    MZ2_MAKRON_BLASTER_4,
    MZ2_MAKRON_BLASTER_5,
    MZ2_MAKRON_BLASTER_6,
    MZ2_MAKRON_BLASTER_7,
    MZ2_MAKRON_BLASTER_8,
    MZ2_MAKRON_BLASTER_9,
    MZ2_MAKRON_BLASTER_10,
    MZ2_MAKRON_BLASTER_11,
    MZ2_MAKRON_BLASTER_12,
    MZ2_MAKRON_BLASTER_13,
    MZ2_MAKRON_BLASTER_14,
    MZ2_MAKRON_BLASTER_15,
    MZ2_MAKRON_BLASTER_16,
    MZ2_MAKRON_BLASTER_17,
    MZ2_MAKRON_RAILGUN_1,
    MZ2_JORG_MACHINEGUN_L1,
    MZ2_JORG_MACHINEGUN_L2,
    MZ2_JORG_MACHINEGUN_L3,
    MZ2_JORG_MACHINEGUN_L4,
    MZ2_JORG_MACHINEGUN_L5,
    MZ2_JORG_MACHINEGUN_L6,
    MZ2_JORG_MACHINEGUN_R1,
    MZ2_JORG_MACHINEGUN_R2,
    MZ2_JORG_MACHINEGUN_R3,
    MZ2_JORG_MACHINEGUN_R4,
    MZ2_JORG_MACHINEGUN_R5,
    MZ2_JORG_MACHINEGUN_R6,
    MZ2_JORG_BFG_1,
    MZ2_BOSS2_MACHINEGUN_R1,
    MZ2_BOSS2_MACHINEGUN_R2,
    MZ2_BOSS2_MACHINEGUN_R3,
    MZ2_BOSS2_MACHINEGUN_R4,
    MZ2_BOSS2_MACHINEGUN_R5,

    //ROGUE
    MZ2_CARRIER_MACHINEGUN_L1,
    MZ2_CARRIER_MACHINEGUN_R1,
    MZ2_CARRIER_GRENADE,
    MZ2_TURRET_MACHINEGUN,
    MZ2_TURRET_ROCKET,
    MZ2_TURRET_BLASTER,
    MZ2_STALKER_BLASTER,
    MZ2_DAEDALUS_BLASTER,
    MZ2_MEDIC_BLASTER_2,
    MZ2_CARRIER_RAILGUN,
    MZ2_WIDOW_DISRUPTOR,
    MZ2_WIDOW_BLASTER,
    MZ2_WIDOW_RAIL,
    MZ2_WIDOW_PLASMABEAM = 151, // PMM unused
    MZ2_CARRIER_MACHINEGUN_L2,
    MZ2_CARRIER_MACHINEGUN_R2,
    MZ2_WIDOW_RAIL_LEFT,
    MZ2_WIDOW_RAIL_RIGHT,
    MZ2_WIDOW_BLASTER_SWEEP1,
    MZ2_WIDOW_BLASTER_SWEEP2,
    MZ2_WIDOW_BLASTER_SWEEP3,
    MZ2_WIDOW_BLASTER_SWEEP4,
    MZ2_WIDOW_BLASTER_SWEEP5,
    MZ2_WIDOW_BLASTER_SWEEP6,
    MZ2_WIDOW_BLASTER_SWEEP7,
    MZ2_WIDOW_BLASTER_SWEEP8,
    MZ2_WIDOW_BLASTER_SWEEP9,
    MZ2_WIDOW_BLASTER_100,
    MZ2_WIDOW_BLASTER_90,
    MZ2_WIDOW_BLASTER_80,
    MZ2_WIDOW_BLASTER_70,
    MZ2_WIDOW_BLASTER_60,
    MZ2_WIDOW_BLASTER_50,
    MZ2_WIDOW_BLASTER_40,
    MZ2_WIDOW_BLASTER_30,
    MZ2_WIDOW_BLASTER_20,
    MZ2_WIDOW_BLASTER_10,
    MZ2_WIDOW_BLASTER_0,
    MZ2_WIDOW_BLASTER_10L,
    MZ2_WIDOW_BLASTER_20L,
    MZ2_WIDOW_BLASTER_30L,
    MZ2_WIDOW_BLASTER_40L,
    MZ2_WIDOW_BLASTER_50L,
    MZ2_WIDOW_BLASTER_60L,
    MZ2_WIDOW_BLASTER_70L,
    MZ2_WIDOW_RUN_1,
    MZ2_WIDOW_RUN_2,
    MZ2_WIDOW_RUN_3,
    MZ2_WIDOW_RUN_4,
    MZ2_WIDOW_RUN_5,
    MZ2_WIDOW_RUN_6,
    MZ2_WIDOW_RUN_7,
    MZ2_WIDOW_RUN_8,
    MZ2_CARRIER_ROCKET_1,
    MZ2_CARRIER_ROCKET_2,
    MZ2_CARRIER_ROCKET_3,
    MZ2_CARRIER_ROCKET_4,
    MZ2_WIDOW2_BEAMER_1,
    MZ2_WIDOW2_BEAMER_2,
    MZ2_WIDOW2_BEAMER_3,
    MZ2_WIDOW2_BEAMER_4,
    MZ2_WIDOW2_BEAMER_5,
    MZ2_WIDOW2_BEAM_SWEEP_1,
    MZ2_WIDOW2_BEAM_SWEEP_2,
    MZ2_WIDOW2_BEAM_SWEEP_3,
    MZ2_WIDOW2_BEAM_SWEEP_4,
    MZ2_WIDOW2_BEAM_SWEEP_5,
    MZ2_WIDOW2_BEAM_SWEEP_6,
    MZ2_WIDOW2_BEAM_SWEEP_7,
    MZ2_WIDOW2_BEAM_SWEEP_8,
    MZ2_WIDOW2_BEAM_SWEEP_9,
    MZ2_WIDOW2_BEAM_SWEEP_10,
    MZ2_WIDOW2_BEAM_SWEEP_11,

    // ROGUE

    // [Paril-KEX]
    MZ2_SOLDIER_RIPPER_1,
    MZ2_SOLDIER_RIPPER_2,
    MZ2_SOLDIER_RIPPER_3,
    MZ2_SOLDIER_RIPPER_4,
    MZ2_SOLDIER_RIPPER_5,
    MZ2_SOLDIER_RIPPER_6,
    MZ2_SOLDIER_RIPPER_7,
    MZ2_SOLDIER_RIPPER_8,

    MZ2_SOLDIER_HYPERGUN_1,
    MZ2_SOLDIER_HYPERGUN_2,
    MZ2_SOLDIER_HYPERGUN_3,
    MZ2_SOLDIER_HYPERGUN_4,
    MZ2_SOLDIER_HYPERGUN_5,
    MZ2_SOLDIER_HYPERGUN_6,
    MZ2_SOLDIER_HYPERGUN_7,
    MZ2_SOLDIER_HYPERGUN_8,
    MZ2_GUARDIAN_BLASTER,
    MZ2_ARACHNID_RAIL1,
    MZ2_ARACHNID_RAIL2,
    MZ2_ARACHNID_RAIL_UP1,
    MZ2_ARACHNID_RAIL_UP2,

    MZ2_INFANTRY_MACHINEGUN_14, // run-attack
    MZ2_INFANTRY_MACHINEGUN_15, // run-attack
    MZ2_INFANTRY_MACHINEGUN_16, // run-attack
    MZ2_INFANTRY_MACHINEGUN_17, // run-attack
    MZ2_INFANTRY_MACHINEGUN_18, // run-attack
    MZ2_INFANTRY_MACHINEGUN_19, // run-attack
    MZ2_INFANTRY_MACHINEGUN_20, // run-attack
    MZ2_INFANTRY_MACHINEGUN_21, // run-attack

    MZ2_GUNCMDR_CHAINGUN_1, // straight
    MZ2_GUNCMDR_CHAINGUN_2, // dodging

    MZ2_GUNCMDR_GRENADE_MORTAR_1,
    MZ2_GUNCMDR_GRENADE_MORTAR_2,
    MZ2_GUNCMDR_GRENADE_MORTAR_3,
    MZ2_GUNCMDR_GRENADE_FRONT_1,
    MZ2_GUNCMDR_GRENADE_FRONT_2,
    MZ2_GUNCMDR_GRENADE_FRONT_3,
    MZ2_GUNCMDR_GRENADE_CROUCH_1,
    MZ2_GUNCMDR_GRENADE_CROUCH_2,
    MZ2_GUNCMDR_GRENADE_CROUCH_3,

    // prone
    MZ2_SOLDIER_BLASTER_9,
    MZ2_SOLDIER_SHOTGUN_9,
    MZ2_SOLDIER_MACHINEGUN_9,
    MZ2_SOLDIER_RIPPER_9,
    MZ2_SOLDIER_HYPERGUN_9,

    // alternate frontwards grenades
    MZ2_GUNNER_GRENADE2_1,
    MZ2_GUNNER_GRENADE2_2,
    MZ2_GUNNER_GRENADE2_3,
    MZ2_GUNNER_GRENADE2_4,

    MZ2_INFANTRY_MACHINEGUN_22,

    // supertonk
    MZ2_SUPERTANK_GRENADE_1,
    MZ2_SUPERTANK_GRENADE_2,

    // hover & daedalus other side
    MZ2_HOVER_BLASTER_2,
    MZ2_DAEDALUS_BLASTER_2,

    // medic (commander) sweeps
    MZ2_MEDIC_HYPERBLASTER1_1,
    MZ2_MEDIC_HYPERBLASTER1_2,
    MZ2_MEDIC_HYPERBLASTER1_3,
    MZ2_MEDIC_HYPERBLASTER1_4,
    MZ2_MEDIC_HYPERBLASTER1_5,
    MZ2_MEDIC_HYPERBLASTER1_6,
    MZ2_MEDIC_HYPERBLASTER1_7,
    MZ2_MEDIC_HYPERBLASTER1_8,
    MZ2_MEDIC_HYPERBLASTER1_9,
    MZ2_MEDIC_HYPERBLASTER1_10,
    MZ2_MEDIC_HYPERBLASTER1_11,
    MZ2_MEDIC_HYPERBLASTER1_12,

    MZ2_MEDIC_HYPERBLASTER2_1,
    MZ2_MEDIC_HYPERBLASTER2_2,
    MZ2_MEDIC_HYPERBLASTER2_3,
    MZ2_MEDIC_HYPERBLASTER2_4,
    MZ2_MEDIC_HYPERBLASTER2_5,
    MZ2_MEDIC_HYPERBLASTER2_6,
    MZ2_MEDIC_HYPERBLASTER2_7,
    MZ2_MEDIC_HYPERBLASTER2_8,
    MZ2_MEDIC_HYPERBLASTER2_9,
    MZ2_MEDIC_HYPERBLASTER2_10,
    MZ2_MEDIC_HYPERBLASTER2_11,
    MZ2_MEDIC_HYPERBLASTER2_12,

    // only used for compile time checks
    MZ2_LAST
};

extern vec3_t monster_flash_offset[212];


// temp entity events
//
// Temp entity events are for things that happen
// at a location seperate from any existing entity.
// Temporary entity messages are explicitly constructed
// and broadcast.
typedef enum {
    TE_GUNSHOT,
    TE_BLOOD,
    TE_BLASTER,
    TE_RAILTRAIL,
    TE_SHOTGUN,
    TE_EXPLOSION1,
    TE_EXPLOSION2,
    TE_ROCKET_EXPLOSION,
    TE_GRENADE_EXPLOSION,
    TE_SPARKS,
    TE_SPLASH,
    TE_BUBBLETRAIL,
    TE_SCREEN_SPARKS,
    TE_SHIELD_SPARKS,
    TE_BULLET_SPARKS,
    TE_LASER_SPARKS,
    TE_PARASITE_ATTACK,
    TE_ROCKET_EXPLOSION_WATER,
    TE_GRENADE_EXPLOSION_WATER,
    TE_MEDIC_CABLE_ATTACK,
    TE_BFG_EXPLOSION,
    TE_BFG_BIGEXPLOSION,
    TE_BOSSTPORT = 22, // used as '22' in a map, so DON'T RENUMBER!!!
    TE_BFG_LASER,
    TE_GRAPPLE_CABLE,
    TE_WELDING_SPARKS,
    TE_GREENBLOOD,
    TE_BLUEHYPERBLASTER_DUMMY, // for compat, use TE_BLUEHYPERBLASTER
    TE_PLASMA_EXPLOSION,
    TE_TUNNEL_SPARKS,
    //ROGUE
    TE_BLASTER2,
    TE_RAILTRAIL2,
    TE_FLAME,
    TE_LIGHTNING,
    TE_DEBUGTRAIL,
    TE_PLAIN_EXPLOSION,
    TE_FLASHLIGHT,
    TE_FORCEWALL,
    TE_HEATBEAM,
    TE_MONSTER_HEATBEAM,
    TE_STEAM,
    TE_BUBBLETRAIL2,
    TE_MOREBLOOD,
    TE_HEATBEAM_SPARKS,
    TE_HEATBEAM_STEAM,
    TE_CHAINFIST_SMOKE,
    TE_ELECTRIC_SPARKS,
    TE_TRACKER_EXPLOSION,
    TE_TELEPORT_EFFECT,
    TE_DBALL_GOAL,
    TE_WIDOWBEAMOUT,
    TE_NUKEBLAST,
    TE_WIDOWSPLASH,
    TE_EXPLOSION1_BIG,
    TE_EXPLOSION1_NP,
    TE_FLECHETTE,
    //ROGUE

#ifdef VRX_REPRO
    // [Paril-KEX]
    TE_BLUEHYPERBLASTER,
    TE_BFG_ZAP,
    TE_BERSERK_SLAM,
    TE_GRAPPLE_CABLE_2,
    TE_POWER_SPLASH,
    TE_LIGHTNING_BEAM,
    TE_EXPLOSION1_NL,
    TE_EXPLOSION2_NL,
#endif
} temp_event_t;

enum splash_color_t : uint8_t {
    SPLASH_UNKNOWN = 0,
    SPLASH_SPARKS = 1,
    SPLASH_BLUE_WATER = 2,
    SPLASH_BROWN_WATER = 3,
    SPLASH_SLIME = 4,
    SPLASH_LAVA = 5,
    SPLASH_BLOOD = 6,
#ifdef VRX_REPRO
    // [Paril-KEX] N64 electric sparks that go zap
    SPLASH_ELECTRIC = 7,
#endif
};


// sound channels
// channel 0 never willingly overrides
// other channels (1-7) allways override a playing sound on that channel
enum soundchan_t : uint8_t {
    CHAN_AUTO = 0,
    CHAN_WEAPON = 1,
    CHAN_VOICE = 2,
    CHAN_ITEM = 3,
    CHAN_BODY = 4,
#ifdef VRX_REPRO
    CHAN_AUX = 5,
    CHAN_FOOTSTEP = 6,
    CHAN_AUX3 = 7,
#endif

    // modifier flags
    CHAN_NO_PHS_ADD = 8, // send to all clients, not just ones in PHS (ATTN 0 will also do this)
    CHAN_RELIABLE = 16, // send by reliable message, not datagram
};



// sound attenuation values
#define	ATTN_NONE               0	// full volume the entire level
#define	ATTN_NORM               1
#define	ATTN_IDLE               2
#define	ATTN_STATIC             3	// diminish very rapidly with distance


// dmflags->value flags
#define	DF_NO_HEALTH		0x00000001	// 1
#define	DF_NO_ITEMS			0x00000002	// 2
#define	DF_WEAPONS_STAY		0x00000004	// 4
#define	DF_NO_FALLING		0x00000008	// 8
#define	DF_INSTANT_ITEMS	0x00000010	// 16
#define	DF_SAME_LEVEL		0x00000020	// 32
#define DF_SKINTEAMS		0x00000040	// 64
#define DF_MODELTEAMS		0x00000080	// 128
#define DF_NO_FRIENDLY_FIRE	0x00000100	// 256
#define	DF_SPAWN_FARTHEST	0x00000200	// 512
#define DF_FORCE_RESPAWN	0x00000400	// 1024
#define DF_NO_ARMOR			0x00000800	// 2048
#define DF_ALLOW_EXIT		0x00001000	// 4096
#define DF_INFINITE_AMMO	0x00002000	// 8192
#define DF_QUAD_DROP		0x00004000	// 16384
#define DF_FIXED_FOV		0x00008000	// 32768

// RAFAEL
#define	DF_QUADFIRE_DROP	0x00010000	// 65536

//ROGUE
#define DF_NO_MINES			0x00020000
#define DF_NO_STACK_DOUBLE	0x00040000
#define DF_NO_NUKES			0x00080000
#define DF_NO_SPHERES		0x00100000
//ROGUE
#define ROGUE_VERSION_ID		1278

#define ROGUE_VERSION_STRING	"08/21/1998 Beta 2 for Ensemble"
/*
==========================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

==========================================================
*/

#define	ANGLE2SHORT(x)	((int)((x)*65536/360) & 65535)
#define	SHORT2ANGLE(x)	((x)*(360.0/65536))


//
// config strings are a general means of communication from
// the server to all connected clients.
// Each config string can be at most MAX_QPATH characters.
//
enum {
    CS_NAME,
    CS_CDTRACK,
    CS_SKY,
    CS_SKYAXIS, // %f %f %f format
    CS_SKYROTATE,
    CS_STATUSBAR, // display program string

#ifndef VRX_REPRO
    CS_AIRACCEL,  // air acceleration control
#else
    CS_AIRACCEL, // air acceleration control
#endif
    CS_MAXCLIENTS,
    CS_MAPCHECKSUM, // for catching cheater maps

    CS_MODELS,
    CS_SOUNDS = CS_MODELS + MAX_MODELS,
    CS_IMAGES = CS_SOUNDS + MAX_SOUNDS,
    CS_LIGHTS = CS_IMAGES + MAX_IMAGES,
    CS_ITEMS = CS_LIGHTS + MAX_LIGHTSTYLES,
    CS_PLAYERSKINS = CS_ITEMS + MAX_ITEMS,
    CS_GENERAL = CS_PLAYERSKINS + MAX_CLIENTS,

#ifdef VRX_REPRO
    CS_WHEEL_WEAPONS = CS_GENERAL + MAX_GENERAL, // [Paril-KEX] see MAX_WHEEL_ITEMS
    CS_WHEEL_AMMO = CS_WHEEL_WEAPONS + MAX_WHEEL_ITEMS, // [Paril-KEX] see MAX_WHEEL_ITEMS
    CS_WHEEL_POWERUPS = CS_WHEEL_AMMO + MAX_WHEEL_ITEMS, // [Paril-KEX] see MAX_WHEEL_ITEMS
    CS_CD_LOOP_COUNT = CS_WHEEL_POWERUPS + MAX_WHEEL_ITEMS, // [Paril-KEX] override default loop count
    CS_GAME_STYLE, // [Paril-KEX] see game_style_t
#endif

    MAX_CONFIGSTRINGS
};

// [Sam-KEX] New define for max config string length
#ifdef VRX_REPRO
constexpr size_t CS_MAX_STRING_LENGTH = 96;
#else
constexpr size_t CS_MAX_STRING_LENGTH = 64;
#endif
constexpr size_t CS_MAX_STRING_LENGTH_OLD = 64;

#include "quake2/bg_local.h"

//==============================================

// player_state->stats[] indexes
enum player_stat_t {
    // #define STAT_HEALTH_ICON		0
    STAT_HEALTH = 1,
    STAT_AMMO_ICON = 2,
    STAT_AMMO = 3,
    // 	STAT_ARMOR_ICON =		4,
    STAT_ARMOR = 5,
    STAT_SELECTED_ICON = 6,
    STAT_PICKUP_ICON = 7,
    STAT_PICKUP_STRING = 8,
    STAT_TIMER_ICON = 9,
    STAT_TIMER = 10,
    STAT_HELPICON = 11,
    STAT_SELECTED_ITEM = 12,
    STAT_LAYOUTS = 13,
    STAT_FRAGS = 14,
    STAT_FLASHES = 15, // cleared each frame, 1 = health, 2 = armor
    STAT_CHASE = 16,

    //K03 Begin
    STAT_TEAM_ICON = 17,
    STAT_STATION_ICON = 18,
    STAT_STATION_TIME = 19,
    STAT_CHARGE_LEVEL = 20,
    STAT_INVASIONTIME = 21, // -az
    STAT_VOTESTRING = 22,
    // STAT_RANK =             24,
    STAT_ID_DAMAGE = 24,
    STAT_STREAK = 25,
    STAT_ID_ARMOR = 26,
    STAT_SELECTED_NUM = 27,
    STAT_TIMEMIN = 28,
    STAT_SPECTATOR = 29,
    STAT_ID_HEALTH = 30,
    STAT_ID_AMMO = 31,
    //K03 End

    // [Kex] More stats for weapon wheel
STAT_WEAPONS_OWNED_1 = 32,
STAT_WEAPONS_OWNED_2 = 33,
STAT_AMMO_INFO_START = 34,
STAT_AMMO_INFO_END = STAT_AMMO_INFO_START + NUM_AMMO_STATS - 1,
STAT_POWERUP_INFO_START,
STAT_POWERUP_INFO_END = STAT_POWERUP_INFO_START + NUM_POWERUP_STATS - 1,

// [Paril-KEX] Key display
STAT_KEY_A,
STAT_KEY_B,
STAT_KEY_C,

// [Paril-KEX] currently active wheel weapon (or one we're switching to)
STAT_ACTIVE_WHEEL_WEAPON,
// [Paril-KEX] top of screen coop respawn state
STAT_COOP_RESPAWN,
// [Paril-KEX] respawns remaining
STAT_LIVES,
// [Paril-KEX] hit marker; # of damage we successfully landed
STAT_HIT_MARKER,
// [Paril-KEX]
STAT_SELECTED_ITEM_NAME,
// [Paril-KEX]
STAT_HEALTH_BARS, // two health bar values; 7 bits for value, 1 bit for active
// [Paril-KEX]
STAT_ACTIVE_WEAPON,

// don't use; just for verification
STAT_LAST
};

#ifdef VRX_REPRO
#define MAX_STATS 64
#else
#define	MAX_STATS				32
#endif

static_assert(STAT_LAST <= MAX_STATS, "playerstats overfilled");


// entity_state_t->event values
// ertity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.
// All muzzle flashes really should be converted to events...
typedef enum {
    EV_NONE,
    EV_ITEM_RESPAWN,
    EV_FOOTSTEP,
    EV_FALLSHORT,
    EV_FALL,
    EV_FALLFAR,
    EV_PLAYER_TELEPORT,
    EV_OTHER_TELEPORT,

#ifdef VRX_REPRO
    // [Paril-KEX]
    EV_OTHER_FOOTSTEP,
    EV_LADDER_STEP,
#endif
} entity_event_t;

#ifdef VRX_REPRO
// [Paril-KEX] player s.skinnum's encode additional data
union player_skinnum_t {
    int32_t skinnum;

    struct {
        uint8_t client_num; // client index
        uint8_t vwep_index; // vwep index
        int8_t viewheight; // viewheight
        uint8_t team_index: 4; // team #; note that teams are 1-indexed here, with 0 meaning no team
        // (spectators in CTF would be 0, for instance)
        uint8_t poi_icon: 4; // poi icon; 0 default friendly, 1 dead, others unused
    };
};
#endif

// entity_state_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
typedef struct entity_state_s {
    uint32_t number; // edict index

    vec3_t origin;
    vec3_t angles;
    vec3_t old_origin; // for lerping
    int32_t modelindex;
    int32_t modelindex2, modelindex3, modelindex4; // weapons, CTF flags, etc
    int32_t frame;
    int32_t skinnum;
    enum effects_t effects; // PGM - we're filling it, so it needs to be unsigned
    enum renderfx_t renderfx;
    uint32_t solid; // for client side prediction, 8*(bits 0-4) is x/y radius
    // 8*(bits 5-9) is z down distance, 8(bits10-15) is z up
    // gi.linkentity sets this properly
    int32_t sound; // for looping sounds, to guarantee shutoff
    entity_event_t event; // impulse events -- muzzle flashes, footsteps, etc
    // events only go out for a single frame, they
    // are automatically cleared each frame

#ifdef VRX_REPRO
    float alpha; // [Paril-KEX] alpha scalar; 0 is a "default" value, which will respect other
    // settings (default 1.0 for most things, EF_TRANSLUCENT will default this
    // to 0.3, etc)
    float scale; // [Paril-KEX] model scale scalar; 0 is a "default" value, like with alpha.
    uint8_t instance_bits; // [Paril-KEX] players that *can't* see this entity will have a bit of 1. handled by
    // the server, do not set directly.
    // [Paril-KEX] allow specifying volume/attn for looping noises; note that
    // zero will be defaults (1.0 and 3.0 respectively); -1 attenuation is used
    // for "none" (similar to target_speaker) for no phs/pvs looping noises
    float loop_volume;
    float loop_attenuation;
    // [Paril-KEX] for proper client-side owner collision skipping
    int32_t owner;
    // [Paril-KEX] for custom interpolation stuff
    int32_t old_frame;
#endif
} entity_state_t;

//==============================================


// player_state_t is the information needed in addition to pmove_state_t
// to rendered a view.  There will only be 10 player_state_t sent each second,
// but the number of pmove_state_t changes will be reletive to client
// frame rates
typedef struct {
    pmove_state_t pmove; // for prediction

    // these fields do not need to be communicated bit-precise

    vec3_t viewangles; // for fixed views
    vec3_t viewoffset; // add to pmovestate->origin
    vec3_t kick_angles; // add to view direction to get render angles
    // set by weapon kicks, pain effects, etc

    vec3_t gunangles;
    vec3_t gunoffset;
    int32_t gunindex;
#ifdef VRX_REPRO
    int32_t gunskin;
#endif
    int32_t gunframe;
#ifdef VRX_REPRO
    int32_t gunrate; // [Paril-KEX] tickrate of gun animations; 0 and 10 are equivalent
#endif

    float screen_blend[4]; // rgba full screen effect
#ifdef VRX_REPRO
    float damage_blend[4]; //  [Paril-KEX] rgba full screen effect
#endif

    float fov; // horizontal field of view

    enum refdef_flags_t rdflags; // refdef flags

    int16_t stats[MAX_STATS]; // fast status bar updates

#ifdef VRX_REPRO
    uint8_t team_id;
#endif
} player_state_t;

// ==================

// Wrapped, thread safe mem allocation.
void *vrx_malloc(size_t Size, int Tag);

void vrx_free(void *mem);

double sigmoid(const double x);

int sigmoid_distribute(int min, int max, int value);

int rand_sigmoid_distribute(int min, int max);

// rng a number within the range with a uniform distribution n times and take the average.
// simulates a bell curve. more iters = more centralized
int rand_clt_distribute(int min, int max, int itercnt);



#endif
