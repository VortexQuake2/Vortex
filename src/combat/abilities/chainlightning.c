#include "g_local.h"

#define CLIGHTNING_MAX_HOPS 4

void lightningstorm_sound(edict_t* self);

void G_SpawnParticleTrail (vec3_t start, vec3_t end, int particles, int color)
{
	int		i, spacing;
	float	dist;
	vec3_t	dir, org;

	VectorCopy(start, org);
	VectorSubtract(end, start, dir);
	dist = VectorLength(dir);
	VectorNormalize(dir);

	spacing = particles+11;

	// particle effects
	for (i=0; i<(int)dist; i+=spacing)
	{
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_LASER_SPARKS);
		gi.WriteByte (particles);
		gi.WritePosition (org);
		gi.WriteDir (dir);
		gi.WriteByte (color);
		gi.multicast (org, MULTICAST_PVS);

		// move starting position forward
		VectorMA(org, spacing, dir, org);
	}
}

void CL_attack(edict_t* self, edict_t *target, int damage)
{
	//if (G_ValidTarget(self, target, true, false))
	//{
		// do less damage to corpses
		if (target->health < 1 && damage > 100)
			damage = 100;
		//gi.dprintf("CL did %d damage at %d\n", damage, (int)level.framenum);
		// deal damage
		T_Damage(target, self, self, vec3_origin, target->s.origin, vec3_origin, damage, 0, DAMAGE_ENERGY, MOD_LIGHTNING);
		//return true;
	//}
	//return false;
}

qboolean CL_targetinlist(edict_t* self, edict_t *target)
{
	for (int i = 0; i < self->monsterinfo.target_index; i++)
	{
		if (target == self->monsterinfo.dmglist[i].player)
			return true;
	}
	return false;
}

edict_t *CL_findtarget(edict_t* self, vec3_t org, float radius, qboolean check_list)
{
	edict_t* e = NULL;

	while ((e = findclosestradius(e, org, radius)) != NULL)
	{
		if (!G_ValidTarget(self, e, true, false))
			continue;
		if (check_list && CL_targetinlist(self, e))
			continue;
		return e;
	}
	return NULL;
}

void chainlightning_think(edict_t* self)
{
	edict_t *enemy = CL_findtarget(self, self->s.origin, self->monsterinfo.sight_range, true);

	if (self->light_level && enemy) // have ammo and valid target
	{
		vec3_t start;

		//gi.dprintf("%d: CL is attacking, ammo: %d\n", (int)level.framenum, self->light_level);
		// add enemy to the list so we don't attack the same target twice
		self->monsterinfo.dmglist[self->monsterinfo.target_index++].player = enemy;

		CL_attack(self, enemy, self->dmg);
		// move
		G_EntMidPoint(enemy, start);
		VectorCopy(start, self->s.origin);
		gi.linkentity(self);

		// lightning graphical effect
		gi.WriteByte(svc_temp_entity);
#ifndef VRX_REPRO
		gi.WriteByte(TE_HEATBEAM);
		gi.WriteShort(self - g_edicts);
#else
		gi.WriteByte(TE_LIGHTNING);
		gi.WriteShort(self - g_edicts);
		gi.WriteShort(0);
#endif

		gi.WritePosition(self->s.old_origin);
		gi.WritePosition(self->s.origin);
		gi.multicast(self->s.origin, MULTICAST_PVS);

		VectorCopy(start, self->s.old_origin);

		// reduce ammo
		self->light_level--;
		// increase damage for next hop
		self->dmg *= CLIGHTNING_DMG_MOD;
	}
	else
	{
		//gi.dprintf("%d: CL is being removed, ammo: %d\n", (int)level.framenum, self->light_level);

		G_FreeEdict(self);
		return;
	}

	self->nextthink = level.time + FRAMETIME;
}

