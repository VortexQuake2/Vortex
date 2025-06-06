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
		vrx_clear_pickup_ent(cl, self);
		// remove from HUD
		layout_remove_tracked_entity(&cl->layout, self);
	}
	AI_EnemyRemoved(self);
	self->think = BecomeExplosion1;
	self->nextthink = level.time + FRAMETIME;
	self->svflags |= SVF_NOCLIENT;
	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT;
	self->deadflag = DEAD_DEAD;
}

void barrel_think(edict_t* self)
{
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

	vrx_set_pickup_owner(self); // sets owner when this entity is picked up, making it non-solid to the player

	// if position has been updated, check for ground entity
	// note: this is checked every frame if using MOVETYPE_STEP, whereas MOVETYPE_TOSS may require gi.linkentity to be run on velocity/position changes
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
	//VectorMA(self->absmin, 0.5, self->size, start);
	// small chunks
	spd = 2 * self->dmg / 200;
	for (i = 0;i < GetRandom(4, 6);i++)
	{
		// randomize starting origin of shrapnel, but keep it inside the bbox of the barrel
		// note: the small adjustments are to accommodate the bbox of the shrapnel entity
		org[0] = start[0] + crandom() * (self->size[0]-8);
		org[1] = start[1] + crandom() * (self->size[1]-8);
		org[2] = start[2] + GetRandom(2, 38);
		ThrowShrapnel(self, "models/objects/debris2/tris.md2", spd, org, self->dmg, MOD_SHRAPNEL);
	}
	// big chunks
	spd = 1.5 * (float)self->dmg / 200.0;
	for (i = 0;i < GetRandom(2, 3);i++)
	{
		org[0] = start[0] + crandom() * (self->size[0]-8);
		org[1] = start[1] + crandom() * (self->size[1]-8);
		org[2] = start[2] + GetRandom(2, 38);
		ThrowShrapnel(self, "models/objects/debris1/tris.md2", spd, org, self->dmg, MOD_SHRAPNEL);
	}
	
	vec3_t adjusted_absmin;
	VectorCopy(self->absmin, adjusted_absmin);
	// the shrapnel has a bbox, so we need to adjust for that
	adjusted_absmin[0] += 8;
	adjusted_absmin[1] += 8;
	adjusted_absmin[2] += 2;
	// bottom corners
	spd = 1.75 * (float)self->dmg / 200.0;
	VectorCopy(adjusted_absmin, org);
	ThrowShrapnel(self, "models/objects/debris3/tris.md2", spd, org, self->dmg, MOD_SHRAPNEL);
	VectorCopy(adjusted_absmin, org);
	org[0] += self->size[0];
	ThrowShrapnel(self, "models/objects/debris3/tris.md2", spd, org, self->dmg, MOD_SHRAPNEL);
	VectorCopy(adjusted_absmin, org);
	org[1] += self->size[1];
	ThrowShrapnel(self, "models/objects/debris3/tris.md2", spd, org, self->dmg, MOD_SHRAPNEL);
	VectorCopy(adjusted_absmin, org);
	org[0] += self->size[0];
	org[1] += self->size[1];
	ThrowShrapnel(self, "models/objects/debris3/tris.md2", spd, org, self->dmg, MOD_SHRAPNEL);
}

void barrel_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, vec3_t point)
{
	BarrelDetonate(self);
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
	barrel->monsterinfo.touchdown = M_Touchdown;
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
	barrel->radius_dmg = barrel->dmg; // for bot AI hazard detection
	barrel->think = barrel_think;//M_droptofloor;
	barrel->nextthink = level.time + FRAMETIME;
	gi.linkentity(barrel);

	if (!vrx_position_player_summonable(ent, barrel, 80))// move barrel into position in front of us
		return;

	layout_add_tracked_entity(&ent->client->layout, barrel); // add to HUD
	AI_EnemyAdded(barrel);
	// pick it up!
	ent->client->pickup = barrel;

	ent->num_barrels++;
	ent->client->pers.inventory[power_cube_index] -= barrel->monsterinfo.cost;
	safe_cprintf(ent, PRINT_HIGH, "Spawned exploding barrel (%d/%d)\n", ent->num_barrels, (int)EXPLODING_BARREL_MAX_COUNT);
}
//PM_MonsterHasPilot

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

void Cmd_ExplodingBarrel_f(edict_t* ent)
{
	if (Q_strcasecmp(gi.args(), "remove") == 0)
	{
		RemoveExplodingBarrels(ent, true);
		safe_cprintf(ent, PRINT_HIGH, "All exploding barrels removed.\n");
		return;
	}

	if (vrx_toggle_pickup(ent, M_BARREL, 128)) // search for entity to pick up, or drop the one we're holding
		return;

	//if (!G_CanUseAbilities(ent, ent->myskills.abilities[EXPLODING_BARREL].current_level, EXPLODING_ARMOR_COST))
	//	return;
	if (!V_CanUseAbilities(ent, EXPLODING_BARREL, EXPLODING_BARREL_COST, true))
		return;
	//if (ent->myskills.abilities[EXPLODING_BARREL].disable)
	//	return;
	//if (ent->client->pers.inventory[power_cube_index] < EXPLODING_ARMOR_AMOUNT)
	//{
	//	safe_cprintf(ent, PRINT_HIGH, "You need at least %d power cubes to use this ability.\n", (int)EXPLODING_BARREL_COST);
	//	return;
	//}
	//if (ent->num_barrels >= EXPLODING_BARREL_MAX_COUNT)
	//{
	//	safe_cprintf(ent, PRINT_HIGH, "Unable to spawn additional exploding barrels (%d/%d).\n", ent->num_barrels, (int)EXPLODING_BARREL_MAX_COUNT);
	//	return;
	//}

	SpawnExplodingBarrel(ent);
	if (ent->ai.is_bot)
		vrx_toggle_pickup(ent, M_BARREL, 128); // toss the barrel
}