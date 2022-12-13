#include "g_local.h"
#include "../../characters/class_limits.h"


void think_ability_ammo_regen(edict_t* ent) {
	float amount_mult = 1;
	if (ent->myskills.abilities[AMMO_REGEN].disable)
		return;

	if (ent->client->buttons & BUTTON_ATTACK) {
		amount_mult = 0.2f; // newvrx
	}

	if (level.time > ent->client->ammo_regentime) {
		float regen_level = (float)ent->myskills.abilities[AMMO_REGEN].current_level;

		V_GiveAmmoClip(ent,
			regen_level * 0.2f * amount_mult,
			AMMO_SHELLS);        //20% of a pack per point
		V_GiveAmmoClip(ent,
			regen_level * 0.1f * amount_mult,
			AMMO_BULLETS);        //10% of a pack per point
		V_GiveAmmoClip(ent,
			regen_level * 0.1f * amount_mult,
			AMMO_CELLS);        //10% of a pack per point
		V_GiveAmmoClip(ent,
			regen_level * 0.2f * amount_mult,
			AMMO_GRENADES);    //20% of a pack per point
		V_GiveAmmoClip(ent,
			regen_level * 0.2f * amount_mult,
			AMMO_ROCKETS);        //20% of a pack per point
		V_GiveAmmoClip(ent,
			regen_level * 0.2f * amount_mult,
			AMMO_SLUGS);        //20% of a pack per point

		ent->client->ammo_regentime = level.time + AMMO_REGEN_DELAY;
	}
}

void think_ability_power_regen(edict_t* ent) {
	gitem_t* item;
	int index, temp;

	if (ent->myskills.abilities[POWER_REGEN].disable)
		return;

	item = Fdi_POWERCUBE;
	if (item) {
		index = ITEM_INDEX(item);
		if (ent->client->pers.inventory[index] < ent->client->pers.max_powercubes) {
			temp = 5/**ent->myskills.abilities[POWER_REGEN].current_level*/;

			ent->client->pers.inventory[index] += temp;

			if (ent->client->pers.inventory[index] > ent->client->pers.max_powercubes)
				ent->client->pers.inventory[index] = ent->client->pers.max_powercubes;

			if (ent->client->pers.inventory[index] < 0)
				ent->client->pers.inventory[index] = 0;
		}
	}

}

void think_trade(edict_t* ent) {//3.0 new trading
	//If this player isn't showing a menu any more, cancel the trade
	if (ent->trade_with && !ent->client->menustorage.menu_active) {
		int i;

		//alert both players
		safe_cprintf(ent, PRINT_HIGH, "%s has stopped the trade.\n", ent->myskills.player_name);
		safe_cprintf(ent->trade_with, PRINT_HIGH, "%s has stopped the trade.\n", ent->myskills.player_name);

		//Clear the trade pointers
		for (i = 0; i < 3; ++i) {
			ent->trade_item[i] = NULL;
			ent->trade_with->trade_item[i] = NULL;
		}

		//cancel the trade (trade_with)
		closemenu(ent->trade_with);
		ent->trade_with->client->trade_accepted = false;
		ent->trade_with->client->trade_final = false;
		ent->trade_with->client->trading = false;
		ent->trade_with->trade_with = NULL;

		//cancel the trade (ent)
		ent->trade_with = NULL;
		ent->client->trade_accepted = false;
		ent->client->trade_final = false;
		ent->client->trading = false;
	}
	//3.0 end

}

void think_chat_protect_activate(edict_t* ent) {
	if (!ptr->value && !domination->value && !ctf->value &&
		!(hw->value && vrx_has_flag(ent))  // the game isn't holywars and the player doesn't have the flag
		&& !ent->myskills.administrator // Not an admin
		&& !que_typeexists(ent->curses, 0)  // Not cursed
		&& (ent->myskills.streak < SPREE_START)) // Not on a spree
	{
		if (!((!ent->myskills.abilities[CLOAK].disable) && ((ent->myskills.abilities[CLOAK].current_level > 0)))) {
			if (!trading->value && !ent->automag) // trading mode no chat protection or if automagging either
			{
				if (sf2qf(ent->client->idle_frames) == CHAT_PROTECT_FRAMES - 100)
					safe_centerprintf(ent, "10 seconds to chat-protect.\n");
				else if (sf2qf(ent->client->idle_frames) == CHAT_PROTECT_FRAMES - 50)
					safe_centerprintf(ent, "5 seconds to chat-protect.\n");

				if (sf2qf(ent->client->idle_frames) == qf2sf(CHAT_PROTECT_FRAMES)) {
					safe_centerprintf(ent, "Now in chat-protect mode.\n");
					ent->flags |= FL_CHATPROTECT;
					if (!pvm->value) {
						ent->solid = SOLID_NOT;
						ent->svflags |= SVF_NOCLIENT;//4.5
					}
					vrx_remove_player_summonables(ent);
					//3.0 Remove all active auras when entering chat protect
					AuraRemove(ent, 0);
				}
			}
		}
	}
}

