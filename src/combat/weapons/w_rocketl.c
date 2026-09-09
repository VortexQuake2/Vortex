#include "g_local.h"

//
// ===== Weapon Fire =====
//

void Weapon_RocketLauncher_Fire(edict_t* ent) {
    vec3_t offset, start;
    vec3_t forward, right;
    int damage;
    float damage_radius;
    int radius_damage;

    //K03 Begin
    const int speed = ROCKETLAUNCHER_INITIAL_SPEED +
    ROCKETLAUNCHER_ADDON_SPEED * ent->client->resp.pstats.weapons[WEAPON_ROCKETLAUNCHER].mods[2].current_level;
    damage = ROCKETLAUNCHER_INITIAL_DAMAGE +
    ROCKETLAUNCHER_ADDON_DAMAGE * ent->client->resp.pstats.weapons[WEAPON_ROCKETLAUNCHER].mods[0].current_level;
    radius_damage = ROCKETLAUNCHER_INITIAL_RADIUS_DAMAGE + ROCKETLAUNCHER_ADDON_RADIUS_DAMAGE *
    ent->client->resp.pstats.weapons[WEAPON_ROCKETLAUNCHER].mods[0].current_level;
    damage_radius = ROCKETLAUNCHER_INITIAL_DAMAGE_RADIUS + ROCKETLAUNCHER_ADDON_DAMAGE_RADIUS *
    ent->client->resp.pstats.weapons[WEAPON_ROCKETLAUNCHER].mods[1].current_level;
    //K03 End
    if (is_quad) {
        damage *= 4;
        radius_damage *= 4;
    }

    AngleVectors(ent->client->v_angle, forward, right, NULL);

    VectorScale(forward, -2, ent->client->kick_origin);
    ent->client->kick_angles[0] = -1;

    VectorSet(offset, 8, 8, ent->viewheight - 8);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
    //K03 Begin
    //gi.dprintf("called rocketlauncher_fire at %d (%d)\n", level.framenum, ent->client->ps.gunframe);
    fire_rocket(ent, start, forward, damage, speed, damage_radius, radius_damage);

    if (ent->client->resp.pstats.weapons[WEAPON_ROCKETLAUNCHER].mods[4].current_level < 1) {
        // send muzzle flash
        gi.WriteByte(svc_muzzleflash);
        gi.WriteShort(ent - g_edicts);
        gi.WriteByte(MZ_ROCKET | is_silenced);
        gi.multicast(ent->s.origin, MULTICAST_PVS);
    }
    //K03 End

    ent->client->ps.gunframe++;

    if (ent->client->resp.pstats.weapons[WEAPON_ROCKETLAUNCHER].mods[4].current_level < 1)//K03
        PlayerNoise(ent, start, PNOISE_WEAPON);

    if (!((int)dmflags->value & DF_INFINITE_AMMO))
        ent->client->pers.inventory[ent->client->ammo_index]--;
}

//
// ===== Weapon Think =====
//

void Weapon_RocketLauncher(edict_t* ent) {
    static int pause_frames[] = { 25, 33, 42, 50, 0 };
    static int fire_frames[] = { 5, 0 };

    //K03 Begin
    const int fire_last = 12;

    Weapon_Generic(ent, 4, fire_last, 50, 54, pause_frames, fire_frames, Weapon_RocketLauncher_Fire);
    //K03 End
    // RAFAEL
    if (is_quadfire)
        Weapon_Generic(ent, 4, 12, 50, 54, pause_frames, fire_frames, Weapon_RocketLauncher_Fire);

}
