#include "g_local.h"
#include "../characters/class_limits.h"
#include "../characters/io/v_sqlite_unidb.h"
#include "characters/io/v_characterio.h"

armoryRune_t WeaponRunes[20];
armoryRune_t AbilityRunes[20];
armoryRune_t ComboRunes[20];

#define ARMORY_MAX_CONSUMABLES	10

void OpenPurchaseMenu (edict_t *ent, int page_num, int lastline);
void OpenBuyRuneMenu(edict_t *ent, int page_num, int lastline);
void OpenSellMenu (edict_t *ent, int lastline);
void Cmd_Armory_f(edict_t *ent, int selection);

int V_ItemCount(edict_t *ent, int itemType)
{
	int i = 0;
	int count = 0;

	if(!ent || !ent->client)
		return 0;

	for(i = 0; i < MAX_VRXITEMS; ++i)
		if(ent->myskills.items[i].itemtype == itemType)
			count++;

	return count;
}

//************************************************************************************************
// Buy stuff
//************************************************************************************************

int getBuyValue(item_t *rune)
{
    int i;
	int count = 0;

	if (rune->itemtype & ITEM_UNIQUE)
		return ARMORY_RUNE_UNIQUE_PRICE;

	for(i = 0; i < MAX_VRXITEMMODS; ++i)
	{
		switch(rune->modifiers[i].type)
		{
		case TYPE_ABILITY:	count += (ARMORY_RUNE_APOINT_PRICE * rune->modifiers[i].value); break;
		case TYPE_WEAPON:	count += (ARMORY_RUNE_WPOINT_PRICE * rune->modifiers[i].value); break;
		}
	}
	return count;
}

//************************************************************************************************
// Sell stuff
//************************************************************************************************

int GetSellValue(item_t *item)
{
	//Uniques have no value
	if (item->itemtype & ITEM_UNIQUE)
		return 1;

	//Standard items have a sell value
	switch(item->itemtype)
	{
	case ITEM_POTION:		return ((float)item->quantity / (float)ARMORY_QTY_POTIONS) * ((float)ARMORY_PRICE_POTIONS / 1.5);
	case ITEM_ANTIDOTE:		return ((float)item->quantity / (float)ARMORY_QTY_ANTIDOTES) * ((float)ARMORY_PRICE_ANTIDOTES / 1.5);
	case ITEM_GRAVBOOTS:	return ((float)item->quantity / (float)ARMORY_QTY_GRAVITYBOOTS) * ((float)ARMORY_PRICE_GRAVITYBOOTS / 1.5);
	case ITEM_FIRE_RESIST:	return ((float)item->quantity / (float)ARMORY_QTY_FIRE_RESIST) * ((float)ARMORY_PRICE_FIRE_RESIST / 1.5);
	case ITEM_AUTO_TBALL:	return ((float)item->quantity / (float)ARMORY_QTY_AUTO_TBALL) * ((float)ARMORY_PRICE_AUTO_TBALL / 1.5);

	//runes
	case ITEM_COMBO:		return item->itemLevel * 250;
	case ITEM_CLASSRUNE:	return item->itemLevel * 500;
	case ITEM_WEAPON:		
	case ITEM_ABILITY:		
	//other items have a value based on item level
	default:				return item->itemLevel * 125;
	}
}

//************************************************************************************************
// Special items stuff
//************************************************************************************************

void GiveRuneToArmory(item_t *rune)
{
	armoryRune_t *firstItem;
	item_t *slot;
	int type = rune->itemtype;
	int newPrice = getBuyValue(rune);
	int i;

	//discard the rune if it's worthless
	if (newPrice == 0)
		return;


	//remove the unique flag if it's there
	// az: Why?
    //	if (type & ITEM_UNIQUE)
    //		type ^= ITEM_UNIQUE;

	//select the correct item list
	switch(type)
	{
	case ITEM_WEAPON:	firstItem = WeaponRunes;	break;
	case ITEM_ABILITY:	firstItem = AbilityRunes;	break;
	case ITEM_COMBO:	firstItem = ComboRunes;		break;
	default: return;	//The armory only sells the above items
	}

	slot = NULL;

	//find an empty slot
	for (i = 0; i < ARMORY_MAX_RUNES; ++i)
	{
		item_t *_slot = &((firstItem + i)->rune);
		if (_slot->itemtype == TYPE_NONE)
		{
			slot = _slot;
			break;
		}
	}
    
	//if there is no empty slot, replace the slot with the
	//cheapest rune in it.
	if (slot == NULL)
	{
		item_t *check;
		slot = &(firstItem->rune);

		for (i = 1; i < ARMORY_MAX_RUNES; ++i)
		{
			check = &((firstItem + i)->rune);
			if ((getBuyValue(check) < getBuyValue(slot)) || (check->itemLevel < slot->itemLevel))
				slot = check;
		}
		if ((getBuyValue(slot) > newPrice) || (slot->itemLevel > rune->itemLevel))
			slot = NULL;
	}

	//If we found a place for this rune, add it!
	if (slot != NULL)
	{
        vrx_item_copy(rune, slot);
		gi.dprintf("Item sold to armory. Price = %d.\n", newPrice);
		SaveArmory();
	}
	//else item is discarded.

}

