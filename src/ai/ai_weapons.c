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

*/

#include "g_local.h"
#include "ai_local.h"


//WEAP_NONE,
//WEAP_BLASTER
//WEAP_SHOTGUN
//WEAP_SUPERSHOTGUN
//WEAP_MACHINEGUN
//WEAP_CHAINGUN
//WEAP_GRENADES
//WEAP_GRENADELAUNCHER
//WEAP_ROCKETLAUNCHER
//WEAP_HYPERBLASTER
//WEAP_RAILGUN
//WEAP_BFG
//WEAP_GRAPPLE

float get_weapon_grenade_speed(edict_t* ent);//GHz
float AI_GetWeaponProjectileVelocity(edict_t *ent, int weapmodelIndex)
{
	switch (weapmodelIndex)
	{
	case WEAP_SWORD:
		if (ent->client->weapon_mode)
		{
			float sword_bonus = 1.0;
			// calculate knight bonus
			if (ent->myskills.class_num == CLASS_KNIGHT)
				sword_bonus = 1.5;
			return 850 + (15 * ent->myskills.weapons[WEAPON_SWORD].mods[2].current_level * sword_bonus);
		}
		return 0;
	case WEAP_BLASTER:
		return BLASTER_INITIAL_SPEED + BLASTER_ADDON_SPEED * ent->myskills.weapons[WEAPON_BLASTER].mods[2].current_level;
	case WEAP_HYPERBLASTER:
		return HYPERBLASTER_INITIAL_SPEED + HYPERBLASTER_ADDON_SPEED * ent->myskills.weapons[WEAPON_HYPERBLASTER].mods[2].current_level;
	case WEAP_ROCKETLAUNCHER:
		return ROCKETLAUNCHER_INITIAL_SPEED + ROCKETLAUNCHER_ADDON_SPEED * ent->myskills.weapons[WEAPON_ROCKETLAUNCHER].mods[2].current_level;
	case WEAP_GRENADELAUNCHER:
		return GRENADELAUNCHER_INITIAL_SPEED + GRENADELAUNCHER_ADDON_SPEED * ent->myskills.weapons[WEAPON_GRENADELAUNCHER].mods[2].current_level;
	case WEAP_BFG:
		return BFG10K_INITIAL_SPEED + BFG10K_ADDON_SPEED * ent->myskills.weapons[WEAPON_BFG10K].mods[2].current_level;
	case WEAP_GRENADES:
		return get_weapon_grenade_speed(ent);
	}
	return 0;
}

float AI_GetWeaponRangeWeightByDistance(edict_t *self, int weapmodelIndex, float distance)
{
	//FIXME: these distances don't match up with those in ChooseWeapon
	if (distance <= AI_RANGE_MELEE)
	{
		if (self->mtype == MORPH_BERSERK)
			return 1.0;
		else if (weapmodelIndex != -1)
			return AIWeapons[weapmodelIndex].RangeWeight[AIWEAP_MELEE_RANGE];
	}
	else if (distance <= AI_RANGE_SHORT)
	{
		if (self->mtype == MORPH_BERSERK)
			return 0.2;
		else if (weapmodelIndex != -1)
			return AIWeapons[weapmodelIndex].RangeWeight[AIWEAP_SHORT_RANGE];
	}
	else if (distance <= AI_RANGE_MEDIUM)
	{
		if (weapmodelIndex != -1)
			return AIWeapons[weapmodelIndex].RangeWeight[AIWEAP_MEDIUM_RANGE];
	}
	else
	{
		if (weapmodelIndex != -1)
			return AIWeapons[weapmodelIndex].RangeWeight[AIWEAP_LONG_RANGE];
	}

	return 0; // unknown weapModelIndex
}

