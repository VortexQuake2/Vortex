#include "g_local.h"

#define DRONE_TARGET_RANGE		1024	// maximum range for finding targets
#define DRONE_ALLY_RANGE		512		// range allied units can be called for assistance
#define DRONE_FOV				90		// viewing fov used by drone
#define DRONE_MIN_GOAL_DIST		32		// goals are tagged as being reached at this distance
#define DRONE_FINDTARGET_FRAMES	2		// drones search for targets every few frames
#define	DRONE_TELEPORT_DELAY	30		// delay in seconds before a drone can teleport
#define DRONE_SLEEP_FRAMES		100		// frames before drone becomes less alert
#define DRONE_SEARCH_TIMEOUT	300	// frames before drone gives up trying to reach an enemy
#define DRONE_SUICIDE_FRAMES	1800	// idle frames before a world monster suicides

#define STEPHEIGHT				18		// standard quake2 step size
#define DRONE_DEBUG				0		// set to 1 to enable drone AI debugging

qboolean drone_ValidChaseTarget (edict_t *self, edict_t *target);
float distance (vec3_t p1, vec3_t p2);
edict_t *SpawnGoalEntity (edict_t *ent, vec3_t org);
void drone_ai_checkattack (edict_t *self);
qboolean drone_findtarget (edict_t *self, qboolean force);
edict_t *drone_get_target (edict_t *self, qboolean get_medic_target, qboolean get_enemy, qboolean get_navi);
void drone_wakeallies (edict_t *self);
qboolean SV_CloseEnough1(edict_t* ent, vec3_t goalpos, float dist);
int NearestNodeNumber(vec3_t start, float range, qboolean vis);

edict_t *potential_targets[MAX_EDICTS];
float potential_target_distances[MAX_EDICTS][MAX_EDICTS];
int potential_target_count = 0;

/*
=============
ai_eval_targets

Makes a list of all potential targets -- to avoid checking over and over once per enemy when finding
new goals.
=============
*/
void ai_eval_targets() {
	edict_t *from;
	potential_target_count = 0;
	for (from = g_edicts; from < &g_edicts[globals.num_edicts]; from++) {
        // checks: G_EntIsAlive, G_EntExists:
        // that it is also not null, and also in use, is alive,
        // takedamage, is solid (so not a spectator), not respawning,
        // no chat protect, no notarget, not cloaked, not a forcewall
        // not in godmode, and not frozen.
        if (!G_ValidTargetEnt(from, true)) continue;
        from->monsterinfo.target_index = potential_target_count;
        potential_targets[potential_target_count++] = from;
        from->monsterinfo.last_target_scanner = NULL; // forget who last looked at this ent
    }

	/* 
		the older findclosestradius sort of takes the center between the origin and
		the bounding box. this seems a little unneccesary. i give that up
		for a little more efficiency by not calculating distance vectors
		so many times.
		the target distance matrix is symmetrical across the diagonal
		so we only reach the um, upper diagonal assuming top left is 0,0.
	*/
	for (int i = 0; i < potential_target_count; i++) {
		for (int j = i; j < potential_target_count; j++) {
			if (i == j) {
				potential_target_distances[i][j] = 0;
				continue;
			}

			vec3_t eorg;
			// the order of the subtraction doesn't really matter
			VectorSubtract(potential_targets[i]->s.origin, potential_targets[j]->s.origin, eorg);

			// make use of that symmetry
			float len = VectorLengthSqr(eorg);
			//gi.dprintf("%s %s distance to %s %s is %.0f\n", potential_targets[i]->classname, V_GetMonsterName(potential_targets[i]), potential_targets[j]->classname, V_GetMonsterName(potential_targets[j]), len);
			potential_target_distances[i][j] = len;
			potential_target_distances[j][i] = len;
		}
	}
}

qboolean vrx_in_target_list(edict_t *ent) {
	if (ent->monsterinfo.target_index >= 0 && ent->monsterinfo.target_index < potential_target_count) {
		return potential_targets[ent->monsterinfo.target_index] == ent;
	}

	return false;
}

// az: findclosestradius_monmask except ents are only validated once.
// so it only does the checks that are specific to the current monster.
edict_t *findclosestradius_targets(edict_t *prev_ed, edict_t* self, float rad)
{
	edict_t *found = NULL;
	float	found_rad, prev_rad;
	qboolean prev_found = false;

	rad *= rad; // az: square it

	if (prev_ed) {
		/*for (int j = 0; j < 3; j++)
			eorg[j] = self->s.origin[j] - (prev_ed->s.origin[j] + (prev_ed->mins[j] + prev_ed->maxs[j])*0.5);
		prev_rad = VectorLengthSqr(eorg);*/
		prev_rad = potential_target_distances[self->monsterinfo.target_index][prev_ed->monsterinfo.target_index];
	}
	else
	{
		prev_rad = rad + 1;
	}
	found_rad = 0;

	for (int i = 0; i < potential_target_count; i++)
	{
		edict_t* from = potential_targets[i];
		float vlen = potential_target_distances[self->monsterinfo.target_index][i];

		/*for (int j = 0; j < 3; j++)
			eorg[j] = self->s.origin[j] - (from->s.origin[j] + (from->mins[j] + from->maxs[j])*0.5);
		vlen = VectorLengthSqr(eorg);*/

		if (vlen > rad) // found edict is outside scanning radius
			continue;
        if ((vlen < prev_rad) && (prev_ed))  // found edict is closer than the previously returned edict
            continue; // thus this edict must have been returned in an earlier call
        if ((vlen == prev_rad) && (!prev_found)) // several edicts may be at the same range
            continue; // from the center of scan, so if the current edict is "in front of"
        if (from == prev_ed) // the previously returned one, it must have been returned
            prev_found = true; // in an earlier call

        // az: we already looked at this ent (not sure why this could end in an infinite loop without this check...)
        if (from->monsterinfo.last_target_scanner == self)
            continue;

        // az: lol
        if (from == self)
            continue;

        if ((!found) || (vlen <= found_rad)) {
			//gi.dprintf("findclosestradius() found %s %s @ %.0f\n", from->classname, V_GetMonsterName(from), vlen);
            found = from;
            found_rad = vlen;
        }
    }

	if (found)
		found->monsterinfo.last_target_scanner = self;

	return found;
}

qboolean G_ValidTarget_Lite(const edict_t *self, const edict_t *target, qboolean vis)
{
	if (trading->value && !(target->flags & FL_NO_TRADING_PROTECT))
		return false;
		
	// check for targets that require medic healing
	if (self && self->mtype == M_MEDIC)
	{
		if (M_ValidMedicTarget(self, target))
			return true;
	}

	if (self)
	{
		if (vis && !visible(self, target))
		{
			//gi.dprintf("potential target: %s is not visible\n", target->classname);
			return false;
		}
		if (OnSameTeam(self, target))
			return false;
	}
	return true;
}

/*
=============
ai_move

Move the specified distance at current facing.
This replaces the QC functions: ai_forward, ai_back, ai_pain, and ai_painforward
==============
*/
void ai_move (edict_t *self, float dist)
{
	M_walkmove (self, self->s.angles[YAW], dist);
}

/*
=============
ai_charge

Turns towards target and advances
Use this call with a distance of 0 to replace ai_face
==============
*/
void ai_charge (edict_t *self, float dist)
{
	edict_t *target;
	vec3_t	v;

	if (!self || !self->inuse)
		return;

	// our current enemy has become invalid
	if (!G_ValidTarget(self, self->enemy, true))
	{
		// search for new targets (monsters and medic targets)
		if ((target = drone_get_target(self, true, true, false)) != NULL)
		{
			// the new target is on our team or is a non-client
			if (OnSameTeam(self, target) || !target->client)
			{
				// if this is an invasion player spawn (base), then clear goal AI flags
				/*
				if (target->mtype == INVASION_PLAYERSPAWN)
				{
					self->monsterinfo.aiflags &= ~AI_FIND_NAVI;
					if (!self->monsterinfo.melee)
						self->monsterinfo.aiflags  &= ~AI_NO_CIRCLE_STRAFE;
				}*/

				self->enemy = target;

				// trigger sight function, if available
				if (self->monsterinfo.sight)
					self->monsterinfo.sight(self, self->enemy);
				
				// alert nearby monsters and make them chase our enemy
				drone_wakeallies(self);
			}
			else
				return; // couldn't find any valid targets
		}
		else
			return; // couldn't find any other targets
	}

	VectorSubtract (self->enemy->s.origin, self->s.origin, v);
	self->ideal_yaw = vectoyaw(v);
	M_ChangeYaw (self);
}

/*
=============
ai_run_slide

Strafe sideways, but stay at aproximately the same range
=============
*/
void ai_run_slide(edict_t *self, float distance)
{
	float	ofs;

	if (!self->enemy)
		return;

	self->ideal_yaw = self->enemy->s.angles[YAW];
	M_ChangeYaw (self);

	if (self->monsterinfo.lefty)
		ofs = 90;
	else
		ofs = -90;
	
	if (M_walkmove (self, self->ideal_yaw + ofs, distance))
		return;
		
	self->monsterinfo.lefty = 1 - self->monsterinfo.lefty;
	M_walkmove (self, self->ideal_yaw - ofs, distance);

	// walk backwards to maintain distance
	if (entdist(self, self->enemy)+distance < 256)
		M_walkmove(self, self->s.angles[YAW], -distance);
}

/*
=============
drone_wakeallies

Makes allied drones chase our enemy if they aren't busy
=============
*/
void drone_wakeallies (edict_t *self)
{
	edict_t *e=NULL;

	if (!self->enemy)
		return; // what are we doing here?

	// if allied monsters arent chasing a visible enemy
	// then enlist their help
	while ((e = findradius (e, self->s.origin, DRONE_ALLY_RANGE)) != NULL)
	{
		if (!G_EntExists(e))
			continue;
		if (e->client)
			continue;
		if (!(e->svflags & SVF_MONSTER))
			continue;
		if (strcmp(e->classname, "drone"))
			continue;
		if (!OnSameTeam(self, e))
			continue;
		if (!visible(self, e))
			continue;
		if (e->enemy && visible(e, e->enemy))
			continue;
		if (M_IgnoreInferiorTarget(e, self->enemy))//4.5
			continue;
	//	gi.dprintf("ally found!\n");
		e->enemy = self->enemy;
	}
}

