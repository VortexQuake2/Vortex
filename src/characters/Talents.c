#include "g_local.h"
#include "menus/upgradehelp.h"

const talent_t null_talent = {0};

typedef struct {
    int talent_id;
    int max_level;
    qboolean general;
} talentdef_t;

const talentdef_t talents_general[] = {
        {-1, 0, 0}
};

const talentdef_t talents_soldier[] = {
        {TALENT_IMP_STRENGTH,  5, false},
        {TALENT_IMP_RESIST,    5, false},
        {TALENT_BLOOD_OF_ARES, 5, false},
        {TALENT_BASIC_HA,      5, false},
        {TALENT_BOMBARDIER,    5, false},
        {TALENT_BLAST_RESIST,  5, false},
   //     {TALENT_MAGMINESELF,   1, false},
   //     {TALENT_INSTANTPROXYS, 2, false},
        {-1,                   0, 0}
};

const talentdef_t talents_poltergeist[] = {
        {TALENT_MELEE_MASTERY,  5, false},
        {TALENT_MORE_AMMO,   5, false},
        {TALENT_SUPERIORITY, 5, false},
        {TALENT_RANGE_MASTERY, 5, false},
        {TALENT_PACK_ANIMAL, 5, false},
        {-1,                 0, 0}
};

const talentdef_t talents_alien[] = {
        {TALENT_SPITTING_GASSER, 5, false},
        {TALENT_SUPER_HEALER,     5, false},
        {TALENT_DEADLY_SPIKES,   5, false},
        {TALENT_SWARMING,         5, false},
        {TALENT_MAGNETISM, 5, false},
        {TALENT_TELECOON, 5, false},
        {-1,                      0, 0}
};

const talentdef_t talents_vampire[] = {
        {TALENT_IMP_CLOAK,      4, false},
        {TALENT_ARMOR_VAMP,     3, false},
        {TALENT_SECOND_CHANCE,  4, false},
        {TALENT_IMP_MINDABSORB, 4, false},
        {TALENT_CANNIBALISM,    5, false},
        {TALENT_FATAL_WOUND,    5, false},
        {-1,                    0, 0}
};

const talentdef_t talents_necromancer[] = {
        {TALENT_HELLSPAWN_MASTERY, 5, false},
        {TALENT_GOLEM_MASTERY, 5, false},
        {TALENT_CORPULENCE,     5, false},
        {TALENT_OBLATION,       5, false},
        {TALENT_AUTOCURSE,     5, false},
        //{TALENT_EVIL_CURSE,     5, false},
        {TALENT_BLACK_DEATH,    5, false},
        {-1,                    0, 0}
};

const talentdef_t talents_mage[] = {
  //     {TALENT_ICE_BOLT,      5, false},
 //       {TALENT_FROST_NOVA,    5, false},
 //       {TALENT_IMP_MAGICBOLT, 5, false},
        {TALENT_CL_STORM,      5, false},
        {TALENT_NOVA_ORB,      5, false},
        {TALENT_METEORIC_FIRE, 5, false},
        {TALENT_WIZARDRY,      5, false},
        {TALENT_MANASHIELD,    5, false},
        {TALENT_MEDITATION,    5, false},
        {TALENT_OVERLOAD,      5, false},
        {-1,                   0, 0}
};

const talentdef_t talents_shaman[] = {
        {TALENT_ICE,      5, false},
        {TALENT_WIND,     4, false},
        {TALENT_STONE,    4, false},
        {TALENT_SHADOW,   4, false},
        {TALENT_PEACE,    4, false},
        {TALENT_TOTEM,    6, false},
        {TALENT_VOLCANIC, 5, false},
        {-1,              0, 0}
};

const talentdef_t talents_engineer[] = {
        {TALENT_LASER_PLATFORM,   5, false},
        {TALENT_ALARM,            5, false},
        {TALENT_RAPID_ASSEMBLY,   5, false},
        {TALENT_PRECISION_TUNING, 5, false},
        {TALENT_STORAGE_UPGRADE,  5, false},
        {TALENT_MAGMINESELF,      1, false},
        {TALENT_INSTANTPROXYS,    2, false},
        {-1,                      0, 0}
};

const talentdef_t talents_cleric[] = {
        {TALENT_BALANCESPIRIT, 5, false},
        {TALENT_HOLY_GROUND,   5, false},
        {TALENT_UNHOLY_GROUND, 5, false},
        {TALENT_BOOMERANG,     5, false},
        {TALENT_PURGE,         5, false},
        {-1,                   0, 0}
};

const talentdef_t talents_knight[] = {
        {TALENT_REPEL,       5, false},
        {TALENT_MAG_BOOTS,   5, false},
        {TALENT_LEAP_ATTACK, 5, false},
        {TALENT_MOBILITY,    5, false},
        {TALENT_DURABILITY,  5, false},
        {-1,                 0, 0}
};

