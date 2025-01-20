#include "g_local.h"
#include "../../characters/class_limits.h"

void vrx_death_cleanup(edict_t *attacker, edict_t *targ);

void vrx_process_exp(edict_t *attacker, edict_t *targ);


void V_GibSound(edict_t *self, int index) {
    switch (index) {
        case 1:
            gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
            break;
        case 2:
            gi.sound(self, CHAN_BODY, gi.soundindex("death/gib2.wav"), 1, ATTN_NORM, 0);
            break;
        case 3:
            gi.sound(self, CHAN_BODY, gi.soundindex("death/gib3.wav"), 1, ATTN_NORM, 0);
            break;
        case 4:
            gi.sound(self, CHAN_BODY, gi.soundindex("death/malive5.wav"), 1, ATTN_NORM, 0);
            break;
        case 5:
            gi.sound(self, CHAN_BODY, gi.soundindex("death/mdeath4.wav"), 1, ATTN_NORM, 0);
            break;
        case 6:
            gi.sound(self, CHAN_BODY, gi.soundindex("death/mdeath5.wav"), 1, ATTN_NORM, 0);
            break;
        default:
            gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
            break;
    }
}

void vrx_player_death(edict_t *self, edict_t *attacker, edict_t *inflictor) {
    if (debuginfo->value > 1)
        gi.dprintf("VortexPlayerDeath()\n");

    // only call this function once
    if (self->deadflag)
        return;

    self->gib_health = -BASE_GIB_HEALTH;

    // don't drop powercubes or tballs
    self->myskills.inventory[ITEM_INDEX(Fdi_POWERCUBE)] = self->client->pers.inventory[ITEM_INDEX(Fdi_POWERCUBE)];
    self->myskills.inventory[ITEM_INDEX(Fdi_TBALL)] = self->client->pers.inventory[ITEM_INDEX(Fdi_TBALL)];

    vrx_process_exp(attacker, self); // modify experience
    vrx_reset_player_state(self);
    vrx_death_cleanup(attacker, self);

    vrx_toss_backpack(self, attacker); // toss a backpack
    vrx_roll_rune_drop(self, attacker, false); // possibly generate a rune
    GetScorePosition();// gets each persons rank and total players in game

    vrx_clean_damage_list(self, true);
}

void vrx_add_basic_weapons(gclient_t *client, gitem_t *item, int spectator) {
    client->pers.inventory[ITEM_INDEX(FindItem("Sword"))] = 1;
    client->pers.inventory[ITEM_INDEX(FindItem("Power Screen"))] = 1;
    client->pers.inventory[flag_index] = 0;
    client->pers.inventory[red_flag_index] = 0;
    client->pers.inventory[blue_flag_index] = 0;

    client->pers.spectator = spectator;
    client->pers.weapon = item;
}

void vrx_add_respawn_items(edict_t *ent) {
//	gi.dprintf("vrx_add_respawn_items()\n");

    ent->client->pers.inventory[ITEM_INDEX(Fdi_POWERCUBE)] = ent->myskills.inventory[ITEM_INDEX(Fdi_POWERCUBE)];
    ent->client->pers.inventory[ITEM_INDEX(Fdi_TBALL)] = ent->myskills.inventory[ITEM_INDEX(Fdi_TBALL)];
    ent->client->pers.inventory[ITEM_INDEX(Fdi_TBALL)] = ent->myskills.inventory[ITEM_INDEX(
            Fdi_TBALL)] += TBALLS_RESPAWN;

    // poltergeist always spawns with at least 50 power cubes (for morphing) and cells (for power screen)
    if (ent->myskills.class_num == CLASS_POLTERGEIST)
    {
        if (ent->client->pers.inventory[power_cube_index] < 50)
            ent->client->pers.inventory[power_cube_index] = 50;
        if (ent->client->pers.inventory[cell_index] < 50)
            ent->client->pers.inventory[cell_index] = 50;
    }
    else
        ent->client->pers.inventory[ITEM_INDEX(Fdi_POWERCUBE)] = ent->myskills.inventory[ITEM_INDEX(
                Fdi_POWERCUBE)] += POWERCUBES_RESPAWN;

    if (ent->client->pers.inventory[ITEM_INDEX(Fdi_TBALL)] > 20)
        ent->client->pers.inventory[ITEM_INDEX(Fdi_TBALL)] = 20;
    if (ent->client->pers.inventory[ITEM_INDEX(Fdi_POWERCUBE)] > ent->client->pers.max_powercubes)
        ent->client->pers.inventory[ITEM_INDEX(Fdi_POWERCUBE)] = ent->client->pers.max_powercubes;
    ent->client->pers.inventory[ITEM_INDEX(Fdi_BLASTER)] = 1;
    ent->client->pers.inventory[ITEM_INDEX(FindItem("Sword"))] = 1;

    //if (!ent->myskills.abilities[START_ARMOR].disable)
    //	ent->client->pers.inventory[ITEM_INDEX(FindItem("Body Armor"))] = ((MAX_ARMOR(ent))*0.2)*ent->myskills.abilities[START_ARMOR].current_level;
    if ((ent->myskills.level >= 10) ||
        (ent->myskills.class_num == CLASS_KNIGHT)) // give starting armor to level 10+ and knight
        ent->client->pers.inventory[ITEM_INDEX(FindItem("Body Armor"))] += 50;
    if (ent->myskills.boss > 0)
        ent->client->pers.inventory[ITEM_INDEX(FindItem("Body Armor"))] = MAX_ARMOR(ent);
}