qboolean M_ValidMedicTarget(const edict_t *self, const edict_t *target) {
    if (target == self)
        return false;

    if (!G_EntExists(target))
        return false;

    // don't heal supertank or tank commander boss
    if (target->monsterinfo.control_cost >= M_COMMANDER_CONTROL_COST)
        return false;

	// don't target players with invulnerability
	if (target->client && (target->client->invincible_framenum > level.framenum))
		return false;

	// don't target spawning players
	if (target->client && (target->client->respawn_time > level.time))
		return false;

	// don't target players in chat-protect
	if (target->client && ((target->flags & FL_CHATPROTECT) || (target->flags & FL_GODMODE)))
		return false;

	// don't target frozen players
	if (que_typeexists(target->curses, CURSE_FROZEN))
		return false;

	// invasion medics shouldn't try to heal base defenders
	if (invasion->value && (self->monsterinfo.aiflags & AI_FIND_NAVI) && !(target->monsterinfo.aiflags & AI_FIND_NAVI))
		return false;

	// target must be visible
	if (!visible(self, target))
		return false;

	// is medic standing ground?
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		// make sure target is within sight range
		if (entdist(self, target) > self->monsterinfo.sight_range)
			return false;
		// make sure we have an unobstructed path to target
		if (!G_ClearShot(self, NULL, target))
			return false;
	}

	// the target is dead
	if ((target->health < 1) || (target->deadflag == DEAD_DEAD))
	{
		// is this a monster corpse?
		if (!strcmp(target->classname, "drone"))
		{
			// if we are player-owned, don't resurrect more than maximum limit
			if (self->activator && self->activator->client 
				&& (self->activator->num_monsters + target->monsterinfo.control_cost > MAX_MONSTERS))
				return false;
			return true;
		}

		// is this a player corpse?
		if (!strcmp(target->classname, "bodyque") || !strcmp(target->classname, "player"))
		{
			// if we are player-owned, don't resurrect more than maximum limit
			if (self->activator && self->activator->client 
				&& (self->activator->num_monsters + 1 > MAX_MONSTERS))
				return false;
			return true;
		}

		// is this a spiker corpse?
		if (!strcmp(target->classname, "spiker"))
		{
			// if we are player-owned, don't resurrect more than maximum limit
			if (self->activator && self->activator->client 
				&& (self->activator->num_spikers + 1 > SPIKER_MAX_COUNT))
				return false;
			return true;
		}

		// is this an obstacle corpse?
		if (!strcmp(target->classname, "obstacle"))
		{
			// if we are player-owned, don't resurrect more than maximum limit
			if (self->activator && self->activator->client 
				&& (self->activator->num_obstacle + 1 > OBSTACLE_MAX_COUNT))
				return false;
			return true;
		}

		// is this a gasser corpse?
		if (!strcmp(target->classname, "gasser"))
		{
			// if we are player-owned, don't resurrect more than maximum limit
			if (self->activator && self->activator->client 
				&& (self->activator->num_gasser + 1 > GASSER_MAX_COUNT))
				return false;
			return true;
		}

		return false; // invalid corpse
	}
	// target must be on our team and require healing
	else if (OnSameTeam(self, target) < 2 || !M_NeedRegen(target))
		return false;

	return true;
}

qboolean drone_validnavi (edict_t *self, edict_t *other, qboolean visibility_check)
{
	// only world-spawn monsters should search for invasion mode navi
	// don't bother if they are not mobile
	if (self->activator->client || (self->monsterinfo.aiflags & AI_STAND_GROUND))
		return false;
	// make sure this is actually a beacon
	if (!other || !other->inuse || (other->mtype != INVASION_NAVI))//INVASION_NAVI))
		return false;
	// do a visibility check if required
	if (visibility_check && !visible(self, other))
		return false;
	return true;
}

qboolean drone_validpspawn (edict_t *self, edict_t *other)
{
	// only world-spawn monsters should search for invasion mode navi
	// don't bother if they are not mobile
	if (self->activator->client || (self->monsterinfo.aiflags & AI_STAND_GROUND))
		return false;
	// make sure this is actually a beacon
	if (!other || !other->inuse || (other->mtype != INVASION_PLAYERSPAWN))//INVASION_NAVI))
		return false;
	return true;
}

qboolean drone_heartarget (edict_t *target)
{
	// monster always senses other AI
	if (!target->client)
		return true;

	// target made a sound (necessary mostly for weapon firing)
	if (target->lastsound + (0.4 / FRAMETIME) >= level.framenum)
		return true;

	// target used an ability
	if (target->client->ability_delay + 0.4 >= level.time)
		return true;

	return false;
}

qboolean M_IgnoreInferiorTarget (edict_t *self, edict_t *target)
{	
	// doesn't work in invasion mode
	if (invasion->value)
		return false;

	// only ignore valid players
	if (!target || !target->inuse || !target->client)
		return false;

	// player-spawned monsters never ignore inferior targets
	if (self->activator && self->activator->inuse && self->activator->client)
		return false;

	// only ignore targets at or below level 5
	if (target->myskills.level > 5)
		return false;

	// if they are already an enemy, then don't ignore them
	if (target == self->enemy)
		return false;

	if (self->monsterinfo.level <= 5)
		return false;

	// if we are above level 5, then ignore easy targets (unless fired upon!)
	return true;
}

edict_t *drone_get_medic_target (edict_t *self)
{
	edict_t *target = NULL;
	float	range = self->monsterinfo.sight_range;

	// if we're a medic, do a sweep for those with medical need!
	if (self->monsterinfo.aiflags & AI_MEDIC)
	{
		// don't bother finding targets out of our range
		if (self->monsterinfo.aiflags & AI_STAND_GROUND)
			range = 256;

		while ((target = findclosestradius (target, self->s.origin, range)) != NULL)
		{
			if (!M_ValidMedicTarget(self, target))
				continue;
			return target;
		}
	}

	// either we are not a medic or we can't find a valid medic target
	return NULL;
}

edict_t *drone_get_enemy (edict_t *self)
{
	edict_t *target = NULL;

	// find an enemy
	while ((target = findclosestradius_targets (target, self, self->monsterinfo.sight_range)) != NULL)
	{
		// screen out invalid targets
		// az: Replaced G_ValidTarget for lighter check.
		//if (!G_ValidTarget_Lite(self, target, true))
		if (!G_ValidTarget(self, target, true))
			continue;
		// ignore low-level players
		if (M_IgnoreInferiorTarget(self, target))
			continue;
		// limit drone/monster FOV, but check for recent sounds
		if (!nearfov(self, target, 0, DRONE_FOV) && !drone_heartarget(target))
			continue;
		return target;
	}

	// can't find a valid target
	return NULL;
}

edict_t *drone_findnavi (edict_t *self);

edict_t *drone_get_target (edict_t *self, 
			qboolean get_medic_target, qboolean get_enemy, qboolean get_navi)
{
	edict_t	*target = NULL;

	// find medic targets
	if (get_medic_target && (target = drone_get_medic_target(self)) != NULL)
		return target;

	// find enemies
	if (get_enemy && (target = drone_get_enemy(self)) != NULL)
		return target;

	// find navi
	if (invasion->value && get_navi && (target = drone_findnavi(self)) != NULL)
	{
		//gi.dprintf("drone_findnavi() returned %s", target->classname);
		return target;
	}

	return NULL;
}

/*
=============
drone_findtarget

Searches a spherical area for enemies and 
returns true if one is found within range
=============
*/
qboolean drone_findtarget (edict_t *self, qboolean force)
{
	int			frames;
	edict_t		*target=NULL;

	if (level.time < pregame_time->value)
		return false; // pre-game time

	// if a monster hasn't found a target for awhile, it becomes less alert
	// and searches less often, freeing up CPU cycles
	if (!(self->monsterinfo.aiflags & AI_STAND_GROUND)
		&& self->monsterinfo.idle_frames > DRONE_SLEEP_FRAMES)
		frames = (int)(0.4 / FRAMETIME);
	else
		frames = qf2sf(DRONE_FINDTARGET_FRAMES);

	if (level.framenum % frames && !force)
		return false;

	// if we're a medic, do a sweep for those with medical need!
	if ((target = drone_get_target(self, true, false, false)) != NULL)
	{
		self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;
		self->enemy = target;
		return true;
	}

	// find an enemy
	if ((target = drone_get_target(self, false, true, false)) != NULL)
	{
		//gi.dprintf("target found: %s\n", target->classname);
		/*
		// GHz: FIX - this tends to cause monsters to give up following the navi path prematurely
		// if this is an invasion player spawn (base), then clear goal AI flags
		if (target->mtype == INVASION_PLAYERSPAWN)
		{
			self->monsterinfo.aiflags &= ~AI_FIND_NAVI;
			if (!self->monsterinfo.melee)
				self->monsterinfo.aiflags  &= ~AI_NO_CIRCLE_STRAFE;
		}*/

		// if we have no melee function, then make sure we can circle strafe
		if (!self->monsterinfo.melee)
			self->monsterinfo.aiflags  &= ~AI_NO_CIRCLE_STRAFE;

		self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;

		self->enemy = target;

		// trigger sight
		if (self->monsterinfo.sight)
			self->monsterinfo.sight(self, self->enemy);

		// alert nearby monsters and make them chase our enemy
		drone_wakeallies(self);
		return true;
	}

	// find a navigational beacon IF we're not already chasing one
	if (self->monsterinfo.aiflags & AI_FIND_NAVI && (!self->goalentity || self->goalentity->mtype != INVASION_NAVI))
	{
		if ((target = drone_get_target(self, false, false, true)) != NULL)
		{
			/*
			// GHz: what is this doing here? we were looking for a navi and found one
			if (self->goalentity && self->goalentity->mtype == INVASION_NAVI) {
				// az: we didn't get a valid enemy; restore looking for the last navi we were following.
				VectorCopy(self->goalentity->s.origin, self->monsterinfo.last_sighting);
				return true;
			}*/

			// set ai flags to chase goal and turn off circle strafing
			self->monsterinfo.aiflags |= (AI_COMBAT_POINT | AI_NO_CIRCLE_STRAFE);
			self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;
			// self->enemy = target;

			self->goalentity = target;
			VectorCopy(target->s.origin, self->monsterinfo.last_sighting);
			return true;
		}
		else
		{
			if (DRONE_DEBUG)
				gi.dprintf("couldn't find navi");//FIXME: take off the AI_FIND_NAVI aiflag so that monsters can search for player spawns instead?
		}
	}
	return false;
}

qboolean drone_ValidLeader(edict_t *leader)
{
	if (!leader || !leader->inuse)
		return false;
	if (leader->mtype == M_COMBAT_POINT)
		return true;
	return (G_EntIsAlive(leader));
}

void drone_ai_idle (edict_t *self)
{
	// regenerate to full in 30 seconds
	if (self->mtype == M_MEDIC)
		M_Regenerate(self, qf2sf(300), qf2sf(10), 1.0, true, true, false, &self->monsterinfo.regen_delay1);

	else if (self->mtype != M_DECOY && self->health >= 0.5 * self->max_health)
	{
		// change skin if we are being healed by someone else
		self->s.skinnum &= ~1;
		if (self->mtype != M_COMMANDER)
			self->s.skinnum &= ~2;
	}

	// call idle func every so often
	if (self->monsterinfo.idle && (level.time > self->monsterinfo.idle_delay))
	{
		self->monsterinfo.idle(self);
		self->monsterinfo.idle_delay = level.time + GetRandom(15, 30);
	}
	self->monsterinfo.idle_frames++;

	// world monsters suicide if they haven't found an enemy in awhile
	if (self->activator && !self->activator->client && !(self->monsterinfo.aiflags & AI_STAND_GROUND)
		&& (self->monsterinfo.idle_frames > DRONE_SUICIDE_FRAMES))
	{
		if (self->monsterinfo.control_cost >= M_TANK_CONTROL_COST) // we're a boss
			self->activator->num_sentries--;
		if (self->activator)
		{
			self->activator->num_monsters -= self->monsterinfo.control_cost;
			self->activator->num_monsters_real = 0;
		}
		if (self->activator->num_monsters < 0)
			self->activator->num_monsters = 0;
		if (self->activator->num_monsters_real < 0)
			self->activator->num_monsters_real = 0;

		DroneList_Remove(self); // az: PESKY BEE
		BecomeTE(self);
		return;
	}
}

