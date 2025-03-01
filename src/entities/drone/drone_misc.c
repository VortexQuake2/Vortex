#include "g_local.h"
#include "../../gamemodes/ctf.h"
#include "../../gamemodes/invasion.h"
#include "../../characters/class_limits.h"

// number of seconds monsters will blink after being selected or issued commands
#define MONSTER_BLINK_DURATION		2.0

#define DRONE_GOAL_TIMEOUT		3		// seconds before a movement goal times out

void drone_think (edict_t *self);
void drone_wakeallies (edict_t *self);
qboolean drone_findtarget (edict_t *self, qboolean force);
void init_drone_gunner (edict_t *self);
void init_drone_parasite (edict_t *self);
void init_drone_bitch (edict_t *self);
void init_drone_brain (edict_t *self);
void init_drone_medic (edict_t *self);
void init_drone_tank (edict_t *self);
void init_drone_mutant (edict_t *self);
void init_drone_decoy (edict_t *self);
void init_drone_commander (edict_t *self);
void init_drone_supertank (edict_t *self);
void init_drone_jorg (edict_t *self);
void init_drone_makron (edict_t *self);
void init_drone_soldier (edict_t *self);
void init_drone_gladiator (edict_t *self);
void init_drone_berserk (edict_t *self);
void init_drone_infantry (edict_t *self);
void init_drone_flyer (edict_t* self);
void init_drone_floater(edict_t* self);
void init_drone_hover(edict_t* self);
void init_drone_shambler(edict_t* self);
void init_baron_fire(edict_t* self);
void init_skeleton(edict_t* self);
void init_golem(edict_t* self);
int crand (void);
edict_t* vrx_inv_get_monster_spawn(edict_t* from);

// Drone Lists -az

/* The purpose of this is to decrease those silly G_Find calls for drones by chaining them together. */
edict_t *DroneList[1024];
int DroneCount = 0;

void DroneList_Clear()
{
    memset(DroneList, 0, sizeof DroneList);
	DroneCount = 0;
}

edict_t *DroneList_Iterate()
{
	return DroneList[0];
}

edict_t *DroneList_Next(edict_t *ent)
{
    if (ent->monsterinfo.dronelist_index + 1 < MAX_EDICTS) {
		edict_t *next_drone = DroneList[ent->monsterinfo.dronelist_index + 1];
		if (ent == next_drone) {
			gi.dprintf("invoking horrible drone list hack because we would otherwise infinite loop\n");
			ent->monsterinfo.dronelist_index = ent->monsterinfo.dronelist_index + 1;
			return DroneList_Next(ent);
		}

		if (next_drone != NULL && next_drone->monsterinfo.dronelist_index < ent->monsterinfo.dronelist_index) {
			// wooee this is terrible 
			gi.dprintf("another horrible hack: next monster would have put us in an infinite loop. removing.\n");
			DroneList_Remove(next_drone); // note: this will fail because the next drone on the list is freed and the drone_list index is 0
			return DroneList_Next(ent);
		}

	    return next_drone;
	}

    return NULL;
}

void DroneList_Insert(edict_t* new_ent)
{
    if (DroneCount >= MAX_EDICTS) {
        gi.dprintf("drone list: overflowed the list");
        return;
    }

    new_ent->monsterinfo.dronelist_index = DroneCount;
	DroneList[DroneCount++] = new_ent;
}

/* we don't free it, we just remove it from the list */
//FIXME: add another parameter for index, so that if we accidentally pass it a freed entity on the list, it can actually remove it because the freed entity's dronelist_index is 0
void DroneList_Remove(edict_t *ent)
{
	// is monster index within valid range of list?
	if (ent->monsterinfo.dronelist_index >= 0 && ent->monsterinfo.dronelist_index < DroneCount) {
        int index = ent->monsterinfo.dronelist_index;
		// have we found this monster within the drone list?
	    if (DroneList[index] == ent) { // follows the same logic as player spawn list
	        DroneCount--; // reduce the count, and hence, the length of the list, by 1

	        DroneList[index] = DroneList[DroneCount]; // move the last monster on the list to this position
	        if (DroneList[index]) // if the list wasn't empty, then update this monster's index to the new position
                DroneList[index]->monsterinfo.dronelist_index = index;

	        DroneList[DroneCount] = NULL; // we moved the monster at the end of the list, so now the end of the list is NULL

	        ent->monsterinfo.dronelist_index = -1; // this monster has been removed from the drone list
	    	if (ent->activator && ent->activator->notify_drone_death) {
	    		ent->activator->notify_drone_death(ent);
	    	}
	    }
#ifdef _DEBUG
        else {
			gi.dprintf("drone list: mismatched index (%d != %d)\n", index, DroneList[index]->monsterinfo.dronelist_index);

			int duplicates = 0; // god forbid
			for (int i = 0; i < DroneCount; i++) {
				if (DroneList[i] == ent) {
					for (int j = i; j < DroneCount - 1; j++) {
						DroneList[j] = DroneList[j + 1];
						DroneList[j]->monsterinfo.dronelist_index = j;
					}

					DroneList[DroneCount - 1] = NULL; // guardian angel
					DroneCount--;
					duplicates += 1;
				}
			}

			if (duplicates > 0) {
				gi.dprintf("removed monster has duplicates %d lol\n", duplicates);
			}
        }
#endif

	}
#ifdef _DEBUG
	else {
		// it's really not a big deal if this happens.
	    // gi.dprintf("drone list: removing a monster twice? %d out of range (0 to %d)\n", ent->monsterinfo.dronelist_index, DroneCount);
	}
#endif
}

void DroneList_Print(edict_t* ent, edict_t *owner)
{
	for (int i = 0; i < DroneCount; i++)
	{
		if (!DroneList[i])
		{
			gi.dprintf("index %d: NULL\n", i);
			continue;
		}
		if (ent && DroneList[i] == ent)
		{
			// found the monster we're looking for
			gi.dprintf("index %d=%d: %s (this monster)\n", i, ent->monsterinfo.dronelist_index, V_GetMonsterName(ent));
		}
		else if (owner && DroneList[i]->activator && DroneList[i]->activator == owner)
		{
			// found monster we own
			gi.dprintf("index %d=%d: %s (activator-owned monster)\n", i, DroneList[i]->monsterinfo.dronelist_index, V_GetMonsterName(DroneList[i]));
		}
		else
		{
			gi.dprintf("index %d=%d: %s (other monster)\n", i, DroneList[i]->monsterinfo.dronelist_index, V_GetMonsterName(DroneList[i]));
			// found a different monster owned by someone else
		}
	}

}
// end Drone Lists

float drone_damagelevel(const edict_t* ent)
{
	int level = ent->monsterinfo.level;

	// player monsters don't get softcapped
	if (G_GetClient(ent)) 
		return level;

	// disable softcap
	if (M_ENABLE_WORLDSPAWN_SOFTCAP == 0)  // NOLINT(clang-diagnostic-float-equal)
		return level;

	if (level <= 0)
	{
		gi.dprintf("weird level %d being passed to drone_damagelevel\n", level);
		return 1;
	}

	return min(7 * log10f(level) + 1, 20);
}

qboolean drone_ValidChaseTarget (edict_t *self, edict_t *target)
{
	/*
	if (target && target->inuse && target->classname)
		gi.dprintf("drone_ValidChaseTarget() is checking %s\n", target->classname);
	else
		gi.dprintf("drone_ValidChaseTarget() is checking <unknown>\n");
	*/
	if (!target || !target->inuse)
	{
		//gi.dprintf("target invalid or not in use\n");
		return false;
	}

	// chasing a combat point/goal
	if ((self->monsterinfo.aiflags & AI_COMBAT_POINT) && target->inuse
		&& ((target->mtype == INVASION_NAVI) || (target->mtype == PLAYER_NAVI)
			|| (target->mtype == M_COMBAT_POINT) || target->mtype == INVASION_PLAYERSPAWN))
		return true;

	//FIXME: we should clean this up
	// chasing invasion monster spawn

//lepe
	if (invasion->value && target->mtype == INVASION_MONSTERSPAWN
		&& !OnSameTeam(self, self->enemy) && visible(self, self->enemy))
		return true;

	if (!target->takedamage || (target->solid == SOLID_NOT))
	{
		//gi.dprintf("target cannot take damage or is non-solid\n");
		return false;
	}

	//if (!G_EntExists(target))
	//	return false;

	// if we're standing ground and have a limited sight range
	// ignore enemies that move outside of this range (since we can't attack them)
	if ((self->monsterinfo.aiflags & AI_STAND_GROUND)
		&& (entdist(self, target) > self->monsterinfo.sight_range))
	{
		//gi.dprintf("target is out of range\n");
		return false;
	}

	if ((self->monsterinfo.aiflags & AI_MEDIC) && M_ValidMedicTarget(self, self->enemy))
		return true;

	// ignore players that are in chat-protect
	if (target->flags & FL_CHATPROTECT)
		return false;

	if (target->flags & FL_GODMODE)
	{
		//gi.dprintf("target has godmode\n");
		return false;
	}

	if (target->svflags & SVF_NOCLIENT && target->mtype != M_FORCEWALL)
	{
		//gi.dprintf("target is invisible\n");
		return false;
	}

	if (target->client)
	{
		if (target->client->respawn_time > level.time)
			return false; // don't pursue respawning players
		if (target->client->cloaking && (target->client->idle_frames >= qf2sf(100)))
			return false; // don't pursue cloaked targets
	}

	if (target->health < 1 || target->deadflag == DEAD_DEAD)
	{
		//gi.dprintf("target is dead\n");
		return false;
	}

	if (que_typeexists(target->curses, CURSE_FROZEN))
	{
		//gi.dprintf("target is frozen\n");
		return false;
	}

	//FIXME: we should do a better check than this
	if (self->enemy && OnSameTeam(self, target))
	{
		//gi.dprintf("target is a teammate\n");
		return false;
	}
	return true;
}

qboolean drone_isclearshot (edict_t *self, edict_t *target)
{
	vec3_t	forward, start, end;
	trace_t	tr;

	// get muzzle origin (roughly)
	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorMA(self->s.origin, self->maxs[1], forward, start);
	start[2] += self->viewheight;
	// get target position
	VectorCopy(target->s.origin, end);
	if (target->client)
		end[2] += target->viewheight;
	tr = gi.trace(start, NULL, NULL, self->enemy->s.origin, self, MASK_SHOT);
	if (!tr.ent || (tr.ent != target))
		return false;
	return true;
}



/*
qboolean CanStep (edict_t *self, float yaw, float dist)
{
	int		stepsize;
	vec3_t	start, end, test, move;
	trace_t trace;

	yaw = yaw*M_PI*2 / 360;
	move[0] = cos(yaw)*dist;
	move[1] = sin(yaw)*dist;
	move[2] = 0;

	// try the move
	VectorAdd (self->s.origin, move, start);

	// push down from a step height above the wished position
	if (!(self->monsterinfo.aiflags & AI_NOSTEP))
		stepsize = STEPHEIGHT;
	else
		stepsize = 1;

	start[2] += stepsize;
	VectorCopy (start, end);
	end[2] -= stepsize*2;

	trace = gi.trace (start, self->mins, self->maxs, end, self, MASK_MONSTERSOLID);

	if (trace.allsolid)
		return false;

	if (trace.startsolid)
	{
		start[2] -= stepsize;
		trace = gi.trace (start, self->mins, self->maxs, end, self, MASK_MONSTERSOLID);
		if (trace.allsolid || trace.startsolid)
			return false;
	}

	// don't go in to water
	if (!self->waterlevel)
	{
		test[0] = trace.endpos[0];
		test[1] = trace.endpos[1];
		test[2] = trace.endpos[2] + self->mins[2] + 1;

		if (gi.pointcontents(test) & MASK_WATER)
			return false;
	}

	if (trace.fraction == 1.0)
		return false; // walked off an edge
	return true;
}
*/

