#ifndef G_ABILITIES_H
#define G_ABILITIES_H

typedef struct upgrade_s
{
	int32_t			level;
	int32_t			current_level;
	int32_t			soft_max;
	int32_t			hard_max;
	int32_t			modifier;
	float		delay;
	int32_t			charge; // 3.5 percent ability is charged up
	int32_t			ammo; // ability-specific ammo
	int32_t			max_ammo; // maximum ability-specific ammo
	int32_t			ammo_regenframe; // frame ability ammo should regenerate
	qboolean	disable;
	int32_t     	general_skill; // vrxchile 2.7 allow mobility menu
	qboolean	hidden;
	qboolean	runed;
}upgrade_t;

typedef struct muted_s
{
	edict_t		*player;
	int			time;
}muted_t;

typedef struct skills_s
{
	muted_t		mutelist[MAX_CLIENTS];	//mute certain players

	uint64_t experience;
	uint64_t next_level;
	int32_t administrator;
	int32_t level;
	int32_t speciality_points;
	int32_t weapon_points;
	int32_t respawn_weapon;

	uint64_t frags;
	uint64_t fragged;
	uint64_t credits;
	uint64_t weapon_respawns;

	int class_num;
	int boss;
	int streak;

	int current_health;
	int max_health;
	int current_armor;
	int max_armor;

	uint64_t shots;
	uint64_t shots_hit;

	uint64_t num_sprees;
	uint32_t max_streak;
	uint32_t suicides;
	uint32_t teleports;
	uint32_t spree_wars;
	uint32_t break_sprees;
	uint32_t break_spree_wars;
	uint32_t num_2fers;

	//ctf
	uint32_t flag_pickups;	//number of times player grabs the flag
	uint32_t flag_captures;	//number of times player caps the flag
	uint32_t flag_returns;	//number of times player returns his own flag
	uint32_t flag_kills;		//number of times player kills the flag carrier
	uint32_t offense_kills;	//number of times player kills a defender
	uint32_t defense_kills;	//number of times player defends his base by killing a player
	uint32_t assists;		//number of times player gains an assist (flag_kill or flag_return just before a flag_cap)
	//end ctf

	uint32_t playingtime;			//Playing time today (in seconds)
	uint32_t total_playtime;			//Total playing time in minutes

	int32_t	inventory[MAX_ITEMS];
	char password[24];
	char member_since[30];
	char last_played[30];
	char player_name[24];
	char owner[24];
	char masterpw[64];
	char title[24];

	int nerfme;
	int inuse;

	item_t			items[MAX_VRXITEMS];
	weapon_t		weapons[MAX_WEAPONS];
	upgrade_t		abilities[MAX_ABILITIES];

	talentlist_t	talents;
	prestigelist_t  prestige;
}skills_t;

#endif