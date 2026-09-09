#include "g_local.h"
#include "characters/io/v_characterio.h"
#include "armory.h"

armoryRune_t WeaponRunes[20];
armoryRune_t AbilityRunes[20];
armoryRune_t ComboRunes[20];



void vrx_armory_open_category_menu(edict_t *ent, int option);

void OpenBuyRuneMenu(edict_t *ent, int page_num, int lastline);

void OpenSellMenu(edict_t *ent, int lastline);

void Cmd_Armory_f(edict_t *ent, const struct armorycategory_s* cat, const struct armoryitem_s* item);


//************************************************************************************************
// Buy stuff
//************************************************************************************************

int getBuyValue(const item_t *rune) {
    int count = 0;

    if (rune->itemtype & ITEM_UNIQUE)
        return ARMORY_RUNE_UNIQUE_PRICE;

    for (int i = 0; i < MAX_VRXITEMMODS; ++i) {
        switch (rune->modifiers[i].type) {
            case TYPE_ABILITY: count += ARMORY_RUNE_APOINT_PRICE * rune->modifiers[i].value;
                break;
            case TYPE_WEAPON: count += ARMORY_RUNE_WPOINT_PRICE * rune->modifiers[i].value;
                break;
        }
    }
    return count;
}

//************************************************************************************************
// Sell stuff
//************************************************************************************************

int GetSellValue(const item_t *item) {
    //Uniques have no value
    if (item->itemtype & ITEM_UNIQUE)
        return 1;

    //Standard items have a sell value
    switch (item->itemtype) {
        case ITEM_POTION: return (float) item->quantity / (float) ARMORY_QTY_POTIONS * (
                                     (float) ARMORY_PRICE_POTIONS / 1.5);
        case ITEM_ANTIDOTE: return (float) item->quantity / (float) ARMORY_QTY_ANTIDOTES * (
                                       (float) ARMORY_PRICE_ANTIDOTES / 1.5);
        case ITEM_GRAVBOOTS: return (float) item->quantity / (float) ARMORY_QTY_GRAVITYBOOTS * (
                                        (float) ARMORY_PRICE_GRAVITYBOOTS / 1.5);
        case ITEM_FIRE_RESIST: return (float) item->quantity / (float) ARMORY_QTY_FIRE_RESIST * (
                                          (float) ARMORY_PRICE_FIRE_RESIST / 1.5);
        case ITEM_AUTO_TBALL: return (float) item->quantity / (float) ARMORY_QTY_AUTO_TBALL * (
                                         (float) ARMORY_PRICE_AUTO_TBALL / 1.5);

        //runes
        case ITEM_COMBO: return item->itemLevel * 250;
        case ITEM_CLASSRUNE: return item->itemLevel * 500;
        case ITEM_WEAPON:
        case ITEM_ABILITY:
            //other items have a value based on item level
        default: return item->itemLevel * 125;
    }
}

//************************************************************************************************
// Special items stuff
//************************************************************************************************

void GiveRuneToArmory(item_t *rune) {
    armoryRune_t *firstItem;
    const int type = rune->itemtype;
    const int newPrice = getBuyValue(rune);
    int i;

    //discard the rune if it's worthless
    if (newPrice == 0)
        return;


    //remove the unique flag if it's there
    // az: Why?
    //	if (type & ITEM_UNIQUE)
    //		type ^= ITEM_UNIQUE;

    //select the correct item list
    switch (type) {
        case ITEM_WEAPON: firstItem = WeaponRunes;
            break;
        case ITEM_ABILITY: firstItem = AbilityRunes;
            break;
        case ITEM_COMBO: firstItem = ComboRunes;
            break;
        default: return; //The armory only sells the above items
    }

    item_t *slot = nullptr;

    //find an empty slot
    for (i = 0; i < ARMORY_MAX_RUNES; ++i) {
        item_t *_slot = &(firstItem + i)->rune;
        if (_slot->itemtype == TYPE_NONE) {
            slot = _slot;
            break;
        }
    }

    //if there is no empty slot, replace the slot with the
    //cheapest rune in it.
    if (slot == nullptr) {
        slot = &firstItem->rune;

        for (i = 1; i < ARMORY_MAX_RUNES; ++i) {
            item_t *check = &(firstItem + i)->rune;
            if (getBuyValue(check) < getBuyValue(slot) || check->itemLevel < slot->itemLevel)
                slot = check;
        }
        if (getBuyValue(slot) > newPrice || slot->itemLevel > rune->itemLevel)
            slot = nullptr;
    }

    //If we found a place for this rune, add it!
    if (slot != nullptr) {
        vrx_item_copy(rune, slot);
        gi.dprintf("Item sold to armory. Price = %d.\n", newPrice);
        SaveArmory();
    }
    //else item is discarded.
}