const talentdef_t talents_weaponmaster[] = {
        {TALENT_BASIC_AMMO_REGEN, 5, false},
        {TALENT_COMBAT_EXP,       5, false},
        {TALENT_TACTICS,          5, false},
        {TALENT_SIDEARMS,         5, false},
        {-1,                      0, 0}
};

typedef const talentdef_t *talentclasslist_t;

const talentclasslist_t talents_by_class[] = {
        talents_general,
        talents_soldier,
        talents_poltergeist,
        talents_vampire,
        talents_mage,
        talents_engineer,
        talents_knight,
        talents_cleric,
        talents_necromancer,
        talents_shaman,
        talents_alien,
        talents_weaponmaster
};

/**
 * Gives the player a new talent.
 * @param ent who to give the talent to
 * @param talentID talent to add
 * @param maxLevel max level of the talent
 */
void vrx_add_talent(edict_t *ent, int talentID, int maxLevel) {
    const int nextEmptySlot = ent->myskills.talents.count;
    int i = 0;

    //Don't add too many talents.
    if (nextEmptySlot >= MAX_TALENTS)
        return;

    //Don't add a talent more than once.
    for (i = 0; i < nextEmptySlot; ++i)
        if (ent->myskills.talents.talent[nextEmptySlot].id == talentID)
            return;

    ent->myskills.talents.talent[nextEmptySlot].id = talentID;
    ent->myskills.talents.talent[nextEmptySlot].maxLevel = maxLevel;
    ent->myskills.talents.talent[nextEmptySlot].upgradeLevel = 0;        //Just in case it's not already zero.

    ent->myskills.talents.count++;
}

/// vrx_remove_talent
/// \param ent player to remove talent from
/// \param talentID talent id to remove
/// \return number of points the talent was upgraded
int vrx_remove_talent(edict_t *ent, int talentID) {
    const int count = ent->myskills.talents.count;
    int ret = 0;

    for (int i = 0; i < count; i++) {
        const talent_t *player_talent = &ent->myskills.talents.talent[i];
        if (player_talent->id != talentID) {
            continue;
        }

        // get how much we need to refund
        ret += player_talent->upgradeLevel;

        // move all that follow back
        for (int j = i; j < count; j++) {
            // there is not a talent that follows this one
            if (j == MAX_TALENTS - 1 || // array limit
                (j + 1) == count) // next one is outside
            {
                ent->myskills.talents.talent[j] = null_talent;

                continue;
            }

            // a talent follows this one, so copy it over
            ent->myskills.talents.talent[j] = ent->myskills.talents.talent[j + 1];
        }

        // that's all folks
        ent->myskills.talents.count--;
        break;
    }

    return ret;
}

//Adds all the required talents for said class.
void vrx_set_talents(edict_t *ent) {
    const talentdef_t *first = talents_by_class[ent->myskills.class_num];

    while (first->talent_id != -1) {
        vrx_add_talent(ent, first->talent_id, first->max_level);
        first++;
    }
}

//Erases all talent information.
void vrx_clear_talents(edict_t *ent) {
    memset(&ent->myskills.talents, 0, sizeof(talentlist_t));
}

//Returns the talent slot with matching talentID.
//Returns -1 if there is no matching talent.
int vrx_get_talent_slot(const edict_t *ent, int talentID) {
    int i;
    int num;

    //Make sure the ent is valid
    if (!ent) {
        WriteServerMsg(va("vrx_get_talent_slot() called with a NULL entity. talentID = %d", talentID), "CRITICAL ERROR",
                       true, true);
        return -1;
    }

    //Make sure we are a player
    if (!ent->client) {
        //gi.dprintf(va("WARNING: vrx_get_talent_slot() called with a non-player entity! talentID = %d\n", talentID));
        return -1;
    }

    num = ent->myskills.talents.count;

    if (num < 5)
        num = 5;

    for (i = 0; i < ent->myskills.talents.count; ++i) {
        if (ent->myskills.talents.talent[i].id == talentID)
            return i;
    }
    return -1;
}

//Returns the talent upgrade level matching talentID.
//Returns -1 if there is no matching talent.
int vrx_get_talent_level(const edict_t *ent, int talentID) {
    int slot = vrx_get_talent_slot(ent, talentID);

    if (slot < 0) {
        if (!ent->client) { // so it's a morphed player?
            if (ent->owner && ent->owner->inuse && ent->owner->client) {
                slot = vrx_get_talent_slot(ent->owner, talentID);
                ent = ent->owner;
            } else if (ent->activator && ent->activator->inuse && ent->activator->client) {
                slot = vrx_get_talent_slot(ent->activator, talentID);
                ent = ent->activator;
            }
        }

        if (slot < 0) // still doesn't exist? k
            return 0;
    } //;//-1;


    return ent->myskills.talents.talent[slot].upgradeLevel;
}

