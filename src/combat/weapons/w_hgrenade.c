#include "g_local.h"
#include "../../quake2/monsterframes/m_player.h"

//
// ===== Weapon Fire =====
//

#define GRENADE_TIMER            3.0
#define GRENADE_MINSPEED        400
#define GRENADE_MAXSPEED        800
#define GRENADE_INITIAL_SPEED    800
#define GRENADE_ADDON_SPEED        40

float get_weapon_grenade_speed(edict_t* ent)
{
    int speed, min_speed;
    const int max_speed = GRENADE_INITIAL_SPEED + GRENADE_ADDON_SPEED * ent->myskills.weapons[WEAPON_HANDGRENADE].mods[1].current_level;
    float timer;

    timer = ent->client->grenade_time - level.time;
    min_speed = 0.5 * max_speed;

    if (ent->health <= 0)
        speed = min_speed;
    else
        speed = min_speed + (GRENADE_TIMER - timer) * ((max_speed - min_speed) / GRENADE_TIMER);
    return speed;
}

void weapon_grenade_fire(edict_t* ent, qboolean held) {
    vec3_t offset;
    vec3_t forward, right;
    vec3_t start;
    int damage = GRENADE_INITIAL_DAMAGE +
    GRENADE_ADDON_DAMAGE * ent->myskills.weapons[WEAPON_HANDGRENADE].mods[0].current_level;//K03
    const int radius_damage = GRENADE_INITIAL_RADIUS_DAMAGE +
    GRENADE_ADDON_RADIUS_DAMAGE * ent->myskills.weapons[WEAPON_HANDGRENADE].mods[0].current_level;
    float timer;
    float speed;
    // int speed, min_speed;
    // int max_speed = GRENADE_INITIAL_SPEED +
    //                GRENADE_ADDON_SPEED * ent->myskills.weapons[WEAPON_HANDGRENADE].mods[1].current_level;
    float radius;

    //3.0 disable chat protect (somehow throwing a hg does not reset a client's idle frames)
    //4.5 the first shot merely resets the chatprotect state, but doesn't actually fire
    if (ent->flags & FL_CHATPROTECT) {
        vrx_remove_chat_protect(ent);

        // re-draw weapon
        ent->client->newweapon = ent->client->pers.weapon;
        ent->client->weaponstate = WEAPON_DROPPING;
        ChangeWeapon(ent);
        return;
    }

    radius = GRENADE_INITIAL_RADIUS +
    GRENADE_ADDON_RADIUS * ent->myskills.weapons[WEAPON_HANDGRENADE].mods[2].current_level;//K03
    if (is_quad)
        damage *= 4;

    VectorSet(offset, 8, 8, ent->viewheight - 8);
    AngleVectors(ent->client->v_angle, forward, right, NULL);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

    timer = ent->client->grenade_time - level.time;
    /*
     *   min_speed = 0.5 * max_speed;
     *
     *   if ( ent->health <= 0 )
     *       speed = min_speed;
     *   else
     *       speed = min_speed + (GRENADE_TIMER - timer) * ((max_speed - min_speed) / GRENADE_TIMER);
     */
    speed = get_weapon_grenade_speed(ent);

    fire_grenade2(ent, start, forward, damage, speed, timer, radius, radius_damage, held);
    //gi.dprintf("fired grenade at %.1f\n", level.time);
    //gi.dprintf("fired grenade at %.1f\n", timer);

    //K03 Begin
    ent->shots++;
    ent->myskills.shots++;
    ent->svflags &= ~SVF_NOCLIENT;
    if (ent->myskills.abilities[CLOAK].current_level < 10 || vrx_get_talent_level(ent, TALENT_IMP_CLOAK) < 4) {
        ent->client->cloaking = false;
        ent->client->cloakable = 0;
    }
    //K03 End

    // ### Hentai ### BEGIN

    /*	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
     * {
     *	ent->client->anim_priority = ANIM_ATTACK;
     *	ent->s.frame = FRAME_crattak1-1;
     *	ent->client->anim_end = FRAME_crattak3;
}
else
{
ent->client->anim_priority = ANIM_REVERSE;
ent->s.frame = FRAME_wave08;
ent->client->anim_end = FRAME_wave01;
}*/
    // ### Hentai ### END

    if (!((int)dmflags->value & DF_INFINITE_AMMO))
        ent->client->pers.inventory[ent->client->ammo_index]--;

    ent->client->grenade_time = level.time + 1.0;

    //K03 Begin
    if (ent->myskills.weapons[WEAPON_HANDGRENADE].mods[4].current_level < 1) {
        gi.WriteByte(svc_muzzleflash);
        gi.WriteShort(ent - g_edicts);
        gi.WriteByte(MZ_BLASTER | is_silenced);
        gi.multicast(ent->s.origin, MULTICAST_PVS);
    }
    //K03 ENd

    if (ent->deadflag || ent->s.modelindex != 255) // VWep animations screw up corpses
    {
        return;
    }

    if (ent->health <= 0)
        return;

    if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
        ent->client->anim_priority = ANIM_ATTACK;
        ent->s.frame = FRAME_crattak1 - 1;
        ent->client->anim_end = FRAME_crattak3;
    }
    else {
        ent->client->anim_priority = ANIM_REVERSE;
        ent->s.frame = FRAME_wave08;
        ent->client->anim_end = FRAME_wave01;
    }
}

