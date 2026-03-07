#pragma once

// define GAME_INCLUDE so that game.h does not define the
// short, server-visible gclient_t and edict_t structures,
// because we define the full size ones in this file
#define GAME_INCLUDE
#include "game.h"

// [Paril-KEX] generic touch list; used for contact entities
typedef struct touch_list_s
{
	size_t	num;
	trace_t traces[MAXTOUCH];
} touch_list_t;

//
// p_move.c
//
typedef struct
{
	int32_t		airaccel;
	qboolean		n64_physics;
} pm_config_t;

extern pm_config_t pm_config;

void Pmove(pmove_t *pmove);
typedef trace_t (*pm_trace_func_t)(const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end);
void PM_StepSlideMove_Generic(vec3_t origin, vec3_t velocity, float frametime, const vec3_t mins, const vec3_t maxs, touch_list_t *touch, qboolean has_time, pm_trace_func_t trace);

typedef enum
{
	STUCK_GOOD_POSITION,
	STUCK_FIXED,
	STUCK_NO_GOOD_POSITION
} stuck_result_t;

typedef trace_t (*stuck_object_trace_fn_t)(const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end);

stuck_result_t G_FixStuckObject_Generic(vec3_t origin, const vec3_t own_mins, const vec3_t own_maxs, stuck_object_trace_fn_t trace);

// state for coop respawning; used to select which
// message to print for the player this is set on.
typedef enum
{
	COOP_RESPAWN_NONE, // no messagee
	COOP_RESPAWN_IN_COMBAT, // player is in combat
	COOP_RESPAWN_BAD_AREA, // player not in a good spot
	COOP_RESPAWN_BLOCKED, // spawning was blocked by something
	COOP_RESPAWN_WAITING, // for players that are waiting to respawn
	COOP_RESPAWN_NO_LIVES, // out of lives, so need to wait until level switch
	COOP_RESPAWN_TOTAL
} coop_respawn_t;

// reserved general CS ranges
enum
{
	CONFIG_CTF_MATCH = CS_GENERAL,
	CONFIG_CTF_TEAMINFO,
	CONFIG_CTF_PLAYER_NAME,
	CONFIG_CTF_PLAYER_NAME_END = CONFIG_CTF_PLAYER_NAME + MAX_CLIENTS,

	// nb: offset by 1 since NONE is zero
	CONFIG_COOP_RESPAWN_STRING,
	CONFIG_COOP_RESPAWN_STRING_END = CONFIG_COOP_RESPAWN_STRING + (COOP_RESPAWN_TOTAL - 1),

	// [Paril-KEX] if 1, n64 player physics apply
	CONFIG_N64_PHYSICS,
	CONFIG_HEALTH_BAR_NAME, // active health bar name

	CONFIG_STORY,

	CONFIG_LAST
};

_Static_assert(CONFIG_LAST <= CS_GENERAL + MAX_GENERAL, "config list overflow");

// powerup IDs
typedef enum
{
	POWERUP_SCREEN,
	POWERUP_SHIELD,

	POWERUP_AM_BOMB,

	POWERUP_QUAD,
	POWERUP_QUADFIRE,
	POWERUP_INVULNERABILITY,
	POWERUP_INVISIBILITY,
	POWERUP_SILENCER,
	POWERUP_REBREATHER,
	POWERUP_ENVIROSUIT,
	POWERUP_ADRENALINE,
	POWERUP_IR_GOGGLES,
	POWERUP_DOUBLE,
	POWERUP_SPHERE_VENGEANCE,
	POWERUP_SPHERE_HUNTER,
	POWERUP_SPHERE_DEFENDER,
	POWERUP_DOPPELGANGER,

	POWERUP_FLASHLIGHT,
	POWERUP_COMPASS,
	POWERUP_TECH1,
	POWERUP_TECH2,
	POWERUP_TECH3,
	POWERUP_TECH4,
	POWERUP_MAX
} powerup_t;

// ammo stats compressed in 9 bits per entry
// since the range is 0-300
#define BITS_PER_AMMO 9

#define num_of_type_for_bits(TI, num_bits) ((num_bits + (sizeof(TI) * 8) - 1) / ((sizeof(TI) * 8)))

#define set_compressed_integer(bits_per_value, start, id, count) { \
	uint16_t bit_offset = bits_per_value * id; \
	uint16_t byte_off = bit_offset / 8; \
	uint16_t bit_shift = bit_offset % 8; \
	uint16_t mask = ((1 << bits_per_value) - 1) << bit_shift; \
	uint16_t *base = (uint16_t *) ((uint8_t *) start + byte_off); \
	*base = (*base & ~mask) | ((count << bit_shift) & mask); \
}

