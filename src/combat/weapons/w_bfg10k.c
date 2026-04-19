#include "g_local.h"

//
// ===== Weapon Fire =====
//

void weapon_bfg_fire(edict_t* ent) {
    vec3_t offset, start;
    vec3_t forward, right;
    int dmg, speed;
    float range;

    speed = BFG10K_INITIAL_SPEED + BFG10K_ADDON_SPEED * ent->myskills.weapons[WEAPON_BFG10K].mods[2].current_level;
    range = BFG10K_RADIUS;
    dmg = BFG10K_INITIAL_DAMAGE + BFG10K_ADDON_DAMAGE * ent->myskills.weapons[WEAPON_BFG10K].mods[0].current_level;

    if (is_quad)
        dmg *= 4;

    if (ent->client->ps.gunframe == 9) {
        //	gi.dprintf("fired bfg at %.1f\n", level.time);
        // send muzzle flash
        if (ent->myskills.weapons[WEAPON_BFG10K].mods[4].current_level < 1) {
            gi.WriteByte(svc_muzzleflash);
            gi.WriteShort(ent - g_edicts);
            gi.WriteByte(MZ_BFG | is_silenced);
            gi.multicast(ent->s.origin, MULTICAST_PVS);
        }

        ent->client->ps.gunframe++;

        if (ent->client->pers.inventory[ent->client->ammo_index] >= 50) {
            AngleVectors(ent->client->v_angle, forward, right, NULL);
            VectorScale(forward, -2, ent->client->kick_origin);

            // make a big pitch kick with an inverse fall
            ent->client->v_dmg_pitch = -20;
            ent->client->v_dmg_roll = crandom() * 4;
            ent->client->v_dmg_time = level.time + DAMAGE_TIME;

            VectorSet(offset, 8, 8, ent->viewheight - 8);
            P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

            if (ent->myskills.weapons[WEAPON_BFG10K].mods[4].current_level < 1)
                PlayerNoise(ent, start, PNOISE_WEAPON);
            fire_bfg(ent, start, forward, dmg, speed, range);

            if (!((int)dmflags->value & DF_INFINITE_AMMO))
                ent->client->pers.inventory[ent->client->ammo_index] -= 50;
        }
        return;
    }

    // cells can go down during windup (from power armor hits), so
    // check again and abort firing if we don't have enough now
    if (ent->client->pers.inventory[ent->client->ammo_index] < 50) {
        ent->client->ps.gunframe++;
        return;
    }

    AngleVectors(ent->client->v_angle, forward, right, NULL);

    VectorScale(forward, -2, ent->client->kick_origin);
    /*
     *	// make a big pitch kick with an inverse fall
     *	ent->client->v_dmg_pitch = -40;
     *	ent->client->v_dmg_roll = crandom()*8;
     *	ent->client->v_dmg_time = level.time + DAMAGE_TIME;
     *
     *	VectorSet(offset, 8, 8, ent->viewheight-8);
     *	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
     *	fire_bfg (ent, start, forward, dmg, speed, range); */

    ent->client->ps.gunframe++;

    /*if (ent->myskills.weapons[WEAPON_BFG10K].mods[4].current_level < 1)
     *	PlayerNoise(ent, start, PNOISE_WEAPON);
     *
     * if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
     *	ent->client->pers.inventory[ent->client->ammo_index] -= 25;*/
}

//
// ===== Weapon Think =====
//

void Weapon_BFG(edict_t* ent) {
    static int pause_frames[] = { 39, 45, 50, 55, 0 };
    static int fire_frames[] = { 9, 17, 0 };

    Weapon_Generic(ent, 8, 32, 55, 58, pause_frames, fire_frames, weapon_bfg_fire);
    //K03 Endda
    // RAFAEL
    //	if (is_quadfire)
    //		Weapon_Generic (ent, 8, 32, 55, 58, pause_frames, fire_frames, weapon_bfg_fire);
}
