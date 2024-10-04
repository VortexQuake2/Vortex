//
// Created by dahum on 04-09-2024.
//

#include "g_local.h"
#include "prestige.h"

struct prestigedef_t {
    int id;
    const char *name;
};

struct prestigedef_t prestige[] = {
    {PRESTIGE_CLASS_SKILL, "New Class Skill"},
    {PRESTIGE_CREDITS, "Credit Earning Buff"},
    {PRESTIGE_SOFTMAX_BUMP, "Softmax Bump"},
    {PRESTIGE_ABILITY_POINTS, "Additional Ability Point"},
    {PRESTIGE_WEAPON_POINTS, "Additional Weapon Points"},
};

uint32_t PRESTIGE_THRESHOLD;

void vrx_prestige_global_init() {
    PRESTIGE_THRESHOLD = 0;
    for (int i = 0; i < 15; i++) {
        PRESTIGE_THRESHOLD += (int) vrx_get_points_tnl(i);
    }
}

void vrx_prestige_init(edict_t *pUser) {
    memset(&pUser->myskills.prestige, 0, sizeof(prestige));
}

uint32_t vrx_prestige_get_upgrade_points(uint32_t exp) {
    return exp / PRESTIGE_THRESHOLD;
}

qboolean vrx_prestige_filter_class_skill(const abilitydef_t *pAbility, void *user) {
    edict_t *pUser = user;

    // general skills cannot become class skills
    if (pAbility->general)
        return false;

    // non-scaleable abilities cannot become class skills
    if (pAbility->softmax != DEFAULT_SOFTMAX)
        return false;

    // already a class skill
    if (!pUser->myskills.abilities[pAbility->index].general_skill)
        return false;

    return true;
}

qboolean vrx_prestige_filter_softmax_bump(const abilitydef_t *pAbility, void *user) {
    edict_t *pUser = user;

    if (pAbility->softmax != DEFAULT_SOFTMAX)
        return false;

    // Don't show the skill if it has reached the maximum softmax bump.
    if (pUser->myskills.prestige.softmaxBump[pAbility->index] >= MAX_SOFTMAX_BUMP)
        return false;

    return true;
}

void vrx_prestige_handle_class_skill(edict_t *ent, int option) {
    if (option >= 10000) {
        vrx_ability_open_select_menu(
            ent,
            "Select a new class skill",
            vrx_prestige_filter_class_skill,
            option - 10000,
            ent,
            vrx_prestige_handle_class_skill
        );
        return;
    }

    if (option >= 1000)
        option -= 1000;

    if (option < 0 || option >= MAX_ABILITIES) {
        menu_close(ent, false);
        return;
    }

    ent->myskills.prestige.classSkill[option / 32] |= 1 << (option % 32);
    vrx_add_ability(ent, option);
    ent->myskills.prestige.points -= 1;

    gi.cprintf(ent, PRINT_HIGH, "You have selected %s as a class skill.\n", GetAbilityString(option));
}

void vrx_prestige_handle_softmax_bump(edict_t *ent, int option) {
    if (option >= 10000) {
        vrx_ability_open_select_menu(
            ent,
            "Increase softmax",
            vrx_prestige_filter_softmax_bump,
            option - 10000,
            ent,
            vrx_prestige_handle_softmax_bump
        );

        return;
    }

    if (option >= 1000)
        option -= 1000;

    if (option < 0 || option >= MAX_ABILITIES) {
        menu_close(ent, false);
        return;
    }

    if (ent->myskills.prestige.softmaxBump[option] >= MAX_SOFTMAX_BUMP) {
        gi.cprintf(ent, PRINT_HIGH, "You have reached the maximum softmax bump for %s.\n", GetAbilityString(option));
        return;
    }

    ent->myskills.prestige.softmaxBump[option] += 1;
    ent->myskills.prestige.points -= 1;
    ent->myskills.abilities[option].max_level += 1;
    ent->myskills.abilities[option].hard_max = vrx_get_hard_max(option, 0, vrx_get_ability_class(option));

    gi.cprintf(ent, PRINT_HIGH, "You have increased the softmax of %s by 1.\n", GetAbilityString(option));
}

void vrx_prestige_ascend(edict_t *self) {
    // check if the player has enough experience to ascend
    if (self->myskills.experience < PRESTIGE_THRESHOLD)
        return;

    // check how many levels the player can ascend
    int upgradePoints = vrx_prestige_get_upgrade_points(self->myskills.experience);

    // add the upgrade points to the player's prestige total and current
    self->myskills.prestige.total += upgradePoints;
    self->myskills.prestige.points += upgradePoints;

    // reset them to start level
    vrx_change_class(self->myskills.player_name, self->myskills.class_num, 3);
}

