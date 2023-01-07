/*
==============================================================================

floater

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/m_float.h"


static int	sound_attack2;
static int	sound_attack3;
static int	sound_death1;
static int	sound_idle;
static int	sound_pain1;
static int	sound_pain2;
static int	sound_sight;

void drone_ai_stand (edict_t *self, float dist);
void drone_ai_run (edict_t *self, float dist);
void drone_ai_walk (edict_t *self, float dist);

void floater_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void floater_idle (edict_t *self)
{
	if (!self->goalentity)
		self->goalentity = world;
	gi.sound (self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

//void floater_stand1 (edict_t *self);
void floater_dead (edict_t *self);
void floater_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);
void floater_run (edict_t *self);
void floater_wham (edict_t *self);
void floater_zap (edict_t *self);
void floater_continue_attack(edict_t* self);

void floater_fire_blaster (edict_t *self)
{
	int		damage, speed=2000, effect;
	vec3_t	forward, start;

	if ((self->s.frame == FRAME_attak104) || (self->s.frame == FRAME_attak107))
		effect = EF_HYPERBLASTER;
	else
		effect = 0;

	damage = M_HYPERBLASTER_DMG_BASE + M_HYPERBLASTER_DMG_ADDON * self->monsterinfo.level;
	if (M_HYPERBLASTER_DMG_MAX && damage > M_HYPERBLASTER_DMG_MAX)
		damage = M_HYPERBLASTER_DMG_MAX;

	MonsterAim(self, M_PROJECTILE_ACC, speed, false, MZ2_FLOAT_BLASTER_1, forward, start);
	monster_fire_blaster(self, start, forward, damage, speed, EF_BLASTER, BLASTER_PROJ_BOLT, 2.0, true, MZ2_FLOAT_BLASTER_1);
}

mframe_t floater_frames_stand1 [] =
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
mmove_t	floater_move_stand1 = {FRAME_stand101, FRAME_stand152, floater_frames_stand1, NULL};

mframe_t floater_frames_stand2 [] =
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
mmove_t	floater_move_stand2 = {FRAME_stand201, FRAME_stand252, floater_frames_stand2, NULL};

void floater_stand (edict_t *self)
{
	if (random() <= 0.5)		
		self->monsterinfo.currentmove = &floater_move_stand1;
	else
		self->monsterinfo.currentmove = &floater_move_stand2;
}

mframe_t floater_frames_activate [] =
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
	ai_move,	0,	NULL
};
mmove_t floater_move_activate = {FRAME_actvat01, FRAME_actvat31, floater_frames_activate, NULL};

mframe_t floater_frames_attack1 [] =
{
	drone_ai_run,	15,	NULL,			// Blaster attack
	drone_ai_run,	15,	NULL,
	drone_ai_run,	15,	NULL,
	drone_ai_run,	15,	floater_fire_blaster,			// BOOM (0, -25.8, 32.5)	-- LOOP Starts
	drone_ai_run,	15,	floater_fire_blaster,
	drone_ai_run,	15,	floater_fire_blaster,
	drone_ai_run,	15,	floater_fire_blaster,
	drone_ai_run,	15,	floater_fire_blaster,
	drone_ai_run,	15,	floater_fire_blaster,
	drone_ai_run,	15,	floater_fire_blaster,
	drone_ai_run,	15,	floater_continue_attack,
	drone_ai_run,	15,	NULL,
	drone_ai_run,	15,	NULL,
	drone_ai_run,	15,	NULL			//							-- LOOP Ends
};
mmove_t floater_move_attack1 = {FRAME_attak101, FRAME_attak114, floater_frames_attack1, floater_run};



mframe_t floater_frames_attack4[] =
{
	ai_charge,	0,	NULL,			// Blaster attack
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	floater_fire_blaster,			// BOOM (0, -25.8, 32.5)	-- LOOP Starts
	ai_charge,	0,	floater_fire_blaster,
	ai_charge,	0,	floater_fire_blaster,
	ai_charge,	0,	floater_fire_blaster,
	ai_charge,	0,	floater_fire_blaster,
	ai_charge,	0,	floater_fire_blaster,
	ai_charge,	0,	floater_fire_blaster,
	ai_charge,	0,	floater_continue_attack,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL			//							-- LOOP Ends
};
mmove_t floater_move_attack4 = { FRAME_attak101, FRAME_attak114, floater_frames_attack4, floater_run };

void floater_continue_attack(edict_t* self)
{
	mmove_t* move;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		move = &floater_move_attack4;
	else
		move = &floater_move_attack1;

	// if our enemy is still valid, then continue firing
	if (M_ContinueAttack(self, move, NULL, 0, 1024, 0.8))
	{
		self->s.frame = FRAME_attak104;
		return;
	}

	M_DelayNextAttack(self, 0, true);
}

mframe_t floater_frames_attack2 [] =
{
	ai_charge,	0,	NULL,			// Claws
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	floater_wham,			// WHAM (0, -45, 29.6)		-- LOOP Starts
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,			//							-- LOOP Ends
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL
};
mmove_t floater_move_attack2 = {FRAME_attak201, FRAME_attak225, floater_frames_attack2, floater_run};

mframe_t floater_frames_attack3 [] =
{
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	floater_zap,		//								-- LOOP Starts
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,		//								-- LOOP Ends
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL
};
mmove_t floater_move_attack3 = {FRAME_attak301, FRAME_attak334, floater_frames_attack3, floater_run};

mframe_t floater_frames_death [] =
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
	ai_move,	0,	NULL
};
mmove_t floater_move_death = {FRAME_death01, FRAME_death13, floater_frames_death, floater_dead};

mframe_t floater_frames_pain1 [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL
};
mmove_t floater_move_pain1 = {FRAME_pain101, FRAME_pain107, floater_frames_pain1, floater_run};

mframe_t floater_frames_pain2 [] =
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
mmove_t floater_move_pain2 = {FRAME_pain201, FRAME_pain208, floater_frames_pain2, floater_run};

mframe_t floater_frames_pain3 [] =
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
mmove_t floater_move_pain3 = {FRAME_pain301, FRAME_pain312, floater_frames_pain3, floater_run};

mframe_t floater_frames_walk [] =
{
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL
};
mmove_t	floater_move_walk = {FRAME_stand101, FRAME_stand152, floater_frames_walk, NULL};

mframe_t floater_frames_run [] =
{
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL
};
mmove_t	floater_move_run = {FRAME_stand101, FRAME_stand152, floater_frames_run, NULL};

void floater_run (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &floater_move_stand1;
	else
		self->monsterinfo.currentmove = &floater_move_run;
}

void floater_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &floater_move_walk;
}

void floater_wham (edict_t *self)
{
	static	vec3_t	aim = {MELEE_DISTANCE, 0, 0};
	gi.sound (self, CHAN_WEAPON, sound_attack3, 1, ATTN_NORM, 0);
	fire_hit (self, aim, 5 + rand() % 6, -50);
}

void floater_zap (edict_t *self)
{
	vec3_t	forward, right;
	vec3_t	origin;
	vec3_t	dir;
	vec3_t	offset;

	VectorSubtract (self->enemy->s.origin, self->s.origin, dir);

	AngleVectors (self->s.angles, forward, right, NULL);
	//FIXME use a flash and replace these two lines with the commented one
	VectorSet (offset, 18.5, -0.9, 10);
	G_ProjectSource (self->s.origin, offset, forward, right, origin);
//	G_ProjectSource (self->s.origin, monster_flash_offset[flash_number], forward, right, origin);

	gi.sound (self, CHAN_WEAPON, sound_attack2, 1, ATTN_NORM, 0);

	//FIXME use the flash, Luke
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_SPLASH);
	gi.WriteByte (32);
	gi.WritePosition (origin);
	gi.WriteDir (dir);
	gi.WriteByte (1);	//sparks
	gi.multicast (origin, MULTICAST_PVS);

	T_Damage (self->enemy, self, self, dir, self->enemy->s.origin, vec3_origin, 5 + rand() % 6, -10, DAMAGE_ENERGY, MOD_UNKNOWN);
}

void floater_attack(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &floater_move_attack4;
	else
		self->monsterinfo.currentmove = &floater_move_attack1;
}


void floater_melee(edict_t *self)
{
	/*
	if (random() < 0.5)		
		self->monsterinfo.currentmove = &floater_move_attack3;
	else
		self->monsterinfo.currentmove = &floater_move_attack2;
		*/
}


