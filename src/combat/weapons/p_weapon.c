// g_weapon.c

#include "g_local.h"
#include "../../quake2/monsterframes/m_player.h"

qboolean is_quad;
// RAFAEL
qboolean is_quadfire;
byte is_silenced;

void lasersight_on(edict_t *ent);

void lasersight_off(edict_t *ent);

void P_ProjectSource(gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result) {
    vec3_t _distance;

    VectorCopy(distance, _distance);
    if (client->pers.hand == LEFT_HANDED)
        _distance[1] *= -1;
    else if (client->pers.hand == CENTER_HANDED)
        _distance[1] = 0;
    G_ProjectSource(point, _distance, forward, right, result);
}

void PlayerNoise(edict_t *who, vec3_t where, int type) {
    // commented out ages ago (between 3.6 and 4.5), deleted v5.99.4
}

void ShowGun(edict_t *ent);

qboolean Pickup_Weapon(edict_t *ent, edict_t *other) {
    int index;
    gitem_t *ammo;

    index = ITEM_INDEX(ent->item);

    if ((((int) (dmflags->value) & DF_WEAPONS_STAY) || coop->value)
        && other->client->pers.inventory[index]) {
        if (!(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)))
            return false; // leave the weapon for others to pickup
    }

    //K03 Begin
    if (other->client->pers.inventory[index] == 0)
        other->client->pers.inventory[index]++;
    //K03 End

    if (!(ent->spawnflags & DROPPED_ITEM)) {
        // give them some ammo with it
        if (ent->item->ammo != NULL) {
            ammo = FindItem(ent->item->ammo);

            if ((int) dmflags->value & DF_INFINITE_AMMO)
                Add_Ammo(other, ammo, 1000);
            else
                Add_Ammo(other, ammo, ammo->quantity);
        }

        if (!(ent->spawnflags & DROPPED_PLAYER_ITEM)) {
            if (deathmatch->value) {
                if ((int) (dmflags->value) & DF_WEAPONS_STAY)
                    ent->flags |= FL_RESPAWN;
                else
                    SetRespawn(ent, 30);
            }
            if (coop->value)
                ent->flags |= FL_RESPAWN;
        }
    }

    if (other->client->pers.weapon != ent->item && // not the same weapon equipped
        (other->client->pers.inventory[index] == 1) && // do not already have one
        (!deathmatch->value || other->client->pers.weapon == Fdi_BLASTER)) {
        if ((V_WeaponUpgradeVal(other, WEAPON_BLASTER) < 1)
            && (other->myskills.respawn_weapon != 13)) //4.2 don't switch weapons if blaster is set as respawn weapon
            other->client->newweapon = ent->item;
    }

    return true;
}

