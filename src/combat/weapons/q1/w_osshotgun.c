#include "g_local.h"

void weapon_supershotgun_fire(edict_t* ent);

void Weapon_OldSuperShotgun(edict_t* ent) {
    static int pause_frames[] = { 7, 0 };
    static int fire_frames[] = { 1, 0 };

    // this needs custom logic
    Weapon_Generic(ent, -1, 6, 7, 7, pause_frames, fire_frames, weapon_supershotgun_fire);
}