void vrx_prestige_menu_handler(edict_t *self, int option) {
    if (option >= PRESTIGE_CREDITS && option <= PRESTIGE_SOFTMAX_BUMP) {
        if (self->myskills.prestige.points == 0) {
            safe_cprintf(self, PRINT_HIGH, "You do not have enough points to purchase this upgrade.\n");
            return;
        }
    }

    switch (option) {
        case PRESTIGE_CLASS_SKILL:
            vrx_ability_open_select_menu(
                self,
                "Select a new class skill",
                vrx_prestige_filter_class_skill,
                0,
                self,
                vrx_prestige_handle_class_skill
            );
            break;
        case PRESTIGE_CREDITS:
            self->myskills.prestige.creditLevel += 1;
            self->myskills.prestige.points -= 1;
            break;
        case PRESTIGE_SOFTMAX_BUMP:
            vrx_ability_open_select_menu(
                self,
                "Increase softmax",
                vrx_prestige_filter_softmax_bump,
                0,
                self,
                vrx_prestige_handle_softmax_bump
            );
            break;
        case PRESTIGE_ABILITY_POINTS:
            self->myskills.speciality_points += 1;
            self->myskills.prestige.abilityPoints += 1;
            self->myskills.prestige.points -= 1;
            gi.cprintf(self, PRINT_HIGH, "You have gained an additional ability point.\n");
            break;
        case PRESTIGE_WEAPON_POINTS:
            self->myskills.weapon_points += 4;
            self->myskills.prestige.weaponPoints += 1;
            self->myskills.prestige.points -= 1;
            gi.cprintf(self, PRINT_HIGH, "You have gained additional weapon points.\n");
            break;
        case PRESTIGE_ASCEND:
            vrx_prestige_ascend(self);
            break;
        default:
            menu_close(self, false);
    }
}

void vrx_prestige_open_menu(edict_t *self) {
    if (!menu_can_show(self))
        return;

    menu_clear(self);
    menu_add_line(self, va("Prestige %d", self->myskills.prestige.total), MENU_GREEN_CENTERED);
    menu_add_line(self, va("You have %d points available", self->myskills.prestige.points), MENU_WHITE_CENTERED);

    int rem = self->myskills.experience % PRESTIGE_THRESHOLD;
    menu_add_line(self, va("%d xp until next point", PRESTIGE_THRESHOLD - rem), MENU_WHITE_CENTERED);

    if (self->myskills.prestige.creditLevel) {
        menu_add_line(self, " ", 0);
        menu_add_line(self, va("Credit Earning Buff: %d%%",
                               self->myskills.prestige.creditLevel * PRESTIGE_CREDIT_BUFF_PERCENT), 0);
    }

    menu_add_line(self, " ", 0);

    for (int i = 0; i < sizeof(prestige) / sizeof(prestige[0]); i++) {
        menu_add_line(self, prestige[i].name, prestige[i].id);
    }

    int upgradePoints = self->myskills.experience / PRESTIGE_THRESHOLD;

    menu_add_line(self, " ", 0);
    if (upgradePoints) {
        menu_add_line(self, va("Ascend (%d)", upgradePoints), PRESTIGE_ASCEND);
    }

    menu_add_line(self, "Exit", 666);

    self->client->menustorage.currentline += self->client->menustorage.num_of_lines;
    menu_set_handler(self, vrx_prestige_menu_handler);
    menu_show(self);
}

qboolean vrx_prestige_has_ability(struct prestigelist_s *pre, uint32_t abIndex) {
    uint32_t arrIdx = abIndex / 32;
    uint32_t mask = 1 << (abIndex % 32);
    return (pre->classSkill[arrIdx] & mask) != 0;
}

void vrx_prestige_reapply_abilities(edict_t* self) {
    struct prestigelist_s *pre = &self->myskills.prestige;
    for (uint32_t abIndex = 0; abIndex < MAX_ABILITIES; abIndex++) {
        if (vrx_prestige_has_ability(pre, abIndex)) {
            vrx_add_ability(self, abIndex);
        }

        self->myskills.abilities[abIndex].max_level += pre->softmaxBump[abIndex];
    }
}

// must be applied after abilities
void vrx_prestige_reapply_all(edict_t *self) {
    struct prestigelist_s *pre = &self->myskills.prestige;
    self->myskills.speciality_points += pre->abilityPoints;
    self->myskills.weapon_points += pre->weaponPoints * 4;

    vrx_prestige_reapply_abilities(self);
}

qboolean vrx_prestige_has_class_skills(edict_t *self) {
    int size = sizeof(self->myskills.prestige.classSkill) / sizeof(self->myskills.prestige.classSkill[0]);
    for (uint32_t i = 0; i < size; i++) {
        if (self->myskills.prestige.classSkill[i])
            return true;
    }

    return false;
}
