#include "g_local.h"

//************************************************************************************************
//		Sorry about the "hacker" approach to indexing through these menus,
//		it's just quicker this way. :) (doomie)
//************************************************************************************************

//************************************************************************************************
//		**ITEM DELETE MENU**
//************************************************************************************************

void DeleteMenu_handler(edict_t *ent, int option) {
    if (option - 777 > 0) {
        int i;

        //Delete item
        memset(&ent->myskills.items[option - 778], 0, sizeof(item_t));

        //Re-apply equipment
        vrx_runes_unapply(ent);
        for (i = 0; i < 3; ++i)
            vrx_runes_apply(ent, &ent->myskills.items[i]);

        safe_cprintf(ent, PRINT_HIGH, "Item deleted.\n");
    } else if (option - 666 > 0) {
        //Back to main
        ShowInventoryMenu(ent, option - 666, false);
        return;
    } else {
        //Closing menu
        menu_close(ent, true);
        return;
    }
}

//************************************************************************************************

void ItemDeleteMenu(edict_t *ent, int itemindex) {
    item_t *item = &ent->myskills.items[itemindex];

    //Process the header
    StartShowInventoryMenu(ent, item);

    //Menu footer
    menu_add_line(ent, " ", 0);
    menu_add_line(ent, "  Delete this item?", 0);
    menu_add_line(ent, "No, I changed my mind!", 667 + itemindex);
    menu_add_line(ent, "Yes, delete this item.", 778 + itemindex);

    //Set handler
    menu_set_handler(ent, DeleteMenu_handler);

    ent->client->menustorage.currentline += 4;

    //Display the menu
    menu_show(ent);
}

//************************************************************************************************
//		**GENERAL ITEM DISPLAY MENU**
//************************************************************************************************

