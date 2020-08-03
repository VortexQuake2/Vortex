#include "g_local.h"
#include "../../gamemodes/ctf.h"

void T_RadiusDamage_Nonplayers (edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, int mod); //only affects players
void T_RadiusDamage_Players (edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, int mod); //only affects players

void proxy_remove (edict_t *self, qboolean print)
{
	if (self->deadflag == DEAD_DEAD)
		return;

	// prepare for removal
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_NO;
	self->think = BecomeExplosion1;
	self->nextthink = level.time + FRAMETIME;

	if (self->creator && self->creator->inuse)
	{
		self->creator->num_proxy--;

		if (print)
			safe_cprintf(self->creator, PRINT_HIGH, "%d/%d proxy grenades remaining.\n",
				self->creator->num_proxy, PROXY_MAX_COUNT);
	}
}

void proxy_explode (edict_t *self)
{
	if (self->style)//4.4 proxy needs to be rearmed
		return;

	safe_cprintf(self->creator, PRINT_HIGH, "Proxy detonated.\n");

	T_RadiusDamage_Players(self, self->creator, self->dmg, self, self->dmg_radius, MOD_PROXY);
	T_RadiusDamage_Nonplayers(self, self->creator, self->dmg * 1.5, self, self->dmg_radius * 1.5, MOD_PROXY);

	// create explosion effect
	gi.WriteByte (svc_temp_entity);
	if (self->waterlevel)
		gi.WriteByte (TE_ROCKET_EXPLOSION_WATER);
	else
		gi.WriteByte (TE_ROCKET_EXPLOSION);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PHS);

	self->style = 1; //4.4 change status to disarmed

    if (vrx_get_talent_level(self->creator, TALENT_INSTANTPROXYS)) // Remove proxys when you have this talent.
		proxy_remove(self, true);
}

qboolean proxy_disabled_think (edict_t *self)
{
	// not disabled
	if (!self->style)
		return false;
	
	// set effects
	if (sf2qf(level.framenum) & 8)
	{
		self->s.effects |= EF_COLOR_SHELL;
		self->s.renderfx |= RF_SHELL_YELLOW;
	}
	else
		self->s.effects = self->s.renderfx = 0;

	self->nextthink = level.time + FRAMETIME;
	return true;
}

void proxy_effects (edict_t *self)
{
	// flash to indicate activity
	if (level.time > self->monsterinfo.pausetime)
	{
		if (self->monsterinfo.lefty)
		{
			// flash on
			self->s.effects |= EF_COLOR_SHELL;
			if (self->creator->teamnum == RED_TEAM || ffa->value)
				self->s.renderfx |= RF_SHELL_RED;
			else
				self->s.renderfx |= RF_SHELL_BLUE;

			// toggle
			self->monsterinfo.lefty = 0;

			gi.sound(self, CHAN_VOICE, gi.soundindex("misc/beep.wav"), 1, ATTN_IDLE, 0);
		}
		else
		{
			self->s.effects = self->s.renderfx = 0; // clear all effects
			self->monsterinfo.lefty = 1;// toggle
		}

		self->monsterinfo.pausetime = level.time + 3.0;
	}
}

void proxy_think (edict_t *self)
{
	edict_t *e=NULL;

	// remove proxy if owner is invalid
	if (!G_EntIsAlive(self->creator))
	{
		proxy_remove(self, false);
		return;
	}

	if (proxy_disabled_think(self))//4.4
		return;
	
	// search for nearby targets
	while ((e = findclosestradius_targets(e, self, self->dmg_radius)) != NULL)
	{
		if (!G_ValidTarget_Lite(self, e, true))
			continue;
		proxy_explode(self);
		break;//4.4
	}

	proxy_effects(self);

	self->nextthink = level.time + FRAMETIME;
}

