#ifndef TALENTS_H
#define TALENTS_H

//Talent ID numbers. The TALENT_ prefix is for "Vortex Talent".
//It is only there to make the talents easy to select using intellisense.
//Soldier
#define TALENT_BOMBARDIER			10
#define TALENT_IMP_STRENGTH			11

#define TALENT_IMP_RESIST			12
#define TALENT_BLOOD_OF_ARES		13
#define TALENT_BASIC_HA				14
//#define TALENT_ENDURANCE			124

#define TALENT_MONSTER_MASTERY		125
//Poltergeist
#define TALENT_MORE_AMMO			20
#define TALENT_SUPERIORITY			21
#define TALENT_RANGE_MASTERY		22 //TALENT_RETALIATION
#define TALENT_PACK_ANIMAL			23
#define TALENT_MELEE_MASTERY		24 //TALENT_MORPHING
//Vampire
#define TALENT_FATAL_WOUND			30
#define TALENT_IMP_CLOAK			31
#define TALENT_ARMOR_VAMP			32
#define TALENT_SECOND_CHANCE		33
#define TALENT_IMP_MINDABSORB		34
#define TALENT_CANNIBALISM			35
//Mage
#define TALENT_METEORIC_FIRE		40 //TALENT_ICE_BOLT
#define TALENT_MEDITATION			41
#define TALENT_OVERLOAD				42
#define TALENT_CL_STORM				43 //TALENT_FROST_NOVA
#define TALENT_WIZARDRY				44 //TALENT_IMP_MAGICBOLT
#define TALENT_MANASHIELD			45
#define TALENT_NOVA_ORB				46

//Engineer
//#define TALENT_DEFENSIVE_CRATE     350
#define TALENT_LASER_PLATFORM		50
#define TALENT_ALARM				51
#define TALENT_RAPID_ASSEMBLY		52
#define TALENT_PRECISION_TUNING		53
#define TALENT_STORAGE_UPGRADE		54
#define TALENT_ARMOR_REG            124
//Knight
#define TALENT_REPEL				60
#define TALENT_MAG_BOOTS			61
#define TALENT_LEAP_ATTACK			62
#define TALENT_MOBILITY				63
#define TALENT_DURABILITY			64
//Cleric
#define TALENT_HOLY_GROUND			70
#define TALENT_UNHOLY_GROUND		71
#define TALENT_PURGE				72
#define TALENT_BOOMERANG			73
#define TALENT_BALANCESPIRIT		74
//Necromancer
#define TALENT_HELLSPAWN_MASTERY	80
#define TALENT_CORPULENCE			81
#define TALENT_OBLATION				82
#define TALENT_AUTOCURSE			83
#define TALENT_BLACK_DEATH			84
//#define TALENT_EVIL_CURSE			85
#define TALENT_GOLEM_MASTERY		85
//Shaman
#define TALENT_WIND					90
#define TALENT_STONE				91
#define TALENT_SHADOW				92
#define TALENT_ICE					93
#define TALENT_PEACE				94
#define TALENT_TOTEM				95
#define TALENT_VOLCANIC				96
//Alien
//#define TALENT_FASTMOVE				130
#define TALENT_MAGNETISM			100
#define TALENT_SWARMING				101
#define TALENT_DEADLY_SPIKES		102
#define TALENT_SPITTING_GASSER		103
#define TALENT_SUPER_HEALER			104
#define TALENT_LIFE_REG				135
//Weapon Master
#define TALENT_BASIC_AMMO_REGEN		110
#define TALENT_COMBAT_EXP			111
#define TALENT_TACTICS				112
#define TALENT_SIDEARMS				113
//Kamikaze
#define TALENT_MARTYR			    120
#define TALENT_BLAST_RESIST			121
#define TALENT_MAGMINESELF			122
#define TALENT_INSTANTPROXYS		123

typedef struct talent_s {
	int		id;
	int		upgradeLevel;
	int		maxLevel;
	float	delay;
}
talent_t;

typedef struct talentlist_s {
    int count;
    int talentPoints;
    talent_t talent[MAX_TALENTS];
} talentlist_t;

/***
* fix differences between class talents and player talents.
* @param ent player to update talents of
*/
void V_UpdatePlayerTalents(edict_t *ent);

#endif