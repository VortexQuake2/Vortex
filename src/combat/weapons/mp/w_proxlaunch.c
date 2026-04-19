#include "g_local.h"

//
// ===== Weapon Fire =====
//

static void weapon_proxlauncher_fire(edict_t *ent)
{
    vec3_t offset, start, forward, right;
    int damage = 90 + 4 * ent->myskills.weapons[WEAPON_PROXLAUNCHER].mods[0].current_level;
    float radius = damage + 30 + 2 * ent->myskills.weapons[WEAPON_PROXLAUNCHER].mods[1].current_level;
    int speed = 600 + 15 * ent->myskills.weapons[WEAPON_PROXLAUNCHER].mods[2].current_level;

    if (is_quad)
        damage *= 4;

    AngleVectors(ent->client->v_angle, forward, right, NULL);
    VectorScale(forward, -2, ent->client->kick_origin);
    ent->client->kick_angles[0] = -1;

    VectorSet(offset, 8, 8, ent->viewheight - 8);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
    fire_prox(ent, start, forward, damage, speed, radius);

    gi.WriteByte(svc_muzzleflash);
    gi.WriteShort(ent - g_edicts);
    gi.WriteByte(MZ_GRENADE | is_silenced);
    gi.multicast(ent->s.origin, MULTICAST_PVS);
    PlayerNoise(ent, start, PNOISE_WEAPON);

    if (!((int)dmflags->value & DF_INFINITE_AMMO))
        ent->client->pers.inventory[ent->client->ammo_index] -= ent->client->pers.weapon->quantity;

    ent->client->ps.gunframe++;
}

//
// ===== Weapon Think =====
//

void Weapon_ProxLauncher(edict_t *ent)
{
    static int pause_frames[] = {34, 51, 59, 0};
    static int fire_frames[] = {6, 0};

    Weapon_Generic(ent, 5, 16, 59, 64, pause_frames, fire_frames, weapon_proxlauncher_fire);
}