void think_player_inactivity(edict_t* ent) {
	if (level.time > pregame_time->value && vrx_get_joined_players() > maxclients->value * 0.8) {
		int frames = MAX_IDLE_FRAMES;

		if (!ent->myskills.administrator && !trading->value) {
			if (ent->client->still_frames == frames - 300)
				safe_centerprintf(ent, "You have 30 seconds to stop\nidling or you will be kicked.\n");
			else if (ent->client->still_frames == frames - 100)
				safe_centerprintf(ent, "You have 10 seconds to stop\nidling or you will be kicked.\n");

			if (ent->client->still_frames > frames) {
				ent->client->still_frames = 0;
				gi.bprintf(PRINT_HIGH, "%s was kicked for inactivity.\n", ent->client->pers.netname);
				stuffcmd(ent, "disconnect\n");
			}
		}
	}
}

void think_idle_frame_counter(const edict_t* ent) {
	if ((ent->velocity[0] != 0) || (ent->velocity[1] != 0) || (ent->velocity[2] != 0)) {
		ent->client->still_frames = 0;

		//Talent: Improved cloak
		//Check to see if the player is cloaked AND crouching
		if (ent->client->cloaking && ent->client->ps.pmove.pm_flags & PMF_DUCKED) {


			//Make sure they have the talent
			int talentLevel = vrx_get_talent_level(ent, TALENT_IMP_CLOAK);

			if (talentLevel > 0) {
				int cloak_cubecost = 6 - talentLevel;
				if (ent->myskills.abilities[CLOAK].current_level == 10 &&
					vrx_get_talent_level(ent, TALENT_IMP_CLOAK) == 4) {
					cloak_cubecost = 0;
				}

				//During night time, improved cloak takes 1/3 the cubes (rounded up).
				if (!level.daytime) cloak_cubecost = ceil((double)cloak_cubecost / 3.0);

				//Take the cubes away
				ent->client->pers.inventory[power_cube_index] -= cloak_cubecost;

				//If the player runs out of cubes, they will uncloak!
				if (ent->client->pers.inventory[power_cube_index] <= 0) {
					ent->client->idle_frames = 0;
					ent->client->cloaking = false;
					ent->client->pers.inventory[power_cube_index] = 0;
				}
			}
			else {
				ent->client->cloaking = false;
				ent->client->idle_frames = 0;
			}
		}
		else {
			ent->client->cloaking = false;
			ent->client->idle_frames = 0;
		}
	}
	else if (ent->client->snipertime > 0 && ent->client->idle_frames > qf2sf(15)) {
		ent->client->idle_frames = qf2sf(15);
	}
	else if ((ent->health > 0) && !G_IsSpectator(ent) && (ent->movetype != MOVETYPE_NONE)) {
		ent->client->idle_frames++;
		ent->client->still_frames++;
	}
}

// SuperSpeed
// p_player.c
trace_t PM_trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);

qboolean CanSuperSpeed(edict_t* ent) {
	// can't superspeed while being hurt
	//if (ent->lasthurt + DAMAGE_ESCAPE_DELAY > level.time)
	//	return false;

	// bless
	if (que_findtype(ent->curses, NULL, BLESS))
		return true;

	// superspeed ability
	if (!ent->myskills.abilities[SUPER_SPEED].disable && ent->myskills.abilities[SUPER_SPEED].current_level > 0
		&& ent->myskills.abilities[SUPER_SPEED].charge >= SUPERSPEED_DRAIN_COST)
		return true;

	// sprint
	if (ent->mtype == MORPH_BERSERK && ent->myskills.abilities[BERSERK].charge >= SPRINT_COST)
		return true;

	return false;
}

/*
pmove_t
V_Think_ApplySuperSpeed(edict_t *ent, const usercmd_t *ucmd, gclient_t *client, int i, pmove_t *pm, int viewheight) {
	if (ent->superspeed) {

		if (!CanSuperSpeed(ent)) {
			ent->superspeed = false;
		} else if (level.time > ent->lasthurt + DAMAGE_ESCAPE_DELAY) {
			// ent->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
			(*pm).s = client->ps.pmove;

			{
				(*pm).s.origin[i] = ent->s.origin[i] * 6;
				//pm.s.velocity[i] = ent->velocity[i]*8;
			}
			(*pm).s.velocity[0] = ent->velocity[0] * 6;
			(*pm).s.velocity[1] = ent->velocity[1] * 6;

			if (memcmp(&client->old_pmove, &(*pm).s, sizeof((*pm).s)) != 0) {
				(*pm).snapinitial = true;
				//		gi.dprintf ("pmove changed!\n");
			}

			(*pm).cmd = *ucmd;

			(*pm).trace = PM_trace;    // adds default parms

			(*pm).pointcontents = gi.pointcontents;

			// perform a pmove
			gi.Pmove(pm);

// GHz START
			// if this is a morphed player, restore saved viewheight
			// this locks them into that viewheight
			if (ent->mtype)
				(*pm).viewheight = viewheight;
//GHz END
			// save results of pmove
			client->ps.pmove = (*pm).s;
			client->old_pmove = (*pm).s;

			for (i = 0; i < 3; i++) {
				ent->s.origin[i] = (*pm).s.origin[i] * 0.125;
				//ent->velocity[i] = pm.s.velocity[i]*0.125;
			}
			ent->velocity[0] = (*pm).s.velocity[0] * 0.125;
			ent->velocity[1] = (*pm).s.velocity[1] * 0.125;

			VectorCopy ((*pm).mins, ent->mins);
			VectorCopy ((*pm).maxs, ent->maxs);

			client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
			client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
			client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);

			// ent->client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
		}
	}
	//K03 End

	return (*pm);
}*/

