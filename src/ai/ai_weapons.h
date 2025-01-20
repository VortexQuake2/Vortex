/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
--------------------------------------------------------------
The ACE Bot is a product of Steve Yeager, and is available from
the ACE Bot homepage, at http://www.axionfx.com/ace.

This program is a modification of the ACE Bot, and is therefore
in NO WAY supported by Steve Yeager.
*/

//GHz: define ideal combat ranges for various weapons
#define AI_RANGE_MELEE	64
#define AI_RANGE_SHORT	128
#define AI_RANGE_MEDIUM	256
#define AI_RANGE_LONG	512
#define AI_RANGE_SNIPER	1024

enum
{
	AI_AIMSTYLE_NONE,
	AI_AIMSTYLE_INSTANTHIT,
	AI_AIMSTYLE_PREDICTION,
	AI_AIMSTYLE_PREDICTION_EXPLOSIVE,
	AI_AIMSTYLE_BALLISTIC,
	AI_AIMSTYLE_DROP,
	
	AIWEAP_AIM_TYPES
};

enum
{
	AIWEAP_MELEE_RANGE,
	AIWEAP_SHORT_RANGE,
	AIWEAP_MEDIUM_RANGE,
	AIWEAP_LONG_RANGE,
	AIWEAP_SNIPER_RANGE,
	
	AIWEAP_RANGES
};

typedef struct
{
	int		aimType;
	float	idealRange;							//GHz: the ideal combat range for this weapon (FIXME: this could be calculated by rangeweights...)
	float	RangeWeight[AIWEAP_RANGES];

	gitem_t	*weaponItem;
	gitem_t	*ammoItem;

} ai_weapon_t;

extern ai_weapon_t		AIWeapons[WEAP_TOTAL];

typedef struct
{
	int			aimType;
	float		idealRange;
	float		RangeWeight[AIWEAP_RANGES];
	qboolean	is_weapon;						// true if ability can be fired like a weapon

} ai_ability_t;

extern ai_ability_t		AIAbilities[MAX_ABILITIES];

