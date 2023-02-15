#include "g_local.h"
#include "../../gamemodes/ctf.h"

void armor_remove(edict_t* self, qboolean refund)
{
	if (!self || !self->inuse || self->deadflag == DEAD_DEAD)
		return;

	// reduce armor count
	if (self->owner && self->owner->inuse && self->owner->client)
	{
		// reduce count
		self->owner->num_armor--;
		// refund player
		if (refund)
			self->owner->client->pers.inventory[power_cube_index] += EXPLODING_ARMOR_COST;
		// remove from HUD
		layout_remove_tracked_entity(&self->owner->client->layout, self);
	}

	// create explosion effect
	gi.WriteByte(svc_temp_entity);
	if (self->waterlevel)
		gi.WriteByte(TE_ROCKET_EXPLOSION_WATER);
	else
		gi.WriteByte(TE_ROCKET_EXPLOSION);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PHS);

	self->think = G_FreeEdict;
	self->nextthink = level.time + FRAMETIME;
	self->svflags |= SVF_NOCLIENT;
	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT;
	self->deadflag = DEAD_DEAD;
}

void RemoveExplodingArmor (edict_t *ent)
{
	edict_t *e=NULL;

	while((e = G_Find(e, FOFS(classname), "exploding_armor")) != NULL)
	{
		if (e && e->inuse && (e->owner == ent))
		{
			armor_remove(e, false);
		}
	}

	// reset armor counter
	ent->num_armor = 0;
}

void DetonateArmor (edict_t *self)
{
	int	i;

	if (self->solid == SOLID_NOT)
		return; // already flagged for removal

	armor_remove(self, false);

	//safe_cprintf(self->owner, PRINT_HIGH, "Exploding armor did %d damage! (%d/%d)\n", self->dmg, self->owner->num_armor, (int)EXPLODING_ARMOR_MAX_COUNT);

	T_RadiusDamage(self, self->owner, self->dmg, NULL, self->dmg_radius, MOD_EXPLODINGARMOR);
	// throw debris around
	if (gi.pointcontents(self->s.origin) == 0) 
	{
		for (i = 0;i < GetRandom(4, 6);i++)
			ThrowShrapnel(self, "models/objects/debris2/tris.md2", 3, self->s.origin, self->dmg, MOD_EXPLODINGARMOR);
		for(i=0;i<GetRandom(2, 3);i++)
			ThrowShrapnel(self, "models/objects/debris1/tris.md2", 2, self->s.origin, self->dmg, MOD_EXPLODINGARMOR);
	}
}

void explodingarmor_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (G_EntIsAlive(other) && !OnSameTeam(self, other))
		DetonateArmor(self);
}

qboolean NearbyEnemy (edict_t *self, float radius)
{
	edict_t *e=NULL;

	while ((e = findradius(e, self->s.origin, radius)) != NULL)
	{
		if (!G_ValidTarget(self, e, true))
			continue;
		return true;
	}
	return false;
}

void explodingarmor_effects (edict_t *self)
{
	self->s.effects = self->s.renderfx = 0;

	self->s.effects |= EF_COLOR_SHELL;
	if (self->owner->teamnum == RED_TEAM || ffa->value)
		self->s.renderfx |= RF_SHELL_RED;
	else
		self->s.renderfx |= RF_SHELL_BLUE;

	// flash white prior to explosion
	if (level.time >= self->delay - 10 && level.framenum > self->count)
	{
		self->s.renderfx |= RF_SHELL_RED|RF_SHELL_BLUE|RF_SHELL_YELLOW;
		self->count = level.framenum + (self->delay - level.time) + 1;
		//gi.dprintf("%d %d\n", level.framenum, self->count);
	}
}

void explodingarmor_think (edict_t *self)
{
	float dist=EXPLODING_ARMOR_DETECTION;

	// must have an owner
	if (!G_EntIsAlive(self->owner)) 
	{
		// reduce armor count
		if (self->owner && self->owner->inuse)
			self->owner->num_armor--;

		G_FreeEdict(self);
		return;
	}

	// auto-remove from solid objects
	if (gi.pointcontents(self->s.origin) & CONTENTS_SOLID)
	{
		// reduce armor count
		if (self->owner && self->owner->inuse)
			self->owner->num_armor--;

		gi.dprintf("INFO: Removed exploding armor from solid object.\n");
		safe_cprintf(self->owner, PRINT_HIGH, "Your armor was removed from a solid object.\n");
		G_FreeEdict(self);
		return;
	}

	// blow up if enemy gets too close
	if (NearbyEnemy(self, dist))
	{
		DetonateArmor(self);
		return;
	}

	// blow up if time-out is reached
	if (level.time > self->delay) 
	{
		DetonateArmor(self);
		return;
	}

	explodingarmor_effects(self);

	self->nextthink = level.time + FRAMETIME;
}