void buyPoint(edict_t *ent, int itemindex)
{
	if (itemindex > 0)
		Cmd_Armory_f(ent, itemindex);
	else
		OpenPurchaseMenu(ent, 1, 0);
}

void armoryConfirmOption(edict_t *ent, int selection)
{
	char* selectionc = "";

	if (!menu_can_show(ent))
		return;

	menu_clear(ent);


	menu_add_line(ent, "Confirm Selection", MENU_GREEN_CENTERED);
	menu_add_line(ent, " ", 0);

	switch (selection)
	{
	case 29: selectionc = "a character reset"; break;
	}

	menu_add_line(ent, "Are you sure you " , MENU_WHITE_CENTERED);
	menu_add_line(ent, "want to buy", MENU_WHITE_CENTERED);
	menu_add_line(ent, va("%s?\n", selectionc), MENU_WHITE_CENTERED);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, " ", 0);

	menu_add_line(ent, "Yes", selection);
	menu_add_line(ent, "No", -1);

	ent->client->menustorage.currentline = 11;

	menu_set_handler(ent, buyPoint);

	menu_show(ent);
}

//************************************************************************************************

void Cmd_Armory_f(edict_t *ent, int selection)
{
	int			cur_credits=ent->myskills.credits;
	gitem_t		*item = 0;
	int			price = 0;
	int			qty = 0;
	item_t		*slot;
	int			type = ITEM_NONE;
	//int			talentLevel;

	if (ent->deadflag == DEAD_DEAD)
		return;

	//What is the price/qty of the item?
	if ((selection < 11) && (selection > 0))
		price = ARMORY_PRICE_WEAPON;
	else if (selection < 17)
		price = ARMORY_PRICE_AMMO;
	
    switch(selection)
	{
		//weapons
		case 1:		item = FindItem("Shotgun");			break;	//sg
		case 2:		item = FindItem("Super Shotgun");	break;	//ssg
		case 3:		item = FindItem("Machinegun");		break;	//mg
		case 4:		item = FindItem("Chaingun");		break;	//cg
		case 5:		item = FindItem("Grenade Launcher");break;	//gl
		case 6:		item = FindItem("Rocket Launcher");	break;	//rl
		case 7:		item = FindItem("Hyperblaster");	break;	//hb
		case 8:		item = FindItem("Railgun");			break;	//rg
		case 9:		item = FindItem("bfg10k");			break;	//bfg
		case 10:	item = FindItem("20mm Cannon");		break;	//20mm

		//ammo
		case 11:
			item = FindItem("Bullets");
			qty = ent->client->pers.max_bullets;
			break;
		case 12:
			item = FindItem("Shells");
			qty = ent->client->pers.max_shells;
			break;
		case 13:
			item = FindItem("Cells");
			qty = ent->client->pers.max_cells;
			break;
		case 14:
			item = FindItem("Grenades");
			qty = ent->client->pers.max_grenades;
			break;
		case 15:
			item = FindItem("Rockets");
			qty = ent->client->pers.max_rockets;
			break;
		case 16:
			item = FindItem("Slugs");
			qty = ent->client->pers.max_slugs;
			break;

		//others
		case 17:	//tballs
			price	= ARMORY_PRICE_TBALLS;
			item	= FindItem("tballs");
			qty		= ent->client->pers.max_tballs;
			break;
		case 18:	//4.5 replaced respawns with health
			price	= ARMORY_PRICE_HEALTH;
			qty		= ent->max_health - ent->health;
			break;
		case 19:	//power cubes
			qty		= MAX_POWERCUBES(ent) - ent->client->pers.inventory[ITEM_INDEX(Fdi_POWERCUBE)];
			//Base the price on how many cubes the player needs.
			price	=  qty * ARMORY_PRICE_POWERCUBE;
			break;
		case 20:	//armor
			price	= ARMORY_PRICE_ARMOR;
			item	= FindItem("Body Armor");
			qty		= MAX_ARMOR(ent);
			break;
		case 21:	//health potions
			type	= ITEM_POTION;
			price	= ARMORY_PRICE_POTIONS;
			qty		= ARMORY_QTY_POTIONS;
			slot	= V_FindFreeItemSlot(ent);
			break;
		case 22:	//antidotes
			type	= ITEM_ANTIDOTE;
			price	= ARMORY_PRICE_ANTIDOTES;
			qty		= ARMORY_QTY_ANTIDOTES;
			slot	= V_FindFreeItemSlot(ent);
			break;
		case 23:	//grav boots
			type	= ITEM_GRAVBOOTS;
			price	= ARMORY_PRICE_GRAVITYBOOTS;
			qty		= ARMORY_QTY_GRAVITYBOOTS;
			slot	= V_FindFreeItemSlot(ent);
			break;
		case 24:	//fire resistant clothing
			type	= ITEM_FIRE_RESIST;
			price	= ARMORY_PRICE_FIRE_RESIST;
			qty		= ARMORY_QTY_FIRE_RESIST;
			slot	= V_FindFreeItemSlot(ent);
			break;
		case 25:	//auto-tball
			type	= ITEM_AUTO_TBALL;
			price	= ARMORY_PRICE_AUTO_TBALL;
			qty		= ARMORY_QTY_AUTO_TBALL;
			slot	= V_FindFreeItemSlot(ent);
			break;
		//Runes
		case 26:	//ability rune
			PurchaseRandomRune(ent, ITEM_ABILITY); return;
		case 27:	//weapon rune
			PurchaseRandomRune(ent, ITEM_WEAPON); return;
		case 28:
			PurchaseRandomRune(ent, ITEM_COMBO); return;
		case 29:	//reset char data

			// justification for lower reset price: allows player
			// to try out new builds and
			// makes it less punishing to decide
			// to reset and try a new build or to buy ab points.

			// note: resetting doesn't give back bought ab points.
			price = ARMORY_PRICE_RESET*ent->myskills.level;
			if (price > 50000)
				price = 50000;
			break;
#ifndef REMOVE_RESPAWNS
		case 30: // respawns
			price = (int)ARMORY_PRICE_RESPAWN * ((int)ARMORY_QTY_RESPAWNS - ent->myskills.weapon_respawns) / (int)ARMORY_QTY_RESPAWNS;
			break;
#endif
		default:
			gi.dprintf("ERROR: Invalid armory item!\n");
			return;
	}

	//Talent: Bartering
    //talentLevel = vrx_get_talent_level(ent, TALENT_BARTERING);
	//if(talentLevel > 0)		price *= 1.0 - 0.05 * talentLevel;

	if (cur_credits > MAX_CREDITS+10000)
	{
		// as of version 3.13, we can only handle up to 65,535 (unsigned int) credits
		// if we have much more than this, then the player's credits probably got corrupted
		gi.dprintf("WARNING: Cmd_Armory_f corrected invalid player credits\n");
		safe_cprintf(ent, PRINT_HIGH, "Your credits were fixed.\n");
		ent->myskills.credits = 0;
		return;
	}

	if (cur_credits < price)
	{
		safe_cprintf(ent, PRINT_HIGH, "You need at least %d credits to buy this item.\n", price);
		return;
	}

	//If a weapon was purchased
	if ((selection < 11) && (selection > 0))
	{
		ent->client->pers.inventory[ITEM_INDEX(item)] = 1;
		safe_cprintf(ent, PRINT_HIGH, "You bought a %s.\n", item->pickup_name);
	}
	//If ammo was purchased (or T-Balls)
	else if (selection < 18)
	{
		ent->client->pers.inventory[ITEM_INDEX(item)] = qty;
		safe_cprintf(ent, PRINT_HIGH, "You bought %d %s.\n", qty, item->pickup_name);
	}
	//Something else was selected
	else
	{
		switch(selection)
		{
		case 18:	//4.5 health
			{
				if (ent->health >= ent->max_health)
				{
					safe_cprintf(ent, PRINT_HIGH, "You don't need health.\n");
					return;
				}
				ent->health = ent->max_health;
				safe_cprintf(ent, PRINT_HIGH, "You bought %d health.\n", qty);
			}
			break;
		case 19:	//Power Cubes
			{
				if (qty < 1)
				{
					safe_cprintf(ent, PRINT_HIGH, "You don't need power cubes.\n");
					return;
				}
				ent->client->pers.inventory[ITEM_INDEX(Fdi_POWERCUBE)] += qty;
				safe_cprintf(ent, PRINT_HIGH, "You bought %d power cubes.\n", qty);
			}
			break;
		case 20:	//armor
			{
				if (ent->client->pers.inventory[ITEM_INDEX(item)] < MAX_ARMOR(ent))
				{
					ent->client->pers.inventory[ITEM_INDEX(item)] += 100;
					if (ent->client->pers.inventory[ITEM_INDEX(item)] > MAX_ARMOR(ent))
						ent->client->pers.inventory[ITEM_INDEX(item)] = MAX_ARMOR(ent);
					safe_cprintf(ent, PRINT_HIGH, "You bought some armor.\n");
				}
				else
				{
					safe_cprintf(ent, PRINT_HIGH, "You are maxed out on armor already.\n");
					return;
				}
			}
			break;

		// handle all the new items in the same way
		case 21:	//Health Potions
		case 22:	//Antidote Potions (Holy Water)
		case 23:	//Anti-Grav boots
		case 24:	//Fire resist clothing
		case 25:	//Auto-tball
			{
				if (slot == NULL)
				{
					safe_cprintf(ent, PRINT_HIGH, "Not enough inventory space.\n");
					return;
				}

				//4.0 Players can't buy too many stackable items
				if(V_ItemCount(ent, type) >= ARMORY_MAX_CONSUMABLES)
				{
					safe_cprintf(ent, PRINT_HIGH, va("You can't buy more than %d of these items.\n", ARMORY_MAX_CONSUMABLES));
					return;
				}

				//Give them the item
				V_ItemClear(slot);
				slot->itemLevel = 0;
				slot->itemtype = type;
				slot->quantity = qty;

				//Tell the user what they bought
                switch(selection)
				{
				case 21:	safe_cprintf(ent, PRINT_HIGH, "You bought %d health potions.\n", qty);		break;
				case 22:	safe_cprintf(ent, PRINT_HIGH, "You bought %d vials of holy water.\n", qty);	break;
				case 23:	safe_cprintf(ent, PRINT_HIGH, "You bought a pair of anti-gravity boots.\n");	break;
				case 24:	safe_cprintf(ent, PRINT_HIGH, "You bought some fire resistant clothing.\n");	break;
				case 25:	safe_cprintf(ent, PRINT_HIGH, "You bought an Auto-Tball.\n");	break;
				}
			}
			break;
		
		case 29:	//Reset char data
			vrx_change_class(ent->client->pers.netname, ent->myskills.class_num, 2);
			break;
#ifndef REMOVE_RESPAWNS
		case 30:
			safe_cprintf(ent, PRINT_HIGH, "You bought %d respawns for %d credits - you now have %d.\n", (int)(ARMORY_QTY_RESPAWNS - ent->myskills.weapon_respawns), (int)(price), (int)(ARMORY_QTY_RESPAWNS));
			ent->myskills.weapon_respawns = ARMORY_QTY_RESPAWNS;
			break;
#endif
		    default:
		        return; // az: just in case
		}
	}

	//spend the credits
	ent->myskills.credits -= price;
	safe_cprintf(ent, PRINT_HIGH, "%d credits left. \n", ent->myskills.credits);
	gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/gold.wav"), 1, ATTN_NORM, 0);

}