// called when a the drone finds a new target
void drone_newtarget(edict_t* self)
{
	float reactionTime;
	// if monster is not standing ground or enemy is a player
	if (!(self->monsterinfo.aiflags & AI_STAND_GROUND)
		|| (self->enemy && self->enemy->inuse && self->enemy->client))
	{
		reactionTime = M_INITIAL_REACTION_TIME + M_ADDON_REACTION_TIME * self->monsterinfo.level;
		if (M_MIN_REACTION_TIME && reactionTime < M_MIN_REACTION_TIME)
			reactionTime = M_MIN_REACTION_TIME;
		// then add a delay before they can initiate an attack
		self->monsterinfo.attack_finished = level.time + reactionTime;
		//+ (GetRandom((int)(10 * M_MIN_REACTION_TIME), (int)(10 * M_MAX_REACTION_TIME)) * FRAMETIME);
	//gi.dprintf("level time: %0.1f reaction time: %.1f attack_finished: %0.1f\n", level.time, reactionTime, self->monsterinfo.attack_finished);
	}
}

qboolean drone_ai_findgoal (edict_t *self)
{
	// did we have a previous enemy?
	if (self->oldenemy)
	{
		// is he still a valid target?
		if (drone_ValidChaseTarget(self, self->oldenemy))
		{
			// go after him
			self->enemy = self->oldenemy;
			if (!(self->monsterinfo.aiflags & AI_STAND_GROUND))
				self->monsterinfo.run(self);
			return true;
		}
	
		// no longer valid, so forget about him
		self->oldenemy = NULL;
	}
	// can we find a new target?
	else if (drone_findtarget(self, false))
	{
		//gi.dprintf("drone_ai_findgoal found new target\n");
		drone_newtarget(self);
		// go after him
		if (!(self->monsterinfo.aiflags & AI_STAND_GROUND))
			self->monsterinfo.run(self);
		return true;
	}
	// are we supposed to be following someone?
	else if (self->monsterinfo.leader)
	{
		// the leader is still alive or is a combat point
		if (drone_ValidLeader(self->monsterinfo.leader))
		{
			// the leader is more than 256 units away
			if (entdist(self, self->monsterinfo.leader) > 256)
			{
				// pursue the leader
				self->goalentity = self->monsterinfo.leader;
				VectorCopy(self->goalentity->s.origin, self->monsterinfo.last_sighting);
				self->monsterinfo.aiflags |= (AI_NO_CIRCLE_STRAFE|AI_COMBAT_POINT);
				self->monsterinfo.aiflags &= ~AI_STAND_GROUND;
				self->monsterinfo.run(self);
				return true;
			}
			return false;
		}

		// the leader is no longer valid, so forget about him
		self->monsterinfo.leader = NULL;
	}
	
	// didn't find anything to chase
	return false;

}
//FIXME: M_MoveToGoal() won't let monster bump around if he doesn't have a goalentity
void drone_ai_walk (edict_t *self, float dist)
{
	if (self->deadflag == DEAD_DEAD)
		return;
	//if (self->monsterinfo.pausetime > level.time)
	//	return;
	//if (DRONE_DEBUG)
	//	gi.dprintf("drone_ai_walk()\n");

	if (DRONE_DEBUG)
	{
		gi.dprintf("drone_ai_walk() AI_FINDNAVI %s goalentity %s\n", self->monsterinfo.aiflags & AI_FIND_NAVI ? "true" : "false", self->goalentity ? "true" : "false");
	}

	M_MoveToGoal(self, dist);

	// we don't have an enemy
	if (!self->enemy)
	{
		// try to find a new goal (e.g. enemy or combat point)
		if (!drone_ai_findgoal(self))
			drone_ai_idle(self);// couldn't find anything, so just idle
	}
	// we have an enemy
	else
	{
		// make sure he is a valid target
		if (drone_ValidChaseTarget(self, self->enemy))
			self->monsterinfo.run(self);
		// otherwise, try to find a new one
		else if (drone_findtarget(self, false))
		{
			drone_newtarget(self);
			self->monsterinfo.run(self);
		}
		// enemy isn't valid, so give up
		else
			self->enemy = NULL;
	}
}

/*
=============
drone_ai_stand

Called when the drone isn't chasing an enemy or goal
It is also called when the drone is holding position
=============
*/
void drone_ai_stand (edict_t *self, float dist)
{
	vec3_t	v;

	if (self->deadflag == DEAD_DEAD)
		return;
	if (self->monsterinfo.pausetime > level.time)
		return;
	if (DRONE_DEBUG)
		gi.dprintf("drone_ai_stand()\n");
	// used for slight position adjustments for animations
	if (dist)
		M_walkmove(self, self->s.angles[YAW], dist);

	if (!self->enemy)
	{
		// try to find a new goal (e.g. enemy or combat point)
		if (!drone_ai_findgoal(self))
			drone_ai_idle(self);// couldn't find anything, so just idle
	}
	else
	{
		// monster is standing ground
		if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		{
			// is our enemy still valid and visible?
			if (drone_ValidChaseTarget(self, self->enemy) && visible(self, self->enemy))
			{
				// turn towards our enemy and try to attack
				VectorSubtract(self->enemy->s.origin, self->s.origin, v);
				self->ideal_yaw = vectoyaw(v);
				M_ChangeYaw(self);
				drone_ai_checkattack(self);
				return;
			}
		
			self->enemy = NULL;
			return;
		}

		if (drone_ValidChaseTarget(self, self->enemy))
			self->monsterinfo.run(self);
		else if (drone_findtarget(self, false))
		{
			drone_newtarget(self);
			self->monsterinfo.run(self);
		}
		else
			self->enemy = NULL;
	}
}

#define	STATE_TOP			0
#define	STATE_BOTTOM		1
#define STATE_UP			2
#define STATE_DOWN			3

qboolean G_IsClearPath (edict_t *ignore, int mask, vec3_t spot1, vec3_t spot2);

/*
=============
FindPlat
returns true if a nearby platform/elevator is found

self		the entity searching for the platform
plat_pos	the position of the platform (if found)	
=============
*/
qboolean FindPlat (edict_t *self, vec3_t plat_pos)
{
	vec3_t	start, end;
	edict_t *e=NULL;
	trace_t	tr;

	if (!self->enemy)
		return false; // what are we doing here?

	while((e = G_Find(e, FOFS(classname), "func_plat")) != NULL) 
	{
		// this is an ugly hack, but it's the only way to test the distance
		// or visiblity of a plat, since the origin and bbox yield no useful
		// information

		// az: avoid doing traces if it's down, it's useless anyway.
		if (e->moveinfo.state != STATE_BOTTOM && e->moveinfo.state != STATE_TOP)
			continue; // plat must be down

		tr = gi.trace(self->s.origin, NULL, NULL, e->absmax, self, MASK_SOLID);
		VectorCopy(tr.endpos, start);
		VectorCopy(tr.endpos, end);
		end[2] -= 8192;
		tr = gi.trace(start, NULL, NULL, end, self, MASK_SOLID);

		VectorCopy(tr.endpos, start);
		VectorCopy(tr.endpos, plat_pos);
		if (!tr.ent || (tr.ent && (tr.ent != e)))
			continue; // we can't see this plat
		if (distance(start, self->s.origin) > 512)
			continue; // too far
		if (start[2] > self->absmin[2]+2*STEPHEIGHT)
			continue;

		VectorCopy(start, end);
		end[2] += fabsf(e->s.origin[2]);
		if (distance(end, self->enemy->s.origin)
			> distance(self->s.origin, self->enemy->s.origin))
			continue; // plat must bring us closer to our goal

		self->goalentity = e;
		self->monsterinfo.aiflags |= AI_PURSUE_PLAT_GOAL;
		return true;
	}
	return false;
}

/*
=============
FindHigherGoal
finds a goal higher than our current position and moves to it


self	the drone searching for the goal
dist	the distance the drone wants to move
=============
*/
void FindHigherGoal (edict_t *self, float dist)
{
	int		i;
	float	yaw, range=128;
	vec3_t	forward, start, end, best, angles;
	trace_t	tr;

	if (DRONE_DEBUG)
		gi.dprintf("finding a higher goal\n");
	VectorCopy(self->absmin, best);

	// try to run forward first
	VectorCopy(self->s.origin, start);
	start[2] = self->absmin[2]+2*STEPHEIGHT;
	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorMA(start, 128, forward, end);
	tr = gi.trace(start, NULL, NULL, end, self, MASK_SOLID);
	vectoangles(tr.plane.normal, angles);
	AngleCheck(&angles[PITCH]);
	if (angles[PITCH] != 0)
	{
		self->ideal_yaw = vectoyaw(forward);
		M_MoveToGoal(self, dist);
		return;
	}

	while (range <= 512)
	{
		// check 8 angles at 45 degree intervals
		for(i=0; i<8; i++)
		{
			yaw = anglemod(i*45);
			forward[0] = cos(DEG2RAD(yaw));
			forward[1] = sin(DEG2RAD(yaw));
			forward[2] = 0;

			// start scanning above step height
			VectorCopy(self->s.origin, start);
			if (self->waterlevel > 1)
				start[2] = self->absmax[2];//VectorCopy(self->absmax, start);
			else
				start[2] = self->absmin[2];//VectorCopy(self->absmin, start);
			start[2] += 2*STEPHEIGHT;
			// dont trace too far forward, or you will find multiple paths!
			VectorMA(start, range, forward, end);
			tr = gi.trace(start, NULL, NULL, end, self, MASK_SOLID);
			// trace down
			VectorCopy(tr.endpos, start);
			VectorCopy(tr.endpos, end);
			end[2] -= 8192;
			tr = gi.trace(start, NULL, NULL, end, self, MASK_SOLID);
			if (distance(self->absmin, tr.endpos) < DRONE_MIN_GOAL_DIST)
				continue; // too close
			// if we can still see our enemy, dont take a path that will
			// put him out of sight
			if (self->enemy && !(self->monsterinfo.aiflags & AI_LOST_SIGHT)
				&& !G_IsClearPath(self, MASK_SOLID, tr.endpos, self->enemy->s.origin))
				continue;
			vectoangles(tr.plane.normal, angles);
			if (angles[PITCH] > 360)
				angles[PITCH] -= 360;
			if (angles[PITCH] < 360)
				angles[PITCH] += 360;
			if (angles[PITCH] != 270)
				continue; // must be flat ground, or we risk getting stuck
			// is this point higher than the last?
			if (tr.endpos[2] > best[2])
				VectorCopy(tr.endpos, best);
			// we may find more than one position at the same height
			// so take it if it's farther from us
			else if ((tr.endpos[2] == best[2]) && (distance(self->absmin, 
				tr.endpos) > distance(self->absmin, best)))
				VectorCopy(tr.endpos, best);
		}

		// were we able to find anything?
		if (best[2] <= self->absmin[2]+1)
			range *= 2;
		else
			break;
	}

	if (range > 512)
	{
		if (DRONE_DEBUG)
			gi.dprintf("couldnt find a higher goal!!!\n");
		self->goalentity = self->enemy;
		M_MoveToGoal(self, dist);
		return;
	}
	if (DRONE_DEBUG)
		gi.dprintf("found higher goal at %d, current %d\n", (int)best[2], (int)self->absmin[2]);
	
	self->goalentity = SpawnGoalEntity(self, best);
	VectorSubtract(self->goalentity->s.origin, self->s.origin, forward);
	self->ideal_yaw = vectoyaw(forward);
	M_MoveToGoal(self, dist);
	
}

