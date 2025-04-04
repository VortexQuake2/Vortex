#include "g_local.h"

#define PARASITE_ATTACK_FRAMES		20
#define PARASITE_REFIRE				0.5//1.0
#define	PARASITE_INIT_COST			50

#define PARASITE_MAXFRAMES	20
#define PARASITE_RANGE		128
#define PARASITE_DELAY		2
#define PARASITE_COST		50

qboolean ParasiteAttack(edict_t* ent, vec3_t start, vec3_t aimdir, float damage, int pull, int attack_range, int hop_range);

static qboolean parasite_cantarget (edict_t *self, edict_t *target)
{
	int para_range = PARASITE_ATTACK_RANGE;

	return (G_EntExists(target) /* && !que_typeexists(target->curses, CURSE_FROZEN) */
		&& !OnSameTeam(self, target) && visible(self, target) && nearfov(self, target, 45, 45) 
		&& (entdist(self, target) <= para_range));
}


qboolean myparasite_findtarget (edict_t *self)
{
	edict_t *other=NULL;
	int para_range = PARASITE_ATTACK_RANGE;

	while ((other = findclosestradius(other, self->s.origin, para_range)) != NULL)
	{
		if (!parasite_cantarget(self, other))
			continue;
		//if (!G_ValidTarget(self, other, true))
		//	return false;
		//if (!infov(self, other, 90))
		//	return false;
		self->enemy = other;
		return true;
	}
	return false;
}

void myparasite_fire (edict_t *self)
{
	int		pull;
	int		damage;
	int		para_range	= PARASITE_ATTACK_RANGE;
	vec3_t	v, start, end, forward;
	trace_t	tr;

	// monitor attack duration
	if (self->shots >= PARASITE_ATTACK_FRAMES)
	{
		self->shots = 0;
		//self->oldenemy = self->enemy;
		self->enemy = NULL;
		self->wait = level.time + PARASITE_REFIRE;
		return;
	}

	damage = PARASITE_INITIAL_DMG+PARASITE_ADDON_DMG*self->myskills.abilities[BLOOD_SUCKER].current_level;
	// morph mastery increases damage
	if (self->myskills.abilities[MORPH_MASTERY].current_level > 0)
		damage *= 1.5;
	pull = PARASITE_INITIAL_KNOCKBACK + PARASITE_ADDON_KNOCKBACK * self->monsterinfo.level;
	if (PARASITE_MAX_KNOCKBACK && pull < PARASITE_MAX_KNOCKBACK)
		pull = PARASITE_MAX_KNOCKBACK;

	self->lastsound = level.framenum;
	if (self->shots == 0)
		gi.sound (self, CHAN_AUTO, gi.soundindex("parasite/paratck3.wav"), 1, ATTN_NORM, 0);

	// get muzzle position
	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorMA(self->s.origin, self->maxs[1]-8, forward, start);
	start[2] += 2;

	// get a valid target if we don't have one already
	if (!parasite_cantarget(self, self->enemy))
	{
		if (!myparasite_findtarget(self))
		{
			self->enemy = NULL;
			//gi.dprintf("couldn't find target\n");
		}
	}

	if (self->enemy)
	{
		// lock-on to target
		//VectorCopy(self->enemy->s.origin, end);
		G_EntMidPoint(self->enemy, end);
		// is this a corpse?
		if (self->enemy->health < 1)
			end[2] -= 8; // aim towards the ground
		VectorSubtract(end, start, v);
		VectorNormalize(v);
		VectorMA(start, para_range, v, end);

		ParasiteAttack(self, start, v, damage, pull, para_range, para_range);
		
	}
	else
	{
		AngleVectors(self->client->v_angle, forward, NULL, NULL);
		// fire straight ahead
		VectorMA(start, para_range, forward, end);

		ParasiteAttack(self, start, forward, damage, pull, para_range, para_range);
	}

	//tr = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);

	// make sure we are actually hitting our enemy
	//if (tr.ent && (tr.ent==self->enemy))
	/*
	if (G_EntExists(tr.ent) && !OnSameTeam(self, tr.ent))
	{
		pull = PARASITE_INITIAL_KNOCKBACK + PARASITE_ADDON_KNOCKBACK*self->monsterinfo.level;
		if (PARASITE_MAX_KNOCKBACK && pull < PARASITE_MAX_KNOCKBACK)
			pull = PARASITE_MAX_KNOCKBACK;
		if (tr.ent->groundentity)
			pull *= 2;
		T_Damage(tr.ent, self, self, v, tr.endpos, 
			tr.plane.normal, damage, pull, DAMAGE_NO_ABILITIES, MOD_PARASITE);
		if (self->health < 2*self->max_health)
		{
			self->health += damage;
			if (self->health > 2*self->max_health)
				self->health = 2*self->max_health;
		}
	}*/
	
	// make the parasite toungue!
	/*
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_PARASITE_ATTACK);
	gi.WriteShort (self-g_edicts);
	gi.WritePosition (start);
	gi.WritePosition (tr.endpos);
	gi.multicast (start, MULTICAST_PVS);
	*/
	self->shots++;
	if (self->shots == PARASITE_ATTACK_FRAMES)
		gi.sound (self, CHAN_AUTO, gi.soundindex("parasite/paratck4.wav"), 1, ATTN_NORM, 0);
}

