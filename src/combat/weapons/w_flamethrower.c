#include "g_local.h"
#include "../../quake2/monsterframes/m_player.h"

//
// ===== Weapon Fire =====
//

void fire_flame(edict_t* self, vec3_t start, vec3_t dir, int speed, int damage);

void Flamethrower_Fire(edict_t* ent) {
    vec3_t start;
    vec3_t forward, right;
    vec3_t angles;
    int kick = 2;
    vec3_t offset;
    float damage = 10;
    int shots = 1;

    if (!(ent->client->buttons & BUTTON_ATTACK)) {
        ent->client->ps.gunframe++;
        return;
    }

    if (ent->client->ps.gunframe == 5)
        ent->client->ps.gunframe = 4;
    else
        ent->client->ps.gunframe = 5;

    /*
     * if (ent->client->pers.inventory[ent->client->ammo_index] < shots)
     *	shots = ent->client->pers.inventory[ent->client->ammo_index];
     *
     * if (!shots && !ent->client->pers.inventory[ent->client->ammo_index]) {
     *
     *	if (level.time >= ent->pain_debounce_time) {
     *		gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
     *		ent->pain_debounce_time = level.time + 1;
}

NoAmmoWeaponChange(ent);
return;
}*/

    if (is_quad) {
        damage *= 4;
        kick *= 4;
    }

    // get start / end positions
    VectorAdd(ent->client->v_angle, ent->client->kick_angles, angles);
    AngleVectors(angles, forward, right, NULL);
    VectorSet(offset, 0, 8, ent->viewheight - 8);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

    fire_flame(ent, start, forward, 600, 50);

    /*if (is_silenced)
     *	gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mg_silenced.wav"), 0.5, ATTN_NORM, 0);
     * else
     *	gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mg_unsilenced.wav"), 1, ATTN_NORM, 0);*/

    //if (ent->client->resp.pstats.weapons[WEAPON_MACHINEGUN].mods[4].current_level < 1) {
    gi.WriteByte(svc_muzzleflash);
    gi.WriteShort(ent - g_edicts);
    gi.WriteByte(MZ_PHALANX | MZ_SILENCED);
    gi.multicast(ent->s.origin, MULTICAST_PVS);
    //}
    //if (ent->client->resp.pstats.weapons[WEAPON_MACHINEGUN].mods[4].current_level < 1)
    PlayerNoise(ent, start, PNOISE_WEAPON);

    /*
     * if (!((int) dmflags->value & DF_INFINITE_AMMO))
     *	ent->client->pers.inventory[ent->client->ammo_index] -= shots;
     */

    ent->client->anim_priority = ANIM_ATTACK;
    if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
        ent->s.frame = FRAME_crattak1 - (int)(random() + 0.25);
        ent->client->anim_end = FRAME_crattak9;
    }
    else {
        ent->s.frame = FRAME_attack1 - (int)(random() + 0.25);
        ent->client->anim_end = FRAME_attack8;
    }
    ent->client->weaponstate = WEAPON_READY;
}

//
// ===== Weapon Think =====
//

void Weapon_Flamethrower(edict_t* ent) {
    static int pause_frames[] = { 23, 45, 0 };
    static int fire_frames[] = { 4, 5, 0 };

    Weapon_Generic(ent, 3, 5, 45, 49, pause_frames, fire_frames, Flamethrower_Fire);

    // RAFAEL
    if (is_quadfire)
        Weapon_Generic(ent, 3, 5, 45, 49, pause_frames, fire_frames, Flamethrower_Fire);
}