//************************************************************************************************
//		**ARMORY PURCHASE MENU**
//************************************************************************************************

void PurchaseMenu_handler (edict_t *ent, int option)
{
	int page_num = (option / 1000);
	int page_choice = (option % 1000);

	if ((page_num == 1) && (page_choice == 1))
	{
		OpenArmoryMenu(ent);
		return;
	}
	else if (page_num > 0)
	{
		if (page_choice == 2)	//next
            OpenPurchaseMenu (ent, page_num+1, 0);
		else if (page_choice == 1)	//prev
			OpenPurchaseMenu (ent, page_num-1, 0);
		return;
	}
	//don't cause an invalid item to be selected (option 99 is checked as well)
	else if ((option > ARMORY_ITEMS) || (option < 1))
	{
		menu_close(ent, true);
		return;
	}

	//Try to buy it
	if (option == 29) // option > 27 && option != 31* // reset, ab pt, weap pt, but not respawns
	{
		armoryConfirmOption(ent, option);
		return;
	}
	else 
		Cmd_Armory_f(ent, option);

	/*
	This next bit of code fixes a logic error in the menu, where
	a player selects the last item in the menu.
	example:	(10 / 10) + 1 = page 2
				instead of page 1, item # 10
	*/
	page_num = option / 10;
	if (option % 10 != 0)
		page_num += 1;

	//Refresh the menu
	OpenPurchaseMenu (ent, page_num, option);
}