/*
================
ToggleSecondary

Returns true if this weapon has secondary fire, and
prints client messages confirming mode change
================
*/
qboolean ToggleSecondary(edict_t *ent, gitem_t *item, qboolean printmsg) {
    if (!ent->mtype) {
        if (!item)
            return false; // must have a weapon
        if (ent->client->weaponstate != WEAPON_READY)
            return false; // weapon must be ready
    }

    if (ent->client->weapon_mode) {
        // flag carrier in CTF can't use weapons
        if (ctf->value && ctf_enable_balanced_fc->value && vrx_has_flag(ent))
            return false;


        if (strcmp(item->pickup_name, "Sword") == 0) {
            if (printmsg)
                safe_cprintf(ent, PRINT_HIGH, "Normal firing\n");
            return true;
        }
        if (strcmp(item->pickup_name, "Blaster") == 0) {
            if (printmsg)
                safe_cprintf(ent, PRINT_HIGH, "Normal firing\n");
            return true;
        }
        if (strcmp(item->pickup_name, "Railgun") == 0) {
            if (printmsg)
                safe_cprintf(ent, PRINT_HIGH, "Normal firing\n");
            return true;
        }
        if (strcmp(item->pickup_name, "Grenade Launcher") == 0) {
            if (printmsg)
                safe_cprintf(ent, PRINT_HIGH, "Normal firing\n");
            return true;
        }
        if (strcmp(item->pickup_name, "Machinegun") == 0) {
            if (printmsg)
                safe_cprintf(ent, PRINT_HIGH, "Full automatic\n");
            return true;
        }
        if (strcmp(item->pickup_name, "Chaingun") == 0) {
            if (printmsg)
                safe_cprintf(ent, PRINT_HIGH, "Normal firing\n");
            return true;
        }
    } else {
        if (!item) // TODO: Should the check be here, or should togglesecondary not be called?
            return false;

        if (strcmp(item->pickup_name, "Sword") == 0) {
            if (printmsg)
                safe_cprintf(ent, PRINT_HIGH, "Lance mode\n");
            return true;
        }
        if (strcmp(item->pickup_name, "Blaster") == 0) {
            if (printmsg)
                safe_cprintf(ent, PRINT_HIGH, "Blast mode\n");
            return true;
        }
        if (strcmp(item->pickup_name, "Railgun") == 0) {
            if (printmsg)
                safe_cprintf(ent, PRINT_HIGH, "Sniper mode\n");
            return true;
        }
        if (strcmp(item->pickup_name, "Grenade Launcher") == 0) {
            if (printmsg)
                safe_cprintf(ent, PRINT_HIGH, "Pipebomb mode\n");
            return true;
        }
        if (strcmp(item->pickup_name, "Machinegun") == 0) {
            if (printmsg)
                safe_cprintf(ent, PRINT_HIGH, "Burst fire\n");
            return true;
        }
        if (strcmp(item->pickup_name, "Chaingun") == 0) {
            if (printmsg)
                safe_cprintf(ent, PRINT_HIGH, "Assault cannon\n");
            return true;
        }
    }
    return false;
}

/*
===============
ChangeWeapon

The old weapon has been dropped all the way, so make the new one
current
===============
*/
// ### Hentai ### BEGIN
void ShowGun(edict_t *ent) {
    int i, j;

    if (!ent->client->pers.weapon) {
        ent->s.modelindex2 = 0;
        return;
    }
    if (!vwep->value) {
        ent->s.modelindex2 = 255;
        return;
    }

    ent->s.modelindex2 = 255;
    if (ent->client->pers.weapon)
        i = ((j & 0xff) << 8);
    else
        i = 0;

    ent->s.skinnum = (ent - g_edicts - 1) | i;
}

// ### Hentai ### END

void ChangeWeapon(edict_t *ent) {
    char *mdl;

    ent->client->refire_frames = 0;
    lasersight_off(ent);

    if (ent->client->grenade_time) {
        ent->client->grenade_time = level.time;
        ent->client->weapon_sound = 0;
        ent->client->grenade_time = 0;
    }

    ent->client->pers.lastweapon = ent->client->pers.weapon;
    ent->client->pers.weapon = ent->client->newweapon;
    ent->client->newweapon = NULL;
    ent->client->machinegun_shots = 0;
    ent->client->burst_count = 0;
    ent->client->vrr.gun_statemachine_time = 0;

    if (ent->client->pers.weapon && ent->client->pers.weapon->ammo)
        ent->client->ammo_index = ITEM_INDEX(FindItem(ent->client->pers.weapon->ammo));
    else
        ent->client->ammo_index = 0;

    if (!ent->client->pers.weapon || ent->deadflag == DEAD_DEAD) {
        // dead
        ent->client->ps.gunindex = 0;
        return;
    }

    ent->client->weaponstate = WEAPON_ACTIVATING;
    ent->client->ps.gunframe = 0;
    //lm ctf
    mdl = ent->client->pers.weapon->view_model;
    ent->client->ps.gunindex = gi.modelindex(mdl/*ent->client->pers.weapon->view_model*/);
    //lm ctf

    //K03 Begin
    if (ent->client->pers.weapon == FindItem("Sword"))
        ent->client->ps.gunindex = 0;
    //K03 End

    // ### Hentai ### BEGIN
    ent->client->anim_priority = ANIM_PAIN;
    if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
        ent->s.frame = FRAME_crpain1;
        ent->client->anim_end = FRAME_crpain4;
    } else {
        ent->s.frame = FRAME_pain301;
        ent->client->anim_end = FRAME_pain304;
    }

    ShowGun(ent);

    // ### Hentai ### END
}