void drone_ai_checkattack (edict_t *self)
{
	vec3_t	start, end;
	trace_t	tr;

	if (!self->monsterinfo.attack)
		return;
	if (!G_EntExists(self->enemy))
		return; // enemy is invalid or a combat point

	//if (self->monsterinfo.aiflags & AI_COMBAT_POINT)
	//	return; // we're busy reaching a combat point

	// don't change attacks if we're busy with the last one
	if (level.time < self->monsterinfo.attack_finished)
	{
		//gi.dprintf("attack not ready\n");
		if (!self->monsterinfo.melee)
			return;
		if (level.time < self->monsterinfo.melee_finished)
			return;
		self->monsterinfo.melee(self);
		return;
	}

	// if we see an easier target, go for it
	if (!visible(self, self->enemy))
	{
		self->oldenemy = self->enemy;
		if (!drone_findtarget(self, false))
			return;
		//gi.dprintf("%d going for an easier target\n", self->mtype);
	}

	//if (!infront(self, self->enemy))
	if (!nearfov(self, self->enemy, 0, 60))
	{
		//gi.dprintf("target is not in front\n");
		return;
	}

	// check for blocked shot
	// if the entity blocking us is a valid target, attack them instead!
	G_EntMidPoint(self, start);
	G_EntMidPoint(self->enemy, end);
	tr = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);
	if (!tr.ent || tr.ent != self->enemy)
	{
		//gi.dprintf("blocked shot\n");
		if (G_ValidTarget(self, tr.ent, false, true))
			self->enemy = tr.ent;
		else
			return;
	}
	//AngleVectors(self->s.angles, forward, NULL, NULL);
	//VectorMA(self->s.origin, self->maxs[1]+8, forward , start);
	// our shot is blocked
	//if (!G_IsClearPath(self->enemy, MASK_SHOT, start, self->enemy->s.origin)/*!drone_isclearshot(self, self->enemy)*/)
	//if (!G_ClearPath(self, self->enemy, MASK_SHOT, self->s.origin, self->enemy->s.origin))
	//{
	//	gi.dprintf("shot is blocked\n");
	//	return;
	//}
	//gi.dprintf("attack!\n");
	self->monsterinfo.attack(self);
}

void goalentity_think (edict_t *self)
{
	if (level.time > self->delay)
	{
		if (self->owner)
			self->owner->movetarget = NULL;
		G_FreeEdict(self);
		return;
	}

	self->nextthink = level.time + 0.1;
}

edict_t *SpawnGoalEntity (edict_t *ent, vec3_t org)
{
	edict_t *goal;

	goal = G_Spawn();
	goal->think = goalentity_think;
//	goal->touch = goalentity_touch;
	goal->delay = level.time + DRONE_GOAL_TIMEOUT;
	goal->owner = ent;
	goal->classname = "drone_goal";
	goal->svflags |= SVF_NOCLIENT; // comment this for debugging
//	goal->solid = SOLID_TRIGGER;
//	goal->s.modelindex = gi.modelindex("models/items/c_head/tris.md2");
//	goal->s.effects |= EF_BLASTER;
	VectorCopy(org, goal->s.origin);
	goal->nextthink = level.time + 0.1;
//	gi.linkentity(goal);
	ent->movetarget = goal; // pointer to goal
	return goal;
}

//void AddDmgList (edict_t *self, edict_t *other, int damage);//4.05
void drone_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	//vec3_t	forward, start;

	// decoys, skeletons, and golems don't use pain skins
	if ((self->s.modelindex != 255) && vrx_has_pain_skin(self) && (self->health < (0.5*self->max_health)))
		self->s.skinnum |= 1;

	if (damage > 0 && self->pain_inner)
		self->pain_inner(self, other, kick, damage);

	// ignore non-living objects
	if (!G_EntIsAlive(other))
		return;
	if (other->flags & FL_GODMODE)
		return;
	// ignore teammates
	if (OnSameTeam(self, other))
		return;

	//4.05 make monsters fight nearer targets
	if (self->enemy && self->enemy->inuse)
	{
		// attack if current enemy can't take damage/move (a node/goal?) OR our current enemy
		// isn't visible OR if new enemy is closer than the old one
		if (!self->enemy->takedamage || self->enemy->movetype == MOVETYPE_NONE
			|| !visible(self, self->enemy)
			|| (entdist(self, other) < entdist(self, self->enemy)))
		{
			self->oldenemy = self->enemy;
			self->enemy = other;
			drone_wakeallies(self);
		}
	}
	else
	{
		// we don't already have an enemy, so attack this one
		self->enemy = other;
		drone_wakeallies(self);
	}
}

void drone_death (edict_t *self, edict_t *attacker)
{
	if (self->monsterinfo.resurrected_time > level.time)
		return;

	// az: don't trigger this more than once!
	if (self->deadflag)
		return; 


	//4.2 bosses can drop up to 4 runes
	if (self->mtype == M_COMMANDER || self->mtype == M_SUPERTANK || self->mtype == M_MAKRON)
	{
		edict_t *e;
		float drop_chance = 0.25;

		if (invasion->value)
		    drop_chance = 0.05;


		if ((e = V_SpawnRune(self, attacker, drop_chance, 0)) != NULL)
			V_TossRune(e, (float)GetRandom(200, 1000), (float)GetRandom(200, 1000));
		if ((e = V_SpawnRune(self, attacker, drop_chance, 0)) != NULL)
			V_TossRune(e, (float)GetRandom(200, 1000), (float)GetRandom(200, 1000));
		if ((e = V_SpawnRune(self, attacker, drop_chance, 0)) != NULL)
			V_TossRune(e, (float)GetRandom(200, 1000), (float)GetRandom(200, 1000));
		if ((e = V_SpawnRune(self, attacker, drop_chance, 0)) != NULL)
			V_TossRune(e, (float)GetRandom(200, 1000), (float)GetRandom(200, 1000));
	}
	else
	{
        vrx_roll_rune_drop(self, attacker, false);

		// world monsters sometimes drop ammo
		if (self->activator && !self->activator->client
			&& self->item && (random() >= 0.2) && vrx_spawn_nonessential_ent(self->s.origin))
			Drop_Item(self, self->item);
	}

	// the other place where they are handled is in player death functions
	if (invasion->value || pvm->value) {
		edict_t *cl = G_GetClient(attacker);
		if (cl) {
			cl->myskills.streak++;
			vrx_trigger_spree_abilities(cl);
		}
	}
}

void drone_heal (edict_t *self, edict_t *other, qboolean heal_while_being_damaged)
{
	if (self->health > 0 && level.framenum > self->monsterinfo.regen_delay1)
	{
		if (heal_while_being_damaged || level.time > self->lasthurt + 0.5)
		{
			// check health
			if (self->health < self->max_health && other->client->pers.inventory[power_cube_index] >= 5)
			{
				self->health_cache += (int)(0.50 * self->max_health) + 1;
				self->monsterinfo.regen_delay1 = level.framenum + (int)(1 / FRAMETIME);
				other->client->pers.inventory[power_cube_index] -= 5;
			}

			// check armor
			if (self->monsterinfo.power_armor_power < self->monsterinfo.max_armor
				&& other->client->pers.inventory[power_cube_index] >= 5)
			{
				self->armor_cache += (int)(0.50 * self->monsterinfo.max_armor) + 1;
				self->monsterinfo.regen_delay1 = level.framenum + (int)(1 / FRAMETIME);
				other->client->pers.inventory[power_cube_index] -= 5;
			}
		}
	}
}

void PassThruEntity (edict_t *self, edict_t *other)
{
	trace_t tr;
	vec3_t	start, end, forward, angles;

	// sanity check
	if (!other || !other->inuse || !other->client || self->health < 1)
		return;

	// convert velocity to angles and zero-out pitch (move along 2-D plane)
	vectoangles(other->client->oldvelocity, angles);
	angles[PITCH] = 0;

	// convert back to a normalized vector
	AngleVectors(angles, forward, NULL, NULL);
	VectorNormalize(forward);

	VectorCopy(other->s.origin, start);
	VectorMA(start, (float)(2*G_GetHypotenuse(other->maxs)+2*G_GetHypotenuse(self->maxs)+1), forward, end);
	tr = gi.trace(start, other->mins, other->maxs, end, NULL, MASK_SOLID);

	// update target position - this is the farthest we can get before running into a wall
	VectorCopy(tr.endpos, end);

	// trace final position
	tr = gi.trace(end, other->mins, other->maxs, end, NULL, MASK_SHOT);

	// position is not clear
	if (tr.fraction < 1 || tr.contents & MASK_SHOT || gi.pointcontents(end) & MASK_SHOT || tr.allsolid)
	{
		VectorClear(other->velocity);
		return;
	}

	VectorCopy(end, other->s.origin);
	VectorCopy(end, other->s.old_origin);
	//other->s.event = EV_PLAYER_TELEPORT;
	gi.linkentity(other);
}

void drone_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t	forward, right, start, offset;

	//gi.dprintf("drone_touch\n");
	V_Touch(self, other, plane, surf);

	// the monster's owner or allies can push him around
	//if (G_EntIsAlive(other) && self->activator
	//	&& ((other == self->activator) || IsAlly(self->activator, other)))

	// if this is player-monster, look at the owner/activator
	// FIXME: this doesn't work well because M_Move() doesn't get you close enough
	if (PM_MonsterHasPilot(other) && ((other->activator == self->activator) || IsAlly(self->activator, other->activator)))
	{
		AngleVectors(other->s.angles, forward, NULL, NULL);
		self->velocity[0] += forward[0] * 200;
		self->velocity[1] += forward[1] * 200;
		self->velocity[2] += 200;

		drone_heal(self, other->activator, false);
		return;
	}

	//4.5 in FFA mode, allow players with PvP combat preferences to teleport world monsters that can't see their enemy
	if (ffa->value && (!self->enemy || !visible(self, self->enemy)) && self->activator && self->activator->inuse
		&& !self->activator->client && OnSameTeam(self, other) == 1)
	{

		// teleport effect at origination point
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_TELEPORT_EFFECT);
		gi.WritePosition (self->s.origin);
		gi.multicast (self->s.origin, MULTICAST_PVS);

		self->s.event = EV_PLAYER_TELEPORT;

		if (!vrx_find_random_spawn_point(self, false))
		{
			M_Remove(self, false, false);
			return;
		}

		self->s.event = EV_PLAYER_TELEPORT;

		self->monsterinfo.pausetime = level.time + 1.0;
		if (self->monsterinfo.walk)
			self->monsterinfo.walk(self);
		else
			self->monsterinfo.stand(self);
		self->monsterinfo.inv_framenum = level.framenum + (int)(1 / FRAMETIME);
	}

	if (other && other->inuse && other->client && self->activator
		&& (other == self->activator || IsAlly(self->activator, other)))
	{
		AngleVectors (other->client->v_angle, forward, right, NULL);
		VectorScale (forward, -3, other->client->kick_origin);
		VectorSet(offset, 0, 7,  other->viewheight-8);
		P_ProjectSource (other->client, other->s.origin, offset, forward, right, start);

		self->velocity[0] += forward[0] * 50;
		self->velocity[1] += forward[1] * 50;
		self->velocity[2] += forward[2] * 50;

		drone_heal(self, other, false);
	}
}

void drone_grow (edict_t *self)
{
	V_HealthCache(self, (int)(0.2 * self->max_health), 1);
	V_ArmorCache(self, (int)(0.2 * self->monsterinfo.max_armor), 1);

	// done growing
	if (self->health >= self->max_health)
		self->think = drone_think;
	// check for heal
	else if (vrx_has_pain_skin(self) && self->health >= 0.3 * self->max_health)
	{
		self->s.skinnum &= ~1;
	}

	// if position has been updated, check for ground entity
	if (self->linkcount != self->monsterinfo.linkcount)
	{
		self->monsterinfo.linkcount = self->linkcount;
		M_CheckGround (self);
	}

	// don't slide
	if (self->groundentity)
		VectorClear(self->velocity);

	M_CatagorizePosition (self);
	M_WorldEffects (self);
	M_SetEffects (self);

	if (self->health <= 0)
		M_MoveFrame(self);
	else
		self->s.effects |= EF_PLASMA;

	self->nextthink = level.time + 0.1;
}

void vrx_roll_to_make_champion(edict_t *drone, int *drone_type)
{
	if ((ffa->value || invasion->value == 2 || (pvm->value && !invasion->value)) && drone->monsterinfo.level >= 10 && GetRandom(1, 100) <= 10)//10% chance for a champion to spawn
	{
		drone->monsterinfo.bonus_flags |= BF_CHAMPION;

		if ( (!invasion->value && GetRandom(1, 100) <= 33) // 33% chance to spawn a special champion
			|| (invasion->value == 2 && GetRandom(1, 100) <= 5) )  // 5% chance in invasion.
		{
			int r = GetRandom(1, 7);

			switch (r)
			{
			case 1: drone->monsterinfo.bonus_flags |= BF_GHOSTLY; break;
			case 2: drone->monsterinfo.bonus_flags |= BF_FANATICAL; break;
			case 3: drone->monsterinfo.bonus_flags |= BF_BERSERKER; break;
			case 4: drone->monsterinfo.bonus_flags |= BF_POSESSED; break;
			case 5: drone->monsterinfo.bonus_flags |= BF_STYGIAN; break;
			case 6: drone->monsterinfo.bonus_flags |= BF_UNIQUE_FIRE; *drone_type = 3; break;
			case 7: drone->monsterinfo.bonus_flags |= BF_UNIQUE_LIGHTNING; *drone_type = 8; break;
			default: break;
			}
		}
	}
}

