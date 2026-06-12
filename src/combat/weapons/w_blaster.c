#include "g_local.h"

//
// ===== Weapon Fire =====
//

void Blaster_Fire(edict_t* ent, vec3_t g_offset, int damage, qboolean hyperblaster, int effect, int speed) {
    vec3_t forward, right;
    vec3_t start;
    vec3_t offset;
    //K03 Begin
    //trace_t tr;
    //vec3_t end;
    //K03 End

    //gi.dprintf("blaster_fire()\n");
    if (is_quad)
        damage *= 4;
    AngleVectors(ent->client->v_angle, forward, right, NULL);
    VectorSet(offset, 24, 8, ent->viewheight - 8);

    if (!(ent->svflags & SVF_MONSTER)) {
        VectorAdd(offset, g_offset, offset);
        P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
        VectorScale(forward, -2, ent->client->kick_origin);
        ent->client->kick_angles[0] = -1;
    }
    else {
        VectorSet(offset, 0, 0, ent->viewheight - 8);
        VectorAdd(offset, ent->s.origin, start);
    }

    // blast mode?
    if (!hyperblaster && ent->client && ent->client->weapon_mode)
        fire_blaster(ent, start, forward, damage, speed, effect, BLASTER_PROJ_BLAST, MOD_BLASTER, 2.0, true);
    // hyperblaster shot?
    else if (hyperblaster)
        fire_blaster(ent, start, forward, damage, speed, effect, BLASTER_PROJ_BOLT, MOD_HYPERBLASTER, 2.0, false);
    // normal blaster shot
    else
        fire_blaster(ent, start, forward, damage, speed, effect, BLASTER_PROJ_BOLT, MOD_BLASTER, 2.0, true);

    if (hyperblaster && ent->client->resp.pstats.weapons[WEAPON_HYPERBLASTER].mods[4].current_level < 1) {
        if (ent->client->resp.pstats.weapons[WEAPON_HYPERBLASTER].mods[4].current_level)
            is_silenced = MZ_SILENCED;
        // send muzzle flash
        gi.WriteByte(svc_muzzleflash);
        gi.WriteShort(ent - g_edicts);
        gi.WriteByte(MZ_HYPERBLASTER | is_silenced);
        gi.multicast(ent->s.origin, MULTICAST_PVS);
    }
    else if (!hyperblaster) {
        // send muzzle flash
        if (ent->client && ent->client->weapon_mode) {
            gi.WriteByte(svc_muzzleflash);
            gi.WriteShort(ent - g_edicts);
            gi.WriteByte(MZ_IONRIPPER | MZ_SILENCED);
            gi.multicast(ent->s.origin, MULTICAST_PVS);
            if (ent->client->resp.pstats.weapons[WEAPON_BLASTER].mods[4].current_level < 1)
                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/photon.wav"), 1, ATTN_NORM, 0);
            //else
            //gi.sound (ent, CHAN_WEAPON, gi.soundindex("weapons/photon.wav"), 0.3, ATTN_NORM, 0);
        }
        else {
            if (ent->client->resp.pstats.weapons[WEAPON_BLASTER].mods[4].current_level > 0)
                is_silenced = MZ_SILENCED;

            gi.WriteByte(svc_muzzleflash);
            gi.WriteShort(ent - g_edicts);
            gi.WriteByte(MZ_BLASTER | is_silenced);
            gi.multicast(ent->s.origin, MULTICAST_PVS);
        }
    }

    if (hyperblaster && ent->client->resp.pstats.weapons[WEAPON_HYPERBLASTER].mods[4].current_level < 1)
        PlayerNoise(ent, start, PNOISE_WEAPON);
    else if (!hyperblaster && ent->client->resp.pstats.weapons[WEAPON_BLASTER].mods[4].current_level < 1)
        PlayerNoise(ent, start, PNOISE_WEAPON);
    //K03 End
}

void Weapon_Blaster_Fire(edict_t* ent) {
    int min, max, damage, effect, ammo;
    const int speed =
    BLASTER_INITIAL_SPEED + BLASTER_ADDON_SPEED * ent->client->resp.pstats.weapons[WEAPON_BLASTER].mods[2].current_level;
    float temp;

    if (ent->client->resp.pstats.weapons[WEAPON_BLASTER].mods[3].current_level < 1)
        effect = EF_BLASTER;
    else
        effect = EF_HYPERBLASTER;

    min = BLASTER_INITIAL_DAMAGE_MIN +
    (BLASTER_ADDON_DAMAGE_MIN * ent->client->resp.pstats.weapons[WEAPON_BLASTER].mods[0].current_level);
    max = BLASTER_INITIAL_DAMAGE_MAX +
    (BLASTER_ADDON_DAMAGE_MAX * ent->client->resp.pstats.weapons[WEAPON_BLASTER].mods[0].current_level);
    damage = GetRandom(min, max);


    if (ent->client->weapon_mode) {
        temp = (float)ent->client->refire_frames / 10 * 2.0;
        if (temp > 5)
            temp = 5;
        damage *= temp;

        // ammo
        ammo = floattoint(temp);
        if (ammo < 1)
            ammo = 1;
    }
    else
        ammo = 1;

    // insufficient ammo
    if (ent->monsterinfo.lefty < ammo) {
        // play no ammo sound
        if (level.time >= ent->pain_debounce_time) {
            gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
            ent->pain_debounce_time = level.time + 1;
        }

        // keep weapon ready
        ent->client->ps.gunframe = 10;
        return;
    }

    ent->monsterinfo.lefty -= ammo; // decrement ammo counter

    if (ent->client->weapon_mode)
        safe_cprintf(ent, PRINT_HIGH, "%d damage blaster bolt fired (%.1fx).\n", damage, temp);

    Blaster_Fire(ent, vec3_origin, damage, false, effect, speed);
    // az FIXME: the blaster skips 2 frames at 10 fps at the first shot. it doesn't at 60.
    ent->client->ps.gunframe++;
    ent->client->refire_frames = 0;
}

//
// ===== Weapon Think =====
//

void Weapon_Blaster(edict_t* ent) {
    static int pause_frames[] = { 19, 32, 0 };
    static int fire_frames[] = { 5, 0 };
    //GHz START
    if (ent->mtype)
        return; // morph does not use weapons

        // are we in secondary mode?
        if (ent->client->weapon_mode) {
            // fire when button is released
            if (!(ent->client->buttons & BUTTON_ATTACK)) {
                if ((ent->client->ps.gunframe > 9) && (ent->client->refire_frames >= 10))
                    ent->client->buttons |= BUTTON_ATTACK;
                else
                    ent->client->refire_frames = 0;
            }
            else {
                // charge up weapon
                if (ent->client->refire_frames == 20) {
                    gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
                    gi.centerprintf(ent, "Blaster fully charged.\n(4.0x damage)\n");
                }

                if (ent->client->refire_frames == 10)
                    gi.centerprintf(ent, "Blaster 50%c charged.\n(2.0x damage)\n", '%');

                // dont fire yet
                ent->client->buttons &= ~BUTTON_ATTACK;
                ent->client->latched_buttons &= ~BUTTON_ATTACK;
                ent->client->refire_frames++;
            }
        }
        //GHz END
        Weapon_Generic(ent, 4, 8, 52, 55, pause_frames, fire_frames, Weapon_Blaster_Fire);
        // RAFAEL
        if (is_quadfire)
            Weapon_Generic(ent, 4, 8, 52, 55, pause_frames, fire_frames, Weapon_Blaster_Fire);
}
