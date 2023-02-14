#include "g_local.h"

void ShowHelpMenu(edict_t *ent, int lastpick);

//************************************************************************************************
//		**SECONDARY HELP MENU HANDLER**
//************************************************************************************************

void ShowSecondaryMenu_handler(edict_t *ent, int option)
{
	if (option != 666)
	{
		ShowHelpMenu(ent, option);
		return;
	}
	menu_close(ent, true);
}

//************************************************************************************************
//		**MODE HELP MENU**
//************************************************************************************************

void ShowModeHelpMenu(edict_t *ent)
{
	//Usual menu stuff
	 if (!menu_can_show(ent))
        return;
	menu_clear(ent);

	//Print header
	//					xxxxxxxxxxxxxxxxxxxxxxxxxxx (max length 27 chars)
	menu_add_line(ent, "Game modes:", MENU_GREEN_LEFT);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, "  Player Vs. Player  (PvP)", 0);
	menu_add_line(ent, "  Player Vs. Monters (PvM)", 0);
	menu_add_line(ent, "  Domination         (DOM)", 0);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, " Domination is a variation", 0);
	menu_add_line(ent, " of capture the flag.", 0);
	menu_add_line(ent, " Protect your flag runner!", 0);

	//Menu footer
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, "Previous", 80);
	menu_add_line(ent, "Exit", 666);

	//Set handler
	menu_set_handler(ent, ShowSecondaryMenu_handler);

	//Set current line
	ent->client->menustorage.currentline = 11;

	//Display the menu
	menu_show(ent);
}

//************************************************************************************************
//		**VOTE HELP MENU**
//************************************************************************************************

void ShowVoteHelpMenu(edict_t *ent)
{
	//Usual menu stuff
	 if (!menu_can_show(ent))
        return;
	menu_clear(ent);

	//Print header
	//					xxxxxxxxxxxxxxxxxxxxxxxxxxx (max length 27 chars)
	menu_add_line(ent, "Voting", MENU_GREEN_LEFT);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, " To vote for the next game", 0);
	menu_add_line(ent, " and map, use the command:", 0);
	menu_add_line(ent, " vote. You must vote for", 0);
	menu_add_line(ent, " both a game mode and ", 0);
	menu_add_line(ent, " a map in order for your", 0);
	menu_add_line(ent, " vote to be recorded.", 0);

	//Menu footer
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, "Previous", 70);
	menu_add_line(ent, "Exit", 666);

	//Set handler
	menu_set_handler(ent, ShowSecondaryMenu_handler);

	//Set current line
	ent->client->menustorage.currentline = 10;

	//Display the menu
	menu_show(ent);
}

//************************************************************************************************
//		**TRADE HELP MENU**
//************************************************************************************************

void ShowTradeHelpMenu(edict_t *ent)
{
	//Usual menu stuff
	 if (!menu_can_show(ent))
        return;
	menu_clear(ent);

	//Print header
	//					xxxxxxxxxxxxxxxxxxxxxxxxxxx (max length 27 chars)
	menu_add_line(ent, "Trading", MENU_GREEN_LEFT);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, " To start a trade, use the", 0);
	menu_add_line(ent, " command: trade.", 0);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, " Select up to 3 items", 0);
	menu_add_line(ent, " for the trade queue,", 0);
	menu_add_line(ent, " then select 'accept'", 0);
	menu_add_line(ent, " to complete the trade.", 0);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, "cmd: trade [on/off]", 0);

	//Menu footer
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, "Previous", 60);
	menu_add_line(ent, "Exit", 666);

	//Set handler
	menu_set_handler(ent, ShowSecondaryMenu_handler);

	//Set current line
	ent->client->menustorage.currentline = 13;

	//Display the menu
	menu_show(ent);
}

//************************************************************************************************
//		**ITEMS HELP MENU**
//************************************************************************************************

void ShowItemsHelpMenu(edict_t *ent)
{
	//Usual menu stuff
	 if (!menu_can_show(ent))
        return;
	menu_clear(ent);

	//Print header
	//					xxxxxxxxxxxxxxxxxxxxxxxxxxx (max length 27 chars)
	menu_add_line(ent, "Item types:", MENU_GREEN_LEFT);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, " Health potions", MENU_GREEN_LEFT);
	menu_add_line(ent, "  Heals 50% max health.", 0);
	menu_add_line(ent, " Holy water", MENU_GREEN_LEFT);
	menu_add_line(ent, "  Removes all curses.", 0);
	menu_add_line(ent, " Tballs", MENU_GREEN_LEFT);
	menu_add_line(ent, "  Teleport self/others.", 0);
	menu_add_line(ent, " Gravity Boots", MENU_GREEN_LEFT);
	menu_add_line(ent, "  Jump great distances.", 0);
	menu_add_line(ent, " Fire Resistant Clothing", MENU_GREEN_LEFT);
	menu_add_line(ent, "  Resists burns.", 0);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, " To use: cmd use <item>", 0);
	menu_add_line(ent, " Some items are automatic.", 0);
	menu_add_line(ent, " Tballs can not be dropped.", 0);

	//Menu footer
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, "Previous", 50);
	menu_add_line(ent, "Exit", 666);

	//Set handler
	menu_set_handler(ent, ShowSecondaryMenu_handler);

	//Set current line
	ent->client->menustorage.currentline = 18;

	//Display the menu
	menu_show(ent);
}

//************************************************************************************************
//		**RUNES HELP MENU**
//************************************************************************************************

