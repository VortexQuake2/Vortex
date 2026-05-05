/*
==============================================================================

hover

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/m_hover.h"

qboolean visible (const edict_t *self, const edict_t *other);


static int	sound_pain1;
static int	sound_pain2;
static int	sound_death1;
static int	sound_death2;
static int	sound_sight;
static int	sound_search1;
static int	sound_search2;


void hover_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void hover_search (edict_t *self)
{
	if (!self->goalentity)
		self->goalentity = world;

	if (random() < 0.5)
		gi.sound (self, CHAN_VOICE, sound_search1, 1, ATTN_NORM, 0);
	else
		gi.sound (self, CHAN_VOICE, sound_search2, 1, ATTN_NORM, 0);
}


void hover_run (edict_t *self);
void hover_stand (edict_t *self);
void hover_dead (edict_t *self);
void hover_deadthink (edict_t *self);
void hover_attack (edict_t *self);
void hover_reattack (edict_t *self);
void hover_fire_blaster (edict_t *self);
void hover_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

static void hover_set_fly_parameters(edict_t *self)
{
	self->monsterinfo.fly_thrusters = false;
	self->monsterinfo.fly_acceleration = 20.0f;
	self->monsterinfo.fly_speed = 120.0f;
	self->monsterinfo.fly_min_distance = 275.0f; //250.0f default value
	self->monsterinfo.fly_max_distance = 550.0f; //450.0f default value
}

mframe_t hover_frames_stand [] =
{
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL
};
mmove_t	hover_move_stand = {FRAME_stand01, FRAME_stand30, hover_frames_stand, NULL};

mframe_t hover_frames_stop1 [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL
};
mmove_t hover_move_stop1 = {FRAME_stop101, FRAME_stop109, hover_frames_stop1, NULL};

mframe_t hover_frames_stop2 [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL
};
mmove_t hover_move_stop2 = {FRAME_stop201, FRAME_stop208, hover_frames_stop2, NULL};

mframe_t hover_frames_takeoff [] =
{
	ai_move,	0,	NULL,
	ai_move,	-2,	NULL,
	ai_move,	5,	NULL,
	ai_move,	-1,	NULL,
	ai_move,	1,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	-1,	NULL,
	ai_move,	-1,	NULL,
	ai_move,	-1,	NULL,
	ai_move,	0,	NULL,
	ai_move,	2,	NULL,
	ai_move,	2,	NULL,
	ai_move,	1,	NULL,
	ai_move,	1,	NULL,
	ai_move,	-6,	NULL,
	ai_move,	-9,	NULL,
	ai_move,	1,	NULL,
	ai_move,	0,	NULL,
	ai_move,	2,	NULL,
	ai_move,	2,	NULL,
	ai_move,	1,	NULL,
	ai_move,	1,	NULL,
	ai_move,	1,	NULL,
	ai_move,	2,	NULL,
	ai_move,	0,	NULL,
	ai_move,	2,	NULL,
	ai_move,	3,	NULL,
	ai_move,	2,	NULL,
	ai_move,	0,	NULL
};
mmove_t hover_move_takeoff = {FRAME_takeof01, FRAME_takeof30, hover_frames_takeoff, NULL};

mframe_t hover_frames_pain3 [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL
};
mmove_t hover_move_pain3 = {FRAME_pain301, FRAME_pain309, hover_frames_pain3, hover_run};

mframe_t hover_frames_pain2 [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL
};
mmove_t hover_move_pain2 = {FRAME_pain201, FRAME_pain212, hover_frames_pain2, hover_run};

mframe_t hover_frames_pain1 [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	2,	NULL,
	ai_move,	-8,	NULL,
	ai_move,	-4,	NULL,
	ai_move,	-6,	NULL,
	ai_move,	-4,	NULL,
	ai_move,	-3,	NULL,
	ai_move,	1,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	3,	NULL,
	ai_move,	1,	NULL,
	ai_move,	0,	NULL,
	ai_move,	2,	NULL,
	ai_move,	3,	NULL,
	ai_move,	2,	NULL,
	ai_move,	7,	NULL,
	ai_move,	1,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	2,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	5,	NULL,
	ai_move,	3,	NULL,
	ai_move,	4,	NULL
};
mmove_t hover_move_pain1 = {FRAME_pain101, FRAME_pain128, hover_frames_pain1, hover_run};

mframe_t hover_frames_land [] =
{
	ai_move,	0,	NULL
};
mmove_t hover_move_land = {FRAME_land01, FRAME_land01, hover_frames_land, NULL};

mframe_t hover_frames_forward [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL
};
mmove_t hover_move_forward = {FRAME_forwrd01, FRAME_forwrd35, hover_frames_forward, NULL};

mframe_t hover_frames_walk [] =
{
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL,
	drone_ai_walk,	5,	NULL
};
mmove_t hover_move_walk = {FRAME_forwrd01, FRAME_forwrd35, hover_frames_walk, NULL};

mframe_t hover_frames_run [] =
{
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL,
	drone_ai_run,	10,	NULL
};
mmove_t hover_move_run = {FRAME_forwrd01, FRAME_forwrd35, hover_frames_run, NULL};

void hover_dying (edict_t *self)
{
	if (self->groundentity)
	{
		hover_deadthink(self);
		return;
	}

	if (random() < 0.5)
		return;

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_PLAIN_EXPLOSION);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PHS);

	if (!vrx_spawn_nonessential_ent(self->s.origin))
		return;

	if (random() < 0.5)
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", 120, GIB_ORGANIC);
	else
		ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", 120, GIB_METALLIC);
}

mframe_t hover_frames_death1 [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	hover_dying,
	ai_move,	0,	NULL,
	ai_move,	0,	hover_dying,
	ai_move,	0,	NULL,
	ai_move,	0,	hover_dying,
	ai_move,	-10,hover_dying,
	ai_move,	3,	NULL,
	ai_move,	5,	hover_dying,
	ai_move,	4,	hover_dying,
	ai_move,	7,	NULL
};
mmove_t hover_move_death1 = {FRAME_death101, FRAME_death111, hover_frames_death1, hover_dead};

mframe_t hover_frames_backward [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL
};
mmove_t hover_move_backward = {FRAME_backwd01, FRAME_backwd24, hover_frames_backward, NULL};

mframe_t hover_frames_start_attack [] =
{
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL
};
mmove_t hover_move_start_attack = {FRAME_attak101, FRAME_attak103, hover_frames_start_attack, hover_attack};

mframe_t hover_frames_attack2[] =
{
	ai_charge,	10,	hover_fire_blaster,
	ai_charge,	10,	hover_fire_blaster,
	ai_charge,	10,	hover_reattack
};
mmove_t hover_move_attack2 = { FRAME_attak104, FRAME_attak106, hover_frames_attack2, hover_run };

mframe_t hover_frames_attack1 [] =
{
	ai_charge,	-10,	hover_fire_blaster,
	ai_charge,	-10,	hover_fire_blaster,
	ai_charge,	0,	hover_reattack
};
mmove_t hover_move_attack1 = {FRAME_attak104, FRAME_attak106, hover_frames_attack1, hover_run };


mframe_t hover_frames_end_attack [] =
{
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL
};
mmove_t hover_move_end_attack = {FRAME_attak107, FRAME_attak108, hover_frames_end_attack, hover_run};

void hover_reattack (edict_t *self)
{
	// if our enemy is still valid, then continue firing
	if (G_ValidTarget(self, self->enemy, true, true) && (random() <= 0.6))
	{
		self->s.frame = FRAME_attak104;
		//hover_fire_blaster(self);
		return;
	}

	/*
	if (M_ContinueAttack(self, &hover_move_attack1, NULL, 0, 512, 0.9))
	{
		//gi.dprintf("continue attack\n");
		return;
	}*/
	//else
		//gi.dprintf("end attack\n");
	// end attack
	self->monsterinfo.attack_finished = level.time + 1.0;
	if (!(self->monsterinfo.aiflags & AI_STAND_GROUND))
		self->monsterinfo.currentmove = &hover_move_end_attack;
}

