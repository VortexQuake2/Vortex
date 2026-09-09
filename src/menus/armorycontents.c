//
// Created by Diego Ahumada on 03-05-26.
//

#include "g_local.h"
#include "armory.h"
#include "characters/class_limits.h"
#include "characters/io/v_characterio.h"

struct weaponparam_s {
    const char *pickup;
    bool includeclip;
};

int buy_weapon_price(edict_t *ent, const void *data) {
    return ARMORY_PRICE_WEAPON;
}

bool buy_weapon(edict_t *ent, const void *data) {
    auto param = (struct weaponparam_s *) data;
    auto pickup = (const char *) param->pickup;
    const auto item = FindItem(pickup);

    assert(item);

    bool had = ent->client->pers.inventory[ITEM_INDEX(item)] > 0;

    if (had) {
        gi.cprintf(ent, PRINT_LOW, "You already have a %s.\n", item->pickup_name);
        return false;
    }

    ent->client->pers.inventory[ITEM_INDEX(item)] += 1;
    gi.cprintf(ent, PRINT_HIGH, "You bought a %s.\n", item->pickup_name);

    if (param->includeclip) {
        auto clip = FindItem(item->ammo);
        if (clip)
            V_GiveAmmoClip(ent, 1, clip->tag);
    }

    return true;
}

struct armoryitem_s weapons[] = {
    {"Shotgun", &(struct weaponparam_s){"Shotgun", false}},
    {"Super Shotgun", &(struct weaponparam_s){"Super Shotgun", false}},
    {"Machinegun", &(struct weaponparam_s){"Machinegun", false}},
    {"Chaingun", &(struct weaponparam_s){"Chaingun", false}},
    {"Grenade Launcher", &(struct weaponparam_s){"Grenade Launcher", false}},
    {"Rocket Launcher", &(struct weaponparam_s){"Rocket Launcher", false}},
    {"Hyperblaster", &(struct weaponparam_s){"Hyperblaster", false}},
    {"Railgun", &(struct weaponparam_s){"Railgun", false}},
    {"bfg10k", &(struct weaponparam_s){"bfg10k", false}},
    {"20mm Cannon", &(struct weaponparam_s){"20mm Cannon", false}},
    {"Ionripper", &(struct weaponparam_s){"Ionripper", false}},
    {"Phalanx", &(struct weaponparam_s){"Phalanx", true}},
    {"Trap", &(struct weaponparam_s){"Trap", true}},
    {"ETF Rifle", &(struct weaponparam_s){"ETF Rifle", true}},
    {"Plasma Beam", &(struct weaponparam_s){"Plasma Beam", false}},
    {"Prox Launcher", &(struct weaponparam_s){"Prox Launcher", true}},
    {"Chainfist", &(struct weaponparam_s){"Chainfist", false}},
    {"Tesla", &(struct weaponparam_s){"Tesla", true}},
    {"Disruptor", &(struct weaponparam_s){"Disruptor", true}},
};

struct ammoparam_s {
    const char *pickup;
    const size_t maxoffset;
};

int buy_ammo_price(edict_t *e, const void *data) {
    return ARMORY_PRICE_AMMO;
}

bool buy_ammo(edict_t *e, const void *data) {
    auto param = (struct ammoparam_s *) data;
    auto item = FindItem(param->pickup);

    assert(item);
    auto max = (int *) ((char *) e->client + param->maxoffset);
    auto ammo_index = ITEM_INDEX(item);
    auto amt = e->client->pers.inventory[ammo_index];
    e->client->pers.inventory[ammo_index] = *max;

    bool bought = amt < *max;
    if (!bought) {
        gi.cprintf(e, PRINT_LOW, "Your %s are maxed out.\n", item->pickup_name);
    } else {
        gi.cprintf(e, PRINT_HIGH, "You bought %d %s.\n", *max - amt, item->pickup_name);
    }

    return bought;
}

struct armoryitem_s ammo[] = {
    {"Bullets", &(struct ammoparam_s){"Bullets", CLOFS(pers.max_bullets)}},
    {"Shells", &(struct ammoparam_s){"Shells", CLOFS(pers.max_shells)}},
    {"Cells", &(struct ammoparam_s){"Cells", CLOFS(pers.max_cells)}},
    {"Grenades", &(struct ammoparam_s){"Grenades", CLOFS(pers.max_grenades)}},
    {"Rockets", &(struct ammoparam_s){"Rockets", CLOFS(pers.max_rockets)}},
    {"Slugs", &(struct ammoparam_s){"Slugs", CLOFS(pers.max_slugs)}},
    {"Flechettes", &(struct ammoparam_s){"Flechettes", CLOFS(pers.max_flechettes)}},
    {"Mag Slug", &(struct ammoparam_s){"Mag Slug", CLOFS(pers.max_magslug)}},
    {"Rounds", &(struct ammoparam_s){"Rounds", CLOFS(pers.max_disruptor)}},
};

