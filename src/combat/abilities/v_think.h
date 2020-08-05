
#ifndef VORTEXQUAKE2_V_THINK_H
#define VORTEXQUAKE2_V_THINK_H

void think_idle_frame_counter(const edict_t *ent);

void think_player_inactivity(edict_t *ent);

void think_chat_protect_activate(edict_t *ent);

qboolean CanSuperSpeed(edict_t *ent);

#endif //VORTEXQUAKE2_V_THINK_H
