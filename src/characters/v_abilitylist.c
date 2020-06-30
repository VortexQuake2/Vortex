#include "../quake2/g_local.h"

abilitydef_t *abilities_by_index[MAX_ABILITIES];

abilitydef_t ability_general[] = {
        {VITALITY,        0, 15,              1},
        {MAX_AMMO,        0, 10,              1},
        {POWER_REGEN,     0, 8,               1},
        {WORLD_RESIST,    0, 1,               1},
        {AMMO_REGEN,      0, 10,              1},
        {REGENERATION,    0, 5,               1},
        {STRENGTH,        0, 5,               1},
        {HASTE,           0, 5,               1},
        {RESISTANCE,      0, 5,               1},
        {SHELL_RESIST,    0, 1,               1},
        {BULLET_RESIST,   0, 1,               1},
        {SPLASH_RESIST,   0, 1,               1},
        {PIERCING_RESIST, 0, 1,               1},
        {ENERGY_RESIST,   0, 1,               1},
        {SCANNER,         0, 1,               1},
        {HA_PICKUP,       0, DEFAULT_SOFTMAX, 1},
        {-1,              0, 0,               0} // Guardian (Add skills above this)
};

abilitydef_t ability_soldier[] = {
        {STRENGTH,      0, INCREASED_SOFTMAX, 0},
        {RESISTANCE,    0, DEFAULT_SOFTMAX,   0},
        {NAPALM,        0, DEFAULT_SOFTMAX,   0},
        {SPIKE_GRENADE, 0, DEFAULT_SOFTMAX,   0},
        {EMP,           0, DEFAULT_SOFTMAX,   0},
        {MIRV,          0, DEFAULT_SOFTMAX,   0},
        {CREATE_QUAD,   0, 1,                 0},
        {CREATE_INVIN,  0, 1,                 0},
        {GRAPPLE_HOOK,  3, 3,                 0},
        {SELFDESTRUCT,  0, DEFAULT_SOFTMAX,   0},
        {-1,            0, 0,                 0} // Guardian (Add skills above this)
};

abilitydef_t ability_vampire[] = {
        {VAMPIRE,        0, INCREASED_SOFTMAX, 0},
        {GHOST,          0, DEFAULT_SOFTMAX,   0},
        {LIFE_DRAIN,     0, DEFAULT_SOFTMAX,   0},
        {FLESH_EATER,    0, DEFAULT_SOFTMAX,   0},
        {CORPSE_EXPLODE, 0, DEFAULT_SOFTMAX,   0},
        {MIND_ABSORB,    0, DEFAULT_SOFTMAX,   0},
        {AMMO_STEAL,     0, DEFAULT_SOFTMAX,   0},
        {CONVERSION,     0, DEFAULT_SOFTMAX,   0},
        {CLOAK,          1, 10,                0},
        {-1,             0, 0,                 0} // Guardian (Add skills above this)
};

abilitydef_t ability_necromancer[] = { // NECROMANCER

        {MONSTER_SUMMON, 0, INCREASED_SOFTMAX, 0},
        {HELLSPAWN,      0, INCREASED_SOFTMAX, 0},
        {PLAGUE,         0, DEFAULT_SOFTMAX,   0},
        {LOWER_RESIST,   0, DEFAULT_SOFTMAX,   0},
        {AMP_DAMAGE,     0, DEFAULT_SOFTMAX,   0},
        {CRIPPLE,        0, DEFAULT_SOFTMAX,   0},
        {CURSE,          0, DEFAULT_SOFTMAX,   0},
        {WEAKEN,         0, DEFAULT_SOFTMAX,   0},
        {JETPACK,        1, 1,                 0},
        {-1,             0, 0,                 0} // Guardian (Add skills above this)
};

abilitydef_t ability_engineer[] = { // ENGINEER
        {PROXY,           0, DEFAULT_SOFTMAX,   0},
        {BUILD_SENTRY,    0, INCREASED_SOFTMAX, 0},
        {SUPPLY_STATION,  0, DEFAULT_SOFTMAX,   0},
        {BUILD_LASER,     0, DEFAULT_SOFTMAX,   0},
        {MAGMINE,         0, INCREASED_SOFTMAX, 0},
        {CALTROPS,        0, DEFAULT_SOFTMAX,   0},
        {AUTOCANNON,      0, DEFAULT_SOFTMAX,   0},
        {DETECTOR,        0, DEFAULT_SOFTMAX,   0},
        {DECOY,           0, DEFAULT_SOFTMAX,   0},
        {EXPLODING_ARMOR, 0, DEFAULT_SOFTMAX,   0},
        {ANTIGRAV,        1, 1,                 0},
        {-1,              0, 0,                 0} // Guardian (Add skills above this)
};