struct consumableparam_s {
    int type;
    double *qty;
    const char* message;
};

struct consumableprice_s {
    double *price;
};

int buy_consumable_price(edict_t *ent, const void *data) {
    auto param = (struct consumableprice_s *) data;
    return *param->price;
}

bool buy_consumable(edict_t *ent, const void *data) {
    auto slot = V_FindFreeItemSlot(ent);
    if (!slot) {
        safe_cprintf(ent, PRINT_HIGH, "Not enough inventory space.\n");
        return false; // todo: message
    }

    const auto param = (struct consumableparam_s *) data;

    //4.0 Players can't buy too many stackable items
    if (V_ItemCount(ent, param->type) >= ARMORY_MAX_CONSUMABLES) {
        safe_cprintf(ent, PRINT_HIGH,
                     va("You can't buy more than %d of these items.\n", ARMORY_MAX_CONSUMABLES));
        return false;
    }

    //Give them the item
    V_ItemClear(slot);
    slot->itemLevel = 0;
    slot->itemtype = param->type;
    slot->quantity = *param->qty;

    //Tell the user what they bought
    safe_cprintf(ent, PRINT_HIGH, va("%s (%d uses)\n", param->message, (int)*param->qty));

    return true;
}

static bool buy_health(edict_t *ent, const void *data) {
    bool bought = ent->health < ent->max_health;

    ent->health = ent->max_health;
    if (bought)
        safe_cprintf(ent, PRINT_HIGH, "You bought some health.\n");
    else
        safe_cprintf(ent, PRINT_HIGH, "You are already at full health.\n");

    return bought;
}

static bool buy_armor(edict_t *ent, const void *data) {
    auto item = FindItem("Body Armor");
    auto current = &ent->client->pers.inventory[ITEM_INDEX(item)];
    auto max = MAX_ARMOR(ent);

    if (*current < max) {
        *current += 100;
        if (*current > max)
            *current = max;

        safe_cprintf(ent, PRINT_HIGH, "You bought some armor.\n");
        return true;
    }

    safe_cprintf(ent, PRINT_HIGH, "You are maxed out on armor already.\n");
    return false;
}

static bool buy_powercubes(edict_t *ent, const void *data) {
    bool bought = ent->client->pers.inventory[ITEM_INDEX(Fdi_POWERCUBE)] < MAX_POWERCUBES(ent);

    ent->client->pers.inventory[ITEM_INDEX(Fdi_POWERCUBE)] = MAX_POWERCUBES(ent);

    if (bought)
        safe_cprintf(ent, PRINT_HIGH, "You bought some power cubes.\n");
    else
        safe_cprintf(ent, PRINT_HIGH, "You are maxed out on power cubes already.\n");

    return bought;
}

static int buy_powercubes_price(edict_t *ent, const void *data) {
    return ARMORY_PRICE_POWERCUBE * (MAX_POWERCUBES(ent) - ent->client->pers.inventory[ITEM_INDEX(Fdi_POWERCUBE)]);
}

static struct armoryitem_s consumables[] = {
    {
        "Health",
        .callback = buy_health,
        .pricecallbackdata = &(struct consumableprice_s){&ARMORY_PRICE_HEALTH}
    },
    {
        "Armor",
        .callback = buy_armor,
        .pricecallbackdata = &(struct consumableprice_s){&ARMORY_PRICE_ARMOR}
    },
    {
        "Power Cubes",
        .callback = buy_powercubes,
        .pricecallback = buy_powercubes_price
    },
    {
        "Tballs",
        &(struct ammoparam_s){"tballs", CLOFS(pers.max_tballs)},
        buy_ammo,
        .pricecallbackdata = &(struct consumableprice_s){&ARMORY_PRICE_TBALLS},
    },
    {
        "Health Potions",
        &(struct consumableparam_s){ITEM_POTION, &ARMORY_QTY_POTIONS, "You bought some health potions.\n"},
        .pricecallbackdata = &(struct consumableprice_s){&ARMORY_PRICE_POTIONS},
    },
    {
        "Antidote",
        &(struct consumableparam_s){ITEM_ANTIDOTE, &ARMORY_QTY_ANTIDOTES, "You bought some vials of holy water.\n"},
        .pricecallbackdata = &(struct consumableprice_s){&ARMORY_PRICE_ANTIDOTES},
    },
    {
        "Gravity Boots",
        &(struct consumableparam_s){ITEM_GRAVBOOTS, &ARMORY_QTY_GRAVITYBOOTS, "You bought a pair of anti-gravity boots.\n"},
        .pricecallbackdata = &(struct consumableprice_s){&ARMORY_PRICE_GRAVITYBOOTS},
    },
    {
        "Bunker Gear",
        &(struct consumableparam_s){ITEM_FIRE_RESIST, &ARMORY_QTY_FIRE_RESIST,  "You bought some fire resistant clothing.\n"},
        .pricecallbackdata = &(struct consumableprice_s){&ARMORY_PRICE_FIRE_RESIST},
    },
    {
        "Auto-Tball",
        &(struct consumableparam_s){ITEM_AUTO_TBALL, &ARMORY_QTY_AUTO_TBALL, "You bought an Auto-Tball.\n"},
        .pricecallbackdata = &(struct consumableprice_s){&ARMORY_PRICE_AUTO_TBALL},
    }
};