void fire_chainlightning(edict_t* self, vec3_t start, vec3_t aimdir, int damage, float radius, int attack_range, int hop_range, int max_hops)
{
	vec3_t end;
	edict_t* enemy = NULL;

	// calling entity made a sound, used to alert monsters
	self->lastsound = level.framenum;
	// play sound
	gi.sound(self, CHAN_ITEM, gi.soundindex("abilities/thunderbolt.wav"), 1, ATTN_NORM, 0);
	//lightningstorm_sound(self);

	// get ending position
	VectorMA(start, attack_range, aimdir, end);

	// trace from attacker to ending position
	trace_t tr = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);

	//vec3_t org;
	// try to attack
	if (radius > 0)
	{
		edict_t* targ;
		if ((targ = CL_findtarget(self, tr.endpos, radius, false)) != NULL)
		{
			//FIXME: randomize endpos within the bbox of target
			CL_attack(self, targ, damage);
			enemy = targ;
			G_EntMidPoint(targ, end);
			end[0] = end[0] + GetRandom(0, (int)targ->maxs[0]) * crandom();
			end[1] = end[1] + GetRandom(0, (int)targ->maxs[1]) * crandom();
			// recalculate starting position directly above endpoint
			const trace_t tr = gi.trace(end, NULL, NULL, tv(end[0], end[1], 8192), self, MASK_SOLID);
			VectorCopy(tr.endpos, start);
		}
		else
			VectorCopy(tr.endpos, end);
	}
	else if (G_ValidTarget(self, tr.ent, true, false))
	{
		CL_attack(self, tr.ent, damage);
		enemy = tr.ent;
		VectorCopy(tr.endpos, end);
	}
	//else
	//	return;

	// lightning graphical effect
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_HEATBEAM);
	gi.WriteShort(self - g_edicts);
	gi.WritePosition(start);
	gi.WritePosition(end);
	gi.multicast(end, MULTICAST_PVS);

	if (!enemy)
		return;

	//gi.dprintf("%d: spawned CL ent\n", (int)level.framenum);
	// spawn lightning entity
	edict_t* lightning = G_Spawn();
	lightning->classname = "lightning";
	lightning->solid = SOLID_NOT;
	lightning->svflags |= SVF_NOCLIENT;
	lightning->owner = G_GetClient(self); // for lightning storm: to ensure the owner is the player, not the LS!
	//lightning->delay = level.time + duration;
	lightning->nextthink = level.time + FRAMETIME;
	lightning->think = chainlightning_think;
	VectorCopy(end, lightning->s.origin);
	VectorCopy(end, lightning->s.old_origin);
	VectorCopy(aimdir, lightning->movedir);
	lightning->dmg_radius = radius;
	lightning->dmg = damage * CLIGHTNING_DMG_MOD; // damage increases with each hop
	lightning->monsterinfo.sight_range = hop_range;
	lightning->light_level = max_hops;
	gi.linkentity(lightning);

	// add enemy to the list so we don't attack again
	lightning->monsterinfo.dmglist[lightning->monsterinfo.target_index++].player = enemy;
}

void Cmd_ChainLightning_f (edict_t *ent, float skill_mult, float cost_mult)
{
	int damage=CLIGHTNING_INITIAL_DMG+CLIGHTNING_ADDON_DMG*ent->myskills.abilities[LIGHTNING].current_level;
	const int attack_range=CLIGHTNING_INITIAL_AR+CLIGHTNING_ADDON_AR*ent->myskills.abilities[LIGHTNING].current_level;
	const int hop_range=CLIGHTNING_INITIAL_HR+CLIGHTNING_ADDON_HR*ent->myskills.abilities[LIGHTNING].current_level;
	const int cost=CLIGHTNING_COST*cost_mult;
	vec3_t start, forward, right, offset;

	if (!G_CanUseAbilities(ent, ent->myskills.abilities[LIGHTNING].current_level, cost))
		return;
	if (ent->myskills.abilities[LIGHTNING].disable)
		return;

	damage *= skill_mult * vrx_get_synergy_mult(ent, LIGHTNING);

	// get starting position and forward vector
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 8,  ent->viewheight-8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	//ChainLightning(ent, start, forward, damage, attack_range, hop_range);
	fire_chainlightning(ent, start, forward, damage, 0, attack_range, hop_range, 4);

	//Talent: Wizardry - makes spell timer ability-specific instead of global
	const int talentLevel = vrx_get_talent_level(ent, TALENT_WIZARDRY);
	if (talentLevel > 0)
	{
		ent->myskills.abilities[LIGHTNING].delay = level.time + CLIGHTNING_DELAY;
		ent->client->ability_delay = level.time + CLIGHTNING_DELAY * (1 - 0.2 * talentLevel);
	}
	else
		ent->client->ability_delay = level.time + CLIGHTNING_DELAY/* * cost_mult*/;
	ent->client->pers.inventory[power_cube_index] -= cost;
}


