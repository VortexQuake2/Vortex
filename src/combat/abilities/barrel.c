#include "g_local.h"

void barrel_remove(edict_t* self, qboolean refund)
{
	gclient_t* cl = NULL;
	if (!self || !self->inuse || self->deadflag == DEAD_DEAD)
		return;

	// reduce armor count
	if (self->creator && self->creator->inuse && self->creator->client)
	{
		// reduce count
		self->creator->num_barrels--;
		cl = self->creator->client;
		// refund player
		if (refund)
			cl->pers.inventory[power_cube_index] += self->monsterinfo.cost;
		// stop tracking this previously picked up entity
		if (cl->pickup_prev && cl->pickup_prev->inuse && cl->pickup_prev == self)
			cl->pickup_prev = NULL;
		// drop it
		if (cl->pickup && cl->pickup->inuse && cl->pickup == self)
			cl->pickup = NULL;
		// remove from HUD
		layout_remove_tracked_entity(&cl->layout, self);
	}

	self->think = BecomeExplosion1;
	self->nextthink = level.time + FRAMETIME;
	self->svflags |= SVF_NOCLIENT;
	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT;
	self->deadflag = DEAD_DEAD;
}

void barrel_think(edict_t* self)
{
	edict_t* owner = NULL;

	if (!G_EntIsAlive(self->creator))
	{
		barrel_remove(self, false);
		return;
	}

	if (gi.pointcontents(self->s.origin) & CONTENTS_SOLID)
	{
		gi.dprintf("Barrel was removed from a solid object.\n");
		barrel_remove(self, true);
		safe_cprintf(self->creator, PRINT_HIGH, "Removed barrel from a solid object (%d/%d).\n", self->creator->num_barrels, (int)EXPLODING_BARREL_MAX_COUNT);
		return;
	}

	// get preferred owner of barrel
	if (PM_PlayerHasMonster(self->creator))
		owner = self->creator->owner;
	else
		owner = self->creator;

	// is a player holding this barrel?
	if (self->creator->client && self->creator->client->pickup && self->creator->client->pickup == self)
	{
		// if owner isn't set or invalid, set it to the preferred owner
		// this will make barrel non-solid to the player (or player-monster)
		if (!self->owner || !self->owner->inuse || self->owner != owner)
			self->owner = owner;
	}
	// player is no longer holding barrel--is owner set?
	else if (self->owner)
	{
		//gi.dprintf("let go!\n");
		// need to make this null first so that the trace works
		self->owner = NULL;
		// barrel isn't being held, so make it solid again to player if it's clear of obstructions
		trace_t tr = gi.trace(self->s.origin, self->mins, self->maxs, self->s.origin, self, MASK_PLAYERSOLID);
		//FIXME: the owner won't be cleared if the player managed to stick the barrel in a bad spot
		if (tr.allsolid || tr.startsolid || tr.fraction < 1)
			self->owner = owner;
	}

	// if position has been updated, check for ground entity
	if (self->linkcount != self->monsterinfo.linkcount)
	{
		self->monsterinfo.linkcount = self->linkcount;
		M_CheckGround(self);
	}

	// don't slide
	if (self->groundentity)
		VectorClear(self->velocity);

	M_CatagorizePosition(self);
	M_WorldEffects(self);
	M_SetEffects(self);

	//G_RunFrames(self, HEALER_FRAMES_START, HEALER_FRAMES_END, false);

	self->nextthink = level.time + FRAMETIME;

}

void BarrelDetonate(edict_t* self)
{
	int		i;
	vec3_t	org, start;
	float	spd;

	if (self->solid == SOLID_NOT)
		return; // already flagged for removal

	barrel_remove(self, false);

	// creator no longer valid?
	if (!G_EntExists(self->creator))
		return;

	safe_cprintf(self->creator, PRINT_HIGH, "Barrel detonated! (%d/%d)\n", self->creator->num_barrels, (int)EXPLODING_BARREL_MAX_COUNT);

	T_RadiusDamage(self, self->creator, self->dmg, NULL, self->dmg_radius, MOD_EXPLODING_BARREL);

	// throw shrapnel around
	VectorCopy(self->s.origin, start);
	VectorMA(self->absmin, 0.5, self->size, start);
	// small chunks
	spd = 2 * self->dmg / 200;
	for (i = 0;i < GetRandom(4, 6);i++)
	{
		org[0] = start[0] + crandom() * self->size[0];
		org[1] = start[1] + crandom() * self->size[1];
		org[2] = start[2] + crandom() * self->size[2];
		ThrowShrapnel(self, "models/objects/debris2/tris.md2", spd, org, self->dmg, MOD_SHRAPNEL);
	}
	// big chunks
	spd = 1.5 * (float)self->dmg / 200.0;
	for (i = 0;i < GetRandom(2, 3);i++)
	{
		org[0] = start[0] + crandom() * self->size[0];
		org[1] = start[1] + crandom() * self->size[1];
		org[2] = start[2] + crandom() * self->size[2];
		ThrowShrapnel(self, "models/objects/debris1/tris.md2", spd, org, self->dmg, MOD_SHRAPNEL);
	}
	// bottom corners
	spd = 1.75 * (float)self->dmg / 200.0;
	VectorCopy(self->absmin, org);
	ThrowShrapnel(self, "models/objects/debris3/tris.md2", spd, org, self->dmg, MOD_SHRAPNEL);
	VectorCopy(self->absmin, org);
	org[0] += self->size[0];
	ThrowShrapnel(self, "models/objects/debris3/tris.md2", spd, org, self->dmg, MOD_SHRAPNEL);
	VectorCopy(self->absmin, org);
	org[1] += self->size[1];
	ThrowShrapnel(self, "models/objects/debris3/tris.md2", spd, org, self->dmg, MOD_SHRAPNEL);
	VectorCopy(self->absmin, org);
	org[0] += self->size[0];
	org[1] += self->size[1];
	ThrowShrapnel(self, "models/objects/debris3/tris.md2", spd, org, self->dmg, MOD_SHRAPNEL);
}

