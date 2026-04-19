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


//WEAPON_NONE,
//WEAPON_BLASTER
//WEAPON_SHOTGUN
//WEAPON_SUPERSHOTGUN
//WEAPON_MACHINEGUN
//WEAPON_CHAINGUN
//WEAPON_GRENADES
//WEAPON_GRENADELAUNCHER
//WEAPON_ROCKETLAUNCHER
//WEAPON_HYPERBLASTER
//WEAPON_RAILGUN
//WEAPON_BFG10K
//WEAPON_GRAPPLE

float get_weapon_grenade_speed(edict_t* ent);//GHz
float AI_GetWeaponProjectileVelocity(edict_t *ent, int weapmodelIndex)
{
	switch (weapmodelIndex)
	{
	case WEAPON_SWORD:
		if (ent->client->weapon_mode)
		{
			float sword_bonus = 1.0;
			// calculate knight bonus
			if (ent->myskills.class_num == CLASS_KNIGHT)
				sword_bonus = 1.5;
			return 850 + (15 * ent->myskills.weapons[WEAPON_SWORD].mods[2].current_level * sword_bonus);
		}
		return 0;
	case WEAPON_BLASTER:
		return BLASTER_INITIAL_SPEED + BLASTER_ADDON_SPEED * ent->myskills.weapons[WEAPON_BLASTER].mods[2].current_level;
	case WEAPON_HYPERBLASTER:
		return HYPERBLASTER_INITIAL_SPEED + HYPERBLASTER_ADDON_SPEED * ent->myskills.weapons[WEAPON_HYPERBLASTER].mods[2].current_level;
	case WEAPON_ROCKETLAUNCHER:
		return ROCKETLAUNCHER_INITIAL_SPEED + ROCKETLAUNCHER_ADDON_SPEED * ent->myskills.weapons[WEAPON_ROCKETLAUNCHER].mods[2].current_level;
	case WEAPON_GRENADELAUNCHER:
		return GRENADELAUNCHER_INITIAL_SPEED + GRENADELAUNCHER_ADDON_SPEED * ent->myskills.weapons[WEAPON_GRENADELAUNCHER].mods[2].current_level;
	case WEAPON_BFG10K:
		return BFG10K_INITIAL_SPEED + BFG10K_ADDON_SPEED * ent->myskills.weapons[WEAPON_BFG10K].mods[2].current_level;
	case WEAPON_HANDGRENADE:
		return get_weapon_grenade_speed(ent);
	case WEAPON_ETFRIFLE:
		return ETFRIFLE_INITIAL_SPEED + (ETFRIFLE_ADDON_SPEED * ent->myskills.weapons[WEAPON_ETFRIFLE].mods[2].current_level);
	case WEAPON_PHALANX:
		return PHALANX_INITIAL_SPEED + (PHALANX_ADDON_SPEED * ent->myskills.weapons[WEAPON_PHALANX].mods[2].current_level);
	case WEAPON_IONRIPPER:
		return IONRIPPER_INITIAL_SPEED + (IONRIPPER_ADDON_SPEED * ent->myskills.weapons[WEAPON_IONRIPPER].mods[2].current_level);
	case WEAPON_DISRUPTOR:
		return DISRUPTOR_INITIAL_SPEED + (DISRUPTOR_ADDON_SPEED * ent->myskills.weapons[WEAPON_DISRUPTOR].mods[2].current_level);
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
	memset( &AIWeapons, 0, sizeof(ai_weapon_t)*WEAPON_TOTAL);

	//BLASTER
	AIWeapons[WEAPON_BLASTER].aimType = AI_AIMSTYLE_PREDICTION;
	AIWeapons[WEAPON_BLASTER].idealRange = AI_RANGE_SHORT;//GHz
	AIWeapons[WEAPON_BLASTER].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.05;
	AIWeapons[WEAPON_BLASTER].RangeWeight[AIWEAP_LONG_RANGE] = 0.05; //blaster must always have some value
	AIWeapons[WEAPON_BLASTER].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.1;
	AIWeapons[WEAPON_BLASTER].RangeWeight[AIWEAP_SHORT_RANGE] = 0.2;
	AIWeapons[WEAPON_BLASTER].RangeWeight[AIWEAP_MELEE_RANGE] = 0.1;
	AIWeapons[WEAPON_BLASTER].weaponItem = FindItemByClassname("weapon_blaster");
	AIWeapons[WEAPON_BLASTER].ammoItem = NULL;		//doesn't use ammo

	//SWORD
	AIWeapons[WEAPON_SWORD].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAPON_SWORD].idealRange = AI_RANGE_MELEE;//GHz
	AIWeapons[WEAPON_SWORD].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.05;
	AIWeapons[WEAPON_SWORD].RangeWeight[AIWEAP_LONG_RANGE] = 0.1;
	AIWeapons[WEAPON_SWORD].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.1;
	AIWeapons[WEAPON_SWORD].RangeWeight[AIWEAP_SHORT_RANGE] = 0.8;
	AIWeapons[WEAPON_SWORD].RangeWeight[AIWEAP_MELEE_RANGE] = 0.9;
	AIWeapons[WEAPON_SWORD].weaponItem = FindItemByClassname("weapon_sword");
	AIWeapons[WEAPON_SWORD].ammoItem = NULL;		//doesn't use ammo

	//20mm
	AIWeapons[WEAPON_20MM].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAPON_20MM].idealRange = AI_RANGE_MEDIUM;//GHz
	AIWeapons[WEAPON_20MM].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.0;
	AIWeapons[WEAPON_20MM].RangeWeight[AIWEAP_LONG_RANGE] = 0.6;
	AIWeapons[WEAPON_20MM].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.7;
	AIWeapons[WEAPON_20MM].RangeWeight[AIWEAP_SHORT_RANGE] = 0.6;
	AIWeapons[WEAPON_20MM].RangeWeight[AIWEAP_MELEE_RANGE] = 0.3;
	AIWeapons[WEAPON_20MM].weaponItem = FindItemByClassname("weapon_20mm");
	AIWeapons[WEAPON_20MM].ammoItem = FindItemByClassname("ammo_shells");

	//SHOTGUN
	AIWeapons[WEAPON_SHOTGUN].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAPON_SHOTGUN].idealRange = AI_RANGE_MEDIUM;//GHz
	AIWeapons[WEAPON_SHOTGUN].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.1;
	AIWeapons[WEAPON_SHOTGUN].RangeWeight[AIWEAP_LONG_RANGE] = 0.3;
	AIWeapons[WEAPON_SHOTGUN].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.4;
	AIWeapons[WEAPON_SHOTGUN].RangeWeight[AIWEAP_SHORT_RANGE] = 0.3;
	AIWeapons[WEAPON_SHOTGUN].RangeWeight[AIWEAP_MELEE_RANGE] = 0.1;
	AIWeapons[WEAPON_SHOTGUN].weaponItem = Fdi_SHOTGUN;//FindItemByClassname("weapon_shotgun");
	AIWeapons[WEAPON_SHOTGUN].ammoItem = Fdi_SHELLS;////FindItemByClassname("ammo_shells");
	
	//SUPERSHOTGUN
	AIWeapons[WEAPON_SUPERSHOTGUN].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAPON_SUPERSHOTGUN].idealRange = AI_RANGE_SHORT;//GHz
	AIWeapons[WEAPON_SUPERSHOTGUN].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.1;
	AIWeapons[WEAPON_SUPERSHOTGUN].RangeWeight[AIWEAP_LONG_RANGE] = 0.2;
	AIWeapons[WEAPON_SUPERSHOTGUN].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.6;
	AIWeapons[WEAPON_SUPERSHOTGUN].RangeWeight[AIWEAP_SHORT_RANGE] = 0.8;
	AIWeapons[WEAPON_SUPERSHOTGUN].RangeWeight[AIWEAP_MELEE_RANGE] = 0.7;
	AIWeapons[WEAPON_SUPERSHOTGUN].weaponItem = Fdi_SUPERSHOTGUN;// FindItemByClassname("weapon_supershotgun");
	AIWeapons[WEAPON_SUPERSHOTGUN].ammoItem = Fdi_SHELLS;//FindItemByClassname("ammo_shells");

	//MACHINEGUN
	AIWeapons[WEAPON_MACHINEGUN].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAPON_MACHINEGUN].idealRange = AI_RANGE_MEDIUM;//GHz
	AIWeapons[WEAPON_MACHINEGUN].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.3;
	AIWeapons[WEAPON_MACHINEGUN].RangeWeight[AIWEAP_LONG_RANGE] = 0.3;
	AIWeapons[WEAPON_MACHINEGUN].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.4;
	AIWeapons[WEAPON_MACHINEGUN].RangeWeight[AIWEAP_SHORT_RANGE] = 0.3;
	AIWeapons[WEAPON_MACHINEGUN].RangeWeight[AIWEAP_MELEE_RANGE] = 0.1;
	AIWeapons[WEAPON_MACHINEGUN].weaponItem = Fdi_MACHINEGUN;//FindItemByClassname("weapon_machinegun");
	AIWeapons[WEAPON_MACHINEGUN].ammoItem = Fdi_BULLETS;//FindItemByClassname("ammo_bullets");

	//CHAINGUN
	AIWeapons[WEAPON_CHAINGUN].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAPON_CHAINGUN].idealRange = AI_RANGE_MEDIUM;//GHz
	AIWeapons[WEAPON_CHAINGUN].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.4;
	AIWeapons[WEAPON_CHAINGUN].RangeWeight[AIWEAP_LONG_RANGE] = 0.6;
	AIWeapons[WEAPON_CHAINGUN].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.7;
	AIWeapons[WEAPON_CHAINGUN].RangeWeight[AIWEAP_SHORT_RANGE] = 0.7;
	AIWeapons[WEAPON_CHAINGUN].RangeWeight[AIWEAP_MELEE_RANGE] = 0.4;
	AIWeapons[WEAPON_CHAINGUN].weaponItem = Fdi_CHAINGUN;//FindItemByClassname("weapon_chaingun");
	AIWeapons[WEAPON_CHAINGUN].ammoItem = Fdi_BULLETS;//FindItemByClassname("ammo_bullets");

	//GRENADES
	AIWeapons[WEAPON_HANDGRENADE].aimType = AI_AIMSTYLE_DROP;
	AIWeapons[WEAPON_HANDGRENADE].idealRange = AI_RANGE_SHORT;//GHz
	AIWeapons[WEAPON_HANDGRENADE].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.0;
	AIWeapons[WEAPON_HANDGRENADE].RangeWeight[AIWEAP_LONG_RANGE] = 0.0;
	AIWeapons[WEAPON_HANDGRENADE].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.2;
	AIWeapons[WEAPON_HANDGRENADE].RangeWeight[AIWEAP_SHORT_RANGE] = 0.3;
	AIWeapons[WEAPON_HANDGRENADE].RangeWeight[AIWEAP_MELEE_RANGE] = 0.2;
	AIWeapons[WEAPON_HANDGRENADE].weaponItem = Fdi_GRENADES;//FindItemByClassname("ammo_grenades");
	AIWeapons[WEAPON_HANDGRENADE].ammoItem = Fdi_GRENADES;//FindItemByClassname("ammo_grenades");

	//GRENADELAUNCHER
	AIWeapons[WEAPON_GRENADELAUNCHER].aimType = AI_AIMSTYLE_PREDICTION;
	AIWeapons[WEAPON_GRENADELAUNCHER].idealRange = AI_RANGE_MEDIUM;//GHz
	AIWeapons[WEAPON_GRENADELAUNCHER].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.0;
	AIWeapons[WEAPON_GRENADELAUNCHER].RangeWeight[AIWEAP_LONG_RANGE] = 0.4;
	AIWeapons[WEAPON_GRENADELAUNCHER].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.5;
	AIWeapons[WEAPON_GRENADELAUNCHER].RangeWeight[AIWEAP_SHORT_RANGE] = 0.4;
	AIWeapons[WEAPON_GRENADELAUNCHER].RangeWeight[AIWEAP_MELEE_RANGE] = 0.3;
	AIWeapons[WEAPON_GRENADELAUNCHER].weaponItem = Fdi_GRENADELAUNCHER;//FindItemByClassname("weapon_grenadelauncher");
	AIWeapons[WEAPON_GRENADELAUNCHER].ammoItem = Fdi_GRENADES;//FindItemByClassname("ammo_grenades");

	//ROCKETLAUNCHER
	AIWeapons[WEAPON_ROCKETLAUNCHER].aimType = AI_AIMSTYLE_PREDICTION_EXPLOSIVE;
	AIWeapons[WEAPON_ROCKETLAUNCHER].idealRange = AI_RANGE_MEDIUM;//GHz
	AIWeapons[WEAPON_ROCKETLAUNCHER].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.1;
	AIWeapons[WEAPON_ROCKETLAUNCHER].RangeWeight[AIWEAP_LONG_RANGE] = 0.5;
	AIWeapons[WEAPON_ROCKETLAUNCHER].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.9;
	AIWeapons[WEAPON_ROCKETLAUNCHER].RangeWeight[AIWEAP_SHORT_RANGE] = 0.7;
	AIWeapons[WEAPON_ROCKETLAUNCHER].RangeWeight[AIWEAP_MELEE_RANGE] = 0.4;
	AIWeapons[WEAPON_ROCKETLAUNCHER].weaponItem = Fdi_ROCKETLAUNCHER;//FindItemByClassname("weapon_rocketlauncher");
	AIWeapons[WEAPON_ROCKETLAUNCHER].ammoItem = Fdi_ROCKETS;//FindItemByClassname("ammo_rockets");

	//WEAP_HYPERBLASTER
	AIWeapons[WEAPON_HYPERBLASTER].aimType = AI_AIMSTYLE_PREDICTION;
	AIWeapons[WEAPON_HYPERBLASTER].idealRange = AI_RANGE_MEDIUM;//GHz
	AIWeapons[WEAPON_HYPERBLASTER].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.1;
	AIWeapons[WEAPON_HYPERBLASTER].RangeWeight[AIWEAP_LONG_RANGE] = 0.5;
	AIWeapons[WEAPON_HYPERBLASTER].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.6;
	AIWeapons[WEAPON_HYPERBLASTER].RangeWeight[AIWEAP_SHORT_RANGE] = 0.5;
	AIWeapons[WEAPON_HYPERBLASTER].RangeWeight[AIWEAP_MELEE_RANGE] = 0.4;
	AIWeapons[WEAPON_HYPERBLASTER].weaponItem = Fdi_HYPERBLASTER;//FindItemByClassname("weapon_hyperblaster");
	AIWeapons[WEAPON_HYPERBLASTER].ammoItem = Fdi_CELLS;//FindItemByClassname("ammo_cells");

	//WEAP_RAILGUN
	AIWeapons[WEAPON_RAILGUN].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAPON_RAILGUN].idealRange = AI_RANGE_LONG;//GHz
	AIWeapons[WEAPON_RAILGUN].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.9;
	AIWeapons[WEAPON_RAILGUN].RangeWeight[AIWEAP_LONG_RANGE] = 0.9;
	AIWeapons[WEAPON_RAILGUN].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.6; // RL is best, 20mm and CG are better
	AIWeapons[WEAPON_RAILGUN].RangeWeight[AIWEAP_SHORT_RANGE] = 0.4;
	AIWeapons[WEAPON_RAILGUN].RangeWeight[AIWEAP_MELEE_RANGE] = 0.3;
	AIWeapons[WEAPON_RAILGUN].weaponItem = Fdi_RAILGUN;//FindItemByClassname("weapon_railgun");
	AIWeapons[WEAPON_RAILGUN].ammoItem = Fdi_SLUGS;//FindItemByClassname("ammo_slugs");

	//WEAP_BFG
	AIWeapons[WEAPON_BFG10K].aimType = AI_AIMSTYLE_PREDICTION_EXPLOSIVE;
	AIWeapons[WEAPON_BFG10K].idealRange = AI_RANGE_MEDIUM;//GHz
	AIWeapons[WEAPON_BFG10K].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.1;
	AIWeapons[WEAPON_BFG10K].RangeWeight[AIWEAP_LONG_RANGE] = 0.5;
	AIWeapons[WEAPON_BFG10K].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.6;
	AIWeapons[WEAPON_BFG10K].RangeWeight[AIWEAP_SHORT_RANGE] = 0.2;
	AIWeapons[WEAPON_BFG10K].RangeWeight[AIWEAP_MELEE_RANGE] = 0.1;
	AIWeapons[WEAPON_BFG10K].weaponItem = Fdi_BFG;//FindItemByClassname("weapon_bfg");
	AIWeapons[WEAPON_BFG10K].ammoItem = Fdi_CELLS;//FindItemByClassname("ammo_cells");

	//WEAP_GRAPPLE
	AIWeapons[WEAPON_GRAPPLE].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAPON_GRAPPLE].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.0;
	AIWeapons[WEAPON_GRAPPLE].RangeWeight[AIWEAP_LONG_RANGE] = 0.0; //grapple is not used for attacks
	AIWeapons[WEAPON_GRAPPLE].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.0;
	AIWeapons[WEAPON_GRAPPLE].RangeWeight[AIWEAP_SHORT_RANGE] = 0.0;
	AIWeapons[WEAPON_GRAPPLE].RangeWeight[AIWEAP_MELEE_RANGE] = 0.0;
	AIWeapons[WEAPON_GRAPPLE].weaponItem = Fdi_GRAPPLE;//FindItemByClassname("weapon_grapplinghook");
	AIWeapons[WEAPON_GRAPPLE].ammoItem = NULL;		//doesn't use ammo


