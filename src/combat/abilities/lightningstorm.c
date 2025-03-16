#include "g_local.h"

void lightningstorm_sound(edict_t* self)
{
	float r = random();
	if (r > 0.33)
		gi.sound(self, CHAN_ITEM, gi.soundindex("abilities/chargedbolt1.wav"), 1, ATTN_NORM, 0);
	else if (r < 0.66)
		gi.sound(self, CHAN_ITEM, gi.soundindex("abilities/chargedbolt2.wav"), 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_ITEM, gi.soundindex("abilities/chargedbolt4.wav"), 1, ATTN_NORM, 0);
}

void lightningstorm_attack(edict_t* self, vec3_t start)
{
	edict_t* e = NULL;
	vec3_t end, dir;
	trace_t tr;

	// calculate randomized starting position
	//VectorCopy(self->s.origin, start);
	//start[0] += GetRandom(0, (int)self->dmg_radius) * crandom();
	//start[1] += GetRandom(0, (int)self->dmg_radius) * crandom();
	//tr = gi.trace(self->s.origin, NULL, NULL, start, NULL, MASK_SOLID);
	//VectorCopy(tr.endpos, start);
	//VectorCopy(start, end);
	//end[2] += 8192;
	//tr = gi.trace(start, NULL, NULL, end, NULL, MASK_SOLID);
	//VectorCopy(tr.endpos, start);

	// calculate ending position
	VectorCopy(start, end);
	end[2] -= 8192;
	tr = gi.trace(start, NULL, NULL, end, NULL, MASK_SHOT);
	VectorCopy(tr.endpos, end);

	// find targets within radius of the impact site to damage
	while ((e = findradius(e, tr.endpos, LIGHTNING_STRIKE_RADIUS)) != NULL)
	{
		//FIXME: make a noise when we hit something?
		if (e && e->inuse && e->takedamage)
		{
			tr = gi.trace(end, NULL, NULL, e->s.origin, NULL, MASK_SHOT);
			VectorSubtract(tr.endpos, end, dir);
			VectorNormalize(dir);
			T_Damage(e, self, self->owner, dir, tr.endpos, tr.plane.normal, self->dmg, 0, DAMAGE_ENERGY, MOD_LIGHTNING_STORM);
		}
	}

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_HEATBEAM);
	gi.WriteShort(self - g_edicts);
	gi.WritePosition(start);
	gi.WritePosition(end);
	gi.multicast(end, MULTICAST_PVS);

	lightningstorm_sound(self);
	//gi.sound(self, CHAN_ITEM, gi.soundindex("abilities/chargedbolt1.wav"), 1, ATTN_NORM, 0);
}

void lightningstorm_think (edict_t *self) 
{
	vec3_t start;
	trace_t tr;

    // owner must be alive
    if (!G_EntIsAlive(self->owner) || vrx_has_flag(self->owner) || (self->delay < level.time)) {
        G_FreeEdict(self);
        return;
    }
	
	// calculate randomized firing position from the ceiling/skybox
	VectorCopy(self->pos1, start);
	start[0] += GetRandom(0, (int)self->dmg_radius) * crandom();
	start[1] += GetRandom(0, (int)self->dmg_radius) * crandom();
	tr = gi.trace(self->pos1, NULL, NULL, start, self, MASK_SOLID);

	// Talent: Chainlightning Storm
	int talentLevel = vrx_get_talent_level(self->owner, TALENT_CL_STORM);

	if (talentLevel && 0.05 * talentLevel > random())
		fire_chainlightning(self, tr.endpos, tv(0, 0, -1), self->dmg_counter, self->dmg_radius, 8192, CLIGHTNING_INITIAL_HR, 4);
	else
		lightningstorm_attack(self, tr.endpos);

	self->nextthink = level.time + GetRandom(LIGHTNING_MIN_DELAY, LIGHTNING_MAX_DELAY) * FRAMETIME;
}