float V_ModifyMovement(edict_t* ent, usercmd_t* ucmd, que_t* curse) {// assault cannon slows you down
	float vel_modification = 1;

	if (ent->client->pers.weapon && (ent->client->pers.weapon->weaponthink == Weapon_Chaingun)
		&& (ent->client->weaponstate == WEAPON_FIRING) && (ent->client->weapon_mode)) {
		vel_modification *= 0.33;
	}

	// sniper mode slows you down
//    if (ent->client->snipertime >= level.time) {
//        vel_modification *= 0.33;
//    }

	curse = que_findtype(ent->curses, curse, AURA_HOLYFREEZE);
	// are we affected by the holy freeze aura?
	if (curse) {
		float modifier = 1 / (1 + 0.1 * curse->ent->owner->myskills.abilities[HOLY_FREEZE].current_level);
		if (modifier < 0.25) modifier = 0.25;

		//gi.dprintf("holyfreeze modifier = %.2f\n", modifier);
		vel_modification *= modifier;
	}

	//Talent: Frost Nova
//4.2 Water Totem
	if (ent->chill_time > level.time) {
		float modifier = 1 / (1 + CHILL_DEFAULT_BASE + CHILL_DEFAULT_ADDON * ent->chill_level);
		if (modifier < 0.25) modifier = 0.25;

		vel_modification *= modifier;
	}

	//4.2 caltrops
	if (ent->slowed_time > level.time) {
		vel_modification *= ent->slowed_factor;
	}


	// 3.5 weaken slows down target
	if ((curse = que_findtype(ent->curses, NULL, WEAKEN)) != NULL) {
		float modifier = 1 / (1 + WEAKEN_SLOW_BASE + WEAKEN_SLOW_BONUS
			* curse->ent->owner->myskills.abilities[WEAKEN].current_level);
		vel_modification *= modifier;
	}

	//GHz: Keep us still and don't allow shooting
// If we have an automag up, don't let us move either. -az
// az: nope nevermind it's bullshit
	if ((ent->holdtime && ent->holdtime > level.time)) {
		vel_modification *= 0;

		if (ent->client->buttons & BUTTON_ATTACK)
			ent->client->buttons &= ~BUTTON_ATTACK;

		if (ucmd->buttons & BUTTON_ATTACK)
			ucmd->buttons &= ~BUTTON_ATTACK;
	}

	// az 2020: it seems like _we_ added this? _why_ did we add this?
	// if (PM_PlayerHasMonster(ent)) {
	//     if (ent->owner->mtype == P_TANK)
	//         ucmd->upmove = 0;
	// }

	if (ent->automag) {
		edict_t* other;
		for (other = g_edicts; other != &g_edicts[256]; other++) {
			int pull;
			vec3_t start, end, dir;

			if (other->absmin[2] + 1 < ent->absmin[2])
				continue;

			if (!G_ValidTarget(ent, other, true))
				continue;

			if (entdist(ent, other) > MAGMINE_RANGE)
				continue;

			pull = MAGMINE_DEFAULT_PULL / 2 + MAGMINE_ADDON_PULL * ent->myskills.abilities[MAGMINE].level;

			G_EntMidPoint(other, end);
			G_EntMidPoint(ent, start);
			VectorSubtract(end, start, dir);
			VectorNormalize(dir);

			// Pull it.
			T_Damage(other, ent, ent, dir, end, vec3_origin, 0, pull, 0, 0);
		}
	}

	/*
		az: rewrite how superspeed works because it fucking sucks
	*/
	qboolean superspeed = ent->superspeed && CanSuperSpeed(ent) && level.time > ent->lasthurt + DAMAGE_ESCAPE_DELAY;
	if (superspeed) {
		vel_modification *= 1.75;
	}


	//K03 Begin
	qboolean hook = (ent->client->hook_state == HOOK_ON) && (VectorLength(ent->velocity) < 10);

	if (hook/* || vel_modification != 1*/) {
		ent->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
	}
	else {
		ent->client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
	}
	//K03 End

	return vel_modification;
}

