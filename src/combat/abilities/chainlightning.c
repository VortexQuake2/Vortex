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



#define CL_CHECK_AND_ATTACK			1		// check if target is valid, then attack
#define CL_CHECK					2		// check if the target is valid
#define CL_ATTACK					3		// attack only

qboolean ChainLightning_Attack (edict_t *ent, edict_t *target, int damage, int mode)
{
	qboolean result=false;


	if (mode != CL_ATTACK)
	{
		if (G_ValidTargetEnt(target, false) && !OnSameTeam(ent, target))
			result = true;
	}
	else
		result = true;

	if (result && mode != CL_CHECK)
	{
		// do less damage to corpses
		if (target->health < 1 && damage > 100)
			damage = 100;
		//gi.dprintf("CL did %d damage at %d\n", damage, level.framenum);
		// deal damage
		T_Damage(target, ent, ent, vec3_origin, target->s.origin, vec3_origin, 
			damage, damage, DAMAGE_ENERGY, MOD_LIGHTNING);
	}

	return result;
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
		gi.WriteByte(TE_HEATBEAM);
		gi.WriteShort(self - g_edicts);
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
			trace_t tr = gi.trace(end, NULL, NULL, tv(end[0], end[1], 8192), self, MASK_SOLID);
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

void ChainLightning (edict_t *ent, vec3_t start, vec3_t aimdir, int damage, int attack_range, int hop_range)
{
	int		i=0, hops=CLIGHTNING_MAX_HOPS;
	vec3_t	end;
	trace_t	tr;
	edict_t	*target=NULL;
	edict_t	*prev_ed[CLIGHTNING_MAX_HOPS]; // list of entities we've previously hit
	qboolean	found=false;

	//gi.dprintf("ChainLightning damage: %d range: %d hop: %d\n", damage, attack_range, hop_range);
	memset(prev_ed, 0, CLIGHTNING_MAX_HOPS*sizeof(prev_ed[0]));

	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_TELEPORT_EFFECT);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	// calling entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;

	// play sound
    gi.sound(ent, CHAN_ITEM, gi.soundindex("abilities/thunderbolt.wav"), 1, ATTN_NORM, 0);

	// randomize damage
	//damage = GetRandom((int)(0.5*damage), damage);

	// get ending position
	VectorMA(start, attack_range, aimdir, end);

	// trace from attacker to ending position
	tr = gi.trace(start, NULL, NULL, end, ent, MASK_SHOT);

	// is this a non-world entity?
	if (tr.ent && tr.ent != world)
	{
		// try to attack it
		if (ChainLightning_Attack(ent, tr.ent, damage, CL_CHECK_AND_ATTACK))
		{
			// damage is modified with each hop
			damage *= CLIGHTNING_DMG_MOD; 

			prev_ed[0] = tr.ent;
		}
		else
			return; // give up
	}
	
	
	// spawn particle trail
	G_SpawnParticleTrail(start, tr.endpos, CLIGHTNING_PARTICLES, CLIGHTNING_COLOR);

	// we didn't find an entity to jump from
	while (!prev_ed[0] && hops > 0)
	{
		hops--;

		VectorCopy(tr.endpos, start);

		// bounce away from the wall
		VectorMA(aimdir, (-2 * DotProduct(aimdir, tr.plane.normal)), tr.plane.normal, aimdir);
		VectorMA(start, attack_range, aimdir, end);
			
		// trace
		tr = gi.trace(start, NULL, NULL, end, NULL, MASK_SHOT);

		// spawn particle trail
		G_SpawnParticleTrail(start, tr.endpos, CLIGHTNING_PARTICLES, CLIGHTNING_COLOR);

		// we hit nothing, give up
		if (tr.fraction == 1.0)
			return;

		// we hit an entity
		if (tr.ent && tr.ent != world)
		{
			// try to attack
			if (ChainLightning_Attack(ent, tr.ent, damage, CL_CHECK_AND_ATTACK))
			{
				// damage is modified with each hop
				damage *= CLIGHTNING_DMG_MOD; 

				prev_ed[0] = tr.ent;
				break;
			}
			else
				return;// give up
		}
		//FIXME: if we didn't hit an entity, shouldn't we modify damage again?
	}
	
	// we never hit a valid target, so give up
	if (!prev_ed[0])
		return;

	// find nearby targets and bounce between them
	while ((i<CLIGHTNING_MAX_HOPS-1) && ((target = findradius(target, prev_ed[i]->s.origin, hop_range)) != NULL))
	{
		if (target == prev_ed[0])
			continue;

		// try to attack, if successful then add entity to list
		if (ChainLightning_Attack(ent, target, 0, CL_CHECK) && visible(prev_ed[i], target))
		{
			ChainLightning_Attack(ent, target, damage, CL_ATTACK);

			// damage is modified with each hop
			damage *= CLIGHTNING_DMG_MOD; 

			G_SpawnParticleTrail(prev_ed[i]->s.origin, target->s.origin, 
				CLIGHTNING_PARTICLES, CLIGHTNING_COLOR);

			prev_ed[++i] = target;
		}
	}
}

void Cmd_ChainLightning_f (edict_t *ent, float skill_mult, float cost_mult)
{
	int damage=CLIGHTNING_INITIAL_DMG+CLIGHTNING_ADDON_DMG*ent->myskills.abilities[LIGHTNING].current_level;
	int attack_range=CLIGHTNING_INITIAL_AR+CLIGHTNING_ADDON_AR*ent->myskills.abilities[LIGHTNING].current_level;
	int hop_range=CLIGHTNING_INITIAL_HR+CLIGHTNING_ADDON_HR*ent->myskills.abilities[LIGHTNING].current_level;
	int cost=CLIGHTNING_COST*cost_mult;
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
	int talentLevel = vrx_get_talent_level(ent, TALENT_WIZARDRY);
	if (talentLevel > 0)
	{
		ent->myskills.abilities[LIGHTNING].delay = level.time + CLIGHTNING_DELAY;
		ent->client->ability_delay = level.time + CLIGHTNING_DELAY * (1 - 0.2 * talentLevel);
	}
	else
		ent->client->ability_delay = level.time + CLIGHTNING_DELAY/* * cost_mult*/;
	ent->client->pers.inventory[power_cube_index] -= cost;
}


