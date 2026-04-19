#include "g_local.h"

//
// ===== Weapon Fire =====
//

void weapon_supershotgun_fire(edict_t* ent) {
    vec3_t start;
    vec3_t forward, right;
    vec3_t offset;
    vec3_t v;
    int kick = 12;
    //K03 Begin
    float damage = SUPERSHOTGUN_INITIAL_DAMAGE +
    SUPERSHOTGUN_ADDON_DAMAGE * ent->myskills.weapons[WEAPON_SUPERSHOTGUN].mods[0].current_level;
    int vspread = DEFAULT_SHOTGUN_VSPREAD;
    int hspread = DEFAULT_SHOTGUN_HSPREAD;
    int bullets = SUPERSHOTGUN_INITIAL_BULLETS +
    SUPERSHOTGUN_ADDON_BULLETS * ent->myskills.weapons[WEAPON_SUPERSHOTGUN].mods[2].current_level;
    //	float		temp;

    if (ent->myskills.weapons[WEAPON_SUPERSHOTGUN].mods[3].current_level >= 1) {
        vspread *= 0.75;
        hspread *= 0.75;
    }

    //K03 End

    AngleVectors(ent->client->v_angle, forward, right, NULL);

    VectorScale(forward, -2, ent->client->kick_origin);
    ent->client->kick_angles[0] = -2;

    VectorSet(offset, 0, 8, ent->viewheight - 8);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

    if (is_quad) {
        damage *= 4;
        kick *= 4;
    }

    v[PITCH] = ent->client->v_angle[PITCH];
    v[YAW] = ent->client->v_angle[YAW] - 5;
    v[ROLL] = ent->client->v_angle[ROLL];
    AngleVectors(v, forward, NULL, NULL);
    /*
     * // special handling of pellet weapons with ghost
     * temp = 0.033 * ent->myskills.ghost_level;
     * if (temp >= random())
     *	damage = 0;
     */

    bullets = ceil((float)bullets / 2);
    //	gi.dprintf("%d dmg %d bullets\n", damage, bullets);
    fire_shotgun(ent, start, forward, damage, kick, hspread, vspread, bullets, MOD_SSHOTGUN);//K03
    v[YAW] = ent->client->v_angle[YAW] + 5;
    AngleVectors(v, forward, NULL, NULL);
    fire_shotgun(ent, start, forward, damage, kick, hspread, vspread, bullets, MOD_SSHOTGUN);//K03

    //K03 Begin
    if (ent->myskills.weapons[WEAPON_SUPERSHOTGUN].mods[4].current_level < 1) {
        // send muzzle flash
        gi.WriteByte(svc_muzzleflash);
        gi.WriteShort(ent - g_edicts);
        gi.WriteByte(MZ_SSHOTGUN | is_silenced);
        gi.multicast(ent->s.origin, MULTICAST_PVS);
    }
    //K03 End

    ent->client->ps.gunframe++;
    //K03 Begin
    if (ent->myskills.weapons[WEAPON_SUPERSHOTGUN].mods[4].current_level < 1)
        PlayerNoise(ent, start, PNOISE_WEAPON);
    //K03 End

    if (!((int)dmflags->value & DF_INFINITE_AMMO))
        ent->client->pers.inventory[ent->client->ammo_index] -= 2;
}

//
// ===== Weapon Think =====
//

void Weapon_SuperShotgun(edict_t* ent) {
    static int pause_frames[] = { 29, 42, 57, 0 };
    static int fire_frames[] = { 7, 0 };

    Weapon_Generic(ent, 6, 17, 57, 61, pause_frames, fire_frames, weapon_supershotgun_fire);
    //K03 End

    // RAFAEL
    if (is_quadfire)
        Weapon_Generic(ent, 6, 17, 57, 61, pause_frames, fire_frames, weapon_supershotgun_fire);
}