void ShowRuneTypes(edict_t *ent)
{
	//Usual menu stuff
	 if (!menu_can_show(ent))
        return;
	menu_clear(ent);

	//Print header
	//					xxxxxxxxxxxxxxxxxxxxxxxxxxx (max length 27 chars)
	menu_add_line(ent, "Rune types:", MENU_GREEN_LEFT);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, " Ability runes   (blue)", 0);
	menu_add_line(ent, " Weapon runes    (red)", 0);
	menu_add_line(ent, " Combo runes     (purple)", 0);
	menu_add_line(ent, " Class runes     (cyan)", 0);
	menu_add_line(ent, " Unique runes    (yellow)", 0);
	menu_add_line(ent, " Special Uniques (green)", 0);
	
	//Menu footer
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, " Previous", 40);
	menu_add_line(ent, " Exit", 666);

	//Set handler
	menu_set_handler(ent, ShowSecondaryMenu_handler);

	//Set current line
	ent->client->menustorage.currentline = 10;

	//Display the menu
	menu_show(ent);
}

//************************************************************************************************
//		**COMMANDS HELP MENU**
//************************************************************************************************

void ShowCommandsMenu(edict_t *ent)
{
	//Usual menu stuff
	 if (!menu_can_show(ent))
        return;
	menu_clear(ent);

	//Print header
	//					xxxxxxxxxxxxxxxxxxxxxxxxxxx (max length 27 chars)
	menu_add_line(ent, "Standard Vortex Commands:", MENU_GREEN_LEFT);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, " set vrx_password <pass> u", 0);
	menu_add_line(ent, " email <address>", 0);
	menu_add_line(ent, " upgrade_weapon", 0);
	menu_add_line(ent, " upgrade_ability", 0);
	menu_add_line(ent, " vrxarmory", 0);
	menu_add_line(ent, " vrxrespawn", 0);
	menu_add_line(ent, " vrxinfo", 0);
	menu_add_line(ent, " rune", 0);
	menu_add_line(ent, " vote", 0);
	menu_add_line(ent, " vrxid", 0);
	menu_add_line(ent, " use tball [self]", 0);
	menu_add_line(ent, " togglesecondary", 0);

	//Menu footer
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, "Previous", 20);
	menu_add_line(ent, "Exit", 666);

	//Set handler
	menu_set_handler(ent, ShowSecondaryMenu_handler);

	//Set current line
	ent->client->menustorage.currentline = 16;

	//Display the menu
	menu_show(ent);
}

//************************************************************************************************
//		**BASICS HELP MENU**
//************************************************************************************************

void ShowBasicsMenu(edict_t *ent)
{
	//Usual menu stuff
	 if (!menu_can_show(ent))
        return;
	menu_clear(ent);

	//Print header
	//					xxxxxxxxxxxxxxxxxxxxxxxxxxx (max length 27 chars)
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, " To find out more about the", 0);
	menu_add_line(ent, " basics of Vortex, go", 0);
	menu_add_line(ent, " visit our website at:", 0);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, " https://q2vortex.com", MENU_GREEN_CENTERED);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, " There is too much info", 0);
	menu_add_line(ent, " to show in this menu.", 0);

	//Menu footer
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, "Previous", 10);
	menu_add_line(ent, "Exit", 666);

	//Set handler
	menu_set_handler(ent, ShowSecondaryMenu_handler);

	//Set current line
	ent->client->menustorage.currentline = 11;

	//Display the menu
	menu_show(ent);
}

//************************************************************************************************
//		**MAIN HELP MENU**
//************************************************************************************************

void ShowHelpMenu_handler(edict_t *ent, int option)
{
	if (option == 666)
	{
		menu_close(ent, true);
		return;
	}
	else
	{
		switch(option)
		{
		case 10:	ShowBasicsMenu(ent);	return;
		case 20:	ShowCommandsMenu(ent);	return;
		case 30:	PrintCommands(ent);		return;
		case 40:	ShowRuneTypes(ent);		return;
		case 50:	ShowItemsHelpMenu(ent);	return;
		case 60:	ShowTradeHelpMenu(ent);	return;
		case 70:	ShowVoteHelpMenu(ent);	return;
		case 80:	ShowModeHelpMenu(ent);	return;
		}
	}
}

//************************************************************************************************

void ShowHelpMenu(edict_t *ent, int lastpick)
{
	//Usual menu stuff
	 if (!menu_can_show(ent))
        return;
	menu_clear(ent);

	//Print header
	menu_add_line(ent, "Choose a help topic:", MENU_GREEN_LEFT);
	menu_add_line(ent, " ", 0);

	//					xxxxxxxxxxxxxxxxxxxxxxxxxxx (max length 27 chars)
	menu_add_line(ent, " Basics", 10);
	menu_add_line(ent, " Standard commands", 20);
	menu_add_line(ent, " Print full command list", 30);
	menu_add_line(ent, " Rune Types", 40);
	menu_add_line(ent, " Consumable items", 50);
	menu_add_line(ent, " Trading", 60);
	menu_add_line(ent, " Voting", 70);
	menu_add_line(ent, " Game modes", 80);

	//Menu footer
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, "Exit", 666);

	//Set handler
	menu_set_handler(ent, ShowHelpMenu_handler);

	//Set current line
	if (lastpick)
		ent->client->menustorage.currentline = (lastpick / 10) + 2;
	else ent->client->menustorage.currentline = 12;

	//Display the menu
	menu_show(ent);
}

