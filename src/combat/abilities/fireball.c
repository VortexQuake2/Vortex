#include "g_local.h"

void fireball_explode (edict_t *self, cplane_t *plane)
{
	int		i;
	vec3_t	forward;
	edict_t *e=NULL;

	// don't call this more than once
	if (self->solid == SOLID_NOT)
		return;

	self->solid = SOLID_NOT;

	// burn targets within explosion radius
	while ((e = findradius(e, self->s.origin, self->dmg_radius)) != NULL)
	{
		if (!G_ValidTarget(self, e, true, true))
			continue;
		burn_person(e, self->owner, self->radius_dmg);
	}
		
	for (i=0; i<self->count; i++)
	{
		if (plane)
		{
			VectorCopy(plane->normal, forward);
		}
		else
		{
			VectorNegate(self->velocity, forward);
			VectorNormalize(forward);
		}

		// randomize aiming vector
		forward[YAW] += 0.5 * crandom();
		forward[PITCH] += 0.5 * crandom();

		// create the flame entities
		ThrowFlame(self->owner, self->s.origin, forward, 0, GetRandom(50, 150), self->radius_dmg, GetRandom(1, 3));
	}

	T_RadiusDamage(self, self->owner, self->dmg, NULL, self->dmg_radius, MOD_FIREBALL);

	// create explosion effect
	gi.WriteByte(svc_temp_entity);
	if (self->waterlevel)
		gi.WriteByte(TE_ROCKET_EXPLOSION_WATER);
	else
		gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PHS);

	// remove fireball entity next frame
	self->think = G_FreeEdict;
	self->nextthink = level.time + FRAMETIME;
}

void fireball_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	// remove fireball if owner dies or becomes invalid or if we touch a sky brush
	if (!G_EntIsAlive(ent->owner) || (surf && (surf->flags & SURF_SKY)))
	{
		G_FreeEdict(ent);
		return;
	}

	if (other == ent->owner)
		return;

	//FIXME: this sound is overridden by BecomeExplosion1()'s sound
    gi.sound(ent, CHAN_VOICE, gi.soundindex(va("abilities/largefireimpact%d.wav", GetRandom(1, 3))), 1, ATTN_NORM, 0);
	fireball_explode(ent, plane);
}

void fireball_think (edict_t *self)
{
	int i;
	vec3_t start, forward;

	if (level.time > self->delay || self->waterlevel)
	{
		G_FreeEdict(self);
		return;
	}

	// 0 = black, 8 = grey, 15 = white, 16 = light brown, 20 = brown, 57 = light orange, 66 = orange/red, 73 = maroon
	// 106 = pink, 113 = light blue, 119 = blue, 123 = dark blue, 200 = pale green, 205 = dark green, 209 = bright green
	// 217 = white, 220 = yellow, 226 = orange, 231 = red/orange, 240 = red, 243 = dark blue

	VectorCopy(self->s.origin, start);
	VectorCopy(self->velocity, forward);
	VectorNormalize(forward);

	// create a trail behind the fireball
	for (i=0; i<6; i++)
	{

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_LASER_SPARKS);
		gi.WriteByte(1); // number of sparks
		gi.WritePosition(start);
		gi.WriteDir(forward);
		gi.WriteByte(223); // color
		gi.multicast(start, MULTICAST_PVS);

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_LASER_SPARKS);
		gi.WriteByte(1); // number of sparks
		gi.WritePosition(start);
		gi.WriteDir(forward);
		gi.WriteByte(231); // color
		gi.multicast(start, MULTICAST_PVS);

		VectorMA(start, -16, forward, start);
	}

	self->nextthink = level.time + FRAMETIME;
}

