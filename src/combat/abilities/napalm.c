#include "g_local.h"
#include "../../gamemodes/ctf.h"

void bfire_think (edict_t *self)
{
	if (self->s.frame > 3)
		G_RunFrames(self, 4, 15, false); // burning frames
	else
		self->s.frame++; // ignite frames

	if (!G_EntIsAlive(self->owner) || (self->delay < level.time) || self->waterlevel) 
	{
		G_FreeEdict(self);
		return;
	}
	self->nextthink = level.time + FRAMETIME;
}

void bfire_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (!G_EntExists(other) || OnSameTeam(self->owner, other))
		return;

	// set them on fire
	burn_person(other, self->owner, sf2qf(self->dmg));

	// hurt them
	if (level.framenum >= self->monsterinfo.nextattack)
	{
		T_Damage(other, self, self->owner, vec3_origin, self->s.origin, 
				plane->normal, sf2qf(self->dmg), 0, DAMAGE_NO_KNOCKBACK, MOD_BURN);
		self->monsterinfo.nextattack = level.framenum + 1;
	}
}

void ThrowFlame (edict_t *ent, vec3_t start, vec3_t forward, float dist, int speed, int damage, int ttl)
{
	edict_t *fire;

	// create the fire ent
	fire = G_Spawn();
	VectorCopy(start, fire->s.origin);
	VectorCopy(start, fire->s.old_origin);
	vectoangles(forward, fire->s.angles);
	fire->s.angles[PITCH] = 0; // always face up

	// toss it
	VectorScale (forward, speed, fire->velocity);
	if (ent->mtype != TOTEM_FIRE)
		fire->velocity[2] += GetRandom(0, 200);//;//GetRandom(200, 400);
	//else
	//	fire->velocity[2] += 200;

	//gi.dprintf(" velocity[2] = %.0f\n", fire->velocity[2]);
	fire->movetype = MOVETYPE_TOSS;
	fire->owner = ent;
	fire->dmg = damage;
	fire->classname = "fire";
	fire->s.sound = gi.soundindex ("weapons/bfg__l1a.wav");
	fire->delay = level.time + ttl;
	fire->think = bfire_think;
	fire->touch = bfire_touch;
	fire->nextthink = level.time+FRAMETIME;
	fire->solid = SOLID_TRIGGER;
	fire->clipmask = MASK_SHOT;
	fire->s.modelindex = gi.modelindex ("models/fire/tris.md2");
	gi.linkentity(fire);
}

void SpawnFlames (edict_t *self, vec3_t start, int num_flames, int damage, int toss_speed)
{
	int		i;
	edict_t *fire;
	vec3_t	forward;

	for (i=0; i<num_flames; i++) {
		// create the fire ent
		fire = G_Spawn();
		VectorCopy(start, fire->s.origin);
		fire->movetype = MOVETYPE_TOSS;
		fire->owner = self;
		fire->dmg = damage;
		fire->classname = "fire";
		fire->s.sound = gi.soundindex ("weapons/bfg__l1a.wav");
		fire->delay = level.time + GetRandom(1, 3);
		fire->think = bfire_think;
		fire->touch = bfire_touch;
		fire->nextthink = level.time+FRAMETIME;
		fire->solid = SOLID_TRIGGER;
		fire->clipmask = MASK_SHOT;
		fire->s.modelindex = gi.modelindex ("models/fire/tris.md2");
		gi.linkentity(fire);
		// toss it
		fire->s.angles[YAW] = GetRandom(0, 360);
		AngleVectors (fire->s.angles, forward, NULL, NULL);
		VectorScale (forward, GetRandom((int)(toss_speed*0.5), (int)(toss_speed*1.5)), fire->velocity);
		fire->velocity[2] = GetRandom(200, 400);
	}
}

void napalm_remove (edict_t *self, qboolean print)
{
	if (self->deadflag == DEAD_DEAD)
		return;

	// prepare for removal
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_NO;
	self->think = G_FreeEdict;
	self->nextthink = level.time + FRAMETIME;

	if (self->owner && self->owner->inuse)
	{
		self->owner->num_napalm--;

		if (print)
			safe_cprintf(self->owner, PRINT_HIGH, "%d/%d napalm grenades remaining.\n",
				self->owner->num_napalm, (int)NAPALM_MAX_COUNT);
	}
}

void napalm_explode (edict_t *self)
{
	vec3_t start;

	VectorCopy(self->s.origin, start);
	start[2] += 16;

	if (!self->waterlevel)
		SpawnFlames(self->owner, start, 3, self->radius_dmg, 80);

	T_RadiusDamage(self, self->owner, self->dmg, NULL, self->dmg_radius, MOD_NAPALM);

	// create explosion effect
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_ROCKET_EXPLOSION);
	gi.WritePosition (start);
	gi.multicast (start, MULTICAST_PHS);
}