void vrx_give_additional_respawn_weapons(edict_t *ent, int nextWeapon) {
    int picks[MAX_WEAPONS] = {0};
    int nextEmptyslot = 0;

    picks[nextEmptyslot++] = ent->myskills.respawn_weapon;

    //Don't crash
    if (nextWeapon > MAX_WEAPONS) return;
    /*
        This next bit of code is hard to read. I'll explain it here.

        Search through the player's upgradeable weapons for the
        one with the most upgrades. Any weapons already in the list
        are skipped. The resultant weapon is added to the list.
        This process is repeated until the list is large enough.

        ie: Sort the weapons in upgraded order until we find weapon
        number X on the list. Then give them respawn weapon X.
    */

    do {
        int i, j;
        int max = 0;
        int maxvalue = V_WeaponUpgradeVal(ent, 0);

        //Skip any weapons already added to the list.
        for (i = 1; i < MAX_WEAPONS; ++i) {
            qboolean skip;
            int value;

            skip = false;
            for (j = 0; j < nextEmptyslot; ++j) {
                if (i == picks[j]) {
                    skip = true;
                    break;
                }
            }
            if (skip) continue;

            //Get the upgrade value and compare it to the current leader.
            value = V_WeaponUpgradeVal(ent, i);
            if (value > maxvalue) {
                maxvalue = value;
                max = i;
            }
        }

        //A maximum has been found. Add it to the list.
        picks[nextEmptyslot++] = max;
    } while (nextEmptyslot < nextWeapon + 1);

    //Give the last item on the list as a respawn weapon.
    vrx_add_respawn_weapon(ent, picks[nextEmptyslot - 1] + 1);
}

// converts the index value stored in respawn_weapon to the inventory weapon index
int vrx_WeapIDtoWeapIndex(int weaponID)
{
    switch (weaponID)
    {
    case 1: return sword_index;
    case 2: return shotgun_index;
    case 3: return supershotgun_index;
    case 4: return machinegun_index;
    case 5: return chaingun_index;
    case 6: return grenadelauncher_index;
    case 7: return rocketlauncher_index;
    case 8: return hyperblaster_index;
    case 9: return railgun_index;
    case 10: return bfg10k_index;
    case 11: return grenade_index;
    case 12: return _20mmcannon_index;
    case 13: return blaster_index;
    default: return blaster_index;
    }
}

