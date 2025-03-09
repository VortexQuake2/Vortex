#include "g_local.h"

//************************************************************************************************
//		ICEBOLT
//************************************************************************************************

void icebolt_remove(edict_t* self)
{
	// remove icebolt entity next frame
	self->think = G_FreeEdict;
	self->nextthink = level.time + FRAMETIME;
	self->solid = SOLID_NOT;// to prevent touch function from being called again, delaying removal
}

void icebolt_think_remove(edict_t* self)
{
	gi.sound(self, CHAN_VOICE, gi.soundindex("weapons/coldimpact1.wav"), 1, ATTN_NORM, 0);
	self->svflags |= SVF_NOCLIENT;
	icebolt_remove(self);
}

void ice_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, vec3_t point)
{
	G_FreeEdict(self);
}

void ThrowIceChunks(edict_t* self, char* modelname, float speed, vec3_t origin)
{
	edict_t* chunk;
	vec3_t	v;

	chunk = G_Spawn();
	VectorCopy(origin, chunk->s.origin);
	gi.setmodel(chunk, modelname);
	v[0] = 100 * crandom();
	v[1] = 100 * crandom();
	v[2] = 100 + 100 * crandom();
	VectorMA(self->velocity, speed, v, chunk->velocity);
	chunk->movetype = MOVETYPE_BOUNCE;
	chunk->solid = SOLID_NOT;
	chunk->avelocity[0] = random() * 600;
	chunk->avelocity[1] = random() * 600;
	chunk->avelocity[2] = random() * 600;
	chunk->think = G_FreeEdict;
	chunk->nextthink = level.time + GetRandom(1, 3);
	chunk->s.frame = 0;
	chunk->flags = 0;
	chunk->classname = "ice";
	chunk->takedamage = DAMAGE_YES;
	chunk->die = ice_die;
	chunk->s.effects |= EF_HALF_DAMAGE;
	gi.linkentity(chunk);
}

void chill_target(edict_t* target, int chill_level, float duration)
{
	target->chill_level = chill_level;
	target->chill_time = level.time + duration;
	if (random() > 0.5)
		gi.sound(target, CHAN_BODY, gi.soundindex("abilities/blue1.wav"), 1, ATTN_NORM, 0);
	else
		gi.sound(target, CHAN_BODY, gi.soundindex("abilities/blue3.wav"), 1, ATTN_NORM, 0);
}

qboolean curse_add(edict_t* target, edict_t* caster, int type, int curse_level, float duration);
void icebolt_explode(edict_t* self, cplane_t* plane)
{
	int		n;
	//vec3_t	org, start;

	edict_t* e = NULL;

	// chill targets within explosion radius
	while ((e = findradius(e, self->s.origin, self->dmg_radius)) != NULL)
	{
		if (!G_ValidTarget(self, e, true, true))
			continue;
		e->chill_level = self->chill_level;
		e->chill_time = level.time + self->chill_time;

		//if (self->owner && self->owner->client)//TESTING!!!!!!!!!!!
		//	curse_add(e, self->owner, CURSE_FROZEN, 0, 3.0);
	}

	// damage nearby entities
	T_RadiusDamage(self, self->owner, self->dmg, NULL, self->dmg_radius, MOD_ICEBOLT);//CHANGEME--fix MOD

	if (vrx_spawn_nonessential_ent(self->s.origin))
	{
		// throw chunks of ice
		n = randomMT() % 5;
		while (n--)
			ThrowIceChunks(self, "models/objects/debris2/tris.md2", 2, self->s.origin);
	}
	// particle effects at impact site
	SpawnDamage(TE_ELECTRIC_SPARKS, self->s.origin, plane->normal);

	icebolt_remove(self);
	//self->think = icebolt_think_remove;
	self->exploded = true;
}

void icebolt_touch(edict_t* ent, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	if (ent->exploded)
		return;

	// remove icebolt if owner dies or becomes invalid or if we touch a sky brush
	if (!G_EntIsAlive(ent->owner) || (surf && (surf->flags & SURF_SKY)))
	{
		icebolt_remove(ent);
		return;
	}

	if (other == ent->owner)
		return;

	//gi.sound(ent, CHAN_VOICE, gi.soundindex(va("weapons/coldimpact%d.wav", GetRandom(1, 3))), 1, ATTN_NORM, 0);

	//gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/acid.wav"), 1, ATTN_NORM, 0);
	gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/coldimpact1.wav"), 1, ATTN_NORM, 0);
	//gi.sound(ent, CHAN_VOICE, gi.soundindex("misc/needlite.wav"), 1, ATTN_NORM, 0);
	icebolt_explode(ent, plane);

}