void hover_fire_blaster (edict_t *self)
{
	int		damage,speed=M_ROCKETLAUNCHER_SPEED_MAX;
	vec3_t	forward, start;

	//gi.dprintf("fired at %d\n", (int)(level.framenum));
	// hover fires a rapid fire, weakened rocket launcher
	damage = M_HYPERBLASTER_DMG_BASE + M_HYPERBLASTER_DMG_ADDON * drone_damagelevel(self);
	if (M_HYPERBLASTER_DMG_MAX && damage > M_HYPERBLASTER_DMG_MAX)
		damage = M_HYPERBLASTER_DMG_MAX;

	MonsterAim(self, M_PROJECTILE_ACC, speed, true, MZ2_BOSS2_ROCKET_3, forward, start);
	if (!M_MonsterHasClearShotFrom(self, start))
	{
		M_MonsterBlockedShot(self, 0.4f);
		return;
	}
	monster_fire_rocket (self, start, forward, damage, speed, MZ2_BOSS2_ROCKET_3);
}

void hover_stand (edict_t *self)
{
		self->monsterinfo.currentmove = &hover_move_stand;
}

void hover_run (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &hover_move_stand;
	else
		self->monsterinfo.currentmove = &hover_move_run;
}

void hover_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &hover_move_walk;
}

void hover_start_attack (edict_t *self)
{
	self->monsterinfo.currentmove = &hover_move_start_attack;
}

void hover_attack(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		self->monsterinfo.attack_state = AS_STRAIGHT;
		self->monsterinfo.currentmove = &hover_move_attack2;
	}
	else if (random() < 0.5f)
	{
		self->monsterinfo.attack_state = AS_STRAIGHT;
		self->monsterinfo.currentmove = &hover_move_attack1;
	}
	else
	{
		if (random() <= 0.5f)
			self->monsterinfo.lefty = 1 - self->monsterinfo.lefty;
		self->monsterinfo.attack_state = AS_SLIDING;
		self->monsterinfo.currentmove = &hover_move_attack2;
	}
}


