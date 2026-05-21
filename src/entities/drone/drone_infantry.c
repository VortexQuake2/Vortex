/*
==============================================================================

INFANTRY

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/m_infantry.h"


void InfantryMachineGun (edict_t *self);


static int	sound_pain1;
static int	sound_pain2;
static int	sound_die1;
static int	sound_die2;

static int	sound_gunshot;
static int	sound_weapon_cock;
static int	sound_weapon_cock_hb;
static int	sound_punch_swing;
static int	sound_punch_hit;
static int	sound_sight;
static int	sound_search;
static int	sound_idle;
static int	sound_grenade_pin;

#define INFANTRY_RUN_ATTACK_MIN_DIST 256
#define INFANTRY_MELEE_RANGE 64
#define INFANTRY_GRENADE_TIMER 2.5f
#define INFANTRY_GRENADE_DAMAGE_RADIUS 150.0f
#define INFANTRY_GRENADE_RADIUS_DAMAGE 100
#define INFANTRY_DEATH_GRENADE_FUSE 1.0f
#define INFANTRY_DEATH_GRENADE_MIN_FUSE 0.2f
#define INFANTRY_DEATH_GRENADE_DROP_CHANCE 0.45f
#define ENFORCER_SCALE 1.15f
#define ENFORCER_BLASTER_FLASH MZ2_MEDIC_HYPERBLASTER1_5
#define INFANTRY_MINS_X -16.0f
#define INFANTRY_MINS_Y -16.0f
#define INFANTRY_MINS_Z -24.0f
#define INFANTRY_MAXS_X 16.0f
#define INFANTRY_MAXS_Y 16.0f
#define INFANTRY_MAXS_Z 32.0f

static void infantry_fire(edict_t *self);
static void infantry_run_fire(edict_t *self);
static void infantry_grenade(edict_t *self);
extern mmove_t infantry_move_attack1;
extern mmove_t infantry_move_attack3;
extern mmove_t infantry_move_attack5;
extern mmove_t infantry_move_attack4;
extern mmove_t infantry_move_grenade_prep;
extern mmove_t infantry_move_grenade_throw;

static qboolean infantry_is_enforcer(edict_t *self)
{
	return self->mtype == M_ENFORCER;
}

static void infantry_set_bbox(edict_t *self, qboolean enforcer)
{
	const float scale = enforcer ? ENFORCER_SCALE : 1.0f;

	VectorSet(self->mins,
		INFANTRY_MINS_X * scale,
		INFANTRY_MINS_Y * scale,
		INFANTRY_MINS_Z * scale);
	VectorSet(self->maxs,
		INFANTRY_MAXS_X * scale,
		INFANTRY_MAXS_Y * scale,
		INFANTRY_MAXS_Z * scale);
}

static void infantry_project_flash(edict_t *self, int flash_number, vec3_t forward, vec3_t start)
{
	vec3_t right, offset;

	AngleVectors(self->s.angles, forward, right, NULL);
	VectorCopy(monster_flash_offset[flash_number], offset);
	if (self->s.scale)
		VectorScale(offset, self->s.scale, offset);
	G_ProjectSource(self->s.origin, offset, forward, right, start);
}

static void infantry_project_offset(edict_t *self, vec3_t offset, vec3_t forward, vec3_t start)
{
	vec3_t right, scaled_offset;

	AngleVectors(self->s.angles, forward, right, NULL);
	VectorCopy(offset, scaled_offset);
	if (self->s.scale)
		VectorScale(scaled_offset, self->s.scale, scaled_offset);
	G_ProjectSource(self->s.origin, scaled_offset, forward, right, start);
}

static void enforcer_project_blaster(edict_t *self, vec3_t forward, vec3_t start)
{
	vec3_t offset = { 26.6f, 6.1f, 10.1f };

	infantry_project_offset(self, offset, forward, start);
}

static int infantry_20mm_flash_for_frame(edict_t *self)
{
	if (self->s.frame >= FRAME_run201 && self->s.frame <= FRAME_run208)
		return MZ2_INFANTRY_MACHINEGUN_14 + (self->s.frame - FRAME_run201);

	switch (self->s.frame)
	{
	case FRAME_attak103:
	case FRAME_attak111:
	case FRAME_attak311:
		return MZ2_INFANTRY_MACHINEGUN_1;
	case FRAME_attak416:
		return MZ2_INFANTRY_MACHINEGUN_22;
	default:
		break;
	}

	if (self->s.frame >= FRAME_death211 && self->s.frame <= FRAME_death222)
		return MZ2_INFANTRY_MACHINEGUN_2 + (self->s.frame - FRAME_death211);

	return MZ2_INFANTRY_MACHINEGUN_1;
}

static qboolean infantry_20mm_aims_direct(edict_t *self)
{
	return (self->s.frame >= FRAME_run201 && self->s.frame <= FRAME_run208) ||
		self->s.frame == FRAME_attak103 ||
		self->s.frame == FRAME_attak111 ||
		self->s.frame == FRAME_attak311 ||
		self->s.frame == FRAME_attak416;
}

static int infantry_20mm_range(edict_t *self)
{
	int range = M_20MM_RANGE_BASE + M_20MM_RANGE_ADDON * drone_damagelevel(self);

	if (M_20MM_RANGE_MAX && range > M_20MM_RANGE_MAX)
		range = M_20MM_RANGE_MAX;

	return range;
}

static int enforcer_blaster2_damage(edict_t *self)
{
	int damage = M_BLASTER2_DMG_BASE + M_BLASTER2_DMG_ADDON * drone_damagelevel(self);

	if (M_BLASTER2_DMG_MAX && damage > M_BLASTER2_DMG_MAX)
		damage = M_BLASTER2_DMG_MAX;

	return damage;
}

static int enforcer_blaster2_speed(edict_t *self)
{
	int speed = M_BLASTER2_SPEED_BASE + M_BLASTER2_SPEED_ADDON * drone_damagelevel(self);

	if (M_BLASTER2_SPEED_MAX && speed > M_BLASTER2_SPEED_MAX)
		speed = M_BLASTER2_SPEED_MAX;

	return speed;
}

static qboolean infantry_has_clear_ranged_shot(edict_t *self)
{
	vec3_t forward, start;

	if (!G_ValidTarget(self, self->enemy, true, true))
		return false;
	if (entdist(self, self->enemy) > infantry_20mm_range(self))
		return false;

	if (infantry_is_enforcer(self))
	{
		enforcer_project_blaster(self, forward, start);
		return M_MonsterHasClearShotFrom(self, start);
	}

	return M_MonsterHasClearShotFromFlash(self, MZ2_INFANTRY_MACHINEGUN_1);
}

mframe_t infantry_frames_stand [] =
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
	drone_ai_stand, 0, NULL
};
mmove_t infantry_move_stand = {FRAME_stand50, FRAME_stand71, infantry_frames_stand, NULL};

void infantry_stand (edict_t *self)
{
	self->monsterinfo.currentmove = &infantry_move_stand;
}


mframe_t infantry_frames_fidget [] =
{
	drone_ai_stand, 1,  NULL,
	drone_ai_stand, 0,  NULL,
	drone_ai_stand, 1,  NULL,
	drone_ai_stand, 3,  NULL,
	drone_ai_stand, 6,  NULL,
	drone_ai_stand, 3,  NULL,
	drone_ai_stand, 0,  NULL,
	drone_ai_stand, 0,  NULL,
	drone_ai_stand, 0,  NULL,
	drone_ai_stand, 0,  NULL,
	drone_ai_stand, 1,  NULL,
	drone_ai_stand, 0,  NULL,
	drone_ai_stand, 0,  NULL,
	drone_ai_stand, 0,  NULL,
	drone_ai_stand, 0,  NULL,
	drone_ai_stand, 1,  NULL,
	drone_ai_stand, 0,  NULL,
	drone_ai_stand, -1, NULL,
	drone_ai_stand, 0,  NULL,
	drone_ai_stand, 0,  NULL,
	drone_ai_stand, 1,  NULL,
	drone_ai_stand, 0,  NULL,
	drone_ai_stand, -2, NULL,
	drone_ai_stand, 1,  NULL,
	drone_ai_stand, 1,  NULL,
	drone_ai_stand, 1,  NULL,
	drone_ai_stand, -1, NULL,
	drone_ai_stand, 0,  NULL,
	drone_ai_stand, 0,  NULL,
	drone_ai_stand, -1, NULL,
	drone_ai_stand, 0,  NULL,
	drone_ai_stand, 0,  NULL,
	drone_ai_stand, 0,  NULL,
	drone_ai_stand, 0,  NULL,
	drone_ai_stand, 0,  NULL,
	drone_ai_stand, -1, NULL,
	drone_ai_stand, 0,  NULL,
	drone_ai_stand, 0,  NULL,
	drone_ai_stand, 1,  NULL,
	drone_ai_stand, 0,  NULL,
	drone_ai_stand, 0,  NULL,
	drone_ai_stand, -1, NULL,
	drone_ai_stand, -1, NULL,
	drone_ai_stand, 0,  NULL,
	drone_ai_stand, -3, NULL,
	drone_ai_stand, -2, NULL,
	drone_ai_stand, -3, NULL,
	drone_ai_stand, -3, NULL,
	drone_ai_stand, -2, NULL
};
mmove_t infantry_move_fidget = {FRAME_stand01, FRAME_stand49, infantry_frames_fidget, infantry_stand};

void infantry_fidget (edict_t *self)
{
	self->monsterinfo.currentmove = &infantry_move_fidget;
	gi.sound (self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

mframe_t infantry_frames_walk [] =
{
	drone_ai_walk, 5,  NULL,
	drone_ai_walk, 4,  NULL,
	drone_ai_walk, 4,  NULL,
	drone_ai_walk, 5,  NULL,
	drone_ai_walk, 4,  NULL,
	drone_ai_walk, 5,  NULL,
	drone_ai_walk, 6,  NULL,
	drone_ai_walk, 4,  NULL,
	drone_ai_walk, 4,  NULL,
	drone_ai_walk, 4,  NULL,
	drone_ai_walk, 4,  NULL,
	drone_ai_walk, 5,  NULL
};
mmove_t infantry_move_walk = {FRAME_walk03, FRAME_walk14, infantry_frames_walk, NULL};

void infantry_walk (edict_t *self)
{
	if (!self->goalentity)
		self->goalentity = world;

	self->monsterinfo.currentmove = &infantry_move_walk;
}

mframe_t infantry_frames_run [] =
{
	drone_ai_run, 25, NULL,
	drone_ai_run, 25, NULL,
	drone_ai_run, 25,  NULL,
	drone_ai_run, 25,  NULL,
	drone_ai_run, 25, NULL,
	drone_ai_run, 25, NULL,
	drone_ai_run, 25,  NULL,
	drone_ai_run, 25,  NULL
};
mmove_t infantry_move_run = {FRAME_run01, FRAME_run08, infantry_frames_run, NULL};

void infantry_run (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &infantry_move_stand;
	else
		self->monsterinfo.currentmove = &infantry_move_run;
}

vec3_t	aimangles[] =
{
	0.0, 5.0, 0.0,
	10.0, 15.0, 0.0,
	20.0, 25.0, 0.0,
	25.0, 35.0, 0.0,
	30.0, 40.0, 0.0,
	30.0, 45.0, 0.0,
	25.0, 50.0, 0.0,
	20.0, 40.0, 0.0,
	15.0, 35.0, 0.0,
	40.0, 35.0, 0.0,
	70.0, 35.0, 0.0,
	90.0, 35.0, 0.0
};

void InfantryMachineGun (edict_t *self)
{
	vec3_t	start, forward, right, vec;
	int		damage, flash_number;

	damage = 10 + 1* drone_damagelevel(self); // dmg: infantry_machinegun

	if (self->s.frame == FRAME_attak111)
	{
		flash_number = MZ2_INFANTRY_MACHINEGUN_1;
		infantry_project_flash(self, flash_number, forward, start);
		MonsterAim(self, 0.8, 0, false, -1, forward, start);
	}
	else
	{
		flash_number = MZ2_INFANTRY_MACHINEGUN_2 + (self->s.frame - FRAME_death211);

		infantry_project_flash(self, flash_number, forward, start);

		VectorSubtract (self->s.angles, aimangles[flash_number-MZ2_INFANTRY_MACHINEGUN_2], vec);
		AngleVectors (vec, forward, NULL, NULL);
	}

	monster_fire_bullet (self, start, forward, damage, damage, 
		DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flash_number);
}

void Infantry20mm(edict_t* self)
{
	vec3_t	start, forward, vec;
	int		damage, flash_number;
	const float range = infantry_20mm_range(self);

	damage = M_20MM_DMG_BASE + M_20MM_DMG_ADDON * drone_damagelevel(self);
	if (M_20MM_DMG_MAX && damage > M_20MM_DMG_MAX)
		damage = M_20MM_DMG_MAX;

	flash_number = infantry_20mm_flash_for_frame(self);

	if (infantry_20mm_aims_direct(self))
	{
		infantry_project_flash(self, flash_number, forward, start);
		MonsterAim(self, M_HITSCAN_CONT_ACC, 0, false, -1, forward, start);
	}
	else
	{
		infantry_project_flash(self, flash_number, forward, start);

		VectorSubtract(self->s.angles, aimangles[flash_number - MZ2_INFANTRY_MACHINEGUN_2], vec);
		AngleVectors(vec, forward, NULL, NULL);
	}

	monster_fire_20mm(self, start, forward, damage, damage, range, flash_number);
}

static void EnforcerBlaster2(edict_t *self)
{
	vec3_t	start, forward, vec;
	int damage = enforcer_blaster2_damage(self);
	int speed = enforcer_blaster2_speed(self);

	if (self->health <= 0 && self->s.frame >= FRAME_death211 && self->s.frame <= FRAME_death222)
	{
		const int flash_number = infantry_20mm_flash_for_frame(self);

		infantry_project_flash(self, flash_number, forward, start);
		VectorSubtract(self->s.angles, aimangles[flash_number - MZ2_INFANTRY_MACHINEGUN_2], vec);
		AngleVectors(vec, forward, NULL, NULL);
	}
	else
	{
		if (!G_ValidTarget(self, self->enemy, true, true))
			return;

		enforcer_project_blaster(self, forward, start);
		MonsterAim(self, M_PROJECTILE_ACC, speed, false, -1, forward, start);
		if (!M_MonsterHasClearShotFrom(self, start))
			return;
	}

	monster_fire_blaster2(self, start, forward, damage, speed, EF_BLASTER, ENFORCER_BLASTER_FLASH);
}

static void infantry_fire_ranged(edict_t *self)
{
	if (infantry_is_enforcer(self))
		EnforcerBlaster2(self);
	else
		Infantry20mm(self);
}

void infantry_sight (edict_t *self, edict_t *other)
{
	if (random() < 0.5)
		gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
	else
		gi.sound (self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

void infantry_dead (edict_t *self)
{
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity (self);
	M_PrepBodyRemoval(self);
}

static void infantry_shrink(edict_t *self)
{
	self->maxs[2] = 0;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

mframe_t infantry_frames_pain1[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
};
mmove_t infantry_move_pain1 = { FRAME_pain101, FRAME_pain110, infantry_frames_pain1, infantry_run };

mframe_t infantry_frames_pain2[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
};
mmove_t infantry_move_pain2 = { FRAME_pain201, FRAME_pain210, infantry_frames_pain2, infantry_run };

void infantry_pain(edict_t* self, edict_t* other, float kick, int damage)
{
	const double rng = random();
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	// we're already in a pain state
	if (self->monsterinfo.currentmove == &infantry_move_pain2 ||
		self->monsterinfo.currentmove == &infantry_move_pain1)
		return;

	// monster players don't get pain state induced
	if (G_GetClient(self))
		return;

	// no pain in invasion hard mode
	if (invasion->value == 2)
		return;

	// if we're fidgeting, always go into pain state.
	if (rng <= (1.0f - self->monsterinfo.pain_chance) &&
		self->monsterinfo.currentmove != &infantry_move_stand &&
		self->monsterinfo.currentmove != &infantry_move_walk)
		return;

	if (random() < 0.5)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else {
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	}

	if (random() < 0.5)
		self->monsterinfo.currentmove = &infantry_move_pain1;
	else
		self->monsterinfo.currentmove = &infantry_move_pain2;
}

mframe_t infantry_frames_death1 [] =
{
	ai_move, -4, NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, -1, NULL,
	ai_move, -4, NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, -1, NULL,
	ai_move, 3,  NULL,
	ai_move, 1,  NULL,
	ai_move, 1,  NULL,
	ai_move, -2, NULL,
	ai_move, 2,  NULL,
	ai_move, 2,  NULL,
	ai_move, 9,  infantry_shrink,
	ai_move, 9,  NULL,
	ai_move, 5,  NULL,
	ai_move, -3, NULL,
	ai_move, -3, NULL
};
mmove_t infantry_move_death1 = {FRAME_death101, FRAME_death120, infantry_frames_death1, infantry_dead};

// Off with his head
mframe_t infantry_frames_death2 [] =
{
	ai_move, 0,   NULL,
	ai_move, 1,   NULL,
	ai_move, 5,   NULL,
	ai_move, -1,  NULL,
	ai_move, 0,   NULL,
	ai_move, 1,   NULL,
	ai_move, 1,   NULL,
	ai_move, 4,   NULL,
	ai_move, 3,   NULL,
	ai_move, 0,   NULL,
	ai_move, -2,  infantry_fire_ranged,
	ai_move, -2,  infantry_fire_ranged,
	ai_move, -3,  infantry_fire_ranged,
	ai_move, -1,  infantry_fire_ranged,
	ai_move, -2,  infantry_fire_ranged,
	ai_move, 0,   infantry_fire_ranged,
	ai_move, 2,   infantry_fire_ranged,
	ai_move, 2,   infantry_fire_ranged,
	ai_move, 3,   infantry_fire_ranged,
	ai_move, -10, infantry_fire_ranged,
	ai_move, -7,  infantry_fire_ranged,
	ai_move, -8,  infantry_fire_ranged,
	ai_move, -6,  infantry_shrink,
	ai_move, 4,   NULL,
	ai_move, 0,   NULL
};
mmove_t infantry_move_death2 = {FRAME_death201, FRAME_death225, infantry_frames_death2, infantry_dead};

mframe_t infantry_frames_death3 [] =
{
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   infantry_shrink,
	ai_move, -6,  NULL,
	ai_move, -11, NULL,
	ai_move, -3,  NULL,
	ai_move, -11, NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL
};
mmove_t infantry_move_death3 = {FRAME_death301, FRAME_death309, infantry_frames_death3, infantry_dead};

static int infantry_grenade_damage(edict_t *self)
{
	int damage = M_GRENADELAUNCHER_DMG_BASE + M_GRENADELAUNCHER_DMG_ADDON * drone_damagelevel(self);

	if (M_GRENADELAUNCHER_DMG_MAX && damage > M_GRENADELAUNCHER_DMG_MAX)
		damage = M_GRENADELAUNCHER_DMG_MAX;

	return damage;
}

static int infantry_grenade_speed(edict_t *self)
{
	int speed = M_GRENADELAUNCHER_SPEED_BASE + M_GRENADELAUNCHER_SPEED_ADDON * drone_damagelevel(self);

	if (M_GRENADELAUNCHER_SPEED_MAX && speed > M_GRENADELAUNCHER_SPEED_MAX)
		speed = M_GRENADELAUNCHER_SPEED_MAX;

	return speed;
}

static qboolean infantry_prethrow_grenade_active(edict_t *self)
{
	if (self->monsterinfo.currentmove == &infantry_move_grenade_prep)
		return true;

	return self->monsterinfo.currentmove == &infantry_move_grenade_throw &&
		self->s.frame < FRAME_attak206;
}

static float infantry_remaining_grenade_fuse(edict_t *self)
{
	float elapsed;
	float remaining;

	if (self->timestamp <= 0)
		return INFANTRY_DEATH_GRENADE_FUSE;

	elapsed = level.time - self->timestamp;
	if (elapsed < 0)
		elapsed = 0;

	remaining = INFANTRY_DEATH_GRENADE_FUSE - elapsed;
	if (remaining < INFANTRY_DEATH_GRENADE_MIN_FUSE)
		return INFANTRY_DEATH_GRENADE_MIN_FUSE;
	if (remaining > INFANTRY_DEATH_GRENADE_FUSE)
		return INFANTRY_DEATH_GRENADE_FUSE;

	return remaining;
}

static void infantry_death_grenade(edict_t *self, float fuse)
{
	vec3_t forward, right, up;
	vec3_t start, aimdir;

	AngleVectors(self->s.angles, forward, right, up);
	VectorMA(self->s.origin, 6, forward, start);
	VectorMA(start, 2, up, start);

	VectorScale(forward, 0.02f + random() * 0.12f, aimdir);
	VectorMA(aimdir, crandom() * 0.35f, right, aimdir);
	VectorMA(aimdir, -(0.95f + random() * 0.40f), up, aimdir);
	if (VectorNormalize(aimdir) == 0)
		VectorSet(aimdir, 0, 0, -1);

	fire_grenade2(self, start, aimdir, infantry_grenade_damage(self), 95, fuse,
		INFANTRY_GRENADE_DAMAGE_RADIUS, INFANTRY_GRENADE_RADIUS_DAMAGE, true);
}

static void infantry_delayed_grenade_explode(edict_t *timer)
{
	edict_t *corpse = timer->owner;
	int damage;

	if (!corpse || !corpse->inuse)
	{
		G_FreeEdict(timer);
		return;
	}

	damage = infantry_grenade_damage(corpse);
	T_RadiusDamage(corpse, corpse, damage, corpse, INFANTRY_GRENADE_DAMAGE_RADIUS, MOD_HG_SPLASH);
	vrx_throw_drone_gibs(corpse, damage);
	BecomeExplosion1(corpse);
	G_FreeEdict(timer);
}

static void infantry_schedule_delayed_grenade_explode(edict_t *self)
{
	edict_t *timer = G_Spawn();

	if (!timer)
	{
		infantry_death_grenade(self, infantry_remaining_grenade_fuse(self));
		return;
	}

	timer->classname = "infantry_delayed_grenade";
	timer->owner = self;
	timer->movetype = MOVETYPE_NONE;
	timer->solid = SOLID_NOT;
	timer->svflags |= SVF_NOCLIENT;
	VectorCopy(self->s.origin, timer->s.origin);
	timer->nextthink = level.time + infantry_remaining_grenade_fuse(self);
	timer->think = infantry_delayed_grenade_explode;
	gi.linkentity(timer);
}

void infantry_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int		n;
	edict_t *head;
	vec3_t	head_dir;
	qboolean grenade_in_hand;
	qboolean grenade_pin_pulled;

	M_Notify(self);

#ifdef OLD_NOLAG_STYLE
	if (nolag->value)
	{
		M_Remove(self, false, true);
		return;
	}
#endif

// check for gib
	if (self->health <= self->gib_health)
	{
		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		vrx_throw_drone_gibs(self, damage);

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

// regular death
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	vrx_update_drone_death_skin(self);

	grenade_in_hand = infantry_prethrow_grenade_active(self);
	grenade_pin_pulled = self->timestamp > 0;
	if (grenade_in_hand && grenade_pin_pulled)
	{
		if (random() <= INFANTRY_DEATH_GRENADE_DROP_CHANCE)
			infantry_death_grenade(self, infantry_remaining_grenade_fuse(self));
		else
			infantry_schedule_delayed_grenade_explode(self);
	}
	else if (grenade_in_hand)
		infantry_death_grenade(self, INFANTRY_DEATH_GRENADE_FUSE);
	self->timestamp = 0;

	n = randomMT() % 3;
	if (n == 0)
	{
		self->monsterinfo.currentmove = &infantry_move_death1;
		gi.sound (self, CHAN_VOICE, sound_die2, 1, ATTN_NORM, 0);
	}
	else if (n == 1)
	{
		self->monsterinfo.currentmove = &infantry_move_death2;
		gi.sound (self, CHAN_VOICE, sound_die1, 1, ATTN_NORM, 0);
	}
	else
	{
		self->monsterinfo.currentmove = &infantry_move_death3;
		gi.sound (self, CHAN_VOICE, sound_die2, 1, ATTN_NORM, 0);
	}

	if (n != 2 && random() <= 0.25f && vrx_spawn_nonessential_ent(self->s.origin))
	{
		head = ThrowGibEx(self, "models/monsters/infantry/gibs/head.md2", damage, GIB_ORGANIC,
			self->s.scale ? self->s.scale : 1.0f);
		if (head)
		{
			VectorCopy(self->s.angles, head->s.angles);
			VectorCopy(self->s.origin, head->s.origin);
			head->s.origin[2] += 32;
			if (inflictor)
				VectorSubtract(self->s.origin, inflictor->s.origin, head_dir);
			else
				VectorSet(head_dir, crandom(), crandom(), 0.5f);
			if (VectorNormalize(head_dir) == 0)
				VectorSet(head_dir, 0, 0, 1);
			VectorScale(head_dir, 100, head->velocity);
			head->velocity[2] = 200;
			VectorScale(head->avelocity, 0.15f, head->avelocity);
			head->s.skinnum = 0;
			gi.linkentity(head);
		}
	}

	if (self->activator && !self->activator->client)
	{
		self->activator->num_monsters_real--;
		// gi.bprintf(PRINT_HIGH, "releasing %p (%d)\n", self, self->activator->num_monsters_real);
	}

}


void infantry_duck_down (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;
	self->monsterinfo.aiflags |= AI_DUCKED;
	self->maxs[2] -= 32;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.pausetime = level.time + 1;
	gi.linkentity (self);
}

void infantry_duck_hold (edict_t *self)
{
	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

void infantry_duck_up (edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] += 32;
	self->takedamage = DAMAGE_AIM;
	gi.linkentity (self);
}

mframe_t infantry_frames_duck [] =
{
	ai_move, -2, infantry_duck_down,
	ai_move, -5, infantry_duck_hold,
	ai_move, 3,  NULL,
	ai_move, 4,  infantry_duck_up,
	ai_move, 0,  NULL
};
mmove_t infantry_move_duck = {FRAME_duck01, FRAME_duck05, infantry_frames_duck, infantry_run};

void infantry_cock_gun (edict_t *self)
{
	gi.sound (self, CHAN_WEAPON, infantry_is_enforcer(self) ? sound_weapon_cock_hb : sound_weapon_cock, 1, ATTN_NORM, 0);
	self->count = 1;
}

static qboolean infantry_should_continue_ranged(edict_t *self)
{
	if (random() > 0.9f)
		return false;

	return infantry_has_clear_ranged_shot(self);
}

static void infantry_fire(edict_t* self)
{
	mmove_t *move = self->monsterinfo.currentmove;

	if (!infantry_has_clear_ranged_shot(self))
		return;

	infantry_fire_ranged(self);
	self->count = 1;

	M_DelayNextAttack(self, 0, true);

	if (!infantry_should_continue_ranged(self))
		return;

	if (move == &infantry_move_attack1)
		self->monsterinfo.nextframe = FRAME_attak102;
	else if (move == &infantry_move_attack3 || move == &infantry_move_attack5)
	{
		self->monsterinfo.currentmove = &infantry_move_attack1;
		self->monsterinfo.nextframe = FRAME_attak102;
	}
}

static void infantry_ai_dodge_slide(edict_t *self, float dist);

static void infantry_ai_dodge_slide(edict_t *self, float dist)
{
	if (!G_EntIsAlive(self->enemy) && G_EntIsAlive(self->monsterinfo.attacker))
		self->enemy = self->monsterinfo.attacker;
	if (!G_EntIsAlive(self->enemy))
		return;

	drone_ai_dodge_slide(self, dist);
}

mframe_t infantry_frames_dodge_slide[] =
{
	infantry_ai_dodge_slide, 12, NULL,
	infantry_ai_dodge_slide, 10, NULL,
	infantry_ai_dodge_slide, 12, NULL,
	infantry_ai_dodge_slide, 10, NULL,
	infantry_ai_dodge_slide, 12, NULL,
	infantry_ai_dodge_slide, 10, NULL,
	infantry_ai_dodge_slide, 8,  NULL,
	infantry_ai_dodge_slide, 6,  NULL
};
mmove_t infantry_move_dodge_slide = { FRAME_run01, FRAME_run08, infantry_frames_dodge_slide, infantry_run };

static void infantry_attack4_dodge_ai(edict_t *self, float dist)
{
	if (!G_EntIsAlive(self->enemy))
		return;

	drone_ai_dodge_slide(self, dist);
}

static void infantry_resume_attack4(edict_t *self)
{
	self->monsterinfo.currentmove = &infantry_move_attack4;
	if (self->monsterinfo.nextattack >= FRAME_run201 && self->monsterinfo.nextattack <= FRAME_run208)
		self->s.frame = self->monsterinfo.nextattack;
	self->monsterinfo.nextattack = 0;
}

mframe_t infantry_frames_attack4_dodge[] =
{
	infantry_attack4_dodge_ai, 10, infantry_run_fire,
	infantry_attack4_dodge_ai, 12, NULL,
	infantry_attack4_dodge_ai, 10, infantry_run_fire,
	infantry_attack4_dodge_ai, 9,  NULL,
	infantry_attack4_dodge_ai, 8,  NULL
};
mmove_t infantry_move_attack4_dodge = { FRAME_run201, FRAME_run205, infantry_frames_attack4_dodge, infantry_resume_attack4 };

static qboolean infantry_try_sidestep(edict_t *self)
{
	if (self->monsterinfo.currentmove == &infantry_move_attack4)
	{
		self->monsterinfo.nextattack = self->s.frame;
		self->monsterinfo.currentmove = &infantry_move_attack4_dodge;
		return true;
	}

	if (self->monsterinfo.currentmove == &infantry_move_run)
	{
		self->monsterinfo.currentmove = &infantry_move_dodge_slide;
		return true;
	}

	return false;
}

static qboolean infantry_is_dodge_move(edict_t *self)
{
	return self->monsterinfo.currentmove == &infantry_move_dodge_slide ||
		self->monsterinfo.currentmove == &infantry_move_attack4_dodge ||
		self->monsterinfo.currentmove == &infantry_move_duck;
}

static qboolean infantry_dodge_hit_low(edict_t *self, vec3_t dir)
{
	const float duck_height = self->absmax[2] - 33;

	return dir[2] > self->absmin[2] && dir[2] <= duck_height;
}

static void infantry_start_duck(edict_t *self)
{
	self->monsterinfo.currentmove = &infantry_move_duck;
	infantry_duck_down(self);
	self->monsterinfo.dodge_time = level.time + 1.2f;
}

static void infantry_dodge(edict_t *self, edict_t *attacker, vec3_t dir, int radius)
{
	if (level.time < self->monsterinfo.dodge_time)
		return;
	if (!attacker)
		return;
	if (OnSameTeam(self, attacker))
		return;
	if (infantry_is_dodge_move(self))
		return;

	self->monsterinfo.attacker = attacker;
	if (!G_EntIsAlive(self->enemy) && G_EntIsAlive(attacker))
		self->enemy = attacker;

	if (random() > 0.75f)
		return;

	// Match remaster: sidestep low shots, duck high direct shots.
	if (!radius && !infantry_dodge_hit_low(self, dir))
	{
		infantry_start_duck(self);
		return;
	}

	drone_set_dodge_side(self, dir);

	if (infantry_try_sidestep(self))
	{
		self->monsterinfo.dodge_time = level.time + 1.0f;
		return;
	}

	if (!(self->monsterinfo.aiflags & AI_STAND_GROUND))
	{
		self->monsterinfo.currentmove = &infantry_move_dodge_slide;
		self->monsterinfo.dodge_time = level.time + 0.9f;
		return;
	}

	infantry_start_duck(self);
}

static void infantry_run_attack_ai(edict_t* self, float dist)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		return;
	if (!G_ValidTarget(self, self->enemy, true, true))
		return;

	ai_charge(self, 0);
	M_MoveToGoal(self, dist);
}

static void infantry_run_fire(edict_t* self)
{
	if (!infantry_has_clear_ranged_shot(self))
		return;

	infantry_fire_ranged(self);
	self->count = 1;

	M_DelayNextAttack(self, 0, true);
}

void infantry_attack4_refire(edict_t* self)
{
	if (level.time >= self->monsterinfo.pausetime)
	{
		self->monsterinfo.currentmove = &infantry_move_attack1;
		self->monsterinfo.nextframe = FRAME_attak114;
	}
	else if ((self->monsterinfo.aiflags & AI_STAND_GROUND)
		|| !G_ValidTarget(self, self->enemy, true, true)
		|| entdist(self, self->enemy) < INFANTRY_RUN_ATTACK_MIN_DIST)
	{
		self->monsterinfo.currentmove = &infantry_move_attack1;
		self->monsterinfo.nextframe = FRAME_attak102;
	}
	else
		self->monsterinfo.nextframe = FRAME_run201;

	infantry_run_fire(self);
}

mframe_t infantry_frames_attack4[] =
{
	infantry_run_attack_ai, 16, infantry_run_fire,
	infantry_run_attack_ai, 16, infantry_run_fire,
	infantry_run_attack_ai, 13, infantry_run_fire,
	infantry_run_attack_ai, 10, infantry_run_fire,
	infantry_run_attack_ai, 16, infantry_run_fire,
	infantry_run_attack_ai, 16, infantry_run_fire,
	infantry_run_attack_ai, 16, infantry_run_fire,
	infantry_run_attack_ai, 16, infantry_attack4_refire
};
mmove_t infantry_move_attack4 = { FRAME_run201, FRAME_run208, infantry_frames_attack4, infantry_run };

mframe_t infantry_frames_attack1 [] =
{
	ai_charge, 0,  NULL,
	ai_charge, 6,  NULL,
	ai_charge, 0,  infantry_fire,
	ai_charge, 0,  NULL,
	ai_charge, 1,  NULL,
	ai_charge, -7, NULL,
	ai_charge, -6, NULL,
	ai_charge, -1, NULL,
	ai_charge, 0,  infantry_cock_gun,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, -1, NULL,
	ai_charge, -1, NULL
};
mmove_t infantry_move_attack1 = {FRAME_attak101, FRAME_attak115, infantry_frames_attack1, infantry_run};

mframe_t infantry_frames_attack3 [] =
{
	ai_charge, 4,  NULL,
	ai_charge, -1, NULL,
	ai_charge, -1, NULL,
	ai_charge, 0,  infantry_cock_gun,
	ai_charge, -1, NULL,
	ai_charge, 1,  NULL,
	ai_charge, 1,  NULL,
	ai_charge, 2,  NULL,
	ai_charge, -2, NULL,
	ai_charge, -3, NULL,
	ai_charge, 1,  infantry_fire,
	ai_charge, 5,  NULL,
	ai_charge, -1, NULL,
	ai_charge, -2, NULL,
	ai_charge, -3, NULL
};
mmove_t infantry_move_attack3 = {FRAME_attak301, FRAME_attak315, infantry_frames_attack3, infantry_run};

mframe_t infantry_frames_attack5 [] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, infantry_cock_gun,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, infantry_fire,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t infantry_move_attack5 = {FRAME_attak401, FRAME_attak423, infantry_frames_attack5, infantry_run};


void infantry_swing (edict_t *self)
{
	gi.sound (self, CHAN_WEAPON, sound_punch_swing, 1, ATTN_NORM, 0);
}

void infantry_smack (edict_t *self)
{
	int		damage;

	if (!G_EntExists(self->enemy))
		return;

	damage = M_MELEE_DMG_BASE + M_MELEE_DMG_ADDON * drone_damagelevel(self); // dmg: infantry_smack
	if (M_MELEE_DMG_MAX && damage > M_MELEE_DMG_MAX)
		damage = M_MELEE_DMG_MAX;

	if (M_MeleeAttack(self, self->enemy, 96, damage, 200))
		gi.sound (self, CHAN_AUTO, sound_punch_hit, 1, ATTN_NORM, 0);
}

mframe_t infantry_frames_attack2 [] =
{
	ai_charge, 3, NULL,
	ai_charge, 6, NULL,
	ai_charge, 0, infantry_swing,
	ai_charge, 8, NULL,
	ai_charge, 5, NULL,
	ai_charge, 8, infantry_smack,
	ai_charge, 8, infantry_smack,
	ai_charge, 3, NULL,
};
mmove_t infantry_move_attack2 = {FRAME_attak201, FRAME_attak208, infantry_frames_attack2, infantry_run};

static void infantry_grenade(edict_t* self)
{
	vec3_t	start, forward;
	int speed;
	int damage;

	if (!G_ValidTarget(self, self->enemy, true, true))
	{
		self->timestamp = 0;
		return;
	}

	speed = infantry_grenade_speed(self);
	damage = infantry_grenade_damage(self);

	MonsterAim(self, 0.8f, speed, true, MZ2_INFANTRY_MACHINEGUN_1, forward, start);

	fire_grenade2(self, start, forward, damage, speed, INFANTRY_GRENADE_TIMER,
		INFANTRY_GRENADE_DAMAGE_RADIUS, INFANTRY_GRENADE_RADIUS_DAMAGE, false);

	self->timestamp = 0;
	M_DelayNextAttack(self, 0, true);
}

static void infantry_prep_grenade(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_grenade_pin, 1, ATTN_NORM, 0);
	self->timestamp = level.time;
	self->count = 1;
}

static void infantry_grenade_cleanup(edict_t *self)
{
	self->timestamp = 0;
	infantry_run(self);
}

static void infantry_continue_grenade_throw(edict_t *self)
{
	self->monsterinfo.currentmove = &infantry_move_grenade_throw;
}

mframe_t infantry_frames_grenade_prep[] =
{
	ai_charge, 0, infantry_prep_grenade,
	ai_charge, 1, NULL,
	ai_charge, 2, infantry_continue_grenade_throw
};
mmove_t infantry_move_grenade_prep = { FRAME_pain108, FRAME_pain110, infantry_frames_grenade_prep, NULL };

mframe_t infantry_frames_grenade_throw[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, infantry_grenade,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t infantry_move_grenade_throw = { FRAME_attak201, FRAME_attak208, infantry_frames_grenade_throw, infantry_grenade_cleanup };

void infantry_attack(edict_t* self)
{
	const int range = entdist(self, self->enemy);
	const int maxrange = infantry_20mm_range(self);

	if (range > maxrange)
		return;

	M_DelayNextAttack(self, 0, true);

	if (range <= INFANTRY_MELEE_RANGE)
	{
		self->monsterinfo.currentmove = &infantry_move_attack2;
		return;
	}

	if (random() <= 0.2f)
	{
		self->monsterinfo.currentmove = &infantry_move_grenade_prep;
		return;
	}

	if (!infantry_has_clear_ranged_shot(self))
		return;

	if (self->count && !(self->monsterinfo.aiflags & AI_STAND_GROUND) &&
		range >= INFANTRY_RUN_ATTACK_MIN_DIST)
	{
		self->monsterinfo.pausetime = level.time + 1.8 + random();
		self->monsterinfo.currentmove = &infantry_move_attack4;
		return;
	}

	if (self->count)
		self->monsterinfo.currentmove = &infantry_move_attack1;
	else
	{
		self->monsterinfo.currentmove = random() <= 0.1f ? &infantry_move_attack5 : &infantry_move_attack3;
		if (self->monsterinfo.currentmove == &infantry_move_attack5)
			self->monsterinfo.nextframe = FRAME_attak405;
	}
}

void infantry_melee(edict_t* self)
{

}

/*QUAKED monster_infantry (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
static void init_drone_infantry_common(edict_t* self, qboolean enforcer)
{
	gi.soundindex("weapons/sgun1.wav");
	gi.soundindex("weapons/grenlf1a.wav");
	gi.soundindex("weapons/hgrent1a.wav");
	gi.soundindex("weapons/hyprbf1a.wav");

	gi.modelindex("models/objects/shell1/tris.md2");
	gi.modelindex("models/objects/grenade2/tris.md2");

	sound_pain1 = gi.soundindex("infantry/infpain1.wav");
	sound_pain2 = gi.soundindex("infantry/infpain2.wav");
	sound_die1 = gi.soundindex("infantry/infdeth1.wav");
	sound_die2 = gi.soundindex("infantry/infdeth2.wav");

	sound_gunshot = gi.soundindex("infantry/infatck1.wav");
	sound_weapon_cock = gi.soundindex("infantry/infatck3.wav");
	sound_weapon_cock_hb = gi.soundindex("weapons/hyprbu1a.wav");
	sound_punch_swing = gi.soundindex("infantry/infatck2.wav");
	sound_punch_hit = gi.soundindex("infantry/melee2.wav");
	sound_grenade_pin = gi.soundindex("weapons/hgrenc1b.wav");

	sound_sight = gi.soundindex("infantry/infsght1.wav");
	sound_search = gi.soundindex("infantry/infsrch1.wav");
	sound_idle = gi.soundindex("infantry/infidle1.wav");

	self->s.modelindex = gi.modelindex("models/monsters/infantry/tris.md2");
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->mass = 400;

	if (enforcer)
		self->s.scale = ENFORCER_SCALE;
	self->mtype = enforcer ? M_ENFORCER : M_INFANTRY;
	infantry_set_bbox(self, enforcer);

	self->monsterinfo.control_cost = M_ENFORCER_CONTROL_COST;
	self->monsterinfo.cost = M_ENFORCER_COST;

	// set health
	self->health = M_ENFORCER_INITIAL_HEALTH + M_ENFORCER_ADDON_HEALTH * self->monsterinfo.level;
	self->max_health = self->health;
	self->gib_health = -1.5 * BASE_GIB_HEALTH;

	// set armor
	self->monsterinfo.power_armor_type = enforcer ? POWER_ARMOR_SCREEN : POWER_ARMOR_SHIELD;
	self->monsterinfo.power_armor_power = M_ENFORCER_INITIAL_ARMOR + M_ENFORCER_ADDON_ARMOR * self->monsterinfo.level;
	self->monsterinfo.max_armor = self->monsterinfo.power_armor_power;

	// jump and movement
	self->monsterinfo.jumpup = 64;
	self->monsterinfo.jumpdn = 512;
	self->monsterinfo.nextattack = 0;
	self->count = 0;
	self->timestamp = 0;
	self->monsterinfo.aiflags |= AI_NO_CIRCLE_STRAFE;

	self->item = FindItemByClassname(enforcer ? "ammo_cells" : "ammo_shells");

	// they're very sensitive to pain!
	self->monsterinfo.pain_chance = 0.3f;
	self->pain = infantry_pain;
	self->die = infantry_die;

	self->monsterinfo.stand = infantry_stand;
	self->monsterinfo.walk = infantry_walk;
	self->monsterinfo.run = infantry_run;
	self->monsterinfo.attack = infantry_attack;
	self->monsterinfo.sight = infantry_sight;
	//self->monsterinfo.idle = infantry_fidget;
	self->monsterinfo.melee = infantry_melee;
	self->monsterinfo.dodge = infantry_dodge;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &infantry_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;
}

void init_drone_infantry(edict_t* self)
{
	init_drone_infantry_common(self, false);
}

void init_drone_enforcer(edict_t* self)
{
	init_drone_infantry_common(self, true);
}