#define get_compressed_integer(bits_per_value, start, id) ({ \
	uint16_t bit_offset = bits_per_value * id; \
	uint16_t byte_off = bit_offset / 8; \
	uint16_t bit_shift = bit_offset % 8; \
	uint16_t mask = ((1 << bits_per_value) - 1) << bit_shift; \
	uint16_t *base = (uint16_t *) ((uint8_t *) start + byte_off); \
	(*base & mask) >> bit_shift; \
})

#define NUM_BITS_FOR_AMMO 9
#define NUM_AMMO_STATS num_of_type_for_bits(uint16_t, NUM_BITS_FOR_AMMO * AMMO_MAX)
// if this value is set on an STAT_AMMO_INFO_xxx, don't render ammo
#define AMMO_VALUE_INFINITE ((1 << NUM_BITS_FOR_AMMO) - 1)

static inline void G_SetAmmoStat(uint16_t *start, uint8_t ammo_id, uint16_t count)
{
	set_compressed_integer(NUM_BITS_FOR_AMMO, start, ammo_id, count);
}

static inline uint16_t G_GetAmmoStat(uint16_t *start, uint8_t ammo_id)
{
	return get_compressed_integer(NUM_BITS_FOR_AMMO, start, ammo_id);
}

// powerup stats compressed in 2 bits per entry;
// 3 is the max you'll ever hold, and for some
// (flashlight) it's to indicate on/off state
#define NUM_BITS_PER_POWERUP 2
#define NUM_POWERUP_STATS num_of_type_for_bits(uint16_t, NUM_BITS_PER_POWERUP * POWERUP_MAX)

static inline void G_SetPowerupStat(uint16_t *start, uint8_t powerup_id, uint16_t count)
{
	set_compressed_integer(NUM_BITS_PER_POWERUP, start, powerup_id, count);
}

static inline uint16_t G_GetPowerupStat(uint16_t *start, uint8_t powerup_id)
{
	return get_compressed_integer(NUM_BITS_PER_POWERUP, start, powerup_id);
}

// player_state->stats[] indexes
typedef enum
{
	BG_STAT_CTF_TEAM1_PIC = 18,
	BG_STAT_CTF_TEAM1_CAPS = 19,
	BG_STAT_CTF_TEAM2_PIC = 20,
	BG_STAT_CTF_TEAM2_CAPS = 21,
	BG_STAT_CTF_FLAG_PIC = 22,
	BG_STAT_CTF_JOINED_TEAM1_PIC = 23,
	BG_STAT_CTF_JOINED_TEAM2_PIC = 24,
	BG_STAT_CTF_TEAM1_HEADER = 25,
	BG_STAT_CTF_TEAM2_HEADER = 26,
	BG_STAT_CTF_TECH = 27,
	BG_STAT_CTF_ID_VIEW = 28,
	BG_STAT_CTF_MATCH = 29,
	BG_STAT_CTF_ID_VIEW_COLOR = 30,
	BG_STAT_CTF_TEAMINFO = 31,

    // [Kex] More stats for weapon wheel
    BG_STAT_WEAPONS_OWNED_1 = 32,
    BG_STAT_WEAPONS_OWNED_2 = 33,
    BG_STAT_AMMO_INFO_START = 34,
    BG_STAT_AMMO_INFO_END = BG_STAT_AMMO_INFO_START + NUM_AMMO_STATS - 1,
	BG_STAT_POWERUP_INFO_START,
	BG_STAT_POWERUP_INFO_END = BG_STAT_POWERUP_INFO_START + NUM_POWERUP_STATS - 1,

    // [Paril-KEX] Key display
    BG_STAT_KEY_A,
    BG_STAT_KEY_B,
    BG_STAT_KEY_C,

    // [Paril-KEX] currently active wheel weapon (or one we're switching to)
    BG_STAT_ACTIVE_WHEEL_WEAPON,
	// [Paril-KEX] top of screen coop respawn state
	BG_STAT_COOP_RESPAWN,
	// [Paril-KEX] respawns remaining
	BG_STAT_LIVES,
	// [Paril-KEX] hit marker; # of damage we successfully landed
	BG_STAT_HIT_MARKER,
	// [Paril-KEX]
	BG_STAT_SELECTED_ITEM_NAME,
	// [Paril-KEX]
	BG_STAT_HEALTH_BARS, // two health bar values; 7 bits for value, 1 bit for active
	// [Paril-KEX]
	BG_STAT_ACTIVE_WEAPON,

	// don't use; just for verification
    BG_STAT_LAST
} bg_player_stat_t;

// [KEX] we have a lot of stats, but let's just make sure we don't overflow the max.
// MAX_STATS is usually 32 in vanilla, but we might have increased it.
_Static_assert(BG_STAT_LAST <= 128, "stats list overflow");