struct transaction_s {
    struct armoryitem_s* item;
    struct armorycategory_s* category;
    int price;
};

void vrx_armory_transact(edict_t* ent, const struct transaction_s *trans) {
    bool bought = false;
    if (trans->item->callback) {
        bought = trans->item->callback(ent, trans->item->buydata);
    } else {
        if (trans->category->default_buy_callback)
            bought = trans->category->default_buy_callback(ent, trans->item->buydata);
        else
            gi.error("No buy callback for item '%s' in category '%s'", trans->item->name, trans->category->name);
    }

    //spend the credits
    if (bought) {
        ent->myskills.credits -= trans->price;
        safe_cprintf(ent, PRINT_HIGH, "Spent %dcr. %d credits left. \n", trans->price, ent->myskills.credits);
        gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/gold.wav"), 1, ATTN_NORM, 0);
    }
}


// ReSharper disable once CppParameterMayBeConstPtrOrRef
void free_transaction(edict_t * ent) {
    if (ent->client->menustorage.usercontext != nullptr) {
        vrx_free(ent->client->menustorage.usercontext);
        ent->client->menustorage.usercontext = nullptr;
    }
}

static void confirmmenu_handler(edict_t* ent, const int option) {
    const struct transaction_s* transaction = ent->client->menustorage.usercontext;
    if (transaction == NULL)
        return;

    if (option == 10) {
        vrx_armory_transact(ent, transaction);
    }

    free_transaction(ent);
    menu_close(ent, false);
}

void vrx_armory_confirm_menu(edict_t *ent, struct transaction_s* transaction) {
    const auto selectionc = "";

    if (!menu_can_show(ent))
        return;

    menu_clear(ent);

    menu_add_line(ent, "Confirm Selection", MENU_GREEN_CENTERED);
    menu_add_line(ent, " ", 0);

    menu_add_line(ent, "Are you sure you ", MENU_WHITE_CENTERED);
    menu_add_line(ent, "want to buy", MENU_WHITE_CENTERED);
    menu_add_line(ent, va("%s?\n", selectionc), MENU_WHITE_CENTERED);
    menu_add_line(ent, " ", 0);
    menu_add_line(ent, " ", 0);
    menu_add_line(ent, " ", 0);
    menu_add_line(ent, " ", 0);

    menu_add_line(ent, "Yes", 10);
    const auto lastline = menu_add_line(ent, "No", 11);

    ent->client->menustorage.currentline = lastline;
    ent->client->menustorage.usercontext = transaction;
    ent->client->menustorage.onclose = free_transaction;

    menu_set_handler(ent, confirmmenu_handler);

    menu_show(ent);
}

void Cmd_Armory_f(edict_t *ent, const struct armorycategory_s *cat, const struct armoryitem_s *item) {
    if (ent->deadflag == DEAD_DEAD)
        return;

    if (!item || !cat)
        return;

    int price = 0;
    if (item->pricecallback)
        price = item->pricecallback(ent, item->pricecallbackdata);
    else {
        if (cat->default_price_callback)
            price = cat->default_price_callback(ent, item->pricecallbackdata);
        else
            gi.error("No price callback defined for item or category\n");
    }

    if (price == 0)
        return;

    const int cur_credits = ent->myskills.credits;

    if (cur_credits < price) {
        safe_cprintf(ent, PRINT_HIGH, "You need at least %d credits to buy this item.\n", price);
        return;
    }

    const auto trans = (struct transaction_s) {
        item,
        cat,
        price
    };

    if (item->needconfirmation) {
        const auto tcopy = vrx_malloc(sizeof (struct transaction_s), TAG_LEVEL);
        memcpy(tcopy, &trans, sizeof (struct transaction_s));
        vrx_armory_confirm_menu(ent, tcopy);
        return;
    }

    vrx_armory_transact(ent, &trans);
}