edict_t *vrx_create_drone_from_ent(edict_t *drone, edict_t *ent, int drone_type, qboolean worldspawn, qboolean link_now, int bonus_level)
{
	vec3_t		forward, right, start, end, offset;
	trace_t		tr;
	float		mult;
	int			talentLevel;

	drone->classname = "drone";

	// set monster level
	if (worldspawn)
	{
		if (drone_type >= 30)// tank commander, supertank
			drone->monsterinfo.level = HighestLevelPlayer();
		else if (INVASION_OTHERSPAWNS_REMOVED)
		{
			if (invasion->value == 1)
				drone->monsterinfo.level = GetRandom(LowestLevelPlayer(), HighestLevelPlayer());
			else if (invasion->value == 2) // hard mode invasion
			{
				drone->monsterinfo.level = HighestLevelPlayer()+next_invasion_wave_level-1;
                vrx_roll_to_make_champion(drone, &drone_type);
			}
		}
		else
		{
			if (pvm->value || ffa->value)
				drone->monsterinfo.level = GetRandom(PvMLowestLevelPlayer(), PvMHighestLevelPlayer());
			else
				drone->monsterinfo.level = GetRandom(LowestLevelPlayer(), HighestLevelPlayer());

			drone->monsterinfo.level += bonus_level;

			// 4.5 assign monster bonus flags
			// Champions spawn on invasion hard mode.
            vrx_roll_to_make_champion(drone, &drone_type);
		}
	}
	else
	{
		// calling entity made a sound, used to alert monsters
		ent->lastsound = level.framenum;

		drone->monsterinfo.level = ent->myskills.abilities[MONSTER_SUMMON].current_level;
	}

	if (drone->monsterinfo.level < 1)
		drone->monsterinfo.level = 1;

	drone->activator = ent;
	//drone->svflags |= SVF_MONSTER;
	drone->yaw_speed = 20;
	drone->takedamage = DAMAGE_AIM;
	drone->clipmask = MASK_MONSTERSOLID;
	drone->movetype = MOVETYPE_STEP;
	drone->s.skinnum = 0;
	drone->deadflag = DEAD_NO;
	drone->svflags &= ~SVF_DEADMONSTER;
	drone->svflags |= SVF_MONSTER;
	// NOPE. NO ONE REALLY LIKED TO CHASE MONSTERS ANYWAY!
	//drone->flags |= FL_CHASEABLE; // 3.65 indicates entity can be chase-cammed
	drone->s.effects |= EF_PLASMA;
	drone->s.renderfx |= (RF_FRAMELERP|RF_IR_VISIBLE);
	drone->solid = SOLID_BBOX;
	// use normal think for worldspawned monsters
	if (worldspawn || !ent->client)
		drone->think = drone_think;
	else
	// use pre-think for player-spawn monsters
		drone->think = drone_grow;
	drone->touch = drone_touch;
	drone->monsterinfo.control_cost = M_DEFAULT_CONTROL_COST;
	drone->monsterinfo.cost = M_DEFAULT_COST;
	drone->monsterinfo.sight_range = 1024; // 3.56 default sight range for finding targets
	drone->inuse = true;

	switch(drone_type) // not to be confused with mtype!
	{
	// normal monsters
	case 1: init_drone_gunner(drone);		break;
	case 2: init_drone_parasite(drone);		break;
	case 3: init_drone_bitch(drone);		break;
	case 4: init_drone_brain(drone);		break;
	case 5: init_drone_medic(drone);		break;
	case 6: init_drone_tank(drone);			break;
	case 7: init_drone_mutant(drone);		break;
	case 8: init_drone_gladiator(drone);	break;
	case 9: init_drone_berserk(drone);		break;
	case 10: init_drone_soldier(drone);		break;
	case 11: init_drone_infantry(drone);	break;
	case 12: init_drone_flyer(drone);		break;
	case 13: init_drone_floater(drone);		break;
	case 14: init_drone_hover(drone);		break;
	case 15: init_drone_shambler(drone);	break;
	case 20: init_drone_decoy(drone);		break;
	case 21: init_skeleton(drone);			break;
	case 22: init_golem(drone);				break;

	// bosses
	case 30: init_drone_commander(drone);	break;
	case 31: init_drone_makron(drone);		break;
	case 32: init_baron_fire(drone);		break;
	case 33: init_drone_supertank(drone);	break;
	case 34: init_drone_jorg(drone);		break;
	// default
	default: init_drone_gunner(drone);		break;
	}

	/* az: init functions might have set up a pain function -- address that here */
	if (drone->pain && drone->pain != drone_pain)
		drone->pain_inner = drone->pain;
	else
		drone->pain_inner = NULL; // maybe redundant?

	drone->pain = drone_pain;

	//4.0 gib health based on monster control cost
	if (drone_type < 30)
		drone->gib_health = -drone->monsterinfo.control_cost * BASE_GIB_HEALTH * M_CONTROL_COST_SCALE;
	else
		drone->gib_health = 0;//gib boss immediately

	// base hp/armor multiplier
	mult = 1.0;

	//if (drone->mtype == M_COMMANDER)
	//	mult *= 80;

	//4.5 monster bonus flags
	if (drone->monsterinfo.bonus_flags & BF_UNIQUE_FIRE
		|| drone->monsterinfo.bonus_flags & BF_UNIQUE_LIGHTNING)
		mult *= 10;
	else if (drone->monsterinfo.bonus_flags & BF_CHAMPION)
		mult *= 3.0;
	else if (drone->monsterinfo.bonus_flags & BF_BERSERKER)
		mult *= 1.5;
	else if (drone->monsterinfo.bonus_flags & BF_POSESSED)
		mult *= 4.0;

	if (!worldspawn)
	{
		float bonus = 0.1;
		//Talent: Corpulence (also in M_Initialize)
		if (pvm->value)
			bonus = 0.2;
        talentLevel = vrx_get_talent_level(ent, TALENT_CORPULENCE);
		if(talentLevel > 0)	mult +=	bonus * talentLevel;	//+10-20% per upgrade

		//if (pvm->value)
		//    mult += 0.3; // base mult for player monsters in pvm
	}

	drone->health *= mult;
	drone->max_health *= mult;
	drone->monsterinfo.power_armor_power *= mult;
	drone->monsterinfo.max_armor *= mult;

	if (worldspawn)
	{
		// non-invasion mode monsters or bosses are spawned randomly throughout the map
		if (!INVASION_OTHERSPAWNS_REMOVED || drone_type >= 30) // only use designated spawns in invasion mode
		{
			if (link_now && drone->mtype != M_JORG && !vrx_find_random_spawn_point(drone, false))
			{
				//gi.dprintf("vrx_create_drone_from_ent couldn't find a valid spawn point\n");
				G_FreeEdict(drone);
				return NULL; // couldn't find a spawn point
			}

			// gives players some time to react to newly spawned monsters
			//drone->nextthink = level.time + 1 + random();
			drone->monsterinfo.pausetime = level.time + 1.0;
			// az: remove invuln. in non-invasion
			// drone->monsterinfo.inv_framenum = level.framenum + (int)(1 / FRAMETIME);

			// trigger spree war if a boss successfully spawns in PvP mode
			if (deathmatch->value && !domination->value && !ctf->value && !invasion->value && !pvm->value
				&& !ptr->value && !ffa->value && (drone_type >= 30))
			{
				SPREE_WAR = true;
				SPREE_TIME = level.time;
				SPREE_DUDE = ent;//4.4
			}
		}
		//else
		drone->nextthink = level.time + 0.1;
	}
	else if (ent->client)
	{
		// 3.9 double monster cost if there are too many monsters in CTF
		if (ctf->value && (CTF_GetNumSummonable("drone", ent->teamnum) > MAX_MONSTERS))
			drone->monsterinfo.cost *= 2;

		// cost is doubled if you are a flyer or cacodemon below skill level 5
		if ((ent->mtype == MORPH_FLYER && ent->myskills.abilities[FLYER].current_level < 5)
			|| (ent->mtype == MORPH_CACODEMON && ent->myskills.abilities[CACODEMON].current_level < 5))
			drone->monsterinfo.cost *= 2;

		if (ent->client->pers.inventory[power_cube_index] < drone->monsterinfo.cost)
		{
			safe_cprintf(ent, PRINT_HIGH, "You need more power cubes to use this ability.\n");
			G_FreeEdict(drone);
			return NULL;
		}
		if (ent->num_monsters+drone->monsterinfo.control_cost > MAX_MONSTERS)
		{
			if (ent->num_monsters == MAX_MONSTERS)
				safe_cprintf(ent, PRINT_HIGH, "All available monster slots in use.\n");
			else
				safe_cprintf(ent, PRINT_HIGH, "Insufficient monster slots.\n");
			G_FreeEdict(drone);
			return NULL;
		}

		// get muzzle origin
		AngleVectors (ent->client->v_angle, forward, right, NULL);
		VectorSet(offset, 0, 7,  ent->viewheight-8);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
		// trace
		VectorMA(start, 96, forward, end);
		tr = gi.trace(start, NULL, NULL, end, ent, MASK_MONSTERSOLID);
		if (tr.fraction < 1)
		{
			if (tr.endpos[2] <= ent->s.origin[2])
				tr.endpos[2] += fabsf(drone->mins[2]); // spawned below us
			else
				tr.endpos[2] -= drone->maxs[2];	// spawned above
		}

		// is this a valid spot?
		tr = gi.trace(tr.endpos, drone->mins, drone->maxs, tr.endpos, NULL, MASK_MONSTERSOLID);
		if (tr.contents & MASK_MONSTERSOLID)
		{
			//gi.dprintf("invalid spot\n");
			G_FreeEdict(drone);
			return NULL;
		}

		VectorCopy(tr.endpos, drone->s.origin);
		drone->s.angles[YAW] = ent->s.angles[YAW];

		ent->client->ability_delay = level.time + drone->monsterinfo.control_cost * 2 * M_CONTROL_COST_SCALE;
		//ent->holdtime = level.time + 2*drone->monsterinfo.control_cost;
		ent->client->pers.inventory[power_cube_index] -= drone->monsterinfo.cost;
		drone->health = 0.5*drone->max_health;
		drone->monsterinfo.power_armor_power = 0.5*drone->monsterinfo.max_armor;
		drone->nextthink = level.time + 0.1;//2*drone->monsterinfo.control_cost;
		//drone->monsterinfo.upkeep_delay = drone->nextthink*10 + 10; // 3.2 upkeep begins 1 second after monster spawn

		layout_add_tracked_entity(&ent->client->layout, drone);
	}
	else
	{
		gi.dprintf("WARNING: vrx_create_new_drone() called without a valid spawner!\n");
		G_FreeEdict(drone);
		return NULL;
	}

	//4.4 FIXME: should we be doing this if ent is world?
	if (!ent->client && (drone_type == 30 || drone_type == 31)) // boss
		ent->num_sentries++;
	else
	{
		ent->num_monsters += drone->monsterinfo.control_cost;
	}
	
	// boss or not, add it to the monster count
	ent->num_monsters_real++;
	// gi.bprintf(PRINT_HIGH, "adding %p (%d)\n", drone, ent->num_monsters_real);

	VectorCopy (drone->s.origin, drone->s.old_origin);

	if (link_now) {
        gi.linkentity(drone);
    }

	// Addition to JABot
	AI_EnemyAdded(drone);
	DroneList_Insert(drone);

	return drone;
}

edict_t *vrx_create_new_drone(edict_t *ent, int drone_type, qboolean worldspawn, qboolean link_now, int bonus_level)
{
	return vrx_create_drone_from_ent(G_Spawn(), ent, drone_type, worldspawn, link_now, bonus_level);
}

void RemoveAllDrones (edict_t *ent, qboolean refund_player)
{
	edict_t *e=NULL;

	DroneRemoveSelected(ent, NULL);

	e = DroneList_Iterate();

	// search for other drones
	while (e)
	{
		// try to restore previous owner
		if (e && e->activator && (e->activator == ent) && !RestorePreviousOwner(e))
			M_Remove(e, refund_player, true);

		e = DroneList_Next(e);
	}
}