void hover_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	// we're already in a pain state
	if (self->monsterinfo.currentmove == &hover_move_pain1 ||
		self->monsterinfo.currentmove == &hover_move_pain2 ||
		self->monsterinfo.currentmove == &hover_move_pain3)
		return;

	// monster players don't get pain state induced
	if (G_GetClient(self))
		return;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0f;

	// stand animation always gets pain state
	if (random() <= (1.0f - self->monsterinfo.pain_chance) &&
		self->monsterinfo.currentmove == &hover_move_stand)
		return;

	if (damage <= 25)
	{
		if (random() < 0.5)
		{
			gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
			self->monsterinfo.currentmove = &hover_move_pain3;
		}
		else
		{
			gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
			self->monsterinfo.currentmove = &hover_move_pain2;
		}
	}
	else
	{
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
		if (random() < 0.3f)
			self->monsterinfo.currentmove = &hover_move_pain1;
		else
			self->monsterinfo.currentmove = &hover_move_pain2;
	}
}

void hover_deadthink (edict_t *self)
{
	vec3_t	end;
	trace_t	tr;
	qboolean on_floor;

	on_floor = self->groundentity != NULL;
	if (!on_floor)
	{
		VectorCopy(self->s.origin, end);
		end[2] -= 24;
		tr = gi.trace(self->s.origin, self->mins, self->maxs, end, self, MASK_SOLID);
		on_floor = tr.fraction < 1.0f && tr.plane.normal[2] > 0.7f;
	}

	if (!on_floor && level.time < self->timestamp)
	{
		self->nextthink = level.time + FRAMETIME;
		return;
	}
	vrx_throw_drone_gibs(self, 150);
	BecomeExplosion1(self);
}

void hover_dead (edict_t *self)
{
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->think = hover_deadthink;
	self->nextthink = level.time + FRAMETIME;
	self->timestamp = level.time + 15;
	gi.linkentity (self);
}

void hover_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	qboolean overkill;

	M_Notify(self);
	overkill = self->health <= self->gib_health;

	if (self->deadflag == DEAD_DEAD)
		return;

	DroneList_Remove(self);

// regular death
	if (overkill)
		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
	else if (random() < 0.5)
		gi.sound (self, CHAN_VOICE, sound_death1, 1, ATTN_NORM, 0);
	else
		gi.sound (self, CHAN_VOICE, sound_death2, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	vrx_update_drone_death_skin(self);
	self->flags &= ~FL_FLY;
	self->movetype = MOVETYPE_TOSS;
	self->gravity = 1.0;
	if (self->velocity[2] > -120)
		self->velocity[2] = -120;
	self->monsterinfo.currentmove = &hover_move_death1;
}

/*QUAKED monster_hover (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void init_drone_hover (edict_t *self)
{
	sound_pain1 = gi.soundindex ("hover/hovpain1.wav");	
	sound_pain2 = gi.soundindex ("hover/hovpain2.wav");	
	sound_death1 = gi.soundindex ("hover/hovdeth1.wav");	
	sound_death2 = gi.soundindex ("hover/hovdeth2.wav");	
	sound_sight = gi.soundindex ("hover/hovsght1.wav");	
	sound_search1 = gi.soundindex ("hover/hovsrch1.wav");	
	sound_search2 = gi.soundindex ("hover/hovsrch2.wav");	

	gi.soundindex ("hover/hovatck1.wav");	

	self->s.sound = gi.soundindex ("hover/hovidle1.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/hover/tris.md2");
	VectorSet (self->mins, -24, -24, -24);
	VectorSet (self->maxs, 24, 24, 32);

	self->health = M_FLOATER_INITIAL_HEALTH + M_FLOATER_ADDON_HEALTH*self->monsterinfo.level;
	self->gib_health = -100;
	self->mass = 150;

	self->mtype = M_HOVER;
	self->flags |= FL_FLY;
	self->monsterinfo.aiflags |= AI_ALTERNATE_FLY;
	hover_set_fly_parameters(self);
	self->max_health = self->health;
	self->monsterinfo.power_armor_power = M_FLOATER_INITIAL_ARMOR + M_FLOATER_ADDON_ARMOR*self->monsterinfo.level;
	self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
	self->monsterinfo.max_armor = self->monsterinfo.power_armor_power;
	self->monsterinfo.control_cost = M_HOVER_CONTROL_COST;
	self->monsterinfo.cost = M_HOVER_COST;
	self->item = FindItemByClassname("ammo_rockets");

	self->pain = hover_pain;
	self->die = hover_die;

	self->monsterinfo.stand = hover_stand;
	self->monsterinfo.walk = hover_walk;
	self->monsterinfo.run = hover_run;
//	self->monsterinfo.dodge = hover_dodge;
	self->monsterinfo.attack = hover_attack;
	self->monsterinfo.sight = hover_sight;
	self->monsterinfo.idle = hover_search;
	self->monsterinfo.pain_chance = 0.2f; 
	//self->monsterinfo.search = hover_search;

	gi.linkentity (self);

	self->monsterinfo.currentmove = &hover_move_stand;	
	self->monsterinfo.scale = MODEL_SCALE;

	//flymonster_start (self);
}