//************************************************************************************************
//		**ARMORY PURCHASE MENU**
//************************************************************************************************

// option layout (category items menu)
// MSB 00000000 00000000 00000000 00000000 LSB
//     item     category page     exit

constexpr auto ARMORY_ITEMS_PER_PAGE = 10;
void vrx_armory_open_category_items_menu(edict_t * ent, int option);

// handler parameters
enum purchasemenu_action {
    PURCHASE_ACTION_EXIT = 99,
    PURCHASE_ACTION_BUY = 10,
    PURCHASE_ACTION_PAGE = 11
};


enum categorymenu_action {
    CATMENU_ACTION_EXIT = 99,
    CATMENU_ACTION_PAGE = 10,
    CATMENU_ACTION_SELECT = 11
};

void purchaseitem_handler(edict_t *ent, const int option) {
    const auto armory = vrx_armory_get();
    const auto itemindex = option >> 24 & 0xFF;
    const auto catindex = option >> 16 & 0xFF;
    const auto pagenum = option >> 8 & 0xFF;
    const auto action = option & 0xFF;

    if (action == PURCHASE_ACTION_EXIT) {
        vrx_armory_open_category_menu(ent, 0);
        return;
    }

    if (action == PURCHASE_ACTION_PAGE) {
        vrx_armory_open_category_items_menu(ent, catindex << 16 | pagenum << 8);
        return;
    }

    if (action == PURCHASE_ACTION_BUY) {
        const auto cat =
            &armory->categories[catindex];
        const auto item =
            &armory->categories[catindex].items[itemindex];
        Cmd_Armory_f(ent, cat, item);
        vrx_armory_open_category_items_menu(ent, catindex << 16 | itemindex << 24 | pagenum << 8);
    }
}

void vrx_armory_open_category_items_menu(edict_t * ent, const int option) {
    const auto armory = vrx_armory_get();
    const auto itemindex = option >> 24 & 0xFF;
    const auto catindex = option >> 16 & 0xFF;
    const auto pagenum = option >> 8 & 0xFF;
    auto category = &armory->categories[catindex];

    int lastline = -1;

    // common
    if (!menu_can_show(ent))
        return;

    menu_clear(ent);

    // header
    menu_add_line(ent, "Please select an item", MENU_GREEN_CENTERED);
    menu_add_line(ent, category->name, MENU_GREEN_CENTERED);
    menu_add_line(ent, " ", 0);

    // body

    const auto page_start = pagenum * ARMORY_ITEMS_PER_PAGE;
    const auto page_end = min(page_start + ARMORY_ITEMS_PER_PAGE, category->numitems);
    for (int itemmenuindex = page_start; itemmenuindex < page_end; itemmenuindex++) {
        const auto items = category->items;
        const auto item = &items[itemmenuindex];
        auto price = -1;

        if (item->pricecallback)
            price = item->pricecallback(ent, item->pricecallbackdata);
        else {
            if (category->default_price_callback)
                price = category->default_price_callback(ent, item->pricecallbackdata);
            else
                gi.error(ent, "No price callback available for item '%s'", item->name);
        }

        const auto fmt = va("%-16s %5dcr", item->name, price);
        const auto index = menu_add_line(ent, fmt, (itemmenuindex << 24 | catindex << 16 | pagenum << 8) + PURCHASE_ACTION_BUY);
        if (itemindex == itemmenuindex)
            lastline = index;
    }

    menu_add_line(ent, " ", 0);

    int nextpage_line = -1;
    int prevpage_line = -1;

    if (page_end < armory->categories[catindex].numitems)
        nextpage_line = menu_add_line(ent, "Next Page",
            ((pagenum + 1) << 8 | catindex << 16) + PURCHASE_ACTION_PAGE);
    if (page_start > 0)
        prevpage_line = menu_add_line(ent, "Previous Page",
            ((pagenum - 1) << 8 | catindex << 16) + PURCHASE_ACTION_PAGE);

    const int back_line = menu_add_line(ent, "Back", PURCHASE_ACTION_EXIT);

    menu_set_handler(ent, purchaseitem_handler);

    if (lastline != -1) {
        ent->client->menustorage.currentline = lastline;
    } else if (nextpage_line != -1) {
        ent->client->menustorage.currentline = nextpage_line;
    } else if (prevpage_line != -1) {
        ent->client->menustorage.currentline = prevpage_line;
    } else if (back_line != -1) {
        // very unlikely
        ent->client->menustorage.currentline = back_line;
    }

    menu_show(ent);
}