qboolean ParasiteCanAttack (edict_t *ent)
{
	// if a player initiated a jump and they are not in the water, then don't let them attack
	if (!ent->groundentity && ent->waterlevel < 1 && ent->monsterinfo.jumpup)
		return false;

	if (level.time < ent->wait)
		return false; // re-fire time
	if (ent->monsterinfo.attack_finished > level.time)
		return false;

	return (((level.time > pregame_time->value) || pvm->value) && (level.time > ent->client->respawn_time) 
		&& (!que_typeexists(ent->curses, CURSE_FROZEN)) && (level.time > ent->holdtime));
}

qboolean myparasite_attack (edict_t *self)
{
//	int	range=PARASITE_ATTACK_RANGE;

	if (!ParasiteCanAttack(self))
	{
		//gi.dprintf("can't attack\n");
		return false;
	}

	myparasite_fire(self);
	return true;
/*
	// find a valid target if we dont have one
	if (!self->enemy)
	{
		if (myparasite_findtarget(self))
		{
			myparasite_fire(self);
			return true;
		}
		return false;
	}
	// make sure our enemy is still a valid target, and we haven't exhausted our attack
	else if (parasite_cantarget(self, self->enemy) && (self->shots<PARASITE_ATTACK_FRAMES)
		&& (entdist(self, self->enemy)<range))
	{
		myparasite_fire(self);
		return true;
	}
	else
	{
		self->shots = 0;
		self->oldenemy = self->enemy;
		self->enemy = NULL;
		self->wait = level.time + PARASITE_REFIRE;
		return false;
	}
*/
}

void RunParasiteFrames (edict_t *ent, usercmd_t *ucmd)
{
	qboolean idle = false;

	// if we aren't a parasite or we are dead, we shouldn't be here!
	if ((ent->mtype != M_MYPARASITE) || (ent->deadflag == DEAD_DEAD))
		return;

	ent->s.modelindex2 = 0;
	ent->s.skinnum = 0;

	if (level.framenum >= ent->count)
	{
		ent->count = level.framenum + qf2sf(1);

		// play running animation if we are moving forward or strafing
		if ((ucmd->forwardmove > 0) || ucmd->sidemove)
			G_RunFrames(ent, 70, 76, false);
		// play animation in reverse if we are going backwards
		else if (ucmd->forwardmove < 0)
			G_RunFrames(ent, 70, 76, true);
		else
			idle = true;
		
		if ((ent->client->buttons & BUTTON_ATTACK) && myparasite_attack(ent) && idle)
		{
			// run attack frames
			G_RunFrames(ent, 39, 51, false);
			ent->client->idle_frames = 0;
		}
		else if (idle)
		{
			// if we were attacking, reset it
			if (ent->shots)
			{
				ent->shots = 0;
				ent->enemy = NULL;
				ent->wait = level.time + PARASITE_REFIRE;
			}

			// if we just finished an attack
			if (ent->oldenemy)
			{
				if (ent->s.frame < 56)
					G_RunFrames(ent, 39, 56, false); // run end attack frames
				else
					ent->oldenemy = NULL; // we're done
			}
			else
				G_RunFrames(ent, 83, 99, false); // run idle frames

			// play parasite's tapping sound
			if (ent->groundentity && ((ent->s.frame==85) || (ent->s.frame==87) || (ent->s.frame==91) 
				|| (ent->s.frame==93) || (ent->s.frame==97) || (ent->s.frame==99)))
			gi.sound (ent, CHAN_WEAPON, gi.soundindex("parasite/paridle1.wav"), 1, ATTN_IDLE, 0);
		}
	}
}



