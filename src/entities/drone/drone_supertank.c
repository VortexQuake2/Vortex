/*
==============================================================================

SUPERTANK

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/m_supertank.h"
qboolean visible (const edict_t *self, const edict_t *other);

#define SUPERTANK_INVASION_SCALE			0.65f
#define SUPERTANK_INVASION_BASE_HEALTH		5000
#define SUPERTANK_INVASION_ADDON_HEALTH		1000

static int	sound_death;
static int	sound_search1;
static int	sound_search2;

static	int	tread_sound;

void BossExplode (edict_t *self);

static qboolean supertank_is_boss5(const edict_t *self)
{
	return self->mtype == M_BOSS5;
}

static void supertank_project_flash(edict_t *self, int flash_number, vec3_t forward, vec3_t start)
{
	vec3_t right, offset;

	AngleVectors(self->s.angles, forward, right, NULL);
	VectorCopy(monster_flash_offset[flash_number], offset);
	if (self->s.scale && self->s.scale != 1.0f)
		VectorScale(offset, self->s.scale, offset);
	G_ProjectSource(self->s.origin, offset, forward, right, start);
}

void TreadSound (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, tread_sound, 1, ATTN_NORM, 0);
}

void supertankRocket (edict_t *self);
void supertankMachineGun (edict_t *self);
void supertankGrenade (edict_t *self);
void supertank_reattack1(edict_t *self);

//
// stand
//

mframe_t supertank_frames_stand []=
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
mmove_t	supertank_move_stand = {FRAME_stand_1, FRAME_stand_60, supertank_frames_stand, NULL};
	
void supertank_stand (edict_t *self)
{
	self->monsterinfo.currentmove = &supertank_move_stand;
}


mframe_t supertank_frames_run [] =
{
	drone_ai_run, 12,	TreadSound,
	drone_ai_run, 12,	NULL,
	drone_ai_run, 12,	NULL,
	drone_ai_run, 12,	NULL,
	drone_ai_run, 12,	NULL,
	drone_ai_run, 12,	NULL,
	drone_ai_run, 12,	NULL,
	drone_ai_run, 12,	NULL,
	drone_ai_run, 12,	NULL,
	drone_ai_run, 12,	NULL,
	drone_ai_run, 12,	NULL,
	drone_ai_run, 12,	NULL,
	drone_ai_run, 12,	NULL,
	drone_ai_run, 12,	NULL,
	drone_ai_run, 12,	NULL,
	drone_ai_run, 12,	NULL,
	drone_ai_run, 12,	NULL,
	drone_ai_run, 12,	NULL
};
mmove_t	supertank_move_run = {FRAME_forwrd_1, FRAME_forwrd_18, supertank_frames_run, NULL};

mframe_t supertank_frames_run_janitor [] =
{
	drone_ai_run, 18,	TreadSound,
	drone_ai_run, 18,	NULL,
	drone_ai_run, 18,	NULL,
	drone_ai_run, 18,	NULL,
	drone_ai_run, 18,	NULL,
	drone_ai_run, 18,	NULL,
	drone_ai_run, 18,	NULL,
	drone_ai_run, 18,	NULL,
	drone_ai_run, 18,	NULL,
	drone_ai_run, 18,	NULL,
	drone_ai_run, 18,	NULL,
	drone_ai_run, 18,	NULL,
	drone_ai_run, 18,	NULL,
	drone_ai_run, 18,	NULL,
	drone_ai_run, 18,	NULL,
	drone_ai_run, 18,	NULL,
	drone_ai_run, 18,	NULL,
	drone_ai_run, 18,	NULL
};
mmove_t	supertank_move_run_janitor = {FRAME_forwrd_1, FRAME_forwrd_18, supertank_frames_run_janitor, NULL};

//
// walk
//


mframe_t supertank_frames_forward [] =
{
	drone_ai_walk, 4,	TreadSound,
	drone_ai_walk, 4,	NULL,
	drone_ai_walk, 4,	NULL,
	drone_ai_walk, 4,	NULL,
	drone_ai_walk, 4,	NULL,
	drone_ai_walk, 4,	NULL,
	drone_ai_walk, 4,	NULL,
	drone_ai_walk, 4,	NULL,
	drone_ai_walk, 4,	NULL,
	drone_ai_walk, 4,	NULL,
	drone_ai_walk, 4,	NULL,
	drone_ai_walk, 4,	NULL,
	drone_ai_walk, 4,	NULL,
	drone_ai_walk, 4,	NULL,
	drone_ai_walk, 4,	NULL,
	drone_ai_walk, 4,	NULL,
	drone_ai_walk, 4,	NULL,
	drone_ai_walk, 4,	NULL
};
mmove_t	supertank_move_forward = {FRAME_forwrd_1, FRAME_forwrd_18, supertank_frames_forward, NULL};

void supertank_forward (edict_t *self)
{
		self->monsterinfo.currentmove = &supertank_move_forward;
}

void supertank_walk (edict_t *self)
{
	if (!self->goalentity)
		self->goalentity = world;

	self->monsterinfo.currentmove = &supertank_move_forward;
}

void supertank_run (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &supertank_move_stand;
	else if (self->mtype == M_JANITOR)
		self->monsterinfo.currentmove = &supertank_move_run_janitor;
	else
		self->monsterinfo.currentmove = &supertank_move_run;
}

mframe_t supertank_frames_turn_right [] =
{
	ai_move,	0,	TreadSound,
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
mmove_t supertank_move_turn_right = {FRAME_right_1, FRAME_right_18, supertank_frames_turn_right, supertank_run};

mframe_t supertank_frames_turn_left [] =
{
	ai_move,	0,	TreadSound,
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
mmove_t supertank_move_turn_left = {FRAME_left_1, FRAME_left_18, supertank_frames_turn_left, supertank_run};

mframe_t supertank_frames_death1 [] =
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
	ai_move,	0,	BossExplode
};
mmove_t supertank_move_death = {FRAME_death_1, FRAME_death_24, supertank_frames_death1, NULL};

mframe_t supertank_frames_backward[] =
{
	drone_ai_walk, 0,	TreadSound,
	drone_ai_walk, 0,	NULL,
	drone_ai_walk, 0,	NULL,
	drone_ai_walk, 0,	NULL,
	drone_ai_walk, 0,	NULL,
	drone_ai_walk, 0,	NULL,
	drone_ai_walk, 0,	NULL,
	drone_ai_walk, 0,	NULL,
	drone_ai_walk, 0,	NULL,
	drone_ai_walk, 0,	NULL,
	drone_ai_walk, 0,	NULL,
	drone_ai_walk, 0,	NULL,
	drone_ai_walk, 0,	NULL,
	drone_ai_walk, 0,	NULL,
	drone_ai_walk, 0,	NULL,
	drone_ai_walk, 0,	NULL,
	drone_ai_walk, 0,	NULL,
	drone_ai_walk, 0,	NULL
};
mmove_t	supertank_move_backward = {FRAME_backwd_1, FRAME_backwd_18, supertank_frames_backward, NULL};

mframe_t supertank_frames_attack3[]=
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
	ai_move,	0,	NULL
};
mmove_t supertank_move_attack3 = {FRAME_attak3_1, FRAME_attak3_27, supertank_frames_attack3, supertank_run};

void supertank_rerocket(edict_t *self)
{
	if (G_ValidTarget(self, self->enemy, true, true) && entdist(self, self->enemy) <= 512 && random() <= 0.8)
		self->s.frame = 25;

	self->monsterinfo.attack_finished = level.time + 2.0;
}

mframe_t supertank_frames_attack2[]=
{
	//ai_charge,	0,	NULL,//20
	//ai_charge,	0,	NULL,
	//ai_charge,	0,	NULL,
	//ai_charge,	0,	NULL,
	//ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	supertankRocket,//27
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	supertankRocket,//30
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	supertankRocket,//33
	ai_charge,	0,	supertank_rerocket,//GHz
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,//40
	//ai_move,	0,	NULL,
	//ai_move,	0,	NULL,
	//ai_move,	0,	NULL,
	//ai_move,	0,	NULL,
	//ai_move,	0,	NULL,
	//ai_move,	0,	NULL
};
mmove_t supertank_move_attack2 = {FRAME_attak2_6, FRAME_attak2_21, supertank_frames_attack2, supertank_run};

mframe_t supertank_frames_attack1[]=
{
	ai_charge,	0,	supertankMachineGun,
	ai_charge,	0,	supertankMachineGun,
	ai_charge,	0,	supertankMachineGun,
	ai_charge,	0,	supertankMachineGun,
	ai_charge,	0,	supertankMachineGun,
	ai_charge,	0,	supertankMachineGun,

};
mmove_t supertank_move_attack1 = {FRAME_attak1_1, FRAME_attak1_6, supertank_frames_attack1, supertank_reattack1};

mframe_t supertank_frames_end_attack1[]=
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
	ai_move,	0,	NULL
};
mmove_t supertank_move_end_attack1 = {FRAME_attak1_7, FRAME_attak1_20, supertank_frames_end_attack1, supertank_run};
void supertankGrenade (edict_t *self)
{
	vec3_t	forward, start;
	int		damage, speed, flash_number;

	if (!G_EntExists(self->enemy))
		return;

	if (self->s.frame == FRAME_attak4_1)
		flash_number = MZ2_SUPERTANK_GRENADE_1;
	else
		flash_number = MZ2_SUPERTANK_GRENADE_2;

	damage = M_GRENADELAUNCHER_DMG_BASE + M_GRENADELAUNCHER_DMG_ADDON * drone_damagelevel(self);
	if (M_GRENADELAUNCHER_DMG_MAX && damage > M_GRENADELAUNCHER_DMG_MAX)
		damage = M_GRENADELAUNCHER_DMG_MAX;

	speed = M_GRENADELAUNCHER_SPEED_BASE + M_GRENADELAUNCHER_SPEED_ADDON * drone_damagelevel(self);
	if (M_GRENADELAUNCHER_SPEED_MAX && speed > M_GRENADELAUNCHER_SPEED_MAX)
		speed = M_GRENADELAUNCHER_SPEED_MAX;

	if (self->s.scale && self->s.scale != 1.0f)
	{
		supertank_project_flash(self, flash_number, forward, start);
		MonsterAim(self, M_PROJECTILE_ACC, speed, true, -1, forward, start);
	}
	else
		MonsterAim(self, M_PROJECTILE_ACC, speed, true, flash_number, forward, start);

	monster_fire_grenade(self, start, forward, damage, speed, flash_number);
}

mframe_t supertank_frames_attack4[]=
{
	ai_charge,	0,	supertankGrenade,//74
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	supertankGrenade,//77
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,

};
mmove_t supertank_move_attack4 = {FRAME_attak4_1, FRAME_attak4_6, supertank_frames_attack4, supertank_run};
void supertank_reattack1(edict_t *self)
{
	if (G_ValidTarget(self, self->enemy, true, true) && random() <= 0.9)
		self->monsterinfo.currentmove = &supertank_move_attack1;
	else
		self->monsterinfo.currentmove = &supertank_move_end_attack1;
	self->monsterinfo.attack_finished = level.time + 1.0;
}

void supertankRocket (edict_t *self)
{
	vec3_t	forward, right, start, offset;
	int		damage, speed, flash_number;

	if (self->s.frame == FRAME_attak2_8)
		flash_number = MZ2_SUPERTANK_ROCKET_1;
	else if (self->s.frame == FRAME_attak2_11)
		flash_number = MZ2_SUPERTANK_ROCKET_2;
	else // (self->s.frame == FRAME_attak2_14)
		flash_number = MZ2_SUPERTANK_ROCKET_3;

	damage = 50 + 10 * drone_damagelevel(self);
	speed = 650 + 30 * drone_damagelevel(self);

	if (supertank_is_boss5(self))
	{
		if (self->s.scale && self->s.scale != 1.0f)
		{
			supertank_project_flash(self, flash_number, forward, start);
			MonsterAim(self, 0.5, speed, true, -1, forward, start);
		}
		else
			MonsterAim(self, 0.5, speed, true, flash_number, forward, start);
		monster_fire_heat(self, start, forward, damage, speed, flash_number, 0.075f);
		return;
	}

	if (self->mtype == M_JANITOR)
	{
		AngleVectors(self->s.angles, forward, right, NULL);
		if (flash_number == MZ2_SUPERTANK_ROCKET_1)
			VectorSet(offset, 16.0, -22.5, 108.7);
		else if (flash_number == MZ2_SUPERTANK_ROCKET_2)
			VectorSet(offset, 16.0, -33.4, 106.7);
		else
			VectorSet(offset, 16.0, -42.8, 104.7);
		if (self->s.scale)
			VectorScale(offset, self->s.scale, offset);
		G_ProjectSource(self->s.origin, offset, forward, right, start);
		MonsterAim(self, 0.5, speed, true, -1, forward, start);
	}
	else
	{
		if (self->s.scale && self->s.scale != 1.0f)
		{
			supertank_project_flash(self, flash_number, forward, start);
			MonsterAim(self, 0.5, speed, true, -1, forward, start);
		}
		else
			MonsterAim(self, 0.5, speed, true, flash_number, forward, start);
	}

	monster_fire_rocket (self, start, forward, damage, speed, flash_number);
}	

void supertankMachineGun (edict_t *self)
{
	vec3_t	forward, start;
	int		damage, flash_number;

	flash_number = MZ2_SUPERTANK_MACHINEGUN_1 + (self->s.frame - FRAME_attak1_1);

	damage = 20 + 2* drone_damagelevel(self);

	if (self->s.scale && self->s.scale != 1.0f)
	{
		supertank_project_flash(self, flash_number, forward, start);
		MonsterAim(self, 0.8, 0, false, -1, forward, start);
	}
	else
		MonsterAim(self, 0.8, 0, false, flash_number, forward, start);

	monster_fire_bullet (self, start, forward, damage, damage, 
		DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flash_number);
}	


void supertank_attack(edict_t *self)
{
	const float	range = entdist(self, self->enemy);
	const float	r = random();

	// medium range
	if (range <= 512)
	{
		if (r <= 0.2)
			self->monsterinfo.currentmove = &supertank_move_attack1;
		else if (r <= 0.4)
			self->monsterinfo.currentmove = &supertank_move_attack4;
		else
			self->monsterinfo.currentmove = &supertank_move_attack2;
	}
	// long range
	else
	{
		if (r <= 0.2)
			self->monsterinfo.currentmove = &supertank_move_attack2;
		else if (r <= 0.4)
			self->monsterinfo.currentmove = &supertank_move_attack4;
		else
			self->monsterinfo.currentmove = &supertank_move_attack1;
	}

	self->monsterinfo.attack_finished = level.time + 1.0;
}


//
// death
//

void BossExplode (edict_t *self)
{
	vec3_t	org;
	int		n;

	self->think = BossExplode;
	VectorCopy (self->s.origin, org);
	org[2] += 24 + (randomMT()&15);
	switch (self->count++)
	{
	case 0:
		org[0] -= 24;
		org[1] -= 24;
		break;
	case 1:
		org[0] += 24;
		org[1] += 24;
		break;
	case 2:
		org[0] += 24;
		org[1] -= 24;
		break;
	case 3:
		org[0] -= 24;
		org[1] += 24;
		break;
	case 4:
		org[0] -= 48;
		org[1] -= 48;
		break;
	case 5:
		org[0] += 48;
		org[1] += 48;
		break;
	case 6:
		org[0] -= 48;
		org[1] += 48;
		break;
	case 7:
		org[0] += 48;
		org[1] -= 48;
		break;
	case 8:
		self->s.sound = 0;
		for (n= 0; n < 4; n++)
			ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", 500, GIB_ORGANIC);
		for (n= 0; n < 8; n++)
			ThrowGib (self, "models/objects/gibs/sm_metal/tris.md2", 500, GIB_METALLIC);
		ThrowGib (self, "models/objects/gibs/chest/tris.md2", 500, GIB_ORGANIC);
		ThrowHead (self, "models/objects/gibs/gear/tris.md2", 500, GIB_METALLIC);
		self->deadflag = DEAD_DEAD;
		M_Remove(self, false, false);
		return;
	}

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION1);
	gi.WritePosition (org);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	self->nextthink = level.time + 0.1;
}


void supertank_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (self->deadflag == DEAD_DEAD)
		return;

	gi.sound (self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_NO;
	self->count = 0;
	self->monsterinfo.currentmove = &supertank_move_death;
}

//
// monster_supertank
//

/*QUAKED monster_supertank (1 .5 0) (-64 -64 0) (64 64 72) Ambush Trigger_Spawn Sight
*/