//Upgrades the talent with a matching talentID
void vrx_upgrade_talent(edict_t *ent, int talentID) {
    const int slot = vrx_get_talent_slot(ent, talentID);
    talent_t *talent;

    if (slot == -1)
        return;

    talent = &ent->myskills.talents.talent[slot];

    // check for conflicting talents
    if (talentID == TALENT_RAPID_ASSEMBLY && vrx_get_talent_level(ent, TALENT_PRECISION_TUNING) > 0) {
        safe_cprintf(ent, PRINT_HIGH, "Rapid Assembly can't be combined with Precision Tuning.\n");
        return;
    }
    if (talentID == TALENT_PRECISION_TUNING && vrx_get_talent_level(ent, TALENT_RAPID_ASSEMBLY) > 0) {
        safe_cprintf(ent, PRINT_HIGH, "Precision Tuning can't be combined with Rapid Assembly.\n");
        return;
    }
    if (talentID == TALENT_CORPULENCE && vrx_get_talent_level(ent, TALENT_OBLATION) > 0) {
        safe_cprintf(ent, PRINT_HIGH, "Corpulence can't be combined with Oblation.\n");
        return;
    }
    if (talentID == TALENT_OBLATION && vrx_get_talent_level(ent, TALENT_CORPULENCE) > 0) {
        safe_cprintf(ent, PRINT_HIGH, "Oblation can't be combined with Corpulence.\n");
        return;
    }

    if (talentID == TALENT_IMP_RESIST && vrx_get_talent_level(ent, TALENT_IMP_STRENGTH) > 0) {
        safe_cprintf(ent, PRINT_HIGH, "Improved Resist can't be combined with Improved Strength.\n");
        return;
    }

    if (talentID == TALENT_IMP_STRENGTH && vrx_get_talent_level(ent, TALENT_IMP_RESIST) > 0) {
        safe_cprintf(ent, PRINT_HIGH, "Improved Strength can't be combined with Improved Resist.\n");
        return;
    }

    if (talent->upgradeLevel == talent->maxLevel) {
        safe_cprintf(ent, PRINT_HIGH, "You can not upgrade this talent any further.\n");
        return;
    }
    if (ent->myskills.talents.talentPoints < 1) {
        safe_cprintf(ent, PRINT_HIGH, "You do not have enough talent points.\n");
        return;
    }

    //We can upgrade.
    talent->upgradeLevel++;
    ent->myskills.talents.talentPoints--;
    safe_cprintf(ent, PRINT_HIGH, va("%s upgraded to level %d/%d.\n", GetTalentString(talent->id), talent->upgradeLevel,
                                     talent->maxLevel));
    safe_cprintf(ent, PRINT_HIGH, va("Talent points remaining: %d\n", ent->myskills.talents.talentPoints));
    //savePlayer(ent);
}

//****************************************
//************* Talent Menus *************
//****************************************
void vrx_open_talent_menu(edict_t* ent, int talentID, qboolean select_upgrade);
void TalentUpgradeMenu_handler(edict_t *ent, int option) {
    //Not upgrading
    if (option > 0) {
        OpenTalentUpgradeMenu(ent, vrx_get_talent_slot(ent, option - 1) + 1);
    } else    //upgrading
    {
        const int talentID = (option * -1) - 1;
        // upgrade the talent
        vrx_upgrade_talent(ent, talentID);
        // refresh the menu
        vrx_open_talent_menu(ent, talentID, true);
    }
}


void vrx_open_talent_menu(edict_t *ent, int talentID, qboolean select_upgrade) {
    talent_t* talent;// = &ent->myskills.talents.talent[vrx_get_talent_slot(ent, talentID)];
    int level;// = talent->upgradeLevel;
    const int slot = vrx_get_talent_slot(ent, talentID);
    const int talentPoints = ent->myskills.talents.talentPoints;
    int lineCount = 7;//12;
    qboolean can_upgrade = false;

    // check for invalid talent index
    if (slot == -1)
        return;
        
    if (!menu_can_show(ent))
        return;
               
    talent = &ent->myskills.talents.talent[slot];
    level = talent->upgradeLevel;
    menu_clear(ent);
    
    menu_add_line(ent, "Talent", MENU_GREEN_CENTERED);
    menu_add_line(ent, va("%s: %d/%d", GetTalentString(talentID), level, talent->maxLevel), MENU_WHITE_CENTERED);
    menu_add_line(ent, " ", 0);

    const auto help = vrx_talenthelp_get(talentID);
    lineCount += vrx_upgradehelp_add_menu_lines(ent, help);

    menu_add_line(ent, " ", 0);
    //menu_add_line(ent, "Current", MENU_GREEN_CENTERED);
    //writeTalentUpgrade(ent, talentID, level);
    menu_add_line(ent, " ", 0);

    if (talent->upgradeLevel < talent->maxLevel && talentPoints)
    {
        can_upgrade = true;
        menu_add_line(ent, "Upgrade this talent.", -1 * (talentID + 1));
    }
    else menu_add_line(ent, " ", 0);

    menu_add_line(ent, "Previous menu.", talentID + 1);

    menu_set_handler(ent, TalentUpgradeMenu_handler);
    if (select_upgrade && can_upgrade)
        ent->client->menustorage.currentline = lineCount-1;
    else
        ent->client->menustorage.currentline = lineCount;
    menu_show(ent);
}