void fire_fireball (edict_t *self, vec3_t start, vec3_t aimdir, int damage, float damage_radius, int speed, int flames, int flame_damage)
{
	edict_t	*fireball;
	vec3_t	dir;
	vec3_t	forward;

	//  entity made a sound, used to alert monsters
	self->lastsound = level.framenum;

	// get aiming angles
	vectoangles(aimdir, dir);
	// get directional vectors
	AngleVectors(dir, forward, NULL, NULL);

	// spawn grenade entity
	fireball = G_Spawn();
	VectorCopy (start, fireball->s.origin);
	fireball->movetype = MOVETYPE_TOSS;//MOVETYPE_FLYMISSILE;
	fireball->clipmask = MASK_SHOT;
	fireball->solid = SOLID_BBOX;
	fireball->s.modelindex = gi.modelindex ("models/objects/flball/tris.md2");
	fireball->owner = self;
	fireball->touch = fireball_touch;
	fireball->think = fireball_think;
	fireball->dmg_radius = damage_radius;
	fireball->dmg = damage;
	fireball->radius_dmg = flame_damage;
	fireball->count = flames;
	fireball->classname = "fireball";
	fireball->delay = level.time + 10.0;
	gi.linkentity (fireball);
	fireball->nextthink = level.time + FRAMETIME;
	VectorCopy(dir, fireball->s.angles);

	// adjust velocity
	VectorScale (aimdir, speed, fireball->velocity);
	fireball->velocity[2] += 150;
}


void icebolt_explode (edict_t *self, cplane_t *plane)
{
	edict_t *e=NULL;

	// chill targets within explosion radius
	while ((e = findradius(e, self->s.origin, self->dmg_radius)) != NULL)
	{
		if (!G_ValidTarget(self, e, true, true))
			continue;
		e->chill_level = self->chill_level;
		e->chill_time = level.time + self->chill_time;
	}

	T_RadiusDamage(self, self->owner, self->dmg, NULL, self->dmg_radius, MOD_ICEBOLT);//CHANGEME--fix MOD
	BecomeTE(self);//CHANGEME--need ice explosion effect
}

void icebolt_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	// remove icebolt if owner dies or becomes invalid or if we touch a sky brush
	if (!G_EntIsAlive(ent->owner) || (surf && (surf->flags & SURF_SKY)))
	{
		G_FreeEdict(ent);
		return;
	}

    gi.sound(ent, CHAN_VOICE, gi.soundindex(va("abilities/blastimpact%d.wav", GetRandom(1, 3))), 1, ATTN_NORM, 0);
	icebolt_explode(ent, plane);
}

void icebolt_think (edict_t *self)
{
	int i;
	vec3_t start, forward;

	if (level.time > self->delay)
	{
		G_FreeEdict(self);
		return;
	}

	// 0 = black, 8 = grey, 15 = white, 16 = light brown, 20 = brown, 57 = light orange, 66 = orange/red, 73 = maroon
	// 106 = pink, 113 = light blue, 119 = blue, 123 = dark blue, 200 = pale green, 205 = dark green, 209 = bright green
	// 217 = white, 220 = yellow, 226 = orange, 231 = red/orange, 240 = red, 243 = dark blue

	VectorCopy(self->s.origin, start);
	VectorCopy(self->velocity, forward);
	VectorNormalize(forward);

	// create a trail behind the fireball
	for (i=0; i<6; i++)
	{

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_LASER_SPARKS);
		gi.WriteByte(1); // number of sparks
		gi.WritePosition(start);
		gi.WriteDir(forward);
		gi.WriteByte(113); // color
		gi.multicast(start, MULTICAST_PVS);

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_LASER_SPARKS);
		gi.WriteByte(1); // number of sparks
		gi.WritePosition(start);
		gi.WriteDir(forward);
		gi.WriteByte(119); // color
		gi.multicast(start, MULTICAST_PVS);

		VectorMA(start, -16, forward, start);
	}

	self->nextthink = level.time + FRAMETIME;
}

