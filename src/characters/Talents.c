#include "g_local.h"

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
        {TALENT_MORPHING,    5, false},
        {TALENT_MORE_AMMO,   5, false},
        {TALENT_SUPERIORITY, 5, false},
        {TALENT_RETALIATION, 5, false},
        {TALENT_PACK_ANIMAL, 5, false},
        {-1,                 0, 0}
};

const talentdef_t talents_alien[] = {
        {TALENT_PHANTOM_OBSTACLE, 5, false},
        {TALENT_SUPER_HEALER,     5, false},
        {TALENT_PHANTOM_COCOON,   5, false},
        {TALENT_SWARMING,         5, false},
        {TALENT_EXPLODING_BODIES, 5, false},
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
        {TALENT_CHEAPER_CURSES, 5, false},
        {TALENT_CORPULENCE,     5, false},
        {TALENT_LIFE_TAP,       5, false},
        {TALENT_DIM_VISION,     5, false},
        {TALENT_EVIL_CURSE,     5, false},
        {TALENT_FLIGHT,         5, false},
        {-1,                    0, 0}
};

const talentdef_t talents_mage[] = {
        {TALENT_ICE_BOLT,      5, false},
        {TALENT_FROST_NOVA,    5, false},
        {TALENT_IMP_MAGICBOLT, 5, false},
        {TALENT_MANASHIELD,    5, false},
        {TALENT_MEDITATION,    5, false},
        {TALENT_OVERLOAD,      5, false},
        {-1,                   0, 0}
};

