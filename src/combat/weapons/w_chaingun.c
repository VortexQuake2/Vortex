#include "g_local.h"
#include "../../quake2/monsterframes/m_player.h"

//
// ===== Weapon Fire =====
//

void Chaingun_Fire(edict_t* ent) {
    int i;
    int shots;
    vec3_t start;
    vec3_t forward, right, up;
    float r, u;
    vec3_t offset;
    //int			kick = 2;

    //K03 Begin
    float damage = CHAINGUN_INITIAL_DAMAGE +
    CHAINGUN_ADDON_DAMAGE * ent->client->resp.pstats.weapons[WEAPON_CHAINGUN].mods[0].current_level;

    int vspread = DEFAULT_BULLET_VSPREAD;
    int hspread = DEFAULT_BULLET_HSPREAD;

    if (ent->client->resp.pstats.weapons[WEAPON_CHAINGUN].mods[3].current_level >= 1) {
        vspread *= 0.75;
        hspread *= 0.75;
    }
    //K03 End

    if (ent->client->ps.gunframe == 5)
        gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);

    if ((ent->client->ps.gunframe == 14) && !(ent->client->buttons & BUTTON_ATTACK)) {
        ent->client->ps.gunframe = 32;
        ent->client->weapon_sound = 0;
        ent->client->weaponstate = WEAPON_READY;
        return;
    }
    else if ((ent->client->ps.gunframe == 21) && (ent->client->buttons & BUTTON_ATTACK)
        && ent->client->pers.inventory[ent->client->ammo_index]) {
        ent->client->ps.gunframe = 15;
        }
        else {
            if (ent->client->ps.gunframe < 64)//K03
                ent->client->ps.gunframe++;
        }

        if (ent->client->ps.gunframe == 22) {
            ent->client->weapon_sound = 0;
            gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnd1a.wav"), 1, ATTN_IDLE, 0);
        }
        else {
            ent->client->weapon_sound = gi.soundindex("weapons/chngnl1a.wav");
        }

        if (ent->client->ps.gunframe <= 9)
            shots = 2;
    else if (ent->client->ps.gunframe <= 14) {
        if (ent->client->buttons & BUTTON_ATTACK)
            shots = 3;
        else
            shots = 2;
    }
    else
        shots = 4;

    if (ent->client->pers.inventory[ent->client->ammo_index] < shots)
        shots = ent->client->pers.inventory[ent->client->ammo_index];

    if (!shots) {
        if (level.time >= ent->pain_debounce_time) {
            gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
            ent->pain_debounce_time = level.time + 1;
        }
        NoAmmoWeaponChange(ent);
        return;
    }

    if (is_quad)
        damage *= 4;

    for (i = 0; i < 3; i++) {
        ent->client->kick_origin[i] = crandom() * 0.35;
        ent->client->kick_angles[i] = crandom() * 0.7;
    }

    for (i = 0; i < shots; i++) {
        // get start / end positions
        AngleVectors(ent->client->v_angle, forward, right, up);
        r = 7 + crandom() * 4;
        u = crandom() * 4;
        VectorSet(offset, 0, r, u + ent->viewheight - 8);
        P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

        fire_bullet(ent, start, forward, damage, 5, hspread, vspread, MOD_CHAINGUN);
    }

    //K03 begin
    if (ent->client->resp.pstats.weapons[WEAPON_CHAINGUN].mods[2].current_level >= 1)
        if (ent->lasthbshot <= level.time) {
            damage = CHAINGUN_ADDON_TRACERDAMAGE * ent->client->resp.pstats.weapons[WEAPON_CHAINGUN].mods[2].current_level;
            fire_blaster(ent, start, forward, damage, 2000, EF_BLUEHYPERBLASTER, BLASTER_PROJ_BOLT, MOD_HYPERBLASTER,
                         2.0, false);
            ent->lasthbshot = level.time + 0.5;
        }
        if (ent->client->resp.pstats.weapons[WEAPON_CHAINGUN].mods[4].current_level < 1) {
            // send muzzle flash
            gi.WriteByte(svc_muzzleflash);
            gi.WriteShort(ent - g_edicts);
            gi.WriteByte((MZ_CHAINGUN1 + shots - 1) | is_silenced);
            gi.multicast(ent->s.origin, MULTICAST_PVS);
        }
        if (ent->client->resp.pstats.weapons[WEAPON_CHAINGUN].mods[4].current_level < 1)
            PlayerNoise(ent, start, PNOISE_WEAPON);
    //K03 End

    // ### Hentai ### BEGIN

    ent->client->anim_priority = ANIM_ATTACK;
    if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
        ent->s.frame = FRAME_crattak1 - 1 + (ent->client->ps.gunframe % 3);
        ent->client->anim_end = FRAME_crattak9;
    }
    else {
        ent->s.frame = FRAME_attack1 - 1 + (ent->client->ps.gunframe % 3);
        ent->client->anim_end = FRAME_attack8;
    }


    // ### Hentai ### END

    if (!((int)dmflags->value & DF_INFINITE_AMMO))
        ent->client->pers.inventory[ent->client->ammo_index] -= shots;
}