abilitydef_t ability_shaman[] = { // SHAMAN
        {FIRE_TOTEM,    0, INCREASED_SOFTMAX, 0},
        {WATER_TOTEM,   0, INCREASED_SOFTMAX, 0},
        {AIR_TOTEM,     0, INCREASED_SOFTMAX, 0},
        {EARTH_TOTEM,   0, INCREASED_SOFTMAX, 0},
        {DARK_TOTEM,    0, INCREASED_SOFTMAX, 0},
        {NATURE_TOTEM,  0, INCREASED_SOFTMAX, 0},
        {HASTE,         0, 5,                 0},
        {TOTEM_MASTERY, 1, 1,                 0},
        {SUPER_SPEED,   1, 1,                 0},
        {-1,            0, 0,                 0} // Guardian (Add skills above this)
};

abilitydef_t ability_mage[] = { // MAGE
        {MAGICBOLT,       0, INCREASED_SOFTMAX, 0},
        {NOVA,            0, DEFAULT_SOFTMAX,   0},
        {BOMB_SPELL,      0, DEFAULT_SOFTMAX,   0},
        {FORCE_WALL,      0, DEFAULT_SOFTMAX,   0},
        {LIGHTNING,       0, DEFAULT_SOFTMAX,   0},
        {METEOR,          0, DEFAULT_SOFTMAX,   0},
        {FIREBALL,        0, DEFAULT_SOFTMAX,   0},
        {LIGHTNING_STORM, 0, DEFAULT_SOFTMAX,   0},
        {TELEPORT,        1, 1,                 0},
        {-1,              0, 0,                 0} // Guardian (Add skills above this)
};

abilitydef_t ability_cleric[] = {
        {SALVATION,   0, DEFAULT_SOFTMAX, 0},
        {HEALING,     0, DEFAULT_SOFTMAX, 0},
        {BLESS,       0, DEFAULT_SOFTMAX, 0},
        {YIN,         0, DEFAULT_SOFTMAX, 0},
        {YANG,        0, DEFAULT_SOFTMAX, 0},
        {HAMMER,      0, DEFAULT_SOFTMAX, 0},
        {DEFLECT,     0, DEFAULT_SOFTMAX, 0},
        {SUPER_SPEED, 1, 1,               0},
        {DOUBLE_JUMP, 1, 1,               0},
        {-1,          0, 0,               0} // Guardian (Add skills above this)
};

abilitydef_t ability_knight[] = { // knight

        {ARMOR_UPGRADE, 0, 10,                0},
        {REGENERATION,  0, DEFAULT_SOFTMAX,   0},
        {POWER_SHIELD,  0, DEFAULT_SOFTMAX,   0},
        {ARMOR_REGEN,   0, DEFAULT_SOFTMAX,   0},
        {BEAM,          0, INCREASED_SOFTMAX, 0},
        {PLASMA_BOLT,   0, DEFAULT_SOFTMAX,   0},
        {SHIELD,        1, 1,                 0},
        {BOOST_SPELL,   1, 1,                 0},
        {-1,            0, 0,                 0} // Guardian (Add skills above this)
};

abilitydef_t ability_alien[] = {
        {SPIKER,    0, DEFAULT_SOFTMAX, 0},
        {OBSTACLE,  0, DEFAULT_SOFTMAX, 0},
        {GASSER,    0, DEFAULT_SOFTMAX, 0},
        {HEALER,    0, DEFAULT_SOFTMAX, 0},
        {SPORE,     0, DEFAULT_SOFTMAX, 0},
        {ACID,      0, DEFAULT_SOFTMAX, 0},
        {SPIKE,     0, DEFAULT_SOFTMAX, 0},
        {COCOON,    0, DEFAULT_SOFTMAX, 0},
        {BLACKHOLE, 1, 1,               0},
        {-1,        0, 0,               0} // Guardian (Add skills above this)
};