/*
=============
FindLowerGoal
finds a goal lower than our current position and moves to it


self	the drone searching for the goal
dist	the distance the drone wants to move
=============
*/
void FindLowerGoal (edict_t *self, float dist)
{
	int		i;
	float	yaw, range=128;
	vec3_t	forward, start, end, best;
	trace_t	tr;

	if (DRONE_DEBUG)
		gi.dprintf("finding a lower goal\n");
	VectorCopy(self->absmin, best);

	while (range <= 512)
	{
		// check 8 angles at 45 degree intervals
		for(i=0; i<8; i++)
		{
			yaw = anglemod(i*45);
			forward[0] = cos(DEG2RAD(yaw));
			forward[1] = sin(DEG2RAD(yaw));
			forward[2] = 0;

			// start scanning above step height
			VectorCopy(self->s.origin, start);
			//VectorCopy(self->absmin, start);
			//start[2] += STEPHEIGHT;
			start[2] = self->absmin[2] + STEPHEIGHT;
			// dont trace too far forward, or you will find multiple paths!
			VectorMA(start, range, forward, end);
			tr = gi.trace(start, NULL, NULL, end, self, MASK_SOLID);
			// trace down
			VectorCopy(tr.endpos, start);
			VectorCopy(tr.endpos, end);
			end[2] -= 64;
			tr = gi.trace(start, NULL, NULL, end, self, MASK_SOLID);

			if ((tr.fraction == 1) && (self->monsterinfo.jumpdn < 1))
				continue; // don't fall off an edge
			if (distance(self->absmin, tr.endpos) < DRONE_MIN_GOAL_DIST)
				continue; // too close
			// is this point lower than the last?
			if ((tr.endpos[2] < best[2]))
				VectorCopy(tr.endpos, best);
			// we may find more than one position at the same height
			// so take it if it's farther from us
			else if ((tr.endpos[2] == best[2]) && (distance(self->absmin, 
				tr.endpos) > distance(self->absmin, best)))
				VectorCopy(tr.endpos, best);
		}

		// were we able to find anything?
		if (VectorEmpty(best) || (best[2] >= self->absmin[2]))
			range *= 2;
		else
			break;
	}

	if (range > 512)
	{
		if (DRONE_DEBUG)
			gi.dprintf("couldn't find a goal!!!\n");
		self->goalentity = self->enemy;
		M_MoveToGoal(self, dist);
		return;
	}

	self->goalentity = SpawnGoalEntity(self, best);
	VectorSubtract(self->goalentity->s.origin, self->s.origin, forward);
	self->ideal_yaw = vectoyaw(forward);
	M_MoveToGoal(self, dist);
	

}

/*
qboolean FollowWall (edict_t *self, vec3_t endpos, vec3_t wall_normal, float dist)
{
	int		i;
	vec3_t	forward, start, end, angles;
	trace_t tr;

	if (self->monsterinfo.aiflags & AI_PURSUIT_LAST_SEEN)
		return false; // give a chance for the regular AI to work

	for (i=90; i<180; i*=2)
	{
		
		vectoangles(wall_normal, angles);
		angles[YAW] += i;
		if (angles[YAW] > 360)
			angles[YAW] -= 360;
		if (angles[YAW] < 360)
			angles[YAW] += 360;
		forward[0] = cos(DEG2RAD(angles[YAW]));
		forward[1] = sin(DEG2RAD(angles[YAW]));
		forward[2] = 0;

		VectorCopy(endpos, start);
		VectorMA(start, 64, forward, end);
		tr = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);
		if (tr.fraction < 1)
			continue;
		VectorCopy(tr.endpos, start);
		VectorCopy(tr.endpos, end);
		end[2] -= 64;
		tr = gi.trace(start, NULL, NULL, end, self, MASK_SOLID);
		if (tr.fraction == 1) // don't fall
			continue;
		self->goalentity = SpawnGoalEntity(self, tr.endpos);
		VectorSubtract(self->goalentity->s.origin, self->s.origin, forwa
			rd);
		self->ideal_yaw = vectoyaw(forward);
		M_MoveToGoal(self, dist);
		gi.dprintf("following wall at %d degrees...\n", i);
		return true;
	}
	gi.dprintf("*** FAILED at %d degrees ***\n", i);
	return false;
	
}
*/

void FindCloserGoal (edict_t *self, vec3_t target_origin, float dist)
{
	int		i;
	float	yaw, best_dist=8192;
	vec3_t	forward, start, end, best;
	trace_t	tr;
	if (DRONE_DEBUG)
		gi.dprintf("finding a closer goal\n");
	VectorCopy(self->s.origin, best);
	// check 8 angles at 45 degree intervals
	for(i=0; i<8; i++)
	{
		yaw = anglemod(i*45);
		forward[0] = cos(DEG2RAD(yaw));
		forward[1] = sin(DEG2RAD(yaw));
		forward[2] = 0;

		// start scanning above step height
		VectorCopy(self->absmin, start);
		start[2] += STEPHEIGHT;
		// trace forward
		VectorMA(start, 64, forward, end);
		tr = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);

		// trace down
		VectorCopy(tr.endpos, start);
		VectorCopy(tr.endpos, end);
		end[2] -= 64;
		tr = gi.trace(start, NULL, NULL, end, self, MASK_SOLID);

		if (tr.fraction == 1) // don't fall
			continue;
		if (distance(self->absmin, tr.endpos) < DRONE_MIN_GOAL_DIST)
			continue; // too close

		// is this point closer?
		if (distance(target_origin, tr.endpos) < best_dist)
		{
			best_dist = distance(target_origin, tr.endpos);
			VectorCopy(tr.endpos, best);
		}
	}

	if (distance(best, target_origin) >= distance(tr.endpos, target_origin))
	{
		self->goalentity = self->enemy;
		M_MoveToGoal(self, dist);
		if (DRONE_DEBUG)
			gi.dprintf("couldnt find a closer goal!!!\n");
		return;
	}

	self->goalentity = SpawnGoalEntity(self, best);
	VectorSubtract(self->goalentity->s.origin, self->s.origin, forward);
	self->ideal_yaw = vectoyaw(forward);
	M_MoveToGoal(self, dist);
}

void drone_unstuck (edict_t *self)
{
	int		i, yaw;
	vec3_t	forward, start;
	trace_t	tr;

	// check 8 angles at 45 degree intervals
	for(i=0; i<8; i++)
	{
		// get new vector
		yaw = anglemod(i*45);
		forward[0] = cos(DEG2RAD(yaw));
		forward[1] = sin(DEG2RAD(yaw));
		forward[2] = 0;
		
		// trace from current position
		VectorMA(self->s.origin, 64, forward, start);
		tr = gi.trace(self->s.origin, self->mins, self->maxs, start, self, MASK_SHOT);
		if ((tr.fraction == 1) && !(gi.pointcontents(start) & CONTENTS_SOLID))
		{
			VectorCopy(tr.endpos, self->s.origin);
			gi.linkentity(self);
			self->monsterinfo.air_frames = 0;
			if (DRONE_DEBUG)
				gi.dprintf("freed stuck monster!\n");
			return;
		}

	}
	if (DRONE_DEBUG)
		gi.dprintf("couldnt fix\n");
}

#if 0
void drone_pursue_goal (edict_t *self, float dist)
{
	float	goal_elevation;
	vec3_t	goal_origin, v;
	vec3_t	forward, start, end;
	trace_t	tr;

	// if we are not on the ground and we can see our goal
	// then turn towards it
	if (!self->groundentity && !self->waterlevel && visible(self, self->goalentity))
	{
		//gi.dprintf("off ground %d\n", level.framenum);
		VectorSubtract(self->goalentity->s.origin, self->s.origin, v);
		self->ideal_yaw = vectoyaw(v);
		M_ChangeYaw(self);

		if (!self->waterlevel)
			self->monsterinfo.air_frames++;
		if (self->monsterinfo.air_frames > 20)
			drone_unstuck(self);
		return;
	}

	self->monsterinfo.air_frames = 0;

	// don't bother pursuing an enemy that isn't on the ground
	if (self->goalentity->groundentity || !self->goalentity->takedamage
		|| (self->waterlevel != self->goalentity->waterlevel))
	{
		// try to find a plat first
		if (FindPlat(self, goal_origin))
		{
			// top of plat
			goal_elevation = goal_origin[2];
		}
		else
		{
			goal_elevation = self->goalentity->absmin[2]; // ent's feet
			VectorCopy(self->goalentity->s.origin, goal_origin);
		}


		// can we see our enemy?
		if (self->monsterinfo.aiflags & AI_LOST_SIGHT)
		{
			// is our enemy higher?
			if (goal_elevation > self->absmin[2]+2*STEPHEIGHT)
				FindHigherGoal(self, dist);
			// is our enemy lower?
			else if (goal_elevation < self->absmin[2]-2*STEPHEIGHT)
				FindLowerGoal(self, dist);
			else
				FindCloserGoal(self, goal_origin, dist);
		}
		else
		{
			// is our enemy higher?
			if (goal_elevation > self->absmin[2]+max(self->monsterinfo.jumpup, 2*STEPHEIGHT)+1)
				FindHigherGoal(self, dist);
			// is our enemy lower?
			else if (goal_elevation < self->absmin[2]-max(self->monsterinfo.jumpdn, 2*STEPHEIGHT))
				FindLowerGoal(self, dist);
			// is anyone standing in our way?
			else
			{
				VectorCopy(self->s.origin, start);
				AngleVectors(self->s.angles, forward, NULL, NULL);
				VectorMA(self->s.origin, self->maxs[1]+dist, forward, start);
				VectorCopy(start, end);
				end[2] -= 2*STEPHEIGHT;
				tr = gi.trace(start, NULL, NULL, end, self, MASK_SOLID);
				// did we fall off an edge?
				if (tr.fraction == 1.0)
				{
					M_MoveToGoal(self, dist);
					return;
				}

				tr = gi.trace(self->s.origin, NULL, NULL, goal_origin, self, MASK_SOLID);
				// is anyone in our way?
				//if (!tr.ent || (tr.ent != self->goalentity))

				// is the path clear of obstruction?
				if (tr.fraction < 1)
				{
					M_MoveToGoal(self, dist);
					return;	
				}

				VectorSubtract(goal_origin, self->s.origin, v);
				self->ideal_yaw = vectoyaw(v);
				M_MoveToGoal(self, dist);
			}
		}
	}
	else
	{
		if (self->monsterinfo.aiflags & AI_LOST_SIGHT)
			FindCloserGoal(self, self->goalentity->s.origin, dist);
		else if (self->waterlevel && self->goalentity->waterlevel)
		{
			VectorSubtract(self->goalentity->s.origin, self->s.origin, v);
			self->ideal_yaw = vectoyaw(v);
			M_MoveToGoal(self, dist);
		}
		else
			M_MoveToGoal(self, dist);
	}
}

#endif

