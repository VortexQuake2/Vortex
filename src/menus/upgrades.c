#include "g_local.h"
#include "upgradehelp.h"

void OpenSpecialUpgradeMenu (edict_t *ent, int lastline);
void OpenMultiUpgradeMenu (edict_t *ent, int lastline, int page, int generaltype); // 3.17

//************************************************************************************************
//************************************************************************************************


//************************************************************************************************

void upgradeSpecialMenu_handler(edict_t *ent, int option)
{
    int cost = vrx_get_ability_upgrade_cost(option - 1);
    const int inc_max = ent->myskills.abilities[option - 1].soft_max + 1;
    qboolean isLimitedMax = true;
    qboolean doubledcost = false;

    if (ent->myskills.abilities[option - 1].level == inc_max - 1) // we've reached the limit of this skill
    {
        const int hmax = vrx_get_hard_max(option - 1, ent->myskills.abilities[option - 1].general_skill, ent->myskills.class_num);
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
			ent->myskills.abilities[option - 1].soft_max++;
			ent->myskills.abilities[option - 1].hard_max++;
		}
	}
	else 
	{
		safe_cprintf(ent, PRINT_HIGH, va("You have already reached the maximum level in this skill. (%d)\n", 
			ent->myskills.abilities[option-1].soft_max));
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
		const int num = i + 1;
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

	const qboolean below_max = ent->myskills.abilities[ability_index].level < ent->myskills.abilities[ability_index].soft_max;
	const qboolean below_hardmax = ent->myskills.abilities[ability_index].current_level < ent->myskills.abilities[ability_index].hard_max;
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
			ent->myskills.abilities[ability_index].soft_max));
		// doon't close the menu. -az
		//return;
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
	const upgrade_t* ability = &ent->myskills.abilities[ability_index];
	const int level = ability->level;//current_level;
	int lineCount = 7;//12;

	if (!menu_can_show(ent))
		return;
	menu_clear(ent);

	menu_add_line(ent, va("%s: %d/%d\n", GetAbilityString(ability_index), level, ability->soft_max), MENU_GREEN_CENTERED);
	menu_add_line(ent, " ", 0);

	const auto help = vrx_upgradehelp_get(ability_index);
	lineCount += vrx_upgradehelp_add_menu_lines(ent, help);

	menu_add_line(ent, " ", 0);
	menu_add_line(ent, " ", 0);

	/* see the comment above to see how this option encoding works.
	 * there's an underlying assumption here that none of these values
	 * will overflow. if they do, change the layout. -az
	 */
	int option_encoded = (last_line << 24) | (page << 16) | ability_index;
	if (general_type) option_encoded |= GENERAL_BIT;

	if (level < ability->soft_max)
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

