#include "g_local.h"

void OpenSpecialUpgradeMenu (edict_t *ent, int lastline);
void OpenMultiUpgradeMenu (edict_t *ent, int lastline, int page, int generaltype); // 3.17

//************************************************************************************************
//************************************************************************************************


//************************************************************************************************

void upgradeSpecialMenu_handler(edict_t *ent, int option)
{
    int cost = vrx_get_ability_upgrade_cost(option - 1);
    int inc_max = ent->myskills.abilities[option - 1].max_level + 1;
    qboolean isLimitedMax = true;
    qboolean doubledcost = false;

    if (ent->myskills.abilities[option - 1].level == inc_max - 1) // we've reached the limit of this skill
    {
        int hmax = vrx_get_hard_max(option - 1, ent->myskills.abilities[option - 1].general_skill, ent->myskills.class_num);
        cost *= 2;
        doubledcost = true; // we're getting past the max level

		if (hmax >= 10) // Not a limited hardmax, we can upgrade 'infinitely'
		{
			isLimitedMax = false;
		}
	}

	//are we navigating the menu?
	switch (option)
	{
	case 502:	OpenUpgradeMenu(ent);	return;
	case 500:	menu_close(ent, true);			return;
	}

	//We are upgrading
	if (ent->myskills.speciality_points < cost)
	{
		//You need 1 points? let's fix that:
		if (cost > 1)
			safe_cprintf(ent, PRINT_HIGH, va("You need %d points to upgrade this ability.\n", cost));
		else safe_cprintf(ent, PRINT_HIGH, va("You need %d point to upgrade this ability.\n", cost));
		return;
	}

	// the cost is doubled, and it's not a limited hardmax, or it's just plain upgradable
	if ((ent->myskills.abilities[option - 1].level < inc_max && (doubledcost && !isLimitedMax)) 
		|| 
		ent->myskills.administrator > 20)
	{
		ent->myskills.speciality_points -= cost;
		ent->myskills.abilities[option-1].level++;
		ent->myskills.abilities[option-1].current_level++;

		if (doubledcost) // the skill is going above max level
		{
			ent->myskills.abilities[option - 1].max_level++;
			ent->myskills.abilities[option - 1].hard_max++;
		}
	}
	else 
	{
		safe_cprintf(ent, PRINT_HIGH, va("You have already reached the maximum level in this skill. (%d)\n", 
			ent->myskills.abilities[option-1].max_level));
	}
	// refresh the menu
	OpenSpecialUpgradeMenu(ent, ent->client->menustorage.currentline);
}

//************************************************************************************************

void OpenSpecialUpgradeMenu(edict_t *ent, int lastline)
{
	int i;
	int total_lines = 7;
	if (!menu_can_show(ent))
        return;
	menu_clear(ent);
	//					xxxxxxxxxxxxxxxxxxxxxxxxxxx (max length 27 chars)
	menu_add_line(ent, "Player Upgrades Menu", MENU_GREEN_CENTERED);
	menu_add_line(ent, " ", 0);
	ent->client->menustorage.currentline =5;
	for (i = 0; i < MAX_ABILITIES; i++)
	{
		upgrade_t *upgrade;
		int num = i + 1;
		char buffer[30];

		upgrade = &ent->myskills.abilities[i];

		if((upgrade->disable) || (upgrade->general_skill) || (upgrade->hidden))
			continue;

		//Create ability menu string
		strcpy(buffer, GetAbilityString(i));
		strcat(buffer, ":");
		padRight(buffer, 15);
		menu_add_line(ent, va("%d. %s %d[%d]", total_lines-6, buffer, upgrade->level, upgrade->current_level), num);
		total_lines++;
	}
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, va("You have %d ability points.", ent->myskills.speciality_points), 0);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, "Previous Menu", 502);
	menu_add_line(ent, "Exit", 500);
	menu_set_handler(ent, upgradeSpecialMenu_handler);
	if (!lastline)
		ent->client->menustorage.currentline = 5;
	menu_show(ent);

	ent->client->menustorage.menu_index = MENU_SPECIAL_UPGRADES;
}

//************************************************************************************************
//************************************************************************************************

void upgrademenu_handler (edict_t *ent, int option)
{
	if (option == 1)
	{
		OpenMultiUpgradeMenu(ent, 0, 0, 0);
	}
	else if (option == 2)
		OpenMultiUpgradeMenu(ent, 0, 0, 1);//OpenGeneralUpgradeMenu(ent, 0);
	else if (option == 3)
		OpenMultiUpgradeMenu(ent, 0, 0, 2);//OpenGeneralUpgradeMenu(ent, 0);
	else menu_close(ent, true);
}

//************************************************************************************************
// upgrade_ability menu root
void OpenUpgradeMenu (edict_t *ent)
{
   if (!menu_can_show(ent))
        return;
	menu_clear(ent);
	//					xxxxxxxxxxxxxxxxxxxxxxxxxxx (max length 27 chars)
	menu_add_line(ent, "Player Upgrades Menu", MENU_GREEN_CENTERED);
    menu_add_line(ent, va("Your class is %s ", vrx_get_class_string(ent->myskills.class_num)), 0);
	menu_add_line(ent, va("and you have %d points.", ent->myskills.speciality_points), 0);
	menu_add_line(ent, " ", 0);

	if (ent->myskills.class_num != CLASS_WEAPONMASTER || vrx_prestige_has_class_skills(ent)) // WMs don't get class specific skills.
		menu_add_line(ent, "Class specific skills", 1);

	menu_add_line(ent, "General skills", 2);
	// Commented out. -az vrxchile 3.2
	//menu_add_line(ent, "Mobility skills", 3); // az, vrxchile 2.7
	menu_add_line(ent, " ", 0);

	menu_add_line(ent, "Exit", 4);
	menu_set_handler(ent, upgrademenu_handler);
	ent->client->menustorage.currentline = 5;
	menu_show(ent);

	// try to shortcut to chat-protect mode
	if (ent->client->idle_frames < qf2sf(CHAT_PROTECT_FRAMES-51))
		ent->client->idle_frames = qf2sf(CHAT_PROTECT_FRAMES-51);
}

//************************************************************************************************
//************************************************************************************************

#define PAGE_NEXT		100
#define PAGE_PREVIOUS	102

int getMultiPageIndex (edict_t *ent, int page, int mode)
{
	int			i, pagenum=0, abilities=0;
	upgrade_t	*upgrade;
	qboolean	done=false;

	for (i=0; i<MAX_ABILITIES; i++)
	{
		upgrade = &ent->myskills.abilities[i];
		if (!upgrade->disable && upgrade->general_skill == mode && !upgrade->hidden)
		{
			abilities++;

			// only 10 abilities per page
			if (abilities > 9)
			{
				pagenum++;
				abilities = 0;
			}
			else if (i == MAX_ABILITIES-1)
			{
				// partial page
				pagenum++;
				break;
			}

			// we've found the page we want
			if (pagenum == page)
			{
				if (!page || done)
					break;
				done = true;
			}
		}
	}
	return i;
}