void StartShowInventoryMenu(edict_t *ent, item_t *item) {
    int type = item->itemtype;
    int linecount = 1;
    int i;

    if (!menu_can_show(ent))
        return;
    menu_clear(ent);

    //If item has a name, use that instead
    if (strlen(item->name) > 0) {
        if (item->setCode == 0)
            menu_add_line(ent, va(" %s", item->name), MENU_GREEN_LEFT);
        else menu_add_line(ent, va(" %s (set item)", item->name), MENU_GREEN_LEFT);
    } else {
        //Print header, depending on the item type
        menu_add_line(ent, va("%s", V_MenuItemString (item, ' ')), MENU_GREEN_LEFT);
    }

    //Unique runes need to display stats too
    if (type & ITEM_UNIQUE)
        type ^= ITEM_UNIQUE;

    //Item's stats
    switch (type) {
        case ITEM_WEAPON: {
            int wIndex = (item->modifiers[0].index / 100) - 10;

            menu_add_line(ent, " ", 0);
            menu_add_line(ent, va(" %s", GetWeaponString(wIndex)), 0);
            menu_add_line(ent, " ", 0);
            menu_add_line(ent, " Stats:", MENU_GREEN_LEFT);
            linecount += 4;
            for (i = 0; i < MAX_VRXITEMMODS; ++i) {
                int mIndex;
                char buf[30];

                if ((item->modifiers[i].type == TYPE_NONE) || (V_HiddenMod(ent, item, &item->modifiers[i])))
                    continue;

                mIndex = item->modifiers[i].index % 100;
                strcpy(buf, GetModString(wIndex, mIndex));
                padRight(buf, 20);
                menu_add_line(ent, va("  %s [%d]", buf, item->modifiers[i].value), 0);
                ++linecount;
            }
            menu_add_line(ent, " ", 0);
            ++linecount;
        }
            break;
        case ITEM_ABILITY: {
            menu_add_line(ent, " ", 0);
            menu_add_line(ent, " Stats:", MENU_GREEN_LEFT);
            linecount += 2;
            for (i = 0; i < MAX_VRXITEMMODS; ++i) {
                int aIndex;
                char buf[30];

                aIndex = item->modifiers[i].index;
                if ((item->modifiers[i].type == TYPE_NONE) || (V_HiddenMod(ent, item, &item->modifiers[i])))
                    continue;

                strcpy(buf, GetAbilityString(aIndex));
                padRight(buf, 20);
                menu_add_line(ent, va("  %s [%d]", buf, item->modifiers[i].value), 0);
                linecount++;
            }
        }
            break;
        case ITEM_COMBO: {
            menu_add_line(ent, " ", 0);
            menu_add_line(ent, " Stats:", MENU_GREEN_LEFT);
            linecount += 2;
            for (i = 0; i < MAX_VRXITEMMODS; ++i) {
                char buf[30];
                if ((item->modifiers[i].type == TYPE_NONE) || (V_HiddenMod(ent, item, &item->modifiers[i])))
                    continue;

                switch (item->modifiers[i].type) {
                    case TYPE_ABILITY: {
                        int aIndex;
                        aIndex = item->modifiers[i].index;

                        strcpy(buf, GetAbilityString(aIndex));
                        padRight(buf, 20);
                        menu_add_line(ent, va("  %s [%d]", buf, item->modifiers[i].value), 0);
                        linecount++;
                    }
                        break;
                    case TYPE_WEAPON: {
                        int wIndex = (item->modifiers[i].index / 100) - 10;
                        int mIndex = item->modifiers[i].index % 100;

                        strcpy(buf, GetShortWeaponString(wIndex));
                        strcat(buf, va(" %s", GetModString(wIndex, mIndex)));
                        padRight(buf, 20);
                        menu_add_line(ent, va("  %s [%d]", buf, item->modifiers[i].value), 0);
                        linecount++;
                    }
                        break;
                }
            }
        }
            break;
        case ITEM_CLASSRUNE: {
            menu_add_line(ent, " ", 0);
            menu_add_line(ent, " Stats:", MENU_GREEN_LEFT);
            linecount += 2;
            for (i = 0; i < MAX_VRXITEMMODS; ++i) {
                int aIndex;
                char buf[30];

                aIndex = item->modifiers[i].index;

                if (item->modifiers[i].type == TYPE_NONE)
                    continue;

                strcpy(buf, GetAbilityString(aIndex));
                padRight(buf, 20);
                menu_add_line(ent, va("  %s [%d]", buf, item->modifiers[i].value), 0);
                linecount++;
            }
        }
            break;
    }

    //Menu footer
    menu_add_line(ent, " ", 0);

    //Items such as Gravity boots need to show the number of charges left
    if ((item->itemtype & ITEM_GRAVBOOTS) || (item->itemtype & ITEM_FIRE_RESIST) ||
        (item->itemtype & ITEM_AUTO_TBALL)) {
        menu_add_line(ent, va(" Charges left: %d", item->quantity), 0);
        menu_add_line(ent, " ", 0);
        linecount += 2;
    }

    ent->client->menustorage.currentline = linecount;
}

//************************************************************************************************
//		**ITEM DISPLAY MENU**
//************************************************************************************************
void OpenSellConfirmMenu(edict_t *ent, int itemindex); // zar: armory.c

void ShowItemMenu_handler(edict_t *ent, int option) {
    if (option - 14999 > 0) {
        OpenSellConfirmMenu(ent, option - 15000);
    } else if (option - 9999 > 0) {
        // stash
        vrx_stash_store(ent, option - 10000);
    } else if (option - 7777 > 0) {
        //Previous menu
        ShowInventoryMenu(ent, option - 7777, false);
        return;
    } else if (option - 4444 > 0) {
        //Equip/Unequip item
        V_EquipItem(ent, option - 4445);
    } else {
        //Closing menu
        menu_close(ent, true);
        return;
    }
}

//************************************************************************************************
//************************************************************************************************

