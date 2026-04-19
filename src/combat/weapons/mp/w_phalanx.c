#include "g_local.h"

//
// ===== Weapon Fire =====
//

static void weapon_phalanx_fire(edict_t *ent)
{
    vec3_t start, forward, right, up, offset, v;
    int damage = PHALANX_INITIAL_DAMAGE + (int)(random() * 10.0) + (PHALANX_ADDON_DAMAGE * ent->myskills.weapons[WEAPON_PHALANX].mods[0].current_level);
    int radius_damage = PHALANX_INITIAL_RADIUS + (PHALANX_ADDON_RADIUS * ent->myskills.weapons[WEAPON_PHALANX].mods[1].current_level);
    float damage_radius = radius_damage = PHALANX_INITIAL_RADIUS + (PHALANX_ADDON_RADIUS * ent->myskills.weapons[WEAPON_PHALANX].mods[1].current_level);
    int speed = PHALANX_INITIAL_SPEED + (PHALANX_ADDON_SPEED * ent->myskills.weapons[WEAPON_PHALANX].mods[2].current_level);

    if (ent->myskills.weapons[WEAPON_PHALANX].mods[4].current_level)
        is_silenced = MZ_SILENCED;

    if (is_quad)
    {
        damage *= 4;
        radius_damage *= 4;
    }

    AngleVectors(ent->client->v_angle, forward, right, NULL);

    VectorScale(forward, -2, ent->client->kick_origin);
    ent->client->kick_angles[0] = -2;

    VectorSet(offset, 0, 8, ent->viewheight - 8);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

    if (ent->client->ps.gunframe == 8)
    {
        v[PITCH] = ent->client->v_angle[PITCH];
        v[YAW] = ent->client->v_angle[YAW] - 1.5;
        v[ROLL] = ent->client->v_angle[ROLL];
        AngleVectors(v, forward, right, up);

        fire_plasma(ent, start, forward, damage, speed, 120, 30);

        if (!((int)dmflags->value & DF_INFINITE_AMMO))
            ent->client->pers.inventory[ent->client->ammo_index]--;
    }
    else
    {
        v[PITCH] = ent->client->v_angle[PITCH];
        v[YAW] = ent->client->v_angle[YAW] + 1.5;
        v[ROLL] = ent->client->v_angle[ROLL];
        AngleVectors(v, forward, right, up);
        fire_plasma(ent, start, forward, damage, speed, damage_radius, radius_damage);

        if (!((int)dmflags->value & DF_INFINITE_AMMO))
            ent->client->pers.inventory[ent->client->ammo_index]--;

        gi.WriteByte(svc_muzzleflash);
        gi.WriteShort(ent - g_edicts);
        gi.WriteByte(MZ_PHALANX | is_silenced);
        gi.multicast(ent->s.origin, MULTICAST_PVS);

        PlayerNoise(ent, start, PNOISE_WEAPON);
    }

    ent->client->ps.gunframe++;
}

//
// ===== Weapon Think =====
//

void Weapon_Phalanx(edict_t *ent)
{
    static int pause_frames[] = {29, 42, 55, 0};
    static int fire_frames[] = {7, 8, 0};

    Weapon_Generic(ent, 5, 20, 58, 63, pause_frames, fire_frames, weapon_phalanx_fire);
    if (is_quadfire)
        Weapon_Generic(ent, 5, 20, 58, 63, pause_frames, fire_frames, weapon_phalanx_fire);
}