void icebolt_effects(vec3_t org, int num, float radius)
{
	int		i;
	vec3_t	start;

	// 0 = black, 8 = grey, 15 = white, 16 = light brown, 20 = brown, 57 = light orange, 66 = orange/red, 73 = maroon
	// 106 = pink, 113 = light blue, 119 = blue, 123 = dark blue, 200 = pale green, 205 = dark green, 209 = bright green
	// 217 = white, 220 = yellow, 226 = orange, 231 = red/orange, 240 = red, 243 = dark blue

	for (i = 0; i < num; i++)
	{
		//VectorCopy(self->s.origin, start);
		VectorCopy(org, start);
		start[0] += crandom() * GetRandom(4, (int)radius);
		start[1] += crandom() * GetRandom(4, (int)radius);
		//start[2] += crandom() * GetRandom(0, (int) self->dmg_radius);

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_LASER_SPARKS);
		gi.WriteByte(1); // number of sparks
		gi.WritePosition(start);
		gi.WriteDir(vec3_origin);
		//gi.WriteByte(GetRandom(200, 209)); // color
		if (random() <= 0.33)
			gi.WriteByte(217);
		else
			gi.WriteByte(113);
		gi.multicast(start, MULTICAST_PVS);
	}
}

void icebolt_think(edict_t* self)
{
	//int i;
	//vec3_t start, end, forward;

	if (level.time > self->delay)
	{
		G_FreeEdict(self);
		return;
	}

	//VectorCopy(self->s.origin, start);
	//VectorCopy(self->velocity, forward);
	//VectorNormalize(forward);
	//VectorMA(start, -24, forward, end);
	//icebolt_effects(self->s.origin, 20, 16);
	// extra particle effects
	G_DrawSparks(self->s.origin, self->s.origin, 113, 217, 4, 1, 0, 0);

	self->nextthink = level.time + FRAMETIME;
}

void fire_icebolt(edict_t* self, vec3_t start, vec3_t aimdir, int damage, float damage_radius, int speed, int chillLevel, float chillDuration)
{
	edict_t* icebolt;
	vec3_t	dir;
	vec3_t	forward;

	//  entity made a sound, used to alert monsters
	self->lastsound = level.framenum;

	// get aiming angles
	vectoangles(aimdir, dir);
	// get directional vectors
	AngleVectors(dir, forward, NULL, NULL);

	// spawn icebolt entity
	icebolt = G_Spawn();
	VectorCopy(start, icebolt->s.origin);
	icebolt->s.effects |= (EF_HALF_DAMAGE | EF_FLAG2);//EF_COLOR_SHELL;
	//icebolt->s.renderfx |= RF_SHELL_BLUE;
	icebolt->movetype = MOVETYPE_FLYMISSILE;
	icebolt->clipmask = MASK_SHOT;
	icebolt->solid = SOLID_BBOX;
	icebolt->s.modelindex = gi.modelindex("models/proj/proj_drole/tris.md2");
	icebolt->s.skinnum = 1;
	icebolt->s.frame = 4;
	icebolt->owner = self;
	icebolt->activator = G_GetSummoner(self); // needed for OnSameTeam() checks
	icebolt->touch = icebolt_touch;
	icebolt->think = icebolt_think;
	icebolt->dmg_radius = damage_radius;
	icebolt->dmg = damage;
	icebolt->chill_level = chillLevel;
	icebolt->chill_time = chillDuration;
	icebolt->classname = "icebolt";
	icebolt->delay = level.time + 10.0; // timeout
	gi.linkentity(icebolt);
	icebolt->nextthink = level.time + FRAMETIME;
	// set angles
	VectorCopy(dir, icebolt->s.angles);
	// adjust angular velocity (make it roll)
	VectorSet(icebolt->avelocity, GetRandom(300, 1000), 0, 0);
	// adjust velocity (movement)
	VectorScale(aimdir, speed, icebolt->velocity);
}