void proxy_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (G_EntIsAlive(other) && other->client && OnSameTeam(ent, other) // a player on our team
		&& other->client->pers.inventory[power_cube_index] >= 5 // has power cubes
		&& ent->style // we need rearming
		&& level.framenum > ent->monsterinfo.regen_delay1) // check delay
	{
		ent->style = 0;// change status to armed
		ent->health = ent->max_health;// restore health
		other->client->pers.inventory[power_cube_index] -= 5;
		safe_cprintf(other, PRINT_HIGH, "Proxy repaired and re-armed.\n");
		gi.sound(other, CHAN_VOICE, gi.soundindex("weapons/repair.wav"), 1, ATTN_NORM, 0);
		ent->monsterinfo.regen_delay1 = (int)(level.framenum + 2 / FRAMETIME);// delay before we can rearm
		ent->nextthink = level.time + 2.0f;// delay before arming again
	}
}

void proxy_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	safe_cprintf(self->creator, PRINT_HIGH, "Proxy grenade destroyed.\n");
	proxy_remove(self, true);
}

qboolean NearbyLasers (edict_t *ent, vec3_t org);
void SpawnProxyGrenade (edict_t *self, int cost, float skill_mult, float delay_mult)
{
	//int		cost = PROXY_COST*cost_mult;
	edict_t	*grenade;
	vec3_t	forward, right, offset, start, end;
	vec3_t	bmin, bmax;
	trace_t	tr;

	// calling entity made a sound, used to alert monsters
	self->lastsound = level.framenum;

	// get starting position and forward vector
	AngleVectors (self->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 8,  self->viewheight-8);
	P_ProjectSource(self->client, self->s.origin, offset, forward, right, start);
	// get end position
	VectorMA(start, 64, forward, end);

	// set bbox size
	VectorSet (bmin, -4, -4, -4);
	VectorSet (bmax, 4, 4, 4);

	//tr = gi.trace (start, bmin, bmax, end, self, MASK_SOLID);
	tr = gi.trace (start, NULL, NULL, end, self, MASK_SOLID);

	// can't build a laser on sky
	if (tr.surface && (tr.surface->flags & SURF_SKY))
		return;

	if (tr.fraction == 1)
	{
		safe_cprintf(self, PRINT_HIGH, "Too far from wall.\n");
		return;
	}

	if (NearbyLasers(self, tr.endpos))
	{
		safe_cprintf(self, PRINT_HIGH, "Too close to a laser.\n");
		return;
	}

	// create bolt
	grenade = G_Spawn();

	grenade->monsterinfo.level = self->myskills.abilities[PROXY].current_level * skill_mult;

	//VectorSet (grenade->mins, -4, -4, -4);
	//VectorSet (grenade->maxs, 4, 4, 4);
	VectorCopy(bmin, grenade->mins);
	VectorCopy(bmax, grenade->maxs);
	grenade->s.modelindex = gi.modelindex ("models/objects/grenade2/tris.md2");
	grenade->health = PROXY_BASE_HEALTH + PROXY_ADDON_HEALTH*self->monsterinfo.level;
	grenade->max_health = grenade->health;
	grenade->takedamage = DAMAGE_AIM;
	grenade->die = proxy_die;
	grenade->movetype = MOVETYPE_NONE;
	grenade->clipmask = MASK_SHOT;
	grenade->touch = proxy_touch;//4.4
	grenade->solid = SOLID_BBOX;
	grenade->creator = self;
	grenade->mtype = M_PROXY;//4.5
	grenade->nextthink = level.time + PROXY_BUILD_TIME * delay_mult;
	grenade->think = proxy_think;
	grenade->dmg = PROXY_BASE_DMG + PROXY_ADDON_DMG*grenade->monsterinfo.level;
	grenade->dmg_radius = PROXY_BASE_RADIUS + PROXY_ADDON_RADIUS*grenade->monsterinfo.level;
	grenade->classname = "proxygrenade";

	VectorCopy(tr.endpos, grenade->s.origin);
	VectorCopy(tr.endpos, grenade->s.old_origin);
	VectorCopy(tr.plane.normal, grenade->s.angles);

	gi.linkentity (grenade);

	// cost is doubled if you are a flyer or cacodemon below skill level 5
	if ((self->mtype == MORPH_FLYER && self->myskills.abilities[FLYER].current_level < 5) 
		|| (self->mtype == MORPH_CACODEMON && self->myskills.abilities[CACODEMON].current_level < 5))
		cost *= 2;

	self->num_proxy++;
	self->client->pers.inventory[power_cube_index] -= cost;

    if (vrx_get_talent_level(self, TALENT_INSTANTPROXYS) < 2) // No hold time for instantproxys talent.
		self->holdtime = level.time + PROXY_BUILD_TIME * delay_mult;

	self->client->ability_delay = level.time + PROXY_BUILD_TIME * delay_mult;

	safe_cprintf(self, PRINT_HIGH, "Proxy grenade built (%d/%d).\n", 
		self->num_proxy, PROXY_MAX_COUNT);
}