//************************************************************************************************

void OpenPurchaseMenu (edict_t *ent, int page_num, int lastline)
{
	int i;

	//Usual menu stuff
	if (!menu_can_show(ent))
        return;
	menu_clear(ent);

	//Header
	menu_add_line(ent, va("You have %d credits", ent->myskills.credits), MENU_GREEN_CENTERED);
	menu_add_line(ent, "Please make a selection:", MENU_GREEN_CENTERED);
	menu_add_line(ent, " ", 0);

	//Print this page's items
    for (i = (page_num-1)*10; i < page_num*10; ++i)
	{
		if (i < ARMORY_ITEMS)
			menu_add_line(ent, va("%s", GetArmoryItemString(i+1)), i+1);
		else menu_add_line(ent, " ", 0);
	}

	//Footer
	menu_add_line(ent, " ", 0);
	if (i < ARMORY_ITEMS) menu_add_line(ent, "Next", (page_num*1000)+2);
	menu_add_line(ent, "Back", (page_num*1000)+1);
	menu_add_line(ent, "Exit", 99);

	//Menu handler
	menu_set_handler(ent, PurchaseMenu_handler);

	//Set the menu cursor
	if (lastline % 10)	//selected 0-9 in this page
		ent->client->menustorage.currentline = (lastline % 10) + 3;
	else if (lastline)	//selected #10 in this page
		ent->client->menustorage.currentline = 13;
	else if (ARMORY_ITEMS > 10)	//menu is under 10 items
		ent->client->menustorage.currentline = 15;
	else ent->client->menustorage.currentline = 5 + i % 10;

	//Show the menu
	menu_show(ent);
}