void think_recharge_abilities(edict_t* ent) {
	if (!G_EntIsAlive(ent))
		return;

	if (!(level.framenum % (int)(1 / FRAMETIME)) && (level.time > ent->client->charge_regentime)) {
		// 3.5 add all abilities here that must recharge

		// beam ability
		if (!ent->myskills.abilities[BEAM].disable
			&& (ent->myskills.abilities[BEAM].current_level > 0)
			&& !ent->client->firebeam) {
			if (ent->myskills.abilities[BEAM].charge < 100) {
				ent->myskills.abilities[BEAM].charge += 10;
				ent->client->charge_time = level.time + 1.0; // show charge until we are full

				if (ent->myskills.abilities[BEAM].charge > 100)
					ent->myskills.abilities[BEAM].charge = 100;
			}
		}

		// shield ability
		if (!ent->myskills.abilities[SHIELD].disable
			&& (ent->myskills.abilities[SHIELD].current_level > 0)
			&& !ent->shield) {
			if (ent->myskills.abilities[SHIELD].charge < SHIELD_MAX_CHARGE) {
				ent->myskills.abilities[SHIELD].charge += SHIELD_CHARGE_RATE;
				ent->client->charge_time = level.time + 1.0; // show charge until we are full
			}

			if (ent->myskills.abilities[SHIELD].charge > SHIELD_MAX_CHARGE)
				ent->myskills.abilities[SHIELD].charge = SHIELD_MAX_CHARGE;
		}

		// berserker sprint
		if ((ent->mtype == MORPH_BERSERK) && !ent->superspeed) {
			if (ent->myskills.abilities[BERSERK].charge < SPRINT_MAX_CHARGE) {
				ent->myskills.abilities[BERSERK].charge += SPRINT_CHARGE_RATE;
				ent->client->charge_time = level.time + 1.0; // show charge until we are full
			}

			if (ent->myskills.abilities[BERSERK].charge > SPRINT_MAX_CHARGE)
				ent->myskills.abilities[BERSERK].charge = SPRINT_MAX_CHARGE;
		}

		// az: super speed sprint
		qboolean can_superspeed = !ent->myskills.abilities[SUPER_SPEED].disable && ent->myskills.abilities[SUPER_SPEED].current_level;
		if (!ent->superspeed && can_superspeed) {
			if (ent->myskills.abilities[SUPER_SPEED].charge < SPRINT_MAX_CHARGE) {
				ent->myskills.abilities[SUPER_SPEED].charge += SPRINT_CHARGE_RATE;
				ent->client->charge_time = level.time + 1.0; // show charge until we are full
			}

			if (ent->myskills.abilities[SUPER_SPEED].charge > SPRINT_MAX_CHARGE)
				ent->myskills.abilities[SUPER_SPEED].charge = SPRINT_MAX_CHARGE;
		}

		// stop showing charge on hud
		if (ent->client->charge_index && (level.time > ent->client->charge_time))
			ent->client->charge_index = 0;
	}
}


void think_ability_cloak(edict_t* ent) {
	if (!ent->myskills.abilities[CLOAK].disable) {
		int min_idle_frames = qf2sf(11);
		int talentlevel = vrx_get_talent_level(ent, TALENT_IMP_CLOAK);

		// if (ent->myskills.abilities[CLOAK].current_level < 10 && vrx_get_talent_level(ent, TALENT_IMP_CLOAK) < 4)

		if (ent->myskills.abilities[CLOAK].current_level < 10) {
			min_idle_frames -= qf2sf(ent->myskills.abilities[CLOAK].current_level);
		}
		else {
			min_idle_frames = 1;
		}

		qboolean idled_enough = ent->client->idle_frames >= min_idle_frames;
		qboolean has_no_curses = !que_typeexists(ent->auras, 0);
		qboolean ability_delay_over = (level.time > ent->client->ability_delay);
		qboolean has_no_flag = !vrx_has_flag(ent);
		qboolean has_no_summons = !V_HasSummons(ent);
		qboolean is_not_automagging = !ent->automag;
		qboolean can_cloak = idled_enough
			&& has_no_curses
			&& ability_delay_over
			&& has_no_flag
			&& has_no_summons
			&& is_not_automagging;

		if (ent->myskills.abilities[CLOAK].current_level > 0 && can_cloak) {
			//if (!ent->svflags & SVF_NOCLIENT)
			//	que_list(ent->auras);

			if (!ent->client->cloaking) // Only when a switch is done
				vrx_remove_player_summonables(ent); // 3.75 no more cheap apps with cloak+laser/monster/etc

			ent->svflags |= SVF_NOCLIENT;
			ent->client->cloaking = true;
		}
		else if (ent->movetype != MOVETYPE_NOCLIP && !(ent->flags & FL_CHATPROTECT)
			&& !(ent->flags & FL_COCOONED))// don't see player-monsters
		{
			ent->client->cloaking = false;
			ent->svflags &= ~SVF_NOCLIENT;

		}
		else
			ent->client->cloaking = false;
	}
}

