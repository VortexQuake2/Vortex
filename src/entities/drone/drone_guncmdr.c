/*
==============================================================================

GUN COMMANDER

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/m_guncmdr.h"

static int sound_pain;
static int sound_pain2;
static int sound_death;
static int sound_idle;
static int sound_open;
static int sound_search;
static int sound_sight;
static int sound_thud;

#define GUNCMDR_GRENADE_RANGE       100.0f
#define GUNCMDR_MORTAR_RANGE        525.0f
#define GUNCMDR_CHAINGUN_RUN_RANGE  400.0f
#define GUNCMDR_MORTAR_SPEED        850
#define GUNCMDR_GRENADE_SPEED       600
#define GUNCMDR_WALK_SPEED_MULT     2.0f

static void guncmdr_stand(edict_t *self);
static void guncmdr_run(edict_t *self);
static void guncmdr_attack(edict_t *self);
static void guncmdr_fire_chain(edict_t *self);
static void guncmdr_refire_chain(edict_t *self);
static void guncmdr_grenade_finished(edict_t *self);
extern mmove_t guncmdr_move_fidget;

static void guncmdr_idle_sound(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

static void guncmdr_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static void guncmdr_search(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

static void guncmdr_fidget(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		return;
	if (self->enemy)
		return;
	if (random() <= 0.05)
		self->monsterinfo.currentmove = &guncmdr_move_fidget;
}

mframe_t guncmdr_frames_fidget[] =
{
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, guncmdr_idle_sound,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,

	drone_ai_stand, 0, guncmdr_idle_sound,
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
mmove_t guncmdr_move_fidget = { FRAME_c_stand201, FRAME_c_stand254, guncmdr_frames_fidget, guncmdr_stand };

mframe_t guncmdr_frames_stand[] =
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
	drone_ai_stand, 0, guncmdr_fidget,

	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, guncmdr_fidget,

	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, guncmdr_fidget,

	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, guncmdr_fidget
};
mmove_t guncmdr_move_stand = { FRAME_c_stand101, FRAME_c_stand140, guncmdr_frames_stand, NULL };

static void guncmdr_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &guncmdr_move_stand;
}

static void guncmdr_ai_walk(edict_t *self, float dist)
{
	drone_ai_walk(self, dist * GUNCMDR_WALK_SPEED_MULT);
}

mframe_t guncmdr_frames_walk[] =
{
	guncmdr_ai_walk, 1.5, NULL,
	guncmdr_ai_walk, 2.5, NULL,
	guncmdr_ai_walk, 3.0, NULL,
	guncmdr_ai_walk, 2.5, NULL,
	guncmdr_ai_walk, 2.3, NULL,
	guncmdr_ai_walk, 3.0, NULL,
	guncmdr_ai_walk, 2.8, NULL,
	guncmdr_ai_walk, 3.6, NULL,
	guncmdr_ai_walk, 2.8, NULL,
	guncmdr_ai_walk, 2.5, NULL,

	guncmdr_ai_walk, 2.3, NULL,
	guncmdr_ai_walk, 4.3, NULL,
	guncmdr_ai_walk, 3.0, NULL,
	guncmdr_ai_walk, 1.5, NULL,
	guncmdr_ai_walk, 2.5, NULL,
	guncmdr_ai_walk, 3.3, NULL,
	guncmdr_ai_walk, 2.8, NULL,
	guncmdr_ai_walk, 3.0, NULL,
	guncmdr_ai_walk, 2.0, NULL,
	guncmdr_ai_walk, 2.0, NULL,

	guncmdr_ai_walk, 3.3, NULL,
	guncmdr_ai_walk, 3.6, NULL,
	guncmdr_ai_walk, 3.4, NULL,
	guncmdr_ai_walk, 2.8, NULL
};
mmove_t guncmdr_move_walk = { FRAME_c_walk101, FRAME_c_walk124, guncmdr_frames_walk, NULL };

static void guncmdr_walk(edict_t *self)
{
	if (!self->goalentity)
		self->goalentity = world;
	self->monsterinfo.currentmove = &guncmdr_move_walk;
}

mframe_t guncmdr_frames_run[] =
{
	drone_ai_run, 15, NULL,
	drone_ai_run, 16, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 18, NULL,
	drone_ai_run, 24, NULL,
	drone_ai_run, 13.5, NULL
};
mmove_t guncmdr_move_run = { FRAME_c_run101, FRAME_c_run106, guncmdr_frames_run, NULL };

static void guncmdr_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &guncmdr_move_stand;
	else
		self->monsterinfo.currentmove = &guncmdr_move_run;
}

static void GunnerCmdrFire(edict_t *self)
{
	int damage;
	int flash_number;
	vec3_t forward, start;

	if (!self->enemy || !self->enemy->inuse)
		return;

	if (self->s.frame >= FRAME_c_run201 && self->s.frame <= FRAME_c_run206)
		flash_number = MZ2_GUNNER_MACHINEGUN_1 + (self->s.frame - FRAME_c_run201);
	else
		flash_number = MZ2_GUNNER_MACHINEGUN_1 + ((self->s.frame - FRAME_c_attack107) % 6);

	damage = M_MACHINEGUN_DMG_BASE + M_MACHINEGUN_DMG_ADDON * drone_damagelevel(self);
	if (M_MACHINEGUN_DMG_MAX && damage > M_MACHINEGUN_DMG_MAX)
		damage = M_MACHINEGUN_DMG_MAX;

	MonsterAim(self, M_HITSCAN_CONT_ACC, 0, false, flash_number, forward, start);
	monster_fire_bullet(self, start, forward, damage, damage,
		DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flash_number);
}

static void guncmdr_opengun(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_open, 1, ATTN_IDLE, 0);
}

mframe_t guncmdr_frames_attack_chain[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, guncmdr_opengun,
	ai_charge, 0, NULL
};
mmove_t guncmdr_move_attack_chain = { FRAME_c_attack101, FRAME_c_attack106, guncmdr_frames_attack_chain, guncmdr_fire_chain };

mframe_t guncmdr_frames_fire_chain[] =
{
	ai_charge, 0, GunnerCmdrFire,
	ai_charge, 0, GunnerCmdrFire,
	ai_charge, 0, GunnerCmdrFire,
	ai_charge, 0, GunnerCmdrFire,
	ai_charge, 0, GunnerCmdrFire,
	ai_charge, 0, GunnerCmdrFire
};
mmove_t guncmdr_move_fire_chain = { FRAME_c_attack107, FRAME_c_attack112, guncmdr_frames_fire_chain, guncmdr_refire_chain };

mframe_t guncmdr_frames_fire_chain_run[] =
{
	drone_ai_run, 15, GunnerCmdrFire,
	drone_ai_run, 16, GunnerCmdrFire,
	drone_ai_run, 20, GunnerCmdrFire,
	drone_ai_run, 18, GunnerCmdrFire,
	drone_ai_run, 24, GunnerCmdrFire,
	drone_ai_run, 13.5, GunnerCmdrFire
};
mmove_t guncmdr_move_fire_chain_run = { FRAME_c_run201, FRAME_c_run206, guncmdr_frames_fire_chain_run, guncmdr_refire_chain };

mframe_t guncmdr_frames_endfire_chain[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, guncmdr_opengun,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t guncmdr_move_endfire_chain = { FRAME_c_attack118, FRAME_c_attack124, guncmdr_frames_endfire_chain, guncmdr_run };

static int guncmdr_grenade_flash(edict_t *self)
{
	if (self->s.frame == FRAME_c_attack205 || self->s.frame == FRAME_c_attack304 || self->s.frame == FRAME_c_attack911)
		return MZ2_GUNNER_GRENADE_1;
	if (self->s.frame == FRAME_c_attack208 || self->s.frame == FRAME_c_attack307 || self->s.frame == FRAME_c_attack912)
		return MZ2_GUNNER_GRENADE_2;
	return MZ2_GUNNER_GRENADE_4;
}

static void GunnerCmdrGrenade(edict_t *self)
{
	int damage, speed, flash_number;
	vec3_t forward, start;

	if (!self->enemy || !self->enemy->inuse)
		return;

	damage = M_GRENADELAUNCHER_DMG_BASE + M_GRENADELAUNCHER_DMG_ADDON * drone_damagelevel(self);
	if (M_GRENADELAUNCHER_DMG_MAX && damage > M_GRENADELAUNCHER_DMG_MAX)
		damage = M_GRENADELAUNCHER_DMG_MAX;

	if (self->s.frame >= FRAME_c_attack201 && self->s.frame <= FRAME_c_attack221)
		speed = GUNCMDR_MORTAR_SPEED;
	else
		speed = GUNCMDR_GRENADE_SPEED + M_GRENADELAUNCHER_SPEED_ADDON * drone_damagelevel(self);

	if (M_GRENADELAUNCHER_SPEED_MAX && speed > M_GRENADELAUNCHER_SPEED_MAX)
		speed = M_GRENADELAUNCHER_SPEED_MAX;

	flash_number = guncmdr_grenade_flash(self);
	MonsterAim(self, M_PROJECTILE_ACC, speed, false, flash_number, forward, start);
	monster_fire_grenade(self, start, forward, damage, speed, flash_number);
}

mframe_t guncmdr_frames_attack_mortar[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, GunnerCmdrGrenade,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, GunnerCmdrGrenade,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,

	ai_charge, 0, GunnerCmdrGrenade,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t guncmdr_move_attack_mortar = { FRAME_c_attack201, FRAME_c_attack221, guncmdr_frames_attack_mortar, guncmdr_grenade_finished };

mframe_t guncmdr_frames_attack_back[] =
{
	ai_charge, -2, NULL,
	ai_charge, -1.5, NULL,
	ai_charge, -0.5, GunnerCmdrGrenade,
	ai_charge, -6.0, NULL,
	ai_charge, -4, NULL,
	ai_charge, -2.5, GunnerCmdrGrenade,
	ai_charge, -7.0, NULL,
	ai_charge, -3.5, NULL,
	ai_charge, -1.1, GunnerCmdrGrenade,

	ai_charge, -4.6, NULL,
	ai_charge, 1.9, NULL,
	ai_charge, 1.0, NULL,
	ai_charge, -4.5, NULL,
	ai_charge, 3.2, NULL,
	ai_charge, 4.4, NULL,
	ai_charge, -6.5, NULL,
	ai_charge, -6.1, NULL,
	ai_charge, 3.0, NULL,
	ai_charge, -0.7, NULL,
	ai_charge, -1.0, NULL
};
mmove_t guncmdr_move_attack_grenade_back = { FRAME_c_attack302, FRAME_c_attack321, guncmdr_frames_attack_back, guncmdr_grenade_finished };

static void guncmdr_kick(edict_t *self)
{
	int damage = M_MELEE_DMG_BASE + M_MELEE_DMG_ADDON * drone_damagelevel(self);
	vec3_t aim;

	if (M_MELEE_DMG_MAX && damage > M_MELEE_DMG_MAX)
		damage = M_MELEE_DMG_MAX;

	VectorSet(aim, MELEE_DISTANCE, 0, -32);
	fire_hit(self, aim, damage, 400);
}

static void guncmdr_kick_finished(edict_t *self)
{
	self->monsterinfo.melee_finished = level.time + 3.0;
	if (G_ValidTarget(self, self->enemy, true, true))
		guncmdr_attack(self);
	else
		guncmdr_run(self);
}

mframe_t guncmdr_frames_attack_kick[] =
{
	ai_charge, -7.7, NULL,
	ai_charge, -4.9, NULL,
	ai_charge, 12.6, guncmdr_kick,
	ai_charge, 0, NULL,
	ai_charge, -3.0, NULL,
	ai_charge, 0, NULL,
	ai_charge, -4.1, NULL,
	ai_charge, 8.6, NULL
};
mmove_t guncmdr_move_attack_kick = { FRAME_c_attack801, FRAME_c_attack808, guncmdr_frames_attack_kick, guncmdr_kick_finished };

static void guncmdr_duck_down(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;
	if (!self->groundentity)
		return;

	self->monsterinfo.aiflags |= AI_DUCKED;
	self->maxs[2] = 4;
	self->takedamage = DAMAGE_YES;
	gi.linkentity(self);
}

static void guncmdr_duck_up(edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] = 36;
	self->takedamage = DAMAGE_AIM;
	VectorClear(self->velocity);
	gi.linkentity(self);
}

mframe_t guncmdr_frames_duck_attack[] =
{
	ai_move, 3.6, NULL,
	ai_move, 5.6, guncmdr_duck_down,
	ai_move, 8.4, NULL,
	ai_move, 2.0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,

	ai_charge, 0, GunnerCmdrGrenade,
	ai_charge, 9.5, GunnerCmdrGrenade,
	ai_charge, -1.5, GunnerCmdrGrenade,
	ai_charge, 0, NULL,
	ai_charge, 0, guncmdr_duck_up,
	ai_charge, 0, NULL,
	ai_charge, 11, NULL,
	ai_charge, 2.0, NULL,
	ai_charge, 5.6, NULL
};
mmove_t guncmdr_move_duck_attack = { FRAME_c_attack901, FRAME_c_attack919, guncmdr_frames_duck_attack, guncmdr_run };

static void guncmdr_jump_now(edict_t *self)
{
	vec3_t forward;

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorMA(self->velocity, 150, forward, self->velocity);
	self->velocity[2] += 350;
}

static void guncmdr_jump_wait_land(edict_t *self)
{
	if (!self->groundentity && level.time < self->monsterinfo.pausetime)
		self->monsterinfo.nextframe = self->s.frame;
	else
		self->monsterinfo.nextframe = self->s.frame + 1;
}

mframe_t guncmdr_frames_jump[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, guncmdr_jump_now,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, guncmdr_jump_wait_land,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t guncmdr_move_jump = { FRAME_c_jump01, FRAME_c_jump10, guncmdr_frames_jump, guncmdr_run };

static void guncmdr_dodge(edict_t *self, edict_t *attacker, vec3_t dir, int radius)
{
	if (random() > 0.8)
		return;
	if (level.time < self->monsterinfo.dodge_time)
		return;
	if (OnSameTeam(self, attacker))
		return;

	if (!self->enemy && G_EntIsAlive(attacker))
		self->enemy = attacker;

	if (radius && self->groundentity)
	{
		self->monsterinfo.pausetime = level.time + 2.0;
		self->monsterinfo.currentmove = &guncmdr_move_jump;
		self->monsterinfo.dodge_time = level.time + 3.0;
	}
	else
	{
		self->monsterinfo.currentmove = &guncmdr_move_duck_attack;
		self->monsterinfo.dodge_time = level.time + 2.0;
	}
}

static void guncmdr_attack(edict_t *self)
{
	float dist;

	if (!G_ValidTarget(self, self->enemy, true, true))
		return;

	dist = entdist(self, self->enemy);
	if (dist <= MELEE_DISTANCE && self->monsterinfo.melee_finished < level.time)
		self->monsterinfo.currentmove = &guncmdr_move_attack_kick;
	else if (dist <= GUNCMDR_GRENADE_RANGE || random() < 0.45)
		self->monsterinfo.currentmove = &guncmdr_move_attack_chain;
	else if (dist >= GUNCMDR_MORTAR_RANGE || fabs(self->s.origin[2] - self->enemy->s.origin[2]) > 64)
		self->monsterinfo.currentmove = &guncmdr_move_attack_mortar;
	else
		self->monsterinfo.currentmove = &guncmdr_move_attack_grenade_back;

	M_DelayNextAttack(self, 0, true);
}

static void guncmdr_fire_chain(edict_t *self)
{
	if (!(self->monsterinfo.aiflags & AI_STAND_GROUND)
		&& G_ValidTarget(self, self->enemy, true, true)
		&& entdist(self, self->enemy) > GUNCMDR_CHAINGUN_RUN_RANGE)
		self->monsterinfo.currentmove = &guncmdr_move_fire_chain_run;
	else
		self->monsterinfo.currentmove = &guncmdr_move_fire_chain;
}

static void guncmdr_refire_chain(edict_t *self)
{
	if (G_ValidTarget(self, self->enemy, true, true) && visible(self, self->enemy) && random() <= 0.5)
	{
		if (!(self->monsterinfo.aiflags & AI_STAND_GROUND) && entdist(self, self->enemy) > GUNCMDR_CHAINGUN_RUN_RANGE)
			self->monsterinfo.currentmove = &guncmdr_move_fire_chain_run;
		else
			self->monsterinfo.currentmove = &guncmdr_move_fire_chain;
	}
	else
		self->monsterinfo.currentmove = &guncmdr_move_endfire_chain;

	self->monsterinfo.attack_finished = level.time + 0.5;
}

static void guncmdr_grenade_finished(edict_t *self)
{
	self->monsterinfo.attack_finished = level.time + 1.0;
	guncmdr_run(self);
}

mframe_t guncmdr_frames_pain1[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t guncmdr_move_pain1 = { FRAME_c_pain101, FRAME_c_pain104, guncmdr_frames_pain1, guncmdr_run };

mframe_t guncmdr_frames_pain2[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t guncmdr_move_pain2 = { FRAME_c_pain201, FRAME_c_pain204, guncmdr_frames_pain2, guncmdr_run };

mframe_t guncmdr_frames_pain3[] =
{
	ai_move, -3, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t guncmdr_move_pain3 = { FRAME_c_pain301, FRAME_c_pain304, guncmdr_frames_pain3, guncmdr_run };

static void guncmdr_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	float r;

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 3;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0;
	gi.sound(self, CHAN_VOICE, (random() < 0.5) ? sound_pain : sound_pain2, 1, ATTN_NORM, 0);

	if (skill->value == 3)
		return;

	r = random();
	if (r < 0.33)
		self->monsterinfo.currentmove = &guncmdr_move_pain1;
	else if (r < 0.66)
		self->monsterinfo.currentmove = &guncmdr_move_pain2;
	else
		self->monsterinfo.currentmove = &guncmdr_move_pain3;
}

static void guncmdr_dead(edict_t *self)
{
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
	M_PrepBodyRemoval(self);
}

static void guncmdr_shrink(edict_t *self)
{
	self->maxs[2] = -8;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

mframe_t guncmdr_frames_death1[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, -6, NULL,
	ai_move, -3, NULL,
	ai_move, -5, NULL,
	ai_move, 8, NULL,
	ai_move, 6, guncmdr_shrink,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t guncmdr_move_death1 = { FRAME_c_death101, FRAME_c_death118, guncmdr_frames_death1, guncmdr_dead };

static void guncmdr_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int n;

	M_Notify(self);

#ifdef OLD_NOLAG_STYLE
	if (nolag->value)
	{
		M_Remove(self, false, true);
		return;
	}
#endif

	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		if (vrx_spawn_nonessential_ent(self->s.origin))
		{
			for (n = 0; n < 2; n++)
				ThrowGib(self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
			for (n = 0; n < 4; n++)
				ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
			ThrowHead(self, "models/monsters/gunner/gibs/head.md2", damage, GIB_ORGANIC);
		}
#ifdef OLD_NOLAG_STYLE
		M_Remove(self, false, false);
#else
		if (nolag->value)
			M_Remove(self, false, true);
		else
			M_Remove(self, false, false);
#endif
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

	DroneList_Remove(self);

	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.currentmove = &guncmdr_move_death1;

	if (self->activator && !self->activator->client)
		self->activator->num_monsters_real--;
}

void init_drone_guncmdr(edict_t *self)
{
	sound_death = gi.soundindex("guncmdr/gcdrdeath1.wav");
	sound_pain = gi.soundindex("guncmdr/gcdrpain2.wav");
	sound_pain2 = gi.soundindex("guncmdr/gcdrpain1.wav");
	sound_idle = gi.soundindex("guncmdr/gcdridle1.wav");
	sound_open = gi.soundindex("guncmdr/gcdratck1.wav");
	sound_search = gi.soundindex("guncmdr/gcdrsrch1.wav");
	sound_sight = gi.soundindex("guncmdr/sight1.wav");
	sound_thud = gi.soundindex("player/land1.wav");

	gi.soundindex("guncmdr/gcdratck2.wav");
	gi.soundindex("guncmdr/gcdratck3.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/gunner/tris.md2");
	gi.modelindex("models/monsters/gunner/gibs/chest.md2");
	gi.modelindex("models/monsters/gunner/gibs/foot.md2");
	gi.modelindex("models/monsters/gunner/gibs/garm.md2");
	gi.modelindex("models/monsters/gunner/gibs/gun.md2");
	gi.modelindex("models/monsters/gunner/gibs/head.md2");

	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 36);
	self->s.skinnum = 2;

	self->monsterinfo.control_cost = M_TANK_CONTROL_COST;
	self->monsterinfo.cost = M_TANK_COST;
	self->health = M_TANK_INITIAL_HEALTH + M_TANK_ADDON_HEALTH * self->monsterinfo.level;
	self->max_health = self->health;
	self->gib_health = -175;
	self->mass = 255;
	self->monsterinfo.jumpdn = 512;
	self->monsterinfo.jumpup = 64;

	if (random() > 0.5)
		self->item = FindItemByClassname("ammo_bullets");
	else
		self->item = FindItemByClassname("ammo_grenades");

	self->pain = guncmdr_pain;
	self->die = guncmdr_die;

	self->monsterinfo.stand = guncmdr_stand;
	self->monsterinfo.walk = guncmdr_walk;
	self->monsterinfo.run = guncmdr_run;
	self->monsterinfo.dodge = guncmdr_dodge;
	self->monsterinfo.attack = guncmdr_attack;
	self->monsterinfo.sight = guncmdr_sight;
	self->monsterinfo.idle = guncmdr_idle_sound;
	self->monsterinfo.pain_chance = 0.2f;
	self->monsterinfo.aiflags |= AI_NO_CIRCLE_STRAFE;

	self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
	self->monsterinfo.power_armor_power = M_TANK_INITIAL_ARMOR + M_TANK_ADDON_ARMOR * self->monsterinfo.level;
	self->monsterinfo.max_armor = self->monsterinfo.power_armor_power;
	self->mtype = M_GUNCMDR;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &guncmdr_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;
	self->nextthink = level.time + FRAMETIME;
}