void RemoveDrone (edict_t *ent)
{
	vec3_t	forward, right, start, end, offset;
	trace_t	tr;
	edict_t* e = NULL;//, * n = NULL;

	//gi.dprintf("DroneList before:\n");
	//DroneList_Print(NULL, ent);

	// get muzzle origin
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 7,  ent->viewheight-8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	// trace
	VectorMA(start, 8192, forward, end);
	tr = gi.trace(start, NULL, NULL, end, ent, MASK_MONSTERSOLID);

	// are we pointing at a drone of ours?
	if (tr.ent && tr.ent->activator && (tr.ent->activator == ent)
		&& !strcmp(tr.ent->classname, "drone"))
	{
		// try to convert back to previous owner
		if (RestorePreviousOwner(tr.ent))
		{
			safe_cprintf(ent, PRINT_HIGH, "Conversion removed.\n");
			return;
		}

		safe_cprintf(ent, PRINT_HIGH, "Monster removed.\n");
		DroneRemoveSelected(ent, tr.ent);
		M_Remove(tr.ent, true, true);
		return;
	}

	// search for other drones

	e = DroneList_Iterate();

	while (e)
	{
		if (e && e->activator && (e->activator == ent))
		{
			// try to convert back to previous owner
			if (RestorePreviousOwner(e))
				continue;
			M_Remove(e, true, true);
		}
		e = DroneList_Next(e);
	}

	// clear all slots
	DroneRemoveSelected(ent, NULL);
	ent->num_monsters = 0;
	safe_cprintf(ent, PRINT_HIGH, "All monsters removed.\n");

	//gi.dprintf("DroneList after:\n");
	//DroneList_Print(NULL, ent);

}

void combatpoint_think (edict_t *self)
{
	edict_t		*e=NULL;
	qboolean	q=false;;

	// if the goal times-out, then reset all drones
	if (!G_EntExists(self->owner) || (level.time > self->delay)
		|| (self->owner->selectedsentry != self))
	{
		G_FreeEdict(self);
		return;
	}

	self->nextthink = level.time + 0.1;
}

edict_t *SpawnCombatPoint (edict_t *ent, vec3_t org)
{
	edict_t *goal;

	goal = G_Spawn();
	goal->think = combatpoint_think;
	goal->delay = level.time + 30;
	goal->owner = ent;
	goal->classname = "drone_goal";
//	goal->clipmask = MASK_MONSTERSOLID;
//	goal->movetype = MOVETYPE_NONE;
//	goal->takedamage = DAMAGE_NO;
//	goal->svflags |= SVF_NOCLIENT; // comment this for debugging
	goal->mtype = PLAYER_NAVI;
	goal->solid = SOLID_TRIGGER;
	goal->s.modelindex = gi.modelindex("models/items/c_head/tris.md2");
//	goal->s.effects |= EF_BLASTER;
	org[2] += 8;
	VectorCopy(org, goal->s.origin);
	goal->nextthink = level.time + 0.1;
	VectorSet (goal->mins, -8, -8, -8);
	VectorSet (goal->maxs, 8, 8, 8);
	gi.linkentity(goal);
//	ent->movetarget = goal; // pointer to goal
	return goal;
}

void DroneBlink (edict_t *ent)
{
	int	i;

	for (i=0; i<4; i++) {
		if (G_EntIsAlive(ent->selected[i]))
			ent->selected[i]->monsterinfo.selected_time = level.time + MONSTER_BLINK_DURATION;
	}
}

edict_t *SelectDrone (edict_t *ent)
{
	int		i=0;
	edict_t *e=NULL;

	while ((e = findclosestreticle(e, ent, 1024)) != NULL)
	{
		i++;
		if (i > 100000)
		{
			WriteServerMsg("SelectDrone() aborted infinite loop.", "ERROR", true, true);
			break;
		}

		if (!G_EntIsAlive(e))
			continue;
		if (!visible(ent, e))
			continue;
		if (!infront(ent, e))
			continue;
		if (!(e->svflags & SVF_MONSTER))
			continue;

		if (e->activator && (e->activator == ent))
			return e;
	}
	return NULL;
}

void DroneRemoveSelected (edict_t *ent, edict_t *drone)
{
	int i;

	for (i=0; i<4; i++)
	{
		if (drone)
		{
			// if this monster was previously selected, remove it from the list
			if (drone == ent->selected[i])
				ent->selected[i] = NULL;
		}
		else
			// remove all monsters from the list
			ent->selected[i] = NULL;
	}

}

// returns true if this is a monster that we summoned
qboolean ValidCommandMonster (edict_t *ent, edict_t *monster)
{
	return (isMonster(monster) && monster->activator
		&& monster->activator->inuse && monster->activator == ent);
}

edict_t **DroneAlreadySelected (edict_t *ent, edict_t *drone)
{
	int i;

	for (i=0; i<4; i++)
	{
		// is this drone already selected?
		if (drone == ent->selected[i])
			return &ent->selected[i];
	}

	return NULL;
}

edict_t **GetFreeSelectSlot (edict_t *ent)
{
	int i;

	for (i=0; i<4; i++)
	{
		// is this slot available?
		if (!G_EntIsAlive(ent->selected[i]) // freed or not alive
			|| !ValidCommandMonster(ent, ent->selected[i])) // not a monster that we own!
			return &ent->selected[i];
	}

	return NULL;
}

void DroneSelect (edict_t *ent)
{
	edict_t *drone;
	edict_t **slot;

	// are we pointing at a drone of ours?
	if ((drone = SelectDrone(ent)) != NULL)
	{
		if ((slot = DroneAlreadySelected(ent, drone)) != NULL)
		{
			safe_centerprintf(ent, "Drone standing down.\n");
			*slot = NULL;
			drone->monsterinfo.selected_time = 0;
			DroneBlink(ent); // all selected monsters blink
			return;
		}

		if ((slot = GetFreeSelectSlot(ent)) != NULL)
		{
			safe_centerprintf(ent, "Drone awaiting orders.\n");
			*slot = drone;
			DroneBlink(ent); // all selected monsters blink
			return;
		}

		// the queue is full, so bump one of our already selected monsters
		safe_centerprintf(ent, "Drone awaiting orders.\n");
		ent->selected[0] = drone;
		DroneBlink(ent);
	}
	else
	{
		safe_cprintf(ent, PRINT_HIGH, "You must be looking at a monster to select it.\n");
		safe_cprintf(ent, PRINT_HIGH, "Once selected, you can give a monster orders.\n");
	}
}

static edict_t *FindMoveTarget (edict_t *ent)
{
	int			i;
	edict_t		*e=NULL;
	qboolean	q=false;

	while ((e = findclosestreticle(e, ent, 1024)) != NULL)
	{
		// don't chase the owner
		if (e == ent)
			continue;

		if (!G_EntIsAlive(e))
			continue;

		for (i=0; i<4; i++)
		{
			// is this a selected drone?
			if (e == ent->selected[i])
			{
				q = true;
				break;
			}
		}

		if (q)
			continue;

		if (!visible(ent, e))
			continue;
		if (!infront(ent, e))
			continue;

		//gi.dprintf("found %s\n", e->classname);
		return e;
	}
	return NULL;
}

void DroneMove (edict_t *ent)
{
	int		i;
	vec3_t	forward, right, start, end, offset;
	trace_t	tr;
	edict_t *e, *target;

	// are we pointing at someone?
	if ((target = FindMoveTarget(ent)) != NULL)
	{
		if (OnSameTeam(target, ent))
		{
			if (target->client)
				safe_centerprintf(ent, "Drones will follow %s.\n", target->client->pers.netname);
			else
				safe_centerprintf(ent, "Drones will follow target.\n");
			for (i=0; i<4; i++) {
				if (G_EntIsAlive(ent->selected[i])
					&& !(ent->selected[i]->spawnflags & AI_STAND_GROUND))
				{
					ent->selected[i]->enemy = target;
					ent->selected[i]->monsterinfo.aiflags |= (AI_NO_CIRCLE_STRAFE|AI_COMBAT_POINT);
				}
			}
		}
		else
		{
			if (target->client)
				safe_centerprintf(ent, "Drones will attack %s.\n", target->client->pers.netname);
			else
				safe_centerprintf(ent, "Drones will attack target.\n");
			for (i=0; i<4; i++) {
				if (G_EntIsAlive(ent->selected[i])
					&& !(ent->selected[i]->spawnflags & AI_STAND_GROUND))
					ent->selected[i]->enemy = target;
			}
		}
		ent->selectedsentry = NULL; // we no longer need any combat point
		return;
	}

	// get muzzle origin
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 7,  ent->viewheight-8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	// trace
	VectorMA(start, 8192, forward, end);
	tr = gi.trace(start, NULL, NULL, end, ent, MASK_SHOT);

	// we're pointing at a spot
	if (ent->selectedsentry)
	{
		ent->selectedsentry = NULL; // reset the combat point
		return;
	}

	safe_centerprintf(ent, "Drones changing position.\n");
	e = SpawnCombatPoint(ent, tr.endpos);
	ent->selectedsentry = e;
	for (i=0; i<4; i++) {
		if (G_EntIsAlive(ent->selected[i]))
		{
			ent->selected[i]->monsterinfo.aiflags |= (AI_NO_CIRCLE_STRAFE|AI_COMBAT_POINT);
			ent->selected[i]->enemy = e;
		}
	}
}

/*
=============
infront

returns 1 if the entity is in front (in sight) of self
=============
*/
qboolean infront (edict_t *self, edict_t *other)
{
	vec3_t	vec;
	float	dot;
	vec3_t	forward;

	AngleVectors (self->s.angles, forward, NULL, NULL);
	VectorSubtract (other->s.origin, self->s.origin, vec);
	VectorNormalize (vec);
	dot = DotProduct (vec, forward);

	if (dot > 0.3)
		return true;

	return false;
}

qboolean infov (edict_t *self, edict_t *other, int degrees)
{
	vec3_t	vec;
	float	dot, value;
	vec3_t	forward;

	AngleVectors (self->s.angles, forward, NULL, NULL);
	VectorSubtract (other->s.origin, self->s.origin, vec);
	VectorNormalize (vec);
	dot = DotProduct (vec, forward);
	value = 1.0-(float)degrees/180.0;
	if (dot > value)
		return true;
	return false;
}

// return a random double in [0.0, 1.0)
double randfrac(void) {
	double res = (rand() % RAND_MAX) / (double)RAND_MAX;
	return res;
}

