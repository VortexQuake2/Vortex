#include "g_local.h"

//************************************************************************************************
//		FIRE
//************************************************************************************************

void bfire_think(edict_t* self)
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

void bfire_touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf)
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

void ThrowFlame(edict_t* ent, vec3_t start, vec3_t forward, float dist, int speed, int damage, int ttl)
{
	edict_t* fire;

	// create the fire ent
	fire = G_Spawn();
	VectorCopy(start, fire->s.origin);
	VectorCopy(start, fire->s.old_origin);
	vectoangles(forward, fire->s.angles);
	fire->s.angles[PITCH] = 0; // always face up

	// toss it
	VectorScale(forward, speed, fire->velocity);
	if (ent->mtype != TOTEM_FIRE)
		fire->velocity[2] += GetRandom(0, 200);//;//GetRandom(200, 400);
	//else
	//	fire->velocity[2] += 200;

	//gi.dprintf(" velocity[2] = %.0f\n", fire->velocity[2]);
	fire->movetype = MOVETYPE_TOSS;
	fire->s.effects |= EF_BLASTER;
	fire->owner = ent;
	fire->dmg = damage;
	fire->classname = "fire";
	fire->s.sound = gi.soundindex("weapons/bfg__l1a.wav");
	fire->delay = level.time + ttl;
	fire->think = bfire_think;
	fire->touch = bfire_touch;
	fire->nextthink = level.time + FRAMETIME;
	fire->solid = SOLID_TRIGGER;
	fire->clipmask = MASK_SHOT;
	fire->s.modelindex = gi.modelindex("models/fire/tris.md2");
	gi.linkentity(fire);

	// cloak a player-owned fire in PvM if there are too many entities nearby
	if (pvm->value && G_GetClient(ent) && !vrx_spawn_nonessential_ent(fire->s.origin))
		fire->svflags |= SVF_NOCLIENT;
}

void SpawnFlames(edict_t* self, vec3_t start, int num_flames, int damage, int toss_speed)
{
	int		i;
	edict_t* fire;
	vec3_t	forward;

	for (i = 0; i < num_flames; i++) {
		// create the fire ent
		fire = G_Spawn();
		VectorCopy(start, fire->s.origin);
		fire->movetype = MOVETYPE_TOSS;
		fire->s.effects |= EF_BLASTER;
		fire->owner = self;
		fire->dmg = damage;
		fire->classname = "fire";
		fire->s.sound = gi.soundindex("weapons/bfg__l1a.wav");
		fire->delay = level.time + GetRandom(1, 3);
		fire->think = bfire_think;
		fire->touch = bfire_touch;
		fire->nextthink = level.time + FRAMETIME;
		fire->solid = SOLID_TRIGGER;
		fire->clipmask = MASK_SHOT;
		fire->s.modelindex = gi.modelindex("models/fire/tris.md2");
		gi.linkentity(fire);
		// toss it
		fire->s.angles[YAW] = GetRandom(0, 360);
		AngleVectors(fire->s.angles, forward, NULL, NULL);
		VectorScale(forward, GetRandom((int)(toss_speed * 0.5), (int)(toss_speed * 1.5)), fire->velocity);
		fire->velocity[2] = GetRandom(200, 400);

		// cloak a player-owned fire in PvM if there are too many entities nearby
		if (pvm->value && G_GetClient(self) && !vrx_spawn_nonessential_ent(fire->s.origin))
			fire->svflags |= SVF_NOCLIENT;
	}
}

//************************************************************************************************
//		FIREBALL
//************************************************************************************************

