#include "g_local.h"

void Weapon_RocketLauncher_Fire(edict_t* ent);

void Weapon_OldRocketLauncher(edict_t* ent) {
    static int pause_frames[] = { 7, 0 };
    static int fire_frames[] = { 1, 0 };

    Weapon_Generic(ent, -1, 6, 7, 7, pause_frames, fire_frames, Weapon_RocketLauncher_Fire);
}