void parasite_endattack (edict_t *ent)
{
	gi.sound (ent, CHAN_AUTO, gi.soundindex("parasite/paratck4.wav"), 1, ATTN_NORM, 0);
	ent->sucking = false;
	ent->parasite_frames = 0;
}

void think_ability_parasite_attack(edict_t *ent)
{
	int		damage, kick;
	vec3_t	forward, right, start, end, offset;
	trace_t	tr;

	if (debuginfo->value)
		gi.dprintf("%s just called think_ability_parasite_attack()\n", ent->client->pers.userinfo);

	// terminate attack
	if (!G_EntIsAlive(ent) || (ent->parasite_frames > qf2sf(PARASITE_MAXFRAMES))
		|| que_typeexists(ent->curses, CURSE_FROZEN))
	{
		parasite_endattack(ent);
		return;
	}

	// calculate starting point
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 7,  ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	// do we already have a valid target?
	if (G_ValidTarget(ent, ent->parasite_target, true, true)
		&& (entdist(ent, ent->parasite_target) <= PARASITE_RANGE)
		&& infov(ent, ent->parasite_target, 90))
	{
		VectorSubtract(ent->parasite_target->s.origin, start, forward);
		VectorNormalize(forward);
	}

	VectorMA(start, PARASITE_RANGE, forward, end);
	tr = gi.trace(start, NULL, NULL, end, ent, MASK_SHOT);
	// did we hit something?
	if (G_EntIsAlive(tr.ent) && !OnSameTeam(ent, tr.ent))
	{
		if (ent->parasite_frames == 0)
		{
			ent->client->pers.inventory[power_cube_index] -= PARASITE_COST;
			gi.sound (ent, CHAN_AUTO, gi.soundindex("parasite/paratck3.wav"), 1, ATTN_NORM, 0);
			ent->client->ability_delay = level.time + PARASITE_DELAY;
		}

		ent->parasite_target = tr.ent;
		damage = 2*ent->myskills.abilities[BLOOD_SUCKER].current_level;
		if (tr.ent->groundentity)
			kick = -100;
		else
			kick = -50;

		T_Damage(tr.ent, ent, ent, forward, tr.endpos,
				 tr.plane.normal, damage, kick, DAMAGE_NO_ABILITIES, MOD_PARASITE);

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_PARASITE_ATTACK);
		gi.WriteShort(ent-g_edicts);
		gi.WritePosition(start);
		gi.WritePosition(tr.endpos);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

		ent->parasite_frames++;
	}
	else if (ent->parasite_frames)
		parasite_endattack(ent);

}

void Cmd_PlayerToParasite_f (edict_t *ent)
{
	int para_cubecost = PARASITE_INIT_COST;

	if (debuginfo->value)
		gi.dprintf("DEBUG: %s just called Cmd_PlayerToParasite_f()\n", ent->client->pers.netname);

	// try to switch back
	if (ent->mtype || PM_PlayerHasMonster(ent))
	{
		// don't let a player-tank unmorph if they are cocooned
		if (ent->owner && ent->owner->inuse && ent->owner->movetype == MOVETYPE_NONE)
			return;

		if (que_typeexists(ent->curses, 0))
		{
			safe_cprintf(ent, PRINT_HIGH, "You can't morph while cursed!\n");
			return;
		}

		V_RestoreMorphed(ent, 0);
		return;
	}

    //Talent: Morphing
    //if (vrx_get_talent_slot(ent, TALENT_MORPHING) != -1)
    //    para_cubecost *= 1.0 - 0.25 * vrx_get_talent_level(ent, TALENT_MORPHING);

//	if (!G_CanUseAbilities(ent, ent->myskills.abilities[BLOOD_SUCKER].current_level, para_cubecost))
    //	return;
    if (!V_CanUseAbilities(ent, BLOOD_SUCKER, para_cubecost, true))
        return;

    if (vrx_has_flag(ent)) {
        safe_cprintf(ent, PRINT_HIGH, "Can't morph while carrying flag!\n");
        return;
    }

    V_ModifyMorphedHealth(ent, M_MYPARASITE, true);

    ent->wait = level.time + 0.5;// can't attack immediately

    ent->client->pers.inventory[power_cube_index] -= para_cubecost;
	ent->client->ability_delay = level.time + PARASITE_DELAY;

	ent->mtype = M_MYPARASITE;
	ent->s.modelindex = gi.modelindex ("models/monsters/parasite/tris.md2");
	ent->s.modelindex2 = 0;
	ent->s.skinnum = 0;

	// decloak
	ent->svflags &= ~SVF_NOCLIENT;
	ent->client->cloaking = false;
	ent->client->cloakable = 0;

	ent->maxs[2] = 8;
	ent->viewheight = 0;

	ent->client->refire_frames = 0; // reset charged weapon
	ent->client->weapon_mode = 0; // reset weapon mode
	ent->client->pers.weapon = NULL;
	ent->client->ps.gunindex = 0;

	lasersight_off(ent);

    gi.sound(ent, CHAN_WEAPON, gi.soundindex("abilities/morph.wav"), 1, ATTN_NORM, 0);
}