void ShowItemMenu(edict_t *ent, int itemindex) {
    item_t *item = &ent->myskills.items[itemindex];

    //Load the item
    StartShowInventoryMenu(ent, item);

    //Check to see if this item can be equipped or not
    if (!((item->itemtype & ITEM_GRAVBOOTS) || (item->itemtype & ITEM_FIRE_RESIST) ||
          (item->itemtype & ITEM_AUTO_TBALL))) {
        if (itemindex < 3) menu_add_line(ent, "Unequip this item", 4445 + itemindex);
        else menu_add_line(ent, "Equip this item", 4445 + itemindex);
        ++ent->client->menustorage.currentline;
    }

    //Append a footer to the menu
    menu_add_line(ent, "Previous menu", 7778 + itemindex);
    menu_add_line(ent, "Exit", 666);
    menu_add_line(ent, " ", 0);

    qboolean hasStash = strlen(ent->myskills.owner) > 0 || strlen(ent->myskills.email) > 0;
    if (hasStash && itemindex >= 3)
		menu_add_line(ent, "Stash this item", 10000 + itemindex);
    menu_add_line(ent, "Sell this item", 15000 + itemindex);

    ent->client->menustorage.currentline += 2;

    menu_set_handler(ent, ShowItemMenu_handler);
    menu_show(ent);
}

//************************************************************************************************
//		**MAIN MENU**
//************************************************************************************************

void ShowInventoryMenu_handler(edict_t *ent, int option) {
    if ((option > 0) && (option - 1 < MAX_VRXITEMS) && (ent->myskills.items[option - 1].itemtype != ITEM_NONE)) {
        //Use the consumable items (health potions, holy water)
        if ((ent->myskills.items[option - 1].itemtype == ITEM_POTION) ||
            (ent->myskills.items[option - 1].itemtype == ITEM_ANTIDOTE)) {
            cmd_Drink(ent, ent->myskills.items[option - 1].itemtype, option);
            return;
        }

        //View selected item
        ShowItemMenu(ent, option - 1);
    } else if (option == 666) {
        menu_close(ent, true);
        return;
    }

    //refresh the menu
    ShowInventoryMenu(ent, option, false);
}

//************************************************************************************************

lva_result_t vrx_get_item_menu_line(item_t* item)
{
	item_menu_t fmt = vrx_menu_item_display(item);//, ' ');
	if (fmt.num >= 0) {
		char* abbr = "";
		if (item->itemtype == ITEM_COMBO)
			abbr = "CO";
		else if (item->itemtype == ITEM_ABILITY)
			abbr = "AB";
		else if (item->itemtype == ITEM_WEAPON)
			abbr = "WE";

        return lva("%-15.15s %2s %2d/%2d", fmt.str, abbr, fmt.num, item->itemLevel);
	} else
	{
        return lva("%s", fmt.str);
	}
}

void ShowInventoryMenu(edict_t *ent, int lastline, qboolean selling) {
    int i;

    //Usual menu stuff
    if (!menu_can_show(ent))
        return;
    menu_clear(ent);

    //Print header
    menu_add_line(ent, va("%s's items", ent->client->pers.netname), MENU_GREEN_CENTERED);
    menu_add_line(ent, " ", 0);

    //Print each item
    for (i = 0; i < MAX_VRXITEMS; ++i) {
        item_t *item;
        item = &ent->myskills.items[i];

        //Print equip slot (if required)
        switch (i) {
            case 0:
                menu_add_line(ent, "Hand", MENU_GREEN_LEFT);
                break;
            case 1:
                menu_add_line(ent, "Neck", MENU_GREEN_LEFT);
                break;
            case 2:
                menu_add_line(ent, "Belt", MENU_GREEN_LEFT);
                break;
            case 3:
                menu_add_line(ent, " ", MENU_GREEN_LEFT);
                break;
        }

        lva_result_t s = vrx_get_item_menu_line(item);
        menu_add_line(ent, s.str, i + 1);
    }

    //Menu footer
    menu_add_line(ent, " ", 0);
    menu_add_line(ent, " Exit", 666);

    //Set handler
    menu_set_handler(ent, ShowInventoryMenu_handler);

    //Where are we in the menu?
    if (lastline) {
        switch (lastline) {
            case 1:
                ent->client->menustorage.currentline = 4;
                break;
            case 2:
                ent->client->menustorage.currentline = 6;
                break;
            case 3:
                ent->client->menustorage.currentline = 8;
                break;
            default:
                ent->client->menustorage.currentline = 6 + lastline;
                break;    //start at line 10
        }
    } else ent->client->menustorage.currentline = 18;

    //Display the menu (don't show it if this menu was loaded from the armory!)
    if (!selling) menu_show(ent);
}