void fire_icebolt (edict_t *self, vec3_t start, vec3_t aimdir, int damage, float damage_radius, int speed, int chillLevel, float chillDuration)
{
	edict_t	*icebolt;
	vec3_t	dir;
	vec3_t	forward;

	//  entity made a sound, used to alert monsters
	self->lastsound = level.framenum;

	// get aiming angles
	vectoangles(aimdir, dir);
	// get directional vectors
	AngleVectors(dir, forward, NULL, NULL);

	// spawn grenade entity
	icebolt = G_Spawn();
	VectorCopy (start, icebolt->s.origin);
	icebolt->s.effects |= EF_COLOR_SHELL;
	icebolt->s.renderfx |= RF_SHELL_BLUE;
	icebolt->movetype = MOVETYPE_FLYMISSILE;
	icebolt->clipmask = MASK_SHOT;
	icebolt->solid = SOLID_BBOX;
	icebolt->s.modelindex = gi.modelindex ("models/objects/flball/tris.md2");
	icebolt->owner = self;
	icebolt->touch = icebolt_touch;
	icebolt->think = icebolt_think;
	icebolt->dmg_radius = damage_radius;
	icebolt->dmg = damage;
	icebolt->chill_level = chillLevel;
	icebolt->chill_time = chillDuration;
	icebolt->classname = "icebolt";
	icebolt->delay = level.time + 10.0;
	gi.linkentity (icebolt);
	icebolt->nextthink = level.time + FRAMETIME;
	VectorCopy(dir, icebolt->s.angles);

	// adjust velocity
	VectorScale (aimdir, speed, icebolt->velocity);
}

// note: there are only 5 talent levels, so addon values will be higher
//Talent: Ice Bolt
void Cmd_IceBolt_f (edict_t *ent, float skill_mult, float cost_mult)
{
    int slvl = vrx_get_talent_level(ent, TALENT_ICE_BOLT);
	int		damage, fblvl, speed, cost=ICEBOLT_COST*cost_mult;
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
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 8,  ent->viewheight-8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	fire_icebolt(ent, start, forward, damage, radius, speed, 2*slvl, chill_duration);

	ent->client->ability_delay = level.time + ICEBOLT_DELAY/* * cost_mult*/;
	ent->client->pers.inventory[power_cube_index] -= cost;

	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_TELEPORT_EFFECT);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

    gi.sound(ent, CHAN_ITEM, gi.soundindex("abilities/coldcast.wav"), 1, ATTN_NORM, 0);
}

void Cmd_Fireball_f (edict_t *ent, float skill_mult, float cost_mult)
{
	int		slvl = ent->myskills.abilities[FIREBALL].current_level;
	int		damage, speed, flames, flamedmg, cost=FIREBALL_COST*cost_mult;
	float	radius;
	vec3_t	forward, right, start, offset;

	if (!V_CanUseAbilities(ent, FIREBALL, cost, true))
		return;

	if (ent->waterlevel > 1)
		return;

	damage = (FIREBALL_INITIAL_DAMAGE + FIREBALL_ADDON_DAMAGE * slvl) * skill_mult;
	radius = FIREBALL_INITIAL_RADIUS + FIREBALL_ADDON_RADIUS * slvl;
	speed = FIREBALL_INITIAL_SPEED + FIREBALL_ADDON_SPEED * slvl;
	flames = FIREBALL_INITIAL_FLAMES + FIREBALL_ADDON_FLAMES * slvl;
	flamedmg = (FIREBALL_INITIAL_FLAMEDMG + FIREBALL_ADDON_FLAMEDMG * slvl) * skill_mult;

	// get starting position and forward vector
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 8,  ent->viewheight-8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	fire_fireball(ent, start, forward, damage, radius, speed, flames, flamedmg);

	ent->client->ability_delay = level.time + FIREBALL_DELAY/* * cost_mult*/;
	ent->client->pers.inventory[power_cube_index] -= cost;

	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_TELEPORT_EFFECT);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

    gi.sound(ent, CHAN_ITEM, gi.soundindex("abilities/firecast.wav"), 1, ATTN_NORM, 0);
}