void categorymenu_handler(edict_t *ent, const int option) {
    const int page_num = option >> 8 & 0xFF;
    const int action = option & 0xFF;
    const int category = option >> 16 & 0xFF;

    menu_close(ent, false);

    if (action == CATMENU_ACTION_EXIT) {
        vrx_armory_open_menu(ent);
        return;
    }

    if (action == CATMENU_ACTION_SELECT) {
        vrx_armory_open_category_items_menu(ent, category << 16);
        return;
    }

    if (action == CATMENU_ACTION_PAGE) {
        vrx_armory_open_category_menu(ent, page_num << 8);
    }
}


// option layout (category menu)
// MSB 00000000 00000000 00000000 00000000 LSB
//              category page     exit

void vrx_armory_open_category_menu(edict_t *ent, const int option) {
    const uint8_t page_num = (option & 0xFF00) >> 8;

    if (!menu_can_show(ent))
        return;

    menu_clear(ent);

    //Header
    menu_add_line(ent, va("You have %d credits", ent->myskills.credits), MENU_GREEN_CENTERED);
    menu_add_line(ent, "Please make a selection:", MENU_GREEN_CENTERED);
    menu_add_line(ent, " ", 0);

    const auto armory = vrx_armory_get();
    const auto page_start = page_num * 10;
    const auto page_end = min(page_start + 10, armory->numcategories);
    for (size_t n = page_start; n < page_end; n++) {
        const auto opt = (n << 16) + CATMENU_ACTION_SELECT;
        menu_add_line(ent,  armory->categories[n].name, opt);
    }

    //Footer
    menu_add_line(ent, " ", 0);
    if (page_end < armory->numcategories)
        menu_add_line(ent, "Next", ((page_num + 1) << 8) + CATMENU_ACTION_PAGE);
    if (page_start > 0)
        menu_add_line(ent, "Back", ((page_num - 1) << 8) + CATMENU_ACTION_PAGE);

    menu_add_line(ent, "Exit", CATMENU_ACTION_EXIT);

    //Menu handler
    menu_set_handler(ent, categorymenu_handler);

    //Set the menu cursor
    ent->client->menustorage.currentline = 4;

    //Show the menu
    menu_show(ent);
}

//************************************************************************************************
//		**ITEM SELL CONFIRM MENU**
//************************************************************************************************
void ShowItemMenu_handler(edict_t *ent, int option); // item_menu.c

void vrx_reapply_items(edict_t *ent) {
    vrx_runes_unapply(ent);
    for (int i = 0; i < 3; ++i)
        vrx_runes_apply(ent, &ent->client->resp.pstats.items[i]);
}

int vrx_sell_item(edict_t *ent, item_t *slot) {
    const int value = GetSellValue(slot);

    const int wpts = V_GetRuneWeaponPts(ent, slot);
    const int apts = V_GetRuneAbilityPts(ent, slot);
    // calculate weighted total
    const int total_pts = ceil(0.5 * wpts + 0.75 * apts); //was 0.66,2.0

    if (total_pts < 30)
        GiveRuneToArmory(slot);

    memset(slot, 0, sizeof(item_t));

    //Re-apply equipment
    vrx_reapply_items(ent);

    return value;
}

void SellConfirmMenu_handler(edict_t *ent, const int option) {
    if (option - 777 > 0) {
        item_t *slot = &ent->client->resp.pstats.items[option - 778];

        //log the sale
        const int value = vrx_sell_item(ent, slot);
        vrx_write_to_logfile(ent, va("Selling rune for %d credits. [%s]", value, slot->id));
        safe_cprintf(ent, PRINT_HIGH, "Item Sold for %d credits.\n", value);

        //refund some credits
        ent->myskills.credits += value;
        safe_cprintf(ent, PRINT_HIGH, "You now have %d credits.\n", ent->myskills.credits);
        gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/gold.wav"), 1, ATTN_NORM, 0);

        //save the player file
        vrx_char_io.save_player_runes(ent);
    } else if (option - 666 > 0) {
        //Back to select sell item
        if (ent->client->menustorage.oldmenuhandler == ShowItemMenu_handler) {
            ShowInventoryMenu(ent, option - 666, false);
        } else
            OpenSellMenu(ent, option - 666);
        return;
    } else {
        //Closing menu
        menu_close(ent, true);
        return;
    }
}