void drone_ai_run_slide (edict_t *self, float dist)
{
	float	ofs, range;
	vec3_t	v;
	
	VectorSubtract(self->enemy->s.origin, self->s.origin, v);
	self->ideal_yaw = vectoyaw(v);
	M_ChangeYaw (self);

	//4.4 try to maintain ideal range/distance to target
	range = VectorLength(v) - 196;
	if (fabs(range) >= dist)
	{
		if (range > dist)
			range = dist;
		else if (range < -dist)
			range = -dist;
		M_walkmove (self, self->ideal_yaw, range);
		if (DRONE_DEBUG)
			gi.dprintf("moved %.0f units\n", range);
	}

	if (self->monsterinfo.lefty)
		ofs = 90;
	else
		ofs = -90;
	
	if (M_walkmove (self, self->ideal_yaw + ofs, dist))
		return;
		
	// change direction and try strafing in the opposite direction
	self->monsterinfo.lefty = 1 - self->monsterinfo.lefty;
	M_walkmove (self, self->ideal_yaw - ofs, dist);
}
		
void TeleportForward (edict_t *ent, vec3_t vec, float dist);

void drone_cleargoal (edict_t *self)
{
	self->enemy = NULL;
	self->oldenemy = NULL;
	self->goalentity = NULL;

	// clear ai flags
	self->monsterinfo.aiflags &= ~AI_COMBAT_POINT;
	if (!self->monsterinfo.melee)
		self->monsterinfo.aiflags  &= ~AI_NO_CIRCLE_STRAFE;

	self->monsterinfo.search_frames = 0;

	if (self->monsterinfo.walk)
		self->monsterinfo.walk(self);
	else if (self->monsterinfo.stand)
		self->monsterinfo.stand(self);
}


int NextWaypointLocation (vec3_t start, vec3_t loc, int *wp);
qboolean NearestNodeLocation (vec3_t start, vec3_t node_loc, float range, qboolean vis);
int FindPath(int searchType, vec3_t start, vec3_t destination);
void M_MoveToPosition(edict_t* ent, vec3_t pos, float dist, qboolean stop_when_close);
int CopyWaypoints (int *wp, int max);
int NearestWaypointNum (vec3_t start, int *wp);
void GetNodePosition (int nodenum, vec3_t pos);
void DrawPath (edict_t *ent);

void drone_ai_giveup (edict_t *self)
{
	// reset enemy/goal pointers
	self->enemy = NULL;
	self->oldenemy = NULL;
	self->goalentity = NULL;

	// reset waypoint data
	self->monsterinfo.numWaypoints = 0;
	self->monsterinfo.nextWaypoint = 0;
	memset(self->monsterinfo.waypoint, 0, sizeof(self->monsterinfo.waypoint));

	// if we have no melee function, then make sure we can circle strafe
	if (!self->monsterinfo.melee)
		self->monsterinfo.aiflags  &= ~AI_NO_CIRCLE_STRAFE;
	
	// we're not lost anymore, but just idle
	self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;
			
	// if we can walk, walk
	if (self->monsterinfo.walk)
		self->monsterinfo.walk(self);
	else
	// otherwise, just stand and idle
		self->monsterinfo.stand(self);

	// we are done searching, reset counter
	self->monsterinfo.search_frames = 0;
}

// bump around while incrementing search counter
// return true if we should keep moving, or false if we should give up
qboolean drone_ai_lost (edict_t *self, edict_t *goalent, float dist)
{
	// give up searching for a goal/path after a while
	if (self->monsterinfo.search_frames > 100)
	{
		if (DRONE_DEBUG)
			gi.dprintf("%s gave up\n", V_GetMonsterKind(self->mtype));
		// we have a goalentity that is not our current goal
		if (self->enemy && self->goalentity && self->goalentity != goalent)
		{
			self->enemy = NULL;
			// try to get to the goalentity
			VectorCopy(self->goalentity->s.origin, self->monsterinfo.last_sighting);
			self->monsterinfo.search_frames = 0;
			return true;
		}
		drone_ai_giveup(self);
		return false;
	}
	if (DRONE_DEBUG)
		gi.dprintf("%s is lost\n", V_GetMonsterKind(self->mtype));
	M_FindPath(self, self->monsterinfo.last_sighting, false);

	// bump around
	M_ChangeYaw(self);
	M_MoveToPosition(self, goalent->s.origin, dist, true);

	// increment search counter
	self->monsterinfo.search_frames++;
	return true;
}

void M_ClearPath (edict_t *self)
{
	// couldn't find a path to our goal
	self->monsterinfo.numWaypoints = 0;
	self->monsterinfo.nextWaypoint = 0;
	memset(self->monsterinfo.waypoint, 0, sizeof(self->monsterinfo.waypoint));
	// wait awhile before searching for another path
	self->monsterinfo.path_time = level.time + (GetRandom(1, 20) * FRAMETIME);
}

void M_FindPath (edict_t *self, vec3_t goalpos, qboolean compute_path_now)
{
	int searchType;

	// update the path now or after a brief delay
	if (compute_path_now || (self->monsterinfo.updatePath && level.time > self->monsterinfo.path_time))
	{
		vec3_t v1, v2;

		if (DRONE_DEBUG)
			gi.dprintf("M_FindPath() trying to recalc path\n");
		// 
		// get node location nearest to us and our goal
		if (!(NearestNodeLocation(self->s.origin, v1, 0, true))
			||!(NearestNodeLocation(goalpos, v2, 0, true)))
		{
			// can't find nearby nodes
			M_ClearPath(self);
			return;
		}
		
		// try to find a path between these two nodes
		if (self->flags & FL_FLY)
			searchType = SEARCHTYPE_FLY;
		else
			searchType = SEARCHTYPE_WALK;

		if (FindPath(searchType, v1, v2))
		{
			if (DRONE_DEBUG)
				gi.dprintf("%s (%d) is recalculating path at %d\n", 
					GetMonsterKindString(self->mtype), G_GetEntityIndex(self), level.framenum);

			// copy waypoints to monster
			self->monsterinfo.numWaypoints =
				CopyWaypoints(self->monsterinfo.waypoint, 1000);
			// get index of next waypoint
			self->monsterinfo.nextWaypoint = NearestWaypointNum(self->s.origin, self->monsterinfo.waypoint) + 1;
			// brief delay before we can re-compute path to goal
			self->monsterinfo.path_time = level.time + (GetRandom(1, 20) * FRAMETIME);
			self->monsterinfo.updatePath = false;
			self->monsterinfo.aiflags &= ~AI_LOST_SIGHT; // we are not lost anymore, try this new path
		}
		else
		{
			if (DRONE_DEBUG)
				gi.dprintf("%s (%d) failed to recalculate path at %d\n", 
					GetMonsterKindString(self->mtype), G_GetEntityIndex(self), level.framenum);

			// couldn't find a path to our goal
			M_ClearPath(self);
		}
	}
}

// returns true if this monster is on patrol
//FIXME: it would probably be easier/smarter if spot1/2 were just entity pointers
// to the temp goals...
qboolean drone_ai_patrol (edict_t *self)
{
	edict_t *temp=NULL, *best=NULL;
	float	dist, bestDist=0;

	// if either locations have not been set, then abort
	if (VectorCompare(self->monsterinfo.spot1, vec3_origin)
		|| VectorCompare(self->monsterinfo.spot2, vec3_origin))
		return false;

	// find the farthest combat point that our activator owns
	while ((temp = G_FindEntityByMtype(M_COMBAT_POINT, temp)) != NULL)
	{
		// does our activator own it?
		if (temp->activator != self->activator)
			continue;
		// is it one of the spots on our patrol route?
		if (!VectorCompare(self->monsterinfo.spot1, temp->s.origin)
			&& !VectorCompare(self->monsterinfo.spot2, temp->s.origin))
			continue;

		//if (temp == self->goalentity) // az: don't switch target to the same point we're already going to
		  //  continue;
		// get distance to combat point/goalentity
		dist = entdist(self, temp);
		// find the spot farthest away
		if (dist > bestDist)
		{
			bestDist = dist;
			best = temp;
		}
	}

	// couldn't find combat point or combat point isn't farther than the one we are chasing
	if (!best)
		return false;
	if (DRONE_DEBUG)
		gi.dprintf("switching patrol spots\n");
	self->monsterinfo.aiflags |= (AI_NO_CIRCLE_STRAFE|AI_COMBAT_POINT);
	self->monsterinfo.aiflags &= ~AI_STAND_GROUND;
	self->enemy = NULL;
	VectorCopy(best->s.origin, self->monsterinfo.last_sighting);
	self->goalentity = best;
	// calculate path immediately
	M_FindPath(self, self->monsterinfo.last_sighting, true);
	// update previous goal position
	VectorCopy(self->monsterinfo.last_sighting, self->monsterinfo.prevGoalPos);
	//self->monsterinfo.run(self);
	//self->monsterinfo.selected_time = level.time + 3.0;// blink briefly
	return true;
}

qboolean M_CanCircleStrafe (edict_t *self, edict_t *target)
{
	vec3_t	start, end;
	trace_t	tr;

	if (self->monsterinfo.aiflags & AI_NO_CIRCLE_STRAFE)
		return false;
	// don't circle strafe non-moving targets
	if (target->movetype == MOVETYPE_NONE)
		return false;
	// target must be less than or equal to 256 units from us
	if (entdist(self, target) > 256)
		return false;
	// target must be within +/- 18 units (1 step) on the Z axis
	if (!self->flags & FL_FLY && fabs(self->absmin[2] - target->absmin[2]) > 18)
		return false;
	// check if anything is blocking our attack
	G_EntMidPoint(self, start);
	G_EntMidPoint(target, end);
	tr = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);
	// is there a clear shot?
	if (tr.ent && tr.ent == target)
		return true;
	// shot is blocked
	return false;
}

