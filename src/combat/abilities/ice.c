#include "g_local.h"

//************************************************************************************************
//		ICEBOLT
//************************************************************************************************

void icebolt_remove(edict_t* self)
{
	// remove icebolt entity next frame
	self->think = G_FreeEdict;
	self->nextthink = level.time + FRAMETIME;
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