// note: there are only 5 talent levels, so addon values will be higher
//Talent: Ice Bolt
void Cmd_IceBolt_f(edict_t* ent, float skill_mult, float cost_mult)
{
	int slvl = vrx_get_talent_level(ent, TALENT_ICE_BOLT);
	int		damage, fblvl, speed, cost = ICEBOLT_COST * cost_mult;
	float	radius, chill_duration;
	vec3_t	forward, right, start, offset;

	// you need to have fireball upgraded to use icebolt
	if (!V_CanUseAbilities(ent, FIREBALL, cost, true))
		return;

	// current fireball level
	fblvl = ent->myskills.abilities[FIREBALL].current_level;

	// talent isn't upgraded
	if (slvl < 1)
	{
		safe_cprintf(ent, PRINT_HIGH, "You must upgrade ice bolt before you can use it.\n");
		return;
	}

	chill_duration = (ICEBOLT_INITIAL_CHILL_DURATION + ICEBOLT_ADDON_CHILL_DURATION * slvl) * skill_mult;
	damage = (FIREBALL_INITIAL_DAMAGE + FIREBALL_ADDON_DAMAGE * fblvl) * skill_mult;
	radius = FIREBALL_INITIAL_RADIUS + FIREBALL_ADDON_RADIUS * fblvl;
	speed = FIREBALL_INITIAL_SPEED + FIREBALL_ADDON_SPEED * fblvl;


	//gi.dprintf("dmg:%d slvl:%d skill_mult:%f chill_duration:%f\n",damage,slvl,skill_mult,chill_duration);

	// get starting position and forward vector
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	fire_icebolt(ent, start, forward, damage, radius, speed, 2 * slvl, chill_duration);

	ent->client->ability_delay = level.time + ICEBOLT_DELAY/* * cost_mult*/;
	ent->client->pers.inventory[power_cube_index] -= cost;

	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_TELEPORT_EFFECT);
	gi.WritePosition(ent->s.origin);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	gi.sound(ent, CHAN_ITEM, gi.soundindex("abilities/coldcast.wav"), 1, ATTN_NORM, 0);
}

//************************************************************************************************
//		FROZEN ORB
//************************************************************************************************

void iceshard_touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	if (!G_EntIsAlive(self->owner) || (surf && (surf->flags & SURF_SKY)))
	{
		icebolt_remove(self);
		return;
	}

	if (G_ValidTarget(self, other, false, true))
	{
		T_Damage(other, self, self->creator, self->velocity, self->s.origin, plane->normal, self->dmg, 0, 0, MOD_ICEBOLT);//FIXME (MOD)
		chill_target(other, self->chill_level, self->chill_time);

		//gi.sound(other, CHAN_WEAPON, gi.soundindex("misc/fhit3.wav"), 1, ATTN_NORM, 0);
	}
	gi.sound(self, CHAN_VOICE, gi.soundindex("weapons/coldimpact1.wav"), 1, ATTN_NORM, 0);
	icebolt_remove(self);
}

void fire_iceshard(edict_t* self, vec3_t start, vec3_t dir, float speed, int damage, int chillLevel, float chillDuration)
{
	edict_t* bolt;

	//gi.dprintf("%d: %s\n", (int)level.framenum, __func__);
	// create bolt
	bolt = G_Spawn();
	bolt->s.modelindex = gi.modelindex("models/objects/spike/tris.md2");
	bolt->s.effects |= (EF_HALF_DAMAGE | EF_FLAG2);
	VectorCopy(start, bolt->s.origin);
	VectorCopy(start, bolt->s.old_origin);
	vectoangles(dir, bolt->s.angles);
	VectorScale(dir, speed, bolt->velocity);
	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->clipmask = MASK_SHOT;
	bolt->solid = SOLID_BBOX;
	bolt->owner = self;
	bolt->creator = self;
	bolt->touch = iceshard_touch;
	bolt->nextthink = level.time + 5;
	bolt->think = G_FreeEdict;
	bolt->dmg = damage;
	bolt->chill_level = chillLevel;
	bolt->chill_time = chillDuration;
	bolt->classname = "ice shard";
	gi.linkentity(bolt);

	// cloak a player-owned ice shard in PvM if there are too many entities nearby
	if (pvm->value && G_GetClient(self) && !vrx_spawn_nonessential_ent(bolt->s.origin))
		bolt->svflags |= SVF_NOCLIENT;
}

void frozenorb_attack(edict_t* self, int num_shards)
{
	float turn_degrees = 360 / num_shards;
	vec3_t forward;

	for (int i = 0; i < num_shards; i++)
	{
		self->s.angles[YAW] += turn_degrees;
		AngleCheck(&self->s.angles[YAW]);
		AngleVectors(self->s.angles, forward, NULL, NULL);
		fire_iceshard(self->owner, self->s.origin, forward, 650, self->dmg, 10, 10);//FIXME: speed & chill values
	}
}