void AssaultCannon_Fire(edict_t* ent) {
    int shots, i;
    int damage, kick, vspread, hspread;
    float f, r, u;
    qboolean canfire = false;
    vec3_t forward, right, start, up, offset;

    damage = kick = 2 * (CHAINGUN_INITIAL_DAMAGE +
    CHAINGUN_ADDON_DAMAGE * ent->client->resp.pstats.weapons[WEAPON_CHAINGUN].mods[0].current_level);
    vspread = DEFAULT_BULLET_VSPREAD;
    hspread = DEFAULT_BULLET_HSPREAD;

    if (is_quad) {
        damage *= 4;
        kick *= 4;
    }

    if (ent->client->resp.pstats.weapons[WEAPON_CHAINGUN].mods[3].current_level > 0) {
        vspread *= 0.75;
        hspread *= 0.75;
    }

    if (ent->client->resp.pstats.weapons[WEAPON_CHAINGUN].mods[4].current_level > 0)
        f = 0.3;
    else
        f = 1;

    if (ent->client->ps.gunframe <= 14) {
        // beginning animation, so play spin-up sound
        if (ent->client->ps.gunframe == 5)
            gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/asscan_spinup.wav"), f, ATTN_NORM, 0);

        ent->client->ps.gunframe++; // we're done, so advance to next frame
        return;
    }
    else if ((ent->client->ps.gunframe == 15) && !(ent->client->buttons & BUTTON_ATTACK)) {
        ent->client->ps.gunframe = 32;
        ent->client->weapon_sound = 0;
        ent->client->weaponstate = WEAPON_READY;
        return;
    }
    // attack frames loop
    else if ((ent->client->ps.gunframe == 21) && (ent->client->buttons & BUTTON_ATTACK)
        && ent->client->pers.inventory[ent->client->ammo_index]) {
        ent->client->ps.gunframe = 15; // go to beginning of fire frames
        }
        else {
            ent->client->ps.gunframe++;
        }

    if (ent->groundentity && (VectorLength(ent->velocity) < 1))
        canfire = true;

    if (ent->client->ps.gunframe == 22)
        gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/asscan_endfire.wav"), f, ATTN_NORM, 0);
    else if (!(sf2qf(level.framenum) % 2)) {
        if (canfire)
            gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/asscan_fire.wav"), f, ATTN_NORM, 0);
        else
            gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/asscan_pause.wav"), f, ATTN_NORM, 0);
    }


    if (!canfire)
        return;

    shots = 4;

    // if we don't have enough ammo, use whatever we have
    if (ent->client->pers.inventory[ent->client->ammo_index] < shots)
        shots = ent->client->pers.inventory[ent->client->ammo_index];

    // out of ammo
    if (!shots) {
        if (level.time >= ent->pain_debounce_time) {
            gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
            ent->pain_debounce_time = level.time + 1;
        }
        NoAmmoWeaponChange(ent);
        return;
    }

    // change player view
    for (i = 0; i < 3; i++) {
        ent->client->kick_origin[i] = crandom() * 1.5;
        ent->client->kick_angles[i] = crandom() * 3;
    }

    if (ent->client->resp.pstats.weapons[WEAPON_CHAINGUN].mods[4].current_level < 1) {
        // send muzzle flash
        gi.WriteByte(svc_muzzleflash);
        gi.WriteShort(ent - g_edicts);
        gi.WriteByte(MZ_IONRIPPER | MZ_SILENCED);
        gi.multicast(ent->s.origin, MULTICAST_PVS);
    }
    PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

    for (i = 0; i < shots; i++) {
        // get start / end positions
        AngleVectors(ent->client->v_angle, forward, right, up);
        r = 7 + crandom() * 4;
        u = crandom() * 4;
        VectorSet(offset, 0, r, u + ent->viewheight - 8);
        P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

        fire_bullet(ent, start, forward, damage, kick, hspread, vspread, MOD_CHAINGUN);
    }

    if ((ent->client->resp.pstats.weapons[WEAPON_CHAINGUN].mods[2].current_level > 0) && (level.time >= ent->lasthbshot)) {
        damage = CHAINGUN_ADDON_TRACERDAMAGE * ent->client->resp.pstats.weapons[WEAPON_CHAINGUN].mods[2].current_level;
        fire_blaster(ent, start, forward, damage, 2000, 0, BLASTER_PROJ_BOLT, MOD_HYPERBLASTER, 2.0, false);
        ent->lasthbshot = level.time + 0.3;
    }

    if (!((int)dmflags->value & DF_INFINITE_AMMO))
        ent->client->pers.inventory[ent->client->ammo_index] -= shots;
}

//
// ===== Weapon Think =====
//

void Weapon_Chaingun(edict_t* ent) {
    static int pause_frames[] = { 38, 43, 51, 61, 0 };
    static int fire_frames[] = { 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 0 };


    //K03 Begin
    int fire_last = 31;

    // spin-up delay
    if (ent->client->resp.pstats.weapons[WEAPON_CHAINGUN].mods[1].current_level > 0) {
        if (ent->client->resp.pstats.weapons[WEAPON_CHAINGUN].mods[1].current_level > 9)
            fire_last = 21;
        else if (ent->client->resp.pstats.weapons[WEAPON_CHAINGUN].mods[1].current_level > 7)
            fire_last = 23;
        else if (ent->client->resp.pstats.weapons[WEAPON_CHAINGUN].mods[1].current_level > 5)
            fire_last = 25;
        else if (ent->client->resp.pstats.weapons[WEAPON_CHAINGUN].mods[1].current_level > 2)
            fire_last = 27;
        else fire_last = 29;
    }

    if (ent->client->weapon_mode)
        Weapon_Generic(ent, 4, fire_last, 61, 64, pause_frames, fire_frames, AssaultCannon_Fire);
    else
        Weapon_Generic(ent, 4, fire_last, 61, 64, pause_frames, fire_frames, Chaingun_Fire);
    //K03 End

    // RAFAEL
    if (is_quadfire)
        Weapon_Generic(ent, 4, 31, 61, 64, pause_frames, fire_frames, Chaingun_Fire);
}