void vrx_add_respawn_weapon(edict_t *ent, int weaponID) {
    //vrx_add_respawn_items(ent);

    if (ent->myskills.class_num == CLASS_KNIGHT) {
        ent->myskills.respawn_weapon = 1;
        vrx_pick_respawn_weapon(ent);
    }

#ifndef REMOVE_RESPAWNS
    if (!ent->myskills.weapon_respawns) // No respawns.
        return;
#endif

#ifndef REMOVE_RESPAWNS
    if (pregame_time->value < level.time) // not in pregame?
        ent->myskills.weapon_respawns--;
#endif

    if (vrx_is_morphing_polt(ent))
        return;


    //3.02 begin new respawn weapon code
    //Give them the weapon
    /*
    switch (weaponID) {
        case 2:
            ent->client->pers.inventory[ITEM_INDEX(Fdi_SHOTGUN)] = 1;
            break;
        case 3:
            ent->client->pers.inventory[ITEM_INDEX(Fdi_SUPERSHOTGUN)] = 1;
            break;
        case 4:
            ent->client->pers.inventory[ITEM_INDEX(Fdi_MACHINEGUN)] = 1;
            break;
        case 5:
            ent->client->pers.inventory[ITEM_INDEX(Fdi_CHAINGUN)] = 1;
            break;
        case 6:
            ent->client->pers.inventory[ITEM_INDEX(Fdi_GRENADELAUNCHER)] = 1;
            break;
        case 7:
            ent->client->pers.inventory[ITEM_INDEX(Fdi_ROCKETLAUNCHER)] = 1;
            break;
        case 8:
            ent->client->pers.inventory[ITEM_INDEX(Fdi_HYPERBLASTER)] = 1;
            break;
        case 9:
            ent->client->pers.inventory[ITEM_INDEX(Fdi_RAILGUN)] = 1;
            break;
        case 10:
            ent->client->pers.inventory[ITEM_INDEX(Fdi_BFG)] = 1;
            break;
        case 11:
            ent->client->pers.inventory[ITEM_INDEX(Fdi_GRENADES)] = 1;
            break;
        case 12:
            ent->client->pers.inventory[ITEM_INDEX(FindItem("20mm Cannon"))] = 1;
            break;
        case 13:
            ent->client->pers.inventory[ITEM_INDEX(Fdi_BLASTER)] = 1;
            break;
        default:
            ent->client->pers.inventory[ITEM_INDEX(Fdi_BLASTER)] = 1;
            break;
    }
    */
    ent->client->pers.inventory[vrx_WeapIDtoWeapIndex(weaponID)] = 1;

    //Give them the ammo
    V_GiveAmmoClip(ent, 2, V_GetRespawnAmmoType(ent));
    //3.02 end new respawn weapon code

    vrx_pick_respawn_weapon(ent);

    //3.0 reset auto-tball flag
    ent->v_flags = SFLG_NONE;
}

void vrx_pick_respawn_weapon(edict_t *ent) {
    int index;
    gitem_t *item;
    /*

    switch (ent->myskills.respawn_weapon) {
        case 1:
            item = FindItem("Sword");
            break;
        case 2:
            item = Fdi_SHOTGUN;
            break;
        case 3:
            item = Fdi_SUPERSHOTGUN;
            break;
        case 4:
            item = Fdi_MACHINEGUN;
            break;
        case 5:
            item = Fdi_CHAINGUN;
            break;
        case 6:
            item = Fdi_GRENADELAUNCHER;
            break;
        case 7:
            item = Fdi_ROCKETLAUNCHER;
            break;
        case 8:
            item = Fdi_HYPERBLASTER;
            break;
        case 9:
            item = Fdi_RAILGUN;
            break;
        case 10:
            item = Fdi_BFG;
            break;
        case 11:
            item = Fdi_GRENADES;
            break;
        case 12:
            item = FindItem("20mm Cannon");
            break;
        case 13:
            item = Fdi_BLASTER;
            break;
        default:
            item = Fdi_BLASTER;
            break;
    }
    */
    index = vrx_WeapIDtoWeapIndex(ent->myskills.respawn_weapon);
    item = &itemlist[index];

    ent->client->pers.selected_item = index;//ITEM_INDEX(item);
    ent->client->pers.weapon = item;
    ent->client->pers.lastweapon = item;
    ent->client->newweapon = item;

    ChangeWeapon(ent);
}

void Use_PowerArmor(edict_t* ent, gitem_t* item);

void vrx_morph_think(edict_t* ent)
{
    // only valid player-morphs should be calling this function
    if (!ent || !ent->inuse || !ent->client || !ent->mtype)
        return;
    // activate power screen for berserk and brain automatically when player has cells for it
    if ((ent->mtype == MORPH_BERSERK || ent->mtype == MORPH_BRAIN) && ent->client->pers.inventory[cell_index] && !(ent->flags & FL_POWER_ARMOR))
        Use_PowerArmor(ent, NULL);
}

void BlinkStrike_think(edict_t* ent)
{
    trace_t tr;
    vec3_t  start;

    if (!ent || !ent->inuse || !ent->client)
        return;
    // try to teleport the player back if the Blink Strike target is still alive
    if (level.framenum == ent->client->tele_timeout && G_EntIsAlive(ent->client->blinkStrike_targ))
    {
        // get previous position
        VectorCopy(ent->client->oldpos, start);
        // clear Blink Strike values
        ent->client->blinkStrike_targ = NULL;
        VectorClear(ent->client->oldpos);
        // check position
        tr = gi.trace(start, ent->mins, ent->maxs, start, ent, MASK_SHOT);
        if (tr.allsolid || tr.startsolid || tr.fraction < 1)
            return;
        // move player into position
        VectorCopy(start, ent->s.origin);
        VectorCopy(start, ent->s.old_origin);
        ent->s.event = EV_PLAYER_TELEPORT;
        gi.linkentity(ent);
    }
}