void floater_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	int		n;

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;
	if (skill->value == 3)
		return;		// no pain anims in nightmare

	n = (rand() + 1) % 3;
	if (n == 0)
	{
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &floater_move_pain1;
	}
	else
	{
		gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &floater_move_pain2;
	}
}

void floater_dead (edict_t *self)
{
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity (self);
}

void floater_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	gi.sound (self, CHAN_VOICE, sound_death1, 1, ATTN_NORM, 0);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION1);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	M_Notify(self);
	M_Remove(self, false, false);
}

/*QUAKED monster_floater (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void init_drone_floater (edict_t *self)
{
	sound_attack2 = gi.soundindex ("floater/fltatck2.wav");
	sound_attack3 = gi.soundindex ("floater/fltatck3.wav");
	sound_death1 = gi.soundindex ("floater/fltdeth1.wav");
	sound_idle = gi.soundindex ("floater/fltidle1.wav");
	sound_pain1 = gi.soundindex ("floater/fltpain1.wav");
	sound_pain2 = gi.soundindex ("floater/fltpain2.wav");
	sound_sight = gi.soundindex ("floater/fltsght1.wav");

	gi.soundindex ("floater/fltatck1.wav");

	self->s.sound = gi.soundindex ("floater/fltsrch1.wav");

	self->movetype = MOVETYPE_STEP;
	self->flags |= FL_FLY;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex ("models/monsters/float/tris.md2");
	VectorSet (self->mins, -24, -24, -24);
	VectorSet (self->maxs, 24, 24, 40);

	self->health = M_FLOATER_INITIAL_HEALTH + M_FLOATER_ADDON_HEALTH*self->monsterinfo.level;
	self->max_health = self->health;
	self->gib_health = -0.6 * BASE_GIB_HEALTH;
	self->mass = 150;
	self->mtype = M_FLOATER;

	self->monsterinfo.power_armor_power = M_FLOATER_INITIAL_ARMOR + M_FLOATER_ADDON_ARMOR*self->monsterinfo.level;

	self->monsterinfo.power_armor_type = POWER_ARMOR_SCREEN;
	self->monsterinfo.max_armor = self->monsterinfo.power_armor_power;

	self->monsterinfo.control_cost = M_FLOATER_CONTROL_COST;
	self->monsterinfo.cost = M_FLOATER_COST;

	self->item = FindItemByClassname("ammo_cells");

	self->pain = floater_pain;
	self->die = floater_die;

	self->monsterinfo.stand = floater_stand;
	self->monsterinfo.walk = floater_walk;
	self->monsterinfo.run = floater_run;
//	self->monsterinfo.dodge = floater_dodge;
	self->monsterinfo.attack = floater_attack;
	self->monsterinfo.melee = floater_melee;
	self->monsterinfo.sight = floater_sight;
	self->monsterinfo.idle = floater_idle;

	gi.linkentity (self);

	if (random() <= 0.5)		
		self->monsterinfo.currentmove = &floater_move_stand1;	
	else
		self->monsterinfo.currentmove = &floater_move_stand2;	
	
	self->monsterinfo.scale = MODEL_SCALE;

	//flymonster_start (self);
}
