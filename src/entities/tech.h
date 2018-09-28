#ifndef VORTEXQUAKE2_TECH_H
#define VORTEXQUAKE2_TECH_H

void tech_think(edict_t *self);

void tech_drop(edict_t *ent, gitem_t *item);

qboolean tech_spawn(int index);

void tech_checkrespawn(edict_t *ent);

void tech_spawnall(void);

qboolean tech_pickup(edict_t *ent, edict_t *other);

void tech_dropall(edict_t *ent);


#endif //VORTEXQUAKE2_TECH_H
