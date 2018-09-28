//
// Created by machindramon on 21-09-2018.
//

#ifndef VORTEXQUAKE2_V_HW_H
#define VORTEXQUAKE2_V_HW_H

float hw_getdamagefactor(edict_t *targ, edict_t *attacker);

void hw_awardpoints(void);

qboolean hw_pickupflag(edict_t *ent, edict_t *other);

void hw_dropflag(edict_t *ent, gitem_t *item);

void hw_spawnflag(void);

// 3.0 new holy vortex mode
void hw_init();

#endif //VORTEXQUAKE2_V_HW_H
