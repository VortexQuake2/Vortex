#include "g_local.h"

//
// ===== Weapon Fire =====
//

static void weapon_ionripper_fire(edict_t *ent)
{
    vec3_t start, forward, right, offset, tempang;
    int damage = IONRIPPER_INITIAL_DAMAGE +
    ( IONRIPPER_ADDON_DAMAGE * ent->client->resp.pstats.weapons[WEAPON_IONRIPPER].mods[0].current_level );
    int kick = 60;
    int speed = IONRIPPER_INITIAL_SPEED +
    ( IONRIPPER_ADDON_SPEED * ent->client->resp.pstats.weapons[WEAPON_IONRIPPER].mods[2].current_level );

    if (is_quad)
    {
        damage *= 4;
        kick *= 4;
    }

    VectorCopy(ent->client->v_angle, tempang);
    tempang[YAW] += crandom();
    AngleVectors(tempang, forward, right, NULL);

    VectorScale(forward, -3, ent->client->kick_origin);
    ent->client->kick_angles[0] = -3;

    VectorSet(offset, 16, 7, ent->viewheight - 8);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

    fire_ionripper(ent, start, forward, damage, speed, EF_IONRIPPER);

    gi.WriteByte(svc_muzzleflash);
    gi.WriteShort(ent - g_edicts);
    gi.WriteByte(MZ_IONRIPPER | is_silenced);
    gi.multicast(ent->s.origin, MULTICAST_PVS);

    ent->client->ps.gunframe++;
    PlayerNoise(ent, start, PNOISE_WEAPON);

    if (!((int)dmflags->value & DF_INFINITE_AMMO))
    {
        ent->client->pers.inventory[ent->client->ammo_index] -= ent->client->pers.weapon->quantity;
        if (ent->client->pers.inventory[ent->client->ammo_index] < 0)
            ent->client->pers.inventory[ent->client->ammo_index] = 0;
    }
}


//
// ===== Weapon Think =====
//

void Weapon_Ionripper(edict_t *ent)
{
    static int pause_frames[] = {36, 0};
    static int fire_frames[] = {5, 0};

    Weapon_Generic(ent, 4, 6, 36, 39, pause_frames, fire_frames, weapon_ionripper_fire);
    if (is_quadfire)
        Weapon_Generic(ent, 4, 6, 36, 39, pause_frames, fire_frames, weapon_ionripper_fire);
}

