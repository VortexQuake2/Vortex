#include "g_local.h"

//***********************************************************
//		Weapon Master class bonus
//***********************************************************
qboolean GiveWeaponMasterUpgrade(edict_t *ent, int WeaponIndex, int ModIndex)
{
	weapon_t *weapon;
	int maxLevel = 40;		//All hard maximums for the weapon master are set to this number.

	if (generalabmode->value == 0) // No weapon master bonuses in non-general ab mode.
		return false;

	if (generalabmode->value && ent->myskills.class_num != CLASS_WEAPONMASTER)
	    return false;

	//Point to the correct weapon
	weapon = &ent->myskills.weapons[WeaponIndex];

	//Don't crash
	if (ModIndex < 0 || ModIndex >= MAX_WEAPONMODS)
		return false;

	//Set the weapon master's bonus	
	switch(WeaponIndex)
	{
	case WEAPON_BLASTER:
		if(ModIndex < 3)
		{
			weapon->mods[ModIndex].soft_max = 20;
			weapon->mods[ModIndex].hard_max = maxLevel;
		}
		else return false;
		break;
	case WEAPON_SHOTGUN:
		if(ModIndex == 0 || ModIndex == 2)
		{
			weapon->mods[ModIndex].soft_max = 20;
			weapon->mods[ModIndex].hard_max = maxLevel;
		}
		else return false;
		break;
	case WEAPON_SUPERSHOTGUN:
		if(ModIndex == 0 || ModIndex == 2)
		{
			weapon->mods[ModIndex].soft_max = 20;
			weapon->mods[ModIndex].hard_max = maxLevel;
		}
		else return false;
		break;
	case WEAPON_MACHINEGUN:
		if(ModIndex < 3)
		{
			weapon->mods[ModIndex].soft_max = 15;
			weapon->mods[ModIndex].hard_max = maxLevel;
		}
		else return false;
		break;
	case WEAPON_CHAINGUN:
		if(ModIndex == 0 || ModIndex == 2)
		{
			weapon->mods[ModIndex].soft_max = 15;
			weapon->mods[ModIndex].hard_max = maxLevel;
		}
		else return false;
		break;
	case WEAPON_GRENADELAUNCHER:
		if(ModIndex == 0 || ModIndex == 2)
		{
			weapon->mods[ModIndex].soft_max = 20;
			weapon->mods[ModIndex].hard_max = maxLevel;
		}
		else return false;
		break;
	case WEAPON_ROCKETLAUNCHER:
		if(ModIndex == 0 || ModIndex == 2)
		{
			weapon->mods[ModIndex].soft_max = 20;
			weapon->mods[ModIndex].hard_max = maxLevel;
		}
		else return false;
		break;
	case WEAPON_HYPERBLASTER:
		if(ModIndex == 0 || ModIndex == 2)
		{
			weapon->mods[ModIndex].soft_max = 20;
			weapon->mods[ModIndex].hard_max = maxLevel;
		}
		else return false;
		break;
	case WEAPON_RAILGUN:
		if(ModIndex == 0 || ModIndex == 2)
		{
			weapon->mods[ModIndex].soft_max = 15;
			weapon->mods[ModIndex].hard_max = maxLevel;
		}
		else return false;
		break;
	case WEAPON_BFG10K:
		if(ModIndex == 0 || ModIndex == 2)
		{
			weapon->mods[ModIndex].soft_max = 20;
			weapon->mods[ModIndex].hard_max = maxLevel;
		}
		else return false;
		break;
	case WEAPON_SWORD:
		if(ModIndex < 4 && ModIndex != 1)
		{
			weapon->mods[ModIndex].soft_max = 20;
			weapon->mods[ModIndex].hard_max = maxLevel;
		}
		else return false;
		break;
	case WEAPON_20MM:
		if(ModIndex < 3)
		{
			weapon->mods[ModIndex].soft_max = 15;
			weapon->mods[ModIndex].hard_max = maxLevel;
		}
		else return false;
		break;
	case WEAPON_HANDGRENADE:	//(increased damage and radius)
		if(ModIndex == 0 || ModIndex == 2)
		{
			weapon->mods[ModIndex].soft_max = 20;
			weapon->mods[ModIndex].hard_max = maxLevel;
		}
		else return false;
		break;
	}
	return true;
}

qboolean GiveKnightUpgrade(edict_t *ent, int WeaponIndex, int ModIndex) {
    if (ent->myskills.class_num != CLASS_KNIGHT)
        return false;

    weapon_t *weapon;
    int maxLevel = 40;		// Sword hard maximums for the knight are set to this number.

    //Point to the correct weapon
    weapon = &ent->myskills.weapons[WeaponIndex];

    //Don't crash
    if (ModIndex < 0 || ModIndex >= MAX_WEAPONMODS)
        return false;

    if (WeaponIndex == WEAPON_SWORD) { // same as WM
        if(ModIndex < 4 && ModIndex != 1)
        {
            weapon->mods[ModIndex].soft_max = 20;
            weapon->mods[ModIndex].hard_max = maxLevel;
            return true;
        }
    }

    return false;
}

//***********************************************************
//		Reset player's weapon maximums
//***********************************************************
void vrx_reset_weapon_maximums(edict_t *ent)
{
	int i, j;
    
	for (i = 0; i < MAX_WEAPONS; ++i)
	{
		for (j = 0; j < MAX_WEAPONMODS; ++j)
		{
			//Reset the current level (make it equal to the user's hard upgrade level)
			ent->myskills.weapons[i].mods[j].current_level = ent->myskills.weapons[i].mods[j].level;

			//Update the player's max levels ONLY IF they need it (ex: not loading a weapon from their player file)
			if(ent->myskills.weapons[i].mods[j].soft_max == 0 || ent->myskills.weapons[i].mods[j].hard_max == 0)
			{
				//Weapon masters and knights get a bonus to some upgrades
				if (ent->myskills.class_num != CLASS_WEAPONMASTER || !GiveWeaponMasterUpgrade(ent, i, j)) {
                    if (ent->myskills.class_num != CLASS_KNIGHT || !GiveKnightUpgrade(ent, i, j)) {
                        if (j < 3) {
                            ent->myskills.weapons[i].mods[j].soft_max = 10;
                            ent->myskills.weapons[i].mods[j].hard_max = 30;
                        } else {
                            //Sword gets an extra bonus
                            if (j == 3 && i == WEAPON_SWORD) { // az note: this is sword burn
                                ent->myskills.weapons[i].mods[j].soft_max = 10;
                                ent->myskills.weapons[i].mods[j].hard_max = 30;
                                continue;
                            }

                            ent->myskills.weapons[i].mods[j].soft_max = 1;
                            ent->myskills.weapons[i].mods[j].hard_max = 1;
                        }
                    }
                }
			}
		}
	}
}
//***********************************************************