void UpgradeAbility(edict_t *ent, int ability_index) {
    int cost;
    cost = vrx_get_ability_upgrade_cost(ability_index);

    //We are upgrading
    if (ent->myskills.speciality_points < cost) {
        //You need 1 points? let's fix that:
        if (cost > 1)
            safe_cprintf(ent, PRINT_HIGH, va("You need %d points to upgrade this ability.\n", cost));
        else safe_cprintf(ent, PRINT_HIGH, va("You need one point to upgrade this ability.\n"));
        return;
    }

	qboolean below_max = ent->myskills.abilities[ability_index].level < ent->myskills.abilities[ability_index].max_level;
	qboolean below_hardmax = ent->myskills.abilities[ability_index].current_level < ent->myskills.abilities[ability_index].hard_max;
	if (below_max || ent->myskills.administrator > 999)
	{
		ent->myskills.speciality_points -= cost;
		ent->myskills.abilities[ability_index].level++;

		if (below_hardmax)
			ent->myskills.abilities[ability_index].current_level++;
	}
	else 
	{
		safe_cprintf(ent, PRINT_HIGH, va("You have already reached the maximum level in this skill. (%d)\n", 
			ent->myskills.abilities[ability_index].max_level));
		// doon't close the menu. -az
		//return;
	}
}