// note: flash_number is used by monsters to determine muzzle location; use -1 if muzzle location is already known, or 0 for non-monsters to estimate muzzle location
// aiming vector will be copied to 'forward' and can be used for firing functions
void MonsterAim (edict_t *self, float accuracy, int projectile_speed, qboolean rocket,
				 int flash_number, vec3_t forward, vec3_t start)
{
	float	velocity, dist, rnd, crnd;//, base_acc = accuracy;
	vec3_t	target, end;
	vec3_t	right, offset;
	trace_t	tr;

	// determine muzzle origin
	AngleVectors (self->s.angles, forward, right, NULL);
	if (self->client && !flash_number)
	{
		VectorSet(offset, 0, 8, self->viewheight-8);
		P_ProjectSource (self->client, self->s.origin, offset, forward, right, start);
	}
	else if (flash_number)
	{
		// -1 flash number indicates we've already calculated our muzzle location as 'start'
		// otherwise proceed using flash offset
		if (flash_number != -1)
		{
			// monsters have special offsets that determine their exact firing origin
			G_ProjectSource(self->s.origin, monster_flash_offset[flash_number], forward, right, start);
			// fix for retarded chick muzzle location
			if ((self->svflags & SVF_MONSTER) && (start[2] < self->absmin[2] + 32))
				start[2] += 32;
		}
	}
	else // can't determine the muzzle origin
	{
		// is viewheight set? if so, use that as a starting point
		if (self->viewheight)
		{
			VectorCopy(self->s.origin, start);
			start[2] += self->viewheight;
		}
		else // otherwise, use the mid-point of the bounding box
			G_EntMidPoint(self, start);
		// move starting point forward
		VectorMA(start, self->maxs[1]+8, forward, start);
	}

	// fire ahead if our enemy is invalid or out of our FOV
	if (!G_EntExists(self->enemy) || !nearfov(self, self->enemy, 0, 60))
	{
		AngleVectors(self->s.angles, forward, right, NULL);
		return;
	}

	// detected entities are easy targets
	if (self->enemy->flags & FL_DETECTED && self->enemy->detected_time > level.time)
		accuracy = -1;

	// get enemy distance and velocity
	dist = entdist(self, self->enemy);
	velocity = VectorLength(self->enemy->velocity);

	// accuracy modifications based on target's velocity, level, and bonus flags
	if (accuracy > 0)
	{
		// non-moving targets have a high minimum accuracy
		if (accuracy < 0.9 && velocity == 0)
			accuracy = 0.9;
		//base_acc = accuracy;//DEBUG

		//4.5 monster bonus flags
		if (self->monsterinfo.bonus_flags & BF_CHAMPION)
			accuracy *= 1.5;
		if (self->monsterinfo.bonus_flags & BF_BERSERKER)
			accuracy *= 2;
		if (self->monsterinfo.bonus_flags & BF_FANATICAL)
			accuracy *= 0.5;

		// try to determine if this is really a monster or some other non-player entity
		if (self->svflags & SVF_MONSTER && self->mtype && self->monsterinfo.sight_range && self->monsterinfo.level > 0)
		{
			float range = 2 * self->monsterinfo.sight_range;
			// reduce accuracy of low-level monsters
			
			float temp;// = self->monsterinfo.level / 10.0;
			/*
			if (temp < 0.5)
				temp = 0.5;
			else if (temp > 1.0)
				temp = 1.0;
			//gi.dprintf("low level mod: %f", temp);
			accuracy *= temp;*/

			// reduce accuracy of far away targets
			if (range < 2048)
				range = 2048;
			temp = 1 - (dist / range);
			if (temp < 0.5)
				temp = 0.5;
			else if (temp > 1.0)
				temp = 1.0;
			//gi.dprintf("far away mod: %f", temp);
			accuracy *= temp;
		}

		// reduce accuracy based on target velocity or superspeed
		if ((velocity = VectorLength(self->enemy->velocity)) > 0)
		{
			/*if (velocity <= 200) // walking speed
				accuracy *= 0.9;
			else */
			if (self->superspeed)
				accuracy *= 0.7;
			else if (velocity > 310)
				accuracy *= 0.8;
			else if (velocity >= 300) // running speed
				accuracy *= 0.9;
		}
	}

	// curse reduces accuracy of monsters dramatically
	if (accuracy > 0 && que_findtype(self->curses, NULL, CURSE))
		accuracy *= 0.2;

	G_EntMidPoint(self->enemy, target); // 3.58 aim at the ent's actual mid point
	//VectorCopy(self->enemy->s.origin, target);

	// miss the shot
	rnd = random();//GetRandom(1, 100) * 0.1;//(float)randfrac();
	crnd = crandom();
	if (crnd > 0)
		crnd = 1;
	else
		crnd = -1;
	//gi.dprintf("%s %.1f %.1f %.0f\n", GetMonsterKindString(self->mtype), base_acc, accuracy, velocity);
	if (accuracy >= 0 && rnd > accuracy)
	{
		/*
		VectorSubtract(start, target, forward);
		VectorNormalize(forward);
		vectoangles(forward, forward);
		AngleVectors(forward, NULL, right, NULL);
		*/
		//gi.dprintf("%d: miss the shot!! %f %f\n", (int)(level.framenum), accuracy, rnd);
		// aim above the bbox
		//target[2] += GetRandom(0, self->enemy->maxs[2]+16);
		if (random() > 0.5)
			target[2] = self->enemy->absmax[2] + fabs(self->enemy->maxs[2]);

		// aim a little to the left or right of the target
		// FIXME: if this doesn't miss reliably, we can 'lead' the target in the opposite direction
		VectorMA(target, ((2 * self->enemy->maxs[1])*crnd), right, target);

		VectorSubtract(target, start, forward);
		VectorNormalize(forward);
		return;
	}
	//gi.dprintf("%d: hit! %f %f\n", (int)(level.framenum), accuracy, rnd);

	// lead shots if our enemy is alive and not too close
	if ((self->enemy->health > 0) && (dist > 64) && (projectile_speed > 0))
	{
		// move our target point based on projectile and enemy velocity
		VectorMA(target, (float)dist/projectile_speed, self->enemy->velocity, end);

		tr = gi.trace(target, NULL, NULL, end, self->enemy, MASK_SOLID);
		VectorCopy(tr.endpos, target);

		if (rocket)
		{
			// if our enemy's feet are within 100 units of the ground
			// then we should aim at the ground and cause radius damage
			VectorCopy(target, end);
			end[2] = self->enemy->absmin[2]-100;
			// FIXME: we should probably be tracing without the verticle prediction
			tr = gi.trace(target, NULL, NULL, end, self->enemy, MASK_SOLID);
			// is there is ground below our target point ?
			if (tr.fraction < 1)
				VectorCopy(tr.endpos, target);
		}

		// if we dont have a clear shot to our predicted target
		if (!G_IsClearPath(self, MASK_SOLID, start, target))
			G_EntMidPoint(self->enemy, target); // 3.58
			//VectorCopy(self->enemy->s.origin, target); // fire directly at our enemy
	}

	VectorSubtract(target, start, forward);
	VectorNormalize (forward);
}

edict_t *M_MeleeAttack (edict_t *self, edict_t *targ, float range, int damage, int knockback)
{
	vec3_t	start, forward, end;
	trace_t	tr;

	if (!targ)
		return NULL;

	// miss the attack if we are cursed/confused
	if (que_typeexists(self->curses, CURSE) && rand() > 0.2)
		return NULL;

	self->lastsound = level.framenum;

	// get starting and ending positions
	G_EntMidPoint(self, start);
	G_EntMidPoint(targ, end);

	// get vector to target
	VectorSubtract(end, start, forward);
	VectorNormalize(forward);
	VectorMA(start, range, forward, end);

	tr = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);

	if (G_EntExists(tr.ent))
	{
		damage = vrx_increase_monster_damage_by_talent(self->activator, damage);
		T_Damage(tr.ent, self, self, forward, tr.endpos, tr.plane.normal, damage, knockback, 0, MOD_HIT);
		return tr.ent; // hit a damageable ent
	}

	return NULL;
}

qboolean M_NeedRegen (const edict_t *ent)
{
	if (!ent || !ent->inuse)
		return false;

	// if they are dead, they can't be regenerated
	if (ent->deadflag > 0)
		return false;

	// check for health
	if (ent->health < ent->max_health)
		return true;

	// check for armor
	if (ent->client)
	{
		// client check
		if (ent->client->pers.inventory[body_armor_index] < MAX_ARMOR(ent))
			return true;
	}
	else
	{
		// non-client check
		if (ent->monsterinfo.max_armor && (ent->monsterinfo.power_armor_power < ent->monsterinfo.max_armor))
			return true;
	}

	return false;
}

qboolean M_Regenerate (edict_t *self, int regen_frames, int delay, float mult, qboolean regen_health, qboolean regen_armor, qboolean regen_ammo, int *nextframe)
{
	int health, armor, ammo, max_health, max_armor;
	qboolean regenerate=false;

	if (!self || !self->inuse)
		return false;

	if (delay > 0)
	{
		if (level.framenum < *nextframe)
			return false;

		*nextframe = level.framenum + delay;
	}
	else
		delay = 1;

	max_health = self->max_health * mult;

	if (regen_health)
	{
		health = floattoint( (float)max_health / ((float)regen_frames / delay) );
		if (health < 1)
			health = 1;

		// heal them if they are weakened
		if (self->health < max_health)
		{
			int health_needed = max_health - self->health;
			if (health > health_needed)
				health = health_needed;
			self->health += health;
			//if (self->health > max_health)
			//	self->health = max_health;

			// switch to normal monster skin when it's healed
			if (!self->client && (self->svflags & SVF_MONSTER) && vrx_has_pain_skin(self) && (self->health >= 0.5 * self->max_health))
			{
				self->s.skinnum &= ~1;
				if (self->mtype != M_COMMANDER)
					self->s.skinnum &= ~2;
			}

			regenerate = true;
		}
	}

	if (regen_armor)
	{
		if (self->client)
		{
			max_armor = MAX_ARMOR(self) * mult;

			if (self->client->pers.inventory[body_armor_index] < max_armor)
			{
				int calc = 1;

				if( (regen_frames / delay) == 0 )
					calc = 1;
				else
					calc = (regen_frames / delay);

				armor = max_armor / calc;

				if (armor < 1)
					armor = 1;

				// repair player armor
				self->client->pers.inventory[body_armor_index] += armor;
				if (self->client->pers.inventory[body_armor_index] > max_armor)
					self->client->pers.inventory[body_armor_index] = max_armor;

				regenerate = true;
			}
		}

		if (self->monsterinfo.power_armor_type && self->monsterinfo.max_armor)
			max_armor = self->monsterinfo.max_armor * mult;
		else
			max_armor = 0;

		//gi.dprintf("type:%d amt:%d max:%d absmax:%d mult:%.1f\n", self->monsterinfo.power_armor_type,
		//	self->monsterinfo.power_armor_power, self->monsterinfo.max_armor, max_armor, mult);

		if (max_armor && self->monsterinfo.power_armor_power < max_armor)
		{
			int calc = 1;

			if( (regen_frames / delay) == 0 )
				calc = 1;
			else
				calc = (regen_frames / delay);

			armor = max_armor / calc;

			if (armor < 1)
				armor = 1;

			self->monsterinfo.power_armor_power += armor;
			if (self->monsterinfo.power_armor_power > max_armor)
				self->monsterinfo.power_armor_power = max_armor;

			regenerate = true;
		}
	}

	if (regen_ammo)
	{
		if (self->client)
		{
			int ammoIndex = G_GetAmmoIndexByWeaponIndex(G_GetRespawnWeaponIndex(self));
			int maxAmmo = MaxAmmoType(self, ammoIndex) * mult;

			if (AmmoLevel(self, ammoIndex) < mult)
			{
				ammo = maxAmmo / (regen_frames / delay);

				if (ammo < 1)
					ammo = 1;

				self->client->pers.inventory[ammoIndex] += ammo;
				if (self->client->pers.inventory[ammoIndex] > maxAmmo)
					self->client->pers.inventory[ammoIndex] = maxAmmo;

				regenerate = true;
			}
		}
	}

	return regenerate;
}

void M_Remove (edict_t *self, qboolean refund, qboolean effect)
{
	// monster is already being removed
	if (!self->inuse || (self->solid == SOLID_NOT))
		return;

	// additional steps must be taken if we have an owner/activator
	if (PM_MonsterHasPilot(self)) // for player-monsters
	{
		if (refund)
			self->owner->client->pers.inventory[power_cube_index]+=self->monsterinfo.cost;
	}
	else if (self->activator && self->activator->inuse
		&& !self->monsterinfo.slots_freed) // for all other monsters
	{

		if (self->activator->client)
		{

			// refund player's power cubes used if the monster has full health
			if (refund && !M_NeedRegen(self))
				self->activator->client->pers.inventory[power_cube_index]
				+= self->monsterinfo.cost;

			layout_remove_tracked_entity(&self->activator->client->layout, self);
		}
		else
		{
			// invasion defender spawns hold a pointer to this monster which must be cleared
			if (self->activator->enemy && (self->activator->enemy == self))
				self->activator->enemy = NULL;

			// reduce boss count
			if (self->monsterinfo.control_cost >= M_COMMANDER_CONTROL_COST)
				self->activator->num_sentries--;


			// end spree war if there are no more bosses remaining
			//if (SPREE_WAR && (SPREE_DUDE == world) && world->num_sentries < 1)
			if (SPREE_WAR && self->activator && !self->activator->client && self->activator->num_sentries < 1)//4.4
			{
				SPREE_WAR = false;
				SPREE_DUDE = NULL;
			}

			// is this an invasion boss?
			if (invasion->value && invasion_data.boss && invasion_data.boss == self)
			{
				invasion_data.boss = NULL;
			}
		}

		// reduce monster count
		self->activator->num_monsters -= self->monsterinfo.control_cost;
		if (self->activator->num_monsters < 0)
			self->activator->num_monsters = 0;
		if (self->mtype == M_SKELETON)
			self->activator->num_skeletons--;
		else if (self->mtype == M_GOLEM)
			self->activator->num_golems--;

	
		self->activator->num_monsters_real--;
		// gi.bprintf(PRINT_HIGH, "releasing %p (%d)\n", self, self->activator->num_monsters_real);

		if (self->activator->num_monsters_real < 0)
			self->activator->num_monsters_real = 0;

		// mark the player slots as being refunded, so it can't happen again
		self->monsterinfo.slots_freed = true;
	}

	DroneList_Remove(self);
	AI_EnemyRemoved(self);

	// mark this entity for removal
	if (effect)
		self->think = BecomeTE;
	else
		self->think = G_FreeEdict;

	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT; //4.0 to keep killbox() happy
	self->deadflag = DEAD_DEAD;
	self->nextthink = level.time + 0.1;
	gi.unlinkentity(self); //4.0 B15 another attempt to make killbox() happy

}