abilitydef_t ability_poltergeist[] = {
        {MORPH_MASTERY, 1, 1,                 0},
        {BERSERK,       1, INCREASED_SOFTMAX, 0},
        {CACODEMON,     1, INCREASED_SOFTMAX, 0},
        {BLOOD_SUCKER,  1, INCREASED_SOFTMAX, 0},
        {BRAIN,         1, INCREASED_SOFTMAX, 0},
        {FLYER,         1, INCREASED_SOFTMAX, 0},
        {MUTANT,        1, INCREASED_SOFTMAX, 0},
        {TANK,          1, INCREASED_SOFTMAX, 0},
        {MEDIC,         1, INCREASED_SOFTMAX, 0},
        {GHOST,         1, DEFAULT_SOFTMAX,   0}, // given for free with morph mastery
        {-1,            0, 0,                 0} // Guardian (Add skills above this)
};

abilitydef_t ability_weaponmaster[] = {
        {-1, 0, 0, 0} // Guardian (Add skills above this)
};

abilitylist_t abilities_by_class[] = {
        ability_general,
        ability_soldier,
        ability_poltergeist,
        ability_vampire,
        ability_mage,
        ability_engineer,
        ability_knight,
        ability_cleric,
        ability_necromancer,
        ability_shaman,
        ability_alien,
        ability_weaponmaster,
};


void vrx_assign_abilities(edict_t *ent) {
    abilitydef_t *first = abilities_by_class[0];

    vrx_disable_abilities(ent);

    // enable general skills
    while (first->index != -1) {
        //gi.dprintf("enabled ability %s\n", GetAbilityString(first->index));
        enableAbility(ent, first->index, first->start, first->softmax, first->general);
        first++;
    }

    // enable all skills (weaponmaster/ab or generalabmode is on)
    if (ent->myskills.class_num == CLASS_WEAPONMASTER || generalabmode->value) {
        int i;
        for (i = 0; i < MAX_ABILITIES; i++) {
            first = abilities_by_index[i];

            if (first) {
                int real_max = first->softmax;

                if (first->softmax > 10) // a 15 softmax? dump down to 8
                    real_max = GENERAL_SOFTMAX;

                enableAbility(ent, first->index, 0, real_max, 1);
            }
        }
    }

    // enable class skills
    if (ent->myskills.class_num != CLASS_WEAPONMASTER) {
        first = abilities_by_class[ent->myskills.class_num];
        while (first->index != -1) {
            //gi.dprintf("enabled ability %s\n", GetAbilityString(first->index));
            enableAbility(ent, first->index, first->start, first->softmax, first->general);
            first++;
        }
    }
}

int getHardMax(int index, qboolean general, int class) {
    switch (index) {
        //Skills that max at level 1
        case ID:
        case WORLD_RESIST:
        case BULLET_RESIST:
        case SHELL_RESIST:
        case ENERGY_RESIST:
        case PIERCING_RESIST:
        case SPLASH_RESIST:
        case CLOAK:
        case CREATE_QUAD:
        case CREATE_INVIN:
        case BOOST_SPELL:
        case SUPER_SPEED:
        case ANTIGRAV:
        case WEAPON_KNOCK:
        case TELEPORT:
        case JETPACK:
        case SHIELD:
            return 1;
            break;

            // Special cases for the non-general ability mode.
        case HASTE:
        case AMMO_REGEN:
            return 5;
        case STRENGTH:
        case RESISTANCE:
            if (!generalabmode->value) {
                if (general && class == CLASS_SOLDIER)
                    return 15;
                else
                    return 30;
                break;
            }

        case REGENERATION:
            if (!generalabmode->value) {
                if (general)
                    return 15;
                break;
            }
        case GRAPPLE_HOOK:
            return 3;
            //Everything else
        default:
            if (vrx_get_ability_upgrade_cost(index) < 2) {
                if (!generalabmode->value) {
                    if (class == CLASS_WEAPONMASTER) {
                        return GENERAL_SOFTMAX * 2;
                    } else {
                        return abilities_by_index[index]->softmax * 4;
                    }

                } else
                    return (int) (abilities_by_index[index]->softmax * 1.5);
            } else
                return abilities_by_index[index]->softmax;
    }
    return abilities_by_index[index]->softmax;
}

