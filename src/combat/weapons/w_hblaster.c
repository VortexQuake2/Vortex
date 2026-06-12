#include "g_local.h"
#include "../../quake2/monsterframes/m_player.h"

//
// ===== Weapon Fire =====
//

void Blaster_Fire(edict_t* ent, vec3_t g_offset, int damage, qboolean hyperblaster, int effect, int speed);

void Weapon_HyperBlaster_Fire(edict_t* ent) {
    float rotation;
    vec3_t offset;
    int effect;
    int damage;
    //GHz START
    int i;
    int speed, shots = 0;
    qboolean fire_this_frame = false;

    // only fire every other frame
    //if (ent->client->ps.gunframe == 6 || ent->client->ps.gunframe == 9
    //	|| ent->client->ps.gunframe == 15)
    //{
    fire_this_frame = true;
    shots++;
    //}
    // get weapon properties
    damage = HYPERBLASTER_INITIAL_DAMAGE +
    HYPERBLASTER_ADDON_DAMAGE * ent->client->resp.pstats.weapons[WEAPON_HYPERBLASTER].mods[0].current_level;
    speed = (HYPERBLASTER_INITIAL_SPEED +
    HYPERBLASTER_ADDON_SPEED * ent->client->resp.pstats.weapons[WEAPON_HYPERBLASTER].mods[2].current_level);

    if (ent->client->resp.pstats.weapons[WEAPON_HYPERBLASTER].mods[4].current_level)
        is_silenced = MZ_SILENCED;
    //GHz END

    ent->client->weapon_sound = gi.soundindex("weapons/hyprbl1a.wav");

    if (!(ent->client->buttons & BUTTON_ATTACK)) {
        ent->client->ps.gunframe++;
    }
    else {
        //GHz START
        if (ent->client->pers.inventory[ent->client->ammo_index] < shots)
            shots = ent->client->pers.inventory[ent->client->ammo_index];

        if (!shots && !ent->client->pers.inventory[ent->client->ammo_index])
            //GHz END
        {
            if (level.time >= ent->pain_debounce_time) {
                gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
                ent->pain_debounce_time = level.time + 1;
            }
            NoAmmoWeaponChange(ent);
        }
        else {
            //GHz START
            for (i = 0; i < shots; i++) {
                //gi.dprintf("Fired HB for %d damage at %.1f\n", damage, level.time);

                rotation = (ent->client->ps.gunframe + i - 5) * 2 * M_PI / 6;
                offset[0] = -4 * sin(rotation);
                offset[1] = 0;
                offset[2] = 4 * cos(rotation);

                if ((ent->client->ps.gunframe == 6) || (ent->client->ps.gunframe == 9))
                    effect = EF_HYPERBLASTER;
                else
                    effect = 0;

                Blaster_Fire(ent, offset, damage, true, effect, speed);

                if (!((int)dmflags->value & DF_INFINITE_AMMO))
                    ent->client->pers.inventory[ent->client->ammo_index]--;
            }
            //GHz END

            ent->client->anim_priority = ANIM_ATTACK;
            if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
                ent->s.frame = FRAME_crattak1 - 1;
                ent->client->anim_end = FRAME_crattak9;
            }
            else {
                ent->s.frame = FRAME_attack1 - 1;
                ent->client->anim_end = FRAME_attack8;
            }
        }

        ent->client->ps.gunframe++;
        if (ent->client->ps.gunframe == 12 && ent->client->pers.inventory[ent->client->ammo_index])
            ent->client->ps.gunframe = 6;
    }

    if (ent->client->ps.gunframe == 12) {
        gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/hyprbd1a.wav"), 1, ATTN_NORM, 0);
        ent->client->weapon_sound = 0;
    }

}

//
// ===== Weapon Think =====
//

void Weapon_HyperBlaster(edict_t* ent) {
    static int pause_frames[] = { 0 };
    static int fire_frames[] = { 6, 7, 8, 9, 10, 11, 0 };

    Weapon_Generic(ent, 5, 20, 49, 53, pause_frames, fire_frames, Weapon_HyperBlaster_Fire);

    // RAFAEL
    if (is_quadfire)
        Weapon_Generic(ent, 5, 20, 49, 53, pause_frames, fire_frames, Weapon_HyperBlaster_Fire);
}