/*
=================
NoAmmoWeaponChange
=================
*/
void NoAmmoWeaponChange(edict_t *ent) {
    gitem_t *item = NULL;

    //GHz START
    ent->client->refire_frames = 0;
    lasersight_off(ent);
    //GHz END

    if (ent->client->pers.inventory[ITEM_INDEX(Fdi_SLUGS)]
        && ent->client->pers.inventory[ITEM_INDEX(Fdi_RAILGUN)]) {
        item = Fdi_RAILGUN;
    } else if (ent->client->pers.inventory[ITEM_INDEX(Fdi_CELLS)]
               && ent->client->pers.inventory[ITEM_INDEX(Fdi_HYPERBLASTER)]) {
        item = Fdi_HYPERBLASTER;
    } else if (ent->client->pers.inventory[ITEM_INDEX(Fdi_BULLETS)]
               && ent->client->pers.inventory[ITEM_INDEX(Fdi_CHAINGUN)]) {
        item = Fdi_CHAINGUN;
    } else if (ent->client->pers.inventory[ITEM_INDEX(Fdi_BULLETS)]
               && ent->client->pers.inventory[ITEM_INDEX(Fdi_MACHINEGUN)]) {
        item = Fdi_MACHINEGUN;
    } else if (ent->client->pers.inventory[ITEM_INDEX(Fdi_SHELLS)] > 1
               && ent->client->pers.inventory[ITEM_INDEX(Fdi_SUPERSHOTGUN)]) {
        item = Fdi_SUPERSHOTGUN;
    } else if (ent->client->pers.inventory[ITEM_INDEX(Fdi_SHELLS)]
               && ent->client->pers.inventory[ITEM_INDEX(Fdi_SHOTGUN)]) {
        item = Fdi_SHOTGUN;
    }

#ifdef VRX_REPRO
    else if (ent->client->pers.inventory[ITEM_INDEX(Fdi_MAGSLUG)]
             && ent->client->pers.inventory[ITEM_INDEX(Fdi_PHALANX)]) {
        item = Fdi_PHALANX;
    } else if (ent->client->pers.inventory[ITEM_INDEX(Fdi_FLECHETTES)]
               && ent->client->pers.inventory[ITEM_INDEX(Fdi_ETFRIFLE)]) {
        item = Fdi_ETFRIFLE;
    } else if (ent->client->pers.inventory[ITEM_INDEX(Fdi_CELLS)]
               && ent->client->pers.inventory[ITEM_INDEX(Fdi_PLASMA)]) {
        item = Fdi_PLASMA;
    } else if (ent->client->pers.inventory[ITEM_INDEX(Fdi_CELLS)]
               && ent->client->pers.inventory[ITEM_INDEX(Fdi_IONRIPPER)]) {
        item = Fdi_IONRIPPER;
    } else if (ent->client->pers.inventory[ITEM_INDEX(Fdi_ROUNDS)]
               && ent->client->pers.inventory[ITEM_INDEX(Fdi_DISRUPTOR)]) {
        item = Fdi_DISRUPTOR;
    }
#endif //VRX_REPRO

    if (item == NULL) item = Fdi_BLASTER;

    if (ent->svflags & SVF_MONSTER) item->use(ent, item);
    else ent->client->newweapon = item;
}

/*
=================
Think_Weapon

Called by ClientBeginServerFrame and ClientThink
=================
*/
void Think_Weapon(edict_t *ent) {
    // if just died, put the weapon away
    if (ent->health < 1) {
        ent->client->newweapon = NULL;
        ChangeWeapon(ent);
    }

    ent->client->vrr.gun_statemachine_time += FRAMETIME;

    // call active weapon think routine
    if (ent->client->pers.weapon && ent->client->pers.weapon->weaponthink) {
        is_quad = (ent->client->quad_framenum > level.framenum);
        // RAFAEL
        is_quadfire = (ent->client->quadfire_framenum > level.framenum);
        if (ent->client->silencer_shots)
            is_silenced = MZ_SILENCED;
        else
            is_silenced = 0;

        ent->client->pers.weapon->weaponthink(ent);
    }
}