void M_PrepBodyRemoval (edict_t *self)
{
	self->think = M_BodyThink;
	self->delay = level.time + 30; // time until body is auto-removed
	self->activator = NULL; // no longer owned by anyone
	self->nextthink = level.time + 0.1;
}

void M_BodyThink (edict_t *self)
{
	// sanity check
	if (!self || !self->inuse)
		return;
	// already in the process of being removed
	if (self->solid == SOLID_NOT)
		return;

	// plague flies
	if (que_typeexists(self->curses, CURSE_PLAGUE))
		self->s.effects |= EF_FLIES;

	// body becomes transparent before removal
	if (level.time == self->delay-5.0)
		self->s.effects |= EF_PLASMA;
	else if (level.time == self->delay-2.0)
		self->s.effects |= EF_SPHERETRANS;

	// remove the body
	if (level.time >= self->delay)
	{
		M_Remove(self, false, false);
		return;
	}

	self->nextthink = level.time + 0.1;
}

qboolean M_Upkeep (edict_t *self, int delay, int upkeep_cost)
{
	int		*cubes;
	edict_t *e;

	if (self->mtype == M_DECOY)
		return true;

	if (delay > 0)
	{
		if (level.framenum < self->monsterinfo.upkeep_delay)
			return true;
		self->monsterinfo.upkeep_delay = level.framenum + delay;
	}
	else
		delay = 1;

	e = G_GetClient(self);

	if (!e)
		return true; // probably owned by world; doesn't pay upkeep

	cubes = &e->client->pers.inventory[power_cube_index];

	if (*cubes < upkeep_cost)
	{
		// owner can't pay upkeep, so we're dead :(
		safe_cprintf(self->activator, PRINT_HIGH, "Couldn't keep up the cost of %s - Removing!\n",
			GetMonsterKindString(self->mtype));
		//T_Damage(self, world, world, vec3_origin, self->s.origin, vec3_origin, 100000, 0, 0, 0);
		M_Remove(self, false, true);
		return false;
	}

	*cubes -= upkeep_cost;
	return true;
}

qboolean M_Initialize (edict_t *ent, edict_t *monster, float dur_bonus)
{
	int		talentLevel;
	// int 	talentLevel2;
	float	mult = 1.0;

	switch (monster->mtype)
	{
	case M_GUNNER: init_drone_gunner(monster); break;
	case M_CHICK: init_drone_bitch(monster); break;
	case M_BRAIN: init_drone_brain(monster); break;
	case M_MEDIC: init_drone_medic(monster); break;
	case M_MUTANT: init_drone_mutant(monster); break;
	case M_PARASITE: init_drone_parasite(monster); break;
	case M_TANK: init_drone_tank(monster); break;
	case M_BERSERK: init_drone_berserk(monster); break;
	case M_SOLDIER: case M_SOLDIERLT: case M_SOLDIERSS: init_drone_soldier(monster); break;
	case M_GLADIATOR: init_drone_gladiator(monster); break;
	case M_INFANTRY: init_drone_infantry(monster); break;
	case M_FLYER: init_drone_flyer(monster); break;
	case M_FLOATER: init_drone_floater(monster); break;
	case M_HOVER: init_drone_hover(monster); break;
	case M_SHAMBLER: init_drone_shambler(monster); break;
	case M_SKELETON: init_skeleton(monster); break;
	case M_GOLEM: init_golem(monster); break;
	default: return false;
	}

	if ( (ent && ent->inuse && ent->client) ||
		(monster->activator && monster->activator->inuse && monster->activator->client) ) // player summons exception.
	{
		float bonus = 0.1;
		//Talent: Corpulence
		if (pvm->value)
			bonus = 0.2;
		talentLevel = vrx_get_talent_level(ent, TALENT_CORPULENCE);
		if (talentLevel > 0)	mult += bonus * talentLevel;	//+10-20% per upgrade

		mult += dur_bonus; // caller defined monster multiplier.
	}

	//gi.dprintf("talentLevel: %d multiplier: %.1f durability bonus = %.1f\n", talentLevel, mult, dur_bonus);
	monster->health *= mult;
	monster->max_health *= mult;
	monster->monsterinfo.power_armor_power *= mult;
	monster->monsterinfo.max_armor *= mult;

	// set shared monster properties
	monster->classname = "drone";
	monster->svflags |= SVF_MONSTER;
	monster->svflags &= ~SVF_DEADMONSTER;
	monster->yaw_speed = 20;
	monster->monsterinfo.sight_range = 1024;
	// No one really liked chasing monsters
	// monster->flags |= FL_CHASEABLE;
	monster->deadflag = DEAD_NO;
	monster->movetype = MOVETYPE_STEP;
	monster->solid = SOLID_BBOX;
	monster->clipmask = MASK_MONSTERSOLID;
	monster->takedamage = DAMAGE_AIM;
	monster->s.renderfx |= RF_FRAMELERP|RF_IR_VISIBLE;
	monster->enemy = NULL;
	monster->oldenemy = NULL;
	monster->goalentity = NULL;
	monster->monsterinfo.aiflags &= ~AI_COMBAT_POINT;
	monster->monsterinfo.bonus_flags = 0;//4.5 reset monster bonus flags

	// set shared monster functions
	if (monster->pain && monster->pain != drone_pain)
		monster->pain_inner = monster->pain;
	else
		monster->pain_inner = NULL; // maybe redundant?

	monster->pain = drone_pain;
	monster->touch = drone_touch;
	monster->think = drone_think;
	DroneList_Insert(monster);
	AI_EnemyAdded(monster);
	if (ent->client)
		layout_add_tracked_entity(&ent->client->layout, monster);
		
	return true;
}

qboolean M_SetBoundingBox (int mtype, vec3_t boxmin, vec3_t boxmax)
{
	switch (mtype)
	{
	case M_SOLDIER:
	case M_SOLDIERLT:
	case M_SOLDIERSS:
	case M_GUNNER:
	case M_BRAIN:
		VectorSet (boxmin, -16, -16, -24);
		VectorSet (boxmax, 16, 16, 32);
		break;
	case M_PARASITE:
		VectorSet (boxmin, -16, -16, -24);
		VectorSet (boxmax, 16, 16, 0);
		break;
	case M_CHICK:
		VectorSet (boxmin, -16, -16, 0);
		VectorSet (boxmax, 16, 16, 56);
		break;
	case M_TANK:
		VectorSet (boxmin, -24, -24, -16);
		VectorSet (boxmax, 24, 24, 64);
		break;
	case M_SHAMBLER:
		VectorSet(boxmin, -32, -32, -24);
		VectorSet(boxmax, 32, 32, 64);
		break;
	case M_MEDIC:
	case M_MUTANT:
		VectorSet (boxmin, -24, -24, -24);
		VectorSet (boxmax, 24, 24, 32);
		break;
	case M_BERSERK: // az: these were missing...
		VectorSet(boxmin, -16, -16, -24);
		VectorSet(boxmax, 16, 16, -8);
		break;
	case M_GLADIATOR:
		VectorSet(boxmin, -24, -24, -24);
		VectorSet(boxmax, 24, 24, 48);
		break;
	case M_INFANTRY:
		VectorSet(boxmin, -16, -16, -24);
		VectorSet(boxmax, 16, 16, 32);
		break;
	default:
		//gi.dprintf("failed to set bbox\n");
		return false;
	}

	//gi.dprintf("set bounding box for mtype %d\n", mtype);
	return true;
}

char *GetMonsterKindString (int mtype)
{
    switch (mtype)
    {
        case M_BRAIN: return "Brain";
        case M_CHICK: return "Praetor";
        case M_MEDIC: return "Medic";
        case M_MUTANT: return "Mutant";
        case M_PARASITE: return "Parasite";
		case M_BERSERK: return "Berserker";
		case M_SOLDIER: case M_SOLDIERLT: case M_SOLDIERSS: return "Soldier";
        case M_TANK: return "Tank";
        case M_GUNNER: return "Gunner";
		case M_YANGSPIRIT: return "Yang Spirit";
		case M_BALANCESPIRIT: return "Balance Spirit";
		case BOSS_TANK:
		case M_COMMANDER:
			return "Tank Commander";
		case BOSS_MAKRON:
		case M_MAKRON:
			return "Makron";
		case M_JORG: return "Jorg";
		case P_TANK: return "Tank";
		case M_SKULL: return "Hellspawn";
		case M_SPIKER: return "Spiker";
		case M_GASSER: return "Gasser";
		case M_SPIKEBALL: return "Spore";
		case M_OBSTACLE: return "Obstacle";
		case M_BOX: return "Box";
		case M_COCOON: return "Cocoon";
		case M_HEALER: return "Healer";
		case TOTEM_FIRE: return "Fire Totem";
		case TOTEM_WATER: return "Water Totem";
		case M_ALARM: return "Laser Trap";
		case M_GLADIATOR: return "Gladiator";
		case M_INFANTRY: return "Enforcer";
		case M_FLYER: return "Flyer";
		case M_FLOATER: return "Floater";
		case M_HOVER: return "Hover";
		case M_SHAMBLER: return "Shambler";
		case M_SKELETON: return "Skeleton";
		case M_GOLEM: return "Golem";
		case M_BARON_FIRE: return "Fire Baron";
        default: return "Monster";
    }
}

// NOTE: M_Notify only works for player-spawn monsters--don't expect it to do anything for worldspawned monsters!
void M_Notify (edict_t *monster)
{
	int count, max;

	if (!monster || !monster->inuse)
		return;
	if (!monster->activator || !monster->activator->inuse || !monster->activator->client) // only works for player-spawned monsters
		return;

	// don't call this more than once
	if (monster->monsterinfo.slots_freed)
		return;

	monster->activator->num_monsters -= monster->monsterinfo.control_cost;
	monster->activator->num_monsters_real--;
	// gi.bprintf(PRINT_HIGH, "releasing %p (%d)\n", monster, monster->activator->num_monsters_real);

	if (monster->activator->num_monsters < 0)
		monster->activator->num_monsters = 0;

	if (monster->mtype == M_SKELETON)
	{
		monster->activator->num_skeletons--;
		count = monster->activator->num_skeletons;
		max = SKELETON_MAX;
	}
	else if (monster->mtype == M_GOLEM)
	{
		monster->activator->num_golems--;
		count = monster->activator->num_golems;
		max = GOLEM_MAX;
	}
	else
	{
		count = monster->activator->num_monsters;
		max = MAX_MONSTERS;
	}

	safe_cprintf(monster->activator, PRINT_HIGH, "You lost a %s! (%d/%d)\n", GetMonsterKindString(monster->mtype), count, max);

	monster->monsterinfo.slots_freed = true;

	DroneList_Remove(monster);
	AI_EnemyRemoved(monster);
	DroneRemoveSelected(monster->activator, monster);//4.2 remove from selection list
}

void DroneAttack (edict_t *ent, edict_t *other)
{
	int		i;
	edict_t *e;

	safe_centerprintf(ent, "Monsters will attack target.\n");

	for (i = 0; i < 4; i++)
	{
		e = ent->selected[i];
		if (G_EntIsAlive(e) && ValidCommandMonster(ent, e) && visible(ent, e))
		{
			e->enemy = other;
			//e->goalentity = other;
			VectorCopy(other->s.origin, e->monsterinfo.last_sighting);
			e->monsterinfo.run(e);
			e->monsterinfo.selected_time = level.time + 1.0;// blink briefly
		}
	}
}

void DroneFollow (edict_t *ent, edict_t *other)
{
	int		i;
	edict_t *e;

	safe_centerprintf(ent, "Monsters will follow target.\n");

	for (i = 0; i < 4; i++)
	{
		e = ent->selected[i];
		if (G_EntIsAlive(e) && ValidCommandMonster(ent, e) && visible(ent, e))
		{
			e->monsterinfo.aiflags |= AI_NO_CIRCLE_STRAFE;
			e->monsterinfo.aiflags &= ~AI_STAND_GROUND;
			e->enemy = NULL;
			e->goalentity = other;
			VectorCopy(other->s.origin, e->monsterinfo.last_sighting);
			e->monsterinfo.run(e);
			e->monsterinfo.leader = other;
			e->monsterinfo.selected_time = level.time + MONSTER_BLINK_DURATION;// blink briefly
		}
	}
}