void barrel_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, vec3_t point)
{
	BarrelDetonate(self);
}

void barrel_touchdown(edict_t* self)
{
	if (level.time > self->monsterinfo.touchdown_delay)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("world/land.wav"), 1, ATTN_IDLE, 0);
	}
	self->monsterinfo.touchdown_delay = level.time + 1.0;
}

void SpawnExplodingBarrel(edict_t* ent)
{
	edict_t* barrel;
	vec3_t	 forward, right, start, offset;

	barrel = G_Spawn();
	//barrel->owner = prop;
	barrel->creator = ent;
	barrel->classname = "barrel";
	barrel->mtype = M_BARREL;
	barrel->solid = SOLID_BBOX;
	barrel->monsterinfo.touchdown = barrel_touchdown;
	barrel->clipmask = MASK_MONSTERSOLID;
	barrel->svflags |= SVF_MONSTER; // used for collision detection--apparently prevents this entity from sliding into other entities and occupying the same space!
	barrel->movetype = MOVETYPE_STEP;
	barrel->model = "models/objects/barrels/tris.md2";
	barrel->s.modelindex = gi.modelindex(barrel->model);
	VectorSet(barrel->mins, -16, -16, 0);
	VectorSet(barrel->maxs, 16, 16, 40);
	barrel->monsterinfo.level = ent->myskills.abilities[EXPLODING_BARREL].current_level;
	barrel->monsterinfo.cost = EXPLODING_BARREL_COST;
	barrel->health = barrel->max_health = EXPLODING_BARREL_INITIAL_HEALTH + EXPLODING_BARREL_ADDON_HEALTH * barrel->monsterinfo.level;
	barrel->mass = 100;
	barrel->takedamage = DAMAGE_YES;
	barrel->die = barrel_die;
	barrel->dmg = EXPLODING_BARREL_INITIAL_DAMAGE + EXPLODING_BARREL_ADDON_DAMAGE * barrel->monsterinfo.level;
	barrel->dmg_radius = 100 + 0.25 * barrel->dmg;
	barrel->think = barrel_think;//M_droptofloor;
	barrel->nextthink = level.time + FRAMETIME;
	gi.linkentity(barrel);
	layout_add_tracked_entity(&ent->client->layout, barrel); // add to HUD

	// get view origin
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	//FIXME: this function modifies start and returns the tr.endpos, so the next bit of code may push the starting position into a wall!
	if (!G_GetSpawnLocation(ent, 80, barrel->mins, barrel->maxs, start, NULL, false))
	{
		G_FreeEdict(barrel);
		return;
	}

	// move armor into position
	VectorCopy(start, barrel->s.origin);
	VectorCopy(start, barrel->s.old_origin);
	//VectorMA(start, 48, forward, barrel->s.origin);
	gi.linkentity(barrel);

	//VectorClear(armor->avelocity);
	VectorCopy(ent->s.angles, barrel->s.angles);
	barrel->s.angles[PITCH] = 0;
	barrel->s.angles[ROLL] = 0;

	ent->num_barrels++;
	ent->client->pers.inventory[power_cube_index] -= barrel->monsterinfo.cost;
	safe_cprintf(ent, PRINT_HIGH, "Spawned exploding barrel (%d/%d)\n", ent->num_barrels, (int)EXPLODING_BARREL_MAX_COUNT);
	// pick it up!
	ent->client->pickup = barrel;
}
//PM_MonsterHasPilot
void V_PickUpEntity(edict_t* ent)
{
	float	dist;
	edict_t	*e = ent, *other = ent->client->pickup;
	vec3_t forward, right, offset, start, org;
	trace_t tr;

	if (!G_EntIsAlive(ent) || !ent->client)
	{
		//gi.dprintf("V_PickUpEntity aborted: ent isn't alive\n");
		return;
	}
	// is the barrel dead?
	if (!G_EntIsAlive(other))
	{
		//gi.dprintf("V_PickUpEntity aborted: barrel is dead\n");
		// drop it
		ent->client->pickup = NULL;
		return;
	}
	// is this a player-tank?
	if (PM_PlayerHasMonster(ent))
	{
		//gi.dprintf("V_PickUpEntity: player tank\n");
		e = ent->owner;
	}

	// get view origin
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	// calculate point in-front of us, leaving just enough room so that our bounding boxes don't collide
	dist = e->maxs[0] + other->maxs[0] + 0.1 * VectorLength(ent->velocity) + 8;
	VectorMA(start, dist, forward, start);
	// begin trace at calling entity's origin, adjusted to fit the height of the entity we're carrying
	VectorCopy(e->s.origin, org);
	org[2] = e->absmin[2] + other->mins[2] + 1;
	// check for obstructions at our starting position - this will cover situations where the player can squeeze under a staircase
	tr = gi.trace(org, other->mins, other->maxs, org, other, MASK_SOLID);
	if (tr.allsolid || tr.startsolid || tr.fraction < 1)
		return;
	// check for obstructions between our starting and ending positions
	tr = gi.trace(org, other->mins, other->maxs, start, other, MASK_SHOT);
	if (tr.allsolid)// || tr.fraction < 1)
	{
		//gi.dprintf("obstruction %s at %.0f distance and %.0f height fraction %.1f allsolid %d startsolid %d\n", tr.ent->classname, dist, tr.endpos[2], tr.fraction, tr.allsolid, tr.startsolid);
		//ent->client->pickup = NULL;
		return;
	}
	// move into position
	VectorCopy(tr.endpos, other->s.origin);
	VectorCopy(tr.endpos, other->s.old_origin);
	gi.linkentity(other);
	VectorClear(other->velocity);
}

