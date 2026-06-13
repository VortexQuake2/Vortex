#include "g_local.h"

//
// ===== Weapon Fire =====
//

void weapon_grenadelauncher_fire(edict_t* ent) {
    vec3_t offset;
    vec3_t forward, right;
    vec3_t start;

    int damage = (int)(GRENADELAUNCHER_INITIAL_DAMAGE +
    GRENADELAUNCHER_ADDON_DAMAGE *
    ent->client->resp.pstats.weapons[WEAPON_GRENADELAUNCHER].mods[0].current_level);
    const float radius = (float)(GRENADELAUNCHER_INITIAL_RADIUS +
    GRENADELAUNCHER_ADDON_RADIUS *
    ent->client->resp.pstats.weapons[WEAPON_GRENADELAUNCHER].mods[1].current_level);
    const int speed = (int)(GRENADELAUNCHER_INITIAL_SPEED +
    (GRENADELAUNCHER_ADDON_SPEED *
    ent->client->resp.pstats.weapons[WEAPON_GRENADELAUNCHER].mods[2].current_level));
    const int radius_damage = (int)(GRENADELAUNCHER_INITIAL_RADIUS_DAMAGE +
    GRENADELAUNCHER_ADDON_RADIUS_DAMAGE *
    ent->client->resp.pstats.weapons[WEAPON_GRENADELAUNCHER].mods[0].current_level);

    if (is_quad)
        damage *= 4;

    //GHz: We dont have enough ammo to fire, so change weapon and abort
    if (ent->client->pers.inventory[ent->client->ammo_index] < 1) {
        ent->client->ps.gunframe++;
        gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
        NoAmmoWeaponChange(ent);
        return;
    }

    VectorSet(offset, 8, 8, ent->viewheight - 8);
    AngleVectors(ent->client->v_angle, forward, right, NULL);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

    VectorScale(forward, -2, ent->client->kick_origin);
    ent->client->kick_angles[0] = -1;

    //K03 Begin
    if ((ent->max_pipes < MAX_PIPES) || !ent->client->weapon_mode) {
        fire_grenade(ent, start, forward, damage, speed, 2.5, radius, radius_damage);
        if (!((int)dmflags->value & DF_INFINITE_AMMO))
            ent->client->pers.inventory[ent->client->ammo_index]--;
        if (ent->client->weapon_mode)
            ent->max_pipes++;
    }

    if (ent->client->resp.pstats.weapons[WEAPON_GRENADELAUNCHER].mods[4].current_level < 1) {
        gi.WriteByte(svc_muzzleflash);
        gi.WriteShort(ent - g_edicts);
        gi.WriteByte(MZ_GRENADE | is_silenced);
        gi.multicast(ent->s.origin, MULTICAST_PVS);
    }
    //K03 End

    ent->client->ps.gunframe++;

    //K03 Begin
    if (ent->client->resp.pstats.weapons[WEAPON_GRENADELAUNCHER].mods[4].current_level < 1)
        PlayerNoise(ent, start, PNOISE_WEAPON);
    //K03 End
}

//
// ===== Weapon Think =====
//

void Weapon_GrenadeLauncher(edict_t* ent) {
    const int fire_last = 13;//16;
    static int pause_frames[] = { 34, 51, 59, 0 };
    static int fire_frames[] = { 6, 0 };

    Weapon_Generic(ent, 5, fire_last, 59, 64, pause_frames, fire_frames, weapon_grenadelauncher_fire);

    // RAFAEL
    if (is_quadfire)
        Weapon_Generic(ent, 5, 16, 59, 64, pause_frames, fire_frames, weapon_grenadelauncher_fire);
}