void fireball_explode(edict_t* self, cplane_t* plane)
{
	int		i;
	vec3_t	forward;
	edict_t* e = NULL;

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

	// loop for creating flame entities
	for (i = 0; i < self->count; i++)
	{
		if (plane) // if touching plane, move in direction of plane.normal (away from it)
		{
			VectorCopy(plane->normal, forward);
		}
		else // otherwise, move in direction opposite of velocity
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

	// do radius damage to nearby entities
	T_RadiusDamage(self, self->owner, self->dmg, NULL, self->dmg_radius, MOD_FIREBALL);

	// create explosion effect
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PHS);

	// remove fireball entity next frame
	self->think = G_FreeEdict;
	self->nextthink = level.time + FRAMETIME;
}

void fireball_touch(edict_t* ent, edict_t* other, cplane_t* plane, csurface_t* surf)
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
	//gi.sound(ent, CHAN_VOICE, gi.soundindex(va("abilities/largefireimpact%d.wav", GetRandom(1, 3))), 1, ATTN_NORM, 0);
	fireball_explode(ent, plane);
}

void fireball_think(edict_t* self)
{
	vec3_t angles;

	if (level.time > self->delay || self->waterlevel)
	{
		G_FreeEdict(self);
		return;
	}

	// extra particle effects
	G_DrawSparks(self->s.origin, self->s.origin, 223, 231, 4, 1, 0, 0);
	// set angles to point in the direction of movement
	vectoangles(self->velocity, angles);
	VectorCopy(angles, self->s.angles);
	// run model animation
	G_RunFrames(self, 0, 3, false);

	self->nextthink = level.time + FRAMETIME;
}

void fire_fireball(edict_t* self, vec3_t start, vec3_t aimdir, int damage, float damage_radius, int speed, int flames, int flame_damage)
{
	edict_t* fireball;
	vec3_t	dir;
	vec3_t	forward;

	//  entity made a sound, used to alert monsters
	self->lastsound = level.framenum;

	// get aiming angles
	vectoangles(aimdir, dir);

	//gi.dprintf("%s: pitch: %.1f\n", __func__, dir[PITCH]);

	// get directional vectors
	AngleVectors(dir, forward, NULL, NULL);

	// spawn grenade entity
	fireball = G_Spawn();
	VectorCopy(start, fireball->s.origin);
	fireball->s.effects |= EF_BLASTER;//EF_FLAG1;
	fireball->movetype = MOVETYPE_TOSS;
	fireball->clipmask = MASK_SHOT;
	fireball->solid = SOLID_BBOX;
	fireball->s.modelindex = gi.modelindex("models/proj/proj_dguardq/tris.md2");//gi.modelindex ("models/objects/flball/tris.md2");
	fireball->owner = self;
	fireball->touch = fireball_touch;
	fireball->think = fireball_think;
	fireball->dmg_radius = damage_radius;
	fireball->dmg = damage;
	fireball->radius_dmg = flame_damage;
	fireball->count = flames;
	fireball->classname = "fireball";
	fireball->delay = level.time + 10.0;
	gi.linkentity(fireball);
	fireball->nextthink = level.time + FRAMETIME;
	VectorCopy(dir, fireball->s.angles);

	// adjust velocity
	VectorScale(aimdir, speed, fireball->velocity);
	// push up
	if (!self->ai.is_bot && !self->lockon)//GHz: don't boost vertical velocity for bots as it will affect ballistic calculations (i.e. finding the right pitch to hit the target)
		fireball->velocity[2] += 150;
	// make it spin/roll
	VectorSet(fireball->avelocity, 0, 0, 600);
	// play a sound
	gi.sound(fireball, CHAN_WEAPON, gi.soundindex("abilities/firecast.wav"), 1, ATTN_NORM, 0);
}

