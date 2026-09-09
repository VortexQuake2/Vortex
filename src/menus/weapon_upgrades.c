#include "g_local.h"

void OpenGeneralWeaponMenu (edict_t *ent, int lastline);
void weaponmenu_handler (edict_t *ent, int option);

//************************************************************************************************

int V_WeaponUpgradeVal(edict_t *ent, int weapnum)
{
    //Returns an integer value, usualy from 0-100. Used as a % of maximum upgrades statistic

	int	i;
	float iMax, iCount;
	float val;
	
	iMax = iCount = 0.0f;

	if ((weapnum >= MAX_WEAPONS) || (weapnum < 0))
		return -666;	//BAD weapon number

	for (i=0; i<MAX_WEAPONMODS;++i)
	{
		iCount += ent->client->resp.pstats.weapons[weapnum].mods[i].current_level;
		iMax += ent->client->resp.pstats.weapons[weapnum].mods[i].soft_max;
	}

	if (iMax == 0)
		return 0;

	val = (iCount / iMax) * 100;
	
	return (int)val;
}

//************************************************************************************************
//************************************************************************************************
/*
			This is how the weapon upgrades menu works.

				Each weapon has an index (defined in weapon_def.h)

				The current menu system requires an option that is not one of it's default
				formatting options, like MENU_GREEN_CENTERED. In order to bypass that, and
				still allow for the menu system to know which weapon it is meant to access,
				A code was used to index the option number. Here it is:


	example:	Blaster	=	1100
				Shotgun =	1200
				Railgun =	1800
				etc...

				Each mod is added to the base weapon number, and the function looks at that.

	example:	Blaster Damage =	1100
				Shotgun Spread =	1204
				Railgun Flash  =	1805


				Using this method, each weapon upgrade is indexed so only one menu is needed
				to load ANY of the weapons. This saves code space, and should be easier to modify.
*/
//************************************************************************************************
//************************************************************************************************

//************************************************************************************************
//	Secondary menu
//************************************************************************************************

void generalWeaponMenu_handler(edict_t *ent, int option)
{
	const int WeaponIndex	= (option / 100)-10;
	const int ModIndex	= (option % 100) - 1;
	
	//Are we just navigating?
	if (option == 6666)
	{
		menu_close(ent, true);
		return;
	}
	else if (option >= 7777)
	{
	    if (ent->myskills.class_num != CLASS_KNIGHT) {
            const int LastWeapon = option - 7777;
            OpenWeaponUpgradeMenu(ent, LastWeapon + 1);
        } else {
            OpenWeaponUpgradeMenu(ent, 0);
	    }

		return;
	}

    if (!(ent->client->resp.pstats.weapons[WeaponIndex].mods[ModIndex].level < ent->client->resp.pstats.weapons[WeaponIndex].mods[ModIndex].soft_max))
	{
		safe_cprintf(ent, PRINT_HIGH, "You have reached the maximum upgrade level in %s %s (%d).\n",
			GetWeaponString(WeaponIndex), GetModString(WeaponIndex,ModIndex), 
			ent->client->resp.pstats.weapons[WeaponIndex].mods[ModIndex].soft_max);
		OpenGeneralWeaponMenu(ent, option);
		return;
	}

	if (ent->myskills.weapon_points < 1)
	{
		safe_cprintf(ent, PRINT_HIGH, "You are out of weapon points.\n");
		menu_close(ent, true);
		return;
	}

	ent->client->resp.pstats.weapons[WeaponIndex].mods[ModIndex].current_level++;
	ent->client->resp.pstats.weapons[WeaponIndex].mods[ModIndex].level++;
	ent->myskills.weapon_points--;

	safe_cprintf(ent, PRINT_HIGH, "%s %s upgraded to level %d.\n",	GetWeaponString(WeaponIndex),GetModString(WeaponIndex,ModIndex), 
		ent->client->resp.pstats.weapons[WeaponIndex].mods[ModIndex].current_level);
    
    //Refresh the menu
	OpenGeneralWeaponMenu(ent, option);
}

//************************************************************************************************

void OpenGeneralWeaponMenu (edict_t *ent, int lastline)
{
	const int WeaponIndex = (lastline / 100) - 10;
	const int modIndex = (lastline % 100);
	int i;

	if (!menu_can_show(ent))
		return;
	menu_clear(ent);

	menu_add_line(ent, va("%s\n(%d%c)", GetWeaponString(WeaponIndex), V_WeaponUpgradeVal(ent, WeaponIndex), '%'), MENU_GREEN_CENTERED);
	menu_add_line(ent, va("Weapon points left: %d", ent->myskills.weapon_points), MENU_GREEN_CENTERED);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, " Select the weapon", 0);
	menu_add_line(ent, " attribute you want to", 0);
	menu_add_line(ent, " improve upon.", 0);
	menu_add_line(ent, " ", 0);

	for (i = 0; i < MAX_WEAPONMODS; ++i)
	{
		char sMod[30];
		int level, cur;
		level = ent->client->resp.pstats.weapons[WeaponIndex].mods[i].level;
		cur = ent->client->resp.pstats.weapons[WeaponIndex].mods[i].current_level;
		strcpy(sMod, GetModString(WeaponIndex, i));
		padRight(sMod, 15);
		strcat(sMod, va("%d", level));
		padRight(sMod, 18);
		menu_add_line(ent, va("%s[%d]", sMod, cur), ((WeaponIndex+10)*100) + i + 1);
	}
		
	menu_set_handler(ent, generalWeaponMenu_handler);

	if (modIndex)
		ent->client->menustorage.currentline = modIndex + 7;
	else
		ent->client->menustorage.currentline = 14;

	menu_add_line(ent, " ", 0);
	menu_add_line(ent, "Previous Menu", 7777 + WeaponIndex);
	menu_add_line(ent, "Exit", 6666);

	menu_show(ent);
}