const talentdef_t talents_shaman[] = {
        {TALENT_ICE,      4, false},
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
        {TALENT_TACTICS,          3, false},
        {TALENT_SIDEARMS,         3, false},
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
    int nextEmptySlot = ent->myskills.talents.count;
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
    int count = ent->myskills.talents.count;
    int ret = 0;

    for (int i = 0; i < count; i++) {
        talent_t *player_talent = &ent->myskills.talents.talent[i];
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
    int slot = vrx_get_talent_slot(ent, talentID);
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
    if (talentID == TALENT_CORPULENCE && vrx_get_talent_level(ent, TALENT_LIFE_TAP) > 0) {
        safe_cprintf(ent, PRINT_HIGH, "Corpulence can't be combined with Life Tap.\n");
        return;
    }
    if (talentID == TALENT_LIFE_TAP && vrx_get_talent_level(ent, TALENT_CORPULENCE) > 0) {
        safe_cprintf(ent, PRINT_HIGH, "Life Tap can't be combined with Corpulence.\n");
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
        int talentID = (option * -1) - 1;
        // upgrade the talent
        vrx_upgrade_talent(ent, talentID);
        // refresh the menu
        vrx_open_talent_menu(ent, talentID, true);
    }
}

//Adds menu lines that describe the general use of the talent.
int writeTalentDescription(edict_t *ent, int talentID) {
    switch (talentID) {
        //Soldier talents
        case TALENT_IMP_STRENGTH:
            addlinetomenu(ent, "Increases damage,", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "but reduces resist.", MENU_WHITE_CENTERED);
            return 2;
        case TALENT_IMP_RESIST:
            addlinetomenu(ent, "Increases resist,", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "but reduces damage.", MENU_WHITE_CENTERED);
            return 2;
        case TALENT_BLOOD_OF_ARES:
            addlinetomenu(ent, "Increases the damage you", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "give/take as you spree.", MENU_WHITE_CENTERED);
            return 2;
        case TALENT_BASIC_HA:
            addlinetomenu(ent, "Increases ammo pickups.", MENU_WHITE_CENTERED);
            return 1;
        case TALENT_BOMBARDIER:
            addlinetomenu(ent, "Reduces self-inflicted", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "grenade damage and", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "reduces cost.", MENU_WHITE_CENTERED);
            return 3;
            //Poltergeist talents
        case TALENT_MORPHING:
            addlinetomenu(ent, "Reduces the cost", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "of your morphs.", MENU_WHITE_CENTERED);
            return 2;
        case TALENT_MORE_AMMO:
            addlinetomenu(ent, "Increases maximum ammo", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "capacity for", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "tank/caco/flyer/medic.", MENU_WHITE_CENTERED);
            return 3;
        case TALENT_SUPERIORITY:
            addlinetomenu(ent, "Increased damage and", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "resistance to monsters.", MENU_WHITE_CENTERED);
            return 2;
        case TALENT_RETALIATION:
            addlinetomenu(ent, "Damage increases as", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "health is reduced.", MENU_WHITE_CENTERED);
            return 2;
        case TALENT_PACK_ANIMAL:
            addlinetomenu(ent, "Increased damage and", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "resistance when near", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "friendly morphed", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "players.", MENU_WHITE_CENTERED);
            return 4;
            //Vampire talents
        case TALENT_IMP_CLOAK:
            addlinetomenu(ent, "Move while cloaked", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "(must be crouching).", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "1/3 pc cost at night!", MENU_WHITE_CENTERED);
            return 3;
        case TALENT_ARMOR_VAMP:
            addlinetomenu(ent, "Also gain armor using", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "your vampire skill.", MENU_WHITE_CENTERED);
            return 2;
        case TALENT_FATAL_WOUND:
            addlinetomenu(ent, "Adds chance for flesh", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "eater to make the", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "victim bleed out.", MENU_WHITE_CENTERED);
            return 3;
        case TALENT_SECOND_CHANCE:
            addlinetomenu(ent, "100% chance of ghost", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "working when hit.", MENU_WHITE_CENTERED);
            return 2;
        case TALENT_IMP_MINDABSORB:
            addlinetomenu(ent, "Increases frequency of", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "mind absorb attacks.", MENU_WHITE_CENTERED);
            return 2;
        case TALENT_CANNIBALISM:
            addlinetomenu(ent, "Increases your maximum", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "health using corpse eater.", MENU_WHITE_CENTERED);
            return 2;
            //Mage talents
        case TALENT_ICE_BOLT:
            addlinetomenu(ent, "Use 'icebolt' instead of", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "fireball to chill targets.", MENU_WHITE_CENTERED);
            return 2;
        case TALENT_MEDITATION:
            addlinetomenu(ent, "Recharge your power", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "cubes at a whim (cmd '+manacharge').", MENU_WHITE_CENTERED);
            return 2;
        case TALENT_OVERLOAD:
            addlinetomenu(ent, "Use extra power cubes", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "to overload abilities,", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "increasing their", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "effectiveness", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "(cmd 'overload').", MENU_WHITE_CENTERED);
            return 5;
        case TALENT_FROST_NOVA:
            addlinetomenu(ent, "Special nova spell", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "that chills players.", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "(cmd frostnova)", MENU_WHITE_CENTERED);
            return 3;
        case TALENT_IMP_MAGICBOLT:
            addlinetomenu(ent, "Power cubes are refunded", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "on successful hits.", MENU_WHITE_CENTERED);
            return 2;
        case TALENT_MANASHIELD:
            addlinetomenu(ent, "Reduces physical damage", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "by 80%%. All damage", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "absorbed consumes power", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "cubes. (cmd manashield)", MENU_WHITE_CENTERED);
            return 4;
            //Engineer talents
        case TALENT_LASER_PLATFORM:
            addlinetomenu(ent, "Create a laser platform", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "(cmd 'laserplatform').", MENU_WHITE_CENTERED);
            return 2;
        case TALENT_ALARM:
            addlinetomenu(ent, "Use 'lasertrap' instead", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "of detector to build a", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "laser trap.", MENU_WHITE_CENTERED);
            return 3;
        case TALENT_RAPID_ASSEMBLY:
            addlinetomenu(ent, "Reduces build time.", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "Can't be combined with", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "Precision Tune.", MENU_WHITE_CENTERED);
            return 3;
        case TALENT_PRECISION_TUNING:
            addlinetomenu(ent, "Increased cost and", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "build time to build", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "higher level devices.", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "Can't be combined with", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "Rapid Assembly.", MENU_WHITE_CENTERED);
            return 3;
        case TALENT_STORAGE_UPGRADE:
            addlinetomenu(ent, "Increases ammunition", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "capacity of SS/sentry/AC.", MENU_WHITE_CENTERED);
            return 2;
            //Knight talents
        case TALENT_REPEL:
            addlinetomenu(ent, "Adds chance for projectiles", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "to deflect from shield.", MENU_WHITE_CENTERED);
            return 2;
        case TALENT_MAG_BOOTS:
            addlinetomenu(ent, "Reduces effect of knockback.", MENU_WHITE_CENTERED);
            return 1;
        case TALENT_LEAP_ATTACK:
            addlinetomenu(ent, "Adds stun/knockback effect", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "to boost spell when landing.", MENU_WHITE_CENTERED);
            return 2;
        case TALENT_MOBILITY:
            addlinetomenu(ent, "Reduces boost cooldown", MENU_WHITE_CENTERED);
            return 1;
        case TALENT_DURABILITY:
            addlinetomenu(ent, "Increases your health", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "per level bonus!", MENU_WHITE_CENTERED);
            return 2;
            //Cleric talents
        case TALENT_BALANCESPIRIT:
            addlinetomenu(ent, "New spirit that can", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "use the skills of both", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "yin and yang spirits.", MENU_WHITE_CENTERED);
            return 3;
        case TALENT_HOLY_GROUND:
            addlinetomenu(ent, "Designate an area as", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "holy ground to regenerate", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "teammates (cmd 'holyground').", MENU_WHITE_CENTERED);
            return 3;
        case TALENT_UNHOLY_GROUND:
            addlinetomenu(ent, "Designate an area as", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "unholy ground to damage", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "enemies (cmd 'unholyground').", MENU_WHITE_CENTERED);
            return 3;
        case TALENT_BOOMERANG:
            addlinetomenu(ent, "Turns blessed hammers", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "into boomerangs", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "(cmd 'boomerang').", MENU_WHITE_CENTERED);
            return 3;
        case TALENT_PURGE:
            addlinetomenu(ent, "Removes curses and grants", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "temporary invincibility", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "and immunity to curses", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "(cmd 'purge').", MENU_WHITE_CENTERED);
            return 4;
            //Weaponmaster talents
        case TALENT_BASIC_AMMO_REGEN:
            addlinetomenu(ent, "Basic ammo regeneration.", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "Regenerates one ammo pack", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "for the weapon in use.", MENU_WHITE_CENTERED);
            return 3;
        case TALENT_COMBAT_EXP:
            addlinetomenu(ent, "Increases physical,", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "damage, but reduces", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "resistance.", MENU_WHITE_CENTERED);
            return 3;
        case TALENT_TACTICS:
            addlinetomenu(ent, "Increases your levelup", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "health and armor bonus!", MENU_WHITE_CENTERED);
            return 2;
        case TALENT_SIDEARMS:
            addlinetomenu(ent, "Gives you additional", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "respawn weapons. Weapon", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "choice is determined by", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "weapon upgrade level.", MENU_WHITE_CENTERED);
            return 4;
            //Necromancer talents
        case TALENT_EVIL_CURSE:
            addlinetomenu(ent, "Increases curse duration.", MENU_WHITE_CENTERED);
            return 1;
        case TALENT_CHEAPER_CURSES:
            addlinetomenu(ent, "Reduces curse cost.", MENU_WHITE_CENTERED);
            return 1;
        case TALENT_CORPULENCE:
            addlinetomenu(ent, "Increases monster health/armor", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "Can't combine with Life Tap.", MENU_WHITE_CENTERED);
            return 2;
        case TALENT_LIFE_TAP:
            addlinetomenu(ent, "Increases monster damage", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "but slowly saps life.", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "Can't combine with", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "Corpulence.", MENU_WHITE_CENTERED);
            return 4;
        case TALENT_DIM_VISION:
            addlinetomenu(ent, "Adds chance to", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "automatically curse", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "enemies that shoot you.", MENU_WHITE_CENTERED);
            return 3;
        case TALENT_FLIGHT:
            addlinetomenu(ent, "Reduces jetpack cost.", MENU_WHITE_CENTERED);
            return 1;
            //Shaman talents
        case TALENT_TOTEM:
            addlinetomenu(ent, "Allows you to spawn", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "healthier totems. Totem can not", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "be of the opposite element.", MENU_WHITE_CENTERED);
            return 3;
        case TALENT_ICE:
            addlinetomenu(ent, "Allows your water totem", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "to damage its targets.", MENU_WHITE_CENTERED);
            return 2;
        case TALENT_WIND:
            addlinetomenu(ent, "Allows your air totem to", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "ghost attacks for you.", MENU_WHITE_CENTERED);
            return 2;
        case TALENT_STONE:
            addlinetomenu(ent, "Allows your earth totem to", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "increase your resistance.", MENU_WHITE_CENTERED);
            return 2;
        case TALENT_SHADOW:
            addlinetomenu(ent, "Allows your darkness totem", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "to let you vamp beyond your", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "maximum health limit.", MENU_WHITE_CENTERED);
            return 3;
        case TALENT_PEACE:
            addlinetomenu(ent, "Allows your nature totem to", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "regenerate your power cubes.", MENU_WHITE_CENTERED);
            return 2;
        case TALENT_VOLCANIC:
            addlinetomenu(ent, "Adds a chance for fire", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "totem to shoot a fireball.", MENU_WHITE_CENTERED);
            return 2;
            //Alien talents
        case TALENT_PHANTOM_OBSTACLE:
            addlinetomenu(ent, "Reduces time to cloak.", MENU_WHITE_CENTERED);
            return 1;
        case TALENT_SUPER_HEALER:
            addlinetomenu(ent, "Allows healer to heal", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "beyond maximum health.", MENU_WHITE_CENTERED);
            return 2;
        case TALENT_PHANTOM_COCOON:
            addlinetomenu(ent, "Allows cocoon to cloak.", MENU_WHITE_CENTERED);
            return 1;
        case TALENT_SWARMING:
            addlinetomenu(ent, "Increases spore damage.", MENU_WHITE_CENTERED);
            // addlinetomenu(ent, "but reduces damage.", MENU_WHITE_CENTERED); // lol
            return 1;
        case TALENT_EXPLODING_BODIES:
            addlinetomenu(ent, "Makes alien-summons'", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "corpses explode.", MENU_WHITE_CENTERED);
            return 2;
            // Kamikaze talents
        case TALENT_MARTYR:
            addlinetomenu(ent, "Creates an explotion", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "when you die.", MENU_WHITE_CENTERED);
            return 2;
        case TALENT_BLAST_RESIST:
            addlinetomenu(ent, "Increases defense against", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "radius damage.", MENU_WHITE_CENTERED);
            return 2;
        case TALENT_MAGMINESELF:
            addlinetomenu(ent, "Gain the ability", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "to turn into a living magmine", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "using 'magmine self'.", MENU_WHITE_CENTERED);
            return 3;
        case TALENT_INSTANTPROXYS:
            addlinetomenu(ent, "Makes proxys be removed", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "instantly when they explode.", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "On level 2, it removes", MENU_WHITE_CENTERED);
            addlinetomenu(ent, "hold time when building them.", MENU_WHITE_CENTERED);
            return 4;
        default:
            return 0;
    }
}

void vrx_open_talent_menu(edict_t *ent, int talentID, qboolean select_upgrade) {
    talent_t *talent = &ent->myskills.talents.talent[vrx_get_talent_slot(ent, talentID)];
    int level = talent->upgradeLevel;
    int talentPoints = ent->myskills.talents.talentPoints;
    int lineCount = 7;//12;
    qboolean can_upgrade = false;

    if (!ShowMenu(ent))
        return;
    clearmenu(ent);

    addlinetomenu(ent, "Talent", MENU_GREEN_CENTERED);
    addlinetomenu(ent, va("%s: %d/%d", GetTalentString(talentID), level, talent->maxLevel), MENU_WHITE_CENTERED);
    addlinetomenu(ent, " ", 0);

    lineCount += writeTalentDescription(ent, talentID);

    addlinetomenu(ent, " ", 0);
    //addlinetomenu(ent, "Current", MENU_GREEN_CENTERED);
    //writeTalentUpgrade(ent, talentID, level);
    addlinetomenu(ent, " ", 0);

    if (talent->upgradeLevel < talent->maxLevel && talentPoints)
    {
        can_upgrade = true;
        addlinetomenu(ent, "Upgrade this talent.", -1 * (talentID + 1));
    }
    else addlinetomenu(ent, " ", 0);

    addlinetomenu(ent, "Previous menu.", talentID + 1);

    setmenuhandler(ent, TalentUpgradeMenu_handler);
    if (select_upgrade && can_upgrade)
        ent->client->menustorage.currentline = lineCount-1;
    else
        ent->client->menustorage.currentline = lineCount;
    showmenu(ent);
}

//****************************************
//*********** Main Talent Menu ***********
//****************************************

void openTalentMenu_handler(edict_t *ent, int option) {
    switch (option) {
        case 9999:    //Exit
        {
            closemenu(ent);
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

    if (!ShowMenu(ent))
        return;
    clearmenu(ent);

    // menu header
    addlinetomenu(ent, "Talents", MENU_GREEN_CENTERED);
    addlinetomenu(ent, " ", 0);

    for (i = 0; i < ent->myskills.talents.count; i++) {
        talent = &ent->myskills.talents.talent[i];

        //create menu string
        strcpy(buffer, GetTalentString(talent->id));
        strcat(buffer, ":");
        padRight(buffer, 15);

        addlinetomenu(ent, va("%d. %s %d/%d", i + 1, buffer, talent->upgradeLevel, talent->maxLevel), talent->id);
    }

    // menu footer
    addlinetomenu(ent, " ", 0);
    addlinetomenu(ent, va("You have %d talent points.", ent->myskills.talents.talentPoints), 0);
    addlinetomenu(ent, " ", 0);

    addlinetomenu(ent, "Exit", 9999);
    setmenuhandler(ent, openTalentMenu_handler);

    if (!lastline) ent->client->menustorage.currentline = ent->myskills.talents.count + 6;
    else ent->client->menustorage.currentline = lastline + 2;

    showmenu(ent);

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
        int talentId = ent->myskills.talents.talent[i].id;
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
                int difference = player_talent->upgradeLevel - player_talent->maxLevel;
                player_talent->upgradeLevel -= difference;
                refunded += difference;
            }
        }
    }

    // check talents missing from the player
    for (const talentdef_t* talent = talents_by_class[ent->myskills.class_num];
         talent->talent_id != -1;
         talent++) {
        int talentLevel = vrx_get_talent_slot(ent, talent->talent_id);
        if (talentLevel == -1) { // not found
            vrx_add_talent(ent, talent->talent_id, talent->max_level);
        }
    }

    if (refunded) {
        ent->myskills.talents.talentPoints += refunded;
        gi.cprintf(ent, PRINT_HIGH, "%d talent points were refunded.\n", refunded);
    }
}