//************************************************************************************************
//		**ITEM SELL CONFIRM MENU**
//************************************************************************************************
void ShowItemMenu_handler(edict_t *ent, int option); // item_menu.c

void SellConfirmMenu_handler(edict_t *ent, int option)
{
	if (option - 777 > 0)
	{
		int i;
		item_t *slot = &ent->myskills.items[option - 778];
		int value = GetSellValue(slot);
		int wpts, apts, total_pts;


		//log the sale
        vrx_write_to_logfile(ent, va("Selling rune for %d credits. [%s]", value, slot->id));

		// calculate number of weapon and ability points separately
		wpts = V_GetRuneWeaponPts(ent, slot);
		apts = V_GetRuneAbilityPts(ent, slot);
		// calculate weighted total
		total_pts = ceil(0.5*wpts + 0.75*apts);//was 0.66,2.0

		//Copy item to armory
		if (total_pts < 30) // only if SOMEONE can actually equip it!
			GiveRuneToArmory(slot);

		//Delete item
		memset(slot, 0, sizeof(item_t));
		safe_cprintf(ent, PRINT_HIGH, "Item Sold for %d credits.\n", value);

		//Re-apply equipment
		vrx_runes_unapply(ent);
		for (i = 0; i < 3; ++i)
			vrx_runes_apply(ent, &ent->myskills.items[i]);

		//refund some credits
		ent->myskills.credits += value;
		safe_cprintf(ent, PRINT_HIGH, "You now have %d credits.\n", ent->myskills.credits);
		gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/gold.wav"), 1, ATTN_NORM, 0);

		//save the player file
        vrx_char_io.save_player_runes(ent);
	}
	else if (option - 666 > 0)
	{
		//Back to select sell item
        if (ent->client->menustorage.oldmenuhandler == ShowItemMenu_handler)
        {
            ShowInventoryMenu(ent, option - 666, false);
        }
        else
		    OpenSellMenu(ent, option - 666);
		return;
	}
	else
	{
		//Closing menu
		menu_close(ent, true);
		return;
	}
}