//
// ===== Weapon Think =====
//

void Weapon_Grenade2(edict_t* ent) {
    start:
    qboolean can_run_frame = ent->client->vrr.gun_statemachine_time >= 0.1;
    auto haste_wait = calculate_haste_wait(ent);

    if (ent->shield)
        return;
    //gi.dprintf("gunframe = %d\n", ent->client->ps.gunframe);
    if ((ent->client->newweapon) && (ent->client->weaponstate == WEAPON_READY)) {
        ChangeWeapon(ent);
        return;
    }

    if (ent->client->weaponstate == WEAPON_ACTIVATING) {
        ent->client->weaponstate = WEAPON_READY;
        ent->client->ps.gunframe = 16;
        return;
    }

    if (ent->client->weaponstate == WEAPON_READY) {
        if (((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK)) {
            ent->client->latched_buttons &= ~BUTTON_ATTACK;
            if (ent->client->pers.inventory[ent->client->ammo_index]) {
                print_wp_state(ent, "firingSTART");
                ent->client->ps.gunframe = 1;
                ent->client->weaponstate = WEAPON_FIRING;
                ent->client->grenade_time = 0;
                ent->client->grenade_delay = 0;

                ent->client->vrr.gun_statemachine_time = 0.0;
                ent->haste_time = 0;
            }
            else {
                if (level.time >= ent->pain_debounce_time) {
                    if (ent->myskills.weapons[WEAPON_HANDGRENADE].mods[4].current_level < 1)//K03
                        gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
                    ent->pain_debounce_time = level.time + 1;
                }
                NoAmmoWeaponChange(ent);
            }

            return;
        }

        if ((ent->client->ps.gunframe == 29) || (ent->client->ps.gunframe == 34) || (ent->client->ps.gunframe == 39) ||
            (ent->client->ps.gunframe == 48)) {
            if (randomMT() & 15) {
                ent->client->vrr.gun_statemachine_time -= 0.1; // advance the frame timer anyway
                return;
            }
            }

            if (can_run_frame) {
                // print_wp_state(ent, "READY");
                ++ent->client->ps.gunframe;
                ent->client->vrr.gun_statemachine_time -= 0.1;
            }

            if (ent->client->ps.gunframe > 48)
                ent->client->ps.gunframe = 16;


        return;
    }

    if (ent->client->weaponstate == WEAPON_FIRING) {
        if (ent->client->ps.gunframe == 5 && ent->myskills.weapons[WEAPON_HANDGRENADE].mods[4].current_level < 1) {//K03
            if (can_run_frame)
                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/hgrena1b.wav"), 1, ATTN_NORM, 0);
        }

        if (ent->client->ps.gunframe == 11) {
            if (!ent->client->grenade_time) {
                ent->client->grenade_time = level.time + GRENADE_TIMER + 0.2;
                if (ent->myskills.weapons[WEAPON_HANDGRENADE].mods[4].current_level < 1)//K03
                    ent->client->weapon_sound = gi.soundindex("weapons/hgrenc1b.wav");
            }


            // they waited too long, detonate it in their hand
            if (!ent->client->grenade_blew_up && level.time >= ent->client->grenade_time) {
                ent->client->weapon_sound = 0;
                weapon_grenade_fire(ent, true);
                ent->client->grenade_blew_up = true;
            }


            print_wp_state(ent, "firing");

            if (ent->client->buttons & BUTTON_ATTACK)
                return;

            if (ent->client->grenade_blew_up) {
                if (level.time >= ent->client->grenade_time) {
                    ent->client->ps.gunframe = 15;
                    ent->client->grenade_blew_up = false;
                }
                else {
                    return;
                }
            }
        }

        if (ent->client->ps.gunframe == 12) {
            ent->client->weapon_sound = 0;
            weapon_grenade_fire(ent, false);
            ent->client->vrr.gun_statemachine_time = 0.1;
            can_run_frame = true;
        }

        bool grenade_pending = (level.time < ent->client->grenade_time);
        // az: let em spam nades with haste
        if ((ent->client->ps.gunframe == 15) && (grenade_pending && !is_haste_active(ent)))
            return;

        //ent->client->ps.gunframe++;

        // (apple)
        // This will skip half the priming animation, thus shortening
        // the firing delay by about half. To keep the same refire rate
        // a delay is applied after the priming phase. This will keep the
        // HG from going into its next weaponstate until delay is done.
        if (can_run_frame) {
            print_wp_state(ent, "adv frame");
            if (ent->client->ps.gunframe < 11) {
                ent->client->ps.gunframe += 2;
                ent->client->grenade_delay++;
            }
            else if (ent->client->grenade_delay > 0) {
                ent->client->ps.gunframe++;
                ent->client->grenade_delay--;
            }
            else {
                ent->client->ps.gunframe++;

            }

            ent->client->vrr.gun_statemachine_time -= 0.1;
        }

        if (ent->client->ps.gunframe >= 16 && ent->client->grenade_delay == 0) {
            ent->client->grenade_time = 0;
            ent->client->weaponstate = WEAPON_READY;
            print_wp_state(ent, "ChangetoREADY");
        }

        bool is_haste_frame = ent->client->ps.gunframe < 11 ||
        (ent->client->ps.gunframe > 11 && ent->client->grenade_delay > 0);
        if (is_haste_frame && is_haste_active(ent)) {
            if (haste_wait != -1 && ent->haste_time > haste_wait) {
                ent->client->vrr.gun_statemachine_time += 0.1;
                ent->haste_time -= haste_wait;
                goto start;
            }
            ent->haste_time += FRAMETIME;
        }
    }
}

//K03 Begin
void Weapon_Grenade(edict_t* ent) {
    // can't use gun if we are just spawning, or we
    // are a morphed player using anything other
    // than a player model
    if (ent->deadflag || (ent->s.modelindex != 255) || (vrx_is_morphing_polt(ent))
        || (ctf->value && ctf_enable_balanced_fc->value && vrx_has_flag(ent))
        || (ent->client->respawn_time > level.time)
        || (ent->flags & FL_WORMHOLE))
        return;

    Weapon_Grenade2(ent);

    //	if (ent->myskills.weapons[WEAPON_HANDGRENADE].mods[1].current_level > 9)
    //		Weapon_Grenade2(ent);
    //	if (ent->myskills.weapons[WEAPON_HANDGRENADE].mods[1].current_level > 4)
    //		Weapon_Grenade2(ent);

}
//K03 End