/*
================
Use_Weapon

Make the weapon ready if there is ammo
================
*/
void Use_Weapon(edict_t *ent, gitem_t *item) {
    int ammo_index;
    gitem_t *ammo_item;

    // see if we're already using it
    if (item == ent->client->pers.weapon)
        return;

    if (ent->svflags & SVF_MONSTER) {
        if (ent->client->newweapon != NULL) return;
        if (!Q_stricmp(item->pickup_name, "Blaster")) {
            ent->client->newweapon = item;
            return;
        }
    }

    if (item->ammo && !g_select_empty->value && !(item->flags & IT_AMMO)) {
        ammo_item = FindItem(item->ammo);
        ammo_index = ITEM_INDEX(ammo_item);

        if (!ent->client->pers.inventory[ammo_index]) {
            if (!(ent->svflags & SVF_MONSTER))
                safe_cprintf(ent, PRINT_HIGH, "No %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
            return;
        }

        if (ent->client->pers.inventory[ammo_index] < item->quantity) {
            if (!(ent->svflags & SVF_MONSTER))
                safe_cprintf(ent, PRINT_HIGH, "Not enough %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
            return;
        }
    }

    // change to this weapon when down
    ent->client->newweapon = item;
}

void Use_Weapon2(edict_t *ent, gitem_t *item) {
    int ammo_index;
    gitem_t *ammo_item;

    if (ent->svflags & SVF_MONSTER) {
        Use_Weapon(ent, item);
        return;
    }

    // see if we're already using it
    if (item == ent->client->pers.weapon)
        return;

    if (item->ammo) {
        ammo_item = FindItem(item->ammo);
        ammo_index = ITEM_INDEX(ammo_item);
        if (!ent->client->pers.inventory[ammo_index] && !g_select_empty->value) {
            safe_cprintf(ent, PRINT_HIGH, "No %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
            return;
        }
    }

    // change to this weapon when down
    ent->client->newweapon = item;
}

/*
================
Drop_Weapon
================
*/
void Drop_Weapon(edict_t *ent, gitem_t *item) {
    int index;

    if ((int) (dmflags->value) & DF_WEAPONS_STAY)
        return;

    index = ITEM_INDEX(item);
    // see if we're already using it
    if (((item == ent->client->pers.weapon) || (item == ent->client->newweapon)) &&
        (ent->client->pers.inventory[index] == 1)) {
        if (!(ent->svflags & SVF_MONSTER)) safe_cprintf(ent, PRINT_HIGH, "Can't drop current weapon\n");
        return;
    }

    Drop_Item(ent, item);
    ent->client->pers.inventory[index]--;
}


/*
================
Weapon_Generic

A generic function to handle the basics of weapon thinking
================
*/
#define FRAME_FIRE_FIRST        (FRAME_ACTIVATE_LAST + 1)
#define FRAME_IDLE_FIRST        (FRAME_FIRE_LAST + 1)
#define FRAME_DEACTIVATE_FIRST    (FRAME_IDLE_LAST + 1)

#define T_EPSILON 0.001
#ifdef DEBUG_WEAPONS

void print_wp_state(edict_t *ent, char *state) {
    gi.dprintf("%.04f;%.04f;%1d;%2d;%s\n",
               level.time,
               ent->client->vrr.gun_statemachine_time,
               ent->client->vrr.gun_statemachine_time >= 0.1 - 0.0001,
               ent->client->ps.gunframe,
               state
    );
}
#endif

/**
 * Manages the state and behavior of a generic weapon, including activation, firing, idle, and deactivation states.
 *
 * This function handles weapon state transitions (activating, ready, firing, dropping), animating frames,
 * and calling the weapon's firing logic. It also enforces constraints like preventing the weapon's usage
 * under specific conditions (e.g., player morphed, shield active, no ammo, etc.).
 *
 * @param ent Pointer to the entity using the weapon (usually the player).
 * @param FRAME_ACTIVATE_LAST The last frame of the activation animation.
 * @param FRAME_FIRE_LAST The last frame of the firing animation sequence.
 * @param FRAME_IDLE_LAST The last frame of the idle animation sequence.
 * @param FRAME_DEACTIVATE_LAST The last frame of the deactivation animation.
 * @param pause_frames A pointer to an array of frame numbers where the weapon may pause in its idle state.
 * @param fire_frames A pointer to an array of frame numbers that represent firing animation frames.
 * @param fire A function pointer representing the weapon's firing logic. It is called during the firing state.
 */
void Weapon_Generic2(edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST,
                     int FRAME_DEACTIVATE_LAST, int *pause_frames, int *fire_frames, void (*fire)(edict_t *ent)) {
    int n;
    const qboolean can_run_frame = ent->client->vrr.gun_statemachine_time >= 0.1 - T_EPSILON;
    qboolean started_at_ready = false;

    //K03 Begin
    // can't use gun if we are just spawning, or we
    // are a morphed player using anything other
    // than a player model
    if (ent->deadflag || (ent->s.modelindex != 255) || (vrx_is_morphing_polt(ent))
        || (ctf->value && ctf_enable_balanced_fc->value && vrx_has_flag(ent)) // special rules, fc can't attack
        // || ((ent->client->respawn_time/* - (0.1 * FRAME_ACTIVATE_LAST)*/ ) > level.time)
        || (ent->flags & FL_WORMHOLE) //4.2 can't use weapons in wormhole
        || ent->shield) // can't use weapons while shield is deployed
        return;


    if (ent->client->ps.gunframe > FRAME_DEACTIVATE_LAST) {
        ent->client->weaponstate = WEAPON_READY;
        ent->client->ps.gunframe = FRAME_IDLE_FIRST;
    }
    //K03 End

    if (ent->client->weaponstate == WEAPON_DROPPING) {
        if (ent->client->ps.gunframe == FRAME_DEACTIVATE_LAST) {
            ChangeWeapon(ent);
            return;
        } // ### Hentai ### BEGIN
        else if ((FRAME_DEACTIVATE_LAST - ent->client->ps.gunframe) == 4) {
            ent->client->anim_priority = ANIM_REVERSE;
            if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
                ent->s.frame = FRAME_crpain4 + 1;
                ent->client->anim_end = FRAME_crpain1;
            } else {
                ent->s.frame = FRAME_pain304 + 1;
                ent->client->anim_end = FRAME_pain301;
            }
        }
        // ### Hentai ### END

        if (can_run_frame) {
            ent->client->ps.gunframe++;
            ent->client->vrr.gun_statemachine_time -= 0.1;
        }
        return;
    }

    if (ent->client->weaponstate == WEAPON_ACTIVATING) {
        // fast weapon switch
        //3.0 weapon masters get free fast weapon switch
        if ((ent->myskills.level >= 10 || ent->myskills.class_num == CLASS_WEAPONMASTER) &&
            (ent->client->newweapon != ent->client->pers.weapon))
            ent->client->ps.gunframe = FRAME_ACTIVATE_LAST;
        if (ent->client->ps.gunframe == FRAME_ACTIVATE_LAST) {
            ent->client->weaponstate = WEAPON_READY;
            ent->client->ps.gunframe = FRAME_IDLE_FIRST;
            return;
        }

        if (can_run_frame) {
            ent->client->ps.gunframe++;
            ent->client->vrr.gun_statemachine_time -= 0.1;
        }

        return;
    }

    if ((ent->client->newweapon) && (ent->client->weaponstate != WEAPON_FIRING)) {
        // fast weapon switch
        //3.0 weapon masters get free fast weapon switch
        ent->client->weaponstate = WEAPON_DROPPING;
        if ((ent->myskills.level >= 10 || ent->myskills.class_num == CLASS_WEAPONMASTER) &&
            (ent->client->newweapon != ent->client->pers.weapon)) {
            ChangeWeapon(ent);
            return;
        } else
            ent->client->ps.gunframe = FRAME_DEACTIVATE_FIRST;

        // ### Hentai ### BEGIN
        if ((FRAME_DEACTIVATE_LAST - FRAME_DEACTIVATE_FIRST) < 4) {
            ent->client->anim_priority = ANIM_REVERSE;
            if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
                ent->s.frame = FRAME_crpain4 + 1;
                ent->client->anim_end = FRAME_crpain1;
            } else {
                ent->s.frame = FRAME_pain304 + 1;
                ent->client->anim_end = FRAME_pain301;
            }
        }
        // ### Hentai ### END

        return;
    }

    if (ent->client->weaponstate == WEAPON_READY) {
        if (((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK)) {
            print_wp_state(ent, "READY");
            ent->client->latched_buttons &= ~BUTTON_ATTACK;
            if ((!ent->client->ammo_index) ||
                (ent->client->pers.inventory[ent->client->ammo_index] >= ent->client->pers.weapon->quantity)) {
                //GHz START
                ent->client->idle_frames = 0;
                //4.5 the first shot merely resets the chatprotect state, but doesn't actually fire
                if (ent->flags & FL_CHATPROTECT) {
                    // reset chat protect and cloaking flags
                    vrx_remove_chat_protect(ent);

                    // re-draw weapon
                    ent->client->newweapon = ent->client->pers.weapon;
                    ent->client->weaponstate = WEAPON_DROPPING;
                    ChangeWeapon(ent);
                    return;
                }
                //GHz END
                started_at_ready = true;
                ent->client->ps.gunframe = FRAME_FIRE_FIRST;
                ent->client->weaponstate = WEAPON_FIRING;

                print_wp_state(ent, "READY DONE");

                // start the animation
                ent->client->anim_priority = ANIM_ATTACK;
                if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
                    ent->s.frame = FRAME_crattak1 - 1;
                    ent->client->anim_end = FRAME_crattak9;
                } else {
                    ent->s.frame = FRAME_attack1 - 1;
                    ent->client->anim_end = FRAME_attack8;
                }
            } else {
                if (level.time >= ent->pain_debounce_time) {
                    gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
                    ent->pain_debounce_time = level.time + 1;
                }
                NoAmmoWeaponChange(ent);
            }
        } else {
            if (ent->client->ps.gunframe == FRAME_IDLE_LAST) {
                ent->client->ps.gunframe = FRAME_IDLE_FIRST;
                return;
            }
            // don't run idle frames at 60 fps
            if (can_run_frame) {
                if (pause_frames) {
                    for (n = 0; pause_frames[n]; n++) {
                        if (ent->client->ps.gunframe == pause_frames[n]) {
                            if (randomMT() & 15)
                                return;
                        }
                    }
                }


                ent->client->ps.gunframe++;
                ent->client->vrr.gun_statemachine_time -= 0.1;
            }
            return;
        }
    }

    if (ent->client->weaponstate == WEAPON_FIRING) {
        for (n = 0; fire_frames[n]; n++) {
            if (ent->client->ps.gunframe == fire_frames[n]) {
                if (level.time > ent->client->ctf_techsndtime) {
                    qboolean quad = false;

                    if (ent->client->quad_framenum > level.framenum)
                        quad = true;

                    if (ent->client->pers.inventory[strength_index]) {
                        if (quad)
                            gi.sound(ent, CHAN_ITEM, gi.soundindex("ctf/tech2x.wav"), 1, ATTN_NORM, 0);
                        else
                            gi.sound(ent, CHAN_ITEM, gi.soundindex("ctf/tech2.wav"), 1, ATTN_NORM, 0);
                    } else if (ent->client->pers.inventory[haste_index])
                        gi.sound(ent, CHAN_ITEM, gi.soundindex("ctf/tech3.wav"), 1, ATTN_NORM, 0);
                    else if (quad)
                        gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);

                    ent->client->ctf_techsndtime = level.time + 0.9;
                }

                if (can_run_frame) {
                    //K03 Begin
                    ent->client->resp.pstats.shots++;
                    if (ent->movetype != MOVETYPE_NOCLIP || (ent->myskills.abilities[CLOAK].current_level == 10 &&
                                                             vrx_get_talent_level(ent, TALENT_IMP_CLOAK) ==
                                                             4))
                        // don't uncloak if they are in noclip, or permacloaked due to upgrade levels
                        ent->svflags &= ~SVF_NOCLIENT;
                    ent->client->cloaking = false;
                    //K03 End

                    print_wp_state(ent, "FIRE");
                    fire(ent);
                    ent->client->vrr.gun_statemachine_time = 0.0f;
                    //gi.dprintf("fired at %d\n",level.framenum);
                }

                break;
            }
        }

        if (!fire_frames[n]) {
            if (can_run_frame) {
                print_wp_state(ent, "CD");
                ent->client->ps.gunframe++;
                ent->client->vrr.gun_statemachine_time -= 0.1;
            }
        }

        if (ent->client->ps.gunframe == FRAME_IDLE_FIRST + 1) {
            print_wp_state(ent, "IDLE");
            ent->client->weaponstate = WEAPON_READY;
        }
    }
}