void RemoveExplodingBarrels(edict_t* ent, qboolean refund)
{
	edict_t* e = NULL;

	while ((e = G_Find(e, FOFS(classname), "barrel")) != NULL)
	{
		if (e && e->inuse && (e->creator == ent))
			barrel_remove(e, refund);
	}

	// reset barrel counter
	ent->num_barrels = 0;
}

qboolean CanDropPickupEnt(edict_t* ent)
{
	edict_t* pickup_prev = ent->client->pickup_prev;
	// don't have a pickup entity to drop
	if (!ent->client->pickup || !ent->client->pickup->inuse)
		return false;
	// previous pickup entity might get in the way
	if (pickup_prev && pickup_prev->inuse && pickup_prev->owner && pickup_prev->owner->inuse && pickup_prev->owner == ent)
		return false;
	return true;

}

void Cmd_ExplodingBarrel_f(edict_t* ent)
{
	edict_t* e = NULL;
	vec3_t forward;

	if (Q_strcasecmp(gi.args(), "remove") == 0)
	{
		RemoveExplodingBarrels(ent, true);
		safe_cprintf(ent, PRINT_HIGH, "All exploding barrels removed.\n");
		return;
	}

	// already picked up a barrel?
	if (ent->client->pickup && ent->client->pickup->inuse)
	{
		//gi.dprintf("have pickup\n");
		if (CanDropPickupEnt(ent))
		{
			//gi.dprintf("dropping it\n");
			AngleVectors(ent->client->v_angle, forward, NULL, NULL);
			// toss it forward if it's airborne
			if (!ent->client->pickup->groundentity)
			{
				VectorScale(forward, 400, ent->client->pickup->velocity);
				ent->client->pickup->velocity[2] += 200;
			}
			// save a pointer to dropped entity
			ent->client->pickup_prev = ent->client->pickup;
			// let go
			ent->client->pickup = NULL;
		}
		//else
			//gi.dprintf("can't drop it\n");
		return;
	}
	//gi.dprintf("no pickup\n");

	// find a barrel close to the player's aiming reticle
	while ((e = findclosestreticle(e, ent, 128)) != NULL)
	{
		if (!G_EntIsAlive(e))
			continue;
		if (!visible(ent, e))
			continue;
		if (!infront(ent, e))
			continue;
		if (!e->creator || e->creator != ent)
			continue;
		if (e->mtype != M_BARREL)
			continue;
		// found one--pick it up!
		ent->client->pickup = e;
		// clear pickup_prev if we've picked up the same entity we previously dropped
		if (ent->client->pickup_prev && ent->client->pickup_prev->inuse && ent->client->pickup_prev == e)
			ent->client->pickup_prev = NULL;
		return;
	}

	if (!G_CanUseAbilities(ent, ent->myskills.abilities[EXPLODING_BARREL].current_level, EXPLODING_ARMOR_COST))
		return;
	if (ent->myskills.abilities[EXPLODING_BARREL].disable)
		return;
	if (ent->client->pers.inventory[power_cube_index] < EXPLODING_ARMOR_AMOUNT)
	{
		safe_cprintf(ent, PRINT_HIGH, "You need at least %d power cubes to use this ability.\n", (int)EXPLODING_BARREL_COST);
		return;
	}
	if (ent->num_barrels >= EXPLODING_BARREL_MAX_COUNT)
	{
		safe_cprintf(ent, PRINT_HIGH, "Unable to spawn additional exploding barrels (%d/%d).\n", ent->num_barrels, (int)EXPLODING_BARREL_MAX_COUNT);
		return;
	}

	SpawnExplodingBarrel(ent);
}