//************************************************************************************************

void OpenSellConfirmMenu(edict_t *ent, int itemindex)
{
	item_t *item = &ent->myskills.items[itemindex];

	//Process the header
	StartShowInventoryMenu(ent, item);

	//Menu footer
	menu_add_line(ent, va("  Sell value: %d", GetSellValue(item)), 0);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, "  Sell this item?", 0);
	menu_add_line(ent, "No, I changed my mind!", 667 + itemindex);
	menu_add_line(ent, "Yes, sell this item.", 778 + itemindex);

	//Set handler
	menu_set_handler(ent, SellConfirmMenu_handler);

	ent->client->menustorage.currentline += 7;

	//Display the menu
	menu_show(ent);
}

//************************************************************************************************
//		**ARMORY SELL MENU**
//************************************************************************************************

void SellMenu_handler (edict_t *ent, int option)
{
	//Navigating the menu?
	if (option == 666) //exit
	{
		menu_close(ent, true);
		return;
	}
	else if (option < 1 || option > MAX_VRXITEMS)	//exit
	{
		menu_close(ent, true);
		return;
	}
	else if (ent->myskills.items[option-1].itemtype == ITEM_NONE)
	{
		//refresh the menu
		OpenSellMenu(ent, option);
		return;
	}

	if (option - 1 < 3) { // az: equipped slot?
		gi.cprintf(ent, PRINT_HIGH, "You can't sell an equipped rune.\n");
		return;
	}
	
	//We picked an item
	OpenSellConfirmMenu(ent, option-1);
}

//************************************************************************************************

void OpenSellMenu (edict_t *ent, int lastline)
{
	//Use the item select menu and change the menu handler
	ShowInventoryMenu(ent, lastline, true);
	menu_set_handler(ent, SellMenu_handler);
	menu_show(ent);
}

//************************************************************************************************
//		**ARMORY BUY RUNE CONFIRM MENU**
//************************************************************************************************

void BuyRuneConfirmMenu_handler (edict_t *ent, int option)
{
	//Navigating the menu?
	if (option > 100)
	{
		int page_num = option / 1000;
		int selection = (option % 1000)-1;
		item_t *slot = V_FindFreeItemSlot(ent);
		int cost;

		armoryRune_t *firstItem;
		item_t *rune;

		switch(page_num)
		{
		case 1: firstItem = WeaponRunes;		break;
		case 2: firstItem = &WeaponRunes[10];	break;
		case 3: firstItem = AbilityRunes;		break;
		case 4: firstItem = &AbilityRunes[10];	break;
		case 5: firstItem = ComboRunes;			break;
		case 6: firstItem = &ComboRunes[10];		break;
		default: 
			gi.dprintf("Error in BuyRuneConfirmMenu_handler(). Invalid page number: %d\n", page_num);
			return;
		}

		rune = &((firstItem + selection)->rune);
		if (rune->itemtype == ITEM_NONE)
		{
			safe_cprintf(ent, PRINT_HIGH, "Sorry, someone else has purchased this rune.\n");
			menu_close(ent, true);
			return;
		}
		cost = getBuyValue(rune);

		//Do we have enough credits?
		if (ent->myskills.credits < cost)
		{
			safe_cprintf(ent, PRINT_HIGH, "You need %d more credits to buy this item.\n", cost - ent->myskills.credits);
		}
		//Check for free space
		else if (slot == NULL)
		{
			safe_cprintf(ent, PRINT_HIGH, "Not enough room in inventory.\n");
		}
		else
		{
			//log the purchase
            vrx_write_to_logfile(ent, va("Buying rune for %d credits. [%s]", cost, rune->id));

			//Buy it!
			V_ItemSwap(rune, slot);
			ent->myskills.credits -= cost;
			SaveArmory();
			vrx_char_io.save_player_runes(ent);

			safe_cprintf(ent, PRINT_HIGH, "Rune purchased for %d credits.\nYou have %d credits left.\n", cost, ent->myskills.credits);
			gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/gold.wav"), 1, ATTN_NORM, 0);
		}

		//done
		menu_close(ent, true);
		return;
	}
	else
	{
		int page_num = option / 10;
		int selection = option % 10;

		OpenBuyRuneMenu(ent, page_num, selection);
		return;
	}

}

//************************************************************************************************