// **************************************************************************************

#define PARASITE_MAX_HOPS	5

qboolean ParasiteDamageTarget(edict_t* self, edict_t *target, vec3_t dir, vec3_t point, vec3_t normal, float damage, int pull)
{
	if (G_EntExists(target) && !OnSameTeam(self,target))
	{
		if (target->groundentity)
			pull *= 2;
		T_Damage(target, self, self, dir, point, normal, damage, pull, DAMAGE_NO_ABILITIES, MOD_PARASITE);
		if (self->health < 2 * self->max_health)
		{
			self->health += damage;
			if (self->health > 2 * self->max_health)
				self->health = 2 * self->max_health;
		}
		return true;
	}
	return false;
}

qboolean ParasiteAttack(edict_t* ent, vec3_t start, vec3_t aimdir, float damage, int pull, int attack_range, int hop_range)
{
	vec3_t	end;
	trace_t	tr;
	edict_t* target = NULL;
	edict_t* prev_ed[PARASITE_MAX_HOPS]; // list of entities we've previously hit
	qboolean	found = false;

	memset(prev_ed, 0, PARASITE_MAX_HOPS * sizeof(prev_ed[0]));

	// calling entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;

	// get ending position
	VectorMA(start, attack_range, aimdir, end);

	// trace from attacker to ending position
	tr = gi.trace(start, NULL, NULL, end, ent, MASK_SHOT);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_PARASITE_ATTACK);
	gi.WriteShort(ent - g_edicts);
	gi.WritePosition(start);
	gi.WritePosition(tr.endpos);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	// hit a non-world entity?
	if (tr.ent && tr.ent != world)
	{
		// try to attack it
		if (ParasiteDamageTarget(ent, tr.ent, aimdir, tr.endpos, tr.plane.normal, damage, pull))
		{
			prev_ed[0] = tr.ent;
		}
		else
			return false; // give up
	}

	// we never hit a valid target, so give up
	if (!prev_ed[0])
		return false;

	int i = 0;
	// Talent: Melee Mastery - allows parasite tongue to strike multiple targets like chain lightning
	int max_hops = floattoint(vrx_get_talent_level(ent, TALENT_MELEE_MASTERY) * 0.5) + 1;
	if (max_hops > PARASITE_MAX_HOPS)
		max_hops = PARASITE_MAX_HOPS;
	//gi.dprintf("max hops: %d\n", max_hops);

	if (max_hops < 2)
		return false;

	// find nearby targets and bounce between them
	while ((i < max_hops - 1) && ((target = findradius(target, prev_ed[i]->s.origin, hop_range)) != NULL))
	{
		if (target == prev_ed[0])
			continue;
		// calculate vector between previous entity and next--this will cause the next entity to get pulled toward the previous
		vec3_t v;
		VectorSubtract(target->s.origin, prev_ed[i]->s.origin, v);
		VectorNormalize(v);
		// try to attack, if successful then add entity to list
		if (ParasiteDamageTarget(ent, target, v, target->s.origin, vec3_origin, damage, pull))
		{
			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_PARASITE_ATTACK);
			gi.WriteShort(prev_ed[i] - g_edicts);
			gi.WritePosition(prev_ed[i]->s.origin);
			gi.WritePosition(target->s.origin);
			gi.multicast(prev_ed[i]->s.origin, MULTICAST_PVS);

			prev_ed[++i] = target;
		}
	}
	return true;
}