#ifdef VRX_REPRO

	//WEAP_ETFRIFLE
	AIWeapons[WEAPON_ETFRIFLE].aimType = AI_AIMSTYLE_PREDICTION;
	AIWeapons[WEAPON_ETFRIFLE].idealRange = AI_RANGE_MEDIUM;//GHz
	AIWeapons[WEAPON_ETFRIFLE].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.1;
	AIWeapons[WEAPON_ETFRIFLE].RangeWeight[AIWEAP_LONG_RANGE] = 0.2;
	AIWeapons[WEAPON_ETFRIFLE].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.5;
	AIWeapons[WEAPON_ETFRIFLE].RangeWeight[AIWEAP_SHORT_RANGE] = 0.5;
	AIWeapons[WEAPON_ETFRIFLE].RangeWeight[AIWEAP_MELEE_RANGE] = 0.2;
	AIWeapons[WEAPON_ETFRIFLE].weaponItem = Fdi_ETFRIFLE;//FindItemByClassname("weapon_hyperblaster");
	AIWeapons[WEAPON_ETFRIFLE].ammoItem = Fdi_FLECHETTES;//FindItemByClassname("ammo_flechettes");

	//WEAP_PHALANX
	AIWeapons[WEAPON_PHALANX].aimType = AI_AIMSTYLE_PREDICTION;
	AIWeapons[WEAPON_PHALANX].idealRange = AI_RANGE_MEDIUM;//GHz
	AIWeapons[WEAPON_PHALANX].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.1;
	AIWeapons[WEAPON_PHALANX].RangeWeight[AIWEAP_LONG_RANGE] = 0.5;
	AIWeapons[WEAPON_PHALANX].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.6;
	AIWeapons[WEAPON_PHALANX].RangeWeight[AIWEAP_SHORT_RANGE] = 0.2;
	AIWeapons[WEAPON_PHALANX].RangeWeight[AIWEAP_MELEE_RANGE] = 0.1;
	AIWeapons[WEAPON_PHALANX].weaponItem = Fdi_PHALANX;//FindItemByClassname("weapon_phalanx");
	AIWeapons[WEAPON_PHALANX].ammoItem = Fdi_MAGSLUG;//FindItemByClassname("ammo_magslug");

	//WEAP_DISRUPTOR
	AIWeapons[WEAPON_DISRUPTOR].aimType = AI_AIMSTYLE_PREDICTION;
	AIWeapons[WEAPON_DISRUPTOR].idealRange = AI_RANGE_MEDIUM;//GHz
	AIWeapons[WEAPON_DISRUPTOR].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.1;
	AIWeapons[WEAPON_DISRUPTOR].RangeWeight[AIWEAP_LONG_RANGE] = 0.3;
	AIWeapons[WEAPON_DISRUPTOR].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.8;
	AIWeapons[WEAPON_DISRUPTOR].RangeWeight[AIWEAP_SHORT_RANGE] = 0.6;
	AIWeapons[WEAPON_DISRUPTOR].RangeWeight[AIWEAP_MELEE_RANGE] = 0.4;
	AIWeapons[WEAPON_DISRUPTOR].weaponItem = Fdi_DISRUPTOR;//FindItemByClassname("weapon_hyperblaster");
	AIWeapons[WEAPON_DISRUPTOR].ammoItem = Fdi_ROUNDS;//FindItemByClassname("ammo_flechettes");

	//WEAP_BOOMER
	AIWeapons[WEAPON_IONRIPPER].aimType = AI_AIMSTYLE_PREDICTION;
	AIWeapons[WEAPON_IONRIPPER].idealRange = AI_RANGE_MEDIUM;//GHz
	AIWeapons[WEAPON_IONRIPPER].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.1;
	AIWeapons[WEAPON_IONRIPPER].RangeWeight[AIWEAP_LONG_RANGE] = 0.3;
	AIWeapons[WEAPON_IONRIPPER].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.8;
	AIWeapons[WEAPON_IONRIPPER].RangeWeight[AIWEAP_SHORT_RANGE] = 0.6;
	AIWeapons[WEAPON_IONRIPPER].RangeWeight[AIWEAP_MELEE_RANGE] = 0.4;
	AIWeapons[WEAPON_IONRIPPER].weaponItem = Fdi_IONRIPPER;//FindItemByClassname("weapon_ionripper");
	AIWeapons[WEAPON_IONRIPPER].ammoItem = Fdi_CELLS;//FindItemByClassname("ammo_cells");

	// WEAP_PLASMA
	AIWeapons[WEAPON_PLASMABEAM].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAPON_PLASMABEAM].idealRange = AI_RANGE_MEDIUM;//GHz
	AIWeapons[WEAPON_PLASMABEAM].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.1;
	AIWeapons[WEAPON_PLASMABEAM].RangeWeight[AIWEAP_LONG_RANGE] = 0.6;
	AIWeapons[WEAPON_PLASMABEAM].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.7;
	AIWeapons[WEAPON_PLASMABEAM].RangeWeight[AIWEAP_SHORT_RANGE] = 0.7;
	AIWeapons[WEAPON_PLASMABEAM].RangeWeight[AIWEAP_MELEE_RANGE] = 0.4;
	AIWeapons[WEAPON_PLASMABEAM].weaponItem = Fdi_PLASMA;//FindItemByClassname("weapon_chaingun");
	AIWeapons[WEAPON_PLASMABEAM].ammoItem = Fdi_CELLS;//FindItemByClassname("ammo_bullets");

#endif

}









