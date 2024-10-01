#ifndef PRESTIGE_H
#define PRESTIGE_H

#include <combat/abilities/ability_def.h>
#include <stdint.h>

#define MAX_SOFTMAX_BUMP 10

enum PrestigeType {
    PRESTIGE_CREDITS = 1,
    PRESTIGE_ABILITY_POINTS = 2,
    PRESTIGE_CLASS_SKILL = 3,
    PRESTIGE_WEAPON_POINTS = 4,
    PRESTIGE_SOFTMAX_BUMP = 5,
    PRESTIGE_ASCEND = 999
};

typedef struct prestigelist_s {
    uint32_t points;
    uint32_t total;
  uint8_t softmaxBump[MAX_ABILITIES];
  uint32_t creditLevel;
  uint32_t abilityPoints;
  uint32_t weaponPoints;

  abilitybitmap_t classSkill;
} prestigelist_t;

#define PRESTIGE_CREDIT_BUFF_PERCENT 30
#define PRESTIGE_CREDIT_BUFF_MULTIPLIER (1.0 + (PRESTIGE_CREDIT_BUFF_PERCENT / 100.0))

void vrx_prestige_global_init();
void vrx_prestige_init(edict_t *pUser);
void vrx_prestige_reapply_all(edict_t *self);
void vrx_prestige_reapply_abilities(edict_t *self);
void vrx_prestige_open_menu(edict_t *self);
uint32_t vrx_prestige_get_upgrade_points(uint32_t exp);
qboolean vrx_prestige_has_class_skills(edict_t *self);
qboolean vrx_prestige_has_ability(struct prestigelist_s *pre, uint32_t abIndex);

#endif //PRESTIGE_H