void think_ability_antigrav(edict_t* ent) {
	if (ent->antigrav && ent->deadflag != DEAD_DEAD && !ent->groundentity) {
		ent->client->pers.inventory[power_cube_index] -= sf2qf(ANTIGRAV_COST);
		ent->velocity[2] += sf2qf(55);
		if (ent->client->pers.inventory[power_cube_index] <= 0) {
			ent->antigrav = 0;
			safe_cprintf(ent, PRINT_HIGH, "Antigrav disabled.\n");
		}
	}
}

void think_ability_superspeed(edict_t* ent) {
	if (ent->superspeed && ent->deadflag != DEAD_DEAD) {
		//3.0 Blessed players get a speed boost too
		que_t* slot = NULL;
		slot = que_findtype(ent->curses, NULL, BLESS);

		//eat cubes if the player isn't blessed
		if (slot == NULL) {
			// if we are not morphed, then just use cubes
			if (ent->mtype != MORPH_BERSERK) {
				// ent->client->pers.inventory[power_cube_index] -= SUPERSPEED_DRAIN_COST;
				if (ent->myskills.abilities[SUPER_SPEED].charge >= SUPERSPEED_DRAIN_COST) {
					ent->client->charge_index = SUPER_SPEED + 1;
					ent->client->charge_time = level.time + 1.0;
					ent->myskills.abilities[SUPER_SPEED].charge -= SUPERSPEED_DRAIN_COST;
				}

			}
			else // berserker can sprint
			{
				// if superspeed is upgraded and we have insufficient charge, then use cubes
				if (ent->myskills.abilities[SUPER_SPEED].current_level > 0 &&
					ent->myskills.abilities[BERSERK].charge < SPRINT_COST) {
					if (ent->myskills.abilities[SUPER_SPEED].charge >= SUPERSPEED_DRAIN_COST) {
						ent->client->charge_index = SUPER_SPEED + 1;
						ent->client->charge_time = level.time + 1.0;
						ent->myskills.abilities[SUPER_SPEED].charge -= SUPERSPEED_DRAIN_COST;
					}
					// otherwise use ability charge
				}
				else {
					if (ent->myskills.abilities[BERSERK].charge >= SPRINT_COST) {
						ent->client->charge_index = BERSERK + 1;
						ent->client->charge_time = level.time + 1.0;
						ent->myskills.abilities[BERSERK].charge -= SPRINT_COST;
					}
				}
			}
		}
	}
}

void think_ability_armor_regen(edict_t* ent, int max_armor, int* armor) {
	armor = &ent->client->pers.inventory[body_armor_index];
	max_armor = MAX_ARMOR(ent);

	if (!ent->myskills.abilities[ARMOR_REGEN].disable
		&& (ent->myskills.abilities[ARMOR_REGEN].current_level > 0)
		&& G_EntIsAlive(ent) && (*armor < max_armor)
		&& (level.time > ent->client->armorregen_time)
		&& !(ctf->value && ctf_enable_balanced_fc->value && vrx_has_flag(ent))
		&& !que_findtype(ent->curses, NULL, CURSE)) // can't regen when cursed
	{
		int health_factor;

		if (ent->myskills.class_num == CLASS_KNIGHT)
			health_factor = floattoint(1.5 * ent->myskills.abilities[ARMOR_REGEN].current_level);
		else
			health_factor = floattoint(1 * ent->myskills.abilities[ARMOR_REGEN].current_level);
		//gi.dprintf("%d\n", health_factor);
		if (health_factor < 1)
			health_factor = 1;

		*armor += health_factor;
		if (*armor > max_armor)
			*armor = max_armor;

		float nextRegen = 5 / (float)ent->myskills.abilities[ARMOR_REGEN].current_level;
		if (nextRegen < 0.5)
			nextRegen = 0.5;

		ent->client->armorregen_time = level.time + nextRegen;
	}
}

void think_ability_health_regen(edict_t* ent) {
	if ((ent->health < ent->max_health) && (!ent->myskills.abilities[REGENERATION].disable)
		&& (ent->deadflag != DEAD_DEAD) && (level.time > ent->client->healthregen_time)
		&& !(ctf->value && ctf_enable_balanced_fc->value && vrx_has_flag(ent))) {
		//3.0 cursed players can't heal through regeneration
		if (que_findtype(ent->curses, NULL, CURSE) == NULL) {
			int health_factor = 1 * ent->myskills.abilities[REGENERATION].current_level; // Regeneration OP. :D
			ent->health += health_factor;

			if (ent->health > ent->max_health)
				ent->health = ent->max_health;
			//stuffcmd(ent, va("\nplay %s\n", "items/n_health.wav"));
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/n_health.wav"), 1, ATTN_STATIC, 0);
			float nextRegen = 5.0 / (float)ent->myskills.abilities[REGENERATION].current_level;
			if (nextRegen < 0.5)
				nextRegen = 0.5;

			ent->client->healthregen_time = level.time + nextRegen;
		}
	}
}