//****************************************
//*********** Main Talent Menu ***********
//****************************************

void openTalentMenu_handler(edict_t *ent, int option) {
    switch (option) {
        case 9999:    //Exit
        {
            menu_close(ent, true);
            return;
        }
        default:
            vrx_open_talent_menu(ent, option, false);
    }
}

void OpenTalentUpgradeMenu(edict_t *ent, int lastline) {
    talent_t *talent;
    char buffer[30];
    int i;

    if (!menu_can_show(ent))
        return;
    menu_clear(ent);

    // menu header
    menu_add_line(ent, "Talents", MENU_GREEN_CENTERED);
    menu_add_line(ent, " ", 0);

    for (i = 0; i < ent->myskills.talents.count; i++) {
        talent = &ent->myskills.talents.talent[i];

        //create menu string
        strcpy(buffer, GetTalentString(talent->id));
        strcat(buffer, ":");
        padRight(buffer, 15);

        menu_add_line(ent, va("%d. %s %d/%d", i + 1, buffer, talent->upgradeLevel, talent->maxLevel), talent->id);
    }

    // menu footer
    menu_add_line(ent, " ", 0);
    menu_add_line(ent, va("You have %d talent points.", ent->myskills.talents.talentPoints), 0);
    menu_add_line(ent, " ", 0);

    menu_add_line(ent, "Exit", 9999);
    menu_set_handler(ent, openTalentMenu_handler);

    if (!lastline) ent->client->menustorage.currentline = ent->myskills.talents.count + 6;
    else ent->client->menustorage.currentline = lastline + 2;

    menu_show(ent);

    // try to shortcut to chat-protect mode
    if (ent->client->idle_frames < qf2sf(CHAT_PROTECT_FRAMES - 51))
        ent->client->idle_frames = qf2sf(CHAT_PROTECT_FRAMES - 51);
}

void V_UpdatePlayerTalents(edict_t *ent) {
    if (!ent) return;
    if (!ent->client) return;
    if (ent->myskills.class_num <= CLASS_NULL || ent->myskills.class_num >= CLASS_MAX) {
        gi.dprintf("warning: invalid class for %s\n", ent->client->pers.netname);
        return;
    }

    int refunded = 0;

    // see differences between class talents and player talents
    for (int i = 0; i < ent->myskills.talents.count; ++i) {
        const int talentId = ent->myskills.talents.talent[i].id;
        talent_t *player_talent = &ent->myskills.talents.talent[i];
        const talentdef_t *class_talent = NULL;

        for (const talentdef_t* talent = talents_by_class[ent->myskills.class_num];
             talent->talent_id != -1;
             talent++) {
            if (talent->talent_id == talentId) {
                class_talent = talent;
                break;
            }
        }

        // not found in class
        if (class_talent == NULL) {
            // remove it
            refunded += vrx_remove_talent(ent, talentId);
            i = -1; // start over
            continue;
        }

        // found in class

        // max level changed
        if (class_talent->max_level != player_talent->maxLevel) {
            player_talent->maxLevel = class_talent->max_level;

            // upgrade level past max level
            if (player_talent->upgradeLevel > player_talent->maxLevel) {
                const int difference = player_talent->upgradeLevel - player_talent->maxLevel;
                player_talent->upgradeLevel -= difference;
                refunded += difference;
            }
        }
    }

    // check talents missing from the player
    for (const talentdef_t* talent = talents_by_class[ent->myskills.class_num];
         talent->talent_id != -1;
         talent++) {
        const int talentLevel = vrx_get_talent_slot(ent, talent->talent_id);
        if (talentLevel == -1) { // not found
            vrx_add_talent(ent, talent->talent_id, talent->max_level);
        }
    }

    if (refunded) {
        ent->myskills.talents.talentPoints += refunded;
        gi.cprintf(ent, PRINT_HIGH, "%d talent points were refunded.\n", refunded);
    }
}