int numDroneLinks (edict_t *self)
{
	int		i, numLinks;
	edict_t *e;
	//vec3_t	pos;

	for (i = numLinks = 0; i < 4; i++)
	{
		// we iterate through each monster/drone that the client has selected
		e = self->activator->selected[i];
		// monster/drone must be alive
		if (!G_EntIsAlive(e))
			continue;
		// must be a valid monster that our activator owns
		if (!ValidCommandMonster(self->activator, e))
			continue;
		// monster/drone has a matching vector to our location
		//FIXME: may want to add a timeout/lost frame counter to this, as monsters might get lost/distracted and forget about the combat point
		if (VectorCompare(e->monsterinfo.spot1, self->s.origin) || VectorCompare(e->monsterinfo.spot2, self->s.origin))
			numLinks++;
	}

	return numLinks;
}

void drone_tempent_think (edict_t *self)
{
	// if no monsters are following this combat point, then free it
	if (!G_EntIsAlive(self->activator) || numDroneLinks(self) < 1)
	{
		//gi.dprintf("freed combat point %d\n", numDroneLinks(self));
		G_FreeEdict(self);
		return;
	}

	self->nextthink = level.time + 0.1;
}

edict_t *DroneTempEnt (edict_t *ent, vec3_t pos, float delay)
{
	//FIXME: entity needs to have a think function that checks for owner death
	edict_t *e = G_Spawn();
	VectorCopy(pos, e->s.origin);
	e->activator = ent;
	e->classname = "combat point";
	e->mtype = M_COMBAT_POINT;
	e->think = drone_tempent_think;
	e->nextthink = level.time + 0.1;
	if (delay)
	{
		e->nextthink = level.time + delay;
		e->think = G_FreeEdict;
	}
	gi.linkentity(e);
	return e;
}

void DroneMovePosition (edict_t *ent, vec3_t pos)
{
	int		i, cmd;
	edict_t *e, *temp=NULL;

	// FIXME: this only allows spawner to have one set of combat points at a time
	// remove any existing combat points that we own
	while ((temp = G_FindEntityByMtype(M_COMBAT_POINT, temp)) != NULL)
	{
		if (temp->activator != ent)
			continue;
		G_FreeEdict(temp);
	}

	// create entity that monsters will follow
	temp = DroneTempEnt(ent, pos, 0); // entity has no time-out delay

	// has the player issued a command very recently?
	if (ent->client->lastCommand > level.time)
	{
		// two different locations selected?
		if (Get2dDistance(ent->client->lastPosition, pos) > 64)
		{
			safe_centerprintf(ent, "Monsters will patrol.\n");
			cmd = 1; // patrol between two points
		}
		else
		{
			safe_centerprintf(ent, "Monsters will defend position.\n");
			cmd = 2; // defend/stay in this spot
		}
	}
	else
	{
		safe_centerprintf(ent, "Monsters will move to spot.\n");
		cmd = 3; // move to this spot
	}

	// search selected monsters
	for (i = 0; i < 4; i++)
	{
		e = ent->selected[i];

		// is this a valid monster in visible range?
		if (G_EntIsAlive(e) && ValidCommandMonster(ent, e) && visible(ent, e))
		{
			e->monsterinfo.aiflags |= (AI_NO_CIRCLE_STRAFE|AI_COMBAT_POINT);
			e->monsterinfo.aiflags &= ~AI_STAND_GROUND;
			e->enemy = NULL;
			e->goalentity = temp;
			VectorClear(e->monsterinfo.spot1);
			VectorClear(e->monsterinfo.spot2);

			// patrol between two points
			if (cmd == 1)
			{
				VectorCopy(ent->client->lastPosition, e->monsterinfo.spot1);
				VectorCopy(pos, e->monsterinfo.spot2);
				e->monsterinfo.leader = temp;
				// create a combat point at the first spot
				DroneTempEnt(ent, ent->client->lastPosition, 0);
			}
			// return to this spot, even if we get distracted
			else if (cmd == 2)
			{
				VectorCopy(pos, e->monsterinfo.spot1);
				e->monsterinfo.leader = temp;
			}
			// move to this spot, but we may get distracted
			else
			{
				VectorCopy(pos, e->monsterinfo.spot1);
				e->monsterinfo.leader = NULL;
			}

			VectorCopy(pos, e->monsterinfo.last_sighting);
			e->monsterinfo.run(e);
			e->monsterinfo.selected_time = level.time + MONSTER_BLINK_DURATION;// blink briefly
		}
	}
}

void DroneToggleStand (edict_t *ent, edict_t *monster)
{
	if (monster->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		safe_centerprintf(ent, "Monster will hunt.\n");

		// if we have no melee function, then make sure we can circle strafe
		if (!monster->monsterinfo.melee)
			monster->monsterinfo.aiflags  &= ~AI_NO_CIRCLE_STRAFE;

		monster->monsterinfo.aiflags &= ~(AI_COMBAT_POINT|AI_STAND_GROUND);
		monster->monsterinfo.sight_range = 1024;
		monster->monsterinfo.leader = NULL;
		VectorClear(monster->monsterinfo.spot1);
		VectorClear(monster->monsterinfo.spot2);
		monster->yaw_speed = 20;
	}
	else
	{
		safe_centerprintf(ent, "Monster will stand ground.\n");

		monster->monsterinfo.aiflags |= AI_STAND_GROUND;
		monster->monsterinfo.leader = NULL;
		monster->monsterinfo.aiflags &= ~AI_COMBAT_POINT;
		VectorClear(monster->monsterinfo.spot1);
		VectorClear(monster->monsterinfo.spot2);
		monster->yaw_speed = 40; // faster turn rate while standing ground

		// some drones have limited sight range while standing ground
		if (monster->mtype == M_BRAIN)
			monster->monsterinfo.sight_range = 256;
		if (monster->mtype == M_PARASITE)
			monster->monsterinfo.sight_range = 128;
		if (monster->mtype == M_MEDIC)
			monster->monsterinfo.sight_range = 256;
		if (monster->mtype == M_BERSERK)
			monster->monsterinfo.sight_range = 128;

		if (monster->monsterinfo.stand)
			monster->monsterinfo.stand(monster);
	}
}

void MonsterToggleShowpath (edict_t *ent)
{
	vec3_t	forward, right, start, offset, end;
	trace_t tr;

	if (!ent->myskills.administrator)
		return;

	// calculate starting position for trace
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 7,  ent->viewheight-8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	// calculate end position
	VectorMA(start, 8192, forward, end);
	// run trace
	tr = gi.trace(start, NULL, NULL, end, ent, MASK_SHOT);

	if (G_EntIsAlive(tr.ent) && isMonster(tr.ent))
	{
		if (tr.ent->showPathDebug)
		{
			safe_centerprintf(ent, "Show path OFF\n");
			tr.ent->showPathDebug = 0;
		}
		else
		{
			if (tr.ent->monsterinfo.aiflags & AI_FIND_NAVI)
				safe_centerprintf(ent, "Show path ON. Monster is searching for navis.\n");
			else
				safe_centerprintf(ent, "Show path ON\n");
			tr.ent->showPathDebug = 1;
		}
	}
}

void MonsterCommand (edict_t *ent)
{
	vec3_t	forward, right, start, offset, end;
	trace_t tr;
	edict_t **slot;

	// calculate starting position for trace
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 7,  ent->viewheight-8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	// calculate end position
	VectorMA(start, 8192, forward, end);
	// run trace
	tr = gi.trace(start, NULL, NULL, end, ent, MASK_SHOT);

	// is this a live entity?
	if (G_EntIsAlive(tr.ent))
	{
		// is this is a monster that we own?
		if (ValidCommandMonster(ent, tr.ent))
		{
			// was this monster already selected?
			if ((slot = DroneAlreadySelected(ent, tr.ent)) != NULL)
			{
				// did we last select this monster very recently ('double-click')?
				if (ent->client->lastCommand > level.time
					&& ent->client->lastEnt == tr.ent)
				{
					// toggle stand ground
					DroneToggleStand(ent, tr.ent);
					tr.ent->monsterinfo.selected_time = level.time + MONSTER_BLINK_DURATION;
				}
				else
				{
					// un-select the monster
					*slot = NULL;
					tr.ent->monsterinfo.selected_time = 0;
					DroneBlink(ent);// all selected monsters blink
				}
			}
			// if the monster has not already been selected
			// then try to add it to the list of currently selected monsters
			else if ((slot = GetFreeSelectSlot(ent)) != NULL)
			{
				*slot = tr.ent;
				DroneBlink(ent);// all selected monsters blink
			}

		}
		// are we on the same team?
		else if (OnSameTeam(ent, tr.ent))
			DroneFollow(ent, tr.ent);
		else
			DroneAttack(ent, tr.ent);
	}
	else
	// move/defend position
	{
		// trace to the floor
		VectorCopy(tr.endpos, start);
		VectorCopy(start, end);
		end[2] -= 8192;
		tr = gi.trace(start, NULL, NULL, end, NULL, MASK_SOLID);

		tr.endpos[2] += 8;
		DroneMovePosition(ent, tr.endpos);
		VectorCopy(tr.endpos, ent->client->lastPosition);
	}

	// update last command timer (used for 'double-click' commands)
	ent->client->lastCommand = level.time + 2.0;
	if (tr.ent)
		ent->client->lastEnt = tr.ent;
	else
		ent->client->lastEnt = NULL;
}

void MonsterFollowMe (edict_t *ent)
{
	int		i;
	edict_t *e;

	safe_centerprintf(ent, "Monsters will follow you.\n");

	// search selected monsters
	for (i = 0; i < 3; i++)
	{
		e = ent->selected[i];

		// is this a valid monster in visible range?
		if (G_EntIsAlive(e) && ValidCommandMonster(ent, e) && visible(ent, e))
		{
			e->monsterinfo.aiflags |= (AI_NO_CIRCLE_STRAFE|AI_COMBAT_POINT);
			e->monsterinfo.aiflags &= ~AI_STAND_GROUND;
			e->enemy = NULL;
			e->goalentity = ent;
			e->monsterinfo.leader = ent;
			VectorClear(e->monsterinfo.spot1);
			VectorClear(e->monsterinfo.spot2);
			VectorCopy(ent->s.origin, e->monsterinfo.last_sighting);
			if (entdist(ent, e) > 512)
				e->monsterinfo.run(e);
			e->monsterinfo.selected_time = level.time + MONSTER_BLINK_DURATION; // blink
		}
	}
}

edict_t *CTF_GetFlagBaseEnt (int teamnum);
void MonsterAttack (edict_t *ent)
{
	int		i;
	edict_t *e, *goal=NULL;

	if (ctf->value)
	{
		if (ent->teamnum == 1)
			goal = CTF_GetFlagBaseEnt(2);
		else
			goal = CTF_GetFlagBaseEnt(1);
		safe_centerprintf(ent, "Monsters will attack enemy base.\n");
	}
	// FIXME: this doesn't work if monster spawns are spread around
	else if (invasion->value)
	{
		goal = G_FindEntityByMtype(INVASION_MONSTERSPAWN, goal);
		safe_centerprintf(ent, "Monsters will attack monster base.\n");
	}
	else
		return;

	if (goal)
		goal = DroneTempEnt(ent, goal->s.origin, 0);

	// search queue for drones
	for (i=0; i<3; i++)
	{
		e = ent->selected[i];
		// is this a live, visible monster that we own?
		if (G_EntIsAlive(e) && ValidCommandMonster(ent, e) && visible(ent, e))
		{
			e->monsterinfo.aiflags |= (AI_NO_CIRCLE_STRAFE|AI_COMBAT_POINT);
			e->monsterinfo.aiflags &= ~AI_STAND_GROUND;
			e->monsterinfo.leader = goal;
			e->goalentity = goal;
			VectorCopy(goal->s.origin, e->monsterinfo.last_sighting);
			VectorCopy(goal->s.origin, e->monsterinfo.spot1);
			VectorClear(e->monsterinfo.spot2);
			if (entdist(e, goal) > 512)
				e->monsterinfo.run(e);
			e->monsterinfo.selected_time = level.time + MONSTER_BLINK_DURATION;
		}
	}

	ent->client->lastCommand = level.time + 2.0;
}

