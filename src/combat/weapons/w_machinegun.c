#include "g_local.h"
#include "../../quake2/monsterframes/m_player.h"

//
// ===== Weapon Fire =====
//

void Machinegun_Fire(edict_t* ent) {
    int i;
    vec3_t start;
    vec3_t forward, right;
    vec3_t angles;
    int kick = 2;
    vec3_t offset;
    float damage = MACHINEGUN_INITIAL_DAMAGE +
    MACHINEGUN_ADDON_DAMAGE * ent->myskills.weapons[WEAPON_MACHINEGUN].mods[0].current_level;
    int vspread = DEFAULT_BULLET_VSPREAD;
    int hspread = DEFAULT_BULLET_HSPREAD;
    int shots = 1;

    if (ent->myskills.weapons[WEAPON_MACHINEGUN].mods[4].current_level)
        is_silenced = MZ_SILENCED;

    // bullet spread is reduced while in burst mode
    if (ent->client->weapon_mode) {
        vspread *= 0.5;
        hspread *= 0.5;
    }
    // bullet spread is reduced when mg is upgraded
    if (ent->myskills.weapons[WEAPON_MACHINEGUN].mods[3].current_level >= 1) {
        vspread *= 0.75;
        hspread *= 0.75;
    }

    if (!(ent->client->buttons & BUTTON_ATTACK)) {
        ent->client->machinegun_shots = 0;
        ent->client->ps.gunframe++;
        return;
    }
    // keep track of burst bullets
    if (ent->client->weapon_mode)
        ent->client->burst_count++;

    if (ent->client->ps.gunframe == 5)
        ent->client->ps.gunframe = 4;
    else
        ent->client->ps.gunframe = 5;

    if (ent->client->pers.inventory[ent->client->ammo_index] < shots)
        shots = ent->client->pers.inventory[ent->client->ammo_index];
    if (!shots && !ent->client->pers.inventory[ent->client->ammo_index]) {
        if (level.time >= ent->pain_debounce_time) {
            gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
            ent->pain_debounce_time = level.time + 1;
        }
        ent->client->burst_count = 0;
        NoAmmoWeaponChange(ent);
        return;
    }

    if (is_quad) {
        damage *= 4;
        kick *= 4;
    }

    for (i = 1; i < 3; i++) {
        ent->client->kick_origin[i] = crandom() * 0.35;
        ent->client->kick_angles[i] = crandom() * 0.7;
    }
    ent->client->kick_origin[0] = crandom() * 0.35;
    ent->client->kick_angles[0] = ent->client->machinegun_shots * -1.5;

    // get start / end positions
    VectorAdd(ent->client->v_angle, ent->client->kick_angles, angles);
    AngleVectors(angles, forward, right, NULL);
    VectorSet(offset, 0, 8, ent->viewheight - 8);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
    //K03 Begin
    if (ent->client->weapon_mode) {
        //	shots *= 2;
        damage *= 2;
    }
    for (i = 0; i < shots; i++) {
        fire_bullet(ent, start, forward, damage, kick, hspread, vspread, MOD_MACHINEGUN);
    }
    // fire tracers
    if (ent->lasthbshot <= level.time) {
        if (ent->myskills.weapons[WEAPON_MACHINEGUN].mods[2].current_level >= 1) {
            damage = MACHINEGUN_ADDON_TRACERDAMAGE * ent->myskills.weapons[WEAPON_MACHINEGUN].mods[2].current_level;
            fire_blaster(ent, start, forward, damage, 2000, EF_BLUEHYPERBLASTER, BLASTER_PROJ_BOLT, MOD_HYPERBLASTER,
                         2.0, false);
        }
        ent->lasthbshot = level.time + 0.5;
    }

    if (is_silenced)
        gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mg_silenced.wav"), 0.5, ATTN_NORM, 0);
    else
        gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mg_unsilenced.wav"), 1, ATTN_NORM, 0);

    if (ent->myskills.weapons[WEAPON_MACHINEGUN].mods[4].current_level < 1) {
        gi.WriteByte(svc_muzzleflash);
        gi.WriteShort(ent - g_edicts);
        gi.WriteByte(MZ_IONRIPPER | MZ_SILENCED);
        gi.multicast(ent->s.origin, MULTICAST_PVS);
    }
    if (ent->myskills.weapons[WEAPON_MACHINEGUN].mods[4].current_level < 1)
        PlayerNoise(ent, start, PNOISE_WEAPON);
    //K03 End

    if (!((int)dmflags->value & DF_INFINITE_AMMO))
        ent->client->pers.inventory[ent->client->ammo_index] -= shots;

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
    ent->client->vrr.gun_fire_time = level.time + 0.1; // wait before next firing
}

//
// ===== Weapon Think =====
//

void Weapon_Machinegun(edict_t* ent) {
    static int pause_frames[] = { 23, 45, 0 };
    static int fire_frames[] = { 4, 5, 0 };

    // burst fire mode fires 5 shots and then waits 0.5 seconds to fire another burst
    if (ent->client->weapon_mode) {

        if (!(ent->client->buttons & BUTTON_ATTACK)) {
            // reset counters if we have completed a burst or are idle
            if (ent->client->burst_count > 4 || ent->client->burst_count == 0) {
                ent->client->machinegun_shots = 0;
                ent->client->burst_count = 0;
            }
            else {
                // continue burst if it wasn't completed
                if (ent->client->burst_count < 5) {
                    ent->client->buttons |= BUTTON_ATTACK;
                    ent->client->trap_time = level.time + 1.0;
                }
            }
        }
        else {
            // delay between each burst
            if (ent->client->trap_time > level.time) {
                ent->client->burst_count = 0;
                ent->client->buttons &= ~BUTTON_ATTACK;
                ent->client->latched_buttons &= ~BUTTON_ATTACK;
            }
            // burst completed
            if (ent->client->burst_count >= 4)
                ent->client->trap_time = level.time + 1.0;
        }
    }
    Weapon_Generic(ent, 3, 5, 45, 49, pause_frames, fire_frames, Machinegun_Fire);

    // RAFAEL
    if (is_quadfire)
        Weapon_Generic(ent, 3, 5, 45, 49, pause_frames, fire_frames, Machinegun_Fire);
}
