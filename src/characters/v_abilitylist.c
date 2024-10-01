#include "g_local.h"

const abilitydef_t *abilities_by_index[MAX_ABILITIES];
uint8_t ability_class[MAX_ABILITIES][CLASS_MAX]; // whether ability i is in character class k (1 or 0)

const abilitydef_t ability_general[] = {
        {VITALITY,        0, DEFAULT_SOFTMAX,  1},
        {MAX_AMMO,        0, DEFAULT_SOFTMAX,  1},
        {POWER_REGEN,     1, DEFAULT_SOFTMAX,                1},
        {WORLD_RESIST,    0, 1,                1},
        {AMMO_REGEN,      0, DEFAULT_SOFTMAX,  1},
        {REGENERATION,    0, 5,                1},
        {STRENGTH,        0, 5,                1},
        {HASTE,           0, 5,                1},
        {RESISTANCE,      0, 5,                1},
        {SHELL_RESIST,    0, 1,                1},
        {BULLET_RESIST,   0, 1,                1},
        {SPLASH_RESIST,   0, 1,                1},
        {PIERCING_RESIST, 0, 1,                1},
        {ENERGY_RESIST,   0, 1,                1},
        {SCANNER,         0, 1,                1},
        {HA_PICKUP,       0, DEFAULT_SOFTMAX,  1},
        {FLASH,           0, 1,                1},
        {-1,              0, 0,                0} // Guardian (Add skills above this)
};

const abilitydef_t ability_soldier[] = {
        {STRENGTH,         0, DEFAULT_SOFTMAX,   0},
        {RESISTANCE,       0, DEFAULT_SOFTMAX,   0},
        {NAPALM,           0, DEFAULT_SOFTMAX,   0},
        {SPIKE_GRENADE,    0, DEFAULT_SOFTMAX,   0},
        {EMP,              0, DEFAULT_SOFTMAX,   0},
        {MIRV,             0, DEFAULT_SOFTMAX,   0},
        {EXPLODING_BARREL, 0, DEFAULT_SOFTMAX,   0},
        {CREATE_QUAD,   0, 1,                 0},
        {CREATE_INVIN,  0, 1,                 0},
        {GRAPPLE_HOOK,  3, 3,                 0},
        {-1,            0, 0,                 0} // Guardian (Add skills above this)
};

const abilitydef_t ability_vampire[] = {
        {VAMPIRE,        0, DEFAULT_SOFTMAX,   0},
        {GHOST,          0, DEFAULT_SOFTMAX,   0},
        {LIFE_DRAIN,     0, DEFAULT_SOFTMAX,   0},
        {FLESH_EATER,    0, DEFAULT_SOFTMAX,   0},
        {CORPSE_EXPLODE, 0, DEFAULT_SOFTMAX,   0},
        {MIND_ABSORB,    0, DEFAULT_SOFTMAX,   0},
        {BLINKSTRIKE,    0, DEFAULT_SOFTMAX,   0},
        {CONVERSION,     0, DEFAULT_SOFTMAX,   0},
        {CLOAK,          1, 1,                 0},
        {-1,             0, 0,                 0} // Guardian (Add skills above this)
};

const abilitydef_t ability_necromancer[] = { // NECROMANCER

        {MONSTER_SUMMON, 0, DEFAULT_SOFTMAX,   0},
        {HELLSPAWN,      0, DEFAULT_SOFTMAX,   0},
        {PLAGUE,         0, DEFAULT_SOFTMAX,   0},
        {LIFE_TAP,       0, DEFAULT_SOFTMAX,   0},
        {AMP_DAMAGE,     0, DEFAULT_SOFTMAX,   0},
        {CRIPPLE,        0, DEFAULT_SOFTMAX,   0},
        {CURSE,          0, DEFAULT_SOFTMAX,   0},
        {WEAKEN,         0, DEFAULT_SOFTMAX,   0},
        {JETPACK,        1, 1,                 0},
        {-1,             0, 0,                 0} // Guardian (Add skills above this)
};

const abilitydef_t ability_engineer[] = { // ENGINEER
        {PROXY,           0, DEFAULT_SOFTMAX,   0},
        {BUILD_SENTRY,    0, DEFAULT_SOFTMAX,   0},
        {SUPPLY_STATION,  0, DEFAULT_SOFTMAX,   0},
        {BUILD_LASER,     0, DEFAULT_SOFTMAX,   0},
        {MAGMINE,         0, DEFAULT_SOFTMAX,   0},
       // {CALTROPS,        0, DEFAULT_SOFTMAX,   0},
        {AUTOCANNON,      0, DEFAULT_SOFTMAX,   0},
        {DETECTOR,        0, DEFAULT_SOFTMAX,   0},
        {DECOY,           0, DEFAULT_SOFTMAX,   0},
        {EXPLODING_ARMOR, 0, DEFAULT_SOFTMAX,   0},
        {ANTIGRAV,        1, 1,                 0},
        {-1,              0, 0,                 0} // Guardian (Add skills above this)
};