void OpenBuyRuneConfirmMenu(edict_t *ent, int option)
{
	int page_num = option / 1000;
	int selection = (option % 1000)-1;

	armoryRune_t *firstItem;
	item_t *rune;

	switch(page_num)
	{
	case 1: firstItem = WeaponRunes;		break;
	case 2: firstItem = &WeaponRunes[10];	break;
	case 3: firstItem = AbilityRunes;		break;
	case 4: firstItem = &AbilityRunes[10];	break;
	case 5: firstItem = ComboRunes;			break;
	case 6: firstItem = &ComboRunes[10];	break;
	default: 
		gi.dprintf("Error in OpenBuyRuneConfirmMenu(). Invalid page number: %d\n", page_num);
		return;
	}

	rune = &((firstItem + selection)->rune);	

	//Process the header
	StartShowInventoryMenu(ent, rune);

	//Menu footer
	menu_add_line(ent, va("  Price: %d", getBuyValue(rune)), 0);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, "  Buy this item?", 0);
	menu_add_line(ent, "No, I changed my mind!", (page_num * 10) + selection);
	menu_add_line(ent, "Yes, GIMME GIMME!!.", option);

	//Set handler
	menu_set_handler(ent, BuyRuneConfirmMenu_handler);

	ent->client->menustorage.currentline += 7;

	//Display the menu
	menu_show(ent);
	
}

//************************************************************************************************
//		**ARMORY BUY RUNE MENU**
//************************************************************************************************

void BuyRuneMenu_handler (edict_t *ent, int option)
{
	int page_num = (option / 10);
	int page_choice = (option % 10);

	if (option == 99)
	{
		menu_close(ent, true);
		return;
	}

	if ((page_num == 1) && (page_choice == 1))
	{
		OpenArmoryMenu(ent);
		return;
	}
	else if ((page_num > 0) && (page_num < 7))	//5 was chosen for no real reason
	{
		if (page_choice == 2)	//next
            OpenBuyRuneMenu (ent, page_num+1, 0);
		else if (page_choice == 1)	//prev
			OpenBuyRuneMenu (ent, page_num-1, 0);
		return;
	}
	//don't cause an invalid item to be selected
	else 
	{
		int selection = (option % 1000);
		page_num = option / 1000;
		
		if ((selection > ARMORY_MAX_RUNES) || (selection < 1))
		{
			menu_close(ent, true);
			return;
		}

		//View it
		OpenBuyRuneConfirmMenu(ent, option);
		return;
	}
}

//************************************************************************************************

void OpenBuyRuneMenu(edict_t *ent, int page_num, int lastline)
{
	int i;
	armoryRune_t *firstItem;
	char* category;

	//Usual menu stuff
	if (!menu_can_show(ent))
        return;
	menu_clear(ent);

	switch(page_num)
	{
	case 1:
		category = "a weapon rune";
		firstItem = WeaponRunes;	break;
	case 2:
		category = "a weapon rune";
		firstItem = &WeaponRunes[10];	break;
	case 3:
		category = "an ability rune";
		firstItem = AbilityRunes;	break;
	case 4:
		category = "an ability rune";
		firstItem = &AbilityRunes[10];	break;
	case 5:
		category = "a combo rune";
		firstItem = ComboRunes;		break;
	case 6:
		category = "a combo rune";
		firstItem = &ComboRunes[10];		break;
	default: 
		gi.dprintf("Error in OpenBuyRuneMenu(). Invalid page number: %d\n", page_num);
		return;
	}

	//Header
	menu_add_line(ent, va("You have %d credits", ent->myskills.credits), MENU_GREEN_CENTERED);
	menu_add_line(ent, va("Select %s:", category), MENU_GREEN_CENTERED);
	menu_add_line(ent, " ", 0);

	//Print this page's items
    for (i = 0; i < ARMORY_MAX_RUNES / 2; ++i)
	{
		item_t *rune = &((firstItem + i)->rune);
		if (rune->itemtype != ITEM_NONE)
		{
			item_menu_t fmt = vrx_menu_item_display(rune);//, ' ');
			lva_result_t txt = lva("%-13.13s %2d/%2d %5d", fmt.str, fmt.num, rune->itemLevel, getBuyValue(rune));
			menu_add_line(ent, txt.str, (page_num * 1000) + i + 1);
		}
		else 
		{
			switch(page_num)
			{
			case 1: 
			case 2:
				menu_add_line(ent, "    <Empty Weapon Slot>", 0); break;
			case 3: 
			case 4:
				menu_add_line(ent, "    <Empty Ability Slot>", 0); break;
			case 5: 
			case 6:
				menu_add_line(ent, "    <Empty Combo Slot>", 0); break;
			}
		}
	}

	//Footer
	menu_add_line(ent, " ", 0);
	if (page_num < 6) menu_add_line(ent, "Next", (page_num*10)+2);
	menu_add_line(ent, "Back", (page_num*10)+1);
	menu_add_line(ent, "Exit", 99);

	//Menu handler
	menu_set_handler(ent, BuyRuneMenu_handler);

	//Set the menu cursor
	if (lastline)	ent->client->menustorage.currentline = 4 + lastline;
	else			ent->client->menustorage.currentline = 15;
	
	//Show the menu
	menu_show(ent);
}