void Cmd_Drone_f (edict_t *ent)
{
	int		i=0;
	char	*s;
	edict_t	*e=NULL;

	if (debuginfo->value)
		gi.dprintf("DEBUG: %s just called Cmd_Drone_f()\n", ent->client->pers.netname);

	if (ent->deadflag == DEAD_DEAD)
		return;

	s = gi.args();

	if (!Q_strcasecmp(s, "remove"))
	{
		RemoveDrone(ent);
		return;
	}

	if (!Q_strcasecmp(s, "command"))
	{
		MonsterCommand(ent);
		return;
	}

	if (!Q_strcasecmp(s, "follow me"))
	{
		MonsterFollowMe(ent);
		return;
	}

	if (!Q_strcasecmp(s, "showpath"))
	{
		MonsterToggleShowpath(ent);
		return;
	}

	if (!Q_strcasecmp(s, "attack"))
	{
		MonsterAttack(ent);
		return;
	}
/*
	if (!Q_strcasecmp(s, "stand"))
	{
		safe_centerprintf(ent, "Drones will hold position.\n");
		for (i=0; i<3; i++) {
			// search queue for drones
			if (G_EntIsAlive(ent->selected[i]))
			{
				ent->selected[i]->monsterinfo.aiflags |= AI_STAND_GROUND;
				ent->selected[i]->yaw_speed = 40; // faster turn rate while standing ground
				// some drones have limited sight range while standing ground
				if (ent->selected[i]->mtype == M_BRAIN)
					ent->selected[i]->monsterinfo.sight_range = 256;
				if (ent->selected[i]->mtype == M_PARASITE)
					ent->selected[i]->monsterinfo.sight_range = 128;
				if (ent->selected[i]->mtype == M_MEDIC)
					ent->selected[i]->monsterinfo.sight_range = 256;
				if (ent->selected[i]->mtype == M_BERSERK)
					ent->selected[i]->monsterinfo.sight_range = 128;
				if (ent->selected[i]->monsterinfo.stand)//don't crash, this list might have non-monsters
					ent->selected[i]->monsterinfo.stand(ent->selected[i]);
			}
		}
		return;
	}*/

	if (!Q_strcasecmp(s, "count"))
	{
		safe_centerprintf(ent, "You have %d drones.\n%d/%d slots used.", ent->num_monsters_real, ent->num_monsters, (int)MAX_MONSTERS);
		return;
	}
/*
	if (!Q_strcasecmp(s, "hunt"))
	{
		safe_centerprintf(ent, "Drones will hunt.\n");
		for (i=0; i<3; i++) {
			// search queue for drones
			if (G_EntIsAlive(ent->selected[i]))
			{
				ent->selected[i]->monsterinfo.aiflags &= ~AI_STAND_GROUND;
				ent->selected[i]->monsterinfo.sight_range = 1024; // reset back to default
				ent->selected[i]->yaw_speed = 40; // reset back to default
			}
		}
		return;
	}

	if (!Q_strcasecmp(s, "select"))
	{
		DroneSelect(ent);
		return;
	}

	if (!Q_strcasecmp(s, "move"))
	{
		DroneMove(ent);
		return;
	}*/

	if (!Q_strcasecmp(s, "help"))
	{
		safe_cprintf(ent, PRINT_HIGH, "Monster summoning:\n");
		safe_cprintf(ent, PRINT_HIGH, "monster [gunner|parasite|brain|praetor|medic|tank|mutant|gladiator|berserker|soldier|enforcer|flyer|floater|hover|shambler\n");
		safe_cprintf(ent, PRINT_HIGH, "Monster utility commands:\n");
		safe_cprintf(ent, PRINT_HIGH, "monster [remove|command|follow me|count|attack]\n");
		return;
	}

	if (ent->myskills.abilities[MONSTER_SUMMON].disable)
		return;

	if (!G_CanUseAbilities(ent, ent->myskills.abilities[MONSTER_SUMMON].current_level, 0))
		return;

	if (ctf->value && (CTF_DistanceFromBase(ent, NULL, CTF_GetEnemyTeam(ent->teamnum)) < CTF_BASE_DEFEND_RANGE))
	{
		safe_cprintf(ent, PRINT_HIGH, "Can't build in enemy base!\n");
		return;
	}

	if (!Q_strcasecmp(s, "gunner"))
        vrx_create_new_drone(ent, 1, false, true, 0);
	else if (!Q_strcasecmp(s, "parasite"))
        vrx_create_new_drone(ent, 2, false, true, 0);
	else if (!Q_strcasecmp(s, "brain"))
        vrx_create_new_drone(ent, 4, false, true, 0);
	else if (!Q_strcasecmp(s, "praetor"))
        vrx_create_new_drone(ent, 3, false, true, 0);
	else if (!Q_strcasecmp(s, "medic"))
        vrx_create_new_drone(ent, 5, false, true, 0);
	else if (!Q_strcasecmp(s, "tank"))
        vrx_create_new_drone(ent, 6, false, true, 0);
	else if (!Q_strcasecmp(s, "mutant"))
        vrx_create_new_drone(ent, 7, false, true, 0);
	else if (!Q_strcasecmp(s, "gladiator")/* && ent->myskills.administrator*/)
        vrx_create_new_drone(ent, 8, false, true, 0);
	else if (!Q_strcasecmp(s, "berserker"))
        vrx_create_new_drone(ent, 9, false, true, 0);
	else if (!Q_strcasecmp(s, "soldier"))
        vrx_create_new_drone(ent, 10, false, true, 0);
	else if (!Q_strcasecmp(s, "enforcer"))
        vrx_create_new_drone(ent, 11, false, true, 0);
	else if (!Q_strcasecmp(s, "flyer"))
		vrx_create_new_drone(ent, 12, false, true, 0);
	else if (!Q_strcasecmp(s, "floater"))
		vrx_create_new_drone(ent, 13, false, true, 0);
	else if (!Q_strcasecmp(s, "hover"))
		vrx_create_new_drone(ent, 14, false, true, 0);
	else if (!Q_strcasecmp(s, "shambler"))
		vrx_create_new_drone(ent, 15, false, true, 0);
	else if (!Q_strcasecmp(s, "golem") && ent->myskills.administrator)
		vrx_create_new_drone(ent, 22, false, true, 0);
	//else if (!Q_strcasecmp(s, "baron fire") && ent->myskills.administrator)
		//vrx_create_new_drone(ent, 32, false, true);
	//else if (!Q_strcasecmp(s, "jorg"))
    //    vrx_create_new_drone(ent, 32, false, true);
	else 


		//case 1: init_drone_gunner(drone);		break;
		//case 2: init_drone_parasite(drone);		break;
		//case 3: init_drone_bitch(drone);		break;
		//case 4: init_drone_brain(drone);		break;
		//case 5: init_drone_medic(drone);		break;
		//case 6: init_drone_tank(drone);			break;
		//case 7: init_drone_mutant(drone);		break;
		//case 8: init_drone_gladiator(drone);	break;
		//case 9: init_drone_berserk(drone);		break;
		//case 10: init_drone_soldier(drone);		break;
		//case 11: init_drone_infantry(drone);	break;
		//case 20: init_drone_decoy(drone);		break;
		//case 30: init_drone_commander(drone);	break;
		//case 31: init_drone_supertank(drone);	break;
		//case 32: init_drone_jorg(drone);		break;
		//case 33: init_drone_makron(drone);		break;


		safe_cprintf(ent, PRINT_HIGH, "Additional parameters required.\nType 'monster help' for a list of commands.\n");

}

//==========================================
// M_DelayNextAttack
// delays the next monster attack based on specified parameters
// add_attack_frames - delays the next attack by the number of frames remaining in currentmove + delay
// delay - the amount of time to delay the next attack + remaining frames (if add_attack_frames is true)
//==========================================
void M_DelayNextAttack(edict_t* self, float delay, qboolean add_attack_frames)
{
	if (add_attack_frames)
	{
		int startframe;
		mmove_t* move = self->monsterinfo.currentmove;

		// if we haven't begun this move yet or we are at the tail-end of
		// an attack (re-attack) then start at the first frame
		if (self->s.frame < move->firstframe || self->s.frame >= move->lastframe)
			startframe = move->firstframe;
		else
			// otherwise, start at the current frame
			startframe = self->s.frame;

		// attack is delayed until the current move frames are finished + delay (if any)
		self->monsterinfo.attack_finished = level.time
			+ ((move->lastframe - startframe) * FRAMETIME) + delay;
	}
	else if (delay)
	{
		// attack is delayed until after delay
		self->monsterinfo.attack_finished = level.time + delay;
	}

	//gi.dprintf("%s attack delayed for %.1f second(s) until %.1f\n", 
	//	GetMonsterKindString(self->mtype), 
	//	(self->monsterinfo.attack_finished - level.time), self->monsterinfo.attack_finished);
}

qboolean M_ContinueAttack(edict_t* self, mmove_t* attack_move, mmove_t* end_move,
	float min_dist, float max_dist, float chance)
{
	float dist = 9999;

	// do we have an attack move to continue?
	if (attack_move)
	{
		// is the target still valid?
		if (G_ValidTarget(self, self->enemy, true, true))
		{
			dist = entdist(self, self->enemy);

			// check valid range and chance conditions
			if (random() <= chance && dist <= max_dist && dist >= min_dist)
			{
				// continue attack move
				self->monsterinfo.currentmove = attack_move;

				// delay attack function call until this one finishes
				M_DelayNextAttack(self, 0, true);
				return true;
			}
		}
		/*
		else
		{
			// target is invalid, clear it
			self->enemy = NULL;
		}
		*/
	}

	// do we have an end move?
	if (end_move)
	{
		// end attack move
		self->monsterinfo.currentmove = end_move;

		// if we are very close to our target or standing ground
		// then delay the next attack only until end_move is finished
		if (dist <= 128 || (self->monsterinfo.aiflags & AI_STAND_GROUND))
			M_DelayNextAttack(self, 0, true);
		else
			// otherwise, delay a little bit longer so monster can walk around
			M_DelayNextAttack(self, (GetRandom(10, 20) * FRAMETIME), true);
	}
	else
		// no end move was specified, so delay the next move for awhile
		M_DelayNextAttack(self, (GetRandom(10, 20) * FRAMETIME), false);

	return false;
}

qboolean M_TryRespawn(edict_t* self, qboolean remove_if_fail)
{
	edict_t		*cl = NULL, *e = NULL;
	vec3_t		start;
	trace_t		tr;

	if (!self->mtype)
		return false;


	// is this monster player/client owned?
	if ((cl = G_GetClient(self)) != NULL && cl != self)
	{
		// try to teleport the monster close to the player
		if (TeleportNearTarget(self, cl, 256, false))
		{
			self->s.event = EV_PLAYER_TELEPORT;
			return true;
		}
	}
	else
	{
		// invasion mode?
		if (invasion->value)
		{
			// try to get an available invasion monster spawn
			// note: if this fails, we could also use TeleportNearArea() to teleport near a monster spawn
			if ((e = vrx_inv_get_monster_spawn(e)) != NULL)
			{
				// calculate starting position just above the spawn point
				VectorCopy(e->s.origin, start);
				start[2] = e->absmax[2] + 1 + fabsf(self->mins[2]);

				// perform trace
				tr = gi.trace(start, self->mins, self->maxs, start, NULL, MASK_SHOT);

				// is it clear?
				if (tr.fraction == 1)
				{
					e->wait = level.time + 1.0; // time until spawn is available again

					// worldspawn monsters in invasion mode should be chasing navis
					if (self->activator && !self->activator->client)
						self->monsterinfo.aiflags |= AI_FIND_NAVI;
					self->prev_navi = NULL;

					self->s.angles[YAW] = e->s.angles[YAW];

					// move to position
					VectorCopy(start, self->s.origin);
					VectorCopy(start, self->s.old_origin);
					gi.linkentity(self);

					self->s.event = EV_PLAYER_TELEPORT;
					return true;
				}
			}
		}
		// all other game modes
		else
		{
			// FIXME: you probably don't want monsters respawning randomly in CTF and other team game modes!
			// note: this function is somewhat unreliable (by design) and would be better to spawn at a player spawn point or grid file location, if available
			if (vrx_find_random_spawn_point(self, false))
			{
				self->s.event = EV_PLAYER_TELEPORT;
				return true;
			}
		}
	}

	if (remove_if_fail)
	{
		// are we sure this is a monster? if so, call the M_Remove() function to remove it
		if (self->svflags & SVF_MONSTER)
			M_Remove(self, true, false);
	}
	return false;
}

void M_Touchdown(edict_t* self)
{
	if (level.time > self->monsterinfo.touchdown_delay)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("world/land.wav"), 1, ATTN_IDLE, 0);
	}
	self->monsterinfo.touchdown_delay = level.time + 1.0;
}