//************************************************************************************************

void OpenSellConfirmMenu(edict_t *ent, const int itemindex) {
    item_t *item = &ent->client->resp.pstats.items[itemindex];

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

void SellMenu_handler(edict_t *ent, const int option) {
    //Navigating the menu?
    if (option == 666) //exit
    {
        menu_close(ent, true);
        return;
    }

    if (option < 1 || option > MAX_VRXITEMS) //exit
    {
        menu_close(ent, true);
        return;
    }

    if (ent->client->resp.pstats.items[option - 1].itemtype == ITEM_NONE) {
        //refresh the menu
        OpenSellMenu(ent, option);
        return;
    }

    if (option - 1 < 3) {
        // az: equipped slot?
        gi.cprintf(ent, PRINT_HIGH, "You can't sell an equipped rune.\n");
        return;
    }

    //We picked an item
    OpenSellConfirmMenu(ent, option - 1);
}

//************************************************************************************************

void OpenSellMenu(edict_t *ent, const int lastline) {
    //Use the item select menu and change the menu handler
    ShowInventoryMenu(ent, lastline, true);
    menu_set_handler(ent, SellMenu_handler);
    menu_show(ent);
}

//************************************************************************************************
//		**ARMORY BUY RUNE CONFIRM MENU**
//************************************************************************************************

void BuyRuneConfirmMenu_handler(edict_t *ent, const int option) {
    //Navigating the menu?
    if (option > 100) {
        const int page_num = option / 1000;
        const int selection = option % 1000 - 1;
        item_t *slot = V_FindFreeItemSlot(ent);

        armoryRune_t *firstItem;

        switch (page_num) {
            case 1: firstItem = WeaponRunes;
                break;
            case 2: firstItem = &WeaponRunes[10];
                break;
            case 3: firstItem = AbilityRunes;
                break;
            case 4: firstItem = &AbilityRunes[10];
                break;
            case 5: firstItem = ComboRunes;
                break;
            case 6: firstItem = &ComboRunes[10];
                break;
            default:
                gi.dprintf("Error in BuyRuneConfirmMenu_handler(). Invalid page number: %d\n", page_num);
                return;
        }

        item_t *rune = &(firstItem + selection)->rune;
        if (rune->itemtype == ITEM_NONE) {
            safe_cprintf(ent, PRINT_HIGH, "Sorry, someone else has purchased this rune.\n");
            menu_close(ent, true);
            return;
        }
        const int cost = getBuyValue(rune);

        //Do we have enough credits?
        if (ent->myskills.credits < cost) {
            safe_cprintf(ent, PRINT_HIGH, "You need %d more credits to buy this item.\n", cost - ent->myskills.credits);
        }
        //Check for free space
        else if (slot == NULL) {
            safe_cprintf(ent, PRINT_HIGH, "Not enough room in inventory.\n");
        } else {
            //log the purchase
            vrx_write_to_logfile(ent, va("Buying rune for %d credits. [%s]", cost, rune->id));

            //Buy it!
            V_ItemSwap(rune, slot);
            ent->myskills.credits -= cost;
            SaveArmory();
            vrx_char_io.save_player_runes(ent);

            safe_cprintf(ent, PRINT_HIGH, "Rune purchased for %d credits.\nYou have %d credits left.\n", cost,
                         ent->myskills.credits);
            gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/gold.wav"), 1, ATTN_NORM, 0);
        }

        //done
        menu_close(ent, true);
        return;
    }
    const int page_num = option / 10;
    const int selection = option % 10;

    OpenBuyRuneMenu(ent, page_num, selection);
    return;
}

//************************************************************************************************

void OpenBuyRuneConfirmMenu(edict_t *ent, const int option) {
    const int page_num = option / 1000;
    const int selection = option % 1000 - 1;

    armoryRune_t *firstItem;

    switch (page_num) {
        case 1: firstItem = WeaponRunes;
            break;
        case 2: firstItem = &WeaponRunes[10];
            break;
        case 3: firstItem = AbilityRunes;
            break;
        case 4: firstItem = &AbilityRunes[10];
            break;
        case 5: firstItem = ComboRunes;
            break;
        case 6: firstItem = &ComboRunes[10];
            break;
        default:
            gi.dprintf("Error in OpenBuyRuneConfirmMenu(). Invalid page number: %d\n", page_num);
            return;
    }

    item_t *rune = &(firstItem + selection)->rune;

    //Process the header
    StartShowInventoryMenu(ent, rune);

    //Menu footer
    menu_add_line(ent, va("  Price: %d", getBuyValue(rune)), 0);
    menu_add_line(ent, " ", 0);
    menu_add_line(ent, " ", 0);
    menu_add_line(ent, " ", 0);
    menu_add_line(ent, "  Buy this item?", 0);
    menu_add_line(ent, "No, I changed my mind!", page_num * 10 + selection);
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

void BuyRuneMenu_handler(edict_t *ent, const int option) {
    const int page_num = option / 10;
    const int page_choice = option % 10;

    if (option == 99) {
        menu_close(ent, true);
        return;
    }

    if (page_num == 1 && page_choice == 1) {
        vrx_armory_open_menu(ent);
        return;
    }
    if (page_num > 0 && page_num < 7) //5 was chosen for no real reason
    {
        if (page_choice == 2) //next
            OpenBuyRuneMenu(ent, page_num + 1, 0);
        else if (page_choice == 1) //prev
            OpenBuyRuneMenu(ent, page_num - 1, 0);
        return;
    }
    //don't cause an invalid item to be selected
    const int selection = option % 1000;

    if (selection > ARMORY_MAX_RUNES || selection < 1) {
        menu_close(ent, true);
        return;
    }

    //View it
    OpenBuyRuneConfirmMenu(ent, option);
}

//************************************************************************************************

void OpenBuyRuneMenu(edict_t *ent, const int page_num, const int lastline) {
    armoryRune_t *firstItem;
    char *category;

    //Usual menu stuff
    if (!menu_can_show(ent))
        return;
    menu_clear(ent);

    switch (page_num) {
        case 1:
            category = "a weapon rune";
            firstItem = WeaponRunes;
            break;
        case 2:
            category = "a weapon rune";
            firstItem = &WeaponRunes[10];
            break;
        case 3:
            category = "an ability rune";
            firstItem = AbilityRunes;
            break;
        case 4:
            category = "an ability rune";
            firstItem = &AbilityRunes[10];
            break;
        case 5:
            category = "a combo rune";
            firstItem = ComboRunes;
            break;
        case 6:
            category = "a combo rune";
            firstItem = &ComboRunes[10];
            break;
        default:
            gi.dprintf("Error in OpenBuyRuneMenu(). Invalid page number: %d\n", page_num);
            return;
    }

    //Header
    menu_add_line(ent, va("You have %d credits", ent->myskills.credits), MENU_GREEN_CENTERED);
    menu_add_line(ent, va("Select %s:", category), MENU_GREEN_CENTERED);
    menu_add_line(ent, " ", 0);

    //Print this page's items
    for (int i = 0; i < ARMORY_MAX_RUNES / 2; ++i) {
        item_t *rune = &(firstItem + i)->rune;
        if (rune->itemtype != ITEM_NONE) {
            const item_menu_t fmt = vrx_menu_item_display(rune); //, ' ');
            const lva_result_t txt = lva("%-13.13s %2d/%2d %5d", fmt.str, fmt.num, rune->itemLevel, getBuyValue(rune));
            menu_add_line(ent, txt.str, page_num * 1000 + i + 1);
        } else {
            switch (page_num) {
                case 1:
                case 2:
                    menu_add_line(ent, "    <Empty Weapon Slot>", 0);
                    break;
                case 3:
                case 4:
                    menu_add_line(ent, "    <Empty Ability Slot>", 0);
                    break;
                case 5:
                case 6:
                    menu_add_line(ent, "    <Empty Combo Slot>", 0);
                    break;
                default:
                    gi.error("OpenBuyRuneMenu: invalid page_num: %d", page_num);
            }
        }
    }

    //Footer
    menu_add_line(ent, " ", 0);
    if (page_num < 6) menu_add_line(ent, "Next", page_num * 10 + 2);
    menu_add_line(ent, "Back", page_num * 10 + 1);
    menu_add_line(ent, "Exit", 99);

    //Menu handler
    menu_set_handler(ent, BuyRuneMenu_handler);

    //Set the menu cursor
    if (lastline) ent->client->menustorage.currentline = 4 + lastline;
    else ent->client->menustorage.currentline = 15;

    //Show the menu
    menu_show(ent);
}

//************************************************************************************************
//		**ARMORY MAIN MENU**
//************************************************************************************************


void SellAllMenu_handler(edict_t *ent, int option) {
    int totalValue = 0;

    for (int i = 3; i < MAX_VRXITEMS; ++i)
        totalValue += vrx_sell_item(ent, &ent->client->resp.pstats.items[i]);

    //refund some credits
    ent->myskills.credits += totalValue;
    safe_cprintf(
        ent,
        PRINT_HIGH,
        "You sold your items for %d credits. You now have %d credits.\n",
        totalValue,
        ent->myskills.credits);
    gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/gold.wav"), 1, ATTN_NORM, 0);

    //save the player file
    vrx_char_io.save_player_runes(ent);
}

void OpenConfirmSellAll(edict_t *ent, int i) {
    if (!menu_can_show(ent))
        return;
    menu_clear(ent);

    menu_add_line(ent, "Confirm Selection", MENU_GREEN_CENTERED);
    menu_add_line(ent, " ", 0);

    menu_add_line(ent, "Are you sure you want to", MENU_WHITE_CENTERED);
    menu_add_line(ent, "sell all unequipped items", MENU_WHITE_CENTERED);
    menu_add_line(ent, "in your inventory?", MENU_WHITE_CENTERED);
    menu_add_line(ent, " ", 0);

    menu_add_line(ent, "Yes", 4);
    menu_add_line(ent, "No", 5);

    menu_set_handler(ent, SellAllMenu_handler);
    ent->client->menustorage.currentline = 8;
    menu_show(ent);
}

void armorymenu_handler(edict_t *ent, const int option) {
    if (option == 1)
        vrx_armory_open_category_menu(ent, 0);
    else if (option == 2)
        OpenBuyRuneMenu(ent, 1, 0);
    else if (option == 3)
        OpenSellMenu(ent, 0);
    else if (option == 4)
        OpenConfirmSellAll(ent, 0);
    else
        menu_close(ent, true);
}

//************************************************************************************************

void vrx_armory_open_menu(edict_t *ent) {
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
    menu_add_line(ent, "Sell All", 4);
    menu_add_line(ent, "Exit", 5);

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

void SaveArmory() {
    char filename[256];
    FILE *fptr;

    //get path
#if defined(_WIN32) || defined(WIN32)
    sprintf(filename, "%s\\%s", game_path->string, "settings\\ArmoryItems.dat");
#else
    sprintf(filename, "%s/%s", game_path->string, "settings/ArmoryItems.dat");
#endif

    if ((fptr = fopen(filename, "wb")) != NULL) {
        fwrite(WeaponRunes, sizeof(armoryRune_t), ARMORY_MAX_RUNES, fptr);
        fwrite(AbilityRunes, sizeof(armoryRune_t), ARMORY_MAX_RUNES, fptr);
        fwrite(ComboRunes, sizeof(armoryRune_t), ARMORY_MAX_RUNES, fptr);
        fclose(fptr);
        gi.dprintf("INFO: Vortex Rune Shop saved successfully\n");
    } else {
        gi.dprintf("Error in SaveArmory(). Error opening file: %s\n", filename);
    }
}

//************************************************************************************************

void LoadArmory() //Call this during InitGame()
{
    char filename[256];
    FILE *fptr;

    //get path
    sprintf(filename, "%s/%s", game_path->string, "settings/ArmoryItems.dat");

    if ((fptr = fopen(filename, "rb")) != NULL) {
        fread(WeaponRunes, sizeof(armoryRune_t), ARMORY_MAX_RUNES, fptr);
        fread(AbilityRunes, sizeof(armoryRune_t), ARMORY_MAX_RUNES, fptr);
        fread(ComboRunes, sizeof(armoryRune_t), ARMORY_MAX_RUNES, fptr);
        fclose(fptr);
        gi.dprintf("INFO: Vortex Rune Shop loaded successfully\n");
    } else {
        gi.dprintf("Error in LoadArmory(). Error opening file: %s\n", filename);
    }
}

//************************************************************************************************