void enableAbility(edict_t *ent, int index, int level, int max_level, int general) {
    ent->myskills.abilities[index].disable = false;

    // we can pass this function -1 if we don't want to alter these variables
    if (max_level != -1)
        ent->myskills.abilities[index].max_level = max_level;

    if (level != -1) {
        ent->myskills.abilities[index].level = level;
        ent->myskills.abilities[index].current_level = level;
    }

    ent->myskills.abilities[index].general_skill = general;
    ent->myskills.abilities[index].hard_max = getHardMax(index, general, ent->myskills.class_num);
}

void vrx_disable_abilities(edict_t *ent) {
    int i;

    for (i = 0; i < MAX_ABILITIES; ++i) {
        ent->myskills.abilities[i].disable = true;
        ent->myskills.abilities[i].hidden = false;
    }
}

int vrx_get_ability_upgrade_cost(int index) {
    switch (index) {
        //Abilities that cost 2 points
        //case FREEZE_SPELL:
        case SCANNER:
        case DOUBLE_JUMP:
        case JETPACK:
        case MORPH_MASTERY:
        case ANTIGRAV:
        case FLASH:
        case ID:
            return 2;
            //Abilities that cost 3 points
        case CLOAK:
        case WORLD_RESIST:
        case BULLET_RESIST:
        case SHELL_RESIST:
        case ENERGY_RESIST:
        case PIERCING_RESIST:
        case BLACKHOLE:
        case TELEPORT:
        case BOOST_SPELL:
        case SPLASH_RESIST:
            return 3;
            //Abilities that cost 4 points
        case CREATE_QUAD:
        case CREATE_INVIN:
        case SUPER_SPEED:
        case WEAPON_KNOCK:
        case TOTEM_MASTERY:
        case SHIELD:
            return 4;
        default:
            return 1;
    }
}

//************************************************************************************************
//	CLASS RUNE ARRAYS
//************************************************************************************************

/*
az: This one needs a bit of explaining.

You used to have to define manually every class rune's possible abilities, meaning every time you 
added a class or such you had to manually update this.

Using ability lists, this will always be up to date, and it'll always be relevant.
*/

abilitydef_t null_ab = {-1, 0, 0, 0};

abilitydef_t *vrx_get_class_rune_stat(int class_index) {
    int ability_index;
    int count = 0;
    abilitydef_t *first, *current;

    // find in our ability list for this class
    first = abilities_by_class[class_index];

    current = first;

    // count them
    while (current->index != -1) {
        count++;
        current++;
    }

    // pick one of the list at random
    if (count) // we might get a division by 0 here..
    {
        ability_index = GetRandom(1, count) - 1;

        // return its ability index
        return &first[ability_index];
    } else
        return &null_ab;
}

abilitydef_t *vrx_get_random_ability() {
    abilitydef_t *ability;

    ability = abilities_by_index[GetRandom(0, MAX_ABILITIES - 1)];
    while (!ability)
        ability = abilities_by_index[GetRandom(0, MAX_ABILITIES - 1)];

    return ability;
}

void vrx_init_ability_list() {
    abilitydef_t *first;
    int i;
    // gi.dprintf("INFO: Initializing ability list... ");

    for (i = 0; i < MAX_ABILITIES; i++)
        abilities_by_index[i] = NULL;

    for (i = 0; i < CLASS_MAX; i++) {
        // iterate through our pointer list
        first = abilities_by_class[i];

        // iterate through class' ability list
        while (first->index != -1) {
            if (abilities_by_index[first->index]) {
                // get the one with the highest softmax
                if (abilities_by_index[first->index]->softmax < first->softmax)
                    abilities_by_index[first->index] = first;
            } else
                abilities_by_index[first->index] = first;
            first++;
        }
    }

    gi.dprintf("Done.\n");
}

int vrx_get_last_enabled_skill_index(edict_t *ent, int mode) {
    int i, returnindex;
    for (i = 0; i < MAX_ABILITIES; i++) {
        if (!ent->myskills.abilities[i].disable) {
            if (ent->myskills.abilities[i].general_skill == mode)
                returnindex = i;
        }
    }
    return returnindex;
}