float calculate_haste_wait(edict_t *ent) {
    // time when to fire next shot
    float haste_wait = -1;
    float haste_tech = INFINITY; // "never"
    float haste_skill = INFINITY; // "never"

    if (ent->myskills.abilities[HASTE].current_level >= 1)
        haste_skill = 1.0f / ent->myskills.abilities[HASTE].current_level;

    // haste tech
    if (ent->client->pers.inventory[haste_index]) {
        haste_tech = 0.1; // 100% improvement (2x firing rate)
    }

    haste_wait = min(haste_tech, haste_skill);
    return haste_wait;
}

#define HASTE_RUN_FRAME(haste_wait, call, ent)

int32_t is_haste_active(edict_t *ent) {
    return (!ent->myskills.abilities[HASTE].disable && ent->myskills.abilities[HASTE].current_level >= 1)
           || ent->client->pers.inventory[haste_index];
}

//K03 Begin
void Weapon_Generic(
    edict_t *ent,
    int FRAME_ACTIVATE_LAST,
    int FRAME_FIRE_LAST,
    int FRAME_IDLE_LAST,
    int FRAME_DEACTIVATE_LAST,
    int *pause_frames,
    int *fire_frames,
    void (*fire)(edict_t *ent)) {
    int freezeLevel = 0;
    que_t *curse;

    // pick the highest level freeze
    if ((curse = que_findtype(ent->curses, NULL, AURA_HOLYFREEZE)) != NULL)
        freezeLevel = h2e(curse->ent)->owner->myskills.abilities[HOLY_FREEZE].current_level;
    if (ent->chill_time > level.time && ent->chill_level > freezeLevel)
        freezeLevel = ent->chill_level;

    //3.0 New hfa code (more balanced?)
    if (freezeLevel) {
        int Continue;

        //Figure out what frame should be skipped (every x frames)
        switch (freezeLevel) {
            case 0:
            case 1:
            case 2:
            case 3:
                Continue = qf2sf(7);
                break;
            case 4:
            case 5:
                Continue = qf2sf(6);
                break;
            case 6:
            case 7:
                Continue = qf2sf(5);
                break;
            case 8:
            case 9:
            default:
                Continue = qf2sf(4);
                break; // 25% reduced firing rate
        }

        //Examples:
        //If Continue == 2, every second frame is skipped (50% slower firing rate)
        //If Continue == 6, every sixth frame is skipped (17% slower firing rate)

        ent->client->FrameShot++;
        if (ent->client->FrameShot >= Continue) {
            ent->client->FrameShot = 0;
            return;
        }
    }

    Weapon_Generic2(ent, FRAME_ACTIVATE_LAST, FRAME_FIRE_LAST,
                    FRAME_IDLE_LAST, FRAME_DEACTIVATE_LAST, pause_frames, fire_frames, fire);

    // ent->FrameShot = 0;

    if (is_haste_active(ent)) {
        float haste_wait = calculate_haste_wait(ent);
        if (haste_wait <= 0) // safeguard lol
            return;

        // if enough frames have passed by, then call the weapon func
        // an additional time
        {
            while (ent->client->haste_time >= haste_wait) {
                ent->client->vrr.gun_statemachine_time += 0.1;
                Weapon_Generic2(ent, FRAME_ACTIVATE_LAST, FRAME_FIRE_LAST,
                                FRAME_IDLE_LAST, FRAME_DEACTIVATE_LAST, pause_frames, fire_frames, fire);
                ent->client->haste_time -= haste_wait;
            }
            ent->client->haste_time += FRAMETIME;
        }
    }

    //gi.dprintf("gunframe=%d\n", ent->client->ps.gunframe);
}

//K03 End