void SpawnExplodingArmor (edict_t *ent, int time)
{
	float	value;
	vec3_t	forward, right, start, offset;
	edict_t *armor;

	value = 1+0.4*ent->myskills.abilities[EXPLODING_ARMOR].current_level;

	if (time < 2)
		time = 2;
	if (time > 120)
		time = 120;

	// create basic entity
	armor = G_Spawn();
	armor->owner = ent;
	armor->movetype = MOVETYPE_TOSS;
	armor->solid = SOLID_TRIGGER;
	armor->s.modelindex = gi.modelindex("models/items/armor/body/tris.md2");
	armor->classname = "exploding_armor";
	armor->mtype = M_ARMOR;
	armor->s.effects |= EF_ROTATE;//(EF_ROTATE|EF_COLOR_SHELL);
	//armor->s.renderfx |= RF_SHELL_RED;	
	armor->s.origin[2] += 32;
	VectorSet(armor->mins,-16,-16,-16);
	VectorSet(armor->maxs, 16, 16, 16);
	armor->touch = explodingarmor_touch;
	armor->think = explodingarmor_think;
	armor->dmg = EXPLODING_ARMOR_DMG_BASE + EXPLODING_ARMOR_DMG_ADDON * ent->myskills.abilities[EXPLODING_ARMOR].current_level;
	armor->dmg_radius = 100 + armor->dmg/3;
	
	if (armor->dmg_radius > EXPLODING_ARMOR_MAX_RADIUS)
		armor->dmg_radius = EXPLODING_ARMOR_MAX_RADIUS;

	armor->delay = level.time + time;
	armor->nextthink = level.time + FRAMETIME;

	// get view origin
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 8,  ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	if (!G_GetSpawnLocation(ent, 64, armor->mins, armor->maxs, start, NULL, PROJECT_HITBOX_FAR, true))
	{
		G_FreeEdict(armor);
		return;
	}
	/*
	// don't spawn in a wall
	if (gi.pointcontents(armor->s.origin) & CONTENTS_SOLID)
	{
		G_FreeEdict(armor);
		return;
	}
	*/

	// move armor into position
	//VectorMA(start, 48, forward, armor->s.origin);
	VectorCopy(start, armor->s.origin);
	VectorCopy(start, armor->s.old_origin);
	gi.linkentity(armor);

	// toss it forward
	VectorScale(forward, 300, armor->velocity);
	armor->velocity[2] += 200;
	//VectorClear(armor->avelocity);
	VectorCopy(ent->s.angles, armor->s.angles);
	armor->s.angles[PITCH] = 0;
	armor->s.angles[ROLL] = 0;

	layout_add_tracked_entity(&ent->client->layout, armor); // add to HUD
	ent->client->pers.inventory[body_armor_index] -= EXPLODING_ARMOR_AMOUNT;
	ent->client->pers.inventory[power_cube_index] -= EXPLODING_ARMOR_COST;
	ent->client->ability_delay = level.time + EXPLODING_ARMOR_DELAY;
	
	ent->num_armor++; // 3.5 keep track of number of armor bombs

	safe_cprintf(ent, PRINT_HIGH, "Your armor will detonate in %d seconds. Move away! (%d/%d)\n", 
		time, ent->num_armor, (int)EXPLODING_ARMOR_MAX_COUNT);

	// calling entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;
}

void Cmd_ExplodingArmor_f (edict_t *ent)
{
	if (debuginfo->value)
		gi.dprintf("DEBUG: Cmd_ExplodingArmor_f()\n");

	if (Q_strcasecmp (gi.args(), "remove") == 0)
	{
		RemoveExplodingArmor(ent);
		safe_cprintf(ent, PRINT_HIGH, "All armor bombs removed.\n");
		return;
	}

	if (!G_CanUseAbilities(ent, ent->myskills.abilities[EXPLODING_ARMOR].current_level, EXPLODING_ARMOR_COST))
		return;
	if (ent->myskills.abilities[EXPLODING_ARMOR].disable)
		return;
	if (ent->client->pers.inventory[body_armor_index] < EXPLODING_ARMOR_AMOUNT)
	{
		safe_cprintf(ent, PRINT_HIGH, "You need at least %d armor to use this ability.\n", (int)EXPLODING_ARMOR_AMOUNT);
		return;
	}
	if (ent->num_armor >= EXPLODING_ARMOR_MAX_COUNT)
	{
		safe_cprintf(ent, PRINT_HIGH, "Maximum count of %d reached.\n", (int)EXPLODING_ARMOR_MAX_COUNT);
		return;
	}

	SpawnExplodingArmor(ent, atoi(gi.args()));
}