void Cmd_Fireball_f(edict_t* ent, float skill_mult, float cost_mult)
{
	int		slvl = ent->myskills.abilities[FIREBALL].current_level;
	int		damage, speed, flames, flamedmg, cost = FIREBALL_COST * cost_mult;
	float	radius;
	vec3_t	forward, right, start, offset;

	if (!V_CanUseAbilities(ent, FIREBALL, cost, true))
		return;

	if (ent->waterlevel > 1)
		return;

	damage = (FIREBALL_INITIAL_DAMAGE + FIREBALL_ADDON_DAMAGE * slvl) * (skill_mult * vrx_get_synergy_mult(ent, FIREBALL));
	radius = FIREBALL_INITIAL_RADIUS + FIREBALL_ADDON_RADIUS * slvl;
	speed = FIREBALL_INITIAL_SPEED + FIREBALL_ADDON_SPEED * slvl;
	flames = FIREBALL_INITIAL_FLAMES + FIREBALL_ADDON_FLAMES * slvl;
	//flamedmg = (FIREBALL_INITIAL_FLAMEDMG + FIREBALL_ADDON_FLAMEDMG * slvl) * (skill_mult * vrx_get_synergy_mult(ent, FIREBALL));
	flamedmg = 0.1 * damage;

	// get starting position and forward vector
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	fire_fireball(ent, start, forward, damage, radius, speed, flames, flamedmg);

	//Talent: Wizardry - makes spell timer ability-specific instead of global
	int talentLevel = vrx_get_talent_level(ent, TALENT_WIZARDRY);
	if (talentLevel > 0)
	{
		ent->myskills.abilities[FIREBALL].delay = level.time + FIREBALL_DELAY;
		ent->client->ability_delay = level.time + FIREBALL_DELAY * (1 - 0.2 * talentLevel);
	}
	else
		ent->client->ability_delay = level.time + FIREBALL_DELAY/* * cost_mult*/;

	ent->client->pers.inventory[power_cube_index] -= cost;

	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_TELEPORT_EFFECT);
	gi.WritePosition(ent->s.origin);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	gi.sound(ent, CHAN_ITEM, gi.soundindex("abilities/firecast.wav"), 1, ATTN_NORM, 0);
}

void ShootFireballsAtNearbyEnemies(edict_t* self, float radius, int max_targets, int fireball_level)
{
	edict_t* e = NULL;

	int damage = FIREBALL_INITIAL_DAMAGE + FIREBALL_ADDON_DAMAGE * fireball_level;
	float fb_radius = FIREBALL_INITIAL_RADIUS + FIREBALL_ADDON_RADIUS * fireball_level;
	//float speed = FIREBALL_INITIAL_SPEED + FIREBALL_ADDON_SPEED * fireball_level;
	int flames = FIREBALL_INITIAL_FLAMES + FIREBALL_ADDON_FLAMES * fireball_level;
	int flamedmg = 0.1 * damage;
	vec3_t forward, start;
	edict_t* owner = G_GetSummoner(self);

	int num_targets = 0; // initialize counter
	while ((e = findradius(e, self->s.origin, radius)) != NULL)
	{
		if (num_targets >= max_targets)
			break;
		if (!G_ValidTarget(self, e, true, true))
			continue;
		//gi.dprintf("%s: found target\n", __func__);
		//MonsterAim(self, -1, speed, true, 0, forward, start);
		VectorSubtract(e->s.origin, self->s.origin, forward);
		VectorNormalize(forward);
		fire_fireball(owner, self->s.origin, forward, damage, fb_radius, 400, flames, flamedmg);
		num_targets++;
	}
}

//************************************************************************************************
//		FIREWALL
//************************************************************************************************

void firewall_remove(edict_t* self)
{
	if (self->activator && self->activator->inuse)
		self->activator->num_firewalls--;

	G_FreeEdict(self);
}

void RemoveFirewalls(edict_t* ent)
{
	edict_t* e = NULL;

	while ((e = G_Find(e, FOFS(classname), "flames")) != NULL)
	{
		if (e && e->inuse && (e->activator == ent))
			firewall_remove(e);
	}

	// reset firewall counter
	ent->num_firewalls = 0;
}

void flames_lg_bbox(vec3_t mins, vec3_t maxs)
{
	VectorSet(mins, -32, -32, 0);
	VectorSet(maxs, 32, 32, 100);
}
void flames_med_bbox(vec3_t mins, vec3_t maxs)
{
	VectorSet(mins, -16, -16, 0);
	VectorSet(maxs, 16, 16, 48);
}