void think_tech_regeneration(edict_t* ent) {
	if (ent->client->pers.inventory[regeneration_index]) {
		edict_t* e;
		qboolean regenerate, regenNow = false;

		if (PM_PlayerHasMonster(ent))
			e = ent->owner;
		else
			e = ent;

		if (e->monsterinfo.regen_delay1 == level.framenum)
			regenNow = true;

		if (ent->myskills.level <= 5)
			regenerate = M_Regenerate(e, qf2sf(100), qf2sf(10), 1.0, true, true, false, &e->monsterinfo.regen_delay1);
		else if (ent->myskills.level <= 10)
			regenerate = M_Regenerate(e, qf2sf(200), qf2sf(10), 1.0, true, true, false, &e->monsterinfo.regen_delay1);
		else
			regenerate = M_Regenerate(e, qf2sf(300), qf2sf(10), 1.0, true, true, false, &e->monsterinfo.regen_delay1);

		if (regenNow && regenerate)
			gi.sound(ent, CHAN_ITEM, gi.soundindex("ctf/tech4.wav"), 1, ATTN_STATIC, 0);
	}
}

void think_ability_fury(edict_t* ent) {
	if (!(level.framenum % (int)(1 / FRAMETIME)) && (ent->fury_time > level.time && ent->client)) {
		if (G_EntIsAlive(ent) && !(ctf->value && ctf_enable_balanced_fc->value && vrx_has_flag(ent))) {
			int maxHP = MAX_HEALTH(ent);
			int maxAP = MAX_ARMOR(ent);
			int* armor = &ent->client->pers.inventory[body_armor_index];
			float factor = FURY_INITIAL_REGEN + (FURY_ADDON_REGEN * ent->myskills.abilities[FURY].current_level);

			if (factor > FURY_MAX_REGEN)
				factor = FURY_MAX_REGEN;

			//Regenerate some health.
			if (ent->health < maxHP)
				ent->health_cache += maxHP * factor;
			//Regenerate some health.
			if (*armor < maxAP)
				ent->armor_cache += maxAP * factor;

		}
		//	else	ent->fury_time = 0.0;
	}
}

void think_ability_mind_absorb(edict_t* ent) {
	if (ent->myskills.abilities[MIND_ABSORB].current_level > 0) {
		int cooldown = (int)(5.0f / FRAMETIME);

		//Talent: Mind Control
		if (vrx_get_talent_slot(ent, TALENT_IMP_MINDABSORB) != -1)
			cooldown -= 5 * vrx_get_talent_level(ent, TALENT_IMP_MINDABSORB);

		if (!(level.framenum % cooldown))
			MindAbsorb(ent);
	}
}

void think_talent_ammo_regen(edict_t* ent) {
	if (ent->client && vrx_get_talent_slot(ent, TALENT_BASIC_AMMO_REGEN) != -1) {
		talent_t* talent = &ent->myskills.talents.talent[vrx_get_talent_slot(ent, TALENT_BASIC_AMMO_REGEN)];

		if (talent->upgradeLevel > 0 && (talent->delay < level.time)
			&& !(ent->client->buttons & BUTTON_ATTACK)) // don't regen ammo while firing
		{
			//Give them some ammo
			if (ent->client->ammo_index == rocket_index)
				V_GiveAmmoClip(ent, 1.0f, AMMO_ROCKETS);
			if (ent->client->ammo_index == bullet_index)
				V_GiveAmmoClip(ent, 1.0f, AMMO_BULLETS);
			if (ent->client->ammo_index == cell_index)
				V_GiveAmmoClip(ent, 1.0f, AMMO_CELLS);
			if (ent->client->ammo_index == grenade_index)
				V_GiveAmmoClip(ent, 1.0f, AMMO_GRENADES);
			if (ent->client->ammo_index == shell_index)
				V_GiveAmmoClip(ent, 1.0f, AMMO_SHELLS);
			if (ent->client->ammo_index == slug_index)
				V_GiveAmmoClip(ent, 1.0f, AMMO_SLUGS);

			talent->delay = level.time + 15 - talent->upgradeLevel * 2;    //10 seconds - 1 seconds per upgrade
		}
	}
}

void think_talent_life_regen(edict_t* ent) {
	if (ent->client
		&& vrx_get_talent_slot(ent, TALENT_LIFE_REG) != -1
		&& G_EntIsAlive(ent) && (ent->health < ent->max_health)) {

		talent_t* talent = &ent->myskills.talents.talent[vrx_get_talent_slot(ent, TALENT_LIFE_REG)];
		if (talent->upgradeLevel > 0) {
			int health_factor = 1;
			ent->health += health_factor;
			if (ent->health > ent->max_health)
				ent->health = ent->max_health;
			ent->client->healthregen_time = 950;
		}
	}
}

void think_talent_armor_regen(const edict_t* ent, int max_armor, int* armor) {
	if (ent->client
		&& vrx_get_talent_slot(ent, TALENT_ARMOR_REG) != -1
		&& G_EntIsAlive(ent) && (*armor < max_armor)) {

		const talent_t* talent = &ent->myskills.talents.talent[vrx_get_talent_slot(ent, TALENT_ARMOR_REG)];
		if (talent->upgradeLevel > 0) {
			int health_factor = 1;
			//gi.dprintf("%d\n", health_factor);
			if (health_factor < 1)
				health_factor = 1;
			*armor += health_factor;
			if (*armor > max_armor)
				*armor = max_armor;

			float nextRegen = 0.7;
			if (nextRegen < 0.7)
				nextRegen = 0.7;

			ent->client->armorregen_time = level.time + nextRegen;
		}
	}
}