//************************************************************************************************
//	Main weapon upgrades menu
//************************************************************************************************

#define WEAPON_MENU_PAGE_SIZE 10

static int BuildWeaponUpgradeList(edict_t *ent, int *weapon_list)
{
	int i;
	int count = 0;
	qboolean is_knight = ent->myskills.class_num == CLASS_KNIGHT;

	for (i = 0; i < MAX_WEAPONS; ++i)
	{
		if (is_knight && (i != WEAPON_SWORD))
			continue;
		weapon_list[count++] = i;
	}

	return count;
}

static void OpenWeaponUpgradeMenuPage(edict_t *ent, int page_num, int selected_weapon)
{
	int i;
	int first;
	int last;
	int total;
	int selected_line = -1;
	int weapon_list[MAX_WEAPONS];

	if (vrx_is_morphing_polt(ent))
	{
		safe_cprintf(ent, PRINT_HIGH, "You can't upgrade weapons.\n");
		return;
	}

	if (!menu_can_show(ent))
		return;

	menu_clear(ent);

	total = BuildWeaponUpgradeList(ent, weapon_list);
	if (total < 1)
	{
		menu_close(ent, true);
		return;
	}

	if (page_num < 1)
		page_num = 1;

	if (selected_weapon >= 0)
	{
		for (i = 0; i < total; ++i)
		{
			if (weapon_list[i] == selected_weapon)
			{
				page_num = (i / WEAPON_MENU_PAGE_SIZE) + 1;
				break;
			}
		}
	}

	first = (page_num - 1) * WEAPON_MENU_PAGE_SIZE;
	if (first >= total)
	{
		first = 0;
		page_num = 1;
	}

	last = first + WEAPON_MENU_PAGE_SIZE;
	if (last > total)
		last = total;

	menu_add_line(ent, "Weapon Upgrades", MENU_GREEN_CENTERED);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, "Select the weapon you", 0);
	menu_add_line(ent, "want to upgrade:", 0);
	menu_add_line(ent, " ", 0);

	for (i = first; i < last; ++i)
	{
		int weapon_index = weapon_list[i];
		char weaponString[24];

		strcpy(weaponString, GetWeaponString(weapon_index));
		padRight(weaponString, 18);
		menu_add_line(ent, va("%s%d%c", weaponString, V_WeaponUpgradeVal(ent, weapon_index), '%'),
		              (weapon_index + 10) * 100);

		if (weapon_index == selected_weapon)
			selected_line = 6 + (i - first);
	}

	menu_add_line(ent, " ", 0);
	if (last < total)
		menu_add_line(ent, "Next", (page_num * 1000) + 2);
	menu_add_line(ent, "Back", (page_num * 1000) + 1);
	menu_add_line(ent, "Exit", 6666);

	menu_set_handler(ent, weaponmenu_handler);
	if (selected_line > 0)
		ent->client->menustorage.currentline = selected_line;
	else
		ent->client->menustorage.currentline = 6;

	menu_show(ent);

	// try to shortcut to chat-protect mode
	if (ent->client->idle_frames < qf2sf(CHAT_PROTECT_FRAMES-51))
		ent->client->idle_frames = qf2sf(CHAT_PROTECT_FRAMES-51);
}

void weaponmenu_handler (edict_t *ent, int option)
{
	int page_num = (option / 1000);
	int page_choice = (option % 1000);
	int weap_num = (option / 100) - 10;

	if (option == 6666)
	{
		menu_close(ent, true);
		return;
	}

	if (page_num > 0)
	{
		if (page_choice == 2) // next
		{
			OpenWeaponUpgradeMenuPage(ent, page_num + 1, -1);
			return;
		}
		else if (page_choice == 1) // back
		{
			if (page_num == 1)
				OpenGeneralMenu(ent);
			else
				OpenWeaponUpgradeMenuPage(ent, page_num - 1, -1);
			return;
		}
	}

	if (weap_num >= MAX_WEAPONS || weap_num < 0)
	{
		menu_close(ent, true);
		return;
	}

	if ((ent->myskills.class_num == CLASS_KNIGHT) && (weap_num != WEAPON_SWORD))
		return;

	OpenGeneralWeaponMenu(ent, option);
}

//************************************************************************************************

void OpenWeaponUpgradeMenu (edict_t *ent, int lastline)
{
	int selected_weapon = -1;

	if (lastline > 0)
		selected_weapon = lastline - 1;

	OpenWeaponUpgradeMenuPage(ent, 1, selected_weapon);
}