void drone_ai_run1 (edict_t *self, float dist)
{
	edict_t		*goal;
	vec3_t		v, dest;
	qboolean	goalVisible=false, goalChanged=false;
	float		maxZ;

	// if we're dead, we shouldn't be here
	if (self->deadflag == DEAD_DEAD)
		return;

	// if the drone is standing, we shouldn't be here
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		self->monsterinfo.stand(self);
		return;
	}

	//if (DRONE_DEBUG)
		//gi.dprintf("drone_ai_run1()\n");

	if (DRONE_DEBUG)
	{
		gi.dprintf("drone_ai_run1() AI_FINDNAVI %s goalentity %s\n", self->monsterinfo.aiflags & AI_FIND_NAVI ? "true" : "false", self->goalentity ? "true" : "false");
	}

	if (self->mtype != M_DECOY && self->health >= 0.5 * self->max_health)
	{
		// change skin if we are being healed by someone else
		self->s.skinnum &= ~1;
		if (self->mtype != M_COMMANDER)
			self->s.skinnum &= ~2;
	}

	// monster is no longer idle
	self->monsterinfo.idle_frames = 0;

	// decoys make step sounds
	if ((self->mtype == M_DECOY) && (level.time > self->wait)) 
	{
		gi.sound (self, CHAN_BODY, gi.soundindex(va("player/step%i.wav", (randomMT()%4)+1)), 1, ATTN_NORM, 0);
		self->wait = level.time + 0.3;
	}

	// determine which goal to chase
	if (self->enemy)
	{
		goal = self->enemy; // an enemy
	}
	else if (self->goalentity)
	{
		// try to find an enemy
		drone_findtarget(self, false);
		if (self->enemy)
		{
			goal = self->enemy;
		}
		else // couldn't find one, so follow goalentity (non-enemy goal)
			goal = self->goalentity;
	}
	else
	{
		drone_ai_giveup(self);
		return; // what are we doing here without a goal?
	}

	// has the goal entity changed between this frame and last frame?
	if (self->monsterinfo.lastGoal != goal)
	{
		goalChanged = true;
		if (DRONE_DEBUG)
		{
			if (goal->mtype == INVASION_NAVI)
			{
				gi.dprintf("changed goal to navi %s\n", goal->targetname);
			}
			else
			{
				gi.dprintf("changed goal entity\n");
			}
		}
	}
	self->monsterinfo.lastGoal = goal; // update new goal

	// is the goal entity valid and chaseable?
	if (drone_ValidChaseTarget(self, goal))
	{
		/* az note 1: chase after killable enemy */
		// goal entity is visible
		if (visible(self, goal))
		{
			goalVisible = true;

			// is the goal our enemy?
			if (self->enemy)
			{
				// try to attack
				drone_ai_checkattack(self);

				// circle strafe at medium distance
				if (M_CanCircleStrafe(self, self->enemy))
				{
					//gi.dprintf("trying to circle strafe\n");
					drone_ai_run_slide(self, dist);
					return;
				}
				
				// don't let a search time-out as long as we can see the enemy
				self->monsterinfo.search_frames = 0;
			}
		}
		// we can't see our goal and the goal entity is not at the last known position
		else if (self->monsterinfo.aiflags & AI_LOST_SIGHT)
		{
			if (DRONE_DEBUG)
				gi.dprintf("drone lost sight\n");
			drone_ai_lost(self, goal, dist);
			return;
		}

		if (DRONE_DEBUG)
		{
			if (self->monsterinfo.last_sighting)
				gi.dprintf("drone last sighting: %f %f %f\n", self->monsterinfo.last_sighting[0], self->monsterinfo.last_sighting[1], self->monsterinfo.last_sighting[2]);
		}
		/* az note 2: move towards last sighting */
		// goal position is within +/- 1 step of our elevation and we have a clear line of sight
		//FIXME: if goal entity is taller than us (e.g. jorg), this wont work very well!
		if (self->flags & FL_FLY)
			maxZ = 8192;
		else if (self->monsterinfo.jumpup)
			maxZ = self->monsterinfo.jumpup;
		else
			maxZ = 18;// step size

		if (DRONE_DEBUG)
		{
			if (self->monsterinfo.last_sighting)
			{
				float Zdelta = fabs(self->s.origin[2] - self->monsterinfo.last_sighting[2]);
				qboolean pCl = G_IsClearPath(self, MASK_SOLID, self->s.origin, self->monsterinfo.last_sighting);
				gi.dprintf("Zdelta = %f clear = %d\n", Zdelta, pCl);
			}
			else
			{
				gi.dprintf("can't move to goal with no last sighting!\n");
			}

		}

		//FIXME: monster will continue to follow the path even after the target has moved far away from it, potentially causing silly situations where the monster runs past
		// players/enemies to complete the path--maybe we need another timer to let a path time-out if the target is no longer there?
		// if we want to forgo the path check and allow monsters to pursue moving to a goal even when there is a path, then we probably need to clear the path somewhere below
		// move to goal if monster has...
		if (/*(!self->monsterinfo.numWaypoints || self->monsterinfo.nextWaypoint == self->monsterinfo.numWaypoints) // no waypoints, or we've reached last waypoint
			&& */fabs(self->s.origin[2] - self->monsterinfo.last_sighting[2]) <= maxZ // we are on the same plane/level, within +/- step up/down
			&& G_IsClearPath(self, MASK_SOLID, self->s.origin, self->monsterinfo.last_sighting))// we have a clear/unobstructed path to goal's last known or current position
		{
			float dst = distance(self->s.origin, self->monsterinfo.last_sighting);

			// turn towards goal last sighting if we are very close to it
			//if (dst <= G_GetHypotenuse(self->maxs) + 32)
			if (SV_CloseEnough1(self, self->monsterinfo.last_sighting, dist))
			{
				if (DRONE_DEBUG)
					gi.dprintf("close to last sighting\n");
				// we've reached the temporary entity used for movement commands
				if (self->monsterinfo.aiflags & AI_COMBAT_POINT)
				{
					
					if (DRONE_DEBUG)
						gi.dprintf("reached combat point\n");
					
					// az: follow the chain
					if (!G_GetClient(self)) // non-player spawned monster, i.e. owned by worldspawn
					{
						if (DRONE_DEBUG)
						{
							if (goal->target_ent)
							{
								gi.dprintf("goal has a chain link\n");
								if (Q_strcasecmp(goal->target_ent->targetname, "end") == 0)
								{
									gi.dprintf("reached end\n");
								}
							}
							

						}
                        if (invasion->value) {
							if (goal->target_ent && goal != goal->target_ent)
							{
								if (DRONE_DEBUG)
									gi.dprintf("current navi: %s, next: %s\n", self->goalentity->targetname, goal->target_ent->targetname);
								// self->enemy = goal->target_ent;
								self->goalentity = goal->target_ent;
								VectorCopy(self->goalentity->s.origin, self->monsterinfo.last_sighting); // az: Move! DO SOMETHING!!
								return;
							}
							else if (goal->mtype == INVASION_NAVI)
							{
								if (DRONE_DEBUG)
									gi.dprintf("reached the end of navi chain\n");
								// monster is currently chasing a navi but we have reached the end
								self->monsterinfo.aiflags &= ~(AI_FIND_NAVI | AI_COMBAT_POINT);
								drone_ai_giveup(self);
								return;
							}
                        }
						/*
						else if (invasion->value) // az: no chain, or target is the same...
                        {
							if (DRONE_DEBUG)
								gi.dprintf("no navi chain or target unchanged\n");
                            // az: Clear this out so we don't seek any more navis after this
                            self->monsterinfo.aiflags &= ~(AI_FIND_NAVI | AI_COMBAT_POINT);
                            drone_ai_findgoal(self); // az: Look for pspawns or players or anything!
                            return;
                        }*/
                    }

					// drone_ai_patrol() returns true if drone is patrolling and swaps between goalentities (points A and B in the patrol route)
					if (!drone_ai_patrol(self))
					{
						// we're close enough, clear the goal
						self->monsterinfo.aiflags &= ~AI_COMBAT_POINT;
						drone_ai_giveup(self);
					}
					if (DRONE_DEBUG)
						gi.dprintf("drone reached temp ent\n");
					return;
				}

				// is the goal entity visible?
				if (!goalVisible)
				{
					// we've reached the last known position of the goal entity
					// but we still haven't found it, so we're lost :(
					if (DRONE_DEBUG)
						gi.dprintf("drone is lost\n");
					self->monsterinfo.aiflags |= AI_LOST_SIGHT;
					drone_ai_lost(self, goal, dist);
					return;
				}

				// turn towards the goal
				VectorSubtract(self->monsterinfo.last_sighting, self->s.origin, v);
				self->ideal_yaw = vectoyaw(v);
			}
			// turn towards goal last sighting if we are not bumping around
			else if (level.time >= self->monsterinfo.bump_delay)
			{
				if (DRONE_DEBUG)
					gi.dprintf("turning towards goal\n");

				VectorSubtract(self->monsterinfo.last_sighting, self->s.origin, v);
				self->ideal_yaw = vectoyaw(v);
			}

			M_ChangeYaw(self);
			M_MoveToPosition(self, self->monsterinfo.last_sighting, dist, true);
			if (DRONE_DEBUG)
				gi.dprintf("drone is moving to goal dist=%f dst=%f\n", dist, dst);
			return;
		}
		/*else
		{
			if (DRONE_DEBUG)
			{
				gi.dprintf("goal Z delta too large or goal is blocked\n");

			}
		}*/

		if (goalChanged)
		{
			if (DRONE_DEBUG)
				gi.dprintf("%s (%d) goal changed at %d\n", 
					GetMonsterKindString(self->mtype), G_GetEntityIndex(self), level.framenum);
			// calculate path immediately
			M_FindPath(self, self->monsterinfo.last_sighting, true);
			// update previous goal position
			VectorCopy(self->monsterinfo.last_sighting, self->monsterinfo.prevGoalPos);
		}
		// FIXME: this code won't actually cause a path recalculation as M_FindPath() isn't called
		// to make this work, there would need to be code elsewhere that looks for updatePath==true and call M_FindPath()
		// has the goal position moved?
		if (!VectorCompare(self->monsterinfo.prevGoalPos, self->monsterinfo.last_sighting))
		{
			if (DRONE_DEBUG)
				gi.dprintf("%s (%d) goal position changed at %d\n", 
					GetMonsterKindString(self->mtype), G_GetEntityIndex(self), level.framenum);
			// compute path after a delay
			self->monsterinfo.updatePath = true;
			M_FindPath(self, self->monsterinfo.last_sighting, false);//FIXME: this really needs to be called continually until updatePath is false; alternatively we need to force a path update
			VectorCopy(self->monsterinfo.last_sighting, self->monsterinfo.prevGoalPos);
		}

		// if we're stuck, then follow a new course
		if (self->monsterinfo.bump_delay > level.time)
		{
			if (DRONE_DEBUG && level.framenum % 10)
				gi.dprintf("bump around\n");
			M_ChangeYaw(self);
			M_MoveToPosition(self, goal->s.origin, dist,true);
			return;
		}

		// do we have waypoints to follow?
		if (self->monsterinfo.numWaypoints)
		{
			if (DRONE_DEBUG)
			gi.dprintf("drone has waypoints\n");
			// 
			// have we reached the end of the path we are searching?
			if (self->monsterinfo.nextWaypoint < self->monsterinfo.numWaypoints)
			{
				int next, nexter, closesterWaypointNode, closestWaypointNode;//for debugging
				int	nearestWaypoint;
				float	wpDist;
				vec3_t	org;

				// if we can't see the nearest waypoint, or it's too far away, then give up
				nearestWaypoint = NearestWaypointNum(self->s.origin, self->monsterinfo.waypoint);
				GetNodePosition(self->monsterinfo.waypoint[nearestWaypoint], dest);
				G_EntViewPoint(self, org);
				wpDist = Get2dDistance(org, dest);
				if (!G_IsClearPath(self, MASK_SOLID, org, dest) || wpDist > 256)
				{
					if (DRONE_DEBUG)
						gi.dprintf("***can't see the nearest waypoint***\n");
					M_ClearPath(self); // path is no good if we can't see it
					VectorClear(self->monsterinfo.prevGoalPos);// this will cause a delayed path recalc
					return;
				}
				//gi.dprintf("close enough to wp: %d distance: %f\n", SV_CloseEnough1(self, dest, dist), distance(org, dest));
				// have we reached the next waypoint in our path?
				if (nearestWaypoint == self->monsterinfo.nextWaypoint && wpDist <= 32)//SV_CloseEnough1(self, dest, dist))//FIXME: should we use SV_CloseEnough() instead of distance check for fast/big monsters?
					// increment the next waypoint counter so we keep moving closer to our goal
					self->monsterinfo.nextWaypoint++;
				//FIXME: after reaching the last wp, the path should be cleared

				if (DRONE_DEBUG)
					gi.dprintf("%s (%d) is now moving towards node %d\n",
						GetMonsterKindString(self->mtype), G_GetEntityIndex(self),
						self->monsterinfo.waypoint[self->monsterinfo.nextWaypoint]);

				//FIXME: code above looks for a clear path to goalent, and follows it regardless of waypoints; either we force waypoints to be followed if present OR
				// we change the code below to find the nearest waypoint, and then find the next waypoint from that one
				
				// get the position of the next waypoint
				GetNodePosition(self->monsterinfo.waypoint[self->monsterinfo.nextWaypoint], dest);

				// if we can't see the next waypoint, or it's too far away, then choose a closer waypoint
				if ((self->monsterinfo.nextWaypoint != (nearestWaypoint + 1)) // monster isn't already moving towards the ideal waypoint
					&& (level.time > self->monsterinfo.backtrack_delay) // monster not affected by backtrack delay
					&& (!G_IsClearPath(self, MASK_MONSTERSOLID, org, dest) || Get2dDistance(org, dest) > 256)) // next (current) waypoint is obstructed or it's too far away
				{
					if (DRONE_DEBUG)
						gi.dprintf("#### can't see next waypoint ###\n");
					self->monsterinfo.nextWaypoint = nearestWaypoint + 1; // move towards the ideal waypoint
					self->monsterinfo.backtrack_delay = level.time + GetRandom(3, 6);// don't do this too often or we might get stuck!
					//return;
				}

				// we are standing on a platform
				if (self->groundentity && (self->groundentity->style == FUNC_PLAT))
				{
					//GetNodePosition(self->monsterinfo.waypoint[self->monsterinfo.nextWaypoint], dest);
					// destination node is above us and the platform is not moving
					if (dest[2] - self->s.origin[2] > 32 && self->groundentity->velocity[2] < 1)
					{
						if (DRONE_DEBUG)
							gi.dprintf("trigger platform up %.0f %.0f\n", dest[2],self->s.origin[2]);
						plat_go_up(self->groundentity);
						//self->groundentity->use(self->groundentity, NULL, NULL);
						self->monsterinfo.pausetime = level.time + 1.0;
						self->monsterinfo.stand(self);
						return;
					}
				}

				// debug stuff below
				closestWaypointNode = self->monsterinfo.waypoint[NearestWaypointNum(self->s.origin, self->monsterinfo.waypoint)];//FIXME: this sometimes returns an incorrect value that doesn't agree with NearestNodeNumber()!
				closesterWaypointNode = NearestNodeNumber(self->s.origin, 255, true);
				next = self->monsterinfo.waypoint[self->monsterinfo.nextWaypoint];
				nexter = self->monsterinfo.waypoint[self->monsterinfo.nextWaypoint + 1];
				//nexter = self->monsterinfo.waypoint[NearestWaypointNum(self->s.origin, self->monsterinfo.waypoint)+1];
				if (DRONE_DEBUG)
					gi.dprintf("next waypoint = %d %d (%d/%d), closest = %d %d (%d/%d)\n", next, nexter, self->monsterinfo.nextWaypoint, self->monsterinfo.numWaypoints,
						closestWaypointNode, closesterWaypointNode, NearestWaypointNum(self->s.origin, self->monsterinfo.waypoint), self->monsterinfo.numWaypoints);
				if (DRONE_DEBUG && closestWaypointNode != closesterWaypointNode)
				{
					qboolean foundWp = false;
					// does the actual closest node exist in our path?
					for (int i = 0; i < self->monsterinfo.numWaypoints; i++)
					{
						if (self->monsterinfo.waypoint[i] == closesterWaypointNode)
						{
							foundWp = true;
							break;
						}
					}
					// it does, so WTF don't these numbers match?!
					if (foundWp)
						gi.dprintf("mismatch!\n");
				}


				// move towards it
				VectorSubtract(dest, self->s.origin, v);
				self->ideal_yaw = vectoyaw(v);
				M_ChangeYaw(self);
				M_MoveToPosition(self, dest, dist, false);//FIXME: stop_when_close must be 'false' otherwise monsters (especially large ones) might not be able to move toward the next wp, as their bbox is already touching the next one!

				//FIXME: the following code to increment might be buggy because the monster moved in the preceding line via M_MoveToPosition, so we have to recalculate distance to waypoint
				//G_EntViewPoint(self, org);
				//wpDist = Get2dDistance(org, dest);

				// have we reached the next waypoint in our path?
				//if (NearestWaypointNum(self->s.origin, self->monsterinfo.waypoint) == self->monsterinfo.nextWaypoint && wpDist <= 32)//FIXME: should we use SV_CloseEnough() instead of distance check for fast/big monsters?
					// increment the next waypoint counter so we keep moving closer to our goal
				//	self->monsterinfo.nextWaypoint++;

			}
			else
			{
				// we've reached the end of the path and can't see our goal
				drone_ai_lost(self, goal, dist);
				self->monsterinfo.aiflags |= AI_LOST_SIGHT;//we're lost
				M_ClearPath(self);
				if (DRONE_DEBUG)
					gi.dprintf("drone reached the end of path\n");
			}
		}
		else
		{
			if (DRONE_DEBUG)
				gi.dprintf("couldnt find valid path\n");
			// we couldn't determine a valid path
			drone_ai_lost(self, goal, dist);			
		}
	}
	else
	{
		if (DRONE_DEBUG)
			gi.dprintf("no valid enemy/goal, give up\n");
		// we don't have a valid enemy/goal, so give up
		drone_ai_giveup(self);
	}
}