void SpawnLightningStorm (edict_t *ent, vec3_t start, float radius, int duration, int damage)
{
	edict_t *storm;
	vec3_t end;
	trace_t tr;

	// trace up to the ceiling/skybox
	VectorCopy(start, end);
	end[2] = 8192;
	tr = gi.trace(start, NULL, NULL, end, ent, MASK_SOLID);
	
	storm = G_Spawn();
	storm->classname = "lightning storm";
	storm->solid = SOLID_NOT;
	storm->svflags |= SVF_NOCLIENT;
	storm->owner = ent;
	storm->delay = level.time + duration;
	storm->nextthink = level.time + FRAMETIME;
	storm->think = lightningstorm_think;
	VectorCopy(start, storm->s.origin);
	VectorCopy(start, storm->s.old_origin);
	VectorCopy(tr.endpos, storm->pos1); // store the ceiling/skybox position
	storm->dmg_radius = radius; // radius of lightning strikes
	storm->radius_dmg = damage; // used for bot AI hazard detection
	storm->dmg = damage;
	storm->mtype = M_LIGHTNINGSTORM; // used for bot AI hazard detection
	gi.linkentity(storm);

	// Talent: Chainlightning Storm
	int talentLevel = vrx_get_talent_level(ent, TALENT_CL_STORM);
	if (talentLevel > 1)
	{
		int skill_level = ent->myskills.abilities[LIGHTNING].current_level;
		if (skill_level < 1)
			skill_level = 1;
		// set chainlightning damage
		storm->dmg_counter = CLIGHTNING_INITIAL_DMG + (CLIGHTNING_ADDON_DMG * skill_level * vrx_get_synergy_mult(ent, LIGHTNING));
	}
}

void Cmd_LightningStorm_f (edict_t *ent, float skill_mult, float cost_mult)
{
	int		slvl = ent->myskills.abilities[LIGHTNING_STORM].current_level;
	int		damage, duration, cost=LIGHTNING_COST*cost_mult;
	float	radius;
	vec3_t	forward, offset, right, start, end;
	trace_t	tr;

	if (!V_CanUseAbilities(ent, LIGHTNING_STORM, cost, true))
		return;

	damage = (LIGHTNING_INITIAL_DAMAGE + LIGHTNING_ADDON_DAMAGE * slvl) * (skill_mult * vrx_get_synergy_mult(ent, LIGHTNING_STORM));
	duration = LIGHTNING_INITIAL_DURATION + LIGHTNING_ADDON_DURATION * slvl;
	radius = LIGHTNING_INITIAL_RADIUS + LIGHTNING_ADDON_DURATION * slvl;

	// randomize damage
	damage = GetRandom((int)(0.5*damage), damage);

	// get starting position and forward vector
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 8,  ent->viewheight-8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	// get ending position
	VectorCopy(start, end);
	VectorMA(start, 8192, forward, end);

	tr = gi.trace(start, NULL, NULL, end, ent, MASK_SHOT);

	// trace up to the ceiling/skybox
	//VectorCopy(tr.endpos, start);
	//VectorCopy(tr.endpos, end);
	//end[2] += 8192;
	//tr = gi.trace(start, NULL, NULL, end, ent, MASK_SOLID);

	SpawnLightningStorm(ent, tr.endpos, radius, duration, damage);

	//Talent: Wizardry - makes spell timer ability-specific instead of global
	int talentLevel = vrx_get_talent_level(ent, TALENT_WIZARDRY);
	if (talentLevel > 0)
	{
		ent->myskills.abilities[LIGHTNING_STORM].delay = level.time + LIGHTNING_ABILITY_DELAY;
		ent->client->ability_delay = level.time + LIGHTNING_ABILITY_DELAY * (1 - 0.2 * talentLevel);
	}
	else
		ent->client->ability_delay = level.time + LIGHTNING_ABILITY_DELAY/* * cost_mult*/;
	ent->client->pers.inventory[power_cube_index] -= cost;

	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_TELEPORT_EFFECT);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	//  entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;

    gi.sound(ent, CHAN_WEAPON, gi.soundindex("abilities/eleccast.wav"), 1, ATTN_NORM, 0);
}