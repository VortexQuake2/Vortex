#include "g_local.h"

//
// ===== Weapon Fire =====
//

void weapon_20mm_fire(edict_t* ent) {
    vec3_t start;
    vec3_t forward, right;
    vec3_t offset;

    int damage;//25 + (int) floor(2.5 * ent->myskills.weapons[WEAPON_20MM].mods[0].current_level);
    int kick;//= 150 - (100 * ent->myskills.weapons[WEAPON_20MM].mods[3].current_level);

    //min = WEAPON_20MM_INITIAL_DMG_MIN + WEAPON_20MM_ADDON_DMG_MIN*ent->myskills.weapons[WEAPON_20MM].mods[0].current_level;
    //max = WEAPON_20MM_INITIAL_DMG_MAX + WEAPON_20MM_ADDON_DMG_MAX*ent->myskills.weapons[WEAPON_20MM].mods[0].current_level;
    //damage = GetRandom(min, max);

    //4.57
    damage = WEAPON_20MM_INITIAL_DMG + WEAPON_20MM_ADDON_DMG * ent->myskills.weapons[WEAPON_20MM].mods[0].current_level;
    kick = damage;
    if (ent->myskills.weapons[WEAPON_20MM].mods[3].current_level)
        kick *= 0.5;
    //if (kick < 0)
    //	kick = 0;

    if (!ent->groundentity && !ent->waterlevel) {
        ent->client->ps.gunframe = 5;
        if (ent->client && !(ent->svflags & SVF_MONSTER))
            safe_cprintf(ent, PRINT_HIGH, "You must be stepping on the ground or in water to fire the 20mm cannon.\n");
        return;
    }

    if (is_quad) {
        damage *= 4;
    }

    AngleVectors(ent->client->v_angle, forward, right, NULL);
    VectorScale(forward, -3, ent->client->kick_origin);
    VectorSet(offset, 0, 7, ent->viewheight - 8);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

    const int range = WEAPON_20MM_INITIAL_RANGE + (WEAPON_20MM_ADDON_RANGE * ent->myskills.weapons[WEAPON_20MM].mods[1].current_level);
    //gi.dprintf("called fire_20mm() at %f for %d damage\n", level.time, damage);
    fire_20mm(ent, start, forward, damage, kick, range);

    if (ent->myskills.weapons[WEAPON_20MM].mods[4].current_level < 1) {
        // send muzzle flash
        gi.WriteByte(svc_muzzleflash);
        gi.WriteShort(ent - g_edicts);
        gi.WriteByte(MZ_IONRIPPER | MZ_SILENCED);
        gi.multicast(ent->s.origin, MULTICAST_PVS);
    }

    ent->client->ps.gunframe++;
    ent->client->vrr.gun_fire_time = level.time + 0.1;

    if (ent->myskills.weapons[WEAPON_20MM].mods[4].current_level < 1) {
        PlayerNoise(ent, start, PNOISE_WEAPON);
        gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/sgun1.wav"), 1, ATTN_NORM, 0);
    }
    else {
        gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/sgun1.wav"), 0.3, ATTN_NORM, 0);
    }

    if (!((int)dmflags->value & DF_INFINITE_AMMO))
        ent->client->pers.inventory[ent->client->ammo_index]--;
}

//
// ===== Weapon Think =====
//

void Weapon_20mm(edict_t* ent) {
    static int pause_frames[] = { 56, 0 };
    static int fire_frames[] = { 4, 0 };

    //K03 Begin
    int fire_last = 18;

    Weapon_Generic(ent, 3, 4, 56, 61, pause_frames, fire_frames, weapon_20mm_fire);
    //K03 End

    // RAFAEL
    if (is_quadfire)
        Weapon_Generic(ent, 3, 18, 56, 61, pause_frames, fire_frames, weapon_20mm_fire);
}
