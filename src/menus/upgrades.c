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
	case 500:	closemenu(ent);			return;
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
	if (!ShowMenu(ent))
        return;
	clearmenu(ent);
	//					xxxxxxxxxxxxxxxxxxxxxxxxxxx (max length 27 chars)
	addlinetomenu(ent, "Player Upgrades Menu", MENU_GREEN_CENTERED);
	addlinetomenu(ent, " ", 0);
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
		addlinetomenu(ent, va("%d. %s %d[%d]", total_lines-6, buffer, upgrade->level, upgrade->current_level), num);
		total_lines++;
	}
	addlinetomenu(ent, " ", 0);
	addlinetomenu(ent, va("You have %d ability points.", ent->myskills.speciality_points), 0);
	addlinetomenu(ent, " ", 0);
	addlinetomenu(ent, "Previous Menu", 502);
	addlinetomenu(ent, "Exit", 500);
	setmenuhandler(ent, upgradeSpecialMenu_handler);
	if (!lastline)
		ent->client->menustorage.currentline = 5;
	showmenu(ent);

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
	else closemenu(ent);
}

//************************************************************************************************
// upgrade_ability menu root
void OpenUpgradeMenu (edict_t *ent)
{
   if (!ShowMenu(ent))
        return;
	clearmenu(ent);
	//					xxxxxxxxxxxxxxxxxxxxxxxxxxx (max length 27 chars)
	addlinetomenu(ent, "Player Upgrades Menu", MENU_GREEN_CENTERED);
    addlinetomenu(ent, va("Your class is %s ", vrx_get_class_string(ent->myskills.class_num)), 0);
	addlinetomenu(ent, va("and you have %d points.", ent->myskills.speciality_points), 0);
	addlinetomenu(ent, " ", 0);

	if (ent->myskills.class_num != CLASS_WEAPONMASTER) // WMs don't get class specific skills.
		addlinetomenu(ent, "Class specific skills", 1);

	addlinetomenu(ent, "General skills", 2);
	// Commented out. -az vrxchile 3.2
	//addlinetomenu(ent, "Mobility skills", 3); // az, vrxchile 2.7
	addlinetomenu(ent, " ", 0);

	addlinetomenu(ent, "Exit", 4);
	setmenuhandler(ent, upgrademenu_handler);
	ent->client->menustorage.currentline = 5;
	showmenu(ent);

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
	if (ent->myskills.abilities[ability_index].level < ent->myskills.abilities[ability_index].max_level || ent->myskills.administrator > 999)
	{
		ent->myskills.speciality_points -= cost;
		ent->myskills.abilities[ability_index].level++;
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
		//Soldier talents
		//					xxxxxxxxxxxxxxxxxxxxxxxxxxxxx (max 21 lines)
	//SOLDIER
	case STRENGTH:
		addlinetomenu(ent, "Passive ability. Increases", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "weapon damage.", MENU_WHITE_CENTERED);
		return 2;
	case RESISTANCE:
		addlinetomenu(ent, "Passive ability. Reduces", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "damage from all sources.", MENU_WHITE_CENTERED);
		return 2;
	case NAPALM:
		addlinetomenu(ent, "Throw a grenade that", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "continuously explodes,", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "spawning flames and", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "catching things on fire.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Consumes power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Command: napalm", MENU_WHITE_CENTERED);
		return 6;
	case SPIKE_GRENADE:
		addlinetomenu(ent, "Throw a grenade that", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "fires deadly spikes in all", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "directions. Consumes power", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Command: spikegrenade", MENU_WHITE_CENTERED);
		return 5;
	case EMP:
		addlinetomenu(ent, "Throw a grenade that, upon", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "exploding, disables monsters", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "and other summonables. Also", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "detonates ammunition.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Consumes power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Command: emp", MENU_WHITE_CENTERED);
		return 6;
	case MIRV:
		addlinetomenu(ent, "Throw a grenade that", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "launches additional grenades", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "in all directions. Consumes", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Command: mirv", MENU_WHITE_CENTERED);
		return 5;
	case CREATE_INVIN:
		addlinetomenu(ent, "Passive ability. Provides", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "temporary invincibility.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Activates after 10 kills.", MENU_WHITE_CENTERED);
		return 3;
	case CREATE_QUAD:
		addlinetomenu(ent, "Passive ability. Provides", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "temporary quad damage.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Activates after 10 kills.", MENU_WHITE_CENTERED);
		return 3;
	case GRAPPLE_HOOK:
		addlinetomenu(ent, "Shoot a grapple hook! Allows", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "for improved mobility and", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "access to hard-to-reach", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "places. Uses power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Commands: hook/unhook", MENU_WHITE_CENTERED);
		return 5;
		//VAMPIRE
	case VAMPIRE:
		addlinetomenu(ent, "Passive ability. Receive", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "health from damage inflicted", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "by weapons.", MENU_WHITE_CENTERED);
		return 3;
	case GHOST:
		addlinetomenu(ent, "Passive ability. Chance for", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "damage taken to be reduced to", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "zero.", MENU_WHITE_CENTERED);
		return 3;
	case LIFE_DRAIN:
		addlinetomenu(ent, "Cast a spell that steals life", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "from nearby enemies. Stolen", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "health is added to yours!", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Command: lifedrain", MENU_WHITE_CENTERED);
		return 5;
	case FLESH_EATER:
		addlinetomenu(ent, "Passive ability.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Take a bite out of nearby", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "bodies. Inflicts damage on", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "enemies and restores your", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "health.", MENU_WHITE_CENTERED);
		return 5;
	case CORPSE_EXPLODE:
		addlinetomenu(ent, "Causes target corpse to", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "detonate, inflicting damage", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "on nearby enemies.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Commands:", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "detonatebody", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "spell_corpseexplode", MENU_WHITE_CENTERED);
		return 7;
	case MIND_ABSORB:
		addlinetomenu(ent, "Passive ability.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Inflict damage on nearby", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "enemies and steal power", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "cubes.", MENU_WHITE_CENTERED);
		return 4;
	case AMMO_STEAL:
		addlinetomenu(ent, "Steals ammunition from nearby", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "enemies.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Commands:", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "ammosteal", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "spell_stealammo", MENU_WHITE_CENTERED);
		return 5;
	case CONVERSION:
		addlinetomenu(ent, "Temporarily makes monsters", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "and other summonables", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "friendly. Uses power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Command: convert", MENU_WHITE_CENTERED);
		return 4;
	case CLOAK:
		addlinetomenu(ent, "Passive ability.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Become invisible after a", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "short period of time", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "of not moving or crawling.", MENU_WHITE_CENTERED);
		return 4;
		//NECROMANCER
	case MONSTER_SUMMON:
		addlinetomenu(ent, "Summon monsters to protect", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "you and fight your enemies!", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Summon commands:", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "monster [gunner|parasite", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "brain|praetor|medic|tank", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "mutant|gladiator|berserker", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "soldier|enforcer|flyer", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "floater|hover]", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Utility commands:", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "monster [remove|command", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "follow me|count|attack]", MENU_WHITE_CENTERED);
		return 12;
	case HELLSPAWN:
		addlinetomenu(ent, "Summon a hellspawn to protect", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "you and fight your enemies!", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Commands:", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "hellspawn [attack|recall]", MENU_WHITE_CENTERED);
		return 5;
	case PLAGUE:
		addlinetomenu(ent, "Passive ability.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Infects nearby enemies with", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "a life-sapping contagion!", MENU_WHITE_CENTERED);
		return 3;
	case LOWER_RESIST:
		addlinetomenu(ent, "Curse your enemies, causing.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "lowered damage resistance.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Command: lowerresist", MENU_WHITE_CENTERED);
		return 4;
	case AMP_DAMAGE:
		addlinetomenu(ent, "Curse your enemies, causing", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "increased damage to physical", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "damage sources.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Command: ampdamage", MENU_WHITE_CENTERED);
		return 5;
	case CRIPPLE:
		addlinetomenu(ent, "Reduces an enemy's health", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "by a percentage. Uses power", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Command: cripple", MENU_WHITE_CENTERED);
		return 4;
	case CURSE:
		addlinetomenu(ent, "Curse your enemies, causing", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "drunkenness and stupor!", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Command: curse", MENU_WHITE_CENTERED);
		return 4;
	case WEAKEN:
		addlinetomenu(ent, "Curse your enemies, reducing", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "the effectiveness of their", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "attacks. Uses power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Command: weaken", MENU_WHITE_CENTERED);
		return 4;
	case JETPACK:
		addlinetomenu(ent, "Pushes you to new heights,", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "allowing air travel!", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Command: +thrust", MENU_WHITE_CENTERED);
		return 4;
		//ENGINEER
	case PROXY:
		addlinetomenu(ent, "Attach a device to a surface", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "that explodes when nearby", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "enemies are detected. Uses", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Commands:", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "proxy [count|remove]", MENU_WHITE_CENTERED);
		return 6;
	case BUILD_SENTRY:
		addlinetomenu(ent, "Build a sentry gun to fight", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "your enemies! Consumes ammo", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "and power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Commands:", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "sentry [rotate|remove]", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "minisentry [beam|aim|aimall", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "|remove]", MENU_WHITE_CENTERED);
		return 7;
	case SUPPLY_STATION:
		addlinetomenu(ent, "Build a supply station that", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "creates and stores armor", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "and ammunition. Uses power", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Command: supplystation", MENU_WHITE_CENTERED);
		return 5;
	case BUILD_LASER:
		addlinetomenu(ent, "Attach a device to a surface", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "that emits a laser beam.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Commands: laser [remove]", MENU_WHITE_CENTERED);
		return 4;
	case MAGMINE:
		addlinetomenu(ent, "Toss a device that attracts", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "nearby enemies, holding them", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "in-place. Uses power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Commands:", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "magmine [count|remove]", MENU_WHITE_CENTERED);
		return 6;
	case CALTROPS:
		addlinetomenu(ent, "Drop spiked devices onto the", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "floor. Enemies that step on", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "them take damage and are", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "slowed. Uses power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Command: caltrops", MENU_WHITE_CENTERED);
		return 5;
	case AUTOCANNON:
		addlinetomenu(ent, "Build an autocannon that", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "fires on enemies that cross", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "its muzzle. Uses power cubes", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "and ammunition.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Commands: autocannon", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "[remove|aim|aimall]", MENU_WHITE_CENTERED);
		return 6;
	case DETECTOR:
		addlinetomenu(ent, "Attach a device to a surface", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "that sounds an alarm on", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "nearby enemies and attracts", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "projectiles. Uses power", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Command: detector [remove]", MENU_WHITE_CENTERED);
		return 6;
	case DECOY:
		addlinetomenu(ent, "Summons mirror images of", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "yourself to harrass enemies.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Explodes on death! Uses", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Commands: decoy", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "[remove|solid|notsolid]", MENU_WHITE_CENTERED);
		return 6;
	case EXPLODING_ARMOR:
		addlinetomenu(ent, "Toss a suit of armor that", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "explodes and throws shrapnel.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Uses armor.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Command: armorbomb", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "[<3-120>|remove]", MENU_WHITE_CENTERED);
		return 5;
	case ANTIGRAV:
		addlinetomenu(ent, "Temporarily reduces the", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "effects of gravity, allowing", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "you to float! Uses power", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Command: antigrav", MENU_WHITE_CENTERED);
		return 5;
		//SHAMAN
	case FIRE_TOTEM:
		addlinetomenu(ent, "Creates a fire totem that", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "throws fire at nearby", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "enemies!. Uses power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Commands:", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "firetotem", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "totem remove", MENU_WHITE_CENTERED);
		return 6;
	case WATER_TOTEM:
		addlinetomenu(ent, "Creates a water totem that", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "chills nearby enemies,", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "reducing their movement and", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "attack speed. Uses power", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Commands:", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "watertotem", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "totem remove", MENU_WHITE_CENTERED);
		return 8;
	case AIR_TOTEM:
		addlinetomenu(ent, "Creates an air totem that", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "absorbs damage you take.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Uses power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Commands:", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "airtotem [protect]", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "totem remove", MENU_WHITE_CENTERED);
		return 6;
	case EARTH_TOTEM:
		addlinetomenu(ent, "Creates an earth totem that", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "provides a physical damage", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "boost to nearby friendly", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "players. Uses power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Commands:", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "earthtotem", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "totem remove", MENU_WHITE_CENTERED);
		return 7;
	case DARK_TOTEM:
		addlinetomenu(ent, "Creates a darkness totem that", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "allows friendly players to", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "steal health with their", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "attacks. Uses power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Commands:", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "darknesstotem", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "totem remove", MENU_WHITE_CENTERED);
		return 7;
	case NATURE_TOTEM:
		addlinetomenu(ent, "Creates a nature totem that", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "heals nearby allies. Uses", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Commands:", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "naturetotem", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "totem remove", MENU_WHITE_CENTERED);
		return 6;
	case HASTE:
		addlinetomenu(ent, "Passive ability. Increases", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "weapon rate of fire.", MENU_WHITE_CENTERED);
		return 2;
	case TOTEM_MASTERY:
		addlinetomenu(ent, "Passive ability. Totems will", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "automatically regenerate", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "health.", MENU_WHITE_CENTERED);
		return 3;
	case SUPER_SPEED:
		addlinetomenu(ent, "Activate to move at double", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "speed! Uses power cubes.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Command: sspeed|nosspeed", MENU_WHITE_CENTERED);
		return 3;
	case FURY:
		//					xxxxxxxxxxxxxxxxxxxxxxxxxxxxx (max 21 lines)
		addlinetomenu(ent, "Passive ability.", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Chance to activate the fury!", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "Provides health regeneration,", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "increased damage to enemies,", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "and reduced damage from", MENU_WHITE_CENTERED);
		addlinetomenu(ent, "enemies while active.", MENU_WHITE_CENTERED);
		return 6;
	}

	return 0;
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
	int level = ability->current_level;
	int lineCount = 7;//12;

	if (!ShowMenu(ent))
		return;
	clearmenu(ent);

	addlinetomenu(ent, va("%s: %d/%d\n", GetAbilityString(ability_index), level, ability->max_level), MENU_GREEN_CENTERED);
	addlinetomenu(ent, " ", 0);

	lineCount += writeAbilityDescription(ent, ability_index);

	addlinetomenu(ent, " ", 0);
	addlinetomenu(ent, " ", 0);

	/* see the comment above to see how this option encoding works.
	 * there's an underlying assumption here that none of these values
	 * will overflow. if they do, change the layout. -az
	 */
	int option_encoded = (last_line << 24) | (page << 16) | ability_index;
	if (general_type) option_encoded |= GENERAL_BIT;

	if (level < ability->max_level)
		// we're going to upgrade it, so set the skill bit.
		addlinetomenu(ent, "Upgrade this ability.", option_encoded | SKILL_BIT);
	else 
		addlinetomenu(ent, " ", 0);

	// we're not going to upgrade it, so do not set the skill bit.
	addlinetomenu(ent, "Previous menu.", option_encoded);

	setmenuhandler(ent, AbilityUpgradeMenu_handler);

	if (!use_upgrade_line)
		ent->client->menustorage.currentline = lineCount - 1;
	else
		ent->client->menustorage.currentline = lineCount - 2;

	showmenu(ent);
}

void upgradeMultiMenu_class_handler (edict_t *ent, int option)
{
	int p, ability_index;

	if (option == 999)
	{
		closemenu(ent);
		return;
	}

	gi.dprintf("option=%d\n", option);

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
		closemenu(ent);
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

	if (!ShowMenu(ent))
       return;
	clearmenu(ent);

	// menu header
	addlinetomenu(ent, "Player Upgrades Menu", MENU_GREEN_CENTERED);
	addlinetomenu(ent, " ", 0);

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

		addlinetomenu(ent, va("%2d. %-14.14s %2d[%2d]", abilities+page*10, buffer, upgrade->level, upgrade->current_level), ((page+1)*1000)+i);
	

		// only display 10 abilities at a time
		if (abilities > 9)
            break;
    }

    //getMultiPageNum(ent, i);

    // menu footer
    addlinetomenu(ent, " ", 0);
    addlinetomenu(ent, va("You have %d ability points.", ent->myskills.speciality_points), 0);
    addlinetomenu(ent, " ", 0);

    if (i < vrx_get_last_enabled_skill_index(ent, generaltype)) {
        addlinetomenu(ent, "Next", 200 + page);
        total_lines++;
        next_option = true;
    }

    addlinetomenu(ent, "Previous", 300 + page);

    addlinetomenu(ent, "Exit", 999);
	
	if (generaltype == 1)
		setmenuhandler(ent, upgradeMultiMenu_handler);
	else
		setmenuhandler(ent, upgradeMultiMenu_class_handler);

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

	showmenu(ent);

	ent->client->menustorage.menu_index = MENU_MULTI_UPGRADE;
}