void RemoveProxyGrenades (edict_t *ent)
{
	edict_t *e=NULL;

	while((e = G_Find(e, FOFS(classname), "proxygrenade")) != NULL)
	{
		if (e && (e->creator == ent))
			proxy_remove(e, false);
	}

	// reset proxy counter
	ent->num_proxy = 0;
}

void Cmd_BuildProxyGrenade (edict_t *ent)
{
	int talentLevel, cost = PROXY_COST;
	float skill_mult=1.0, cost_mult=1.0, delay_mult=1.0;//Talent: Rapid Assembly & Precision Tuning

	if(ent->myskills.abilities[PROXY].disable)
		return;

	if (Q_strcasecmp (gi.args(), "count") == 0)
	{
		safe_cprintf(ent, PRINT_HIGH, "You have %d/%d proxy grenades.\n",
			ent->num_proxy, PROXY_MAX_COUNT);
		return;
	}

	if (Q_strcasecmp (gi.args(), "remove") == 0)
	{
		RemoveProxyGrenades(ent);
		safe_cprintf(ent, PRINT_HIGH, "All proxy grenades removed.\n");
		return;
	}

	// cost is doubled if you are a flyer or cacodemon below skill level 5
	if ((ent->mtype == MORPH_FLYER && ent->myskills.abilities[FLYER].current_level < 5) 
		|| (ent->mtype == MORPH_CACODEMON && ent->myskills.abilities[CACODEMON].current_level < 5))
		cost *= 2;

	//Talent: Rapid Assembly
    talentLevel = vrx_get_talent_level(ent, TALENT_RAPID_ASSEMBLY);
	if (talentLevel > 0)
		delay_mult -= 0.1 * talentLevel;
	//Talent: Precision Tuning
    else if ((talentLevel = vrx_get_talent_level(ent, TALENT_PRECISION_TUNING)) > 0)
	{
		cost_mult += PRECISION_TUNING_COST_FACTOR * talentLevel;
		delay_mult += PRECISION_TUNING_DELAY_FACTOR * talentLevel;
		skill_mult += PRECISION_TUNING_SKILL_FACTOR * talentLevel;
	}
	cost *= cost_mult;

	if (!G_CanUseAbilities(ent, ent->myskills.abilities[PROXY].current_level, cost))
		return;

	if (ent->num_proxy >= PROXY_MAX_COUNT)
	{
		safe_cprintf(ent, PRINT_HIGH, "Can't build any more proxy grenades.\n");
		return;
	}

	if (ctf->value && (CTF_DistanceFromBase(ent, NULL, CTF_GetEnemyTeam(ent->teamnum)) < CTF_BASE_DEFEND_RANGE))
	{
		safe_cprintf(ent, PRINT_HIGH, "Can't build in enemy base!\n");
		return;
	}

	SpawnProxyGrenade(ent, cost, skill_mult, delay_mult);
}