//==========================================
// AI_InitAIWeapons
// 
// AIWeapons are the way the AI uses to analize
// weapon types, for choosing and fire them
//==========================================
void AI_InitAIWeapons (void)
{
	//clear all
	memset( &AIWeapons, 0, sizeof(ai_weapon_t)*WEAP_TOTAL);

	//BLASTER
	AIWeapons[WEAP_BLASTER].aimType = AI_AIMSTYLE_PREDICTION;
	AIWeapons[WEAP_BLASTER].idealRange = AI_RANGE_SHORT;//GHz
	AIWeapons[WEAP_BLASTER].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.05;
	AIWeapons[WEAP_BLASTER].RangeWeight[AIWEAP_LONG_RANGE] = 0.05; //blaster must always have some value
	AIWeapons[WEAP_BLASTER].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.1;
	AIWeapons[WEAP_BLASTER].RangeWeight[AIWEAP_SHORT_RANGE] = 0.2;
	AIWeapons[WEAP_BLASTER].RangeWeight[AIWEAP_MELEE_RANGE] = 0.1;
	AIWeapons[WEAP_BLASTER].weaponItem = FindItemByClassname("weapon_blaster");
	AIWeapons[WEAP_BLASTER].ammoItem = NULL;		//doesn't use ammo

	//SWORD
	AIWeapons[WEAP_SWORD].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_SWORD].idealRange = AI_RANGE_MELEE;//GHz
	AIWeapons[WEAP_SWORD].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.05;
	AIWeapons[WEAP_SWORD].RangeWeight[AIWEAP_LONG_RANGE] = 0.1;
	AIWeapons[WEAP_SWORD].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.1;
	AIWeapons[WEAP_SWORD].RangeWeight[AIWEAP_SHORT_RANGE] = 0.8;
	AIWeapons[WEAP_SWORD].RangeWeight[AIWEAP_MELEE_RANGE] = 0.9;
	AIWeapons[WEAP_SWORD].weaponItem = FindItemByClassname("weapon_sword");
	AIWeapons[WEAP_SWORD].ammoItem = NULL;		//doesn't use ammo

	//20mm
	AIWeapons[WEAP_20MM].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_20MM].idealRange = AI_RANGE_MEDIUM;//GHz
	AIWeapons[WEAP_20MM].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.0;
	AIWeapons[WEAP_20MM].RangeWeight[AIWEAP_LONG_RANGE] = 0.6;
	AIWeapons[WEAP_20MM].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.7;
	AIWeapons[WEAP_20MM].RangeWeight[AIWEAP_SHORT_RANGE] = 0.6;
	AIWeapons[WEAP_20MM].RangeWeight[AIWEAP_MELEE_RANGE] = 0.3;
	AIWeapons[WEAP_20MM].weaponItem = FindItemByClassname("weapon_20mm");
	AIWeapons[WEAP_20MM].ammoItem = FindItemByClassname("ammo_shells");

	//SHOTGUN
	AIWeapons[WEAP_SHOTGUN].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_SHOTGUN].idealRange = AI_RANGE_MEDIUM;//GHz
	AIWeapons[WEAP_SHOTGUN].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.1;
	AIWeapons[WEAP_SHOTGUN].RangeWeight[AIWEAP_LONG_RANGE] = 0.3;
	AIWeapons[WEAP_SHOTGUN].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.4;
	AIWeapons[WEAP_SHOTGUN].RangeWeight[AIWEAP_SHORT_RANGE] = 0.3;
	AIWeapons[WEAP_SHOTGUN].RangeWeight[AIWEAP_MELEE_RANGE] = 0.1;
	AIWeapons[WEAP_SHOTGUN].weaponItem = Fdi_SHOTGUN;//FindItemByClassname("weapon_shotgun");
	AIWeapons[WEAP_SHOTGUN].ammoItem = Fdi_SHELLS;////FindItemByClassname("ammo_shells");
	
	//SUPERSHOTGUN
	AIWeapons[WEAP_SUPERSHOTGUN].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_SUPERSHOTGUN].idealRange = AI_RANGE_SHORT;//GHz
	AIWeapons[WEAP_SUPERSHOTGUN].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.1;
	AIWeapons[WEAP_SUPERSHOTGUN].RangeWeight[AIWEAP_LONG_RANGE] = 0.2;
	AIWeapons[WEAP_SUPERSHOTGUN].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.6;
	AIWeapons[WEAP_SUPERSHOTGUN].RangeWeight[AIWEAP_SHORT_RANGE] = 0.8;
	AIWeapons[WEAP_SUPERSHOTGUN].RangeWeight[AIWEAP_MELEE_RANGE] = 0.7;
	AIWeapons[WEAP_SUPERSHOTGUN].weaponItem = Fdi_SUPERSHOTGUN;// FindItemByClassname("weapon_supershotgun");
	AIWeapons[WEAP_SUPERSHOTGUN].ammoItem = Fdi_SHELLS;//FindItemByClassname("ammo_shells");

	//MACHINEGUN
	AIWeapons[WEAP_MACHINEGUN].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_MACHINEGUN].idealRange = AI_RANGE_MEDIUM;//GHz
	AIWeapons[WEAP_MACHINEGUN].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.3;
	AIWeapons[WEAP_MACHINEGUN].RangeWeight[AIWEAP_LONG_RANGE] = 0.3;
	AIWeapons[WEAP_MACHINEGUN].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.4;
	AIWeapons[WEAP_MACHINEGUN].RangeWeight[AIWEAP_SHORT_RANGE] = 0.3;
	AIWeapons[WEAP_MACHINEGUN].RangeWeight[AIWEAP_MELEE_RANGE] = 0.1;
	AIWeapons[WEAP_MACHINEGUN].weaponItem = Fdi_MACHINEGUN;//FindItemByClassname("weapon_machinegun");
	AIWeapons[WEAP_MACHINEGUN].ammoItem = Fdi_BULLETS;//FindItemByClassname("ammo_bullets");

	//CHAINGUN
	AIWeapons[WEAP_CHAINGUN].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_CHAINGUN].idealRange = AI_RANGE_MEDIUM;//GHz
	AIWeapons[WEAP_CHAINGUN].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.4;
	AIWeapons[WEAP_CHAINGUN].RangeWeight[AIWEAP_LONG_RANGE] = 0.6;
	AIWeapons[WEAP_CHAINGUN].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.7;
	AIWeapons[WEAP_CHAINGUN].RangeWeight[AIWEAP_SHORT_RANGE] = 0.7;
	AIWeapons[WEAP_CHAINGUN].RangeWeight[AIWEAP_MELEE_RANGE] = 0.4;
	AIWeapons[WEAP_CHAINGUN].weaponItem = Fdi_CHAINGUN;//FindItemByClassname("weapon_chaingun");
	AIWeapons[WEAP_CHAINGUN].ammoItem = Fdi_BULLETS;//FindItemByClassname("ammo_bullets");

	//GRENADES
	AIWeapons[WEAP_GRENADES].aimType = AI_AIMSTYLE_DROP;
	AIWeapons[WEAP_GRENADES].idealRange = AI_RANGE_SHORT;//GHz
	AIWeapons[WEAP_GRENADES].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.0;
	AIWeapons[WEAP_GRENADES].RangeWeight[AIWEAP_LONG_RANGE] = 0.0;
	AIWeapons[WEAP_GRENADES].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.2;
	AIWeapons[WEAP_GRENADES].RangeWeight[AIWEAP_SHORT_RANGE] = 0.3;
	AIWeapons[WEAP_GRENADES].RangeWeight[AIWEAP_MELEE_RANGE] = 0.2;
	AIWeapons[WEAP_GRENADES].weaponItem = Fdi_GRENADES;//FindItemByClassname("ammo_grenades");
	AIWeapons[WEAP_GRENADES].ammoItem = Fdi_GRENADES;//FindItemByClassname("ammo_grenades");

	//GRENADELAUNCHER
	AIWeapons[WEAP_GRENADELAUNCHER].aimType = AI_AIMSTYLE_PREDICTION;
	AIWeapons[WEAP_GRENADELAUNCHER].idealRange = AI_RANGE_MEDIUM;//GHz
	AIWeapons[WEAP_GRENADELAUNCHER].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.0;
	AIWeapons[WEAP_GRENADELAUNCHER].RangeWeight[AIWEAP_LONG_RANGE] = 0.4;
	AIWeapons[WEAP_GRENADELAUNCHER].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.5;
	AIWeapons[WEAP_GRENADELAUNCHER].RangeWeight[AIWEAP_SHORT_RANGE] = 0.4;
	AIWeapons[WEAP_GRENADELAUNCHER].RangeWeight[AIWEAP_MELEE_RANGE] = 0.3;
	AIWeapons[WEAP_GRENADELAUNCHER].weaponItem = Fdi_GRENADELAUNCHER;//FindItemByClassname("weapon_grenadelauncher");
	AIWeapons[WEAP_GRENADELAUNCHER].ammoItem = Fdi_GRENADES;//FindItemByClassname("ammo_grenades");

	//ROCKETLAUNCHER
	AIWeapons[WEAP_ROCKETLAUNCHER].aimType = AI_AIMSTYLE_PREDICTION_EXPLOSIVE;
	AIWeapons[WEAP_ROCKETLAUNCHER].idealRange = AI_RANGE_MEDIUM;//GHz
	AIWeapons[WEAP_ROCKETLAUNCHER].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.1;
	AIWeapons[WEAP_ROCKETLAUNCHER].RangeWeight[AIWEAP_LONG_RANGE] = 0.5;
	AIWeapons[WEAP_ROCKETLAUNCHER].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.9;
	AIWeapons[WEAP_ROCKETLAUNCHER].RangeWeight[AIWEAP_SHORT_RANGE] = 0.7;
	AIWeapons[WEAP_ROCKETLAUNCHER].RangeWeight[AIWEAP_MELEE_RANGE] = 0.4;
	AIWeapons[WEAP_ROCKETLAUNCHER].weaponItem = Fdi_ROCKETLAUNCHER;//FindItemByClassname("weapon_rocketlauncher");
	AIWeapons[WEAP_ROCKETLAUNCHER].ammoItem = Fdi_ROCKETS;//FindItemByClassname("ammo_rockets");

	//WEAP_HYPERBLASTER
	AIWeapons[WEAP_HYPERBLASTER].aimType = AI_AIMSTYLE_PREDICTION;
	AIWeapons[WEAP_HYPERBLASTER].idealRange = AI_RANGE_MEDIUM;//GHz
	AIWeapons[WEAP_HYPERBLASTER].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.1;
	AIWeapons[WEAP_HYPERBLASTER].RangeWeight[AIWEAP_LONG_RANGE] = 0.5;
	AIWeapons[WEAP_HYPERBLASTER].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.6;
	AIWeapons[WEAP_HYPERBLASTER].RangeWeight[AIWEAP_SHORT_RANGE] = 0.5;
	AIWeapons[WEAP_HYPERBLASTER].RangeWeight[AIWEAP_MELEE_RANGE] = 0.4;
	AIWeapons[WEAP_HYPERBLASTER].weaponItem = Fdi_HYPERBLASTER;//FindItemByClassname("weapon_hyperblaster");
	AIWeapons[WEAP_HYPERBLASTER].ammoItem = Fdi_CELLS;//FindItemByClassname("ammo_cells");

	//WEAP_RAILGUN
	AIWeapons[WEAP_RAILGUN].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_RAILGUN].idealRange = AI_RANGE_LONG;//GHz
	AIWeapons[WEAP_RAILGUN].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.9;
	AIWeapons[WEAP_RAILGUN].RangeWeight[AIWEAP_LONG_RANGE] = 0.9;
	AIWeapons[WEAP_RAILGUN].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.6; // RL is best, 20mm and CG are better
	AIWeapons[WEAP_RAILGUN].RangeWeight[AIWEAP_SHORT_RANGE] = 0.4;
	AIWeapons[WEAP_RAILGUN].RangeWeight[AIWEAP_MELEE_RANGE] = 0.3;
	AIWeapons[WEAP_RAILGUN].weaponItem = Fdi_RAILGUN;//FindItemByClassname("weapon_railgun");
	AIWeapons[WEAP_RAILGUN].ammoItem = Fdi_SLUGS;//FindItemByClassname("ammo_slugs");

	//WEAP_BFG
	AIWeapons[WEAP_BFG].aimType = AI_AIMSTYLE_PREDICTION_EXPLOSIVE;
	AIWeapons[WEAP_BFG].idealRange = AI_RANGE_MEDIUM;//GHz
	AIWeapons[WEAP_BFG].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.1;
	AIWeapons[WEAP_BFG].RangeWeight[AIWEAP_LONG_RANGE] = 0.5;
	AIWeapons[WEAP_BFG].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.6;
	AIWeapons[WEAP_BFG].RangeWeight[AIWEAP_SHORT_RANGE] = 0.2;
	AIWeapons[WEAP_BFG].RangeWeight[AIWEAP_MELEE_RANGE] = 0.1;
	AIWeapons[WEAP_BFG].weaponItem = Fdi_BFG;//FindItemByClassname("weapon_bfg");
	AIWeapons[WEAP_BFG].ammoItem = Fdi_CELLS;//FindItemByClassname("ammo_cells");

	//WEAP_GRAPPLE
	AIWeapons[WEAP_GRAPPLE].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_GRAPPLE].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.0;
	AIWeapons[WEAP_GRAPPLE].RangeWeight[AIWEAP_LONG_RANGE] = 0.0; //grapple is not used for attacks
	AIWeapons[WEAP_GRAPPLE].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.0;
	AIWeapons[WEAP_GRAPPLE].RangeWeight[AIWEAP_SHORT_RANGE] = 0.0;
	AIWeapons[WEAP_GRAPPLE].RangeWeight[AIWEAP_MELEE_RANGE] = 0.0;
	AIWeapons[WEAP_GRAPPLE].weaponItem = Fdi_GRAPPLE;//FindItemByClassname("weapon_grapplinghook");
	AIWeapons[WEAP_GRAPPLE].ammoItem = NULL;		//doesn't use ammo

}









