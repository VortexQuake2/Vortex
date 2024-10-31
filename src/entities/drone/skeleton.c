/*
==============================================================================

SKELETON

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/skeleton.h"

#define SKELETON_TYPE_ICE			0			// ice skeleton skinnum
#define SKELETON_TYPE_POISON		1			// poison skeleton skinnum
#define SKELETON_TYPE_FIRE			2			// fire skeleton skinnum

#define SKELETON_RAPID_ATTACKS		4			// number of rapid-fire slash attacks before attack move ends
#define SKELETON_REVIVE_TIME		5.0			// seconds before a skeleton auto-revives
#define SKELETON_ELEMENTAL_CHANCE	0.33		// chance a melee strike causes elemental damage/effects

static int sound_step1;
static int sound_step2;
static int sound_step3;
static int sound_step4;
static int sound_step5;
static int sound_step6;

static int sound_death1;
static int sound_death2;
static int sound_death3;
static int sound_death4;
static int sound_death5;

static int sound_swing1;
static int sound_swing2;

static int sound_swing_hit1;
static int sound_swing_hit2;
static int sound_swing_hit3;
static int sound_swing_hit4;
static int sound_swing_hit5;
static int sound_swing_hit6;

static int sound_raise;

mframe_t skeleton_frames_stand[] =
{
	drone_ai_stand, 0, NULL //FRAME_stand01
};
mmove_t	skeleton_move_stand = { FRAME_stand01, FRAME_stand01, skeleton_frames_stand, NULL };

void skeleton_stand(edict_t* self)
{
	self->superspeed = false;
	self->monsterinfo.currentmove = &skeleton_move_stand;
}

void skeleton_step(edict_t* self)
{
	int sound;

	switch (GetRandom(1, 6))
	{
	case 1: sound = sound_step1; break;
	case 2: sound = sound_step2; break;
	case 3: sound = sound_step3; break;
	case 4: sound = sound_step4; break;
	case 5: sound = sound_step5; break;
	case 6: sound = sound_step6; break;
	}
	gi.sound(self, CHAN_BODY, sound, 1.0, ATTN_STATIC, 0);
}

void skeleton_death(edict_t* self)
{
	int sound;

	switch (GetRandom(1, 5))
	{
	case 1: sound = sound_death1; break;
	case 2: sound = sound_death2; break;
	case 3: sound = sound_death3; break;
	case 4: sound = sound_death4; break;
	case 5: sound = sound_death5; break;
	}
	gi.sound(self, CHAN_BODY, sound, 1, ATTN_NORM, 0);
}

mframe_t skeleton_frames_walk[] =
{
	drone_ai_walk, 5, skeleton_step,//step L
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, skeleton_step,//step R
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL//FRAME_walk06
};
mmove_t skeleton_move_walk = { FRAME_walk01, FRAME_walk06, skeleton_frames_walk, NULL };

void skeleton_walk(edict_t* self)
{
	if (!self->goalentity)
		self->goalentity = world;
	self->monsterinfo.currentmove = &skeleton_move_walk;
}

void skeleton_step_charge(edict_t* self)
{
	skeleton_step(self);
	//self->s.effects |= EF_TAGTRAIL;
	//gi.dprintf("skeleton superspeed ON\n");
	//self->superspeed = true;
}

void skeleton_charge(edict_t* self)
{
	//self->s.effects |= EF_TAGTRAIL;
	//gi.dprintf("skeleton superspeed OFF\n");
	//self->superspeed = false;
}

mframe_t skeleton_frames_run[] =
{
	drone_ai_run, 25, skeleton_step,//step L
	drone_ai_run, 25, NULL,
	drone_ai_run, 25, NULL,
	drone_ai_run, 25, skeleton_step,//step R
	drone_ai_run, 25, NULL,
	drone_ai_run, 25, NULL//FRAME_run06
};
mmove_t skeleton_move_run = { FRAME_run01, FRAME_run06, skeleton_frames_run, NULL };

void skeleton_run(edict_t* self);

mframe_t skeleton_frames_charge[] =
{
	drone_ai_run, 40, skeleton_step_charge,//step L
	drone_ai_run, 40, skeleton_charge,
	drone_ai_run, 40, skeleton_charge,
	drone_ai_run, 40, skeleton_step_charge,//step R
	drone_ai_run, 40, skeleton_charge,
	drone_ai_run, 40, skeleton_charge//FRAME_run06
};
mmove_t skeleton_move_charge = { FRAME_run01, FRAME_run06, skeleton_frames_charge, skeleton_run };

qboolean skeleton_can_sprint(edict_t* self)
{
	return self->enemy && self->enemy->inuse && visible(self, self->enemy) // enemy is visible
		&& (fabs(self->absmin[2] - self->enemy->absmin[2]) <= 18 || self->enemy->flags & FL_FLY); // and Z coordinates are within +/- stepsize, or flying

}

void skeleton_run(edict_t* self)
{
	self->superspeed = false;
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &skeleton_move_stand;
	else if (skeleton_can_sprint(self))
	{
		//gi.dprintf("skeleton is sprinting\n");
		self->monsterinfo.currentmove = &skeleton_move_charge;
		self->superspeed = true;
	}
	else
		self->monsterinfo.currentmove = &skeleton_move_run;
}

void skeleton_end_swing(edict_t* self)
{
	//self->light_level = 0;
}

void skeleton_attack_swing(edict_t* self);

mframe_t skeleton_frames_attack_swing[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,//pullback
	ai_charge, 0, NULL,//swing L
	ai_charge, 0, skeleton_attack_swing,//strike
	ai_charge, 0, NULL,//swing R
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, skeleton_end_swing //FRAME_attack08
};
mmove_t skeleton_move_attack_swing = { FRAME_attack01, FRAME_attack08, skeleton_frames_attack_swing, skeleton_run };

edict_t* drone_get_enemy(edict_t* self, float range);

qboolean skeleton_findtarget(edict_t* self)
{
	edict_t* e;

	if ((e = drone_get_enemy(self, M_MELEE_RANGE)) != NULL)
	{
		vec3_t v;

		//if (!self->groundentity)
		//	gi.dprintf("skeleton found new enemy while mid-air!");
		//gi.dprintf("skeleton found new target within attack move!\n");
		self->enemy = e;
		// face new target
		VectorSubtract(self->enemy->s.origin, self->s.origin, v);
		self->s.angles[YAW] = vectoyaw(v);
		// reset counter for continous attacks
		self->light_level = 0;
		return true;
	}
	return false;
}

void skeleton_check_landing(edict_t* self)
{
	// hold frame unless we find an enemy, touch the ground, or the jump times out
	if (skeleton_findtarget(self) || self->groundentity || level.time > self->monsterinfo.pausetime)
	{
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	}
	else
	{
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
	}

	
}

void skeleton_jump(edict_t* self)
{
	int	speed = 800;
	vec3_t	forward, start;

	//gi.dprintf("%d: baron_fire_jump %d\n", (int)(level.framenum), self->s.frame);
	//gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);

	self->lastsound = level.framenum;

	self->groundentity = NULL;

	MonsterAim(self, -1, speed, false, 0, forward, start);
	self->velocity[0] += forward[0] * speed;
	self->velocity[1] += forward[1] * speed;
	self->velocity[2] += forward[2] * speed;

	//self->touch = mutant_jump_touch;
	self->monsterinfo.pausetime = level.time + 2.0; // maximum duration of jump
	self->velocity[2] += 200;

	//self->s.frame = FRAME_attack05; // skip to airborne jump frame
	//self->monsterinfo.aiflags |= AI_HOLD_FRAME; // hold this frame
}

void skeleton_check_attack(edict_t* self);

mframe_t skeleton_frames_jump[] =
{
	ai_charge, 0, skeleton_jump,
	ai_charge, 0, skeleton_check_landing,
	ai_charge, 0, NULL,
	ai_charge, 0, skeleton_check_attack,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t skeleton_move_jump = { FRAME_jump01, FRAME_jump06, skeleton_frames_jump, skeleton_run };

mframe_t skeleton_frames_attack_crouch[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,//pullback
	ai_charge, 0, NULL,
	ai_charge, 0, skeleton_attack_swing,//strike
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t skeleton_move_attack_crouch = { FRAME_crattack01, FRAME_crattack10, skeleton_frames_attack_crouch, skeleton_run };

void skeleton_check_attack(edict_t* self)
{
	if (self->enemy && self->enemy->inuse && entdist(self, self->enemy) <= M_MELEE_RANGE)
	{
		//gi.dprintf("landed close to enemy--launching crouch attack!\n");
		self->monsterinfo.currentmove = &skeleton_move_attack_crouch;
		self->monsterinfo.nextframe = FRAME_crattack02;
	}
}

void skeleton_swing_hit(edict_t* self)
{
	int sound;

	switch (GetRandom(1, 6))
	{
	case 1: sound = sound_swing_hit1; break;
	case 2: sound = sound_swing_hit2; break;
	case 3: sound = sound_swing_hit3; break;
	case 4: sound = sound_swing_hit4; break;
	case 5: sound = sound_swing_hit5; break;
	case 6: sound = sound_swing_hit6; break;
	}
	gi.sound(self, CHAN_BODY, sound, 1, ATTN_NORM, 0);
}

void poison_target(edict_t* ent, edict_t* target, int damage, float duration, int meansOfdeath, qboolean stack);
void chill_target(edict_t* target, int chill_level, float duration);

void skeleton_attack_swing(edict_t* self)
{
	int		damage;
	edict_t* e;

	// enemy is invalid or dead?
	if (!G_EntIsAlive(self->enemy))
	{
		//gi.dprintf("skeleton finding a new target within attack move\n");
		// immediately try to find a new one without ending the attack
		if (!skeleton_findtarget(self))
			return;
	}

	damage = M_MELEE_DMG_BASE + M_MELEE_DMG_ADDON * drone_damagelevel(self); // dmg: berserker_attack_club
	if (M_MELEE_DMG_MAX && damage > M_MELEE_DMG_MAX)
		damage = M_MELEE_DMG_MAX;

	// weapon swinging sound
	if (random() < 0.5)
		gi.sound(self, CHAN_WEAPON, sound_swing1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_WEAPON, sound_swing2, 1, ATTN_NORM, 0);

	if ((e = M_MeleeAttack(self, M_MELEE_RANGE, damage, 0)) != NULL)
	{
		//gi.dprintf("skeleton swing attack caused enemy to burn\n");
		// a successful strike has a chance to cause elemental damage/effects
		if (random() <= SKELETON_ELEMENTAL_CHANCE)
		{
			int slvl = drone_damagelevel(self);
			if (self->s.skinnum == SKELETON_TYPE_ICE) // blue skin = cold enchanted
			{
				float duration = ICEBOLT_INITIAL_CHILL_DURATION + ICEBOLT_ADDON_CHILL_DURATION * slvl;
				chill_target(e, (2 * slvl), duration);
			}
			else if (self->s.skinnum == SKELETON_TYPE_POISON) // green skin = poison enchanted
			{
				damage = POISON_INITIAL_DAMAGE + POISON_ADDON_DAMAGE * slvl;
				damage = vrx_increase_monster_damage_by_talent(self->activator, damage);
				//gi.dprintf("skeleton poisoned target: damage %d\n", damage);
				poison_target(self->activator, e, damage, 10.0, MOD_GAS, true);
			}
			else // fire enchanted
			{
				damage = FIREBALL_INITIAL_FLAMEDMG + FIREBALL_ADDON_FLAMEDMG * self->monsterinfo.level;
				damage = vrx_increase_monster_damage_by_talent(self->activator, damage);
				//gi.dprintf("skeleton burned target: damage %d\n", damage);
				burn_person(e, self, damage);
			}
		}
		else // weapon hitting sound
			skeleton_swing_hit(self);
	}

	// continue rapid attack if enemy is within melee range
	if (entdist(self, self->enemy) <= M_MELEE_RANGE && self->light_level < SKELETON_RAPID_ATTACKS)
	{
		if (self->monsterinfo.currentmove == &skeleton_move_attack_crouch)
			self->s.frame = FRAME_crattack02;
		else
			self->s.frame = FRAME_attack02;
		self->light_level++;
	}
}

void skeleton_fire_magic(edict_t* self)
{
	//monster_fire_poison(self);
	//monster_fire_fireball(self);
	//monster_fire_icebolt(self);
	if (self->s.skinnum == SKELETON_TYPE_ICE)
	{
		// ice attack
		monster_fire_icebolt(self);
	}
	else if (self->s.skinnum == SKELETON_TYPE_POISON)
	{
		// poison attack
		monster_fire_poison(self);
	}
	else
	{
		// fire attack
		monster_fire_fireball(self);
	}
}

mframe_t skeleton_frames_wave[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, skeleton_fire_magic,//raised
	ai_charge, 0, NULL,//flat
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,//fist
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t skeleton_move_wave = { FRAME_wave01, FRAME_wave11, skeleton_frames_wave, skeleton_run };

void skeleton_attack(edict_t* self)
{
	float	r = random();
	float	dist = entdist(self, self->enemy);
	mmove_t* prev_move = self->monsterinfo.currentmove;

	self->light_level = 0; // reset counter for continuous or rapid-fire slashing attacks

	// short range attacks
	if (dist <= M_MELEE_RANGE)
	{
		if (random() < 0.5)
			self->monsterinfo.currentmove = &skeleton_move_attack_swing;
		else
			self->monsterinfo.currentmove = &skeleton_move_attack_crouch;
	}
	else if (dist <= 512) // medium range attacks
	{
		if (self->monsterinfo.aiflags & AI_STAND_GROUND || self->enemy->flags & FL_FLY || r < 0.2)
			self->monsterinfo.currentmove = &skeleton_move_wave;
		// jump towards enemy if they're on even ground (+/- STEPSIZE)
		else if (dist > 200 && (fabs(self->absmin[2] - self->enemy->absmin[2]) <= 18) && r < 0.5)
			self->monsterinfo.currentmove = &skeleton_move_jump;

		self->monsterinfo.attack_finished = level.time + 0.5;
		//M_DelayNextAttack(self, (float)GetRandom(0, 2), true);
	}

	// switching attacks?
	if (prev_move != self->monsterinfo.currentmove)
		self->superspeed = false;
}

void skeleton_melee(edict_t* self)
{

}

void skeleton_set_bbox(vec3_t mins, vec3_t maxs)
{
	VectorSet(mins, -16, -16, -18);
	VectorSet(maxs, 16, 16, 38);
}

void skeleton_revive(edict_t* self)
{
	//gi.dprintf("revive!\n");
	self->movetype = MOVETYPE_STEP;
	self->svflags &= ~SVF_DEADMONSTER;
	self->deadflag = DEAD_NO;
	self->enemy = NULL;
	self->monsterinfo.attack_finished = level.time + 1.0;
	//skeleton_set_bbox(self->mins, self->maxs);
	gi.linkentity(self);
	self->monsterinfo.currentmove = &skeleton_move_stand;
	if (G_EntIsAlive(self->activator))
		self->monsterinfo.leader = self->activator;
	self->health = self->max_health;
	gi.sound(self, CHAN_VOICE, sound_raise, 1, ATTN_NORM, 0);
}

mframe_t skeleton_frames_revive[] =
{
	ai_move, 0,	 NULL,//FRAME_death08
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL //FRAME_death01
};
mmove_t skeleton_move_revive = { FRAME_death08, FRAME_death01, skeleton_frames_revive, skeleton_revive };

void skeleton_wait_for_revival(edict_t* self)
{
	vec3_t mins, maxs;
	trace_t tr;

	// not ready to revive yet? hold current frame
	if (self->monsterinfo.pausetime > level.time)
	{
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
		return;
	}

	// can't see the leader and too far away from him?
	if (self->monsterinfo.leader && entdist(self, self->monsterinfo.leader) > 512 && !visible(self, self->monsterinfo.leader))
	{
		TeleportNearArea(self, self->activator->s.origin, 256, false);
	}

	skeleton_set_bbox(mins, maxs);
	tr = gi.trace(self->s.origin, mins, maxs, self->s.origin, self, MASK_MONSTERSOLID); // trace box

	// check for obstructions
	if (!tr.startsolid && !tr.allsolid && tr.fraction == 1.0)
	{
		//gi.dprintf("begin revive\n");
		// begin revive frames
		self->monsterinfo.currentmove = &skeleton_move_revive;
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
		// set bounding box before another entity gets in the way!
		skeleton_set_bbox(self->mins, self->maxs);
		gi.linkentity(self);
	}
}

void skeleton_dead(edict_t* self)
{
	VectorSet(self->mins, -16, -16, -18);
	VectorSet(self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	//self->nextthink = 0;
	gi.linkentity(self);
	//M_PrepBodyRemoval(self);
}

mframe_t skeleton_frames_death[] =
{
	ai_move, 0,	 NULL,//FRAME_death01
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 skeleton_wait_for_revival //FRAME_death08
};
mmove_t skeleton_move_death = { FRAME_death01, FRAME_death08, skeleton_frames_death, NULL };

void skeleton_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, vec3_t point)
{
	self->superspeed = false;

	// skeleton is only removed if his bones are "gibbed"
	if (self->health <= self->gib_health)
	{
		M_Notify(self);
		M_Remove(self, false, true);
		return;
	}

	if (self->deadflag == DEAD_DYING)
		return;
	skeleton_dead(self);
	//DroneList_Remove(self);
	skeleton_death(self);
	self->monsterinfo.pausetime = level.time + SKELETON_REVIVE_TIME; // time until auto-revive
	self->deadflag = DEAD_DYING;
	//self->takedamage = DAMAGE_YES;
	self->monsterinfo.currentmove = &skeleton_move_death;
}

void init_skeleton(edict_t* self)
{
	sound_step1 = gi.soundindex("skeleton/walk1.wav");
	sound_step2 = gi.soundindex("skeleton/walk2.wav");
	sound_step3 = gi.soundindex("skeleton/walk3.wav");
	sound_step4 = gi.soundindex("skeleton/walk4.wav");
	sound_step5 = gi.soundindex("skeleton/walk5.wav");
	sound_step6 = gi.soundindex("skeleton/walk6.wav");

	sound_death1 = gi.soundindex("skeleton/death1.wav");
	sound_death2 = gi.soundindex("skeleton/death2.wav");
	sound_death3 = gi.soundindex("skeleton/death3.wav");
	sound_death4 = gi.soundindex("skeleton/death4.wav");
	sound_death5 = gi.soundindex("skeleton/death5.wav");

	sound_swing1 = gi.soundindex("weapons/1h_swing_lg01.wav");
	sound_swing2 = gi.soundindex("weapons/1h_swing_lg02.wav");

	sound_swing_hit1 = gi.soundindex("weapons/sword1_impact.wav");
	sound_swing_hit2 = gi.soundindex("weapons/sword2_impact.wav");
	sound_swing_hit3 = gi.soundindex("weapons/sword3_impact.wav");
	sound_swing_hit4 = gi.soundindex("weapons/sword4_impact.wav");
	sound_swing_hit5 = gi.soundindex("weapons/sword5_impact.wav");
	sound_swing_hit6 = gi.soundindex("weapons/sword6_impact.wav");

	sound_raise = gi.soundindex("skeleton/raiseskeleton.wav");

	self->s.modelindex = gi.modelindex("models/monsters/skel/tris.md2");

	//VectorSet(self->mins, -16, -16, -18);
	//VectorSet(self->maxs, 16, 16, 38);
	skeleton_set_bbox(self->mins, self->maxs);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->monsterinfo.leader = self->activator;//always follow our leader!
	self->health = self->max_health = SKELETON_INITIAL_HEALTH + SKELETON_ADDON_HEALTH * self->monsterinfo.level;
	self->gib_health = -self->max_health;
	self->mass = 250;
	self->viewheight = 20;
	self->monsterinfo.control_cost = 0;
	self->monsterinfo.cost = M_DEFAULT_COST;//FIXME
	self->monsterinfo.jumpup = 64;
	self->monsterinfo.jumpdn = 512;
	self->monsterinfo.aiflags |= AI_NO_CIRCLE_STRAFE;
	self->mtype = M_SKELETON;
	self->flags |= FL_UNDEAD;
	//self->s.skinnum = GetRandom(0, 2);

	//self->pain = berserk_pain;
	//self->monsterinfo.pain_chance = 0.15f;

	self->die = skeleton_die;

	self->monsterinfo.stand = skeleton_stand;
	//self->monsterinfo.walk = skeleton_walk;
	self->monsterinfo.run = skeleton_run;
	//self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = skeleton_attack;
	self->monsterinfo.melee = skeleton_melee;
	//self->monsterinfo.sight = berserk_sight;
	//self->monsterinfo.search = berserk_search;

	self->monsterinfo.currentmove = &skeleton_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;

	gi.linkentity(self);

	//self->nextthink = level.time + FRAMETIME;
}

qboolean spawn_skeleton(edict_t* ent, vec3_t start, int skill_level, int type)
{
	edict_t *e = G_Spawn();
	e->activator = ent;
	e->monsterinfo.level = skill_level;
	e->mtype = M_SKELETON;
	e->s.skinnum = type;
	
	if (!M_Initialize(ent, e, 1.0) || !G_IsValidLocation(NULL, start, e->mins, e->maxs))
	{
		DroneList_Remove(e);
		layout_remove_tracked_entity(&ent->client->layout, e);
		G_FreeEdict(e);
		return false;
	}
	
	e->monsterinfo.currentmove = &skeleton_move_revive;
	VectorCopy(start, e->s.origin);
	gi.linkentity(e);
	e->nextthink = level.time + FRAMETIME;

	ent->num_skeletons++;
	safe_cprintf(ent, PRINT_HIGH, "Raised a %s. (%d/%d)\n", V_GetMonsterName(e), ent->num_skeletons, (int)SKELETON_MAX);
	return true;
}

// find a single valid target that is in-range and nearest to the aiming reticle
qboolean raise_skeleton_from_corpse(edict_t* ent, float range, int type)
{
	int slvl = ent->myskills.abilities[SKELETON].current_level;
	edict_t* e = NULL;
	vec3_t start;
	qboolean found = false;

	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_TELEPORT_EFFECT);
	gi.WritePosition(ent->s.origin);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	ent->client->idle_frames = 0; // disable cp/cloak on caster
	ent->client->ability_delay = level.time + 1.0; // avoid spell spam

	while ((e = findreticle(e, ent, range, 90, true)) != NULL) {
		if (ent->num_skeletons >= SKELETON_MAX)
			break;
		if (!G_EntExists(e))
			continue;
		if (e->health > 0)
			continue;
		if (e->max_health < 1)
			continue;

		VectorCopy(e->s.origin, start);
		start[2] = e->absmin[2] + 19; // add the mins bbox height of the skeleton + 1

		// kill the corpse
		T_Damage(e, e, e, vec3_origin, e->s.origin, vec3_origin, 10000, 0, DAMAGE_NO_PROTECTION, 0);
		
		if (spawn_skeleton(ent, start, slvl, type))
			found = true;
	}

	if (found)
	{
		ent->client->pers.inventory[power_cube_index] -= SKELETON_COST;
		return true;
	}
	return false;
}

void raise_skeleton(edict_t* ent, int type)
{
	int slvl = ent->myskills.abilities[SKELETON].current_level;
	vec3_t forward, right, start, offset, mins, maxs;

	// get view origin
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	skeleton_set_bbox(mins, maxs);

	if (!G_GetSpawnLocation(ent, 64, mins, maxs, start, NULL, PROJECT_HITBOX_FAR, false))
		return;

	if (spawn_skeleton(ent, start, slvl, type))
		ent->client->pers.inventory[power_cube_index] -= SKELETON_COST;
}
void RemoveDrone(edict_t* ent);
void MonsterCommand(edict_t* ent);
void MonsterFollowMe(edict_t* ent);

void Cmd_Raise_Skeleton_f(edict_t *ent)
{
	int type;
	char* s;

	if (debuginfo->value)
		gi.dprintf("DEBUG: %s just called Cmd_Raise_Skeleton_f()\n", ent->client->pers.netname);

	if (ent->deadflag == DEAD_DEAD)
		return;

	s = gi.args();

	if (!Q_strcasecmp(s, "help"))
	{
		safe_cprintf(ent, PRINT_HIGH, "Skeleton raising:\n");
		safe_cprintf(ent, PRINT_HIGH, "skeleton [ice|poison|fire]\n");
		safe_cprintf(ent, PRINT_HIGH, "Skeleton utility commands:\n");
		safe_cprintf(ent, PRINT_HIGH, "skeleton [remove|command|follow me]\n");
		return;
	}

	if (!Q_strcasecmp(s, "command"))
	{
		MonsterCommand(ent);
		return;
	}

	if (!Q_strcasecmp(s, "follow me"))
	{
		MonsterFollowMe(ent);
		return;
	}

	if (ent->myskills.abilities[SKELETON].disable)
		return;

	if (!G_CanUseAbilities(ent, ent->myskills.abilities[SKELETON].current_level, 0))
		return;

	if (!Q_strcasecmp(s, "remove"))
	{
		RemoveDrone(ent);
		return;
	}

	if (ent->client->pers.inventory[power_cube_index] < SKELETON_COST)
	{
		safe_cprintf(ent, PRINT_HIGH, "You need more power cubes to use this ability.\n");
		return;
	}
	if (ent->num_skeletons >= SKELETON_MAX)
	{
		safe_cprintf(ent, PRINT_HIGH, "Can't raise any more skeletons.\n");
		return;
	}

	if (!Q_strcasecmp(s, "ice"))
		type = SKELETON_TYPE_ICE;
	else if (!Q_strcasecmp(s, "poison"))
		type = SKELETON_TYPE_POISON;
	else if (!Q_strcasecmp(s, "fire"))
		type = SKELETON_TYPE_FIRE;
	else
		type = GetRandom(0, 2);

	if (!raise_skeleton_from_corpse(ent, 512, type))
		raise_skeleton(ent, type);
}