void frozenorb_think(edict_t* self)
{
	//int i;
	vec3_t start, forward;

	if (level.time > self->delay)
	{
		frozenorb_attack(self, 12);
		G_FreeEdict(self);
		return;
	}

	if (level.time > self->monsterinfo.attack_finished)
	{
		frozenorb_attack(self, 8);
		self->monsterinfo.attack_finished = level.time + 0.3;//SPIKEGRENADE_TURN_DELAY;
	}

	//self->s.angles[YAW] += SPIKEGRENADE_TURN_DEGREES;
	//AngleCheck(&self->s.angles[YAW]);
	
	//G_EntMidPoint(self, start);
	//AngleVectors(self->s.angles, forward, NULL, NULL);
	//fire_iceshard(self->owner, start, forward, 650, self->dmg, 10, 10);

	// extra particle effects
	G_DrawSparks(self->s.origin, self->s.origin, 113, 217, 4, 1, 0, 0);

	self->nextthink = level.time + FRAMETIME;
}

void frozenorb_touch(edict_t* ent, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	if (ent->exploded)
		return;

	// only stop on walls
	if (other->takedamage)
		return;

	// remove icebolt if owner dies or becomes invalid or if we touch a sky brush
	if (!G_EntIsAlive(ent->owner) || (surf && (surf->flags & SURF_SKY)))
	{
		icebolt_remove(ent);
		return;
	}

	gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/coldimpact1.wav"), 1, ATTN_NORM, 0);
	
	if (vrx_spawn_nonessential_ent(ent->s.origin))
	{
		// throw chunks of ice
		int n = randomMT() % 5;
		while (n--)
			ThrowIceChunks(ent, "models/objects/debris2/tris.md2", 2, ent->s.origin);
	}
	// particle effects at impact site
	SpawnDamage(TE_ELECTRIC_SPARKS, ent->s.origin, plane->normal);

	frozenorb_attack(ent, 12);
	icebolt_remove(ent);
	ent->exploded = true;

}

void fire_frozenorb(edict_t* self, vec3_t start, vec3_t aimdir, int damage, int chillLevel, float chillDuration)
{
	edict_t* orb;
	vec3_t	dir;
	vec3_t	forward;

	//  entity made a sound, used to alert monsters
	self->lastsound = level.framenum;

	// get aiming angles
	vectoangles(aimdir, dir);
	// get directional vectors
	AngleVectors(dir, forward, NULL, NULL);

	// spawn icebolt entity
	orb = G_Spawn();
	VectorCopy(start, orb->s.origin);
	orb->s.effects |= (EF_HALF_DAMAGE | EF_FLAG2);
	orb->movetype = MOVETYPE_FLYMISSILE;
	orb->clipmask = MASK_SOLID; // stop on walls
	orb->solid = SOLID_BBOX;
	orb->s.modelindex = gi.modelindex("models/pbullet/tris.md2");
	orb->owner = self;
	orb->activator = G_GetSummoner(self); // needed for OnSameTeam() checks
	orb->touch = frozenorb_touch;
	orb->think = frozenorb_think;
	orb->dmg = damage;
	orb->chill_level = chillLevel;
	orb->chill_time = chillDuration;
	orb->classname = "frozen orb";
	orb->delay = level.time + FROZEN_ORB_DURATION; // timeout
	gi.linkentity(orb);
	orb->nextthink = level.time + FRAMETIME;
	// set angles
	VectorCopy(dir, orb->s.angles);
	orb->s.angles[PITCH] = 0;
	// adjust angular velocity (make it roll)
	//VectorSet(orb->avelocity, GetRandom(300, 1000), 0, 0);
	VectorSet(orb->avelocity, 0, GetRandom(300, 1000), 0);
	// adjust velocity (movement)
	VectorScale(aimdir, FROZEN_ORB_SPEED, orb->velocity);
}

void Cmd_FrozenOrb_f(edict_t* ent, float skill_mult, float cost_mult)
{
	vec3_t	forward, right, start, offset;

	int cost = FROZEN_ORB_COST * cost_mult;

	if (!V_CanUseAbilities(ent, FROZEN_ORB, cost, true))
		return;

	int	skill_level = ent->myskills.abilities[FROZEN_ORB].current_level;
	int damage = (FROZEN_ORB_INITIAL_DAMAGE + FROZEN_ORB_ADDON_DAMAGE * skill_level) * (skill_mult * vrx_get_synergy_mult(ent, FROZEN_ORB));
	float chill_duration = (FROZEN_ORB_INITIAL_CHILL + FROZEN_ORB_ADDON_CHILL * skill_level) * skill_mult;

	// get starting position and forward vector
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	fire_frozenorb(ent, start, forward, damage, (2 * skill_level), chill_duration);

	ent->client->ability_delay = level.time + FROZEN_ORB_DELAY;
	ent->client->pers.inventory[power_cube_index] -= cost;

	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_TELEPORT_EFFECT);
	gi.WritePosition(ent->s.origin);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	gi.sound(ent, CHAN_ITEM, gi.soundindex("abilities/coldcast.wav"), 1, ATTN_NORM, 0);
}