/*
=============
drone_ai_run

Called when the monster is chasing an enemy or goal
=============
*/
void drone_ai_run (edict_t *self, float dist)
{
	// float           time;
	// vec3_t          start, end, v;
	// trace_t         tr;
	// edict_t         *tempgoal=NULL;
	// qboolean        enemy_vis=false;

	// if (level.pathfinding > 0)
	{
		drone_ai_run1(self, dist);
		return;
	}

#if 0
	// if we're dead, we shouldn't be here
	if (self->deadflag == DEAD_DEAD)
		return;


	// decoys make step sounds
	if ((self->mtype == M_DECOY) && (level.time > self->wait)) 
	{
		gi.sound (self, CHAN_BODY, gi.soundindex(va("player/step%i.wav", (randomMT()%4)+1)), 1, ATTN_NORM, 0);
		self->wait = level.time + 0.3;
	}


	// monster is no longer idle
	self->monsterinfo.idle_frames = 0;


	// if the drone is standing, we shouldn't be here
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		self->monsterinfo.stand(self);
		return;
	}


	if (drone_ValidChaseTarget(self, self->enemy))
	{
		if ((self->monsterinfo.aiflags & AI_COMBAT_POINT) && !self->enemy->takedamage)
		{
			if (!drone_findtarget(self, false) && (entdist(self, self->enemy) <= 64))
			{
				// the goal is a navi
				if (self->enemy->mtype == INVASION_NAVI)
				{
					edict_t *e;

					// we're close enough to this navi; follow the next one in the chain
					if (self->enemy->target && self->enemy->targetname &&
						((e = self->enemy->target_ent) != NULL))
					{
						//gi.dprintf("following next navi in chain\n");
						self->enemy = e;
					}
					else
					{
						//FIXME: this never happens because monsters usually see the player/bases and begin attacking before they reach the last navi
						// we've reached our final destination
						drone_cleargoal(self);
						self->monsterinfo.aiflags &= ~AI_FIND_NAVI;
						//gi.dprintf("reached final destination\n");
						return;
					}
				}
				else
				{
					// the goal is a combat point
					//gi.dprintf("cleared combat point\n");
					drone_cleargoal(self);
					return;
				}
			}
		}


		drone_ai_checkattack(self); // try attacking


		// constantly update the last place we remember
		// seeing our enemy
		if (visible(self, self->enemy))
		{
			VectorCopy(self->enemy->s.origin, self->monsterinfo.last_sighting);
			self->monsterinfo.aiflags &= ~(AI_LOST_SIGHT|AI_PURSUIT_LAST_SEEN);
			self->monsterinfo.teleport_delay = level.time + DRONE_TELEPORT_DELAY; // teleport delay
			self->monsterinfo.search_frames = 0;
			enemy_vis = true;


			// circle-strafe our target if we are close
			if ((entdist(self, self->enemy) < 256) 
				&& !(self->monsterinfo.aiflags & AI_NO_CIRCLE_STRAFE))
			{
				drone_ai_run_slide(self, dist);
				return;
			}
		}
		else
		{
			// drone should try to find a navi if enemy isn't visible
			if ((self->monsterinfo.aiflags & AI_FIND_NAVI) && self->enemy->takedamage)
			{
				//gi.dprintf("can't see %s, trying another target...", self->enemy->classname);

				// save current target
				self->oldenemy = self->enemy;
				self->enemy = NULL; // must be cleared to find navi


				// try to find a new target
				if (drone_findtarget(self, false))
				{
					//gi.dprintf("found %s!\n", self->enemy->classname);
					drone_ai_checkattack(self);
				}
				else
				{
					// restore current target
					self->enemy = self->oldenemy;
					self->oldenemy = NULL;
					//gi.dprintf("no target.\n");
				}
			}


			// drones give up if they can't reach their enemy
			if (!self->enemy || (self->monsterinfo.search_frames > DRONE_SEARCH_TIMEOUT))
			{
				//gi.dprintf("drone gave up\n");


				self->enemy = NULL;
				self->oldenemy = NULL;
				self->goalentity = NULL;


				//self->monsterinfo.aiflags &= ~AI_COMBAT_POINT;


				if (!self->monsterinfo.melee)
					self->monsterinfo.aiflags  &= ~AI_NO_CIRCLE_STRAFE;


				self->monsterinfo.stand(self);
				self->monsterinfo.search_frames = 0;
				return;
			}
			self->monsterinfo.search_frames++;
			//gi.dprintf("%d\n", self->monsterinfo.search_frames);
		}


		if (dist < 1)
		{
			VectorSubtract(self->enemy->s.origin, self->s.origin, v);
			self->ideal_yaw = vectoyaw(v);
			M_ChangeYaw(self);
			return;
		}


		// if we are on a platform going up, wait around until we've reached the top
		if (self->groundentity && (self->monsterinfo.aiflags & AI_PURSUE_PLAT_GOAL)
			&& (self->groundentity->style == FUNC_PLAT) 
			&& (self->groundentity->moveinfo.state == STATE_UP)) 
		{
			//      gi.dprintf("standing on plat!\n");
			// divide by speed to get time to reach destination
			time = 0.1 * (fabsf(self->groundentity->s.origin[2]) / self->groundentity->moveinfo.speed);
			self->monsterinfo.pausetime = level.time + time;
			self->monsterinfo.stand(self);
			self->monsterinfo.aiflags &= ~AI_PURSUE_PLAT_GOAL;
			return;
		}


		// if we're stuck, then follow new course
		if (self->monsterinfo.bump_delay > level.time)
		{
			M_ChangeYaw(self);
			M_MoveToGoal(self, dist);
			return;
		}


		// we're following a temporary goal
		if (self->movetarget && self->movetarget->inuse)
		{
			// pursue the movetarget
			M_MoveToGoal(self, dist);


			// occasionally, a monster will bump to a better course
			// than the one he was previously following
			if ((self->enemy->absmin[2] > self->absmin[2]+2*STEPHEIGHT) &&
				(self->movetarget->s.origin[2] < self->absmin[2]))
			{
				// we are higher than our current goal
				G_FreeEdict(self->movetarget);
				self->movetarget = NULL;
			}
			else if ((self->enemy->absmin[2] < self->absmin[2]-2*STEPHEIGHT) &&
				(self->movetarget->s.origin[2] > self->absmin[2]))
			{
				// we are lower than our current goal
				G_FreeEdict(self->movetarget);
				self->movetarget = NULL;
			}
			else if (entdist(self, self->movetarget) <= DRONE_MIN_GOAL_DIST)
			{
				// we are close 'nuff to our goal
				G_FreeEdict(self->movetarget);
				self->movetarget = NULL;
			}
		}
		else
		{       
			// we don't already have a goal, make the enemy our goal
			self->goalentity = self->enemy;


			// pursue enemy if he's visible, otherwise pursue the last place
			// he was seen if we haven't already been there
			if (!enemy_vis)
			{
				if (!(self->monsterinfo.aiflags & AI_LOST_SIGHT))
					self->monsterinfo.aiflags |= (AI_LOST_SIGHT|AI_PURSUIT_LAST_SEEN);

				if (self->monsterinfo.aiflags & AI_PURSUIT_LAST_SEEN)
				{
					VectorCopy(self->monsterinfo.last_sighting, start);
					VectorCopy(start, end);
					end[2] -= 64;
					tr = gi.trace(start, NULL, NULL, end, self, MASK_SOLID);
					// we are done chasing player last sighting if we're close to it
					// or it's hanging in mid-air
					if ((tr.fraction == 1) || (distance(self->monsterinfo.last_sighting, self->s.origin) < DRONE_MIN_GOAL_DIST))
					{
						self->monsterinfo.aiflags &= ~AI_PURSUIT_LAST_SEEN;
					}
					else
					{
						tempgoal = G_Spawn();
						VectorCopy(self->monsterinfo.last_sighting, tempgoal->s.origin);
						VectorCopy(self->monsterinfo.last_sighting, tempgoal->absmin);
						self->goalentity = tempgoal;
					}
				}
			}

			// go get him!
			drone_pursue_goal(self, dist);


			if (tempgoal)
				G_FreeEdict(tempgoal);
		}
	}
	else
	{
		//      gi.dprintf("stand was called because monster has no valid target!\n");
		self->monsterinfo.stand(self);
	}