int writeAbilityDescription(edict_t* ent, int abilityIndex)
{
	switch (abilityIndex) {
	//GENERAL
	case VITALITY:
		menu_add_line(ent, "Passive ability. Increases", MENU_WHITE_CENTERED);
		menu_add_line(ent, "maximum health.", MENU_WHITE_CENTERED);
		return 2;
	case MAX_AMMO:
		menu_add_line(ent, "Passive ability. Increases", MENU_WHITE_CENTERED);
		menu_add_line(ent, "maximum ammunition and power", MENU_WHITE_CENTERED);
		menu_add_line(ent, "cubes.", MENU_WHITE_CENTERED);
		return 3;
	case POWER_REGEN:
		menu_add_line(ent, "Passive ability. Increases", MENU_WHITE_CENTERED);
		menu_add_line(ent, "power cube regeneration rate.", MENU_WHITE_CENTERED);
		return 2;
	case WORLD_RESIST:
		menu_add_line(ent, "Passive ability. Reduces", MENU_WHITE_CENTERED);
		menu_add_line(ent, "world damage (e.g. lava).", MENU_WHITE_CENTERED);
		return 2;
	case AMMO_REGEN:
		menu_add_line(ent, "Passive ability. Regenerates", MENU_WHITE_CENTERED);
		menu_add_line(ent, "ammunition for weapons.", MENU_WHITE_CENTERED);
		return 2;
	case SHELL_RESIST:
		menu_add_line(ent, "Passive ability. Reduces", MENU_WHITE_CENTERED);
		menu_add_line(ent, "damage from shell-based", MENU_WHITE_CENTERED);
		menu_add_line(ent, "weapons.", MENU_WHITE_CENTERED);
		return 3;
	case BULLET_RESIST:
		menu_add_line(ent, "Passive ability. Reduces", MENU_WHITE_CENTERED);
		menu_add_line(ent, "damage from bullet-based", MENU_WHITE_CENTERED);
		menu_add_line(ent, "weapons.", MENU_WHITE_CENTERED);
		return 3;
	case PIERCING_RESIST:
		menu_add_line(ent, "Passive ability. Reduces", MENU_WHITE_CENTERED);
		menu_add_line(ent, "damage from piercing", MENU_WHITE_CENTERED);
		menu_add_line(ent, "weapons.", MENU_WHITE_CENTERED);
		return 3;
	case ENERGY_RESIST:
		menu_add_line(ent, "Passive ability. Reduces", MENU_WHITE_CENTERED);
		menu_add_line(ent, "damage from energy-based", MENU_WHITE_CENTERED);
		menu_add_line(ent, "weapons.", MENU_WHITE_CENTERED);
		return 3;
	case SPLASH_RESIST:
		menu_add_line(ent, "Passive ability. Reduces", MENU_WHITE_CENTERED);
		menu_add_line(ent, "damage from explosive", MENU_WHITE_CENTERED);
		menu_add_line(ent, "weapons.", MENU_WHITE_CENTERED);
		return 3;
	case SCANNER:
		menu_add_line(ent, "Shows nearby enemies and", MENU_WHITE_CENTERED);
		menu_add_line(ent, "allies on your HUD.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: scanner", MENU_WHITE_CENTERED);
		return 3;
	case HA_PICKUP:
		menu_add_line(ent, "Increases the amount of", MENU_WHITE_CENTERED);
		menu_add_line(ent, "health and armor provided by", MENU_WHITE_CENTERED);
		menu_add_line(ent, "items. Passive ability.", MENU_WHITE_CENTERED);
		return 3;
		//					xxxxxxxxxxxxxxxxxxxxxxxxxxxxx (max 21 lines)
	//SOLDIER
	case STRENGTH:
		menu_add_line(ent, "Passive ability. Increases", MENU_WHITE_CENTERED);
		menu_add_line(ent, "weapon damage.", MENU_WHITE_CENTERED);
		return 2;
	case RESISTANCE:
		menu_add_line(ent, "Passive ability. Reduces", MENU_WHITE_CENTERED);
		menu_add_line(ent, "damage from all sources.", MENU_WHITE_CENTERED);
		return 2;
	case NAPALM:
		menu_add_line(ent, "Throw a grenade that", MENU_WHITE_CENTERED);
		menu_add_line(ent, "continuously explodes,", MENU_WHITE_CENTERED);
		menu_add_line(ent, "spawning flames and", MENU_WHITE_CENTERED);
		menu_add_line(ent, "catching things on fire.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Consumes power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: napalm", MENU_WHITE_CENTERED);
		return 6;
	case SPIKE_GRENADE:
		menu_add_line(ent, "Throw a grenade that", MENU_WHITE_CENTERED);
		menu_add_line(ent, "fires deadly spikes in all", MENU_WHITE_CENTERED);
		menu_add_line(ent, "directions. Consumes power", MENU_WHITE_CENTERED);
		menu_add_line(ent, "cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: spikegrenade", MENU_WHITE_CENTERED);
		return 5;
	case EMP:
		menu_add_line(ent, "Throw a grenade that, upon", MENU_WHITE_CENTERED);
		menu_add_line(ent, "exploding, disables monsters", MENU_WHITE_CENTERED);
		menu_add_line(ent, "and other summonables. Also", MENU_WHITE_CENTERED);
		menu_add_line(ent, "detonates ammunition.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Consumes power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: emp", MENU_WHITE_CENTERED);
		return 6;
	case MIRV:
		menu_add_line(ent, "Throw a grenade that", MENU_WHITE_CENTERED);
		menu_add_line(ent, "launches additional grenades", MENU_WHITE_CENTERED);
		menu_add_line(ent, "in all directions. Consumes", MENU_WHITE_CENTERED);
		menu_add_line(ent, "power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: mirv", MENU_WHITE_CENTERED);
		return 5;
	case EXPLODING_BARREL:
		menu_add_line(ent, "Toss a barrel that explodes", MENU_WHITE_CENTERED);
		menu_add_line(ent, "and throws shrapnel after", MENU_WHITE_CENTERED);
		menu_add_line(ent, "being destroyed.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: barrel [remove]", MENU_WHITE_CENTERED);
		return 4;
	case CREATE_INVIN:
		menu_add_line(ent, "Passive ability. Provides", MENU_WHITE_CENTERED);
		menu_add_line(ent, "temporary invincibility.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Activates after 10 kills.", MENU_WHITE_CENTERED);
		return 3;
	case CREATE_QUAD:
		menu_add_line(ent, "Passive ability. Provides", MENU_WHITE_CENTERED);
		menu_add_line(ent, "temporary quad damage.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Activates after 10 kills.", MENU_WHITE_CENTERED);
		return 3;
	case GRAPPLE_HOOK:
		menu_add_line(ent, "Shoot a grapple hook! Allows", MENU_WHITE_CENTERED);
		menu_add_line(ent, "for improved mobility and", MENU_WHITE_CENTERED);
		menu_add_line(ent, "access to hard-to-reach", MENU_WHITE_CENTERED);
		menu_add_line(ent, "places. Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Commands: hook/unhook", MENU_WHITE_CENTERED);
		return 5;
		//VAMPIRE
	case VAMPIRE:
		menu_add_line(ent, "Passive ability. Receive", MENU_WHITE_CENTERED);
		menu_add_line(ent, "health from damage inflicted", MENU_WHITE_CENTERED);
		menu_add_line(ent, "by weapons.", MENU_WHITE_CENTERED);
		return 3;
	case GHOST:
		menu_add_line(ent, "Passive ability. Chance for", MENU_WHITE_CENTERED);
		menu_add_line(ent, "damage taken to be reduced to", MENU_WHITE_CENTERED);
		menu_add_line(ent, "zero.", MENU_WHITE_CENTERED);
		return 3;
	case LIFE_DRAIN:
		menu_add_line(ent, "Cast a spell that steals life", MENU_WHITE_CENTERED);
		menu_add_line(ent, "from nearby enemies. Stolen", MENU_WHITE_CENTERED);
		menu_add_line(ent, "health is added to yours!", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: lifedrain", MENU_WHITE_CENTERED);
		return 5;
	case FLESH_EATER:
		menu_add_line(ent, "Passive ability.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Take a bite out of nearby", MENU_WHITE_CENTERED);
		menu_add_line(ent, "bodies. Inflicts damage on", MENU_WHITE_CENTERED);
		menu_add_line(ent, "enemies and restores your", MENU_WHITE_CENTERED);
		menu_add_line(ent, "health.", MENU_WHITE_CENTERED);
		return 5;
	case CORPSE_EXPLODE:
		menu_add_line(ent, "Causes target corpse to", MENU_WHITE_CENTERED);
		menu_add_line(ent, "detonate, inflicting damage", MENU_WHITE_CENTERED);
		menu_add_line(ent, "on nearby enemies.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Commands:", MENU_WHITE_CENTERED);
		menu_add_line(ent, "detonatebody", MENU_WHITE_CENTERED);
		menu_add_line(ent, "spell_corpseexplode", MENU_WHITE_CENTERED);
		return 7;
	case MIND_ABSORB:
		menu_add_line(ent, "Passive ability.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Inflict damage on nearby", MENU_WHITE_CENTERED);
		menu_add_line(ent, "enemies and steal power", MENU_WHITE_CENTERED);
		menu_add_line(ent, "cubes.", MENU_WHITE_CENTERED);
		return 4;
	case BLINKSTRIKE:
		menu_add_line(ent, "Teleports behind an enemy for", MENU_WHITE_CENTERED);
		menu_add_line(ent, "several seconds. During this", MENU_WHITE_CENTERED);
		menu_add_line(ent, "time, a damage bonus applies", MENU_WHITE_CENTERED);
		menu_add_line(ent, "as long as you remain unseen.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: blinkstrike", MENU_WHITE_CENTERED);
		return 6;
	case CONVERSION:
		menu_add_line(ent, "Temporarily makes monsters", MENU_WHITE_CENTERED);
		menu_add_line(ent, "and other summonables", MENU_WHITE_CENTERED);
		menu_add_line(ent, "friendly. Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: convert", MENU_WHITE_CENTERED);
		return 4;
	case CLOAK:
		menu_add_line(ent, "Passive ability.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Become invisible after a", MENU_WHITE_CENTERED);
		menu_add_line(ent, "short period of time", MENU_WHITE_CENTERED);
		menu_add_line(ent, "of not moving or crawling.", MENU_WHITE_CENTERED);
		return 4;
		//NECROMANCER
	case MONSTER_SUMMON:
		menu_add_line(ent, "Summon monsters to protect", MENU_WHITE_CENTERED);
		menu_add_line(ent, "you and fight your enemies!", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Summon commands:", MENU_WHITE_CENTERED);
		menu_add_line(ent, "monster [gunner|parasite", MENU_WHITE_CENTERED);
		menu_add_line(ent, "brain|praetor|medic|tank", MENU_WHITE_CENTERED);
		menu_add_line(ent, "mutant|gladiator|berserker", MENU_WHITE_CENTERED);
		menu_add_line(ent, "soldier|enforcer|flyer", MENU_WHITE_CENTERED);
		menu_add_line(ent, "floater|hover]", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Utility commands:", MENU_WHITE_CENTERED);
		menu_add_line(ent, "monster [remove|command", MENU_WHITE_CENTERED);
		menu_add_line(ent, "follow me|count|attack]", MENU_WHITE_CENTERED);
		return 12;
	case SKELETON:
		menu_add_line(ent, "Raise skeletons to protect", MENU_WHITE_CENTERED);
		menu_add_line(ent, "you and fight your enemies!", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Summon commands:", MENU_WHITE_CENTERED);
		menu_add_line(ent, "skeleton [ice|poison|fire]", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Utility commands:", MENU_WHITE_CENTERED);
		menu_add_line(ent, "skeleton [remove|command", MENU_WHITE_CENTERED);
		menu_add_line(ent, "follow me]", MENU_WHITE_CENTERED);
		return 8;
	case GOLEM:
		menu_add_line(ent, "Raise a golem to protect", MENU_WHITE_CENTERED);
		menu_add_line(ent, "you and fight your enemies!", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Commands:", MENU_WHITE_CENTERED);
		menu_add_line(ent, "golem [remove|command", MENU_WHITE_CENTERED);
		menu_add_line(ent, "follow me]", MENU_WHITE_CENTERED);
		return 6;
	case HELLSPAWN:
		menu_add_line(ent, "Summon a hellspawn to protect", MENU_WHITE_CENTERED);
		menu_add_line(ent, "you and fight your enemies!", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Commands:", MENU_WHITE_CENTERED);
		menu_add_line(ent, "hellspawn [attack|recall]", MENU_WHITE_CENTERED);
		return 5;
	case PLAGUE:
		menu_add_line(ent, "Passive ability.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Infects nearby enemies with", MENU_WHITE_CENTERED);
		menu_add_line(ent, "a life-sapping contagion!", MENU_WHITE_CENTERED);
		return 3;
	case LIFE_TAP:
		menu_add_line(ent, "Curse your enemies, allowing.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "attacks to leech life!", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: lifetap", MENU_WHITE_CENTERED);
		return 4;
	case AMP_DAMAGE:
		menu_add_line(ent, "Curse your enemies, causing", MENU_WHITE_CENTERED);
		menu_add_line(ent, "increased damage to physical", MENU_WHITE_CENTERED);
		menu_add_line(ent, "damage sources.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: ampdamage", MENU_WHITE_CENTERED);
		return 5;
	case STATIC_FIELD:
		menu_add_line(ent, "Reduces an enemy's health", MENU_WHITE_CENTERED);
		menu_add_line(ent, "by a percentage. Uses power", MENU_WHITE_CENTERED);
		menu_add_line(ent, "cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: staticfield", MENU_WHITE_CENTERED);
		return 4;
	case CURSE:
		menu_add_line(ent, "Curse your enemies, causing", MENU_WHITE_CENTERED);
		menu_add_line(ent, "drunkenness and stupor!", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: curse", MENU_WHITE_CENTERED);
		return 4;
	case WEAKEN:
		menu_add_line(ent, "Curse your enemies, reducing", MENU_WHITE_CENTERED);
		menu_add_line(ent, "the effectiveness of their", MENU_WHITE_CENTERED);
		menu_add_line(ent, "attacks. Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: weaken", MENU_WHITE_CENTERED);
		return 4;
	case JETPACK:
		menu_add_line(ent, "Pushes you to new heights,", MENU_WHITE_CENTERED);
		menu_add_line(ent, "allowing air travel!", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: +thrust", MENU_WHITE_CENTERED);
		return 4;
		//ENGINEER
	case PROXY:
		menu_add_line(ent, "Attach a device to a surface", MENU_WHITE_CENTERED);
		menu_add_line(ent, "that explodes when nearby", MENU_WHITE_CENTERED);
		menu_add_line(ent, "enemies are detected. Uses", MENU_WHITE_CENTERED);
		menu_add_line(ent, "power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Commands:", MENU_WHITE_CENTERED);
		menu_add_line(ent, "proxy [count|remove]", MENU_WHITE_CENTERED);
		return 6;
	case BUILD_SENTRY:
		menu_add_line(ent, "Build a sentry gun to fight", MENU_WHITE_CENTERED);
		menu_add_line(ent, "your enemies! Consumes ammo", MENU_WHITE_CENTERED);
		menu_add_line(ent, "and power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Commands:", MENU_WHITE_CENTERED);
		menu_add_line(ent, "sentry [rotate|remove]", MENU_WHITE_CENTERED);
		menu_add_line(ent, "minisentry [beam|aim|aimall", MENU_WHITE_CENTERED);
		menu_add_line(ent, "|remove]", MENU_WHITE_CENTERED);
		return 7;
	case SUPPLY_STATION:
		menu_add_line(ent, "Build a supply station that", MENU_WHITE_CENTERED);
		menu_add_line(ent, "creates and stores armor", MENU_WHITE_CENTERED);
		menu_add_line(ent, "and ammunition. Uses power", MENU_WHITE_CENTERED);
		menu_add_line(ent, "cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: supplystation", MENU_WHITE_CENTERED);
		return 5;
	case BUILD_LASER:
		menu_add_line(ent, "Attach a device to a surface", MENU_WHITE_CENTERED);
		menu_add_line(ent, "that emits a laser beam.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Commands: laser [remove]", MENU_WHITE_CENTERED);
		return 4;
	case MAGMINE:
		menu_add_line(ent, "Toss a device that attracts", MENU_WHITE_CENTERED);
		menu_add_line(ent, "nearby enemies, holding them", MENU_WHITE_CENTERED);
		menu_add_line(ent, "in-place. Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Commands:", MENU_WHITE_CENTERED);
		menu_add_line(ent, "magmine [count|remove]", MENU_WHITE_CENTERED);
		return 6;
	case CALTROPS:
		menu_add_line(ent, "Drop spiked devices onto the", MENU_WHITE_CENTERED);
		menu_add_line(ent, "floor. Enemies that step on", MENU_WHITE_CENTERED);
		menu_add_line(ent, "them take damage and are", MENU_WHITE_CENTERED);
		menu_add_line(ent, "slowed. Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: caltrops", MENU_WHITE_CENTERED);
		return 5;
	case AUTOCANNON:
		menu_add_line(ent, "Build an autocannon that", MENU_WHITE_CENTERED);
		menu_add_line(ent, "fires on enemies that cross", MENU_WHITE_CENTERED);
		menu_add_line(ent, "its muzzle. Uses power cubes", MENU_WHITE_CENTERED);
		menu_add_line(ent, "and ammunition.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Commands: autocannon", MENU_WHITE_CENTERED);
		menu_add_line(ent, "[remove|aim|aimall]", MENU_WHITE_CENTERED);
		return 6;
	case DETECTOR:
		menu_add_line(ent, "Attach a device to a surface", MENU_WHITE_CENTERED);
		menu_add_line(ent, "that sounds an alarm on", MENU_WHITE_CENTERED);
		menu_add_line(ent, "nearby enemies and attracts", MENU_WHITE_CENTERED);
		menu_add_line(ent, "projectiles. Uses power", MENU_WHITE_CENTERED);
		menu_add_line(ent, "cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: detector [remove]", MENU_WHITE_CENTERED);
		return 6;
	case DECOY:
		menu_add_line(ent, "Summons mirror images of", MENU_WHITE_CENTERED);
		menu_add_line(ent, "yourself to harrass enemies.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Explodes on death! Uses", MENU_WHITE_CENTERED);
		menu_add_line(ent, "power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Commands: decoy", MENU_WHITE_CENTERED);
		menu_add_line(ent, "[remove|solid|notsolid]", MENU_WHITE_CENTERED);
		return 6;
	case EXPLODING_ARMOR:
		menu_add_line(ent, "Toss a suit of armor that", MENU_WHITE_CENTERED);
		menu_add_line(ent, "explodes and throws shrapnel.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Uses armor.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: armorbomb", MENU_WHITE_CENTERED);
		menu_add_line(ent, "[<3-120>|remove]", MENU_WHITE_CENTERED);
		return 5;
	case ANTIGRAV:
		menu_add_line(ent, "Temporarily reduces the", MENU_WHITE_CENTERED);
		menu_add_line(ent, "effects of gravity, allowing", MENU_WHITE_CENTERED);
		menu_add_line(ent, "you to float! Uses power", MENU_WHITE_CENTERED);
		menu_add_line(ent, "cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: antigrav", MENU_WHITE_CENTERED);
		return 5;
		//SHAMAN
	case FIRE_TOTEM:
		menu_add_line(ent, "Creates a fire totem that", MENU_WHITE_CENTERED);
		menu_add_line(ent, "throws fire at nearby", MENU_WHITE_CENTERED);
		menu_add_line(ent, "enemies!. Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Commands:", MENU_WHITE_CENTERED);
		menu_add_line(ent, "firetotem", MENU_WHITE_CENTERED);
		menu_add_line(ent, "totem remove", MENU_WHITE_CENTERED);
		return 6;
	case WATER_TOTEM:
		menu_add_line(ent, "Creates a water totem that", MENU_WHITE_CENTERED);
		menu_add_line(ent, "chills nearby enemies,", MENU_WHITE_CENTERED);
		menu_add_line(ent, "reducing their movement and", MENU_WHITE_CENTERED);
		menu_add_line(ent, "attack speed. Uses power", MENU_WHITE_CENTERED);
		menu_add_line(ent, "cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Commands:", MENU_WHITE_CENTERED);
		menu_add_line(ent, "watertotem", MENU_WHITE_CENTERED);
		menu_add_line(ent, "totem remove", MENU_WHITE_CENTERED);
		return 8;
	case AIR_TOTEM:
		menu_add_line(ent, "Creates an air totem that", MENU_WHITE_CENTERED);
		menu_add_line(ent, "absorbs damage you take.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Commands:", MENU_WHITE_CENTERED);
		menu_add_line(ent, "airtotem [protect]", MENU_WHITE_CENTERED);
		menu_add_line(ent, "totem remove", MENU_WHITE_CENTERED);
		return 6;
	case EARTH_TOTEM:
		menu_add_line(ent, "Creates an earth totem that", MENU_WHITE_CENTERED);
		menu_add_line(ent, "provides a physical damage", MENU_WHITE_CENTERED);
		menu_add_line(ent, "boost to nearby friendly", MENU_WHITE_CENTERED);
		menu_add_line(ent, "players. Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Commands:", MENU_WHITE_CENTERED);
		menu_add_line(ent, "earthtotem", MENU_WHITE_CENTERED);
		menu_add_line(ent, "totem remove", MENU_WHITE_CENTERED);
		return 7;
	case DARK_TOTEM:
		menu_add_line(ent, "Creates a darkness totem that", MENU_WHITE_CENTERED);
		menu_add_line(ent, "allows friendly players to", MENU_WHITE_CENTERED);
		menu_add_line(ent, "steal health with their", MENU_WHITE_CENTERED);
		menu_add_line(ent, "attacks. Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Commands:", MENU_WHITE_CENTERED);
		menu_add_line(ent, "darknesstotem", MENU_WHITE_CENTERED);
		menu_add_line(ent, "totem remove", MENU_WHITE_CENTERED);
		return 7;
	case NATURE_TOTEM:
		menu_add_line(ent, "Creates a nature totem that", MENU_WHITE_CENTERED);
		menu_add_line(ent, "heals nearby allies. Uses", MENU_WHITE_CENTERED);
		menu_add_line(ent, "power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Commands:", MENU_WHITE_CENTERED);
		menu_add_line(ent, "naturetotem", MENU_WHITE_CENTERED);
		menu_add_line(ent, "totem remove", MENU_WHITE_CENTERED);
		return 6;
	case HASTE:
		menu_add_line(ent, "Passive ability. Increases", MENU_WHITE_CENTERED);
		menu_add_line(ent, "weapon rate of fire.", MENU_WHITE_CENTERED);
		return 2;
	case TOTEM_MASTERY:
		menu_add_line(ent, "Passive ability. Totems will", MENU_WHITE_CENTERED);
		menu_add_line(ent, "automatically regenerate", MENU_WHITE_CENTERED);
		menu_add_line(ent, "health.", MENU_WHITE_CENTERED);
		return 3;
	case SUPER_SPEED:
		menu_add_line(ent, "Activate to move at double", MENU_WHITE_CENTERED);
		menu_add_line(ent, "speed! Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: sspeed|nosspeed", MENU_WHITE_CENTERED);
		return 3;
	case FURY:
		menu_add_line(ent, "Passive ability.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Chance to activate the fury!", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Provides health regeneration,", MENU_WHITE_CENTERED);
		menu_add_line(ent, "increased damage to enemies,", MENU_WHITE_CENTERED);
		menu_add_line(ent, "and reduced damage from", MENU_WHITE_CENTERED);
		menu_add_line(ent, "enemies while active.", MENU_WHITE_CENTERED);
		return 6;
	//MAGE
	case MAGICBOLT:
		menu_add_line(ent, "Fires a magic bolt!", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: magicbolt", MENU_WHITE_CENTERED);
		return 3;
	case NOVA:
		menu_add_line(ent, "Creates a nova explosion", MENU_WHITE_CENTERED);
		menu_add_line(ent, "that damages nearby enemies.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: nova", MENU_WHITE_CENTERED);
		return 4;
	case BOMB_SPELL:
		menu_add_line(ent, "Causes bombs to fall from", MENU_WHITE_CENTERED);
		menu_add_line(ent, "the sky or drop on top of", MENU_WHITE_CENTERED);
		menu_add_line(ent, "enemies! Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Commands:", MENU_WHITE_CENTERED);
		menu_add_line(ent, "bombspell [forward|area]", MENU_WHITE_CENTERED);
		return 5;
	case FORCE_WALL:
		menu_add_line(ent, "Spawns a wall that sets", MENU_WHITE_CENTERED);
		menu_add_line(ent, "enemies that touch it on", MENU_WHITE_CENTERED);
		menu_add_line(ent, "fire, or a solid wall that", MENU_WHITE_CENTERED);
		menu_add_line(ent, "provides shelter. Uses power", MENU_WHITE_CENTERED);
		menu_add_line(ent, "cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command:", MENU_WHITE_CENTERED);
		menu_add_line(ent, "forcewall [solid]", MENU_WHITE_CENTERED);
		return 7;
	case LIGHTNING:
		menu_add_line(ent, "Fires a bolt of lightning", MENU_WHITE_CENTERED);
		menu_add_line(ent, "that damages enemies and", MENU_WHITE_CENTERED);
		menu_add_line(ent, "jumps between them. Uses", MENU_WHITE_CENTERED);
		menu_add_line(ent, "power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Synergy: Lightning", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: chainlightning", MENU_WHITE_CENTERED);
		return 6;
	case METEOR:
		menu_add_line(ent, "Drops a meteor from the sky", MENU_WHITE_CENTERED);
		menu_add_line(ent, "causing radius damage on", MENU_WHITE_CENTERED);
		menu_add_line(ent, "impact, setting enemies", MENU_WHITE_CENTERED);
		menu_add_line(ent, "on fire and throwing", MENU_WHITE_CENTERED);
		menu_add_line(ent, "flames! Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Synergy: Fire", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: meteor", MENU_WHITE_CENTERED);
		return 7;
	case FIREBALL:
		menu_add_line(ent, "Toss a fireball, causing", MENU_WHITE_CENTERED);
		menu_add_line(ent, "radius damage on impact,", MENU_WHITE_CENTERED);
		menu_add_line(ent, "setting enemies on fire and", MENU_WHITE_CENTERED);
		menu_add_line(ent, "throwing flames! Uses power", MENU_WHITE_CENTERED);
		menu_add_line(ent, "cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Synergy: Fire", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: fireball", MENU_WHITE_CENTERED);
		return 7;
	case LIGHTNING_STORM:
		menu_add_line(ent, "Creates a lightning storm,", MENU_WHITE_CENTERED);
		menu_add_line(ent, "causing bolts to shoot from", MENU_WHITE_CENTERED);
		menu_add_line(ent, "the sky and strike your", MENU_WHITE_CENTERED);
		menu_add_line(ent, "enemies! Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Synergy: Lightning", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: lightningstorm", MENU_WHITE_CENTERED);
		return 6;
	case FIREWALL:
		menu_add_line(ent, "Creates an inferno that", MENU_WHITE_CENTERED);
		menu_add_line(ent, "quickly grows to a wall of", MENU_WHITE_CENTERED);
		menu_add_line(ent, "flames! Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Synergy: Fire", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: firewall", MENU_WHITE_CENTERED);
		return 5;
	case GLACIAL_SPIKE:
		menu_add_line(ent, "Fires a glacial spike", MENU_WHITE_CENTERED);
		menu_add_line(ent, "that damages and freezes", MENU_WHITE_CENTERED);
		menu_add_line(ent, "enemies! Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Synergy: Ice", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: glacialspike", MENU_WHITE_CENTERED);
		return 5;
	case FROZEN_ORB:
		menu_add_line(ent, "Fires a frozen orb that", MENU_WHITE_CENTERED);
		menu_add_line(ent, "shoots icy shards to damage", MENU_WHITE_CENTERED);
		menu_add_line(ent, "and slow enemies. Uses", MENU_WHITE_CENTERED);
		menu_add_line(ent, "power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Synergy: Ice", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: frozenorb", MENU_WHITE_CENTERED);
		return 6;
	case TELEPORT:
		menu_add_line(ent, "Teleport in the direction you", MENU_WHITE_CENTERED);
		menu_add_line(ent, "aiming! Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: teleport_fwd", MENU_WHITE_CENTERED);
		return 3;
	//CLERIC
	case SALVATION:
		menu_add_line(ent, "Activates salvation aura,", MENU_WHITE_CENTERED);
		menu_add_line(ent, "protecting yourself and", MENU_WHITE_CENTERED);
		menu_add_line(ent, "allies and reducing damage", MENU_WHITE_CENTERED);
		menu_add_line(ent, "received from both physical", MENU_WHITE_CENTERED);
		menu_add_line(ent, "and magical sources. Uses", MENU_WHITE_CENTERED);
		menu_add_line(ent, "power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: salvation", MENU_WHITE_CENTERED);
		return 7;
	case HEALING:
		menu_add_line(ent, "Blesses target with healing!", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Restores health and armor.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: heal [self]", MENU_WHITE_CENTERED);
		return 4;
	case BLESS:
		menu_add_line(ent, "Blesses target with speed,", MENU_WHITE_CENTERED);
		menu_add_line(ent, "increased damage output,", MENU_WHITE_CENTERED);
		menu_add_line(ent, "and damage resistance! Uses", MENU_WHITE_CENTERED);
		menu_add_line(ent, "power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: bless [self]", MENU_WHITE_CENTERED);
		return 5;
	case YIN:
		menu_add_line(ent, "Spawns a Yin Spirit!", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Destroys corpses in exchange", MENU_WHITE_CENTERED);
		menu_add_line(ent, "for health, armor, and ammo.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: yin", MENU_WHITE_CENTERED);
		return 5;
	case YANG:
		menu_add_line(ent, "Spawns a Yang Spirit!", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Attacks your enemies. Uses", MENU_WHITE_CENTERED);
		menu_add_line(ent, "power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: yang", MENU_WHITE_CENTERED);
		return 4;
	case HAMMER:
		menu_add_line(ent, "Fires a spinning magical", MENU_WHITE_CENTERED);
		menu_add_line(ent, "hammer that spirals away from", MENU_WHITE_CENTERED);
		menu_add_line(ent, "you, and causes damage to", MENU_WHITE_CENTERED);
		menu_add_line(ent, "anything it touches. Uses", MENU_WHITE_CENTERED);
		menu_add_line(ent, "power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: blessedhammer", MENU_WHITE_CENTERED);
		return 6;
	case DEFLECT:
		menu_add_line(ent, "Blesses target with", MENU_WHITE_CENTERED);
		menu_add_line(ent, "deflection! Causes", MENU_WHITE_CENTERED);
		menu_add_line(ent, "projectiles to harmlessly", MENU_WHITE_CENTERED);
		menu_add_line(ent, "bounce away. Uses power", MENU_WHITE_CENTERED);
		menu_add_line(ent, "cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: deflect [self]", MENU_WHITE_CENTERED);
		return 6;
	case DOUBLE_JUMP:
		menu_add_line(ent, "Allows you to jump a second", MENU_WHITE_CENTERED);
		menu_add_line(ent, "time while airborne!", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Passive ability.", MENU_WHITE_CENTERED);
		return 3;
	case HOLY_FREEZE:
		menu_add_line(ent, "Activates holy freeze aura!", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Alows movement and attacks of", MENU_WHITE_CENTERED);
		menu_add_line(ent, "enemies. Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: holyfreeze", MENU_WHITE_CENTERED);
		return 4;
	//KNIGHT
	case ARMOR_UPGRADE:
		menu_add_line(ent, "Increases the effectiveness", MENU_WHITE_CENTERED);
		menu_add_line(ent, "of armor. Passive ability.", MENU_WHITE_CENTERED);
		return 2;
	case REGENERATION:
		menu_add_line(ent, "Automatically regenerates", MENU_WHITE_CENTERED);
		menu_add_line(ent, "your health.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Passive ability.", MENU_WHITE_CENTERED);
		return 3;
	case POWER_SHIELD:
		menu_add_line(ent, "Activate power screen in", MENU_WHITE_CENTERED);
		menu_add_line(ent, "your inventory to provide", MENU_WHITE_CENTERED);
		menu_add_line(ent, "frontal protection. Each", MENU_WHITE_CENTERED);
		menu_add_line(ent, "upgrade increases", MENU_WHITE_CENTERED);
		menu_add_line(ent, "effectiveness.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: use power screen", MENU_WHITE_CENTERED);
		return 6;
	case ARMOR_REGEN:
		menu_add_line(ent, "Automatically regenerates", MENU_WHITE_CENTERED);
		menu_add_line(ent, "your armor. Passive ability.", MENU_WHITE_CENTERED);
		return 2;
	case BEAM:
		menu_add_line(ent, "Fires a laser beam!", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: beam_on|beam_off", MENU_WHITE_CENTERED);
		return 2;
	case PLASMA_BOLT:
		menu_add_line(ent, "Fires a plasma bolt,", MENU_WHITE_CENTERED);
		menu_add_line(ent, "exploding each time it", MENU_WHITE_CENTERED);
		menu_add_line(ent, "impacts a surface.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: plasmabolt", MENU_WHITE_CENTERED);
		return 4;
	case SHIELD:
		//					xxxxxxxxxxxxxxxxxxxxxxxxxxxxx (max 21 lines)
		menu_add_line(ent, "Activate shield to provide", MENU_WHITE_CENTERED);
		menu_add_line(ent, "frontal protection. Similar", MENU_WHITE_CENTERED);
		menu_add_line(ent, "to power screen, but uses", MENU_WHITE_CENTERED);
		menu_add_line(ent, "charge instead of cells.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "You can't attack while", MENU_WHITE_CENTERED);
		menu_add_line(ent, "shield is activated.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Commands: shieldon|shieldoff", MENU_WHITE_CENTERED);
		return 7;
	case BOOST_SPELL:
		menu_add_line(ent, "Boosts you in the direction", MENU_WHITE_CENTERED);
		menu_add_line(ent, "you are aiming!", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: boost", MENU_WHITE_CENTERED);
		return 3;
	//ALIEN
	case SPIKER:
		menu_add_line(ent, "Spawns an organism that", MENU_WHITE_CENTERED);
		menu_add_line(ent, "shoots spikes at enemies.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Receives synergy bonus", MENU_WHITE_CENTERED);
		menu_add_line(ent, "from spike. Users power", MENU_WHITE_CENTERED);
		menu_add_line(ent, "cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: spiker [remove]", MENU_WHITE_CENTERED);
		return 6;
	case OBSTACLE:
		menu_add_line(ent, "Spawns an organism that", MENU_WHITE_CENTERED);
		menu_add_line(ent, "damages enemies that touch", MENU_WHITE_CENTERED);
		menu_add_line(ent, "it. Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: obstacle [remove]", MENU_WHITE_CENTERED);
		return 4;
	case GASSER:
		menu_add_line(ent, "Spawns an organism that", MENU_WHITE_CENTERED);
		menu_add_line(ent, "spits a damaging gas cloud", MENU_WHITE_CENTERED);
		menu_add_line(ent, "at enemies. Receives synergy", MENU_WHITE_CENTERED);
		menu_add_line(ent, "bonus from acid. Uses power", MENU_WHITE_CENTERED);
		menu_add_line(ent, "cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: gasser [remove]", MENU_WHITE_CENTERED);
		return 6;
	case HEALER:
		menu_add_line(ent, "Spawns an organism that", MENU_WHITE_CENTERED);
		menu_add_line(ent, "heals friendly units. Uses", MENU_WHITE_CENTERED);
		menu_add_line(ent, "power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: healer", MENU_WHITE_CENTERED);
		return 4;
	case SPORE:
		menu_add_line(ent, "Throws a spiked organism", MENU_WHITE_CENTERED);
		menu_add_line(ent, "that attacks enemies. Uses", MENU_WHITE_CENTERED);
		menu_add_line(ent, "power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Commands: spore", MENU_WHITE_CENTERED);
		menu_add_line(ent, "[move|remove]", MENU_WHITE_CENTERED);
		return 5;
	case SPIKE:
		menu_add_line(ent, "Fires a volley of spikes that", MENU_WHITE_CENTERED);
		menu_add_line(ent, "damage and stun enemies they", MENU_WHITE_CENTERED);
		menu_add_line(ent, "touch. Receives synergy bonus", MENU_WHITE_CENTERED);
		menu_add_line(ent, "from spiker. Users power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: spike", MENU_WHITE_CENTERED);
		return 5;
	case ACID:
		menu_add_line(ent, "Spits a volume of highly", MENU_WHITE_CENTERED);
		menu_add_line(ent, "poisonous and corrosive", MENU_WHITE_CENTERED);
		menu_add_line(ent, "liquid. Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Receives synergy bonus", MENU_WHITE_CENTERED);
		menu_add_line(ent, "from gassers.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: acid", MENU_WHITE_CENTERED);
		return 6;
	case COCOON:
		menu_add_line(ent, "Spawns an organism that", MENU_WHITE_CENTERED);
		menu_add_line(ent, "can boost your attack damage", MENU_WHITE_CENTERED);
		menu_add_line(ent, "and resistance. Uses power", MENU_WHITE_CENTERED);
		menu_add_line(ent, "cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: cocoon", MENU_WHITE_CENTERED);
		return 5;
	case BLACKHOLE:
		menu_add_line(ent, "Creates a wormhole that you", MENU_WHITE_CENTERED);
		menu_add_line(ent, "can enter to temporarily", MENU_WHITE_CENTERED);
		menu_add_line(ent, "move about the map in noclip", MENU_WHITE_CENTERED);
		menu_add_line(ent, "mode. Use again to exit. Uses", MENU_WHITE_CENTERED);
		menu_add_line(ent, "power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: wormhole", MENU_WHITE_CENTERED);
		return 6;
	//POLTERGEIST
	case MORPH_MASTERY:
		menu_add_line(ent, "Adds secondary weapon modes", MENU_WHITE_CENTERED);
		menu_add_line(ent, "to various morphs. Passive", MENU_WHITE_CENTERED);
		menu_add_line(ent, "ability.", MENU_WHITE_CENTERED);
		return 3;
	case BERSERK:
		menu_add_line(ent, "Morph into a berserker! Has", MENU_WHITE_CENTERED);
		menu_add_line(ent, "the ability to sprint short", MENU_WHITE_CENTERED);
		menu_add_line(ent, "distances with the +sprint", MENU_WHITE_CENTERED);
		menu_add_line(ent, "command. Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: berserker", MENU_WHITE_CENTERED);
		return 5;
	case CACODEMON:
		menu_add_line(ent, "Morph into the cacodemon!", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: cacodemon", MENU_WHITE_CENTERED);
		return 3;
	case BLOOD_SUCKER:
		menu_add_line(ent, "Morph into a parasite! Uses", MENU_WHITE_CENTERED);
		menu_add_line(ent, "power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: parasite", MENU_WHITE_CENTERED);
		return 3;
	case BRAIN:
		menu_add_line(ent, "Morph into a brain! Uses", MENU_WHITE_CENTERED);
		menu_add_line(ent, "power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: brain", MENU_WHITE_CENTERED);
		return 3;
	case FLYER:
		menu_add_line(ent, "Morph into a flyer! Uses", MENU_WHITE_CENTERED);
		menu_add_line(ent, "power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: flyer", MENU_WHITE_CENTERED);
		return 3;
	case MUTANT:
		menu_add_line(ent, "Morph into a mutant! Uses", MENU_WHITE_CENTERED);
		menu_add_line(ent, "power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: mutant", MENU_WHITE_CENTERED);
		return 3;
	case TANK:
		menu_add_line(ent, "Morph into a tank! Uses", MENU_WHITE_CENTERED);
		menu_add_line(ent, "power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: tank", MENU_WHITE_CENTERED);
		return 3;
	case MEDIC:
		//					xxxxxxxxxxxxxxxxxxxxxxxxxxxxx (max 21 lines)
		menu_add_line(ent, "Morph into a medic! Can heal", MENU_WHITE_CENTERED);
		menu_add_line(ent, "allies and resurrect dead", MENU_WHITE_CENTERED);
		menu_add_line(ent, "monsters, among others. Uses", MENU_WHITE_CENTERED);
		menu_add_line(ent, "power cubes.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: medic", MENU_WHITE_CENTERED);
		return 5;
	case FLASH:
		menu_add_line(ent, "Teleports you to a random", MENU_WHITE_CENTERED);
		menu_add_line(ent, "location on the map.", MENU_WHITE_CENTERED);
		menu_add_line(ent, "Command: flash", MENU_WHITE_CENTERED);
		return 3;
	default:
		menu_add_line(ent, "Unknown ability--oops!", MENU_WHITE_CENTERED);
		return 1;
	}
}



/*
 * Sorry for encoding it this way.
 * I think it's more or less the most obvious way of encoding this information.
 * -az
 *
 * option layout:
 * L: Line to restore the multimenu on, P: page of multimenu, A: ability index.
 * S: Skill bit. Are we upgrading the ability encoded in the number?
 * G: General bit. Is this the general upgrades menu?
 * SGLLLLLL PPPPPPPP AAAAAAAA AAAAAAAA
 * Allows for 256 pages, 64 lines, and 65k abilities.
 *
 * This gets encoded on vrx_open_ability_menu and decoded on AbilityUpgradeMenu_handler.
 */

#define SKILL_BIT (1 << 31)
#define GENERAL_BIT (1 << 30)

 // ability index is the ability to upgrade.
 // page, general_type and last_line are so that we can restore the multimenu afterwards.
 // "use_upgrade_line" is whether to open this menu
 // on the "upgrade this ability" menu item or not. -az
void vrx_open_ability_menu(
	edict_t* ent, 
	int ability_index, 
	int page, 
	int general_type, 
	int last_line, 
	qboolean use_upgrade_line
);

void AbilityUpgradeMenu_handler(edict_t* ent, int option) {
	//Not upgrading
	const qboolean is_ability = (option & SKILL_BIT) != 0;
	const int general_type = (option & GENERAL_BIT) != 0;
	const int ability = option & 0xFFFF;
	const int page = (option >> 16) & 0xFF;
	const int last_line = (option >> 24) & 0x3F;
	

	if (is_ability) {
		//upgrading
		UpgradeAbility(ent, ability);
		vrx_open_ability_menu(ent, ability, page, general_type, last_line, true);
	}
	else    
	{
		//OpenMultiUpgradeMenu
		OpenMultiUpgradeMenu(ent, last_line, page, general_type);
	}
}

// this menu is a sub-menu of OpenMultiUpgradeMenu (ability_upgrade) that
// displays a description of each ability, allowing you to upgrade (multiple times) or
// go back to the OpenMultiUpgradeMenu
void vrx_open_ability_menu(
	edict_t* ent, 
	int ability_index, 
	int page, 
	int general_type, 
	int last_line, 
	qboolean use_upgrade_line
) {
	upgrade_t* ability = &ent->myskills.abilities[ability_index];
	int level = ability->level;//current_level;
	int lineCount = 7;//12;

	if (!menu_can_show(ent))
		return;
	menu_clear(ent);

	menu_add_line(ent, va("%s: %d/%d\n", GetAbilityString(ability_index), level, ability->max_level), MENU_GREEN_CENTERED);
	menu_add_line(ent, " ", 0);

	lineCount += writeAbilityDescription(ent, ability_index);

	menu_add_line(ent, " ", 0);
	menu_add_line(ent, " ", 0);

	/* see the comment above to see how this option encoding works.
	 * there's an underlying assumption here that none of these values
	 * will overflow. if they do, change the layout. -az
	 */
	int option_encoded = (last_line << 24) | (page << 16) | ability_index;
	if (general_type) option_encoded |= GENERAL_BIT;

	if (level < ability->max_level)
		// we're going to upgrade it, so set the skill bit.
		menu_add_line(ent, "Upgrade this ability.", option_encoded | SKILL_BIT);
	else 
		menu_add_line(ent, " ", 0);

	// we're not going to upgrade it, so do not set the skill bit.
	menu_add_line(ent, "Previous menu.", option_encoded);

	menu_set_handler(ent, AbilityUpgradeMenu_handler);

	if (!use_upgrade_line)
		ent->client->menustorage.currentline = lineCount - 1;
	else
		ent->client->menustorage.currentline = lineCount - 2;

	menu_show(ent);
}

void upgradeMultiMenu_class_handler (edict_t *ent, int option)
{
	int p, ability_index;

	if (option == 999)
	{
		menu_close(ent, true);
		return;
	}

	//gi.dprintf("option=%d\n", option);

	// next menu
	if (option < 300)
	{
		OpenMultiUpgradeMenu(ent, PAGE_NEXT, option-199, 0);
		return;
	}
	// previous menu
	else if (option < 400)
	{
		p = option-301;
		if (p < 0)
			OpenUpgradeMenu(ent);//upgrade menu root
		else	
			OpenMultiUpgradeMenu(ent, PAGE_PREVIOUS, p, 0);
		return;
	}

	p = option/1000-1;
	//gi.dprintf("page=%d\n", p);

	ability_index = option%1000;
	//gi.dprintf("ability = %s (%d)\n", GetAbilityString(ability_index), ability_index);
	vrx_open_ability_menu(ent, ability_index, p, 0, ent->client->menustorage.currentline, false);
}

void upgradeMultiMenu_handler (edict_t *ent, int option)
{
	int p, ability_index;

	if (option == 999)
	{
		menu_close(ent, true);
		return;
	}

	//gi.dprintf("option=%d\n", option);

	// next menu
	if (option < 300)
	{
		OpenMultiUpgradeMenu(ent, PAGE_NEXT, option-199, 1);
		return;
	}
	// previous menu
	else if (option < 400)
	{
		p = option-301;
		if (p < 0)
			OpenUpgradeMenu(ent);
		else	
			OpenMultiUpgradeMenu(ent, PAGE_PREVIOUS, p, 1);
		return;
	}

	p = option/1000-1;
	//gi.dprintf("page=%d\n", p);

	ability_index = option%1000;
	//gi.dprintf("ability = %s (%d)\n", GetAbilityString(ability_index), ability_index);
	
	vrx_open_ability_menu(ent, ability_index, p, 1, ent->client->menustorage.currentline, false);
}

// this menu lists each ability along with current level
void OpenMultiUpgradeMenu (edict_t *ent, int lastline, int page, int generaltype)
{
	int			i, index, abilities=0,total_lines=7;
	char		buffer[30];
	upgrade_t	*upgrade;
	qboolean	next_option=false;

	if (!menu_can_show(ent))
       return;
	menu_clear(ent);

	// menu header
	menu_add_line(ent, "Player Upgrades Menu", MENU_GREEN_CENTERED);
	menu_add_line(ent, " ", 0);

	index = getMultiPageIndex(ent, page, generaltype);
	//gi.dprintf("index= %d\n", index);

	for (i=index; i<MAX_ABILITIES; i++)
	{
		upgrade = &ent->myskills.abilities[i];
		if((upgrade->disable) || (upgrade->general_skill != generaltype) || (upgrade->hidden))
			continue;

		abilities++;
		total_lines++;

		// create ability menu string
		strcpy(buffer, GetAbilityString(i));

		menu_add_line(ent, va("%2d. %-14.14s %2d[%2d]", abilities+page*10, buffer, upgrade->level, upgrade->current_level), ((page+1)*1000)+i);
	

		// only display 10 abilities at a time
		if (abilities > 9)
            break;
    }

    //getMultiPageNum(ent, i);

    // menu footer
    menu_add_line(ent, " ", 0);
    menu_add_line(ent, va("You have %d ability points.", ent->myskills.speciality_points), 0);
    menu_add_line(ent, " ", 0);

    if (i < vrx_get_last_enabled_skill_index(ent, generaltype)) {
        menu_add_line(ent, "Next", 200 + page);
        total_lines++;
        next_option = true;
    }

    menu_add_line(ent, "Previous", 300 + page);

    menu_add_line(ent, "Exit", 999);
	
	if (generaltype == 1)
		menu_set_handler(ent, upgradeMultiMenu_handler);
	else
		menu_set_handler(ent, upgradeMultiMenu_class_handler);

	if (!lastline)
	{
		ent->client->menustorage.currentline = total_lines-1;
	}
	else
	{
		if (lastline == PAGE_PREVIOUS)
			lastline = total_lines-1;
		else if (lastline == PAGE_NEXT)
		{
			if (next_option)
				lastline = total_lines-2;
			else
				lastline = total_lines-1;
		}
		ent->client->menustorage.currentline = lastline;
	}

	menu_show(ent);

	ent->client->menustorage.menu_index = MENU_MULTI_UPGRADE;
}