void think_cocoon_player_bonus_timeout(edict_t* ent) {
	if (ent->cocoon_factor > 0 && level.time > ent->cocoon_time - 5) {
		if (!(level.framenum % (int)(1 / FRAMETIME)))
			safe_cprintf(ent, PRINT_HIGH, "Cocoon bonus wears off in %.0f second(s)\n", ent->cocoon_time - level.time);

		if (level.time >= ent->cocoon_time) {
			safe_cprintf(ent, PRINT_HIGH, "Cocoon bonus wore off\n");
			ent->cocoon_time = 0;
			ent->cocoon_factor = 0;
		}
	}
}

void think_alliance_timeout(edict_t* ent) {
	if (ent->client->allying && (level.time >= ent->client->ally_time + ALLY_WAIT_TIMEOUT)) {
		safe_cprintf(ent->client->allytarget, PRINT_HIGH, "Alliance request timed out.\n");
		AbortAllyWait(ent->client->allytarget);
		safe_cprintf(ent, PRINT_HIGH, "Alliance request timed out.\n");
		AbortAllyWait(ent);
	}
}

void think_spree_war(edict_t* ent) {
	if (SPREE_WAR && (ent->myskills.streak >= SPREE_WARS_START) && (SPREE_DUDE == ent)) {
		if (level.time < SPREE_TIME + 120) {
			if (SPREE_TIME + 90 == level.time)
				safe_cprintf(ent, PRINT_HIGH, "Hurry up! Only 30 seconds remaining...\n");
			else if (SPREE_TIME + 110 == level.time)
				gi.bprintf(PRINT_HIGH, "HURRY! War ends in 10 seconds. %s must die!\n", ent->client->pers.netname);
		}
		else {
			gi.bprintf(PRINT_HIGH, "%s's spree war ran out of time :(\n", ent->client->pers.netname);
			SPREE_WAR = false;
			SPREE_DUDE = NULL;
			ent->myskills.streak = 0;
		}
	}
}

void think_wormhole(edict_t* ent) {
	if (ent->flags & FL_WORMHOLE) {
		if (level.time == ent->client->wormhole_time - 10)
			safe_cprintf(ent, PRINT_HIGH, "You have 10 seconds to exit the wormhole.\n");
		if (level.time == ent->client->wormhole_time - 5)
			safe_cprintf(ent, PRINT_HIGH, "You have 5 seconds to exit the wormhole.\n");
		if (level.time == ent->client->wormhole_time - 4)
			safe_cprintf(ent, PRINT_HIGH, "You have 4 seconds to exit the wormhole.\n");
		if (level.time == ent->client->wormhole_time - 3)
			safe_cprintf(ent, PRINT_HIGH, "You have 3 seconds to exit the wormhole.\n");
		if (level.time == ent->client->wormhole_time - 2)
			safe_cprintf(ent, PRINT_HIGH, "You have 2 seconds to exit the wormhole.\n");
		if (level.time == ent->client->wormhole_time - 1)
			safe_cprintf(ent, PRINT_HIGH, "You have 1 second to exit the wormhole.\n");

		// remove wormhole flag and force respawn
		if (level.time == ent->client->wormhole_time) {
			ent->flags &= ~FL_WORMHOLE;
			ent->svflags &= ~SVF_NOCLIENT;
			ent->movetype = MOVETYPE_WALK;
			Teleport_them(ent);
		}
	}
}

void think_ability_vampire(edict_t* self) {
	if (self->myskills.abilities[VAMPIRE].current_level > 5) {
		if (!level.daytime && self->deadflag != DEAD_DEAD && self->solid != SOLID_NOT) {
			if (!self->client->lowlight) {
				self->client->ps.rdflags |= RDF_IRGOGGLES;
				self->client->lowlight = true;
			}
		}
		else {
			if (self->client->lowlight) {
				self->client->ps.rdflags &= ~RDF_IRGOGGLES;
				self->client->lowlight = false;
			}
		}
	}
}


// grid.c
void DrawNearbyGrid(edict_t* ent);

void DrawChildLinks(edict_t* ent);

// magic.c
void DeflectProjectiles(edict_t* self, float chance, qboolean in_front);

// p_parasite.c
void think_ability_parasite_attack(edict_t* ent);

// class_brain.c
void brain_fire_beam(edict_t* self);

// class_demon.c
void PlagueCloudSpawn(edict_t* ent);