void flames_attack(edict_t* self)
{
	if (level.framenum >= self->monsterinfo.nextattack)
	{
		edict_t* touch[MAX_EDICTS], * hit;
		int num_edicts = gi.BoxEdicts(self->absmin, self->absmax, touch, MAX_EDICTS, AREA_SOLID);

		for (int i = 0; i < num_edicts; i++)
		{
			hit = touch[i];
			if (hit == self)
				continue;
			if (!G_ValidTarget(self, hit, false, false))
				continue;
			//gi.dprintf("%d: %s\n", (int)level.framenum, __func__);
			//V_TouchSolids(self);
			self->touch(self, hit, NULL, NULL);
		}
		self->monsterinfo.nextattack = level.framenum + 1;
	}
}

// style 0 - just created, 1 - ready to grow, 2 - large, 3 - ready to shrink, 4 - shrunk
void flames_adjust_size(edict_t* self)
{
	// recently spawned (early stage)
	if (!self->style)
	{
		// end of the first run of medium frames, so it's time to grow
		if (self->s.frame == 32)
			self->style = 1;
		else
			return;
	}

	// large flames (mature stage)
	if (self->style == 2)
	{
		// flames will die out soon, so it's time to shrink
		if (level.time + 1 > self->delay)
			self->style = 3;
		else
			return;
	}

	// growth stage--try to expand bbox
	if (self->style == 1)
	{
		vec3_t mins, maxs;
		flames_lg_bbox(mins, maxs);
		trace_t tr = gi.trace(self->s.origin, mins, maxs, self->s.origin, self, MASK_SOLID);
		if (tr.fraction == 1.0)
		{
			self->s.modelindex = gi.modelindex("models/flames_big/tris.md2");
			flames_lg_bbox(self->mins, self->maxs);
			gi.linkentity(self);
			self->style = 2;
			self->s.frame = 0;
		}
	}
	else if (self->style == 3) // almost outta time--shrink bbox
	{
		self->s.modelindex = gi.modelindex("models/flames_med/tris.md2");
		flames_med_bbox(self->mins, self->maxs);
		gi.linkentity(self);
		self->style = 4;
		self->s.frame = 22;
	}
}

// style 0 - just created, 1 - ready to grow, 2 - large, 3 - ready to shrink, 4 - shrunk
void flames_runframes(edict_t* self)
{
	self->s.frame++;
	int firstframe = 0, lastframe;
	int style = self->style; // growth/lifecycle stage

	if (style)
	{
		if (style == 1 || style == 4) // medium size
		{
			firstframe = 22;
			lastframe = 32;
		}
		else // 2 & 3: large size
		{
			lastframe = 10;
		}
	}
	else
		lastframe = 32;
	if (self->s.frame > lastframe)
		self->s.frame = firstframe;
	//gi.dprintf("%s: style %d firstframe %d lastframe %d frame %d\n", __func__, style, firstframe, lastframe, self->s.frame);
}

void flames_think(edict_t* self)
{
	if (!G_EntIsAlive(self->activator) || (self->delay < level.time) || self->waterlevel)
	{
		firewall_remove(self);
		return;
	}

	flames_attack(self);
	flames_adjust_size(self);
	flames_runframes(self);

	self->nextthink = level.time + FRAMETIME;
}

void flames_touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	if (other == self)
		return;

	if (G_EntExists(other) && !OnSameTeam(self, other))
	{
		// set them on fire
		burn_person(other, self->owner, sf2qf(self->dmg));

		// hurt them
		if (level.framenum >= self->monsterinfo.nextattack)
		{
			T_Damage(other, self, self->activator, vec3_origin, self->s.origin, plane->normal, self->dmg, 0, DAMAGE_NO_KNOCKBACK, MOD_BURN);
			//gi.dprintf("%d: %s\n", (int)level.framenum, __func__);
			self->monsterinfo.nextattack = level.framenum + 1;
		}
		// actively hurting something--don't free us yet!
		return;
	}
}

