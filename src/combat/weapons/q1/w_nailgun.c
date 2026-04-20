#include "g_local.h"

void weapon_etf_rifle_fire(edict_t *ent);

void Weapon_Nailgun(edict_t* ent) {
    static int pause_frames[] = { 7, 0 };
    static int fire_frames[] = { 1, 2, 3, 4, 0 };

    Weapon_Generic(ent, -1, 4, 7, 7, pause_frames, fire_frames, weapon_etf_rifle_fire);
}