void vrx_client_think(edict_t* ent) {
	int max_armor;    // 3.5 max armor client can hold
	int* armor;        // 3.5 pointer to client armor

	if (ent->client->showGridDebug > 0)
	{
		if (ent->client->showGridDebug <= 2)
			DrawNearbyGrid(ent);
		if (ent->client->showGridDebug >= 2)
			DrawChildLinks(ent);
	}

	//DrawPath();

	// 3.5 don't stuff player commands all at once, or they will overflow
	if (!(level.framenum % (int)(1 / FRAMETIME)))
		StuffPlayerCmds(ent);

	think_cocoon_player_bonus_timeout(ent);

	//4.2 are they in a wormhole? they can't stay in there forever!
	think_wormhole(ent);
	think_spree_war(ent);
	think_alliance_timeout(ent);

	// 4.0 we shouldn't be here if we're dead
	if ((ent->health < 1) || (ent->deadflag == DEAD_DEAD) || G_IsSpectator(ent) || (ent->flags & FL_WORMHOLE))
		return;

	// 10 cubes per second (new meditation)
	if (ent->manacharging)
		ent->client->pers.inventory[power_cube_index] += 1 * vrx_get_talent_level(ent, TALENT_MEDITATION);

	// 4.2 recharge blaster ammo
	// (apple)
	// Blaster should be affected by player's max ammo.
	if (!(sf2qf(level.framenum) % 2) && !(ent->client->buttons & BUTTON_ATTACK)
		&& (ent->monsterinfo.lefty < (25 + 12.5 * ent->myskills.abilities[MAX_AMMO].level)))
		ent->monsterinfo.lefty++;

	if (ent->shield && (level.time > ent->shield_activate_time)) {
		if (ent->myskills.abilities[SHIELD].charge < SHIELD_COST) {
			ent->shield = 0;
		}
		else {
			//Talent: Repel
			int talentLevel;
			if ((talentLevel = vrx_get_talent_level(ent, TALENT_REPEL)) > 0)
				DeflectProjectiles(ent, ((float)0.1 * talentLevel), true);

			ent->myskills.abilities[SHIELD].charge -= SHIELD_COST;
			ent->client->charge_index = SHIELD + 1;
			ent->client->charge_time = level.time + 1.0;
			ent->client->ability_delay = level.time + FRAMETIME;        // can't use abilities
			ent->monsterinfo.attack_finished = level.time + FRAMETIME;    // morphed players can't attack
		}
	}

	think_recharge_abilities(ent);
	CursedPlayer(ent);
	AllyID(ent);

	armor = &ent->client->pers.inventory[body_armor_index];
	max_armor = MAX_ARMOR(ent);

	//Talent: NanoSuit
	think_talent_armor_regen(ent, max_armor, armor);

	//Talent: Life Alien
	think_talent_life_regen(ent);

	//Talent: Basic Ammo Regeneration
	think_talent_ammo_regen(ent);


	if (ent->myskills.abilities[AMMO_REGEN].current_level > 0)
		think_ability_ammo_regen(ent);

	// We simply restore 5 cubes more often.
	int regen_frame = (int)(5.0f / ent->myskills.abilities[POWER_REGEN].current_level / FRAMETIME);
	qboolean is_regen_frame = !(level.framenum % regen_frame);
	if (ent->myskills.abilities[POWER_REGEN].current_level > 0 && is_regen_frame)
		think_ability_power_regen(ent);

	//3.0 Mind absorb every x seconds
	think_ability_mind_absorb(ent);

	//4.1 Fury ability causes regeneration to occur every second.
	think_ability_fury(ent);

	// regeneration tech
	think_tech_regeneration(ent);

	// regenerate health
	think_ability_health_regen(ent);

	//4.4 - max hp/sec capped to 50/sec (25 per 5 frames)
	V_HealthCache(ent, 50, 5);
	//4.4 - max armor/sec capped to 25/sec
	V_ArmorCache(ent, 25, 5);

	// 3.5 regenerate armor
	think_ability_armor_regen(ent, max_armor, armor);

	if (ent->client->thrusting)
		think_ability_jetpack(ent);

	if (ent->sucking)
		think_ability_parasite_attack(ent);

	think_ability_superspeed(ent);

	think_ability_antigrav(ent);

	if ((ent->client->hook_state == HOOK_ON) && ent->client->hook)
		hook_service(ent->client->hook);

	if (!ent->myskills.abilities[VAMPIRE].disable)
		think_ability_vampire(ent);

	if (ent->client->firebeam)
		brain_fire_beam(ent);

	if (ent->myskills.administrator == 11)
		ent->client->ping = GetRandom(300, 400);

	// cloak them
	think_ability_cloak(ent);

	if ((!ent->myskills.abilities[PLAGUE].disable) && (ent->myskills.abilities[PLAGUE].current_level)) {
		PlagueCloudSpawn(ent);
	}

	if ((!ent->myskills.abilities[BLOOD_SUCKER].disable) && (ent->myskills.abilities[BLOOD_SUCKER].current_level)) {
		if (ent->mtype == M_MYPARASITE)
			PlagueCloudSpawn(ent);
	}

	if (ent->flags & FL_CHATPROTECT)
		ent->client->ability_delay = level.time + 1; // can't use abilities in chat-protect
}