#endif
}				

void drone_togglelight (edict_t *self)
{
	if (self->health > 0)
	{
		if (level.daytime && self->flashlight) 
		{
			// turn light off
			G_FreeEdict(self->flashlight);
			self->flashlight = NULL;
			return;;
		}
		else if (!level.daytime && !self->flashlight)
		{
			FL_make(self);
		}
	}
	else
	{
		// turn off the flashlight if we're dead!
		if (self->flashlight)
		{
			G_FreeEdict(self->flashlight);
			self->flashlight = NULL;
			return;
		}
	}
}

void drone_dodgeprojectiles (edict_t *self) {
    qboolean alive = self->health > 0;
    qboolean can_dodge = self->monsterinfo.dodge && (self->monsterinfo.aiflags & AI_DODGE);
    qboolean stand_ground = (self->monsterinfo.aiflags & AI_STAND_GROUND);

    // dodge incoming projectiles
    if (alive && can_dodge && !stand_ground) // don't dodge if we are holding position
    {
        if (((self->monsterinfo.radius > 0) && ((level.time + 0.3) > self->monsterinfo.eta))
            || (level.time + FRAMETIME) > self->monsterinfo.eta) {
            self->monsterinfo.dodge(self, self->monsterinfo.attacker, self->monsterinfo.dir, self->monsterinfo.radius);
            self->monsterinfo.aiflags &= ~AI_DODGE;
        }
    }
}

/*
=============
drone_validposition

Returns false if the monster gets stuck in a solid object
and cannot be re-spawned
=============
*/
qboolean drone_validposition (edict_t *self)
{
	//trace_t		tr;
	//qboolean	respawned = false;

	if (gi.pointcontents(self->s.origin) & CONTENTS_SOLID)
	{
		//if (self->activator && self->activator->inuse && !self->activator->client)
		//	respawned = vrx_find_random_spawn_point(self, false);
		
		//if (!respawned)
		//{
			WriteServerMsg("A drone was removed from a solid object.", "Info", true, false);
			M_Remove(self, true, false);
			return false;
		//}
	}
	return true;
}

qboolean drone_boss_stuff (edict_t *self)
{
	if (self->deadflag == DEAD_DEAD)
		return true; // we're dead

	//4.05 boss always regenerates
	/*
	if (self->monsterinfo.control_cost > 2)
	{
		if (self->enemy)
			M_Regenerate(self, 900, 10, 1.0, true, true, false, &self->monsterinfo.regen_delay1);
		else // idle
			M_Regenerate(self, 600, 10, 1.0, true, true, false, &self->monsterinfo.regen_delay1);
	}
	*/

	// bosses teleport away from danger when they are weakened
	if (self->activator && !self->activator->client && (self->monsterinfo.control_cost >= 100)
		&& (level.time > self->sentrydelay) && (self->health < (0.3*self->max_health))
		&& self->enemy && visible(self, self->enemy)
		&& invasion->value != 1) // easy modo invasion doesn't allow bosses to escape. :p
	{
		gi.bprintf(PRINT_HIGH, "Boss teleported away.\n");

		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BOSSTPORT);
		gi.WritePosition (self->s.origin);
		gi.multicast (self->s.origin, MULTICAST_PVS);

		if(!vrx_find_random_spawn_point(self, false))
		{
			// boss failed to re-spawn
			/*
			if (self->activator)
				self->activator->num_sentries--;
			G_FreeEdict(self);
			*/
			M_Remove(self, false, false);
			gi.dprintf("WARNING: Boss failed to re-spawn!\n");
			return false;
		}
		self->enemy = NULL; // find a new target
		self->oldenemy = NULL;
		self->monsterinfo.stand(self);
		self->sentrydelay = level.time + GetRandom(10, 30);

		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BOSSTPORT);
		gi.WritePosition (self->s.origin);
		gi.multicast (self->s.origin, MULTICAST_PVS);
	}
	return true;
}

void decoy_copy (edict_t *self);

void M_UpdateLastSight (edict_t *self)
{
	edict_t *goal;

	// determine goal entity
	if (self->enemy)
		goal = self->enemy;
	else if (self->goalentity)
		goal = self->goalentity;
	else
		return;

	// is the goal entity visible?
	if (!visible(self, goal))
		return;

	// last sight position has not been reached yet
	self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;

	// update last known position of goal
	VectorCopy(goal->s.origin, self->monsterinfo.last_sighting);
}

void drone_return(edict_t* self)
{
	edict_t* leader = self->monsterinfo.leader;

	if (self->mtype != M_DECOY)
		return;

	// teleport back to our leader if we can't reach him
	if (!self->enemy && leader && leader->inuse && self->monsterinfo.search_frames > 300 
		&& level.time > self->monsterinfo.teleport_delay)
	{
		TeleportNearArea(self, self->activator->s.origin, 256, false);
		self->monsterinfo.teleport_delay = level.time + 1;
	}
}

void drone_think (edict_t *self)
{
	//gi.dprintf("drone_think()\n");

	if (!drone_validposition(self))
		return;
	if (!drone_boss_stuff(self))
		return;

	drone_return(self);
	//decoy_copy(self);

	CTF_SummonableCheck(self);

	// monster will auto-remove if it is asked to
	if (self->removetime > 0)
	{
		qboolean converted=false;

		if (self->flags & FL_CONVERTED)
			converted = true;

		if (level.time > self->removetime)
		{
			// if we were converted, try to convert back to previous owner
			if (!RestorePreviousOwner(self))
			{
				M_Remove(self, false, true);
				return;
			}
		}
		// warn the converted monster's current owner
		else if (converted && self->activator && self->activator->inuse && self->activator->client 
			&& (level.time > self->removetime-5)
			&& !(level.framenum % (int)(1 / FRAMETIME)))
				safe_cprintf(self->activator, PRINT_HIGH, "%s conversion will expire in %.0f seconds\n", 
					V_GetMonsterName(self), self->removetime-level.time);	
	}

	// if owner can't pay upkeep, monster dies
	if (!M_Upkeep(self, 1 / FRAMETIME, self->monsterinfo.cost/20))
		return;

	M_UpdateLastSight(self);
	V_HealthCache(self, (int)(0.2 * self->max_health), 1);
	V_ArmorCache(self, (int)(0.2 * self->monsterinfo.max_armor), 1);

	// draw waypoint path for debugging if enabled
	DrawPath(self);

	//Talent: Life Tap
	// if (self->activator && self->activator->inuse && self->activator->client && !(level.framenum % 10))
	// {
    // 	if (vrx_get_talent_level(self->activator, TALENT_LIFE_TAP) > 0)
	// 	{
	// 		int damage = 0.01 * self->max_health;
	// 		if (damage < 1)
	// 			damage = 1;
	// 		T_Damage(self, world, world, vec3_origin, self->s.origin, vec3_origin, damage, 0, DAMAGE_NO_ABILITIES, 0);
	// 	}
	// }

	if (self->linkcount != self->monsterinfo.linkcount)
	{
		self->monsterinfo.linkcount = self->linkcount;
		M_CheckGround (self);
	}

	// don't slide
	if (self->groundentity)
		VectorClear(self->velocity);
	else if (self->flags & FL_FLY)
	{
		self->velocity[0] *= 0.8;
		if (self->velocity[0] < 1)
			self->velocity[0] = 0;
		self->velocity[1] *= 0.8;
		if (self->velocity[1] < 1)
			self->velocity[1] = 0;
		self->velocity[2] *= 0.8;
		if (self->velocity[2] < 1)
			self->velocity[2] = 0;
	}

	/*drone_togglelight(self);*/
	drone_dodgeprojectiles(self);
	
	// this must come before M_MoveFrame() because a monster's dead
	// function may set the nextthink to 0
	self->nextthink = level.time + 0.1; // run at 10 frames/sec
	/*
	if (self->mtype == M_DECOY && level.framenum % 10)
	{
		self->model = self->activator->model;
		self->s.skinnum = self->activator->s.skinnum;
		self->s.modelindex = self->activator->s.modelindex;
		self->s.modelindex2 = self->activator->s.modelindex2;
		self->s.modelindex3 = self->activator->s.modelindex3;
		self->s.modelindex4 = self->activator->s.modelindex4;

		if (self->activator && self->activator->inuse)
		{
			gi.dprintf("class skin: %s\n", V_GetClassModel(self->activator));
			gi.dprintf("activator model: %s skinnum: %d modelindex: %d modelindex2: %d\n", self->activator->model, self->activator->s.skinnum, self->activator->s.modelindex, self->activator->s.modelindex2);
			gi.dprintf("decoy model: %s skinnum: %d modelindex: %d modelindex2: %d\n", self->model, self->s.skinnum, self->s.modelindex, self->s.modelindex2);
		}
		else
			gi.dprintf("no activator\n");
	}*/

	M_MoveFrame (self);
	M_CatagorizePosition (self);
	M_WorldEffects (self);
	M_SetEffects (self);
}