#define FIREWALL_MAX 4 //FIXME: add to lua

edict_t* CreateFirewallFlames(edict_t* ent, int skill_level, float skill_mult)
{
	// create the fire ent
	edict_t* fire = G_Spawn();
	//VectorCopy(start, fire->s.origin);
	fire->movetype = MOVETYPE_TOSS;
	fire->s.effects |= EF_BLASTER;
	fire->s.renderfx |= RF_FRAMELERP;
	//fire->owner = ent;
	fire->activator = ent;
	fire->monsterinfo.level = skill_level;
	fire->mtype = M_FIREWALL;
	fire->dmg = (FIREWALL_INITIAL_DAMAGE + FIREWALL_ADDON_DAMAGE * skill_level) * skill_mult;
	fire->classname = "flames";
	fire->s.sound = gi.soundindex("weapons/bfg__l1a.wav");
	fire->delay = level.time + FIREWALL_DURATION;
	fire->think = flames_think;
	fire->touch = flames_touch;
	fire->nextthink = level.time + FRAMETIME;
	fire->solid = SOLID_TRIGGER;
	fire->clipmask = MASK_SHOT;
	//fire->svflags |= SVF_MONSTER;//testing
	//if (random() > 0.5)
	//{
	//	fire->style = 1;
	//	fire->s.modelindex = gi.modelindex("models/flames_big/tris.md2");
	//	flames_lg_bbox(fire->mins, fire->maxs);
	//}
	//else
	//{
	fire->s.modelindex = gi.modelindex("models/flames_med/tris.md2");
	flames_med_bbox(fire->mins, fire->maxs);
	//}
	return fire;
}

void Cmd_Firewall_f(edict_t* ent, float skill_mult, float cost_mult)
{
	int	cost = FIREWALL_COST * cost_mult;
	int skill_level = ent->myskills.abilities[FIREWALL].current_level;
	
	if (ent->num_firewalls > FIREWALL_MAX)
	{
		safe_cprintf(ent, PRINT_HIGH, "Can't make any more firewalls.\n");
		return;
	}
	if (!G_CanUseAbilities(ent, skill_level, cost))
		return;
	if (ent->myskills.abilities[FIREWALL].disable)
		return;

	vec3_t start, angles;
	edict_t* flames = CreateFirewallFlames(ent, skill_level, (skill_mult * vrx_get_synergy_mult(ent, FIREWALL)));

	if (!G_GetSpawnLocation(ent, FIREWALL_RANGE, flames->mins, flames->maxs, start, angles, PROJECT_HITBOX_FLOOR, true))
	{
		//gi.dprintf("failed to spawn firewall\n");
		G_FreeEdict(flames);
		return;
	}
	// move flames in to place
	VectorCopy(start, flames->s.origin);
	gi.linkentity(flames);

	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_TELEPORT_EFFECT);
	gi.WritePosition(ent->s.origin);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	gi.sound(ent, CHAN_ITEM, gi.soundindex("abilities/firecast.wav"), 1, ATTN_NORM, 0);
	//  entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;

	//Talent: Wizardry - makes spell timer ability-specific instead of global
	int talentLevel = vrx_get_talent_level(ent, TALENT_WIZARDRY);
	if (talentLevel > 0)
	{
		ent->myskills.abilities[FIREWALL].delay = level.time + FIREWALL_DELAY;
		ent->client->ability_delay = level.time + FIREWALL_DELAY * (1 - 0.2 * talentLevel);
	}
	else
		ent->client->ability_delay = level.time + FIREWALL_DELAY;
	ent->client->pers.inventory[power_cube_index] -= cost;
	ent->num_firewalls++;// increase firewall counter
}