const abilitydef_t ability_shaman[] = { // SHAMAN
        {FIRE_TOTEM,    0, DEFAULT_SOFTMAX, 0},
        {WATER_TOTEM,   0, DEFAULT_SOFTMAX, 0},
        {AIR_TOTEM,     0, DEFAULT_SOFTMAX, 0},
        {EARTH_TOTEM,   0, DEFAULT_SOFTMAX, 0},
        {DARK_TOTEM,    0, DEFAULT_SOFTMAX, 0},
        {NATURE_TOTEM,  0, DEFAULT_SOFTMAX, 0},
        {HASTE,         0, 5,               0},
        {TOTEM_MASTERY, 1, 1,               0},
        {SUPER_SPEED,   1, 1,               0},
        {FURY,          0, DEFAULT_SOFTMAX, 0},
        {-1,            0, 0,               0} // Guardian (Add skills above this)
};

const abilitydef_t ability_mage[] = { // MAGE
        {MAGICBOLT,       0, DEFAULT_SOFTMAX,   0},
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

const abilitydef_t ability_cleric[] = {
        {SALVATION,   0, DEFAULT_SOFTMAX, 0},
        {HEALING,     0, DEFAULT_SOFTMAX, 0},
        {BLESS,       0, DEFAULT_SOFTMAX, 0},
        {YIN,         0, DEFAULT_SOFTMAX, 0},
        {YANG,        0, DEFAULT_SOFTMAX, 0},
        {HAMMER,      0, DEFAULT_SOFTMAX, 0},
        {DEFLECT,     0, DEFAULT_SOFTMAX, 0},
        {SUPER_SPEED, 1, 1,               0},
        {DOUBLE_JUMP, 1, 1,               0},
        {HOLY_FREEZE, 0, DEFAULT_SOFTMAX, 0},
        {-1,          0, 0,               0} // Guardian (Add skills above this)
};

const abilitydef_t ability_knight[] = { // knight

        {ARMOR_UPGRADE, 0, DEFAULT_SOFTMAX,   0},
        {REGENERATION,  0, DEFAULT_SOFTMAX,   0},
        {POWER_SHIELD,  0, DEFAULT_SOFTMAX,   0},
        {ARMOR_REGEN,   0, DEFAULT_SOFTMAX,   0},
        {BEAM,          0, DEFAULT_SOFTMAX,   0},
        {PLASMA_BOLT,   0, DEFAULT_SOFTMAX,   0},
        {CALTROPS,      0, DEFAULT_SOFTMAX,   0},
        {SHIELD,        1, 1,                 0},
        {BOOST_SPELL,   1, 1,                 0},
        {-1,            0, 0,                 0} // Guardian (Add skills above this)
};

const abilitydef_t ability_alien[] = {
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

const abilitydef_t ability_poltergeist[] = {
        {MORPH_MASTERY, 1, 1,               0},
        {BERSERK,       1, DEFAULT_SOFTMAX, 0},
        {CACODEMON,     1, DEFAULT_SOFTMAX, 0},
        {BLOOD_SUCKER,  1, DEFAULT_SOFTMAX, 0},
        {BRAIN,         1, DEFAULT_SOFTMAX, 0},
        {FLYER,         1, DEFAULT_SOFTMAX, 0},
        {MUTANT,        1, DEFAULT_SOFTMAX, 0},
        {TANK,          1, DEFAULT_SOFTMAX, 0},
        {MEDIC,         1, DEFAULT_SOFTMAX, 0},
        {GHOST,         1, DEFAULT_SOFTMAX, 0}, // given for free with morph mastery
        {-1,            0, 0,               0} // Guardian (Add skills above this)
};

const abilitydef_t ability_weaponmaster[] = {
        {-1, 0, 0, 0} // Guardian (Add skills above this)
};

// needs to match class enum
const abilitylist_t abilities_by_class[] = {
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

const abilitydef_t * vrx_get_ability_by_index(int index) {
    
    if (index < 0 || index >= MAX_ABILITIES)
        return NULL;

    return abilities_by_index[index];
}


void vrx_assign_abilities(edict_t *ent) {
    vrx_disable_abilities(ent);

    // enable all skills (weaponmaster/ab or generalabmode is on)
    if (ent->myskills.class_num == CLASS_WEAPONMASTER || generalabmode->value) {
        int i;
        for (i = 0; i < MAX_ABILITIES; i++) {
            const abilitydef_t *first = abilities_by_index[i];

            if (first) {
                int real_max = first->softmax;
                int start = 0;

                if (first->softmax >= DEFAULT_SOFTMAX) // a 10 softmax? dump down to 5
                    real_max = GENERAL_SOFTMAX;

                if (first->general) // non-class can get starting points
                    start = first->start;

                vrx_enable_ability(ent, first->index, start, real_max, 1);
            }
        }
    }

    // enable class skills
    if (ent->myskills.class_num != CLASS_WEAPONMASTER) {
        const abilitydef_t *first = abilities_by_class[ent->myskills.class_num];
        while (first->index != -1) {
            //gi.dprintf("enabled ability %s\n", GetAbilityString(first->index));
            vrx_enable_ability(ent, first->index, first->start, first->softmax, first->general);
            first++;
        }
    }

    // enable general skills
    // has to be done after enabling all skills and class skills
    // since we check if there's a class version already enabled.
    const abilitydef_t *first = abilities_by_class[0];
    while (first->index != -1) {
        //gi.dprintf("enabled ability %s\n", GetAbilityString(first->index));
        // if this ability doesn't have a class specialization, or is disabled, use our general version
        if (ent->myskills.abilities[first->index].general_skill || 
            ent->myskills.abilities[first->index].disable ||
            ent->myskills.abilities[first->index].hard_max == 0)
            vrx_enable_ability(ent, first->index, first->start, first->softmax, first->general);
        first++;
    }
}

int vrx_get_hard_max(int index, qboolean general, int class) {
    switch (index) {
        //Skills that max at level 1
        case ID:
        case WORLD_RESIST:
        case BULLET_RESIST:
        case SHELL_RESIST:
        case ENERGY_RESIST:
        case PIERCING_RESIST:
        case SPLASH_RESIST:
        case CREATE_QUAD:
        case CREATE_INVIN:
        case BOOST_SPELL:
        case SUPER_SPEED:
        case ANTIGRAV:
        case WEAPON_KNOCK:
        case MORPH_MASTERY:
        case TELEPORT:
        case JETPACK:
        case SHIELD:
        case CLOAK: // az: sure it scales whatever the scaling is dumb
        case FLASH:
            return 1;

        case GRAPPLE_HOOK:
            return 3;

            
        case HASTE:
            return 10;

        // Special cases for the non-general ability mode.
        // Falls through to the default case...
        case STRENGTH:
        case RESISTANCE:
            if (!generalabmode->value) {
                if (general && class == CLASS_SOLDIER)
                    return 15;
                else
                    return 20;
            }

        case REGENERATION:
            if (!generalabmode->value) {
                if (general)
                    return 15;
            }
            //Everything else
        default:
            if (vrx_get_ability_upgrade_cost(index) < 2) {
                if (!generalabmode->value) {
                    if (class == CLASS_WEAPONMASTER) { // apprentice
                        return GENERAL_SOFTMAX * 2;
                    } else {
                        return abilities_by_index[index]->softmax * 2;
                    }

                } else { // general ability mode
                    if (ability_class[index][class] != 0) // current class has this ability
                        return (int) (abilities_by_index[index]->softmax * 2);
                    else
                        return (int) (abilities_by_index[index]->softmax);
                }
            } else
                return abilities_by_index[index]->softmax;
    }
    return abilities_by_index[index]->softmax;
}

void vrx_add_ability(edict_t* ent, int index) {
    if (index < 0 || index >= MAX_ABILITIES)
        return;

    int class = vrx_get_ability_class(index);
    if (class == CLASS_NULL)
        return;

    const abilitydef_t *ability = abilities_by_index[index];
    int hardmax = vrx_get_hard_max(index, 0, class);
    vrx_enable_ability(
        ent, index,
        ability->start,
        ability->softmax + ent->myskills.prestige.softmaxBump[index],
        0
        );
}

void vrx_enable_ability(edict_t *ent, int abil, int level, int max_level, int general) {
    ent->myskills.abilities[abil].disable = false;

    // we can pass this function -1 if we don't want to alter these variables
    if (max_level != -1)
        ent->myskills.abilities[abil].max_level = max_level;

    if (level != -1) {
        ent->myskills.abilities[abil].level = level;
        ent->myskills.abilities[abil].current_level = level;
    }

    ent->myskills.abilities[abil].general_skill = general;
    if (vrx_prestige_has_ability(&ent->myskills.prestige, abil))
        ent->myskills.abilities[abil].hard_max = vrx_get_hard_max(abil, general, vrx_get_ability_class(abil));
    else
        ent->myskills.abilities[abil].hard_max = vrx_get_hard_max(abil, general, ent->myskills.class_num);
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
        case WORLD_RESIST:
        case BULLET_RESIST:
        case SHELL_RESIST:
        case ENERGY_RESIST:
        case PIERCING_RESIST:
        case BLACKHOLE:
        case TELEPORT:
        case CLOAK:
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

const abilitydef_t * vrx_get_class_rune_stat(int class_index) {
    int ability_index;
    int count = 0;
    const abilitydef_t *first, *current;

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

const abilitydef_t * vrx_get_random_ability() {
    const abilitydef_t *ability;

    ability = abilities_by_index[GetRandom(0, MAX_ABILITIES - 1)];
    while (!ability)
        ability = abilities_by_index[GetRandom(0, MAX_ABILITIES - 1)];

    return ability;
}

void vrx_init_ability_list() {
    const abilitydef_t *first;
    // gi.dprintf("INFO: Initializing ability list... ");

    memset(abilities_by_index, 0, sizeof abilities_by_index);
    memset(ability_class, 0, sizeof ability_class);

    for (int i = 0; i < CLASS_MAX; i++) {
        // iterate through our pointer list
        first = abilities_by_class[i];

        // iterate through class' ability list
        while (first->index != -1) {
            if (abilities_by_index[first->index]) {
                // get the one with the highest softmax, since we want to track the abilities
                // as they exist on their class.
                if (abilities_by_index[first->index]->softmax < first->softmax) {
                    abilities_by_index[first->index] = first;
                }
            } else {
                abilities_by_index[first->index] = first;
            }

            ability_class[first->index][i] = 1;

            // all classes have general abilities
            if (i == 0) {
                for (int pclass = CLASS_NULL + 1; pclass < CLASS_MAX; pclass++) {
                    ability_class[first->index][pclass] = 1;
                }
            }

#ifdef _DEBUG
            if (vrx_get_hard_max(first->index, first->general, i) < first->softmax) {
                gi.dprintf("warning: ability '%s' (%d) has hardmax < softmax\n", GetAbilityString(first->index), first->index);
            }
#endif
            first++;
        }
    }

    gi.dprintf("Done.\n");
}

int vrx_get_last_enabled_skill_index(edict_t *ent, int mode) {
    int return_index;
    for (int i = 0; i < MAX_ABILITIES; i++) {
        if (!ent->myskills.abilities[i].disable) {
            if (ent->myskills.abilities[i].general_skill == mode)
                return_index = i;
        }
    }
    return return_index;
}

// this needs to match vrx_add_levelup_boons() in player_points.c
// NOTE: this function can't/won't award refunds if the ability was already upgraded
void vrx_update_free_abilities(edict_t *ent) {
    // free scanner at level 10
    if (ent->myskills.level >= 10) {
        if (!ent->myskills.abilities[SCANNER].level) {
            ent->myskills.abilities[SCANNER].level++;
            ent->myskills.abilities[SCANNER].current_level++;
        }
    }

    // free max ammo and vitality upgrade every 5 levels
    if (!ent->myskills.abilities[MAX_AMMO].level)
        ent->myskills.abilities[MAX_AMMO].level = ent->myskills.abilities[MAX_AMMO].current_level =
                ent->myskills.level / 5;
    if (!ent->myskills.abilities[VITALITY].level)
        ent->myskills.abilities[VITALITY].level = ent->myskills.abilities[VITALITY].current_level =
                ent->myskills.level / 5;
}


//FIXME: this doesn't work with 2-4 point abilities
void vrx_normalize_abilities(edict_t *ent) {
    int i, refunded = 0;
    upgrade_t old_abilities[MAX_ABILITIES];
    qboolean points_refunded = false;

    vrx_update_free_abilities(ent);

    // make a copy of old abilities that we can use as a reference for comparison
    for (i = 0; i < MAX_ABILITIES; ++i)
        memcpy(&old_abilities, ent->myskills.abilities, sizeof(upgrade_t) * MAX_ABILITIES);

    // reset all ability upgrades
    memset(ent->myskills.abilities, 0, sizeof(upgrade_t) * MAX_ABILITIES);

    vrx_assign_abilities(ent);
    vrx_prestige_reapply_abilities(ent);

    for (i = 0; i < MAX_ABILITIES; ++i) {
        // if this ability was previously enabled, restore the upgrade level
        if ((ent->myskills.abilities[i].disable == false) && (old_abilities[i].disable == false)) {
            // restore previous upgrade level if our new max_level is greater or equal to the old one
            if (ent->myskills.abilities[i].max_level >= old_abilities[i].max_level) {
                //if (old_abilities[i].level > 0)
                //	gi.dprintf("Restoring %s to level %d\n", GetAbilityString(i), old_abilities[i].level);
                ent->myskills.abilities[i].level = old_abilities[i].level;
            }
                // otherwise set new upgrade level to soft max and refund unused points
            else {
                refunded += old_abilities[i].level - ent->myskills.abilities[i].max_level;
                //gi.dprintf("Refunding %d points due to hard max cap\n", refunded);
                if (refunded > 0) // az: I added this check. I'm not sure why we need this.
                    ent->myskills.abilities[i].level = ent->myskills.abilities[i].max_level;
            }
        }
            // if this upgrade was previously disabled, refund them points
        else if ((ent->myskills.abilities[i].disable == true) && (old_abilities[i].disable == false)) {
            refunded = old_abilities[i].level;
            //gi.dprintf("Refunding %d points because ability is currently disabled\n", refunded);
        }
    }

    // re-apply equipment
    vrx_runes_unapply(ent);
    for (i = 0; i < 3; ++i)
        vrx_runes_apply(ent, &ent->myskills.items[i]);

    /*safe_cprintf(ent, PRINT_HIGH, "Your abilities have been updated.\n");	*/

    // have points been refunded?
    if (refunded > 0) {
        ent->myskills.speciality_points += old_abilities[i].level;
        safe_cprintf(ent, PRINT_HIGH, "%d ability points have been refunded.\n", refunded);
    }
}

int vrx_get_ability_class(int abil) {
    if (abil < 0 || abil >= MAX_ABILITIES)
        return 0;

    for (int i = 1; i < CLASS_MAX; i++) {
        if (ability_class[abil][i])
            return i;
    }

    return 0;
}

void vrx_ability_open_select_menu(
    edict_t* self,
    const char* title,
    qboolean (*filter)(const abilitydef_t*, void*),
    int page,
    void* userdata,
    void (*handler)(edict_t *ent,int option)) {
    if (!menu_can_show(self))
        return;

    abilitybitmap_t skills;

    memset(skills, 0, sizeof(abilitybitmap_t));

    for (int i = 0; i < MAX_ABILITIES; i++) {
        const abilitydef_t *ability = vrx_get_ability_by_index(i);
        if (ability && filter(ability, userdata)) {
            skills[i / 32] |= 1 << (i % 32);
        }
    }

    int ab_count = 0;
    int start_index = -1;
    // find the first ability to display
    for (int i = 0; i < MAX_ABILITIES; i++) {
        if (skills[i / 32] & (1 << (i % 32))) {
            ab_count++;
        }

        if (ab_count > 10 * page && start_index == -1) {
            start_index = i;
        }
    }

    menu_clear(self);
    menu_add_line(self, "Ability Selection", MENU_GREEN_CENTERED);
    if (title)
        menu_add_line(self, title, MENU_WHITE_CENTERED);

    menu_add_line(self, " ", 0);

    int ab_count_on_page = 0;
    int limit = 10;
    for (int i = 0; i < limit && ab_count_on_page <= 10 && i < MAX_ABILITIES; i++) {
        int index = start_index + i;
        // if the ability is enabled by the filter
        if (skills[index / 32] & (1 << (index % 32))) {
            const abilitydef_t *ability = vrx_get_ability_by_index(index);
            if (ability) {
                menu_add_line(self, va("%s", GetAbilityString(index)), index + 1000);
                ab_count_on_page++;
            }
        } else {
            limit++;
        }
    }

    menu_add_line(self, " ", 0);

    if (page > 0)
        menu_add_line(self, "Previous Page", 10000 + page - 1);
    if (ab_count > 10 * (page + 1))
        menu_add_line(self, "Next Page", 10000 + page + 1);

    menu_add_line(self, "Exit", 666);

    self->client->menustorage.currentline += self->client->menustorage.num_of_lines - 1;
    menu_set_handler(self, handler);
    menu_show(self);
}