void supertank_sight (edict_t *self, edict_t *other)
{
	if (random() > 0.5)
		gi.sound (self, CHAN_VOICE, sound_search1, 1, ATTN_NORM, 0);
	else
		gi.sound (self, CHAN_VOICE, sound_search2, 1, ATTN_NORM, 0);
}

void init_drone_supertank (edict_t *self)
{
	qboolean janitor = (self->mtype == M_JANITOR);
	qboolean boss5 = supertank_is_boss5(self);

	sound_death = gi.soundindex ("bosstank/btkdeth1.wav");
	sound_search1 = gi.soundindex ("bosstank/btkunqv1.wav");
	sound_search2 = gi.soundindex ("bosstank/btkunqv2.wav");
	tread_sound = gi.soundindex ("bosstank/btkengn1.wav");
	if (boss5)
		gi.soundindex("weapons/railgr1a.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	if (!janitor && !boss5)
		self->mtype = M_SUPERTANK;
	self->monsterinfo.control_cost = janitor ? M_TANK_CONTROL_COST : M_SUPERTANK_CONTROL_COST;
	self->monsterinfo.cost = janitor ? 150 : 300;
	self->s.modelindex = gi.modelindex ("models/monsters/boss1/tris.md2");
	if (janitor)
	{
		self->s.skinnum = 2;
		self->s.scale = 0.6f;
		self->monsterinfo.scale = MODEL_SCALE * 0.6f;
		VectorSet (self->mins, -38, -38, 0);
		VectorSet (self->maxs, 38, 38, 67);
		self->health = self->max_health = M_JANITOR_INITIAL_HEALTH + M_JANITOR_ADDON_HEALTH * self->monsterinfo.level;
	}
	else
	{
		if (boss5)
			self->s.skinnum = 2;
		if (invasion->value)
		{
			self->s.scale = SUPERTANK_INVASION_SCALE;
			self->monsterinfo.scale = MODEL_SCALE * SUPERTANK_INVASION_SCALE;
			VectorSet (self->mins, -40, -40, 0);
			VectorSet (self->maxs, 40, 40, 72);
			self->health = self->max_health = SUPERTANK_INVASION_BASE_HEALTH + SUPERTANK_INVASION_ADDON_HEALTH * self->monsterinfo.level;
		}
		else
		{
			VectorSet (self->mins, -64, -64, 0);
			VectorSet (self->maxs, 64, 64, 112);
			self->health = self->max_health = 20000*self->monsterinfo.level;
			self->monsterinfo.scale = MODEL_SCALE;
		}
	}
	self->gib_health = -5 * BASE_GIB_HEALTH;
	self->mass = janitor ? 480 : 800;

	self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
	if (janitor)
		self->monsterinfo.power_armor_power = M_JANITOR_INITIAL_ARMOR + M_JANITOR_ADDON_ARMOR * self->monsterinfo.level;
	else if (boss5)
		self->monsterinfo.power_armor_power = 400 * self->monsterinfo.level;
	else
		self->monsterinfo.power_armor_power = 0;
	self->monsterinfo.max_armor = self->monsterinfo.power_armor_power;

	self->die = supertank_die;
	self->monsterinfo.stand = supertank_stand;
	self->monsterinfo.walk = supertank_walk;
	self->monsterinfo.run = supertank_run;
	self->monsterinfo.attack = supertank_attack;
	self->monsterinfo.jumpup = 64;
	self->monsterinfo.jumpdn = 512;
	self->monsterinfo.aiflags |= AI_NO_CIRCLE_STRAFE;
	self->monsterinfo.currentmove = &supertank_move_stand;
	self->monsterinfo.sight = supertank_sight;

	self->nextthink = level.time + FRAMETIME;
	gi.linkentity (self);

	if (!janitor && !invasion->value && (!self->activator || !self->activator->client))
		G_PrintGreenText(va("A level %d %s has spawned!", self->monsterinfo.level, boss5 ? "super tank heat" : "super tank"));
}

void init_drone_boss5(edict_t *self)
{
	self->mtype = M_BOSS5;
	init_drone_supertank(self);
}
