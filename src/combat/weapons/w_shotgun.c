#include "g_local.h"

//
// ===== Weapon Fire =====
//

void weapon_shotgun_fire(edict_t* ent) {
    vec3_t start;
    vec3_t forward, right;
    vec3_t offset;
    int kick = 8;
    float temp;

    //K03 Begin
    float damage =
    SHOTGUN_INITIAL_DAMAGE + SHOTGUN_ADDON_DAMAGE * ent->myskills.weapons[WEAPON_SHOTGUN].mods[0].current_level;
    int vspread = 500;
    int hspread = 500;
    const int bullets = SHOTGUN_INITIAL_BULLETS +
    SHOTGUN_ADDON_BULLETS * ent->myskills.weapons[WEAPON_SHOTGUN].mods[2].current_level;
    if (ent->myskills.weapons[WEAPON_SHOTGUN].mods[3].current_level >= 1) {
        vspread *= 0.75;
        hspread *= 0.75;
    }

    //K03 End

    if (ent->client->ps.gunframe == 9) {
        ent->client->ps.gunframe++;
        return;
    }

    //gi.dprintf("fired shotgun at %d\n", level.framenum);

    AngleVectors(ent->client->v_angle, forward, right, NULL);

    VectorScale(forward, -2, ent->client->kick_origin);
    ent->client->kick_angles[0] = -2;

    VectorSet(offset, 0, 8, ent->viewheight - 8);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

    if (is_quad) {
        damage *= 4;
        kick *= 4;
    }

    // shotgun strike upgrade
    // 20% chance to deal double damage at level 10
    temp = 1.0 / (1.0 + 0.025 * ent->myskills.weapons[WEAPON_SHOTGUN].mods[1].current_level);

    if (random() > temp) {
        damage *= 1.5;
        gi.sound(ent, CHAN_WEAPON, gi.soundindex("ctf/tech2.wav"), 1, ATTN_NORM, 0);
    }

    fire_shotgun(ent, start, forward, damage, kick, vspread, hspread, bullets, MOD_SHOTGUN);
    // send muzzle flash
    if (ent->myskills.weapons[WEAPON_SHOTGUN].mods[4].current_level < 1) {
        gi.WriteByte(svc_muzzleflash);
        gi.WriteShort(ent - g_edicts);
        gi.WriteByte(MZ_SHOTGUN | is_silenced);
        gi.multicast(ent->s.origin, MULTICAST_PVS);
    }
    //K03 End

    ent->client->ps.gunframe++;
    //K03 Begin
    if (ent->myskills.weapons[WEAPON_SHOTGUN].mods[4].current_level < 1)
        PlayerNoise(ent, start, PNOISE_WEAPON);
    //K03 End

    if (!((int)dmflags->value & DF_INFINITE_AMMO))
        ent->client->pers.inventory[ent->client->ammo_index]--;
}

//
// ===== Weapon Think =====
//

void Weapon_Shotgun(edict_t* ent) {
    static int pause_frames[] = { 22, 28, 34, 0 };
    static int fire_frames[] = { 8, 9, 0 };

    //K03 Begin
    const int fire_last = 16;
    /*
     * if (ent->myskills.weapons[WEAPON_SHOTGUN].mods[1].current_level > 0)
     * {
     *	if (ent->myskills.weapons[WEAPON_SHOTGUN].mods[1].current_level > 9)
     *		fire_last = 14;
     *	else if (ent->myskills.weapons[WEAPON_SHOTGUN].mods[1].current_level > 6)
     *		fire_last = 15;
     *	else if (ent->myskills.weapons[WEAPON_SHOTGUN].mods[1].current_level > 3)
     *		fire_last = 16;
     *	else fire_last = 17;
}*/

    Weapon_Generic(ent, 7, fire_last, 36, 39, pause_frames, fire_frames, weapon_shotgun_fire);
    //K03 End

    // RAFAEL
    if (is_quadfire)
        Weapon_Generic(ent, 7, 18, 36, 39, pause_frames, fire_frames, weapon_shotgun_fire);
}