//************************************************************************************************
//		**ARMORY MAIN MENU**
//************************************************************************************************

void armorymenu_handler (edict_t *ent, int option)
{
	if (option == 1)
		OpenPurchaseMenu(ent, 1, 0);
	else if (option == 2)
		OpenBuyRuneMenu(ent, 1, 0);
	else if (option == 3)
		OpenSellMenu(ent, 0);
	else
		menu_close(ent, true);
}

//************************************************************************************************

void OpenArmoryMenu (edict_t *ent)
{
	if (!menu_can_show(ent))
        return;

	menu_clear(ent);

	menu_add_line(ent, "The Armory", MENU_GREEN_CENTERED);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, "Welcome to the Armory!", 0);
	menu_add_line(ent, "Select the item you want", 0);
	menu_add_line(ent, "to purchase from the", 0);
	menu_add_line(ent, va("Armory. You have %d", ent->myskills.credits), 0);
	menu_add_line(ent, "credits.", 0);
	menu_add_line(ent, " ", 0);
	if (level.time < pregame_time->value || trading->value)
		menu_add_line(ent, "Buy", 1);
	menu_add_line(ent, "Buy Runes", 2);
	menu_add_line(ent, "Sell", 3);
	menu_add_line(ent, "Exit", 4);

	menu_set_handler(ent, armorymenu_handler);
	ent->client->menustorage.currentline = 9;
	menu_show(ent);
}

//************************************************************************************************


//************************************************************************************************
//************************************************************************************************
//Move this to v_utils.c!!
//************************************************************************************************
//************************************************************************************************

void SaveArmory()
{
	char filename[256];
	FILE *fptr;

	//get path
	#if defined(_WIN32) || defined(WIN32)
		sprintf(filename, "%s\\%s", game_path->string, "settings\\ArmoryItems.dat");
	#else
		sprintf(filename, "%s/%s", game_path->string, "settings/ArmoryItems.dat");
	#endif	

	if ((fptr = fopen(filename, "wb")) != NULL)
	{
        fwrite(WeaponRunes, sizeof(armoryRune_t), ARMORY_MAX_RUNES, fptr);
		fwrite(AbilityRunes, sizeof(armoryRune_t), ARMORY_MAX_RUNES, fptr);
		fwrite(ComboRunes, sizeof(armoryRune_t), ARMORY_MAX_RUNES, fptr);
		fclose(fptr);
		gi.dprintf("INFO: Vortex Rune Shop saved successfully\n");
	}
	else
	{
		gi.dprintf("Error in SaveArmory(). Error opening file: %s\n", filename);
	}
}

//************************************************************************************************

void LoadArmory()	//Call this during InitGame()
{
	char filename[256];
	FILE *fptr;

	//get path
	sprintf(filename, "%s/%s", game_path->string, "settings/ArmoryItems.dat");

	if ((fptr = fopen(filename, "rb")) != NULL)
	{
        fread(WeaponRunes, sizeof(armoryRune_t), ARMORY_MAX_RUNES, fptr);
		fread(AbilityRunes, sizeof(armoryRune_t), ARMORY_MAX_RUNES, fptr);
		fread(ComboRunes, sizeof(armoryRune_t), ARMORY_MAX_RUNES, fptr);
		fclose(fptr);
		gi.dprintf("INFO: Vortex Rune Shop loaded successfully\n");
	}
	else
	{
		gi.dprintf("Error in LoadArmory(). Error opening file: %s\n", filename);
	}
}

//************************************************************************************************