static int buy_rune_price(edict_t *ent, [[maybe_unused]] const void *data) {
    return RUNE_COST_BASE + RUNE_COST_ADDON * ent->myskills.level;
}

static bool buy_rune(edict_t *ent, const void *data) {
    const struct consumableparam_s *param = data;
    auto slot = V_FindFreeItemSlot(ent);
    if (!slot) {
        safe_cprintf(ent, PRINT_HIGH, "Not enough inventory space!\n");
        return false;
    }

    auto rune = G_Spawn(); // create a rune
    auto runetype = param->type;

    qboolean reroll = true;

    while (reroll) {
        V_ItemClear(&rune->vrxitem); // initialize the rune

        if (runetype == ITEM_COMBO) {
            vrx_spawn_combo_rune(rune, ent->myskills.level);
        } else if (runetype) {
            vrx_spawn_normal_rune(rune, ent->myskills.level, runetype);
        } else if (random() > 0.5) {
            vrx_spawn_normal_rune(rune, ent->myskills.level, ITEM_WEAPON);
        } else {
            vrx_spawn_normal_rune(rune, ent->myskills.level, ITEM_ABILITY);
        }

        if (rune->vrxitem.itemLevel != 0)
            reroll = false;
    }

    // az: remove rune delay while buying, reapply afterwards
    auto rune_delay = ent->client->rune_delay;
    ent->client->rune_delay = level.time;
    if (Pickup_Rune(rune, ent) == false) {
        G_FreeEdict(rune);
        //gi.dprintf("WARNING: PurchaseRandomRune() was unable to spawn a rune\n");
        return false;
    }
    G_FreeEdict(rune);

    ent->client->rune_delay = rune_delay;

    //Find out what the player bought
    char buf[1024];
    strcpy(buf, GetRuneValString(slot));
    switch (slot->itemtype) {
        case ITEM_WEAPON: strcat(buf, va(" weapon rune (%d mods)", slot->numMods));
            break;
        case ITEM_ABILITY: strcat(buf, va(" ability rune (%d mods)", slot->numMods));
            break;
        case ITEM_COMBO: strcat(buf, va(" combo rune (%d mods)", slot->numMods));
            break;
    }

    //send the message to the player
    safe_cprintf(ent, PRINT_HIGH, "You bought a %s.\n", buf);

    //Save the player
    vrx_char_io.save_player_runes(ent);

    //write to the log
    int cost = buy_rune_price(ent, nullptr);
    gi.dprintf("INFO: %s purchased a level %d rune (%s).\n",
               ent->client->pers.netname, slot->itemLevel, slot->id);
    vrx_write_to_logfile(ent, va("Purchased a level %d rune (%s) for %d credits. Player has %d credits left.\n",
                                 slot->itemLevel, slot->id, cost, ent->myskills.credits));

    return true;
}

bool buy_reset(edict_t *ent, const void *data) {
    vrx_change_class(ent->client->pers.netname, ent->myskills.class_num, 2);
    return true;
}

int buy_reset_price(edict_t *ent, const void *data) {
    auto price = ARMORY_PRICE_RESET * ent->myskills.level;
    if (price > 50000)
        price = 50000;

    return price;
}

struct armoryitem_s special[] = {
    {
        "Ability Rune",
        &(struct consumableparam_s){ITEM_ABILITY},
        buy_rune,
        buy_rune_price
    },
    {
        "Weapon Rune",
        &(struct consumableparam_s){ITEM_WEAPON},
        buy_rune,
        buy_rune_price
    },
    {
        "Combo Rune",
        &(struct consumableparam_s){ITEM_COMBO},
        buy_rune,
        buy_rune_price
    },
    {
        "Character Reset",
        .callback = buy_reset,
        .pricecallback = buy_reset_price
    },
};


struct armorycategory_s armorycategories[] = {
    {
        "weapons",
        weapons,
        buy_weapon,
        buy_weapon_price,
        sizeof weapons / sizeof (struct armoryitem_s)
    },
    {
        "ammo",
        ammo,
        buy_ammo,
        buy_ammo_price,
        sizeof ammo / sizeof (struct armoryitem_s)
    },
    {
        "consumables",
        consumables,
        buy_consumable,
        buy_consumable_price,
        sizeof consumables / sizeof (struct armoryitem_s)
    },
    {
        "runes or reset",
        special,
        .numitems = sizeof special / sizeof (struct armoryitem_s)
    }
};

struct armory_s armory = {
    .categories = armorycategories,
    .numcategories = sizeof(armorycategories) / sizeof(struct armorycategory_s)
};

struct armory_s* vrx_armory_get() {
    return &armory;
}