void napalm_effects (edict_t *self)
{
	self->s.effects = self->s.renderfx = 0;

	self->s.effects |= EF_COLOR_SHELL;
	if (self->owner->teamnum == RED_TEAM || ffa->value)
		self->s.renderfx |= RF_SHELL_RED;
	else
		self->s.renderfx |= RF_SHELL_BLUE;
}

void napalm_think (edict_t *self)
{
	if (!G_EntIsAlive(self->owner) || (level.time > self->delay))
	{
		napalm_remove(self, true);
		return;
	}

	// assign team colors
	napalm_effects(self);

	if (level.time > self->monsterinfo.attack_finished)
	{
		napalm_explode(self);
		self->monsterinfo.attack_finished = level.time + NAPALM_ATTACK_DELAY;
	}

	self->nextthink = level.time + FRAMETIME;
}

void fire_napalm (edict_t *self, vec3_t start, vec3_t aimdir,
	int explosive_damage, int burn_damage, int speed, float duration, float damage_radius)
{
	edict_t	*grenade;

	// calling entity made a sound, used to alert monsters
	self->lastsound = level.framenum;

	// create bolt
	grenade = G_Spawn();
	VectorClear(grenade->mins);
	VectorClear(grenade->maxs);
	grenade->s.modelindex = gi.modelindex ("models/objects/grenade2/tris.md2");
	VectorCopy (start, grenade->s.origin);
	VectorCopy (start, grenade->s.old_origin);
	vectoangles (aimdir, grenade->s.angles);
	grenade->movetype = MOVETYPE_BOUNCE;
	grenade->clipmask = MASK_SHOT;
	grenade->s.effects |= EF_GRENADE;
	grenade->solid = SOLID_BBOX;
	grenade->owner = self;
	//grenade->touch = spike_touch;
	grenade->nextthink = level.time + 1;
	grenade->delay = level.time+duration;
	grenade->think = napalm_think;
	grenade->dmg = explosive_damage;
	grenade->dmg_radius = damage_radius;
	grenade->radius_dmg = burn_damage;

	grenade->classname = "napalm";
	gi.linkentity (grenade);

	VectorScale (aimdir, speed, grenade->velocity);
	grenade->velocity[2] += 250.0;

	self->num_napalm++;
	//gi.sound (self, CHAN_WEAPON, gi.soundindex("brain/brnatck2.wav"), 1, ATTN_NORM, 0);
}

void RemoveNapalmGrenades (edict_t *ent)
{
	edict_t *e=NULL;

	while((e = G_Find(e, FOFS(classname), "napalm")) != NULL)
	{
		if (e && (e->owner == ent))
			napalm_remove(e, false);
	}

	// reset napalm counter
	ent->num_napalm = 0;
}

void Cmd_Napalm_f (edict_t *ent)
{
	int		damage, burn_dmg, cost;
	float	radius;
	vec3_t	offset, forward, right, start;

	if(ent->myskills.abilities[NAPALM].disable)
		return;

	if (Q_strcasecmp (gi.args(), "count") == 0)
	{
		safe_cprintf(ent, PRINT_HIGH, "You have %d/%d napalm grenades.\n",
			ent->num_napalm, (int)NAPALM_MAX_COUNT);
		return;
	}

	if (Q_strcasecmp (gi.args(), "remove") == 0)
	{
		RemoveNapalmGrenades(ent);
		safe_cprintf(ent, PRINT_HIGH, "All napalm grenades removed.\n");
		return;
	}

	//Talent: Bombardier - reduces grenade cost
    cost = NAPALM_COST - vrx_get_talent_level(ent, TALENT_BOMBARDIER);

	if (!G_CanUseAbilities(ent, ent->myskills.abilities[NAPALM].current_level, cost))
		return;

	if (ent->client->pers.inventory[grenade_index] < 1)
	{
		safe_cprintf(ent, PRINT_HIGH, "You need at least 1 grenade to use this ability.\n");
		return;
	}

	if (ent->num_napalm >= NAPALM_MAX_COUNT)
	{
		safe_cprintf(ent, PRINT_HIGH, "Can't throw any more napalm grenades.\n");
		return;
	}

	// get starting position and forward vector
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 8,  ent->viewheight-8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	damage = NAPALM_INITIAL_DMG + NAPALM_ADDON_DMG*ent->myskills.abilities[NAPALM].current_level;
	burn_dmg = NAPALM_INITIAL_BURN + NAPALM_ADDON_BURN*ent->myskills.abilities[NAPALM].current_level;
	radius = NAPALM_INITIAL_RADIUS + NAPALM_ADDON_RADIUS*ent->myskills.abilities[NAPALM].current_level;

	fire_napalm(ent, start, forward, damage, burn_dmg, 400, NAPALM_DURATION, radius);

	ent->client->pers.inventory[power_cube_index] -= cost;
	ent->client->pers.inventory[grenade_index]--;
	ent->client->ability_delay = level.time + NAPALM_DELAY;
}